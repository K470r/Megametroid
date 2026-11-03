#!/usr/bin/env python3
"""MegaMetroid — una entrada del bot (líneas "cuadros botones") a res/gen/md/test_input.s de la ROM de prueba, desde el
cuadro T0 de la entrada (el del estado inicial: como tools/compare/lado_a_lado.sh).

Uso: mkinput.py entrada.txt T0 test_input.s"""
import sys

B = {'B': 1, 'Y': 2, 's': 4, 'S': 8, 'U': 16, 'D': 32, '<': 64, '>': 128, 'A': 256, 'X': 512, 'L': 1024, 'R': 2048}
SN = [0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10]
v = []
for line in open(sys.argv[1]):
    p = line.split()
    if not p:
        continue
    x = 0
    for ch in (p[1] if len(p) > 1 else ''):
        x |= B.get(ch, 0)
    v += [sum(m for i, m in enumerate(SN) if x & (1 << i))] * int(p[0])
v = v[int(sys.argv[2]):]
with open(sys.argv[3], 'w') as o:
    o.write("    .section .rodata\n    .align 2\n    .global kTestInputLen\nkTestInputLen:\n    .word %d\n"
            "    .global kTestInput\nkTestInput:\n" % len(v))
    for i in range(0, len(v), 16):
        o.write("    .word " + ",".join("0x%04X" % x for x in v[i:i + 16]) + "\n")
