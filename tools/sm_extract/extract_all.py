#!/usr/bin/env python3
"""MegaMetroid — extractor completo de recursos de Super Metroid (NTSC).

Recorre la ROM del usuario siguiendo los mismos punteros y estructuras que
usa el código del juego (snesrev/sm). No adivina límites de datos:
  * El inventario de estructuras sale de sm/assets/restool.py (visit_all).
  * Cada regla de carga está citada contra la función C correspondiente.

Uso:
  python tools/sm_extract/extract_all.py sm.smc data/extracted_full [--only rooms,samus,...]

Requiere Python 3.8+, Pillow y numpy.
"""
import argparse, hashlib, json, os, struct, sys, wave, zlib
from pathlib import Path

import numpy as np
from PIL import Image

ROM_SIZE = 0x300000
ROM_SHA256 = "12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72"

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent.parent
SM_ASSETS = PROJECT / "sm" / "assets"

# ---------------------------------------------------------------- ROM helpers
ROM = b""

def lorom(a):
    assert a & 0x8000, hex(a)
    return (((a >> 16) & 0x7f) << 15) | (a & 0x7fff)

def u8(a): return ROM[lorom(a)]
def u16(a):
    o = lorom(a); return ROM[o] | ROM[o + 1] << 8
def u24(a):
    o = lorom(a); return ROM[o] | ROM[o + 1] << 8 | ROM[o + 2] << 16
def rbytes(a, n):
    o = lorom(a); return ROM[o:o + n]

def decompress(ea):
    """Descompresor LZ de Super Metroid (idéntico a decode_common.decompress /
    DecompressToMem). Devuelve (datos, bytes_comprimidos_consumidos)."""
    data, ea0 = ROM, lorom(ea)
    p, rv = ea0, bytearray()
    while True:
        b = data[p]; p += 1
        if b == 0xff:
            return bytes(rv), p - ea0
        if (b & 0xe0) == 0xe0:
            cmd = (8 * b) & 0xe0
            size = ((b & 3) << 8 | data[p]) + 1; p += 1
        else:
            cmd = b & 0xe0
            size = (b & 0x1f) + 1
        if cmd & 0x80:
            x = 0xff if cmd & 0x20 else 0
            if cmd >= 0xc0:
                src = len(rv) - data[p]; p += 1
            else:
                src = data[p] | data[p + 1] << 8; p += 2
            for i in range(size):
                rv.append(rv[src + i] ^ x)
        elif cmd == 0x20:
            rv += bytes([data[p]]) * size; p += 1
        elif cmd == 0x40:
            b1, b2 = data[p], data[p + 1]; p += 2
            for i in range(size):
                rv.append(b1 if i % 2 == 0 else b2)
        elif cmd == 0x60:
            b1 = data[p]; p += 1
            for i in range(size):
                rv.append((b1 + i) & 0xff)
        else:
            rv += data[p:p + size]; p += size

# ------------------------------------------------------------ graphics helpers
def bgr555_to_rgb(v):
    r, g, b = v & 31, (v >> 5) & 31, (v >> 10) & 31
    return (r << 3 | r >> 2, g << 3 | g >> 2, b << 3 | b >> 2)

def palette_from_bytes(b):
    return [bgr555_to_rgb(b[i] | b[i + 1] << 8) for i in range(0, len(b) - 1, 2)]

def pal_words(b):
    return [b[i] | b[i + 1] << 8 for i in range(0, len(b) - 1, 2)]

def decode_tiles_4bpp(buf):
    """SNES 4bpp planar: filas P0/P1 en los primeros 16 bytes y P2/P3 en los
    16 siguientes. Devuelve array (n,8,8) de índices 0..15."""
    n = len(buf) // 32
    a = np.frombuffer(bytes(buf[:n * 32]), np.uint8).reshape(n, 2, 8, 2)
    bits = np.unpackbits(a, axis=3, bitorder="big").reshape(n, 2, 8, 2, 8)
    p0, p1 = bits[:, 0, :, 0], bits[:, 0, :, 1]
    p2, p3 = bits[:, 1, :, 0], bits[:, 1, :, 1]
    return (p0 | p1 << 1 | p2 << 2 | p3 << 3).astype(np.uint8)

def decode_tiles_2bpp(buf):
    n = len(buf) // 16
    a = np.frombuffer(bytes(buf[:n * 16]), np.uint8).reshape(n, 8, 2)
    bits = np.unpackbits(a, axis=2, bitorder="big").reshape(n, 8, 2, 8)
    return (bits[:, :, 0] | bits[:, :, 1] << 1).astype(np.uint8)

GRAY16 = [(i * 17, i * 17, i * 17) for i in range(16)]
GRAY4 = [(0, 0, 0), (96, 96, 96), (176, 176, 176), (255, 255, 255)]

def tiles_sheet(tiles, palette=None, cols=16):
    """Hoja de tiles indexada (PNG 'P'). Índice 0 = transparente."""
    n = len(tiles)
    rows = max(1, (n + cols - 1) // cols)
    out = np.zeros((rows * 8, cols * 8), np.uint8)
    for i in range(n):
        y, x = divmod(i, cols)
        out[y * 8:y * 8 + 8, x * 8:x * 8 + 8] = tiles[i]
    im = Image.fromarray(out, "P")
    pal = palette or (GRAY16 if tiles.max(initial=0) > 3 else GRAY4)
    flat = [c for rgb in pal for c in rgb]
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.info["transparency"] = 0
    return im

def palette_swatch(colors, per_row=16, cell=12):
    rows = (len(colors) + per_row - 1) // per_row
    im = Image.new("RGB", (per_row * cell, rows * cell), (0, 0, 0))
    px = im.load()
    for i, c in enumerate(colors):
        y0, x0 = (i // per_row) * cell, (i % per_row) * cell
        for y in range(cell):
            for x in range(cell):
                px[x0 + x, y0 + y] = c
    return im

def write_json(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, ensure_ascii=False), encoding="utf-8")

def hexa(a): return f"${a:06X}"

# ---------------------------------------------------------- restool inventory
def load_restool():
    """Importa restool.py y ejecuta visit_all(): inventario verificado."""
    cwd = os.getcwd()
    os.chdir(SM_ASSETS)
    sys.path.insert(0, str(SM_ASSETS))
    old_argv = sys.argv
    sys.argv = ["restool.py", "none"]
    try:
        import restool  # noqa
        restool.visit_all()
    finally:
        sys.argv = old_argv
        os.chdir(cwd)
    return restool

# ============================================================== 1. BLOBS
def extract_blobs(rt, out):
    """Todo bloque de datos que restool identifica como Blob/PackedBlob."""
    d = out / "blobs"
    manifest = []
    for ea, v in sorted(rt.g_locations.items()):
        if not isinstance(v, tuple) or len(v) != 2:
            continue
        tp = v[1]
        name_ = type(tp).__name__
        if name_ not in ("Blob", "PackedBlob") or not isinstance(v[0], tuple):
            continue
        label, raw = v[0]
        label = label or f"unk_{ea:06X}"
        fn = f"{ea:06X}_{label}"
        (d / "raw").mkdir(parents=True, exist_ok=True)
        (d / "raw" / f"{fn}.bin").write_bytes(raw)
        entry = {"snes": hexa(ea), "rom_offset": f"0x{lorom(ea):06X}", "name": label,
                 "kind": name_, "size": len(raw), "file": f"raw/{fn}.bin"}
        if name_ == "PackedBlob":
            dec, used = decompress(ea)
            assert used == len(raw), (label, used, len(raw))
            (d / "decompressed").mkdir(parents=True, exist_ok=True)
            (d / "decompressed" / f"{fn}.bin").write_bytes(dec)
            entry.update(decompressed_size=len(dec), decompressed_file=f"decompressed/{fn}.bin")
            data = dec
        else:
            data = raw
        # vista previa en gris para bloques con nombre de tiles
        if "Tiles" in label or "tiles" in label.lower():
            if len(data) >= 32:
                (d / "preview").mkdir(parents=True, exist_ok=True)
                tiles_sheet(decode_tiles_4bpp(data)).save(d / "preview" / f"{fn}_4bpp.png")
                entry["preview_4bpp"] = f"preview/{fn}_4bpp.png"
        manifest.append(entry)
    write_json(d / "manifest.json", manifest)
    return len(manifest)

# ============================================================ 2. TILESETS
# LoadStateHeader (0x82DEF2): TileSet = {tile_table_ptr, tiles_ptr, palette_ptr}
# kStateHeaderTileSets en $8F:E7A7, 29 entradas (restool visit_blobs).
# LoadCRETilesTilesetTilesAndPalette (0x82E783):
#   tiles del tileset -> VRAM byte 0x0000; CRE $B98000 -> VRAM byte 0x5000
#   paleta -> CGRAM (RAM $7EC200)
# LoadLevelDataAndOtherThings (0x82E7D3):
#   área != 6: CRE tile table $B9A09D -> $7EA000, tileset table -> $7EA800
#   área == 6 (Ceres): tileset table -> $7EA000
CRE_TILES, CRE_TABLE = 0xB98000, 0xB9A09D
TILESET_TABLE = 0x8FE7A7

def tileset_ptrs(i):
    ea = 0x8F0000 | u16(TILESET_TABLE + i * 2)
    return u24(ea), u24(ea + 3), u24(ea + 6)

_tileset_cache = {}
def build_tileset(i, ceres):
    key = (i, ceres)
    if key in _tileset_cache:
        return _tileset_cache[key]
    tt, tiles, pal = tileset_ptrs(i)
    vram = bytearray(0x10000)
    # Orden de LoadCRETilesTilesetTilesAndPalette: primero CRE en 0x5000,
    # luego el tileset en 0x0000 (si es grande, pisa parte del CRE).
    cre, _ = decompress(CRE_TILES)
    vram[0x5000:0x5000 + len(cre)] = cre
    t, _ = decompress(tiles)
    vram[0:len(t)] = t
    table = bytearray(0x2000)
    tdata, _ = decompress(tt)
    if ceres:
        table[0:len(tdata)] = tdata[:0x2000]
    else:
        ctab, _ = decompress(CRE_TABLE)
        table[0:len(ctab)] = ctab[:0x800]
        table[0x800:0x800 + len(tdata)] = tdata[:0x1800]
    pbytes, _ = decompress(pal)
    res = {
        "ptrs": {"tile_table": tt, "tiles": tiles, "palette": pal},
        "tiles_raw": t, "cre_raw": cre, "vram": bytes(vram),
        "table": bytes(table), "palette_bytes": pbytes,
        "palette": palette_from_bytes(pbytes)[:256],
        "tiles_idx": decode_tiles_4bpp(vram[:0x8000]),  # 1024 tiles
    }
    _tileset_cache[key] = res
    return res

def render_metatile_rgba(ts, idx, cache):
    """Metatile 16x16 desde la tile table: 4 entradas de tilemap SNES
    (TL,TR,BL,BR); bits 0-9 tile, 10-12 paleta, 14 hflip, 15 vflip."""
    if idx in cache:
        return cache[idx]
    pal = np.array(ts["palette"] + [(0, 0, 0)] * (256 - len(ts["palette"])), np.uint8)
    img = np.zeros((16, 16, 4), np.uint8)
    for q in range(4):
        w = struct.unpack_from("<H", ts["table"], idx * 8 + q * 2)[0]
        tile = ts["tiles_idx"][w & 0x3ff]
        if w & 0x4000: tile = tile[:, ::-1]
        if w & 0x8000: tile = tile[::-1, :]
        p = (w >> 10) & 7
        rgb = pal[p * 16 + tile]
        rgb[tile == 0] = 0                       # transparente = (0,0,0,0)
        a = np.where(tile == 0, 0, 255).astype(np.uint8)
        oy, ox = (q // 2) * 8, (q % 2) * 8
        img[oy:oy + 8, ox:ox + 8, :3] = rgb
        img[oy:oy + 8, ox:ox + 8, 3] = a
    cache[idx] = img
    return img

def extract_tilesets(out):
    d = out / "tilesets"
    man = []
    for i in range(29):
        for ceres in (False,):
            ts = build_tileset(i, ceres)
            sd = d / f"{i:02d}"
            sd.mkdir(parents=True, exist_ok=True)
            (sd / "tiles_tileset_4bpp.bin").write_bytes(ts["tiles_raw"])
            (sd / "palette_bgr555.bin").write_bytes(ts["palette_bytes"])
            (sd / "tile_table_combined.bin").write_bytes(ts["table"])
            (sd / "vram_bg_0000-7FFF.bin").write_bytes(ts["vram"][:0x8000])
            palette_swatch(ts["palette"][:128]).save(sd / "palette.png")
            for p in range(8):
                sub = ts["palette"][p * 16:p * 16 + 16]
                tiles_sheet(ts["tiles_idx"], sub, 32).save(sd / f"vram_tiles_pal{p}.png")
            cache = {}
            sheet = np.zeros((64 * 16, 16 * 16, 4), np.uint8)
            for m in range(1024):
                y, x = divmod(m, 16)
                sheet[y * 16:y * 16 + 16, x * 16:x * 16 + 16] = render_metatile_rgba(ts, m, cache)
            Image.fromarray(sheet, "RGBA").save(sd / "metatiles_1024.png")
            man.append({"tileset": i,
                        "tile_table": hexa(ts["ptrs"]["tile_table"]),
                        "tiles": hexa(ts["ptrs"]["tiles"]),
                        "palette": hexa(ts["ptrs"]["palette"]),
                        "cre_tiles": hexa(CRE_TILES), "cre_table": hexa(CRE_TABLE),
                        "notes": "metatiles 0x000-0x0FF = CRE, 0x100+ = tileset (áreas != Ceres)"})
    (d / "cre").mkdir(parents=True, exist_ok=True)
    cre, _ = decompress(CRE_TILES)
    (d / "cre" / "cre_tiles_4bpp.bin").write_bytes(cre)
    (d / "cre" / "cre_tile_table.bin").write_bytes(decompress(CRE_TABLE)[0])
    tiles_sheet(decode_tiles_4bpp(cre)).save(d / "cre" / "cre_tiles_gray.png")
    write_json(d / "manifest.json", man)
    return len(man)

# =============================================================== 3. ROOMS
STATE_SELECTS = {0xE5E6: "Default", 0xE612: "IsEventSet", 0xE629: "IsBossDead",
                 0xE669: "PowerBombs", 0xE652: "MorphBallMissiles", 0xE5FF: "TourianBoss01"}

def parse_room(ea):
    """Room header ($8F) + state selects, igual que Room.parse_actual_binary."""
    h = {"snes": hexa(ea), "roomnr": u8(ea), "area": u8(ea + 1), "x": u8(ea + 2),
         "y": u8(ea + 3), "w": u8(ea + 4), "h": u8(ea + 5), "up_scroll": u8(ea + 6),
         "down_scroll": u8(ea + 7), "cre_bitset": u8(ea + 8),
         "doorout_ptr": hexa(0x8F0000 | u16(ea + 9))}
    p, states = ea + 11, []
    while True:
        w = u16(p)
        kind = STATE_SELECTS[w]
        if w == 0xE5E6:
            states.append((kind, None, p + 2)); break
        if w in (0xE612, 0xE629):
            states.append((kind, u8(p + 2), 0x8F0000 | u16(p + 3))); p += 5
        else:
            states.append((kind, None, 0x8F0000 | u16(p + 2))); p += 4
    parsed = []
    for kind, arg, s in states:
        parsed.append({
            "condition": kind, "arg": arg, "state_snes": hexa(s),
            "level_data": u24(s), "gfx_set": u8(s + 3), "music": u8(s + 4),
            "music_ctrl": u8(s + 5), "layer3_fx": hexa(0x830000 | u16(s + 6)),
            "enemy_population": hexa(0xA10000 | u16(s + 8)),
            "enemy_tilesets": hexa(0xB40000 | u16(s + 10)),
            "nudge_limit": u16(s + 12), "scrolls": hexa(0x8F0000 | u16(s + 14)),
            "xray_special": u16(s + 16), "main_code": hexa(0x8F0000 | u16(s + 18)),
            "room_plm": hexa(0x8F0000 | u16(s + 20)), "bg_data": hexa(0x8F0000 | u16(s + 22)),
            "setup_code": hexa(0x8F0000 | u16(s + 24)),
        })
    h["states"] = parsed
    return h

def render_layer(words, wb, hb, ts):
    cache = {}
    img = np.zeros((hb * 16, wb * 16, 4), np.uint8)
    for by in range(hb):
        for bx in range(wb):
            w = words[by * wb + bx]
            mt = render_metatile_rgba(ts, w & 0x3ff, cache)
            if w & 0x400: mt = mt[:, ::-1]
            if w & 0x800: mt = mt[::-1, :]
            img[by * 16:by * 16 + 16, bx * 16:bx * 16 + 16] = mt
    return img

def extract_rooms(rt, out):
    d = out / "rooms"
    import consts
    rooms = sorted(ea for ea, v in rt.g_locations.items()
                   if isinstance(v, tuple) and len(v) == 2 and type(v[1]).__name__ == "Room")
    index, done_levels = [], {}
    for ea in rooms:
        r = parse_room(ea)
        name = consts.kRoomNames.get((r["area"], r["roomnr"]), f"room_{ea & 0xffff:04X}")
        r["name"] = name
        slug = f"{r['area']}_{ea & 0xffff:04X}_" + "".join(c if c.isalnum() else "_" for c in name)
        rd = d / slug
        rd.mkdir(parents=True, exist_ok=True)
        wb, hb = r["w"] * 16, r["h"] * 16
        for si, st in enumerate(r["states"]):
            lv = st["level_data"]
            key = (lv, st["gfx_set"])
            st["level_data"] = hexa(lv)
            tag = f"state{si}_{st['condition']}"
            if key in done_levels:
                st["render"] = done_levels[key]; continue
            raw, _ = decompress(lv)
            (rd / f"{tag}_level_data.bin").write_bytes(raw)
            size = raw[0] | raw[1] << 8
            n = size // 2
            l1 = struct.unpack_from(f"<{n}H", raw, 2)
            bts = raw[2 + size:2 + size + n]
            has_l2 = len(raw) >= 2 + size + n + size
            st.update(layer1_bytes=size, blocks=n, expected_blocks=wb * hb, has_layer2=has_l2)
            ts = build_tileset(st["gfx_set"], r["area"] == 6)
            if n >= wb * hb:
                if n > wb * hb:
                    st["note"] = "level data mayor que w*h; el juego solo indexa los primeros w*h bloques (igual aquí)"
                Image.fromarray(render_layer(l1, wb, hb, ts), "RGBA").save(rd / f"{tag}_layer1.png")
                bt = np.array([(w >> 12) for w in l1[:wb * hb]], np.uint8).reshape(hb, wb)
                (rd / f"{tag}_block_types.bin").write_bytes(bt.tobytes())
                (rd / f"{tag}_bts.bin").write_bytes(bytes(bts))
                files = [f"{tag}_layer1.png"]
                if has_l2:
                    l2 = struct.unpack_from(f"<{n}H", raw, 2 + size + n)
                    Image.fromarray(render_layer(l2, wb, hb, ts), "RGBA").save(rd / f"{tag}_layer2.png")
                    files.append(f"{tag}_layer2.png")
                st["render"] = files
                done_levels[key] = [f"../{slug}/{f}" for f in files]
            else:
                st["render"] = None
                st["warning"] = "tamaño de level data no coincide con w*h; no se renderiza"
        write_json(rd / "room.json", r)
        index.append({"snes": r["snes"], "name": name, "area": r["area"], "dir": slug,
                      "screens": f"{r['w']}x{r['h']}", "states": len(r["states"])})
    write_json(d / "index.json", index)
    return len(index)

# =============================================================== 4. SAMUS
# Basado en tools/compose_samus_frame.py (23R), generalizado a todas las poses.
# SetSamusTilesDefsForCurAnim / NmiTransferSamusToVram / Samus_Draw / DrawSamusSpritemap
SUIT_PALETTES = {"power": 0x9B9400, "varia": 0x9B9520, "gravity": 0x9B9800}

def samus_frames_per_pose(rt):
    src = open(SM_ASSETS / "restool.py", encoding="utf-8").read()
    start = src.index("kAnimdefSizes = [") + len("kAnimdefSizes = [")
    return [int(x) for x in src[start:src.index("]", start)].split(",")]

def samus_spritemap(index):
    ptr = u16(0x92808D + index * 2)
    if ptr < 0x8000:
        return None, []
    ea = 0x920000 | ptr
    n = u16(ea)
    ents = []
    for i in range(n):
        p = ea + 2 + i * 5
        x, y, ch = u16(p), u8(p + 2), u16(p + 3)
        ents.append({"x": x, "y": y - 256 if y & 0x80 else y, "large": bool(x & 0x8000),
                     "char": ch})
    return hexa(ea), ents

def samus_vram(ad):
    vram = bytearray(0x10000)
    std = rbytes(0x9AD200, 0x2E00)          # LoadStdBG3andSpriteTilesClearTilemaps
    vram[0xC000:0xC000 + len(std)] = std    # VRAM word $6000
    xfers = []
    for tbl, idx, pos, dst in ((0x92D91E, ad[0], ad[1], 0x6000), (0x92D938, ad[2], ad[3], 0x6080)):
        if idx == 0xFF:
            continue
        e = 0x920000 + u16(tbl + idx * 2) + 7 * pos
        src, p1, p2 = u24(e), u16(e + 3), u16(e + 5)
        s = lorom(src)
        vram[dst * 2:dst * 2 + p1] = ROM[s:s + p1]
        vram[(dst + 0x100) * 2:(dst + 0x100) * 2 + p2] = ROM[s + p1:s + p1 + p2]
        xfers.append({"desc": hexa(e), "src": hexa(src), "part1": p1, "part2": p2, "vram_word": hex(dst)})
    return vram, xfers

def draw_spritemap(canvas, ents, vram, pal, ox, oy):
    H, W = canvas.shape[:2]
    for e in ents:
        x = (ox + e["x"]) & 0x1FF
        if x >= 256: x -= 512
        y = oy + e["y"]
        size = 16 if e["large"] else 8
        ch = e["char"]
        name, low = (ch >> 8) & 1, ch & 0xFF
        hf, vf, big = ch & 0x4000, ch & 0x8000, []
        for ty in range(size // 8):
            row = []
            for tx in range(size // 8):
                word = 0x6000 + name * 0x1000 + ((low + tx + ty * 16) & 0xFF) * 16
                row.append(decode_tiles_4bpp(vram[word * 2:word * 2 + 32])[0])
            big.append(np.hstack(row))
        t = np.vstack(big)
        if hf: t = t[:, ::-1]
        if vf: t = t[::-1, :]
        for yy in range(size):
            for xx in range(size):
                c = t[yy, xx]
                if c and 0 <= x + xx < W and 0 <= y + yy < H:
                    canvas[y + yy, x + xx] = (*pal[c], 255)

# ---- kSamusIsBottomDrawnFuncs (0x9085E2), por movement_type -------------
def samus_bottom_drawn(pose, frame, mtype):
    """Replica SamusBottomDrawn_* de sm_90.c. True = el juego dibuja la mitad inferior."""
    if mtype in (4, 7, 8, 9, 17, 18, 19):                  # SamusBottomDrawn_4
        return False
    if mtype == 3:                                          # _3_SpinJump
        return pose in (0x81, 0x82, 0x1B, 0x1C) or frame == 0 or frame >= 0x0B
    if mtype == 10:                                         # _A_Knockback
        return (pose not in (0xD7, 0xD8)) or frame >= 3
    if mtype == 13:                                         # UNUSED_D
        return (pose not in (0x65, 0x66)) or frame < 1
    if mtype == 15:                                         # _F_Transitions
        if pose >= 0xF1:
            return True
        if pose >= 219:
            return frame == 0 if pose < 221 else frame == 2
        return pose in (0x35, 0x36, 0x3B, 0x3C)
    if mtype == 20:                                         # _14_WallJump
        return frame < 3 or frame >= 13
    if mtype == 25:                                         # _19_DamageBoost
        return frame < 2 or frame >= 9
    if mtype == 27:                                         # _1B
        if pose < 0xCF:
            return pose not in (0xCB, 0xCC)
        return (pose not in (0xE8, 0xE9)) or frame >= 2
    return True                                             # _0_Standing y _1

def samus_pose_params(pose):
    b = rbytes(0x91B629 + pose * 8, 8)
    return {"pose_x_dir": b[0], "movement_type": b[1], "y_offset_to_gfx": b[4] - 256 if b[4] & 0x80 else b[4],
            "y_radius": b[6]}

# ---- kSamus_CalcSpritemapPos (0x908C1F): desplazamiento Y del spritemap ------
def samus_spritemap_dy(pose, frame, mtype, yoff):
    """Devuelve dy tal que spritemap_y = samus_y + dy."""
    if mtype == 0:                                          # _Standing 0x908CC3
        if pose in (0x00, 0x9B):
            if frame >= 2:
                return -1
        elif 0xA4 <= pose < 0xA8:
            t = [3, 6, 0, 0, 3, 6, 0, 0, 3, 3, 6, 0, 3, 3, 6, 0]
            i = frame + 4 * (pose - 0xA4)
            if i < len(t):
                return -t[i]                                # la decomp lee uint16; el ASM usa el byte
    elif mtype == 15:                                       # _Crouch 0x908D3C
        if 0x35 <= pose < 0x41:
            t = [-8, 0, -8, 0, -4, -2, -4, -2, 0, 0, 0, 0, -4, 0, -4, 0, 5, 4, 5, 4, 0, 0, 0, 0]
            i = frame + 2 * (pose - 0x35)
            if i < len(t):
                return t[i]
    elif mtype == 27:                                       # _Special 0x908D98
        t = [7, 5, -8, -8, -8, -8, -8, -5, 4, 4, 4, 4, 0, 0, 4, -3,
             -5, 0, 0, 4, -3, -5, -3, 4, 0, 0, 4, 0, 0, 4, 0, 0]
        if pose in (0xE8, 0xE9) and frame < len(t):
            return t[frame]
        if pose in (0xEA, 0xEB) and frame >= 5:
            return -3
    return -yoff                                            # _Default 0x908C94

def extract_samus(rt, out):
    d = out / "samus"
    frames = samus_frames_per_pose(rt)
    pal = palette_from_bytes(rbytes(SUIT_PALETTES["power"], 32))
    for k, a in SUIT_PALETTES.items():
        (d / "palettes").mkdir(parents=True, exist_ok=True)
        (d / "palettes" / f"{k}_suit_bgr555.bin").write_bytes(rbytes(a, 32))
        palette_swatch(palette_from_bytes(rbytes(a, 32))).save(d / "palettes" / f"{k}_suit.png")
    man = []
    total = 0
    for pose in range(253):
        top_base, bot_base = u16(0x929263 + pose * 2), u16(0x92945D + pose * 2)
        anim = 0x920000 | u16(0x92D94E + pose * 2)
        nf = frames[pose]
        strip = []
        pinfo = {"pose": pose, "frames": []}
        pp = samus_pose_params(pose)
        mt = pp["movement_type"]
        pinfo.update(pp)
        if pose in (0x45, 0x46):
            pinfo["note"] = ("Pose no usada: sin constante kPose_ en la decompilación y la única transición "
                             "que entra es desde sí misma (kPoseTrans_45/46). Sus mitades superior e inferior "
                             "apuntan a tiles de poses distintas; el dibujo 'roto' es el contenido real de la ROM.")
        for f in range(nf):
            ad = list(rbytes(anim + 4 * f, 4))
            vram, xf = samus_vram(ad)
            canvas = np.zeros((64, 64, 4), np.uint8)
            sx, sy = 32, 36                              # posición de Samus en el lienzo
            dy = samus_spritemap_dy(pose, f, mt, pp["y_offset_to_gfx"])
            tsm, tents = samus_spritemap(top_base + f)
            bottom = samus_bottom_drawn(pose, f, mt)
            bsm, bents = samus_spritemap(bot_base + f) if bottom else (None, [])
            draw_spritemap(canvas, tents, vram, pal, sx, sy + dy)
            if bottom:
                draw_spritemap(canvas, bents, vram, pal, sx, sy + dy)
            if pose == 0:
                # SamusBottomDrawn_0_Standing: OAM extra charnum 0x3821 en (x-7, y-17)
                draw_spritemap(canvas, [{"x": 0x1F9, "y": -17, "large": False, "char": 0x3821}], vram, pal, sx, sy)
            strip.append(canvas)
            pinfo["frames"].append({"frame": f, "anim_def": hexa(anim + 4 * f), "anim_bytes": bytes(ad).hex(),
                                    "spritemap_dy": dy, "top_spritemap": tsm,
                                    "bottom_drawn": bottom, "bottom_spritemap": bsm,
                                    "dma": xf})
            total += 1
        img = Image.fromarray(np.hstack(strip), "RGBA")
        (d / "poses").mkdir(parents=True, exist_ok=True)
        img.save(d / "poses" / f"pose_{pose:03d}.png")
        man.append(pinfo)
    write_json(d / "samus_poses.json", man)
    # Atlas de todas las poses (frame 0) para revisión rápida
    cols = 16
    atlas = Image.new("RGBA", (cols * 64, ((253 + cols - 1) // cols) * 64), (40, 40, 60, 255))
    for pose in range(253):
        im = Image.open(d / "poses" / f"pose_{pose:03d}.png").crop((0, 0, 64, 64))
        atlas.alpha_composite(im, ((pose % cols) * 64, (pose // cols) * 64))
    atlas.save(d / "atlas_frame0.png")
    return total

# ============================================================= 5. ENEMIES
def extract_enemies(rt, out):
    d = out / "enemies"
    man = []
    for ea, v in sorted(rt.g_locations.items()):
        if not (isinstance(v, tuple) and len(v) == 2 and type(v[1]).__name__ == "Enemy"):
            continue
        size = u16(ea) & 0x7FFF
        bank = u8(ea + 0x0C)
        pal_ea = (bank << 16) | u16(ea + 2)
        tile_ea = u24(ea + 0x36)
        name_ptr = u16(ea + 0x3E)
        dbg = "".join(chr(c) for c in rbytes(0xB40000 | name_ptr, 10)).strip() if name_ptr else ""
        label = rt.get_ea_name(ea)
        slug = f"{ea:06X}_{label}"
        entry = {"snes": hexa(ea), "label": label, "debug_name": dbg, "bank": hex(bank),
                 "health": u16(ea + 4), "damage": u16(ea + 6),
                 "x_radius": u16(ea + 8), "y_radius": u16(ea + 10),
                 "palette": hexa(pal_ea), "tile_data": hexa(tile_ea) if tile_ea else None,
                 "tile_data_size": size, "compressed_tiles": bool(u16(ea) & 0x8000)}
        ed = d / slug
        ed.mkdir(parents=True, exist_ok=True)
        pal_b = rbytes(pal_ea, 32) if pal_ea & 0x8000 else b""
        if pal_b:
            (ed / "palette_bgr555.bin").write_bytes(pal_b)
        if tile_ea and size and tile_ea & 0x8000:
            raw = rbytes(tile_ea, size)
            (ed / "tiles_4bpp.bin").write_bytes(raw)
            tiles_sheet(decode_tiles_4bpp(raw), palette_from_bytes(pal_b) if pal_b else None).save(ed / "tiles.png")
        write_json(ed / "enemy.json", entry)
        man.append(entry)
    # spritemaps de enemigos (recorridos por enemy_instr_decode)
    sm = []
    for ea in sorted(rt.enemy_instr_parser.sprite_maps):
        n = u16(ea) if ea & 0x8000 else 0
        sm.append({"snes": hexa(ea), "name": rt.ea2name.get(ea, f"unk_{ea:06X}"), "entries": n})
    write_json(d / "spritemaps_index.json", sm)
    write_json(d / "manifest.json", man)
    return len(man)

# ============================================================= 6. PALETTES
def extract_palettes(rt, out):
    d = out / "palettes"
    d.mkdir(parents=True, exist_ok=True)
    man = []
    for ea in (0x8CE1E9, 0x8CE3E9, 0x8CE5E9, 0x8CE7E9, 0x8CE9E9, 0x8CEBE9, 0x8CEDE9, 0x8CEFE9, 0x9A8000):
        b = rbytes(ea, 512)
        (d / f"{ea:06X}_palette256.bin").write_bytes(b)
        palette_swatch(palette_from_bytes(b)).save(d / f"{ea:06X}_palette256.png")
        man.append({"snes": hexa(ea), "colors": 256, "name": rt.ea2name.get(ea, "")})
    for i in range(29):
        b, _ = decompress(tileset_ptrs(i)[2])
        (d / f"tileset{i:02d}_palette.bin").write_bytes(b)
        palette_swatch(palette_from_bytes(b)).save(d / f"tileset{i:02d}_palette.png")
        man.append({"snes": hexa(tileset_ptrs(i)[2]), "colors": len(b) // 2, "name": f"tileset {i}"})
    write_json(d / "manifest.json", man)
    return len(man)

# ======================================================== 7. MESSAGES/TEXT
# Message boxes: kMessageBoxConfig $85:869B (29 entradas), tilemap Tile16 en banco $85.
# BG3 estándar: $9AB200 (0x2000 bytes 2bpp) -> VRAM word $4000
# (LoadStdBG3andSpriteTilesClearTilemaps 0x8282E2).
def extract_text(rt, out):
    d = out / "text"
    d.mkdir(parents=True, exist_ok=True)
    bg3 = decode_tiles_2bpp(rbytes(0x9AB200, 0x2000))
    tiles_sheet(bg3, GRAY4).save(d / "bg3_standard_font_2bpp.png")
    ptrs = sorted(set(0x850000 | u16(0x85869B + i * 6 + 4) for i in range(29)))
    msgs = []
    for i, p in enumerate(ptrs):
        end = ptrs[i + 1] if i + 1 < len(ptrs) else p + 0x80 * 2
        words = [u16(p + j * 2) for j in range((end - p) // 2)]
        rows = (len(words) + 31) // 32
        img = np.zeros((rows * 8, 256), np.uint8)
        for j, w in enumerate(words):
            t = bg3[w & 0x3FF] if (w & 0x3FF) < len(bg3) else np.zeros((8, 8), np.uint8)
            if w & 0x4000: t = t[:, ::-1]
            if w & 0x8000: t = t[::-1, :]
            y, x = divmod(j, 32)
            img[y * 8:y * 8 + 8, x * 8:x * 8 + 8] = t
        im = Image.fromarray(img, "P")
        im.putpalette([c for rgb in GRAY4 for c in rgb] + [0] * (768 - 12))
        im.save(d / f"msgbox_{i:02d}_{p:06X}.png")
        msgs.append({"snes": hexa(p), "words": len(words), "png": f"msgbox_{i:02d}_{p:06X}.png"})
    write_json(d / "message_boxes.json", msgs)
    # Nombres de depuración de enemigos (ASCII en banco $B4) y nombres de salas (consts de la decompilación)
    import consts
    names = sorted({e for e in (json.loads((out / "enemies" / "manifest.json").read_text(encoding="utf-8"))
                               if (out / "enemies" / "manifest.json").exists() else [])
                    for e in [e["debug_name"]] if e})
    write_json(d / "enemy_debug_names.json", names)
    write_json(d / "room_names.json", {f"{a}:{n}": v for (a, n), v in consts.kRoomNames.items()})
    return len(msgs)

# ================================================================ 8. AUDIO
# APU_UploadBank (0x808028) / SpcPlayer_Upload: bloques [size16][dest16][data] hasta size 0.
# kMusicPointers $8F:E7E1, entradas de 3 bytes (music_data_index = 0,3,...,0x48).
# Entrada 0 = motor SPC base ($CF8000). Instrumentos: ARAM $6C00, 6 bytes c/u.
# DIR = $6D (SpcPlayer init) -> directorio de samples en ARAM $6D00 (start, loop).
# Tabla de canciones: ARAM $5820 (Word por track).
def spc_blocks(ea):
    o = lorom(ea)
    blocks = []
    while True:
        n = ROM[o] | ROM[o + 1] << 8
        if n == 0:
            blocks.append({"end": True, "entry": ROM[o + 2] | ROM[o + 3] << 8, "rom_offset": o})
            return blocks
        dst = ROM[o + 2] | ROM[o + 3] << 8
        blocks.append({"size": n, "aram": dst, "rom_offset": o + 4})
        o += 4 + n

def brr_decode(aram, start):
    """Decodificador BRR según snes_spc (blargg): el buffer guarda muestras
    duplicadas (int16); p2 se usa desplazado >>1."""
    out, pos, blocks = [], start, 0
    def i16(v):
        v &= 0xFFFF
        return v - 0x10000 if v & 0x8000 else v
    while pos + 9 <= 0x10000 and blocks < 4096:
        hdr = aram[pos]
        shift, filt, end, loop = hdr >> 4, hdr & 0x0C, hdr & 1, (hdr >> 1) & 1
        for i in range(16):
            b = aram[pos + 1 + i // 2]
            nib = (b >> 4) if i % 2 == 0 else (b & 15)
            if nib >= 8: nib -= 16
            s = (nib << shift) >> 1 if shift <= 12 else (-2048 if nib < 0 else 0)
            p1 = out[-1] if len(out) >= 1 else 0
            p2 = (out[-2] if len(out) >= 2 else 0) >> 1
            if filt >= 8:
                s += p1
                s -= p2
                if filt == 8:
                    s += p2 >> 4
                    s += (p1 * -3) >> 6
                else:
                    s += (p1 * -13) >> 7
                    s += (p2 * 3) >> 4
            elif filt:
                s += p1 >> 1
                s += (-p1) >> 5
            s = max(-32768, min(32767, s))
            out.append(i16(s * 2))
        pos += 9; blocks += 1
        if end:
            return out, pos - start, bool(loop)
    return out, pos - start, False

def write_wav(path, samples, rate=32000):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes(struct.pack(f"<{len(samples)}h", *samples))

def extract_audio(out):
    d = out / "audio"
    d.mkdir(parents=True, exist_ok=True)
    base = bytearray(0x10000)
    base_blocks = spc_blocks(0xCF8000)
    for b in base_blocks:
        if "size" in b:
            base[b["aram"]:b["aram"] + b["size"]] = ROM[b["rom_offset"]:b["rom_offset"] + b["size"]]
    (d / "aram_base_engine.bin").write_bytes(base)
    sets = []
    sample_seen = {}
    for i in range(25):
        ptr = u24(0x8FE7E1 + i * 3)
        idx = i * 3
        if i == 0:
            aram, blocks = base, base_blocks
        else:
            aram = bytearray(base)
            blocks = spc_blocks(ptr)
            for b in blocks:
                if "size" in b:
                    aram[b["aram"]:b["aram"] + b["size"]] = ROM[b["rom_offset"]:b["rom_offset"] + b["size"]]
        sd = d / f"songset_{idx:02X}"
        sd.mkdir(parents=True, exist_ok=True)
        (sd / "aram.bin").write_bytes(aram)
        # instrumentos y samples
        insts = []
        for k in range(64):
            e = aram[0x6C00 + k * 6:0x6C00 + k * 6 + 6]
            insts.append({"id": k, "srcn": e[0], "adsr1": e[1], "adsr2": e[2], "gain": e[3],
                          "pitch_base": e[4] << 8 | e[5]})
        samples = []
        (sd / "samples").mkdir(exist_ok=True)
        for s in range(64):
            st = aram[0x6D00 + s * 4] | aram[0x6D01 + s * 4] << 8
            lp = aram[0x6D02 + s * 4] | aram[0x6D03 + s * 4] << 8
            if st == 0 or st == 0xFFFF or st < 0x0200:
                continue
            pcm, nbytes, loops = brr_decode(aram, st)
            if nbytes < 9 or nbytes % 9:
                continue
            h = hashlib.sha1(bytes(aram[st:st + nbytes])).hexdigest()[:12]
            fn = f"sample_{s:02X}_{st:04X}.wav"
            write_wav(sd / "samples" / fn, pcm)
            (sd / "samples" / fn.replace(".wav", ".brr")).write_bytes(bytes(aram[st:st + nbytes]))
            samples.append({"srcn": s, "aram_start": hex(st), "aram_loop": hex(lp),
                            "loop_offset_samples": (lp - st) // 9 * 16 if lp >= st else None,
                            "brr_bytes": nbytes, "loops": loops, "sha1_12": h, "wav": f"samples/{fn}"})
            sample_seen.setdefault(h, []).append(f"songset_{idx:02X}/{fn}")
        tracks = []
        for t in range(16):
            w = aram[0x5820 + t * 2] | aram[0x5821 + t * 2] << 8
            tracks.append(hex(w))
        write_json(sd / "songset.json", {
            "music_data_index": hex(idx), "rom_pointer": hexa(ptr),
            "blocks": [{k: (hex(v) if k in ("aram", "entry", "rom_offset") else v) for k, v in b.items()} for b in blocks],
            "track_table_5820": tracks, "instruments_6C00": insts, "samples_6D00": samples,
            "notes": "WAV a 32000 Hz nominal; la altura real depende de pitch_base del instrumento."})
        sets.append({"index": hex(idx), "rom": hexa(ptr), "blocks": len(blocks) - 1, "samples": len(samples)})
    write_json(d / "manifest.json", {"songsets": sets, "unique_samples": len(sample_seen)})
    return len(sets)

# ================================================================== MAIN
def main():
    global ROM
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("out")
    ap.add_argument("--only", default="blobs,tilesets,palettes,rooms,samus,enemies,text,audio")
    a = ap.parse_args()
    ROM = Path(a.rom).read_bytes()
    sha = hashlib.sha256(ROM).hexdigest()
    if len(ROM) != ROM_SIZE or sha != ROM_SHA256:
        raise SystemExit(f"ROM no coincide con Super Metroid NTSC sin header (sha256 {sha})")
    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    rt = load_restool()
    what = a.only.split(",")
    summary = {"rom_sha256": sha, "rom_crc32": f"{zlib.crc32(ROM) & 0xffffffff:08X}"}
    steps = [("blobs", lambda: extract_blobs(rt, out)), ("tilesets", lambda: extract_tilesets(out)),
             ("palettes", lambda: extract_palettes(rt, out)), ("rooms", lambda: extract_rooms(rt, out)),
             ("samus", lambda: extract_samus(rt, out)), ("enemies", lambda: extract_enemies(rt, out)),
             ("text", lambda: extract_text(rt, out)), ("audio", lambda: extract_audio(out))]
    for k, fn in steps:
        if k in what:
            n = fn(); summary[k] = n
            print(f"[{k}] {n}", flush=True)
    prev = out / "summary.json"
    if prev.exists():
        old = json.loads(prev.read_text(encoding="utf-8")); old.update(summary); summary = old
    write_json(prev, summary)

if __name__ == "__main__":
    main()
