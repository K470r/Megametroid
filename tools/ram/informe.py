#!/usr/bin/env python3
"""Informe del paso 1 del plan de RAM (docs/60): cruza las máscaras de uso de la RAM del SNES (ram_uso.py,
medir_snes.sh) con las variables del motor (res/gen/engine/vars.h: nombre, dirección del SNES, tamaño que les dio
gen_vars.py) y con la RAM extra de enemigos (enemy_ram7800, 7E:7800-97FF).

"Usado" = alguna vez distinto de cero o alguna vez cambió en las grabaciones, fuera de la carga de las salas. Un arreglo cuyo final nunca se usó es
candidato a recortarse, pero hay que confirmarlo en el código (un índice que solo llega ahí en un caso que las
grabaciones no tienen; un byte que se escribe siempre con cero).

Uso: informe.py vars.h salida.md ACUMULADO.npz [ACUMULADO.npz...]
"""
import re
import sys

import numpy as np

vh, out, accs = sys.argv[1], sys.argv[2], sys.argv[3:]
used = st = None; frames = 0
for a in accs:
    z = np.load(a)
    u = z['nz'] | z['ch']
    used = u if used is None else used | u
    st = z['st'] if st is None else st | z['st']
    frames += int(z['frames'])
LOAD = (6, 11)                                     # carga de la sala: el SNES usa 7E:7000-97FF como búfer
play = np.zeros(0x10000, bool)
for g in range(64):
    if g not in LOAD:
        play |= st[g]
TS = {'uint8': 1, 'int8': 1, 'uint16': 2, 'int16': 2}
rows = []
for m in re.finditer(r'extern (u?int(?:8|16)) (\w+)(?:\[(\d+)\])?;\s*/\* 7E:([0-9A-F]+)', open(vh).read()):
    t, n, cnt, a = m.group(1), m.group(2), m.group(3), int(m.group(4), 16)
    es = TS[t]; size = es * int(cnt) if cnt else es
    u = np.nonzero(play[a:a + size])[0]
    ext = int(u[-1]) + 1 if len(u) else 0
    ext = -(-ext // es) * es                       # (en elementos enteros)
    rows.append((n, a, t, int(cnt) if cnt else 0, size, ext))
L = []
p = L.append
p('# Uso de la RAM del SNES (paso 1 del plan de RAM)\n')
p('Grabaciones: %s; %d cuadros. Bytes de 7E:0000-FFFF usados: %d de 65536.\n' %
  (', '.join(x.split('/')[-1] for x in accs), frames, int(used[:0x10000].sum())))
arr = [r for r in rows if r[3] and r[4] - r[5] >= 8]
arr.sort(key=lambda r: -(r[4] - r[5]))
p('## Arreglos con el final sin usar (candidatos)\n')
p('| Variable | SNES | Declarado | Usado hasta | Sobra (bytes) |')
p('|---|---|---|---|---|')
for n, a, t, cnt, size, ext in arr:
    p('| `%s` | 7E:%04X | %s[%d] = %d | %d | %d |' % (n, a, t, cnt, size, ext, size - ext))
p('\nTotal: %d bytes en %d arreglos.\n' % (sum(r[4] - r[5] for r in arr), len(arr)))
never = [r for r in rows if r[5] == 0]
p('## Variables nunca usadas en las grabaciones\n')
p('%d variables, %d bytes (pueden ser escritas siempre con cero o usadas en casos sin grabar): %s\n' %
  (len(never), sum(r[4] for r in never), ', '.join('`%s`' % r[0] for r in sorted(never, key=lambda r: -r[4])[:60])))
p('## RAM extra de enemigos (enemy_ram7800, 7E:7800-97FF, 8 KB)\n')
p('En la carga de las salas (estados %s) el SNES usa esta zona como búfer; el motor no (host/check la compara '
  'en los demás estados). "En juego": los demás estados.\n' % (LOAD,))
p('| Tramo | Bytes usados (todo) | En juego | Estados que la usan en juego |')
p('|---|---|---|---|')
for b in range(0x7800, 0x9800, 0x100):
    gs = [g for g in range(64) if g not in LOAD and st[g][b:b + 0x100].any()]
    p('| 7E:%04X-%04X | %d | %d | %s |' % (b, b + 0xFF, int(used[b:b + 0x100].sum()), int(play[b:b + 0x100].sum()),
                                         ' '.join(str(g) for g in gs)))
u = play[0x7800:0x9800]
runs, s = [], None
for i, v in enumerate(list(u) + [True]):
    if not v and s is None:
        s = i
    if v and s is not None:
        if i - s >= 64:
            runs.append((0x7800 + s, i - s))
        s = None
p('\nTramos sin usar de 64 bytes o más: %d bytes en total. %s\n' %
  (sum(r[1] for r in runs), ', '.join('7E:%04X (+%d)' % r for r in runs)))
open(out, 'w').write('\n'.join(L) + '\n')
print('\n'.join(L[:3]))
print('arreglos candidatos: %d bytes; enemy_ram7800 sin usar en tramos >= 64: %d bytes' %
      (sum(r[4] - r[5] for r in arr), sum(r[1] for r in runs)))
