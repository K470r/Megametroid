#!/usr/bin/env python3
"""Registros del SNES (copias en la RAM) en los cuadros de las capturas de VRAM (docs/32).
Uso: snap_regs.py ref.bin cuadro_inicial vram_dir sala:cuadro ...   -> vram_dir/regs.json
(cuadro = número de cuadro global de hlref; la grabación empieza en cuadro_inicial)"""
import json, sys
ref, first, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
f = open(ref, "rb")
res = {}
for a in sys.argv[4:]:
    room, fr = a.split(":")
    n = int(fr) - first - 1
    f.seek(0x30000 + n * 0x20002 + 2)
    r = f.read(0x20000)
    w = lambda x: r[x] | r[x + 1] << 8
    res[room] = {"OBSEL": r[0x52], "BG1SC": r[0x58], "BG2SC": r[0x59], "BG3SC": r[0x5A], "NBA": r[0x5D],
                 "room": w(0x79B), "state": w(0x7BB), "bg2_x_scroll": w(0x93B), "frame": int(fr)}
json.dump(res, open(out + "/regs.json", "w"), indent=1)
print(res)
