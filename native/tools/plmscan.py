#!/usr/bin/env python3
"""Análisis estático de los PLMs (banco $84): qué bloques pueden dibujar (docs/37).

Recorre las listas de instrucciones de un PLM (las dos listas de la cabecera) siguiendo los saltos, y junta los
bloques de todas sus instrucciones de dibujo. Las instrucciones (su largo y sus saltos) se leen de
native/src/plm.c, que es el port de sm_84.c: así este análisis sigue siempre al motor."""
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
PLM_C = HERE.parent / "src" / "plm.c"


def instr_table():
    """dirección de la instrucción -> (largo de los argumentos, [posiciones de saltos], termina, llamada)"""
    src = PLM_C.read_text()
    tab = {}
    for m in re.finditer(r'static uint16 (PlmInstr_\w+)\(uint16 p, uint16 k\) \{  // 0x84([0-9A-F]{4})\n(.*?)\n\}\n',
                         src, re.S):
        name, addr, body = m.group(1), int(m.group(2), 16), m.group(3)
        lens = [int(x) for x in re.findall(r'return p \+ (\d+);', body)]
        if re.search(r'return p;', body):
            lens.append(0)
        br = [int(x or 0) for x in re.findall(r'return \(?B84W\(p(?: \+ (\d+))?\)\)?;', body)]
        br += [int(x or 0) for x in re.findall(r'PlmInstr_Goto\(p(?: \+ (\d+))?, k\)', body)]
        stop = not lens and not br
        tab[addr] = (max(lens) if lens else 0, sorted(set(br)), stop, name)
    return tab


class PlmScan:
    def __init__(self, rom_w16):
        self.w = rom_w16                      # w(addr24) -> palabra
        self.tab = instr_table()

    def draws_of_list(self, start, seen):
        """punteros de instrucciones de dibujo de una lista"""
        out = set()
        todo = [start]
        while todo:
            p = todo.pop()
            while 0x8000 <= p < 0x10000 and p not in seen:
                seen.add(p)
                f = self.w(0x840000 | p)
                if f < 0x8000:                         # temporizador + instrucción de dibujo
                    out.add(self.w(0x840000 | p + 2))
                    p += 4
                    continue
                t = self.tab.get(f)
                if t is None:
                    break                              # instrucción desconocida: se deja ahí
                ln, br, stop, name = t
                for b in br:
                    todo.append(self.w(0x840000 | p + 2 + b))
                if name == 'PlmInstr_Call':
                    todo.append(p + 4)
                if name in ('PlmInstr_DrawItemFrame0', 'PlmInstr_DrawItemFrame1'):
                    tb = 0xE05F if name.endswith('0') else 0xE077
                    for i in range(4):
                        out.add(self.w(0x840000 | tb + 2 * i))
                # DrawItemFrame0/1 dibujan y esperan 4 cuadros; después la lista sigue con la instrucción siguiente
                # (plm_instr_list_ptrs = p): el cuadro 1 del objeto viene después del 0
                if name in ('PlmInstr_DrawItemFrame0', 'PlmInstr_DrawItemFrame1'):
                    p += 2
                    continue
                if stop or name in ('PlmInstr_Goto', 'PlmInstr_Return', 'PlmInstr_Call', 'PlmInstr_Sleep',
                                    'PlmInstr_Delete', 'PlmInstr_DrawPlmBlock', 'PlmInstr_DrawItemFrame_Common'):
                    break
                p += 2 + ln
        return out

    def blocks_of_draw(self, d):
        """bloques (palabras del nivel) de una instrucción de dibujo"""
        out = []
        if d < 0x8000:
            return out
        p = d
        for _ in range(64):
            n = self.w(0x840000 | p)
            p += 2
            for i in range(n & 0xFF):
                out.append(self.w(0x840000 | p))
                p += 2
            if not self.w(0x840000 | p):
                break
            p += 2
        return out

    def blocks_of_plm(self, header):
        seen, blocks = set(), set()
        for off in (2, 4):                             # lista principal y (cabeceras de 6 bytes) la segunda
            lst = self.w(0x840000 | header + off)
            if lst >= 0x8000:
                for d in self.draws_of_list(lst, seen):
                    blocks.update(self.blocks_of_draw(d))
        return blocks
