#!/usr/bin/env python3
"""MegaMetroid — las canciones del juego como proyectos de Furnace (.fur, Genesis: YM2612 + PSG) (docs/55).

Para elegir los instrumentos escuchando la canción entera: cada canción sale con sus notas, cuadro a cuadro, en los
mismos canales de la Genesis que la ROM (convert_track de mksnd.py, con la misma tabla tools/snd/instrumentos.txt), y
un instrumento por muestra del SNES ("03_25" = conjunto $03, muestra 25). En Furnace se cambian los instrumentos FM y
se oye el resultado en toda la canción; después fur2tabla.py los pasa a la tabla y `make snd` rehace la ROM.

Uso: mkfur.py sm.smc [carpeta] [CONJ PISTA ...]
  carpeta     donde se escriben (por omisión work/furnace)
  CONJ PISTA  solo esas (por omisión todas las de CANCIONES)

Qué hay en cada fila (una por cuadro del SNES: velocidad 1, 60 filas por segundo):
  - nota e instrumento cuando el SNES toca una nota; OFF cuando la suelta (o se la quita otra voz);
  - volumen: la envolvente y el volumen del SNES (FM: 127 - atenuación en pasos del TL, como el TL de los portadores
    de la ROM; PSG: 15 - atenuación);
  - E5xx: la altura fina (1/128 de semitono, 80 = la nota justa): el vibrato y los deslizamientos del SNES;
  - EAFF / EA00: nota ligada (cambio de altura de más de un semitono sin volver a tocar la nota);
  - 08xy: el paneo (FM); 2001 en el ruido (ruido blanco con las 3 velocidades fijas: C, C#, D);
  - 0D00 / 0Bxx / FF00 en FM 1: el lazo de la canción (o el final) como en la ROM.
Las pistas PCM (las voces de la intro) no van: no son música con instrumentos.

Se escribe el formato de Furnace 0.6.8.1 (versión 228, bloque INFO): lo abren esa versión y todas las siguientes.
"""
import math
import struct
import sys
import zlib
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import mksnd  # noqa: E402

# canciones del juego: (conjunto, pista, nombre). El conjunto es el de QueueMusic_Delayed8(0xFF00 | conjunto) (la
# tabla $8F:E7E1); las pistas 1-4 son las del motor (siempre cargadas), 5 en adelante las del conjunto.
# Las que se piden en el decomp llevan la función; las demás, el nombre del conjunto (las de las salas las pide la
# cabecera de cada sala). Las 1-4 son del motor: se graban con el conjunto $06 (sus muestras son las de base).
# Sin las voces (PCM): $3F y $42. Las que en el SPC son silencio no están ($21 pista 6, $33 pistas 8 y 9).
CANCIONES = [
    (0x06, 1, "Fanfarria de Samus"),                  # PlaySamusFanfare ($92:ED24)
    (0x06, 2, "Pista 2 del motor"),
    (0x06, 3, "Pista 3 del motor (después de un jefe)"),  # Draygon_Func_31, Kraid, Botwoon, VariaSuitPickup_6
    (0x06, 4, "Pista 4 del motor (Game Over)"),       # GameOverMenu_2_PlayMusic ($81:93E8)
    (0x03, 5, "Título"),                              # CinematicFunctionOpening ($8B:9B68)
    (0x03, 6, "Menú de partidas"),                    # HandleCinematicsTransitions_1 ($8B:9A83)
    (0x06, 5, "Crateria vacía, pista 5"),
    (0x06, 6, "Crateria vacía, pista 6"),
    (0x09, 5, "Crateria subterránea, pista 5"),
    (0x09, 6, "Crateria subterránea, pista 6"),
    (0x0C, 5, "Crateria superficie"),
    (0x0F, 5, "Brinstar verde"),
    (0x12, 5, "Brinstar rojo"),
    (0x15, 5, "Norfair superior"),
    (0x18, 5, "Norfair inferior"),
    (0x1B, 5, "Maridia, pista 5"),
    (0x1B, 6, "Maridia, pista 6"),
    (0x1E, 5, "Tourian"),
    (0x21, 5, "Mother Brain, segunda fase"),          # MotherBrainBody_FakeDeath_Ascent_12_StartMusic ($A9:8DEC)
    (0x24, 5, "Jefe 1, pelea (Ridley)"),              # CeresRidley_Func_5 ($A6:A3DF)
    (0x24, 6, "Jefe 1, pista 6"),
    (0x24, 7, "Huida"),                               # Ceres (Ridley_C09F) y Zebes (MotherBrain_Phase3_Death_16_StartEscape)
    (0x27, 5, "Jefe 2, pelea (Kraid, Phantoon, Crocomire)"),  # Kraid_Raise..., Phantoon_SpawnFireballs..., Crocomire
    (0x27, 6, "Jefe 2, pista 6"),
    (0x2A, 5, "Minijefe (Torizo)"),                   # Torizo_Instr_42 ($AA:CAE2)
    (0x2D, 5, "Vuelo a Ceres"),                       # CinematicFunction_Intro_Func54 ($8B:BCA0)
    (0x2D, 6, "Ceres"),
    (0x2D, 7, "Ceres explota y viaje a Zebes"),       # CinematicFunctionBlackoutFromCeres ($8B:C11B)
    (0x2D, 8, "Ceres explota con Samus dentro"),      # CinematicFunctionBlackoutFromCeres (kGameState_37)
    (0x30, 5, "Wrecked Ship, sin energía"),
    (0x30, 6, "Wrecked Ship, con energía"),
    (0x33, 5, "Zebes explota"),                       # CinematicFunctionEscapeFromCebes ($8B:D480)
    (0x36, 5, "Intro (diario y recuerdos)"),          # CinematicFunc_Intro_SetupTextPage2 ($8B:A66F)
    (0x39, 5, "Muerte de Samus"),                     # GameState_20_SamusNoHealth_BlackOut ($82:DCE0)
    (0x3C, 5, "Créditos"),                            # CinematicFunction_Intro_Func119 ($8B:DB9E)
    (0x45, 5, "Conjunto 45, pista 5"),
    (0x45, 6, "Conjunto 45, pista 6"),
    (0x48, 5, "Tema de Samus (la larva metroide)"),   # ShitroidInCutscene_PlaySamusTheme ($A9:CC60)
]

FUR_VERSION = 228
DT_REG2FUR = (3, 4, 5, 6, 7, 2, 1, 0)                # detune del registro -> el de Furnace (al revés que dtTable)
NOTE_A4 = 117                                      # nota de Furnace: 0 = C-(-5), A-4 = 117
ROWS = 256
NFX = 3                                            # columnas de efectos por canal
FLOW = 2                                           # columna de FM 1 para el lazo (0B/0D/FF)
CHANS = 10                                         # YM2612: FM 1-6; PSG: cuadrado 1-3, ruido
INS_STD, INS_FM = 0, 1


def chan_index(c):
    kind, i = c
    return i if kind == "fm" else 6 + i if kind == "psg" else 9


# ---------------------------------------------------------------- de la traza de convert_track a filas
def build_rows(trace, n, loop):
    """filas[canal][cuadro] = dict(note, ins, vol, fx=[(efecto, valor)]) e instrumentos usados (clave -> índice)"""
    rows = [[None] * n for _ in range(CHANS)]
    by = {}
    for t in trace:
        by.setdefault((t[0], chan_index(t[1])), []).append(t)
    ins_keys = {}                                  # (srcn, tipo de canal) -> índice de Furnace
    order = []

    def ins_of(ins, kind):
        key = (ins.srcn, "fm" if kind == "fm" else "psg" if kind == "psg" else "ruido")
        if key not in ins_keys:
            ins_keys[key] = len(order)
            order.append((key, ins))
        return ins_keys[key]

    state = [dict() for _ in range(CHANS)]
    for f in range(n):
        force = loop is not None and f == loop[0]  # al volver al lazo todo se escribe de nuevo
        for ch in range(CHANS):
            ev = by.get((f, ch))
            S = state[ch]
            if force:
                for k in ("vol", "fine", "pan", "ins"):
                    S.pop(k, None)
            if not ev:
                continue
            r = dict(note=None, ins=None, vol=None, fx=[])
            last = ev[-1]
            if last[2] is None:                     # se soltó
                if S.get("on"):
                    r["note"] = 180
                    S["on"] = False
                rows[ch][f] = r
                continue
            _, c, ins, hz, att, pan, kon, rate = last
            kon = kon or any(e[2] is None for e in ev) or not S.get("on")
            kind = c[0]
            idx = ins_of(ins, kind)
            if kind == "ruido":
                note, fine = 108 + (2 - rate), 0    # C-4: velocidad 2, C#-4: 1, D-4: 0 (sms.cpp: 2 - nota % 12)
            else:
                semis = 12 * math.log2(hz / 440.0) if hz > 0 else -48
                if kon:
                    note = max(0, min(179, NOTE_A4 + int(round(semis))))
                else:
                    note = S["note"]
                fine = int(round((semis - (note - NOTE_A4)) * 128))
            if kon:
                r["note"] = note
                if S.get("legato"):
                    r["fx"].append((0xEA, 0x00))
                    S["legato"] = False
                S["on"], S["note"] = True, note
                S.pop("fine", None)
            elif abs(fine) > 127 or (kind == "ruido" and note != S["note"]):  # más de un semitono: nota ligada
                if kind != "ruido":
                    semis_note = NOTE_A4 + int(round(semis))
                    note = max(0, min(179, semis_note))
                    fine = int(round((semis - (note - NOTE_A4)) * 128))
                r["note"] = note
                S["note"] = note
                if not S.get("legato"):
                    r["fx"].append((0xEA, 0xFF))
                    S["legato"] = True
            if kon or S.get("ins") != idx:
                r["ins"] = idx
                S["ins"] = idx
            vol = max(0, min(127, 127 - att)) if kind == "fm" else max(0, min(15, 15 - att))
            if kon or S.get("vol") != vol:
                r["vol"] = vol
                S["vol"] = vol
            fine = max(-127, min(127, fine))
            if kind != "ruido" and S.get("fine") != fine:
                r["fx"].append((0xE5, 0x80 + fine if fine < 128 else 0xFF))
                S["fine"] = fine
            if kind == "fm":
                pv = {0xC0: 0xFF, 0x80: 0xF0, 0x40: 0x0F}[pan]
                if S.get("pan") != pv:
                    r["fx"].append((0x08, pv))
                    S["pan"] = pv
            rows[ch][f] = r
    return rows, order


def split_orders(n, loop):
    """trozos (desde, hasta) de a lo sumo ROWS filas; la parte del lazo empieza en un trozo propio"""
    s = loop[0] if loop else n
    out = []
    for a, b in ((0, s), (s, n)):
        for x in range(a, b, ROWS):
            out.append((x, min(b, x + ROWS)))
    return out


# ---------------------------------------------------------------- escritura del .fur
def cstr(s):
    return s.encode("utf-8") + b"\0"


def ins_block(name, kind, ins):
    """bloque INS2 (papers/newIns.md): nombre y, si es FM, el parche de la tabla"""
    feat = b"NA" + struct.pack("<H", len(cstr(name))) + cstr(name)
    if kind == INS_FM:
        alg, fb = ins.alg & 7, ins.fb & 7
        b = bytes([0xF0 | 4, (alg << 4) | fb, 0, 32, 0])     # 4 operadores encendidos; alg/fb; sin LFO; ops=4; bloque 0
        for o in ins.ops:                                     # el orden de la tabla es el de Furnace (S1, S3, S2, S4)
            dt, mul, tl, ks, ar, d1r, sl, d2r, rr = o
            b += bytes([(DT_REG2FUR[dt & 7] << 4) | (mul & 15), tl & 127, ((ks & 3) << 6) | (ar & 31), d1r & 31,
                        (2 << 5) | (d2r & 31), ((sl & 15) << 4) | (rr & 15), 0, 0])   # kvs 2: el volumen solo en los portadores
        feat += b"FM" + struct.pack("<H", len(b)) + b
    feat += b"EN"
    body = struct.pack("<HH", FUR_VERSION, kind) + feat
    return b"INS2" + struct.pack("<I", len(body)) + body


def pat_block(ch, index, rows):
    d = bytearray()
    skip = 0

    def flush():
        nonlocal skip
        while skip >= 2:
            k = min(skip, 128)                      # 0x80 | 127 sería 0xFF: el fin del patrón
            d.append(0x80 | (k - 2))
            skip -= k
        if skip == 1:
            d.append(0)
            skip = 0

    for r in rows:
        if r is None or (r["note"] is None and r["ins"] is None and r["vol"] is None and not r["fx"]):
            skip += 1
            continue
        flush()
        mask, em, data = 0, 0, bytearray()
        if r["note"] is not None:
            mask |= 1
            data.append(r["note"])
        if r["ins"] is not None:
            mask |= 2
            data.append(r["ins"])
        if r["vol"] is not None:
            mask |= 4
            data.append(r["vol"])
        fx = r["fx"][:NFX]
        if fx:
            mask |= 32
            for k, (e, v) in enumerate(fx):
                em |= 3 << (2 * k)
                data += bytes([e, v])
        d.append(mask)
        if fx:
            d.append(em)
        d += data
    d.append(0xFF)
    body = struct.pack("<BBH", 0, ch, index) + cstr("") + bytes(d)
    return b"PATN" + struct.pack("<I", len(body)) + body


def write_fur(path, title, comment, rows, order_list, chunks, loop):
    """el .fur (comprimido con zlib, como lo guarda Furnace)"""
    nord = len(chunks)
    # filas de cada trozo, con el lazo en FM 1
    loop_ord = next((k for k, (a, b) in enumerate(chunks) if loop and a == loop[0]), None)
    pats = []
    for k, (a, b) in enumerate(chunks):
        for ch in range(CHANS):
            rr = [dict(r, fx=list(r["fx"])) if r else None for r in rows[ch][a:b]]
            if ch == 0:
                last = rr[-1] or dict(note=None, ins=None, vol=None, fx=[])
                fx = (last["fx"] + [None] * NFX)[:NFX]
                if k == nord - 1:
                    fx[FLOW] = (0x0B, loop_ord) if loop_ord is not None else (0xFF, 0x00)
                elif b - a < ROWS:
                    fx[FLOW] = (0x0D, 0x00)
                last["fx"] = [x if x else (0, 0) for x in fx] if fx[FLOW] else last["fx"]
                rr[-1] = last
            if ch == 9 and k == 0:                  # el ruido: blanco, las 3 velocidades fijas
                first = rr[0] or dict(note=None, ins=None, vol=None, fx=[])
                first["fx"] = [(0x20, 0x01)] + first["fx"][:NFX - 1]
                rr[0] = first
            pats.append((ch, k, rr))

    ins_blocks = []
    for (key, ins) in order_list:
        srcn, kind = key
        name = f"{ins.song:02X}_{srcn}" + ("" if kind == "fm" else f" {kind}")
        ins_blocks.append(ins_block(name, INS_FM if kind == "fm" else INS_STD, ins))

    info = bytearray()
    info += struct.pack("<BBBBf", 0, 1, 1, 1, 60.0)         # base de tiempo, velocidad 1 y 2, arpegio, 60 Hz
    info += struct.pack("<HHBB", ROWS, nord, 16, 64)
    info += struct.pack("<HHHI", len(ins_blocks), 0, 0, len(pats))
    info += bytes([0x83, 0x03] + [0] * 30)                   # YM2612 y PSG
    info += bytes([64, 32] + [0] * 30)                        # volúmenes (viejos)
    info += bytes(32)                                         # paneos (viejos)
    info += bytes(128)                                        # punteros a las opciones de cada chip: ninguna
    info += cstr(title) + cstr("MegaMetroid (mkfur.py, desde tu ROM)")
    info += struct.pack("<f", 440.0)
    # limit slides, linear pitch (2: completo), loop modality, proper noise, wave duty, reset macro porta, legacy vol
    # slides, compat arp, note off resets slides, target resets slides, arp non porta, alg macro, broken shortcut,
    # ignore dup slides, stop porta on off, continuous vib, broken DAC, one tick cut, new ins porta, arp0 reset
    info += bytes([0, 2, 2, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1])
    ptr_at = len(info)
    info += bytes(4 * (len(ins_blocks) + len(pats)))         # punteros (después)
    for ch in range(CHANS):                                   # órdenes: el trozo k usa el patrón k en cada canal
        info += bytes(range(nord))
    info += bytes([NFX] * CHANS)
    info += bytes([3] * CHANS) + bytes(CHANS)                # visibles (y en el osciloscopio), sin plegar
    info += b"".join(cstr("") for _ in range(2 * CHANS))
    info += cstr(comment)
    info += struct.pack("<f", 1.0)
    # extended compat flags (>=70): broken speed sel ... old arp strategy (29)
    info += bytes([0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 4, 0, 0, 1, 1, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0])
    info += struct.pack("<HH", 1, 1)                          # tempo virtual
    info += cstr(title) + cstr("") + bytes([0, 0, 0, 0])     # primera subcanción, sin más subcanciones
    info += cstr("Sega Genesis") + cstr("Super Metroid") + cstr("") * 4
    info += struct.pack("<fff", 1.0, 0.0, 0.0) + struct.pack("<fff", 0.5, 0.0, 0.0)
    info += struct.pack("<I", 0) + bytes([1])                 # patchbay automática
    info += bytes([0, 0, 0, 0, 0, 0, 0, 0])                   # compat (>=138 ... >=200)
    info += bytes([1, 1] + [0] * 15) + bytes([0])             # velocidad: 1 valor (1); sin grooves
    adir_at = len(info)
    info += bytes(12)                                         # punteros a las carpetas (después)

    head = 32
    info_blk = b"INFO" + struct.pack("<I", len(info)) + bytes(info)
    blobs = [b"ADIR" + struct.pack("<I", 4) + struct.pack("<I", 0) for _ in range(3)]
    pos = head + len(info_blk)
    adir_ptrs = []
    for blob in blobs:
        adir_ptrs.append(pos)
        pos += len(blob)
    ins_ptrs = []
    for blob in ins_blocks:
        ins_ptrs.append(pos)
        pos += len(blob)
    pat_blobs = [pat_block(ch, k, rr) for ch, k, rr in pats]
    pat_ptrs = []
    for blob in pat_blobs:
        pat_ptrs.append(pos)
        pos += len(blob)
    info[ptr_at:ptr_at + 4 * (len(ins_ptrs) + len(pat_ptrs))] = struct.pack(f"<{len(ins_ptrs) + len(pat_ptrs)}I",
                                                                           *ins_ptrs, *pat_ptrs)
    info[adir_at:adir_at + 12] = struct.pack("<3I", *adir_ptrs)
    info_blk = b"INFO" + struct.pack("<I", len(info)) + bytes(info)
    header = b"-Furnace module-" + struct.pack("<HHI", FUR_VERSION, 0, head) + bytes(8)
    raw = header + info_blk + b"".join(blobs) + b"".join(ins_blocks) + b"".join(pat_blobs)
    assert len(raw) == pos
    Path(path).write_bytes(zlib.compress(raw, 9))
    return raw


# ---------------------------------------------------------------- todo junto
def convert(spc, table, song, track, name, out):
    st_frames = mksnd.MAX_FRAMES
    a, st = mksnd.run_track(spc, song, track, st_frames, out.parent / f"_ram{song:02X}.bin")
    loop = mksnd.find_loop(st)
    play = np.nonzero(st[:, 98])[0]
    end = loop[1] if loop else (int(play[-1]) + 2 if len(play) and play[-1] < len(st) - 2 else len(st))
    if not len(play) or end < 4:
        return None
    ram = (out.parent / f"_ram{song:02X}.bin").read_bytes()
    v = st[:end, :96].reshape(-1, 8, 12)
    used = sorted(set(v[:, :, 0][v[:, :, 9] > 0].tolist()))
    instrs = {}
    for s in used:
        ins = mksnd.make_instr(ram, song, s, table)
        ins.srcn, ins.song = s, song
        instrs[s] = ins
    trace = []
    mksnd.convert_track(st, instrs, loop, end, mksnd.MIX, trace)
    rows, order = build_rows(trace, end, loop)
    if not order:                                  # silencio
        return None
    # los instrumentos por nombre (CONJ_MUESTRA), no por orden de aparición
    srt = sorted(range(len(order)), key=lambda k: (order[k][0][1] != "fm", order[k][0][0], order[k][0][1]))
    new = {old: k for k, old in enumerate(srt)}
    order = [order[k] for k in srt]
    for ch in rows:
        for r in ch:
            if r and r["ins"] is not None:
                r["ins"] = new[r["ins"]]
    chunks = split_orders(end, loop)
    if len(chunks) > 256:
        raise SystemExit(f"{song:#04x}/{track}: {end} cuadros no entran en 256 órdenes")
    t = f"{song:02X}_{track} {name}"
    seg = f"lazo desde el cuadro {loop[0]}" if loop else "sin lazo"
    comment = (f"Super Metroid, conjunto ${song:02X}, pista {track}: {name}. {end} cuadros (60 por segundo), {seg}.\n"
               "Generado por tools/snd/mkfur.py desde tu ROM: no se distribuye. Cambia los instrumentos FM (los "
               "nombres son CONJ_MUESTRA) y pásalos a la tabla con tools/snd/fur2tabla.py (docs/55).")
    import unicodedata
    plain = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode().lower()
    safe = "_".join("".join(ch if ch.isalnum() else " " for ch in plain).split())
    path = out / f"{song:02X}_{track}_{safe}.fur"
    write_fur(path, t, comment, rows, order, chunks, loop)
    # cada muestra del SNES sola (como mksnd.py --analizar), para imitarla en Furnace
    for s in used:
        wav = out / "muestras" / f"{song:02X}_{s}.wav"
        if not wav.exists():
            x, l = mksnd.brr.decode(ram, s)
            mksnd.write_sample(wav, x, l)
    convert.last = (trace, end, loop, order)
    return path, end, loop, len(order)


def main(argv):
    if len(argv) < 1 or argv[0] in ("-h", "--help"):
        print(__doc__)
        return
    def is_num(x):
        try:
            int(x, 0)
            return True
        except ValueError:
            return False
    rom, rest = argv[0], argv[1:]
    out = ROOT / "work" / "furnace"
    if rest and not is_num(rest[0]):
        out, rest = Path(rest[0]), rest[1:]
    want = None
    if rest:
        want = {(int(rest[i], 0), int(rest[i + 1], 0)) for i in range(0, len(rest) - 1, 2)}
    out.mkdir(parents=True, exist_ok=True)
    spc = mksnd.Spc(rom, ROOT / "work" / "snd")
    table = mksnd.read_instr_table(HERE / "instrumentos.txt")
    seen = {}                                      # notas de cada canción: las repetidas no se escriben dos veces
    lista = ["# Instrumentos de cada canción (mkfur.py): nombre en Furnace, tipo, notas que toca y su muestra del SNES",
             "# sola en muestras/CONJ_MUESTRA.wav. Los FM son los que se cambian; psg y ruido salen del PSG.", ""]
    for song, track, name in CANCIONES:
        if want and (song, track) not in want:
            continue
        r = convert(spc, table, song, track, name, out)
        if r is None:
            print(f"{song:#04x}/{track} {name}: no suena")
            continue
        path, end, loop, nins = r
        trace = convert.last[0]
        sig = hash(tuple((t[0], t[1], t[2] and t[2].srcn, round(float(t[3]), 1), t[4], t[6]) for t in trace))
        if sig in seen:
            path.unlink()
            print(f"{song:#04x}/{track} {name}: igual que {seen[sig]}, no se escribe")
            continue
        seen[sig] = path.name
        print(f"{path.name}: {end} cuadros, lazo {loop}, {nins} instrumentos")
        notes = {}
        for t in trace:
            if t[2] is not None and t[6]:
                k = (t[2].srcn, "fm" if t[1][0] == "fm" else t[1][0])
                notes[k] = notes.get(k, 0) + 1
        lista.append(path.name)
        for (srcn, kind), ins in convert.last[3]:
            nm = f"{song:02X}_{srcn}" + ("" if kind == "fm" else f" {kind}")
            nn = notes.get((srcn, kind), 0)
            cnt = f"{nn:5d} notas" if nn else "  (notas que pasan aquí cuando faltan canales FM)"
            lista.append(f"  {nm:12s} {kind:5s} {cnt}   muestras/{song:02X}_{srcn}.wav")
        lista.append("")
    if not want:
        (out / "instrumentos_por_cancion.txt").write_text("\n".join(lista), encoding="utf-8")


if __name__ == "__main__":
    main(sys.argv[1:])
