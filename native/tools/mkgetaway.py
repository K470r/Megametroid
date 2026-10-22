#!/usr/bin/env python3
"""MegaMetroid — huida de Ridley de Ceres para la Genesis: cuadros ya armados (docs/33).

Entrada: snes_ref/getaway.npz (snap_getaway.py), el plano B de la sala de Ridley (bins/kRoomPlaneB_E0B5.bin) y
md_data.h (dónde quedan libres los tiles de Ridley). Cada cuadro de la huida pasa a ser:
  - una paleta de 15 colores de la Genesis (línea 1 de la CRAM, la de Ridley);
  - sus tiles (sin repetir, contando volteos);
  - el rectángulo de celdas del plano B que ocupa, con prioridad alta (delante del plano A), en dos versiones según
    la mitad de la zona de tiles en la que se cargue (doble búfer: la Genesis carga los tiles de un cuadro mientras
    muestra el anterior). Las celdas del rectángulo sin Ridley llevan el plano B original.
Uso: mkgetaway.py getaway.npz gen_dir"""
import re
import struct
import sys
from pathlib import Path

import numpy as np

npz, gen = sys.argv[1], Path(sys.argv[2])
d = np.load(npz)
masks, cols = d["mask"], d["col"]
hdr = (gen / "md_data.h").read_text()
first = int(re.search(r"RIDLEY_TILE_FIRST_E0B5 (\d+)", hdr).group(1))
REG = (1280 - first) // 2                       # tiles por mitad
planeB = struct.unpack(">4096H", (gen / "bins" / "kRoomPlaneB_E0B5.bin").read_bytes())
PW = 64
LINE = 1


def tile_bytes(px):
    p = px.astype(np.uint8).reshape(8, 4, 2)
    return bytes((p[:, :, 0] << 4 | p[:, :, 1]).reshape(-1))


def reduce_palette(counts, n=15):
    """colores (índice de 9 bits: r | g<<3 | b<<6) -> a lo sumo n, fundiendo los más parecidos (el más usado se queda)"""
    alias = {}
    alive = dict(counts)
    rgb = lambda c: np.array([c & 7, (c >> 3) & 7, c >> 6])
    while len(alive) > n:
        ks = sorted(alive)
        best = None
        for i in range(len(ks)):
            for j in range(i + 1, len(ks)):
                dd = int(((rgb(ks[i]) - rgb(ks[j])) ** 2).sum())
                if best is None or dd < best[0]:
                    best = (dd, ks[i], ks[j])
        _, a, b = best
        keep, drop = (a, b) if alive[a] >= alive[b] else (b, a)
        alive[keep] += alive.pop(drop)
        for k, v in list(alias.items()):
            if v == drop:
                alias[k] = keep
        alias[drop] = keep
    return sorted(alive), alias


C = ["/* Generado por native/tools/mkgetaway.py (docs/33): huida de Ridley de Ceres. No editar. */",
     '#include "md_data.h"', '#include "md_getaway.h"']
tiles_blob = bytearray()
maps_blob = bytearray()
frames = []
max_t = 0
for f in range(len(masks)):
    m, c = masks[f], cols[f]
    ys, xs = np.nonzero(m)
    if len(xs) == 0:
        frames.append((0, 0, 0, 0, 0, 0, 0, [0] * 15))
        continue
    counts = {}
    for v in c[m].tolist():
        counts[v] = counts.get(v, 0) + 1
    pal, alias = reduce_palette(counts)
    slot = {v: i + 1 for i, v in enumerate(pal)}
    cells = {}
    for y, x in zip(ys.tolist(), xs.tolist()):
        v = int(c[y, x])
        v = alias.get(v, v)
        cy, cx = (y + 1) >> 3, x >> 3              # la fila y de la captura del SNES es la fila y+1 del fondo
        cells.setdefault((cx, cy), np.zeros((8, 8), np.int32))[(y + 1) & 7, x & 7] = slot[v]
    uniq, index = [], {}
    cellw = {}
    for (cx, cy), px in sorted(cells.items()):
        found = None
        for v, hf, vf in ((px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1, :], 0, 1), (px[::-1, ::-1], 1, 1)):
            k = tile_bytes(v)
            if k in index:
                found = (index[k], hf, vf)
                break
        if found is None:
            k = tile_bytes(px)
            index[k] = len(uniq)
            uniq.append(k)
            found = (index[k], 0, 0)
        cellw[(cx, cy)] = found
    assert len(uniq) <= REG, (f, len(uniq), REG)
    max_t = max(max_t, len(uniq))
    x0, x1 = min(k[0] for k in cells), max(k[0] for k in cells)
    y0, y1 = min(k[1] for k in cells), max(k[1] for k in cells)
    w, h = x1 - x0 + 1, y1 - y0 + 1
    toff = len(tiles_blob)
    tiles_blob += b"".join(uniq)
    moff = len(maps_blob) // 2
    for reg in (0, 1):
        base = first + reg * REG
        for cy in range(y0, y1 + 1):
            for cx in range(x0, x1 + 1):
                if (cx, cy) in cellw:
                    t, hf, vf = cellw[(cx, cy)]
                    word = (1 << 15) | (LINE << 13) | (vf << 12) | (hf << 11) | (base + t)
                else:
                    word = planeB[cy * PW + cx]
                maps_blob += struct.pack(">H", word)
    cram = [((v >> 6) << 9) | (((v >> 3) & 7) << 5) | ((v & 7) << 1) for v in pal] + [0] * (15 - len(pal))
    frames.append((len(uniq), toff, x0, y0, w, h, moff, cram))
(gen / "bins" / "kGetawayTiles.bin").write_bytes(bytes(tiles_blob) or bytes(2))
(gen / "bins" / "kGetawayMaps.bin").write_bytes(bytes(maps_blob) or bytes(2))
C.append(f"const GetawayFrame kGetaway[{len(frames)}] = {{")
for (nt, toff, x0, y0, w, h, moff, cram) in frames:
    C.append("  { %u, %u, %u, %u, %u, %u, %u, { %s } }," % (nt, toff, x0, y0, w, h, moff, ", ".join("0x%03X" % v for v in cram)))
C.append("};")
(gen / "md_getaway.c").write_text("\n".join(C) + "\n")
H = ["/* Generado por native/tools/mkgetaway.py (docs/33). No editar. */", "#ifndef MD_GETAWAY_H", "#define MD_GETAWAY_H",
     "#include <stdint.h>",
     "typedef struct { uint16_t ntiles; uint32_t toff; uint16_t x0, y0, w, h; uint32_t moff; uint16_t pal[15]; } GetawayFrame;",
     "/* moff: palabra en kGetawayMaps donde empieza el rectángulo (w x h) para la mitad 0; sigue el de la mitad 1 */",
     f"#define GETAWAY_FRAMES {len(frames)}", f"#define GETAWAY_TILE0 {first}", f"#define GETAWAY_REGION {REG}",
     "extern const GetawayFrame kGetaway[GETAWAY_FRAMES];", "extern const uint8_t kGetawayTiles[];",
     "extern const uint16_t kGetawayMaps[];", "#endif"]
(gen / "md_getaway.h").write_text("\n".join(H) + "\n")
s = (gen / "md_bins.s").read_text()
for sym in ("kGetawayTiles", "kGetawayMaps"):
    if sym not in s:
        s += f"    .section .rodata\n    .align 2\n    .global {sym}\n{sym}:\n    .incbin \"bins/{sym}.bin\"\n"
(gen / "md_bins.s").write_text(s)
print(f"huida: {len(frames)} cuadros, {len(tiles_blob) // 32} tiles, máx. {max_t} por cuadro (mitad: {REG}), "
      f"mapas {len(maps_blob)} bytes")
