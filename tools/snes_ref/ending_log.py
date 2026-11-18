#!/usr/bin/env python3
"""MegaMetroid — lo que dibuja el final (estado 39), para mkending.py (docs/63): de la salida de MM_ENDLOG (host/check
con la grabación del final) a res/snes_ref/ending_draw.log. Solo números (palabras de mapa del SNES, spritemaps y su
chr): nada de la ROM.
Grupos (escenas que comparten la VRAM del SNES): D1 créditos y Samus (cuadros 3457-12566), D2 y D2B el símbolo en
modo 7 (12567-12883 y 12884-13303: cambia la base de los sprites), D3 la pantalla final (desde 13304). Líneas: "<grupo> w <palabra>" (mapas que sube el juego: el texto) y "<grupo> s <spritemap> <chr>"
(sprites; spritemap "star" = las estrellas, chr = el de la OAM).
Uso: ending_log.py endlog.txt salida.log"""
import sys

GROUPS = (("D1", 3457, 12567), ("D2", 12567, 12884), ("D2B", 12884, 13304), ("D3", 13304, 1 << 30))


def group(k):
    for g, a, b in GROUPS:
        if a <= k < b:
            return g
    return None


words, sprs = {}, {}
last = 0
for l in open(sys.argv[1]):
    p = l.split()
    if not p:
        continue
    if p[0] == "f":
        last = int(p[1])
        continue
    k = last + 1                                  # (las líneas del cuadro van antes de su "f")
    if p[0] == "bgt":                             # (este va después de la "f" de su cuadro)
        k = last
    g = group(k)
    if g is None:
        continue
    if p[0] in ("text", "bgt"):
        words.setdefault(g, set()).update(int(x, 16) for x in p[2:])
    elif p[0] == "spr":
        sprs.setdefault(g, set()).add((p[1], p[4]))
    elif p[0] == "star":
        sprs.setdefault(g, set()).add(("star", p[3]))
with open(sys.argv[2], "w") as o:
    o.write("# MegaMetroid: lo que dibuja el final (tools/snes_ref/ending_log.py). No editar.\n")
    for g, _, _ in GROUPS:
        for v in sorted(words.get(g, ())):
            o.write("%s w %04X\n" % (g, v))
        for sm, chr_ in sorted(sprs.get(g, ())):
            o.write("%s s %s %s\n" % (g, sm, chr_))
print({g: (len(words.get(g, ())), len(sprs.get(g, ()))) for g, _, _ in GROUPS})
