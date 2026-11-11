#!/usr/bin/env python3
"""Paso 1 del plan de RAM (docs/60), lado de la Genesis: qué bytes de la RAM del 68000 toca el motor en cada modo.
Lee una traza de retrorun (archivo o tubería) con la RAM entera (64 KB, palabras con los bytes cambiados, como da
libretro) por cuadro y acumula, por modo = game_state | md_zebes << 6, qué bytes cambiaron o dejaron de ser cero.

Uso: md_ram_uso.py TRAZA SÍMBOLOS(nm) ACUMULADO.npz
"""
import os
import sys

import numpy as np

tr, symf, acc = sys.argv[1:4]
sym = {}
for l in open(symf):
    p = l.split()
    if len(p) >= 3:
        sym[p[2]] = int(p[0], 16) & 0xFFFF
GS, MZ = sym['game_state'], sym['md_zebes']
st = np.load(acc)['st'].copy() if os.path.exists(acc) else np.zeros((128, 0x10000), bool)
R = 0x10000
prev = None
n = 0
with open(tr, 'rb', buffering=1 << 22) as f:
    while True:
        b = f.read(R)
        if len(b) < R:
            break
        cur = np.frombuffer(b, np.uint8).reshape(-1, 2)[:, ::-1].reshape(-1)
        u = cur != 0 if prev is None else (cur != 0) | (cur != prev)
        m = ((int(cur[GS]) << 8 | int(cur[GS + 1])) & 63) | (64 if cur[MZ] else 0)
        st[m] |= u
        prev = cur
        n += 1
np.savez_compressed(acc, st=st)
print('%s: %d cuadros' % (os.path.basename(tr), n))
