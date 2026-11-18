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
from mkmd import bgr_to_gen, gen_to_cram   # noqa: E402

# grupos: captura, BG1 (base de tiles), BG2 (pantalla fija, base de tiles), sprites (base de la OAM: OBSEL)
GROUPS = {
    "D1": dict(snap="END_D1.bin", bg1_chr=0x4000, bg2_scr=0x4C00, bg2_chr=0x5000, obj=0x0000),
    "D2": dict(snap="END_D2.bin", bg1_chr=0x4000, bg2_scr=None, bg2_chr=0, obj=0x0000),     # el símbolo (modo 7: solo sprites)
    "D2B": dict(snap="END_D2B.bin", bg1_chr=0x4000, bg2_scr=None, bg2_chr=0, obj=0x6000),
    "D3": dict(slogan=True, snap="END_D3.bin", bg1_chr=0x4000, bg2_scr=0x5400, bg2_chr=0x6000, obj=0x6000),
}


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
     "typedef struct { const uint8 *tiles; uint16 ntiles; const uint16 *lutTa, *lutVa, *lutTb, *lutVb;",
     "  const uint8 *lutPa, *lutPb; const uint16 *bg2; const uint8 *src; uint16 nkeys; const uint16 *keys;",
     "  const uint16 *recs; uint16 star[4]; uint16 bg2_scr;",
     "  const uint16 *slogan; } EdGroup;   /* slogan: 64 palabras del SNES (filas 16-17 de la pantalla 0x4C00) o NULL */   /* keys: (spritemap, chr) de a pares; star: celda por chr 09F0-09F3 */"]
gnames = []
for gname, G in GROUPS.items():
    d = (SNAP / G["snap"]).read_bytes()
    vram, cg = d[:0x10000], list(struct.unpack("<256H", d[0x10000:0x10200]))
    L = log.get(gname, dict(w=set(), s=set()))
    bg2 = [vram[2 * (G["bg2_scr"] + i)] | vram[2 * (G["bg2_scr"] + i) + 1] << 8 for i in range(1024)] \
        if G["bg2_scr"] is not None else [0] * 1024
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
    wa = sorted(L["w"])
    wb = sorted(set(bg2)) if G["bg2_scr"] is not None else []
    # sprites: piezas de cada spritemap de $8C (x, y, grande, attr | chr)
    pieces = {}
    for sm, chr_ in sorted(L["s"]):
        if sm == "star":
            pieces[(sm, chr_)] = [(0, 0, 0, chr_)]
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
            lst.append((x, y, 1 if xw & 0x8000 else 0, attr))
        pieces[(sm, chr_)] = lst

    def obj_tile(t):
        base = G["obj"] + (0x1000 if t & 0x100 else 0)
        return tile4(vram, base, t & 0xFF)

    def spr_tiles(t, big):     # orden de la Genesis (por columnas); en el SNES la fila de abajo es t + 16
        if not big:
            return [t]
        return [t, (t & 0x100) | ((t + 16) & 0xFF), (t & 0x1F0) | ((t + 1) & 0x0F), (t & 0x100) | ((t + 17) & 0xFF)]

    # tiles a convertir: (tipo, tile, paleta) -> píxeles (índices 0-15) e índices de palette_buffer
    need = {}
    for v in wa:
        need[("a", v & 0x3FF, (v >> 10) & 7)] = tile4(vram, G["bg1_chr"], v & 0x3FF)
    for v in wb:
        need[("b", v & 0x3FF, (v >> 10) & 7)] = tile4(vram, G["bg2_chr"], v & 0x3FF)
    for k, lst in pieces.items():
        for x, y, big, attr in lst:
            for t in spr_tiles(attr & 0x1FF, big):
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
            for x, y, big, attr in lst:
                p = (attr >> 9) & 7
                out.add(frozenset().union(*(gkeys(("s", tt, p)) for tt in spr_tiles(attr & 0x1FF, big))))
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
            for li in range(4):
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
        for x, y, big, attr in pieces[k]:
            t, p = attr & 0x1FF, (attr >> 9) & 7
            key = (t, big, p)
            if key not in stile:
                first = len(tiles)
                gg = frozenset().union(*(gkeys(("s", tt, p)) for tt in spr_tiles(t, big)))
                li = next(l for l in range(4) if gg <= set(lines[l]))
                for tt in spr_tiles(t, big):
                    rows = [[1 + lines[li].index(A(128 + p * 16 + c)) if c else 0 for c in row] for row in need[("s", tt, p)]]
                    tiles.append(tbytes(rows))
                stile[key] = (first, li)
            first, li = stile[key]
            prio = 0x8000 if ((attr >> 12) & 3) >= 2 else 0
            gattr = prio | li << 13 | (0x1000 if attr & 0x8000 else 0) | (0x800 if attr & 0x4000 else 0) | (ED_TILE_BASE + first)
            out.append((y, 0x0500 if big else 0, gattr, x))
        if k[0] == "star":
            if 0x09F0 <= k[1] <= 0x09F3:
                star[k[1] - 0x09F0] = out[0][2]
            continue
        hb = 15 if any(r[1] for r in out) else 7
        ys = [r[0] for r in out] or [0]
        xs = [r[3] for r in out] or [0]
        keys += [int(k[0], 16), k[1], len(recs)]
        recs += [v & 0xFFFF for v in (len(out), min(ys), max(ys) + hb, min(xs), max(xs) + hb)]
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
    arr("uint16", "kBg2_%s" % n, bg2)
    arr("uint8", "kSrc_%s" % n, src)
    arr("uint16", "kSlogan_%s" % n, slogan or [0])
    arr("uint16", "kKeys_%s" % n, keys or [0])
    arr("uint16", "kRecs_%s" % n, recs or [0])
    C.append("const EdGroup kEd_%s = { kEdTiles_%s, %d, kLutTa_%s, kLutVa_%s, kLutTb_%s, kLutVb_%s, kLutPa_%s, kLutPb_%s, "
             "kBg2_%s, kSrc_%s, %d, kKeys_%s, kRecs_%s, { %s }, 0x%X, %s };"
             % (n, n, len(tiles), n, n, n, n, n, n, n, n, len(keys) // 3, n, n, ", ".join("0x%X" % s for s in star),
                G["bg2_scr"] or 0xFFFF, "kSlogan_%s" % n if slogan else "0"))
    H.append("extern const EdGroup kEd_%s;" % n)
    gnames.append(gname)
    print("final %s: %d palabras BG1, %d BG2, %d sprites, %d tiles (%d KB), colores %s, juntados %d"
          % (gname, len(wa), len(wb), len(pieces), len(tiles), len(tb) // 1024, [len(x) for x in lines], merged))
H.append("#endif")
(OUT / "md_ending_data.h").write_text("\n".join(H) + "\n")
(OUT / "md_ending_data.c").write_text("\n".join(C) + "\n")
