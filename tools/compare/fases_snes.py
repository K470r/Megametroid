#!/usr/bin/env python3
"""MegaMetroid — los cambios de estado de un jefe y de las capas del SNES en una grabación de hlref (docs/59).
Imprime una línea cada vez que cambia el estado (por omisión: kraid_var_A, 7E:0FA8), BG2SC, BG12NBA, TM o la sala.
Uso: fases_snes.py grabación.bin [primer cuadro de la grabación, 23500] [dirección del estado, 0FA8]"""
import sys

f = open(sys.argv[1], 'rb')
first = int(sys.argv[2]) if len(sys.argv) > 2 else 23500
st = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x0FA8
f.seek(0, 2)
n = (f.tell() - 0x30000) // 0x20002
names = {st: 'estado', 0x0F7A: 'x', 0x0F7E: 'y', 0x0F8C: 'vida', 0x59: 'BG2SC', 0x58: 'BG1SC', 0x5D: 'NBA', 0x69: 'TM',
         0x911: 'l1x', 0x915: 'l1y', 0xB5: 'b2h', 0xB7: 'b2v', 0x998: 'gs', 0x79B: 'sala'}
prev = None
for i in range(n):
    f.seek(0x30000 + i * 0x20002 + 2)
    ram = f.read(0x20000)
    v = {k: ram[a] | ram[a + 1] << 8 for a, k in names.items()}
    key = (v['estado'], v['BG2SC'] & 0xFF, v['NBA'] & 0xFF, v['TM'] & 0xFF, v['sala'])
    if key != prev:
        print(first + i, ' '.join('%s=%04X' % (k, v[k]) for k in
                                  ('sala', 'gs', 'estado', 'vida', 'x', 'y', 'BG1SC', 'BG2SC', 'NBA', 'TM', 'l1x', 'l1y', 'b2h', 'b2v')))
        prev = key
