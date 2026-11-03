#!/usr/bin/env python3
"""MegaMetroid — compara el sonido de una sala, Genesis contra SNES (tools/snd/salas.sh, docs/55).

En la carpeta: log.txt (pedidos al SPC, MM_SNDLOG de host/check), gen.wav (Genesis con MD_SND_NOSFX, RR_WAV) y tr.bin
(md_shown_frame y md_snd_ev, el último pedido de sonido, en cada cuadro de la consola). Hace snes.wav con tools/snd/snesref.py (solo los pedidos de música) y
mide:
  - pedidos sin sonido en la Genesis (tools/snd/sndcheck.py con las tablas de mksnd.py);
  - para cada pedido de pista, desde el cuadro del pedido en cada una: el desfase de la Genesis (±30 cuadros, por la
    correlación de las envolventes en dB);
  - con ese desfase, 20 s: la correlación de la envolvente y del croma (12 notas) cada 0,1 s (donde suena) y del
    croma de todo el tramo.

Uso: sndcmp.py sm.smc md_snd_data.s carpeta nombre desplazamiento
  (desplazamiento: cuadro del motor de la Genesis = cuadro de la grabación + desplazamiento; salas.sh: FROM - T0)"""
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
MD_FPS = 53693175 / (3420 * 262)                     # cuadros por segundo de la Genesis NTSC (Genesis Plus GX)


def read_wav(p):
    w = wave.open(str(p))
    r = w.getframerate()
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).reshape(-1, 2).mean(axis=1)
    return x, r


def env60(x, r, fps=60.0):
    """RMS por cuadro. fps: el del sistema que grabó (SNES: 534 muestras a 32 040 Hz = 60; Genesis Plus GX: NTSC, 53 693 175 /
    (3420 * 262) = 59,922 por segundo: con 60 se corre 17 cuadros a los 13 000)"""
    n = int(len(x) * fps / r)
    s = (np.arange(n + 1) * r / fps).astype(int)
    return np.array([np.sqrt(np.mean(x[s[i]:s[i + 1]] ** 2) + 1e-9) for i in range(n)])


def chroma(x, r, step=0.1, win=0.4):
    """croma (12 notas) cada `step` s, con ventanas de `win` s: la misma resolución en Hz con cualquier frecuencia de
    muestreo (el SNES graba a 32 040 Hz y Genesis Plus GX a 44 100: con ventanas de igual cantidad de muestras, a
    60-130 Hz cada casilla del espectro cubre más de un semitono y en distinto lugar)"""
    n, hop = int(r * win), int(r * step)
    fr = np.fft.rfftfreq(n, 1 / r)
    m = (fr > 80) & (fr < 4000)
    pc = np.round(12 * np.log2(fr[m] / 440.0)).astype(int) % 12
    w = np.hanning(n)
    out = []
    for i in range(0, len(x) - n, hop):
        sp = np.abs(np.fft.rfft(x[i:i + n] * w)) ** 2
        c = np.bincount(pc, weights=sp[m], minlength=12)
        out.append(c / (c.sum() + 1e-9))
    return np.array(out)


def main():
    rom, tab, d, name, off = sys.argv[1], sys.argv[2], Path(sys.argv[3]), sys.argv[4], int(sys.argv[5])
    log = [l.split() for l in open(d / "log.txt") if l.strip()]
    if not log:
        print(f"{name}: sin pedidos en el registro")
        return
    chk = subprocess.run([sys.executable, str(HERE / "sndcheck.py"), tab, str(d / "log.txt"), "06"],
                         capture_output=True, text=True).stdout
    f0 = int(log[0][0]) - 30
    f1 = int(log[-1][0]) + 300
    (d / "log_musica.txt").write_text("".join(" ".join(l) + "\n" for l in log if l[2] in ("up", "play")))
    subprocess.run([sys.executable, str(HERE / "snesref.py"), rom, str(d / "log_musica.txt"), str(d / "snes.wav"),
                    str(f0), str(f1)], check=True, capture_output=True)
    a, ra = read_wav(d / "snes.wav")
    g, rg = read_wav(d / "gen.wav")
    ea, eg = env60(a, ra), env60(g, rg, MD_FPS)
    tr = np.fromfile(d / "tr.bin", dtype="<u2").reshape(-1, 3)   # md_shown_frame, md_snd_ev[0..1] por cuadro
    shown, evn, evc = tr[:, 0], tr[:, 1], tr[:, 2]
    # los pedidos de pista de la Genesis (md_snd_ev, ROM de prueba): el cuadro de la consola de cada uno, después del
    # salto (el primer cuadro que muestra el motor en el cuadro de la grabación del primer pedido del registro)
    c_from = int(np.argmax(shown >= off + int(log[0][5]) - 2))
    gplays = [(c, int(evc[c]) & 0xFF) for c in range(max(1, c_from), len(tr))
              if evn[c] != evn[c - 1] and int(evc[c]) >> 8 == 2]
    out = []
    for l in log:
        if l[2] != "play" or l[3] == "0":
            continue
        fs = int(l[0]) - f0                                 # cuadro del pedido en snes.wav
        m = [c for c, t in gplays if t == int(l[3])]
        if not m:
            out.append(f"pista {l[3]}: la Genesis no la pidió")
            continue
        cg = m[0]
        gplays = [x for x in gplays if x[0] > cg]
        # el desfase: la correlación de las envolventes (en dB) desde el pedido, 20 s, corriendo la Genesis ±30 cuadros
        n = min(len(ea) - fs, len(eg) - cg, 1200) - 30
        if n < 120:
            out.append(f"pista {l[3]}: muy cerca del final de la grabación")
            continue
        A = np.log(ea[fs:fs + n] + 1)
        ce, lag = max((np.corrcoef(A, np.log(eg[cg + k:cg + k + n] + 1))[0, 1], k) for k in range(-30, 31)
                      if cg + k >= 0)
        og = cg + lag
        ca = chroma(a[int(fs * ra / 60):int((fs + n) * ra / 60)], ra)
        cgm = chroma(g[int(og * rg / MD_FPS):int((og + n) * rg / MD_FPS)], rg)
        k = min(len(ca), len(cgm))
        cc = [np.corrcoef(ca[i], cgm[i])[0, 1] for i in range(k) if ea[min(fs + i * 6, len(ea) - 1)] > 30]
        tot = np.corrcoef(ca[:k].mean(axis=0), cgm[:k].mean(axis=0))[0, 1]
        out.append(f"pista {l[3]}: la Genesis {lag:+d} cuadros del SNES (+: después); envolvente {ce:.2f}, "
                   f"croma {np.nanmean(cc):.2f} cada 0,1 s ({len(cc)} ventanas), {tot:.2f} en los 20 s")
    print(f"{name}: {chk.splitlines()[0] if chk else '?'}")
    for o in out:
        print("   " + o)
    for l in chk.splitlines()[1:]:
        print("   " + l)


if __name__ == "__main__":
    main()
