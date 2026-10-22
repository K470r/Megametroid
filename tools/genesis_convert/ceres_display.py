#!/usr/bin/env python3
"""Salas de Ceres con modo 7 (o con fondo que no sale del level data): imagen de fondo propia.

En el SNES, dos salas de Ceres no se dibujan con el level data normal:

  * Ceres Shaft ($8F:DF45): BG en modo 7 durante toda la sala (DoorCode_CeresElevatorShaft pone
    BGMODE 7, M7X=128, M7Y=1008). La VRAM del tileset ($C0:E22A, 32 KB) va intercalada: byte bajo
    de cada palabra = mapa de 128x128 celdas, byte alto = píxeles 8bpp de los tiles. El píxel
    (x, y) del plano es el píxel (x, y) de la sala (256x768); fuera de la sala el plano repite un
    bloque de 16x16 (tiles 152-155). Paleta: la del tileset ($C2:C104 al llegar, $C2:C1E3 en la
    huida). En la huida (ceres_status & 0x8000) RoomCode_CeresElevatorShaft balancea la torre con
    la tabla $89:AD5F (ver main.c: se imita con scroll por línea y por columna).
  * Ceres Ridley Boss Room ($8F:E0B5): RoomCode_CeresColorMathHdma_BgBase pone BG12NBA=$66, así
    la capa 2 usa los tiles de VRAM $C000 (bytes), que vienen de la ROM $B0:A000: piso, marco de
    la puerta, columnas y el escalón de la izquierda; el fondo es el "haze" (FxTypeFunc_2C_Haze): suma
    de color azul por línea de pantalla, nivel = clamp((y - 56) / 8, 0, 15) (medido contra el
    SNES real: el piso coincide píxel a píxel). La cámara de esta sala no se mueve (y = 0), así
    que el degradado se hornea en los tiles, con tramado ordenado para suavizar los 8 niveles de
    azul de la Genesis.

Las imágenes se convierten a tiles Genesis + <= 2 paletas + un mapa de "bloques de pantalla"
(16x16, un id por bloque distinto) con el mismo formato de metatiles que el resto de las salas.
"""
import struct
from pathlib import Path

import numpy as np

from convert_rooms import (snes_to_genesis, gen_to_cram, genesis_tile_bytes, pack_palettes,
                           color_dist, DAC_LEVELS)

SHAFT_ROOM, RIDLEY_ROOM = 0xDF45, 0xE0B5


# ------------------------------------------------------------------ puertas de Ceres (enemigo $E23F)
# En Ceres las puertas son enemigos (CeresDoor, banco $A6), no bloques: en el SNES son sprites.
# Mientras no exista el sistema de enemigos se dibujan fijas dentro del fondo, cerradas, con el
# spritemap que muestra su lista de instrucciones al aparecer (CeresDoor_Init, $A6:F52C):
#   Ceres Shaft: parámetro 2 en (232, 631) -> $A6:F921 (el parámetro 1/4 en (224, 639) es invisible)
#   Ridley:      parámetro 3 en (16, 127)  -> $A6:FAA7
# Tiles: los del enemigo ($E23F: tile_data, bit 15 de tamaño -> desde el tile de OBJ $D0) y, para
# el parámetro 2, 1 KB extra de $B0:C400 en el tile $100 (CeresDoor_Func_1). Paleta: $A6:F4EC,
# o $A6:F50C con ceres_status (huida). Comparado con el SNES: mismos píxeles (menos el haze).
def _lorom(rom, a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def _w(rom, a):
    return struct.unpack_from("<H", rom, _lorom(rom, a))[0]


def _tile4(b):
    o = np.zeros((8, 8), int)
    for y in range(8):
        p = (b[2 * y], b[2 * y + 1], b[16 + 2 * y], b[17 + 2 * y])
        for x in range(8):
            s = 7 - x
            o[y, x] = sum(((p[k] >> s) & 1) << k for k in range(4))
    return o


def _door_obj_tiles(rom):
    ed = 0xA00000 + 0xE23F
    size = _w(rom, ed) & 0x7FFF
    src = _lorom(rom, _w(rom, ed + 0x36) | rom[_lorom(rom, ed + 0x38)] << 16)
    vram = {0xD0 + i: rom[src + 32 * i:src + 32 * i + 32] for i in range(size // 32)}
    s2 = _lorom(rom, 0xB0C400)
    vram.update({0x100 + i: rom[s2 + 32 * i:s2 + 32 * i + 32] for i in range(32)})
    return vram


def draw_ceres_door(rom, img, spritemap, cx, cy, escape=False):
    """Dibuja el spritemap $A6:spritemap centrado en (cx, cy) sobre img (colores SNES)."""
    vram = _door_obj_tiles(rom)
    pal_addr = 0xA6F50C if escape else 0xA6F4EC
    pal = [_w(rom, pal_addr + 2 * i) for i in range(16)]
    a = 0xA60000 | spritemap
    n = _w(rom, a)
    ents = [struct.unpack_from("<HbH", rom, _lorom(rom, a) + 2 + 5 * i) for i in range(n)]
    for x, y, t in reversed(ents):                     # el primero del spritemap queda encima
        sz = 16 if x & 0x8000 else 8
        xx = x & 0x1FF
        if xx >= 256:
            xx -= 512
        tn = t & 0x1FF
        blk = np.zeros((sz, sz), int)
        for ty in range(sz // 8):
            for tx in range(sz // 8):
                blk[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = _tile4(vram.get(tn + ty * 16 + tx, bytes(32)))
        if t & 0x4000:
            blk = blk[:, ::-1]
        if t & 0x8000:
            blk = blk[::-1, :]
        for yy in range(sz):
            for x2 in range(sz):
                c = blk[yy, x2]
                Y, X = cy + y + yy, cx + xx + x2
                if c and 0 <= Y < img.shape[0] and 0 <= X < img.shape[1]:
                    img[Y, X] = pal[c]


def _blob(ext, addr):
    return next((Path(ext) / "blobs" / "decompressed").glob(f"{addr:06X}_*.bin")).read_bytes()


def _pal128(ext, addr):
    p = _blob(ext, addr)[:256]
    return list(struct.unpack("<128H", p))


# ------------------------------------------------------------------ imágenes (color SNES por píxel)
def shaft_image(ext, pal_addr, rom=None, escape=False):
    """Plano de modo 7 de Ceres Shaft: (sala 768x256, bloque de relleno 16x16). -1 = transparente.
    Con rom, dibuja además la puerta (enemigo) cerrada."""
    b = np.frombuffer(_blob(ext, 0xC0E22A), np.uint8)
    mp, px = b[0::2].astype(int), b[1::2].astype(int)
    pal = np.array(_pal128(ext, pal_addr) + [0] * 128)

    def region(x0, y0, w, h):
        Y, X = np.mgrid[y0:y0 + h, x0:x0 + w]
        t = mp[((Y >> 3) & 127) * 128 + ((X >> 3) & 127)]
        idx = px[t * 64 + (Y & 7) * 8 + (X & 7)]
        return np.where(idx > 0, pal[idx], -1)
    room = region(0, 0, 256, 768)
    fill = region(256, 0, 16, 16)
    # el relleno se repite cada 16 píxeles en todo el plano fuera de la sala
    assert (region(256, 0, 256, 256) == np.tile(fill, (16, 16))).all()
    if rom is not None:
        draw_ceres_door(rom, room, 0xF921, 232, 631, escape)
    return room, fill


def ridley_image(ext, rom, room_dir, tile_table_addr):
    """Ceres Ridley Boss Room (estado normal): capa 2 completa + puerta + haze horneado. Valores > 0x7FFF
    marcan píxeles del degradado (se tramarán): 0x10000 | nivel de azul 0-15."""
    raw = _blob(ext, tile_table_addr)
    pal = _pal128(ext, 0xC2C104)
    d = (Path(ext) / "rooms" / room_dir / "state1_Default_level_data.bin").read_bytes()
    size = d[0] | d[1] << 8
    n = size // 2
    w2 = struct.unpack_from(f"<{n}H", d, 2 + size + n)
    chars = rom[0x182000:0x184000]            # $B0:A000 -> VRAM $C000 (BG12NBA = $66)

    def tile(t):
        bb = chars[t * 32:t * 32 + 32]
        o = np.zeros((8, 8), int)
        for y in range(8):
            p = (bb[2 * y], bb[2 * y + 1], bb[16 + 2 * y], bb[17 + 2 * y])
            for x in range(8):
                s = 7 - x
                o[y, x] = sum(((p[k] >> s) & 1) << k for k in range(4))
        return o
    img = np.full((256, 256), -1, int)
    for by in range(16):                      # toda la capa 2: piso, marco de la puerta, columnas y escalón
        for bx in range(16):
            ww = w2[by * 16 + bx]
            q = list(struct.unpack_from("<4H", raw, (ww & 0x3FF) * 8))
            g = [[q[0], q[1]], [q[2], q[3]]]
            if ww & 0x400:
                g = [[r[1] ^ 0x4000, r[0] ^ 0x4000] for r in g]
            if ww & 0x800:
                g = [[c ^ 0x8000 for c in g[1]], [c ^ 0x8000 for c in g[0]]]
            for yy in range(2):
                for xx in range(2):
                    e = g[yy][xx]
                    tp = tile(e & 0x3FF)
                    if e & 0x4000:
                        tp = tp[:, ::-1]
                    if e & 0x8000:
                        tp = tp[::-1, :]
                    col = np.where(tp > 0, np.array(pal)[((e >> 10) & 7) * 16 + tp], -1)
                    img[by * 16 + yy * 8:by * 16 + yy * 8 + 8, bx * 16 + xx * 8:bx * 16 + xx * 8 + 8] = col
    draw_ceres_door(rom, img, 0xFAA7, 16, 127)       # puerta de la izquierda (parámetro 3)
    out = np.zeros_like(img)
    for y in range(256):
        # el SNES muestra la línea de sala y+1 en la línea de pantalla y: el haze va por pantalla
        hz = min(15, max(0, (y - 1 - 56) // 8))
        for x in range(256):
            c = img[y, x]
            if c < 0:
                out[y, x] = 0x10000 | hz
            else:
                out[y, x] = (c & 0x3FF) | (min(31, ((c >> 10) & 31) + hz) << 10)
    return out


# ------------------------------------------------------------------ imagen -> datos Genesis
BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def _gen(c, mode="dac"):
    if c < 0:
        return None
    if c >= 0x10000:                            # degradado del haze: azul 5 bits = nivel
        lvl = c & 0xFFFF
        return ("haze", lvl)
    return snes_to_genesis(c, mode)


def _haze_pixel(lvl, x, y):
    """Nivel de azul SNES (0-15, en 5 bits) -> color Genesis con tramado ordenado 4x4 entre los dos
    niveles del DAC más cercanos (en brillo)."""
    v = (lvl << 3) | (lvl >> 2)                 # 5 -> 8 bits
    lo = max(i for i in range(8) if DAC_LEVELS[i] <= v)
    hi = min(lo + 1, 7)
    if hi == lo or DAC_LEVELS[hi] == DAC_LEVELS[lo]:
        return (0, 0, lo)
    f = (v - DAC_LEVELS[lo]) / (DAC_LEVELS[hi] - DAC_LEVELS[lo])
    return (0, 0, hi if f > BAYER4[y & 3, x & 3] else lo)


def image_to_display(img, fill=None, npal=2, backdrop=(0, 0, 0), mode="dac"):
    """img: matriz HxW de colores SNES (-1 transparente, 0x10000|n degradado). fill: bloque 16x16
    para fuera de la sala (o None). Devuelve tiles, cram, tabla de metatiles (formato de
    make_samus_play_data: entrada 0 vacía, 4 variantes x 4 celdas), ids y mapa de bloques."""
    H, W = img.shape
    blocks_src = [img[by * 16:by * 16 + 16, bx * 16:bx * 16 + 16] for by in range(H // 16) for bx in range(W // 16)]
    if fill is not None:
        blocks_src.append(fill)
    # colores Genesis por píxel
    gimg = []
    for bi, blk in enumerate(blocks_src):
        by, bx = divmod(bi, W // 16) if bi < (H // 16) * (W // 16) else (0, 0)
        g = [[None] * 16 for _ in range(16)]
        for y in range(16):
            for x in range(16):
                c = int(blk[y, x])
                if c >= 0x10000:
                    g[y][x] = _haze_pixel(c & 0xFF, bx * 16 + x, by * 16 + y)
                elif c >= 0:
                    g[y][x] = snes_to_genesis(c, mode)
        gimg.append(g)
    # conjuntos de colores por tile 8x8 (sin el fondo)
    def tile_cols(g, ty, tx):
        return {g[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)} - {None, backdrop}
    items = []
    for bi, g in enumerate(gimg):
        for ty in range(2):
            for tx in range(2):
                items.append(((bi, ty, tx), tile_cols(g, ty, tx)))
    merges = {}
    bins = pack_palettes(items, npal)
    while bins is None:                          # fusionar los dos colores más parecidos hasta que quepa
        allc = sorted(set().union(*[{merges.get(c, c) for c in cs} for _, cs in items]))
        best = min(((color_dist(allc[i], allc[j]), allc[i], allc[j])
                    for i in range(len(allc)) for j in range(i + 1, len(allc))))
        _, a, b = best
        for k, v in list(merges.items()):
            if v == b:
                merges[k] = a
        merges[b] = a
        bins = pack_palettes([(k, {merges.get(c, c) for c in cs}) for k, cs in items], npal)
    assign = {}
    pals = []
    for pi, (cs, keys) in enumerate(bins):
        pals.append(sorted(cs, key=lambda g: (sum(DAC_LEVELS[v] for v in g), g)))
        for k in keys:
            assign[k] = pi
    # tiles deduplicados con flips
    tile_list, tile_index = [bytes(32)], {bytes(32): 0}

    def add_tile(px):
        for v, fh, fv in ((px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1, :], 0, 1), (px[::-1, ::-1], 1, 1)):
            key = genesis_tile_bytes(v)
            if key in tile_index:
                return tile_index[key], fh, fv
        key = genesis_tile_bytes(px)
        tile_index[key] = len(tile_list)
        tile_list.append(key)
        return tile_index[key], 0, 0
    block_ids, blocks = {}, []
    disp = []
    for bi, g in enumerate(gimg):
        cells = []
        for ty in range(2):
            for tx in range(2):
                pi = assign[(bi, ty, tx)]
                pl = pals[pi]
                px = np.zeros((8, 8), np.uint8)
                for y in range(8):
                    for x in range(8):
                        c = g[ty * 8 + y][tx * 8 + x]
                        if c is None or c == backdrop:
                            continue
                        c = merges.get(c, c)
                        px[y, x] = pl.index(c) + 1
                t, fh, fv = add_tile(px)
                cells.append(((pi << 13) | (fv << 12) | (fh << 11) | t) if t else 0)
        q = tuple(cells)                         # arriba-izq, arriba-der, abajo-izq, abajo-der
        if q not in block_ids:
            block_ids[q] = len(blocks)
            blocks.append(q)
        disp.append(block_ids[q])
    fill_id = disp.pop() if fill is not None else None
    assert len(blocks) <= 1024, len(blocks)
    cram = []
    for pl in pals:
        words = [gen_to_cram(backdrop)] + [gen_to_cram(c) for c in pl]
        cram += words + [0] * (16 - len(words))
    # tabla de metatiles: sin flips de bloque (las 4 variantes iguales), celdas en orden de columnas
    table = bytes(32) + b"".join(struct.pack(">4H", q[0], q[2], q[1], q[3]) * 4 for q in blocks)
    return {"tiles": b"".join(tile_list), "ntiles": len(tile_list), "cram": cram, "npal": len(pals),
            "meta_ids": list(range(len(blocks))), "meta_cells": table,
            "disp": struct.pack(f"<{len(disp)}H", *disp), "fill": fill_id, "merges": len(merges)}


def render_display(res, W, H):
    """Render de control (RGB) desde los datos Genesis."""
    tiles = res["tiles"]
    cram = res["cram"]
    ids = struct.unpack(f"<{len(res['disp']) // 2}H", res["disp"])
    tbl = res["meta_cells"]
    out = np.zeros((H, W, 3), np.uint8)

    def rgb(cw):
        return (DAC_LEVELS[(cw >> 1) & 7], DAC_LEVELS[(cw >> 5) & 7], DAC_LEVELS[(cw >> 9) & 7])
    for bi, m in enumerate(ids):
        by, bx = divmod(bi, W // 16)
        c = struct.unpack_from(">4H", tbl, 32 + m * 32)      # variante 0: tl, bl, tr, br
        for k, (dx, dy) in enumerate(((0, 0), (0, 8), (8, 0), (8, 8))):
            w = c[k]
            t = w & 0x7FF
            tb = tiles[t * 32:t * 32 + 32]
            px = np.array([[tb[y * 4 + x // 2] >> (4 if x % 2 == 0 else 0) & 15 for x in range(8)] for y in range(8)])
            if w & 0x800:
                px = px[:, ::-1]
            if w & 0x1000:
                px = px[::-1, :]
            pal = (w >> 13) & 3
            for y in range(8):
                for x in range(8):
                    out[by * 16 + dy + y, bx * 16 + dx + x] = rgb(cram[pal * 16 + px[y, x]] if px[y, x] else cram[0])
    return out
