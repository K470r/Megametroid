#!/usr/bin/env python3
"""MegaMetroid — comprueba que lo que se saca de las copias de los bancos (el relleno del final, mkdata.py) no lo lee
nadie (docs/68). Lee los archivos de cobertura de host/bankcov.c (MM_BANKCOV: un bit por byte de cada banco) y
también dice cuánto de cada banco se leyó.
Uso: bankcov_check.py rom.smc cobertura.bin [más.bin ...]"""
import sys
from pathlib import Path

IDS = [0x86, 0x91, 0x94, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAD, 0xB2, 0xB3, 0xB4,
       0x81, 0x82, 0x88, 0x8B, 0x8D, 0x87, 0x84, 0x8F, 0x8C]
N = 32769 // 8 + 1
KEEP = 2

rom = Path(sys.argv[1]).read_bytes()
rom = rom[len(rom) % 0x8000:]
cov = bytearray(N * len(IDS))
for f in sys.argv[2:]:
    d = Path(f).read_bytes()
    for i, v in enumerate(d[:len(cov)]):
        cov[i] |= v
bad = 0
for i, b in enumerate(IDS):
    c = cov[i * N:(i + 1) * N]
    read = [o for o in range(32769) if c[o >> 3] >> (o & 7) & 1]
    base = (b & 0x7F) * 0x8000
    f = 0
    while f < 0x8000 and rom[base + 0x7FFF - f] == 0xFF:
        f += 1
    n = min(0x8001, 0x8000 - f + KEEP)
    over = [o for o in read if o >= n]
    bad += len(over)
    print("%02X: leídos %5d bytes, hasta %04X; relleno %5d (se saca desde %04X)%s" % (
        b, len(read), 0x8000 + (max(read) if read else 0), f, 0x8000 + n,
        "  LEÍDO EN LO QUE SE SACA: " + " ".join("%04X" % (0x8000 + o) for o in over[:8]) if over else ""))
print("MAL" if bad else "BIEN: nada leído en lo que se saca")
sys.exit(1 if bad else 0)
