#!/usr/bin/env python3
"""MegaMetroid — la luz de la explosión de la bomba de poder para la Genesis (docs/58).

En el SNES la explosión es una ventana por línea (power_bomb_explosion_left_hdma / _right_hdma, 192 filas por
distancia al centro) donde se suma un color fijo (COLDATA). Las formas siguen siempre la misma secuencia
(HdmaobjPreInstr_PowerBombExplode_*, banco $88; la lista de instrucciones $88:8ACE):
  1. destello blanco: el cuarto de círculo g_byte_88A206 escalado por HIBYTE(power_bomb_pre_explosion_flash_radius)
     (PreExplosionWhite $88:90DF, CalculatePowerBombHdmaScaled_* $88:8CC6...);
  2. destello amarillo: formas ya escaladas desde $88:9F06, 192 bytes cada una (PreExplosionYellow $88:91A8);
  3. explosión amarilla: el mismo cuarto escalado por HIBYTE(power_bomb_explosion_radius) (ExplosionYellow $88:8DE9);
  4. explosión blanca: formas ya escaladas desde $88:9246 (ExplosionWhite $88:8EB2);
  5. resplandor: la última forma, el color baja (Stage5_Afterglow $88:8B98).
Las ya escaladas cortan en el primer 0 (CalculatePowerBombHdma_OnScreen $88:8C12) y dejan lo del cuadro anterior: por
eso se simula la secuencia entera, como el SNES, y no cada forma suelta.

La forma no depende de dónde explota: aquí se guarda, para cada una (clave: etapa y radio o número de la forma; el
motor anota cuál armó en nt_pb_shape, hdmaobj.c), un cuarto del óvalo (abajo a la derecha, filas d desde el centro y
columnas k desde el centro) cubierto con sprites de hasta 4 x 4 tiles; la Genesis pone los otros tres cuartos con los
giros. Por bandas de 32 líneas: las columnas llenas en toda la banda van con tiles llenos fijos; las del borde, con
sus tiles (el color 14 de la línea 3: "aclarar" del modo sombra/luz), seguidos por caja: la Genesis sube con una DMA
los de cada caja que se ve (md_cone.c). Los tiles van en la última página de los niveles (tables.h, NT_LEVEL_PAGES);
las tablas, en la parte común. Lo que no entra en los sprites o en la VRAM se hace con el tinte de los colores.

Uso (desde make_data.sh): mkpbomb.py <rom> <carpeta de salida>  ->  md_pbomb_data.c, md_pbomb_data.h
"""
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "genesis_convert"))
import mkmd  # noqa: E402
from convert_rooms import genesis_tile_bytes  # noqa: E402

PB_COLOR = 14          # modo sombra/luz: aclarar
SHAPE_SCALED = 0xA206  # g_byte_88A206: media anchura por punto (j = 96..127) y alto (j + 32)
SHAPE_TOP = 0xA286     # g_byte_88A286
PRE_YELLOW0, PRE_YELLOW_END = 0x9F06, 0xA206
WHITE0, WHITE_END = 0x9246, 0x9F06   # addr_kPowerBombExplosionShapeDef0
# etapas (nt_pb_shape = etapa << 12 | parámetro)
PW, PY, EY, EW = 1, 2, 3, 4


def simulate(rom):
    """[(clave, media anchura por fila d o -1)] de cada cuadro de la secuencia, con el centro en la mitad (las
    medias anchuras sin recortar por la pantalla)"""
    B = lambda a: rom.b(0x880000 | a)
    C = 128
    L, R, W = [255] * 256, [0] * 256, [0] * 256

    def scaled(k, j, m):                       # CalcPowerBombHdmaScaled (en pantalla)
        while True:
            r20 = (m * B(SHAPE_SCALED + j + 32)) >> 8
            reg = (m * B(SHAPE_SCALED + j)) >> 8
            right, left = min(255, C + reg), max(0, C - reg)
            while True:
                L[k], R[k], W[k] = left, right, reg
                if k == r20:
                    break
                k = (k - 1) & 0xFF
            j = (j + 1) & 0xFF
            if j & 0x80:
                return k, left, right, reg

    def scaled_frame(m):                       # PowerBombScaledCircle
        kk = (m * B(SHAPE_TOP)) >> 8
        i, left, right, reg = scaled(kk, 96, m)
        while True:
            L[i], R[i], W[i] = left, right, reg
            i = (i - 1) & 0xFF
            if i & 0x80:
                break
        for v in range(kk + 1, 0xC0):
            L[v], R[v] = 255, 0

    def from_rom(ptr):                         # CalculatePowerBombHdma_OnScreen
        for k in range(192):
            w = B(ptr + k)
            if not w:
                break
            L[k], R[k], W[k] = max(0, C - w), min(255, C + w), w

    def shape():
        return tuple(W[d] if L[d] <= R[d] else -1 for d in range(192))
    out = []
    flash, speed = 0x400, 0x3000               # Hdmaobj_PreExplodeWhite
    while True:
        scaled_frame(flash >> 8)
        out.append((PW << 12 | flash >> 8, shape()))
        flash += speed
        if flash < 0x9200:
            speed -= 0x80
        else:
            break
    ptr = PRE_YELLOW0
    while True:
        from_rom(ptr)
        out.append((PY << 12 | (ptr - PRE_YELLOW0) // 192, shape()))
        ptr += 192
        if speed + flash < 0x10000:
            flash += speed
            speed -= 0x80
        if ptr == PRE_YELLOW_END:
            break
    rad, speed = 0x400, 0                      # Hdmaobj_ExplodeYellow
    while True:
        scaled_frame(rad >> 8)
        out.append((EY << 12 | rad >> 8, shape()))
        rad += speed
        if rad < 0x8600:
            speed += 0x30
        else:
            break
    ptr = WHITE0
    while True:
        from_rom(ptr)
        out.append((EW << 12 | (ptr - WHITE0) // 192, shape()))
        ptr += 192
        if ptr == WHITE_END:
            break
    return out


def quadrant(s):
    """el cuarto de abajo a la derecha: img[d, k] = 1 si k <= media anchura de la fila d"""
    H = max([d for d in range(192) if s[d] >= 0] + [-1]) + 1
    W = max(s) + 1
    img = np.zeros((max(8, (H + 7) // 8 * 8), max(8, (W + 7) // 8 * 8)), np.uint8)
    for d in range(H):
        if s[d] >= 0:
            img[d, :s[d] + 1] = 1
    return img


def cover(img):
    """cajas (columna, fila, ancho, alto en tiles, llena) por bandas de 4 filas de tiles: primero las columnas llenas
    en toda la banda (de a 4), después las del borde"""
    Hc, Wc = img.shape[0] // 8, img.shape[1] // 8
    cell = img.reshape(Hc, 8, Wc, 8).sum(axis=(1, 3))
    boxes = []
    for by in range(0, Hc, 4):
        rows = cell[by:by + 4]
        if not rows.any():
            continue
        nfull = 0
        while nfull < Wc and (rows[:, nfull] == 64).all():
            nfull += 1
        for bx in range(0, nfull, 4):
            boxes.append((bx, by, min(4, nfull - bx), rows.shape[0], True))
        c1 = int(np.nonzero(rows.sum(axis=0))[0][-1]) + 1
        for bx in range(nfull, c1, 4):
            rr = np.nonzero(rows[:, bx:bx + 4].sum(axis=1))[0]
            boxes.append((bx, by, min(4, c1 - bx), int(rr[-1]) + 1, False))
    return boxes


def build(rom):
    shapes = {}
    for key, s in simulate(rom):
        shapes.setdefault(key, s)
    keys, shp, boxw, tiles = [], [], [], bytearray()
    for key in sorted(shapes):
        img = quadrant(shapes[key])
        boxes = cover(img)
        b0 = len(boxw) // 3
        for (bx, by, w, h, full) in boxes:
            boxw += [bx << 8 | by, ((w - 1) << 2 | (h - 1)) << 8, 0xFFFF if full else len(tiles) // 32]
            if not full:
                for cx in range(w):                    # orden de la Genesis: columna por columna
                    for cy in range(h):
                        px = img[(by + cy) * 8:(by + cy) * 8 + 8, (bx + cx) * 8:(bx + cx) * 8 + 8] * PB_COLOR
                        tiles += genesis_tile_bytes(px.astype(np.int32))
        keys.append(key)
        shp += [b0, len(boxes)]
    assert len(tiles) // 32 < 0xFFFF
    return keys, shp, boxw, bytes(tiles), len(shapes)


def main():
    rom = mkmd.Rom(sys.argv[1])
    out = Path(sys.argv[2])
    keys, shp, boxw, tiles, total = build(rom)
    import re
    th = (HERE.parent.parent / "res" / "gen" / "engine" / "tables.h").read_text()
    page = 7 + int(re.search(r'#define NT_LEVEL_PAGES (\d+)', th).group(1))   # la última de los niveles
    H = ['/* Generado por tools/conv/mkpbomb.py desde la ROM del SNES: no editar. */',
         '#ifndef MD_PBOMB_DATA_H', '#define MD_PBOMB_DATA_H', '#include <stdint.h>',
         '/* luz de la bomba de poder (docs/58): formas por clave (nt_pb_shape), ordenadas. kPbShape: (primera caja,',
         '   cajas). kPbBox: (columna << 8 | fila, tamaño del VDP, primer tile en kPbTiles o 0xFFFF: llena), del cuarto',
         '   de abajo a la derecha, en tiles desde el centro. kPbTiles: en la página PB_PAGE del SSF2 */',
         f'#define PB_SHAPES {len(keys)}', f'#define PB_PAGE {page}',
         f'extern const uint16_t kPbKey[{max(1, len(keys))}], kPbShape[{max(2, len(shp))}], kPbBox[{max(3, len(boxw))}];',
         f'extern const uint8_t kPbTiles[{max(32, len(tiles))}];', '#endif']
    C = ['/* Generado por tools/conv/mkpbomb.py desde la ROM del SNES: no editar. */', '#include "md_pbomb_data.h"',
         f'const uint16_t kPbKey[{max(1, len(keys))}] = {{' + ','.join('0x%X' % k for k in keys or [0]) + '};',
         f'const uint16_t kPbShape[{max(2, len(shp))}] = {{' + ','.join(str(x) for x in shp or [0] * 2) + '};',
         f'const uint16_t kPbBox[{max(3, len(boxw))}] = {{' + ','.join('0x%X' % x for x in boxw or [0] * 3) + '};',
         f'__attribute__((section(".bank{page}"))) const uint8_t kPbTiles[{max(32, len(tiles))}] __attribute__((aligned(2))) = {{' +
         ','.join(str(x) for x in tiles or bytes(32)) + '};']
    (out / "md_pbomb_data.h").write_text('\n'.join(H) + '\n')
    (out / "md_pbomb_data.c").write_text('\n'.join(C) + '\n')
    print(f"bomba de poder: {total} formas, {len(tiles) // 32} tiles (página {page}), {len(boxw) // 3} cajas", file=sys.stderr)


if __name__ == "__main__":
    main()
