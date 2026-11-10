#!/usr/bin/env python3
"""MegaMetroid — la entrada de prueba (kTestInput: una palabra por cuadro) en tramos (kTestRle: cuántos cuadros, botones),
para la ROM de prueba: la plana ocupaba ~31 KB de la parte común (debajo de 0x300000) y con Kraid ya no entraba.
Uso: rle_test_input.py entrada.s salida.s   (md_main.c la lee por tramos)"""
import re
import sys

t = open(sys.argv[1]).read()
n = int(re.search(r'kTestInputLen:\s*\.word\s+(\d+)', t).group(1))
body = t[t.index('kTestInput:') + len('kTestInput:'):]
vals = [int(x, 0) for x in re.findall(r'(?:0x[0-9A-Fa-f]+|\b\d+\b)', ' '.join(
    l.split('.word', 1)[1] for l in body.splitlines() if '.word' in l))][:n]
runs = []
for v in vals:
    if runs and runs[-1][1] == v and runs[-1][0] < 0xFFFF:
        runs[-1][0] += 1
    else:
        runs.append([1, v])
with open(sys.argv[2], 'w') as o:
    o.write("    .section .rodata\n    .align 2\n    .global kTestInputLen\nkTestInputLen:\n    .word %d\n"
            "    .global kTestRle\nkTestRle:\n" % len(vals))
    for i in range(0, len(runs), 8):
        o.write("    .word " + ",".join("%d,0x%04X" % (c, v) for c, v in runs[i:i + 8]) + "\n")
