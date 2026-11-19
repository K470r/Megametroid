#!/usr/bin/env python3
"""MegaMetroid — gráficos del final (estado 39) para la Genesis: las escenas en modo 1 (docs/63).

Como la pausa (mkpause.py): lo que el SNES tiene en la VRAM en cada grupo de escenas (captura del SNES con tu ROM:
snes_ref/END_<grupo>.bin, VRAM y CGRAM) y lo que el juego dibuja (snes_ref/ending_draw.log: las palabras de los
mapas que sube y los spritemaps, de tools/snes_ref/ending_log.py) se convierten a tiles de la Genesis:
  plano A = BG1 (el texto: créditos, "SEE YOU NEXT MISSION"...; lo sube el juego cuadro a cuadro)
  plano B = BG2 (una pantalla fija de la captura: Samus, el símbolo)
  sprites = DrawCinematicSpritemap (spritemaps de $8C) y las estrellas.
Los colores van por índice de palette_buffer (el final los cambia: fundidos, brillo del texto): cada índice usado
tiene su lugar en una línea de la CRAM y la Genesis los copia en cada cuadro. Si no entran en 4 líneas de 15, el
menos usado se junta con el de color más parecido.

Salida: md_ending_data.c/.h (tiles en la página ED_PAGE del SSF2).
Uso: mkending.py rom.smc snes_ref carpeta_salida"""
import struct
import sys
from pathlib import Path

ROM = Path(sys.argv[1]).read_bytes()
SNAP = Path(sys.argv[2])
OUT = Path(sys.argv[3])
ED_PAGE = 24                 # página del SSF2 de los tiles (vacía: docs/48; mkzebes.py la cuenta como ocupada)
ED_TILE_BASE = 16            # tiles de la Genesis desde aquí (en el final no hay HUD ni sala)
ED_TILE_MAX = 1260           # hasta 0x9FFF (los planos empiezan en 0xA000)
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "genesis_convert"))
from mkmd import bgr_to_gen, gen_to_cram   # noqa: E402

# grupos (escenas que comparten la VRAM del SNES): captura, base de tiles del BG1 y del BG2, pantallas fijas de cada
# uno (las que no sube el juego), base de los sprites (OBSEL) y sus tamaños (chico, grande), modo 7 (la textura sale
# de la misma captura: m7 = (forma, filas): "wrap" se repite cada 256 texels; "box" fuera de la textura no hay nada)
GROUPS = {
    "D0A": dict(snap="END_D0A.bin", bg1_chr=0, bg2_chr=0, obj=0x6000, sizes=(32, 64), m7="wrap"),   # Zebes de cerca y las nubes
    "D0B": dict(snap="END_D0B.bin", bg1_chr=0, bg2_chr=0, obj=0x6000, sizes=(32, 64), m7="wrap"),   # las nubes grises
    "D0C": dict(snap="END_D0C.bin", bg1_chr=0, bg2_chr=0, obj=0x4000, sizes=(8, 16), m7="wrap"),    # Zebes se aleja
    "D0S": dict(snap="END_D0S.bin", bg1_chr=0x4000, bg2_chr=0x4000, obj=0x4000, sizes=(8, 16),      # el espacio, la explosión
                scr_a=(0x7000, 0x7400), scr_b=(0x7800, 0x7C00)),
    "D0T": dict(snap="END_D0T.bin", bg1_chr=0, bg2_chr=0, obj=0x4000, sizes=(8, 16), m7="box"),     # la nave y el tiempo
    "D1": dict(snap="END_D1.bin", bg1_chr=0x4000, bg2_chr=0x5000, obj=0x0000, sizes=(8, 16), scr_b=(0x4C00,)),
    "D2": dict(snap="END_D2.bin", bg1_chr=0x4000, bg2_chr=0, obj=0x0000, sizes=(8, 16)),            # Samus chica
    "D2B": dict(snap="END_D2B.bin", bg1_chr=0x4000, bg2_chr=0, obj=0x6000, sizes=(8, 16), m7="box"),  # el símbolo
    "D3": dict(slogan=True, snap="END_D3.bin", bg1_chr=0x4000, bg2_chr=0x6000, obj=0x6000, sizes=(8, 16),
               scr_b=(0x5400,)),
}
ED_M7_PAT = 1024             # modo 7: tiles de los bloques (256: dos colores, arriba y abajo) desde aquí: los del grupo, antes


def off(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def rw(a):
    return ROM[off(a)] | ROM[off(a) + 1] << 8


def rb(a):
    return ROM[off(a)]


log = {}
for l in (SNAP / "ending_draw.log").read_text().splitlines():
    if l.startswith("#") or not l.strip():
        continue
    p = l.split()
    g = log.setdefault(p[0], dict(w=set(), s=set()))
    if p[1] == "w":
        g["w"].add(int(p[2], 16))
    else:
        g["s"].add((p[2], int(p[3], 16)))


def tile4(vram, wbase, t):
    a = (wbase * 2 + t * 32) & 0xFFFF
    out = []
    for y in range(8):
        b0, b1, b2, b3 = vram[a + 2 * y], vram[a + 2 * y + 1], vram[a + 16 + 2 * y], vram[a + 17 + 2 * y]
        out.append([((b0 >> (7 - x)) & 1) | ((b1 >> (7 - x)) & 1) << 1 | ((b2 >> (7 - x)) & 1) << 2 |
                    ((b3 >> (7 - x)) & 1) << 3 for x in range(8)])
    return out


def tbytes(rows):
    return bytes((r[2 * i] << 4) | r[2 * i + 1] for r in rows for i in range(4))


C = ["/* Generado por tools/conv/mkending.py (docs/63): el final en modo 1 desde tu ROM. No editar. */",
     '#include "md_ending_data.h"',
     "#ifdef NT_SSF2", '#define ED_PAGE_ATTR __attribute__((section(".bank%d")))' % ED_PAGE, "#else",
     "#define ED_PAGE_ATTR", "#endif"]
H = ["/* Generado por tools/conv/mkending.py (docs/63). No editar. */", "#ifndef MD_ENDING_DATA_H",
     "#define MD_ENDING_DATA_H", "#include <stdint.h>", "typedef uint8_t uint8; typedef uint16_t uint16;",
     "#define ED_TILE_BASE %d" % ED_TILE_BASE, "#define ED_PAGE_N %d   /* todas las tablas (salvo EdGroup) están en esta página: ventana 7 */" % ED_PAGE,
     "/* un grupo de escenas: tiles, LUT de palabras del SNES (BG1 -> plano A, BG2 -> plano B) por tile: kLutT[tile] =",
     "   primera variante en lp/lv (paleta, 0xFF fin; celda de la Genesis), pantalla fija del BG2 (palabras del SNES),",
     "   colores (CRAM -> índice de palette_buffer, 0xFF nada), sprites (claves ordenadas -> registros) */",
     "/* modo 7: textura (bytes = color 1-15 de la línea 0; 0 nada); box 0: 256x256 que se repite (filas 128-255 primero:",
     "   el índice de 16 bits va con signo), box 1: filas r0..r1-1 de 256 y fuera no hay nada; col: color -> índice de",
     "   palette_buffer */",
     "typedef struct { const uint8 *tex; uint8 box, r0; uint16 r1; uint8 col[16]; } EdM7;",
     "#define ED_M7_PAT %d   /* tiles de los bloques del modo 7: 256 (arriba y abajo) */" % ED_M7_PAT,
     "typedef struct { const uint8 *tiles; uint16 ntiles; const uint16 *lutTa, *lutVa, *lutTb, *lutVb;",
     "  const uint8 *lutPa, *lutPb; const uint16 *scr; const uint8 *src; uint16 nkeys; const uint16 *keys;",
     "  const uint16 *recs; uint16 star[4];",
     "  const uint16 *slogan; const EdM7 *m7; } EdGroup;",
     "/* scr: pantallas fijas: nA, nB, sus direcciones del SNES (BG1, BG2) y 1024 palabras de cada una; keys: (spritemap,",
     "   chr, registros) de a tres; star: celda por chr 09F0-09F3; slogan: 64 palabras del SNES (filas 16-17 de la",
     "   pantalla 0x4C00) o NULL */"]
gnames = []
m7_kb = {}


def m7_texture(vram, cg, kind):
    """la textura del modo 7 de la captura: 256x256 texels (o las filas con algo, kind "box") con 15 colores"""
    v = vram[:0x8000]
    mp = [v[2 * i] for i in range(0x4000)]
    ch = bytes(v[2 * i + 1] for i in range(0x4000))
    tex = bytearray(256 * 256)
    for y in range(256):
        for x in range(256):
            t = mp[(y >> 3) * 128 + (x >> 3)]
            tex[y * 256 + x] = ch[t * 64 + (y & 7) * 8 + (x & 7)]
    rows = range(256)
    if kind == "box":
        nz = [y for y in range(256) if any(tex[y * 256:(y + 1) * 256])]
        rows = range(min(nz), max(nz) + 1)
    cnt = {}
    for y in rows:
        for i in tex[y * 256:(y + 1) * 256]:
            cnt[i] = cnt.get(i, 0) + 1
    # un lugar de la línea 0 por índice de la paleta (el color sigue a palette_buffer: fundidos); el 0 es nada (box) o
    # el fondo (wrap). Si son más de 15, el menos usado se junta con el de color más parecido
    ids = sorted(i for i in cnt if i or kind == "wrap")
    rgb = lambda i: snes_to_genesis(cg[i] & 0x7FFF, "dac")
    alias = {}
    live = list(ids)
    while len(live) > 15:
        k = min(live, key=lambda i: (cnt[i], i))
        live.remove(k)
        near = min(live, key=lambda j: sum((x - y) ** 2 for x, y in zip(rgb(k), rgb(j))))
        alias[k] = near
        for k2, v2 in list(alias.items()):
            if v2 == k:
                alias[k2] = near
    slot = {i: n + 1 for n, i in enumerate(live)}
    lut = [slot.get(alias.get(i, i), 0) if (i or kind == "wrap") else 0 for i in range(256)]
    col = [0] * 16
    for i in live:
        col[slot[i]] = i
    if kind == "box":
        out = bytes(lut[tex[y * 256 + x]] if tex[y * 256 + x] else 0 for y in rows for x in range(256))
        return out, 1, rows.start, rows.stop, col
    order = list(range(128, 256)) + list(range(128))
    out = bytes(lut[tex[y * 256 + x]] for y in order for x in range(256))
    return out, 0, 0, 256, col


from convert_rooms import snes_to_genesis  # noqa: E402
for gname, G in GROUPS.items():
    d = (SNAP / G["snap"]).read_bytes()
    vram, cg = d[:0x10000], list(struct.unpack("<256H", d[0x10000:0x10200]))
    L = log.get(gname, dict(w=set(), s=set()))
    def screen(a):
        return [vram[2 * (a + i)] | vram[2 * (a + i) + 1] << 8 for i in range(1024)]
    scr_a = [(a, screen(a)) for a in G.get("scr_a", ())]
    scr_b = [(a, screen(a)) for a in G.get("scr_b", ())]
    # el slogan (decisión de Edwin, docs/63): "GENESIS DOES..." con la letra grande de "SEE YOU NEXT MISSION" (BG1,
    # paleta 0, prioridad), debajo del porcentaje (filas 16-17 de la pantalla 0x4C00), centrado como el resto
    slogan = []
    if G.get("slogan"):
        txt = "GENESIS DOES..."
        top, bot = [0x007F] * 32, [0x007F] * 32
        x0 = (32 - len(txt)) // 2
        for i, ch in enumerate(txt):
            if ch == " ":
                continue
            if ch == ".":
                t0, t1 = 0x7F, 0x5A                       # (la mitad de abajo de "!": un punto)
            else:
                n = ord(ch) - 65
                t0 = 0x20 + n if n < 16 else 0x40 + n - 16
                t1 = t0 + 0x10
            top[x0 + i], bot[x0 + i] = 0x2000 | t0, 0x2000 | t1
        slogan = top + bot
        L["w"] |= set(slogan)
    wa = sorted(L["w"] | {v for _, ws in scr_a for v in ws})
    wb = sorted({v for _, ws in scr_b for v in ws})
    # sprites: piezas de cada spritemap de $8C (x, y, grande, attr | chr)
    pieces = {}
    for sm, chr_ in sorted(L["s"]):
        if sm == "star":
            pieces[(sm, chr_)] = [(0, 0, 8, chr_)]
            continue
        a = 0x8C0000 | int(sm, 16)
        n = rw(a)
        lst = []
        for i in range(n):
            e = a + 2 + 5 * i
            xw = rw(e)
            x = xw & 0x1FF
            x = x - 0x200 if x & 0x100 else x
            y = rb(e + 2)
            y = y - 256 if y & 0x80 else y
            attr = chr_ | (rw(e + 3) & 0xF1FF)
            lst.append((x, y, G["sizes"][1] if xw & 0x8000 else G["sizes"][0], attr))
        pieces[(sm, chr_)] = lst

    def obj_tile(t):
        base = G["obj"] + (0x1000 if t & 0x100 else 0)
        return tile4(vram, base, t & 0xFF)

    def snes_tile(t, cx, cy):  # el tile de la columna cx y la fila cy de un sprite del SNES (vuelta en 16 y en 16 filas)
        return (t & 0x100) | (((t >> 4) + cy) & 15) << 4 | ((t + cx) & 15)

    def subsprites(x, y, size, attr):
        """un sprite del SNES -> sprites de la Genesis (hasta 32x32): (x, y, celdas, lista de tiles por columnas)"""
        t, hf, vf = attr & 0x1FF, bool(attr & 0x4000), bool(attr & 0x8000)
        n = size // 8
        q = 4 if n > 4 else n                    # celdas por lado de cada sprite de la Genesis
        out = []
        for qy in range(0, n, q):
            for qx in range(0, n, q):
                px = (n - q - qx) if hf else qx  # (volteado: el cuadrante va del otro lado)
                py = (n - q - qy) if vf else qy
                ts = [snes_tile(t, qx + cx, qy + cy) for cx in range(q) for cy in range(q)]
                out.append((x + px * 8, y + py * 8, q, ts))
        return out

    def spr_tiles(t, size):    # todos los tiles de un sprite del SNES
        n = size // 8
        return [snes_tile(t, cx, cy) for cx in range(n) for cy in range(n)]

    # tiles a convertir: (tipo, tile, paleta) -> píxeles (índices 0-15) e índices de palette_buffer
    need = {}
    for v in wa:
        need[("a", v & 0x3FF, (v >> 10) & 7)] = tile4(vram, G["bg1_chr"], v & 0x3FF)
    for v in wb:
        need[("b", v & 0x3FF, (v >> 10) & 7)] = tile4(vram, G["bg2_chr"], v & 0x3FF)
    for k, lst in pieces.items():
        for x, y, size, attr in lst:
            for t in spr_tiles(attr & 0x1FF, size):
                need[("s", t, (attr >> 9) & 7)] = obj_tile(t)

    def keys_of(kind, p, px):
        pal = p * 16 if kind != "s" else 128 + p * 16
        return frozenset(pal + c for row in px for c in row if c)

    cnt = {}
    for (kind, t, p), px in need.items():
        for row in px:
            for c in row:
                if c:
                    i = (p * 16 if kind != "s" else 128 + p * 16) + c
                    cnt[i] = cnt.get(i, 0) + 1
    alias = {}

    def A(i):
        while i in alias:
            i = alias[i]
        return i

    def gkeys(k):
        kind, t, p = k
        return frozenset(A(i) for i in keys_of(kind, p, need[k]))

    def sgroups():                              # un sprite grande: sus 4 tiles en la misma línea
        out = set()
        for lst in pieces.values():
            for x, y, size, attr in lst:
                p = (attr >> 9) & 7
                for _, _, _, ts in subsprites(x, y, size, attr):
                    out.add(frozenset().union(*(gkeys(("s", tt, p)) for tt in ts)))
        return out

    def pack():
        gs = sorted(({gkeys(k) for k in need} | sgroups()) - {frozenset()}, key=lambda g: (-len(g), sorted(g)))
        Gs = [g for g in gs if not any(g < h for h in gs)]
        tries = [0]

        def rec(i, lines):
            tries[0] += 1
            if tries[0] > 200000:
                return None
            if i == len(Gs):
                return [set(x) for x in lines]
            seen = set()
            for li in range(1 if G.get("m7") else 0, 4):   # (con modo 7, la línea 0 es suya)
                if frozenset(lines[li]) in seen:
                    continue
                seen.add(frozenset(lines[li]))
                u = lines[li] | Gs[i]
                if len(u) <= 15:
                    old = lines[li]
                    lines[li] = u
                    r = rec(i + 1, lines)
                    lines[li] = old
                    if r:
                        return r
            return None
        return rec(0, [set(), set(), set(), set()])

    def rgb(i):
        return bgr_to_gen(cg[i] & 0x7FFF)

    merged = 0
    while True:
        res = pack()
        if res:
            break
        live = sorted((i for i in cnt if i not in alias), key=lambda i: (cnt[i], i))
        k = live[0]
        near = min((j for j in live if j != k),
                   key=lambda j: sum((a - b) ** 2 for a, b in zip(rgb(k), rgb(j))))
        alias[k] = near
        cnt[near] += cnt.pop(k)
        merged += 1
    lines = [sorted(x) for x in res]
    where = {}
    for k in need:
        g = gkeys(k)
        where[k] = next((li for li in range(4) if g <= set(lines[li])), 0)
    src = [0xFF] * 64
    src[0] = 0
    for li in range(4):
        for j, i in enumerate(lines[li]):
            src[li * 16 + 1 + j] = i

    tiles, tindex = [], {}

    def put(rows, flip=True):
        cands = [(rows, 0)]
        if flip:
            cands += [([r[::-1] for r in rows], 0x800), (rows[::-1], 0x1000), ([r[::-1] for r in rows[::-1]], 0x1800)]
        for rr, fl in cands:
            b = tbytes(rr)
            if b in tindex:
                return tindex[b], fl
        b = tbytes(rows)
        tindex[b] = len(tiles)
        tiles.append(b)
        return tindex[b], 0

    def gen_rows(k):
        kind, t, p = k
        li = where[k]
        pal = p * 16 if kind != "s" else 128 + p * 16
        return li, [[1 + lines[li].index(A(pal + c)) if c else 0 for c in row] for row in need[k]]

    put([[0] * 8 for _ in range(8)])           # tile 0 (vacío) primero

    def lut_of(kind, ws):
        lut = {}
        for v in ws:
            k = (kind, v & 0x3FF, (v >> 10) & 7)
            li, rows = gen_rows(k)
            n, fl = put(rows)
            # (los volteos y la prioridad de la palabra los suma la Genesis al buscarla: como pz_lookup)
            lut[(v & 0x1C00) | (v & 0x3FF)] = (ED_TILE_BASE + n) | li << 13 | fl
        lt, lp, lv = [0xFFFF] * 1024, [], []
        for t in range(1024):
            vs = [(p, lut[(p << 10) | t]) for p in range(8) if lut.get((p << 10) | t) is not None]
            if vs:
                lt[t] = len(lp)
                for p, c in vs:
                    lp.append(p)
                    lv.append(c)
                lp.append(0xFF)
                lv.append(0)
        return lt, lp, lv

    lta, lpa, lva = lut_of("a", wa)
    ltb, lpb, lvb = lut_of("b", wb)
    # sprites: tiles seguidos por sprite grande (sin compartir), registros como kPzSprRecs (n, rectángulo, y, tamaño,
    # attr, x)
    stile = {}
    recs, keys = [], []
    star = [0, 0, 0, 0]
    for k in sorted(pieces, key=lambda k: (k[0] != "star", k)):
        out = []
        # los sprites de la Genesis de cada pieza (x, y, ancho, alto en celdas, tiles por columnas, paleta, attr)
        subs = []
        for x, y, size, attr in pieces[k]:
            p = (attr >> 9) & 7
            for sx, sy, q, ts in subsprites(x, y, size, attr):
                subs.append([sx, sy, q, q, list(ts), p, attr & 0xF000])
        # juntar los vecinos de la misma fila (misma altura, paleta, prioridad, sin volteo) en sprites de hasta 4
        # celdas de ancho: la Genesis (H32) muestra 16 sprites por línea; el texto "THE OPERATION WAS..." son letras
        # de 8x8 (el SNES muestra 32 por línea)
        def line_of(ts, p):
            gg = frozenset().union(*(gkeys(("s", tt, p)) for tt in ts))
            return next((l for l in range(4) if gg <= set(lines[l])), None)
        merged_subs = []
        for e in sorted(subs, key=lambda e: (e[1], e[3], e[5], e[6], e[0])):
            if merged_subs:
                c = merged_subs[-1]
                if (c[1] == e[1] and c[3] == e[3] and c[5] == e[5] and c[6] == e[6] and not (e[6] & 0xC000)
                        and c[0] + c[2] * 8 == e[0] and c[2] + e[2] <= 4 and line_of(c[4] + e[4], e[5]) is not None):
                    c[2] += e[2]
                    c[4] = c[4] + e[4]
                    continue
            merged_subs.append(list(e))
        for sx, sy, w, h, ts, p, ab in merged_subs:
            key = (tuple(ts), p)
            if key not in stile:
                first = len(tiles)
                li = line_of(ts, p)
                for tt in ts:
                    rows = [[1 + lines[li].index(A(128 + p * 16 + c)) if c else 0 for c in row] for row in need[("s", tt, p)]]
                    tiles.append(tbytes(rows))
                stile[key] = (first, li)
            first, li = stile[key]
            prio = 0x8000 if ((ab >> 12) & 3) >= 2 else 0
            gattr = prio | li << 13 | (0x1000 if ab & 0x8000 else 0) | (0x800 if ab & 0x4000 else 0) | (ED_TILE_BASE + first)
            out.append((sy, ((w - 1) << 10) | ((h - 1) << 8), gattr, sx))
        if k[0] == "star":
            if 0x09F0 <= k[1] <= 0x09F3:
                star[k[1] - 0x09F0] = out[0][2]
            continue
        ys = [r[0] for r in out] or [0]
        xs = [r[3] for r in out] or [0]
        y1 = max((r[0] + ((r[1] >> 8) & 3) * 8 + 7 for r in out), default=7)
        x1 = max((r[3] + ((r[1] >> 10) & 3) * 8 + 7 for r in out), default=7)
        keys += [int(k[0], 16), k[1], len(recs)]
        recs += [v & 0xFFFF for v in (len(out), min(ys), y1, min(xs), x1)]
        for r in out:
            recs += [r[0] & 0xFFFF, r[1], r[2], r[3] & 0xFFFF]
    assert len(tiles) <= ED_TILE_MAX, "mkending %s: %d tiles" % (gname, len(tiles))
    tb = b"".join(tiles)
    n = gname.lower()

    def arr(t, name, vals, per=16):              # todo en la página ED_PAGE (la Genesis la pone en la ventana 7)
        C.append("ED_PAGE_ATTR static const %s %s[%d] __attribute__((aligned(2))) = {" % (t, name, len(vals)))
        for i in range(0, len(vals), per):
            C.append("  " + ", ".join("0x%X" % v for v in vals[i:i + per]) + ",")
        C.append("};")
    C.append("ED_PAGE_ATTR static const uint8 kEdTiles_%s[%d] __attribute__((aligned(2))) = {" % (n, len(tb)))
    for i in range(0, len(tb), 32):
        C.append("  " + ", ".join("0x%X" % v for v in tb[i:i + 32]) + ",")
    C.append("};")
    arr("uint16", "kLutTa_%s" % n, lta)
    arr("uint8", "kLutPa_%s" % n, lpa or [0xFF], 32)
    arr("uint16", "kLutVa_%s" % n, lva or [0])
    arr("uint16", "kLutTb_%s" % n, ltb)
    arr("uint8", "kLutPb_%s" % n, lpb or [0xFF], 32)
    arr("uint16", "kLutVb_%s" % n, lvb or [0])
    scr = [len(scr_a), len(scr_b)] + [a for a, _ in scr_a] + [a for a, _ in scr_b]
    for _, ws in scr_a + scr_b:
        scr += ws
    arr("uint16", "kScr_%s" % n, scr)
    m7ref = "0"
    if G.get("m7"):
        m7ref = "&kM7_%s" % n
        texb, box, r0, r1, col = m7_texture(vram, cg, G["m7"])
        C.append("ED_PAGE_ATTR static const uint8 kM7Tex_%s[%d] = {" % (n, len(texb)))
        for i in range(0, len(texb), 64):
            C.append("  " + ",".join("%d" % v for v in texb[i:i + 64]) + ",")
        C.append("};")
        C.append("static const EdM7 kM7_%s = { kM7Tex_%s, %d, %d, %d, { %s } };"
                 % (n, n, box, r0, r1, ", ".join("%d" % c for c in col)))
        m7_kb[gname] = len(texb) // 1024
        assert len(tiles) <= ED_M7_PAT - ED_TILE_BASE, "mkending %s: %d tiles (con modo 7: %d)" % (gname, len(tiles), ED_M7_PAT - ED_TILE_BASE)
    arr("uint8", "kSrc_%s" % n, src)
    arr("uint16", "kSlogan_%s" % n, slogan or [0])
    arr("uint16", "kKeys_%s" % n, keys or [0])
    arr("uint16", "kRecs_%s" % n, recs or [0])
    C.append("const EdGroup kEd_%s = { kEdTiles_%s, %d, kLutTa_%s, kLutVa_%s, kLutTb_%s, kLutVb_%s, kLutPa_%s, kLutPb_%s, "
             "kScr_%s, kSrc_%s, %d, kKeys_%s, kRecs_%s, { %s }, %s, %s };"
             % (n, n, len(tiles), n, n, n, n, n, n, n, n, len(keys) // 3, n, n, ", ".join("0x%X" % s for s in star),
                "kSlogan_%s" % n if slogan else "0", m7ref))
    H.append("extern const EdGroup kEd_%s;" % n)
    gnames.append(gname)
    print("final %s: %d palabras BG1, %d BG2, %d sprites, %d tiles (%d KB), colores %s, juntados %d%s"
          % (gname, len(wa), len(wb), len(pieces), len(tiles), len(tb) // 1024, [len(x) for x in lines], merged,
             ", modo 7 %d KB" % m7_kb[gname] if gname in m7_kb else ""))
H.append("#endif")
(OUT / "md_ending_data.h").write_text("\n".join(H) + "\n")
(OUT / "md_ending_data.c").write_text("\n".join(C) + "\n")
