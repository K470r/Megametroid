#!/usr/bin/env python3
"""MegaMetroid — instrumento FM de Furnace (.fui, YM2612 / OPN) a una línea de tools/snd/instrumentos.txt (docs/55).

Para ajustar la música a oído: se arma el sonido en Furnace (instrumento "FM (OPN)"), se guarda con "Save instrument"
(.fui) y este script escribe la línea `CONJ SRCN tipo=fm parche=...` con los valores de los registros del YM2612.

Uso: fui2parche.py instrumento.fui [CONJ SRCN] [clave=valor ...] [--escribir [tabla]] [--envolvente-snes]
  CONJ SRCN          conjunto de música (o *) y número de muestra (de `mksnd.py ... --analizar`); sin ellos solo se
                     muestra el parche
  clave=valor        más claves de la línea (oct=, vol=, f0=...)
  --escribir         reemplaza en la tabla (por omisión tools/snd/instrumentos.txt) la línea de ese CONJ SRCN, o la
                     agrega al final
  --envolvente-snes  pone en los portadores ar=31 d1r=0 sl=0 d2r=0 rr=15: la envolvente la da el SNES (en el TL)

Lee los dos formatos de Furnace (como src/engine/instrument.cpp): el nuevo ("FINS", bloque "FM") y el viejo
("-Furnace instr.-"). Furnace guarda los operadores en el mismo orden que la tabla (los registros: S1, S3, S2, S4) y
el detune de 0 a 7 con el centro en 3 (al registro con dtTable, src/engine/platform/fmsharedbase.h).
"""
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TABLE = ROOT / "tools" / "snd" / "instrumentos.txt"
DT_TABLE = (7, 6, 5, 0, 1, 2, 3, 4)            # detune de Furnace -> registro del YM2612
CARRIERS = {7: (0, 1, 2, 3), 6: (1, 2, 3), 5: (1, 2, 3), 4: (2, 3)}   # como Instr.carriers (mksnd.py)
DIV_INS_FM = 1                                 # tipo de instrumento de Furnace: FM (OPN)


def op(dt, mult, tl, rs, ar, dr, sl, d2r, rr, am=0, ssg=0, enable=True):
    return dict(dt=dt, mult=mult, tl=tl, rs=rs, ar=ar, dr=dr, sl=sl, d2r=d2r, rr=rr, am=am, ssg=ssg, enable=enable)


def read_new(d):
    """formato nuevo: "FINS", versión, tipo y bloques (código, largo, datos) (papers/newIns.md)"""
    version, kind = struct.unpack_from("<HH", d, 4)
    p, name, fm = 8, None, None
    while p + 4 <= len(d):
        code, n = d[p:p + 2], struct.unpack_from("<H", d, p + 2)[0]
        blk = d[p + 4:p + 4 + n]
        p += 4 + n
        if code == b"EN":
            break
        if code == b"NA":
            name = blk.split(b"\0")[0].decode("utf-8", "replace")
        elif code == b"FM":
            flags = blk[0]
            count = flags & 15
            alg, fb = (blk[1] >> 4) & 7, blk[1] & 7
            fms, ams = blk[2] & 7, (blk[2] >> 3) & 3
            q = 5 if version >= 224 else 4     # el byte del bloque (OPL) desde la versión 224
            ops = []
            for i in range(count):
                b = blk[q + 8 * i:q + 8 * i + 8]
                ops.append(op(dt=(b[0] >> 4) & 7, mult=b[0] & 15, tl=b[1] & 127, rs=(b[2] >> 6) & 3, ar=b[2] & 31,
                              dr=b[3] & 31, am=b[3] >> 7, d2r=b[4] & 31, sl=(b[5] >> 4) & 15, rr=b[5] & 15,
                              ssg=b[6] & 15, enable=bool(flags & (16 << i))))
            fm = dict(alg=alg, fb=fb, fms=fms, ams=ams, ops=ops)
    return kind, name, fm


def read_old(d):
    """formato viejo: "-Furnace instr.-", versión, reservado, puntero a un bloque "INST" (readInsDataOld)"""
    version = struct.unpack_from("<H", d, 16)[0]
    p = struct.unpack_from("<I", d, 20)[0]
    if d[p:p + 4] != b"INST":
        sys.exit("el .fui viejo no tiene el bloque INST donde dice el puntero")
    p += 4 + 4 + 2                              # "INST", largo, versión
    kind = d[p]
    p += 2
    e = d.index(b"\0", p)
    name = d[p:e].decode("utf-8", "replace")
    p = e + 1
    alg, fb, fms, ams = d[p], d[p + 1], d[p + 2], d[p + 3]
    p += 8
    ops = []
    for _ in range(4):
        b = d[p:p + 32]                          # am ar dr mult rr sl tl dt2 rs dt d2r ssg dam dvb egt ksl sus vib ws ksr en kvs
        ops.append(op(am=b[0], ar=b[1], dr=b[2], mult=b[3], rr=b[4], sl=b[5], tl=b[6], rs=b[8], dt=b[9], d2r=b[10],
                      ssg=b[11], enable=bool(b[20]) if version >= 114 else True))
        p += 32
    return kind, name, dict(alg=alg, fb=fb, fms=fms, ams=ams, ops=ops)


def read_fui(path):
    d = Path(path).read_bytes()
    if d[:4] in (b"FINS", b"FINB"):
        return read_new(d)
    if d[:16] == b"-Furnace instr.-":
        return read_old(d)
    sys.exit(f"{path}: no es un instrumento de Furnace (.fui)")


def to_parche(fm, snes_env=False):
    """(texto parche=..., avisos)"""
    alg, fb, ops = fm["alg"] & 7, fm["fb"] & 7, fm["ops"]
    warn = []
    if len(ops) != 4:
        sys.exit(f"el instrumento tiene {len(ops)} operadores: el YM2612 usa 4 (¿es de OPL u OPLL?)")
    car = CARRIERS.get(alg, (3,))
    names = ("OP1", "OP3", "OP2", "OP4")         # el orden de la tabla y de Furnace (interno 1/3/2/4)
    out = []
    for i, o in enumerate(ops):
        tl = o["tl"]
        if not o["enable"]:
            tl = 127
            warn.append(f"{names[i]} está apagado en Furnace: va con TL=127 (no suena)")
        if o["am"]:
            warn.append(f"{names[i]} usa AM (LFO): la Genesis del juego no lo usa, se ignora")
        if o["ssg"] & 8:
            warn.append(f"{names[i]} usa SSG-EG: la tabla no lo lleva, se ignora")
        ar, dr, sl, d2r, rr = o["ar"], o["dr"], o["sl"], o["d2r"], o["rr"]
        if i in car and snes_env:
            ar, dr, sl, d2r, rr = 31, 0, 0, 0, 15
        elif i in car and (ar, dr, sl, d2r, rr) != (31, 0, 0, 0, 15):
            warn.append(f"{names[i]} es portador y tiene envolvente propia (ar={ar} d1r={dr} sl={sl} d2r={d2r} "
                        f"rr={rr}): se suma a la del SNES; con --envolvente-snes queda solo la del SNES")
        out.append(":".join(str(v) for v in (DT_TABLE[o["dt"] & 7], o["mult"] & 15, tl & 127, o["rs"] & 3, ar & 31,
                                             dr & 31, sl & 15, d2r & 31, rr & 15)))
    if fm["fms"] or fm["ams"]:
        warn.append("el instrumento usa la sensibilidad al LFO (FMS/AMS): se ignora")
    return f"parche={alg},{fb}," + ",".join(out), warn


def norm(x):
    return x if x == "*" else str(int(x, 0))


def write_line(table, conj, srcn, line, src):
    lines = Path(table).read_text(encoding="utf-8").splitlines()
    key = (norm(conj), norm(srcn))
    done = False
    for i, ln in enumerate(lines):
        t = ln.split("#")[0].split()
        if len(t) >= 2 and not ln.lstrip().startswith("#"):
            try:
                if (norm(t[0]), norm(t[1])) == key:
                    lines[i] = line
                    done = True
                    break
            except ValueError:
                continue
    if not done:
        lines += ["", f"# {src} (fui2parche.py)", line]
    Path(table).write_text("\n".join(lines) + "\n", encoding="utf-8")
    return "reemplazada" if done else "agregada"


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return
    snes_env = "--envolvente-snes" in argv
    argv = [a for a in argv if a != "--envolvente-snes"]
    table = None
    if "--escribir" in argv:                    # --escribir [tabla.txt]
        i = argv.index("--escribir")
        if i + 1 < len(argv) and argv[i + 1].endswith(".txt"):
            table = argv.pop(i + 1)
        else:
            table = str(TABLE)
        argv.pop(i)
    path, rest = argv[0], argv[1:]
    kv = [a for a in rest if "=" in a]
    pos = [a for a in rest if "=" not in a]
    kind, name, fm = read_fui(path)
    if fm is None:
        sys.exit(f"{path}: el instrumento no tiene datos de FM")
    if kind != DIV_INS_FM:
        print(f"aviso: el instrumento es de tipo {kind} en Furnace (FM de la Genesis es {DIV_INS_FM}); se toma su FM",
              file=sys.stderr)
    parche, warn = to_parche(fm, snes_env)
    for w in warn:
        print("aviso:", w, file=sys.stderr)
    if len(pos) < 2:
        print(f"{parche}   # {name or Path(path).stem}")
        if table:
            sys.exit("para --escribir hacen falta CONJ y SRCN")
        return
    conj, srcn = pos[0], pos[1]
    line = " ".join([conj, srcn, "tipo=fm", parche] + kv) + f"   # {name or Path(path).stem}"
    print(line)
    if table:
        print(f"{write_line(table, conj, srcn, line, Path(path).name)} en {table}", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv[1:])
