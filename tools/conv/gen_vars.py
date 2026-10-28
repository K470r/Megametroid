#!/usr/bin/env python3
"""Variables del motor nativo (docs/31).

El motor usa los mismos nombres de variable que el decomp (variables.h de snesrev/sm), pero cada una es
una variable global nativa del 68000 (no hay RAM del SNES en la Genesis). Este script busca en src/core
qué nombres del decomp se usan y genera:
  gen/vars.h   declaraciones (tipo del decomp: uint8/uint16, arreglos con su tamaño)
  gen/vars.c   definiciones
  gen/vars_map.c  (solo PC) tabla nombre -> dirección del SNES, para cargar el estado del SNES y comparar
                  cuadro a cuadro (host/check.c)
Uso: gen_vars.py smsrc/src/variables.h src/core:inc/core res/gen/engine
"""
import re, sys
from pathlib import Path

vh, out = Path(sys.argv[1]), Path(sys.argv[3])
# el motor: carpetas separadas por ':' (src/core:inc/core); vars_extra.txt va en la primera
srcs = [Path(d) for d in sys.argv[2].split(':')]
src = srcs[0]
# variables.h y, a su lado, variables_extra.h (la RAM de las cinemáticas: comparte direcciones con los proyectiles
# enemigos; esas pasan a ser trozos de los arreglos de proyectiles, ver ALIAS)
VH_FILES = [vh] + ([vh.parent / 'variables_extra.h'] if (vh.parent / 'variables_extra.h').exists() else [])
# tamaños de arreglos que no se deducen bien de la distancia a la variable siguiente
ARRAY_LEN = {
    # los arreglos de proyectiles tienen 10 entradas (5 rayos + 5 bombas); el decomp nombra aparte la parte de
    # las bombas (bomb_functions, bomb_timers, bomb_subspeeds): esas pasan a ser alias (ver ALIAS abajo)
    # cinemáticas (variables_extra.h): 16 objetos sprite, 4 de fondo, 2 de modo 7
    **{n: 16 for n in ('cinematicspr_whattodraw', 'cinematicbg_arr7', 'cinematicbg_arr8', 'cinematicbg_arr9',
                       'cinematicspr_arr6', 'cinematicspr_arr7', 'cinematicspr_instr_ptr', 'cinematicspr_preinstr_func',
                       'cinematicspr_instr_timer', 'cinematicspr_goto_timer')},
    **{n: 4 for n in ('cinematicbg_instr_ptr', 'cinematicbg_preinstr', 'cinematicbg_instr_timer', 'cinematicbg_arr6')},
    'cinematicbg_arr1': 12,
    **{n: 2 for n in ('mode7_obj_instr_ptr', 'mode7_obj_preinstr_func', 'mode7_obj_instr_timer', 'mode7_obj_goto_timer')},
    'projectile_bomb_pre_instructions': 10, 'enemy_drawing_queue_sizes': 8, 'tilemap_stuff': 2048, 'hdma_table_1': 256, 'scrolling_sky_bg2_hdma_data': 64, 'scrolling_sky_bg2_indirect_hdma': 128, 'projectile_variables': 10, 'projectile_timers': 10,
}
# variables que el motor define a su manera (no como en la RAM del SNES)
# kraid_unk9000 (7E:9000) cae dentro de la RAM extra de enemigos: es ERAM(0x9000) (inc/core/enemies.h)
EXCLUDE = {'level_data', 'BTS', 'kraid_unk9000'}
# variables propias del motor nativo (no están en el SNES): nombre -> declaración
EXTRA = Path(src / 'vars_extra.txt')

defs = {}
src_of = {}   # variable -> archivo (las distancias para el largo de los arreglos se miden dentro del mismo archivo)
for l in ((f, l) for f in VH_FILES for l in f.read_text().splitlines()):
    f, l = l
    m = re.match(r'#define (\w+) \(\*\((u?int(?:8|16))\*\)\(g_ram\+0x([0-9A-Fa-f]+)\)\)', l)
    if m:
        if m.group(1) not in defs:   # definida en los dos archivos: vale la de variables.h
            defs[m.group(1)] = ('s', m.group(2), int(m.group(3), 16)); src_of[m.group(1)] = f
        continue
    m = re.match(r'#define (\w+) \(\((u?int(?:8|16))\*\)\(g_ram\+0x([0-9A-Fa-f]+)\)\)', l)
    if m:
        if m.group(1) not in defs:
            defs[m.group(1)] = ('a', m.group(2), int(m.group(3), 16)); src_of[m.group(1)] = f
addrs_of = {}
for n, (_, _, a) in defs.items():
    addrs_of.setdefault(src_of[n], set()).add(a)
addrs_of = {f: sorted(v) for f, v in addrs_of.items()}

def arr_len(name, t, a):
    if name in ARRAY_LEN:
        return ARRAY_LEN[name]
    nxt = [x for x in addrs_of[src_of[name]] if x > a]
    sz = (nxt[0] - a) if nxt else 2
    return max(1, sz // (2 if '16' in t else 1))

text = ''
for f in sorted(f for d in srcs for f in d.glob('*.[ch]')):
    text += f.read_text()
used = sorted(n for n in set(re.findall(r'\b[A-Za-z_][A-Za-z0-9_]*\b', text)) if n in defs and n not in EXCLUDE)

h = ['/* Generado por tools/conv/gen_vars.py: variables del motor (nombres del decomp, tipos nativos). */',
     '#ifndef GEN_VARS_H', '#define GEN_VARS_H']
c = ['/* Generado por tools/conv/gen_vars.py */', '#include "nt.h"']
m = ['/* Generado por tools/conv/gen_vars.py: variable -> dirección del SNES (solo para la comprobación en PC) */',
     '#include "nt.h"', '#include "vars_map.h"', 'const VarMap g_vars_map[] = {']
# alias: una variable del decomp que cae dentro de un arreglo usado más largo es un trozo de ese arreglo
span = {}
for n in used:
    k, t, a = defs[n]
    if k == 'a':
        sz = 2 if '16' in t else 1
        span[n] = (a, a + arr_len(n, t, a) * sz, t)
ALIAS = {}
# el arreglo que contiene a cada variable: el más largo (y, a igual largo, el de nombre menor); un arreglo que
# contiene a otro nunca pasa a ser alias de él
for n in used:
    k, t, a = defs[n]
    best = None
    for b, (lo, hi, bt) in span.items():
        if b == n or bt != t or not (lo <= a < hi) or src_of[b] != src_of[n]:
            continue
        if n in span and (span[n][1] - span[n][0], b) >= (hi - lo, n) and a == lo:
            continue
        if n in span and span[n][1] > hi:
            continue
        if best is None or (hi - lo, -ord(b[0])) > (best[2] - best[1], 0):
            best = (b, lo, hi)
    if best:
        ALIAS[n] = (best[0], (a - best[1]) // (2 if '16' in t else 1))
# un alias apunta siempre a un arreglo que no es alias
for n in list(ALIAS):
    b, off = ALIAS[n]
    while b in ALIAS:
        b2, off2 = ALIAS[b]; b, off = b2, off + off2
    ALIAS[n] = (b, off)
ramtab = []   # (dirección del SNES, tamaño del elemento, elementos, nombre) para nt_clear_ram
for n in used:
    if n in ALIAS:
        b, off = ALIAS[n]
        if defs[n][0] == 's':
            h.append(f'#define {n} ({b}[{off}])  /* 7E:{defs[n][2]:04X}: elemento de {b} */')
        else:
            h.append(f'#define {n} ({b} + {off})  /* 7E:{defs[n][2]:04X}: parte de {b} */')
        continue
    k, t, a = defs[n]
    if k == 's':
        h.append(f'extern {t} {n};  /* 7E:{a:04X} */')
        c.append(f'{t} {n};')
        m.append(f'  {{ "{n}", &{n}, 0x{a:04X}, {2 if "16" in t else 1}, 1 }},')
        ramtab.append((a, 2 if '16' in t else 1, 1, '&' + n))
    else:
        ln = arr_len(n, t, a)
        h.append(f'extern {t} {n}[{ln}];  /* 7E:{a:04X} */')
        c.append(f'{t} {n}[{ln}];')
        m.append(f'  {{ "{n}", {n}, 0x{a:04X}, {2 if "16" in t else 1}, {ln} }},')
        ramtab.append((a, 2 if '16' in t else 1, ln, n))
if EXTRA.exists():
    for l in EXTRA.read_text().splitlines():
        l = l.split('#')[0].strip()
        if not l:
            continue
        if l.startswith('alias '):                  # alias nombre = arreglo + desplazamiento: la misma RAM del SNES
            n, _, expr = l[6:].partition('=')
            h.append(f'#define {n.strip()} ({expr.strip()})')
            continue
        decl, _, addr = l.partition('@')
        decl = decl.strip()
        h.append(f'extern {decl};')
        c.append(f'{decl};')
        if addr.strip():
            t, n = decl.split()[0], decl.split()[1]
            cnt = 1
            if '[' in n:                         # arreglo: nombre[N]
                n, cnt = n.split('[')[0], int(n.split('[')[1].rstrip(']'))
            ref = n if cnt > 1 else '&' + n
            m.append(f'  {{ "{n}", {ref}, {addr.strip()}, {2 if "16" in t else 1}, {cnt} }},')
            ramtab.append((int(addr.strip(), 0), 2 if '16' in t else 1, cnt, ref))
# tabla de la RAM del SNES (también en la Genesis): para borrar rangos como el SNES (nt_clear_ram, src/nt_ram.c)
ramtab.sort()
h.append('typedef struct { uint16 addr; uint8 esize; uint16 n; void *p; } NtRamVar;')
h.append(f'extern const NtRamVar g_ram_vars[{len(ramtab)}];')
h.append(f'#define NT_RAM_VARS {len(ramtab)}')
c.append(f'const NtRamVar g_ram_vars[{len(ramtab)}] = {{')
for a, es, ln, ref in ramtab:
    c.append(f'  {{ 0x{a:04X}, {es}, {ln}, {ref} }},')
c.append('};')
h.append('#endif')
m.append('  { 0, 0, 0, 0, 0 },')
m.append('};')
out.mkdir(parents=True, exist_ok=True)
(out / 'vars.h').write_text('\n'.join(h) + '\n')
(out / 'vars.c').write_text('\n'.join(c) + '\n')
(out / 'vars_map.c').write_text('\n'.join(m) + '\n')
print(f'{len(used)} variables del decomp')
