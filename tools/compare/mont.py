#!/usr/bin/env python3
"""MegaMetroid — pares Genesis | SNES del mismo cuadro del motor (lado_a_lado.sh, docs/51).
Uso (en la carpeta de trabajo): mont.py salida.png e1 e2 ...  (e: cuadro de la consola de la Genesis)"""
import sys
from PIL import Image
G0, T0 = map(int, open('gen/desde.txt').read().split())
tr = open('gen/tr.bin', 'rb').read()
shown = lambda e: tr[2 * e + 1] << 8 | tr[2 * e]        # md_shown_frame en ese cuadro (RAM del 68000, en palabras)
es = [int(x) for x in sys.argv[2:]]
rows = (len(es) + 1) // 2
im = Image.new('RGB', (1040, 226 * rows), (255, 255, 255))
for i, e in enumerate(es):
    m = shown(e)
    x, y = (i % 2) * 520, (i // 2) * 226
    im.paste(Image.open('gen/g%05d.ppm' % (e - G0)), (x, y))
    # cuadro del motor m: la captura del SNES del mismo cuadro es a(T0 + m) (comprobado con la RAM, docs/59)
    im.paste(Image.open('snes/a%05d.ppm' % (T0 + m)), (x + 258, y))
    print(e, "motor", m, "SNES", T0 + m)
im.save(sys.argv[1])
