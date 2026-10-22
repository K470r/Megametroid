import re
#!/usr/bin/env python3
"""MegaMetroid — traductor del decomp (snesrev sm/src) al motor nativo (docs/42).

El decomp es C que lee la ROM y la RAM del SNES con punteros. El motor nativo tiene las mismas variables (con los
mismos nombres) pero sin la RAM ni la ROM del SNES: los enemigos son EnemyData (64 bytes, igual al decomp) más
enemy_ram7800 (ERAM), y los datos de la ROM se leen por dirección de las tablas de bancos (kBankXXW, sacadas de la
ROM de cada quien al compilar). Este programa pasa funciones del decomp a C nativo con esas reglas:

  * Enemy_X *E = Get_X(k)   ->  EnemyData *E = gEnemyData(k)  (+ la k para la RAM extra)
    E->base.campo           ->  E->campo
    E->x_var_A ...          ->  el campo de EnemyData en esa posición (ai_var_A ... parameter_2)
    E->x_var_00 ...         ->  ERAM(0x7800 + k + desplazamiento)  (la parte de 7E:7800 de la estructura)
  * g_tabla[i] (RomFixedPtr) ->  lectura de la ROM por dirección (BW_XX / BB_XX)
  * FUNC16(F)               ->  la dirección de F en el SNES (16 bits), igual que en la RAM del SNES
  * instrucciones (const uint16 *F(uint16 k, const uint16 *jp)) -> uint16 F(uint16 k, uint16 jp): jp es la
    dirección en el banco del enemigo; *jp / jp[n] se leen con IW; jp + n avanza 2n bytes

Lo que no sabe traducir lo deja marcado con XLATE_TODO (no compila hasta que se revisa a mano).
Uso: xlate.py salida.c banco funcion...   (o @lista.txt)
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cgraph import Graph, SRC

TYPES = os.path.join(SRC, 'enemy_types.h')
BASE_FIELDS = ['enemy_ptr', 'x_pos', 'x_subpos', 'y_pos', 'y_subpos', 'x_width', 'y_height', 'properties',
               'extra_properties', 'ai_handler_bits', 'health', 'spritemap_pointer', 'timer', 'current_instruction',
               'instruction_timer', 'palette_index', 'vram_tiles_index', 'layer', 'flash_timer', 'frozen_timer',
               'invincibility_timer', 'shake_timer', 'frame_counter']
EDATA = {2 * i: n for i, n in enumerate(BASE_FIELDS)}
EDATA.update({0x2E: 'bank', 0x2F: 'field_2F', 0x30: 'ai_var_A', 0x32: 'ai_var_B', 0x34: 'ai_var_C', 0x36: 'ai_var_D',
              0x38: 'ai_var_E', 0x3A: 'ai_preinstr', 0x3C: 'parameter_1', 0x3E: 'parameter_2'})


def parse_structs():
    """Enemy_X -> {campo: (dirección relativa a 7E:0F78+k, tipo, largo del arreglo o 0)}"""
    txt = open(TYPES).read()
    out = {}
    for m in re.finditer(r'typedef struct (Enemy_\w+) \{(.*?)\}\s*\1;', txt, re.S):
        name, body = m.group(1), m.group(2)
        off, fields = 0, {}
        for line in body.split('\n'):
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            if line.startswith('EnemyBase base;'):
                off = 0x30
                continue
            fm = re.match(r'(uint8|uint16|int16|int8|uint32|int32)\s+(\w+)(?:\[([^\]]+)\])?;', line)
            if not fm:
                raise SystemExit('estructura %s: no entiendo "%s"' % (name, line))
            t, f, n = fm.group(1), fm.group(2), fm.group(3)
            sz = {'uint8': 1, 'int8': 1, 'uint16': 2, 'int16': 2, 'uint32': 4, 'int32': 4}[t]
            cnt = eval(n) if n else 0
            if not f.startswith('pad'):
                fields[f] = (0xF78 + off, t, cnt)
            off += sz * (cnt or 1)
        out[name] = fields
    return out


def match_paren(s, i, o='(', c=')'):
    """s[i] == o: índice de su cierre"""
    d = 0
    for j in range(i, len(s)):
        if s[j] == o:
            d += 1
        elif s[j] == c:
            d -= 1
            if d == 0:
                return j
    raise ValueError('sin cierre')


ROMDATA = open(os.path.join(os.path.dirname(__file__), '..', '..', 'sm', 'sm.smc'), 'rb').read()


class Xlate:
    def rom_value(self, a, t):
        h = len(ROMDATA) % 0x8000
        o = ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF) + h
        if t in ('uint8', 'int8'):
            v = ROMDATA[o]
            return v - 256 if t == 'int8' and v >= 128 else v
        v = ROMDATA[o] | ROMDATA[o + 1] << 8
        return v - 65536 if t == 'int16' and v >= 32768 else v

    def __init__(self):
        self.bad_tables = {}
        self.table_defs = {}    # tablas del decomp que no están (o no se hallan) en la ROM: se copian tal cual
        self.need_tables = set()
        self.fnaddr = {}
        for m in re.finditer(r'^#define fn(\w+) 0x([0-9A-Fa-f]{6})', open(os.path.join(SRC, 'funcs.h')).read(), re.M):
            self.fnaddr[m.group(1)] = int(m.group(2), 16)
        self.g = Graph()
        self.structs = parse_structs()
        self.rom = {}           # nombre -> (dirección, tipo)
        self.scalars = {}       # constante escalar de la ROM -> (dirección, tipo)
        for f in os.listdir(SRC):
            if not ((f.startswith('sm_') and f.endswith('.c')) or f.endswith('.h')):
                continue
            t = open(os.path.join(SRC, f), encoding='utf-8', errors='replace').read()
            for m in re.finditer(r'#define (\w+) \(\((\w+)\s*\*\)RomFixedPtr\(0x([0-9a-fA-F]{6})\)\)', t):
                self.rom[m.group(1)] = (int(m.group(3), 16), m.group(2))
            for m in re.finditer(r'#define (\w+) \(\*\((\w+)\s*\*\)RomFixedPtr\(0x([0-9a-fA-F]{6})\)\)', t):
                self.scalars[m.group(1)] = (int(m.group(3), 16), m.group(2))
                self.rom[m.group(1)] = (int(m.group(3), 16), m.group(2))
            for m in re.finditer(r'^static const (u?int(?:8|16)) (\w+_([0-9A-Fa-f]{6})) = (-?(?:0x[0-9a-fA-F]+|\d+));', t, re.M):
                t_, name, addr, v = m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4), 0)
                if self.rom_value(addr, t_) == (v if not t_.startswith('u') else v & (0xFFFF if '16' in t_ else 0xFF)):
                    self.scalars[name] = (addr, t_)
                else:
                    self.bad_tables[name] = addr
            # tablas sin dirección en el nombre: se buscan en la ROM (deben aparecer una sola vez)
            for m in re.finditer(r'^static const (u?int(?:8|16)) ([A-Za-z]\w*)\[\d*\] = \{([^}]*)\};', t, re.M):
                t_, name = m.group(1), m.group(2)
                if re.search(r'_[0-9A-Fa-f]{6}$', name):
                    continue
                vals = [int(v, 0) for v in re.findall(r'-?(?:0x[0-9a-fA-F]+|\d+)', re.sub(r'//[^\n]*', '', m.group(3)))]
                if len(vals) < 4:                    # muy corta para hallarla en la ROM: se copia tal cual
                    self.table_defs[name] = m.group(0)
                    continue
                sz = 2 if '16' in t_ else 1
                pat = b''.join((v & 0xFFFF).to_bytes(2, 'little') if sz == 2 else (v & 0xFF).to_bytes(1, 'little') for v in vals)
                hits = []
                if len(pat) >= 8:
                    i = ROMDATA.find(pat)
                    while i >= 0 and len(hits) < 3:
                        hits.append(i)
                        i = ROMDATA.find(pat, i + 1)
                if len(hits) == 1:
                    o = hits[0] - len(ROMDATA) % 0x8000
                    self.rom[name] = (((o // 0x8000) | 0x80) << 16 | 0x8000 | (o & 0x7FFF), t_)
                else:
                    self.bad_tables[name] = len(hits)
                    self.table_defs[name] = m.group(0)
            for m in re.finditer(r'static const (u?int(?:8|16)) (\w+_([0-9A-Fa-f]{6}))\[\d*\] = \{([^}]*)\};', t):
                t_, name, addr = m.group(1), m.group(2), int(m.group(3), 16)
                vals = [int(v, 0) for v in re.findall(r'-?(?:0x[0-9a-fA-F]+|\d+)', re.sub(r'//[^\n]*', '', m.group(4)))]
                sz = 2 if '16' in t_ else 1
                ok = bool(vals) and all(self.rom_value(addr + i * sz, t_) == v for i, v in enumerate(vals))
                if ok:
                    self.rom[name] = (addr, t_)
                else:
                    self.bad_tables[name] = addr
                    self.table_defs[name] = m.group(0)
        # macros con parámetros de los archivos del decomp (se expanden antes de traducir)
        self.macros = {}
        for f in os.listdir(SRC):
            if f.startswith('sm_') and f.endswith('.c'):
                t = open(os.path.join(SRC, f), encoding='utf-8', errors='replace').read()
                for m in re.finditer(r'^#define (\w+)\(([\w, ]*)\) (.*)$', t, re.M):
                    self.macros[m.group(1)] = ([p.strip() for p in m.group(2).split(',') if p.strip()], m.group(3))
        self.used_banks = set()
        self.todo = []

    def rom_read(self, addr, t, idx):
        bank, a = addr >> 16, addr & 0xFFFF
        self.used_banks.add(bank)
        sz = 2 if t in ('uint16', 'int16') else 1 if t in ('uint8', 'int8') else 4 if t in ('uint32', 'int32') else None
        if sz == 4:
            return '((%s)((uint32)BW_%02X(0x%04X + 4 * (%s)) | (uint32)BW_%02X(0x%04X + 4 * (%s) + 2) << 16))' % (t, bank, a, idx, bank, a, idx)
        if sz is None:
            self.todo.append('tabla de %s en %06X' % (t, addr))
            return 'XLATE_TODO_ROM_%s(0x%06X, %s)' % (t, addr, idx)
        fn = 'BW_%02X' % bank if sz == 2 else 'BB_%02X' % bank
        return '((%s)%s(0x%04X + %d * (%s)))' % (t, fn, a, sz, idx) if sz == 2 else '((%s)%s(0x%04X + (%s)))' % (t, fn, a, idx)

    def fields_of(self, typ):
        return self.structs.get(typ, {})

    def translate(self, name, bank):
        src = self.g.funcs[name][4]
        hdr, body = src.split('\n', 1)
        binstr = re.match(r'(?:static )?const uint8 \*(\w+)\(uint16 k, const uint8 \*(\w+)\)', hdr)
        if binstr:
            hdr = 'uint16 %s(uint16 k, uint16 %s) {  // %s' % (binstr.group(1), binstr.group(2), hdr.split('//')[-1].strip() if '//' in hdr else '')
        instr = re.match(r'const uint16 \*(\w+)\(uint16 k, const uint16 \*jp\)', hdr)
        if instr:
            hdr = 'uint16 %s(uint16 k, uint16 jp) {  // %s' % (instr.group(1), hdr.split('//')[-1].strip() if '//' in hdr else '')
        for mname, (params, mbody) in self.macros.items():
            rx = re.compile(r'\b%s\(' % mname)
            while True:
                m = rx.search(body)
                if not m:
                    break
                e = match_paren(body, m.end() - 1)
                args = [a_.strip() for a_ in body[m.end():e].split(',')]
                rep_ = mbody
                for p_, a_ in zip(params, args):
                    rep_ = re.sub(r'\b%s\b' % p_, '(%s)' % a_, rep_)
                body = body[:m.start()] + '(' + rep_ + ')' + body[e + 1:]
        # RAM extra 7E:7800 con la forma de Kraid (Ram7800_Kraid, la usa también el mini Kraid): campos -> ERAM
        KR = {'kraid_next': 0, 'field_2': 2, 'field_4': 4, 'kraid_thinking': 6, 'kraid_min_y_pos_eject': 8,
              'kraid_mouth_flags': 10, 'kraid_healths_8ths': 12, 'field_1C': 28, 'kraid_target_x': 30,
              'kraid_healths_4ths': 32, 'field_28': 40, 'kraid_hurt_frame': 42, 'kraid_hurt_frame_timer': 44}
        kv = {}
        for m in re.finditer(r'\b(\w+) = gExtraEnemyRam7800\(([^;]*)\);', body):
            kv[m.group(1)] = m.group(2)
        for v_, x_ in kv.items():
            body = re.sub(r'(?m)^\s*ExtraEnemyRam7800 \*%s;\n?' % v_, '', body)
            body = re.sub(r'\n\s*%s = gExtraEnemyRam7800\([^;]*\);' % v_, '', body)
            body = re.sub(r'\b%s->kraid\.' % v_, 'gExtraEnemyRam7800(%s)->kraid.' % x_, body)
        def kraid(m):
            off = KR[m.group(2)] + (2 * int(m.group(4)) if m.group(4) else 0)
            return 'ERAM(0x%04X + (uint16)(%s))' % (0x7800 + off, m.group(1))
        body = re.sub(r'gExtraEnemyRam7800\(([^()]*)\)->kraid\.(\w+)(\[(\d+)\])?', kraid, body)
        # variables de tipo Enemy_X
        vars_ = {}
        def decl(m):
            typ, v, expr = m.group(1), m.group(2), m.group(4)
            vars_[v] = typ
            return 'EnemyData *%s = gEnemyData(%s); const uint16 %s_k = (%s);' % (v, expr, v, expr)
        body = re.sub(r'\b(Enemy_\w+) \*(\w+) = Get_(\w+)\(([^;]*?)\);', decl, body)
        def assign(m):
            v, typ, expr = m.group(1), 'Enemy_' + m.group(2), m.group(3)
            return '%s = gEnemyData(%s); %s_k = (%s);' % (v, expr, v, expr)
        for m in re.finditer(r'\b(Enemy_\w+) \*(\w+);', body):
            vars_[m.group(2)] = m.group(1)
        body = re.sub(r'\bEnemy_(\w+) \*(\w+);', lambda m: 'EnemyData *%s; uint16 %s_k;' % (m.group(2), m.group(2)), body)
        body = re.sub(r'\b(\w+) = Get_(\w+)\(([^;]*?)\);', assign, body)
        # Get_X(expr)->campo (sin variable)
        def inline(m):
            typ, expr, field = 'Enemy_' + m.group(1), m.group(2), m.group(3)
            return self.field(typ, 'gEnemyData(%s)' % expr, '(%s)' % expr, field)
        body = re.sub(r'\bGet_(\w+)\(([^()]*(?:\([^()]*\))?[^()]*)\)->(\w+(?:\.\w+)?)', inline, body)
        # V->campo
        for v, typ in vars_.items():
            body = re.sub(r'\b%s->(base\.)?(\w+)' % re.escape(v),
                          lambda m, v=v, typ=typ: self.field(typ, v, v + '_k', (m.group(1) or '') + m.group(2)), body)
        # arreglos en la RAM extra: ERAM(a + k)[i] -> ERAM(a + k + 2 * i)
        body = re.sub(r'ERAM\(([^()]+)\)\[', 'ERAM_ARR(\\1)[', body)
        # *(T *)((uint8 *)&TABLA[i] + desplazamiento) / *(T *)((uint8 *)TABLA + desplazamiento): lectura por bytes
        rx = re.compile(r'\*\((u?int(?:8|16)) \*\)\(\(uint8 \*\)&?(\w+)(\[[^\]]+\])? \+ ')
        pos = 0
        while True:
            m = rx.search(body, pos)
            if not m:
                break
            open_ = body.index('((uint8', m.start())
            e = match_paren(body, open_)
            off = body[m.end():e]
            tname, idx = m.group(2), m.group(3)
            em = re.match(r'g_(?:word|byte)_7E([0-9A-F]{4})$', tname)
            if em and 0x7800 <= int(em.group(1), 16) < 0x9800 and not idx and m.group(1) in ('uint16', 'int16'):
                rep_ = 'ERAM(0x%s + (uint16)(%s))' % (em.group(1), off)
                body = body[:m.start()] + rep_ + body[e + 1:]
                pos = m.start() + len(rep_)
                continue
            if tname in self.rom:
                addr, tt = self.rom[tname]
                sz = 2 if tt in ('uint16', 'int16') else 1
                if idx:
                    off = '%d * (%s) + %s' % (sz, idx[1:-1], off)
                rep_ = self.rom_read(addr, m.group(1), '0').replace('(0)', '(%s)' % off).replace('2 * ', '')
            elif m.group(1) in ('uint16', 'int16') and not idx:   # arreglo de la RAM (palabras): por índice
                rep_ = '%s[(uint16)(%s) >> 1]' % (tname, off)
            else:
                self.todo.append('%s: lectura por bytes de %s' % (name, tname))
                pos = m.end()
                continue
            body = body[:m.start()] + rep_ + body[e + 1:]
            pos = m.start() + len(rep_)
        # punteros a la ROM: *(T *)RomPtr_XX(e) -> lectura; const uint8 *v = RomPtr_XX(e) -> v es una dirección
        body = re.sub(r'\*\(u?int16 \*\)RomPtr_([0-9A-F]{2})\(', lambda m: 'BW_%s(' % m.group(1), body)
        for m in list(re.finditer(r'const (uint8|uint16) \*(\w+) = (?:\(const uint16 \*\))?RomPtr_([0-9A-F]{2})\(', body)):
            t_, v, bk = m.group(1), m.group(2), m.group(3)
            self.used_banks.add(int(bk, 16))
            body = body.replace(m.group(0), 'uint16 %s = (' % v, 1)
            sz = 2 if t_ == 'uint16' else 1
            body = re.sub(r'\bGET_WORD\(%s\)' % v, 'BW_%s(%s)' % (bk, v), body)
            body = re.sub(r'\bGET_WORD\(%s \+ ([^)]*)\)' % v, lambda mm: 'BW_%s(%s + %s)' % (bk, v, mm.group(1)), body)
            body = re.sub(r'\b%s\[([^\]]+)\]' % v, lambda mm: '%s(%s + %d * (%s))' % ('BW_' + bk if sz == 2 else 'BB_' + bk, v, sz, mm.group(1)), body)
            body = re.sub(r'\*%s\b' % v, '%s(%s)' % ('BW_' + bk if sz == 2 else 'BB_' + bk, v), body)
            if re.search(r'\b%s\s*(\+=|\+\+|=[^=])' % v, body.split('uint16 %s = (' % v, 1)[1]):
                body = re.sub(r'\b%s \+= (\d+)' % v, lambda mm: '%s += %d' % (v, int(mm.group(1)) * (1 if sz == 1 else 2)) if sz == 1 else mm.group(0), body)
        # variables del decomp en 7E:7800-97FF: la RAM extra de los enemigos (ERAM)
        def eram_var(m):
            ad = int(m.group(1), 16)
            return 'ERAM(0x%04X)' % ad if 0x7800 <= ad < 0x9800 else m.group(0)
        def eram_arr(m):                    # arreglo: g_word_7E97DC[i] -> ERAM(0x97DC + 2 * (i))
            ad = int(m.group(1), 16)
            return 'ERAM(0x%04X + 2 * (%s))' % (ad, m.group(2)) if 0x7800 <= ad < 0x9800 else m.group(0)
        body = re.sub(r'\bg_word_7E([0-9A-F]{4})\[([^\[\]]+)\]', eram_arr, body)
        body = re.sub(r'\bg_word_7E([0-9A-F]{4})\b', eram_var, body)
        # tablas de la ROM
        for tname, (addr, t) in sorted(self.rom.items(), key=lambda kv: -len(kv[0])):
            if tname in self.scalars:
                continue
            pos = 0
            while True:
                m = re.compile(r'\b%s\b' % re.escape(tname)).search(body, pos)
                if not m:
                    break
                j = m.end()
                if j < len(body) and body[j] == '[':
                    e = match_paren(body, j, '[', ']')
                    rep = self.rom_read(addr, t, body[j + 1:e])
                    body = body[:m.start()] + rep + body[e + 1:]
                    pos = m.start() + len(rep)
                else:
                    self.todo.append('%s: uso de la tabla %s sin índice' % (name, tname))
                    pos = j
        body = body.replace('eproj_spawn_pt.x', 'eproj_spawn_x').replace('eproj_spawn_pt.y', 'eproj_spawn_y')
        body = re.sub(r'\beproj_spawn_pt = \(Point16U\)\s*\{ ([^,]+), ([^}]+?) \};', r'eproj_spawn_x = \1; eproj_spawn_y = \2;', body)
        # memcpy(g_ram + dirección, RomPtr_XX(j), n): paletas (7E:C000-C3FF) desde la ROM
        body = re.sub(r'memcpy\(g_ram \+ ([^,]+), RomPtr_([0-9A-F]{2})\(([^()]*)\), (\d+)\);',
                      lambda m: (self.used_banks.add(int(m.group(2), 16)) or
                                 'nt_ram_copy_rom((uint16)(%s), BW_%s, (uint16)(%s), %s);' % (m.group(1), m.group(2), m.group(3), m.group(4))), body)
        def romw(m):
            b = int(m.group(1), 16)
            self.used_banks.add(b)
            return 'BW_%02X((uint16)((%s) + %d))' % (b, m.group(2), 2 * int(m.group(3) or 0))
        body = re.sub(r'\*\(\(uint16 \*\)RomPtr_([0-9A-F]{2})\(([^()]*(?:\([^()]*\))?[^()]*)\) \+ (\d+)\)', romw, body)
        body = re.sub(r'\*\(uint16 \*\)RomPtr_([0-9A-F]{2})\(([^()]*(?:\([^()]*\))?[^()]*)\)()', romw, body)
        body = body.replace('*(uint16 *)&enemy_gfx_drawn_hook.bank', 'nt_gfx_hook_bank').replace('enemy_gfx_drawn_hook.addr', 'nt_gfx_hook_addr')
        body = re.sub(r'\bWriteReg\([^;]*\);', '(void)0;', body)
        for tname, (addr, t) in self.scalars.items():
            if re.search(r'\b%s\b' % tname, body):
                body = re.sub(r'\b%s\b' % tname, self.rom_read(addr, t, '0'), body)
        # FUNC16(F)
        def fn16(m):
            f = m.group(1)
            a = self.g.funcs.get(f, (0, 0, 0, None))[3]
            if a is None:
                a = self.fnaddr.get(f)
            if a is None:
                self.todo.append('%s: FUNC16(%s) sin dirección' % (name, f))
                return 'XLATE_TODO_FUNC16(%s)' % f
            return '0x%04X /* %s */' % (a & 0xFFFF, f)
        body = re.sub(r'FUNC16\((\w+)\)', fn16, body)
        # fnF (constantes de dirección del decomp, funcs.h)
        def fnc(m):
            f = m.group(1)
            a = self.g.funcs.get(f, (0, 0, 0, None))[3]
            if a is None:
                a = self.fnaddr.get(f)
            return '0x%06X' % a if a is not None else m.group(0)
        body = re.sub(r'\bfn(\w+)\b', fnc, body)
        # nivel y BTS (en el motor nativo: room.h)
        body = self.subst_index(body, 'level_data', 'level_get')
        # RAM extra de los enemigos en 7E:8800 (estructura con relleno): por byte -> ERAM
        while True:
            m = re.search(r'\*\(uint16 \*\)&extra_enemy_ram8800\[0\]\.pad\[', body)
            if not m:
                break
            e = match_paren(body, m.end() - 1, '[', ']')
            body = body[:m.start()] + 'ERAM(0x8800 + (uint16)(%s))' % body[m.end():e] + body[e + 1:]
        body = body.replace('*(uint16 *)&hdma_window_1_left_pos[0].field_0', 'statue_bg2_ypos')
        body = re.sub(r'CallEnemyAi\((\w+(?:->\w+)?) << 16 \| ([^;]+)\);', r'CallEnemyAi(\1, \2);', body)   # nativo: (banco, función)
        body = body.replace('*(uint16 *)&E->bank = *(uint16 *)&Edef->bank;',
                            'E->bank = Edef->bank; E->field_2F = Edef->hurt_ai_time;')   # sin leer 2 bytes a la vez
        body = self.subst_index(body, 'BTS', 'bts_get')
        body = body.replace('Unreachable()', 'nt_unimpl("Unreachable")')
        # el decomp lee campos de otro enemigo (el anterior) por arreglos de la RAM que están justo antes de
        # enemy_data (7E:0F78): enemy_drawing_queue (7E:0E84), enemy_drawing_queue_sizes (7E:0F68)
        EFIELDS = ['enemy_ptr', 'x_pos', 'x_subpos', 'y_pos', 'y_subpos', 'x_width', 'y_height', 'properties',
                   'extra_properties', 'ai_handler_bits', 'health', 'spritemap_pointer', 'timer',
                   'current_instruction', 'instruction_timer', 'palette_index', 'vram_tiles_index', 'layer',
                   'flash_timer', 'frozen_timer', 'invincibility_timer', 'shake_timer', 'frame_counter', None,
                   'ai_var_A', 'ai_var_B', 'ai_var_C', 'ai_var_D', 'ai_var_E', 'ai_preinstr', 'parameter_1',
                   'parameter_2']
        def other_enemy(m):
            arr, var, n = m.group(1), m.group(2), int(m.group(3))
            dm = re.search(r'\bint %s = \(?([\w>()]+?)\)? >> 1;' % var, body)
            if not dm:
                return m.group(0)
            base = 0x0E84 if arr == 'enemy_drawing_queue' else 0x0F68
            rel = base + 2 * n - 0x0F78
            d, off = (rel // 64) * 64, rel % 64
            f = EFIELDS[off // 2]
            if f is None or off & 1:
                return m.group(0)
            return 'gEnemyData((uint16)(%s + (%d)))->%s' % (dm.group(1), d, f)
        body = re.sub(r'\b(enemy_drawing_queue(?:_sizes)?)\[(\w+) \+ (\d+)\]', other_enemy, body)
        body = re.sub(r'SpawnHardcodedPlm\(\(SpawnHardcodedPlmArgs\) ?\{ ?([^,]+), ([^,]+), ([^}]+?) ?\}\)', r'SpawnHardcodedPlm(\1, \2, \3)', body)
        # palabra que empieza en el byte alto de un campo (en el SNES, little-endian): byte alto de ese campo y byte
        # bajo del siguiente
        def mid_word(m):
            v, f = m.group(1), m.group(2)
            nxt = chr(ord(f) + 1)
            return '((uint16)(HIBYTE(%s->ai_var_%s) | LOBYTE(%s->ai_var_%s) << 8))' % (v, f, v, nxt)
        body = re.sub(r'\*\(uint16 \*\)\(\(uint8 \*\)&(\w+)->ai_var_([A-E]) \+ 1\)', mid_word, body)
        if re.search(r'\(uint8 \*\)&\w+->\w+ \+ 1\)', body):
            self.todo.append('%s: palabra a mitad de un campo' % name)
        if instr:
            body = self.instr_body(name, body, bank)
        if binstr:
            body = self.binstr_body(name, body, bank, binstr.group(2))
        body = body.replace('(int8)bts_get(cur_block_index) * room_width_in_blocks',
                            'Mul16((int8)bts_get(cur_block_index), room_width_in_blocks)')   # mulu.w, sin __mulsi3
        # byte bajo de una lectura de la ROM (no es un lugar): LOBYTE(BW_XX(...)) -> (uint8)(...)
        body = body.replace('LOBYTE(((uint16)BW_', '(uint8)(((uint16)BW_')
        # byte de una tabla de palabras: *((uint8 *)&g_word_XXXXXX + i) (ya traducida a BW_XX(dirección)) -> BB_XX
        body = re.sub(r'\*\(\(uint8 \*\)&\(\(uint16\)BW_([0-9A-F]{2})\((0x[0-9A-Fa-f]+) \+ 2 \* \(0\)\)\) \+ ([^)]+)\)',
                      r'BB_\1((uint16)(\2 + (\3)))', body)
        for tn in self.table_defs:
            if re.search(r'\b%s\b' % tn, body):
                self.need_tables.add(tn)
        return hdr + '\n' + body

    def binstr_body(self, name, b, bank, P):
        """instrucciones con puntero de bytes (proyectiles enemigos, $86): P es la dirección en el banco"""
        b = re.sub(r'\bINSTRB_RETURN_ADDR\(', '(uint16)(', b)
        b = re.sub(r'\b%s - RomBankBase\(0x[0-9A-Fa-f]+\)' % P, P, b)
        b = re.sub(r'\bLoad24\(\(LongPtr \*\)%s\)' % P, '(BW_%02X(%s) | (uint32)BB_%02X((uint16)(%s + 2)) << 16)' % (bank, P, bank, P), b)
        b = re.sub(r'\bGET_WORD\(%s\)' % P, 'BW_%02X(%s)' % (bank, P), b)
        b = re.sub(r'\bGET_WORD\(%s \+ (\d+)\)' % P, lambda m: 'BW_%02X(%s + %s)' % (bank, P, m.group(1)), b)
        b = re.sub(r'\b%s\[(\d+)\]' % P, lambda m: 'BB_%02X(%s + %s)' % (bank, P, m.group(1)), b)
        b = re.sub(r'\*%s\b' % P, 'BB_%02X(%s)' % (bank, P), b)
        b = re.sub(r'\b%s \+ (\d+)\b' % P, lambda m: '(uint16)(%s + %s)' % (P, m.group(1)), b)
        self.used_banks.add(bank)
        if re.search(r'\(uint8 \*\)|RomBankBase|INSTR_ADDR_TO_PTR|\b%s - ' % P, b):
            self.todo.append('%s: uso del puntero que no sé traducir' % name)
        return b

    def subst_index(self, body, arr, fn):
        pos = 0
        rx = re.compile(r'\b%s\[' % arr)
        while True:
            m = rx.search(body, pos)
            if not m:
                return body
            e = match_paren(body, m.end() - 1, '[', ']')
            rest = body[e + 1:e + 4]
            if re.match(r'\s*=[^=]', rest) or re.match(r'\s*(\+\+|--|[+\-|&^]=)', rest):
                self.todo.append('escritura en %s' % arr)
            rep = '%s(%s)' % (fn, body[m.end():e])
            body = body[:m.start()] + rep + body[e + 1:]
            pos = m.start() + len(rep)

    def field(self, typ, ptr, kexpr, field):
        if field.startswith('base.'):
            return '%s->%s' % (ptr, field[5:])
        fs = self.fields_of(typ)
        if field not in fs:
            if field in BASE_FIELDS or field in EDATA.values():
                return '%s->%s' % (ptr, field)
            self.todo.append('%s.%s desconocido' % (typ, field))
            return '%s->XLATE_TODO_%s' % (ptr, field)
        a, t, cnt = fs[field]
        if 0xF78 <= a < 0xFB8:
            o = a - 0xF78
            if o in EDATA and (t in ('uint16', 'int16') or o >= 0x2E and t in ('uint8',)):
                return '%s->%s' % (ptr, EDATA[o])
            self.todo.append('%s.%s en %X (%s)' % (typ, field, o, t))
            return '%s->XLATE_TODO_%s' % (ptr, field)
        if 0x7800 <= a < 0x9800 and t in ('uint16', 'int16'):
            if cnt:
                return 'ERAM_ARR(0x%04X + %s)' % (a, kexpr)
            return '%sERAM(0x%04X + %s)' % ('(int16)' if t == 'int16' and False else '', a, kexpr)
        self.todo.append('%s.%s en %X (%s)' % (typ, field, a, t))
        return 'XLATE_TODO_FIELD_%s' % field

    def instr_body(self, name, body, bank):
        b = body
        b = re.sub(r'\bINSTR_RETURN_ADDR\(', '(uint16)(', b)
        while True:
            m = re.search(r'\bINSTR_ADDR_TO_PTR\(\s*\w+\s*,\s*', b)
            if not m:
                break
            e = match_paren(b, m.end() - len(m.group(0)) + len('INSTR_ADDR_TO_PTR'))
            b = b[:m.start()] + '(' + b[m.end():e] + ')' + b[e + 1:]
        # las instrucciones comunes ($A0, copiadas al principio de cada banco de enemigos) leen la lista del banco del
        # enemigo
        iw = 'IW_EN' if name.startswith('EnemyInstr_') else 'IW_%02X' % bank
        b = re.sub(r'\*jp\b', '%s(jp)' % iw, b)
        b = re.sub(r'\bjp\[(\d+)\]', lambda m: '%s(jp + %d)' % (iw, 2 * int(m.group(1))), b)
        b = re.sub(r'\bjp \+ (\d+)\b', lambda m: '(uint16)(jp + %d)' % (2 * int(m.group(1))), b)
        if re.search(r'\bjp\[|\(uint8 \*\)jp|\(uint8\*\)jp|INSTR_ADDR_TO_PTR|\bjp - ', b):
            self.todo.append('%s: uso de jp que no sé traducir' % name)
        return b


def main():
    out, bank = sys.argv[1], int(sys.argv[2], 16)
    names = []
    for a in sys.argv[3:]:
        if a.startswith('@'):
            names += open(a[1:]).read().split()
        else:
            names.append(a)
    x = Xlate()
    res = ['/* Generado por tools/port/xlate.py desde el decomp (snesrev sm/src); revisado a mano donde dice. */']
    for n in names:
        if n not in x.g.funcs:
            raise SystemExit('no existe %s' % n)
        res.append(x.translate(n, bank))
        res.append('')
    open(out, 'w').write('\n'.join(res))
    print('bancos de ROM:', ' '.join('%02X' % b for b in sorted(x.used_banks)))
    for t in x.todo:
        print('PENDIENTE:', t)


if __name__ == '__main__':
    main()
