#!/usr/bin/env python3
"""Qué usa la intro del SNES en cada escena (lo lee tools/conv/mkintro.py; docs/38).

Entrada: la RAM del SNES cuadro a cuadro desde que empieza la intro (grabación de hlref con HL_ANY desde el cuadro
siguiente a CinematicFunction_Intro_Initial, como /tmp/.../ref_intro.bin) y los gráficos extraídos de tu ROM
(data/extracted_full: el mapa inicial de BG3, $97:8D12).

Escenas: las de IntroScene (src/core/intro.c), con la misma lógica (fundido cruzado incluido). Por escena:
  * bgA: palabras del mapa que se ve en el plano A de la Genesis (BG3 en $4C00, o BG1 en $5C00 en "SPACE COLONY").
    Se sigue lo que el SNES tiene en la VRAM: el mapa inicial y, mientras el juego lo manda
    (HandleCinematicBgObjects: objetos y mapa activados, fuera del modo 7), las filas 0-29 de 7E:3000.
  * eyes: palabras de los ojos del casco (filas 13-14 de 7E:3800, UpdateSamusEyesTilemap) en la escena del diario.
  * classes: índices de palette_buffer que tienen siempre el mismo color durante la escena (fundidos y efectos
    incluidos): comparten un color de la Genesis.
  * m7 (vuelo): registros de modo 7 de cada cuadro (M7A-D, M7X/Y, BG1HOFS/VOFS) sin repetir y qué mapa de la nave
    está en la VRAM (0 de frente; 1 de atrás, desde el destello de CinematicFunction_Intro_Func56).

Uso: intro_usage.py ref_intro.bin data/extracted_full salida.json
"""
import json
import sys
from pathlib import Path

import numpy as np

R = 0x20000
SCENES = ["CAPT", "TEXT", "MB", "BABY", "SCIENTIST", "LAB", "FLY", "SPACE"]


def w16(r, a):
    return int(r[a]) | int(r[a + 1]) << 8


def s16(v):
    return v - 65536 if v >= 32768 else v


def fb_scene(sc):
    return {0x54: 3, 0x58: 4, 0x5C: 5}.get(sc, 2)


def scene_of(r, shown):
    """IntroScene (src/core/intro.c): escena y brillo (0-64)"""
    tm, sc = int(r[0x69]), int(r[0x58]) & 0xFC
    if tm != 0x06:
        if tm == 0x04:
            return 0, 64
        if tm == 0x16:
            return 1, 64
        if tm == 0x15:
            return fb_scene(sc), 64
        if tm == 0x11:
            return (6 if (int(r[0x55]) & 7) == 7 else 7), 64
        return shown, 64
    cf = w16(r, 0x1F51)
    to_fb = cf in (0xB250, 0xB2D2)
    p = 127 - s16(w16(r, 0x1A49 if to_fb else 0x1A4B))
    p = max(0, min(128, p))
    fb = fb_scene(sc)
    if p < 64:
        return (1 if to_fb else fb), 64 - p
    return (fb if to_fb else 1), p - 64


def main():
    ref = np.memmap(sys.argv[1], dtype=np.uint8, mode="r")
    ext = Path(sys.argv[2])
    n = (len(ref) - 0x30000) // (R + 2)
    frames = ref[0x30000:0x30000 + n * (R + 2)].reshape(n, R + 2)[:, 2:]
    blob = next((ext / "blobs" / "decompressed").glob("978D12_*.bin")).read_bytes()
    shadow = np.frombuffer(blob[:0x800], "<u2").copy()          # BG3 en $4C00 (DMA de Intro_Initial)
    dest = 0x4C00
    eyes = None
    shown = 0
    out = {s: {"frames": 0, "bgA": set(), "eyes": set()} for s in SCENES}
    pal = {s: [] for s in SCENES}
    m7 = {}
    for k in range(n):
        r = frames[k]
        if w16(r, 0x998) != 30:
            break
        mode7 = (int(r[0x55]) & 7) == 7
        objs = w16(r, 0x19F1) & 0x8000
        if objs and (w16(r, 0x19F3) & 0x8000) and not mode7:
            d = w16(r, 0x19F5)
            if d != dest:
                dest = d
                shadow[:] = 0
            shadow[:960] = np.frombuffer(bytes(r[0x3000:0x3000 + 1920]), "<u2")
        if objs and not mode7:
            eyes = np.frombuffer(bytes(r[0x3800 + 832:0x3800 + 832 + 128]), "<u2").copy()
        sc, factor = scene_of(r, shown)
        shown = sc
        name = SCENES[sc]
        d = out[name]
        d["frames"] += 1
        pal[name].append(np.frombuffer(bytes(r[0xC000:0xC200]), "<u2") & 0x7FFF)
        if name != "FLY":
            d["bgA"] |= set(int(x) for x in shadow)
        if name == "TEXT" and eyes is not None:
            d["eyes"] |= set(int(x) for x in eyes)
        if name == "FLY":
            tex = 0 if w16(r, 0x1F51) in (0xBDE4, 0xBDF9) else 1
            m7.setdefault(tuple(w16(r, a) for a in (0x78, 0x7A, 0x7C, 0x7E, 0x80, 0x82, 0xB1, 0xB3)) + (tex,), len(m7))
    js = {}
    for s in SCENES:
        series = pal[s]
        if series:
            a = np.stack(series)
            keyof, cls = {}, []
            for i in range(256):
                cls.append(keyof.setdefault(a[:, i].tobytes(), i))
        else:
            cls = list(range(256))
        js[s] = {"frames": out[s]["frames"], "bgA": sorted(out[s]["bgA"]), "eyes": sorted(out[s]["eyes"]), "classes": cls}
    js["FLY"]["m7"] = [list(p) for p in m7]
    Path(sys.argv[3]).write_text(json.dumps(js))
    for s in SCENES:
        print(s, js[s]["frames"], "cuadros; plano A", len(js[s]["bgA"]), "ojos", len(js[s]["eyes"]))
    print("modo 7:", len(m7), "juegos de registros")


if __name__ == "__main__":
    main()
