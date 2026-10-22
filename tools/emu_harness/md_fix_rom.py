#!/usr/bin/env python3
"""Rellena una ROM de Genesis a múltiplo de 128 KB y corrige fin de ROM y checksum de la cabecera
(lo mismo que hace sizebnd.jar -sizealign 131072 -checksum en SGDK)."""
import struct, sys

path = sys.argv[1]
d = bytearray(open(path, "rb").read())
align = 0x20000
if len(d) % align:
    d += b"\xff" * (align - len(d) % align)
struct.pack_into(">I", d, 0x1A4, len(d) - 1)
s = 0
for i in range(0x200, len(d), 2):
    s = (s + (d[i] << 8 | d[i + 1])) & 0xFFFF
struct.pack_into(">H", d, 0x18E, s)
open(path, "wb").write(d)
