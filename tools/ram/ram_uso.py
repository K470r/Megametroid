#!/usr/bin/env python3
"""Uso de la RAM del SNES (paso 1 del plan de RAM, docs/60): lee una grabación de hlref (archivo o tubería) y suma
a un acumulado qué bytes de 7E:0000-7F:FFFF se usan.

Grabación de hlref: 0x30000 bytes al empezar (RAM + VRAM iniciales), luego por cuadro 2 bytes de entrada y la RAM
(0x20000). Por byte se acumulan dos máscaras:
  nz  alguna vez distinto de cero (en la RAM inicial o en algún cuadro)
  ch  alguna vez cambió de un cuadro al siguiente (escrito con otro valor)
Lo que el juego escribe siempre con el mismo valor que ya tenía (o solo con cero) no se ve: el informe lo cruza con
el código (informe.py).

Uso: ram_uso.py GRABACIÓN ACUMULADO.npz [desde_cuadro]
"""
import os
import sys

import numpy as np

R = 0x20000
src, acc = sys.argv[1], sys.argv[2]
skip = int(sys.argv[3]) if len(sys.argv) > 3 else 0
if os.path.exists(acc):
    z = np.load(acc)
    nz, ch, frames = z['nz'].copy(), z['ch'].copy(), int(z['frames'])
else:
    nz, ch, frames = np.zeros(R, bool), np.zeros(R, bool), 0
with open(src, 'rb', buffering=1 << 22) as f:
    head = f.read(0x30000)
    if len(head) < R:
        sys.exit('grabación vacía')
    prev = np.frombuffer(head[:R], np.uint8)
    if not skip:
        nz |= prev != 0
    n = 0
    while True:
        b = f.read(R + 2)
        if len(b) < R + 2:
            break
        cur = np.frombuffer(b, np.uint8, R, 2)
        if n >= skip:
            nz |= cur != 0
            ch |= cur != prev
            frames += 1
        prev = cur
        n += 1
np.savez_compressed(acc, nz=nz, ch=ch, frames=frames)
print('%s: %d cuadros (acumulado %d); bytes usados 7E %d, 7F %d' %
      (os.path.basename(src), n, frames, int((nz | ch)[:0x10000].sum()), int((nz | ch)[0x10000:].sum())))
