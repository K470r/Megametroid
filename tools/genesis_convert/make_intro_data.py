#!/usr/bin/env python3
"""Datos gráficos de la intro portada (tests/samus_play/src/intro_md.c; docs/29).

La lógica de la intro es el código del SNES portado (port/genesis/intro_glue.cpp). Lo que se
convierte aquí es lo que en el SNES está en la VRAM y la CGRAM: tiles, mapas y paletas, en la forma
que la Genesis dibuja directo.

Entradas:
  sm.smc              tu ROM (no se distribuye)
  extraido/           salida de tools/sm_extract (blobs/decompressed: los gráficos de la intro)
  intro_usage.json    qué usa cada fase (port/diff/intro_usage.json, de tools/snes_ref/intro_usage.py)
  intro_ram.bin       RAM del SNES justo antes de CinematicFunction_Intro_Initial (plantilla)
Salida (outdir): intro_data.c, intro_data.s, bins/itr_*.bin

VRAM del SNES al arrancar la intro: la misma carga de CinematicFunction_Intro_Initial ($8B:A395):
  $0000 tiles de BG1/BG2 (95F90E)   $4000 fuente BG3 (95D089)   $4800 mapa del casco (9788CC)
  $4C00 mapa de BG3 (978D12)        $5000 mapas de los recuerdos (96FF14)
  $6000 sprites (ROM $9A:D200)      $6300 tiles del rayo (UpdateBeamTilesAndPalette)   $6E00 (95E4C2)
Desde la página 1 ($8B:A66F), $4180-$447F son los tiles del texto japonés, limpios.

Fases (las mismas de intro_usage.py): CAPT, TEXT, FB50 (Mother Brain), FB54 (larva), FB58
(científico), FB5C (laboratorio), FLY (vuelo a Ceres, modo 7) y SPACE ("SPACE COLONY"); lo propio del
vuelo está en make_intro_flight.py. Por fase:
  * 4 líneas de paleta. Cada color de la Genesis apunta a un índice de la CGRAM del SNES: en cada
    cuadro se toma de palette_buffer (fundidos y efectos de paleta tal cual los hace el juego).
    Si Samus está en escena, la línea 3 es la paleta 4 de sprites del SNES tal cual (sus tiles se
    copian sin convertir, como en la ROM jugable). El resto (celdas de BG y sprites) se empaqueta
    por conjuntos de colores, juntando los índices que en esa fase tienen siempre el mismo color.
  * Tiles de la fase: plano B (BG1 del recuerdo o BG2 del casco), plano A (BG3: texto y franjas
    negras) y sprites (8x8 y 16x16 en columnas, como los espera el VDP).
  * Mapa fijo del plano B y tablas para convertir en el momento las celdas que el juego cambia
    (texto de BG3 y ojos del casco) y los sprites del OAM.
"""
import argparse, glob, json, os, struct, sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from convert_rooms import genesis_tile_bytes, pack_palettes  # noqa: E402
import make_intro_flight as flight  # noqa: E402

PHASES = ['CAPT', 'TEXT', 'FB50', 'FB54', 'FB58', 'FB5C', 'FLY', 'SPACE']
FLIGHT = ('FLY', 'SPACE')        # vuelo a Ceres (make_intro_flight.py)
SHIP_MAX_TILES = 520           # tiles de un cuadro de la nave (dos búferes en la VRAM después de los tiles de la fase)
BEAM_SRC = 0x9AF200            # tiles del rayo en $6300 (el de la plantilla: rayo normal)
SAMUS_LINE = 3
TILE_BASE = 16                 # TILE_USER_INDEX de SGDK: ya sumado en mapas y tablas (intro_md.c lo comprueba)
MAX_TILES = 1300               # tiles por fase
MAX_TILES_SAMUS = 320          # con Samus en escena: sus tiles van en TILE_USER_INDEX + 320


def lorom(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def snes_vram(rom, ext, japanese_cleared):
    def blob(name):
        return next((ext / 'blobs' / 'decompressed').glob(name + '_*.bin')).read_bytes()
    vr = bytearray(0x10000)
    def put(w, data):
        vr[w * 2:w * 2 + len(data)] = data
    put(0x0000, blob('95F90E')[:0x8000])
    put(0x4000, blob('95D089')[:0x900])
    put(0x4800, blob('9788CC')[:0x800])
    put(0x4C00, blob('978D12')[:0x800])
    put(0x5000, blob('96FF14')[:0x2000])
    put(0x6000, rom[lorom(0x9AD200):lorom(0x9AD200) + 0x2000])
    put(0x6300, rom[lorom(BEAM_SRC):lorom(BEAM_SRC) + 0x100])
    put(0x6E00, blob('95E4C2')[:0x2400])
    if japanese_cleared:            # ClearJapaneseTextTiles: 8 palabras de 7F:8290 repetidas
        pat = blob('95D089')[0x290:0x2A0]
        put(0x4180, pat * (0x600 // 16))
    return np.frombuffer(bytes(vr), dtype='<u2')


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


def obj_cells(c, big):
    """Tiles del SNES de un sprite: 8x8 = [c]; 16x16 en el orden del VDP (columnas: arriba-izq,
    abajo-izq, arriba-der, abajo-der), con la vuelta de fila del SNES dentro de cada tabla de 256."""
    if not big:
        return [c]
    cells = []
    for cx, cy in ((0, 0), (0, 1), (1, 0), (1, 1)):
        cells.append((c & 0x100) | ((((c >> 4) & 15) + cy) & 15) << 4 | (((c & 15) + cx) & 15))
    return cells


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('rom'); ap.add_argument('extracted'); ap.add_argument('usage'); ap.add_argument('ram')
    ap.add_argument('out'); ap.add_argument('--proj', default=None)
    a = ap.parse_args()
    rom = Path(a.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    ext, out = Path(a.extracted), Path(a.out)
    proj = Path(a.proj) if a.proj else out.parent
    bins = out / 'bins'
    bins.mkdir(parents=True, exist_ok=True)
    for old in bins.glob('itr_*.bin'):
        old.unlink()
    usage = json.load(open(a.usage))
    asm = ['/* Generado por tools/genesis_convert/make_intro_data.py: no editar. */', '    .section .rodata', '']
    c = ['/* Generado por tools/genesis_convert/make_intro_data.py: no editar. */', '#include "intro_data.h"', '']

    def incbin(sym, data, align=2):
        (bins / f'{sym}.bin').write_bytes(data)
        rel = os.path.relpath(bins / f'{sym}.bin', proj).replace('\\', '/')
        asm.extend([f'    .align {align}', f'    .global {sym}', f'{sym}:', f'    .incbin "{rel}"'])
        c.append(f'extern const u8 {sym}[];')

    vr_capt = snes_vram(rom, ext, False)
    vr = snes_vram(rom, ext, True)
    def blob(name):
        return next((ext / 'blobs' / 'decompressed').glob(name + '_*.bin')).read_bytes()
    vr_fly = flight.flight_vram(rom, blob)
    info = []
    phase_rows = []
    for ph in PHASES:
        u = usage.get(ph)
        if not u:
            phase_rows.append(None)
            continue
        v = vr_capt if ph == 'CAPT' else vr_fly if ph in FLIGHT else vr
        cls = u['classes']
        a_4bpp = ph == 'SPACE'                         # texto en BG1 (4bpp, tiles en $6000); en las demás, BG3
        bg_b_base = 0x4800 if ph == 'TEXT' else (int(ph[2:], 16) << 8 if ph.startswith('FB') else None)
        # ---- elementos: (clave, capa, píxeles SNES 8x8 por celda, base de CGRAM)
        items = []                                     # (clave, set de clases)
        pix = {}                                       # clave -> lista de matrices (una por celda) con índice de CGRAM (0 = transparente)
        def add_bg(entry, layer):
            ch, pal = entry & 0x3FF, (entry >> 10) & 7
            key = (layer, ch, pal)
            if key in pix:
                return
            if layer == 'A' and a_4bpp:
                t = tile(v, 0x6000, ch, 4); idx = np.where(t > 0, pal * 16 + t, 0)
            elif layer == 'A':
                t = tile(v, 0x4000, ch, 2); idx = np.where(t > 0, pal * 4 + t, 0)
            else:
                t = tile(v, 0x0000, ch, 4); idx = np.where(t > 0, pal * 16 + t, 0)
            pix[key] = [idx]
        for e in u['bg3']:
            add_bg(e, 'A')
        static_b = None
        if bg_b_base is not None:
            static_b = [int(x) for x in v[bg_b_base:bg_b_base + 0x400]]
            for e in set(static_b) | set(u['bgB']):
                add_bg(e, 'B')
        samus = ph in ('FB50', 'FB54')
        for ch, big, pal in u['obj']:
            key = ('O', ch, big, pal)
            cells = []
            for cc in obj_cells(ch, big):
                t = tile(v, 0x6000, cc, 4)
                cells.append(np.where(t > 0, 128 + pal * 16 + t, 0))
            pix[key] = cells
        pool_keys = []
        for key, cells in pix.items():
            if samus and key[0] == 'O' and key[3] == 4:
                continue                                # paleta de Samus: línea propia, sin convertir
            s = set()
            for m_ in cells:
                s |= {cls[int(i)] for i in np.unique(m_) if i}
            items.append((key, s))
            pool_keys.append(key)
        ship = ph == 'FLY'
        npal = 3 if samus or ship else 4
        if ph in FLIGHT:                                # vuelo: cada paleta de sprites del SNES en una sola línea
            byp = {}                                    # (los spritemaps grandes se juntan en una imagen)
            rest = []
            for key, st in items:
                if key[0] == 'O':
                    byp.setdefault(key[3], set()).update(st)
                else:
                    rest.append((key, st))
            items = rest + [(('OP', p_), st) for p_, st in byp.items()]
        bins_ = pack_palettes(items, npal)
        assert bins_ is not None, f'{ph}: no entra en {npal} líneas de paleta'
        cram_map = [0xFF] * 64
        cram_map[0] = 0                                 # color de fondo (CGRAM 0)
        where, slot_of = {}, {}
        for line, (cs, keys) in enumerate(bins_):
            for k in keys:
                where[k] = line
                if k[0] == 'OP':
                    for key in pix:
                        if key[0] == 'O' and key[3] == k[1]:
                            where[key] = line
            for s_i, cl in enumerate(sorted(cs)):
                cram_map[line * 16 + 1 + s_i] = cl
                slot_of[(line, cl)] = 1 + s_i
        if ship:                                        # la nave: la línea 5 de la CGRAM (80-95) tal cual
            for k in range(1, 16):
                cram_map[flight.M7_LINE * 16 + k] = 80 + k
        if samus:
            for k in range(1, 16):
                cram_map[SAMUS_LINE * 16 + k] = 128 + 4 * 16 + k
            for key in pix:
                if key[0] == 'O' and key[3] == 4:
                    where[key] = SAMUS_LINE
        # ---- tiles de la fase (sin repetir)
        tiles, tindex = [], {}
        def gtile(m_, line):
            if line == SAMUS_LINE and samus:
                g = np.where(m_ > 0, m_ - (128 + 64), 0)
            else:
                g = np.vectorize(lambda i: slot_of[(line, cls[int(i)])] if i else 0)(m_)
            b = genesis_tile_bytes(g)
            if b not in tindex:
                tindex[b] = len(tiles); tiles.append(b)
            return tindex[b]
        # ---- spritemaps grandes del vuelo: cada uno como imagen, cubierta con sprites de la Genesis. Van
        #      primero, así FLY y SPACE los tienen en los mismos tiles (se cambia de fase sin apagar)
        msm = []
        if ph in FLIGHT:
            line_of_pal = {}
            for key in pix:
                if key[0] == 'O':
                    line_of_pal.setdefault(key[3], set()).add(where[key])
            def objpix(ch, big, pal):
                m = np.zeros((16, 16) if big else (8, 8), int)
                for i, cc in enumerate(obj_cells(ch, big)):
                    cx, cy = (0, 0) if not big else ((0, 0), (0, 1), (1, 0), (1, 1))[i]
                    t = tile(v, 0x6000, cc, 4)
                    m[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] = np.where(t > 0, 128 + pal * 16 + t, 0)
                return m
            for pp, chr_ in u.get('spritemaps', []):
                pal = (chr_ >> 9) & 7
                assert len(line_of_pal.get(pal, ())) == 1, f'{ph}: la paleta {pal} de sprites quedó en varias líneas'
                line = next(iter(line_of_pal[pal]))
                img, x0, y0, prio = flight.compose_spritemap(rom, pp, chr_, objpix)
                boxes, pad, M = flight.cover(img)
                sprs = []
                for bx, by, w, h in boxes:
                    first = None
                    for cx in range(w):                 # tiles del sprite por columnas, como el VDP
                        for cy in range(h):
                            m_ = pad[by + M + cy * 8:by + M + cy * 8 + 8, bx + M + cx * 8:bx + M + cx * 8 + 8]
                            g = np.vectorize(lambda i: slot_of[(line, cls[int(i)])] if i else 0)(m_)
                            tiles.append(genesis_tile_bytes(g))
                            if first is None:
                                first = len(tiles) - 1
                    attr = (first + TILE_BASE) | line << 13 | (0x8000 if prio else 0)
                    sprs.append((by + y0, bx + x0, ((w - 1) << 2 | (h - 1)) << 8, attr))
                msm.append((pp, chr_, sprs))
        gword = {}
        for key, cells in pix.items():
            if key[0] == 'O' and ph in FLIGHT:          # vuelo: todos los sprites vienen de los spritemaps grandes
                continue
            line = where[key]
            idxs = [gtile(m_, line) for m_ in cells]
            if key[0] == 'O' and key[2]:                # 16x16: los 4 tiles seguidos
                seq = b''.join(tiles[i] for i in idxs)
                first = len(tiles)
                tiles.extend(tiles[i] for i in idxs)
                gword[key] = (first, line)
            else:
                gword[key] = (idxs[0], line)
        assert len(tiles) <= (MAX_TILES_SAMUS if samus else MAX_TILES), f'{ph}: {len(tiles)} tiles'
        # ---- tablas de conversión: capa -> idx[char] (desplazamiento en vals, 8 paletas) + vals
        def lookup(layer):
            chars = sorted({k[1] for k in pix if k[0] == layer})
            idx = [0xFFFF] * 1024
            vals = []
            for ch in chars:
                idx[ch] = len(vals)
                row = [0] * 8
                for pal in range(8):
                    if (layer, ch, pal) in gword:
                        t, line = gword[(layer, ch, pal)]
                        row[pal] = (t + TILE_BASE) | line << 13
                vals += row
            return idx, vals
        sym = ph.lower()
        incbin(f'itr_{sym}_tiles', b''.join(tiles))
        incbin(f'itr_{sym}_cram', bytes(cram_map))
        ia, va = lookup('A')
        incbin(f'itr_{sym}_a_idx', struct.pack('>1024H', *ia))
        incbin(f'itr_{sym}_a_val', struct.pack(f'>{len(va)}H', *va) if va else b'\0\0')
        has_b = static_b is not None
        if has_b:
            ib, vb = lookup('B')
            incbin(f'itr_{sym}_b_idx', struct.pack('>1024H', *ib))
            incbin(f'itr_{sym}_b_val', struct.pack(f'>{len(vb)}H', *vb) if vb else b'\0\0')
            mb = []
            for e in static_b:
                t, line = gword[('B', e & 0x3FF, (e >> 10) & 7)]
                mb.append((t + TILE_BASE) | line << 13 | ((e >> 14) & 1) << 11 | ((e >> 15) & 1) << 12)
            incbin(f'itr_{sym}_b_map', struct.pack('>1024H', *mb))
        osmall, obig = [0] * 512, [0] * 512
        oline = [0] * 8
        for key, (t, line) in gword.items():
            if key[0] != 'O':
                continue
            (obig if key[2] else osmall)[key[1]] = t + TILE_BASE + 1
            oline[key[3]] = line
        incbin(f'itr_{sym}_obj', struct.pack('>512H', *osmall) + struct.pack('>512H', *obig) + bytes(oline))
        c.append(f'/* {ph}: {len(tiles)} tiles, líneas {[len(b[0]) for b in bins_]}, sprites {len(u["obj"])} */')
        if msm:
            for i, (pp, chr_, sprs) in enumerate(msm):
                vals = [x & 0xFFFF for spr in sprs for x in spr]
                c.append(f'static const u16 itr_{sym}_msm{i}[{len(vals)}] = {{ ' + ', '.join(str(x) for x in vals) + ' };')
            c.append(f'static const IntroMsm itr_{sym}_msm[{len(msm)}] = {{')
            for i, (pp, chr_, sprs) in enumerate(msm):
                c.append(f'    {{ 0x{pp:06X}, 0x{chr_:04X}, {len(sprs)}, itr_{sym}_msm{i} }},')
            c.append('};')
        phase_rows.append((sym, len(tiles), has_b, 0x5C00 if a_4bpp else 0x4C00, len(msm), ship))
        info.append(f'{ph}: {len(tiles)} tiles, colores por línea {[len(b[0]) for b in bins_]}' + (' + Samus' if samus else ''))

    c.append(f'const IntroPhase g_intro_phases[{len(PHASES)}] = {{')
    for ph, row in zip(PHASES, phase_rows):
        if row is None:
            c.append('    { 0 },'); continue
        sym, n, has_b, a_base, nmsm, ship = row
        b = (f'(const u16 *) itr_{sym}_b_map, (const u16 *) itr_{sym}_b_idx, (const u16 *) itr_{sym}_b_val'
             if has_b else 'NULL, NULL, NULL')
        m = f'itr_{sym}_msm, {nmsm}' if nmsm else 'NULL, 0'
        c.append(f'    {{ {n}, (const u32 *) itr_{sym}_tiles, itr_{sym}_cram, (const u16 *) itr_{sym}_a_idx, '
                 f'(const u16 *) itr_{sym}_a_val, {b}, (const u16 *) itr_{sym}_obj, 0x{a_base:04X}, {m}, {int(ship)} }},  /* {ph} */')
    c.append('};')

    # ---- la nave del vuelo a Ceres (modo 7 del SNES): un cuadro por juego de registros, en la página 8
    m7 = usage['FLY']['m7']
    vr_back = flight.flight_vram(rom, blob, 1)
    ship, offs, skipped, _ = flight.ship_frames((vr_fly, vr_back), m7, genesis_tile_bytes, SHIP_MAX_TILES)
    (bins / 'itr_ship.bin').write_bytes(ship)
    rel = os.path.relpath(bins / 'itr_ship.bin', proj).replace('\\', '/')
    asm.extend(['', '    .section .bank8,"a"', '    .align 2', '    .global itr_ship', 'itr_ship:', f'    .incbin "{rel}"',
                '    .section .rodata'])
    c.append('extern const u8 itr_ship[];')
    c.append(f'const u16 g_intro_ship_n = {len(m7)};')
    c.append(f'const u16 g_intro_ship_key[{len(m7)}][8] = {{ ' + ', '.join('{' + ', '.join(str(x) for x in k[:8]) + '}' for k in m7) + ' };')
    c.append(f'const u32 g_intro_ship_off[{len(m7)}] = {{ ' + ', '.join(str(o) for o in offs) + ' };')
    c.append('const u8 *const g_intro_ship = itr_ship;')
    info.append(f'nave: {len(m7)} cuadros, {len(ship)} bytes (página 8 del SSF2); sin dibujar por grandes: {skipped}')
    wins = [(0x95F90E, 0x16400, blob('95F90E')[0x6400:0x6600]),
            (0x95D089, 0x18200, blob('95D089')[0x200:0x300]),
            (0x9788CC, 0x19000, blob('9788CC')[0x000:0x800]),
            (0x978D12, 0x1E000, blob('978D12')[0x000:0x800])]   # mapa de BG3 inicial: solo el DMA de Intro_Initial
    for k, (src, dst, data) in enumerate(wins):
        incbin(f'itr_win{k}', data)
    c.append('const MMIntroWindow g_intro_windows[] = {')
    for k, (src, dst, data) in enumerate(wins):
        c.append(f'    {{ 0x{src:06X}, 0x{dst:05X}, {len(data)}, itr_win{k} }},')
    c.append('    { 0, 0, 0, 0 },\n};')

    # ---- plantilla de RAM (zonas en el orden de intro_glue.cpp: lo, 20, 30, 38, 40, 98, pal, cd, d8)
    ram = Path(a.ram).read_bytes()
    regs = [(0x00000, 0x02000), (0x02000, 0x03000), (0x03000, 0x03800), (0x03800, 0x04000), (0x04000, 0x04600),
            (0x09800, 0x09C00), (0x0C000, 0x0C800), (0x0CD00, 0x0CE00), (0x0D600, 0x0D920)]
    for k, (lo, hi) in enumerate(regs):
        incbin(f'itr_ram{k}', ram[lo:hi])
    c.append('const u8 *const g_intro_ram[10] = { ' + ', '.join(f'itr_ram{k}' for k in range(len(regs))) + ', NULL };')

    (out / 'intro_data.c').write_text('\n'.join(c) + '\n', encoding='utf-8')
    (out / 'intro_data.s').write_text('\n'.join(asm) + '\n', encoding='utf-8')
    tot = sum((bins / f).stat().st_size for f in os.listdir(bins) if f.startswith('itr_'))
    print('\n'.join(info))
    print(f'intro: {tot} bytes')


if __name__ == '__main__':
    main()
