#!/usr/bin/env python3
"""MegaMetroid — la muerte de Samus en la Genesis (docs/64): los pedazos del traje que explota.

El SNES sube a la VRAM de los sprites $9B:8000-93FF (QueueTransferOfSamusDeathSequence: 5 bloques de 32 tiles; el de
$9B:8000 va donde están los tiles de Samus, 0x6000) y dibuja los spritemaps de Samus 2076-2093 (DrawSamusSuitExploding,
$92:EDBE) con la paleta de Samus (4) y la de Samus sin traje (7, palette_buffer 240-255).
Aquí: los 160 tiles en el orden de la VRAM del SNES (tile t -> DEATH_BASE + t), los registros de esos spritemaps (los
de 16x16 se parten en cuatro de 8x8) y todo en la página 23 del SSF2.
Uso: mkdeath.py rom.smc carpeta_salida"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "genesis_convert"))
from mkmd import snes_tile4, SAMUS_PAL, BG_BASE  # noqa: E402
from convert_rooms import genesis_tile_bytes     # noqa: E402

ROM = Path(sys.argv[1]).read_bytes()
OUT = Path(sys.argv[2])
DEATH_BASE = BG_BASE        # tiles de la sala (en la muerte no se ve: md_engine.c la borra)
DEATH_LINE = 1              # línea de la CRAM de la paleta 7 (Samus sin traje)
FIRST, LAST = 2076, 2093
PAGE = 23                   # página del SSF2 (mkzebes.py la cuenta como ocupada)


def off(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def rw(a):
    return ROM[off(a)] | ROM[off(a) + 1] << 8


blocks = [0x9B8000, 0x9B8400, 0x9B8800, 0x9B8C00, 0x9B9000]   # tiles 0-31 (el de $9B:8000), 32-63, ...
tiles = b""
for t in range(160):
    a = off(blocks[t // 32]) + (t % 32) * 32
    tiles += genesis_tile_bytes(snes_tile4(ROM[a:a + 32]))

recs, idx = [], []
for i in range(FIRST, LAST + 1):
    p = rw(0x92808D + 2 * i)
    n = rw(0x920000 | p)
    out = []
    for j in range(n):
        x, y, t = struct.unpack_from("<HbH", ROM, off(0x920000 | p) + 2 + 5 * j)
        big = bool(x & 0x8000)
        dx = x & 0x1FF
        if dx >= 0x100:
            dx -= 0x200
        tt, pal, hf, vf = t & 0x1FF, (t >> 9) & 7, (t >> 14) & 1, (t >> 15) & 1
        line = SAMUS_PAL if pal == 4 else DEATH_LINE
        prio = 1 if (t >> 12) & 3 == 3 else 0
        subs = [(0, 0, tt)]
        if big:                                   # 16x16 -> cuatro de 8x8 (con el volteo, cada cuarto del otro lado)
            subs = [((cx ^ hf) * 8, (cy ^ vf) * 8, (tt & 0x100) | (((tt >> 4) + cy) & 15) << 4 | ((tt + cx) & 15))
                    for cy in (0, 1) for cx in (0, 1)]
        for sx, sy, st in subs:
            attr = prio << 15 | line << 13 | vf << 12 | hf << 11 | (DEATH_BASE + st)
            out.append((y + sy, 0, attr, dx + sx))
    idx.append(len(recs))
    ys = [r[0] for r in out] or [0]
    xs = [r[3] for r in out] or [0]
    recs += [v & 0xFFFF for v in (len(out), min(ys), max(ys), min(xs), max(xs))]
    for r in out:
        recs += [r[0] & 0xFFFF, r[1], r[2], r[3] & 0xFFFF]

C = ["/* Generado por tools/conv/mkdeath.py (docs/64): los pedazos del traje de Samus desde tu ROM. No editar. */",
     '#include "md_death_data.h"',
     "#ifdef NT_SSF2", '#define DEATH_PAGE_ATTR __attribute__((section(".bank%d")))' % PAGE, "#else",
     "#define DEATH_PAGE_ATTR", "#endif",
     "DEATH_PAGE_ATTR const uint8_t kDeathTiles[%d] __attribute__((aligned(2))) = {" % len(tiles)]
for i in range(0, len(tiles), 32):
    C.append("  " + ", ".join("0x%X" % v for v in tiles[i:i + 32]) + ",")
C.append("};")
C.append("DEATH_PAGE_ATTR const uint16_t kDeathRecs[%d] = {" % len(recs))
for i in range(0, len(recs), 16):
    C.append("  " + ", ".join("0x%X" % v for v in recs[i:i + 16]) + ",")
C.append("};")
C.append("const uint16_t kDeathIdx[%d] = { %s };" % (len(idx), ", ".join(str(v) for v in idx)))
H = ["/* Generado por tools/conv/mkdeath.py (docs/64). No editar. */", "#ifndef MD_DEATH_DATA_H", "#define MD_DEATH_DATA_H",
     "#include <stdint.h>",
     "#define DEATH_BASE %d   /* tile de la Genesis del tile 0 de la hoja del SNES */" % DEATH_BASE,
     "#define DEATH_TILES %d" % (len(tiles) // 32),
     "#define DEATH_LINE %d   /* línea de la CRAM: palette_buffer 240-255 (la paleta 7) */" % DEATH_LINE,
     "#define DEATH_PAGE %d   /* página del SSF2 de kDeathTiles y kDeathRecs (ventana 7) */" % PAGE,
     "#define DEATH_SM_FIRST %d" % FIRST, "#define DEATH_SM_LAST %d" % LAST,
     "extern const uint8_t kDeathTiles[%d];" % len(tiles),
     "extern const uint16_t kDeathRecs[%d];   /* como kSamusSprRecs */" % len(recs),
     "extern const uint16_t kDeathIdx[%d];    /* spritemap - DEATH_SM_FIRST -> kDeathRecs */" % len(idx),
     "#endif"]
(OUT / "md_death_data.h").write_text("\n".join(H) + "\n")
(OUT / "md_death_data.c").write_text("\n".join(C) + "\n")
print("muerte: %d tiles, %d spritemaps, %d palabras de registros" % (len(tiles) // 32, len(idx), len(recs)))
