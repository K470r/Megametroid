#!/usr/bin/env python3
"""Genera los datos de la secuencia de título para la Genesis (tests/samus_play, title.c).

Entrada:
  sm.smc          tu ROM (no se distribuye)
  extraido/       salida de tools/sm_extract (blobs descomprimidos)
  title.rec       grabación cuadro a cuadro del SNES real (tools/snes_ref/record_title.py)
Salida (outdir): title_data.s (+ bins/) y title_data.c

Idea: en la Genesis no se calcula nada del modo 7. La grabación del SNES da, por cuadro, el brillo,
la cámara de modo 7 (zoom var6, scroll var8/var10), la matemática de color, los sprites (OAM) y la
paleta. Con eso:
  * Las 3 tomas con acercamiento fijo (zoom 72, 96, 96) se pre-renderizan como una imagen ancha (o
    alta) con el mismo muestreo del modo 7; en la Genesis solo se desplaza el plano.
  * El alejamiento (zoom 67 -> 256) usa 16 imágenes pre-renderizadas ("tamaños"), alternando dos
    bancos de VRAM para cargar la siguiente mientras se ve la actual.
  * Los sprites (logo de arranque, letras rojas, logo del título, (c)) se convierten del OAM grabado.
    Logos: "Síndrome" en lugar de "Nintendo" y "MEGA METROID" en lugar de "SUPER METROID"
    (title_logos/, del proyecto metroid_genesis).
  * La paleta se reproduce desde la grabación (paletas animadas de las pantallas del laboratorio,
    brillo de los fundidos, oscurecimiento de las tomas 1 y 3, degradado del título por línea).
"""
import argparse, json, os, struct, sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "title_logos"))
sys.path.insert(0, str(HERE.parent / "snes_ref"))
from convert_rooms import snes_to_genesis, genesis_tile_bytes, pack_palettes, color_dist, quant_channel  # noqa: E402
from record_title import offsets, REC  # noqa: E402

V_MIN, V_MAX = 67, 256
# Carga de un tamaño en la Genesis (title.c): FRAME_BYTES por cuadro entre tiles y mapa (dos mitades
# de 14 filas x 64 celdas), más 1 cuadro de latencia. Con eso se elige cuántos tamaños caben sin
# perder cuadros (tantos como se puedan cargar mientras se ve el anterior).
FRAME_BYTES = 3200
MAP_HALF_BYTES = 14 * 64 * 2
# Velocidad del alejamiento respecto del SNES (1.0 = igual; el SNES tarda ~378 cuadros).
ZOOM_SPEED = 1.4
TILE_BASE = 16           # TILE_USER_INDEX de SGDK (fuente al final de la VRAM; title.c lo comprueba al compilar)
BANK_TILES = 576
# funciones de cinemática del SNES (dirección en el banco $8B) que identifican cada toma
CF_SHOT = {0x9D17: 1, 0x9D90: 2, 0x9E12: 3}
TITLE_FRAME = 1850            # cuadro con el logo y el (c) completos: destino al saltar la intro
# Logo de arranque (Síndrome): cuadros de más que se queda encendido antes de fundir a negro. Se
# toman del negro que sigue, así las letras rojas y todo lo demás salen en el mismo cuadro de siempre.
LOGO_HOLD_EXTRA = 90
LOGO_BLACK_MIN = 40           # negro que se deja entre el logo y lo siguiente


def lorom(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


# ------------------------------------------------------------------ modo 7
class Mode7:
    def __init__(self, ext):
        d = Path(ext) / "blobs" / "decompressed"
        t = next(d.glob("94E000_*.bin")).read_bytes()
        m = next(d.glob("96FC04_*.bin")).read_bytes()
        self.map = np.frombuffer(m[:0x1000], np.uint8).reshape(32, 128).astype(int)
        self.tiles = np.frombuffer(t[:0x4000], np.uint8).reshape(256, 8, 8).astype(int)

    def px(self, px, py):
        ok = (px >= 0) & (px < 1024) & (py >= 0) & (py < 256)
        pxc, pyc = np.clip(px, 0, 1023), np.clip(py, 0, 255)
        return np.where(ok, self.tiles[self.map[pyc >> 3, pxc >> 3], pyc & 7, pxc & 7], 0)

    def render(self, v, x0, x1, y0, y1):
        """Imagen en coordenadas pantalla + scroll (X = x + BG1HOFS), zoom v, centro (128,128)."""
        Y, X = np.mgrid[y0:y1, x0:x1]
        return self.px((128 * 256 + v * (X - 128)) >> 8, (128 * 256 + v * (Y - 128)) >> 8)


# ------------------------------------------------------------------ grabación
class Rec:
    def __init__(self, path):
        self.d = Path(path).read_bytes()
        self.n = len(self.d) // REC
        self.o = offsets()

    def f(self, i, name):
        o, s = self.o[name]
        return self.d[i * REC + o:i * REC + o + s]

    def u16(self, i, name):
        return struct.unpack("<H", self.f(i, name)[:2])[0]

    def s16(self, i, name):
        return struct.unpack("<h", self.f(i, name)[:2])[0]

    def pal(self, i):
        return struct.unpack("<256H", self.f(i, "pal"))

    def remap(self, src):
        """Rehace la grabación con el cuadro src[i] en el lugar i (mismo largo)."""
        self.d = b"".join(self.d[j * REC:(j + 1) * REC] for j in src)


def speed_up_zoom(rec, speed):
    """Acorta el alejamiento (zoom 67 -> 256) a 1/speed de su duración; lo que sigue se adelanta.
    Devuelve cuántos cuadros se quitaron."""
    z0 = next(i for i in range(rec.n) if rec.u16(i, "cin_func") == 0x9E8B)
    z1 = next(i for i in range(z0, rec.n) if rec.u16(i, "var6") >= V_MAX)
    L = z1 - z0
    L2 = max(1, round(L / speed))
    src = list(range(z0)) + [z0 + min(L - 1, round(k * L / L2)) for k in range(L2)] + list(range(z1, rec.n))
    rec.remap(src)
    rec.n = len(src)
    return L - L2


def hold_boot_logo(rec, extra):
    """Deja el logo de arranque encendido `extra` cuadros más, comiéndose el negro que le sigue."""
    bright = [0 if rec.f(i, "inidisp")[0] & 0x80 else rec.f(i, "inidisp")[0] & 15 for i in range(rec.n)]
    full = next(i for i in range(rec.n) if bright[i] == 15)               # logo encendido
    fade = next(i for i in range(full, rec.n) if bright[i] < 15)          # empieza a fundir
    black = next(i for i in range(fade, rec.n) if bright[i] == 0)         # ya negro
    nxt = next(i for i in range(black, rec.n) if bright[i] > 0)           # lo que sigue (laboratorio)
    extra = max(0, min(extra, nxt - black - LOGO_BLACK_MIN))
    src = list(range(rec.n))
    for i in range(fade, fade + extra):
        src[i] = fade - 1
    for i in range(fade + extra, black + extra):
        src[i] = i - extra
    rec.remap(src)
    return full, fade, fade + extra, black + extra, nxt


# ------------------------------------------------------------------ fondo: imagen -> tiles Genesis
def convert_scenes(images, rec, frames_by_scene, npal, fixed=None):
    """images: {id: matriz de índices SNES (0 = transparente)} que comparten un mismo reparto de
    paletas. Devuelve (reparto slot->índice SNES [npal][16], {id: (tiles, mapa)}, fusiones).
    fixed: {(id, ty, tx): [bloque 8x8 por cuadro de animación]}: esas celdas llevan un tile propio
    (sin compartir) y se devuelven sus variantes en out[('anim', id)] = (primer tile, [bytes por cuadro])."""
    fixed = fixed or {}
    # identidad de cada índice: sus colores Genesis en los cuadros donde se ven estas imágenes
    fr = sorted(set(f for i in images for f in frames_by_scene.get(i, [])))[::2] or [rec.n - 1]
    pals = np.array([rec.pal(f) for f in fr])
    ident = {}
    for c in range(1, 128):
        ident[c] = tuple(snes_to_genesis(int(v), "dac") for v in pals[:, c])
    # un representante por identidad
    rep = {}
    for c in range(1, 128):
        rep.setdefault(ident[c], c)
    canon = {c: rep[ident[c]] for c in range(1, 128)}
    items = []
    for sid, img in images.items():
        H, W = img.shape
        for ty in range(0, H, 8):
            for tx in range(0, W, 8):
                s = {canon[c] for c in np.unique(img[ty:ty + 8, tx:tx + 8]).tolist() if c}
                for blk in fixed.get((sid, ty, tx), []):
                    s |= {canon[c] for c in np.unique(blk).tolist() if c}
                if s:
                    items.append(((sid, ty, tx), s))
    merges = {}
    def m(c):
        while c in merges:
            c = merges[c]
        return c
    bins = pack_palettes(items, npal)
    while bins is None:
        allc = sorted(set().union(*[{m(c) for c in cs} for _, cs in items]))
        base = {c: snes_to_genesis(int(pals[-1][c]), "dac") for c in allc}
        best = min((color_dist(base[a], base[b]) + (8 if len(set(ident[a])) > 1 or len(set(ident[b])) > 1 else 0), a, b)
                   for i, a in enumerate(allc) for b in allc[i + 1:])
        merges[best[2]] = best[1]
        bins = pack_palettes([(k, {m(c) for c in cs}) for k, cs in items], npal)
    slot = {}                                       # (paleta, índice SNES canónico) -> slot
    table = [[0xFF] * 16 for _ in range(npal)]
    where = {}
    for p, (cs, keys) in enumerate(bins):
        for k in keys:
            where[k] = p
        for i, c in enumerate(sorted(cs)):
            slot[(p, c)] = i + 1
            table[p][i + 1] = c
    out = {}
    def to_px(blk, p):
        px = np.zeros((8, 8), np.uint8)
        pal_cols = [(k, snes_to_genesis(int(pals[-1][table[p][k]]), "dac")) for k in range(1, 16) if table[p][k] != 0xFF]
        for (y, x), c in np.ndenumerate(blk):
            if c:
                cc = m(canon[c])
                if (p, cc) in slot:
                    px[y, x] = slot[(p, cc)]
                else:                                  # color que no quedó en la paleta: el más parecido
                    g = snes_to_genesis(int(pals[-1][c]), "dac")
                    px[y, x] = min(pal_cols, key=lambda kc: color_dist(kc[1], g))[0]
        return px
    for sid, img in images.items():
        H, W = img.shape
        tiles, tindex, mp = [bytes(32)], {bytes(32): 0}, []
        anim = [c for c in fixed if c[0] == sid]
        if anim:
            first = len(tiles)
            nfr = len(fixed[anim[0]])
            frames_b = [[] for _ in range(nfr)]
            for (s_, ty, tx) in sorted(anim):
                p = where[(sid, ty, tx)]
                for fi, blk in enumerate(fixed[(s_, ty, tx)]):
                    frames_b[fi].append(genesis_tile_bytes(to_px(blk, p)))
            # los tiles propios: el cuadro 0 queda en la imagen
            for k_ in frames_b[0]:
                tiles.append(k_)
            out[("anim", sid)] = (first, [b"".join(f) for f in frames_b])
            anim_slot = {c: first + i for i, c in enumerate(sorted(anim))}
        else:
            anim_slot = {}
        for ty in range(0, H, 8):
            row = []
            for tx in range(0, W, 8):
                blk = img[ty:ty + 8, tx:tx + 8]
                if (sid, ty, tx) in anim_slot:
                    row.append((where[(sid, ty, tx)] << 13) | anim_slot[(sid, ty, tx)])
                    continue
                if not blk.any():
                    row.append(0)
                    continue
                p = where[(sid, ty, tx)]
                px = np.zeros((8, 8), np.uint8)
                px = to_px(blk, p)
                found = None
                for v, fh, fv in ((px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1], 0, 1), (px[::-1, ::-1], 1, 1)):
                    k = genesis_tile_bytes(v)
                    if k in tindex:
                        found = (tindex[k], fh, fv)
                        break
                if not found:
                    k = genesis_tile_bytes(px)
                    tindex[k] = len(tiles)
                    tiles.append(k)
                    found = (tindex[k], 0, 0)
                t, fh, fv = found
                row.append((p << 13) | (fv << 12) | (fh << 11) | t)
            mp.append(row)
        out[sid] = (b"".join(tiles), mp)
    return table, out, len(merges)


# ------------------------------------------------------------------ sprites
def oam_sprites(o):
    spr = []
    for i in range(128):
        y = o[i * 4 + 1]
        hi = (o[512 + (i >> 2)] >> ((i & 3) * 2)) & 3
        x = o[i * 4] | (hi & 1) << 8
        if x >= 256:
            x -= 512
        big = hi >> 1
        if y >= 0xE0 or x <= -16 or x >= 256:
            continue
        fl = o[i * 4 + 3]
        spr.append((x, y, o[i * 4 + 2] | (fl & 1) << 8, (fl >> 1) & 7, big, (fl >> 6) & 1, (fl >> 7) & 1))
    return tuple(spr)


def snes_obj_tile(sheet, t):
    b = sheet[t * 32:t * 32 + 32]
    px = np.zeros((8, 8), np.uint8)
    for y in range(8):
        p = (b[2 * y], b[2 * y + 1], b[16 + 2 * y], b[17 + 2 * y])
        for x in range(8):
            s = 7 - x
            px[y, x] = sum(((p[k] >> s) & 1) << k for k in range(4))
    return px


# ------------------------------------------------------------------ degradado del título (HDMA)
def gradient_lines(rom, table_idx):
    """Color (r, g, b) de 5 bits sumado al fondo en cada línea: tabla $8C:BC5D[i], registro COLDATA."""
    off = struct.unpack_from("<16H", rom, lorom(0x8CBC5D))[table_idx]
    a = 0x8C0000 | off
    rgb = [0, 0, 0]
    lines = []
    while len(lines) < 224:
        cnt, val = rom[lorom(a)], rom[lorom(a) + 1]
        a += 2
        if cnt == 0:
            break
        for ch in range(3):
            if val & (0x20 << ch):
                rgb[ch] = val & 31
        lines += [tuple(rgb)] * cnt
    lines += [tuple(rgb)] * (224 - len(lines))
    return lines[:224]


# ------------------------------------------------------------------ main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("extracted"); ap.add_argument("rec"); ap.add_argument("out")
    ap.add_argument("--proj", default=None)
    a = ap.parse_args()
    rom = Path(a.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    ext, out = Path(a.extracted), Path(a.out)
    proj = Path(a.proj) if a.proj else out.parent
    bins = out / "bins"
    bins.mkdir(parents=True, exist_ok=True)
    for old in bins.glob("ttl_*.bin"):
        old.unlink()
    rec = Rec(a.rec)
    logo = hold_boot_logo(rec, LOGO_HOLD_EXTRA)
    zcut = speed_up_zoom(rec, ZOOM_SPEED)          # cuadros quitados: lo que sigue al alejamiento se adelanta
    title_frame = TITLE_FRAME - zcut
    m7 = Mode7(ext)
    asm = ["/* Generado por tools/genesis_convert/make_title_data.py: no editar. */", "    .section .rodata_title,\"a\"", ""]
    c = []
    decl = ["/* Generado por tools/genesis_convert/make_title_data.py: no editar. */", '#include "title.h"', ""]

    def incbin(sym, data, align=2):
        (bins / f"{sym}.bin").write_bytes(data)
        rel = os.path.relpath(bins / f"{sym}.bin", proj).replace("\\", "/")
        asm.extend([f"    .align {align}", f"    .global {sym}", f"{sym}:", f'    .incbin "{rel}"'])
        decl.append(f"extern const u8 {sym}[];")

    # --- fin: cuando el SNES deja el título (sin tocar nada, a los 900 cuadros van las demos) y
    # termina de fundir a negro; ahí la Genesis vuelve a empezar la secuencia
    N = rec.n
    for i in range(1882 - zcut, rec.n):
        if rec.u16(i, "cin_func") not in (0x9F28, 0x9F29):
            N = i
            while N < rec.n and (rec.f(N, "inidisp")[0] & 15) and not (rec.f(N, "inidisp")[0] & 0x80):
                N += 1
            break
    # --- tomas y zona del alejamiento
    base_scene = []
    for i in range(N):
        ini, tm = rec.f(i, "inidisp")[0], rec.f(i, "tm")[0]
        cf = rec.u16(i, "cin_func")
        s = 0
        if not (ini & 0x80) and (tm & 1):
            s = CF_SHOT.get(cf, 0) or (-1 if i >= 1400 else 0)      # -1: alejamiento / título
        base_scene.append(s)
    zoom = [i for i in range(N) if base_scene[i] == -1]
    z0 = zoom[0]
    z_end = next(i for i in zoom if rec.u16(i, "var6") >= V_MAX)       # zoom 256: pantalla de título
    var6 = {i: rec.u16(i, "var6") for i in zoom}

    # --- elección de tamaños: tantos como se puedan cargar mientras se ve el anterior
    def ntiles(v):
        img = m7.render(v, 0, 256, 0, 224)
        seen = set()
        for ty in range(0, 224, 8):
            for tx in range(0, 256, 8):
                blk = img[ty:ty + 8, tx:tx + 8]
                ks = (blk.tobytes(), blk[:, ::-1].tobytes(), blk[::-1].tobytes(), blk[::-1, ::-1].tobytes())
                if not any(k in seen for k in ks):
                    seen.add(ks[0])
        return len(seen) + 1
    tcache = {}
    def load_frames(v):
        """Cuadros hasta poder mostrar el tamaño (misma cuenta que prep_job en title.c)."""
        if v not in tcache:
            tcache[v] = ntiles(v)
        left, halves, f = tcache[v], 2, 0
        while halves:
            f += 1
            b = FRAME_BYTES
            n = min(left, b // 32); left -= n; b -= n * 32
            while not left and halves and b >= MAP_HALF_BYTES:
                halves -= 1; b -= MAP_HALF_BYTES
        return f + 1
    keys = [(z0, var6[z0])]                 # (primer cuadro, zoom)
    while keys[-1][1] < V_MAX:
        t = keys[-1][0]
        nt = t + 8
        for _ in range(6):                  # el tiempo de carga depende del tamaño elegido
            nt2 = t + load_frames(var6[min(nt, z_end)]) + 1
            if nt2 == nt:
                break
            nt = nt2
        if nt >= z_end:
            keys.append((z_end, V_MAX))
            break
        keys.append((nt, var6[nt]))
    NKF = len(keys)
    scene_of = []
    for i in range(N):
        s = base_scene[i]
        if s == -1:
            s = 4 + max(j for j in range(NKF) if keys[j][0] <= i)
        scene_of.append(s)
    frames_by_scene = {}
    for i, s in enumerate(scene_of):
        frames_by_scene.setdefault(s, []).append(i)

    # --- imágenes de fondo
    shots = {}
    geo = {}
    for s in (1, 2, 3):
        fr = frames_by_scene[s]
        v = rec.u16(fr[0], "var6")
        hs = [rec.s16(f, "var8") for f in fr]
        vs = [rec.s16(f, "var10") for f in fr]
        x0, x1 = min(hs), max(hs) + 256
        y0, y1 = min(vs), max(vs) + 224
        x1 = x0 + ((x1 - x0 + 7) & ~7)
        y1 = y0 + ((y1 - y0 + 7) & ~7)
        shots[s] = m7.render(v, x0, x1, y0, y1)
        geo[s] = (x0, y0)
    kf = {4 + k: m7.render(v, 0, 256, 0, 224) for k, (t, v) in enumerate(keys)}
    scene_data, scene_tab = {}, {}
    info = []
    for s in (1, 2, 3):
        tab, d, nm = convert_scenes({s: shots[s]}, rec, frames_by_scene, 4)
        scene_data.update(d); scene_tab[s] = tab
        info.append(f"toma {s}: {len(d[s][0]) // 32} tiles, {shots[s].shape[1]}x{shots[s].shape[0]}, fusiones {nm}")
    # Bebé metroide de la pantalla de título: objeto de modo 7 que reescribe los tiles 224-227 del
    # plano con 3 cuadros de $95:A5E1 (transferencias $8B:A337/A341/A34B). A zoom 256 cada tile del
    # plano es una celda de la pantalla: esas 4 celdas llevan tiles propios y sus 3 variantes.
    last = 4 + NKF - 1
    baby_blob = next((ext / "blobs" / "decompressed").glob("95A5E1_*.bin")).read_bytes()
    fixed = {}
    for ty in range(28):
        for tx in range(32):
            t = int(m7.map[ty, tx])
            if 224 <= t <= 227:
                fixed[(last, ty * 8, tx * 8)] = [np.frombuffer(baby_blob[f * 256 + (t - 224) * 64:f * 256 + (t - 223) * 64],
                                                               np.uint8).reshape(8, 8).astype(int) for f in range(3)]
    # cada tamaño con su propio reparto de paletas (se ven de a uno); durante el alejamiento no hay
    # sprites, así que usan las 4 líneas; el último (título, con el logo) deja PAL3 para los sprites
    tot_m = 0
    baby = None
    for s_, img in kf.items():
        npal = 3 if s_ == last else 4
        tab, d, nm = convert_scenes({s_: img}, rec, {s_: frames_by_scene.get(s_, [])}, npal,
                                    fixed if s_ == last else None)
        if ("anim", s_) in d:
            baby = d.pop(("anim", s_))
        scene_data.update(d)
        scene_tab[s_] = tab + [[0xFF] * 16] * (4 - npal)
        tot_m += nm
    kf_bytes = sum(len(scene_data[s][0]) + 28 * 64 for s in kf)
    info.append(f"alejamiento: {NKF} tamaños (cada ~{(z_end - z0) // max(1, NKF - 1)} cuadros), tiles máx. "
                f"{max(len(scene_data[s][0]) // 32 for s in kf)}, fusiones {tot_m}, {kf_bytes // 1024} KB")
    assert max(len(scene_data[s][0]) // 32 for s in kf) <= BANK_TILES
    # --- escenas: tiles, mapa, tabla de paletas
    NS = 4 + NKF
    c.append(f"const u16 g_title_num_scenes = {NS};")
    c.append(f"const u16 g_title_tile_base = {TILE_BASE};")
    c.append(f"const TitleScene g_title_scenes[{NS}] = {{")
    rows_c = []
    for s in range(NS):
        if s not in scene_data:
            rows_c.append("    { 0 },")
            continue
        tiles, mp = scene_data[s]
        H, W = len(mp), len(mp[0])
        incbin(f"ttl_tiles_{s}", tiles)
        colmajor = s in (1, 2)                     # tomas con paneo horizontal: se dibujan por columnas
        if colmajor:
            incbin(f"ttl_map_{s}", b"".join(struct.pack(f">{H}H", *[mp[y][x] for y in range(H)]) for x in range(W)))
        elif s >= 4:
            # tamaños: 28 filas x 64 celdas (el ancho del plano, así cada mitad va en una sola
            # transferencia desde la ROM) con la base de tiles de su banco ya sumada; las celdas vacías
            # apuntan al tile 0 de la escena (vacío) y las columnas 32-63 quedan en 0
            base = TILE_BASE + ((s - 4) & 1) * BANK_TILES
            incbin(f"ttl_map_{s}", b"".join(struct.pack(">64H", *([(w + base) if w & 0x7FF else (w | base) for w in r[:32]] + [0] * 32))
                                            for r in mp[:28]))
        else:
            incbin(f"ttl_map_{s}", b"".join(struct.pack(f">{W}H", *r) for r in mp))
        incbin(f"ttl_pal_{s}", bytes(v for p in scene_tab[s] for v in p))
        x0, y0 = geo.get(s, (0, 0))
        rows_c.append(f"    {{ {len(tiles) // 32}, {W}, {H}, {x0}, {y0}, {int(colmajor)}, (const u32 *) ttl_tiles_{s}, "
                      f"(const u16 *) ttl_map_{s}, ttl_pal_{s} }},")
    c += rows_c + ["};", ""]
    if baby:
        incbin("ttl_baby", b"".join(baby[1]))
        c.append(f"const u16 g_title_baby_scene = {last}, g_title_baby_tile = {baby[0]}, "
                 f"g_title_baby_ntiles = {len(baby[1][0]) // 32};")
    else:
        incbin("ttl_baby", bytes(32))
        c.append("const u16 g_title_baby_scene = 0xFFFF, g_title_baby_tile = 0, g_title_baby_ntiles = 0;")
    BABY_T = {0xA337: 0, 0xA341: 1, 0xA34B: 2}

    # --- hojas de sprites: logos modificados
    import logo_tool as Lt, logo_sindrome as LS, logo_mega as LM
    sheet = bytearray(next((ext / "blobs" / "decompressed").glob("9580D8_*.bin")).read_bytes())
    lay = json.loads((HERE / "title_logos" / "logo_layout.json").read_text())
    for name, builder in (("nintendo", lambda i: LS.build(i)[0]), ("title", LM.build)):
        Lt.recut(sheet, lay[name], builder(Lt.compose(sheet, lay[name])))
    # --- conjuntos de sprites
    sets, set_of = {}, []
    for i in range(N):
        ini = rec.f(i, "inidisp")[0]
        sp = () if ini & 0x80 else oam_sprites(rec.f(i, "oam"))
        set_of.append(sets.setdefault(sp, len(sets)))
    gtiles, gidx = [], {}
    def gtile(t, big, remap=None):
        k = (t, big, remap)
        if k not in gidx:
            gidx[k] = len(gtiles)
            lut = np.array(remap if remap else range(16), np.uint8)
            if big:
                cells = [(0, 0), (0, 1), (1, 0), (1, 1)]          # columnas: arriba-izq, abajo-izq, ...
                for cx, cy in cells:
                    tt = (t & 0x100) | ((((t >> 4) & 15) + cy) & 15) << 4 | (((t & 15) + cx) & 15)
                    gtiles.append(genesis_tile_bytes(lut[snes_obj_tile(sheet, tt)]))
            else:
                gtiles.append(genesis_tile_bytes(lut[snes_obj_tile(sheet, t)]))
        return gidx[k]
    # Una sola línea de paleta para sprites (PAL3): los fondos usan las otras 3. Si un conjunto usa
    # dos paletas del SNES (logo + (c)), los píxeles de la segunda se pasan al color más parecido de
    # la primera (con la paleta del título ya encendida).
    final_pal = rec.pal(N - 1)
    def remap_for(pa, pb):
        ca = [snes_to_genesis(final_pal[128 + 16 * pa + i], "dac") for i in range(16)]
        out_ = [0]
        for i in range(1, 16):
            cb = snes_to_genesis(final_pal[128 + 16 * pb + i], "dac")
            out_.append(min(range(1, 16), key=lambda j: color_dist(ca[j], cb)))
        return tuple(out_)
    set_recs = bytearray()
    set_off = []
    for sp in sorted(sets, key=lambda s: sets[s]):
        pals = []
        for e in sp:
            if e[3] not in pals:
                pals.append(e[3])
        assert len(pals) <= 2, pals
        set_off.append(len(set_recs))
        set_recs += struct.pack(">HBB", len(sp), 128 + 16 * pals[0] if pals else 0xFF, 0xFF)
        for (x, y, t, p, big, hf, vf) in sp:
            gt = gtile(t, big, remap_for(pals[0], p) if p != pals[0] else None)
            size = 0x05 if big else 0x00
            attr = (3 << 13) | (vf << 12) | (hf << 11) | gt
            set_recs += struct.pack(">hBBH h", y + 129, size, 0, attr, x + 128)
    incbin("ttl_spr_tiles", b"".join(gtiles))
    incbin("ttl_spr_sets", bytes(set_recs))
    c.append(f"const u16 g_title_spr_tiles = {len(gtiles)};")
    c.append("const u16 g_title_spr_set_off[] = {" + ", ".join(map(str, set_off)) + "};")
    info.append(f"sprites: {len(sets)} conjuntos, {len(gtiles)} tiles")

    # --- degradado: 16 tablas x 224 líneas (color de 5 bits empaquetado BGR555)
    gpath = Path(a.rec + ".grad")
    if gpath.exists():                               # medido en el SNES (con el CGADSUB por línea)
        grads = gpath.read_bytes()
    else:                                            # solo la tabla de COLDATA
        grads = b""
        for t in range(16):
            for (r, g, b) in gradient_lines(rom, t):
                grads += struct.pack(">H", r | g << 5 | b << 10)
    # ya como colores de CRAM de la Genesis (la interrupción los copia tal cual)
    g5 = struct.unpack(f">{len(grads) // 2}H", grads)
    grads = struct.pack(f">{len(g5)}H", *[(quant_channel((c >> 10) & 31, "dac") << 9) | (quant_channel((c >> 5) & 31, "dac") << 5)
                                         | (quant_channel(c & 31, "dac") << 1) for c in g5])
    incbin("ttl_grad", grads)

    # --- línea de tiempo + cambios de paleta
    tl = bytearray()
    pdel = bytearray()
    prev = None
    for i in range(N):
        ini, tm = rec.f(i, "inidisp")[0], rec.f(i, "tm")[0]
        bright = 0 if ini & 0x80 else ini & 15
        s = scene_of[i]
        hs = vs = 0
        if s in (1, 2, 3):
            x0, y0 = geo[s]
            hs, vs = rec.s16(i, "var8") - x0, rec.s16(i, "var10") - y0
        grad = 0xFF
        sub = 0
        col = rec.f(i, "coldata")
        if s >= 4:
            grad = (rec.u16(i, "var6") & 0xF0) >> 4
        elif (rec.f(i, "cgadsub")[0] & 0x81) == 0x81 and s:
            sub = 1
        pal = rec.pal(i)
        start = len(pdel) // 4
        if prev is None:
            for k in range(256):
                pdel += struct.pack(">HH", k, pal[k])
        else:
            for k in range(256):
                if pal[k] != prev[k]:
                    pdel += struct.pack(">HH", k, pal[k])
        prev = pal
        n = len(pdel) // 4 - start
        cold = (col[0] & 31, col[1] & 31, col[2] & 31) if sub else (0, 0, 0)
        # subtract: COLDATA escribe canales por bits 5-7; el registro sombra guarda cada canal
        cold5 = 0
        if sub:
            r = g = b = 0
            for v in col:
                if v & 0x20: r = v & 31
                if v & 0x40: g = v & 31
                if v & 0x80: b = v & 31
            cold5 = r | g << 5 | b << 10
        bf = 0xFF
        if i >= z_end:                                  # el bebé se anima con la imagen ya quieta
            for ptr in struct.unpack("<2H", rec.f(i, "m7obj")):
                if ptr:
                    bf = BABY_T.get(struct.unpack_from("<H", rom, lorom(0x8B0000 | (ptr - 2)))[0], bf)
        tl += struct.pack(">BBhhBBHHHH", bright, s, hs, vs, set_of[i], grad, cold5, start & 0xFFFF, n, bf)
    assert len(pdel) // 4 < 65536, "demasiados cambios de paleta para índices de 16 bits"
    incbin("ttl_timeline", bytes(tl))
    incbin("ttl_pdelta", bytes(pdel))
    c.append(f"const u16 g_title_frames = {N};")
    c.append(f"const u16 g_title_skip_frame = {title_frame};")
    (out / "title_data.c").write_text("\n".join(decl + [""] + c) + "\n", encoding="utf-8")
    (out / "title_data.s").write_text("\n".join(asm) + "\n", encoding="utf-8")
    tot = sum((bins / f).stat().st_size for f in os.listdir(bins) if f.startswith("ttl_"))
    info.append(f"logo de arranque: encendido desde {logo[0]}, funde en {logo[2]}, negro desde {logo[3]}, sigue en {logo[4]}")
    print("\n".join(info))
    print(f"título: {N} cuadros, {tot} bytes")


if __name__ == "__main__":
    main()
