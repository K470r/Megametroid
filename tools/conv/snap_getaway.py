#!/usr/bin/env python3
"""MegaMetroid — huida de Ridley de Ceres (modo 7 en el SNES): captura de cada cuadro (docs/33).

El SNES dibuja a Ridley alejándose y acercándose con el modo 7 (escala y giro por cuadro). La Genesis no tiene modo
7: se usa la imagen que el SNES genera. Se corre el SNES dos veces con la misma entrada (hlref con HL_SHOTS): una
normal (a/) y otra sin la capa de modo 7 (b/, HL_NOM7). Los píxeles distintos son Ridley.

Uso: snap_getaway.py dir_a dir_b primera_captura salida.npz
  primera_captura: la captura del cuadro de la huida 0 (cry_var_33 = 2 después del cuadro; el SNES la muestra un
  cuadro después). Resultado: por cuadro, máscara y color ya en niveles de la Genesis (0-7 por canal).
Los datos salen de la ROM del usuario: no se distribuyen."""
import sys
import numpy as np
from PIL import Image

DAC = np.array([0, 52, 87, 116, 144, 172, 206, 255])
a_dir, b_dir, first, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
masks, cols = [], []
for f in range(113):
    a = np.asarray(Image.open(f"{a_dir}/s{first + f:05d}.ppm").convert("RGB")).astype(int)
    b = np.asarray(Image.open(f"{b_dir}/s{first + f:05d}.ppm").convert("RGB")).astype(int)
    m = np.abs(a - b).sum(-1) > 0
    q = np.abs(a[..., None] - DAC).argmin(-1)
    masks.append(m)
    cols.append((q[..., 0] | q[..., 1] << 3 | q[..., 2] << 6).astype(np.uint16) * m)
np.savez_compressed(out, mask=np.array(masks), col=np.array(cols))
print("huida:", len(masks), "cuadros, píxeles por cuadro máx.", max(int(m.sum()) for m in masks))
