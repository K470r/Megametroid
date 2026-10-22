#!/usr/bin/env python3
"""Ubica en qué cuadro de una corrida de hlref se tomó cada captura de VRAM (docs/45).
Entrada: el volcado de HL_VDUMP (66592 bytes por cuadro desde el cuadro 'desde') y capturas (.bin de snes_ref).
Compara la VRAM (64 KB) y, si la captura la tiene, la CGRAM (512 B). Uso: find_frames.py volcado desde captura...
Salida: por captura, los cuadros que coinciden."""
import sys, numpy as np
dump, first = sys.argv[1], int(sys.argv[2])
REC = 66592
d = np.memmap(dump, dtype=np.uint8, mode='r')
n = len(d) // REC
d = d[:n * REC].reshape(n, REC)
for cap in sys.argv[3:]:
    c = np.fromfile(cap, dtype=np.uint8)
    k = len(c)                                   # VRAM + CGRAM (+ OAM en las capturas nuevas)
    if len(c) == 512:                            # solo colores (CGRAM)
        hits = np.where((d[:, 0x10000:0x10200] == c).all(axis=1))[0]
    else:
        hits = np.where((d[:, :k] == c[:k]).all(axis=1))[0]
    print(cap, [first + int(h) for h in hits[:8]], '...' if len(hits) > 8 else '')
