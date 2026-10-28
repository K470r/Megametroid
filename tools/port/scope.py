#!/usr/bin/env python3
"""Qué funciones del decomp hacen falta para los enemigos de una lista (con lo que llaman) y cuáles ya existen en
el motor nativo. Uso: scope.py ENEMY_PTR... (hex, banco $A0)"""
import sys, re, glob, os, struct
from cgraph import Graph
ROM = open(os.path.join(os.path.dirname(__file__), '../../sm/sm.smc'), 'rb').read()
H = len(ROM) % 0x8000
def off(a): return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF) + H
def w16(a): o = off(a); return ROM[o] | ROM[o + 1] << 8
AI = {'ai_init': 0x12, 'main_ai': 0x18, 'grapple_ai': 0x1A, 'hurt_ai': 0x1C, 'frozen_ai': 0x1E,
      'time_is_frozen_ai': 0x20, 'powerbomb_reaction': 0x28, 'field_2A': 0x2A, 'touch_ai': 0x30, 'shot_ai': 0x32,
      'field_34': 0x34}
def enemy_roots(g, e):
    b = 0xA00000 | e
    bank = ROM[off(b + 0x0C)]
    out = []
    for k, o in AI.items():
        a = (bank << 16) | w16(b + o)
        if a in g.addr: out.append(g.addr[a])
    return bank, out
NATIVE = set()
for f in glob.glob(os.path.join(os.path.dirname(__file__), '../../src/core/*.c')):
    for m in re.finditer(r'^[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\([^;]*\)\s*\{', open(f).read(), re.M):
        NATIVE.add(m.group(1))
if __name__ == '__main__':
    g = Graph()
    roots = []
    for e in sys.argv[1:]:
        bank, r = enemy_roots(g, int(e, 16))
        prefix = r[0].split('_')[0] if r else None
        fam = [n for n in g.funcs if prefix and n.startswith(prefix + '_') and g.funcs[n][3] and g.funcs[n][3] >> 16 == bank]
        roots += r + fam
    stop = lambda n: n in NATIVE
    need = g.closure(roots, stop)
    have = sorted(n for n in g.closure(roots) if n in NATIVE)
    miss = sorted(need - NATIVE, key=lambda n: (g.funcs[n][0], g.funcs[n][1]))
    tot = 0
    byfile = {}
    for n in miss:
        f, a, b, _, _ = g.funcs[n]
        byfile.setdefault(f, []).append((n, b - a + 1))
    for f, l in byfile.items():
        print('%s: %d funciones, %d líneas' % (f, len(l), sum(x for _, x in l)))
        tot += sum(x for _, x in l)
    print('total %d funciones, %d líneas; ya nativas %d' % (len(miss), tot, len(have)))
    if os.environ.get('LIST'):
        for f, l in byfile.items():
            print(f, ' '.join(n for n, _ in l))
