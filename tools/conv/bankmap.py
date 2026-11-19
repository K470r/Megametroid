#!/usr/bin/env python3
"""MegaMetroid — el código del 65816 en los bancos del SNES (docs/68).

El port lee de las copias de los bancos (kBankXXB) solo datos: el código del SNES está reescrito en C y nunca se lee.
Aquí se encuentra ese código: desde cada función del decomp (el comentario "// 0xBBAAAA" de su definición) se sigue
el flujo del 65816 (saltos y ramas dentro del banco, con el ancho de A y X que ponen REP/SEP) hasta RTS, RTL, RTI o un
salto sin vuelta. Si una función no se puede seguir sin dudas (un código inválido, un salto a la mitad de una
instrucción ya vista) se descarta entera: se queda como datos.
Uso (módulo): code_bytes(rom, bank) -> set de direcciones $8000-$FFFF que son código seguro."""
import collections
import glob
import re
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "sm" / "src"
_fn = collections.defaultdict(set)
for f in glob.glob(str(SRC / "*.c")):
    t = open(f, errors="ignore").read()
    for m in re.finditer(r"\)\s*\{\s*//\s*0x([0-9A-Fa-f]{6})", t):
        a = int(m.group(1), 16)
        _fn[a >> 16].add(a & 0xFFFF)

# modos de direccionamiento: largo fijo, o 'M' / 'X' (inmediato de 1 o 2 bytes según el flag)
_LEN = {}
_imm_m = {0x09, 0x29, 0x49, 0x69, 0x89, 0xA9, 0xC9, 0xE9}
_imm_x = {0xA0, 0xA2, 0xC0, 0xE0}
_len1 = {0x0A, 0x0B, 0x18, 0x1A, 0x1B, 0x2A, 0x2B, 0x38, 0x3A, 0x3B, 0x40, 0x48, 0x4A, 0x4B, 0x58, 0x5A, 0x5B, 0x60,
         0x68, 0x6A, 0x6B, 0x78, 0x7A, 0x7B, 0x88, 0x8A, 0x8B, 0x98, 0x9A, 0x9B, 0xA8, 0xAA, 0xAB, 0xB8, 0xBA, 0xBB,
         0xC8, 0xCA, 0xCB, 0xD8, 0xDA, 0xDB, 0xE8, 0xEA, 0xEB, 0xF8, 0xFA, 0xFB}
_len3 = {0x0C, 0x0D, 0x0E, 0x19, 0x1C, 0x1D, 0x1E, 0x20, 0x2C, 0x2D, 0x2E, 0x39, 0x3C, 0x3D, 0x3E, 0x4C, 0x4D, 0x4E,
         0x59, 0x5D, 0x5E, 0x62, 0x6C, 0x6D, 0x6E, 0x79, 0x7C, 0x7D, 0x7E, 0x82, 0x8C, 0x8D, 0x8E, 0x99, 0x9C, 0x9D,
         0x9E, 0xAC, 0xAD, 0xAE, 0xB9, 0xBC, 0xBD, 0xBE, 0xCC, 0xCD, 0xCE, 0xD9, 0xDC, 0xDD, 0xDE, 0xEC, 0xED, 0xEE,
         0xF4, 0xF9, 0xFC, 0xFD, 0xFE, 0x44, 0x54}
_len4 = {0x0F, 0x1F, 0x22, 0x2F, 0x3F, 0x4F, 0x5C, 0x5F, 0x6F, 0x7F, 0x8F, 0x9F, 0xAF, 0xBF, 0xCF, 0xDF, 0xEF, 0xFF}
for op in range(256):
    if op in _imm_m:
        _LEN[op] = 'M'
    elif op in _imm_x:
        _LEN[op] = 'X'
    elif op in _len1:
        _LEN[op] = 1
    elif op in _len3:
        _LEN[op] = 3
    elif op in _len4:
        _LEN[op] = 4
    else:
        _LEN[op] = 2
_BRANCH = {0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0}
_END = {0x40, 0x60, 0x6B, 0x5C, 0x6C, 0x7C, 0xDC}       # RTI RTS RTL JML JMP (ind) JMP (ind,X) JML [ind]
_BAD = {0x00, 0x02, 0x42, 0xDB, 0xCB}                     # BRK COP WDM STP WAI: en el código del juego no aparecen


def _off(a):
    return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)


def _trace(rom, bank, entry, m16, x16):
    """-> set de bytes o None si no se puede seguir sin dudas"""
    seen = {}                                 # dirección de instrucción -> largo
    work = [(entry, m16, x16)]
    done = set()
    while work:
        pc, m, x = work.pop()
        while True:
            if (pc, m, x) in done:
                break
            done.add((pc, m, x))
            if pc < 0x8000 or pc > 0xFFFF:
                return None
            op = rom[_off((bank << 16) | pc)]
            if op in _BAD:
                return None
            ln = _LEN[op]
            if ln == 'M':
                ln = 3 if m else 2
            elif ln == 'X':
                ln = 3 if x else 2
            if pc in seen and seen[pc] != ln:
                return None
            seen[pc] = ln
            nb = rom[_off((bank << 16) | pc) + 1] if ln > 1 else 0
            if op == 0xC2:                    # REP
                if nb & 0x20: m = 1
                if nb & 0x10: x = 1
            elif op == 0xE2:                  # SEP
                if nb & 0x20: m = 0
                if nb & 0x10: x = 0
            if op in _BRANCH or op == 0x80:
                d = nb - 256 if nb >= 128 else nb
                work.append(((pc + 2 + d) & 0xFFFF, m, x))
                if op == 0x80:
                    break
            elif op == 0x82:                  # BRL
                w = rom[_off((bank << 16) | pc) + 1] | rom[_off((bank << 16) | pc) + 2] << 8
                work.append(((pc + 3 + w) & 0xFFFF, m, x))
                break
            elif op == 0x4C:                  # JMP abs (en el banco)
                w = rom[_off((bank << 16) | pc) + 1] | rom[_off((bank << 16) | pc) + 2] << 8
                work.append((w, m, x))
                break
            elif op in _END:
                break
            pc += ln
    # las instrucciones no pueden pisarse entre sí
    out = set()
    for a, ln in seen.items():
        for k in range(ln):
            if a + k in out and (a + k) not in seen:
                pass
            out.add(a + k)
    starts = sorted(seen)
    for i in range(len(starts) - 1):
        if starts[i] + seen[starts[i]] > starts[i + 1]:
            return None
    return out


def code_bytes(rom, bank):
    """el código seguro del banco (el decomp: casi todo corre con A y X de 16 bits; si no sale, se prueba 8)"""
    out = set()
    for e in sorted(_fn[bank]):
        for m, x in ((1, 1), (0, 1), (1, 0), (0, 0)):
            r = _trace(rom, bank, e, m, x)
            if r is not None:
                out |= r
                break
    return out
