#!/usr/bin/env python3
"""MegaMetroid — capa del modo 7 de las cinemáticas de Ceres para la Genesis: imágenes ya armadas (docs/35).

El SNES dibuja la estación, la nave y Zebes con el modo 7 (girar y escalar el BG1). La Genesis no puede: aquí se
dibujan en el PC, con el mismo cálculo de la PPU, las imágenes que la escena necesita, y la Genesis solo las copia
al plano A y las mueve con el scroll.

Cada imagen depende de la matriz (M7A-D) y del contenido del mapa del modo 7 (la estación, la nave al explotar, la
nave hacia Zebes); el desplazamiento (BG1HOFS/VOFS) no la cambia: mover el scroll mueve la imagen entera. Así que
la imagen se guarda en coordenadas U = x + HOFS, V = y + VOFS y la Genesis pone el scroll del plano en HOFS/VOFS.
En la parte de Zebes en modo 1 la "matriz" es el nivel del mosaico.

Para no guardar una imagen por cuadro, se hace una nueva solo cuando la escala cambió más de ESCALA o el ángulo más
de ANGULO; entre tanto se usa la anterior (con el scroll de cada cuadro). Cada imagen: hasta 14 colores (línea 0
de la CRAM; el 15 es el del círculo de la explosión), sus tiles sin repetir (si son más que los que entran en la
mitad de la zona de tiles libre, se juntan los más parecidos) y el rectángulo de celdas con el tile relativo (la
Genesis carga los tiles en una mitad de la zona libre mientras muestra la otra: doble búfer, como la huida de Ridley).

Tabla kM7Keys: por cada cuadro de las grabaciones, (variante, M7A, M7B, M7C, M7D) -> imagen; la Genesis la busca
con los valores del motor (son los mismos que en el SNES: el motor es el mismo).
Variantes: 1 estación, 2 nave al explotar, 3 Ceres explota con Samus (sin nave), 4 la nave hacia Zebes,
0x10 + n: Zebes en modo 1 con mosaico de n+1.
Uso: mkm7.py snes_ref gen_dir"""
import json
import math
import re
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools" / "genesis_convert"))
from convert_rooms import snes_to_genesis, gen_to_cram, genesis_tile_bytes  # noqa: E402

ESCALA, ANGULO = 0.03, 2.5
ESCALA_VAR = {2: 0.06, 3: 0.06}               # la nave al explotar: crece rápido, no hay vblank para tantas imágenes
TILES_MAX = 200                               # tiles por imagen como mucho (se cargan en pocos cuadros)
TILES_MAX_VAR = {4: 320}                      # la nave muy de cerca (viaje a Zebes): menos imágenes, pero sin tiles cambiados
VBLANK_WORDS = 1700                           # palabras de DMA por vblank en la Genesis (aprox., con el resto del vblank)
VBLANK_WORDS_VAR = {2: 10 ** 6}               # la nave al explotar: sin esperar, cada cuadro la más nueva que se pueda cargar


def load_frames(ntiles, w, h, var):
    """cuadros que tarda la Genesis en armar una imagen en la otra mitad del plano (tiles, borrar, celdas): mientras
    tanto no se hace otra imagen (las claves de esos cuadros usan la anterior, que es la que se va a ver)"""
    words = ntiles * 16 + 2 * h * (w + 48)
    return max(1, -(-words // VBLANK_WORDS_VAR.get(var, VBLANK_WORDS)))
MARGIN = 2                                      # celdas de más alrededor de la pantalla (la imagen se usa varios cuadros)
MARGIN_V = 1                                    # arriba y abajo: la imagen entra en las 32 filas de su mitad del plano
LINE = 0
snap, gen = Path(sys.argv[1]), Path(sys.argv[2])
hdr = (gen / "md_data.h").read_text()
FIRST = {1: int(re.search(r"CINE_TILE_FIRST_C001 (\d+)", hdr).group(1)),
         2: int(re.search(r"CINE_TILE_FIRST_C002 (\d+)", hdr).group(1))}
REG = {st: (1280 - f) // 2 for st, f in FIRST.items()}


def s16(v): return v - 0x10000 if v & 0x8000 else v


def s13(v):
    v &= 0x1FFF
    return v - 0x2000 if v & 0x1000 else v


def load(name):
    d = (snap / f"{name}.bin").read_bytes()
    return d[:0x10000], np.frombuffer(d[0x10000:0x10200], "<u2")


CAP = {}
for name in ("CINE1", "CINE1_M7", "CINE1_TU", "CINE2_M7", "CINE2_M1"):
    CAP[name] = load(name)
VAR_CAP = {1: "CINE1", 2: "CINE1_M7", 3: "CINE1_TU", 4: "CINE2_M7"}


def variant(P):
    if P["mode"] != 7:
        return 0x10 | (P["mosaic"] >> 4)
    cf = P["cf"]
    if cf in (0xC11B, 0xC2E4, 0xC2F1, 0xC345):
        return 1
    if cf in (0xC5CA, 0xC610, 0xC627):
        return 3 if P["gs"] == 37 else 2
    return 4


def render_m7(var, P, U0, V0, W, H):
    """índices de CGRAM (0 = transparente) de la región U0..U0+W, V0..V0+H"""
    vram, _ = CAP[VAR_CAP[var]]
    v = np.frombuffer(vram[:0x8000], np.uint8)
    mp, ch = v[0::2].reshape(128, 128), v[1::2].reshape(256, 8, 8)
    a, b, c, d = s16(P["a"]), s16(P["b"]), s16(P["c"]), s16(P["d"])
    x0, y0 = s13(P["x0"]), s13(P["y0"])
    hofs, vofs = s13(P["hofs"]), s13(P["vofs"])
    U = np.arange(U0, U0 + W)[None, :]
    V = np.arange(V0, V0 + H)[:, None]
    sx = U - hofs
    sy = (V - vofs) + 1                         # la fila 0 de la imagen es la línea 1 de la PPU
    xx, yy = hofs - x0, vofs - y0
    u = (a * (sx + xx) + b * (sy + yy) + (x0 << 8)) >> 8
    w = (c * (sx + xx) + d * (sy + yy) + (y0 << 8)) >> 8
    inside = (u >= 0) & (u < 1024) & (w >= 0) & (w < 1024)
    t = mp[(w >> 3) & 127, (u >> 3) & 127]
    px = ch[t, w & 7, u & 7].astype(np.int32)
    return np.where(inside, px, 0)


def render_mosaic(level, U0, V0, W, H):
    """Zebes en modo 1 (BG1: mapa en 0x5C00, tiles 4bpp en 0x6000) con mosaico de level+1 (scroll en 0)"""
    vram, _ = CAP["CINE2_M1"]
    m = level + 1
    U = np.arange(U0, U0 + W)[None, :]
    V = np.arange(V0, V0 + H)[:, None]
    yy = ((V + 1) // m) * m if m > 1 else V + 1          # el mosaico cuenta desde la línea 1 de la PPU
    xx = (U // m) * m
    out = np.zeros((H, W), np.int32)
    for y in range(H):
        for x in range(W):
            X, Y = int(xx[0, x]) & 255, int(yy[y, 0]) & 255
            e = vram[0xB800 + 2 * ((Y >> 3) * 32 + (X >> 3))] | vram[0xB801 + 2 * ((Y >> 3) * 32 + (X >> 3))] << 8
            tx, ty = X & 7, Y & 7
            if e & 0x4000: tx = 7 - tx
            if e & 0x8000: ty = 7 - ty
            a = 0xC000 + (e & 0x3FF) * 32 + ty * 2
            b0, b1, b2, b3 = vram[a], vram[a + 1], vram[a + 16], vram[a + 17]
            sh = 7 - tx
            c = ((b0 >> sh) & 1) | ((b1 >> sh) & 1) << 1 | ((b2 >> sh) & 1) << 2 | ((b3 >> sh) & 1) << 3
            out[y, x] = ((e >> 10) & 7) * 16 + c if c else 0
    return out


def reduce_colors(counts, n):
    """colores de la Genesis (9 bits) -> a lo sumo n, juntando los más parecidos (queda el más usado)"""
    rgb = lambda c: np.array([c & 7, (c >> 3) & 7, c >> 6])
    alive, alias = dict(counts), {}
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


def gen9(c555):
    g = snes_to_genesis(int(c555), "dac")          # (r, g, b) de 3 bits
    return g[0] | g[1] << 3 | g[2] << 6


def reduce_tiles(tiles, cap):
    """tiles (lista de 8x8) -> a lo sumo cap: se reemplaza el menos usado por el más parecido"""
    T = np.array(tiles)
    use = np.ones(len(T), np.int64)
    alias = list(range(len(T)))
    alive = list(range(len(T)))
    flat = T.reshape(len(T), -1)
    while len(alive) > cap:
        A = flat[alive]
        dist = (A[:, None, :] != A[None, :, :]).sum(-1) * 1000 + np.minimum(use[alive][:, None], use[alive][None, :])
        np.fill_diagonal(dist, 1 << 40)
        i, j = np.unravel_index(np.argmin(dist), dist.shape)
        a, b = alive[i], alive[j]
        keep, drop = (a, b) if use[a] >= use[b] else (b, a)
        use[keep] += use[drop]
        alive.remove(drop)
        for k in range(len(alias)):
            if alias[k] == drop:
                alias[k] = keep
    return alias


traj = json.loads((snap / "cine_traj.json").read_text())
images = []                                    # (stage, ntiles, toff, x0, y0, w, h, moff, pal14)
keys = {}                                      # (var, a, b, c, d) -> imagen
tiles_blob, maps_blob = bytearray(), bytearray()
max_t = 0
for name in ("escape", "tiempo"):
    last = None
    for P in traj[name]:
        if P["ini"] & 0x80:
            continue
        var = variant(P)
        key = (var, P["a"], P["b"], P["c"], P["d"]) if var < 0x10 else (var, 0, 0, 0, 0)
        if key in keys:
            continue
        a, b = s16(P["a"]), s16(P["b"])
        sc, th = math.hypot(a, b) or 1e-9, math.degrees(math.atan2(b, a))
        if last and last[0] == var and (var >= 0x10 or (abs(math.log(sc / last[1])) < ESCALA_VAR.get(var, ESCALA)
                                                         and abs(th - last[2]) < ANGULO)
                                        or P["k"] - last[4] < last[5]):   # la anterior todavía se está cargando
            keys[key] = last[3]
            continue
        stage = 2 if var >= 4 else 1
        hofs, vofs = (s13(P["hofs"]), s13(P["vofs"])) if var < 0x10 else (0, 0)
        U0 = ((hofs >> 3) - MARGIN) * 8
        V0 = ((vofs >> 3) - MARGIN_V) * 8
        W, H = (32 + 2 * MARGIN + 1) * 8, (28 + 2 * MARGIN_V + 1) * 8
        px = render_m7(var, P, U0, V0, W, H) if var < 0x10 else render_mosaic(var & 15, U0, V0, W, H)
        cg = P.get("pal") or CAP[VAR_CAP[var] if var < 0x10 else "CINE2_M1"][1]   # colores de ese cuadro
        counts, icount = {}, {}
        for i in np.unique(px[px > 0]).tolist():
            g = gen9(cg[i])
            icount[i] = int((px == i).sum())
            counts[g] = counts.get(g, 0) + icount[i]
        pal, calias = reduce_colors(counts, 14)
        slot = {g: k + 1 for k, g in enumerate(pal)}
        lut = np.zeros(256, np.int32)
        for i in range(1, 256):
            g = gen9(cg[i])
            lut[i] = slot.get(calias.get(g, g), 0)
        gp = np.where(px > 0, lut[px], 0)
        cells = {}
        for cy in range(H // 8):
            for cx in range(W // 8):
                t = gp[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8]
                if t.any():
                    cells[(cx, cy)] = t
        if not cells:
            images.append((stage, 0, 0, 0, 0, 0, 0, 0, [0] * 14))
            idx = len(images) - 1
            keys[key] = idx
            last = (var, sc, th, idx, P["k"], 1)
            continue
        # tiles sin repetir (con volteos); si no entran en la mitad de la zona libre, se juntan los parecidos
        uniq, index, cellw = [], {}, {}
        for pos, t in sorted(cells.items()):
            found = None
            for v, hf, vf in ((t, 0, 0), (t[:, ::-1], 1, 0), (t[::-1, :], 0, 1), (t[::-1, ::-1], 1, 1)):
                k = v.tobytes()
                if k in index:
                    found = (index[k], hf, vf)
                    break
            if found is None:
                index[t.tobytes()] = len(uniq)
                uniq.append(t)
                found = (len(uniq) - 1, 0, 0)
            cellw[pos] = found
        cap = min(REG[stage], TILES_MAX_VAR.get(var, TILES_MAX))
        if len(uniq) > cap:
            al = reduce_tiles(uniq, cap)
            keep = sorted(set(al))
            ren = {o: n for n, o in enumerate(keep)}
            cellw = {pos: (ren[al[t]], hf, vf) for pos, (t, hf, vf) in cellw.items()}
            uniq = [uniq[o] for o in keep]
        max_t = max(max_t, len(uniq))
        xs = [p[0] for p in cells]; ys = [p[1] for p in cells]
        x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
        w, h = x1 - x0 + 1, y1 - y0 + 1
        toff = len(tiles_blob)
        tiles_blob += b"".join(genesis_tile_bytes(t) for t in uniq)
        moff = len(maps_blob) // 2
        for cy in range(y0, y1 + 1):               # tile relativo (+1; 0 = celda vacía): la Genesis suma la base
            for cx in range(x0, x1 + 1):
                if (cx, cy) in cellw:
                    t, hf, vf = cellw[(cx, cy)]
                    word = (LINE << 13) | (vf << 12) | (hf << 11) | (t + 1)
                else:
                    word = 0
                maps_blob += struct.pack(">H", word)
        # cada color de la Genesis es el del índice del SNES más usado de los que junta: la Genesis lo lee de
        # palette_buffer en cada cuadro (así siguen los cambios de paleta del juego, como las luces de la nave)
        rep = [0] * 14
        best = [0] * 14
        for i, n in icount.items():
            sl = int(lut[i])
            if sl and n > best[sl - 1]:
                best[sl - 1], rep[sl - 1] = n, i
        images.append((stage, len(uniq), toff, (U0 >> 3) + x0, (V0 >> 3) + y0, w, h, moff, rep))
        idx = len(images) - 1
        keys[key] = idx
        last = (var, sc, th, idx, P["k"], load_frames(len(uniq), w, h, var))

# la imagen que sigue a cada una en la cinemática (la Genesis la va cargando antes de que haga falta)
nxt = [-1] * len(images)
for name in ("escape", "tiempo"):
    prev = None
    for P in traj[name]:
        if P["ini"] & 0x80:
            continue
        var = variant(P)
        i = keys.get((var, P["a"], P["b"], P["c"], P["d"]) if var < 0x10 else (var, 0, 0, 0, 0))
        if i is None or i == prev:
            continue
        if prev is not None and nxt[prev] < 0:
            nxt[prev] = i
        prev = i

(gen / "bins" / "kM7Tiles.bin").write_bytes(bytes(tiles_blob) or bytes(2))
(gen / "bins" / "kM7Maps.bin").write_bytes(bytes(maps_blob) or bytes(2))
C = ["/* Generado por native/tools/mkm7.py (docs/35): capa del modo 7 de las cinemáticas. No editar. */",
     '#include "md_m7.h"', f"const M7Image kM7Images[{len(images)}] = {{"]
for n, (st, nt, toff, x0, y0, w, h, moff, cram) in enumerate(images):
    C.append("  { %u, %u, %u, %d, %d, %u, %u, %u, { %s }, %d }," % (st, nt, toff, x0, y0, w, h, moff,
                                                                 ", ".join("%u" % v for v in cram), nxt[n]))
C.append("};")
ks = sorted(keys.items())
C.append(f"const M7Key kM7Keys[{len(ks)}] = {{")
for (var, a, b, c, d), i in ks:
    C.append("  { 0x%02X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, %u }," % (var, a, b, c, d, i))
C.append("};")
(gen / "md_m7.c").write_text("\n".join(C) + "\n")
H = ["/* Generado por native/tools/mkm7.py (docs/35). No editar. */", "#ifndef MD_M7_H", "#define MD_M7_H",
     "#include <stdint.h>",
     "/* x0, y0: celda (U/8, V/8) de la esquina; moff: palabra en kM7Maps del rectángulo (tile + 1 relativo a la mitad; 0 = vacía) */",
     "typedef struct { uint8_t stage; uint16_t ntiles; uint32_t toff; int16_t x0, y0; uint16_t w, h; uint32_t moff; uint8_t col[14]; int16_t next; } M7Image;   /* col: índice de palette_buffer de cada color */",
     "typedef struct { uint8_t var; uint16_t a, b, c, d; uint16_t img; } M7Key;   /* ordenadas */",
     f"#define M7_IMAGES {len(images)}", f"#define M7_KEYS {len(ks)}",
     f"#define M7_TILE0_1 {FIRST[1]}", f"#define M7_REGION_1 {REG[1]}",
     f"#define M7_TILE0_2 {FIRST[2]}", f"#define M7_REGION_2 {REG[2]}",
     "extern const M7Image kM7Images[M7_IMAGES];", "extern const M7Key kM7Keys[M7_KEYS];",
     "extern const uint8_t kM7Tiles[];", "extern const uint16_t kM7Maps[];", "#endif"]
(gen / "md_m7.h").write_text("\n".join(H) + "\n")
s = (gen / "md_bins.s").read_text()
for sym in ("kM7Tiles", "kM7Maps"):
    if sym not in s:
        s += f"    .section .rodata\n    .align 2\n    .global {sym}\n{sym}:\n    .incbin \"bins/{sym}.bin\"\n"
(gen / "md_bins.s").write_text(s)
print(f"modo 7: {len(images)} imágenes, {len(tiles_blob) // 32} tiles ({len(tiles_blob) // 1024} KB), máx. {max_t} "
      f"por imagen (mitades: {REG}), mapas {len(maps_blob) // 1024} KB, {len(ks)} claves")
