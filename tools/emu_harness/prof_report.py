#!/usr/bin/env python3
"""Resumen del perfil de ciclos del 68000 (GPGX con MM_PROFILE) por función.
Uso: prof_report.py prof.bin symbol.txt [n]"""
import sys, struct, bisect, subprocess
prof = open(sys.argv[1], 'rb').read()
cyc = struct.unpack(f'<{len(prof)//4}I', prof)
syms = []
for l in open(sys.argv[2]):
    p = l.split()
    if len(p) >= 3 and p[1] in 'TtWw':
        syms.append((int(p[0], 16), p[2]))
syms.sort()
addrs = [a for a, _ in syms]
tot = {}
for i, c in enumerate(cyc):
    if not c: continue
    k = bisect.bisect_right(addrs, i * 2) - 1
    n = syms[k][1] if k >= 0 else '?'
    tot[n] = tot.get(n, 0) + c
names = list(tot)
try:
    dem = subprocess.run(['c++filt'], input='\n'.join(names), capture_output=True, text=True).stdout.split('\n')
except Exception:
    dem = names
allc = sum(tot.values())
N = int(sys.argv[3]) if len(sys.argv) > 3 else 40
for n, d in sorted(zip(names, dem), key=lambda x: -tot[x[0]])[:N]:
    print(f'{100 * tot[n] / allc:6.2f}%  {tot[n]:12d}  {d}')
