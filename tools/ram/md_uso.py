#!/usr/bin/env python3
"""Resumen de medir_md.sh: por cuadro, la RAM de la Genesis desde _bend (A) hasta 0xFFFF (la de libretro: palabras
con los bytes cambiados de lugar). Montón de SGDK (memory.c): desde _bend (alineado), bloques con una palabra de
cabecera (tamaño en bytes con la cabecera; bit 0 = usado), hasta MEMORY_HIGH = 0xF600 (0xE1000000 - STACK_SIZE).
La pila baja desde 0xFFFF: la consola limpia la RAM al arrancar, así que la dirección más baja que dejó de ser cero
por encima del último bloque usado del montón es hasta dónde llegó (si pasa de MEMORY_HIGH, se metió en el montón).

Uso: md_uso.py TRAZA A_hex escena
"""
import sys

import numpy as np

tr, A, sc = sys.argv[1], int(sys.argv[2], 16), sys.argv[3]
N = 0x10000 - A
d = np.fromfile(tr, np.uint8)
F = len(d) // N
d = d[:F * N].reshape(F, N)
d = d.reshape(F, N // 2, 2)[:, :, ::-1].reshape(F, N)   # bytes del 68000 en orden
HIGH = 0xF600
heap0 = (A + 1) & ~1
used_max = alloc_max = 0
stk_low = 0x10000
fr_used = fr_stk = 0
F0 = 30                                                # cuadro desde el que se mide la pila
for f in range(F):
    r = d[f]
    w = lambda a: int(r[a - A]) << 8 | int(r[a - A + 1])
    a = heap0; alloc = 0; top = heap0
    for _ in range(4096):
        if a + 2 > HIGH:
            break
        s = w(a)
        if s == 0:
            break
        if s & 1:
            alloc += (s & ~1); top = a + (s & ~1)
        a += s & ~1
    if top > used_max:
        used_max, fr_used = top, f
    alloc_max = max(alloc_max, alloc)
    if f == F0:
        base = r.copy()                                # (lo que dejó el arranque en lo libre: bloques ya liberados)
    if f >= F0:
        lo = top + 2                                   # (encima del último bloque usado y la cabecera del libre)
        nzz = np.nonzero(r[lo - A:] != base[lo - A:])[0]
        nzz = nzz if len(nzz) else np.nonzero(r[max(lo, HIGH) - A:])[0] + (max(lo, HIGH) - lo)
        if len(nzz) and lo + nzz[0] < stk_low:
            stk_low, fr_stk = lo + int(nzz[0]), f
print('%s: %d cuadros; montón de SGDK desde %04X: hasta %04X usado (máx %d bytes asignados, cuadro %d); '
      'de %04X a %04X libre' % (sc, F, heap0, used_max, alloc_max, fr_used, used_max, HIGH))
print('%s: pila (desde el cuadro 30): llegó hasta %04X (%d de %d bytes, cuadro %d)' % (sc, stk_low, 0x10000 - stk_low, 0x10000 - HIGH, fr_stk))
