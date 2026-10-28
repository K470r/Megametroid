#!/usr/bin/env python3
"""MegaMetroid — datos de la Genesis para el motor nativo (docs/32).

Todo lo que la Genesis dibuja se prepara aquí, en el PC, ya en el formato del VDP. En la consola no queda
ninguna conversión del SNES: copia tiles y mapas por DMA, lee tablas y escribe sprites.

Entradas
  rom.smc                 la ROM del usuario (no se distribuye)
  data/extracted_full     extracción verificada (docs/26): level data, tablas de tiles, tilesets
  vram_dir/<sala>.bin     VRAM (64 KB) + CGRAM (512 B) del SNES dentro de cada sala (hlref HL_VRAM):
                          tiles de fondo y de sprites ya cargados y la capa 2 de biblioteca, tal cual el juego
  res/gen/engine              tablas del motor (para saber qué dibuja)
  draw.log, hud.log       lo que el motor dibujó en la grabación de Ceres (MM_DRAWLOG / MM_HUDLOG): se suma a
                          lo que se enumera desde las tablas, para no dejar nada fuera

Salida (res/gen/md)
  md_data.h / md_data.c   tablas (big-endian, alineadas)
  md_bins.s + bins/       bloques grandes (tiles, planos) con .incbin

Formato de la Genesis (VRAM, en tiles de 32 bytes):
  0             tile vacío
  HUD_BASE..    tiles del HUD (fijos)
  SAMUS_BASE    32 tiles de Samus: columna c de la fila r del SNES en SAMUS_BASE + 2c + r (así un sprite de 16x16
                del SNES es un sprite de 2x2 de la Genesis)
  BG_BASE..     tiles de la sala; detrás, los de sus objetos (puertas, rayo, cápsula, plataforma)
  planos A y B  imagen completa de la sala (o de la capa 2 de biblioteca), cargada entera al entrar
Paletas: PAL0-PAL1 fondo, PAL2 Samus, PAL3 HUD y demás sprites (o PAL1 si el fondo usa una sola paleta).
Cada color de la CRAM sale de un índice de la paleta del SNES (palette_buffer): la tabla `src` de cada sala dice
cuál; la Genesis toma el color con kCram555 (tabla de 32768 colores BGR555 -> CRAM), así los fundidos del motor
se ven igual.
"""
import json
import os
import struct
import sys
from collections import Counter, OrderedDict
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent / "tools" / "genesis_convert"))
from convert_rooms import snes_to_genesis, gen_to_cram, gen_to_rgb, color_dist, genesis_tile_bytes  # noqa: E402

HUD_BASE = 1
SAMUS_BASE = 224
BG_BASE = 256
CINE_BG_BASE = 120                # cinemáticas y menús: 1-119 los tiles del círculo de la explosión (docs/35)
TILE_LIMIT = 1280                 # 0x0000-0x9FFF: tiles; planos desde 0xA000
SAMUS_PAL = 2

ROOMS = OrderedDict([             # id -> (sala, estado, carpeta extraída); id = sala, o el estado en la huida
    (0xDF45, (0xDF45, 0xDF57, "6_DF45_Ceres_Shaft")),
    (0xDF8D, (0xDF8D, 0xDF9F, "6_DF8D_Ceres_Falling_Tile_Room")),
    (0xDFD7, (0xDFD7, 0xDFE9, "6_DFD7_Ceres_Stairs_Room")),
    (0xE021, (0xE021, 0xE033, "6_E021_Ceres_Computer_Room")),
    (0xE06B, (0xE06B, 0xE07D, "6_E06B_Ceres_58_Escape")),
    (0xE0B5, (0xE0B5, 0xE0C7, "6_E0B5_Ceres_Ridley_Boss_Room")),
    # huida (ceres_status 2): otro estado de cada sala, con su VRAM capturada en la huida (<sala>_<estado>.bin)
    (0xE097, (0xE06B, 0xE097, "6_E06B_Ceres_58_Escape")),
    (0xE04D, (0xE021, 0xE04D, "6_E021_Ceres_Computer_Room")),
    (0xE003, (0xDFD7, 0xE003, "6_DFD7_Ceres_Stairs_Room")),
    (0xDFB9, (0xDF8D, 0xDFB9, "6_DF8D_Ceres_Falling_Tile_Room")),
    (0xDF71, (0xDF45, 0xDF71, "6_DF45_Ceres_Shaft")),
])
ESCAPE_STATES = {0xE097, 0xE04D, 0xE003, 0xDFB9, 0xDF71}


def room_file(rid):
    room, state, _ = ROOMS[rid]
    return f"{room:04X}" if rid == room else f"{room:04X}_{state:04X}"


# ------------------------------------------------------------------ ROM
class Rom:
    def __init__(self, path):
        d = Path(path).read_bytes()
        if len(d) % 0x8000:
            d = d[len(d) % 0x8000:]
        self.d = d

    def off(self, a):
        return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)

    def w(self, a):
        return struct.unpack_from("<H", self.d, self.off(a))[0]

    def b(self, a):
        return self.d[self.off(a)]

    def bytes(self, a, n):
        o = self.off(a)
        return self.d[o:o + n]


def snes_tile4(b):
    """32 bytes 4bpp planar del SNES -> 8x8 índices 0-15"""
    a = np.frombuffer(bytes(b), np.uint8)
    bits = np.unpackbits(a.reshape(2, 8, 2), axis=2, bitorder="big").reshape(2, 8, 2, 8)
    return (bits[0, :, 0] | bits[0, :, 1] << 1 | bits[1, :, 0] << 2 | bits[1, :, 1] << 3).astype(np.int32)


def snes_tile2(b):
    """16 bytes 2bpp -> 8x8 índices 0-3"""
    a = np.frombuffer(bytes(b), np.uint8).reshape(8, 2)
    lo = np.unpackbits(a[:, 0:1], axis=1, bitorder="big")
    hi = np.unpackbits(a[:, 1:2], axis=1, bitorder="big")
    return (lo | hi << 1).astype(np.int32)


def bgr_to_gen(c):
    return snes_to_genesis(c, "dac")


# ------------------------------------------------------------------ paletas por índice del SNES
class PalPacker:
    """Agrupa celdas (conjuntos de índices de CGRAM) en líneas de 15 colores. Índices con el mismo color se
    comparten; si no cabe, se funden los índices de colores más parecidos (el más usado se queda)."""

    def __init__(self, cgram):
        self.cg = cgram

    def pack(self, cellsets, nlines, counts):
        # índices con igual color -> uno solo (el más usado)
        alias = {}
        bycol = {}
        for i in sorted(counts, key=lambda i: -counts[i]):
            c = self.cg[i]
            if c in bycol:
                alias[i] = bycol[c]
            else:
                bycol[c] = i
        while True:
            sets = {}
            for k, s in cellsets.items():
                fs = frozenset(self._res(alias, i) for i in s)
                sets.setdefault(fs, []).append(k)
            bins = self._bins(list(sets.items()), nlines)
            if bins is not None:
                return bins, alias
            # fundir el par de índices más cercano (en colores de la Genesis)
            alive = sorted({self._res(alias, i) for s in cellsets.values() for i in s})
            best = None
            for a in range(len(alive)):
                for b in range(a + 1, len(alive)):
                    ia, ib = alive[a], alive[b]
                    d = color_dist(bgr_to_gen(self.cg[ia]), bgr_to_gen(self.cg[ib]))
                    if best is None or d < best[0]:
                        best = (d, ia, ib)
            _, ia, ib = best
            keep, drop = (ia, ib) if counts.get(ia, 0) >= counts.get(ib, 0) else (ib, ia)
            alias[drop] = keep

    @staticmethod
    def _res(alias, i):
        while i in alias:
            i = alias[i]
        return i

    @staticmethod
    def _bins(groups, n, cap=15):
        groups = sorted(groups, key=lambda g: -len(g[0]))
        bins = [set() for _ in range(n)]
        keys = [[] for _ in range(n)]
        for fs, ks in groups:
            # la línea donde agrega menos índices nuevos y cabe
            best = None
            for bi in range(n):
                u = bins[bi] | fs
                if len(u) <= cap:
                    sc = (len(u) - len(bins[bi]), len(u))
                    if best is None or sc < best[0]:
                        best = (sc, bi)
            if best is None:
                return None
            bins[best[1]] |= fs
            keys[best[1]] += ks
        return list(zip(bins, keys))


# ------------------------------------------------------------------ tiles Genesis
class TileBank:
    def __init__(self, base):
        self.base = base
        self.tiles = []
        self.index = {}

    def add(self, px, flips=True):
        """px: 8x8 índices de la línea (0-15). Devuelve (tile absoluto, hf, vf); 0 si vacío."""
        if not px.any():
            return 0, 0, 0
        vs = [(px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1, :], 0, 1), (px[::-1, ::-1], 1, 1)] if flips else [(px, 0, 0)]
        for v, hf, vf in vs:
            k = genesis_tile_bytes(v)
            if k in self.index:
                return self.index[k], hf, vf
        k = genesis_tile_bytes(px)
        self.index[k] = self.base + len(self.tiles)
        self.tiles.append(k)
        return self.index[k], 0, 0

    def add_exact(self, pxs):
        """varios tiles seguidos (un sprite de varias celdas): se guardan juntos, sin flips por tile"""
        keys = [genesis_tile_bytes(p) for p in pxs]
        joined = b"".join(keys)
        if joined in self.index:
            return self.index[joined]
        t = self.base + len(self.tiles)
        self.tiles += keys
        self.index[joined] = t
        return t

    def data(self):
        return b"".join(self.tiles)


# ------------------------------------------------------------------ fuentes de celdas de fondo
def cells_from_words(words, W, H, tile_px):
    """words: palabras de tilemap del SNES (W*H). Devuelve lista HxW de (idx 8x8 CGRAM, pri) o None si vacío."""
    out = []
    for y in range(H):
        row = []
        for x in range(W):
            e = words[y * W + x]
            px = tile_px(e & 0x3FF)
            if e & 0x4000:
                px = px[:, ::-1]
            if e & 0x8000:
                px = px[::-1, :]
            pal = (e >> 10) & 7
            idx = np.where(px > 0, pal * 16 + px, 0)
            row.append((idx, (e >> 13) & 1) if idx.any() else None)
        out.append(row)
    return out


def level_words(level, wb, hb, table):
    """bloques 16x16 (level data) -> palabras de tilemap 8x8 como BuildBgColumn (sm_80.c)"""
    W, H = wb * 2, hb * 2
    out = [0] * (W * H)
    for by in range(hb):
        for bx in range(wb):
            w = level[by * wb + bx]
            mt = w & 0x3FF
            q = list(struct.unpack_from("<4H", table, mt * 8))
            g = [[q[0], q[1]], [q[2], q[3]]]
            if w & 0x400:
                g = [[r[1] ^ 0x4000, r[0] ^ 0x4000] for r in g]
            if w & 0x800:
                g = [[c ^ 0x8000 for c in g[1]], [c ^ 0x8000 for c in g[0]]]
            for yy in range(2):
                for xx in range(2):
                    out[(by * 2 + yy) * W + bx * 2 + xx] = g[yy][xx]
    return out


def mode7_cells(vram_m7, x0, y0, W, H):
    """Ceres Shaft: plano de modo 7 (byte bajo = mapa 128x128, byte alto = píxeles 8bpp). Índice = CGRAM."""
    mp, px = vram_m7[0::2], vram_m7[1::2]
    out = []
    for cy in range(H):
        row = []
        for cx in range(W):
            t = mp[(((y0 >> 3) + cy) & 127) * 128 + (((x0 >> 3) + cx) & 127)]
            idx = np.frombuffer(bytes(px[t * 64:t * 64 + 64]), np.uint8).reshape(8, 8).astype(np.int32)
            row.append((idx, 0) if idx.any() else None)
        out.append(row)
    return out


def apply_mode7_queue(rom, vram_m7, bank, ptr):
    """QueueMode7Transfers + NMI_ProcessMode7QueueInner (sm_80.c) sobre una copia de la VRAM de modo 7"""
    v = bytearray(vram_m7)
    p = (bank << 16) | ptr
    while True:
        f = rom.b(p)
        if f & 0x80:
            src = rom.w(p + 1) | rom.b(p + 3) << 16
            cnt = rom.w(p + 4)
            vaddr = rom.w(p + 6)
            vmain = rom.b(p + 8)
            hi = bool(f & 0x40)
            data = rom.bytes(src, cnt)
            step = 128 if (vmain & 3) == 3 else (32 if (vmain & 3) == 1 else 1)
            for i, bt in enumerate(data):
                a = (vaddr + i * step) & 0x7FFF if hi or True else 0
                v[2 * a + (1 if hi else 0)] = bt
            p += 9
        elif f & 0x40:
            p += 7          # escritura a CGRAM: la hace el motor en palette_buffer
        else:
            break
    return bytes(v)


# ------------------------------------------------------------------ main
def proj_list_sms(rom, ip):
    """spritemaps de una lista de instrucciones de proyectil de Samus ($93)"""
    out, seen = [], set()
    while ip not in seen and len(seen) < 64:
        seen.add(ip)
        t = rom.w(0x930000 | ip)
        if t == 0x822F:                     # Proj93Instr_Delete
            break
        if t == 0x8239:                     # Proj93Instr_Goto
            ip = rom.w(0x930000 | (ip + 2))
            continue
        if t & 0x8000:
            break
        out.append(rom.w(0x930000 | (ip + 2)))
        ip += 8
    return out


def msgbox_words(rom):
    """palabras de los mapas de las cajas de mensaje (sm_85.c): los mensajes ($85:877F-95BF), los bordes
    ($85:825A, $85:8289), las letras de los botones ($85:83E2 kTileNumbersForButtonLetters) y la confirmación de
    guardar ($85:8507)"""
    ws = {rom.w(0x850000 | a) for a in range(0x877F, 0x95BF, 2)}
    ws |= {0xe, 0x284e, 0x280f}                                      # bordes (kLarge/kSmallMsgBoxTopBottomBorderTilemap)
    ws |= {0x28e0, 0x3ce1, 0x2cf7, 0x38f8, 0x38d0, 0x38eb, 0x38f1, 0x284e}   # kTileNumbersForButtonLetters
    ws |= {0x3c4e, 0x38cc, 0x38cd, 0x3cf8, 0x3ce4, 0x3cf2, 0x2ced, 0x2cee, 0x2cf8, 0x2ce4, 0x2cf2, 0x3ced, 0x3cee}  # kSaveConfirmation...
    return ws


def zebes_proj_keys(rom):
    """proyectiles de Samus que no son el rayo, desde la ROM: (spritemaps de $93, tiles de las estelas).
    - $93:83F1 (cada entrada: daño + listas por dirección): misiles (1) y bombas (5; HudSelectionHandler_MorphBall
      usa la primera lista, InitializeInstrForMissile; las bombas en abanico, BombSpread, también); los supermisiles
      (2) y el rayo cargado normal ($93:83D9, índice 0); las bombas de poder (3) se agregan cuando se porten (docs/46)
    - explosiones: la del misil, KillProjectileInner ($93:80CF) toma la lista de $93:867F ($A039); la de la bomba,
      InitializeBombExplosion ($93:814E): $93:8683 ($A06B); la del rayo ($A007) va con el rayo; la del supermisil
      ($93:8693), con él
    - estelas de los misiles (HandleProjectileTrails $90:B657): un sprite de 8x8 por cuadro con el tile y atributo de
      las listas $90:B5BB (izquierda) y $90:B609 (derecha): (tiempo, tile) hasta el tiempo 0"""
    def sm_ok(sm):                          # spritemap creíble: entre 1 y 32 piezas
        return 0x8000 <= sm and 1 <= rom.w(0x930000 | sm) <= 32
    sms = set()
    # (índice, listas): los misiles tienen una por dirección (InitializeProjectile: 10); la bomba una sola
    # supermisiles (2): su lista por dirección ($93:83F1) y la del acompañante de Missile_Func2
    # (InitializeInstrForSuperMissile $93:8071, kRunInstrForSuperMissile $93:842B: una sola lista)
    # rayo cargado (FireChargedBeam $90:B986, kProjectileData_ChargedBeams $93:83D9): por ahora solo el del rayo
    # normal (índice 0; onda, hielo, spazer y plasma se agregan cuando se porten)
    for tab, idx in ((0x83F1, ((1, 10), (2, 10), (5, 1))), (0x842B, ((2, 1),)), (0x83D9, ((0, 10),))):
        for i, ndir in idx:
            v3 = rom.w(0x930000 | (tab + 2 * i))
            if v3 < 0x8000:
                continue
            for d in range(ndir):
                ip = rom.w(0x930000 | (v3 + 2 + 2 * d))
                if ip >= 0x8000:
                    sms |= {sm for sm in proj_list_sms(rom, ip) if sm_ok(sm)}
    for ip in (rom.w(0x93867F), rom.w(0x938683), rom.w(0x938693)):   # + explosión del supermisil ($93:80CF)
        sms |= {sm for sm in proj_list_sms(rom, ip) if sm_ok(sm)}
    # destello de la carga (DrawFlareAnimationComponent $90:BBE1 -> DrawBeamGrappleSpritemap $81:8A37): los 54
    # spritemaps de g_off_93A1A1 (destello 0-29, chispas lentas 30-35/42-47, rápidas 36-41/48-53)
    sms |= {rom.w(0x93A1A1 + 2 * i) for i in range(54)}
    trail = set()
    for tab in (0x90B5BB, 0x90B609):
        for i in range(39):
            ip = rom.w(tab + 2 * i)
            for _ in range(64):
                t = rom.w(0x900000 | ip)
                if t == 0:                                   # tiempo 0: la estela termina
                    break
                if t >= 0x8000:
                    if t in (0xB525, 0xB587, 0xB5B3):          # mover la estela: sin argumentos
                        ip += 2
                        continue
                    break
                trail.add(rom.w(0x900000 | (ip + 2)))
                ip += 4
    return sms, trail


def main():
    rom_path, ext, vram_dir, gen_dir, out_dir, draw_log, hud_log = sys.argv[1:8]
    rom = Rom(rom_path)
    ext, vram_dir, out_dir = Path(ext), Path(vram_dir), Path(out_dir)
    (out_dir / "bins").mkdir(parents=True, exist_ok=True)
    bins = []          # (símbolo, archivo)
    C = []             # líneas de md_data.c
    Hh = []            # md_data.h

    def binfile(sym, data):
        (out_dir / "bins" / f"{sym}.bin").write_bytes(data)
        bins.append((sym, f"{sym}.bin"))

    def carray(t, name, vals, per=16, fmt="0x%04X"):
        C.append(f"const {t} {name}[{len(vals)}] = {{")
        for i in range(0, len(vals), per):
            C.append("  " + ", ".join(fmt % v for v in vals[i:i + per]) + ",")
        C.append("};")

    # ---------- 1) BGR555 -> CRAM
    lut = [gen_to_cram(bgr_to_gen(c)) for c in range(32768)]
    binfile("kCram555", struct.pack(">32768H", *lut))
    Hh.append("extern const uint16 kCram555[32768];   /* color BGR555 del SNES -> CRAM (nivel del DAC más cercano) */")

    # haze de Ceres (color math del SNES: azul sumado por banda de líneas): azul de 5 bits + nivel -> azul de la Genesis
    from convert_rooms import quant_channel
    q = [quant_channel(min(31, v), "dac") for v in range(48)]
    carray("uint8", "kHazeQ", q, fmt="%u")
    chg = []
    for b in range(32):
        m = 0
        for L in range(1, 16):
            if q[b + L] != q[b + L - 1]:
                m |= 1 << L
        chg.append(m)
    carray("uint16", "kHazeChg", chg)
    Hh.append("extern const uint8 kHazeQ[48];     /* azul de 5 bits (con el nivel sumado) -> azul de 3 bits de la CRAM */")
    Hh.append("extern const uint16 kHazeChg[32];  /* por azul de 5 bits: bit L = el azul de la Genesis cambia al pasar al nivel L */")

    # ---------- registros de lo que dibuja el motor
    draws = Counter()
    for l in Path(draw_log).read_text().splitlines():
        draws[tuple(l.split()[:6])] += 1
    hud_words = {int(x, 16) for x in Path(hud_log).read_text().split()}
    # texto de la huida (EscapeTextCell): palabras del BG3 como las del HUD (misma letra, $9A:B200)
    global ESC_VRAM
    ESC_VRAM = (Path(vram_dir) / "E0B5_ESC.bin").read_bytes()[:0x10000]

    # ---------- 2) Samus: tiles por definición ($92) y spritemaps
    T = Path(gen_dir) / "tables.c"
    tables = T.read_text()

    def table_vals(name):
        import re
        m = re.search(r"\b%s\[\d+\] = \{(.*?)\};" % name, tables, re.S)
        return [int(x, 0) for x in m.group(1).replace("\n", " ").split(",") if x.strip()]
    animdefs = table_vals("kBank92_AnimDefs")
    top_lists = table_vals("kSamus_TileDefs_TopHalf")
    bot_lists = table_vals("kSamus_TileDefs_BottomHalf")
    tiledefs = set()
    for i in range(0, len(animdefs) - 3, 4):
        a0, a1, a2, a3 = animdefs[i:i + 4]
        if a0 < len(top_lists):
            tiledefs.add((0, top_lists[a0] + 7 * a1))
        if a2 != 255 and a2 < len(bot_lists):
            tiledefs.add((1, bot_lists[a2] + 7 * a3))
    tiledefs = sorted(tiledefs, key=lambda t: t[1])
    samus_blob = bytearray()
    td_index = []           # (dirección, desplazamiento en bloques de 32 bytes, tiles)
    seen_td = {}
    for half, addr in tiledefs:
        if addr in seen_td:
            continue
        e = rom.bytes(0x920000 | addr, 7)
        src = e[0] | e[1] << 8 | e[2] << 16
        p1 = e[3] | e[4] << 8
        p2 = e[5] | e[6] << 8
        if not (0x9B <= (src >> 16) <= 0x9F) or p1 > 0x200 or p2 > 0x200:
            continue
        r0 = rom.bytes(src, p1)
        r1 = rom.bytes(src + p1, p2)
        n0, n1 = p1 // 32, p2 // 32
        ncol = max(n0, n1)
        out = bytearray()
        for c in range(ncol):
            for r, n, data in ((0, n0, r0), (1, n1, r1)):
                if c < n:
                    out += genesis_tile_bytes(snes_tile4(data[c * 32:c * 32 + 32]))
                else:
                    out += bytes(32)
        seen_td[addr] = (len(samus_blob) // 32, ncol * 2)
        samus_blob += out
    binfile("kSamusTileBlob", bytes(samus_blob))
    # cañón del brazo abierto ($90:C663): un tile de 8x8 que el juego copia de $9A según dirección y cuadro
    ac_src = sorted({rom.w(0x900000 | (rom.w(0x90C7A5 + 2 * i) + 2 * f)) for i in range(10) for f in range(1, 4)})
    ac = bytearray()
    for a in ac_src:
        ac += genesis_tile_bytes(snes_tile4(rom.bytes(0x9A0000 | a, 32)))
    binfile("kArmCannonTiles", bytes(ac))
    carray("uint16", "kArmCannonSrc", ac_src)
    Hh.append(f"#define ARM_CANNON_TILES {len(ac_src)}")
    Hh.append("extern const uint16 kArmCannonSrc[ARM_CANNON_TILES];   /* origen en $9A de cada tile del cañón */")
    Hh.append("extern const uint8 kArmCannonTiles[];                  /* los mismos tiles, ya de la Genesis */")
    keys = sorted(seen_td)
    C.append("/* definiciones de tiles de Samus ($92): dirección, primer tile en kSamusTileBlob, cantidad de tiles\n"
             "   (ya intercalados: columna c, fila r -> 2c + r) */")
    C.append(f"const SamusTileDef kSamusTileDefs[{len(keys)}] = {{")
    for k in keys:
        C.append("  { 0x%04X, %u, %u }," % (k, seen_td[k][0], seen_td[k][1]))
    C.append("};")
    Hh.append("typedef struct { uint16 addr, first, count; } SamusTileDef;")
    Hh.append(f"#define SAMUS_TILEDEF_COUNT {len(keys)}")
    Hh.append("extern const SamusTileDef kSamusTileDefs[SAMUS_TILEDEF_COUNT];")
    Hh.append("extern const uint8 kSamusTileBlob[];")
    print(f"samus: {len(keys)} definiciones de tiles, {len(samus_blob)} bytes")

    # spritemaps de Samus: tabla $92:808D
    def conv_piece(x, y, t, tile_of, pal_line):
        """pieza de spritemap del SNES -> registro de sprite de la Genesis (dy, tamaño, atributo, dx)"""
        big = bool(x & 0x8000)
        dx = x & 0x1FF
        if dx >= 0x100:
            dx -= 0x200
        tile = tile_of(t & 0x1FF, big)
        if tile is None:
            return None
        attr = ((1 if (t >> 12) & 3 == 3 else 0) << 15) | (pal_line << 13) | (((t >> 15) & 1) << 12) | \
            (((t >> 14) & 1) << 11) | tile
        size = 0x0500 if big else 0x0000
        return (y, size, attr, dx)

    def samus_tile(t, big):
        r, c = (t >> 4) & 0x1F, t & 15
        if t >= 0x20 or (big and r != 0) or (big and c == 15):
            return None
        return SAMUS_BASE + 2 * c + r

    sm_recs = bytearray()
    sm_index = []
    bad = Counter()
    n_sm = (0x9263 - 0x808D) // 2
    for i in range(n_sm):
        p = rom.w(0x92808D + 2 * i)
        k = rom.w(0x920000 | p) if p else 0
        if not p or k > 32:
            sm_index.append(0xFFFF)
            continue
        recs = []
        for j in range(k):
            x, y, t = struct.unpack_from("<HbH", rom.d, rom.off(0x920000 | p) + 2 + 5 * j)
            if (t >> 9) & 7 != 4:
                bad["paleta"] += 1
            r = conv_piece(x, y, t, samus_tile, SAMUS_PAL)
            if r is None:
                bad["tile"] += 1
                continue
            recs.append(r)
        sm_index.append(len(sm_recs) // 2)
        sm_recs += rec_header(recs)
        for r in recs:
            sm_recs += struct.pack(">hHHh", *r)
    binfile("kSamusSprRecs", bytes(sm_recs))
    carray("uint16", "kSamusSprIndex", sm_index)
    Hh.append(f"#define SAMUS_SPRITEMAP_COUNT {n_sm}")
    Hh.append("extern const uint16 kSamusSprIndex[SAMUS_SPRITEMAP_COUNT];  /* índice -> palabra en kSamusSprRecs (0xFFFF: nada) */")
    Hh.append("extern const uint16 kSamusSprRecs[];  /* n, y mín., y máx., x mín., x máx., luego n x (dy, tamaño, atributo, dx) */")
    print(f"samus: {n_sm} spritemaps, {len(sm_recs)} bytes, piezas fuera de lo normal: {dict(bad)}")
    # las piezas que se dejan fuera no deben aparecer en Ceres
    for key in draws:
        if key[1] == "samus":
            idx = int(key[2])
            assert sm_index[idx] != 0xFFFF, idx

    # ---------- 3) HUD ($9A:B200, 2bpp; palabras del tilemap del SNES -> celdas del plano Window)
    hud_gfx = rom.bytes(0x9AB200, 0x2000)
    digits_h = table_vals("kDigitTilesetsHealth")
    digits_w = table_vals("kDigitTilesetsWeapon")
    autores = table_vals("kHudTilemaps_AutoReserve")
    amap = table_vals("kAreaMapTilemaps")         # minimapa: las celdas de todas las áreas
    hudmis = table_vals("kHudTilemaps_Missiles")  # iconos de misiles, supermisiles, bombas de poder, gancho y rayos X
    words = set(hud_words) | set(digits_h) | set(digits_w) | set(autores) | set(hudmis) | {13360, 10289, 11279, 11295}
    for wv in list(words):
        for pb in (0x1000, 0x1400):                 # resaltado de ítems (ToggleHudItemHighlight)
            words.add(pb | (wv & 0xE3FF))
    for mw in amap:
        words.add((mw & 0xC3FF) | 0x2C00)
        words.add((mw & 0xC3FF) | 0x2800)
        words.add((mw & 0xC3FF) | 0x3C00)       # la sala de Samus, parpadeando (UpdateMinimapInside: |= 0x1C00)
    hud_cells = {}       # (tile, pal) -> conjunto de índices de CGRAM
    for wv in words:
        t, pal = wv & 0x3FF, (wv >> 10) & 7
        if t >= 0x200:
            continue
        px = snes_tile2(hud_gfx[t * 16:t * 16 + 16])
        hud_cells[(t, pal)] = px
    hud_idx = set()
    for (t, pal), px in hud_cells.items():
        hud_idx |= {pal * 4 + int(c) for c in np.unique(px) if c}
    # cajas de mensaje (banco $85): celdas del mismo tileset del BG3, aparte (sprites, md_msgbox)
    msg_px = {}
    for wv in msgbox_words(rom):
        t, pal = wv & 0x3FF, (wv >> 10) & 7
        if t < 0x200:
            msg_px[(t, pal)] = snes_tile2(hud_gfx[t * 16:t * 16 + 16])
    hud_cells["msg"] = msg_px
    print(f"hud: {len(words)} palabras, {len(hud_cells)} celdas, índices {sorted(hud_idx)}")

    # ---------- 3b) niebla (FX tipo 0xC, FxTypeFunc_C $88:DB08): el BG3 de la niebla se suma a la imagen
    # (LayerBlendFunc_18: BG3 en la pantalla principal, BG1/BG2/sprites en la subpantalla, suma). La Genesis no suma
    # capas: se suma a todos los colores de la sala el color medio de la niebla (md_tint, md_engine.c). kFogW[i]:
    # píxeles del mapa visible del BG3 ($5C00, 32x32) con el color i de la CGRAM (0-31); el mapa sale de
    # kFxTypeTilemapPtrs ($83:ABF0) -> $8A:ptr, copiado a la VRAM $5BE0 (2112 bytes); los tiles, los del BG3 ($9A:B200)
    fog_ptr = rom.w(0x83ABF0 + 2 * (0xC >> 1))
    fog_w = [0] * 32
    for k in range(1024):
        e = rom.w(0x8A0000 | (fog_ptr + 2 * (32 + k)))
        t, pal = e & 0x3FF, (e >> 10) & 7
        if t >= 0x200:
            continue
        px = snes_tile2(hud_gfx[t * 16:t * 16 + 16])
        for c in (1, 2, 3):
            fog_w[pal * 4 + c] += int((px == c).sum())
    carray("uint16", "kFogW", fog_w)
    Hh.append("extern const uint16 kFogW[32];  /* niebla: píxeles de cada color del BG3 en 32x32 tiles (de 65536) */")
    print("niebla: cobertura %.2f" % (sum(fog_w) / 65536))
    # neblina del Bomb Torizo (SpawnBombTorizoHaze, $88:DD62 -> tabla $88:DD74 del COLDATA): el color medio
    hz = [0, 0, 0]
    a, lines = 0x88DD75, 0
    while rom.b(a) and lines < 224:
        n = min(rom.b(a), 224 - lines)
        for bb in (rom.b(a + 1), rom.b(a + 2)):
            for k, bit in enumerate((0x20, 0x40, 0x80)):
                if bb & bit:
                    hz[k] += (bb & 31) * n
        lines += n
        a += 3
    Hh.append("#define MD_TORIZO_HAZE_R %d" % round(hz[0] / 224))
    Hh.append("#define MD_TORIZO_HAZE_G %d" % round(hz[1] / 224))
    Hh.append("#define MD_TORIZO_HAZE_B %d   /* neblina del Bomb Torizo: COLDATA medio (5 bits) */" % round(hz[2] / 224))

    # ---------- 4) salas
    room_recs = []
    obj_draws = {}
    for key in draws:
        room = int(key[0], 16)
        obj_draws.setdefault(room, set()).add(key[1:])
    # rayo normal en las 10 direcciones y su explosión ($93): spritemaps de las listas de instrucciones, para que
    # aparezcan aunque la grabación no dispare en todas las direcciones
    beam_sms = set()
    v3 = rom.w(0x930000 | 0x83C1)              # kProjectileData_UnchargedBeams + 0 (rayo normal)
    for d in range(10):
        beam_sms |= set(proj_list_sms(rom, rom.w(0x930000 | (v3 + 2 + 2 * d))))
    beam_sms |= set(proj_list_sms(rom, 0xA007))     # kProjInstr_BeamExplosion
    # (misiles, explosiones y estelas de Zebes: en los objetos de cada sala, mkzobj.py con zebes_proj_keys)
    for room in ROOMS:
        for sm in beam_sms:
            obj_draws.setdefault(room, set()).add(("proj", "%04X" % sm))
    print("rayo: spritemaps", sorted("%04X" % x for x in beam_sms))
    obj_draws.setdefault(0xE0B5, set()).add(("a6s", "A329", "0400"))   # puerta cerrada que dibuja Ridley_A2F2
    # Ridley de Ceres: todas sus spritemaps (cola, alas, cuerpo), las bolas de fuego y la bebé, aunque la grabación
    # no las muestre todas
    obj_draws[0xE0B5] |= ridley_keys(rom)
    # puertas de Ceres (enemigo $E23F): todas sus spritemaps vistas en cualquier sala, con las dos paletas que usa
    door_sms = {k[2] for ks in obj_draws.values() for k in ks if k[0] == "enemy" and k[1] == "A6"}
    # familias de spritemaps de la puerta (cada tipo de puerta usa una; se agrega la familia completa de las que
    # aparecen en la sala, con la paleta de esa sala, que depende de su población de enemigos)
    families = [{"F921"}, {"F95F", "F989", "F9B3", "F9D3", "F9F3"}, {"FA13", "FA3D", "FA67", "FA87", "FAA7"}]
    for room in ROOMS:
        seen = {k for k in obj_draws.get(room, ()) if k[0] == "enemy" and k[1] == "A6"}
        for sm_, pal in {(k[2], k[3]) for k in seen}:
            for fam in families:
                if sm_ in fam:
                    for sm in fam:
                        obj_draws[room].add(("enemy", "A6", sm, pal, "0000"))
    print("puertas: spritemaps", sorted(door_sms))
    hud_bank = TileBank(HUD_BASE)
    hud_lut = None
    for rid, (room_addr, state_addr, rdir) in ROOMS.items():
        rj = json.loads((ext / "rooms" / rdir / "room.json").read_text())
        st = next(s for s in rj["states"] if int(s["state_snes"][1:], 16) & 0xFFFF == state_addr)
        tag = "state%d_%s" % (rj["states"].index(st), st["condition"])
        vr = (vram_dir / f"{room_file(rid)}.bin").read_bytes()
        vram, cg = vr[:0x10000], list(struct.unpack("<256H", vr[0x10000:0x10200]))
        wb, hb = rj["w"] * 16, rj["h"] * 16
        lv = (ext / "rooms" / rdir / f"{tag}_level_data.bin").read_bytes()
        size = lv[0] | lv[1] << 8
        n = size // 2
        layer1 = struct.unpack_from(f"<{n}H", lv, 2)[:wb * hb]
        layer2 = struct.unpack_from(f"<{n}H", lv, 2 + size + n)[:wb * hb] if st.get("has_layer2") else None
        man = json.loads((ext / "tilesets" / "manifest.json").read_text())
        tt = int(man[st["gfx_set"]]["tile_table"].replace("$", ""), 16)
        table = (next((ext / "blobs" / "decompressed").glob(f"{tt:06X}_*.bin")).read_bytes() + bytes(0x2000))[:0x2000]
        ram_regs = json.loads((vram_dir / "regs.json").read_text())[room_file(rid)]
        nba = ram_regs["NBA"]
        bg1_base, bg2_base = (nba & 0xF) * 0x2000, (nba >> 4) * 0x2000      # en bytes

        def tp(base):
            cache = {}

            def f(t):
                if t not in cache:
                    a = (base + t * 32) & 0xFFFF
                    cache[t] = snes_tile4(vram[a:a + 32])
                return cache[t]
            return f
        W, H = wb * 2, hb * 2
        patches = []
        if room_addr == 0xDF45:
            # modo 7 (DoorCode_CeresElevatorShaft): plano = sala; la puerta del fondo alterna dos versiones
            m7 = (next((ext / "blobs" / "decompressed").glob("C0E22A_*.bin")).read_bytes() + bytes(0x8000))[:0x8000]
            offs = [rom.w(0xA6F900), rom.w(0xA6F902)]
            m7v = [apply_mode7_queue(rom, m7, 0xA6, o) for o in offs]
            PH = 128
            la = [mode7_cells(m7v[0], 0, 0, W, PH), mode7_cells(m7v[1], 0, 0, W, PH)]
            lb = None
            pw, ph = 32, 128
        else:
            la = [cells_from_words(level_words(layer1, wb, hb, table), W, H, tp(bg1_base))]
            if st["bg_data"] != "$8F0000":
                sc = ram_regs["BG2SC"]
                words2 = []
                base = (sc & 0xFC) << 9          # dirección en bytes del tilemap de BG2
                for y in range(32):
                    for x in range(64):
                        a = base + ((x >> 5) * 0x800) + (y * 32 + (x & 31)) * 2
                        words2.append(vram[a] | vram[a + 1] << 8)
                lb = cells_from_words(words2, 64, 32, tp(bg2_base))
                lb_lib = True
            elif layer2 is not None:
                lb = cells_from_words(level_words(layer2, wb, hb, table), W, H, tp(bg2_base))
                lb_lib = False
            else:
                lb = None
            pw, ph = 64, 64                    # planos de 64x64: caben todas las salas de Ceres menos el pozo
            assert W <= pw and H <= ph
        obj_keys = sorted(k for k in obj_draws.get(rid, ()) if k[0] in ("enemy", "enemyext", "eproj", "proj", "a6", "a6s", "front", "spr80", "spr8c", "spr82", "trail"))
        room_recs.append(dict(id=rid, addr=room_addr, state=state_addr, W=W, H=H, pw=pw, ph=ph, la=la, lb=lb,
                              lb_lib=(lb is not None and st["bg_data"] != "$8F0000"), cg=cg, vram=vram,
                              obj_keys=obj_keys, regs=ram_regs, extra=ROOM_EXTRA.get(rid),
                              text=sorted({int(k[2], 16) for k in obj_draws.get(rid, ()) if k[0] == "text"}),
                              bg1_base=bg1_base))
        if "pal" in ROOM_EXTRA.get(rid, {}):
            pal = vram_dir / ROOM_EXTRA[rid]["pal"]
            # colores para agrupar: los de la pelea (al entrar a la sala Ridley todavía está en negro)
            room_recs[-1]["cg"] = list(struct.unpack("<256H", pal.read_bytes()))
    # ---------- cinemáticas y menús: "salas" sin nivel (docs/35). Solo sprites (y en el menú, el BG1 fijo).
    # La línea 0 de la CRAM queda para la capa del modo 7 (cada imagen trae sus colores, mkm7.py) y su color 15 para
    # el círculo de la explosión.
    for pid, cap, title in ((0xC001, "CINE1", "Ceres explota"), (0xC002, "CINE2_M7", "Zebes"), (0xC0FF, "MENU", "fin de juego")):
        vr = (vram_dir / f"{cap}.bin").read_bytes()
        vram, cg = vr[:0x10000], list(struct.unpack("<256H", vr[0x10000:0x10200]))
        if pid == 0xC0FF:                      # BG1 del menú (7E:3600 -> VRAM 0x5000): siempre el mismo texto
            words = [vram[0xA000 + 2 * i] | vram[0xA001 + 2 * i] << 8 for i in range(32 * 32)]   # primera pantalla 32x32
            W, H = 32, 28
            la = [cells_from_words(words[:W * 32], W, 32, lambda t: snes_tile4(vram[t * 32:t * 32 + 32]))[:H]]
            extra = dict(no_hud=True)
        else:
            W, H = 1, 1
            la = [[[None]]]
            extra = dict(no_hud=True, free_line=0)
        obj_keys = sorted(k for k in obj_draws.get(pid, ()) if k[0] in ("spr8c", "spr82"))
        room_recs.append(dict(id=pid, addr=pid, state=0, W=W, H=H, pw=64, ph=64, la=la, lb=None, lb_lib=False, cg=cg,
                              vram=vram, obj_keys=obj_keys, regs={}, extra=extra, text=[], bg1_base=0))
        print(f"{title}: {len(obj_keys)} claves de sprites")
    # (continúa en build_rooms)
    build_rooms(rom, room_recs, hud_cells, out_dir, binfile, carray, C, Hh)
    write_out(out_dir, C, Hh, bins)


def rec_header(recs):
    """cabecera de una lista de sprites: n y el rectángulo de sus esquinas (y mín., y máx., x mín., x máx.): si cae
    entero en pantalla, la Genesis los copia sin mirar uno por uno"""
    if not recs:
        return struct.pack(">Hhhhh", 0, 0, 0, 0, 0)
    ys = [r[0] for r in recs]
    xs = [r[3] for r in recs]
    return struct.pack(">Hhhhh", len(recs), min(ys), max(ys), min(xs), max(xs))


OBJ_BANK = {"enemy": None, "eproj": 0x8D, "a6": 0xA6, "proj": 0x93}

RIDLEY_TAIL = [0xDC90, 0xDC97, 0xDC9E]
RIDLEY_BODY_EXT = [0xE983, 0xE9A5, 0xE9C7, 0xE9E9, 0xEA0B, 0xEA2D, 0xEA4F, 0xEA71, 0xEA93, 0xEAB5, 0xEAD7]
# sala de Ridley (E0B5):
#   reserve: la línea 1 de la CRAM es solo de Ridley, igual a su paleta del SNES (índices 241-255 -> colores 1-15):
#            así los tiles de Ridley no se convierten y su parpadeo al recibir un disparo (paleta 0 del SNES,
#            índices 128-143) es solo cambiar esa línea de la CRAM, sin tiles extra
#   dyn:     tiles de Ridley que el juego cambia en la VRAM durante la pelea (Ridley_Func_118/119): grupos fijos
#            en la Genesis con todas sus versiones preparadas
#   door:    la puerta cerrada que dibuja Ridley_A2F2 (sprite A329 en x=16, y=127) pasa a ser parte del plano A
#            mientras dura la pelea (celdas alternativas): ahorra 8 sprites
ROOM_EXTRA = {
    0xE0B5: dict(pal="E0B5_pal.bin", black_clear=True, reserve_line=1, reserve_pal=7, flash_base=128,
                 dyn=[dict(tiles=(0x122, 0x132, 0x123, 0x133), rows=(0x7220, 0x7320), n=2),
                      dict(tiles=(0x1AC, 0x1BC, 0x1AD, 0x1BD), rows=(0x7AC0, 0x7BC0), n=2, col=0),
                      dict(tiles=(0x1AE, 0x1BE, 0x1AF, 0x1BF), rows=(0x7AC0, 0x7BC0), n=2, col=2)],
                 door=dict(sm=0xA329, chr=0x0400, x=0x10, y=0x7F)),
    # pozo en la huida (modo 7): fuera del pozo el mapa de modo 7 del SNES es una pared (tiles 98 99 / 9A 9B que se
    # repiten), y al girar se ve. La Genesis usa un plano de 64x64: columnas 32-63 la pared, 0-31 el pozo en anillo.
    0xDF71: dict(m7wall=(0x98, 0x99, 0x9A, 0x9B)),
}


def ridley_spritemaps(rom):
    """spritemaps de Ridley ($A6): cola (sub_A6DB2A), alas (g_off_A6DB02) y piezas del cuerpo (spritemaps
    extendidos de sus listas de instrucciones)"""
    tail = set(RIDLEY_TAIL) | {rom.w(0xA6DCBA + 2 * i) for i in range(16)}
    wings = {rom.w(0xA6DB02 + 2 * i) for i in range(20)}
    body = set()
    for e in RIDLEY_BODY_EXT:
        n = rom.w(0xA60000 | e)
        for i in range(n):
            body.add(rom.w(0xA60000 | (e + 2 + 8 * i + 4)))
    return tail | wings, body


def ridley_keys(rom):
    a6, body = ridley_spritemaps(rom)
    keys = {("a6", "%04X" % sm, "0E00") for sm in a6}
    keys |= {("enemy", "A6", "%04X" % sm, "0E00", "0000") for sm in body}
    keys |= {("enemyext", "A6", "%04X" % e, "0E00", "0000") for e in RIDLEY_BODY_EXT}
    # parpadeo (paleta 0): las mismas, como claves que apuntan a los mismos sprites
    keys |= {("a6", "%04X" % sm, "0000") for sm in a6}
    keys |= {("enemy", "A6", "%04X" % sm, "0000", "0000") for sm in body}
    keys |= {("enemyext", "A6", "%04X" % e, "0000", "0000") for e in RIDLEY_BODY_EXT}
    # bolas de fuego ($86:9552...) y sus explosiones: spritemaps de las listas de instrucciones
    args = {0x816A: 0, 0x8161: 1, 0x95BA: 0, 0x95ED: 0, 0x9620: 0}
    for st in (0x9552, 0x9574, 0x958C, 0x95A0, 0x95D3, 0x9606):
        a, seen = st, set()
        while a not in seen:
            seen.add(a)
            v = rom.w(0x860000 | a)
            if v & 0x8000:
                if v in (0x81AB, 0x8154, 0x8159):
                    break
                a += 2 + 2 * args[v]
            else:
                keys.add(("eproj", "%04X" % rom.w(0x860000 | (a + 2)), "0A00"))
                a += 4
    for sm in (0xBFFD, 0xC018, 0xC033):          # la bebé en la cápsula (kBabyMetroid_Ilist_BF31)
        keys.add(("a6", "%04X" % sm, "0000"))
    return keys


def is_flash_key(rom, key):
    """dibujo de Ridley con la paleta 0 (parpadeo al recibir un disparo): mismo sprite, otra línea de la CRAM"""
    a6, body = ridley_spritemaps(rom)
    if key[0] == "a6" and int(key[1], 16) in a6 and key[2] == "0000":
        return ("a6", key[1], "0E00")
    if key[0] == "enemy" and key[1] == "A6" and int(key[2], 16) in body and key[3] == "0000":
        return ("enemy", "A6", key[2], "0E00", key[4])
    if key[0] == "enemyext" and key[1] == "A6" and int(key[2], 16) in RIDLEY_BODY_EXT and key[3] == "0000":
        return ("enemyext", "A6", key[2], "0E00", key[4])
    return None


def obj_tile_px(vram, T, dx, dy):
    t = (T & 0x100) | ((T + dy * 16) & 0xF0) | ((T + dx) & 0x0F)
    a = (0xC000 + t * 32) & 0xFFFF
    return snes_tile4(vram[a:a + 32])


def key_pieces(rom, key):
    """(tipo, ...) del registro de dibujo -> lista de (x, y, atributo del SNES) como lo arma el decomp"""
    kind = key[0]
    if kind == "front":                       # SamusBottomDrawn_0_Standing ($90:868D): un sprite 8x8, tile 0x3821
        return [(0, 0, 0x3821)]
    if kind == "trail":                       # estela del misil: un sprite de 8x8 con ese tile y atributo
        return [(0, 0, int(key[1], 16))]
    if kind == "enemyext":                    # spritemap extendido entero (WriteEnemyOams): sus spritemaps en orden
        bank, ext = int(key[1], 16), int(key[2], 16)
        out = []
        for i in range(rom.w((bank << 16) | ext) & 0xFF):      # el original lee un byte
            e = (bank << 16) | (ext + 2 + 8 * i)
            ox, oy, sm = rom.w(e), rom.w(e + 2), rom.w(e + 4)
            if rom.w((bank << 16) | sm) == 0xFFFE:                 # trozo de tilemap (no se dibuja sin el bit 0x8000)
                continue
            ox = ox - 0x10000 if ox & 0x8000 else ox
            oy = oy - 0x10000 if oy & 0x8000 else oy
            for (x, y, t) in key_pieces(rom, ("enemy", key[1], "%04X" % sm, key[3], key[4])):
                dx = x & 0x1FF
                dx = (dx - 0x200 if dx >= 0x100 else dx) + ox
                out.append(((dx & 0x1FF) | (x & 0x8000), y + oy, t))
        return out
    if kind == "enemy":
        bank, sm, r3, r0 = int(key[1], 16), int(key[2], 16), int(key[3], 16), int(key[4], 16)
        f = lambda t: r3 | ((r0 + t) & 0xFFFF)
    elif kind == "eproj":
        bank, sm, g = 0x8D, int(key[1], 16), int(key[2], 16)
        f = lambda t: (g & 0xFF00) | (((g & 0xFF) + t) & 0xFFFF)
    elif kind == "a6":
        bank, sm, a = 0xA6, int(key[1], 16), int(key[2], 16)
        f = lambda t: a | t
    elif kind == "a6s":                         # DrawSpritemap ($81:879F): chr | (palabra & 0xF1FF)
        bank, sm, a = 0xA6, int(key[1], 16), int(key[2], 16)
        f = lambda t: a | (t & 0xF1FF)
    elif kind == "proj":
        bank, sm = 0x93, int(key[1], 16)
        f = lambda t: t
    elif kind == "spr80":                       # contador de la huida: DrawSpritemap($80, sm, x, y, 0x0A00)
        bank, sm, a = 0x80, int(key[1], 16), int(key[2], 16)
        f = lambda t: a | (t & 0xF1FF)
    elif kind == "spr8c":                       # cinemáticas: DrawSpritemap($8C, sm, x, y, chr)
        bank, sm, a = 0x8C, int(key[1], 16), int(key[2], 16)
        f = lambda t: a | (t & 0xF1FF)
    elif kind == "spr82":                       # menús: DrawMenuSpritemap(i, x, y, chr) = $82:(g_off_82C569[i])
        bank, sm, a = 0x82, rom.w(0x82C569 + 2 * int(key[1], 16)), int(key[2], 16)
        f = lambda t: a | (t & 0xF1FF)
    else:
        raise ValueError(kind)
    base = (bank << 16) | sm
    n = rom.w(base)
    out = []
    for j in range(n):
        x, y, t = struct.unpack_from("<HbH", rom.d, rom.off(base) + 2 + 5 * j)
        out.append((x, y, f(t)))
    return out


MERGE_STATS = [0, 0]
ESC_TEXT = []     # (palabra del SNES, celda de la Genesis) del texto de la huida
ESC_VRAM = None   # VRAM de la sala de Ridley ya en la huida: los tiles del contador (los demás cuartos los tienen)


def key_vram(key, vram):
    return ESC_VRAM if key[0] == "spr80" and ESC_VRAM is not None else vram

DYN = []          # (fila de VRAM del SNES, origen en $B0, tile de la Genesis, 128 bytes)


def merge_sprite(parts):
    """parts: piezas del SNES en orden de OAM (la primera queda delante): (x, y, píxeles hxw ya volteados).
    Las compone en una imagen y la cubre con sprites de la Genesis de hasta 4x4 celdas, eligiendo la alineación de
    la grilla de 8x8 que deja menos celdas y menos sprites. Devuelve [(x, y, w, h, [celdas por columnas])]."""
    canvas = {}
    for x, y, px in reversed(parts):
        h, w = px.shape
        for yy in range(h):
            for xx in range(w):
                if px[yy, xx]:
                    canvas[(x + xx, y + yy)] = int(px[yy, xx])
    if not canvas:
        return []
    xs = [p[0] for p in canvas]
    ys = [p[1] for p in canvas]
    best = None
    for oy in range(8):
        for ox in range(8):
            X0, Y0 = min(xs) - ox, min(ys) - oy
            cells = {((x - X0) >> 3, (y - Y0) >> 3) for (x, y) in canvas}
            W = max(c[0] for c in cells) + 1
            H = max(c[1] for c in cells) + 1
            rects = cover_cells(cells, W, H)
            sc = (len(rects), sum(r[2] * r[3] for r in rects))
            if best is None or sc < best[0]:
                best = (sc, X0, Y0, rects)
    _, X0, Y0, rects = best
    out = []
    for (cx, cy, w, h) in rects:
        tiles = []
        for c in range(w):
            for r in range(h):
                px = np.zeros((8, 8), np.int32)
                for yy in range(8):
                    for xx in range(8):
                        px[yy, xx] = canvas.get((X0 + (cx + c) * 8 + xx, Y0 + (cy + r) * 8 + yy), 0)
                tiles.append(px)
        out.append((X0 + cx * 8, Y0 + cy * 8, w, h, tiles))
    return out


def cover_cells(cells, W, H):
    """cubre las celdas no vacías con rectángulos de hasta 4x4 sin superponerse (codicioso: el que cubre más celdas
    llenas con menos vacías, desde la primera celda libre arriba a la izquierda)"""
    left = set(cells)
    rects = []
    while left:
        cy, cx = min((c[1], c[0]) for c in left)
        best = None
        for h in range(1, 5):
            for w in range(1, 5):
                for sx in range(cx - w + 1, cx + 1):
                    rc = {(sx + i, cy + j) for i in range(w) for j in range(h)}
                    if any(c not in left and c in cells for c in rc):
                        continue                  # no tapar celdas ya cubiertas
                    full = len(rc & left)
                    if cx not in range(sx, sx + w) or (cx, cy) not in rc:
                        continue
                    sc = (-full + 0.6 * (w * h - full), w * h)
                    if best is None or sc < best[0]:
                        best = (sc, (sx, cy, w, h), rc)
        _, r, rc = best
        rects.append(r)
        left -= rc
    return rects


def key_hash(kind, a, b, c):
    """la misma cuenta que find_key en md_engine.c (16 bits)"""
    return (a ^ (a >> 5) ^ (b >> 8) ^ b ^ c ^ (kind * 0x9E)) & 0xFFFF


def build_rooms(rom, room_recs, hud_cells, out_dir, binfile, carray, C, Hh):
    # ---------- HUD: tiles fijos y tabla palabra del SNES -> celda de la Genesis
    msg_px = hud_cells.pop("msg")
    hud_idx = set()
    for (t, pal), px in hud_cells.items():
        hud_idx |= {pal * 4 + int(c) for c in np.unique(px) if c}
    # índices del HUD con el mismo color comparten uno (CGRAM de referencia: la de la primera sala)
    cg0 = room_recs[0]["cg"]
    hud_slots, hud_alias = [0], {}
    for i in sorted(hud_idx):
        same = next((j for j in hud_slots if cg0[j] == cg0[i]), None)
        if same is not None:
            hud_alias[i] = same
        else:
            hud_slots.append(i)
    assert len(hud_slots) <= 15, hud_slots
    hud_slot_of = {i: k + 1 for k, i in enumerate(hud_slots)}
    for i, j in hud_alias.items():
        hud_slot_of[i] = hud_slot_of[j]
    # cajas de mensaje: sin colores nuevos en la línea 3 (los que siguen al HUD son de las salas). Sus índices toman
    # el color del HUD más parecido; 25 y 26 (InitializePpuForMessageBoxes: 0x0BB1 y 0x001F) toman dos colores del
    # HUD que no usa la caja, y en las líneas de la caja esos dos se cambian (md_engine.c, MB_SLOT25/26)
    def rgb(c):
        return np.array([c & 31, (c >> 5) & 31, (c >> 10) & 31], np.int32)
    msg_idx = set()
    for (t, pal), px in msg_px.items():
        msg_idx |= {pal * 4 + int(c) for c in np.unique(px) if c}
    msg_slot_of = dict(hud_slot_of)
    for i in sorted(msg_idx - set(hud_slot_of) - {25, 26}):
        j = min(hud_slots[1:], key=lambda j: int(np.abs(rgb(cg0[i]) - rgb(cg0[j])).sum()))
        msg_slot_of[i] = hud_slot_of[j]
    used = {msg_slot_of[i] for i in msg_idx - {25, 26} if i in msg_slot_of} | {msg_slot_of[0]}
    free = [k for k in range(1, len(hud_slots) + 1) if k not in used]
    assert len(free) >= 2, ("caja de mensaje: no quedan colores para 25/26", free)
    msg_slot_of[25], msg_slot_of[26] = free[0], free[1]
    Hh.append(f"#define MB_SLOT25 {free[0]}   /* color de la línea 3 que la caja usa como el 25 de la CGRAM */")
    Hh.append(f"#define MB_SLOT26 {free[1]}")
    print("hud: caja de mensaje, colores 25/26 en", free[:2])
    hud_bank = TileBank(HUD_BASE)
    black = np.full((8, 8), hud_slot_of[0], np.int32)
    t_black, _, _ = hud_bank.add(black, flips=False)
    lut = [(1 << 15) | (3 << 13) | t_black] * 8192
    for (t, pal), px in sorted(hud_cells.items()):
        g = np.vectorize(lambda c: hud_slot_of[pal * 4 + c if c else 0])(px)
        gt, _, _ = hud_bank.add(g.astype(np.int32), flips=False)
        lut[(pal << 10) | t] = (1 << 15) | (3 << 13) | gt
    assert HUD_BASE + len(hud_bank.tiles) <= SAMUS_BASE, len(hud_bank.tiles)
    # cajas de mensaje (md_msgbox en md_engine.c): sprites con los tiles de las celdas del BG3, ya volteados, en la
    # línea 3 con los colores del HUD. kMsgCells: (palabra del SNES & 0x7FFF, tile) ordenadas; tile 0 = vacío
    msg_tiles, msg_cells = [bytes(32)], []
    for wv in sorted({w & 0x7FFF for w in msgbox_words(rom)}):
        t, pal = wv & 0x3FF, (wv >> 10) & 7
        if (t, pal) not in msg_px:
            continue
        g = np.vectorize(lambda c: msg_slot_of[pal * 4 + c if c else 0])(msg_px[(t, pal)]).astype(np.int32)
        if wv & 0x4000:
            g = g[:, ::-1]
        if (wv & 0x3FF) == 0xE:
            g = np.zeros((8, 8), np.int32)                    # fuera de la caja: transparente
        k = genesis_tile_bytes(g)
        if k not in msg_tiles:
            msg_tiles.append(k)
        msg_cells += [wv, msg_tiles.index(k)]
    binfile("kMsgTiles", b"".join(msg_tiles))
    carray("uint16", "kMsgCells", msg_cells)
    Hh.append(f"#define MSG_CELL_COUNT {len(msg_cells) // 2}")
    Hh.append(f"#define MSG_TILE_COUNT {len(msg_tiles)}")
    Hh.append("extern const uint8 kMsgTiles[];")
    Hh.append("extern const uint16 kMsgCells[];  /* (palabra del BG3 & 0x7FFF, tile en kMsgTiles) ordenadas */")
    binfile("kHudTiles", hud_bank.data())
    binfile("kHudLut", struct.pack(">8192H", *lut))
    Hh.append(f"#define HUD_TILE_COUNT {len(hud_bank.tiles)}")
    Hh.append("extern const uint8 kHudTiles[];")
    Hh.append("extern const uint16 kHudLut[8192];  /* palabra del HUD del SNES & 0x1FFF -> celda del plano Window */")
    Hh.append(f"#define HUD_BLACK_CELL 0x{(1 << 15) | (3 << 13) | t_black:04X}")
    Hh.append(f"#define HUD_NSLOTS {len(hud_slots)}   /* colores 1..n de la línea 3: el HUD */")
    print(f"hud: {len(hud_bank.tiles)} tiles, índices {hud_slots}")

    obj_recs = bytearray()
    rooms_c = []
    for R in room_recs:
        cg, vram = R["cg"], R["vram"]
        pw, ph = R["pw"], R["ph"]
        X = R.get("extra") or {}
        if X.get("black_clear") and R["lb"] is not None:
            # haze de Ceres: en el SNES el color math suma azul a todo; en la Genesis el degradado está solo en el color
            # de fondo. Los píxeles negros de los planos (sin nada distinto detrás) pasan a transparentes para que se
            # vea el degradado. (Los planos de esta sala no se desplazan entre sí: misma celda en A y en B.)
            def clear_black(L, under=None):
                for y, row in enumerate(L):
                    for x, c in enumerate(row):
                        if c is None:
                            continue
                        px = c[0].copy()
                        blk = np.vectorize(lambda i: i != 0 and cg[i] == 0)(px)
                        if under is not None:
                            u = under[y][x] if y < len(under) and x < len(under[y]) else None
                            if u is not None:
                                blk &= (u[0] == 0)
                        px[blk] = 0
                        row[x] = (px, c[1]) if px.any() else None
            clear_black(R["lb"])
            clear_black(R["la"][0], R["lb"])
        if X.get("reserve_line") is not None:
            # el cuerpo de Ridley se dibuja con los spritemaps extendidos enteros (enemyext): sus piezas sueltas sobran
            _, body = ridley_spritemaps(rom)
            R["obj_keys"] = [k for k in R["obj_keys"] if not (k[0] == "enemy" and k[1] == "A6" and int(k[2], 16) in body)]
        # ---- piezas de los objetos de la sala (sin la puerta que pasa al fondo ni los parpadeos de Ridley)
        flash_keys = []
        obj_keys = []
        for key in R["obj_keys"]:
            if X.get("door") and key[0] == "a6s" and int(key[1], 16) == X["door"]["sm"]:
                continue
            if X.get("reserve_line") and is_flash_key(rom, key):
                flash_keys.append(key)
                continue
            obj_keys.append(key)
        # lo de Ridley (paleta reservada) al final: sus tiles quedan juntos y la huida puede usar ese espacio
        # (solo las claves de Ridley: otros objetos con la paleta 7 del SNES, como las columnas de los costados que
        # tapan el borde en el temblor, usan la misma línea de la CRAM pero sus tiles van antes, fuera de donde carga
        # la huida: se ven después de ella)
        rid_set = ridley_keys(rom) if X.get("reserve_pal") is not None else set()

        def is_rid(k):
            return k in rid_set
        obj_keys.sort(key=is_rid)
        pieces = []
        for key in obj_keys:
            for (x, y, at) in key_pieces(rom, key):
                pieces.append((key, x, y, at))
        rpal = X.get("reserve_pal", -1)
        # ---- puerta en el fondo: la imagen del sprite sobre las celdas del plano A (capa A0)
        door_cells = {}
        if X.get("door"):
            D = X["door"]
            canvas = {}
            for (x, y, at) in reversed(key_pieces(rom, ("a6s", "%04X" % D["sm"], "%04X" % D["chr"]))):
                T, pal = at & 0x1FF, (at >> 9) & 7
                assert not (x & 0x8000)
                dx = x & 0x1FF
                dx = dx - 0x200 if dx >= 0x100 else dx
                px = obj_tile_px(vram, T, 0, 0)
                if (at >> 14) & 1:
                    px = px[:, ::-1]
                if (at >> 15) & 1:
                    px = px[::-1, :]
                # un sprite del SNES en y se ve en la fila y+1 del fondo (con la cámara en 0,0 en esta sala)
                X0, Y0 = D["x"] + dx, D["y"] + y + 1
                for yy in range(8):
                    for xx in range(8):
                        if px[yy, xx]:
                            canvas[(X0 + xx, Y0 + yy)] = 128 + pal * 16 + int(px[yy, xx])
            L0 = R["la"][0]
            for (X1, Y1), v in canvas.items():
                cx, cy = X1 >> 3, Y1 >> 3
                if (cx, cy) not in door_cells:
                    base = L0[cy][cx]
                    door_cells[(cx, cy)] = [base[0].copy() if base else np.zeros((8, 8), np.int32),
                                            base[1] if base else 0]
                door_cells[(cx, cy)][0][Y1 & 7, X1 & 7] = v
            R["door_cells"] = door_cells
        # ---- todo lo que necesita colores: celdas de fondo y piezas de sprites (menos las de Samus, PAL2)
        items, counts = {}, Counter()
        layers = [("A%d" % i, L) for i, L in enumerate(R["la"])] + ([("B", R["lb"])] if R["lb"] else [])
        if door_cells:
            layers.append(("D", {k: (v[0], v[1]) for k, v in door_cells.items()}))
        # texto de la huida (EscapeTextCell): celdas del BG1 del SNES con tiles que el juego carga al empezar la huida
        # (tomados de la VRAM de la huida). Sus colores no se juntan con otros: la paleta 5 del SNES cambia al empezar la
        # huida y los colores 97-99 cambian solos (Ridley_Func_61).
        text_px = {}
        for wv in R.get("text", ()):
            a = (R["bg1_base"] + (wv & 0x3FF) * 32) & 0xFFFF
            px = snes_tile4(ESC_VRAM[a:a + 32])
            px = np.where(px != 0, px + ((wv >> 10) & 7) * 16, 0).astype(np.int32)
            if wv & 0x4000:
                px = px[:, ::-1]
            if wv & 0x8000:
                px = px[::-1, :]
            text_px[wv] = px
            ss = set(np.unique(px).tolist()) - {0}
            items[("text", wv)] = ss
            for i in ss:
                counts[i] += int((px == i).sum())
        wall_px = []
        if X.get("m7wall"):
            for t in X["m7wall"]:                  # tiles del modo 7: 8 bits por píxel (byte alto de cada palabra)
                px = np.array([[vram[2 * (t * 64 + y * 8 + x) + 1] for x in range(8)] for y in range(8)], np.int32)
                wall_px.append(px)
                ss = set(np.unique(px).tolist()) - {0}
                items[("wall", t)] = ss
                for i in ss:
                    counts[i] += int((px == i).sum()) * 64
        for name, L in layers:
            cells = L.items() if isinstance(L, dict) else ((( x, y), c) for y, row in enumerate(L) for x, c in enumerate(row))
            for (x, y), c in cells:
                if c is None:
                    continue
                ss = set(np.unique(c[0]).tolist()) - {0}
                items[("bg", name, x, y)] = ss
                for i in ss:
                    counts[i] += int((c[0] == i).sum())
        for pi, (key, x, y, at) in enumerate(pieces):
            T, pal, big = at & 0x1FF, (at >> 9) & 7, bool(x & 0x8000)
            if pal == 4 or pal == rpal:
                continue
            ss = set()
            for dy in range(2 if big else 1):
                for dx in range(2 if big else 1):
                    px = obj_tile_px(key_vram(key, vram), T, dx, dy)
                    for c in np.unique(px):
                        if c:
                            ss.add(128 + pal * 16 + int(c))
                            counts[128 + pal * 16 + int(c)] += int((px == c).sum())
            items[("obj", pi)] = ss
        # alias por color igual (sin mezclar el HUD, índices 0-31, con el resto: se funden distinto)
        # grupos que se funden igual: el HUD (0-31) y el rayo (224-239) no cambian en los fundidos de puerta
        # (LoadColorsForSpritesBeamsAndEnemies deja su objetivo igual al actual); el negro es negro siempre
        text_idx = {i for ss in (items[("text", w)] for w in text_px) for i in ss}
        grp = lambda i: ("txt", i) if i in text_idx else \
            (2 if cg[i] == 0 else (0 if (i < 32 or 224 <= i < 240) else 1))
        alias, bycol = {}, {}
        for i in sorted(counts, key=lambda i: -counts[i]):
            k = (grp(i), cg[i])
            if k in bycol:
                alias[i] = bycol[k]
            else:
                bycol[k] = i
        prefill = {3: set(hud_slots[1:])} if not X.get("no_hud") else {3: set()}
        lines = [0, 1, 3]
        if X.get("free_line") is not None:     # línea que maneja la Genesis (capa del modo 7 de las cinemáticas)
            lines = [l for l in lines if l != X["free_line"]]
        if X.get("reserve_line") is not None:
            lines = [l for l in lines if l != X["reserve_line"]]
        caps = {0: 15, 1: 15, 3: 15 - 1 if not X.get("no_hud") else 15}   # en PAL3 el índice 0 (negro del HUD) ocupa un color
        import random
        rnd = random.Random(1234)

        def try_pack(order):
            bins = {l: set(prefill.get(l, ())) for l in lines}
            assign = {}
            for fs, ks in order:
                best = None
                for l in lines:
                    u = bins[l] | fs
                    if len(u) <= caps[l]:
                        sc = (len(u) - len(bins[l]), l)
                        if best is None or sc < best[0]:
                            best = (sc, l)
                if best is None:
                    return None, None
                bins[best[1]] |= fs
                for k in ks:
                    assign[k] = best[1]
            return bins, assign
        while True:
            groups = {}
            for k, ss in items.items():
                groups.setdefault(frozenset(PalPacker._res(alias, i) for i in ss), []).append(k)
            # quitar los conjuntos contenidos en otros (caben donde vaya el mayor)
            gl = sorted(groups.items(), key=lambda g: -len(g[0]))
            bins, assign = try_pack(gl)
            tries = 0
            while bins is None and tries < 300:
                tries += 1
                o = gl[:]
                rnd.shuffle(o)
                o.sort(key=lambda g: -len(g[0]) + rnd.random() * 6)
                bins, assign = try_pack(o)
            if bins is not None:
                break
            alive = sorted({PalPacker._res(alias, i) for ss in items.values() for i in ss} - set(prefill[3]))
            best = None
            for a_ in range(len(alive)):
                for b_ in range(a_ + 1, len(alive)):
                    ia, ib = alive[a_], alive[b_]
                    if grp(ia) != grp(ib):
                        continue
                    d = color_dist(bgr_to_gen(cg[ia]), bgr_to_gen(cg[ib]))
                    if best is None or d < best[0]:
                        best = (d, ia, ib)
            _, ia, ib = best
            keep, drop = (ia, ib) if counts[ia] >= counts[ib] else (ib, ia)
            alias[drop] = keep
        res = lambda i: PalPacker._res(alias, i)
        slot_of = {}
        for l in lines:
            fixed = [] if (l != 3 or X.get("no_hud")) else hud_slots
            rest = sorted(bins[l] - set(fixed))
            order = fixed + rest if l == 3 else rest
            for si, i in enumerate(order):
                slot_of.setdefault(l, {})[i] = si + 1
        if X.get("reserve_line") is not None:
            # línea reservada: color c = índice reserve_pal*16 + c del SNES (los tiles quedan como en el SNES)
            slot_of[X["reserve_line"]] = {128 + rpal * 16 + c: c for c in range(1, 16)}
            bins[X["reserve_line"]] = set(slot_of[X["reserve_line"]])
        nmerge = sum(1 for d, k in alias.items() if cg[d] != cg[k])
        # ---- fondo
        TB = CINE_BG_BASE if X.get("no_hud") else BG_BASE   # cinemáticas y menús: sin HUD ni Samus, desde CINE_BG_BASE
        bank = TileBank(TB)

        def cell_word(name, x, y, c):
            if c is None:
                return 0
            l = assign[("bg", name, x, y)]
            px = np.vectorize(lambda i: slot_of[l][res(i)] if i else 0)(c[0]).astype(np.int32)
            t, hf, vf = bank.add(px)
            return (c[1] << 15) | (l << 13) | (vf << 12) | (hf << 11) | t if t else 0

        def plane_of(name, L, rep):
            sh, sw = len(L), len(L[0])
            img = [0] * (pw * ph)
            for y in range(ph):
                for x in range(pw):
                    sy, sx = (y % sh, x % sw) if rep else (y, x)
                    if sy < sh and sx < sw:
                        img[y * pw + x] = cell_word(name, sx, sy, L[sy][sx])
            return img
        la = R["la"]
        pa = plane_of("A0", la[0], False)
        patches = []
        if len(la) > 1:
            pa1 = plane_of("A1", la[1], False)
            patches = [(i, pa[i], pa1[i]) for i in range(len(pa)) if pa[i] != pa1[i]]
        for (cx, cy), cell in sorted(R.get("door_cells", {}).items()):
            i = cy * pw + cx
            patches.append((i, pa[i], cell_word("D", cx, cy, (cell[0], cell[1]))))
        pb = plane_of("B", R["lb"], R["lb_lib"]) if R["lb"] is not None else None
        wall_w = []
        for t, px in zip(X.get("m7wall", ()), wall_px):
            l = assign[("wall", t)]
            g = np.vectorize(lambda i: slot_of[l][res(i)] if i else 0)(px).astype(np.int32)
            ti, hf, vf = bank.add(g)
            wall_w.append((l << 13) | (vf << 12) | (hf << 11) | ti)
        if wall_w:
            # filas debajo del pozo (y, al dar la vuelta, encima): pared, como el mapa de modo 7
            for y in range(R["H"], ph):
                for x in range(pw):
                    pa[y * pw + x] = wall_w[(y & 1) * 2 + (x & 1)]
            carray("uint16", f"kWall_{R['id']:04X}", wall_w)
        for wv, px in sorted(text_px.items()):
            l = assign[("text", wv)]
            g = np.vectorize(lambda i: slot_of[l][res(i)] if i else 0)(px).astype(np.int32)
            t, hf, vf = bank.add(g)
            ESC_TEXT.append((wv, ((wv >> 13) & 1) << 15 | (l << 13) | (vf << 12) | (hf << 11) | t))
        nbg_tiles = len(bank.tiles)
        # ---- objetos: tiles de cada pieza (por columnas, como el sprite de la Genesis) y registros
        obank = TileBank(TB + nbg_tiles)
        # tiles que el juego cambia en la VRAM (dyn): grupos fijos de 4 (sprite de 2x2), primero el contenido inicial
        dyn_of = {}
        for g in X.get("dyn", ()):
            t0 = TB + nbg_tiles + len(obank.tiles)
            for T in g["tiles"]:
                obank.tiles.append(genesis_tile_bytes(obj_tile_px(vram, T, 0, 0)))
            g["genesis_tile"] = t0
            dyn_of[g["tiles"][0]] = t0
        keys_c = []
        rec_of = {}
        pi = -1
        rid_first = None
        for key in obj_keys:
            recs = []
            kp = key_pieces(rom, key)
            if rid_first is None and X.get("reserve_pal") is not None and is_rid(key):
                rid_first = TB + nbg_tiles + len(obank.tiles)
            if rpal >= 0 and kp and all((at >> 9) & 7 == rpal for (x, y, at) in kp) \
                    and len({(at >> 12) & 3 for (x, y, at) in kp}) == 1:
                # Ridley: las piezas del SNES se juntan en sprites más grandes de la Genesis (menos sprites por línea).
                # Las piezas de tiles dinámicos quedan como están y parten la lista (se respeta qué va delante).
                prio = 1 if ((kp[0][2] >> 12) & 3) == 3 else 0
                runs, cur = [], []
                for (x, y, at) in kp:
                    pi += 1
                    T, big = at & 0x1FF, bool(x & 0x8000)
                    dxx = x & 0x1FF
                    dxx = dxx - 0x200 if dxx >= 0x100 else dxx
                    if big and T in dyn_of:
                        runs.append(cur)
                        runs.append(("dyn", dxx, y, at))
                        cur = []
                        continue
                    n = 2 if big else 1
                    img = np.zeros((8 * n, 8 * n), np.int32)
                    for dx in range(n):
                        for dy in range(n):
                            img[dy * 8:dy * 8 + 8, dx * 8:dx * 8 + 8] = obj_tile_px(vram, T, dx, dy)
                    if (at >> 14) & 1:
                        img = img[:, ::-1]
                    if (at >> 15) & 1:
                        img = img[::-1, :]
                    cur.append((dxx, y, img))
                runs.append(cur)
                for run in runs:
                    if isinstance(run, tuple):
                        _, dxx, y, at = run
                        attr = (prio << 15) | (X["reserve_line"] << 13) | (((at >> 15) & 1) << 12) | \
                            (((at >> 14) & 1) << 11) | dyn_of[at & 0x1FF]
                        recs.append((y, 0x0500, attr, dxx))
                        continue
                    for (mx, my, w, h, tiles) in merge_sprite(run):
                        tile = obank.add_exact(tiles)
                        attr = (prio << 15) | (X["reserve_line"] << 13) | tile
                        recs.append((my, ((w - 1) << 10) | ((h - 1) << 8), attr, mx))
                MERGE_STATS[0] += len(kp)
                MERGE_STATS[1] += len(recs)
                kp = []
            elif X.get("no_hud") and len(kp) >= 4 and len({(at >> 9) & 7 for (x, y, at) in kp}) == 1 \
                    and len({(at >> 12) & 3 for (x, y, at) in kp}) == 1:
                # cinemáticas y menús: las piezas del mismo spritemap (misma paleta y prioridad) se juntan en sprites
                # más grandes (la Genesis en 256 de ancho tiene 64 sprites; el planeta Zebes solo son 50 piezas). Solo los
                # spritemaps grandes: en los chicos (asteroides) los tiles volteados ya se comparten entre piezas
                pal = (kp[0][2] >> 9) & 7
                glines = {SAMUS_PAL if pal == 4 else assign.get(("obj", pi + 1 + j), 0) for j in range(len(kp))}
                if len(glines) == 1:
                    line = glines.pop()
                    prio = 1 if ((kp[0][2] >> 12) & 3) == 3 else 0
                    run = []
                    pi0 = pi
                    for (x, y, at) in kp:
                        pi += 1
                        T, big = at & 0x1FF, bool(x & 0x8000)
                        dxx = x & 0x1FF
                        dxx = dxx - 0x200 if dxx >= 0x100 else dxx
                        n = 2 if big else 1
                        img = np.zeros((8 * n, 8 * n), np.int32)
                        for dx in range(n):
                            for dy in range(n):
                                img[dy * 8:dy * 8 + 8, dx * 8:dx * 8 + 8] = obj_tile_px(key_vram(key, vram), T, dx, dy)
                        if pal != 4:
                            img = np.vectorize(lambda c: slot_of[line][res(128 + pal * 16 + c)] if c else 0)(img)
                        if (at >> 14) & 1:
                            img = img[:, ::-1]
                        if (at >> 15) & 1:
                            img = img[::-1, :]
                        run.append((dxx, y, img.astype(np.int32)))
                    merged = merge_sprite(run)
                    if True:   # (cuesta tiles: las piezas compartidas entre spritemaps quedan repetidas; los sprites mandan)
                        for (mx, my, w, h, tiles) in merged:
                            tile = obank.add_exact(tiles)
                            recs.append((my, ((w - 1) << 10) | ((h - 1) << 8), (prio << 15) | (line << 13) | tile, mx))
                        kp = []
                    else:
                        pi = pi0                       # no conviene: quedan las piezas como están
            for (x, y, at) in kp:
                pi += 1
                T, pal, big = at & 0x1FF, (at >> 9) & 7, bool(x & 0x8000)
                rid = pal == rpal
                if rid:
                    line = X["reserve_line"]
                elif pal == 4:
                    line = SAMUS_PAL
                else:
                    line = assign[("obj", pi)] if ("obj", pi) in assign else 0
                pxs = []
                for dx in range(2 if big else 1):
                    for dy in range(2 if big else 1):
                        px = obj_tile_px(key_vram(key, vram), T, dx, dy)
                        if pal != 4 and not rid:
                            px = np.vectorize(lambda c: slot_of[line][res(128 + pal * 16 + c)] if c else 0)(px)
                        pxs.append(px.astype(np.int32))
                if rid and big and T in dyn_of:
                    tile = dyn_of[T]
                elif not any(p.any() for p in pxs):
                    continue
                else:
                    tile = obank.add_exact(pxs)
                prio = 1 if ((at >> 12) & 3) == 3 else 0
                attr = (prio << 15) | (line << 13) | (((at >> 15) & 1) << 12) | (((at >> 14) & 1) << 11) | tile
                dx = x & 0x1FF
                if dx >= 0x100:
                    dx -= 0x200
                recs.append((y, 0x0500 if big else 0, attr, dx))
            off = len(obj_recs) // 2
            obj_recs += rec_header(recs)
            for r in recs:
                obj_recs += struct.pack(">hHHh", *r)
            kind = {"enemy": 1, "eproj": 2, "a6": 3, "proj": 4, "front": 5, "a6s": 6, "enemyext": 7, "spr80": 8,
                    "spr8c": 9, "spr82": 10, "trail": 11}[key[0]]
            if key[0] in ("enemy", "enemyext"):
                a, b, c2 = int(key[2], 16), int(key[3], 16), int(key[4], 16)
            elif key[0] in ("eproj", "a6", "a6s", "spr80", "spr8c", "spr82"):
                a, b, c2 = int(key[1], 16), int(key[2], 16), 0
            elif key[0] in ("proj", "trail"):
                a, b, c2 = int(key[1], 16), 0, 0
            else:
                a, b, c2 = 0, 0, 0
            keys_c.append((kind, a, b, c2, off))
            rec_of[key] = off
        # parpadeo de Ridley (paleta 0 del SNES): los mismos registros; la Genesis cambia la línea reservada
        for key in flash_keys:
            nk = is_flash_key(rom, key)
            kind = {"enemy": 1, "a6": 3, "enemyext": 7}[key[0]]
            if key[0] in ("enemy", "enemyext"):
                a, b, c2 = int(key[2], 16), int(key[3], 16), int(key[4], 16)
            else:
                a, b, c2 = int(key[1], 16), int(key[2], 16), 0
            keys_c.append((kind | 0x80, a, b, c2, rec_of[nk]))
        # versiones de los tiles dinámicos ($B0, Ridley_Func_118/119), ya en el formato de la Genesis
        if X.get("dyn"):
            anim = []
            a, seen = 0xDA71, set()
            while a not in seen:
                seen.add(a)
                v = rom.w(0xA60000 | a)
                if v & 0x8000:
                    a = v
                    continue
                anim.append((rom.w(0xA60000 | (a + 2)), rom.w(0xA60000 | (a + 4))))
                a += 6
            claw = [(rom.w(0xA6DAD0), rom.w(0xA6DAD2)), (rom.w(0xA6DAD4), rom.w(0xA6DAD6))]
            for g in X["dyn"]:
                pairs = sorted(set(anim)) if g["rows"][0] == 0x7220 else claw
                col = g.get("col", 0)
                for s1, s2 in pairs:
                    data = b""
                    for c in range(2):                 # columna del sprite de 2x2: fila de arriba, fila de abajo
                        for src in (s1, s2):
                            b32 = rom.bytes(0xB00000 | (src + (col + c) * 32), 32)
                            data += genesis_tile_bytes(snes_tile4(b32))
                    DYN.append((g["rows"][0], s1, g["genesis_tile"], data))
        total = nbg_tiles + len(obank.tiles)
        assert TB + total <= TILE_LIMIT, total
        if 0xC000 <= R["id"] < 0xC100:          # cinemáticas: después de sus tiles, los de la capa del modo 7 (mkm7.py)
            Hh.append(f"#define CINE_TILE_FIRST_{R['id']:04X} {TB + total}")
        if rid_first is not None:
            Hh.append(f"#define RIDLEY_TILE_FIRST_{R['addr']:04X} {rid_first}   /* tiles solo de Ridley: libres en la huida */")
            Hh.append(f"#define RIDLEY_TILE_END_{R['addr']:04X} {TB + total}")
        sym = f"{R['id']:04X}"
        binfile(f"kRoomTiles_{sym}", bank.data() + obank.data())
        binfile(f"kRoomPlaneA_{sym}", struct.pack(f">{len(pa)}H", *pa))
        if pb:
            binfile(f"kRoomPlaneB_{sym}", struct.pack(f">{len(pb)}H", *pb))
        # CRAM: índice de palette_buffer por color (0xFF = negro fijo)
        src = [0xFF] * 64
        src[0] = 0
        for l, m in slot_of.items():
            for i, si in m.items():
                src[l * 16 + si] = i
        for k in range(1, 16):
            src[SAMUS_PAL * 16 + k] = 192 + k
        fade = sorted({i for i in src if i != 0xFF} | set(range(192, 208)) | set(range(224, 240)))
        carray("uint8", f"kFade_{sym}", fade, fmt="0x%02X")
        # ordenadas por (tipo, a, b, c): la Genesis las busca por bisección
        keys_c.sort(key=lambda k: (k[0] & 0x7F, k[1], k[2], k[3]))
        C.append(f"static const SprKey kKeys_{sym}[{max(1, len(keys_c))}] = {{")
        for kk in keys_c:
            C.append("  { %u, 0x%04X, 0x%04X, 0x%04X, %u }," % kk)
        if not keys_c:
            C.append("  { 0, 0, 0, 0, 0 },")
        C.append("};")
        # tabla de dispersión de las claves (la Genesis busca cada dibujo en ~1 intento): índice de la clave, 0xFFFF libre
        hsize = 64
        while hsize < 2 * len(keys_c):
            hsize *= 2
        htab = [0xFFFF] * hsize
        for ki, kk in enumerate(keys_c):
            h = key_hash(kk[0] & 0x7F, kk[1], kk[2], kk[3]) & (hsize - 1)
            while htab[h] != 0xFFFF:
                h = (h + 1) & (hsize - 1)
            htab[h] = ki
        carray("uint16", f"kKeyHash_{sym}", htab)
        if patches:
            carray("uint16", f"kPatch_{sym}", [v for p in patches for v in p])
        carray("uint8", f"kSrc_{sym}", src, fmt="0x%02X")
        bmode = 0 if pb is None else (1 if R["lb_lib"] else 2)
        obj_line = 3
        flash_line = X.get("reserve_line", 0) or 0
        door_sm = X["door"]["sm"] if X.get("door") else 0
        rooms_c.append("  { 0x%04X, 0x%04X, %u, %u, %u, %u, kRoomTiles_%s, kRoomPlaneA_%s, %s, %u, kSrc_%s, kKeys_%s, %u, %s, %u, %u, kFade_%s, %u, %u, %u, 0x%04X, kKeyHash_%s, 0x%X, %s, %u }," % (
            R["addr"], R["state"], pw, ph, nbg_tiles, total, sym, sym, f"kRoomPlaneB_{sym}" if pb else "0", bmode,
            sym, sym, len(keys_c), f"kPatch_{sym}" if patches else "0", len(patches), obj_line, sym, len(fade),
            flash_line, X.get("flash_base", 0), door_sm, sym, hsize - 1, f"kWall_{sym}" if wall_w else "0",
            X["free_line"] if X.get("free_line") is not None else 0xFF))
        Hh.append(f"extern const uint8 kRoomTiles_{sym}[];")
        Hh.append(f"extern const uint16 kRoomPlaneA_{sym}[];")
        if pb:
            Hh.append(f"extern const uint16 kRoomPlaneB_{sym}[];")
        if MERGE_STATS[0]:
            print(f"sala {sym}: piezas juntadas {MERGE_STATS[0]} -> {MERGE_STATS[1]} sprites")
        print(f"sala {sym}: plano {pw}x{ph}, colores por línea {[len(bins[l]) for l in lines]}, "
              f"{nmerge} fundidos, tiles {nbg_tiles}+{len(obank.tiles)}, parches {len(patches)}")
    # texto de la huida: palabra del SNES -> celda del plano A (ordenadas por palabra)
    carray("uint16", "kEscText", [v for p in sorted(ESC_TEXT) for v in p] or [0, 0])
    Hh.append(f"#define ESC_TEXT_N {len(ESC_TEXT)}")
    Hh.append("extern const uint16 kEscText[];  /* pares (palabra del BG1 del SNES, celda del plano A) */")
    binfile("kObjRecs", bytes(obj_recs))
    # tiles dinámicos (Ridley): al pedir el juego la copia (dst, src) a la VRAM, la Genesis copia esos 4 tiles
    binfile("kDynTiles", b"".join(d[3] for d in DYN) or bytes(2))
    C.append(f"const DynTiles kDyn[{max(1, len(DYN))}] = {{")
    for i, (dst, src, tile, _) in enumerate(DYN):
        C.append("  { 0x%04X, 0x%04X, %u, %u }," % (dst, src, tile, i * 128))
    if not DYN:
        C.append("  { 0, 0, 0, 0 },")
    C.append("};")
    Hh.append("typedef struct { uint16 dst, src, tile, off; } DynTiles;   /* VRAM del SNES (palabras), origen $B0, tile Genesis, desplazamiento en kDynTiles */")
    Hh.append(f"#define DYN_COUNT {len(DYN)}")
    Hh.append("extern const DynTiles kDyn[];")
    Hh.append("extern const uint8 kDynTiles[];")
    Hh.append("extern const uint16 kObjRecs[];   /* n, y mín., y máx., x mín., x máx., luego n x (dy, tamaño, atributo, dx) */")
    Hh.append("typedef struct { uint8 kind; uint16 a, b, c; uint16 rec; } SprKey;   /* kind: 1 enemigo, 2 proy. enemigo, 3 $A6, 4 rayo, 5 Samus de frente, 6 $A6 en pantalla; +0x80: parpadeo de Ridley */")
    Hh.append("typedef struct { uint16 room, state, plane_w, plane_h, bg_tiles, tiles_total; const uint8 *tiles; const uint16 *planeA, *planeB;\n"
              "  uint16 b_mode; const uint8 *src; const SprKey *keys; uint16 nkeys; const uint16 *patch; uint16 npatch, obj_line; const uint8 *fade; uint16 nfade;\n"
              "  uint16 flash_line, flash_base, door_sm; const uint16 *khash; uint16 khash_mask; const uint16 *wall; uint8 free_line; } MdRoom;   /* flash_line: línea de Ridley en la CRAM (parpadeo: colores desde flash_base); door_sm: puerta que pasa al fondo; wall: pozo de modo 7 (celdas de la pared); free_line: línea de la CRAM que pone la Genesis (0xFF: ninguna) */")
    Hh.append(f"#define MD_ROOM_COUNT {len(rooms_c)}")
    Hh.append("extern const MdRoom kMdRooms[MD_ROOM_COUNT];")
    C.append(f"const MdRoom kMdRooms[{len(rooms_c)}] = {{")
    C += rooms_c
    C.append("};")


def write_out(out_dir, C, Hh, bins):
    h = ["/* Generado por tools/conv/mkmd.py (docs/32): datos de la Genesis ya convertidos. No editar. */",
         "#ifndef MD_DATA_H", "#define MD_DATA_H", "#include <stdint.h>",
         "typedef uint8_t uint8; typedef uint16_t uint16; typedef int16_t int16;",
         f"#define HUD_BASE {HUD_BASE}", f"#define SAMUS_BASE {SAMUS_BASE}", f"#define BG_BASE {BG_BASE}", f"#define CINE_BG_BASE {CINE_BG_BASE}",
         f"#define SAMUS_PAL {SAMUS_PAL}"]
    h += [l.replace("\\n", "\n") for l in Hh]
    h.append("#endif")
    (out_dir / "md_data.h").write_text("\n".join(h).replace("\\n", "\n") + "\n")
    c = ["/* Generado por tools/conv/mkmd.py (docs/32). No editar. */", '#include "md_data.h"'] + C
    (out_dir / "md_data.c").write_text("\n".join(c) + "\n")
    s = ["/* Generado por tools/conv/mkmd.py: bloques grandes (big-endian, alineados) */"]
    for sym, f in bins:
        s += ["    .section .rodata", "    .align 2", f"    .global {sym}", f"{sym}:", f'    .incbin "bins/{f}"']
    (out_dir / "md_bins.s").write_text("\n".join(s) + "\n")


if __name__ == "__main__":
    main()
