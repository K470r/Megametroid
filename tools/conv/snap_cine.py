#!/usr/bin/env python3
"""MegaMetroid — trayectoria de la capa de modo 7 de las cinemáticas de Ceres, desde grabaciones hlref (docs/35).

Por cuadro del motor en los estados 34 y 37: la matriz del modo 7 (M7A-D, M7X/Y), el desplazamiento (BG1HOFS/VOFS),
el modo de video, el mosaico, INIDISP, la función de la cinemática y la paleta (palette_buffer). mkm7.py arma con esto las imágenes de la capa.
La salida (snes_ref/cine_traj.json) sale de la ROM del usuario: no se distribuye.
Uso: snap_cine.py salida.json ref_escape.bin ref_tiempo.bin"""
import json
import sys

FIRST = {"escape": 8297, "tiempo": 9295}     # primer cuadro de la cinemática en cada grabación (bot del SNES)


def traj(ref, a):
    f = open(ref, "rb")
    out = []
    k = a
    while True:
        f.seek(0x30000 + k * 0x20002 + 2)
        r = f.read(0x20000)
        if len(r) < 0x20000:
            break
        w = lambda ad: r[ad] | r[ad + 1] << 8
        gs = w(0x998)
        if gs not in (34, 37):
            break
        out.append(dict(k=k, gs=gs, a=w(0x78), b=w(0x7A), c=w(0x7C), d=w(0x7E), x0=w(0x80), y0=w(0x82),
                        hofs=w(0xB1), vofs=w(0xB3), mode=r[0x55], ini=r[0x51], mosaic=r[0x57], cf=w(0x1F51),
                        pal=[w(0xC000 + 2 * i) for i in range(256)]))   # palette_buffer: los colores de ese cuadro
        k += 1
    return out


out = {"escape": traj(sys.argv[2], FIRST["escape"]), "tiempo": traj(sys.argv[3], FIRST["tiempo"])}
json.dump(out, open(sys.argv[1], "w"))
print({k: len(v) for k, v in out.items()})
