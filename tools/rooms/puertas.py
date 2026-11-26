#!/usr/bin/env python3
"""MegaMetroid — puertas de color y barreras con interruptor de cada sala de Zebes (para compararlas con el SNES).

Recorre las salas alcanzables por puertas (como salas.py) y los PLM de cada estado (estado + 20: la lista de PLM en $8F;
entradas de 6 bytes: PLM, x, y, parámetro). El tipo sale del setup del PLM ($84: primera palabra de la cabecera), como en
el decomp (sm_84.c): puertas grises PlmSetup_C794_GreyDoor, de color PlmSetup_Door_Colored, azules PlmSetup_Door_Blue;
interruptores de las barreras PlmSetup_C806..C822_*GateTrigger; barreras PlmSetup_C826..C836/C73A_*Gate*.

Uso: puertas.py rom.smc   (una línea por PLM: área sala:puerta estado tipo plm x y parámetro)
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "conv"))
import mkmd  # noqa: E402

rom = mkmd.Rom(sys.argv[1])
w = rom.w
SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}
SETUP = {0xC794: "gris", 0xC7B1: "color", 0xC7BB: "azul", 0xC7E2: "rara",
         0xC54D: "interruptor verde izq", 0xC56C: "interruptor verde der", 0xC58B: "interruptor rojo izq",
         0xC5AF: "interruptor rojo der", 0xC5D3: "interruptor amarillo izq", 0xC5F1: "interruptor amarillo der",
         0xC610: "interruptor azul izq", 0xC627: "interruptor azul der",
         0xC6BE: "barrera cerrada abajo", 0xC6CB: "barrera cerrada arriba", 0xC6D8: "barrera abierta abajo",
         0xC6DC: "barrera abierta arriba", 0xC6E0: "barrera bloque abajo", 0xC73A: "barrera bloque arriba"}


def doors_of(ra):
    dl = 0x8F0000 | w(0x8F0000 | (ra + 9))
    out, i = [], 0
    while True:
        d = w(dl + 2 * i)
        if d < 0x8000:
            return out
        out.append(d)
        i += 1


def states(ra):
    q, sts = 0x8F0000 | (ra + 11), []
    while True:
        n = SEL_LEN[w(q)]
        if n == 0:
            return sts + [(q + 2) & 0xFFFF]
        sts.append(w(q + n))
        q += 2 + n


seen, todo, entry = set(), [0x91F8], {}
while todo:
    ra = todo.pop()
    if ra in seen:
        continue
    seen.add(ra)
    for d in doors_of(ra):
        dest = w(0x830000 | d)
        if dest >= 0x8000:
            entry.setdefault(dest, d)
            if dest not in seen:
                todo.append(dest)
for ra in sorted(seen):
    area = rom.b(0x8F0000 | (ra + 1))
    if area > 5:
        continue
    for st in states(ra):
        p = 0x8F0000 | w(0x8F0000 | (st + 20))
        while True:
            plm = w(p)
            if not plm:
                break
            x, y, prm = rom.b(p + 2), rom.b(p + 3), w(p + 4)
            kind = SETUP.get(w(0x840000 | plm))
            if kind:
                print("%d %04X:%04X %04X %-25s %04X %3d %3d %04X" % (area, ra, entry.get(ra, 0), st, kind.replace(" ", "_"), plm, x, y, prm))
            p += 6
