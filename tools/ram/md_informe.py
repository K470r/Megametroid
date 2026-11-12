#!/usr/bin/env python3
"""Informe de md_vars.sh: cada variable de la RAM de la Genesis (64 bytes o más) con los modos en que el motor la
toca (Z:8 = Zebes con game_state 8; sin Z = intro, Ceres...). Dos variables que nunca se tocan en el mismo modo
pueden compartir la RAM (con cuidado de lo que pasa de un modo al siguiente). Cada traza va con los símbolos de su
ROM (las direcciones cambian un poco entre las ROM de prueba).

Uso: md_informe.py salida.md TRAZA.npz=SÍMBOLOS(nm -S) [...]
"""
import subprocess
import sys

import numpy as np

out, pairs = sys.argv[1], [a.split('=') for a in sys.argv[2:]]
modes, used, size = {}, {}, {}
for acc, symf in pairs:
    st = np.load(acc)['st']
    for l in open(symf):
        p = l.split()
        if len(p) == 4 and p[2] in 'bBdD' and (int(p[0], 16) & 0xFFFFFF) >= 0xFF0000 and int(p[1], 16) >= 64:
            a, s, n = int(p[0], 16) & 0xFFFF, int(p[1], 16), p[3]
            if n in size and size[n] != s:
                n = n + '@%04X' % a                 # (dos estáticas con el mismo nombre)
            size[n] = s
            m = st[:, a:a + s].any(axis=1)
            modes[n] = modes.get(n, np.zeros(128, bool)) | m
            u = st[:, a:a + s].any(axis=0)
            used[n] = used.get(n, np.zeros(s, bool)) | u
L = ['# Variables de la RAM de la Genesis por modo\n', 'Trazas: %s\n' % ', '.join(a for a, _ in pairs),
     '| Variable | Bytes | Usados | Modos (game_state; Z = Zebes) |', '|---|---|---|---|']
for n in sorted(size, key=lambda n: -size[n]):
    ms = np.nonzero(modes[n])[0]
    txt = ' '.join(('Z:%d' if m & 64 else '%d') % (m & 63) for m in ms)
    L.append('| `%s` | %d | %d | %s |' % (n, size[n], int(used[n].sum()), txt or 'nunca'))
open(out, 'w').write('\n'.join(L) + '\n')
print('\n'.join(L))
