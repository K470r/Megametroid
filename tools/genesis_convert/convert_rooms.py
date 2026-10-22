#!/usr/bin/env python3
"""MegaMetroid 27 — Conversión de salas de Super Metroid a Sega Genesis.

Entrada: data/extracted_full (verificada contra la ROM, doc 26).
Salida:  data/genesis/rooms/<sala>/stateN_*  (tiles, paletas, mapas, preview, reporte)

Pasos:
  1. Tiles SNES 4bpp planar -> Genesis 4bpp packed.
  2. Color BGR555 -> CRAM Genesis de 9 bits (niveles del DAC; ver COLOR_MODE).
  3. Paletas por sala: las subpaletas SNES que usa la sala se agrupan en
     N paletas Genesis de 15 colores (+ transparente), de modo que cada tile
     8x8 quepa entero en una sola.
  4. Reindexado de píxeles, deduplicación con flips y mapas de plano Genesis
     (PRI | PAL(2) | VF | HF | TILE(11)), big-endian.

Uso:
  python3 tools/genesis_convert/convert_rooms.py data/extracted_full data/genesis/rooms [--area 0] [--pals 2]
"""
import argparse, json, struct, sys
from pathlib import Path
from itertools import product

import numpy as np
from PIL import Image

# ------------------------------------------------------------------ color
# Niveles de salida del DAC del VDP (modo normal) por cada valor de 3 bits,
# según mediciones publicadas por la comunidad (p. ej. plutiedev / SpritesMind).
# No son lineales: por eso "vecino más cercano" en este espacio conserva mejor
# el brillo que un simple >>2.
DAC_LEVELS = [0, 52, 87, 116, 144, 172, 206, 255]

def snes5_to_8(v):
    return (v << 3) | (v >> 2)

def quant_channel(v5, mode):
    if mode == "linear":
        return v5 >> 2
    x = snes5_to_8(v5)
    return min(range(8), key=lambda i: abs(DAC_LEVELS[i] - x))

def _srgb_lin(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4

def _oklab(rgb):
    r, g, b = (_srgb_lin(x) for x in rgb)
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    return (0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)

_GEN_ALL = None
def snes_to_genesis(c, mode):
    """dac   : canal por canal al nivel del DAC más cercano (predeterminado)
       linear: >>2 por canal
       oklab : color Genesis completo más cercano en espacio perceptual OKLab"""
    global _GEN_ALL
    r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31
    if mode == "oklab":
        if _GEN_ALL is None:
            _GEN_ALL = [((i, j, k), _oklab((DAC_LEVELS[i], DAC_LEVELS[j], DAC_LEVELS[k])))
                        for i in range(8) for j in range(8) for k in range(8)]
        o = _oklab((snes5_to_8(r), snes5_to_8(g), snes5_to_8(b)))
        return min(_GEN_ALL, key=lambda e: sum((e[1][n] - o[n]) ** 2 for n in range(3)))[0]
    return (quant_channel(r, mode), quant_channel(g, mode), quant_channel(b, mode))

def gen_to_cram(g):
    r, gg, b = g
    return (b << 9) | (gg << 5) | (r << 1)

def gen_to_rgb(g):
    return tuple(DAC_LEVELS[x] for x in g)

# ------------------------------------------------------------------ tiles
def snes_tile(vram, t):
    b = vram[t * 32:t * 32 + 32]
    a = np.frombuffer(bytes(b), np.uint8)
    bits = np.unpackbits(a.reshape(2, 8, 2), axis=2, bitorder="big").reshape(2, 8, 2, 8)
    return (bits[0, :, 0] | bits[0, :, 1] << 1 | bits[1, :, 0] << 2 | bits[1, :, 1] << 3).astype(np.uint8)

def genesis_tile_bytes(px):
    """8x8 índices -> 32 bytes Genesis (4bpp packed, nibble alto = píxel izquierdo)."""
    p = px.astype(np.uint8).reshape(8, 4, 2)
    return bytes((p[:, :, 0] << 4 | p[:, :, 1]).reshape(-1))

# ------------------------------------------------------------------ palettes
def pack_palettes(items, npal, cap=15):
    """items: lista de (clave, set_de_colores). Devuelve lista de bins
    [(set_colores, [claves])] o None si no cabe. cap: colores por paleta (un número, o uno por paleta)."""
    caps = list(cap) if isinstance(cap, (list, tuple)) else [cap] * npal
    items = sorted(items, key=lambda kv: -len(kv[1]))
    if len(items) <= 10:
        best = None
        for assign in product(range(npal), repeat=len(items)):
            bins = [set() for _ in range(npal)]
            for (k, cs), b in zip(items, assign):
                bins[b] |= cs
            if all(len(b) <= c for b, c in zip(bins, caps)):
                score = sum(len(b) for b in bins)
                if best is None or score < best[0]:
                    best = (score, assign)
        if best:
            bins = [(set(), []) for _ in range(npal)]
            for (k, cs), b in zip(items, best[1]):
                bins[b][0].update(cs); bins[b][1].append(k)
            return bins
        return None
    return pack_search(items, npal, caps)

def pack_search(items, npal, cap, restarts=400, seed=1234):
    """Empaquetado por tile: agrupa conjuntos de colores idénticos, prueba
    ordenes aleatorios con colocación voraz y luego mejora local
    (mover un conjunto de paleta si reduce el exceso). Determinista (semilla fija)."""
    import random
    rnd = random.Random(seed)
    caps = list(cap) if isinstance(cap, (list, tuple)) else [cap] * npal
    groups = {}
    for k, cs in items:
        groups.setdefault(frozenset(cs), []).append(k)
    sets = sorted(groups, key=lambda c: -len(c))
    def try_order(order):
        bins = [set() for _ in range(npal)]
        where = {}
        for cs in order:
            best = None
            for i, b in enumerate(bins):
                grow = len(b | cs) - len(b)
                over = max(0, len(b | cs) - caps[i])
                key = (over, grow, len(b))
                if best is None or key < best[0]:
                    best = (key, i)
            bins[best[1]] |= cs
            where[cs] = best[1]
        return bins, where
    def excess(where):
        bins = [set() for _ in range(npal)]
        for cs, i in where.items():
            bins[i] |= cs
        return sum(max(0, len(b) - c) for b, c in zip(bins, caps)), bins
    for r in range(restarts):
        order = list(sets) if r == 0 else rnd.sample(sets, len(sets))
        if r > 0 and r % 2:
            order.sort(key=lambda c: -len(c) + rnd.random() * 3)
        _, where = try_order(order)
        ex, bins = excess(where)
        improved = True
        while ex and improved:
            improved = False
            for cs in sets:
                cur = where[cs]
                for j in range(npal):
                    if j == cur: continue
                    where[cs] = j
                    ex2, bins2 = excess(where)
                    if ex2 < ex:
                        ex, bins, improved = ex2, bins2, True
                        cur = j
                    else:
                        where[cs] = cur
        if ex == 0:
            out = [(set(b), []) for b in bins]
            for cs, i in where.items():
                out[i][1].extend(groups[cs])
            return out
    return None

def color_dist(a, b):
    ra, rb = gen_to_rgb(a), gen_to_rgb(b)
    rm = (ra[0] + rb[0]) / 2
    dr, dg, db = ra[0] - rb[0], ra[1] - rb[1], ra[2] - rb[2]
    return ((2 + rm / 256) * dr * dr + 4 * dg * dg + (2 + (255 - rm) / 256) * db * db) ** 0.5

# ------------------------------------------------------------------ room
def load_tileset(ext, gfx, ceres=False):
    d = ext / "tilesets" / f"{gfx:02d}"
    pal = (d / "palette_bgr555.bin").read_bytes()
    pal_w = [pal[i] | pal[i + 1] << 8 for i in range(0, min(len(pal), 512), 2)]
    pal_w += [0] * (256 - len(pal_w))
    table = (d / "tile_table_combined.bin").read_bytes()
    if ceres:
        # LoadLevelDataAndOtherThings: en el área 6 la tabla del tileset va
        # completa a $7EA000, sin la tabla CRE.
        man = json.loads((ext / "tilesets" / "manifest.json").read_text(encoding="utf-8"))
        tt = int(man[gfx]["tile_table"].replace("$", ""), 16)
        raw = next((ext / "blobs" / "decompressed").glob(f"{tt:06X}_*.bin")).read_bytes()
        table = (raw + bytes(0x2000))[:0x2000]
    return {"table": table,
            "vram": (d / "vram_bg_0000-7FFF.bin").read_bytes(),
            "pal": pal_w}

def room_cells(words, wb, hb, table):
    """Bloques 16x16 -> celdas 8x8 SNES (tile, pal, pri, hf, vf), aplicando
    los flips de bloque como BuildBgColumn (sm_80.c)."""
    W, H = wb * 2, hb * 2
    cells = [[None] * W for _ in range(H)]
    for by in range(hb):
        for bx in range(wb):
            w = words[by * wb + bx]
            mt, bh, bv = w & 0x3FF, bool(w & 0x400), bool(w & 0x800)
            q = [struct.unpack_from("<H", table, mt * 8 + i * 2)[0] for i in range(4)]
            grid = [[q[0], q[1]], [q[2], q[3]]]
            if bh:
                grid = [[r[1] ^ 0x4000, r[0] ^ 0x4000] for r in grid]
            if bv:
                grid = [[c ^ 0x8000 for c in grid[1]], [c ^ 0x8000 for c in grid[0]]]
            for yy in range(2):
                for xx in range(2):
                    e = grid[yy][xx]
                    cells[by * 2 + yy][bx * 2 + xx] = (e & 0x3FF, (e >> 10) & 7, (e >> 13) & 1,
                                                       (e >> 14) & 1, (e >> 15) & 1)
    return cells

def convert_state(ext, room, st, npal, mode, tiles_cache):
    wb, hb = room["w"] * 16, room["h"] * 16
    d = ext / "rooms" / room["_dir"]
    tag = st["_tag"]
    lv = (d / f"{tag}_level_data.bin").read_bytes()
    size = lv[0] | lv[1] << 8
    n = size // 2
    ts = load_tileset(ext, st["gfx_set"], room["area"] == 6)
    layers = {"layer1": struct.unpack_from(f"<{n}H", lv, 2)[:wb * hb]}
    if st.get("has_layer2"):
        layers["layer2"] = struct.unpack_from(f"<{n}H", lv, 2 + size + n)[:wb * hb]
    cells = {k: room_cells(v, wb, hb, ts["table"]) for k, v in layers.items()}

    # --- colores usados por cada combo (tile, subpaleta SNES)
    def snes_px(t):
        k = (st["gfx_set"], t)
        if k not in tiles_cache:
            tiles_cache[k] = snes_tile(ts["vram"], t)
        return tiles_cache[k]
    gcol = {}
    def gc(p, c):
        k = (p, c)
        if k not in gcol:
            gcol[k] = snes_to_genesis(ts["pal"][p * 16 + c], mode)
        return gcol[k]
    combos = {}
    for L in cells.values():
        for row in L:
            for (t, p, pri, hf, vf) in row:
                if (t, p) not in combos:
                    px = snes_px(t)
                    combos[(t, p)] = {gc(p, int(c)) for c in np.unique(px) if c}
    # --- agrupación: primero por subpaleta SNES, luego por combo, luego fusión de colores
    merges = {}
    def apply_merges(cs):
        return {merges.get(c, c) for c in cs}
    level = "subpaleta"
    by_pal = {}
    for (t, p), cs in combos.items():
        by_pal.setdefault(p, set()).update(cs)
    bins = pack_palettes(list(by_pal.items()), npal)
    if bins is None:
        level = "tile"
        bins = pack_palettes([((t, p), cs) for (t, p), cs in combos.items()], npal)
    while bins is None:
        level = "tile+fusión"
        allc = sorted(set().union(*[apply_merges(cs) for cs in combos.values()]))
        best = None
        for i in range(len(allc)):
            for j in range(i + 1, len(allc)):
                dd = color_dist(allc[i], allc[j])
                if best is None or dd < best[0]:
                    best = (dd, allc[i], allc[j])
        _, a, b = best
        for k, v in list(merges.items()):
            if v == b: merges[k] = a
        merges[b] = a
        bins = pack_palettes([((t, p), apply_merges(cs)) for (t, p), cs in combos.items()], npal)
    # combo -> índice de paleta Genesis
    assign = {}
    for bi, (cs, keys) in enumerate(bins):
        for k in keys:
            if level == "subpaleta":
                for (t, p) in combos:
                    if p == k: assign[(t, p)] = bi
            else:
                assign[k] = bi
    palettes = []
    for cs, _ in bins:
        cl = sorted(cs, key=lambda g: (sum(gen_to_rgb(g)), g))
        palettes.append(cl)
    # backdrop = color 0 de la paleta 0 del SNES (CGRAM[0])
    backdrop = snes_to_genesis(ts["pal"][0], mode)

    # --- tiles Genesis deduplicados con flips
    tile_list, tile_index = [], {}
    def add_tile(px):
        variants = [(px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1, :], 0, 1), (px[::-1, ::-1], 1, 1)]
        for v, fh, fv in variants:
            key = genesis_tile_bytes(v)
            if key in tile_index:
                return tile_index[key], fh, fv
        key = genesis_tile_bytes(px)
        tile_index[key] = len(tile_list)
        tile_list.append(key)
        return tile_index[key], 0, 0
    add_tile(np.zeros((8, 8), np.uint8))       # tile 0 = vacío
    gen_cache = {}
    def gen_tile(t, p):
        k = (t, p)
        if k in gen_cache:
            return gen_cache[k]
        bi = assign[k]
        lut = np.zeros(16, np.uint8)
        for c in range(1, 16):
            g = merges.get(gc(p, c), gc(p, c))
            if g in palettes[bi]:
                lut[c] = palettes[bi].index(g) + 1
        res = add_tile(lut[snes_px(t)]), bi
        gen_cache[k] = res
        return res

    maps = {}
    for name, L in cells.items():
        out = bytearray()
        for row in L:
            for (t, p, pri, hf, vf) in row:
                (gi, fh, fv), bi = gen_tile(t, p)
                word = (pri << 15) | (bi << 13) | ((vf ^ fv) << 12) | ((hf ^ fh) << 11) | gi
                out += struct.pack(">H", word)
        maps[name] = bytes(out)

    cram = []
    for pl in palettes:
        words = [gen_to_cram(backdrop)] + [gen_to_cram(g) for g in pl]
        words += [0] * (16 - len(words))
        cram.append(words)
    return {"tiles": b"".join(tile_list), "ntiles": len(tile_list), "maps": maps,
            "cram": cram, "palettes": palettes, "backdrop": backdrop,
            "level": level, "merges": {str(k): str(v) for k, v in merges.items()},
            "merge_pairs": [(k, v) for k, v in merges.items()],
            "combos": len(combos), "W": wb * 2, "H": hb * 2}

# ------------------------------------------------------------------ render/verify
def render_genesis(res, layer):
    """Renderiza desde los datos Genesis (tiles+mapa+CRAM). Transparente = alfa 0."""
    tiles = res["tiles"]
    W, H = res["W"], res["H"]
    m = struct.unpack(f">{W * H}H", res["maps"][layer])
    tpx = np.frombuffer(tiles, np.uint8).reshape(-1, 8, 4)
    tpx = np.stack([tpx >> 4, tpx & 15], axis=-1).reshape(-1, 8, 8)
    pals = []
    for words in res["cram"]:
        pals.append([(DAC_LEVELS[(w >> 1) & 7], DAC_LEVELS[(w >> 5) & 7], DAC_LEVELS[(w >> 9) & 7]) for w in words])
    img = np.zeros((H * 8, W * 8, 4), np.uint8)
    for cy in range(H):
        for cx in range(W):
            w = m[cy * W + cx]
            px = tpx[w & 0x7FF]
            if w & 0x800: px = px[:, ::-1]
            if w & 0x1000: px = px[::-1, :]
            pal = np.array(pals[(w >> 13) & 3], np.uint8)
            blk = img[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8]
            blk[..., :3] = pal[px]
            blk[..., 3] = np.where(px == 0, 0, 255)
            blk[px == 0, :3] = 0
    return img

def reference_quantized(ext, room, st, layer, mode):
    """Render SNES verificado (doc 26) con cada color pasado por la misma cuantización."""
    d = ext / "rooms" / room["_dir"]
    png = d / f"{st['_tag']}_{layer}.png"
    snes = np.array(Image.open(png))
    ts = load_tileset(ext, st["gfx_set"], room["area"] == 6)
    lut = {}
    for p in range(8):
        for c in range(16):
            v = ts["pal"][p * 16 + c]
            r, g, b = v & 31, (v >> 5) & 31, (v >> 10) & 31
            lut[(snes5_to_8(r), snes5_to_8(g), snes5_to_8(b))] = gen_to_rgb(snes_to_genesis(v, mode))
    out = np.zeros_like(snes)
    flat = snes.reshape(-1, 4)
    o = out.reshape(-1, 4)
    keys, inv = np.unique(flat[:, :3], axis=0, return_inverse=True)
    mapped = np.array([lut.get(tuple(int(x) for x in k), (0, 0, 0)) for k in keys], np.uint8)
    o[:, :3] = mapped[inv.reshape(-1)]
    o[:, 3] = flat[:, 3]
    o[flat[:, 3] == 0, :3] = 0
    return out

# ------------------------------------------------------------------ main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("extracted"); ap.add_argument("out")
    ap.add_argument("--area", type=int, default=None, help="0=Crateria ... 6=Ceres; omitir = todas")
    ap.add_argument("--pals", type=int, default=2, help="paletas Genesis preferidas para fondos")
    ap.add_argument("--max-pals", type=int, default=3,
                    help="si con --pals hay que fusionar colores, probar hasta este número sin pérdida")
    ap.add_argument("--color", default="dac", choices=["dac", "linear", "oklab"])
    a = ap.parse_args()
    ext, out = Path(a.extracted), Path(a.out)
    idx = json.loads((ext / "rooms" / "index.json").read_text(encoding="utf-8"))
    report = []
    tiles_cache = {}
    for e in idx:
        if a.area is not None and e["area"] != a.area:
            continue
        room = json.loads((ext / "rooms" / e["dir"] / "room.json").read_text(encoding="utf-8"))
        room["_dir"] = e["dir"]
        for si, st in enumerate(room["states"]):
            st["_tag"] = f"state{si}_{st['condition']}"
            if not (ext / "rooms" / e["dir"] / f"{st['_tag']}_level_data.bin").exists():
                continue
            res = convert_state(ext, room, st, a.pals, a.color, tiles_cache)
            npal = a.pals
            while res["merge_pairs"] and npal < a.max_pals:
                npal += 1
                alt = convert_state(ext, room, st, npal, a.color, tiles_cache)
                if len(alt["merge_pairs"]) < len(res["merge_pairs"]):
                    res = alt                      # quedarse con la opción de menos pérdida
            res["npal"] = len(res["cram"])
            od = out / e["dir"]
            od.mkdir(parents=True, exist_ok=True)
            base = st["_tag"]
            (od / f"{base}_tiles.bin").write_bytes(res["tiles"])
            (od / f"{base}_cram.bin").write_bytes(b"".join(struct.pack(">16H", *w) for w in res["cram"]))
            entry = {"room": e["name"], "dir": e["dir"], "state": base, "gfx_set": st["gfx_set"],
                     "width_cells": res["W"], "height_cells": res["H"],
                     "tiles": res["ntiles"], "tiles_bytes": len(res["tiles"]),
                     "snes_combos": res["combos"], "palettes": len(res["cram"]),
                     "colors_per_palette": [len(p) for p in res["palettes"]],
                     "packing": res["level"], "merged_colors": len(res["merge_pairs"]),
                     "needs_extra_palette": len(res["cram"]) > a.pals,
                     "layers": {}}
            for layer, mp in res["maps"].items():
                (od / f"{base}_{layer}_map.bin").write_bytes(mp)
                g = render_genesis(res, layer)
                ref = reference_quantized(ext, room, st, layer, a.color)
                Image.fromarray(g, "RGBA").save(od / f"{base}_{layer}_genesis.png")
                diff = np.any(g != ref, axis=2)
                err = np.abs(g[..., :3].astype(int) - ref[..., :3].astype(int)).max(axis=2)
                entry["layers"][layer] = {"map_bytes": len(mp),
                                          "pixels_different": int(diff.sum()),
                                          "pixels_total": int(diff.size),
                                          "max_channel_error": int(err.max())}
            (od / f"{base}_report.json").write_text(json.dumps(entry, indent=2, ensure_ascii=False), encoding="utf-8")
            report.append(entry)
            print(f"{e['dir']:45s} {base:22s} tiles={res['ntiles']:4d} pals={[len(p) for p in res['palettes']]} "
                  f"{res['level']:12s} merges={len(res['merge_pairs'])} "
                  + " ".join(f"{k}:diff={v['pixels_different']}" for k, v in entry["layers"].items()), flush=True)
    out.mkdir(parents=True, exist_ok=True)
    prev = out / "report.json"
    if a.area is not None and prev.exists():
        done = {(r["dir"], r["state"]) for r in report}
        report = [r for r in json.loads(prev.read_text(encoding="utf-8"))
                  if (r["dir"], r["state"]) not in done] + report
        report.sort(key=lambda r: (r["dir"], r["state"]))
    (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

if __name__ == "__main__":
    main()
