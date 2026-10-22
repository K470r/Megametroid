#!/usr/bin/env python3
"""Instantáneas del SNES real para el menú y la intro (make_menu_data.py las convierte).

Corre tu ROM en snes9x desde el encendido con una secuencia fija de botones (START en el título,
START en el slot A, A en "START GAME", A en cada pausa de la intro) y guarda, en cuadros
elegidos, la VRAM (64 KB) y la WRAM (128 KB: registros sombra, palette_buffer, OAM).

Uso: record_menus.py retrorun snes9x_libretro.so sm.smc carpeta_salida
Salida: <nombre>.vram y <nombre>.wram por instantánea (ver SNAPS).
"""
import subprocess, sys, tempfile
from pathlib import Path

# (cuadros a esperar, botones durante 'pulsar' cuadros, pulsar) y luego instantánea con nombre
SCRIPT = [
    ("wait", 1900), ("press", "S", 2), ("wait", 120), ("snap", "fsel"),
    ("press", "S", 2), ("wait", 160), ("snap", "opt"),
    ("press", "A", 6), ("wait", 150), ("snap", "page1"),
    ("wait", 1350), ("snap", "page2"),
]
# rondas de la intro: A y 8 x 60 cuadros (ver docs/30); instantáneas en (ronda, paso)
ROUNDS = {(2, 4): "mbdemo", (4, 8): "page3", (5, 5): "larva", (7, 8): "page4", (8, 2): "scient",
          (10, 8): "page5"}


def main():
    retrorun, core, rom, outdir = sys.argv[1:5]
    out = Path(outdir)
    out.mkdir(parents=True, exist_ok=True)
    cmd = []
    for st in SCRIPT:
        if st[0] == "wait":
            cmd.append(f"hold {st[1]} -")
        elif st[0] == "press":
            cmd.append(f"hold {st[2]} {st[1]}")
        else:
            cmd += [f"dumpvram {out / (st[1] + '.vram')}", f"dumpram {out / (st[1] + '.wram')}",
                    f"shot {out / (st[1] + '.ppm')}"]
    for r in range(1, 11):
        cmd.append("hold 4 A")
        for j in range(1, 9):
            cmd.append("hold 60 -")
            if (r, j) in ROUNDS:
                n = ROUNDS[(r, j)]
                cmd += [f"dumpvram {out / (n + '.vram')}", f"dumpram {out / (n + '.wram')}", f"shot {out / (n + '.ppm')}"]
    tmp = Path(tempfile.mkdtemp())
    (tmp / "m.txt").write_text("\n".join(cmd) + "\n")
    subprocess.run([retrorun, core, rom, str(tmp / "m.txt")], check=True, capture_output=True)
    print("instantáneas:", sorted(p.stem for p in out.glob("*.vram")))


if __name__ == "__main__":
    main()
