#!/usr/bin/env python3
"""MegaMetroid — todos los pedidos de efectos de sonido del decomp (docs/55).

Recorre sm/src/*.c (el decomp de snesrev/sm) y tu ROM y lista cada pedido (biblioteca, efecto) con quién lo pide:
  - llamadas con número fijo: QueueSfxN_MaxM(0x..) (con la función y su dirección del SNES, el comentario // 0x...);
  - números elegidos en el código: `(NextRandom() & 1) ? a : b`, y tablas `kXxx[...]` / `g_word_XXXXXX[...]` (las
    tablas se leen de la ROM o del decomp);
  - instrucciones de listas que llevan el efecto como parámetro (PlmInstr_QueueSfx*, EprojInstr_QueueSfx*,
    AnimtilesInstr_QueueSfx*, PalInstr_QueueSfx*, y las de cada enemigo que hacen QueueSfxN(*jp)): se busca la
    dirección de la instrucción como palabra en su banco de la ROM y se lee el número que sigue.
    La búsqueda en el banco puede dar algún número de más (una palabra de datos igual a la dirección): solo agranda
    la lista, nunca saca un efecto que se pide.
Nada de esto se escribe a mano: si el decomp o la ROM cambian, la lista cambia.

Uso: sfxscan.py sm.smc [--tabla]   (--tabla: biblioteca, efecto, banco y funciones que lo piden)"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SMSRC = ROOT / "sm" / "src"

CALL = re.compile(r"\bQueueSfx([123])_Max\d+\(([^;]*?)\);")
FUNC = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\(([^)]*)\)\s*\{\s*//\s*0x([0-9A-Fa-f]{6})", re.M)


def lorom(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def functions():
    """(archivo, nombre, dirección, parámetros, cuerpo) de cada función del decomp con su dirección"""
    out = []
    for f in sorted(SMSRC.glob("sm_*.c")):
        src = f.read_text(errors="replace")
        ms = list(FUNC.finditer(src))
        for i, m in enumerate(ms):
            body = src[m.end():ms[i + 1].start() if i + 1 < len(ms) else len(src)]
            out.append((f.name, m.group(1), int(m.group(3), 16), m.group(2), body))
    return out


# tablas de la ROM que el decomp lee con un índice (g_word_BBAAAA[...]): (paso, cuántas). Las medidas salen del
# índice del decomp: g_word_A3BC4A[projectile_type & 0xF] (Bang_Func_4 $A3:BBEB), g_word_A3EAD6[NextRandom() & 7]
# (Metroid_Instr_1 $A3:EAB1), g_word_A7CDED[g_word_7E9032] (Phantoon_Func_1 $A7:CEED: 0-2), g_word_88B256: pares
# (efecto, tiempo) hasta 0x8000 (HandleEarthquakeSoundEffect $88:B21D)
ROM_TABLES = {"g_word_A3BC4A": (1, 16), "g_word_A3EAD6": (1, 8), "g_word_A7CDED": (1, 3), "g_word_88B256": (2, 8)}


def table_values(rom, src_all, name):
    """los valores de una tabla del decomp (static const uint16 kX[] = {...}) o de la ROM (g_word_BBAAAA)"""
    m = re.search(r"\b%s\[\w*\]\s*=\s*\{([^}]*)\}" % re.escape(name), src_all)
    if m:
        return [int(x, 0) for x in re.findall(r"0x[0-9A-Fa-f]+|\b\d+\b", m.group(1))]
    m = re.match(r"g_word_([0-9A-F]{6})$", name)
    if m and name in ROM_TABLES:
        step, n = ROM_TABLES[name]
        o = lorom(int(m.group(1), 16))
        return [rom[o + 2 * step * i] | rom[o + 2 * step * i + 1] << 8 for i in range(n)]
    return None


def scan(rom):
    funcs = functions()
    src_all = "\n".join(f.read_text(errors="replace") for f in sorted(SMSRC.glob("*.c")))
    reqs = {}                                          # (lib, id) -> {(banco, función)}
    unknown = []                                       # pedidos con número variable que no se resolvieron

    def add(lib, i, bank, who):
        if 0 < i < 0x80:
            reqs.setdefault((lib, i), set()).add((bank, who))

    instr_funcs = []                                   # (lib, dirección, nombre, bytes): el número sigue
    hurt = []
    for fn, name, addr, params, body in funcs:
        bank = addr >> 16
        for lib, arg in CALL.findall(body):
            lib = int(lib)
            arg = arg.strip()
            if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", arg):
                add(lib, int(arg, 0), bank, name)
                continue
            m = re.fullmatch(r"\(NextRandom\(\) & 1\) \? (\d+) : (\d+)", arg)
            if m:
                add(lib, int(m.group(1)), bank, name)
                add(lib, int(m.group(2)), bank, name)
                continue
            m = re.fullmatch(r"(\w+)\[.*\]", arg)
            if m and m.group(1) not in ("jp", "plmp"):
                vals = table_values(rom, src_all, m.group(1))
                if vals:
                    for v in vals:
                        add(lib, v, bank, name)
                    continue
            if arg in ("plmp[0]", "*epjp", "*jp", "jp[0]") or (arg == "*v2" and "RomPtr" in body):
                width = 1 if re.search(r"uint8 \*(plmp|epjp|v2)", params + body) else 2
                instr_funcs.append((lib, addr, name, width))
                continue
            m = re.fullmatch(r"v\d+", arg)
            if m:                                      # una variable: sus valores en la función (número o tabla)
                vals = [int(x, 0) for x in re.findall(r"(?:\b|LOBYTE\()%s\)? = (0x[0-9A-Fa-f]+|\d+);" % arg, body)]
                for t in re.findall(r"\b%s = (\w+)\[" % arg, body):
                    vals += table_values(rom, src_all, t) or []
                if vals:
                    for v in vals:
                        add(lib, v, bank, name)
                    continue
            if arg == "hurt_sfx":                      # el de cada enemigo (EnemyDef.hurt_sfx, +0x0C)
                hurt.append((lib, bank, name))
                continue
            unknown.append((lib, arg, bank, name, addr))
    # instrucciones con el número detrás: la palabra con la dirección en el banco, el número sigue (16 bits)
    for lib, addr, name, width in instr_funcs:
        bank = addr >> 16
        base = lorom(bank << 16 | 0x8000)
        blk = rom[base:base + 0x8000]
        lo, hi = addr & 0xFF, (addr >> 8) & 0xFF
        for p in range(0, len(blk) - 3):
            if blk[p] == lo and blk[p + 1] == hi:
                v = blk[p + 2] | (blk[p + 3] << 8 if width == 2 else 0)
                if 0 < v < 0x80:
                    add(lib, v, bank, name + "@%04X" % (0x8000 + p))
    # EnemyDef.hurt_sfx de cada enemigo (addr_kEnemyDef_XXXX del decomp, banco $A0)
    defs = sorted({int(x, 16) for x in re.findall(r"addr_kEnemyDef_([0-9A-F]{4})", (SMSRC / "ida_types.h").read_text())})
    for lib, bank, name in hurt:
        for d in defs:
            o = lorom(0xA00000 | d) + 0x0C
            add(lib, rom[o] | rom[o + 1] << 8, bank, f"{name}(EnemyDef_{d:04X})")
    return reqs, unknown, funcs


def main():
    rom = open(sys.argv[1], "rb").read()
    rom = rom[len(rom) & 0x7FFF:]
    reqs, unknown, _ = scan(rom)
    for k in sorted(reqs):
        whos = sorted(reqs[k])
        banks = sorted({b for b, _ in whos})
        print(f"{k[0]} {k[1]:#04x} " + " ".join(f"{b:02X}" for b in banks) + "  " +
              ", ".join(sorted({w for _, w in whos}))[:300])
    for u in unknown:
        print("# sin resolver:", u[0], u[1], f"{u[2]:02X}", u[3], f"{u[4]:06X}")


if __name__ == "__main__":
    main()
