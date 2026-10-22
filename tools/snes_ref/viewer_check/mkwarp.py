#!/usr/bin/env python3
"""Entrada de hlref para recorrer salas con el visor.
Uso: mkwarp.py "sala:puerta sala:puerta ..." n0 cada salida_entrada.txt [base.txt]
La entrada es base.txt (por omisión ../ceres_bot/in_z2.txt: hasta el aterrizaje en Zebes) más, por sala, 150
cuadros de movimientos cortos. Imprime HL_WARP: "n:puerta,..." con n = n0 + cada * i (cuadro grabado)."""
import sys
from pathlib import Path
doors = [x.split(':')[1] for x in sys.argv[1].split()]
n0, per = int(sys.argv[2]), int(sys.argv[3])
base = Path(sys.argv[5]) if len(sys.argv) > 5 else Path(__file__).resolve().parent.parent / 'ceres_bot' / 'in_z2.txt'
pat = ['40 >', '20 >A', '40 <', '20 A', '30 -']
with open(sys.argv[4], 'w') as f:
    f.write(base.read_text())
    for _ in doors:
        f.write('\n'.join(pat) + '\n')
print(','.join('%d:%s' % (n0 + per * i, d) for i, d in enumerate(doors)))
