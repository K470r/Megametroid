#!/usr/bin/env python3
"""MegaMetroid — compara la ROM nativa de la Genesis (src/platform, compilada con TEST=1) con el SNES (docs/32).

1. Corre la ROM en Genesis Plus GX (retrorun) con la entrada de la grabación del bot, captura cada cuadro y
   registra variables del motor (cuadro del motor, sala, Samus, cámara).
2. Variables: deben ser iguales a la RAM del SNES en el mismo cuadro del juego (el motor es el mismo).
3. Imagen: cada captura de la Genesis contra la del SNES del mismo cuadro del juego (diferencia media por píxel,
   en colores de la Genesis), y una hoja con pares lado a lado.

Uso: native_compare.py rom.bin symbol.txt ref.bin snes_shots_dir out_dir [cuadros_emu] [cada]
"""
import struct
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

RETRORUN = "/home/claude/m68kbin/retrorun"
CORE = "/home/claude/gpgx/genesis_plus_gx_libretro.so"
REF_FIRST = 10659          # la grabación hlref empieza tras este cuadro global
INIT = 97                  # índice de la grabación del estado inicial del motor
VARS = [("md_frame_count", None), ("room_ptr", 0x79B), ("samus_x_pos", 0xAF6), ("samus_y_pos", 0xAFA),
        ("layer1_x_pos", 0x911), ("layer1_y_pos", 0x915), ("samus_pose", 0xA1C), ("game_state", 0x998),
        ("samus_health", 0x9C2), ("random_number", 0x5E5), ("enemy_data+2", 0xF7A), ("enemy_data+6", 0xF7E),
        ("enemy_data+48", 0xFA8), ("md_busy", None)]
DAC = [0, 52, 87, 116, 144, 172, 206, 255]


def to_genesis(img):
    """RGB del SNES -> nivel de 3 bits de la Genesis (el del DAC más cercano, como kCram555), escalado x36"""
    a = np.asarray(img).astype(int)
    lv = np.array(DAC)
    return np.abs(a[..., None] - lv).argmin(-1) * 36


def gen_levels(img):
    """captura de Genesis Plus GX (su escala de 3 bits a 8) -> nivel de 3 bits, escalado x36"""
    a = np.asarray(img).astype(int)
    return np.clip(np.rint(a / 34.0), 0, 7).astype(int) * 36


def main():
    rom, symf, ref, snes_dir, out = sys.argv[1:6]
    nframes = int(sys.argv[6]) if len(sys.argv) > 6 else 2400
    every = int(sys.argv[7]) if len(sys.argv) > 7 else 60
    out = Path(out)
    (out / "g").mkdir(parents=True, exist_ok=True)
    syms = {}
    for l in Path(symf).read_text().splitlines():
        p = l.split()
        if len(p) >= 3:
            syms[p[2]] = int(p[0], 16) & 0xFFFF
    def sym(n):
        b, _, o = n.partition("+")
        return syms[b] + int(o or 0)
    addrs = ",".join("%X:2" % sym(n) for n, _ in VARS)
    script = out / "run.txt"
    script.write_text(f"trace {out}/trace.bin {addrs}\nshotseq {out}/g/g\nhold {nframes} -\n")
    subprocess.run([RETRORUN, CORE, rom, str(script)], check=True, env={"RR_PAD": "6"}, capture_output=True)
    tr = (out / "trace.bin").read_bytes()
    rec = 2 * len(VARS)
    rows = [struct.unpack_from("<%dH" % len(VARS), tr, i) for i in range(0, len(tr) - rec + 1, rec)]
    f = open(ref, "rb")

    def snes_ram(k):
        f.seek(0x30000 + (INIT + k) * 0x20002 + 2)
        return f.read(0x20000)
    # variables
    bad = 0
    seen = set()
    for t, r in enumerate(rows):
        k = r[0]
        if k == 0 or k in seen or r[-1]:
            continue                       # md_busy: el motor estaba a mitad de un cuadro (cuadro perdido)
        seen.add(k)
        ram = snes_ram(k)
        for (name, a), v in zip(VARS[1:-1], r[1:-1]):
            sv = ram[a] | ram[a + 1] << 8
            if sv != v:
                if bad < 20:
                    print(f"cuadro del motor {k}: {name} genesis {v:04X} snes {sv:04X}")
                bad += 1
    print(f"variables: {len(seen)} cuadros del motor, {bad} diferencias; última sala {rows[-1][1]:04X}")
    # imágenes: la captura del cuadro de emulación t muestra lo que el motor dejó en el cuadro anterior
    diffs = []
    pairs = []
    for t in range(1, len(rows)):
        k = rows[t - 1][0]
        if k == 0:
            continue
        sf = REF_FIRST + INIT + k + 1
        sp = Path(snes_dir) / f"s{sf:05d}.ppm"
        gp = out / "g" / f"g{t:05d}.ppm"
        if not sp.exists() or not gp.exists():
            continue
        if k % every == 0 and (not pairs or pairs[-1][0] != k):
            s = to_genesis(Image.open(sp).convert("RGB"))
            g = gen_levels(Image.open(gp).convert("RGB"))
            d = np.abs(s - g).sum(-1)
            diffs.append((k, rows[t - 1][1], float(d.mean()), float((d > 60).mean())))
            pairs.append((k, sp, gp))
    for k, room, m, frac in diffs:
        print(f"cuadro {k:5d} sala {room:04X}: diferencia media {m:6.1f}, píxeles muy distintos {100 * frac:5.1f}%")
    # hoja: SNES | Genesis
    if pairs:
        W = 2
        im = Image.new("RGB", (512 * W + 8 * (W - 1), 224 * ((len(pairs) + W - 1) // W)))
        for i, (k, sp, gp) in enumerate(pairs):
            x0, y0 = (i % W) * (512 + 8), (i // W) * 224
            im.paste(Image.open(sp).convert("RGB"), (x0, y0))
            im.paste(Image.open(gp).convert("RGB"), (x0 + 256, y0))
        im.save(out / "sheet.png")


if __name__ == "__main__":
    main()
