#!/usr/bin/env python3
"""MegaMetroid — despachos del final (docs/63): los casos de los switch del decomp (sm_8b.c) de las funciones del final
($8B:D480 en adelante) para los objetos sprite (inicio, pre-instrucción, instrucción) y las instrucciones de los objetos
de BG. cinematic.c / intro.c los llaman cuando no es un caso de la intro. Escribe src/core/ending_disp.c.
Uso: ending_disp.py"""
import os, re
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
s = open(os.path.join(ROOT, 'sm', 'src', 'sm_8b.c')).read()
addr = {n: int(a, 16) for n, a in re.findall(r'#define fn(\w+) 0x([0-9A-F]{6})', open(os.path.join(ROOT, 'sm', 'src', 'funcs.h')).read())}


def cases(disp):
    m = re.search(r'^\w[\w ]*\b%s\(uint32 ea[^)]*\) \{(.*?)^\}' % disp, s, re.M | re.S)
    return [n for n, c in re.findall(r'case fn(\w+): (.*?);', m.group(1)) if addr.get(n, 0) >= 0x8BD480 and not n.startswith('nullsub')]


pre, ins, setup, bg = (cases(d) for d in ('CallCinematicSprPreInstr', 'CallCinematicSprInstr', 'CallCinematicSpriteObjectSetup', 'CallCinematicBgInstr'))
L = ['/* Generado por tools/port/ending_disp.py desde sm_8b.c: no editar (docs/63). */', '#include "samus.h"', '']
L += ['void %s(uint16 k);' % n for n in pre] + ['uint16 %s(uint16 k, uint16 j);' % n for n in ins]
L += ['void %s(uint16 j);' % n for n in setup] + ['uint16 %s(uint16 k, uint16 j);' % n for n in bg] + ['']
L += ['uint8 EndingSpritePreInstr(uint16 f, uint16 k) {', '  switch (f) {']
L += ['  case 0x%04X: %s(k); return 1;' % (addr[n] & 0xFFFF, n) for n in pre]
L += ['  case 0x%04X: return 1;                        /* nullsub_128 */' % (addr['nullsub_128'] & 0xFFFF), '  }', '  return 0;', '}', '']
L += ['uint16 EndingSpriteInstr(uint16 f, uint16 k, uint16 j) {   /* 0xFFFF: no es del final */', '  switch (f) {']
L += ['  case 0x%04X: return %s(k, j);' % (addr[n] & 0xFFFF, n) for n in ins] + ['  }', '  return 0xFFFF;', '}', '']
L += ['uint8 EndingSpriteSetup(uint16 f, uint16 j) {', '  switch (f) {']
L += ['  case 0x%04X: %s(j); return 1;' % (addr[n] & 0xFFFF, n) for n in setup] + ['  }', '  return 0;', '}', '']
L += ['uint16 EndingBgInstr(uint16 f, uint16 k, uint16 j) {   /* 0xFFFF: no es del final */', '  switch (f) {']
L += ['  case 0x%04X: return %s(k, j);' % (addr[n] & 0xFFFF, n) for n in bg] + ['  }', '  return 0xFFFF;', '}']
open(os.path.join(ROOT, 'src', 'core', 'ending_disp.c'), 'w').write('\n'.join(L) + '\n')
print('ending_disp.c: %d + %d + %d + %d casos' % (len(pre), len(ins), len(setup), len(bg)))
