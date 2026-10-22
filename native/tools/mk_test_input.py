#!/usr/bin/env python3
"""Entrada de prueba para la ROM de la Genesis (MD_TEST_INPUT, docs/32): la del bot del SNES, en el orden de
JOY1, desde el cuadro siguiente al estado inicial. Uso: mk_test_input.py ref.bin desde hasta salida.s"""
import struct, sys
ref, a, b, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
M = [0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10]
f = open(ref, "rb")
vals = []
for n in range(a, b + 1):
    f.seek(0x30000 + n * 0x20002)
    d = f.read(2)
    if len(d) < 2:
        break
    v = d[0] | d[1] << 8
    vals.append(sum(m for i, m in enumerate(M) if v & (1 << i)))
with open(out, "w") as o:
    o.write("/* entrada de prueba (grabación del SNES), JOY1 */\n    .section .rodata\n    .align 2\n")
    o.write("    .global kTestInputLen\nkTestInputLen:\n    .word %d\n    .global kTestInput\nkTestInput:\n" % len(vals))
    for i in range(0, len(vals), 16):
        o.write("    .word " + ",".join("0x%04X" % v for v in vals[i:i + 16]) + "\n")
print(len(vals), "cuadros")
