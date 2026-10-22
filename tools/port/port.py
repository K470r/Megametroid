#!/usr/bin/env python3
"""MegaMetroid — genera native/src/port_gen.c (docs/42): las funciones del decomp que necesitan los enemigos de
tools/port/roots.txt (y todo lo que llaman y no está escrito a mano en el motor nativo), traducidas por xlate.py,
más los despachos (IA, instrucciones, pre-instrucciones) de esas funciones.
Uso: port.py [roots.txt]"""
import os, re, sys, glob
sys.path.insert(0, os.path.dirname(__file__))
from xlate import Xlate, SRC
HERE = os.path.dirname(os.path.abspath(__file__))
NAT = os.path.join(HERE, '..', '..', 'native', 'src')
OUT = os.path.join(NAT, 'port_gen.c')

NATIVE_PROTOS = {}      # función escrita a mano -> su prototipo (para declararla en port_gen.c)

def native_names():
    names = set()
    for f in glob.glob(os.path.join(NAT, '*.c')):
        if os.path.basename(f) == 'port_gen.c':
            continue
        for m in re.finditer(r'^([A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\(([^;{]*)\))\s*\{', open(f).read(), re.M):
            names.add(m.group(2))
            if not m.group(1).startswith(('static', 'inline')) and '\n' not in m.group(1):
                NATIVE_PROTOS[m.group(2)] = m.group(1)
    return names

def fbank(g, n):
    a = g.funcs[n][3]
    if a:
        return a >> 16
    m = re.match(r'sm_([0-9a-f]{2})\.c', g.funcs[n][0])
    return int(m.group(1), 16) if m else None

def roots_from(path, g):
    roots = []
    for line in open(path):
        line = line.split('#')[0].strip()
        if not line:
            continue
        if line.endswith('*'):                      # familia: Roach_* (solo las de los bancos de enemigos)
            pre = line[:-1]
            roots += [n for n in g.funcs if n.startswith(pre) and not n.startswith('off_') and 'CorpseRotting' not in n
                      and (pre != 'EprojInstr_' or g.funcs[n][3] and g.funcs[n][3] < 0x868300) and fbank(g, n) in ({0xA0} if pre.startswith('EnemyInstr') else {0x86} if pre.startswith('Eproj') else ENEMY_BANKS)]
        else:
            roots.append(line)
    return roots

ENEMY_BANKS = {0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3}
# bancos que se traducen enteros si hacen falta (enemigos, su núcleo $A0 y sus proyectiles $86); de los demás, lo que
# no esté escrito a mano queda como función vacía que avisa (SIN PORTAR) para hacerla a mano
XLATE_BANKS = ENEMY_BANKS | {0xA0, 0x86}

# funciones del decomp que no se traducen (usan la RAM o la VRAM del SNES directamente): quedan vacías y avisan
# hasta que se escriban a mano para la Genesis
MANUAL_STUBS = {'EprojInstr_GotoIfFunc1', 'Torizo_CorpseRottingInitFunc', 'Torizo_CorpseRottingMoveFunc', 'Torizo_CorpseRottingCopyFunc'}

# estructuras de la ROM que lee el decomp con get_X(dirección): en el motor, una copia con los mismos campos
ROM_STRUCTS = {'EnemyDef': 0xA0, 'Vulnerability': 0xB4}

DISPATCH = {'CallEnemyAi': ('void', ''), 'CallEnemyInstr': ('instr', ''), 'CallEnemyPreInstr': ('void', ''),
            'CallEprojInit': ('void', 'uint16 j'), 'CallEprojPreInstr': ('void', 'uint16 k'), 'CallEprojInstr': ('instr', '')}

def main():
    rootsf = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'roots.txt')
    x = Xlate()
    g = x.g
    nat = native_names()
    skip = set(DISPATCH) | {'Unreachable', 'Load24'}
    def bank_of(n):
        a = g.funcs.get(n, (0, 0, 0, None))[3]
        if a is None:
            f = g.funcs.get(n, ('',))[0]
            m = re.match(r'sm_([0-9a-f]{2})\.c', f)
            return int(m.group(1), 16) if m else None
        return a >> 16
    stubs = set(MANUAL_STUBS)
    def stop(n):
        if n in nat or n in skip:
            return True
        if n in MANUAL_STUBS or 'CorpseRotting' in n:
            if n in g.funcs and n not in g.arrays:
                stubs.add(n)
            return True
        if 'Grapple' in n or 'CorpseRotting' in n or (n in g.funcs and n not in g.arrays and bank_of(n) not in XLATE_BANKS):
            if n not in nat and n in g.funcs and n not in g.arrays:
                stubs.add(n)
            return True
        return False
    roots = roots_from(rootsf, g)
    roots = [r for r in roots if 'Grapple' not in r]
    todo = g.closure(roots, stop)
    # proyectiles que crea el código traducido: su cabecera ($86) dice su inicio y su pre-instrucción, que el código no
    # llama directamente (están en datos); se agregan y se vuelve a cerrar hasta que no aparezca nada nuevo
    import xlate as _xl
    names = {}
    for f_ in ('funcs.h', 'ida_types.h'):
        for m in re.finditer(r'(?:#define (addr_\w+) |\b(addr_\w+) = )0x([0-9a-fA-F]{4})', open(os.path.join(_xl.SRC, f_)).read()):
            names[m.group(1) or m.group(2)] = int(m.group(3), 16)
    byaddr = {}
    for m in re.finditer(r'#define fn(\w+) 0x([0-9A-Fa-f]{6})', open(os.path.join(_xl.SRC, 'funcs.h')).read()):
        byaddr[int(m.group(2), 16)] = m.group(1)
    rw = lambda a: x.rom_value(a, 'uint16')
    done_hdr = set()
    while True:
        new_roots = []
        for n in list(todo):
            if n not in g.funcs:
                continue
            body = g.funcs[n][4]
            cands = re.findall(r'\baddr_(?:kEproj|stru_86|kEprojInit)\w*', body)
            # cabeceras con nombre de otro banco en el decomp (addr_loc_A89E90 = $86:9E90): el 3.er argumento de SpawnEproj*
            cands += re.findall(r'SpawnEproj\w*\([^;]*?,\s*(addr_\w+)\)', body)
            for a in cands:
                h_ = names.get(a)
                if h_ is None and re.search(r'_[0-9A-F]{2}([0-9A-F]{4})$', a):
                    h_ = int(a[-4:], 16)
                if h_ is None or h_ < 0x8000 or h_ in done_hdr:
                    continue
                done_hdr.add(h_)
                for off in (0, 2):
                    fa = 0x860000 | rw(0x860000 | (h_ + off))
                    if fa in byaddr:
                        new_roots.append(byaddr[fa])
        # las listas de instrucciones de esos proyectiles (la cabecera + 4 y las que el código pone en
        # eproj_instr_list_ptr): sus instrucciones solo están en datos ($86), como EprojInstr_D15C de los bichos del
        # Kago o $86:A050 del láser de los piratas. Se recorren con lo que dice cada instrucción en el decomp: cuántas
        # palabras usa (return epjp + N), si salta (GET_WORD(epjp)) o si es una pre-instrucción usada de instrucción
        lists = set(h_ + 4 for h_ in done_hdr)
        for n in list(todo):
            if n in g.funcs:
                for a in re.findall(r'eproj_instr_list_ptr\[[^\]]*\] = (\w+);', g.funcs[n][4]):
                    v_ = names.get(a)
                    if v_ is None and re.search(r'_86([0-9A-F]{4})$', a):
                        v_ = int(a[-4:], 16)
                    if v_ is not None:
                        lists.add(('l', v_))
        starts = set()
        for it_ in lists:
            starts.add(it_[1] if isinstance(it_, tuple) else rw(0x860000 | it_))
        seen_l = set()
        todo_l = [s_ for s_ in starts if s_ >= 0x8000]
        while todo_l:
            p_ = todo_l.pop()
            for _ in range(256):
                if p_ < 0x8000 or p_ in seen_l:
                    break
                seen_l.add(p_)
                v_ = rw(0x860000 | p_)
                if not v_ & 0x8000:
                    p_ += 4
                    continue
                nm = byaddr.get(0x860000 | v_)
                if not nm or nm not in g.funcs:
                    break
                new_roots.append(nm)
                body = g.funcs[nm][4]
                sig = re.match(r'(?:static )?const uint8 \*\w+\(uint16 \w+, const uint8 \*(\w+)\)', body)
                if not sig:
                    if re.match(r'(?:static )?void \w+\(uint16 \w+\)', body):
                        p_ += 2                    # pre-instrucción usada de instrucción: sigue
                        continue
                    break
                P = sig.group(1)
                steps, nxt = set(), []
                for r_ in re.findall(r'return ([^;]*);', body):
                    r_ = r_.strip()
                    mm = re.fullmatch(r'%s(?: \+ (\d+))?' % P, r_)
                    if mm:
                        steps.add(int(mm.group(1) or 0))
                    elif re.fullmatch(r'INSTRB_RETURN_ADDR\(GET_WORD\(%s(?: \+ (\d+))?\)\)' % P, r_):
                        o_ = re.search(r'\+ (\d+)', r_)
                        nxt.append(rw(0x860000 | (p_ + 2 + (int(o_.group(1)) if o_ else 0))))
                todo_l += nxt
                if not steps:
                    break
                for st_ in sorted(steps)[1:]:
                    todo_l.append(p_ + 2 + st_)
                p_ = p_ + 2 + min(steps)
        new_roots = [r_ for r_ in new_roots if r_ in g.funcs and r_ not in todo and r_ not in stubs and not stop(r_)]
        if not new_roots:
            break
        roots += new_roots
        todo = g.closure(roots, stop)
    funcs = sorted((n for n in todo if n in g.funcs and not n in g.arrays),
                   key=lambda n: (g.funcs[n][0], g.funcs[n][1]))
    arrays = sorted(n for n in todo if n in g.arrays)
    out = ['/* Generado por tools/port/port.py (docs/42): funciones del decomp (snesrev sm/src) traducidas al motor',
           '   nativo con tools/port/xlate.py. No editar: se cambia el traductor o se escribe la función a mano en otro',
           '   archivo (entonces deja de generarse). */',
           '#include "port_gen.h"', '']
    protos = []
    bodies = []
    for n in funcs:
        a = g.funcs[n][3]
        bank = (a >> 16) if a else 0xA0
        t = x.translate(n, bank)
        # tabla local del decomp que ya se lee de la ROM (BW_XX): su declaración sobra
        t = re.sub(r'\n\s*static const \w+ \(+\w+\)B[WB]_\w+\([^;]*\)+ = \{[^}]*\};', '', t)
        hdr = t.split('\n', 1)[0]
        proto = re.sub(r'\s*\{.*$', ';', hdr)
        protos.append(proto if proto.startswith('static') else proto)
        bodies.append(t)
    for n in arrays:
        protos.append(g.arrays[n].split('=')[0].strip() + ';')
    # funciones escritas a mano que llama lo traducido: su prototipo nativo
    called = set()
    for n in funcs:
        called |= set(g.calls.get(n, []))
    for n in sorted(called):
        if n in nat and n in NATIVE_PROTOS:
            protos.insert(0, NATIVE_PROTOS[n] + ';')
    # tipos propios de los archivos del decomp de donde salen las funciones (typedef struct ... / typedef ... Func_)
    files = sorted({g.funcs[n][0] for n in funcs})
    from xlate import SRC
    for f in files:
        t = open(os.path.join(SRC, f), encoding='utf-8', errors='replace').read()
        for m in re.finditer(r'^typedef struct \w+ \{.*?^\} \w+;|^typedef [^;{]*\(.*?\);', t, re.M | re.S):
            out.append(m.group(0))
    out.append('')
    # funciones de otros bancos que no están escritas a mano: vacías, avisan
    stub_bodies = []
    for n in sorted(stubs):
        hdr = g.funcs[n][4].split('\n', 1)[0]
        if re.match(r'const uint16 \*\w+\(uint16 k, const uint16 \*jp\)', hdr):
            continue
        hdr = re.sub(r'^static ', '', hdr)
        m = re.match(r'(.*?)\b%s\((.*)\)\s*\{' % n, hdr)
        if not m:
            continue
        rt = m.group(1).strip()
        protos.append('%s %s(%s);' % (rt, n, m.group(2)))
        ret = '' if rt == 'void' else ' return (%s){0};' % rt if not rt.endswith('*') and rt not in ('uint8', 'uint16', 'int16', 'uint32', 'int32', 'bool', 'int') else ' return 0;'
        stub_bodies.append('%s %s(%s) { nt_unimpl("%s");%s }' % (rt, n, m.group(2), n, ret))
    out += protos + ['']
    out += stub_bodies + ['']
    for n in arrays:
        out.append(g.arrays[n])
    out.append('')
    for tn in sorted(x.need_tables):
        out.append(re.sub(r'^static const', 'static const', x.table_defs[tn]))
    out += bodies
    # despachos
    out.append('void port_nop_v(void) {}\nvoid port_nop_k(uint16 k) { (void)k; }   /* nullsubs: quien llama puede saltarlas */')
    have = set(funcs)
    for d, (kind, params) in DISPATCH.items():
        body = g.funcs[d][4]
        if d == 'CallEnemyAi':      # el motor llama por CallEnemyAi también a las de tocar y recibir disparos
            body += g.funcs['CallHitboxTouch'][4] + g.funcs['CallHitboxShot'][4]
        cases = []
        for m in re.finditer(r'case (fn\w+): (?:return )?(\w+)\(([^)]*)\);(?: return;)?', body):
            fn, target, args = m.group(1), m.group(2), m.group(3)
            a = g.funcs.get(fn[2:], (0, 0, 0, None))[3]
            if a is None:
                a = x.fnaddr.get(fn[2:])
            if target in have and a is not None:
                cases.append((a, target, args))
        for m in re.finditer(r'case (fn\w+): return;', body):        # nullsubs y similares: no hacen nada
            a = g.funcs.get(m.group(1)[2:], (0, 0, 0, None))[3]
            if a is None:
                a = x.fnaddr.get(m.group(1)[2:])
            if a is not None and (a >> 16) in XLATE_BANKS:
                cases.append((a, None, None))
        if kind == 'instr':
            out.append('uint16 port_%s(uint32 ea, uint16 k, uint16 jp, uint8 *ok) {  /* ok = 0: no está traducida */' % d)
            cases = [(a_, t, 'k, jp') for a_, t, _ in cases]
            out.append('  *ok = 1;\n  switch (ea) {')
            for a, t, args in sorted(cases):
                if not t:
                    continue
                if re.match(r'(?:static )?void %s\(uint16 \w+\)' % t, g.funcs[t][4]):   # f(k); return j;
                    out.append('  case 0x%06X: %s(k); return jp;' % (a, t))
                else:
                    out.append('  case 0x%06X: return %s(k, jp);' % (a, t))
            out.append('  }\n  *ok = 0;\n  return 0;\n}\n')
        else:
            # tabla ordenada (dirección -> función) y búsqueda: quien llama guarda el puntero (eproj.c, enemy_main.c)
            # f(cur_enemy_index) en un despacho sin argumentos: una función de paso
            if all(not args or args == 'cur_enemy_index' for a, t, args in cases if t) and any(args == 'cur_enemy_index' for a, t, args in cases if t):
                nc = []
                for a, t, args in cases:
                    if t and args == 'cur_enemy_index':
                        out.append('static void port_w_%s(void) { %s(cur_enemy_index); }' % (t, t))
                        nc.append((a, 'port_w_' + t, ''))
                    else:
                        nc.append((a, t, args))
                cases = nc
            withk = any(t and args for a, t, args in cases)          # f(k) / f(j); si no, f()
            ft = 'PortFnK' if withk else 'PortFnV'
            nop = 'port_nop_k' if withk else 'port_nop_v'
            ents = []
            for a, t, args in sorted(cases):
                if t and args not in ('', 'k', 'j', 'cur_enemy_index'):
                    raise SystemExit('despacho %s: argumentos %s' % (d, args))
                ents.append((a, t if t else nop))
            out.append('static const struct { uint32 ea; %s f; } kPort%s[%d] = {' % (ft, d, len(ents)))
            out.append(',\n'.join('  {0x%06X, %s}' % (a, t) for a, t in ents) + '\n};')
            out.append('%s port_Find%s(uint32 ea) {  /* 0: no está traducida; las últimas, recordadas */' % (ft, d))
            out.append('  static uint32 cea[16]; static %s cf[16];' % ft)
            out.append('  uint16 h = ((uint16)ea ^ ((uint16)ea >> 4)) & 15;\n  if (cea[h] == ea) return cf[h];')
            out.append('  %s f = 0;\n  int lo = 0, hi = %d;' % (ft, len(ents) - 1))
            out.append('  while (lo <= hi) {\n    int m = (lo + hi) >> 1;\n    uint32 e = kPort%s[m].ea;' % d)
            out.append('    if (e == ea) { f = kPort%s[m].f; break; }\n    if (e < ea) lo = m + 1; else hi = m - 1;\n  }' % d)
            out.append('  cea[h] = ea; cf[h] = f;\n  return f;\n}')
            out.append('uint8 port_%s(uint32 ea%s) {  /* 0: no está traducida */' % (d, ', ' + params if params else ''))
            out.append('  %s f = port_Find%s(ea);\n  if (!f) return 0;\n  f(%s);\n  return 1;\n}\n'
                       % (ft, d, (params.split()[-1] if params else 'cur_enemy_index') if withk else ''))
    open(OUT, 'w').write('\n'.join(out) + '\n')
    banks = sorted(x.used_banks | {(g.funcs[n][3] >> 16) for n in funcs if g.funcs[n][3]} | set(ROM_STRUCTS.values()))
    open(os.path.join(HERE, 'banks.txt'), 'w').write(' '.join('%02X' % b for b in banks) + '\n')
    h = ['/* Generado por tools/port/port.py: lectura de los bancos de la ROM (kBankXXB, gen/tables.c) */',
         '#ifndef PORT_GEN_H', '#define PORT_GEN_H', '#include "nt.h"', '#include "dcompat.h"', '#include "samus.h"',
         '#include "game.h"', '#include "enemies.h"', '#include "plm.h"', '#include "hdmaobj.h"', '#include "room.h"', '#include "nmath.h"', '#include "../gen/tables.h"']
    for b in banks:
        h.append('static inline uint16 BW_%02X(uint16 a) { const uint8 *p = kBank%02XB + (uint16)(a - 0x8000); '
                 'uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }' % (b, b))
        h.append('static inline uint8 BB_%02X(uint16 a) { return kBank%02XB[(uint16)(a - 0x8000)]; }' % (b, b))
        h.append('#define IW_%02X(a) BW_%02X(a)' % (b, b))
    # estructuras de la ROM (ida_types.h del decomp): copia con los mismos campos
    it = open(os.path.join(SRC, 'ida_types.h')).read()
    for sname, sbank in ROM_STRUCTS.items():
        m = re.search(r'typedef struct %s \{(.*?)\} %s;' % (sname, sname), it, re.S)
        fields, off, decl, fill = [], 0, [], []
        for line in m.group(1).split('\n'):
            fm = re.match(r'\s*(uint8|uint16|VoidP|LongPtr|int16)\s+(\w+);', line)
            if not fm:
                continue
            t, f = fm.group(1), fm.group(2)
            sz = {'uint8': 1, 'uint16': 2, 'int16': 2, 'VoidP': 2, 'LongPtr': 3}[t]
            ct = {'uint8': 'uint8', 'uint16': 'uint16', 'int16': 'int16', 'VoidP': 'uint16', 'LongPtr': 'uint32'}[t]
            decl.append('  %s %s;' % (ct, f))
            if sz == 1:
                fill.append('  d->%s = BB_%02X(a + %d);' % (f, sbank, off))
            elif sz == 2:
                fill.append('  d->%s = BW_%02X(a + %d);' % (f, sbank, off))
            else:
                fill.append('  d->%s = BW_%02X(a + %d) | (uint32)BB_%02X(a + %d) << 16;' % (f, sbank, off, sbank, off + 2))
            off += sz
        h.append('typedef struct %s {\n%s\n} %s;' % (sname, '\n'.join(decl), sname))
        getter = re.search(r'static inline %s \*(get_\w+)\(uint16 a\)' % sname, it).group(1)
        h.append('static inline %s *%s(uint16 a) {  /* $%02X:a (copia; 4 a la vez) */\n  static %s b[4]; static uint8 i;\n'
                 '  %s *d = &b[i++ & 3];\n%s\n  return d;\n}' % (sname, getter, sbank, sname, sname, '\n'.join(fill)))
    h += ['static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }',
          'void CallEnemyPreInstr(uint32 ea);']
    h += ['typedef void Func_V(void);', 'typedef uint8 Func_U8(void);', 'typedef void Func_Y_V(uint16 j);',
          'extern uint16 eproj_spawn_x, eproj_spawn_y, eproj_spawn_varE24;',
          'extern uint16 nt_gfx_hook_addr; extern uint8 nt_gfx_hook_bank;', 'uint8 port_call_void(uint32 ea);']
    h += ['uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal);', 'uint32 Multiply16x16(uint16 a, uint16 j);', 'void SpawnPalfxObject(uint16 j);',
          'void SpawnEprojWithGfx(uint16 a, uint16 k, uint16 j);', 'uint8 EprojBlockCollisition_Horiz(uint16 k);',
          'uint8 EprojBlockCollisition_Vertical(uint16 k);']
    h += ['uint8 port_CallEprojInit(uint32 ea, uint16 j);', 'uint8 port_CallEprojPreInstr(uint32 ea, uint16 k);',
          'uint16 port_CallEprojInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);']
    h += ['typedef void (*PortFnV)(void);', 'typedef void (*PortFnK)(uint16);', 'void port_nop_v(void);', 'void port_nop_k(uint16 k);',
          'PortFnV port_FindCallEnemyAi(uint32 ea);', 'PortFnK port_FindCallEnemyPreInstr(uint32 ea);',
          'PortFnK port_FindCallEprojInit(uint32 ea);', 'PortFnK port_FindCallEprojPreInstr(uint32 ea);']
    h += ['uint8 port_CallEnemyAi(uint32 ea);', 'uint16 port_CallEnemyInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);',
          'uint8 port_CallEnemyPreInstr(uint32 ea);', 'uint16 EnemyInstrWord(uint8 bank, uint16 a);',
          '#define IW_EN(a) EnemyInstrWord(gEnemyData(k)->bank, (a))  /* lista del banco del enemigo k */', 'uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w);', '#endif']
    open(os.path.join(NAT, 'port_gen.h'), 'w').write('\n'.join(h) + '\n')
    out2 = ['void CallEnemyPreInstr(uint32 ea) { if (!port_CallEnemyPreInstr(ea)) { nt_unimpl_v("CallEnemyPreInstr", (uint16)(ea >> 8)); nt_unimpl_v("CallEnemyPreInstr lo", (uint16)ea); } }', '']
    out2 += ['uint8 port_call_void(uint32 ea) {  /* cualquier función void(void) traducida, por su dirección */', '  switch (ea) {']
    for n in funcs:
        a_ = g.funcs[n][3]
        if a_ and re.match(r'(static )?void %s\(void\)' % n, g.funcs[n][4]):
            out2.append('  case 0x%06X: %s(); return 1;' % (a_, n))
    out2 += ['  }', '  return 0;', '}', '']
    out2 += ['uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w) {', '  switch (bank) {']
    for b in banks:
        out2.append('  case 0x%02X: *w = BW_%02X(a); return 1;' % (b, b))
    out2 += ['  }', '  return 0;', '}']
    open(OUT, 'a').write('\n'.join(out2) + '\n')
    print('%d funciones, %d arreglos; bancos de ROM %s' % (len(funcs), len(arrays), ' '.join('%02X' % b for b in sorted(x.used_banks))))
    for t in sorted(set(x.todo)):
        print('PENDIENTE:', t)

main()
