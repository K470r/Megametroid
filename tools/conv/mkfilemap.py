#!/usr/bin/env python3
"""MegaMetroid — el mapa de la partida (docs/67): el planeta con las áreas.

El SNES (FileSelectMap, $81:9E3E) muestra el planeta con el BG1 ($81:B71A, tiles $8E:8000) y le suma el BG3 del área
($81:BF1A + área * 0x800, tiles 2bpp $8E:D600) con la matemática de color (CGADSUB = 0x25). Las paletas: kMenuPalettes
($8E:E400) con los colores de las áreas apagadas y la de la partida encendida (Load(In)activeAreaMapForegroundColors,
$81:A40E / $81:A4E6). La Genesis no suma capas: aquí se arma la imagen ya sumada, una por área (sin el modo de
depuración el área no se cambia: es la de la partida).
"PLANET ZEBES" y los nombres de las áreas son spritemaps del menú ($82:C569 desde g_word_82C749 = $82:C749) con los
tiles de OBJ $B6:C000 y las paletas de OBJ 0 (encendido) y 1 (apagado): solo los colores 13 y 14; en la Genesis van en
la línea 3, el apagado con los índices 11 y 12.
El marco y el mapa del área son los de la pausa (mkpause.py). Todo esto va en la página FM_PAGE del SSF2.
Uso: mkfilemap.py rom.smc carpeta_salida [captura del SNES del área 0 sin extensión, para comparar]"""
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "genesis_convert"))
from make_menu_data import t4, t2, convert_image  # noqa: E402
from convert_rooms import snes_to_genesis, gen_to_cram, genesis_tile_bytes  # noqa: E402

ROM = Path(sys.argv[1]).read_bytes()
if len(ROM) % 0x8000:
    ROM = ROM[len(ROM) % 0x8000:]
OUT = Path(sys.argv[2])
FM_PAGE = 23
FM_TILE_BASE = 816          # tiles del planeta (los de la pausa: 256-805, mkpause.py)
AREAS = 6


def off(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def w(a):
    return struct.unpack_from("<H", ROM, off(a))[0]


def words(a, n):
    return list(struct.unpack_from("<%dH" % n, ROM, off(a)))


MENU_PAL = words(0x8EE400, 256)


def area_palette(cur):
    pal = list(MENU_PAL)

    def load(j):
        while w(0x81A4E6 + j) != 0xFFFF:
            src, dst = w(0x81A4E6 + j), w(0x81A4E6 + j + 2)
            for k in range(5):
                pal[(dst >> 1) + k] = w(0x81A40E + src + 2 * k)
            j += 4
    for a in range(6):                                  # FileSelectMap_1: de 0 a 5, la activa encendida
        load([0, 0xA, 0x10, 0x16, 0x24, 0x2A][a] if a == cur else [0x30, 0x3A, 0x40, 0x46, 0x54, 0x5A][a])
    pal[14] = pal[30] = 0                               # FileSelectMap_0/1: los colores 14 y 30 van a negro
    return pal


BG1_TILES = ROM[off(0x8E8000):off(0x8E8000) + 0x5600]
BG3_TILES = ROM[off(0x8ED600):off(0x8ED600) + 0x600]


def layer(mp, tiles, bpp):
    img = np.full((224, 256), -1, int)
    for ty in range(28):
        for tx in range(32):
            e = mp[ty * 32 + tx]
            t = e & 0x3FF
            sz = 32 if bpp == 4 else 16
            if t * sz + sz > len(tiles):
                continue
            px = (t4 if bpp == 4 else t2)(tiles, t * sz).astype(int)
            if e & 0x4000:
                px = px[:, ::-1]
            if e & 0x8000:
                px = px[::-1]
            img[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = np.where(px > 0, ((e >> 10) & 7) * (1 << bpp) + px, -1)
    return img


def compose(area):
    pal = np.array(area_palette(area))
    main = layer(words(0x81B71A, 1024), BG1_TILES, 4)
    sub = layer(words(0x81BF1A + area * 0x800, 1024), BG3_TILES, 2)
    mc = np.where(main >= 0, pal[np.clip(main, 0, 255)], pal[0])
    sc = np.where(sub >= 0, pal[np.clip(sub, 0, 255)], 0)      # (sin BG3: el color fijo, negro)
    out = 0
    for sh in (0, 5, 10):
        out = out | np.minimum(((mc >> sh) & 31) + ((sc >> sh) & 31), 31) << sh
    return out


if len(sys.argv) > 3:
    p = Path(sys.argv[3] + ".ppm").read_bytes()
    e = 0
    for _ in range(3):
        e = p.index(b"\n", e) + 1
    a = np.frombuffer(p[e:], np.uint8).reshape(224, 256, 3).astype(int) >> 3
    shot = a[:, :, 0] | a[:, :, 1] << 5 | a[:, :, 2] << 10
    d = np.roll(compose(0), -1, 0) != shot              # (el SNES muestra la línea 1 del BG arriba de todo)
    d[0:24] = False                                     # (los sprites del título y del nombre del área)
    d[36:48, 40:120] = False
    print("planeta: %d píxeles distintos de la captura (sin los sprites)" % int(d.sum()))

C = ['/* Generado por tools/conv/mkfilemap.py (docs/67): el mapa de la partida desde tu ROM. No editar. */',
     '#include "md_fmap_data.h"',
     '#ifdef NT_SSF2', '#define FM_ATTR __attribute__((section(".bank%d")))' % FM_PAGE, '#else', '#define FM_ATTR',
     '#endif']
total = 0
maxt = 0
for a in range(AREAS):
    tiles, mp, cram, nm = convert_image(compose(a), 3)
    total += len(tiles)
    maxt = max(maxt, len(tiles) // 32)
    C.append("FM_ATTR const uint8_t kFmTiles%d[%d] __attribute__((aligned(2))) = {" % (a, len(tiles)))
    for i in range(0, len(tiles), 32):
        C.append("  " + ", ".join("0x%X" % v for v in tiles[i:i + 32]) + ",")
    C.append("};")
    C.append("FM_ATTR const uint16_t kFmMap%d[896] = {" % a)
    for i in range(0, 896, 32):
        C.append("  " + ", ".join("0x%X" % ((v & ~0x7FF) | ((v & 0x7FF) + FM_TILE_BASE) if v & 0x7FF else 0)
                                  for v in mp[i:i + 32]) + ",")
    C.append("};")
    C.append("FM_ATTR const uint16_t kFmCram%d[48] = { %s };" % (a, ", ".join(str(v) for v in cram + [0] * (48 - len(cram)))))
    print("área %d: %d tiles, %d colores fundidos" % (a, len(tiles) // 32, nm))
C.append("FM_ATTR const FmImage kFmPlanet[%d] = {" % AREAS)
for a in range(AREAS):
    C.append("  { sizeof kFmTiles%d / 32, kFmTiles%d, kFmMap%d, kFmCram%d }," % (a, a, a, a))
C.append("};")

# sprites: "PLANET ZEBES" (g_word_82C749[0]) y los nombres de las áreas (+1+área)
SM0 = w(0x82C749)
used, smaps = [], []
for i in range(SM0, SM0 + 7):
    p = w(0x82C569 + 2 * i)
    ents = []
    for k in range(w(0x820000 | p)):
        x, y, t = struct.unpack_from("<HbH", ROM, off(0x820000 | p) + 2 + 5 * k)
        dx = (x & 0x1FF) - (0x200 if x & 0x100 else 0)
        assert not x & 0x8000 and not (t >> 9) & 7, "spritemap del menú con 16x16 o paleta"
        tt = t & 0x1FF
        if tt not in used:
            used.append(tt)
        ents.append((dx, y, used.index(tt), (t >> 14) & 1, (t >> 15) & 1))
    smaps.append(ents)
spr_tiles = b""
for dim in (0, 1):                                     # 0 encendido (13, 14), 1 apagado (11, 12)
    for tt in used:
        px = t4(ROM, off(0xB6C000) + tt * 32)
        assert set(px.flatten().tolist()) <= {0, 13, 14}
        if dim:
            px = np.where(px == 13, 11, np.where(px == 14, 12, px)).astype(np.uint8)
        spr_tiles += genesis_tile_bytes(px)
FM_SPR_BASE = 920                                       # tiles de los sprites (después del planeta)
assert FM_TILE_BASE + 100 <= FM_SPR_BASE and FM_SPR_BASE + 2 * len(used) <= 1280
recs, idx = [], []                                      # como kDeathRecs (put_recs, md_engine.c)
for dim in (0, 1):
    for ents in smaps:
        idx.append(len(recs))
        out = [(dy, 0, 1 << 15 | 3 << 13 | vf << 12 | hf << 11 | (FM_SPR_BASE + dim * len(used) + t), dx)
               for dx, dy, t, hf, vf in ents]
        ys = [r[0] for r in out]
        xs = [r[3] for r in out]
        recs += [v & 0xFFFF for v in (len(out), min(ys), max(ys), min(xs), max(xs))]
        for r in out:
            recs += [v & 0xFFFF for v in r]
spr_pal = [0] * 16
for i, c in ((11, MENU_PAL[144 + 13]), (12, MENU_PAL[144 + 14]), (13, MENU_PAL[128 + 13]), (14, MENU_PAL[128 + 14])):
    spr_pal[i] = gen_to_cram(snes_to_genesis(c, "dac"))
C.append("FM_ATTR const uint8_t kFmSprTiles[%d] __attribute__((aligned(2))) = {" % len(spr_tiles))
for i in range(0, len(spr_tiles), 32):
    C.append("  " + ", ".join("0x%X" % v for v in spr_tiles[i:i + 32]) + ",")
C.append("};")
C.append("FM_ATTR const uint16_t kFmSprRecs[%d] = { %s };" % (len(recs), ", ".join(str(v) for v in recs)))
C.append("FM_ATTR const uint16_t kFmSprIdx[14] = { %s };" % ", ".join(str(v) for v in idx))
C.append("FM_ATTR const uint16_t kFmSprPal[16] = { %s };" % ", ".join(str(v) for v in spr_pal))

H = ['/* Generado por tools/conv/mkfilemap.py (docs/67). No editar. */', '#ifndef MD_FMAP_DATA_H',
     '#define MD_FMAP_DATA_H', '#include <stdint.h>',
     '#define FM_PAGE %d   /* página del SSF2 de lo del mapa de la partida (ventana 7) */' % FM_PAGE,
     '#define FM_TILE_BASE %d   /* tiles del planeta */' % FM_TILE_BASE,
     '#define FM_TILE_MAX %d' % maxt,
     '#define FM_SPR_TILES %d   /* tiles de los sprites (encendidos; los apagados siguen) */' % len(used),
     '#define FM_SM0 0x%X   /* g_word_82C749[0]: el spritemap de "PLANET ZEBES" (los nombres: + 1 + área) */' % SM0,
     'typedef struct { uint16_t ntiles; const uint8_t *tiles; const uint16_t *map; const uint16_t *cram; } FmImage;',
     'extern const FmImage kFmPlanet[%d];' % AREAS,
     'extern const uint8_t kFmSprTiles[%d];' % len(spr_tiles),
     'extern const uint16_t kFmSprRecs[%d];   /* como kDeathRecs */' % len(recs),
     'extern const uint16_t kFmSprIdx[14];   /* spritemap - FM_SM0 (+7: apagado) -> kFmSprRecs */',
     '#define FM_SPR_BASE %d' % FM_SPR_BASE,
     'extern const uint16_t kFmSprPal[16];   /* línea 3 de la CRAM */',
     '#endif']
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "md_fmap_data.h").write_text("\n".join(H) + "\n")
(OUT / "md_fmap_data.c").write_text("\n".join(C) + "\n")
print("mapa de la partida: %d KB de tiles del planeta (máximo %d tiles), %d tiles de sprites" % (total // 1024, maxt, len(used)))
