#!/usr/bin/env python3
"""MegaMetroid — lo que dibuja el final (estado 39), para mkending.py (docs/63): de la salida de MM_ENDLOG (host/check
con una grabación del final) a líneas de res/snes_ref/ending_draw.log. Solo números (palabras de mapa del SNES,
spritemaps y su chr): nada de la ROM.
Grupos (escenas que comparten la VRAM del SNES), elegidos por el estado como los elige la Genesis (md_ending.c,
ed_want): la escena (EndingGfx), el modo, OBSEL y BG1SC. D0A-D0T la huida, el espacio y la nave; D1 créditos y Samus;
D2 y D2B Samus chica y el símbolo; D3 la pantalla final. Con sufijo, los de otro final (el tiempo de juego: _5 de 3 a
9 horas, _12 de 10 en adelante): solo los que cambian (D1, D2, D2B).
Líneas: "<grupo> w <palabra>" (mapas que sube el juego) y "<grupo> s <spritemap> <chr>" (sprites; spritemap "star" =
las estrellas, chr = el de la OAM).
Uso: ending_log.py endlog.txt salida.log [sufijo]   (con sufijo, agrega al final de salida.log)"""
import sys

suf = sys.argv[3] if len(sys.argv) > 3 else ""
ONLY = {"D1", "D2", "D2B"} if suf else None
words, sprs = {}, {}
scene, space, grp = 0, False, None


def group(mode, obsel, bg1sc):
    global space
    if scene == 1:
        return "D0A"
    if scene == 2:
        return "D0B"
    if scene == 3:
        if mode != 7:
            space = True
            return "D0S"
        return "D0T" if space else "D0C"
    if scene >= 4:
        if mode == 7:
            return "D2" if obsel == 0 else "D2B"
        return "D3" if bg1sc == 0x4E else "D1"
    return None


pend = []                                         # líneas del cuadro (van antes de su "f")
for l in open(sys.argv[1]):
    p = l.split()
    if not p:
        continue
    if p[0] == "gfx":
        scene = int(p[1])
        continue
    if p[0] == "f":
        g = group(int(p[3]), int(p[29], 16), int(p[30], 16))
        for q in pend:
            if g and (ONLY is None or g in ONLY):
                if q[0] == "text":
                    words.setdefault(g, set()).update(int(x, 16) for x in q[2:])
                elif q[0] == "spr":
                    sprs.setdefault(g, set()).add((q[1], q[4]))
                elif q[0] == "star":
                    sprs.setdefault(g, set()).add(("star", q[3]))
        pend = []
        grp = g
        continue
    if p[0] == "bgt":                             # (va después de la "f" de su cuadro)
        if grp and (ONLY is None or grp in ONLY):
            words.setdefault(grp, set()).update(int(x, 16) for x in p[2:])
        continue
    pend.append(p)
with open(sys.argv[2], "a" if suf else "w") as o:
    if not suf:
        o.write("# MegaMetroid: lo que dibuja el final (tools/snes_ref/ending_log.py). No editar.\n")
    for g in sorted(set(words) | set(sprs)):
        for v in sorted(words.get(g, ())):
            o.write("%s%s w %04X\n" % (g, suf, v))
        for sm, chr_ in sorted(sprs.get(g, ())):
            o.write("%s%s s %s %s\n" % (g, suf, sm, chr_))
print({g + suf: (len(words.get(g, ())), len(sprs.get(g, ()))) for g in sorted(set(words) | set(sprs))})
