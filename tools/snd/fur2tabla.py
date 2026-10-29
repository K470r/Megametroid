#!/usr/bin/env python3
"""MegaMetroid — los instrumentos FM de un proyecto de Furnace (.fur) a tools/snd/instrumentos.txt (docs/55).

Uso: fur2tabla.py proyecto.fur [--escribir [tabla.txt]] [--envolvente-snes] [--revisar]
  --escribir         escribe en la tabla (por omisión tools/snd/instrumentos.txt) una línea por instrumento FM cuyo
                     nombre empiece con CONJ_MUESTRA (como los deja mkfur.py: "03_25"); sin esto solo las muestra
  --envolvente-snes  pone en los portadores ar=31 d1r=0 sl=0 d2r=0 rr=15 (como fui2parche.py)
  --revisar          lee el proyecto entero como el cargador de Furnace (src/engine/fileOps/fur.cpp) y muestra sus
                     datos y las primeras filas de cada canal (para comprobar lo que escribe mkfur.py)

Lee el .fur comprimido o no, del formato viejo (bloque INFO, hasta Furnace 0.6.8.1) o del nuevo (INF2). Los
instrumentos se pasan con fui2parche.to_parche: la misma línea que daría guardarlos uno por uno como .fui.
"""
import re
import struct
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import fui2parche  # noqa: E402

NAME_RE = re.compile(r"^([0-9A-Fa-f]{2})_(\d+)(?:\s|$)")


class R:
    def __init__(self, d, p=0):
        self.d, self.p = d, p

    def u8(self):
        self.p += 1
        return self.d[self.p - 1]

    def s8(self):
        return struct.unpack("<b", bytes([self.u8()]))[0]

    def u16(self):
        self.p += 2
        return struct.unpack_from("<H", self.d, self.p - 2)[0]

    def u32(self):
        self.p += 4
        return struct.unpack_from("<I", self.d, self.p - 4)[0]

    def f32(self):
        self.p += 4
        return struct.unpack_from("<f", self.d, self.p - 4)[0]

    def s(self):
        e = self.d.index(b"\0", self.p)
        v = self.d[self.p:e].decode("utf-8", "replace")
        self.p = e + 1
        return v

    def raw(self, n):
        self.p += n
        return self.d[self.p - n:self.p]

    def magic(self, m):
        got = self.raw(4)
        if got != m:
            raise SystemExit(f"se esperaba {m!r} en {self.p - 4:#x} y hay {got!r}")


CHIP_CHANS = {0x83: 6, 0x03: 4, 0xA0: 9, 0xBE: 7, 0xBD: 11, 0xC1: 10}


def load(path):
    d = Path(path).read_bytes()
    if d[:16] != b"-Furnace module-":
        d = zlib.decompress(d)
    if d[:16] != b"-Furnace module-":
        raise SystemExit(f"{path}: no es un proyecto de Furnace (.fur)")
    return d


def read_ins(d, p):
    """un bloque INS2 en p: (nombre, tipo, fm (como fui2parche.read_new) o None)"""
    r = R(d, p)
    r.magic(b"INS2")
    end = r.u32() + r.p
    version, kind = r.u16(), r.u16()
    name, fm = "", None
    while r.p < end:
        code = r.raw(2)
        if code == b"EN":
            break
        n = r.u16()
        blk = d[r.p:r.p + n]
        r.p += n
        if code == b"NA":
            name = blk.split(b"\0")[0].decode("utf-8", "replace")
        elif code == b"FM":
            # el mismo bloque que en un .fui: se lee con fui2parche (envuelto en una cabecera FINS)
            fins = b"FINS" + struct.pack("<HH", version, kind) + b"FM" + struct.pack("<H", n) + blk + b"EN"
            fm = fui2parche.read_new(fins)[2]
    return name, kind, fm


def parse_old(d, verbose=False):
    """el formato viejo (versión < 240), campo por campo como fur.cpp"""
    r = R(d, 16)
    version = r.u16()
    r.u16()
    r.p = r.u32()
    r.magic(b"INFO")
    r.u32()
    song = {"version": version}
    song["timebase"], sp1, sp2, _arp = r.u8(), r.u8(), r.u8(), r.u8()
    song["hz"] = r.f32()
    song["patlen"], song["ordlen"] = r.u16(), r.u16()
    r.u8(), r.u8()
    nins, nwave, nsmp, npat = r.u16(), r.u16(), r.u16(), r.u32()
    chips = list(r.raw(32))
    chips = chips[:chips.index(0)] if 0 in chips else chips
    song["chips"] = chips
    tchans = sum(CHIP_CHANS.get(c, 0) for c in chips)
    if 0 in [CHIP_CHANS.get(c, 0) for c in chips]:
        raise SystemExit(f"chip sin canales conocidos: {chips}")
    song["chans"] = tchans
    r.raw(32), r.raw(32), r.raw(128)
    song["name"], song["author"] = r.s(), r.s()
    song["tuning"] = r.f32()
    song["compat1"] = list(r.raw(20))
    ins_ptr = [r.u32() for _ in range(nins)]
    [r.u32() for _ in range(nwave)]
    [r.u32() for _ in range(nsmp)]
    pat_ptr = [r.u32() for _ in range(npat)]
    song["orders"] = [list(r.raw(song["ordlen"])) for _ in range(tchans)]
    song["fxcols"] = list(r.raw(tchans))
    r.raw(tchans), r.raw(tchans)
    [r.s() for _ in range(2 * tchans)]
    song["comment"] = r.s()
    song["master"] = r.f32()
    song["compat2"] = list(r.raw(28))
    r.u16(), r.u16()
    r.s(), r.s()
    nsub = r.u8()
    r.raw(3)
    [r.u32() for _ in range(nsub)]
    [r.s() for _ in range(6)]
    song["chipvol"] = [r.f32() for _ in range(3 * len(chips))]
    nconn = r.u32()
    [r.u32() for _ in range(nconn)]
    r.u8()
    r.raw(8)
    slen = r.u8()
    speeds = list(r.raw(16))
    song["speeds"] = speeds[:slen]
    ngroove = r.u8()
    r.raw(17 * ngroove)
    adir = [r.u32() for _ in range(3)]
    for a in adir:
        R(d, a).magic(b"ADIR")
    song["ins"] = [read_ins(d, p) for p in ins_ptr]
    pats = {}
    for p in pat_ptr:
        q = R(d, p)
        q.magic(b"PATN")
        q.u32()
        sub, ch, idx = q.u8(), q.u8(), q.u16()
        q.s()
        rows = []
        while len(rows) < song["patlen"]:
            m = q.u8()
            if m == 0xFF:
                break
            if m & 0x80:
                rows += [None] * ((m & 0x7F) + 2)
                continue
            em = 0
            if m & 32:
                em |= q.u8()
            if m & 64:
                em |= q.u8() << 8
            if m & 8:
                em |= 1
            if m & 16:
                em |= 2
            row = {}
            if m & 1:
                row["note"] = q.u8()
            if m & 2:
                row["ins"] = q.u8()
            if m & 4:
                row["vol"] = q.u8()
            fx = []
            for k in range(8):
                e = q.u8() if em & (1 << (2 * k)) else None
                v = q.u8() if em & (2 << (2 * k)) else None
                if e is not None or v is not None:
                    fx.append((e, v))
            row["fx"] = fx
            rows.append(row if (m & 0x7F) else None)
        if ch >= tchans:
            raise SystemExit(f"patrón del canal {ch} (hay {tchans})")
        pats[(ch, idx)] = rows
    song["pats"] = pats
    return song


def instruments(d):
    """(nombre, tipo, fm) de cada instrumento: por los punteros del formato viejo o, en el nuevo, buscando los bloques
    INS2 (cada uno se valida al leerlo)"""
    if struct.unpack_from("<H", d, 16)[0] < 240:
        return parse_old(d)["ins"]
    out = []
    p = d.find(b"INS2")
    while p >= 0:
        try:
            out.append(read_ins(d, p))
            p = d.find(b"INS2", p + 8 + struct.unpack_from("<I", d, p + 4)[0])
        except (struct.error, IndexError, ValueError):
            p = d.find(b"INS2", p + 4)
    return out


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return
    snes_env = "--envolvente-snes" in argv
    revisar = "--revisar" in argv
    argv = [a for a in argv if a not in ("--envolvente-snes", "--revisar")]
    table = None
    if "--escribir" in argv:
        i = argv.index("--escribir")
        table = argv.pop(i + 1) if i + 1 < len(argv) and argv[i + 1].endswith(".txt") else str(fui2parche.TABLE)
        argv.pop(i)
    d = load(argv[0])
    if revisar:
        s = parse_old(d)
        print(f"versión {s['version']}, {s['name']!r}, chips {[hex(c) for c in s['chips']]}, {s['chans']} canales, "
              f"{s['hz']} Hz, velocidad {s['speeds']}, {s['patlen']} filas x {s['ordlen']} órdenes, "
              f"{len(s['pats'])} patrones, {len(s['ins'])} instrumentos")
        for name, kind, fm in s["ins"]:
            print(f"  {name!r} tipo {kind}" + (f" alg {fm['alg']} fb {fm['fb']}" if fm else ""))
        for ch in range(s["chans"]):
            rows = s["pats"].get((ch, s["orders"][ch][0]), [])
            first = [(j, r) for j, r in enumerate(rows) if r][:3]
            print(f"  canal {ch}: {sum(1 for r in rows if r)} filas con algo en el primer patrón; {first}")
        return
    n = 0
    for name, kind, fm in instruments(d):
        m = NAME_RE.match(name)
        if not m or kind != fui2parche.DIV_INS_FM or fm is None:
            continue
        conj, srcn = f"0x{int(m.group(1), 16):02X}", m.group(2)
        parche, warn = fui2parche.to_parche(fm, snes_env)
        for w in warn:
            print(f"aviso ({name}): {w}", file=sys.stderr)
        line = f"{conj} {srcn} tipo=fm {parche}   # {name} ({Path(argv[0]).name})"
        print(line)
        if table:
            print(f"  {fui2parche.write_line(table, conj, srcn, line, Path(argv[0]).name)} en {table}",
                  file=sys.stderr)
        n += 1
    if not n:
        print("no hay instrumentos FM con nombre CONJ_MUESTRA (como 03_25)", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv[1:])
