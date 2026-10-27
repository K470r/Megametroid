#!/usr/bin/env python3
"""MegaMetroid — spritemaps de los enemigos de cada sala de Zebes, sin jugar (docs/43).

Los registros de dibujo del motor (MM_DRAWLOG, mkzobj.py) solo tienen lo que se vio al jugar: un enemigo puede tener
cuadros de animación que no salieron. Aquí se recorren las listas de instrucciones de cada enemigo de la sala:
  * los puntos de entrada: el parámetro de la población de la sala (su primera lista), las listas con nombre que usa
    el código de su familia en el decomp (addr_kFamilia_Ilist_XXXX, kFamilia_Ilist_XXXX) y las tablas de listas que
    ese código lee (g_off_XXXXXX, off_XXXXXX);
  * cada lista: (tiempo, spritemap) o una instrucción (>= 0x8000). Las comunes ($A0, al principio de cada banco) se
    conocen; las del enemigo se leen de su traducción (port_gen.c): cuántas palabras salta, si salta a otra lista o
    si se detiene.
Salida (en el formato del registro de dibujo): "sala enemy banco spritemap paleta tiles", para cada enemigo de cada
estado de sala (su paleta y sus tiles como los da el motor) y la paleta 0 (el destello al recibir un disparo).

Uso: mkzkeys.py rom.smc decomp/src port_gen.c salida.log
"""
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import mkmd  # noqa: E402

rom = mkmd.Rom(sys.argv[1])
SRC = Path(sys.argv[2])
PORT = Path(sys.argv[3]).read_text()
OUT = sys.argv[4]

w = rom.w

# nombres del decomp: dirección -> nombre
fn = {}
for m in re.finditer(r'#define fn(\w+) 0x([0-9A-Fa-f]{6})', (SRC / 'funcs.h').read_text()):
    fn[int(m.group(2), 16)] = m.group(1)

# instrucciones traducidas: dirección -> (pasos, saltos_desde_argumento, se_detiene)
instr = {}
for m in re.finditer(r'^uint16 (\w+)\(uint16 k, uint16 jp\) \{  // 0x([0-9A-F]{6})\n(.*?)^\}', PORT, re.M | re.S):
    a, body = int(m.group(2), 16), m.group(3)
    steps, jumps, stop = set(), set(), False
    for r in re.findall(r'return ([^;]*);', body):
        r = r.strip()
        mm = re.fullmatch(r'\(uint16\)\(jp \+ (\d+)\)', r)
        if r == 'jp':
            steps.add(0)
        elif mm:
            steps.add(int(mm.group(1)))
        elif r == '0':
            stop = True
        elif re.fullmatch(r'\(uint16\)\(IW_\w+\(jp(?: \+ (\d+))?\)\)', r) or re.fullmatch(r'IW_\w+\(jp(?: \+ (\d+))?\)', r):
            o = re.search(r'jp \+ (\d+)', r)
            jumps.add(int(o.group(1)) if o else 0)
        elif re.fullmatch(r'\w+\(k, jp\)', r):           # otra instrucción (Goto...)
            jumps.add(0)
        else:
            stop = True                                   # no se sabe: no seguir por ahí
    instr[a & 0xFFFF | (a & 0xFF0000)] = (steps, jumps, stop)

# instrucciones comunes ($A0:8000-818x, copiadas en cada banco): (pasos, salto desde argumento, se detiene)
COMMON = {0x806B: ({2}, set(), False), 0x8074: ({0}, set(), False), 0x807C: (set(), set(), True),
          0x80ED: (set(), {0}, False), 0x8110: ({2}, {0}, False), 0x8108: ({2}, {0}, False),
          0x8123: ({2}, set(), False), 0x812C: ({2}, set(), False), 0x812F: (set(), set(), True),
          0x813A: ({2}, set(), False), 0x814B: ({7}, set(), False), 0x8173: ({0}, set(), False),
          0x817D: ({0}, set(), False)}
REL = {0x80F2, 0x8118}                                    # saltos relativos (byte con signo)


UNKNOWN = set()
EINSTR_SEEN = set()


def walk(bank, start, out, seen):
    todo = [start]
    while todo:
        p = todo.pop()
        while True:
            if p < 0x8000 or (bank, p) in seen or len(seen) > 20000:
                break
            seen.add((bank, p))
            v = w((bank << 16) | p)
            if v & 0x8000:
                if v in REL:
                    d = rom.b((bank << 16) | (p + 2))
                    d = d - 256 if d & 0x80 else d
                    todo.append((p + 2 + d) & 0xFFFF)
                    if v == 0x8118:
                        p += 3
                        continue
                    break
                ent = COMMON.get(v) if v < 0x8190 else instr.get((bank << 16) | v)
                if ent is None:
                    UNKNOWN.add((bank << 16) | v)
                    break
                steps, jumps, stop = ent
                EINSTR_SEEN.add(v)
                for j in jumps:
                    todo.append(w((bank << 16) | (p + 2 + j)))
                if not steps:
                    break
                for s in sorted(steps)[1:]:
                    todo.append(p + 2 + s)
                p = p + 2 + min(steps)
                continue
            sm = w((bank << 16) | (p + 2))
            if sm >= 0x8000 and w((bank << 16) | sm) < 0x80:
                out.add(sm)
            p += 4


def family_of(name):
    return name.split('_')[0] if name else None


# código del decomp por banco: listas con nombre y tablas de listas, por familia
bank_src = {}


def fam_variants(fam):
    """la familia y sus nombres sin las primeras palabras: WalkingSpacePirates usa las listas y funciones
    kSpacePirates_* y SpacePirates_* del decomp (comunes a los piratas)"""
    words = re.findall(r'[A-Z][a-z0-9]*|[a-z0-9]+', fam)
    return [''.join(words[i:]) for i in range(len(words)) if len(''.join(words[i:])) >= 4]


def entries_for(bank, fam):
    return entries_for1(bank, fam)


def entries_for1(bank, fam):
    if bank not in bank_src:
        f = SRC / ('sm_%02x.c' % bank)
        bank_src[bank] = f.read_text(errors='replace') if f.exists() else ''
    t = bank_src[bank]
    res = set()
    for m in re.finditer(r'\b(?:addr_)?k%s\w*_Ilist_([0-9A-F]{4})\b' % re.escape(fam), t):
        res.add(int(m.group(1), 16))
    # funciones de la familia: listas y tablas que usan
    for m in re.finditer(r'^\S[^\n]*\b%s_\w+\([^)]*\) \{.*?^\}' % re.escape(fam), t, re.M | re.S):
        body = m.group(0)
        for mm in re.finditer(r'\baddr_\w*?_?([0-9A-F]{4})\b', body):
            res.add(int(mm.group(1), 16))
        # listas calculadas: current_instruction = ((v & máscara) >> n) +- base (MorphBallEye_Func_3, $A8:9160: una
        # lista por ángulo, ((ángulo & 0xF0) >> 2) - 28756 = $8FAC + 4 * i): todos los valores posibles
        for mm in re.finditer(r'current_instruction = \(\(\w+ & 0x([0-9A-Fa-f]+)\) >> (\d+)\) ([-+]) (\d+);', body):
            mask, sh, base = int(mm.group(1), 16), int(mm.group(2)), int(mm.group(4))
            base = -base if mm.group(3) == '-' else base
            for v in range(mask + 1):
                if v & ~mask == 0:
                    res.add(((v >> sh) + base) & 0xFFFF)
        for mm in re.finditer(r'\b(?:g_)?off_([0-9A-Fa-f]{2})([0-9A-Fa-f]{4})\b', body):
            if int(mm.group(1), 16) != bank:
                continue
            a = int(mm.group(2), 16)
            for i in range(64):
                v = w((bank << 16) | (a + 2 * i))
                if v < 0x8000:
                    break
                res.add(v)
    return res


def eproj_heads(bank, fam):
    res = set()
    for f in fam_variants(fam):
        res |= eproj_heads1(bank, f)
    return res


def eproj_heads1(bank, fam):
    """cabeceras de proyectiles que crea el código de la familia: (dirección, con los tiles de la sala)"""
    entries_for1(bank, fam)
    t = bank_src[bank]
    res = set()
    names = {}
    for m in re.finditer(r'(?:#define (addr_\w+) |\b(addr_\w+) = )0x([0-9a-fA-F]{4})', (SRC / 'funcs.h').read_text() + (SRC / 'ida_types.h').read_text() + t):
        names[m.group(1) or m.group(2)] = int(m.group(3), 16)
    for m in re.finditer(r'^\S[^\n]*\b%s_\w+\([^)]*\) \{.*?^\}' % re.escape(fam), t, re.M | re.S):
        for mm in re.finditer(r'SpawnEproj(WithRoomGfx|WithGfx)\(([^;]*)\);', m.group(0)):
            for a in re.findall(r'\baddr_\w+', mm.group(2)):
                v = names.get(a)
                if v is None:                         # addr_loc_A8D02E: cabecera en $86:D02E con nombre de otro banco
                    m2 = re.search(r'_[0-9A-F]{2}([0-9A-F]{4})$', a)
                    v = int(m2.group(1), 16) if m2 else None
                if v is not None and 0x8000 <= v:
                    res.add((v, mm.group(1) == 'WithRoomGfx'))
    return res


# estados de las salas de Crateria (área 0)
SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}   # como mkzebes.py


def room_states(ra):
    q, sts = 0x8F0000 | ra + 11, []
    for _ in range(16):
        k = w(q)
        n = SEL_LEN.get(k)
        if n is None:
            break
        if n == 0:
            sts.append((q + 2) & 0xFFFF)
            break
        sts.append(w(q + 2 + n - 2))
        q += 2 + n
    return sts


# proyectiles con los tiles comunes (gfx 0): explosiones de muerte de los enemigos (off_86EFD5) y lo que sueltan
# (off_86EF04): los mismos en todas las salas
einstr = {}
for m in re.finditer(r'^uint16 (\w+)\(uint16 k, uint16 (\w+)\) \{  // 0x(86[0-9A-F]{4})\n(.*?)^\}', PORT, re.M | re.S):
    P, a, body = m.group(2), int(m.group(3), 16), m.group(4)
    steps, jumps, stop = set(), set(), False
    for r in re.findall(r'return ([^;]*);', body):
        r = r.strip()
        mm = re.fullmatch(r'\(uint16\)\(%s \+ (\d+)\)' % P, r)
        if r == P:
            steps.add(0)
        elif mm:
            steps.add(int(mm.group(1)))
        elif re.fullmatch(r'\(uint16\)\(BW_86\(%s\)\)' % P, r):
            jumps.add(0)
        else:
            stop = True
    einstr[a & 0xFFFF] = (steps, jumps, stop)
# pre-instrucciones que una lista usa como instrucción (despacho de port_gen.c: "case 0x86XXXX: F(k); return jp;"),
# p. ej. $86:A05C tras EprojInstr_SetPreInstrAndRun en el láser de los piratas: siguen con la palabra de después
for m in re.finditer(r'case 0x86([0-9A-F]{4}): \w+\(k\); return jp;', PORT):
    einstr.setdefault(int(m.group(1), 16), ({0}, set(), False))


# funciones del banco $86 traducidas (port_gen.c): las listas que ponen en eproj_instr_list_ptr, las
# pre-instrucciones que ponen y las funciones que llaman (para seguir un proyectil por todo su código)
PFUN = {}
for m in re.finditer(r'^(?:static )?[\w ]+?\*?\b(\w+)\([^)]*\) \{  // 0x86([0-9A-F]{4})\n(.*?)^\}', PORT, re.M | re.S):
    PFUN[m.group(1)] = (int(m.group(2), 16), m.group(3))
PADDR = {a: n for n, (a, _) in PFUN.items()}
NAMES86 = {}
for m in re.finditer(r'\b(addr_\w+) = 0x([0-9a-fA-F]{4})', (SRC / 'ida_types.h').read_text()):
    NAMES86[m.group(1)] = int(m.group(2), 16)


def code_lists(addrs):
    """listas de instrucciones que pone el código de estas funciones ($86) y de lo que ellas ponen o llaman"""
    todo, seen, lists = [PADDR[a] for a in addrs if a in PADDR], set(), set()
    while todo:
        n = todo.pop()
        if n in seen or n not in PFUN:
            continue
        seen.add(n)
        body = PFUN[n][1]
        for a in re.findall(r'eproj_instr_list_ptr\[[^\]]*\] = (\w+);', body):
            v = NAMES86.get(a)
            if v is None and re.fullmatch(r'0x[0-9A-Fa-f]{4}', a):
                v = int(a, 16)
            if v is not None:
                lists.add(v)
        for a in re.findall(r'eproj_pre_instr\[[^\]]*\] = 0x([0-9A-F]{4})', body):
            if int(a, 16) in PADDR:
                todo.append(PADDR[int(a, 16)])
        for c in re.findall(r'\b(\w+)\(', body):
            if c in PFUN:
                todo.append(c)
    return lists


def ewalk(start, out):
    todo, seen = [start], set()
    while todo:
        p = todo.pop()
        while p >= 0x8000 and p not in seen:
            seen.add(p)
            v = w(0x860000 | p)
            if v in (0xECE3, 0xED17):                # EprojInstr_ECE3/ED17: crean un objeto de sprite (CreateSpriteAtPos)
                out.add(('s', w(0x860000 | (p + 2))))
            if v & 0x8000:
                ent = einstr.get(v)
                if ent is None:
                    UNKNOWN.add(0x860000 | v)
                    break
                steps, jumps, stop = ent
                for j in jumps:
                    todo.append(w(0x860000 | (p + 2 + j)))
                if not steps:
                    break
                for st_ in sorted(steps)[1:]:
                    todo.append(p + 2 + st_)
                p = p + 2 + min(steps)
                continue
            out.add(w(0x860000 | (p + 2)))
            p += 4


def sprite_ilist(idx):
    """spritemaps ($B4) de la lista de un objeto de sprite (kCreateSprite_Ilists[idx], $B4:BDA8): (tiempo, spritemap)"""
    out = set()
    p = w(0xB4BDA8 + 2 * idx)
    for _ in range(64):
        t = w(0xB40000 | p)
        if t & 0x8000:
            break
        out.add(w(0xB40000 | (p + 2)))
        p += 4
    return out


def ep_list(tab, i):
    out = set()
    v = w(tab + 2 * i)
    if v >= 0x8000:
        ewalk(v, out)
    return out


DEATH = [ep_list(0x86EFD5, i) for i in range(5)]       # explosión de muerte por tipo (EnemyDeathAnimation)
PICKUP = [ep_list(0x86EF04, i) for i in range(6)]      # 1 energía, 2 energía grande, 3 power bombs, 4 misiles, 5 súper
# probabilidades de lo que suelta ($B4: energía, energía grande, misil, nada, súper, power bomb) -> PICKUP
DROP_TO_PICKUP = {0: 1, 1: 2, 2: 4, 4: 5, 5: 3}

rooms = sorted({int(x.split(':')[0], 16) for x in sys.argv[5].split(',')}) if len(sys.argv) > 5 else []
lines = set()
for ra in rooms:
    for st in room_states(ra):
        pop = w(0x8F0000 | st + 8)
        ets = w(0x8F0000 | st + 10)
        # tilesets: paleta y tiles de cada enemigo (LoadEnemyGfxIndexes, $A0:8BF3)
        gfx = {}
        f = 0
        e = 0xB40000 | ets
        while w(e) != 0xFFFF:
            ed, dst = w(e), w(e + 2)
            gfx.setdefault(ed, ((dst & 0xF) << 9, f))
            f += (w(0xA00000 | ed) & 0x7FFF) >> 5
            e += 4
        e = 0xA10000 | pop
        while w(e) != 0xFFFF:
            ed, ip = w(e), w(e + 6)
            d = 0xA00000 | ed
            bank = rom.b(d + 0x0C)
            init_ai = w(d + 0x12)
            name = fn.get((bank << 16) | init_ai) or ''
            fam = family_of(name)
            pal, tiles = gfx.get(ed, (2560, 0))
            sms = set()
            seen = set()
            starts = {ip} | (entries_for(bank, fam) if fam else set())
            for s in starts:
                walk(bank, s, sms, seen)
            ext_code = bool(fam) and any('extra_properties |= 4' in mm.group(0) for mm in re.finditer(
                r'^\S[^\n]*\b%s_\w+\([^)]*\) \{.*?^\}' % re.escape(fam), bank_src.get(bank, ''), re.M | re.S))
            if w(e + 10) & 4 or ext_code:              # (Torizo_Init lo pone en su código)
                # spritemaps extendidos (extra_properties & 4, WriteEnemyOams $A0:944A): la lista apunta a una tabla
                # de partes (x, y, spritemap, caja); se dibuja cada parte (las de tilemap, 0xFFFE, no son sprites)
                ext = set()
                # las tablas de un enemigo van seguidas en la ROM: desde la primera que se encontró, todas las de la
                # tira (Torizo: $AA:A4F0-B096; sus listas se eligen en el código y el recorrido no las ve todas)
                def ext_ok(a):
                    n = w((bank << 16) | a)
                    return 1 <= n <= 32 and all(w((bank << 16) | (a + 2 + 8 * i + 4)) >= 0x8000 for i in range(n))
                valid = sorted(a for a in sms if ext_ok(a)) if ext_code else []
                for a in valid:
                    while a < 0xFFF0 and ext_ok(a):
                        sms = sms | {a}
                        a += 2 + 8 * w((bank << 16) | a)
                for sm in sms:
                    n = rom.b((bank << 16) | sm)
                    if not 1 <= n <= 32:
                        continue
                    for i in range(n):
                        sub = w((bank << 16) | (sm + 2 + 8 * i + 4))
                        if sub >= 0x8000 and 1 <= w((bank << 16) | sub) < 0x80:   # spritemap (no tilemap 0xFFFE)
                            ext.add(sub)
                sms = ext
            # paleta 0 puesta por el código de la familia (Torizo_Init: palette_index = 0; los sprites usan la de cada
            # pieza del spritemap)
            pal0 = bool(fam) and any(re.search(r'palette_index = 0;', mm.group(0)) for mm in re.finditer(
                r'^\S[^\n]*\b%s_\w+\([^)]*\) \{.*?^\}' % re.escape(fam), bank_src.get(bank, ''), re.M | re.S))
            init_src = re.search(r'^\S[^\n]*\b%s_Init\(void\) \{.*?^\}' % re.escape(fam), bank_src.get(bank, ''), re.M | re.S) if fam else None
            only0 = bool(init_src) and 'palette_index = 0;' in init_src.group(0)   # (Torizo: siempre la de las piezas)
            for sm in sms:
                for p in ((0,) if only0 else (pal, 0) if pal0 else (pal,)):   # el destello (paleta 0): misma imagen, otra línea (md_engine.c)
                    lines.add('%04X enemy %02X %04X %04X %04X' % (ra, bank, sm, p, tiles))
            # sus proyectiles (los que crea el código de su familia): la lista de la cabecera ($86)
            for hdr, room_gfx in eproj_heads(bank, fam) if fam else ():
                es = set()
                EINSTR_SEEN.clear()
                ewalk(w(0x860000 | (hdr + 4)), es)
                # las listas que pone su código (inicio, pre-instrucciones y sus instrucciones), p. ej. los bichos
                # del Kago ($86:D03C/D04A/D052)
                done_l = set()
                while True:
                    ls = code_lists({w(0x860000 | hdr), w(0x860000 | (hdr + 2))} | EINSTR_SEEN) - done_l
                    if not ls:
                        break
                    for l_ in ls:
                        ewalk(l_, es)
                    done_l |= ls
                for sm in es:
                    lines.add('%04X eproj %04X %04X' % (ra, sm, 0 if room_gfx else (pal | tiles)))
            # al morir: su explosión (0 al dispararle; death_anim) y lo que puede soltar
            eps = DEATH[0] | (DEATH[w(d + 0x22)] if w(d + 0x22) < 5 else set())
            # explosiones que pide el código de la familia con su tipo (Kago_Shot: EnemyDeathAnimation(k, 4))
            if fam:
                for mm in re.finditer(r'^\S[^\n]*\b%s_\w+\([^)]*\) \{.*?^\}' % re.escape(fam), bank_src.get(bank, ''), re.M | re.S):
                    for t_ in re.findall(r'EnemyDeathAnimation\(\w+, (\d)\)', mm.group(0)):
                        if int(t_) < 5:
                            eps |= DEATH[int(t_)]
            dc = w(d + 0x3A)
            if dc:
                for i, pk in DROP_TO_PICKUP.items():
                    if rom.b(0xB40000 | (dc + i)):
                        eps |= PICKUP[pk]
            eps.add(('s', 55))                       # NormalEnemyShotAi: humo al recibir un disparo
            eps.add(('s', 6))                        # disparo que no hace daño (NormalEnemyShotAiSkipDeathAnim, CreateDudShot)
            for sm in eps:
                if isinstance(sm, tuple):
                    for so in sprite_ilist(sm[1]):
                        lines.add('%04X sprobj %04X 0000 0000' % (ra, so))
                else:
                    lines.add('%04X eproj %04X 0000' % (ra, sm))
            print('%04X %04X %s: %d listas, %d spritemaps' % (st, ed, fam, len(starts), len(sms)), file=sys.stderr)
            e += 16
Path(OUT).write_text('\n'.join(sorted(lines)) + '\n')
print('%d claves' % len(lines), file=sys.stderr)
print('instrucciones sin traducir:', ' '.join('%06X %s' % (a, fn.get(a, '?')) for a in sorted(UNKNOWN)), file=sys.stderr)
