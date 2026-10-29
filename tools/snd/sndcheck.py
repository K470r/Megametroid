#!/usr/bin/env python3
"""MegaMetroid — comprueba que cada pedido de sonido del SNES tenga qué tocar en la Genesis (docs/55).

Lee las tablas que armó mksnd.py (res/gen/md/md_snd_data.s: lo fijo, la página de cada conjunto y las tablas de cada
página) y los pedidos al SPC de una corrida del motor en el PC (MM_SNDLOG=archivo host/check ...: "cuadro estado
up|play|sfx a b"), y hace lo mismo que music_upload / music_play / sfx_play (src/platform/md_snd.c): con el conjunto
cargado, ¿está la pista en su página (o es del motor)? ¿está el efecto en su página o en lo fijo? Lista lo que falta.

Uso: sndcheck.py md_snd_data.s registro.txt [conjunto inicial en hex]"""
import re
import sys


def tables(path):
    s = open(path).read()
    fixed, music_fixed, pages, set_page = set(), set(), {}, [0] * 256
    for blk in re.split(r"\n    \.global ", s):
        name = blk.split("\n", 1)[0].strip()
        rows_sfx = re.findall(r"\.byte (\d+), (\d+), (\d+), (\d+), 0, 0", blk)
        rows_mus = re.findall(r"\.byte (\d+), (\d+), ([12]), 0\n    \.long kSndPcm", blk)
        if name == "kSndSfx":
            fixed = {(int(a), int(b)) for a, b, c, d in rows_sfx}
        elif name == "kSndMusic":
            music_fixed = {int(b) for a, b, c in rows_mus}
        elif name == "kSndSetPage":
            set_page = [int(x) for x in re.search(r"\.byte ([\d, ]+)", blk).group(1).split(",")]
        else:
            m = re.match(r"kSndPage(\d+)(Music|Sfx)$", name)
            if m:
                pg = pages.setdefault(int(m.group(1)), {"music": set(), "sfx": set()})
                if m.group(2) == "Music":
                    pg["music"] = {(int(a), int(b)) for a, b, c in rows_mus}
                else:
                    pg["sfx"] = {(int(a), int(b), int(c)) for a, b, c, d in rows_sfx}
    return fixed, music_fixed, pages, set_page


def main():
    fixed, music_fixed, pages, set_page = tables(sys.argv[1])
    cur = int(sys.argv[3], 16) if len(sys.argv) > 3 else None
    miss, ok = {}, 0
    for line in open(sys.argv[2]):
        p = line.split()
        if len(p) < 5:
            continue
        f, kind, a, b = int(p[0]), p[2], int(p[3]), int(p[4])
        if kind == "up":
            cur = a
            continue
        pg = pages.get(set_page[cur], {"music": set(), "sfx": set()}) if cur is not None else {"music": set(), "sfx": set()}
        if kind == "play":
            if a == 0 or a in music_fixed or (cur, a) in pg["music"]:
                ok += 1
            else:
                miss.setdefault(("pista", cur, a), []).append(f)
        elif kind == "sfx":
            lib = a + 1
            if (lib, b) in fixed or (lib, b, cur) in pg["sfx"] or (lib, b, 0xFF) in pg["sfx"]:
                ok += 1
            else:
                miss.setdefault(("efecto", cur, (lib, b)), []).append(f)
    print(f"{ok} pedidos con sonido; {sum(len(v) for v in miss.values())} sin sonido")
    for k, fr in sorted(miss.items(), key=lambda t: t[1][0]):
        what = f"pista {k[2]}" if k[0] == "pista" else f"efecto {k[2][0]}/{k[2][1]:#04x}"
        cs = "?" if k[1] is None else f"{k[1]:#04x}"
        print(f"  conjunto {cs}: {what}: {len(fr)} veces (cuadro {fr[0]})")


if __name__ == "__main__":
    main()
