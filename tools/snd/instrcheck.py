#!/usr/bin/env python3
"""MegaMetroid — ¿cada instrumento FM suena en la misma nota que su muestra del SNES? (docs/55)

Para cada muestra que tocan las canciones de musica.txt (con la tabla instrumentos.txt): el croma (12 notas) de la
muestra a su altura natural (32 kHz) contra el del instrumento FM que arma mksnd.py (sus armónicas k*f0 con el TL de
cada operador). Si el FM se parece más corrido k semitonos, la altura (f0) de la tabla o la medida está corrida.

Uso: instrcheck.py sm.smc [trabajo]"""
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import brr                                                          # noqa: E402
import mksnd                                                        # noqa: E402


def chroma_of(freqs, amps):
    c = np.zeros(12)
    for f, a in zip(freqs, amps):
        if 30 < f < 8000:
            c[int(round(12 * np.log2(f / 440.0))) % 12] += a ** 2
    return c / (c.sum() + 1e-12)


def sample_chroma(x, loopidx):
    seg = x[loopidx:] if loopidx is not None and len(x) - loopidx >= 32 else x[len(x) // 10:]
    N = 1 << 15
    t = np.tile(seg, N // max(1, len(seg)) + 1)[:N] if loopidx is not None else np.pad(seg, (0, max(0, N - len(seg))))[:N]
    sp = np.abs(np.fft.rfft((t - t.mean()) * np.hanning(N)))
    fr = np.fft.rfftfreq(N, 1 / 32000.0)
    return chroma_of(fr, sp)


def main():
    rom = sys.argv[1]
    work = Path(sys.argv[2] if len(sys.argv) > 2 else mksnd.ROOT / "work" / "snd")
    spc = mksnd.Spc(rom, work)
    table = mksnd.read_instr_table(HERE / "instrumentos.txt")
    seen = set()
    for song, track, tipo, frames in mksnd.read_music(HERE / "musica.txt"):
        if tipo != "xgm2":
            continue
        a, st = mksnd.run_track(spc, song, track, min(frames, 3000), work / f"ram{song:02X}.bin")
        ram = (work / f"ram{song:02X}.bin").read_bytes()
        v = st[:, :96].reshape(-1, 8, 12)
        for s in sorted(set(v[:, :, 0][v[:, :, 9] > 0].tolist())):
            if (song, s) in seen:
                continue
            seen.add((song, s))
            ins = mksnd.make_instr(ram, song, s, table)
            notes = int(((v[:, :, 9] > 0) & (v[:, :, 0] == s)).sum())
            if ins.kind != "fm" or not ins.f0:
                continue
            x, l = brr.decode(ram, s)
            cs = sample_chroma(x, l)
            fr, am = [], []
            for op in ins.ops:
                tl = op[2]
                if tl < 127:
                    fr.append(ins.f0 * max(0.5, op[1]))
                    am.append(10 ** (-tl * 0.75 / 20))
            cf = chroma_of(fr, am)
            r = [np.corrcoef(cs, np.roll(cf, -k))[0, 1] for k in range(12)]
            best = int(np.argmax(r))
            mark = "  <-- corrida" if best and r[best] - r[0] > 0.2 else ""
            print(f"{song:02X}_{s}: f0={ins.f0:.1f} notas={notes}: corrida 0 {r[0]:.2f}, la mejor {best} "
                  f"({r[best]:.2f}){mark}")


if __name__ == "__main__":
    main()
