#!/usr/bin/env python3
"""Grafo de llamadas del decomp (sm/src/sm_*.c): funciones, su dirección y a quién llaman.
Uso como módulo: g = Graph(); g.funcs[nombre] = (archivo, inicio, fin, dirección, cuerpo)"""
import re, glob, os
SRC = os.path.join(os.path.dirname(__file__), '..', '..', 'sm', 'src')
HDR = re.compile(r'^([A-Za-z_][\w \*]*?)\b([A-Za-z_]\w*)\(([^;{]*)\)\s*\{\s*(?://\s*0x([0-9A-Fa-f]{6}))?\s*$')


class Graph:
    def __init__(self, src=SRC):
        self.funcs = {}
        self.addr = {}
        for f in sorted(glob.glob(os.path.join(src, 'sm_*.c'))):
            lines = open(f, encoding='utf-8', errors='replace').read().split('\n')
            i = 0
            while i < len(lines):
                m = HDR.match(lines[i])
                if m and not lines[i].startswith((' ', '\t')) and m.group(2) not in ('if', 'while', 'for', 'switch'):
                    name = m.group(2)
                    j = i + 1
                    while j < len(lines) and not lines[j].startswith('}'):
                        j += 1
                    body = '\n'.join(lines[i:j + 1])
                    a = int(m.group(4), 16) if m.group(4) else None
                    self.funcs[name] = (os.path.basename(f), i + 1, j + 1, a, body)
                    if a is not None:
                        self.addr[a] = name
                    i = j + 1
                    continue
                i += 1
        # arreglos de punteros a funciones (static Func_X *const off_XXXX[] = { f, g, ... };): como funciones
        # que "llaman" a sus elementos
        self.arrays = {}
        for f in sorted(glob.glob(os.path.join(src, 'sm_*.c'))):
            t = open(f, encoding='utf-8', errors='replace').read()
            for m in re.finditer(r'^(static )?(\w+) \*const (\w+)\[\w*\] = \{(.*?)\};', t, re.M | re.S):
                self.arrays[m.group(3)] = m.group(0)
                self.funcs.setdefault(m.group(3), (os.path.basename(f), 0, 0, None, 'ARRAY\n' + m.group(4)))
        names = set(self.funcs)
        tok = re.compile(r'\b([A-Za-z_]\w*)\b')
        self.calls = {}
        for n, (_, _, _, _, body) in self.funcs.items():
            b = body.split('\n', 1)[1] if '\n' in body else ''
            self.calls[n] = sorted({t for t in tok.findall(b) if t in names and t != n})

    def closure(self, roots, stop=lambda n: False):
        seen, todo = set(), list(roots)
        while todo:
            n = todo.pop()
            if n in seen or n not in self.funcs or stop(n):
                continue
            seen.add(n)
            todo += self.calls.get(n, [])
        return seen
