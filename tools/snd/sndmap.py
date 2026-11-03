#!/usr/bin/env python3
"""MegaMetroid — qué efectos de sonido pueden sonar con cada conjunto de música cargado (docs/55).

La Genesis guarda la música y los efectos de cada conjunto en una página del SSF2 que se ve en la ventana 6
(0x300000-0x37FFFF) y que se cambia solo al subir un conjunto (music_upload, como APU_UploadBank $80:8F0C). Hace falta
saber qué efectos se pueden pedir mientras cada conjunto está cargado. Todo sale del decomp (sm/src) y de tu ROM:

1. Quién pide cada efecto (tools/snd/sfxscan.py): una función del decomp, o una instrucción con el número en la
   lista (PlmInstr_QueueSfx*, EprojInstr_QueueSfx*, las de los enemigos...).
2. Qué alcanza cada "entrada" (hacia abajo, siguiendo el decomp):
   - un enemigo (EnemyDef en $A0: sus punteros de IA, en su banco);
   - un proyectil de enemigo (EprojDef en $86: init, pre-instrucción, listas);
   - un PLM (cabecera en $84: su inicio y sus listas, recorridas con las instrucciones de src/core/plm.c);
   - el código de la sala (RoomDefRoomstate: main_code_ptr, room_setup_code; $8F).
   De una función: las funciones que llama o que nombra (FUNC16, fn...), los datos que nombra (addr_..., RomPtr) y,
   si un dato es una lista de instrucciones, las funciones de su banco cuyas direcciones aparecen en la lista (desde
   el dato hasta el siguiente que el decomp nombra) con el número que sigue a las que lo llevan. Los despachadores
   (funciones con muchos `case fn...`) no se siguen. Puede dar de más (una palabra de datos igual a una dirección);
   de menos solo si el decomp no nombra algo.
3. Cada sala: sus enemigos, PLM, efectos de la capa 3 (lava/ácido) y su código; los conjuntos que se pueden escuchar en
   ella: el de su cabecera (estado), o, si es 0 (sin cambio), los de las salas vecinas por puertas; más los que pide
   el código de sus enemigos/PLM (los jefes: QueueMusic_Delayed8(0xFFxx)).
4. Efectos globales: los que pide el código de Samus, proyectiles, HUD, pausa, puertas, el código común de enemigos
   ($A0) y las cajas de mensaje: suenan en cualquier conjunto y van en la parte fija.

Uso: sndmap.py sm.smc [--salas]"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "tools" / "conv"))
import sfxscan                                       # noqa: E402
import sfxlist                                       # noqa: E402
from plmscan import instr_table                       # noqa: E402

SMSRC = sfxscan.SMSRC
GLOBAL_BANKS = {0x80, 0x81, 0x82, 0x85, 0x90, 0x91, 0x92, 0x93, 0x94, 0x9B, 0xA0}
# $88 (HDMA): la lava y el ácido (HdmaobjPreInstr_LavaAcidBG3YScroll) van con las salas que tienen esa capa 3; lo demás
# del banco (rayos X, bomba de poder, terremoto) es global
# Lo que alcanza una sala no entra en los bancos globales ni en el de las cinemáticas ($8B): lo que piden ahí es global
# o va con la cinemática (los cambios de estado del juego, como el final que arranca la nave, llevan a todo)
STOP_BANKS = GLOBAL_BANKS | {0x8B}
FX_LAVA_ACID = {2, 4}                                # FxDef.type: 2 lava, 4 ácido (sm_88.c: kFxTypeFuncPtrs)
SEL_LEN = {0xE5E6: 0, 0xE5FF: 2, 0xE612: 3, 0xE629: 3, 0xE652: 2, 0xE669: 2}
MUSIC_CALL = re.compile(r"\bQueueMusic_Delayed8\((0x[0-9A-Fa-f]+)\)")


class Rom:
    def __init__(self, path):
        b = open(path, "rb").read()
        self.b = b[len(b) & 0x7FFF:]

    def w(self, a):
        o = sfxscan.lorom(a)
        return self.b[o] | self.b[o + 1] << 8

    def byte(self, a):
        return self.b[sfxscan.lorom(a)]


class Decomp:
    """las funciones del decomp con sus referencias (funciones y datos) y sus pedidos de efectos"""

    def __init__(self, rom):
        self.rom = rom
        reqs, unknown, funcs = sfxscan.scan(rom.b)
        self.unknown = unknown
        self.funcs = {}                              # nombre -> (dirección, cuerpo)
        self.by_addr = {}
        for fn, name, addr, params, body in funcs:
            if name not in self.funcs:
                self.funcs[name] = (addr, body)
                self.by_addr[addr] = name
        self.req_func = {}                           # nombre -> {(lib, id)}
        self.req_param = {}                          # (banco, dirección de la instrucción en la lista) -> {(lib, id)}
        for k, whos in reqs.items():
            for bank, who in whos:
                m = re.match(r"(\w+)@([0-9A-F]{4})$", who)
                if m:
                    self.req_param.setdefault((bank, int(m.group(2), 16)), set()).add(k)
                else:
                    self.req_func.setdefault(re.sub(r"\(.*", "", who), set()).add(k)
        self.music = {}                              # nombre -> {conjunto} (QueueMusic_Delayed8(0xFFxx))
        names = set(self.funcs)
        self.refs, self.data = {}, {}
        lab = {}
        for name, (addr, body) in self.funcs.items():
            bank = addr >> 16
            toks = set(re.findall(r"\b(?:fn|FUNC16\()?([A-Za-z_]\w*)", body))
            self.refs[name] = {t for t in toks if t in names and t != name} | \
                {t[2:] for t in toks if t.startswith("fn") and t[2:] in names}
            d = set()
            for m in re.finditer(r"\baddr_\w*?_([0-9A-F]{6}|[0-9A-F]{4})\b", body):
                v = int(m.group(1), 16)
                d.add(v if v > 0xFFFF else bank << 16 | v)
            for m in re.finditer(r"RomPtr_([0-9A-F]{2})\((0x[0-9A-Fa-f]+)\)", body):
                d.add(int(m.group(1), 16) << 16 | int(m.group(2), 16))
            for m in re.finditer(r"\b0x([89ABab][0-9A-Fa-f])([89A-Fa-f][0-9A-Fa-f]{3})\b", body):
                d.add(int(m.group(1) + m.group(2), 16))
            self.data[name] = {a for a in d if a & 0x8000}
            # SpawnEnemy(banco, entrada): la entrada es como las de la población (EnemyPopulation): el enemigo primero
            for b, a in re.findall(r"\bSpawnEnemy\(0x([0-9A-Fa-f]{2}), addr_\w*?_([0-9A-F]{4,6})\)", body):
                v = int(a, 16) & 0xFFFF
                self.data[name].add(0xA00000 | rom.w(int(b, 16) << 16 | v))
            for a in self.data[name]:
                lab.setdefault(a >> 16, set()).add(a & 0xFFFF)
            for m in MUSIC_CALL.findall(body):
                v = int(m, 16)
                if v >> 8 == 0xFF:
                    self.music.setdefault(name, set()).add(v & 0xFF)
        # los despachadores (muchos case fn...) no se siguen: llevan a todo
        self.dispatch = {n for n, (a, b) in self.funcs.items() if len(re.findall(r"\bcase fn", b)) >= 8}
        for n in self.funcs:
            self.funcs[n][0] >> 16
            lab.setdefault(self.funcs[n][0] >> 16, set()).add(self.funcs[n][0] & 0xFFFF)
        self.labels = {b: sorted(v) for b, v in lab.items()}
        self.datalabels = {b: set(v) - {self.funcs[n][0] & 0xFFFF for n in self.funcs if self.funcs[n][0] >> 16 == b}
                           for b, v in lab.items()}
        self.enemy_defs = {int(x, 16) for x in re.findall(r"addr_kEnemyDef_([0-9A-F]{4})",
                                                          (SMSRC / "ida_types.h").read_text())}
        self.addr_of = {n: a for n, (a, _) in self.funcs.items()}

    def region(self, a):
        """[a, siguiente dato o función nombrados) en el banco de a"""
        lst = self.labels.get(a >> 16, [])
        import bisect
        i = bisect.bisect_right(lst, a & 0xFFFF)
        end = lst[i] if i < len(lst) else 0x10000
        return a & 0xFFFF, min(end, (a & 0xFFFF) + 0x800)

    def scan_list(self, a):
        """funciones del banco cuyas direcciones aparecen en la lista de a, y efectos de instrucciones con número"""
        bank = a >> 16
        lo, hi = self.region(a)
        fs, sfx, ds = set(), set(), set()
        dl = self.datalabels.get(bank, set())
        for p in range(lo, hi - 1):
            v = self.rom.w(bank << 16 | p)
            n = self.by_addr.get(bank << 16 | v)
            if n:
                fs.add(n)
            elif v in dl and v != lo:                 # otra lista o dato que el decomp nombra (las listas de un
                ds.add(bank << 16 | v)                 # EprojDef, los saltos de una lista)
            sfx |= self.req_param.get((bank, p), set())
        return fs, sfx, ds

    def closure(self, funcs, lists=()):
        """efectos y conjuntos que pueden pedir esas funciones y listas (hacia abajo)"""
        sfx, music = set(), set()
        seen_f, seen_d = set(), set()
        todo_f, todo_d = list(funcs), list(lists)
        while todo_f or todo_d:
            while todo_d:
                a = todo_d.pop()
                if a in seen_d:
                    continue
                seen_d.add(a)
                if a in self.by_addr:
                    todo_f.append(self.by_addr[a])
                    continue
                if a >> 16 == 0xA0 and self.is_enemy_def(a & 0xFFFF):     # un enemigo que crea el código
                    fs, ls = self.enemy_ptrs(a & 0xFFFF)
                    todo_f += fs
                    todo_d += ls
                    continue
                fs, s, ds = self.scan_list(a)
                sfx |= s
                todo_f += fs
                todo_d += ds
            while todo_f:
                n = todo_f.pop()
                if n in seen_f or n not in self.funcs or (self.addr_of[n] >> 16) in STOP_BANKS:
                    continue
                seen_f.add(n)
                sfx |= self.req_func.get(n, set())
                music |= self.music.get(n, set())
                if n in self.dispatch:
                    continue
                todo_f += self.refs[n]
                todo_d += self.data[n]
        return sfx, music, seen_f

    def is_enemy_def(self, d):
        """un EnemyDef: los del decomp (addr_kEnemyDef_...) o uno con banco de enemigos y su inicio en ese banco"""
        if d in self.enemy_defs:
            return True
        bank = self.rom.byte(0xA00000 | d + 0x0C)
        return 0xA2 <= bank <= 0xB3 and (bank << 16 | self.rom.w(0xA00000 | d + 0x12)) in self.by_addr

    def enemy_ptrs(self, d):
        """los punteros de IA de un enemigo (EnemyDef $A0:d, ida_types.h), en su banco: (funciones, datos)"""
        base = 0xA00000 | d
        bank = self.rom.byte(base + 0x0C)
        fs, ls = [], []
        for off in (0x12, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x28, 0x2A, 0x30, 0x32, 0x34):
            p = self.rom.w(base + off)
            if p >= 0x8000:
                a = bank << 16 | p
                if a in self.by_addr:
                    fs.append(self.by_addr[a])
                else:
                    ls.append(a)
        return fs, ls

    def func_at(self, a):
        return self.by_addr.get(a)


def enemy_entry(rom, dec, d):
    """lo que alcanza un enemigo (EnemyDef $A0:d)"""
    return dec.closure(*dec.enemy_ptrs(d))


def plm_entry(rom, dec, tab, h):
    """lo que alcanza un PLM (cabecera $84:h): su inicio y sus listas (instrucciones de src/core/plm.c)"""
    fs = []
    f = dec.func_at(0x840000 | rom.w(0x840000 | h))
    if f:
        fs.append(f)
    sfx = set()
    seen = set()
    todo = [rom.w(0x840000 | h + off) for off in (2, 4)]
    while todo:
        p = todo.pop()
        while 0x8000 <= p < 0x10000 and p not in seen:
            seen.add(p)
            v = rom.w(0x840000 | p)
            if v < 0x8000:
                p += 4
                continue
            n = dec.func_at(0x840000 | v)
            if n:
                fs.append(n)
            sfx |= dec.req_param.get((0x84, p), set())
            t = tab.get(v)
            if t is None:
                break
            ln, br, stop, name = t
            for b in br:
                todo.append(rom.w(0x840000 | p + 2 + b))
            if name == "PlmInstr_Call":
                todo.append(p + 4)
            if stop or name in ("PlmInstr_Goto", "PlmInstr_Return", "PlmInstr_Call", "PlmInstr_Sleep",
                                "PlmInstr_Delete", "PlmInstr_DrawPlmBlock", "PlmInstr_DrawItemFrame_Common"):
                break
            p += 2 + ln
    s, m, _ = dec.closure(fs)
    return sfx | s, m


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
    dl = 0x8F0000 | rom.w(0x8F0000 | ra + 9)
    out, i = [], 0
    while True:
        d = rom.w(dl + 2 * i)
        if d < 0x8000:
            return out
        out.append(d)
        i += 1


def room_states(rom, ra, cond=False):
    """los estados de la sala (RoomDefStateSelect); con cond, (estado, condición): (tipo, argumento) o None (el de
    siempre, el último)"""
    q, sts = 0x8F0000 | ra + 11, []
    while True:
        k = rom.w(q)
        n = SEL_LEN[k]
        if n == 0:
            sts.append(((q + 2) & 0xFFFF, None))
            return sts if cond else [x for x, _ in sts]
        sts.append((rom.w(q + 2 + n - 2), (k, rom.byte(q + 2) if n == 3 else None)))
        q += 2 + n


def build(rom_path, log=None):
    rom = Rom(rom_path)
    dec = Decomp(rom)
    tab = instr_table()
    glob = set(sfxlist.zebes_sfx(rom.b))
    for name, s in dec.req_func.items():
        bank = dec.addr_of[name] >> 16
        if bank in GLOBAL_BANKS or (bank == 0x88 and "LavaAcid" not in name) or name.startswith("Eproj_Pickup") \
                or name in ("DisplayMessageBox_Async",):
            glob |= s
    lava = dec.req_func.get("HdmaobjPreInstr_LavaAcidBG3YScroll", set())
    glob -= lava
    cache_e, cache_p = {}, {}
    rooms = all_rooms(rom)
    # cada estado de cada sala (RoomDefRoomstate): su conjunto (0: sin cambio), sus efectos, los conjuntos que carga su
    # código (los jefes)
    info = {}
    for ra in rooms:
        doors = [rom.w(0x830000 | d) for d in room_doors(rom, ra)]
        for st, cond in room_states(rom, ra, True):
            s = 0x8F0000 | st
            sfx, music_code = set(), set()
            # enemigos ($A1)
            pop = rom.w(s + 8)
            if pop >= 0x8000:
                p = 0xA10000 | pop
                while rom.w(p) != 0xFFFF and rom.w(p) >= 0x8000:
                    d = rom.w(p)
                    if d not in cache_e:
                        cache_e[d] = enemy_entry(rom, dec, d)[:2]
                    sfx |= cache_e[d][0]
                    music_code |= cache_e[d][1]
                    p += 16
            # PLM ($8F)
            pl = rom.w(s + 20)
            if pl >= 0x8000:
                p = 0x8F0000 | pl
                while rom.w(p):
                    h = rom.w(p)
                    if h not in cache_p:
                        cache_p[h] = plm_entry(rom, dec, tab, h)
                    sfx |= cache_p[h][0]
                    music_code |= cache_p[h][1]
                    p += 6
            # código de la sala
            fs = [dec.func_at(0x8F0000 | rom.w(s + off)) for off in (18, 24)]
            s2, m2, _ = dec.closure([f for f in fs if f])
            sfx |= s2
            music_code |= m2
            # capa 3 ($83): lava y ácido
            fx = rom.w(s + 6)
            if fx >= 0x8000:
                p = 0x830000 | fx
                for _ in range(16):
                    door = rom.w(p)
                    if rom.byte(p + 9) in FX_LAVA_ACID:
                        sfx |= lava
                    if door in (0, 0xFFFF):
                        break
                    p += 16
            info[(ra, st)] = {"set": rom.byte(s + 4), "sfx": sfx - glob, "code_sets": music_code, "doors": doors,
                              "cond": cond}
    by_room = {}
    for (ra, st) in info:
        by_room.setdefault(ra, []).append((ra, st))
    # conjuntos que se pueden escuchar en cada estado: el suyo y los que carga su código; con 0 (sin cambio), además
    # los de las salas vecinas (por donde se puede llegar), en el estado de la misma condición (el mismo evento o jefe
    # muerto) si lo tienen, o en el de siempre
    def nb_states(nb, cond):
        sts = by_room.get(nb, ())
        same = [k for k in sts if info[k]["cond"] == cond]
        return same or [k for k in sts if info[k]["cond"] is None]

    active = {k: ({i["set"]} if i["set"] else set()) | i["code_sets"] for k, i in info.items()}
    changed = True
    while changed:
        changed = False
        for k, i in info.items():
            if i["set"]:
                continue
            for nb in i["doors"]:
                for k2 in nb_states(nb, i["cond"]):
                    if not active[k2] <= active[k]:
                        active[k] |= active[k2]
                        changed = True
    per_set = {}
    for k, i in info.items():
        for s in active[k]:
            per_set.setdefault(s, set()).update(i["sfx"])
    # las cinemáticas ($8B) y las pantallas de antes del juego: sus efectos van con los conjuntos que cargan ellas
    cine_sfx, cine_sets = set(), set()
    for name, addr in dec.addr_of.items():
        if addr >> 16 == 0x8B:
            cine_sfx |= dec.req_func.get(name, set())
        if addr >> 16 in (0x8B, 0x81, 0x82):
            cine_sets |= dec.music.get(name, set())
    cine_sfx |= {k for (b, _), ks in dec.req_param.items() if b == 0x8B for k in ks}
    for s in cine_sets:
        per_set.setdefault(s, set()).update(cine_sfx - glob)
    return {"global": glob, "per_set": per_set, "rooms": info, "active": active, "dec": dec, "cine_sets": cine_sets}


def main():
    m = build(sys.argv[1])
    print(f"globales: {len(m['global'])}")
    print(" ".join(f"{a}/{b:#04x}" for a, b in sorted(m["global"])))
    for s in sorted(m["per_set"]):
        print(f"conjunto {s:#04x}: {len(m['per_set'][s])} efectos:",
              " ".join(f"{a}/{b:#04x}" for a, b in sorted(m["per_set"][s])))
    if "--salas" in sys.argv:
        for (ra, st), i in sorted(m["rooms"].items()):
            print(f"sala {ra:04X} estado {st:04X}: conjunto {i['set']:#04x}, se escuchan "
                  f"{[hex(x) for x in sorted(m['active'][(ra, st)])]}, efectos {len(i['sfx'])}")


if __name__ == "__main__":
    main()
