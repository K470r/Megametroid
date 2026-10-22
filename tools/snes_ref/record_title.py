#!/usr/bin/env python3
"""Graba, cuadro a cuadro, la secuencia de título del Super Metroid real (desde el encendido).

Corre tu ROM en snes9x (libretro, con tools/emu_harness/retrorun) y guarda por cuadro los registros
sombra y buffers que definen la imagen de la secuencia: brillo (INIDISP), capas (TM), matemática de
color (CGADSUB, COLDATA), la cámara de modo 7 (cinematic_var6/8/10), la función de cinemática, el
buffer de OAM (sprites) y palette_buffer. make_title_data.py convierte eso en una línea de tiempo
para la Genesis. Así los tiempos, fundidos, letras y paletas animadas son los del juego original.

Uso: record_title.py retrorun snes9x_libretro.so sm.smc salida.bin [cuadros]
Formato de salida: cuadros x REC bytes (ver FIELDS).
Además graba salida.bin.grad: el degradado del título tal como sale en pantalla (el HDMA de COLDATA
junto con el de CGADSUB, que lo apaga en parte): 16 tablas (una por var6 >> 4) x 224 líneas, color
BGR555 big-endian, medido con la capa BG1 y los sprites apagados.
"""
import os, struct, subprocess, sys, tempfile
from pathlib import Path

# (nombre, dirección WRAM, bytes)
FIELDS = [("inidisp", 0x51, 1), ("tm", 0x69, 1), ("cgadsub", 0x72, 1), ("coldata", 0x74, 3),
          ("game_state", 0x998, 2), ("cin_func", 0x1F51, 2), ("var6", 0x198F, 2), ("var8", 0x1993, 2),
          ("var10", 0x1997, 2), ("frame", 0x5B6, 2), ("oam", 0x370, 544), ("pal", 0xC000, 512),
          ("m7obj", 0x19A1, 4)]            # punteros de instrucciones de los objetos de modo 7 (bebé metroide)
REC = sum(f[2] for f in FIELDS)


def offsets():
    o, d = 0, {}
    for n, a, s in FIELDS:
        d[n] = (o, s)
        o += s
    return d


def main():
    retrorun, core, rom, out = sys.argv[1:5]
    frames = int(sys.argv[5]) if len(sys.argv) > 5 else 2840   # hasta que empiezan las demos
    tmp = Path(tempfile.mkdtemp())
    spec = ",".join(f"{a:X}:{s}" for _, a, s in FIELDS)
    (tmp / "s.txt").write_text(f"trace {out} {spec}\nhold {frames} -\n")
    subprocess.run([retrorun, core, rom, str(tmp / "s.txt")], check=True, capture_output=True)
    n = os.path.getsize(out) // REC
    print(f"{n} cuadros grabados ({REC} bytes por cuadro)")
    # degradado: un cuadro por cada tabla usada durante el alejamiento y el título
    d = Path(out).read_bytes()
    o6 = offsets()["var6"][0]
    ocf = offsets()["cin_func"][0]
    want = {}
    for i in range(n):
        if struct.unpack_from("<H", d, i * REC + ocf)[0] in (0x9E8B, 0x9F28, 0x9F29):
            want.setdefault((struct.unpack_from("<H", d, i * REC + o6)[0] & 0xF0) >> 4, min(i + 5, n - 1))
    cmd, prev = "", 0
    for t, f in sorted(want.items(), key=lambda x: x[1]):
        cmd += f"hold {f - prev} -\nshot {tmp}/gr{t}.ppm\n"
        prev = f
    (tmp / "g.txt").write_text(cmd)
    subprocess.run([retrorun, core, rom, str(tmp / "g.txt")], check=True, capture_output=True,
                   env=dict(os.environ, RR_VARS="snes9x_layer_1=disabled;snes9x_layer_5=disabled"))
    tabs = {}
    for t in want:
        p = (tmp / f"gr{t}.ppm").read_bytes()
        # PPM P6: cabecera de 3 líneas
        hdr_end = 0
        for _ in range(3):
            hdr_end = p.index(b"\n", hdr_end) + 1
        w = int(p.split()[1])
        px = p[hdr_end:]
        tabs[t] = [((px[(y * w + 8) * 3] >> 3) | (px[(y * w + 8) * 3 + 1] >> 3) << 5 | (px[(y * w + 8) * 3 + 2] >> 3) << 10)
                   for y in range(224)]
    known = sorted(tabs)
    grad = b""
    for t in range(16):
        tt = t if t in tabs else min(known, key=lambda k: abs(k - t))
        grad += struct.pack(">224H", *tabs[tt])
    Path(out + ".grad").write_bytes(grad)
    print(f"degradado: tablas medidas {known}")


if __name__ == "__main__":
    main()
