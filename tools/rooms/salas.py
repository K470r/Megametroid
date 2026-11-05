#!/usr/bin/env python3
"""MegaMetroid — salas de Zebes con enemigos y una puerta que lleva a cada una (para HL_WARP, docs/41).

Recorre las salas alcanzables por puertas (como mkzebes.py) y, para cada sala con enemigos en alguno de sus estados,
escribe "área sala:puerta" (la primera puerta, en el orden de las salas, cuyo destino es esa sala).

Uso: salas.py rom.smc [área...]     (áreas: 0 Crateria, 1 Brinstar, 2 Norfair, 3 Wrecked Ship, 4 Maridia, 5 Tourian)
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "conv"))
import mkmd  # noqa: E402

rom = mkmd.Rom(sys.argv[1])
areas = {int(a) for a in sys.argv[2:]} or set(range(6))
w = rom.w
SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}


def doors_of(ra):
    dl = 0x8F0000 | w(0x8F0000 | (ra + 9))
    out = []
    i = 0
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
    if area not in areas or ra not in entry:
        continue
    if any(w(0xA10000 | w(0x8F0000 | (st + 8))) != 0xFFFF for st in states(ra)):
        print("%d %04X:%04X" % (area, ra, entry[ra]))
