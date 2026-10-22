#!/usr/bin/env python3
"""MegaMetroid — gráficos de la intro para la Genesis (docs/38).

La lógica de la intro es el motor nativo (native/src/intro.c); aquí se convierte lo que en el SNES está en la VRAM y
la CGRAM, en la forma que dibuja md/md_intro.c. Entradas:
  rom                      tu ROM del SNES (no se distribuye)
  data/extracted_full      los gráficos extraídos de tu ROM (tools/sm_extract): los bloques comprimidos de la intro
  snes_ref/intro_usage.json  qué usa cada escena (tools/snes_ref/intro_usage.py)
  snes_ref/intro_draw.log    qué spritemaps dibuja el motor en cada escena (registro de dibujo del PC, MM_DRAWLOG)

VRAM del SNES en la intro (CinematicFunction_Intro_Initial, $8B:A395), en palabras:
  $0000 tiles de BG1/BG2 (95F90E)   $4000 fuente de BG3 (95D089)   $4800 mapa del casco (9788CC)
  $4C00 mapa de BG3 (978D12)        $5000 mapas de los recuerdos (96FF14)
  $6000 sprites (ROM $9A:D200)      $6300 tiles del rayo (UpdateBeamTilesAndPalette)   $6E00 (95E4C2)
Desde la página 1 ($8B:A66F), $4180-$447F (texto japonés) quedan limpios.
En el vuelo (CinematicFunction_Intro_Func54): modo 7 con la nave (95A82F / 96FE69) y los tiles del espacio (96D10A)
en $6000.

Grupos (cada uno, tiles y paletas propios; cambiar de grupo es cargar todo con la pantalla en negro):
  CAPT, TEXT, MB, BABY, SCIENTIST, LAB y FLIGHT (vuelo + "SPACE COLONY", que comparte los sprites: el SNES no apaga
  la pantalla entre los dos).
Por grupo:
  * 4 líneas de la CRAM; cada color de la Genesis es un índice de palette_buffer (se toma en cada cuadro: fundidos
    y efectos de paleta tal cual). Con Samus (MB, BABY) la línea 2 es su paleta (índices 192-207) como en el juego;
    en el vuelo, una línea para la nave (índices 80-95).
  * plano A = BG3 (texto, franjas) o BG1 de "SPACE COLONY": tabla palabra del SNES -> celda (se convierte en el
    momento lo que el juego cambia).
  * plano B = BG2 (casco) o BG1 (recuerdo): mapa fijo y tabla para los ojos del casco.
  * sprites: cada clave del registro de dibujo (spritemap $8C y atributo, proyectil, estela) ya en registros de
    sprites de la Genesis (los de md_put_recs). En el vuelo, cada spritemap grande se compone como imagen y se cubre
    con sprites de hasta 32x32 (la estación y los asteroides son ~140 sprites del SNES; la Genesis tiene 64).
  * la nave del vuelo: cada juego de registros de modo 7 dibujado como el PPU del SNES, en tiles.

Uso: mkintro.py rom extracted_full snes_ref salida_gen
"""
import json
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent.parent / "tools" / "genesis_convert"))
import mkmd  # noqa: E402
from convert_rooms import genesis_tile_bytes, pack_palettes  # noqa: E402

SCENES = ["CAPT", "TEXT", "MB", "BABY", "SCIENTIST", "LAB", "FLY", "SPACE"]
GROUPS = [("CAPT", ["CAPT"]), ("TEXT", ["TEXT"]), ("MB", ["MB"]), ("BABY", ["BABY"]), ("SCIENTIST", ["SCIENTIST"]),
          ("LAB", ["LAB"]), ("FLIGHT", ["FLY", "SPACE"])]
SAMUS_GROUPS = ("MB", "BABY")
SAMUS_LINE = 2
SHIP_LINE = 3
# VRAM de la Genesis en la intro (planos de 64x32 en $A000 y $C000, sprites en $E800, scroll en $EC00): zonas de
# tiles. 223-255 quedan para el cañón y Samus (md_engine.c).
REGIONS = [(1, 223), (256, 1280), (1408, 1536), (1664, 1856), (1920, 2048)]
SHIP_BUF = (256, 768)       # el vuelo: dos búferes de tiles de la nave (512 cada uno)
SHIP_MAX_TILES = 512
INTRO_PAGE = 19             # página del SSF2 con los datos de la intro (la nave, en las siguientes)


def snes_vram(rom, blob, japanese_cleared):
    vr = bytearray(0x10000)

    def put(w, data):
        vr[w * 2:w * 2 + len(data)] = data
    put(0x0000, blob("95F90E")[:0x8000])
    put(0x4000, blob("95D089")[:0x900])
    put(0x4800, blob("9788CC")[:0x800])
    put(0x4C00, blob("978D12")[:0x800])
    put(0x5000, blob("96FF14")[:0x2000])
    put(0x6000, rom.bytes(0x9AD200, 0x2000))
    put(0x6300, rom.bytes(0x9AF200, 0x100))
    put(0x6E00, blob("95E4C2")[:0x2400])
    if japanese_cleared:            # ClearJapaneseTextTiles: 8 palabras de 7F:8290 repetidas
        pat = blob("95D089")[0x290:0x2A0]
        put(0x4180, pat * (0x600 // 16))
    return np.frombuffer(bytes(vr), dtype="<u2")


def flight_vram(rom, blob, tex=0):
    """VRAM del SNES en el vuelo: tiles de modo 7 en los bytes altos, el mapa de la nave en los bajos (tex = 1: la
    nave de atrás, que Func56 manda en el destello) y los tiles del espacio en $6000"""
    vr = np.zeros(0x8000, np.uint16)
    hi = np.frombuffer(blob("95A82F")[:0x4000], np.uint8).astype(np.uint16)
    vr[:0x4000] = (hi << 8) | 0x8C
    lo = np.frombuffer(blob("96FE69")[tex * 0x300:tex * 0x300 + 0x300], np.uint8).astype(np.uint16)
    vr[:0x300] = (vr[:0x300] & 0xFF00) | lo
    vr[0x6000:0x8000] = np.frombuffer(blob("96D10A")[:0x4000], "<u2")
    return vr


def tile(vr, base, ch, bpp):
    a = base + ch * (16 if bpp == 4 else 8)
    out = np.zeros((8, 8), int)
    for y in range(8):
        w0 = int(vr[(a + y) & 0x7FFF])
        planes = [w0 & 255, w0 >> 8]
        if bpp == 4:
            w1 = int(vr[(a + 8 + y) & 0x7FFF])
            planes += [w1 & 255, w1 >> 8]
        for x in range(8):
            px = 0
            for p, b in enumerate(planes):
                px |= ((b >> (7 - x)) & 1) << p
            out[y, x] = px
    return out


def obj_px(vr, ch, big, pal):
    """píxeles (índice de CGRAM, 0 transparente) de un sprite del SNES de 8x8 o 16x16 (con la vuelta de fila del
    SNES dentro de cada tabla de 256 tiles)"""
    n = 2 if big else 1
    m = np.zeros((8 * n, 8 * n), int)
    for cy in range(n):
        for cx in range(n):
            c = (ch & 0x100) | ((((ch >> 4) & 15) + cy) & 15) << 4 | (((ch & 15) + cx) & 15)
            t = tile(vr, 0x6000, c, 4)
            m[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] = np.where(t > 0, 128 + pal * 16 + t, 0)
    return m


# ------------------------------------------------------------------ la nave (modo 7)
def s16(v):
    return v - 65536 if v >= 32768 else v


def s13(v):
    v &= 0x1FFF
    return v - 8192 if v >= 4096 else v


def render_m7(vr, prm):
    """BG1 en modo 7 como el PPU del SNES (M7SEL = $80: fuera del mapa de 1024x1024 es transparente)"""
    A, B, C, D = (s16(x) for x in prm[:4])
    X0, Y0 = s13(prm[4]), s13(prm[5])
    H, V = s13(prm[6]), s13(prm[7])
    lo = (vr & 0xFF).astype(np.int64)
    hi = (vr >> 8).astype(np.int64)
    ys, xs = np.mgrid[0:224, 0:256]
    dx = xs + H - X0
    dy = ys + V - Y0
    tx = ((A * dx + B * dy) >> 8) + X0
    ty = ((C * dx + D * dy) >> 8) + Y0
    out = (tx < 0) | (tx >= 1024) | (ty < 0) | (ty >= 1024)
    tx &= 1023
    ty &= 1023
    t = lo[(ty >> 3) * 128 + (tx >> 3)]
    px = hi[t * 64 + (ty & 7) * 8 + (tx & 7)]
    return np.where(out, 0, px)


def ship_frames(vrs, m7list):
    """cada cuadro de la nave: celdas con algo dibujado y sus tiles (sin repetir dentro del cuadro). Registro:
    celdas (u16), tiles (u16), celdas x (fila * 32 + columna), celdas x (tile dentro del cuadro), tiles x 32 bytes.
    Un cuadro con más de SHIP_MAX_TILES tiles queda vacío (la Genesis deja el anterior)."""
    blob = bytearray()
    offs, skipped = [], []
    for i, prm in enumerate(m7list):
        px = render_m7(vrs[prm[8]], prm)
        cells = px.reshape(28, 8, 32, 8).transpose(0, 2, 1, 3)
        pos, tid, tiles, index = [], [], [], {}
        for cy in range(28):
            for cx in range(32):
                c = cells[cy, cx]
                if not c.any():
                    continue
                g = np.where(c > 0, c - 80, 0)
                assert g.max() <= 15 and (c[c > 0] >= 81).all(), "la nave usa colores fuera de la CGRAM 81-95"
                b = genesis_tile_bytes(g)
                if b not in index:
                    index[b] = len(tiles)
                    tiles.append(b)
                pos.append(cy * 32 + cx)
                tid.append(index[b])
        if len(tiles) > SHIP_MAX_TILES:
            skipped.append(i)
            pos, tid, tiles = [], [], []
        while len(blob) % 4:
            blob.append(0)
        offs.append(len(blob))
        blob += struct.pack(">HH", len(pos), len(tiles))
        blob += struct.pack(f">{len(pos)}H", *pos) + struct.pack(f">{len(tid)}H", *tid)
        while len(blob) % 4:
            blob.append(0)
        blob += b"".join(tiles)
    return bytes(blob), offs, skipped


def cover(img, alpha=0.1):
    """cubre los píxeles de la imagen con sprites de la Genesis (de 1-4 x 1-4 tiles): se toma el primer píxel aún
    sin cubrir y, entre las cajas que lo tienen en su borde de arriba, la que cubre más píxeles nuevos.
    Devuelve [(x, y, w, h)] (x, y en píxeles de la imagen; w, h en tiles), la imagen con margen y el margen."""
    M = 32
    Hh, Ww = img.shape
    pad = np.zeros((Hh + 2 * M, Ww + 2 * M), img.dtype)
    pad[M:M + Hh, M:M + Ww] = img
    todo = pad > 0
    out = []
    while todo.any():
        ys, xs = np.nonzero(todo)
        y, x = ys[0], xs[0]
        best = None
        for w in range(1, 5):
            for h in range(1, 5):
                for left in range(x - w * 8 + 1, x + 1):
                    n = int(todo[y:y + h * 8, left:left + w * 8].sum())
                    key = (n - alpha * w * h * 64, -w * h)
                    if best is None or key > best[0]:
                        best = (key, left, w, h)
        _, left, w, h = best
        todo[y:y + h * 8, left:left + w * 8] = False
        out.append((left - M, y - M, w, h))
    return out, pad, M


# ------------------------------------------------------------------ tiles de la VRAM de la Genesis
class Alloc:
    """tiles de un grupo en las zonas libres de la VRAM; los de un sprite, seguidos"""

    def __init__(self, regions):
        self.free = [list(r) for r in regions]
        self.data = {}          # tile -> 32 bytes
        self.index = {}         # bytes -> tile (tiles sueltos, sin repetir)

    def take(self, n):
        for r in self.free:
            if r[1] - r[0] >= n:
                t = r[0]
                r[0] += n
                return t
        raise RuntimeError(f"no hay lugar para {n} tiles")

    def one(self, b):
        if b not in self.index:
            t = self.take(1)
            self.index[b] = t
            self.data[t] = b
        return self.index[b]

    def run(self, bs):
        t = self.take(len(bs))
        for i, b in enumerate(bs):
            self.data[t + i] = b
        return t

    def segments(self):
        """(primer tile, bytes) de cada tramo seguido"""
        out = []
        for t in sorted(self.data):
            if out and out[-1][0] + len(out[-1][1]) // 32 == t:
                out[-1][1].extend(self.data[t])
            else:
                out.append([t, bytearray(self.data[t])])
        return [(t, bytes(b)) for t, b in out]


def read_draw_log(path):
    keys = {s: set() for s in SCENES}
    for l in Path(path).read_text().split("\n"):
        f = l.split()
        if len(f) < 3 or not f[0].startswith("C18"):
            continue
        sc = SCENES[int(f[0], 16) & 0x7F]
        if f[1] == "spr8c":
            keys[sc].add(("spr8c", int(f[2], 16), int(f[3], 16)))
        elif f[1] == "proj":
            keys[sc].add(("proj", int(f[2], 16), 0))
        elif f[1] == "trail":
            keys[sc].add(("trail", int(f[2], 16), 0))
    return keys


def key_pieces(rom, key):
    kind, a, b = key
    if kind == "trail":                    # DrawProjectileTrailSprite: un sprite de 8x8 con ese atributo
        return [(0, 0, a)]
    return mkmd.key_pieces(rom, (kind, "%04X" % a, "%04X" % b))


KIND = {"spr8c": 0, "proj": 1, "trail": 2}


def main():
    rom_path, ext, snap, out = sys.argv[1:5]
    rom = mkmd.Rom(rom_path)
    ext, snap, out = Path(ext), Path(snap), Path(out)

    def blob(name):
        return next((ext / "blobs" / "decompressed").glob(name + "_*.bin")).read_bytes()
    usage = json.loads((snap / "intro_usage.json").read_text())
    draw = read_draw_log(snap / "intro_draw.log")
    vr_capt = snes_vram(rom, blob, False)
    vr_main = snes_vram(rom, blob, True)
    vr_fly = flight_vram(rom, blob, 0)
    C = ["/* Generado por native/tools/mkintro.py (docs/38): gráficos de la intro para la Genesis. No editar. */",
         '#include "md_intro_data.h"', ""]
    H = ["/* Generado por native/tools/mkintro.py (docs/38). No editar. */", "#ifndef MD_INTRO_DATA_H",
         "#define MD_INTRO_DATA_H", "#include <stdint.h>",
         "typedef uint8_t uint8; typedef uint16_t uint16; typedef int16_t int16; typedef uint32_t uint32;",
         f"#define INTRO_PAGE {INTRO_PAGE}", f"#define INTRO_SHIP_BUF0 {SHIP_BUF[0]}", f"#define INTRO_SHIP_BUF1 {SHIP_BUF[1]}",
         f"#define INTRO_SHIP_LINE {SHIP_LINE}",
         "typedef struct { uint16 tile, n; const uint8 *data; } IntroSeg;                /* tramo de tiles seguidos */",
         "typedef struct { uint8 kind; uint8 pad; uint16 a, b, rec; } IntroKey;          /* clave de sprites -> registros */",
         "typedef struct {",
         "  uint16 nseg; const IntroSeg *seg;          /* tiles */",
         "  const uint8 *cram_map;                     /* 64 ranuras: índice de palette_buffer (0xFF: sin color) */",
         "  const uint16 *a_idx, *a_val;               /* plano A: carácter del SNES -> fila de 8 (una por paleta) */",
         "  const uint16 *b_map, *b_idx, *b_val;       /* plano B: mapa fijo (32x32) y tabla de los ojos */",
         "  uint16 nkeys; const IntroKey *keys;        /* sprites */",
         "  const uint16 *recs;",
         "  uint8 samus, a_4bpp;",
         "} IntroGroup;",
         "#define INTRO_KIND_SPR8C 0", "#define INTRO_KIND_PROJ 1", "#define INTRO_KIND_TRAIL 2"]
    INB = f"__attribute__((section(\".bank{INTRO_PAGE}\")))"
    group_rows = []
    total = 0
    for gname, scenes in GROUPS:
        sym = gname.lower()
        samus = gname in SAMUS_GROUPS
        flight = gname == "FLIGHT"
        v = vr_capt if gname == "CAPT" else vr_fly if flight else vr_main
        # clases de color del grupo: iguales en todas sus escenas
        cls_s = [usage[s]["classes"] for s in scenes]
        rep = {}
        cls = []
        for i in range(256):
            k = tuple(c[i] for c in cls_s)
            cls.append(rep.setdefault(k, i))
        # ---- elementos: (clave, celdas con índice de CGRAM)
        pix = {}
        a_words = set()
        for s in scenes:
            a_words |= set(usage[s]["bgA"])
        a_4bpp = flight                                      # "SPACE COLONY": BG1 de 4bpp con los tiles en $6000
        for e in a_words:
            ch, pal = e & 0x3FF, (e >> 10) & 7
            if ("A", ch, pal) in pix:
                continue
            if a_4bpp:
                t = tile(v, 0x6000, ch, 4)
                pix[("A", ch, pal)] = [np.where(t > 0, pal * 16 + t, 0)]
            else:
                t = tile(v, 0x4000, ch, 2)
                pix[("A", ch, pal)] = [np.where(t > 0, pal * 4 + t, 0)]
        b_base = {"TEXT": 0x4800, "MB": 0x5000, "BABY": 0x5400, "SCIENTIST": 0x5800, "LAB": 0x5C00}.get(gname)
        static_b = None
        if b_base is not None:
            static_b = [int(x) for x in v[b_base:b_base + 0x400]]
            for e in set(static_b) | set(usage[gname]["eyes"] if gname == "TEXT" else []):
                ch, pal = e & 0x3FF, (e >> 10) & 7
                t = tile(v, 0x0000, ch, 4)
                pix[("B", ch, pal)] = [np.where(t > 0, pal * 16 + t, 0)]
        keys = sorted(set().union(*(draw[s] for s in scenes)))
        pieces = {}
        big_img = {}
        for key in keys:
            if flight:                                       # spritemap grande: una imagen
                sm, chr_ = key[1], key[2]
                ents = key_pieces(rom, key)
                xs, ys, prio = [], [], set()
                for (x, y, at) in ents:
                    dx = x & 0x1FF
                    dx = dx - 0x200 if dx >= 0x100 else dx
                    n = 16 if x & 0x8000 else 8
                    xs += [dx, dx + n]
                    ys += [y, y + n]
                    prio.add((at >> 12) & 3)
                assert len(prio) == 1, f"{sm:04X}: prioridades mezcladas"
                x0, y0 = min(xs), min(ys)
                img = np.zeros((max(ys) - y0, max(xs) - x0), np.int64)
                for (x, y, at) in reversed(ents):            # la primera entrada queda encima
                    dx = x & 0x1FF
                    dx = dx - 0x200 if dx >= 0x100 else dx
                    m = obj_px(v, at & 0x1FF, bool(x & 0x8000), (at >> 9) & 7)
                    if at & 0x4000:
                        m = m[:, ::-1]
                    if at & 0x8000:
                        m = m[::-1]
                    h, w = m.shape
                    sub = img[y - y0:y - y0 + h, dx - x0:dx - x0 + w]
                    img[y - y0:y - y0 + h, dx - x0:dx - x0 + w] = np.where(m > 0, m, sub)
                big_img[key] = (img, x0, y0, prio.pop())
                pix[("I", key)] = [img]
                continue
            pl = []
            for (x, y, at) in key_pieces(rom, key):
                pal = (at >> 9) & 7
                m = obj_px(v, at & 0x1FF, bool(x & 0x8000), pal)
                pl.append((x, y, at, m))
                if not (samus and pal == 4):
                    pix[("O", at & 0x1FF, bool(x & 0x8000), pal)] = [m]
            pieces[key] = pl
        # ---- paletas
        items = []
        for k, cells in pix.items():
            s = set()
            for m_ in cells:
                s |= {cls[int(i)] for i in np.unique(m_) if i}
            items.append((k, s))
        free_lines = [l for l in range(4) if not ((samus and l == SAMUS_LINE) or (flight and l == SHIP_LINE))]
        bins_ = pack_palettes(items, len(free_lines))
        assert bins_ is not None, f"{gname}: no entra en {len(free_lines)} líneas de paleta"
        cram_map = [0xFF] * 64
        cram_map[0] = 0                                       # color de fondo
        where, slot_of = {}, {}
        for bi, (cs, ks) in enumerate(bins_):
            line = free_lines[bi]
            for k in ks:
                where[k] = line
            for s_i, cl in enumerate(sorted(cs)):
                cram_map[line * 16 + 1 + s_i] = cl
                slot_of[(line, cl)] = 1 + s_i
        if samus:
            for k in range(1, 16):
                cram_map[SAMUS_LINE * 16 + k] = 192 + k
        if flight:
            for k in range(1, 16):
                cram_map[SHIP_LINE * 16 + k] = 80 + k
        # ---- tiles
        al = Alloc(REGIONS)
        if flight:                                            # los búferes de la nave
            al.free = [r for r in al.free]
            al.free[1] = [SHIP_BUF[1] + SHIP_MAX_TILES, 1280]

        def gconv(m_, line):
            return np.vectorize(lambda i: slot_of[(line, cls[int(i)])] if i else 0)(m_)

        def cell_word(k):
            line = where[k]
            return al.one(genesis_tile_bytes(gconv(pix[k][0], line))) | line << 13

        recs = []
        index = []
        if flight:
            for key in keys:
                img, x0, y0, prio = big_img[key]
                line = where[("I", key)]
                boxes, pad, M = cover(img)
                sprs = []
                for bx, by, w, h in boxes:
                    bs = []
                    for cx in range(w):                       # tiles del sprite por columnas, como el VDP
                        for cy in range(h):
                            m_ = pad[by + M + cy * 8:by + M + cy * 8 + 8, bx + M + cx * 8:bx + M + cx * 8 + 8]
                            bs.append(genesis_tile_bytes(gconv(m_, line)))
                    first = al.run(bs)
                    attr = first | line << 13 | (0x8000 if prio == 3 else 0)
                    sprs.append((by + y0, ((w - 1) << 2 | (h - 1)) << 8, attr, bx + x0))
                off = len(recs)
                recs += list(struct.unpack(">5h", mkmd.rec_header(sprs)))
                for r in sprs:
                    recs += list(r)
                index.append((KIND[key[0]], key[1], key[2], off))
        else:
            gtile = {}
            for key in keys:
                sprs = []
                for (x, y, at, m) in pieces[key]:
                    big = bool(x & 0x8000)
                    pal = (at >> 9) & 7
                    if samus and pal == 4:
                        line = SAMUS_LINE
                        g = np.where(m > 0, m - 192, 0)
                    else:
                        line = where[("O", at & 0x1FF, big, pal)]
                        g = gconv(m, line)
                    n = 2 if big else 1
                    bs = [genesis_tile_bytes(g[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8]) for cx in range(n) for cy in range(n)]
                    tk = tuple(bs)
                    if tk not in gtile:
                        gtile[tk] = al.run(bs) if big else al.one(bs[0])
                    attr = gtile[tk] | line << 13 | (((at >> 15) & 1) << 12) | (((at >> 14) & 1) << 11) | \
                        (0x8000 if (at >> 12) & 3 == 3 else 0)
                    dx = x & 0x1FF
                    dx = dx - 0x200 if dx >= 0x100 else dx
                    sprs.append((y, 0x0500 if big else 0, attr, dx))
                off = len(recs)
                recs += list(struct.unpack(">5h", mkmd.rec_header(sprs)))
                for r in sprs:
                    recs += list(r)
                index.append((KIND[key[0]], key[1], key[2], off))
        # ---- planos

        def lookup(layer):
            chars = sorted({k[1] for k in pix if k[0] == layer})
            idx = [0xFFFF] * 1024
            vals = []
            for ch in chars:
                idx[ch] = len(vals)
                row = [0] * 8
                for pal in range(8):
                    if (layer, ch, pal) in pix:
                        row[pal] = cell_word((layer, ch, pal))
                vals += row
            return idx, vals
        ia, va = lookup("A")
        b_map = None
        ib = vb = None
        if static_b is not None:
            ib, vb = lookup("B")
            b_map = []
            for e in static_b:
                w = cell_word(("B", e & 0x3FF, (e >> 10) & 7))
                b_map.append(w | ((e >> 14) & 1) << 11 | ((e >> 15) & 1) << 12)
        index.sort()                                          # md_intro.c busca por (tipo, a, b)
        segs = al.segments()
        ntiles = sum(len(b) // 32 for _, b in segs)
        total += ntiles * 32
        # ---- salida
        for i, (t, b) in enumerate(segs):
            C.append(f"{INB} static const uint8 kIntro_{sym}_t{i}[{len(b)}] __attribute__((aligned(2))) = {{" +
                     ",".join(str(x) for x in b) + "};")
        C.append(f"static const IntroSeg kIntro_{sym}_seg[{len(segs)}] = {{" +
                 ",".join(f"{{{t},{len(b) // 32},kIntro_{sym}_t{i}}}" for i, (t, b) in enumerate(segs)) + "};")
        C.append(f"{INB} static const uint8 kIntro_{sym}_cram[64] = {{" + ",".join(str(x) for x in cram_map) + "};")

        def arr(name, vals):
            C.append(f"{INB} static const uint16 {name}[{max(1, len(vals))}] = {{" + ",".join(str(x) for x in vals or [0]) + "};")
        arr(f"kIntro_{sym}_aidx", ia)
        arr(f"kIntro_{sym}_aval", va)
        if b_map is not None:
            arr(f"kIntro_{sym}_bmap", b_map)
            arr(f"kIntro_{sym}_bidx", ib)
            arr(f"kIntro_{sym}_bval", vb)
        C.append(f"{INB} static const IntroKey kIntro_{sym}_keys[{max(1, len(index))}] = {{" +
                 ",".join(f"{{{k},0,{a},{b},{o}}}" for k, a, b, o in index) + ("" if index else "{0}") + "};")
        arr(f"kIntro_{sym}_recs", [x & 0xFFFF for x in recs])
        bm = f"kIntro_{sym}_bmap, kIntro_{sym}_bidx, kIntro_{sym}_bval" if b_map is not None else "0, 0, 0"
        group_rows.append(f"  {{ {len(segs)}, kIntro_{sym}_seg, kIntro_{sym}_cram, kIntro_{sym}_aidx, kIntro_{sym}_aval, {bm}, "
                          f"{len(index)}, kIntro_{sym}_keys, kIntro_{sym}_recs, {int(samus)}, {int(a_4bpp)} }},  /* {gname} */")
        print(f"{gname}: {ntiles} tiles, colores por línea {[len(b[0]) for b in bins_]}, {len(index)} claves de sprites",
              file=sys.stderr)
    C.append(f"{INB} const IntroGroup kIntroGroups[{len(GROUPS)}] = {{")
    C += group_rows
    C.append("};")
    # mapa de BG3 inicial ($97:8D12, el DMA de Intro_Initial a $4C00)
    C.append(f"{INB} const uint16 kIntroBg3Initial[1024] = {{" + ",".join(str(int(x)) for x in vr_capt[0x4C00:0x5000]) + "};")
    H.append("extern const uint16 kIntroBg3Initial[1024];   /* en la página INTRO_PAGE */")
    # escena -> grupo
    scene_group = [next(i for i, (g, ss) in enumerate(GROUPS) if s in ss) for s in SCENES]
    C.append("const uint8 kIntroSceneGroup[%d] = {%s};" % (len(SCENES), ",".join(map(str, scene_group))))
    H += [f"extern const IntroGroup kIntroGroups[{len(GROUPS)}];   /* en la página INTRO_PAGE */",
          f"extern const uint8 kIntroSceneGroup[{len(SCENES)}];"]
    # ---- la nave
    m7 = usage["FLY"]["m7"]
    vr_back = flight_vram(rom, blob, 1)
    ship, offs, skipped = ship_frames((vr_fly, vr_back), m7)
    pages = []
    PAGE = 0x80000
    # cuadros en páginas de 512 KB (cada cuadro entero en una página)
    cur = bytearray()
    page_of, off_in = [], []
    for i in range(len(offs)):
        a, b = offs[i], offs[i + 1] if i + 1 < len(offs) else len(ship)
        rec = ship[a:b]
        if len(cur) + len(rec) > PAGE:
            pages.append(bytes(cur))
            cur = bytearray()
        page_of.append(INTRO_PAGE + 1 + len(pages))
        off_in.append(len(cur))
        cur += rec
    pages.append(bytes(cur))
    for i, p in enumerate(pages):
        C.append(f"__attribute__((section(\".bank{INTRO_PAGE + 1 + i}\"))) const uint8 kIntroShip{i}[{len(p)}] "
                 f"__attribute__((aligned(4))) = {{" + ",".join(str(x) for x in p) + "};")
    C.append(f"{INB} const uint16 kIntroShipKey[{len(m7)}][8] = {{" +
             ",".join("{" + ",".join(str(x) for x in k[:8]) + "}" for k in m7) + "};")
    C.append(f"{INB} const uint8 kIntroShipTex[{len(m7)}] = {{" + ",".join(str(k[8]) for k in m7) + "};")
    C.append(f"{INB} const uint8 kIntroShipPage[{len(m7)}] = {{" + ",".join(map(str, page_of)) + "};")
    C.append(f"{INB} const uint32 kIntroShipOff[{len(m7)}] = {{" + ",".join(map(str, off_in)) + "};")
    H += [f"#define INTRO_SHIP_N {len(m7)}",
          "extern const uint16 kIntroShipKey[INTRO_SHIP_N][8];  /* M7A-D, M7X/Y, BG1HOFS/VOFS de cada cuadro */",
          "extern const uint8 kIntroShipTex[INTRO_SHIP_N];      /* 0: la nave de frente, 1: de atrás */",
          "extern const uint8 kIntroShipPage[INTRO_SHIP_N];     /* página del SSF2 del cuadro */",
          "extern const uint32 kIntroShipOff[INTRO_SHIP_N];     /* desplazamiento dentro de la página */",
          "#define INTRO_SHIP_ADDR(i) ((const uint16 *)(0x380000 + kIntroShipOff[i]))"]
    H.append("#endif")
    (out / "md_intro_data.c").write_text("\n".join(C) + "\n")
    (out / "md_intro_data.h").write_text("\n".join(H) + "\n")
    print(f"intro: {total} bytes de tiles; nave: {len(m7)} cuadros, {len(ship)} bytes en {len(pages)} páginas; "
          f"sin dibujar por grandes: {len(skipped)}", file=sys.stderr)


if __name__ == "__main__":
    main()
