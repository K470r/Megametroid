#!/usr/bin/env python3
"""MegaMetroid — sprites de los enemigos de Zebes para la Genesis (docs/37).

Para cada estado de sala de Zebes con enemigos dibujados (registros de dibujo del motor en el PC, MM_DRAWLOG):
  * arma la VRAM de sprites del SNES como la deja el juego al cargar la sala (ProcessEnemyTilesets $A0:8D64:
    los tiles de cada enemigo de la lista de la sala, en orden, desde el tile 0x100; la paleta de cada uno en
    la línea que dice la lista);
  * convierte cada spritemap dibujado (clave enemigo: banco, spritemap, paleta, índice de tiles) a sprites de la
    Genesis pieza por pieza (16x16 -> sprite de 2x2, 8x8 -> 1x1, los flips en el atributo), con los tiles
    repetidos compartidos;
  * arma la paleta de objetos: en la Genesis la línea 3 de la CRAM es del HUD arriba y, debajo del HUD (la
    interrupción horizontal la cambia), de los objetos: los colores 1-12 son de los enemigos de la sala (13-15
    quedan como en la sala base: el rayo). Para cada color, el índice de palette_buffer del SNES que le
    corresponde (así los fundidos y los efectos de paleta se ven igual). Si los enemigos usan más de 12 colores,
    se juntan los más parecidos.

Uso (desde mkzebes.py): build(rom_path, draw_logs) -> {estado: datos}
"""
import os
import struct
import sys
import re
from collections import Counter
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent.parent / "tools" / "genesis_convert"))
import mkmd  # noqa: E402
from convert_rooms import color_dist, genesis_tile_bytes  # noqa: E402

ZOBJ_BASE = 880        # primer tile de los objetos de la sala en la VRAM de la Genesis (la sala: 448 + hasta 409)
ZOBJ_MAX = 1280 - ZOBJ_BASE
OBJ_LINE = 3
# Línea 3 debajo del HUD (la interrupción horizontal la cambia): colores 1-8 y 14-15 de los enemigos de la sala,
# 9-13 del rayo de Samus (paleta 6 del SNES, E1-E5, como en la sala base de Ceres: los sprites de los disparos los
# usan en esos índices; md_engine.c los pasa a la línea 3 en Zebes).
OBJ_SLOT_LIST = [1, 2, 3, 4, 5, 6, 7, 8, 14, 15]
OBJ_SLOTS = len(OBJ_SLOT_LIST)
BEAM_SRC = {9 + k: 0xE1 + k for k in range(5)}
BEAM_IDX = set(BEAM_SRC.values())
# estados de sala con el ojo (MorphBallEye, init $A8:9058) en su población: su haz de luz (md_cone.c) usa el color 15
# de la línea 3 (has_eye: se lee de la población de enemigos de cada estado en la ROM: hoy 9A90, 9E9F y 9F64)
EYE_INIT = (0xA8, 0x9058)


# paletas que escribe el inicio de un enemigo (p. ej. TourianEntranceStatue_Init, $AA:D7C8: las líneas 2 y 7 de los
# sprites con las estatuas de Ridley y la base): se leen del decomp (target_palettes[i + N] = g_word_XXXXXX[i] en un
# bucle de 16) y valen para los objetos de ese estado de sala
DECOMP = Path(__file__).resolve().parents[2] / "sm" / "src"
_init_pal_cache = {}


def init_palettes(rom, st):
    out = {}
    fnames = {}
    for m in re.finditer(r'#define fn(\w+) 0x([0-9A-Fa-f]{6})', (DECOMP / "funcs.h").read_text()):
        fnames[int(m.group(2), 16)] = m.group(1)
    p = 0xA10000 | rom.w(0x8F0000 | (st + 8))
    while rom.w(p) != 0xFFFF:
        e = 0xA00000 | rom.w(p)
        bank, init = rom.d[rom.off(e + 0x0C)], rom.w(e + 0x12)
        name = fnames.get((bank << 16) | init)
        if name:
            if name not in _init_pal_cache:
                res = {}
                f = DECOMP / ("sm_%02x.c" % bank)
                t = f.read_text(errors="replace") if f.exists() else ""
                m = re.search(r"^void %s\(void\) \{.*?^\}" % re.escape(name), t, re.M | re.S)
                if m:
                    for mm in re.finditer(r"target_palettes\[\w+ \+ (\d+)\] = g_word_([0-9A-F]{6})\[", m.group(0)):
                        base, src = int(mm.group(1)), int(mm.group(2), 16)
                        for c in range(16):
                            res[base + c] = rom.w(src + 2 * c)
                _init_pal_cache[name] = res
            out.update(_init_pal_cache[name])
        p += 16
    return out


def has_eye(rom, st):
    p = 0xA10000 | rom.w(0x8F0000 | (st + 8))
    while rom.w(p) != 0xFFFF:
        e = 0xA00000 | rom.w(p)
        if (rom.d[rom.off(e + 0x0C)], rom.w(e + 0x12)) == EYE_INIT:
            return True
        p += 16
    return False


CONE_SLOTS = (14, 15)  # modo sombra/luz (md_cone.c): en los sprites, el 14 de la línea 3 aclara y el 15 oscurece
# proyectiles de Samus (misiles, bombas, explosiones, estelas: mkmd.zebes_proj_keys) en cada sala, con los colores de
# la sala (su paleta 5 del SNES, la común, no tiene lugar fijo). Pesan poco al juntar colores: si no alcanzan los 10
# lugares, ceden ellos y no los enemigos.
SAMUS_PROJ_WEIGHT = 1 / 64


def read_logs(paths):
    """registros de dibujo -> {sala: set((banco, sm, pal, tiles))} (solo enemigos con spritemap normal)"""
    keys = {}
    for p in paths:
        for l in Path(p).read_text().splitlines():
            f = l.split()
            if len(f) == 6 and f[1] == "enemy":
                k = ("e", f[2], f[3], f[4], f[5])                 # banco, spritemap, paleta, tiles
            elif len(f) == 5 and f[1] == "sprobj":
                k = ("s", f[2], f[3], f[4])                       # objeto de sprite ($B4): spritemap, paleta, tiles
            elif len(f) == 4 and f[1] == "eproj":
                k = ("p", f[2], f[3])                             # proyectil enemigo: spritemap ($8D), tiles y paleta
            else:
                continue
            room = int(f[0], 16)
            if room >= 0xDF45 and room <= 0xE0B5 or room >= 0xC000 and room < 0xC100:
                continue                                          # Ceres y cinemáticas: mkmd.py
            keys.setdefault(room, set()).add(k)
    return keys


def enemy_vram(rom, tilesets_ptr):
    """VRAM de los sprites (bytes 0xC000-0xFFFF del SNES) y paletas de la sala (índice 128..255 -> BGR555)"""
    vram = bytearray(0x10000)
    pal = {}
    # sprites comunes (LoadStdBG3andSpriteTilesClearTilemaps $82:82E2: $9A:D200 -> VRAM $6000) y sus colores
    # (línea 0: $9A:FC00; línea 5: la paleta inicial $9A:8000): explosiones, humo, gotas...
    vram[0xC000:0xC000 + 0x2E00] = rom.bytes(0x9AD200, 0x2E00)
    for c in range(16):
        pal[128 + c] = rom.w(0x9AFC00 + 2 * c)
        pal[208 + c] = rom.w(0x9A8000 + 2 * (208 + c))
    # el rayo (UpdateBeamTilesAndPalette $90:AC8D): 8 tiles de kBeamTilePtrs ($90:C3B1, $9A) -> VRAM $6300 (tiles
    # 0x30-0x37, vacíos en los comunes) y sus colores, la línea 6 (kBeamPalettePtrs $90:C3C9); los del rayo normal:
    # los usan el rayo cargado, sus estelas y el destello de la carga
    vram[0xC600:0xC700] = rom.bytes(0x9A0000 | rom.w(0x90C3B1), 0x100)
    for c in range(16):
        pal[224 + c] = rom.w(0x900000 | (rom.w(0x90C3C9) + 2 * c))
    off = 2048
    e = 0xB40000 | tilesets_ptr
    while True:
        ed, dst = rom.w(e), rom.w(e + 2)
        if ed == 0xFFFF:
            break
        b = 0xA00000 | ed
        size = rom.w(b)
        pptr = rom.w(b + 2)
        bank = rom.d[rom.off(b + 0x0C)]
        tp = rom.w(b + 0x36) | rom.d[rom.off(b + 0x38)] << 16
        line = ((dst & 0xFF) + 8) * 16
        for c in range(16):
            pal[line + c] = rom.w((bank << 16) | (pptr + 2 * c))
        o = ((dst & 0x3000) >> 3) if size & 0x8000 else off
        n = size & 0x7FFF
        data = rom.bytes(tp, n)
        a = 0xD800 + o                     # 7E:7000 -> VRAM 0x6C00 (palabras) = 0xD800 (bytes)
        vram[a:a + n] = data[:max(0, min(n, 0x10000 - a))]
        if not size & 0x8000:
            off += size
        e += 4
    return bytes(vram), pal


COMPOSE_MIN = 6        # spritemaps de 6 piezas o más: se arman con sprites de hasta 32x32 (ver compose)


def cover_bands(img, band=32, alpha=0.1):
    """como mkintro.cover, pero ninguna caja cruza el borde de una franja de "band" líneas: así en cada línea de la
    pantalla los sprites de la imagen no se encimen (la Genesis dibuja hasta 256 puntos de sprites por línea en H32;
    lo que pasa de ahí no se ve: el hueco gris de la nave)"""
    M = 32
    Hh, Ww = img.shape
    pad = np.zeros((Hh + 2 * M, Ww + 2 * M), img.dtype)
    pad[M:M + Hh, M:M + Ww] = img
    todo = pad > 0
    out = []
    while todo.any():
        ys, xs = np.nonzero(todo)
        y, x = ys[0], xs[0]
        y = M + ((y - M) // 8) * 8               # cajas en la cuadrícula de 8 líneas
        end = ((y - M) // band + 1) * band + M
        best = None
        for w in range(1, 5):
            for h in range(1, 5):
                if y + h * 8 > end:
                    break
                for left in range(x - w * 8 + 1, x + 1):
                    n = int(todo[y:y + h * 8, left:left + w * 8].sum())
                    key = (n - alpha * w * h * 64, -w * h)
                    if best is None or key > best[0]:
                        best = (key, left, w, h)
        if best is None:                       # menos de 8 líneas hasta el borde: una caja de 1 tile de alto
            best = (None, x, 1, 1)
        _, left, w, h = best
        todo[y:y + h * 8, left:left + w * 8] = False
        out.append((left - M, y - M, w, h))
    return out, pad, M


def line_dots(recs):
    """puntos de sprites por línea de un spritemap (y, tamaño, atributos, x)"""
    dots = Counter()
    for (y, size, attr, x) in recs:
        w, h = ((size >> 10) & 3) + 1, ((size >> 8) & 3) + 1
        for yy in range(y, y + 8 * h):
            dots[yy] += 8 * w
    return max(dots.values()) if dots else 0


def compose(bank, pieces, maps):
    """Un spritemap grande con menos sprites: la Genesis muestra 64 sprites (el SNES, 128) y 16 por línea (el SNES,
    32). Las piezas se dibujan en una imagen por prioridad (las primeras del spritemap encima, como en el SNES) y la
    imagen se cubre con sprites de 1-4 x 1-4 tiles (cover). Si la imagen es simétrica (la nave), se cubre la mitad
    izquierda y la derecha es la misma con el giro horizontal: los mismos tiles."""
    from mkintro import cover
    xs, ys = [], []
    for (x, y, at, p, big, pxs) in pieces:
        dx = x & 0x1FF
        dx = dx - 0x200 if dx >= 0x100 else dx
        n = 16 if big else 8
        xs += [dx, dx + n]
        ys += [y, y + n]
    ox, oy = min(xs), min(ys)
    W, H = max(xs) - ox, max(ys) - oy
    layers = {}
    for (x, y, at, p, big, pxs), mp in reversed(list(zip(pieces, maps))):
        dx = x & 0x1FF
        dx = dx - 0x200 if dx >= 0x100 else dx
        n = 2 if big else 1
        blk = np.zeros((8 * n, 8 * n), np.int32)
        for i, m in enumerate(mp):
            cx, cy = i // n, i % n
            blk[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] = m
        if (at >> 14) & 1:
            blk = blk[:, ::-1]
        if (at >> 15) & 1:
            blk = blk[::-1, :]
        prio = 1 if ((at >> 12) & 3) == 3 or os.environ.get("MM_ZOBJ_ALLHI") else 0
        img = layers.setdefault(prio, np.zeros((H, W), np.int32))
        sub = img[y - oy:y - oy + 8 * n, dx - ox:dx - ox + 8 * n]
        np.copyto(sub, blk, where=blk > 0)
    recs = []
    above = np.zeros((H, W), bool)               # píxeles ya tapados por una capa de más prioridad (va antes: encima)
    for prio in sorted(layers, reverse=True):
        img = np.where(above, 0, layers[prio])
        above |= layers[prio] > 0
        if not img.any():
            continue
        cols = np.nonzero(img.any(axis=0))[0]
        x0, x1 = int(cols[0]), int(cols[-1]) + 1
        img = img[:, x0:x1]
        w = x1 - x0
        sym = w % 2 == 0 and np.array_equal(img, img[:, ::-1])
        half = img[:, :w // 2] if sym else img
        boxes, pad, M = cover_bands(half)
        import os as _os
        if _os.environ.get('MM_COMPOSE_DBG'):
            print('    capa', prio, 'img', img.shape, 'sym', sym, 'cajas', [(bx, by, bw, bh) for bx, by, bw, bh in boxes], 'oy', oy, file=sys.stderr)
        for bx, by, bw, bh in boxes:
            run = [pad[by + M + cy * 8:by + M + cy * 8 + 8, bx + M + cx * 8:bx + M + cx * 8 + 8]
                   for cx in range(bw) for cy in range(bh)]
            mir = [pad[by + M + cy * 8:by + M + cy * 8 + 8, bx + M + cx * 8:bx + M + cx * 8 + 8][:, ::-1]
                   for cx in reversed(range(bw)) for cy in range(bh)]
            jm = b"".join(genesis_tile_bytes(t) for t in mir)
            if jm in bank.index:
                tile, hf = bank.index[jm], 1
            else:
                tile, hf = bank.add_exact(run), 0
            size = ((bw - 1) << 2 | (bh - 1)) << 8
            base = (prio << 15) | (OBJ_LINE << 13) | tile
            recs.append((by + oy, size, base | hf << 11, bx + ox + x0))
            if sym:                                            # la mitad derecha: la misma caja girada
                mx = w - (bx + bw * 8)
                recs.append((by + oy, size, base | (hf ^ 1) << 11, mx + ox + x0))
    return recs


def zkey(k):
    """clave de la Genesis (md_zebes.c, zebes_obj_find): enemigo (spritemap, paleta, tiles); proyectil
    (spritemap, tiles y paleta, 0x8000)"""
    if k[0] == "e":
        return (int(k[2], 16), int(k[3], 16), int(k[4], 16))
    if k[0] == "s":                                       # objetos de sprite: tiles | 0x4000
        return (int(k[1], 16), int(k[2], 16), int(k[3], 16) | 0x4000)
    if k[0] == "j":                                       # proyectil de Samus: (spritemap $93, 0, 0xC000)
        return (int(k[1], 16), 0, 0xC000)
    if k[0] == "t":                                       # estela del misil: (tile y atributo, 0, 0xC001)
        return (int(k[1], 16), 0, 0xC001)
    return (int(k[1], 16), int(k[2], 16), 0x8000)


def convert_state(rom, keys, tilesets_ptr, beam_slots, compose_min=None, slots=OBJ_SLOT_LIST, extra_pal=None, base=ZOBJ_BASE):
    compose_min = compose_min or COMPOSE_MIN
    vram, pal = enemy_vram(rom, tilesets_ptr)
    if extra_pal:
        pal.update(extra_pal)
    bank = mkmd.TileBank(base)
    # piezas de cada clave y colores que usan
    pieces = {}
    counts = Counter()
    for k in sorted(keys):
        key = {"e": ("enemy",), "s": ("enemy", "B4"), "p": ("eproj",), "j": ("proj",), "t": ("trail",)}[k[0]] + k[1:]
        kp = mkmd.key_pieces(rom, key)
        wgt = SAMUS_PROJ_WEIGHT if k[0] in "jt" else 1
        out = []
        for (x, y, at) in kp:
            T, p, big = at & 0x1FF, (at >> 9) & 7, bool(x & 0x8000)
            n = 2 if big else 1
            pxs = []
            for dx in range(n):
                for dy in range(n):
                    px = mkmd.obj_tile_px(vram, T, dx, dy)
                    pxs.append(px)
                    for c in np.unique(px):
                        if c and 128 + p * 16 + int(c) not in BEAM_IDX:      # el rayo: lugares fijos 9-13
                            counts[128 + p * 16 + int(c)] += int((px == c).sum()) * wgt
            out.append((x, y, at, p, big, pxs))
        pieces[k] = out
    if not counts:
        return None
    # colores: por valor; si son más de OBJ_SLOTS, se juntan los más parecidos (pesando por píxeles)
    gen = lambda i: mkmd.bgr_to_gen(pal.get(i, 0))
    rep = {}
    groups = {}
    for i in sorted(counts, key=lambda i: -counts[i]):
        g = gen(i)
        if g in groups:
            rep[i] = groups[g]
        else:
            groups[g] = i
            rep[i] = i
    # (los colores del rayo, 9-13, no se comparten: cambian con el tipo de rayo)
    alive = {i: counts[i] for i in groups.values()}
    for i, r in rep.items():
        if r != i:
            alive[r] += counts[i]
    merged = 0
    while len(alive) > len(slots):
        best = None
        al = sorted(alive)
        for a_ in range(len(al)):
            for b_ in range(a_ + 1, len(al)):
                d = color_dist(gen(al[a_]), gen(al[b_]))
                w = d * min(alive[al[a_]], alive[al[b_]])
                if best is None or w < best[0]:
                    best = (w, al[a_], al[b_])
        _, ia, ib = best
        keep, drop = (ia, ib) if alive[ia] >= alive[ib] else (ib, ia)
        alive[keep] += alive.pop(drop)
        for i in rep:
            if rep[i] == drop:
                rep[i] = keep
        merged += 1
    order = sorted(alive, key=lambda i: -alive[i])
    slot = {i: slots[s] for s, i in enumerate(order)}
    for sl, i in BEAM_SRC.items():                        # colores del rayo (explosión del rayo): 9-13
        slot[i] = sl
        rep[i] = i
    src = [0xFFFF] * 15                                   # índice de palette_buffer de los colores 1-15
    for i, sl in slot.items():
        src[sl - 1] = i
    for sl, i in BEAM_SRC.items():
        src[sl - 1] = i
    # registros
    recs_all = []
    index = []
    nspr = [0, 0]
    for k in sorted(pieces):
        recs = []
        maps = [[np.vectorize(lambda c: slot[rep[128 + p * 16 + c]] if c else 0)(px).astype(np.int32) for px in pxs]
                for (x, y, at, p, big, pxs) in pieces[k]]
        nspr[0] += len(pieces[k])
        if len(pieces[k]) >= compose_min:
            recs = compose(bank, pieces[k], maps)
            print('  %s: %d piezas -> %d sprites, hasta %d puntos por línea' % (k[1:], len(pieces[k]), len(recs), line_dots(recs)), file=sys.stderr)
            nspr[1] += len(recs)
            off = len(recs_all)
            recs_all += list(struct.unpack(">5h", mkmd.rec_header(recs)))
            for r in recs:
                recs_all += list(r)
            index.append(zkey(k) + (off,))
            continue
        nspr[1] += len(pieces[k])
        for (x, y, at, p, big, pxs) in pieces[k]:
            mp = [np.vectorize(lambda c: slot[rep[128 + p * 16 + c]] if c else 0)(px).astype(np.int32) for px in pxs]
            if not any(m.any() for m in mp):
                continue
            tile = bank.add_exact(mp)
            prio = 1 if ((at >> 12) & 3) == 3 or os.environ.get("MM_ZOBJ_ALLHI") else 0
            attr = (prio << 15) | (OBJ_LINE << 13) | (((at >> 15) & 1) << 12) | (((at >> 14) & 1) << 11) | tile
            dx = x & 0x1FF
            if dx >= 0x100:
                dx -= 0x200
            recs.append((y, 0x0500 if big else 0, attr, dx))
        off = len(recs_all)
        recs_all += list(struct.unpack(">5h", mkmd.rec_header(recs)))
        for r in recs:
            recs_all += list(r)
        index.append(zkey(k) + (off,))
    tiles = bank.data()
    if len(tiles) // 32 > 1280 - base:
        print(f"  objetos {tilesets_ptr:04X}: {len(tiles) // 32} tiles, más de {1280 - base}: no entran", file=sys.stderr)
        return None
    return dict(base=base, tiles=tiles, src=src, keys=sorted(index), recs=[v & 0xFFFF for v in recs_all], merged=merged,
                ncolors=len(counts), nspr=nspr)


def build(rom_path, log_paths, states):
    """states: [(estado, sala, tilesets_ptr)] -> {estado: datos o None}"""
    rom = mkmd.Rom(rom_path)
    keys = read_logs(log_paths)
    psms, ptrail = mkmd.zebes_proj_keys(rom)
    pkeys = {("j", "%04X" % sm) for sm in psms} | {("t", "%04X" % t) for t in ptrail}
    out, cache = {}, {}
    for st, room, ts, room_tiles in states:
        k = keys.get(room, set()) | pkeys
        eye = has_eye(rom, st)
        ipal = init_palettes(rom, st)
        low = min(ZOBJ_BASE, 448 + ((room_tiles + 7) & ~7))   # justo después de los tiles de la sala (md_zebes.c)
        ck = (frozenset(k), ts, eye, tuple(sorted(ipal.items())), low)   # salas con lo mismo comparten los datos
        if ck not in cache:
            sl = [x for x in OBJ_SLOT_LIST if x not in CONE_SLOTS] if eye else OBJ_SLOT_LIST
            cache[ck] = convert_state(rom, k, ts, None, slots=sl, extra_pal=ipal)
            if cache[ck] is None and low < ZOBJ_BASE:   # más lugar: desde el final de los tiles de la sala
                cache[ck] = convert_state(rom, k, ts, None, slots=sl, extra_pal=ipal, base=low)
            if cache[ck] is None:                 # no entran los tiles: sin juntar piezas (comparten los tiles del SNES)
                cache[ck] = convert_state(rom, k, ts, None, 1 << 30, slots=sl, extra_pal=ipal, base=low)
                if cache[ck] is None:
                    print(f"  ¡sala {room:04X} (tiles {ts:04X}): sus objetos no entran ni sin juntar piezas!", file=sys.stderr)
            d = cache[ck]
            if d:
                print(f"objetos sala {room:04X}: {len(d['keys'])} spritemaps, {len(d['tiles']) // 32} tiles, "
                      f"sprites {d['nspr'][0]} -> {d['nspr'][1]}, "
                      f"{d['ncolors']} colores ({d['merged']} fusiones)", file=sys.stderr)
        if cache[ck]:
            out[st] = cache[ck]
    return out
