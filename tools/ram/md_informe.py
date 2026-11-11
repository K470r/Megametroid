#!/usr/bin/env python3
"""Informe de md_vars.sh: cada variable de la RAM de la Genesis (64 bytes o más) con los modos en que el motor la
toca (Z = md_zebes, Z:8 = Zebes con game_state 8; sin Z = intro, Ceres, título...). Dos variables que nunca se tocan
en el mismo modo pueden compartir la RAM (con cuidado de lo que se guarda de un modo al siguiente).

Uso: md_informe.py SÍMBOLOS(nm -S) salida.md md_escena.npz [...]
"""
import sys

import numpy as np

symf, out, accs = sys.argv[1], sys.argv[2], sys.argv[3:]
st = None
for a in accs:
    z = np.load(a)['st']
    st = z if st is None else st | z
rows = []
for l in open(symf):                                   # nm -S: dirección, tamaño, tipo, nombre
    p = l.split()
    if len(p) == 4 and p[2] in 'bBdD' and (int(p[0], 16) & 0xFFFFFF) >= 0xFF0000:
        rows.append((int(p[0], 16) & 0xFFFF, int(p[1], 16), p[3]))
rows.sort(key=lambda r: -r[1])
L = ['# Variables de la RAM de la Genesis por modo\n', 'Trazas: %s\n' % ', '.join(a.split('/')[-1] for a in accs),
     '| Variable | Bytes | Usados | Modos (game_state; Z = Zebes) |', '|---|---|---|---|']
for a, s, n in rows:
    if s < 64:
        break
    ms = [m for m in range(128) if st[m][a:a + s].any()]
    used = int(np.logical_or.reduce(st[:, a:a + s], axis=0).sum())
    txt = ' '.join(('Z:%d' if m & 64 else '%d') % (m & 63) for m in ms)
    L.append('| `%s` | %d | %d | %s |' % (n, s, used, txt or 'nunca'))
open(out, 'w').write('\n'.join(L) + '\n')
print('\n'.join(L))
