#!/usr/bin/env python3
"""El vuelo a Ceres de la intro (CinematicFunction_Intro_Func54, $8B:BCA0, hasta Func73): lo que
make_intro_data.py necesita además de lo de las otras fases (docs/29).

En el SNES:
  * la nave es BG1 en modo 7 (tiles y mapa de $95:A82F / $96:FE69, 15 colores: CGRAM 81-94). No gira:
    solo cambia de tamaño y de posición con dos ángulos fijos. La Genesis no tiene modo 7, así que
    cada cuadro de la nave se dibuja aquí igual que lo haría el PPU del SNES con los registros que pone
    el juego (M7A-D, M7X/Y, BG1HOFS/VOFS: port/diff/intro_usage.json) y se guarda como tiles. En la
    Genesis se muestran en un plano (intro_md.c).
  * la estación Ceres, los asteroides, las estrellas y el planeta son sprites de 6 spritemaps grandes
    ($8C:8FE7...): hasta ~140 sprites de 8x8 y 16x16 por cuadro, y la Genesis tiene 64. Cada spritemap
    se compone aquí como imagen (con el mismo orden de dibujo del SNES) y se cubre con sprites de hasta
    32x32 de la Genesis.
  * "SPACE COLONY": texto en BG1 en modo 1 (mapa $5C00, tiles 4bpp en $6000).
"""
import struct

import numpy as np

M7_LINE = 3                  # línea de paleta de la nave: la línea 5 de la CGRAM (80-95) tal cual


def lorom(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def flight_vram(rom, blob, tex=0):
    """VRAM del SNES después de CinematicFunction_Intro_Func54: tiles de modo 7 en los bytes altos
    ($2119, VMAIN $80), el mapa lleno de $8C y luego las primeras $300 celdas del mapa ($2118, VMAIN 0),
    y los tiles del espacio ($96:D10A) en $6000. tex = 1: el mapa de la nave vista de atrás, que
    CinematicFunction_Intro_Func56 ($8B:BDF9) manda por la cola de modo 7 en el destello ($7F:4300)."""
    vr = np.zeros(0x8000, np.uint16)
    hi = np.frombuffer(blob('95A82F')[:0x4000], np.uint8).astype(np.uint16)
    vr[:0x4000] = (hi << 8) | 0x8C
    lo = np.frombuffer(blob('96FE69')[tex * 0x300:tex * 0x300 + 0x300], np.uint8).astype(np.uint16)
    vr[:0x300] = (vr[:0x300] & 0xFF00) | lo
    vr[0x6000:0x8000] = np.frombuffer(blob('96D10A')[:0x4000], '<u2')
    return vr


def s16(v):
    return v - 65536 if v >= 32768 else v


def s13(v):
    v &= 0x1FFF
    return v - 8192 if v >= 4096 else v


def render_m7(vr, prm):
    """Píxeles (índice de CGRAM, 0 = transparente) de BG1 en modo 7, como el PPU del SNES
    (M7SEL = $80: fuera del mapa de 1024x1024 es transparente)."""
    A, B, C, D = (s16(x) for x in prm[:4])
    X0, Y0 = s13(prm[4]), s13(prm[5])
    H, V = s13(prm[6]), s13(prm[7])
    lo = (vr & 0xFF).astype(np.int64)
    hi = (vr >> 8).astype(np.int64)
    ys, xs = np.mgrid[0:224, 0:256]
    dx = xs + H - X0
    dy = ys + V - Y0
    tx = ((A * dx + B * dy) >> 8) + X0
    ty = ((C * dx + D * dy) >> 8) + Y0
    out = (tx < 0) | (tx >= 1024) | (ty < 0) | (ty >= 1024)
    tx &= 1023
    ty &= 1023
    tile = lo[(ty >> 3) * 128 + (tx >> 3)]
    px = hi[tile * 64 + (ty & 7) * 8 + (tx & 7)]
    return np.where(out, 0, px)


def ship_frames(vrs, m7list, genesis_tile_bytes, max_tiles):
    """vrs: VRAM de cada mapa de la nave (prm[8]). Cada cuadro de la nave: celdas de 8x8 de la pantalla con algo dibujado y sus tiles (sin repetir
    dentro del cuadro). Devuelve el bloque para la página del SSF2 y la tabla de desplazamientos.
    Registro: celdas (u16), tiles (u16), celdas x u16 (fila * 32 + columna), celdas x u16 (tile
    dentro del cuadro), tiles x 32 bytes. Un cuadro con más de max_tiles tiles queda vacío (ncells = 0
    y la Genesis deja el cuadro anterior)."""
    blob = bytearray()
    offs = []
    skipped = []
    colors = set()
    for i, prm in enumerate(m7list):
        px = render_m7(vrs[prm[8]], prm)
        cells = px.reshape(28, 8, 32, 8).transpose(0, 2, 1, 3)
        pos, tid, tiles, index = [], [], [], {}
        for cy in range(28):
            for cx in range(32):
                c = cells[cy, cx]
                if not c.any():
                    continue
                colors |= set(int(x) for x in np.unique(c))
                g = np.where(c > 0, c - 80, 0)
                assert g.max() <= 15 and (c[c > 0] >= 81).all(), 'la nave usa colores fuera de CGRAM 81-95'
                b = genesis_tile_bytes(g)
                if b not in index:
                    index[b] = len(tiles)
                    tiles.append(b)
                pos.append(cy * 32 + cx)
                tid.append(index[b])
        if len(tiles) > max_tiles:
            skipped.append(i)
            pos, tid, tiles = [], [], []
        while len(blob) % 4:
            blob.append(0)
        offs.append(len(blob))
        blob += struct.pack('>HH', len(pos), len(tiles))
        blob += struct.pack(f'>{len(pos)}H', *pos) + struct.pack(f'>{len(tid)}H', *tid)
        while len(blob) % 4:
            blob.append(0)
        blob += b''.join(tiles)
    return bytes(blob), offs, skipped, sorted(colors)


def parse_spritemap(rom, pp):
    o = lorom(pp)
    n = rom[o] | rom[o + 1] << 8
    ents = []
    for i in range(n):
        e = rom[o + 2 + i * 5:o + 7 + i * 5]
        xw = e[0] | e[1] << 8
        x = (xw & 0x1FF) - (0x200 if xw & 0x100 else 0)
        y = e[2] - 256 if e[2] >= 128 else e[2]
        ents.append(dict(x=x, y=y, big=bool(xw & 0x8000), ch=e[3] | (e[4] & 1) << 8,
                         hf=bool(e[4] & 0x40), vf=bool(e[4] & 0x80), prio=(e[4] >> 4) & 3))
    return ents


def compose_spritemap(rom, pp, chr_, obj_pixels):
    """Imagen del spritemap como la dibuja el SNES (la primera entrada queda encima): índice de CGRAM
    por píxel. obj_pixels(ch, big, pal) -> matriz 8x8 o 16x16 con índices de CGRAM (0 transparente).
    Devuelve (imagen, x0, y0) con x0/y0 = posición de la esquina respecto al origen del spritemap."""
    ents = parse_spritemap(rom, pp)
    pal = (chr_ >> 9) & 7
    xs = [e['x'] for e in ents] + [e['x'] + (16 if e['big'] else 8) for e in ents]
    ys = [e['y'] for e in ents] + [e['y'] + (16 if e['big'] else 8) for e in ents]
    x0, y0 = min(xs), min(ys)
    img = np.zeros((max(ys) - y0, max(xs) - x0), np.int64)
    prio = set()
    for e in reversed(ents):
        m = obj_pixels(e['ch'], e['big'], pal)
        if e['hf'] ^ bool(chr_ & 0x4000):
            m = m[:, ::-1]
        if e['vf'] ^ bool(chr_ & 0x8000):
            m = m[::-1]
        h, w = m.shape
        sub = img[e['y'] - y0:e['y'] - y0 + h, e['x'] - x0:e['x'] - x0 + w]
        img[e['y'] - y0:e['y'] - y0 + h, e['x'] - x0:e['x'] - x0 + w] = np.where(m > 0, m, sub)
        prio.add(e['prio'] | (chr_ >> 12 & 3))
    assert len(prio) == 1, f'{pp:06X}: prioridades mezcladas'
    return img, x0, y0, prio.pop()


def cover(img, alpha=0.1):
    """Cubre los píxeles de la imagen con sprites de la Genesis (de 1-4 x 1-4 tiles, en cualquier
    posición de píxel): se toma el primer píxel aún sin cubrir (de arriba abajo, de izquierda a derecha)
    y, entre las cajas que lo tienen en su borde de arriba, la que cubre más píxeles nuevos (a igualdad,
    la más chica). Devuelve [(x, y, w, h)] con x, y en píxeles de la imagen y w, h en tiles, y la imagen
    con margen para recortar los tiles."""
    M = 32
    Hh, Ww = img.shape
    pad = np.zeros((Hh + 2 * M, Ww + 2 * M), img.dtype)
    pad[M:M + Hh, M:M + Ww] = img
    todo = pad > 0
    out = []
    while todo.any():
        ys, xs = np.nonzero(todo)
        y, x = ys[0], xs[0]                 # el primero en orden de filas
        best = None
        for w in range(1, 5):
            for h in range(1, 5):
                for left in range(x - w * 8 + 1, x + 1):
                    n = int(todo[y:y + h * 8, left:left + w * 8].sum())
                    key = (n - alpha * w * h * 64, -w * h)
                    if best is None or key > best[0]:
                        best = (key, left, w, h)
        _, left, w, h = best
        todo[y:y + h * 8, left:left + w * 8] = False
        out.append((left - M, y - M, w, h))
    return out, pad, M
