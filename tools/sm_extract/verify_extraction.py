#!/usr/bin/env python3
"""Verificación independiente de data/extracted_full contra la ROM.

No reutiliza el código de extract_all.py: usa decode_common.decompress de
snesrev/sm, un decodificador 4bpp/BRR distinto y un renderizador de salas
escrito aparte. Imprime un reporte y sale con código 1 si algo no coincide.

Uso: python tools/sm_extract/verify_extraction.py sm.smc data/extracted_full
"""
import hashlib, json, os, struct, sys, wave
from pathlib import Path
import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
SM_ASSETS = HERE.parent.parent / "sm" / "assets"
os.chdir(SM_ASSETS); sys.path.insert(0, str(SM_ASSETS))
import decode_common as dc  # descompresor de snesrev (independiente del nuestro)

rom_path, out = Path(sys.argv[1]), Path(sys.argv[2])
if not rom_path.is_absolute():
    rom_path = HERE.parent.parent / rom_path
if not out.is_absolute():
    out = HERE.parent.parent / out
ROM = rom_path.read_bytes()
assert hashlib.sha256(ROM).hexdigest() == "12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72"
R = dc.ROM
def L(a): return R.map(a)
def w16(a): return R.get_word(a)
def w24(a): return R.get_long(a)
def dec(a): return bytes(dc.decompress(R, a)[0])
def H(v): return int(v.replace("$", ""), 16)

stats = {}
fails = []
def check(cat, ok, what):
    s = stats.setdefault(cat, [0, 0]); s[0] += 1
    if ok: s[1] += 1
    else: fails.append(f"{cat}: {what}")

# ---- codificador 4bpp inverso (índices -> bytes SNES) para validar PNGs
def encode_4bpp(tile):
    b = bytearray(32)
    for y in range(8):
        for x in range(8):
            v, bit = int(tile[y][x]), 7 - x
            b[2*y] |= (v & 1) << bit; b[2*y+1] |= ((v >> 1) & 1) << bit
            b[16+2*y] |= ((v >> 2) & 1) << bit; b[17+2*y] |= ((v >> 3) & 1) << bit
    return bytes(b)

def sheet_to_bytes(png, ntiles, cols):
    a = np.array(Image.open(png))
    out_ = bytearray()
    for i in range(ntiles):
        y, x = divmod(i, cols)
        out_ += encode_4bpp(a[y*8:y*8+8, x*8:x*8+8])
    return bytes(out_)

# ================= 1. blobs
man = json.loads((out / "blobs/manifest.json").read_text(encoding="utf-8"))
for e in man:
    ea = H(e["snes"])
    raw = (out / "blobs" / e["file"]).read_bytes()
    check("blobs crudos = ROM", raw == ROM[L(ea):L(ea) + e["size"]], e["name"])
    if e["kind"] == "PackedBlob":
        d = (out / "blobs" / e["decompressed_file"]).read_bytes()
        check("blobs descomprimidos = snesrev decompress", d == dec(ea), e["name"])
    if e.get("preview_4bpp"):
        data = (out / "blobs" / (e.get("decompressed_file") or e["file"])).read_bytes()
        n = len(data) // 32
        check("PNG de tiles -> bytes = datos", sheet_to_bytes(out / "blobs" / e["preview_4bpp"], n, 16) == data[:n*32], e["name"])

# ================= 2. tilesets
for i in range(29):
    t = 0x8F0000 | w16(0x8FE7A7 + i*2)
    tt, tiles, pal = w24(t), w24(t+3), w24(t+6)
    d = out / f"tilesets/{i:02d}"
    check("tileset tiles", (d/"tiles_tileset_4bpp.bin").read_bytes() == dec(tiles), i)
    check("tileset paleta", (d/"palette_bgr555.bin").read_bytes() == dec(pal), i)
    tab = (d/"tile_table_combined.bin").read_bytes()
    c, s = dec(0xB9A09D), dec(tt)
    check("tileset tile table", tab[:0x800] == c[:0x800] and tab[0x800:0x800+len(s)] == s[:0x1800], i)
    vram = (d/"vram_bg_0000-7FFF.bin").read_bytes()
    ref = bytearray(0x8000); cre = dec(0xB98000); ref[0x5000:0x5000+len(cre)] = cre[:0x3000]
    tl = dec(tiles); ref[:len(tl)] = tl[:0x8000]
    check("tileset VRAM (CRE y luego tileset, como el juego)", vram == bytes(ref), i)
    for p in range(8):
        check("PNG vram_tiles -> bytes = VRAM", sheet_to_bytes(d/f"vram_tiles_pal{p}.png", 1024, 32) == vram[:0x8000], f"{i} pal{p}")
check("CRE tiles", (out/"tilesets/cre/cre_tiles_4bpp.bin").read_bytes() == dec(0xB98000), "cre")
check("CRE tile table", (out/"tilesets/cre/cre_tile_table.bin").read_bytes() == dec(0xB9A09D), "cre")

# ================= 3. paletas
for e in json.loads((out/"palettes/manifest.json").read_text(encoding="utf-8")):
    ea = H(e["snes"])
    if e["name"].startswith("tileset"):
        i = int(e["name"].split()[1])
        check("paletas", (out/f"palettes/tileset{i:02d}_palette.bin").read_bytes() == dec(ea), e["name"])
    else:
        check("paletas", (out/f"palettes/{ea:06X}_palette256.bin").read_bytes() == ROM[L(ea):L(ea)+512], e["snes"])

# ================= 4. salas: level data + render independiente
def snes_tile_pixels(vram, idx):
    b = vram[idx*32:idx*32+32]
    px = [[0]*8 for _ in range(8)]
    for y in range(8):
        for x in range(8):
            bit = 7 - x
            px[y][x] = ((b[2*y]>>bit)&1) | (((b[2*y+1]>>bit)&1)<<1) | (((b[16+2*y]>>bit)&1)<<2) | (((b[17+2*y]>>bit)&1)<<3)
    return np.array(px, np.uint8)

ts_cache = {}
def tileset_ctx(gfx, ceres):
    k = (gfx, ceres)
    if k in ts_cache: return ts_cache[k]
    t = 0x8F0000 | w16(0x8FE7A7 + gfx*2)
    tt, tiles, pal = w24(t), w24(t+3), w24(t+6)
    vram = bytearray(0x8000)
    cre = dec(0xB98000); vram[0x5000:0x5000+len(cre)] = cre[:0x3000]   # 1º CRE
    tl = dec(tiles); vram[:len(tl)] = tl[:0x8000]                        # 2º tileset encima
    table = bytearray(0x2000)
    if ceres: s = dec(tt); table[:len(s)] = s[:0x2000]
    else:
        c = dec(0xB9A09D); table[:0x800] = c[:0x800]; s = dec(tt); table[0x800:0x800+len(s)] = s[:0x1800]
    pw = dec(pal)
    colors = []
    for j in range(256):
        v = pw[2*j] | pw[2*j+1] << 8 if 2*j+1 < len(pw) else 0
        r, g, b = v & 31, (v>>5) & 31, (v>>10) & 31
        colors.append((r*8 + r//4, g*8 + g//4, b*8 + b//4))
    tcache = {}
    ts_cache[k] = (vram, table, colors, tcache)
    return ts_cache[k]

def metatile(ctx, m):
    vram, table, colors, tcache = ctx
    if m in tcache: return tcache[m]
    img = np.zeros((16, 16, 4), np.uint8)
    for q in range(4):
        w = table[m*8+q*2] | table[m*8+q*2+1] << 8
        px = snes_tile_pixels(vram, w & 0x3FF)
        if w & 0x4000: px = np.fliplr(px)
        if w & 0x8000: px = np.flipud(px)
        p = (w >> 10) & 7
        for y in range(8):
            for x in range(8):
                c = px[y, x]
                if c:
                    img[(q//2)*8+y, (q%2)*8+x] = (*colors[p*16+c], 255)
    tcache[m] = img
    return img

for rd in sorted((out/"rooms").iterdir()):
    if not rd.is_dir(): continue
    r = json.loads((rd/"room.json").read_text(encoding="utf-8"))
    wb, hb = r["w"]*16, r["h"]*16
    for si, st in enumerate(r["states"]):
        tag = f"state{si}_{st['condition']}"
        f = rd / f"{tag}_level_data.bin"
        if not f.exists():
            continue
        lv = dec(H(st["level_data"]))
        check("salas level data = descompresión", f.read_bytes() == lv, f"{rd.name}/{tag}")
        size = lv[0] | lv[1] << 8; n = size // 2
        words = struct.unpack_from(f"<{n}H", lv, 2)[:wb*hb]
        check("salas BTS", (rd/f"{tag}_bts.bin").read_bytes() == lv[2+size:2+size+n], f"{rd.name}/{tag}")
        check("salas tipo de bloque", (rd/f"{tag}_block_types.bin").read_bytes() == bytes(w >> 12 for w in words), f"{rd.name}/{tag}")
        ctx = tileset_ctx(st["gfx_set"], r["area"] == 6)
        for layer, off in (("layer1", 2), ("layer2", 2 + size + n)):
            png = rd / f"{tag}_{layer}.png"
            if not png.exists(): continue
            ws = struct.unpack_from(f"<{n}H", lv, off)[:wb*hb]
            ref = np.zeros((hb*16, wb*16, 4), np.uint8)
            for by in range(hb):
                for bx in range(wb):
                    w = ws[by*wb+bx]
                    m = metatile(ctx, w & 0x3FF)
                    if w & 0x400: m = m[:, ::-1]
                    if w & 0x800: m = m[::-1, :]
                    ref[by*16:by*16+16, bx*16:bx*16+16] = m
            check("salas PNG = render independiente", np.array_equal(np.array(Image.open(png)), ref), f"{rd.name}/{tag}_{layer}")

# ================= 5. enemigos
for e in json.loads((out/"enemies/manifest.json").read_text(encoding="utf-8")):
    d = out / "enemies" / f"{H(e['snes']):06X}_{e['label']}"
    pe = H(e["palette"])
    if (d/"palette_bgr555.bin").exists():
        check("enemigos paleta", (d/"palette_bgr555.bin").read_bytes() == ROM[L(pe):L(pe)+32], e["label"])
    if (d/"tiles_4bpp.bin").exists():
        te = H(e["tile_data"]); b = (d/"tiles_4bpp.bin").read_bytes()
        check("enemigos tiles = ROM", b == ROM[L(te):L(te)+e["tile_data_size"]], e["label"])
        n = len(b)//32
        check("enemigos PNG -> bytes", sheet_to_bytes(d/"tiles.png", n, 16) == b[:n*32], e["label"])

# ================= 6. Samus (datos de ROM que usa cada frame)
for k, a in {"power": 0x9B9400, "varia": 0x9B9520, "gravity": 0x9B9800}.items():
    check("Samus paletas", (out/f"samus/palettes/{k}_suit_bgr555.bin").read_bytes() == ROM[L(a):L(a)+32], k)
sp = json.loads((out/"samus/samus_poses.json").read_text(encoding="utf-8"))
for p in sp:
    for f in p["frames"]:
        ea = H(f["anim_def"])
        ok = bytes.fromhex(f["anim_bytes"]) == ROM[L(ea):L(ea)+4] and ea == (0x920000 | w16(0x92D94E + p["pose"]*2)) + 4*f["frame"]
        for x in f["dma"]:
            de = H(x["desc"])
            ok &= w24(de) == H(x["src"]) and w16(de+3) == x["part1"] and w16(de+5) == x["part2"]
        check("Samus anim def + DMA = ROM", ok, f"pose {p['pose']} frame {f['frame']}")

# ================= 7. texto
bg3 = ROM[L(0x9AB200):L(0x9AB200)+0x2000]
a = np.array(Image.open(out/"text/bg3_standard_font_2bpp.png"))
enc = bytearray()
for i in range(len(bg3)//16):
    y, x = divmod(i, 16); t = a[y*8:y*8+8, x*8:x*8+8]; b = bytearray(16)
    for yy in range(8):
        for xx in range(8):
            v = int(t[yy, xx]); b[2*yy] |= (v & 1) << (7-xx); b[2*yy+1] |= ((v>>1) & 1) << (7-xx)
    enc += b
check("texto fuente PNG -> bytes = ROM", bytes(enc) == bg3, "bg3")

# ================= 8. audio
def upload(ptr, base=None):
    aram = bytearray(base) if base is not None else bytearray(0x10000)
    o = L(ptr)
    while True:
        n = ROM[o] | ROM[o+1] << 8
        if n == 0: return aram
        dst = ROM[o+2] | ROM[o+3] << 8
        for i in range(n):
            aram[(dst + i) & 0xFFFF] = ROM[o+4+i]
        o += 4 + n

def brr_ref(data):
    """BRR según fullsnes (muestras de 15 bits), implementación independiente."""
    res, old, older = [], 0, 0
    for blk in range(len(data)//9):
        h = data[blk*9]; sh, fl = h >> 4, (h >> 2) & 3
        for i in range(16):
            byte = data[blk*9+1+i//2]; n = byte >> 4 if i % 2 == 0 else byte & 15
            n = n - 16 if n >= 8 else n
            s = ((n << sh) >> 1) if sh <= 12 else (-2048 if n < 0 else 0)
            if fl == 1: s += old + ((-old) >> 4)
            elif fl == 2: s += 2*old + ((-3*old) >> 5) - older + (older >> 4)
            elif fl == 3: s += 2*old + ((-13*old) >> 6) - older + ((3*older) >> 4)
            s = max(-32768, min(32767, s))
            s15 = (s << 1) & 0xFFFF; s15 = s15 - 0x10000 if s15 & 0x8000 else s15
            older, old = old, s15 >> 1
            res.append(s15)
    return res

base = upload(0xCF8000)
check("audio ARAM base", (out/"audio/aram_base_engine.bin").read_bytes() == bytes(base), "base")
for i in range(25):
    ptr = w24(0x8FE7E1 + i*3)
    aram = base if i == 0 else upload(ptr, base)
    d = out / f"audio/songset_{i*3:02X}"
    check("audio ARAM por song set", (d/"aram.bin").read_bytes() == bytes(aram), d.name)
    sj = json.loads((d/"songset.json").read_text(encoding="utf-8"))
    for s in sj["samples_6D00"]:
        st = int(s["aram_start"], 16)
        brr = (d / s["wav"].replace(".wav", ".brr")).read_bytes()
        check("audio BRR = ARAM", brr == bytes(aram[st:st+len(brr)]) and st == aram[0x6D00+s["srcn"]*4] | aram[0x6D01+s["srcn"]*4] << 8, f"{d.name} {s['srcn']:02X}")
        w = wave.open(str(d / s["wav"])); pcm = list(struct.unpack(f"<{w.getnframes()}h", w.readframes(w.getnframes())))
        check("audio WAV = BRR decodificado (fullsnes)", pcm == brr_ref(brr), f"{d.name} {s['srcn']:02X}")


# ================= 9. hojas de metatiles, mensajes y frames de Samus (render independiente)
for i in range(29):
    ctx = tileset_ctx(i, False)
    ref = np.zeros((64*16, 256, 4), np.uint8)
    for m in range(1024):
        y, x = divmod(m, 16); ref[y*16:y*16+16, x*16:x*16+16] = metatile(ctx, m)
    check("tileset metatiles_1024.png = render independiente", np.array_equal(np.array(Image.open(out/f"tilesets/{i:02d}/metatiles_1024.png")), ref), i)

def px2bpp(b, idx):
    t = b[idx*16:idx*16+16]
    return np.array([[((t[2*y] >> (7-x)) & 1) | (((t[2*y+1] >> (7-x)) & 1) << 1) for x in range(8)] for y in range(8)], np.uint8)
for e in json.loads((out/"text/message_boxes.json").read_text(encoding="utf-8")):
    p = H(e["snes"]); words = [w16(p + j*2) for j in range(e["words"])]
    ref = np.zeros((((len(words)+31)//32)*8, 256), np.uint8)
    for j, w in enumerate(words):
        t = px2bpp(bg3, w & 0x3FF) if (w & 0x3FF) < 512 else np.zeros((8, 8), np.uint8)
        if w & 0x4000: t = t[:, ::-1]
        if w & 0x8000: t = t[::-1, :]
        y, x = divmod(j, 32); ref[y*8:y*8+8, x*8:x*8+8] = t
    check("texto message box PNG = render independiente", np.array_equal(np.array(Image.open(out/"text"/e["png"])), ref), e["png"])

# Samus: reglas (bottom drawn, dy) tomadas de extract_all tal como se citaron del C;
# VRAM, spritemaps y dibujo reimplementados aquí desde la ROM.
sys.path.insert(0, str(HERE)); import extract_all as ex; ex.ROM = ROM
pal_s = []
for j in range(16):
    v_ = w16(0x9B9400 + j*2); r, g, b = v_ & 31, (v_>>5) & 31, (v_>>10) & 31
    pal_s.append((r*8 + r//4, g*8 + g//4, b*8 + b//4))
std = ROM[L(0x9AD200):L(0x9AD200)+0x2E00]
for p in sp:
    pose = p["pose"]; strip = np.array(Image.open(out/f"samus/poses/pose_{pose:03d}.png"))
    anim = 0x920000 | w16(0x92D94E + pose*2)
    tb, bb = w16(0x929263 + pose*2), w16(0x92945D + pose*2)
    pb = ROM[L(0x91B629 + pose*8):L(0x91B629 + pose*8)+8]
    mt, yo = pb[1], (pb[4] - 256 if pb[4] & 0x80 else pb[4])
    for f in range(len(p["frames"])):
        ad = ROM[L(anim + 4*f):L(anim + 4*f)+4]
        vr = bytearray(0x10000); vr[0xC000:0xC000+len(std)] = std
        for tbl, idx, pos, dst in ((0x92D91E, ad[0], ad[1], 0x6000), (0x92D938, ad[2], ad[3], 0x6080)):
            if idx == 0xFF: continue
            de = 0x920000 + w16(tbl + idx*2) + 7*pos
            src, p1, p2 = L(w24(de)), w16(de+3), w16(de+5)
            vr[dst*2:dst*2+p1] = ROM[src:src+p1]; vr[(dst+0x100)*2:(dst+0x100)*2+p2] = ROM[src+p1:src+p1+p2]
        img = np.zeros((64, 64, 4), np.uint8)
        dy = ex.samus_spritemap_dy(pose, f, mt, yo)
        def draw(index, ox, oy, extra_ents=None):
            ents = extra_ents
            if ents is None:
                ptr = w16(0x92808D + index*2)
                if ptr < 0x8000: return
                ea = 0x920000 | ptr; ents = []
                for k in range(w16(ea)):
                    q = ea + 2 + k*5; yy = ROM[L(q+2)]
                    ents.append((w16(q), yy - 256 if yy & 0x80 else yy, w16(q+3)))
            for x0, y0, ch in ents:
                x = (ox + x0) & 0x1FF; x = x - 512 if x >= 256 else x; y = oy + y0
                size = 16 if x0 & 0x8000 else 8
                blk = np.vstack([np.hstack([snes_tile_pixels(vr[(0x6000 + ((ch >> 8) & 1)*0x1000)*2:],
                                  (((ch & 0xFF) + tx + ty*16) & 0xFF) * 16 // 16) for tx in range(size//8)]) for ty in range(size//8)])
                if ch & 0x4000: blk = blk[:, ::-1]
                if ch & 0x8000: blk = blk[::-1, :]
                for yy in range(size):
                    for xx in range(size):
                        c = blk[yy, xx]
                        if c and 0 <= x+xx < 64 and 0 <= y+yy < 64:
                            img[y+yy, x+xx] = (*pal_s[c], 255)
        draw(tb + f, 32, 36 + dy)
        if ex.samus_bottom_drawn(pose, f, mt): draw(bb + f, 32, 36 + dy)
        if pose == 0: draw(None, 32, 36, [(0x1F9, -17, 0x3821)])
        check("Samus frame PNG = render independiente", np.array_equal(strip[:, f*64:(f+1)*64], img), f"pose {pose} frame {f}")

# ================= reporte
print("\nVERIFICACIÓN CONTRA LA ROM")
tot = ok = 0
for k, (n, g) in stats.items():
    tot += n; ok += g
    print(f"  {'OK ' if n == g else 'ERR'} {k}: {g}/{n}")
print(f"\nTOTAL: {ok}/{tot} comprobaciones correctas")
for f in fails[:40]: print("  FALLA", f)
sys.exit(0 if ok == tot else 1)
