#!/usr/bin/env python3
"""Datos del motor nativo, preparados en el PC (docs/31).

Lee la ROM del SNES del usuario (y algunas tablas que el decomp tiene como arreglos de C) y escribe
gen/tables.c / gen/tables.h con TODO en formato nativo del 68000: estructuras con campos del tamaño
justo, palabras de 16 bits ya en el orden de la CPU, listas de longitud variable aplanadas con su
índice. El motor no lee nunca bytes del SNES en tiempo de ejecución.

Las tablas que el juego guarda como "punteros" de 16 bits del SNES (por ejemplo la tabla de retardos
de animación de cada pose) se emiten como un bloque de bytes con su dirección base: el puntero sigue
siendo el del SNES (así se compara la RAM con la del SNES cuadro a cuadro) y el motor lo convierte en
un índice con una resta (bankXX_ptr).

Uso: mkdata.py sm.smc smsrc/src res/gen/engine
"""
import re, struct, sys
from pathlib import Path

rom = Path(sys.argv[1]).read_bytes()
src = Path(sys.argv[2])
out = Path(sys.argv[3])
HDR = len(rom) % 0x8000
NUM_POSES = 253


def r(addr, n):
    b = (addr >> 16) & 0x7F
    o = HDR + b * 0x8000 + (addr & 0x7FFF)
    return rom[o:o + n]


def w16(addr):
    return struct.unpack('<H', r(addr, 2))[0]


def words(addr, n):
    return list(struct.unpack('<%dH' % n, r(addr, 2 * n)))


H = ['/* Generado por tools/conv/mkdata.py desde la ROM del SNES: no editar. */',
     '#ifndef GEN_TABLES_H', '#define GEN_TABLES_H', '#include "nt.h"']
C = ['/* Generado por tools/conv/mkdata.py desde la ROM del SNES: no editar. */', '#include "tables.h"']


def arr(ctype, name, vals, per=16, fmt='{}'):
    al = ' __attribute__((aligned(2)))' if ctype in ('uint8', 'int8') else ''   # gcc lee de a palabras pares
    H.append(f'extern const {ctype} {name}[{len(vals)}]{al};')
    C.append(f'const {ctype} {name}[{len(vals)}]{al} = {{')
    for i in range(0, len(vals), per):
        C.append('  ' + ', '.join(fmt.format(v) for v in vals[i:i + per]) + ',')
    C.append('};')


def hexs(v):
    return '0x%X' % v


def c_array_from_source(fname, name):
    """Un arreglo constante tal como está en el decomp (la biblia): devuelve sus valores."""
    s = (src / fname).read_text()
    m = re.search(r'(?:static )?const (\w+) ' + name + r'\[(\d*)\] = \{(.*?)\};', s, re.S)
    if not m:
        raise SystemExit('no encuentro ' + name)
    body = re.sub(r'//[^\n]*', '', m.group(3))
    vals = [int(x, 0) for x in re.findall(r'-?(?:0x[0-9a-fA-F]+|\d+)', body)]
    return m.group(1), vals


def wordblob(name, bank, lo, hi):
    """Palabras nativas de la ROM para las direcciones [lo, hi) del banco: name[a - lo] = la palabra que empieza en
    $bank:a (en cualquier dirección, como kBankXXW pero solo un trozo)."""
    data = r((bank << 16) | lo, hi - lo + 1)
    vals = [data[i] | data[i + 1] << 8 for i in range(hi - lo)]
    arr('uint16', name, vals, per=24, fmt='0x{:X}')
    H.append(f'#define {name}_LO 0x{lo:X}')
    H.append(f'#define {name}_HI 0x{hi:X}')


def blob(name, bank, lo, hi):
    """Bloque de bytes de la ROM [lo, hi) del banco (para datos que el juego recorre byte a byte)."""
    data = r((bank << 16) | lo, hi - lo)
    H.append(f'extern const uint8 {name}[{hi - lo}];  /* ${bank:02X}:{lo:04X}-{hi:04X} */')
    C.append(f'const uint8 {name}[{hi - lo}] = {{')
    for i in range(0, len(data), 32):
        C.append('  ' + ','.join(str(x) for x in data[i:i + 32]) + ',')
    C.append('};')


# ------------------------------------------------------------------ poses de Samus ($91:B629)
pp = r(0x91B629, 8 * NUM_POSES)
H.append('typedef struct { uint8 pose_x_dir, movement_type, new_pose_unless_buttons, direction_shots_fired,'
         ' y_offset_to_gfx, field_5, y_radius, field_7; } SamusPoseParams;')
H.append(f'extern const SamusPoseParams kPoseParams[{NUM_POSES}];')
C.append(f'const SamusPoseParams kPoseParams[{NUM_POSES}] = {{')
for p in range(NUM_POSES):
    C.append('  {' + ','.join(str(x) for x in pp[8 * p:8 * p + 8]) + '},')
C.append('};')

# transiciones por botones ($91:9EE2): lista por pose, aplanada; termina en new_input = 0xFFFF
H.append('typedef struct { uint16 new_input, cur_input, new_pose; } PoseEntry;')
ptrs = words(0x919EE2, NUM_POSES)
flat, idx, seen = [], [], {}
for p in range(NUM_POSES):
    a = ptrs[p]
    if a in seen:
        idx.append(seen[a]); continue
    seen[a] = len(flat)
    idx.append(len(flat))
    if a < 0x8000:                 # pose sin tabla (no se usa): lista vacía
        flat.append((0xFFFF, 0, 0)); continue
    while True:
        e = struct.unpack('<HHH', r(0x910000 | a, 6))
        if e[0] == 0xFFFF:
            flat.append((0xFFFF, 0, 0)); break
        flat.append(e)
        a += 6
H.append(f'extern const uint16 kPoseTransitionIndex[{NUM_POSES}];')
arr('uint16', 'kPoseTransitionIndex', idx)
H.append(f'extern const PoseEntry kPoseTransitions[{len(flat)}];')
C.append(f'const PoseEntry kPoseTransitions[{len(flat)}] = {{')
for e in flat:
    C.append('  {0x%X, 0x%X, 0x%X},' % e)
C.append('};')

# retardos de animación ($91:B010): punteros por pose a secuencias de bytes en el banco $91
arr('uint16', 'kSamusAnimationDelayData', words(0x91B010, NUM_POSES), fmt='0x{:X}')
H.append('#define kDefaultAnimFramePtr 0x%X' % w16(0x91B5D1))
arr('uint16', 'kSpeedBoostToAnimFramePtr', words(0x91B5DE, 5), fmt='0x{:X}')
arr('uint16', 'kSpeedBoostToCtr', words(0x91B61F, 5), fmt='0x{:X}')
blob('kBank91_AnimDelays', 0x91, 0xB010, 0xB629)
H.append('static inline const uint8 *bank91_ptr(uint16 a) { return kBank91_AnimDelays + (uint16)(a - 0xB010); }')

# velocidades horizontales ($90:9F49): tablas Normal, Agua, Lava/Ácido seguidas, 12 bytes por tipo
H.append('typedef struct { uint16 accel, accel_sub, max_speed, max_speed_sub, decel, decel_sub; } SamusSpeedEntry;')
sp = []
a = 0x9F25   # desde las 3 entradas de salto de bomba / retroceso ($90:9F25)
while a < 0xA1DD + 28 * 12:
    sp.append(words(0x900000 | a, 6)); a += 12
H.append(f'extern const SamusSpeedEntry kSamusSpeedTable[{len(sp)}];')
C.append(f'const SamusSpeedEntry kSamusSpeedTable[{len(sp)}] = {{')
for e in sp:
    C.append('  {' + ','.join('0x%X' % x for x in e) + '},')
C.append('};')
H.append('static inline const SamusSpeedEntry *speed_entry(uint16 a) { return &kSamusSpeedTable[(uint16)(a - 0x9F25) / 12]; }')
for n, v in (('Tab0', 0), ('Tab1', 0x1000), ('Tab2', 7), ('Tab3', 0), ('Tab4', 2), ('Tab5', 0)):
    H.append(f'#define kSamus_HandleExtraRunspeedX_{n} 0x{v:X}  /* sm_90.c, índice 0 (aire) */')

# spritemaps de Samus por pose ($92:9263 / $92:945D)
arr('uint16', 'kSamusPoseToBaseSpritemapIndexTop', words(0x929263, NUM_POSES))
arr('uint16', 'kSamusPoseToBaseSpritemapIndexBottom', words(0x92945D, NUM_POSES))

# tablas de poses de $91
arr('uint16', 'word_91E921', words(0x91E921, 8), fmt='0x{:X}')
arr('uint16', 'word_91E9F3', words(0x91E9F3, 16), fmt='0x{:X}')
arr('uint16', 'word_91EB74', words(0x91EB74, 16), fmt='0x{:X}')
for n, a in (('Standing', 0x91F9C2), ('Crouching', 0x91F9CC), ('Jumping', 0x91F9D6), ('Falling', 0x91F9E0), ('Moonwalk', 0x91F9EA)):
    arr('uint8', 'kSamusTurnPose_' + n, list(r(a, 10)), fmt='0x{:X}')
arr('uint8', 'kSamusFramesForUnderwaterSfx', list(r(0x90A514, 16)))

# paletas de Samus ($91 punteros, $9B colores BGR555 del SNES)
arr('uint16', 'off_91D727', words(0x91D727, 3), fmt='0x{:X}')
arr('uint16', 'kSamusPalette_NonPseudoScrew', words(0x91D7D5, 3), fmt='0x{:X}')
arr('uint16', 'kSamusPalette_PseudoScrew', words(0x91D7FF, 3), fmt='0x{:X}')
pal_lo, pal_hi = 0x9400, 0xA400
pw = words(0x9B0000 | pal_lo, (pal_hi - pal_lo) // 2)
arr('uint16', 'kBank9B_Palettes', pw, fmt='0x{:X}')
H.append(f'static inline const uint16 *bank9B_ptr(uint16 a) {{ return kBank9B_Palettes + (uint16)(a - 0x{pal_lo:X}) / 2; }}')
arr('uint16', 'word_9BA3C0', words(0x9BA3C0, 6), fmt='0x{:X}')
# los punteros de color de carga de rayo (por pose de Samus, en el banco $91): palabra en $91
arr('uint16', 'kTanTable', words(0x91C9D4, 128), fmt='0x{:X}')   # tangentes ($91:C9D4): conos de los rayos X y del ojo
# paletas especiales de Samus ($91:D727-DCB4: punteros de carga, tornillo, speed booster, shinespark, destello de
# cristal; bank91_word lee una palabra de ahí, byte bajo primero, como RomPtr_91 en el decomp)
blob('kBank91_Pals', 0x91, 0xD727, 0xDCB4)
blob('kBank91_SuitHdma', 0x91, 0xD5A2, 0xD692)
blob('kBank9B_Grapple', 0x9B, 0xC0DB, 0xC9CE)   # tablas del gancho (grapple.c)   # listas del objeto de HDMA de los trajes (suit.c)
H.append('static inline uint16 bank91_word(uint16 a) { const uint8 *p = kBank91_Pals + (uint16)(a - 0xD727); '
         'uint16 lo = p[0]; __asm__("" : "+d"(lo)); return lo | p[1] << 8; }')

# tiles de Samus por cuadro de animación ($92:D94E): registros de 4 bytes (top_idx, top_pos, bottom_idx, bottom_pos)
adp = words(0x92D94E, NUM_POSES)
arr('uint16', 'kSamus_AnimationDefinitionPtrs', adp, fmt='0x{:X}')
lo, hi = min(adp), min(0x10000, max(adp) + 4 * 40)
blob('kBank92_AnimDefs', 0x92, lo, hi)
H.append(f'static inline const uint8 *bank92_animdef(uint16 a) {{ return kBank92_AnimDefs + (uint16)(a - 0x{lo:X}); }}')
arr('uint16', 'kSamus_TileDefs_TopHalf', words(0x92D91E, 13), fmt='0x{:X}')
arr('uint16', 'kSamus_TileDefs_BottomHalf', words(0x92D938, 11), fmt='0x{:X}')

# cañón del brazo ($90:C7DF punteros por pose a registros de bytes en $90)
pp2 = words(0x90C7DF, NUM_POSES)
arr('uint16', 'kPlayerPoseToPtr', pp2, fmt='0x{:X}')
blob('kBank90_ArmCannon', 0x90, 0xC7A5, 0xCD00)
H.append('static inline const uint8 *bank90_armcannon(uint16 a) { return kBank90_ArmCannon + (uint16)(a - 0xC7A5); }')
# destello de la carga del rayo (HandleChargingBeamGfxAudio $90:BAFC): kFlareAnimDelays ($90:C481, 3 punteros) y sus
# listas de tiempos (bytes; 255 vuelve al principio, 254 n retrocede n)
blob('kBank90_Flare', 0x90, 0xC481, 0xC4B5)
H.append('static inline const uint8 *bank90_flare(uint16 a) { return kBank90_Flare + (uint16)(a - 0xC481); }')
H.append('static inline uint16 bank90_flare_word(uint16 a) { const uint8 *p = bank90_flare(a); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }')

# pendientes (arreglos del decomp, sm_94.c)
for name in ('kAlignYPos_Tab0', 'kTab948E54', 'kBlockColl_Horiz_Slope_NonSquare_Tab', 'kAlignPos_Tab1'):
    t, v = c_array_from_source('sm_94.c', name)
    arr({'uint8': 'uint8', 'uint16': 'uint16'}[t], name, v, fmt='0x{:X}')

# ------------------------------------------------------------------ salas (cabeceras, estados, puertas, nivel)
def decompress(ea):
    """LZ de Super Metroid (DecompressToMem): se descomprime aquí, en el PC."""
    o = HDR + ((ea >> 16) & 0x7F) * 0x8000 + (ea & 0x7FFF)
    data, p, rv = rom, o, bytearray()
    while True:
        b = data[p]; p += 1
        if b == 0xff:
            return bytes(rv)
        if (b & 0xe0) == 0xe0:
            cmd = (8 * b) & 0xe0; size = ((b & 3) << 8 | data[p]) + 1; p += 1
        else:
            cmd = b & 0xe0; size = (b & 0x1f) + 1
        if cmd & 0x80:
            x = 0xff if cmd & 0x20 else 0
            if cmd >= 0xc0:
                src = len(rv) - data[p]; p += 1
            else:
                src = data[p] | data[p + 1] << 8; p += 2
            for i in range(size):
                rv.append(rv[src + i] ^ x)
        elif cmd == 0x20:
            rv += bytes([data[p]]) * size; p += 1
        elif cmd == 0x40:
            b1, b2 = data[p], data[p + 1]; p += 2
            for i in range(size):
                rv.append(b1 if i % 2 == 0 else b2)
        elif cmd == 0x60:
            b1 = data[p]; p += 1
            for i in range(size):
                rv.append((b1 + i) & 0xff)
        else:
            rv += data[p:p + size]; p += size

SEL = {0xE5E6: 0, 0xE5FF: 1, 0xE612: 2, 0xE629: 3, 0xE652: 4, 0xE669: 5}   # tipos de RoomDefStateSelect


def all_rooms():
    """todas las salas: se recorren las puertas desde Ceres y desde Landing Site (docs/37)"""
    seen, todo = set(), [0xDF45, 0x91F8]
    while todo:
        ra = todo.pop()
        if ra in seen:
            continue
        seen.add(ra)
        dl = 0x8F0000 | w16(0x8F0000 | ra + 9)
        i = 0
        while True:
            d = w16(dl + 2 * i)
            if d < 0x8000:
                break
            dest = w16(0x830000 | d)
            if dest >= 0x8000 and dest not in seen:
                todo.append(dest)
            i += 1
    return sorted(seen)


ROOMS_ALL = all_rooms()
LEVEL_PAGE0 = 8                     # Genesis: los niveles van en las páginas 8, 9... del SSF2 (ventana 0x380000)
H.append('typedef struct { uint16 addr, room_definition_ptr; uint8 door_bitflags, door_orientation, x_pos_plm,'
         ' y_pos_plm, x_pos_in_room, y_pos_in_room; uint16 samus_distance_from_door, door_setup_code; } DoorDefN;')
H.append('typedef struct { uint16 addr; uint8 graphics_set, music_track, music_control, level; uint16 room_layer3_fx_ptr,'
         ' enemy_population_ptr, enemy_tilesets_ptr, vertical_screen_nudge_limit, rdf_scroll_ptr, special_xray,'
         ' main_code_ptr, room_plm_header_ptr, bg_data_ptr, room_setup_code; } RoomStateN;')
H.append('typedef struct { uint8 kind, arg; uint16 state; } StateSelectN;  /* kind: 0 fin, 1 TourianBoss01, 2 IsEventSet,'
         ' 3 IsBossDead, 4 MorphBallMissiles, 5 PowerBombs */')
H.append('typedef struct { uint16 addr; uint8 semiunique_room_number, area_index, x_coordinate_on_map, y_coordinate_on_map,'
         ' width, height, up_scroller, down_scroller, cre_bitset, ndoors, nsel; uint16 ptr_to_doorout;'
         ' const uint16 *doors; const StateSelectN *sel; } RoomDefN;')
H.append('typedef struct { uint16 size; uint8 page; const uint16 *level; const uint8 *bts; } LevelDataN;  /* size: bytes de level_data; page: página del SSF2 (0 = fija) */')
H.append('typedef struct { uint16 addr, door_ptr, base_y_pos, target_y_pos, y_vel; uint8 timer, type, default_layer_blend,'
         ' layer3_layer_blend, fx_liquid_options_, palette_fx_bitset, animtiles_bitset, palette_blend; } FxDefN;')
doors, states, levels, fxs = {}, {}, {}, {}
rooms_c = []
for ra in ROOMS_ALL:
    a = 0x8F0000 | ra
    hb = r(a, 11)
    dl = 0x8F0000 | struct.unpack('<H', hb[9:11])[0]
    dlist = []
    while True:
        d = w16(dl + 2 * len(dlist))
        if d < 0x8000: break
        dlist.append(d)
    sels, q = [], a + 11
    while True:
        k = w16(q)
        kind = SEL[k]
        if kind == 0:
            sels.append((0, 0, (q + 2) & 0xFFFF)); break
        if kind in (2, 3):
            sels.append((kind, r(q + 2, 1)[0], w16(q + 3))); q += 5
        else:
            sels.append((kind, 0, w16(q + 2))); q += 4
    for d in dlist:
        doors[d] = struct.unpack('<HBBBBBBHH', r(0x830000 | d, 12))
    for _, _, st in sels:
        b = r(0x8F0000 | st, 26)
        lvl = b[0] | b[1] << 8 | b[2] << 16
        levels.setdefault(lvl, None)
        states[st] = (lvl, b[3], b[4], b[5]) + struct.unpack('<10H', b[6:26])
        fxs.setdefault(states[st][4], None)
    rooms_c.append((ra, hb, dlist, sels, dl & 0xFFFF))
# puertas de las estaciones de carga ($80:C4B5; la de la nave, $83:88FE, no está en la lista de ninguna sala)
for a in range(8):
    lo = w16(0x80C4B5 + 2 * a)
    if lo < 0xC4B5: continue
    for i in range(32):
        q = 0x800000 | lo + 14 * i
        if (q & 0xFFFF) >= 0xCD07: break
        d = w16(q + 2)
        if d >= 0x8000 and w16(0x830000 | d) in ROOMS_ALL:
            doors.setdefault(d, struct.unpack('<HBBBBBBHH', r(0x830000 | d, 12)))
# visor de salas (viewer.c): las salas de Zebes (áreas 0-5) por área, y una puerta que entra a cada una
_enter = {}
for ra_ in ROOMS_ALL:
    dl_ = 0x8F0000 | w16(0x8F0000 | ra_ + 9)
    i_ = 0
    while True:
        d_ = w16(dl_ + 2 * i_)
        if d_ < 0x8000:
            break
        dest_ = w16(0x830000 | d_)
        flags_ = r(0x830000 | d_ + 2, 1)[0]
        if dest_ >= 0x8000 and r(0x8F0000 | dest_ + 1, 1)[0] <= 5:
            prev_ = _enter.get(dest_)
            if prev_ is None or (prev_[1] & 0x80 and not flags_ & 0x80):   # mejor una puerta que un ascensor
                _enter[dest_] = (d_, flags_)
        i_ += 1
for d_, _f in _enter.values():
    doors.setdefault(d_, struct.unpack('<HBBBBBBHH', r(0x830000 | d_, 12)))
_vr = sorted(_enter, key=lambda ra_: (r(0x8F0000 | ra_ + 1, 1)[0], ra_))
H.append('typedef struct { uint16 room, door; } ViewerRoomN;')
H.append(f'extern const ViewerRoomN kViewerRooms[{len(_vr)}];')
C.append(f'const ViewerRoomN kViewerRooms[{len(_vr)}] = {{ ' + ', '.join('{0x%X, 0x%X}' % (ra_, _enter[ra_][0]) for ra_ in _vr) + ' };')
lvl_idx = {}
H.append('#ifdef NT_SSF2')
H.append('#define NT_BANK(n) __attribute__((section(".bank" #n)))   /* página n del mapeador SSF2 (md_ssf2.ld) */')
H.append('#else')
H.append('#define NT_BANK(n)')
H.append('#endif')
page, used, lvl_page = LEVEL_PAGE0, 0, {}
for i, lvl in enumerate(sorted(levels)):
    lvl_idx[lvl] = i
    d = decompress(lvl)
    size = d[0] | d[1] << 8
    need = size + size // 2 + 4
    if used + need > 0x80000:
        page, used = page + 1, 0
    used += need
    lvl_page[lvl] = page
    lw = list(struct.unpack('<%dH' % (size // 2), d[2:2 + size]))
    bts = list(d[2 + size:2 + size + size // 2])
    C.append(f'NT_BANK({page}) const uint16 kLevel{i}_Data[{len(lw)}] = {{' + ','.join('0x%X' % v for v in lw) + '};')
    C.append(f'NT_BANK({page}) const uint8 kLevel{i}_Bts[{len(bts)}] = {{' + ','.join(str(v) for v in bts) + '};')
H.append(f'#define NT_LEVEL_PAGES {page - LEVEL_PAGE0 + 1}   /* páginas del SSF2 con niveles, desde la {LEVEL_PAGE0} */')
H.append(f'extern const LevelDataN kLevels[{len(levels)}];')
C.append(f'const LevelDataN kLevels[{len(levels)}] = {{')
for lvl, i in sorted(lvl_idx.items(), key=lambda t: t[1]):
    d = decompress(lvl); size = d[0] | d[1] << 8
    C.append(f'  {{ 0x{size:X}, {lvl_page[lvl]}, kLevel{i}_Data, kLevel{i}_Bts }},  /* ${lvl:06X} */')
C.append('};')
H.append(f'extern const DoorDefN kDoorDefs[{len(doors)}];')
C.append(f'const DoorDefN kDoorDefs[{len(doors)}] = {{')
for d in sorted(doors):
    C.append('  { 0x%X, ' % d + ', '.join('0x%X' % v for v in doors[d]) + ' },')
C.append('};')
H.append(f'extern const RoomStateN kRoomStates[{len(states)}];')
C.append(f'const RoomStateN kRoomStates[{len(states)}] = {{')
for st in sorted(states):
    v = states[st]
    C.append('  { 0x%X, %d, %d, %d, %d, ' % (st, v[1], v[2], v[3], lvl_idx[v[0]]) + ', '.join('0x%X' % x for x in v[4:]) + ' },')
C.append('};')
for ra, hb, dlist, sels, dlp in rooms_c:
    arr('uint16', 'kRoomDoors_%04X' % ra, dlist, fmt='0x{:X}')
    H.append(f'extern const StateSelectN kRoomSel_{ra:04X}[{len(sels)}];')
    C.append(f'const StateSelectN kRoomSel_{ra:04X}[{len(sels)}] = {{ ' + ', '.join('{%d, %d, 0x%X}' % s for s in sels) + ' };')
H.append(f'extern const RoomDefN kRoomDefs[{len(rooms_c)}];')
rooms_c.sort(key=lambda t: t[0])
C.append(f'const RoomDefN kRoomDefs[{len(rooms_c)}] = {{')
for ra, hb, dlist, sels, dlp in rooms_c:
    C.append('  { 0x%X, ' % ra + ', '.join(str(x) for x in hb[:9]) +
             f', {len(dlist)}, {len(sels)}, 0x{dlp:X}, kRoomDoors_{ra:04X}, kRoomSel_{ra:04X} }},')
C.append('};')
# efectos (FX) de capa 3 ($83): listas por puntero; la entrada con door_ptr 0 o 0xFFFF termina
fxl = []
for fp in sorted(fxs):
    if not fp: continue
    q = fp
    while True:
        e = struct.unpack('<4HBBBBBBBB', r(0x830000 | q, 16))
        fxl.append((q,) + e)
        if e[0] in (0, 0xFFFF): break
        q += 16
H.append(f'extern const FxDefN kFxDefs[{len(fxl)}];')
C.append(f'const FxDefN kFxDefs[{len(fxl)}] = {{')
for e in fxl:
    C.append('  { ' + ', '.join('0x%X' % v for v in e) + ' },')
C.append('};')
arr('uint16', 'kFxTypeTilemapPtrs', words(0x83ABF0, 24), fmt='0x{:X}')
# efectos de paleta y tiles animados de cada área (8 por área, elegidos por los bits del FX): $83:AC46 / $83:AC56
arr('uint16', 'kAreaPalFxLists', sum((words(0x830000 | p, 8) for p in words(0x83AC46, 8)), []), fmt='0x{:X}')
arr('uint16', 'kAreaAnimtilesLists', sum((words(0x830000 | p, 8) for p in words(0x83AC56, 8)), []), fmt='0x{:X}')
arr('uint16', 'g_word_89AA02', words(0x89AA02, 130), fmt='0x{:X}')   # paletas de mezcla del FX: índice = byte de la cabecera / 2 (+2)
arr('uint16', 'g_word_89AD5F', words(0x89AD5F, 69 * 3), fmt='0x{:X}')        # balanceo del pozo en la huida
arr('uint16', 'kInitialPalette_Sprites', words(0x9A8000 + 2 * 208, 16), fmt='0x{:X}')   # kInitialPalette[208..223]
arr('uint16', 'kCommonSpritesPalette1', words(0x9AFC00, 16), fmt='0x{:X}')
bp = words(0x90C3C9, 12)
arr('uint16', 'kBeamPalettes', sum((words(0x900000 | x, 16) for x in bp), []), fmt='0x{:X}')  # 12 x 16 colores

# ------------------------------------------------------------------ proyectiles (banco $93) y estelas ($90, $9B)
# Las listas de instrucciones de proyectil empiezan en direcciones impares: se emiten como palabras desde una base
# impar (todas las direcciones que usa el juego tienen la misma paridad; el motor lo comprueba).
B93_LO, B93_HI = 0x83C1, 0xA20D   # hasta el final de g_off_93A1A1 (spritemaps del destello de la carga)
arr('uint16', 'kBank93', words(0x930000 | B93_LO, (B93_HI - B93_LO) // 2), fmt='0x{:X}')
H.append(f'#define BANK93_LO 0x{B93_LO:X}')
H.append(f'#define BANK93_HI 0x{B93_HI:X}')
H.append('#define kProjectileData_UnchargedBeams 0x83C1')
H.append('#define kProjectileData_ChargedBeams 0x83D9')
H.append('#define kProjectileData_NonBeams 0x83F1')
H.append('#define kShinesparkEchoSpazer_ProjectileData 0x8403')
H.append('#define kProjInstr_BeamExplosion 0x%X  /* HIWORD(g_stru_938679.damages) */' % w16(0x93867B))
H.append('#define kProjInstr_MissileExplosion 0x%X  /* *(&g_stru_938679.instr_ptr + 1) */' % w16(0x93867F))
H.append('#define kProjInstr_SuperMissileExplosion 0x%X  /* HIWORD(g_stru_938691.damages) */' % w16(0x938693))
H.append('#define kProjInstr_BombExplosion 0x%X  /* HIWORD(kProjInstrList_Explosion.damages) */' % w16(0x938683))
TR_LO, TR_HI = 0xB4C9, 0xB5BB
arr('uint16', 'kBank90_Trails', words(0x900000 | TR_LO, (TR_HI - TR_LO) // 2), fmt='0x{:X}')
H.append(f'#define BANK90_TRAILS_LO 0x{TR_LO:X}')
arr('uint16', 'off_90B5BB', words(0x90B5BB, 39), fmt='0x{:X}')
arr('uint16', 'off_90B609', words(0x90B609, 39), fmt='0x{:X}')
T9_LO, T9_HI = 0xA4B3, 0xA56F
arr('uint16', 'kBank9B_TrailPtrs', words(0x9B0000 | T9_LO, (T9_HI - T9_LO) // 2), fmt='0x{:X}')
H.append(f'#define BANK9B_TRAILPTRS_LO 0x{T9_LO:X}')
blob('kBank9B_TrailOffsets', 0x9B, 0xA56F, 0xB3A7)

# ------------------------------------------------------------------ enemigos de Ceres (cabeceras $A0, poblaciones $A1, sets $B4)
# todos los enemigos que aparecen en las poblaciones y sets de tiles de las salas preparadas (ordenados: búsqueda binaria)
_ed = {0xE23F, 0xE13F, 0xE1FF, 0xDAFF}   # DAFF: el que queda en lugar de un enemigo muerto que reaparece
for st, v in states.items():
    q = 0xA10000 | v[5]
    while True:
        d = w16(q)
        if d == 0xFFFF: break
        _ed.add(d); q += 16
    q = 0xB40000 | v[6]
    while True:
        d = w16(q)
        if d == 0xFFFF: break
        _ed.add(d); q += 4
ENEMY_DEFS = sorted(_ed)
H.append('typedef struct { uint16 addr, tile_data_size; uint16 palette[16]; uint16 health, damage, x_radius, y_radius;'
         ' uint8 bank, hurt_ai_time; uint16 boss_fight_value, ai_init, ai_bits[5], powerbomb_reaction, touch_ai, shot_ai;'
         ' uint8 layer; uint16 vulnerability_ptr, name_ptr, num_parts; } EnemyDefN;  /* ai_bits: main, grapple, hurt, frozen, time_is_frozen */')
H.append(f'extern const EnemyDefN kEnemyDefs[{len(ENEMY_DEFS)}];')
C.append(f'const EnemyDefN kEnemyDefs[{len(ENEMY_DEFS)}] = {{')
for e in ENEMY_DEFS:
    b = r(0xA00000 | e, 64)
    W = lambda o: b[o] | b[o + 1] << 8
    bank = b[12]
    pal = words((bank << 16) | W(2), 16)
    vals = [W(0), '{' + ','.join('0x%X' % x for x in pal) + '}', W(4), W(6), W(8), W(10), bank, b[13], W(16), W(18),
            '{' + ','.join('0x%X' % W(o) for o in (24, 26, 28, 30, 32)) + '}', W(40), W(48), W(50), b[57], W(62), W(64) if len(b) > 65 else 0]
    nm = r(0xA00000 | e + 62, 2)
    vals[-1] = nm[0] | nm[1] << 8
    vul = r(0xA00000 | e + 60, 2)
    vals[-2] = vul[0] | vul[1] << 8
    vals.append(W(20))
    C.append('  { 0x%X, ' % e + ', '.join(v if isinstance(v, str) else '0x%X' % v for v in vals) + ' },')
C.append('};')
# sets de tiles de enemigos ($B4) y poblaciones ($A1) de los estados de las salas de Ceres
H.append('typedef struct { uint16 addr; uint8 n; uint16 enemy_def[4], vram_dst[4]; } EnemyTilesetN;')
H.append('typedef struct { uint16 enemy_ptr, x_pos, y_pos, init_param, properties, extra_properties, parameter1, parameter2; } EnemyPopEntryN;')
H.append('typedef struct { uint16 addr; uint8 n, kill_count; const EnemyPopEntryN *e; } EnemyPopulationN;')
tsets, pops = {}, {}
for st, v in states.items():
    pops[v[5]] = None; tsets[v[6]] = None
H.append(f'extern const EnemyTilesetN kEnemyTilesets[{len(tsets)}];')
C.append(f'const EnemyTilesetN kEnemyTilesets[{len(tsets)}] = {{')
for t in sorted(tsets):
    ents, q = [], 0xB40000 | t
    while True:
        d, v = struct.unpack('<HH', r(q, 4))
        if d == 0xFFFF: break
        ents.append((d, v)); q += 4
    assert len(ents) <= 4, hex(t)
    ents += [(0, 0)] * (4 - len(ents))
    C.append('  { 0x%X, %d, {%s}, {%s} },' % (t, len([x for x in ents if x[0]]), ','.join('0x%X' % x[0] for x in ents),
                                            ','.join('0x%X' % x[1] for x in ents)))
C.append('};')
popl = []
for pp_ in sorted(pops):
    ents, q = [], 0xA10000 | pp_
    while True:
        e = struct.unpack('<8H', r(q, 16))
        if e[0] == 0xFFFF: break
        ents.append(e); q += 16
    kill = r(q + 2, 1)[0]
    C.append('static const EnemyPopEntryN kEnemyPop_%04X[%d] = { %s };' % (pp_, max(1, len(ents)),
             ', '.join('{' + ','.join('0x%X' % x for x in e) + '}' for e in ents) or '{0}'))
    popl.append((pp_, len(ents), kill))
H.append(f'extern const EnemyPopulationN kEnemyPopulations[{len(pops)}];')
C.append(f'const EnemyPopulationN kEnemyPopulations[{len(pops)}] = {{')
for pp_, n, kill in popl:
    C.append('  { 0x%X, %d, %d, kEnemyPop_%04X },' % (pp_, n, kill, pp_))
C.append('};')
H.append('#define kEnemyLayerToQueue_0 0xE84')
arr('uint16', 'kEnemyLayerToQueuePtr', words(0xA0B133, 8), fmt='0x{:X}')
# puerta de Ceres ($A6): listas de instrucciones, paletas
arr('uint16', 'g_off_A6F72B', words(0xA6F72B, 4), fmt='0x{:X}')
arr('uint16', 'g_off_A6F52C', words(0xA6F52C, 4), fmt='0x{:X}')
arr('uint16', 'kBankA6_CeresDoorInstr', words(0xA6F52C, (0xF620 - 0xF52C) // 2), fmt='0x{:X}')
H.append('#define BANKA6_CERESDOOR_LO 0xF52C')
H.append('#define BANKA6_CERESDOOR_HI 0xF620')
arr('uint16', 'kCeresDoor_Palette', words(0xA6F4EC, 16), fmt='0x{:X}')
arr('uint16', 'word_A6F50C', words(0xA6F50C, 16), fmt='0x{:X}')
arr('uint16', 'kCeresDoor_PaletteCycle', words(0xA6F871, 64), fmt='0x{:X}')   # desde $A6:F871 (impar)

# ------------------------------------------------------------------ bebé metroide en la sala de Ridley de Ceres ($A6)
arr('uint16', 'kBankA6_BabyIlist', words(0xA6BF31, (0xBFC9 - 0xBF31) // 2), fmt='0x{:X}')   # base impar $BF31
H.append('#define BANKA6_BABY_LO 0xBF31')
H.append('#define BANKA6_BABY_HI 0xBFC9')
arr('uint16', 'kBankA6_RidleyPals', words(0xA6E16F, (0xE26B - 0xE16F) // 2), fmt='0x{:X}')  # base impar $E16F
H.append('#define BANKA6_RIDLEYPALS_LO 0xE16F')
H.append('#define BANKA6_RIDLEYPALS_HI 0xE26B')

# ------------------------------------------------------------------ proyectiles enemigos del ascensor de Ceres ($86) y PLM B7EB ($84)
arr('uint16', 'kBank86_Elevator', words(0x86A28B, (0xA2A1 - 0xA28B) // 2), fmt='0x{:X}')   # base impar $A28B
H.append('#define BANK86_ELEVATOR_LO 0xA28B')
H.append('#define BANK86_ELEVATOR_HI 0xA2A1')

# ------------------------------------------------------------------ paletas de los tilesets de Ceres (lo que el SNES descomprime en 7E:C200)
gsets = sorted(set(v[1] for v in states.values()))
H.append(f'extern const uint8 kTilesetPaletteSets[{len(gsets)}];')
C.append(f'const uint8 kTilesetPaletteSets[{len(gsets)}] = {{ ' + ', '.join(str(g) for g in gsets) + ' };')
pals = []
for g in gsets:
    tsp = w16(0x8FE7A7 + 2 * g)
    tb = r(0x8F0000 | tsp, 9)
    pp_ = tb[6] | tb[7] << 8 | tb[8] << 16
    d = decompress(pp_)
    pals.append(list(struct.unpack('<128H', d[:256])))
arr('uint16', 'kTilesetPalettes', sum(pals, []), fmt='0x{:X}')   # 128 colores por set, en el orden de kTilesetPaletteSets

# ------------------------------------------------------------------ mapas de las áreas para el minimapa del HUD
# kAreaMapData[area * 256]: qué celdas tienen mapa (kPauseMenuMapData $82:9717); kAreaMapTilemaps[area * 2048]:
# las celdas del mapa (kPauseMenuMapTilemaps $82:964A, 3 bytes por área). Áreas 0-6 (7 es la de depuración).
_md, _mt = [], []
for a in range(7):
    _md += list(r(0x820000 | w16(0x829717 + 2 * a), 256))
    t = r(0x82964A + 3 * a, 3); t = t[0] | t[1] << 8 | t[2] << 16
    _mt += words(t, 2048)
arr('uint8', 'kAreaMapData', _md)
arr('uint16', 'kAreaMapTilemaps', _mt, fmt='0x{:X}')

# ------------------------------------------------------------------ comienzo de la partida (docs/37)
arr('uint16', 'kInitialPalette', words(0x9A8000, 256), fmt='0x{:X}')
# estaciones de carga ($80:C4B5: una lista por área; entradas de 14 bytes)
H.append('typedef struct { uint8 area, index; uint16 room_ptr, door_ptr, door_bts, screen_x_pos, screen_y_pos,'
         ' samus_y_offset, samus_x_offset; } LoadStationN;')
lsl = [w16(0x80C4B5 + 2 * a) for a in range(8)]
ends = sorted(set(lsl)) + [0xCD07]                       # SetElevatorsAsUsed ($80:CD07) viene después
stations = []
for a in range(8):
    lo = lsl[a]
    hi = min(x for x in ends if x > lo)
    for i in range((hi - lo) // 14):
        e = struct.unpack('<7H', r(0x800000 | lo + 14 * i, 14))
        stations.append((a, i) + e)
H.append(f'extern const LoadStationN kLoadStations[{len(stations)}];')
C.append(f'const LoadStationN kLoadStations[{len(stations)}] = {{')
for st in stations:
    C.append('  { %d, %d, ' % st[:2] + ', '.join('0x%X' % v for v in st[2:]) + ' },')
C.append('};')
# ascensores usados ($80:CD46: puntero por área a pares (índice, bit) de 4 bytes; el resto de la tabla, tal cual)
arr('uint8', 'kElevatorUsedBits', list(r(0x80CD46, 0xCD9A - 0xCD46)))

# ------------------------------------------------------------------ HUD ($80)
arr('uint16', 'kDigitTilesetsHealth', words(0x809DBF, 10), fmt='0x{:X}')
arr('uint16', 'kDigitTilesetsWeapon', words(0x809DD3, 10), fmt='0x{:X}')
arr('uint16', 'kHudTilemaps_AutoReserve', words(0x80998B, 12), fmt='0x{:X}')
for name in ('kHudTilemaps_Row1to3', 'kHudTilemaps_Missiles'):
    t, v = c_array_from_source('sm_80.c', name)
    arr('uint16', name, v, fmt='0x{:X}')

# ------------------------------------------------------------------ bancos completos como palabras nativas (docs/33)
# kBankXXW[a - 0x8000] = la palabra de 16 bits que empieza en $XX:a (en cualquier dirección, par o impar), ya en el
# orden del 68000. Así el motor sigue usando los punteros de 16 bits del SNES (y la RAM se compara con la del SNES)
# sin leer bytes del SNES: un puntero es un índice. Para los bytes, el byte bajo de la palabra.
def bankwords(name, bank):
    data = r((bank << 16) | 0x8000, 0x8000) + r(((bank + 1) << 16) | 0x8000, 1)
    vals = [data[i] | data[i + 1] << 8 for i in range(0x8000)]
    arr('uint16', name, vals, per=24, fmt='0x{:X}')

# $A6: kBankA6B (banks.txt); las palabras se leen de a dos bytes (nmath.h), como los demás bancos
# bancos que lee el código traducido del decomp (tools/port/banks.txt, docs/42): bytes tal cual
import os as _os
_bk = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..', '..', 'tools', 'port', 'banks.txt')
_bl = open(_bk).read().split() if _os.path.exists(_bk) else []
if '86' not in _bl:
    _bl.append('86')                       # proyectiles de enemigos: también el motor escrito a mano (nmath.h)
for _b in _bl:
    _b = int(_b, 16)
    arr('uint8', 'kBank%02XB' % _b, list(r((_b << 16) | 0x8000, 0x8000) + r(((_b + 1) << 16) | 0x8000, 1)), per=32)
# bancos poco usados: bytes (la mitad de ROM que las palabras; nmath.h los lee de a dos)
def bankbytes(name, bank):
    arr('uint8', name, list(r((bank << 16) | 0x8000, 0x8000) + r(((bank + 1) << 16) | 0x8000, 1)), per=32)
# cinemáticas: listas de instrucciones ($8B), efectos de paleta ($8D), objetos de HDMA y tablas del power bomb ($88)
bankbytes('kBank81B', 0x81)   # menú de fin de juego: tilemaps y objetos de HDMA
bankbytes('kBank82B', 0x82)   # menú de fin de juego: la bebé metroide (instrucciones y paletas)
arr('uint16', 'kMenuPalettes', words(0x8EE400, 256), fmt='0x{:X}')            # $8E:E400
bankbytes('kBank88B', 0x88)
bankbytes('kBank8BB', 0x8B)
bankbytes('kBank8DB', 0x8D)
bankbytes('kBank87B', 0x87)   # tiles animados (listas; los tiles en sí, la Genesis)
bankbytes('kBank84B', 0x84)   # PLMs (cabeceras, listas de instrucciones, tablas de bloques)
bankbytes('kBank8FB', 0x8F)
wordblob('kBank85_MsgBox', 0x85, 0x869B, 0x95C0)   # cajas de mensaje: MsgBoxConfig y mapas de los mensajes (msgbox.c)
# pausa: mapas del BG2 y del equipo ($B6:E000-EFFF) y paletas ($B6:F000-F1FF), de a palabras (direcciones pares)
arr('uint16', 'kBankB6_Pause', words(0xB6E000, 0x900), fmt='0x{:X}')
C[:] = [l.replace('const uint16 kBankB6_Pause', 'NT_LATE_DATA const uint16 kBankB6_Pause') for l in C]   # al final de la parte fija (nt.h)
H.append('static inline uint16 BB6W(uint16 a) { return kBankB6_Pause[(uint16)(a - 0xE000) >> 1]; }')
wordblob('kBankA2_Gunship', 0xA2, 0xA5BE, 0xA7D8)   # nave de Samus: listas de instrucciones y tablas
wordblob('kBank90_Atmos', 0x90, 0x8B93, 0x8C1F)     # efectos de ambiente (pisadas, polvo, burbujas)
wordblob('kBank94_React', 0x94, 0x9100, 0xA060)     # bloques especiales: qué PLM crea cada uno
wordblob('kBankB4_Sprite', 0xB4, 0xBDA8, 0xC700)    # objetos de sprite (explosiones sueltas): listas
bankbytes('kBank8CB', 0x8C)   # (bytes) intro: listas de los objetos de fondo, textos, paletas, niveles de los recuerdos
wordblob('kBank91_Demo', 0x91, 0x8400, 0x87A0)
arr('int16', 'kSuperMissileAccel', words(0x90C303, 40), fmt='{}')   # $90:C303 (misil) y C32B (super): x, y por dirección      # intro: entrada de demo de Samus (listas y objetos)
# intro: mapa del fondo de los recuerdos (DecompressToMem $97:88CC -> 7F:9000, copiado a 7E:3800) y los niveles de
# los recuerdos (Mother Brain y la larva): las palabras de $8C y, después, lo que hay en 7F:0002 en ese momento
# (los tiles descomprimidos de $95:F90E, CinematicFunction_Intro_Initial)
_bg = decompress(0x9788CC)
arr('uint16', 'kIntroBgTilemap', [_bg[2 * i] | _bg[2 * i + 1] << 8 for i in range(1024)], fmt='0x{:X}')
_t = decompress(0x95F90E) + bytes(0x1000)
def _intro_level(addr, nwords, total):
    v = words(addr, nwords)
    v += [_t[2 + 2 * i] | _t[3 + 2 * i] << 8 for i in range(nwords, total)]
    return v
arr('uint16', 'kIntroLevelMotherBrain', _intro_level(0x8CBEC3, 224, 256), fmt='0x{:X}')
arr('uint16', 'kIntroLevelBabyMetroid', _intro_level(0x8CC083, 384, 512), fmt='0x{:X}')
arr('uint16', 'kPalettes_Intro2', words(0x8CE5E9, 256), fmt='0x{:X}')        # $8C:E5E9
_, sc = c_array_from_source('sm_94.c', 'kSinCosTable8bit_Sext')
H.append('extern const int16 kSinCosTable8bit_Sext[%d];' % len(sc))
C.append('const int16 kSinCosTable8bit_Sext[%d] = { %s };' % (len(sc), ', '.join(str(v) for v in sc)))

_, rs = c_array_from_source('sm_a0.c', 'kRoomShakes')
arr('uint16', 'kRoomShakes', rs, fmt='0x{:X}')                                  # $A0:8687
arr('uint16', 'kScreenShakeOffsets', words(0x86846B, 72), fmt='0x{:X}')       # $86:846B (36 tipos x 2)

H.append('#endif')
out.mkdir(parents=True, exist_ok=True)
(out / 'tables.h').write_text('\n'.join(H) + '\n')
(out / 'tables.c').write_text('\n'.join(C) + '\n')
print('tablas:', len([l for l in H if l.startswith('extern')]))
