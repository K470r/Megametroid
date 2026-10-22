#!/usr/bin/env python3
"""Genera los datos del menú de partida y de la intro con texto (tests/samus_play: menu.c, intro.c).

Entradas:
  sm.smc          tu ROM (no se distribuye)
  extraido/       blobs descomprimidos (tools/sm_extract)
  instantáneas/   VRAM + WRAM del SNES real en cada pantalla (tools/snes_ref/record_menus.py)
  tools/lang/     textos por idioma: menu.txt (inglés | español) e intro_es.txt (traducción de la intro)
Salida (outdir): menu_data.c, menu_data.s y bins/mnu_*.bin

Qué sale de dónde (todo de tu ROM, nada dibujado a mano salvo las tildes y la Ñ):
  * Fuente grande del menú (8x16, compuesta de mitades) y fuente pequeña: ROM $8E:8000.
  * Fuente de la intro (8x8 verde del diario y 8x16 blanca de la primera página): blob $95:D089.
  * Marcos, cursor y cascos: tiles de OBJ de la pantalla de partidas.
  * Fondos (planeta Zebes, casco de Samus, científico, escenas de las demos): capas del SNES real.
  * Texto de la intro en inglés: de la ROM (objetos de texto $8C, y el mapa de la primera página).
    En español: tools/lang/intro_es.txt (si una página está vacía se usa el inglés).
"""
import argparse, os, re, struct, sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from convert_rooms import snes_to_genesis, gen_to_cram, genesis_tile_bytes, pack_palettes, color_dist  # noqa: E402

LANGS = ("en", "es")


def lorom(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


# ------------------------------------------------------------------ tiles SNES
def t4(data, off):
    b = data[off:off + 32]
    o = np.zeros((8, 8), np.uint8)
    for y in range(8):
        p = (b[2 * y], b[2 * y + 1], b[16 + 2 * y], b[17 + 2 * y])
        for x in range(8):
            o[y, x] = sum(((p[k] >> (7 - x)) & 1) << k for k in range(4))
    return o


def t2(data, off):
    b = data[off:off + 16]
    o = np.zeros((8, 8), np.uint8)
    for y in range(8):
        for x in range(8):
            o[y, x] = ((b[2 * y] >> (7 - x)) & 1) | (((b[2 * y + 1] >> (7 - x)) & 1) << 1)
    return o


class Snap:
    """Instantánea del SNES (VRAM + WRAM)."""
    def __init__(self, d, name):
        self.v = (Path(d) / f"{name}.vram").read_bytes()
        self.w = (Path(d) / f"{name}.wram").read_bytes()
        self.cg = struct.unpack_from("<256H", self.w, 0xC000)
        self.ppm = Path(d) / f"{name}.ppm"

    def reg(self, a):
        return self.w[a]

    def layer(self, n, bpp=4):
        """Capa BGn (1-3) como matriz de índices de CGRAM (-1 transparente), con su scroll."""
        sc = self.reg(0x57 + n)
        nba = self.reg(0x5D) & 15 if n == 1 else (self.reg(0x5D) >> 4 if n == 2 else self.reg(0x5E) & 15)
        base = nba * 0x2000
        scb = (sc >> 2) * 0x800
        hs, vs = struct.unpack_from("<HH", self.w, 0xB1 + (n - 1) * 4)
        vv = self.v + self.v
        img = np.full((256, 256), -1, int)
        for ty in range(32):
            for tx in range(32):
                e = struct.unpack_from("<H", self.v, (scb + (ty * 32 + tx) * 2) & 0xFFFF)[0]
                tp = t4(vv, (base + (e & 0x3FF) * 32) & 0xFFFF) if bpp == 4 else t2(vv, (base + (e & 0x3FF) * 16) & 0xFFFF)
                if e & 0x4000:
                    tp = tp[:, ::-1]
                if e & 0x8000:
                    tp = tp[::-1]
                pal = (e >> 10) & 7
                img[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = np.where(tp > 0, pal * (16 if bpp == 4 else 4) + tp.astype(np.int32), -1)
        img = np.roll(np.roll(img, -vs, 0), -hs, 1)
        return img[:224]

    def colors(self, img):
        """Índices -> colores SNES (BGR555), -1 transparente."""
        cg = np.array(self.cg + (0,))
        return np.where(img >= 0, cg[np.clip(img, 0, 255)], -1)

    def screenshot(self):
        p = self.ppm.read_bytes()
        end = 0
        for _ in range(3):
            end = p.index(b"\n", end) + 1
        a = np.frombuffer(p[end:], np.uint8).reshape(224, 256, 3).astype(int) >> 3
        return a[:, :, 0] | a[:, :, 1] << 5 | a[:, :, 2] << 10


# ------------------------------------------------------------------ imagen -> tiles/mapa/paletas Genesis
def convert_image(img, npal, backdrop=0):
    """img: 224x256 de colores SNES (-1 = transparente). Devuelve (tiles, mapa 32x28, cram npal*16)."""
    bd = snes_to_genesis(backdrop, "dac")
    g = [[None if c < 0 else snes_to_genesis(int(c), "dac") for c in row] for row in img]
    items = []
    for ty in range(28):
        for tx in range(32):
            s = {g[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)} - {None, bd}
            if s:
                items.append(((ty, tx), s))
    merges = {}
    def m(c):
        while c in merges:
            c = merges[c]
        return c
    bins = pack_palettes(items, npal)
    while bins is None:
        allc = sorted(set().union(*[{m(c) for c in cs} for _, cs in items]))
        _, a, b = min((color_dist(allc[i], allc[j]), allc[i], allc[j]) for i in range(len(allc)) for j in range(i + 1, len(allc)))
        merges[b] = a
        bins = pack_palettes([(k, {m(c) for c in cs}) for k, cs in items], npal)
    where, pals = {}, []
    for p, (cs, keys) in enumerate(bins):
        pals.append(sorted(cs))
        for k in keys:
            where[k] = p
    tiles, index, mp = [bytes(32)], {bytes(32): 0}, []
    for ty in range(28):
        for tx in range(32):
            if (ty, tx) not in where:
                mp.append(0)
                continue
            p = where[(ty, tx)]
            px = np.zeros((8, 8), np.uint8)
            for y in range(8):
                for x in range(8):
                    c = g[ty * 8 + y][tx * 8 + x]
                    if c is not None and c != bd:
                        c = m(c)
                        px[y, x] = pals[p].index(c) + 1 if c in pals[p] else \
                            1 + min(range(len(pals[p])), key=lambda k: color_dist(pals[p][k], c))
            found = None
            for v, fh, fv in ((px, 0, 0), (px[:, ::-1], 1, 0), (px[::-1], 0, 1), (px[::-1, ::-1], 1, 1)):
                k = genesis_tile_bytes(v)
                if k in index:
                    found = (index[k], fh, fv)
                    break
            if not found:
                k = genesis_tile_bytes(px)
                index[k] = len(tiles)
                tiles.append(k)
                found = (index[k], 0, 0)
            t, fh, fv = found
            mp.append((p << 13) | (fv << 12) | (fh << 11) | t)
    cram = []
    for pl in pals:
        w = [gen_to_cram(bd)] + [gen_to_cram(c) for c in pl]
        cram += w + [0] * (16 - len(w))
    return b"".join(tiles), mp, cram, len(merges)


def cram_of(snes_colors):
    return [gen_to_cram(snes_to_genesis(int(c), "dac")) for c in snes_colors]


# ------------------------------------------------------------------ tildes y signos que el SNES no tiene
ACUTE = [".....##.", "....##..", "...##...", "........"]
TILDE = ["..##.#..", ".#.##...", "........", "........"]
DIAER = [".##..##.", "........", "........", "........"]


def accent_tile(rows, col, shadow=None):
    """Tile 8x8 con la marca en las filas de abajo (va en la fila de tiles de arriba de la letra)."""
    px = np.zeros((8, 8), np.uint8)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == "#":
                px[4 + y, x] = col
    if shadow:
        sh = np.zeros_like(px)
        sh[1:, 1:] = (px[:-1, :-1] == col)
        px[(sh > 0) & (px == 0)] = shadow
    return px


ACC = {"Á": ("A", 1), "É": ("E", 1), "Í": ("I", 1), "Ó": ("O", 1), "Ú": ("U", 1), "Ñ": ("N", 2), "Ü": ("U", 3)}


def parse_menu_txt(path):
    out = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        k, en, es = [x.strip() for x in line.split("|")]
        out[k] = {"en": en, "es": es}
    return out


def parse_intro_es(path):
    pages, cur = {}, None
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        mm = re.match(r"\[(\w+)\]", line.strip())
        if mm:
            cur = mm.group(1)
            pages[cur] = []
        elif cur is not None and line.strip():
            pages[cur].append(line.strip().upper())
    for name, lines in pages.items():
        lim = 24 if name == "captivity" else 30
        for ln in lines:
            if len(ln) > lim:
                sys.exit(f"intro_es.txt [{name}]: la línea tiene {len(ln)} letras (máximo {lim}): {ln}")
    return pages


def enc(s):
    """Texto -> bytes (Latin-1; 0 termina)."""
    return s.encode("latin-1") + b"\0"


# ------------------------------------------------------------------ main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("extracted"); ap.add_argument("snaps"); ap.add_argument("out")
    ap.add_argument("--proj", default=None)
    a = ap.parse_args()
    rom = Path(a.rom).read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    ext, out = Path(a.extracted), Path(a.out)
    proj = Path(a.proj) if a.proj else out.parent
    bins = out / "bins"
    bins.mkdir(parents=True, exist_ok=True)
    for old in bins.glob("mnu_*.bin"):
        old.unlink()
    asm = ["/* Generado por tools/genesis_convert/make_menu_data.py: no editar. */", "    .section .rodata", ""]
    decl = ["/* Generado por tools/genesis_convert/make_menu_data.py: no editar. */", '#include "menu_data.h"', ""]
    c = []

    def incbin(sym, data, align=2):
        (bins / f"{sym}.bin").write_bytes(data)
        rel = os.path.relpath(bins / f"{sym}.bin", proj).replace("\\", "/")
        asm.extend([f"    .align {align}", f"    .global {sym}", f"{sym}:", f'    .incbin "{rel}"'])
        decl.append(f"extern const u8 {sym}[];")

    def u16tab(name, vals):
        c.append(f"const u16 {name}[{len(vals)}] = {{" + ", ".join(str(v) for v in vals) + "};")

    snaps = {n: Snap(a.snaps, n) for n in ("fsel", "opt", "page1", "page2", "scient", "mbdemo", "larva")}
    fsel, opt = snaps["fsel"], snaps["opt"]

    # ---------------- fuente del menú ($8E:8000): tiles 0x00-0xBF + variante apagada (13,14 -> 11,12)
    mfont = rom[lorom(0x8E8000):lorom(0x8E8000) + 0xC0 * 32]
    tiles_n = [t4(mfont, i * 32) for i in range(0xC0)]
    tiles_d = [np.where(t == 13, 11, np.where(t == 14, 12, t)).astype(np.uint8) for t in tiles_n]
    extra = [accent_tile(ACUTE, 14, 13), accent_tile(TILDE, 14, 13), accent_tile(DIAER, 14, 13)]
    extra_d = [np.where(t == 13, 11, np.where(t == 14, 12, t)).astype(np.uint8) for t in extra]
    # signos invertidos de la fuente grande: '?' y '!' volteados
    PAIRS = {"A": (0x0A, 0x1A), "B": (0x0B, 0x1B), "C": (0x0C, 0x1C), "D": (0x0D, 0x1D), "E": (0x0E, 0x1E),
             "F": (0x0E, 0x38), "G": (0x0C, 0x30), "H": (0x21, 0x31), "I": (0x22, 0x11), "J": (0x23, 0x33),
             "K": (0x24, 0x34), "L": (0x25, 0x35), "M": (0x26, 0x36), "N": (0x27, 0x37), "O": (0x00, 0x10),
             "P": (0x0D, 0x38), "Q": (0x00, 0x39), "R": (0x0D, 0x3A), "S": (0x2B, 0x3B), "T": (0x2C, 0x11),
             "U": (0x2D, 0x10), "V": (0x2D, 0x3E), "W": (0x2F, 0x3F), "X": (0x40, 0x50), "Y": (0x40, 0x11),
             "Z": (0x42, 0x52), "?": (0x45, 0x55), "+": (0x46, 0x56), "-": (0x47, 0x57), "(": (0x48, 0x58),
             ")": (0x49, 0x59), " ": (0x0F, 0x0F), "/": (0x0F, 0x0F), "!": (0x0F, 0x0F)}
    for d in range(9):
        PAIRS[str(d)] = (d, 0x10 + d)
    PAIRS["9"] = (0x08, 0x19)
    # tabla por byte Latin-1: arriba, abajo, tilde (0 ninguna, 1 aguda, 2 virgulilla, 3 diéresis)
    gb = [[0x0F, 0x0F, 0] for _ in range(256)]
    for ch, (t, b) in PAIRS.items():
        gb[ord(ch)] = [t, b, 0]
    for ch, (base, acc) in ACC.items():
        t, b = PAIRS[base]
        gb[ord(ch.encode("latin-1").decode("latin-1"))] = [t, b, acc]
    # fuente pequeña: 0-9 en 0x60, A-Z en 0x6A, ! en 0x84
    gs = [0x0F] * 256
    for d in range(10):
        gs[ord(str(d))] = 0x60 + d
    for k in range(26):
        gs[65 + k] = 0x6A + k
    gs[ord("!")] = 0x84
    for ch, (base, acc) in ACC.items():
        gs[ord(ch.encode("latin-1").decode("latin-1"))] = gs[ord(base)]
    font_bytes = b"".join(genesis_tile_bytes(t) for t in tiles_n + extra + tiles_d + extra_d)
    incbin("mnu_font", font_bytes)
    c.append(f"const u16 g_mnu_font_tiles = {len(tiles_n) + len(extra)};   /* x2: normal y apagada */")
    c.append(f"const u16 g_mnu_font_accent = {len(tiles_n)};                /* aguda, virgulilla, diéresis */")
    u16tab("g_mnu_big", [gb[i][0] | gb[i][1] << 8 for i in range(256)])
    c.append("const u8 g_mnu_big_acc[256] = {" + ", ".join(str(gb[i][2]) for i in range(256)) + "};")
    c.append("const u8 g_mnu_small[256] = {" + ", ".join(str(gs[i]) for i in range(256)) + "};")
    # iconos del pie (cruz, A, B) y "TIME" tal como están en el mapa del SNES (palabras con volteos)
    scb = (opt.reg(0x58) >> 2) * 0x800
    def tm(sn, tx, ty):
        return struct.unpack_from("<H", sn.v, scb + (ty * 32 + tx) * 2)[0]
    icons = [tm(opt, 2, 25), tm(opt, 3, 25), tm(opt, 2, 26), tm(opt, 3, 26),       # cruz 2x2
             tm(opt, 12, 25), tm(opt, 13, 25), tm(opt, 12, 26), tm(opt, 13, 26),   # A 2x2
             tm(opt, 20, 25), tm(opt, 21, 25), tm(opt, 20, 26), tm(opt, 21, 26),   # B 2x2
             tm(fsel, 27, 5), tm(fsel, 28, 5), tm(fsel, 29, 5)]                     # TIME
    u16tab("g_mnu_icons", [(w & 0x3FF) | (w & 0xC000) >> 3 for w in icons])       # tile | volteos Genesis
    # paleta del texto: línea 0 del SNES (normal) + línea 1 (apagada) en 11, 12
    pal0 = list(opt.cg[0:16])
    pal0[11], pal0[12] = opt.cg[16 + 13], opt.cg[16 + 14]
    u16tab("g_mnu_text_pal", cram_of(pal0))

    # ---------------- OBJ del menú: tiles 0xC0-0xFF (marcos, cursor, cascos), paleta 7
    obj = [t4(fsel.v, 0xC000 + t * 32) for t in range(0xC0, 0x100)]
    incbin("mnu_obj", b"".join(genesis_tile_bytes(t) for t in obj))
    u16tab("g_mnu_obj_pal", cram_of(fsel.cg[128 + 7 * 16:128 + 8 * 16]))
    # spritemaps del menú ($82:C569, como DrawMenuSpritemap $81:891F): cascos de las ranuras, que
    # giran al elegir partida (0x2C de lado ... 0x33 de frente; DrawFileSelectSlotSamusHelmet $81:9DE4).
    # Entrada: x (9 bits con signo; bit 15 = sprite de 16x16), y (con signo), tile | volteos. Los tiles
    # son los de OBJ 0xC0-0xFF de arriba.
    smap = []
    for i in range(0x2C, 0x34):
        o = lorom(0x820000 | struct.unpack_from("<H", rom, lorom(0x82C569) + 2 * i)[0])
        n = struct.unpack_from("<H", rom, o)[0]
        ents = []
        for k in range(n):
            xw, y, ch = struct.unpack_from("<HbH", rom, o + 2 + 5 * k)
            x = (xw & 0x1FF) - (0x200 if xw & 0x100 else 0)
            t = ch & 0x1FF
            assert 0xC0 <= t and t + (17 if xw & 0x8000 else 0) < 0x100, hex(t)
            ents.append((x, y, t - 0xC0, (xw >> 15) & 1, (ch >> 14) & 1, (ch >> 15) & 1))
        smap.append(ents)
    flat, offs = [], []
    for ents in smap:
        offs.append(len(flat))
        flat.append(len(ents))
        for (x, y, t, big, hf, vf) in ents:
            flat += [x & 0xFFFF, y & 0xFFFF, t | big << 8 | hf << 9 | vf << 10]
    u16tab("g_mnu_helmet_smap", flat)
    u16tab("g_mnu_helmet_smap_off", offs)

    # ---------------- fondos
    info = []
    def image(sym, img, npal, backdrop=0):
        tiles, mp, cram, nm = convert_image(img, npal, backdrop)
        incbin(f"mnu_{sym}_tiles", tiles)
        incbin(f"mnu_{sym}_map", struct.pack(f">{len(mp)}H", *mp))
        c.append(f"const MnuImage g_img_{sym} = {{ {len(tiles) // 32}, (const u32 *) mnu_{sym}_tiles, "
                 f"(const u16 *) mnu_{sym}_map, {{ " + ", ".join(map(str, cram + [0] * (48 - len(cram)))) + " } };")
        info.append(f"{sym}: {len(tiles) // 32} tiles, fusiones {nm}")
    image("planet", fsel.colors(fsel.layer(2)), 2)
    # (las imágenes fijas de la intro vieja ya no van: la intro es el código del juego, docs/29)

    # ---------------- fuente de la intro (blob $95:D089, 2bpp)
    f1 = next((ext / "blobs" / "decompressed").glob("95D089_*.bin")).read_bytes()
    nf1 = len(f1) // 16
    ftiles = [t2(f1, i * 16) for i in range(min(nf1, 0x70))]
    fex = [accent_tile(ACUTE, 3), accent_tile(TILDE, 3), accent_tile(DIAER, 3)]
    # la fuente de la intro ya no se copia aquí (la usa intro_md.c desde make_intro_data.py);
    # estas tablas solo sirven para sacar el texto en inglés de referencia para la traducción
    # objetos de letra del diario ($8C:D67D espacio, $8C:D685 + 6k): su tile es la letra
    def objtile(ob):
        return struct.unpack_from("<H", rom, lorom(0x8C0000 | (ob + 4)))[0] & 0x3FF
    OBJ_CH = {0xD67D: " "}
    chars = [chr(65 + k) for k in range(26)] + [str(d) for d in range(10)] + list(".,.':!")
    for k, ch in enumerate(chars):
        OBJ_CH[0xD685 + 6 * k] = ch
    g8 = [objtile(0xD67D)] * 256
    for ob, ch in OBJ_CH.items():
        if ch != " " and g8[ord(ch)] == g8[0]:
            g8[ord(ch)] = objtile(ob)
    g8[ord("?")] = g8[ord("!")]
    acc8 = [0] * 256
    for ch, (base, ac) in ACC.items():
        g8[ord(ch.encode("latin-1").decode("latin-1"))] = g8[ord(base)]
        acc8[ord(ch.encode("latin-1").decode("latin-1"))] = ac
    g8[0xBF] = g8[ord("?")]      # ¿ (se dibuja volteado)
    g8[0xA1] = g8[ord("!")]      # ¡
    c.append("const u8 g_font8[256] = {" + ", ".join(map(str, g8)) + "};")
    c.append("const u8 g_font8_acc[256] = {" + ", ".join(map(str, acc8)) + "};")
    # 8x16 blanca: A-P en 0x30 (abajo +0x10), Q-Z en 0x50 (abajo +0x10), '.' abajo 0x26
    g16 = [0x2F | 0x2F << 8] * 256
    for k in range(26):
        t = 0x30 + k if k < 16 else 0x50 + k - 16
        g16[65 + k] = t | (t + 0x10) << 8
    g16[ord(".")] = 0x2F | 0x26 << 8
    g16[ord(",")] = 0x2F | 0x25 << 8
    for ch, (base, ac) in ACC.items():
        g16[ord(ch.encode("latin-1").decode("latin-1"))] = g16[ord(base)]
    u16tab("g_font16", g16)
    # paletas: verde del diario (línea 3 de BG3 en la página 2), brillo (línea 4), blanca (línea 0)
    p2 = snaps["page2"]
    u16tab("g_font_green_pal", cram_of([0] + list(p2.cg[13:16]) + [0] * 12))
    u16tab("g_font_white_pal", cram_of([0] + list(snaps["page1"].cg[17:20]) + [0] * 12))

    # ---------------- textos del menú
    mt = parse_menu_txt(HERE.parent / "lang" / "menu.txt")
    keys = sorted(mt)
    c.append(f"const u16 g_mnu_num_strings = {len(keys)};")
    blob, offs = b"", []
    for lang in LANGS:
        for k in keys:
            offs.append(len(blob))
            blob += enc(mt[k][lang])
    incbin("mnu_strings", blob)
    u16tab("g_mnu_str_off", offs)
    hdr = ["/* Generado por make_menu_data.py: índices de los textos del menú (tools/lang/menu.txt) */"]
    for i, k in enumerate(keys):
        hdr.append(f"#define S_{k.upper()} {i}")
    (out / "menu_strings.h").write_text("\n".join(hdr) + "\n", encoding="utf-8")

    # ---------------- intro: texto en inglés desde la ROM, español desde intro_es.txt
    pages_en = {}
    # primera página: mapa de BG3 de la instantánea (fuente 8x16 blanca) -> letras
    inv16 = {}
    for code in range(256):
        top, bot = g16[code] & 0xFF, g16[code] >> 8
        if code in (ord(" "),) or (code >= 65 and code <= 90) or chr(code) in ".,":
            inv16.setdefault((top, bot), chr(code))
    p1 = snaps["page1"]
    scb3 = (p1.reg(0x5A) >> 2) * 0x800
    lines = []
    ty = 0
    while ty < 27:
        row = [struct.unpack_from("<H", p1.v, scb3 + (ty * 32 + tx) * 2)[0] & 0x3FF for tx in range(32)]
        nxt = [struct.unpack_from("<H", p1.v, scb3 + ((ty + 1) * 32 + tx) * 2)[0] & 0x3FF for tx in range(32)]
        ty += 1
        if any(0x30 <= t < 0x40 or 0x50 <= t < 0x5A for t in row):
            ty += 1
            s = "".join(inv16.get((row[x], nxt[x]), " ") for x in range(32)).rstrip()
            lines.append(s[5:] if s.startswith("     ") else s.strip())
    pages_en["captivity"] = lines
    # páginas del diario: objetos de texto ($8B:CF3F...)
    for pi, d in enumerate((0xCF3F, 0xCF45, 0xCF4B, 0xCF51, 0xCF57)):
        p = 0x8C0000 | struct.unpack_from("<H", rom, lorom(0x8B0000 | (d + 4)))[0]
        grid = {}
        for _ in range(800):
            v = struct.unpack_from("<H", rom, lorom(p))[0]
            if v & 0x8000:
                if v == 0x9698:
                    break
                p += 2
                continue
            pos, ob = struct.unpack_from("<HH", rom, lorom(p) + 2)
            p += 6
            if ob in OBJ_CH:
                grid[(pos >> 8, pos & 0xFF)] = OBJ_CH[ob]
        rows = sorted(set(r for r, _ in grid if r >= 4))
        pages_en[f"page{pi + 1}"] = ["".join(grid.get((r, x), " ") for x in range(1, 32)).rstrip() for r in rows]
    pages_es = parse_intro_es(HERE.parent / "lang" / "intro_es.txt")
    order = ["captivity", "page1", "page2", "page3", "page4", "page5"]
    blob, offs = b"", []
    for lang in LANGS:
        for pg in order:
            lines = pages_es.get(pg) if lang == "es" and pages_es.get(pg) else pages_en[pg]
            offs.append(len(blob))
            blob += "\n".join(lines).encode("latin-1") + b"\0"
    (out / "intro_en_extraido.txt").write_text(
        "# Texto en inglés sacado de tu ROM (referencia para traducir; no se distribuye)\n" +
        "\n".join(f"[{pg}]\n" + "\n".join(pages_en[pg]) + "\n" for pg in order), encoding="utf-8")

    (out / "menu_data.c").write_text("\n".join(decl + [""] + c) + "\n", encoding="utf-8")
    (out / "menu_data.s").write_text("\n".join(asm) + "\n", encoding="utf-8")
    print("\n".join(info))
    print("páginas de la intro:", {k: len(v) for k, v in pages_en.items()})


if __name__ == "__main__":
    main()
