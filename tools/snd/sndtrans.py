#!/usr/bin/env python3
"""MegaMetroid — ¿la música de la Genesis está corrida de altura? (docs/55). Con las grabaciones de tools/snd/salas.sh:
el croma medio de 20 s desde cada pedido de pista, SNES contra Genesis corrida 0..11 semitonos.

Uso: sndtrans.py carpeta_de_la_sala desplazamiento"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sndcmp                                                       # noqa: E402

d, off = Path(sys.argv[1]), int(sys.argv[2])
log = [l.split() for l in open(d / "log.txt") if l.strip()]
f0 = int(log[0][0]) - 30
a, ra = sndcmp.read_wav(d / "snes.wav")
g, rg = sndcmp.read_wav(d / "gen.wav")
tr = np.fromfile(d / "tr.bin", dtype="<u2").reshape(-1, 3)
plays = [c for c in range(1, len(tr)) if tr[c, 1] != tr[c - 1, 1] and tr[c, 2] >> 8 == 2 and tr[c, 2] & 255]
c_from = int(np.argmax(tr[:, 0] >= off + int(log[0][5]) - 2))
plays = [c for c in plays if c >= c_from]
for l in log:
    if l[2] == "play" and l[3] != "0" and plays:
        fs, cg = int(l[0]) - f0, plays.pop(0)
        A = sndcmp.chroma(a[int(fs * ra / 60):int((fs + 1200) * ra / 60)], ra).mean(axis=0)
        G = sndcmp.chroma(g[int(cg * rg / sndcmp.MD_FPS):int((cg + 1200) * rg / sndcmp.MD_FPS)], rg).mean(axis=0)
        r = [np.corrcoef(A, np.roll(G, -k))[0, 1] for k in range(12)]
        print(f"{d.name} pista {l[3]}: corrida 0: {r[0]:.2f}; la mejor: {int(np.argmax(r))} semitonos ({max(r):.2f})")
        print("   SNES   ", np.round(A / A.max(), 2))
        print("   Genesis", np.round(G / G.max(), 2))
