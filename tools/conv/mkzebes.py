#!/usr/bin/env python3
"""MegaMetroid — gráficos de las salas de Zebes para la Genesis (docs/37).

Para cada sala (y estado) de las áreas 0-5 prepara, desde la ROM del usuario:
  * los tiles de la Genesis que usa la sala (4bpp, sin repetir, con los flips aprovechados);
  * 2 o 3 paletas de 15 colores: para cada color, el índice de la paleta del SNES (palette_buffer) que le
    corresponde; la Genesis toma el color de ahí en cada cuadro (kCram555), así los fundidos y los efectos de
    paleta del motor se ven igual;
  * la tabla de bloques (metatiles): para cada bloque de 16x16 que puede aparecer en la sala, sus 4 celdas de
    la Genesis en las 4 combinaciones de flips del bloque.
Los bloques que pueden aparecer son los del nivel (capa 1 y, si la capa 2 es propia de la sala, la capa 2) y
los que dibujan los PLMs (puertas, scroll, bloques que se rompen...), que se sacan con un análisis estático de
sus listas de instrucciones (plmscan.py). El motor dibuja los planos de la Genesis por columnas y filas desde
el nivel (level_get) y esta tabla, como el SNES.

Uso: mkzebes.py sm.smc res/gen/engine
Salida: gen/zebes.c, gen/zebes.h (tablas en las páginas del SSF2 que siguen a las de los niveles)
"""
import os
import re
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
POOL_CHUNK = 1024          # tiles por trozo del depósito común (32 KB)
CINE_BLOBS = ("kM7Tiles", "kM7Maps", "kGetawayTiles", "kGetawayMaps")   # cinemáticas de Ceres en páginas (docs/48)
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent.parent / "tools" / "genesis_convert"))
from convert_rooms import snes_to_genesis, gen_to_cram, gen_to_rgb, color_dist, genesis_tile_bytes, snes_tile, \
    pack_palettes  # noqa: E402
from plmscan import PlmScan  # noqa: E402

rom = Path(sys.argv[1]).read_bytes()
out = Path(sys.argv[2])
HDR = len(rom) % 0x8000
COLOR_MODE = "dac"
MAX_PALS = int(__import__("os").environ.get("MM_MAX_PALS", "3"))
# Colores reservados de la línea 1 (MM_BEAM_SLOTS=1: los 9-13 para el rayo de Samus, paleta 6 del SNES, como en la
# sala base de Ceres). Apagado: con la reserva casi el doble de salas tienen que juntar colores (docs/37).
BEAM_SLOTS = range(9, 14) if __import__("os").environ.get("MM_BEAM_SLOTS") else range(16, 16)


def caps_for(npal):
    return [15 if b != 1 else 15 - len(BEAM_SLOTS) for b in range(npal)]


def slot(bi, i):
    """color i (0..) de la paleta bi -> índice en la línea de la CRAM (1..15)"""
    k = i + 1
    if bi == 1 and len(BEAM_SLOTS) and k >= BEAM_SLOTS[0]:
        k += len(BEAM_SLOTS)
    return k


def r(addr, n):
    o = HDR + ((addr >> 16) & 0x7F) * 0x8000 + (addr & 0x7FFF)
    return rom[o:o + n]


def w16(addr):
    return struct.unpack('<H', r(addr, 2))[0]


def decompress(ea):
    o = HDR + ((ea >> 16) & 0x7F) * 0x8000 + (ea & 0x7FFF)
    data, p, rv = rom, o, bytearray()
    while True:
        b = data[p]; p += 1
        if b == 0xff:
            return bytes(rv)
        if (b & 0xe0) == 0xe0:
            cmd = (8 * b) & 0xe0; size = ((b & 3) << 8 | data[p]) + 1; p += 1
        else:
            cmd = b & 0xe0; size = (b & 0x1f) + 1
        if cmd & 0x80:
            x = 0xff if cmd & 0x20 else 0
            if cmd >= 0xc0:
                s = len(rv) - data[p]; p += 1
            else:
                s = data[p] | data[p + 1] << 8; p += 2
            for i in range(size):
                rv.append(rv[s + i] ^ x)
        elif cmd == 0x20:
            rv += bytes([data[p]]) * size; p += 1
        elif cmd == 0x40:
            b1, b2 = data[p], data[p + 1]; p += 2
            for i in range(size):
                rv.append(b1 if i % 2 == 0 else b2)
        elif cmd == 0x60:
            b1 = data[p]; p += 1
            for i in range(size):
                rv.append((b1 + i) & 0xff)
        else:
            rv += data[p:p + size]; p += size


def ptr24(a):
    b = r(a, 3)
    return b[0] | b[1] << 8 | b[2] << 16


# ------------------------------------------------------------------ salas y estados (como mkdata.py)
SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}


def all_rooms():
    seen, todo = set(), [0xDF45, 0x91F8]
    while todo:
        ra = todo.pop()
        if ra in seen:
            continue
        seen.add(ra)
        dl = 0x8F0000 | w16(0x8F0000 | ra + 9)
        i = 0
        while True:
            d = w16(dl + 2 * i)
            if d < 0x8000:
                break
            dest = w16(0x830000 | d)
            if dest >= 0x8000 and dest not in seen:
                todo.append(dest)
            i += 1
    return sorted(seen)


def room_states(ra):
    q, sts = 0x8F0000 | ra + 11, []
    while True:
        k = w16(q)
        n = SEL_LEN[k]
        if n == 0:
            sts.append((q + 2) & 0xFFFF)
            return sts
        sts.append(w16(q + 2 + n - 2))
        q += 2 + n


def room_doors_in(rooms):
    """puertas que llevan a cada sala (para las tapas que se cierran al entrar)"""
    res = {}
    for ra in rooms:
        dl = 0x8F0000 | w16(0x8F0000 | ra + 9)
        i = 0
        while True:
            d = w16(dl + 2 * i)
            if d < 0x8000:
                break
            res.setdefault(w16(0x830000 | d), []).append(d)
            i += 1
    return res


# ------------------------------------------------------------------ tilesets ($8F:E6A2, 9 bytes cada uno)
_ts_cache = {}


def tileset(gs):
    if gs in _ts_cache:
        return _ts_cache[gs]
    a = 0x8FE6A2 + 9 * gs
    table_p, tiles_p, pal_p = ptr24(a), ptr24(a + 3), ptr24(a + 6)
    vram = bytearray(0x10000)
    t = decompress(tiles_p)
    vram[0:len(t)] = t
    cre = decompress(0xB98000)
    vram[0x5000:0x5000 + len(cre)] = cre
    table = bytearray(0x2000)
    ct = decompress(0xB9A09D)
    table[0:len(ct)] = ct
    tt = decompress(table_p)
    table[0x800:0x800 + len(tt)] = tt[:0x1800]
    pal = decompress(pal_p)
    palw = [pal[i] | pal[i + 1] << 8 for i in range(0, min(len(pal), 256), 2)]
    palw += [0] * (128 - len(palw))
    res = {"vram": bytes(vram), "table": bytes(table), "pal": palw}
    _ts_cache[gs] = res
    return res


# ------------------------------------------------------------------ tiles animados de la sala ($87)
ANIM_ARGS = {0x80B7: ('goto', 2), 0x80BC: ('rel', 1), 0x80D4: ('cgoto', 2), 0x80DC: ('crel', 1), 0x80E3: (None, 1),
             0x80F0: (None, 1), 0x80FC: (None, 1), 0x8108: (None, 1), 0x8114: (None, 1), 0x8120: ('bgoto1', 3),
             0x8133: (None, 1), 0x813F: ('bgoto2', 4), 0x8150: (None, 2), 0x81BA: (None, 0), 0x8303: ('bgoto2', 4),
             0x8320: (None, 2), 0x832F: (None, 2), 0x833E: ('bgoto0', 2), 0x8349: (None, 2), 0x8352: (None, 2),
             0x835B: (None, 2), 0x8372: (None, 2), 0x837F: (None, 2)}


def anim_frames(h):
    """objeto de tiles animados (cabecera $87:h) -> (bytes por trozo, palabra de VRAM, [orígenes de los trozos])"""
    lst, size, dst = w16(0x870000 | h), w16(0x870000 | h + 2), w16(0x870000 | h + 4)
    srcs, seen, todo = [], set(), [lst]
    while todo:
        p = todo.pop()
        while 0x8000 <= p and p not in seen:
            seen.add(p)
            f = w16(0x870000 | p)
            if f < 0x8000:
                srcs.append(w16(0x870000 | p + 2))
                p += 4
                continue
            if f == 0x80B2:                                   # Delete
                break
            kind, n = ANIM_ARGS.get(f, ('stop', 0))
            if kind == 'stop':
                break
            j = p + 2
            if kind == 'goto':
                p = w16(0x870000 | j)
                continue
            if kind in ('rel', 'crel'):
                t = j + ((w16(0x870000 | j) & 0xFF) ^ 0x80) - 0x80
                if kind == 'rel':
                    p = t & 0xFFFF
                    continue
                todo.append(t & 0xFFFF)
            elif kind == 'cgoto':
                todo.append(w16(0x870000 | j))
            elif kind == 'bgoto0':
                todo.append(w16(0x870000 | j))
            elif kind == 'bgoto1':
                todo.append(w16(0x870000 | j + 1))
            elif kind == 'bgoto2':
                todo.append(w16(0x870000 | j + 2))
            p = j + n
    return size, dst, sorted(set(srcs))


AREA_ANIM = [[w16(0x830000 | w16(0x83AC56 + 2 * a) + 2 * i) for i in range(8)] for a in range(8)]


def room_anims(ra, area):
    """objetos de tiles animados que puede tener la sala: los de sus FX (por puerta) y los que crean los códigos"""
    hs = set()
    for st in room_states(ra):
        b = r(0x8F0000 | st, 26)
        fx = struct.unpack_from('<H', b, 6)[0]
        setup = struct.unpack_from('<H', b, 24)[0]
        if setup == 0x91D7:
            hs |= {0x854C, 0x8552, 0x8558, 0x855E}
        if fx:
            q = 0x830000 | fx
            for _ in range(64):
                door = w16(q)
                if door == 0xFFFF:
                    break
                bits = r(q + 14, 1)[0]
                for i in range(8):
                    if bits >> i & 1 and AREA_ANIM[area][i]:
                        hs.add(AREA_ANIM[area][i])
                if door == 0:
                    break
                q += 16
    if area == 3:
        hs |= {0x8275, 0x827B}                                # cintas del Wrecked Ship (códigos de puerta)
    out = {}
    for h in hs:
        size, dst, srcs = anim_frames(h)
        if dst >= 0x4000 or not srcs or not size:              # BG3 (lluvia, esporas) y sprites: no
            continue
        t0 = dst >> 4
        for src in srcs:
            data = r(0x870000 | src, size)
            for k in range(size // 32):
                out.setdefault(t0 + k, {})[(src, dst)] = data[32 * k:32 * k + 32]
    return out


# ------------------------------------------------------------------ bloques que puede dibujar cada sala
scan = PlmScan(w16)
DOOR_SETUPS = {0xC794, 0xC7B1, 0xC7BB, 0xDA8C, 0xDAB9}
_plm_cache = {}


def plm_blocks(h):
    if h not in _plm_cache:
        bl = scan.blocks_of_plm(h) if (w16(0x840000 | h) & 0xFFFF) in DOOR_SETUPS else \
            set().union(*[scan.blocks_of_draw(d) for d in scan.draws_of_list(w16(0x840000 | h + 2), set())])
        _plm_cache[h] = bl
    return _plm_cache[h]


# funciones de inicio de PLM conocidas (el decomp): una "cabecera" cuyo inicio no es una de estas no es un PLM
_FUNCS = (HERE.parent.parent / "sm" / "src" / "funcs.h").read_text()
PLM_SETUPS = {int(m.group(1), 16) & 0xFFFF for m in re.finditer(r'#define fn\w+ 0x(84[0-9A-F]{4})', _FUNCS)
              if 'PlmSetup' in m.group(0) or 'sub_84' in m.group(0) or 'nullsub' in m.group(0)}
DOOR_CLOSING = [w16(0x8FE68A + 2 * i) for i in range(8)]   # kDoorClosingPlmIds (por dirección)
# PLMs de las reacciones de los bloques (romper, disparar, bombas, granadas...): tablas del banco $94
REACTION_PLMS = set()
for base, n in ((0x949139, 64), (0x94936B, 64), (0x949EA6, 32), (0x949DA4, 32), (0x94A012, 32)):
    for i in range(n):
        h = w16(base + 2 * i)
        if 0x8000 <= h and w16(0x840000 | h) in PLM_SETUPS and 0x8000 <= w16(0x840000 | h + 2):
            REACTION_PLMS.add(h)
for base in (0x949E44, 0x949FC6):          # por área: punteros a tablas por BTS
    for a in range(8):
        tp = w16(base + 2 * a)
        if tp < 0x8000:
            continue
        for i in range(32):
            h = w16(0x940000 | tp + 2 * i)
            if 0x8000 <= h and w16(0x840000 | h) in PLM_SETUPS and 0x8000 <= w16(0x840000 | h + 2):
                REACTION_PLMS.add(h)
_reaction_blocks = None


def reaction_blocks():
    global _reaction_blocks
    if _reaction_blocks is None:
        s = set()
        for h in REACTION_PLMS:
            try:
                s |= plm_blocks(h)
            except Exception:
                pass
        _reaction_blocks = s
    return _reaction_blocks


# ------------------------------------------------------------------ conversión de un estado
def room_items(plm_ptr):
    """objetos de la sala (PlmInstr_LoadItemPlmGfx $84:8764, la primera instrucción de su lista): por cada uno, en el
    orden en que se crean (y se procesan), (ranura, tiles en $89, 8 paletas). La ranura (plm_item_gfx_index) avanza
    con cada objeto aunque ya esté tomado: lo comprueba la instrucción siguiente."""
    items = []
    q = 0x8F0000 | plm_ptr
    while w16(q):
        h = w16(q)
        il = w16(0x840000 | h + 2)
        if 0x8000 <= il and w16(0x840000 | il) == 0x8764:
            items.append((len(items) & 3, w16(0x840000 | il + 2), list(r(0x840000 | il + 4, 8))))
        q += 6
    return items


def put_items(table, vram, items):
    """como PlmInstr_LoadItemPlmGfx: los 256 bytes de tiles a la VRAM de la ranura y los 8 tiles (con su paleta) en
    los bloques del objeto de la tabla ($84:87CD VRAM, $84:87D5 desplazamiento en la tabla, $84:87DD primer tile)"""
    table, vram = bytearray(table), bytearray(vram)
    for sl, gfx, pals in items:
        va, to, t0 = w16(0x8487CD + 2 * sl), w16(0x8487D5 + 2 * sl), w16(0x8487DD + 2 * sl)
        vram[va * 2:va * 2 + 256] = r(0x890000 | gfx, 256)
        for i in range(8):
            struct.pack_into('<H', table, to + 2 * i, (t0 + i) | pals[i] << 10)
    return bytes(table), bytes(vram)


def convert(gs, blocks, bg2=None, vram=None, anim=None, items=None, max_pals=None, usage=None):
    """bg2: las 64x32 palabras del tilemap del BG2 del SNES (fondo de biblioteca) o None; vram: la VRAM con los
    tiles que haya cargado ese fondo; items: los objetos de la sala (room_items); max_pals: líneas de la Genesis
    para la sala (sin dar: MAX_PALS; 1 en las salas de jefes que usan la línea 1, mkzobj.BOSS_LINE); usage: veces que
    aparece cada bloque en el nivel (con max_pals 1: al juntar colores pesan los píxeles que se ven, así los colores
    de bloques que la sala casi no muestra, como los de reacción de todo el juego, ceden antes que los del fondo)"""
    max_pals = max_pals or MAX_PALS
    ts = tileset(gs)
    table, pal = ts["table"], ts["pal"]
    vram = vram or ts["vram"]
    if items:
        table, vram = put_items(table, vram, items)
    anim = anim or {}
    blocks = sorted(blocks)
    ent = {}
    for mt in blocks:
        ent[mt] = [struct.unpack_from('<H', table, mt * 8 + 2 * i)[0] for i in range(4)]
    tcache, gcol = {}, {}

    def px(t):
        if t not in tcache:
            tcache[t] = snes_tile(vram, t)
        return tcache[t]

    def gc(p, c):
        k = (p, c)
        if k not in gcol:
            gcol[k] = snes_to_genesis(pal[p * 16 + c], COLOR_MODE)
        return gcol[k]
    combos = {}
    extra = list(bg2 or [])
    for e in [e for mt in blocks for e in ent[mt]] + extra:
        t, p = e & 0x3FF, (e >> 10) & 7
        if (t, p) not in combos:
            combos[(t, p)] = {gc(p, int(c)) for c in np.unique(px(t)) if c}
            for fr in anim.get(t, {}).values():               # tiles animados: los colores de todos sus cuadros
                combos[(t, p)] |= {gc(p, int(c)) for c in np.unique(snes_tile(fr, 0)) if c}
    merges = {}
    level = "subpaleta"
    bins, npal = None, min(2, max_pals)
    by_pal = {}
    for (t, p), cs in combos.items():
        by_pal.setdefault(p, set()).update(cs)
    for npal in range(min(2, max_pals), max_pals + 1):
        level = "subpaleta"
        bins = pack_palettes(list(by_pal.items()), npal, caps_for(npal))
        if bins:
            break
        level = "tile"
        bins = pack_palettes([(k, cs) for k, cs in combos.items()], npal, caps_for(npal))
        if bins:
            break
    cw = None
    if usage is not None and bins is None:                # peso de cada color: píxeles en el nivel
        cw = {}
        for mt in blocks:
            u = usage.get(mt, 0) + 1 / 64
            for e in ent[mt]:
                t, p = e & 0x3FF, (e >> 10) & 7
                a = px(t)
                for c in np.unique(a):
                    if c:
                        g = gc(p, int(c))
                        cw[g] = cw.get(g, 0) + u * int((a == c).sum())
    while bins is None:
        level = "tile+fusión"
        allc = sorted(set().union(*[{merges.get(c, c) for c in cs} for cs in combos.values()]))
        best = None
        for i in range(len(allc)):
            for j in range(i + 1, len(allc)):
                dd = color_dist(allc[i], allc[j])
                if cw is not None:
                    dd *= min(cw.get(allc[i], 0), cw.get(allc[j], 0))
                if best is None or dd < best[0]:
                    best = (dd, allc[i], allc[j])
        _, a, b = best
        if cw is not None and cw.get(b, 0) > cw.get(a, 0):   # se queda el que más se ve
            a, b = b, a
        if cw is not None:
            cw[a] = cw.get(a, 0) + cw.pop(b, 0)
        for k, v in list(merges.items()):
            if v == b:
                merges[k] = a
        merges[b] = a
        bins = pack_palettes([(k, {merges.get(c, c) for c in cs}) for k, cs in combos.items()], npal, caps_for(npal))
    assign = {}
    for bi, (cs, keys) in enumerate(bins):
        for k in keys:
            if level == "subpaleta":
                for (t, p) in combos:
                    if p == k:
                        assign[(t, p)] = bi
            else:
                assign[k] = bi
    palettes = [sorted(cs, key=lambda g: (sum(gen_to_rgb(g)), g)) for cs, _ in bins]
    # índice del SNES de cada color: el (p, c) más usado entre los que dan ese color
    uses = {}
    for (t, p), bi in assign.items():
        for c in np.unique(px(t)):
            if c:
                g = merges.get(gc(p, int(c)), gc(p, int(c)))
                uses.setdefault((bi, g), {}).setdefault(p * 16 + int(c), 0)
                uses[(bi, g)][p * 16 + int(c)] += 1
    src = []
    for bi, pl in enumerate(palettes):
        row = [0] * 16
        for i, g in enumerate(pl):
            u = uses.get((bi, g), {0: 1})
            row[slot(bi, i)] = max(u.items(), key=lambda kv: (kv[1], -kv[0]))[0]
        src.append(row)
    tile_list, tile_index = [], {}

    def own_tile(a):                                          # tile animado: propio (no se comparte)
        tile_list.append(genesis_tile_bytes(a))
        return len(tile_list) - 1, 0, 0

    def add_tile(a):
        for v, fh, fv in ((a, 0, 0), (a[:, ::-1], 1, 0), (a[::-1, :], 0, 1), (a[::-1, ::-1], 1, 1)):
            key = genesis_tile_bytes(v)
            if key in tile_index:
                return tile_index[key], fh, fv
        key = genesis_tile_bytes(a)
        tile_index[key] = len(tile_list)
        tile_list.append(key)
        return tile_index[key], 0, 0
    add_tile(np.zeros((8, 8), np.uint8))
    gtile = {}

    def cell(e):
        t, p = e & 0x3FF, (e >> 10) & 7
        pri, hf, vf = (e >> 13) & 1, (e >> 14) & 1, (e >> 15) & 1
        k = (t, p)
        if k not in gtile:
            bi = assign[k]
            lut = np.zeros(16, np.uint8)
            for c in range(1, 16):
                g = merges.get(gc(p, c), gc(p, c))
                if g in palettes[bi]:
                    lut[c] = slot(bi, palettes[bi].index(g))
            gtile[k] = ((own_tile if t in anim else add_tile)(lut[px(t)]), bi, lut)
        (gi, fh, fv), bi, _ = gtile[k]
        return (pri << 15) | (bi << 13) | ((vf ^ fv) << 12) | ((hf ^ fh) << 11) | gi
    cells = []
    for mt in blocks:
        q = ent[mt]
        for flip in range(4):                     # bit 0: flip horizontal del bloque, bit 1: vertical
            grid = [[q[0], q[1]], [q[2], q[3]]]
            if flip & 1:
                grid = [[rw[1] ^ 0x4000, rw[0] ^ 0x4000] for rw in grid]
            if flip & 2:
                grid = [[c ^ 0x8000 for c in grid[1]], [c ^ 0x8000 for c in grid[0]]]
            # orden: arriba-izq, abajo-izq, arriba-der, abajo-der (columnas)
            cells += [cell(grid[0][0]), cell(grid[1][0]), cell(grid[0][1]), cell(grid[1][1])]
    bg2c = [cell(e) for e in bg2] if bg2 else None
    # tiles animados: para cada trozo (origen, VRAM) los tiles de la Genesis que cambia, ya convertidos
    anims = {}
    for (t, p), ((gi, _, _), bi, lut) in gtile.items():
        for key, fr in anim.get(t, {}).items():
            anims.setdefault(key, []).append((gi, genesis_tile_bytes(lut[snes_tile(fr, 0)])))
    return {"anims": anims, "bg2": bg2c, "tiles": b"".join(tile_list), "ntiles": len(tile_list), "src": src, "npal": len(palettes),
            "ids": blocks, "cells": cells, "level": level, "merges": len(merges)}


# ------------------------------------------------------------------ fondo de biblioteca (LoadLibraryBackground $82:E97C)
def library_bg(gs, bg_ptr):
    """VRAM (con los tiles que cargue el fondo) y las 64x32 palabras del tilemap del BG2 (VRAM $4800, BG2SC 0x49)"""
    vram = bytearray(tileset(gs)["vram"])
    ram = {0x7E: bytearray(0x10000), 0x7F: bytearray(0x10000)}
    fill = r(0x80A29A, 1)[0]                         # ClearBG2Tilemap: los dos bytes de cada palabra
    for a in range(0x4800 * 2, 0x5000 * 2):
        vram[a] = fill
    def src_bytes(a, n):
        bank = a >> 16
        if bank in (0x7E, 0x7F):
            o = a & 0xFFFF
            return bytes(ram[bank][o:o + n])
        return r(a, n)
    j = bg_ptr
    if j & 0x8000:
        for _ in range(64):
            cmd = w16(0x8F0000 | j)
            j += 2
            if cmd == 0:
                break
            if cmd in (2, 8):
                a = ptr24(0x8F0000 | j)
                dst = w16(0x8F0000 | j + 3)
                n = w16(0x8F0000 | j + 5) or 0x10000
                data = src_bytes(a, n)
                vram[dst * 2:dst * 2 + len(data)] = data[:0x10000 - dst * 2]
                j += 7
            elif cmd == 4:
                a = ptr24(0x8F0000 | j)
                dst = w16(0x8F0000 | j + 3)
                data = decompress(a)
                ram[0x7E][dst:dst + len(data)] = data[:0x10000 - dst]
                if dst + len(data) > 0x10000:
                    rest = data[0x10000 - dst:]
                    ram[0x7F][0:len(rest)] = rest
                j += 5
            elif cmd == 0xE:                          # depende de la puerta de entrada: se toma la primera
                j += 9
            elif cmd in (6, 0xA, 0xC):
                if cmd != 6:
                    for a in range(0x4800 * 2, 0x5000 * 2):
                        vram[a] = fill
            else:
                break
    words = []
    for y in range(32):
        for x in range(64):
            wa = 0x4800 + (0x400 if x >= 32 else 0) + y * 32 + (x & 31)
            words.append(vram[wa * 2] | vram[wa * 2 + 1] << 8)
    return bytes(vram), words


# ------------------------------------------------------------------ todo
def main():
    rooms = all_rooms()
    doors_in = room_doors_in(rooms)
    th = (out / "tables.h").read_text()
    page0 = 8 + int(re.search(r'#define NT_LEVEL_PAGES (\d+)', th).group(1))
    gfx_keys, entries = {}, []
    obj_states = []
    import mkmd
    import mkzobj
    mrom = mkmd.Rom(sys.argv[1])
    zlogs = [x for x in os.environ.get("MM_ZDRAW", str(HERE.parent.parent / "res" / "snes_ref" / "zdraw.log")).split(":") if x]
    zkeys = mkzobj.read_logs(zlogs)
    for ra in rooms:
        hb = r(0x8F0000 | ra, 11)
        area = hb[1]
        if area > 5:
            continue
        anim = room_anims(ra, area)
        for st in room_states(ra):
            b = r(0x8F0000 | st, 26)
            obj_states.append((st, ra, struct.unpack_from('<H', b, 10)[0]))
            lvl = b[0] | b[1] << 8 | b[2] << 16
            gs = b[3]
            l2x = b[12]
            plm_ptr = struct.unpack_from('<H', b, 20)[0]
            # salas con el ojo (MorphBallEye en la población de este estado, mkzobj.has_eye): su haz usa el modo
            # sombra/luz del VDP (md_cone.c), que oscurece lo que no tenga prioridad alta en ningún plano: aquí todas
            # las celdas van con prioridad alta (y los sprites también, md_cone.c)
            eye = mkzobj.has_eye(mrom, st)
            # estados con un jefe de colores propios (mkzobj.BOSS_PALETTES, docs/51): la sala en una sola línea, la
            # línea 1 es de los objetos
            boss = mkzobj.boss_line(mrom, st, zkeys.get(ra, set())) is not None
            key = (lvl, gs, plm_ptr, ra, eye, boss)
            if key not in gfx_keys:
                d = decompress(lvl)
                size = d[0] | d[1] << 8
                n = size // 2
                l1 = struct.unpack_from('<%dH' % n, d, 2)
                blocks = {w & 0x3FF for w in l1}
                l2, bg2, vram = None, None, None
                bg_ptr = struct.unpack_from('<H', b, 22)[0]
                if not (l2x & 1) and len(d) >= 2 + size + n + size:
                    l2 = struct.unpack_from('<%dH' % n, d, 2 + size + n)
                    blocks |= {w & 0x3FF for w in l2}
                elif l2x & 1:
                    vram, bg2 = library_bg(gs, bg_ptr)
                setup = struct.unpack_from('<H', b, 24)[0]
                sky_tab = {0x91C9: 0x88AD9C, 0x91BD: 0x88AD9C, 0x91CE: 0x88ADA6}.get(setup)
                if sky_tab:                       # cielo de la superficie: el BG2 sale de los mapas de $8A por filas
                    bg2 = []                      # (RoomMainAsm_ScrollingSky $88:AFA3): 5 trozos de 32x32 celdas
                    for k in range(5):
                        a = 0x8A0000 | w16(sky_tab + 2 * k)
                        bg2 += [w16(a + 2 * i) for i in range(1024)]
                if plm_ptr:
                    q = 0x8F0000 | plm_ptr
                    while w16(q):
                        blocks |= {x & 0x3FF for x in plm_blocks(w16(q))}
                        q += 6
                for h in DOOR_CLOSING:
                    if h:
                        blocks |= {x & 0x3FF for x in plm_blocks(h)}
                blocks |= {x & 0x3FF for x in reaction_blocks()}
                items = room_items(plm_ptr) if plm_ptr else []
                usage = {}
                for w_ in list(l1) + list(l2 or []):
                    usage[w_ & 0x3FF] = usage.get(w_ & 0x3FF, 0) + 1
                res = convert(gs, blocks, bg2, vram, anim, items, max_pals=1 if boss else None,
                              usage=usage if boss else None)
                if eye:
                    res["cells"] = [c | 0x8000 for c in res["cells"]]
                    if res["bg2"]:
                        res["bg2"] = [c | 0x8000 for c in res["bg2"]]
                res["eye"] = 1 if eye else 0
                res["l2"] = list(l2) if l2 else None
                res["sky"] = 1 if sky_tab else 0
                gfx_keys[key] = len(entries)
                res["key"] = key
                entries.append(res)
                print(f"sala {ra:04X} estado {st:04X}: {res['ntiles']} tiles, {res['npal']} paletas, "
                      f"{len(res['ids'])} bloques, {res['level']}, fusiones {res['merges']}", file=sys.stderr)
            gfx_keys.setdefault(("st", st), gfx_keys[key])
    # ---- objetos (enemigos) de cada estado: mkzobj.py
    logs = zlogs
    st_tiles = {k[1]: entries[v]["ntiles"] for k, v in gfx_keys.items() if k[0] == "st"}
    objs = mkzobj.build(sys.argv[1], logs, [(st, ra, ts, st_tiles.get(st, 432)) for st, ra, ts in obj_states])
    obj_list, obj_of = [], {}
    for st, d in objs.items():
        if id(d) not in obj_of:
            obj_of[id(d)] = len(obj_list)
            obj_list.append(d)
    # ---- salida
    H = ['/* Generado por tools/conv/mkzebes.py desde la ROM del SNES: no editar. */',
         '#ifndef GEN_ZEBES_H', '#define GEN_ZEBES_H', '#include "nt.h"',
         'typedef struct { const uint8 *tiles; uint8 page; } ZebesPool;   /* un trozo del depósito de tiles */',
         '/* runs: nruns tramos (índice en el depósito de tiles kZebesPool, cantidad): los tiles de la sala en orden */',
         'typedef struct { uint8 page, npal; uint16 ntiles, nmeta, nruns; const uint16 *runs; const uint16 *src;',
         '                 const uint16 *ids; const uint16 *cells; const uint16 *l2; const uint16 *bg2; uint8 sky, eye;',
         '                 uint16 nanim; const uint16 *anim; const uint16 *anim_data; } ZebesGfx;',
         '/* anim: tiles animados, nanim x (origen en $87, VRAM del SNES, desplazamiento en anim_data, cantidad) ordenados;',
         '   anim_data: por tile (tile de la sala, 16 palabras del tile de la Genesis) */',
         '/* src: npal * 16 índices de palette_buffer; l2: capa 2 propia de la sala (bloques, como el nivel) o NULL;',
         '   bg2: fondo de biblioteca ya convertido (64x32 celdas) o NULL; con sky, el cielo de la superficie: 160 filas',
         '   de 32 celdas (la fila R, píxel R * 8 de la sala) */',
         'typedef struct { uint16 state; uint16 gfx; uint16 obj; } ZebesState;   /* obj: 0xFFFF sin objetos */',
         'typedef struct { uint8 page; uint16 ntiles, nkeys, nruns; const uint16 *runs; const uint16 *src; const uint16 *keys;',
         '                 const uint16 *recs; uint16 base; } ZebesObj;   /* base: primer tile en la VRAM (ZOBJ_BASE o justo después de la sala) */',
         '/* objetos: tiles desde ZOBJ_BASE; src: 30 índices de palette_buffer, colores 1-15 de la línea 3 debajo del HUD (9-13 el rayo)',
         '   y 1-15 de la línea 1 (solo con un jefe de colores propios, la sala en una línea: mkzobj.BOSS_LINE), 0xFFFF sin uso;',
         '   keys: (spritemap, paleta, tiles, desplazamiento en recs) ordenadas; recs: como los de md_data */',
         '#define ZOBJ_BASE %d' % __import__('mkzobj').ZOBJ_BASE]
    C = ['/* Generado por tools/conv/mkzebes.py desde la ROM del SNES: no editar. */', '#include "zebes.h"',
         '#include "tables.h"']
    # páginas de la intro (mkintro.py: INTRO_PAGE y las de la nave, secciones .bankN de md_intro_data.c): se saltan
    idat = HERE.parent.parent / "res" / "gen" / "md" / "md_intro_data.c"
    busy = {int(x) for x in re.findall(r'section\(\\?"\.bank(\d+)', idat.read_text())} if idat.exists() else set()
    busy = busy or {19, 20}
    # la pausa (mkpause.py, md_pause_data.c): sus tiles en una página propia
    pdat = HERE.parent.parent / "res" / "gen" / "md" / "md_pause_data.c"
    pause_pages = {}
    if pdat.exists():
        pt = pdat.read_text()
        pg = re.search(r'#define PZ_PAGE __attribute__\(\(section\("\.bank(\d+)"\)\)\)', pt)
        sz = re.search(r'kPzTiles\[(\d+)\]', pt)
        if pg and sz:
            pause_pages[int(pg.group(1))] = int(sz.group(1)) + 2
    # lo que ocupa la intro en sus páginas (de las declaraciones de md_intro_data.c, con margen): el resto de esas
    # páginas también sirve para Zebes
    tsz = {'uint8': 1, 'int8': 1, 'uint16': 2, 'int16': 2, 'uint32': 4}
    busy_used = {pg: 0 for pg in busy}
    if idat.exists():
        for m in re.finditer(r'section\(\\?"\.bank(\d+)\\?"\)\)\)\s+(?:static\s+)?const\s+(\w+)\s+\w+\[(\d+)\]',
                             idat.read_text()):
            busy_used[int(m.group(1))] = busy_used.get(int(m.group(1)), 0) + tsz.get(m.group(2), 8) * int(m.group(3)) + 2
    page, used = page0, 0

    def next_page(pg):
        pg += 1
        assert pg <= int(os.environ.get("MM_LAST_PAGE", "23")), "zebes: no entra en las páginas del SSF2 (md_ssf2.ld llega a la 23)"
        return pg
    # primer hueco: cada bloque va a la primera página donde cabe (antes, una página tras otra: lo que sobraba al
    # final de cada página se perdía)
    fill = {page0: 0}
    for pg, u in busy_used.items():
        if pg > page0:
            fill[pg] = u + 16384                    # margen
    for pg, u in pause_pages.items():
        fill[pg] = fill.get(pg, 0) + u + 4096

    def place(need):
        for pg in sorted(fill):
            if fill[pg] + need <= 0x80000 - 4096:   # margen: la cuenta no incluye alineaciones
                fill[pg] += need
                return pg
        pg = page0
        while pg in fill:
            pg = next_page(pg)
        fill[pg] = need
        return pg
    # tiles en un depósito común (docs/48): cada tile distinto una sola vez, en trozos de POOL_CHUNK tiles repartidos
    # en las páginas; cada sala y cada juego de objetos guarda sus tramos (índice en el depósito, cantidad) y la
    # Genesis los copia a la VRAM en el mismo orden de siempre (md_zebes.c, zebes_dma_runs): la VRAM queda igual
    pool_idx, pool = {}, []

    def to_runs(tiles):
        runs = []
        for k in range(len(tiles) // 32):
            t = tiles[k * 32:k * 32 + 32]
            if t not in pool_idx:
                pool_idx[t] = len(pool)
                pool.append(t)
            j = pool_idx[t]
            if runs and runs[-1][0] + runs[-1][1] == j and runs[-1][1] < 0xFFFF:
                runs[-1][1] += 1
            else:
                runs.append([j, 1])
        return [v for r in runs for v in r]
    for e in entries:
        e["runs"] = to_runs(bytes(e["tiles"]))
    for d in obj_list:
        d["runs"] = to_runs(bytes(d["tiles"]))
    for e in entries + obj_list:                   # comprobación: los tramos rehacen los tiles de siempre
        rr = e["runs"]
        assert b"".join(pool[j + k] for j, n in zip(rr[0::2], rr[1::2]) for k in range(n)) == bytes(e["tiles"])
    assert len(pool) <= 0xFFFF, "zebes: el depósito de tiles pasa de 65535"
    nchunks = (len(pool) + POOL_CHUNK - 1) // POOL_CHUNK
    print(f"zebes: tiles {sum(len(e['tiles']) for e in entries + obj_list) // 32} -> depósito {len(pool)} "
          f"({nchunks} trozos), tramos {sum(len(e['runs']) for e in entries + obj_list) // 2}", file=sys.stderr)
    # los más grandes primero (primer hueco decreciente): las páginas quedan más llenas
    g_need = [2 * len(e["runs"]) + 32 * e["npal"] + 2 * len(e["ids"]) + 2 * len(e["cells"]) + 64 +
              sum(8 + 34 * len(v) for v in e["anims"].values()) +
              2 * len(e["l2"] or []) + 2 * len(e["bg2"] or []) for e in entries]
    o_need = [2 * len(d["runs"]) + 2 * len(d["src"]) + 8 * len(d["keys"]) + 2 * len(d["recs"]) for d in obj_list]
    p_need = [32 * len(pool[c * POOL_CHUNK:(c + 1) * POOL_CHUNK]) for c in range(nchunks)]
    # cinemáticas de Ceres (mkm7.py, mkgetaway.py: corren antes): se usan una vez por partida, van en páginas y no en
    # la parte fija (docs/48). La Genesis pone la página de cada bloque antes de leerlo (md_cine.h)
    cine_dir = HERE.parent.parent / "res" / "gen" / "md"
    cine = [s_ for s_ in CINE_BLOBS if (cine_dir / "bins" / f"{s_}.bin").exists()]
    assert len(cine) == len(CINE_BLOBS), "zebes: faltan los bloques de las cinemáticas (correr mkm7.py y mkgetaway.py antes)"
    c_need = [(cine_dir / "bins" / f"{s_}.bin").stat().st_size + 16 for s_ in cine]
    pages, obj_pages, pool_pages, cine_pages = [0] * len(entries), [0] * len(obj_list), [0] * nchunks, [0] * len(cine)
    for n_, kind, i in sorted([(n, 0, i) for i, n in enumerate(g_need)] + [(n, 1, i) for i, n in enumerate(o_need)] +
                              [(n, 2, i) for i, n in enumerate(p_need)] + [(n, 3, i) for i, n in enumerate(c_need)],
                              reverse=True):
        (pages, obj_pages, pool_pages, cine_pages)[kind][i] = place(n_)
    cs = ["/* Generado por tools/conv/mkzebes.py: bloques de las cinemáticas de Ceres en páginas del SSF2 (md_cine.h) */"]
    ch = ["/* Generado por tools/conv/mkzebes.py: página del SSF2 de cada bloque de las cinemáticas */",
          "#ifndef MD_CINE_H", "#define MD_CINE_H"]
    for s_, pg in zip(cine, cine_pages):
        cs += [f'    .section .bank{pg},"a"', "    .align 2", f"    .global {s_}", f"{s_}:", f'    .incbin "bins/{s_}.bin"']
        ch.append(f"#define CINE_PAGE_{s_} {pg}")
    (cine_dir / "md_cine.s").write_text("\n".join(cs) + "\n")
    (cine_dir / "md_cine.h").write_text("\n".join(ch + ["#endif"]) + "\n")
    for c in range(nchunks):
        tb = b"".join(pool[c * POOL_CHUNK:(c + 1) * POOL_CHUNK])
        C.append(f'NT_BANK({pool_pages[c]}) static const uint8 kZP{c}[{len(tb)}] __attribute__((aligned(2))) = {{' +
                 ','.join(str(x) for x in tb) + '};')
    H.append(f'#define ZEBES_POOL_CHUNK {POOL_CHUNK}')
    H.append(f'extern const ZebesPool kZebesPool[{nchunks}];')
    C.append(f'const ZebesPool kZebesPool[{nchunks}] = {{')
    C += [f'  {{ kZP{c}, {pool_pages[c]} }},' for c in range(nchunks)]
    C.append('};')
    for i, e in enumerate(entries):
        page = pages[i]
        C.append(f'NT_BANK({page}) static const uint16 kZG{i}_runs[{len(e["runs"])}] = {{' +
                 ','.join(str(x) for x in e["runs"]) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZG{i}_src[{16 * e["npal"]}] = {{' +
                 ','.join(str(x) for row in e["src"] for x in row) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZG{i}_ids[{len(e["ids"])}] = {{' +
                 ','.join(str(x) for x in e["ids"]) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZG{i}_cells[{len(e["cells"])}] = {{' +
                 ','.join('0x%X' % x for x in e["cells"]) + '};')
        if e["l2"]:
            C.append(f'NT_BANK({page}) static const uint16 kZG{i}_l2[{len(e["l2"])}] = {{' +
                     ','.join('0x%X' % x for x in e["l2"]) + '};')
        if e["anims"]:
            idx, data = [], []
            for key in sorted(e["anims"]):
                lst = e["anims"][key]
                idx += [key[0], key[1], len(data), len(lst)]
                for gi, tb in lst:
                    data.append(gi)
                    data += list(struct.unpack('>16H', tb))
            C.append(f'NT_BANK({page}) static const uint16 kZG{i}_anim[{len(idx)}] = {{' + ','.join('0x%X' % x for x in idx) + '};')
            C.append(f'NT_BANK({page}) static const uint16 kZG{i}_animd[{len(data)}] = {{' + ','.join('0x%X' % x for x in data) + '};')
        if e["bg2"]:
            C.append(f'NT_BANK({page}) static const uint16 kZG{i}_bg2[{len(e["bg2"])}] = {{' +
                     ','.join('0x%X' % x for x in e["bg2"]) + '};')
    for i, d in enumerate(obj_list):
        page = obj_pages[i]
        C.append(f'NT_BANK({page}) static const uint16 kZO{i}_runs[{len(d["runs"])}] = {{' +
                 ','.join(str(x) for x in d["runs"]) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZO{i}_src[30] = {{' + ','.join(str(x) for x in d["src"]) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZO{i}_keys[{4 * len(d["keys"])}] = {{' +
                 ','.join('0x%X' % v for k in d["keys"] for v in k) + '};')
        C.append(f'NT_BANK({page}) static const uint16 kZO{i}_recs[{len(d["recs"])}] = {{' +
                 ','.join('0x%X' % v for v in d["recs"]) + '};')
    H.append(f'extern const ZebesObj kZebesObj[{max(1, len(obj_list))}];')
    C.append(f'const ZebesObj kZebesObj[{max(1, len(obj_list))}] = {{')
    for i, d in enumerate(obj_list):
        C.append(f'  {{ {obj_pages[i]}, {len(d["tiles"]) // 32}, {len(d["keys"])}, {len(d["runs"]) // 2}, kZO{i}_runs, kZO{i}_src, kZO{i}_keys, kZO{i}_recs, {d["base"]} }},')
    if not obj_list:
        C.append('  { 0 },')
    C.append('};')
    H.append(f'#define NT_ZEBES_PAGE0 {page0}')
    H.append(f'#define NT_ZEBES_PAGES {max(fill) - page0 + 1}')
    H.append(f'extern const ZebesGfx kZebesGfx[{len(entries)}];')
    C.append(f'const ZebesGfx kZebesGfx[{len(entries)}] = {{')
    for i, e in enumerate(entries):
        C.append(f'  {{ {pages[i]}, {e["npal"]}, {e["ntiles"]}, {len(e["ids"])}, {len(e["runs"]) // 2}, kZG{i}_runs, kZG{i}_src, '
                 f'kZG{i}_ids, kZG{i}_cells, {"kZG%d_l2" % i if e["l2"] else "0"}, '
                 f'{"kZG%d_bg2" % i if e["bg2"] else "0"}, {e["sky"]}, {e["eye"]}, {len(e["anims"])}, '
                 f'{"kZG%d_anim" % i if e["anims"] else "0"}, {"kZG%d_animd" % i if e["anims"] else "0"} }},  /* sala {e["key"][3]:04X} */')
    C.append('};')
    sts = sorted((k[1], v, obj_of[id(objs[k[1]])] if k[1] in objs else 0xFFFF) for k, v in gfx_keys.items() if k[0] == "st")
    H.append(f'extern const ZebesState kZebesStates[{len(sts)}];  /* ordenados por estado (búsqueda binaria) */')
    C.append(f'const ZebesState kZebesStates[{len(sts)}] = {{ ' + ', '.join('{0x%X, %d, 0x%X}' % s for s in sts) + ' };')
    H.append('#endif')
    (out / "zebes.h").write_text('\n'.join(H) + '\n')
    (out / "zebes.c").write_text('\n'.join(C) + '\n')
    tot = sum(len(e["tiles"]) + 32 * e["npal"] + 2 * len(e["ids"]) + 2 * len(e["cells"]) +
              2 * len(e["l2"] or []) + 2 * len(e["bg2"] or []) for e in entries)
    print(f"zebes: {len(entries)} gráficos de sala, {tot // 1024} KB, páginas {page0}-{max(fill)}, libre {sum(0x80000 - v for v in fill.values()) // 1024} KB", file=sys.stderr)


main()
