#!/usr/bin/env python3
"""MegaMetroid — auditoría: qué hace el SNES (decomp, la biblia) y qué hace ya el motor nativo (docs/49).

Compara, sin correr nada:
  * el decomp (sm/src): las funciones de cada banco (con su dirección) y los despachos (CallEnemyAi,
    CallEnemyPreInstr, CallEnemyInstr, CallEproj*, PLMs, FX, estados de juego, movimientos de Samus);
  * el motor (src/core, src/platform): las funciones escritas a mano (por nombre o por la dirección del comentario),
    lo traducido (port_gen.c: tablas kPortCall*), los `case` de cada despacho y las llamadas a nt_unimpl(...);
  * con la ROM (opcional, sm/sm.smc): las 310 salas/estados alcanzables por puertas (como mkzebes.py) con su
    población de enemigos, PLMs, FX, código de sala y de puerta, scroll y BG; las cabeceras de enemigos ($A0) que
    usan, y para cada una si su IA y sus instrucciones están portadas;
  * lo ya probado: tools/rooms/rooms_*.txt (host/check contra el SNES) y res/snes_ref/zdraw.log (claves de dibujo).

Nada de la ROM se copia a la salida: solo direcciones, cantidades y nombres del decomp.

Uso: python3 tools/audit/audit.py [--rom sm/sm.smc] [--out docs/49_AUDITORIA_TABLAS.md] [--json salida.json]
"""
import argparse
import json
import re
import struct
from collections import defaultdict, Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AREAS = ['Crateria', 'Brinstar', 'Norfair', 'Wrecked Ship', 'Maridia', 'Tourian', 'Ceres', 'Debug']

# ------------------------------------------------------------------ utilidades de texto C


def c_functions(text):
    """definiciones de funciones: nombre -> (línea de la cabecera, cuerpo). Solo funciones en la columna 0."""
    out = {}
    for m in re.finditer(r'^(?!static inline)(?:static |inline )*[A-Za-z_][\w \*]*?\b(\w+)\(([^;{)]*)\)\s*\{([^\n]*)\n',
                         text, re.M):
        name = m.group(1)
        if name in ('if', 'while', 'for', 'switch', 'return', 'sizeof'):
            continue
        start = m.end()
        depth, i = 1, start
        n = len(text)
        while i < n and depth:
            c = text[i]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
            elif c == '/' and text.startswith('//', i):
                j = text.find('\n', i)
                i = n if j < 0 else j
                continue
            elif c == '/' and text.startswith('/*', i):
                j = text.find('*/', i + 2)
                i = n if j < 0 else j + 2
                continue
            elif c == '"' or c == "'":
                q = c
                i += 1
                while i < n and text[i] != q:
                    i += 2 if text[i] == '\\' else 1
            i += 1
        out.setdefault(name, (m.group(0) + text[start:i], m.group(3)))
    return out


ADDR_RE = re.compile(r'0x([89AB][0-9A-F]{5})\b|\$([89AB][0-9A-F]):([0-9A-F]{4})')


def addrs_in(s):
    res = set()
    for m in ADDR_RE.finditer(s):
        res.add(int(m.group(1), 16) if m.group(1) else int(m.group(2) + m.group(3), 16))
    return res


def cases(body, fnmap=None):
    """valores de los `case` de un cuerpo: números (0x...) o nombres fnXxx del decomp"""
    vals = set()
    for m in re.finditer(r'case\s+(0x[0-9A-Fa-f]+|\d+|fn\w+)\s*:', body):
        v = m.group(1)
        if v.startswith('fn'):
            if fnmap and v[2:] in fnmap:
                vals.add(fnmap[v[2:]])
        else:
            vals.add(int(v, 0))
    return vals


def table_eas(text, table):
    m = re.search(r'%s\[\d+\] = \{(.*?)\};' % re.escape(table), text, re.S)
    if not m:
        return {}
    return {int(a, 16): f for a, f in re.findall(r'\{0x([0-9A-F]{6}), (\w+)\}', m.group(1))}


# ------------------------------------------------------------------ el decomp


class Decomp:
    def __init__(self, src):
        self.src = src
        self.fn = {}                       # nombre -> dirección (funcs.h)
        for m in re.finditer(r'#define fn(\w+) 0x([0-9A-Fa-f]{6})', (src / 'funcs.h').read_text()):
            self.fn[m.group(1)] = int(m.group(2), 16)
        self.name_of = {}
        for n, a in self.fn.items():
            self.name_of.setdefault(a, n)
        self.funcs = {}                    # nombre -> (banco, dirección o None)
        self.text = {}
        for f in sorted(src.glob('sm_*.c')):
            bank = f.stem[3:].upper()
            t = f.read_text(errors='replace')
            self.text[bank] = t
            for name, (body, tail) in c_functions(t).items():
                a = re.search(r'//\s*0x([0-9A-Fa-f]{6})', tail)
                addr = int(a.group(1), 16) if a else self.fn.get(name)
                self.funcs.setdefault(name, (bank, addr))
                if addr:
                    self.name_of.setdefault(addr, name)
        a0 = c_functions(self.text['A0'])
        self.ai = self._dispatch(a0['CallEnemyAi'][0])
        self.preinstr = self._dispatch(a0['CallEnemyPreInstr'][0])
        self.instr = self._dispatch(a0['CallEnemyInstr'][0])
        b86 = c_functions(self.text['86'])
        self.eproj_init = self._dispatch(b86['CallEprojInit'][0]) if 'CallEprojInit' in b86 else {}
        self.eproj_pre = self._dispatch(b86['CallEprojPreInstr'][0]) if 'CallEprojPreInstr' in b86 else {}
        b84 = c_functions(self.text['84'])
        self.plm_setup = self._dispatch(b84['CallPlmHeaderFunc'][0])

    def _dispatch(self, body):
        """case fnX: ... -> {dirección: nombre de la función que se llama}"""
        res = {}
        for m in re.finditer(r'((?:case fn\w+:\s*)+)([^\n]*)', body):
            names = re.findall(r'case fn(\w+):', m.group(1))
            call = re.search(r'\b(\w+)\(', m.group(2))
            target = call.group(1) if call else 'return'
            for n in names:
                if n in self.fn:
                    res[self.fn[n]] = target if target not in ('return',) else n
        return res


# ------------------------------------------------------------------ el motor


class Native:
    def __init__(self, root, dec):
        self.files = sorted((root / 'src' / 'core').glob('*.c')) + sorted((root / 'src' / 'platform').glob('*.c'))
        self.funcs = {}                    # nombre -> (archivo, cuerpo)
        self.addr = {}                     # dirección del SNES (comentario en la cabecera) -> nombre
        self.mentioned = set()             # direcciones nombradas en cualquier comentario
        self.unimpl = []                   # (archivo, función, mensaje)
        for f in self.files:
            t = f.read_text(errors='replace')
            rel = f.relative_to(root).as_posix()
            self.mentioned |= addrs_in(t)
            for name, (body, tail) in c_functions(t).items():
                self.funcs.setdefault(name, (rel, body))
                for a in addrs_in(tail):
                    self.addr.setdefault(a, name)
                for m in re.finditer(r'nt_unimpl(?:_v)?\("([^"]*)"', body):
                    self.unimpl.append((rel, name, m.group(1)))
        pg = (root / 'src' / 'core' / 'port_gen.c').read_text(errors='replace')
        self.port_ai = table_eas(pg, 'kPortCallEnemyAi')
        self.port_pre = table_eas(pg, 'kPortCallEnemyPreInstr')
        self.port_einit = table_eas(pg, 'kPortCallEprojInit')
        self.port_epre = table_eas(pg, 'kPortCallEprojPreInstr')
        self.port_instr = cases(self.funcs['port_CallEnemyInstr'][1])
        self.port_einstr = cases(self.funcs['port_CallEprojInstr'][1]) if 'port_CallEprojInstr' in self.funcs else set()
        self.ai_hand = cases(self.funcs['CallEnemyAi'][1])
        ci = self.funcs['CallEnemyInstr'][1]          # ceres_door.c: comunes y el banco $A6
        self.instr_common = {0x80ED, 0x812F} | {v for v in cases(ci) if v >= 0x8000 and v < 0x8190}
        self.instr_a6 = {0xA60000 | v for v in cases(ci) if v >= 0x8000}
        self.plm_setup = cases(self.funcs['CallPlmHeaderFunc'][1], dec.fn)
        self.plm_instr = {}
        for m in re.finditer(r'static uint16 (PlmInstr_\w+)\(uint16 p, uint16 k\) \{  // 0x84([0-9A-F]{4})', pg + (root / 'src/core/plm.c').read_text()):
            self.plm_instr[int(m.group(2), 16)] = m.group(1)
        self.fx = cases(self.funcs['FxTypeFunc'][1])
        self.room_setup = cases(self.funcs['RunRoomSetupCode'][1])
        self.room_main = cases(self.funcs['RunRoomMainCode'][1])
        self.door_setup = cases(self.funcs['RunDoorSetupCode'][1])
        self.game_states = set()
        for name, (fl, body) in self.funcs.items():
            if 'game_frame (estado de juego sin portar)' in body:
                self.game_states = cases(body)
        self.samus_mov = cases(self.funcs['Samus_MovementHandler_Normal'][1]) if 'Samus_MovementHandler_Normal' in self.funcs else set()
        # grafo de llamadas del motor: para saber si una función portada llega a un nt_unimpl
        self.calls = {n: set(re.findall(r'\b(\w+)\(', b)) & self.funcs.keys() for n, (fl, b) in self.funcs.items()}
        self.has_unimpl = {n for n, (fl, b) in self.funcs.items()
                           if re.search(r'nt_unimpl(?:_v)?\("(?!Unreachable)', b) and not n.startswith(('Call', 'port_'))}
        self._reach = {}

    def ported(self, name, addr):
        return name in self.funcs or (addr is not None and addr in self.addr)

    def native_name(self, name, addr):
        if name in self.funcs:
            return name
        return self.addr.get(addr)

    def partial(self, name):
        """nombres de funciones con nt_unimpl a las que llega esta (lo que falta dentro)"""
        if name in self._reach:
            return self._reach[name]
        seen, todo = set(), [name]
        while todo:
            n = todo.pop()
            if n in seen or n not in self.funcs:
                continue
            seen.add(n)
            todo += self.calls.get(n, ())
        r = sorted(seen & self.has_unimpl)
        self._reach[name] = r
        return r


# ------------------------------------------------------------------ la ROM (opcional)


class Rom:
    def __init__(self, path):
        self.d = Path(path).read_bytes()
        self.hdr = len(self.d) % 0x8000

    def off(self, a):
        return self.hdr + ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)

    def b(self, a):
        return self.d[self.off(a)]

    def w(self, a):
        return struct.unpack_from('<H', self.d, self.off(a))[0]


SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}   # como mkzebes.py


def all_rooms(rom):
    seen, todo = set(), [0xDF45, 0x91F8]
    while todo:
        ra = todo.pop()
        if ra in seen:
            continue
        seen.add(ra)
        for d in room_doors(rom, ra):
            dest = rom.w(0x830000 | d)
            if dest >= 0x8000 and dest not in seen:
                todo.append(dest)
    return sorted(seen)


def room_doors(rom, ra):
    dl, res = 0x8F0000 | rom.w(0x8F0000 | ra + 9), []
    for i in range(64):
        d = rom.w(dl + 2 * i)
        if d < 0x8000:
            break
        res.append(d)
    return res


def room_states(rom, ra):
    q, sts = 0x8F0000 | ra + 11, []
    for _ in range(16):
        k = rom.w(q)
        n = SEL_LEN.get(k)
        if n is None:
            break
        if n == 0:
            sts.append(((q + 2) & 0xFFFF, 'por defecto'))
            break
        cond = {0xE5FF: 'evento/jefe', 0xE612: 'evento %02X' % rom.b(q + 2), 0xE629: 'jefe %02X' % rom.b(q + 2),
                0xE652: 'morfosfera', 0xE669: 'bombas de poder'}[k]
        sts.append((rom.w(q + 2 + n - 2), cond))
        q += 2 + n
    return sts


def population(rom, pop):
    res = []
    e = 0xA10000 | pop
    for _ in range(64):
        ed = rom.w(e)
        if ed == 0xFFFF:
            break
        res.append((ed, rom.w(e + 6)))
        e += 16
    return res


def plm_population(rom, ptr):
    res = []
    p = 0x8F0000 | ptr
    for _ in range(128):
        pid = rom.w(p)
        if pid == 0:
            break
        res.append((pid, rom.w(p + 4)))
        p += 6
    return res


def fx_types(rom, ptr):
    if ptr in (0, 0xFFFF):
        return []
    res = []
    p = 0x830000 | ptr
    for _ in range(8):
        door = rom.w(p)
        if door == 0xFFFF:
            break
        res.append(rom.b(p + 9))
        if door == 0:
            break
        p += 16
    return res


EDEF = {'init': 0x12, 'main': 0x18, 'grapple': 0x1A, 'hurt': 0x1C, 'frozen': 0x1E, 'time_frozen': 0x20,
        'powerbomb': 0x28, 'touch': 0x30, 'shot': 0x32}


# ------------------------------------------------------------------ análisis


class Audit:
    def __init__(self, root, rom_path=None):
        self.root = root
        self.dec = Decomp(root / 'sm' / 'src')
        self.nat = Native(root, self.dec)
        self.rom = Rom(rom_path) if rom_path else None
        self.tested = self._tested_rooms()
        self.drawn = {int(l.split()[0], 16) for l in (root / 'res/snes_ref/zdraw.log').read_text().split('\n')
                      if l.strip()}
        self.ai_ok = self.nat.ai_hand | set(self.nat.port_ai)

    def _tested_rooms(self):
        res = set()
        for f in (self.root / 'tools' / 'rooms').glob('rooms_*.txt'):
            for m in re.finditer(r'^([0-9A-Fa-f]{4}):', f.read_text(), re.M):
                res.add(int(m.group(1), 16))
        res.add(0x9804)                    # Bomb Torizo: 10000 cuadros con 0 diferencias (docs/46, crateria13)
        return res

    # -------------------------------------------------------- funciones del decomp por banco
    def bank_coverage(self):
        rows = []
        per = defaultdict(lambda: [0, 0, 0])
        for name, (bank, addr) in self.dec.funcs.items():
            p = per[bank]
            p[0] += 1
            if self.nat.ported(name, addr):
                p[1] += 1
            elif addr is not None and addr in self.nat.mentioned:
                p[2] += 1
        for bank in sorted(per):
            t, ok, men = per[bank]
            rows.append((bank, t, ok, men))
        return rows

    def fn_status(self, ea, extra_ok=()):
        """estado de una función del despacho de enemigos por su dirección"""
        if ea in extra_ok:
            name = self.dec.name_of.get(ea, '%06X' % ea)
            nn = self.nat.port_ai.get(ea) or self.nat.port_pre.get(ea) or self.nat.addr.get(ea) or name
            miss = self.nat.partial(nn)
            return ('parcial', miss) if miss else ('hecho', [])
        return ('falta', [])

    # -------------------------------------------------------- familias de enemigos (sin ROM)
    def families(self):
        fam = defaultdict(lambda: {'ai': {}, 'pre': {}, 'instr': {}})
        for kind, disp, ok in (('ai', self.dec.ai, self.ai_ok), ('pre', self.dec.preinstr, set(self.nat.port_pre)),
                               ('instr', self.dec.instr, self._instr_ok_all())):
            for ea, target in disp.items():
                name = self.dec.name_of.get(ea, target)
                if name.startswith(('nullsub', 'Enemy_', 'EnemyInstr_', 'Unreachable')) or target == 'return':
                    continue
                f = name.split('_')[0]
                fam[f][kind][ea] = (name, ea in ok)
        return fam

    def _instr_ok_all(self):
        ok = set(self.nat.port_instr) | self.nat.instr_a6
        for ea in self.dec.instr:
            if (ea & 0xFFFF) in self.nat.instr_common:
                ok.add(ea)
        # instrucciones traducidas con firma (k, jp) en port_gen.c, por dirección
        for name, (fl, body) in self.nat.funcs.items():
            m = re.search(r'^uint16 \w+\(uint16 k, uint16 jp\) \{  // 0x([0-9A-F]{6})', body)
            if m:
                ok.add(int(m.group(1), 16))
        return ok

    # -------------------------------------------------------- instrucciones de enemigos en la ROM
    def walk_instr(self, bank, start, seen, missing):
        """recorre una lista de instrucciones de enemigo: junta las instrucciones que el motor no tiene"""
        rom, ok = self.rom, self._instr_ok
        todo = [start]
        steps = self._instr_len
        while todo:
            p = todo.pop()
            for _ in range(256):
                if p < 0x8000 or (bank, p) in seen or len(seen) > 50000:
                    break
                seen.add((bank, p))
                v = rom.w((bank << 16) | p)
                if not v & 0x8000:
                    p += 4
                    continue
                ea = (bank << 16) | v
                if ea not in ok and v not in self.nat.instr_common:
                    missing.add(ea)
                    break
                st = steps.get(v) if v < 0x8190 else steps.get(ea)
                if st is None:
                    break                              # se sabe que está portada, pero no su largo: hasta aquí
                n, jumps, stop = st
                for j in jumps:
                    todo.append(rom.w((bank << 16) | (p + 2 + j)))
                if stop:
                    break
                p += 2 + n

    def _prep_instr(self):
        self._instr_ok = self._instr_ok_all()
        L = {0x806B: (2, (), False), 0x8074: (0, (), False), 0x807C: (0, (), True), 0x80ED: (0, (0,), True),
             0x8110: (2, (0,), False), 0x8108: (2, (0,), False), 0x8123: (2, (), False), 0x812C: (2, (), False),
             0x812F: (0, (), True), 0x813A: (2, (), False), 0x814B: (7, (), False), 0x8173: (0, (), False),
             0x817D: (0, (), False)}
        for name, (fl, body) in self.nat.funcs.items():
            m = re.search(r'^uint16 \w+\(uint16 k, uint16 jp\) \{  // 0x([0-9A-F]{6})', body)
            if not m:
                continue
            rets = re.findall(r'return ([^;]*);', body)
            lens = [int(x) for r in rets for x in re.findall(r'^\(uint16\)\(jp \+ (\d+)\)$', r.strip())]
            if any(r.strip() == 'jp' for r in rets):
                lens.append(0)
            jumps = tuple(int(o or 0) for r in rets for o in re.findall(r'IW_\w+\(jp(?: \+ (\d+))?\)', r))
            L[int(m.group(1), 16)] = (min(lens) if lens else 0, jumps, not lens)
        self._instr_len = L

    def entries_for(self, bank, fam):
        """listas con nombre que usa el código de la familia en el decomp (como mkzkeys.py)"""
        t = self.dec.text.get('%02X' % bank, '')
        res = set()
        for m in re.finditer(r'\b(?:addr_)?k%s\w*_Ilist_([0-9A-F]{4})\b' % re.escape(fam), t):
            res.add(int(m.group(1), 16))
        return res

    # -------------------------------------------------------- salas (con ROM)
    def rooms(self):
        rom = self.rom
        self._prep_instr()
        rooms = all_rooms(rom)
        out, enemies = [], {}
        for ra in rooms:
            area = rom.b(0x8F0000 | ra + 1)
            doors = room_doors(rom, ra)
            door_codes = {rom.w(0x830000 | d + 10) for d in doors} - {0}
            for si, (st, cond) in enumerate(room_states(rom, ra)):
                S = 0x8F0000 | st
                row = {'room': ra, 'area': area, 'state': si, 'st': st, 'cond': cond}
                pop = population(rom, rom.w(S + 8))
                row['enemies'] = Counter(ed for ed, _ in pop)
                for ed, ip in pop:
                    e = enemies.setdefault(ed, {'rooms': set(), 'states': 0, 'count': 0, 'init_params': set()})
                    e['rooms'].add(ra)
                    e['states'] += 1
                    e['count'] += 1
                    e['init_params'].add(ip)
                plms = plm_population(rom, rom.w(S + 0x14))
                row['plms'] = Counter(p for p, _ in plms)
                row['fx'] = fx_types(rom, rom.w(S + 6))
                row['setup'] = rom.w(S + 0x18)
                row['main'] = rom.w(S + 0x12)
                row['scroll'] = rom.w(S + 0xE)
                row['bg'] = rom.w(S + 0x16)
                row['door_codes'] = door_codes
                row['music'] = (rom.b(S + 4), rom.b(S + 5))
                out.append(row)
        self.enemy_rooms = enemies
        return out

    def enemy_header(self, ed):
        rom = self.rom
        A = 0xA00000 | ed
        bank = rom.b(A + 0xC)
        h = {'bank': bank, 'hp': rom.w(A + 4), 'parts': rom.w(A + 0x14), 'boss': rom.w(A + 0x10)}
        for k, o in EDEF.items():
            h[k] = (bank << 16) | rom.w(A + o)
        main = self.dec.name_of.get(h['main']) or self.dec.name_of.get(h['init']) or '?'
        h['family'] = main.split('_')[0] if main != '?' else '?%04X' % ed
        return h

    def enemy_status(self, ed):
        h = self.enemy_header(ed)
        res = {}
        for k in EDEF:
            ea = h[k]
            if ea & 0xFFFF < 0x8000:
                continue
            name = self.dec.name_of.get(ea, '%06X' % ea)
            if ea in self.ai_ok:
                nn = self.nat.port_ai.get(ea) or self.nat.addr.get(ea) or name
                miss = self.nat.partial(nn)
                res[k] = ('parcial' if miss else 'hecho', name, miss)
            else:
                res[k] = ('falta', name, [])
        # instrucciones: listas de la población y las con nombre de la familia
        missing, seen = set(), set()
        starts = set(self.enemy_rooms[ed]['init_params']) | self.entries_for(h['bank'], h['family'])
        for s in starts:
            if s >= 0x8000:
                self.walk_instr(h['bank'], s, seen, missing)
        return h, res, missing


# ------------------------------------------------------------------ salida (Markdown)


def mark(s):
    return {'hecho': 'hecho', 'parcial': 'parcial', 'falta': '**falta**'}.get(s, s)


def fmt_names(names, n=4):
    names = sorted(names)
    s = ', '.join('`%s`' % x for x in names[:n])
    return s + (' (+%d)' % (len(names) - n) if len(names) > n else '')


def write_md(A, rooms, path, js=None):
    nat, dec = A.nat, A.dec
    L = ['# 49b — Tablas de la auditoría (generado)', '',
         'Generado por `python3 tools/audit/audit.py%s`: no editar a mano (el análisis y el orden están en '
         '[docs/49](49_AUDITORIA.md)).' % (' --rom sm/sm.smc' if A.rom else ''), '']
    if not A.rom:
        L += ['> Sin ROM: faltan las tablas de salas y de cabeceras de enemigos (`--rom sm/sm.smc`).', '']

    # ---- bancos
    L += ['## 1. Funciones del decomp por banco', '',
          'Portada = hay en `src/` una función con el mismo nombre o con su dirección en el comentario de la cabecera '
          '(lo traducido en `port_gen.c` cuenta). Mencionada = la dirección aparece en un comentario pero no como '
          'función (suele ser código metido dentro de otra). El decomp tiene funciones que el juego no usa: el 100 % '
          'no es la meta.', '',
          '| Banco | Funciones | Portadas | % | Mencionadas |', '|---|---:|---:|---:|---:|']
    tot = [0, 0, 0]
    for bank, t, ok, men in A.bank_coverage():
        L.append('| $%s | %d | %d | %d %% | %d |' % (bank, t, ok, 100 * ok // max(t, 1), men))
        tot[0] += t; tot[1] += ok; tot[2] += men
    L += ['| **Total** | %d | %d | %d %% | %d |' % (tot[0], tot[1], 100 * tot[1] // tot[0], tot[2]), '']

    # ---- despachos chicos
    L += ['## 2. Despachos del juego', '']
    L += ['### Estados de juego (`kGameStateFuncs`, $82)', '',
          '| Estado | Función del decomp | Motor |', '|---:|---|---|']
    gs = re.search(r'kGameStateFuncs\[\d+\] = \{(.*?)\};', dec.text['82'], re.S).group(1)
    for i, n in enumerate(re.findall(r'(\w+),', gs)):
        L.append('| %d | `%s` | %s |' % (i, n, 'hecho' if i in nat.game_states else '**falta**'))
    L += ['']
    L += ['### Movimientos de Samus (`Samus_MovementHandler_Normal`, $90:A337)', '',
          '| Tipo | Función del decomp | Motor | Lo que falta dentro |', '|---:|---|---|---|']
    mv = re.search(r'kSamusMovementHandlers\w*\[\d+\]\)\(void\) = \{(.*?)\};|Func_V \*const kSamus_MovementHandlers?\w*\[\d+\] = \{(.*?)\};',
                   dec.text['90'], re.S)
    mvs = re.findall(r'&?(Samus_Movement_\w+)', mv.group(1) or mv.group(2)) if mv else []
    for i, n in enumerate(mvs):
        st = 'hecho' if i in nat.samus_mov else ('—' if 'Unused' in n else '**falta**')
        miss = nat.partial(n) if n in nat.funcs else []
        L.append('| %02X | `%s` | %s | %s |' % (i, n, st, fmt_names(miss)))
    L += ['']
    L += ['### FX (`kFxTypeFuncPtrs`, $89:AB82)', '', '| Tipo | Función | Lógica (hdmaobj.c) |', '|---:|---|---|']
    fxl = re.search(r'kFxTypeFuncPtrs\[\d+\] = \{(.*?)\};', dec.text['89'], re.S).group(1)
    for i, n in enumerate(re.findall(r'(\w+),', fxl)):
        if n.startswith('nullsub') or n == 'FxTypeFunc_Null':
            continue
        L.append('| %02X | `%s` | %s |' % (2 * i, n, 'hecho' if 2 * i in nat.fx else '**falta**'))
    L += ['']

    # ---- familias de enemigos
    fam = A.families()
    L += ['## 3. Enemigos por familia (despachos de $A0 del decomp)', '',
          'Familia = la primera palabra del nombre de la función en el decomp. IA = `CallEnemyAi` (init, main, '
          'tocar, disparo, congelado...), pre = `CallEnemyPreInstr`, instr = `CallEnemyInstr`. "Lo que falta dentro": '
          'funciones del motor con `nt_unimpl` a las que llega la IA portada.', '',
          '| Familia | IA | pre | instr | Estado | Lo que falta dentro |', '|---|---:|---:|---:|---|---|']
    fam_rows = []
    for f in sorted(fam):
        d = fam[f]
        cnt = {k: (sum(1 for _, ok in d[k].values() if ok), len(d[k])) for k in ('ai', 'pre', 'instr')}
        done = sum(c[0] for c in cnt.values())
        total = sum(c[1] for c in cnt.values())
        miss = set()
        for ea, (name, ok) in d['ai'].items():
            if ok:
                miss |= set(nat.partial(nat.port_ai.get(ea) or nat.addr.get(ea) or name))
        st = 'hecho' if done == total and not miss else ('parcial' if done else '**falta**')
        fam_rows.append((f, cnt, st, miss))
        L.append('| %s | %s | %s | %s | %s | %s |' % (f, *('%d/%d' % cnt[k] if cnt[k][1] else '—' for k in ('ai', 'pre', 'instr')),
                                                    st, fmt_names(miss, 3)))
    L += ['']

    # ---- cabeceras y salas (ROM)
    js_out = {'banks': A.bank_coverage(), 'families': {f: [c, s, sorted(m)] for f, c, s, m in fam_rows}}
    if A.rom:
        ens = {}
        L += ['## 4. Cabeceras de enemigos usadas en las salas ($A0, desde la ROM)', '',
              'Una fila por cabecera que aparece en alguna población de los estados alcanzables. IA: init / main / '
              'tocar / disparo (las demás, si faltan, en la última columna). Instr: instrucciones de sus listas '
              '(las de la población y las con nombre de la familia) que el motor no tiene. Dibujo: salas donde aparece '
              'con claves en `zdraw.log` / salas. Probado: salas donde aparece con `host/check` contra el SNES.', '',
              '| Cabecera | Familia | Banco | Salas | init | main | tocar | disparo | Otras IA que faltan | Instr. sin portar | Dibujo | Probado |',
              '|---|---|---|---:|---|---|---|---|---|---|---|---|']
        for ed in sorted(A.enemy_rooms, key=lambda e: (A.enemy_header(e)['family'], e)):
            h, st, miss = A.enemy_status(ed)
            rs = A.enemy_rooms[ed]['rooms']
            other = [k for k in ('frozen', 'grapple', 'powerbomb', 'hurt', 'time_frozen') if k in st and st[k][0] == 'falta']
            dr = len(rs & A.drawn)
            te = len(rs & A.tested)
            core = [st.get(k, ('—',))[0] for k in ('init', 'main', 'touch', 'shot')]
            ens[ed] = {'family': h['family'], 'bank': h['bank'], 'rooms': sorted(rs), 'ai': {k: v[0] for k, v in st.items()},
                       'missing_instr': sorted(miss), 'other_missing': other, 'drawn': dr, 'tested': te,
                       'partial': sorted({x for v in st.values() for x in v[2]})}
            L.append('| %04X | %s | $%02X | %d | %s | %s | %s | %s | %s | %s | %d/%d | %d/%d |' % (
                ed, h['family'], h['bank'], len(rs), *(mark(c) for c in core), ', '.join(other) or '—',
                ', '.join('%06X' % m for m in sorted(miss)[:3]) + (' …' if len(miss) > 3 else '') if miss else '—',
                dr, len(rs), te, len(rs)))
        L += ['']
        js_out['enemies'] = {'%04X' % k: v for k, v in ens.items()}

        # PLMs
        plm_all = Counter()
        for r in rooms:
            plm_all.update(r['plms'].keys())
        plm_st = {}
        for pid in plm_all:
            setup = 0x840000 | A.rom.w(0x840000 | pid)
            ok = setup in nat.plm_setup or setup in nat.addr or dec.name_of.get(setup, '') in nat.funcs
            # instrucciones de su lista
            missing = set()
            seen = set()
            todo = [A.rom.w(0x840000 | pid + 2)]
            while todo:
                p = todo.pop()
                for _ in range(200):
                    if p < 0x8000 or p in seen:
                        break
                    seen.add(p)
                    v = A.rom.w(0x840000 | p)
                    if v < 0x8000:
                        p += 4
                        continue
                    if v not in nat.plm_instr:
                        missing.add(v)
                        break
                    break   # el largo de cada instrucción lo sabe plmscan.py; aquí basta con la primera
            plm_st[pid] = (ok, dec.name_of.get(setup, '%06X' % setup), missing)
        L += ['## 5. PLMs usados en las salas', '',
              'Inicio = la función de la cabecera ($84) está en `CallPlmHeaderFunc`. Instrucción = la primera '
              'instrucción de su lista está en `plm.c` (el recorrido completo es el de `tools/conv/plmscan.py`).', '',
              '| PLM | Inicio | Estados que lo usan | Estado |', '|---|---|---:|---|']
        for pid in sorted(plm_all):
            ok, nm, miss = plm_st[pid]
            st = 'hecho' if ok and not miss else ('parcial' if ok or not miss else '**falta**')
            L.append('| %04X | `%s` | %d | %s%s |' % (pid, nm, plm_all[pid], st,
                                                       ' (instr. %s)' % ', '.join('%04X' % m for m in sorted(miss)) if miss else ''))
        L += ['']

        # salas por área
        L += ['## 6. Salas por área (cada estado alcanzable)', '',
              'Enemigos: cabecera×cantidad (en negrita las que tienen IA sin portar). PLMs: los que faltan / total. '
              'FX: tipos (en negrita si la lógica falta). Código: de sala (inicio/cada cuadro) y de las puertas que '
              'salen de ella, si falta. Dibujo: la sala tiene claves en `zdraw.log`. Probado: `host/check` 0 '
              'diferencias (tools/rooms). Veredicto: lo que falta para jugarla igual que el SNES.', '']
        js_rooms = []
        for area in range(8):
            rs = [r for r in rooms if r['area'] == area]
            if not rs:
                continue
            ok_n = 0
            body = []
            for r in rs:
                bad_en = [ed for ed in r['enemies'] if any(v == 'falta' for k, v in ens[ed]['ai'].items() if k in ('init', 'main', 'touch', 'shot'))]
                part_en = [ed for ed in r['enemies'] if ed not in bad_en and (ens[ed]['missing_instr'] or ens[ed]['partial'])]
                en = ', '.join(('**%04X**' if ed in bad_en else '%04X') % ed + ('×%d' % n if n > 1 else '')
                               for ed, n in sorted(r['enemies'].items()))
                bad_plm = [p for p in r['plms'] if not plm_st[p][0] or plm_st[p][2]]
                fx = ', '.join(('%02X' if t in nat.fx or t == 0 else '**%02X**') % t for t in r['fx']) or '—'
                code = []
                if r['setup'] and r['setup'] not in nat.room_setup:
                    code.append('inicio %04X' % r['setup'])
                if r['main'] and r['main'] not in nat.room_main:
                    code.append('cuadro %04X' % r['main'])
                dc = sorted(c for c in r['door_codes'] if c not in nat.door_setup)
                if dc:
                    code.append('puertas ' + ' '.join('%04X' % c for c in dc))
                drawn = r['room'] in A.drawn or not r['enemies']
                tested = r['room'] in A.tested
                miss = []
                if bad_en:
                    miss.append('IA')
                if part_en:
                    miss.append('IA parcial')
                if bad_plm:
                    miss.append('PLM')
                if any(t not in nat.fx and t for t in r['fx']):
                    miss.append('FX')
                if code:
                    miss.append('código')
                if not drawn:
                    miss.append('dibujo')
                verdict = ', '.join(miss) if miss else ('hecho' if tested else 'hecho sin probar')
                if not miss:
                    ok_n += 1
                body.append('| %04X | %d | %s | %s | %d/%d | %s | %s | %s | %s | %s |' % (
                    r['room'], r['state'], r['cond'], en or '—', len(bad_plm), len(r['plms']), fx,
                    '; '.join(code) or '—', 'sí' if drawn else '**no**', 'sí' if tested else 'no', verdict))
                js_rooms.append({'room': '%04X' % r['room'], 'area': AREAS[area], 'state': r['state'], 'cond': r['cond'],
                                 'enemies': {'%04X' % k: v for k, v in r['enemies'].items()}, 'bad_enemies': ['%04X' % e for e in bad_en],
                                 'plms_missing': ['%04X' % p for p in bad_plm], 'fx': r['fx'], 'code_missing': code,
                                 'drawn': drawn, 'tested': tested, 'verdict': verdict})
            L += ['### %s (área %d): %d estados, %d sin nada pendiente' % (AREAS[area], area, len(rs), ok_n), '',
                  '| Sala | Est. | Condición | Enemigos | PLM falta | FX | Código que falta | Dibujo | Probado | Veredicto |',
                  '|---|---:|---|---|---:|---|---|---|---|---|'] + body + ['']
        js_out['rooms'] = js_rooms

    # ---- nt_unimpl
    L += ['## 7. Avisos `nt_unimpl` en el motor', '',
          'Todo lo que el motor sabe que no tiene (en la Genesis se ve en pantalla). Por archivo.', '',
          '| Archivo | Función | Mensaje |', '|---|---|---|']
    for fl, fn_, msg in sorted(set(nat.unimpl)):
        if msg == 'Unreachable' or 'port_gen' in fl:
            continue
        L.append('| %s | `%s` | %s |' % (fl.replace('src/', ''), fn_, msg.replace('|', '/')))
    pg = [(fn_, msg) for fl, fn_, msg in nat.unimpl if 'port_gen' in fl and msg != 'Unreachable']
    if pg:
        L += ['', 'Dentro de lo traducido (`port_gen.c`): ' + ', '.join('`%s` (%s)' % (f, m) for f, m in sorted(set(pg)))]
    L += ['']
    Path(path).write_text('\n'.join(L))
    if js:
        Path(js).write_text(json.dumps(js_out, indent=1, default=lambda o: sorted(o) if isinstance(o, set) else str(o)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--rom', default=None)
    ap.add_argument('--out', default=str(ROOT / 'docs' / '49_AUDITORIA_TABLAS.md'))
    ap.add_argument('--json', default=None)
    a = ap.parse_args()
    rom = a.rom
    if rom is None and (ROOT / 'sm' / 'sm.smc').exists():
        rom = str(ROOT / 'sm' / 'sm.smc')
    A = Audit(ROOT, rom)
    rooms = A.rooms() if rom else []
    write_md(A, rooms, a.out, a.json)
    print('escrito', a.out, '(con ROM)' if rom else '(sin ROM)')


if __name__ == '__main__':
    main()
