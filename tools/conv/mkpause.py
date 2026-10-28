#!/usr/bin/env python3
"""MegaMetroid — gráficos de la pausa (mapa y equipo) para la Genesis, desde la ROM del SNES (docs/53).

El SNES carga en la pausa ($82:8E75) los tiles de $B6:8000 (BG1/BG2, VRAM 0x0000) y $B6:C000 (sprites, VRAM
0x2000 = tiles de BG 0x200-0x2FF), y la paleta kPauseScreenPalettes ($B6:F000). Lo que se dibuja sale de mapas
fijos de la ROM ($B6:E000 BG2, $B6:E800 equipo, los mapas de las áreas en $B5, los textos del equipo en $82) con
algunas palabras cambiadas por el código (paletas de los textos, flecha que brilla, dígitos, AUTO/MANUAL).

Aquí se juntan todas las palabras que pueden aparecer en el BG1/BG2 de la pausa y los spritemaps que dibuja el
código ($82:C569, con la paleta que les da), y se arman:
  kPzTiles      tiles de la Genesis (desde PZ_TILE_BASE; en la página PZ_PAGE del SSF2): cada par (tile, paleta) del SNES con sus colores puestos
                en una línea de la CRAM; los iguales (también volteados) una sola vez
  kPzLutT/P/V   palabra del SNES (tile y paleta) -> celda de la Genesis (tile, línea y volteo a sumar)
  kPzSrc[64]    color de la CRAM -> índice de palette_buffer (0xFF: nada); la línea 3 empieza con los del HUD
  kPzSprIdx     spritemap (0-0x67) y paleta (0-7) -> registros en kPzSprRecs (como kSamusSprRecs)
Los colores se juntan por valor (la paleta de la pausa es fija) salvo los que el código cambia cuadro a cuadro
(176-191: la paleta animada; 102 y 107: la flecha de la reserva), que van cada uno en su lugar.
Uso: mkpause.py rom.smc carpeta_salida (res/gen/md, después de mkmd.py: lee de md_data.h los colores del HUD)
"""
import sys
from pathlib import Path

ROM = Path(sys.argv[1]).read_bytes()
OUT = Path(sys.argv[2])
PZ_TILE_BASE = 256          # tiles 256-1279 de la Genesis: los de la sala (se vuelven a cargar al salir)
PZ_TILE_MAX = 1024
PZ_PAGE = 18                # página del SSF2 de los tiles (vacía; mkzebes.py la cuenta como ocupada)
ANIMATED = set(range(176, 192)) | {102, 107}


def off(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def w(a):
    o = off(a)
    return ROM[o] | ROM[o + 1] << 8


def b(a):
    return ROM[off(a)]


def ws(a, n):
    return [w(a + 2 * i) for i in range(n)]


PAL = ws(0xB6F000, 256)
VRAM = bytearray(0x6000)
VRAM[0:0x4000] = ROM[off(0xB68000):off(0xB68000) + 0x4000]
VRAM[0x4000:0x6000] = ROM[off(0xB6C000):off(0xB6C000) + 0x2000]


def tile_px(t):
    """tile t de BG (0-0x2FF) del SNES (4bpp) -> 8 filas de 8 índices"""
    d = VRAM[t * 32:t * 32 + 32]
    rows = []
    for y in range(8):
        p0, p1, p2, p3 = d[2 * y], d[2 * y + 1], d[16 + 2 * y], d[17 + 2 * y]
        rows.append(tuple(((p0 >> (7 - x)) & 1) | (((p1 >> (7 - x)) & 1) << 1) | (((p2 >> (7 - x)) & 1) << 2)
                          | (((p3 >> (7 - x)) & 1) << 3) for x in range(8)))
    return tuple(rows)


# ---------------------------------------------------------------- palabras del BG1/BG2 que pueden aparecer
words = set()


def add(v, pals=None):
    if pals is None:
        words.add(v & 0x1FFF)
    else:
        for p in pals:
            words.add(((v & 0xE3FF) | p << 10) & 0x1FFF)


# mapas de las áreas ($82:964A: punteros largos a $B5) y celda vacía; explorado = sin el bit 0x400
for a in range(7):
    p = w(0x82964A + 3 * a) | b(0x82964A + 3 * a + 2) << 16
    for v in ws(p, 2048):
        add(v)
        add(v & ~0x400)
add(0x1F)
# BG2 ($B6:E000) y las filas de L/R/START (7E:3640, desde $B6:E640) con los textos en paleta 2 o 5
for v in ws(0xB6E000, 1024):
    add(v)
for v in ws(0xB6E640, 64):
    add(v)
    add(v, (2, 5))
# nombres de las áreas (kPauseAreaLabelTilemap, $82:965F)
for a in range(8):
    for v in ws(0x820000 | w(0x82965F + 2 * a), 12):
        add(v)
# equipo: $B6:E800 y lo que se le escribe encima
base = ws(0xB6E800, 1024)
for v in base:
    add(v)
for i in (129 + 32 * k for k in range(8)):
    add(base[i], (6, 7))
for i in (385, 386):
    add(base[i], (6, 7))
for d in range(10):
    add(2052 + d)
for tab in (0x82BF22, 0x82BF2A):                       # MANUAL / AUTO sobre lo que haya
    for i in range(4):
        add((base[327 + i] & 0xFC00) | w(tab + 2 * i))
for wf in range(4):                                     # siluetas de Samus (kEquipmentScreenWireframePtrs)
    for v in ws(0x820000 | w(0x82B25F + 2 * wf), 17 * 8):
        add(v)
eq_tabs = [(0x82C088, 2, 7), (0x82C08C, 5, 5), (0x82C0A8, 5, 5), (0x82C096, 6, 9), (0x82C0A2, 3, 9)]
for tab, n, ln in eq_tabs:
    for i in range(n):
        for v in ws(0x820000 | w(tab + 2 * i), ln):
            add(v)
            add(v, (3,))                                # ChangePaletteValues(…, 0xC00): sin equipar
words.discard(0)

# ---------------------------------------------------------------- spritemaps ($82:C569) y paletas que se usan
SPR = {}   # id -> set de paletas


def spr(i, pal):
    SPR.setdefault(i, set()).add(pal)


for i in (9, 0x62, 0xB, 0xA, 0x4E, 0x63, 0x5F, 0x60, 0x61):
    spr(i, 7)
spr(9, 6)
spr(8, 2)
for i in (0x28, 0x29, 0x2B):
    spr(i, 0)
for a in range(6):                                      # destinos de los ascensores (kMapElevatorDests; Ceres no tiene estación)
    p = w(0x82C74D + 2 * a)
    while w(0x820000 | p) != 0xFFFF:
        spr(w(0x820000 | p + 4), 0)
        p += 6
for i in (0x1B, 0x1F) + tuple(range(0x20, 0x28)):      # reserva (paleta 3, $82:B3F9)
    spr(i, 3)
# animaciones (DrawPauseScreenSpriteAnim): 2 L/R (por modo 0-2), 3 selector (categoría 0-3), 6-9 flechas
for a, sels in ((2, range(3)), (3, range(4)), (6, (0,)), (7, (0,)), (8, (0,)), (9, (0,))):
    t = a - 1
    data = w(0x82C0E8 + 2 * t)
    frames = []
    f = 0
    while True:
        if f and b(0x820000 | data + 3 * f) == 0xFF:
            break
        frames.append(b(0x820000 | data + 3 * f + 2))
        f += 1
    for s in sels:
        bid = w(0x820000 | (2 * s + w(0x82C1E4 + 2 * t)))
        for r24 in frames:
            spr(r24 + bid, 3)

# piezas de cada spritemap
pieces = {}
for i in SPR:
    p = 0x820000 | w(0x82C569 + 2 * i)
    n = w(p)
    lst = []
    for k in range(n):
        q = p + 2 + 5 * k
        xw = w(q)
        x = (xw & 0x1FF) - (0x200 if xw & 0x100 else 0)
        y = b(q + 2)
        y = y - 256 if y >= 128 else y
        attr = w(q + 3)
        lst.append((x, y, bool(xw & 0x8000), attr))
    pieces[i] = lst

# ---------------------------------------------------------------- colores: claves y grupos
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mkmd import bgr_to_gen, gen_to_cram   # noqa: E402  (el mismo color de la CRAM que dará kCram555)


def ckey(idx):
    """colores que el código cambia: cada uno el suyo; los demás, por el color que tendrán en la CRAM"""
    return ('i', idx) if idx in ANIMATED else ('v', gen_to_cram(bgr_to_gen(PAL[idx] & 0x7FFF)))


groups = {}     # (base, tiles, pal) -> set de claves


def bg_group(t, p):
    keys = set()
    for row in tile_px(t):
        for c in row:
            if c:
                keys.add(ckey(p * 16 + c))
    return frozenset(keys)


def spr_tiles(t, big):
    return [t, t + 16, t + 1, t + 17] if big else [t]    # orden de la Genesis (por columnas)


need = {}       # (kind, tile, pal) -> claves; kind 'b' (BG) o 's' (sprite: pal 8-15)
for v in words:
    t, p = v & 0x3FF, (v >> 10) & 7
    need[('b', t, p)] = bg_group(t, p)
for i, lst in pieces.items():
    for pal in SPR[i]:
        for x, y, big, attr in lst:
            for t in spr_tiles(attr & 0x1FF, big):
                assert t < 0x100, "sprite fuera de $B6:C000"
                need[('s', t, pal)] = bg_group(0x200 + t, 8 + pal)

# Líneas: 0-2 (15 colores cada una) y la 3, que ya tiene el HUD en los colores 1-HUD_NSLOTS (índices HUD_IDX de
# palette_buffer, mkmd.py: los de la pausa en esos índices se pueden usar) y deja 3 libres. Se busca un reparto en que
# cada tile tenga todos sus colores en una línea; si no hay, el color estático menos usado pasa al más parecido.
import re  # noqa: E402
_hd = (OUT / "md_data.h").read_text()
HUD_NSLOTS = int(re.search(r"#define HUD_NSLOTS (\d+)", _hd).group(1))
HUD_IDX = [int(x) for x in re.search(r"/\* HUD_IDX ([\d ]+) :", _hd).group(1).split()]
assert len(HUD_IDX) == HUD_NSLOTS
alias = {}
cnt = {}


def K(idx):
    k = ckey(idx)
    while k in alias:
        k = alias[k]
    return k


def tile_keys(kind, t, p):
    pal = p if kind == 'b' else 8 + p
    keys = set()
    for row in tile_px(t if kind == 'b' else 0x200 + t):
        for c in row:
            if c:
                keys.add(K(pal * 16 + c))
    return frozenset(keys)


for (kind, t, p) in list(need):
    pal = p if kind == 'b' else 8 + p
    for row in tile_px(t if kind == 'b' else 0x200 + t):
        for c in row:
            if c:
                cnt[ckey(pal * 16 + c)] = cnt.get(ckey(pal * 16 + c), 0) + 1


def pack():
    hudk = {K(i) for i in HUD_IDX}
    gs = sorted({tile_keys(*k) for k in need} - {frozenset()}, key=lambda g: (-len(g), sorted(g)))   # (orden fijo)
    G = [g for g in gs if not any(g < h for h in gs)]
    tries = [0]

    def rec(i, lines):
        tries[0] += 1
        if tries[0] > 300000:
            return None
        if i == len(G):
            return [set(l) for l in lines]
        seen = set()
        for li in range(4):
            if frozenset(lines[li]) in seen:   # (líneas iguales: probar una)
                continue
            seen.add(frozenset(lines[li]))
            u = lines[li] | G[i]
            if len(u - hudk if li == 3 else u) <= (15 - HUD_NSLOTS if li == 3 else 15):
                old = lines[li]
                lines[li] = u
                r = rec(i + 1, lines)
                lines[li] = old
                if r:
                    return r
        return None
    return rec(0, [set(), set(), set(), set()]), hudk


def cram_rgb(k):
    return ((k[1] >> 1) & 7, (k[1] >> 5) & 7, (k[1] >> 9) & 7)


merged = []
while True:
    res, hudk = pack()
    if res:
        break
    static = sorted((k for k in cnt if k[0] == 'v' and k not in alias), key=lambda k: (cnt[k], k))
    k = static[0]
    near = min((j for j in static if j != k), key=lambda j: sum((a - b) ** 2 for a, b in zip(cram_rgb(k), cram_rgb(j))))
    alias[k] = near
    cnt[near] += cnt.pop(k)
    merged.append((k[1], near[1]))
# orden de los colores en cada línea; la 3 empieza con los del HUD
lines = [sorted(res[li]) for li in range(3)]
hud_keys = [K(i) for i in HUD_IDX]
lines.append(hud_keys + sorted(res[3] - set(hud_keys)))
where = {}
for kk in need:
    g = tile_keys(*kk)
    where[g] = next(li for li in range(4) if g <= set(lines[li]))
src = [0xFF] * 64
src[0] = 0                                             # fondo: palette_buffer[0]
for li in range(4):
    for j, k in enumerate(lines[li]):
        if li == 3 and j < HUD_NSLOTS:
            src[48 + 1 + j] = HUD_IDX[j]                  # el HUD, como en las salas (mkmd.py: hud_slots)
        elif k[0] == 'i':
            src[li * 16 + 1 + j] = k[1]
        else:
            src[li * 16 + 1 + j] = next(i for i in range(256) if i not in ANIMATED and K(i) == k)
assert all(len(l) <= 15 for l in lines)


def slot_of(li, key):
    return 1 + lines[li].index(key)


# ---------------------------------------------------------------- tiles
tiles = []      # bytes de 32
tindex = {}     # bytes -> (n, flips)


def gen_tile(kind, t, p):
    li = where[tile_keys(kind, t, p)]
    src_t = t if kind == 'b' else 0x200 + t
    pal = p if kind == 'b' else 8 + p
    px = tile_px(src_t)
    rows = [[slot_of(li, K(pal * 16 + c)) if c else 0 for c in row] for row in px]
    return li, rows


def tbytes(rows):
    return bytes((r[2 * i] << 4) | r[2 * i + 1] for r in rows for i in range(4))


def put_tile(rows, dedup_flip=True):
    """-> (n, flips de la Genesis 0x800 h / 0x1000 v)"""
    cands = [(rows, 0)]
    if dedup_flip:
        cands += [([r[::-1] for r in rows], 0x800), (rows[::-1], 0x1000), ([r[::-1] for r in rows[::-1]], 0x1800)]
    for rr, fl in cands:
        k = tbytes(rr)
        if k in tindex:
            return tindex[k], fl
    k = tbytes(rows)
    tindex[k] = len(tiles)
    tiles.append(k)
    return tindex[k], 0


lut = [0] * 8192
for v in sorted(words):
    t, p = v & 0x3FF, (v >> 10) & 7
    li, rows = gen_tile('b', t, p)
    if not tile_keys('b', t, p):
        n, fl = put_tile([[0] * 8 for _ in range(8)])
        li = 0
    else:
        n, fl = put_tile(rows)
    lut[v] = (PZ_TILE_BASE + n) | li << 13 | fl

# sprites: los cuatro tiles de uno grande seguidos (sin voltear ni compartir con los de BG)
spr_tile_of = {}
recs = []
spr_idx = {}
for i in sorted(pieces):
    for pal in sorted(SPR[i]):
        out = []
        for x, y, big, attr in pieces[i]:
            t = attr & 0x1FF
            key = (t, big, pal)
            gg = set().union(*(tile_keys('s', tt, pal) for tt in spr_tiles(t, big)))
            li = next((l for l in range(4) if gg <= set(lines[l])), None)
            assert li is not None, "sprite grande con colores en dos líneas: %X" % t
            if key not in spr_tile_of:
                first = len(tiles)
                for tt in spr_tiles(t, big):
                    rows = [[slot_of(li, K((8 + pal) * 16 + c)) if c else 0 for c in row] for row in tile_px(0x200 + tt)]
                    tiles.append(tbytes(rows))
                spr_tile_of[key] = (first, li)
            first, li = spr_tile_of[key]
            prio = 0x8000 if ((attr >> 12) & 3) == 3 else 0
            gattr = prio | li << 13 | (0x1000 if attr & 0x8000 else 0) | (0x800 if attr & 0x4000 else 0) | (PZ_TILE_BASE + first)
            out.append((y, 0x0500 if big else 0, gattr, x))
        ys = [r[0] for r in out] or [0]
        xs = [r[3] for r in out] or [0]
        hb = 15 if any(r[1] for r in out) else 7
        spr_idx[(i, pal)] = len(recs)
        recs += [v & 0xFFFF for v in (len(out), min(ys), max(ys) + hb, min(xs), max(xs) + hb)]
        for r in out:
            recs += [r[0] & 0xFFFF, r[1], r[2], r[3] & 0xFFFF]
assert len(tiles) <= PZ_TILE_MAX, "mkpause: %d tiles (máx. %d)" % (len(tiles), PZ_TILE_MAX)

# ---------------------------------------------------------------- salida
NSM = 0x68
c = ["/* Generado por tools/conv/mkpause.py (docs/53): gráficos de la pausa desde la ROM. No editar. */",
     '#include "md_pause_data.h"']


def arr(t, name, vals, per=16, fmt="0x%X"):
    c.append("const %s %s[%d] = {" % (t, name, len(vals)))
    for k in range(0, len(vals), per):
        c.append("  " + ", ".join(fmt % v for v in vals[k:k + per]) + ",")
    c.append("};")


tb = b"".join(tiles)
c.append("#ifdef NT_SSF2")
c.append('#define PZ_PAGE __attribute__((section(".bank%d")))' % PZ_PAGE)
c.append("#else")
c.append("#define PZ_PAGE")
c.append("#endif")
c.append("PZ_PAGE const uint8 kPzTiles[%d] __attribute__((aligned(2))) = {" % len(tb))
for k in range(0, len(tb), 32):
    c.append("  " + ", ".join("0x%X" % v for v in tb[k:k + 32]) + ",")
c.append("};")
# LUT por tile: kPzLutT[tile] -> primera variante en kPzLutP/kPzLutV (paleta, celda), cada lista termina en 0xFF
lt, lp, lv = [0xFFFF] * 1024, [], []
for t in range(1024):
    vs = [(p, lut[(p << 10) | t]) for p in range(8) if lut[(p << 10) | t]]
    if vs:
        lt[t] = len(lp)
        for p, v in vs:
            lp.append(p)
            lv.append(v)
        lp.append(0xFF)
        lv.append(0)
arr("uint16", "kPzLutT", lt)
arr("uint8", "kPzLutP", lp, per=32)
arr("uint16", "kPzLutV", lv)
arr("uint8", "kPzSrc", src)
arr("uint16", "kPzSprRecs", recs)
idx = []
for i in range(NSM):
    for pal in range(8):
        idx.append(spr_idx.get((i, pal), 0xFFFF))
arr("uint16", "kPzSprIdx", idx, per=8)
h = ["/* Generado por tools/conv/mkpause.py (docs/53). No editar. */", "#ifndef MD_PAUSE_DATA_H", "#define MD_PAUSE_DATA_H",
     "#include <stdint.h>", "typedef uint8_t uint8; typedef uint16_t uint16;",
     "#define PZ_TILE_BASE %d" % PZ_TILE_BASE, "#define PZ_TILE_COUNT %d" % len(tiles),
     "#define PZ_SPR_COUNT %d   /* spritemaps de $82:C569 */" % NSM,
     "#define PZ_PAGE_N %d   /* página del SSF2 de kPzTiles (ventana 7 al copiarlos) */" % PZ_PAGE,
     "extern const uint8 kPzTiles[%d];" % len(tb),
     "/* palabra del SNES (tile t, paleta p) -> celda: kPzLutT[t] = primera de sus variantes; kPzLutP (paleta, 0xFF fin)",
     "   y kPzLutV (celda: tile, línea y volteo de la Genesis) */",
     "extern const uint16 kPzLutT[1024];",
     "extern const uint8 kPzLutP[%d];" % len(lp),
     "extern const uint16 kPzLutV[%d];" % len(lv),
     "extern const uint8 kPzSrc[64];      /* color de la CRAM -> índice de palette_buffer (0xFF: nada) */",
     "extern const uint16 kPzSprRecs[%d];" % len(recs),
     "extern const uint16 kPzSprIdx[%d];   /* [spritemap * 8 + paleta] -> kPzSprRecs (0xFFFF: no está) */" % len(idx),
     "#endif"]
(OUT / "md_pause_data.h").write_text("\n".join(h) + "\n")
(OUT / "md_pause_data.c").write_text("\n".join(c) + "\n")
print("pausa: %d palabras, %d spritemaps, %d tiles (%d KB), colores por línea %s, colores juntados %s"
      % (len(words), sum(len(v) for v in SPR.values()), len(tiles), len(tb) // 1024, [len(l) for l in lines],
         ["%03X->%03X" % m for m in merged]))
