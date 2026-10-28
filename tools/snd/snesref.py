#!/usr/bin/env python3
"""MegaMetroid — el audio del SNES de referencia (docs/55): toma los pedidos al SPC de una corrida del motor en el PC
(MM_SNDLOG=archivo host/check grabación.bin rom 0 99999: "cuadro estado up|play|sfx a b") y los toca con el N-SPC del
decomp (tools/snd/spcrun.c), cuadro a cuadro. Así se compara la Genesis (retrorun con RR_WAV) contra el SNES.

Uso: snesref.py sm.smc registro.txt salida.wav [desde hasta]"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import mksnd                                          # noqa: E402


def main():
    rom, logf, out = sys.argv[1:4]
    ev = [l.split() for l in open(logf) if l.strip()]
    f0 = int(sys.argv[4]) if len(sys.argv) > 4 else int(ev[0][0])
    f1 = int(sys.argv[5]) if len(sys.argv) > 5 else int(ev[-1][0]) + 600
    spc = mksnd.Spc(rom, HERE.parents[1] / "work" / "snd")
    lines, cur = [], None
    for e in ev:
        f = int(e[0])
        if f < f0 or f > f1:
            continue
        lines.append(f"f {f - f0 + 1}")
        if e[2] == "up":
            lines.append(f"up {e[3]}")
        elif e[2] == "play":
            lines.append(f"p 0 {e[3]}")
        elif e[2] == "sfx":
            lines.append(f"p {int(e[3]) + 1} {e[4]}")
    lines.append(f"f {f1 - f0}")
    g = Path(out).with_suffix(".txt")
    g.write_text("\n".join(lines) + "\n")
    subprocess.run([str(spc.exe), rom, str(g), out, "-"], check=True)


if __name__ == "__main__":
    main()
