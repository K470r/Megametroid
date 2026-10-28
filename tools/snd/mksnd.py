#!/usr/bin/env python3
"""MegaMetroid — el sonido del cartucho desde tu ROM (docs/55).

Corre el motor de sonido del SNES (N-SPC, sm/src/spc_player.c del decomp: tools/snd/spcrun.c) en el PC y arma:
  - los efectos de sonido como PCM de 8 bits a 13 300 Hz (el controlador XGM2 de SGDK), con su lazo si no terminan;
  - la música: cada voz del SPC pasada a FM/PSG con la tabla de instrumentos (tools/snd/instrumentos.txt), en VGM, y
    de ahí a XGM2 con xgm2tool; las pistas marcadas "pcm" (las voces de la intro) como PCM.
Escribe res/gen/md/md_snd_data.s y md_snd_data.h. Nada de lo que escribe va a git (docs/43).

Uso: mksnd.py sm.smc salida_md [trabajo]"""
import os
import re
import subprocess
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import brr                                          # noqa: E402
import sfxlist                                      # noqa: E402

RATE_IN = 32040                                     # 534 muestras por cuadro (RtlRenderAudio)
RATE_OUT = 13300                                    # XGM2: PCM a ~13,3 kHz
SMSRC = Path(os.environ.get("SMSRC", ROOT / "sm" / "src"))
GDK = Path(os.environ.get("GDK", ROOT / "third_party" / "SGDK"))


def log(*a):
    print(*a, file=sys.stderr)


class Spc:
    """spcrun: compila el N-SPC del decomp y lo corre con un guion"""

    def __init__(self, rom, work):
        self.rom, self.work = rom, Path(work)
        self.work.mkdir(parents=True, exist_ok=True)
        self.exe = self.work / "spcrun"
        src = HERE / "spcrun.c"
        if not self.exe.exists() or self.exe.stat().st_mtime < src.stat().st_mtime:
            subprocess.run(["gcc", "-O2", "-w", "-iquote", str(SMSRC), "-iquote", str(SMSRC / "snes"), "-o",
                            str(self.exe), str(src), str(SMSRC / "snes" / "dsp.c"), "-lm"], check=True)
        self.n = 0

    def run(self, script, want_states=False, want_sfxst=False):
        """devuelve (audio estéreo float [n,2], estados [cuadros,104] o None, sfxst o None)"""
        self.n += 1
        g, wav, st, sx = (self.work / f"_{self.n % 4}{e}" for e in (".txt", ".wav", ".st", ".sx"))
        if want_sfxst:
            script = f"sfxst {sx}\n" + script
        g.write_text(script)
        subprocess.run([str(self.exe), str(self.rom), str(g), str(wav), str(st) if want_states else "-"], check=True)
        a = np.frombuffer(wav.read_bytes()[44:], dtype=np.int16).reshape(-1, 2).astype(np.float64)
        s = np.fromfile(st, dtype=np.uint8).reshape(-1, 104) if want_states else None
        x = None
        if want_sfxst:
            raw = sx.read_bytes()
            rec = 4 + 88
            x = [(int.from_bytes(raw[i:i + 4], "little"), raw[i + 4:i + rec]) for i in range(0, len(raw), rec)]
        return a, s, x


# ---------------------------------------------------------------- PCM
def lowpass_resample(x, rin, rout, n_out=None):
    """remuestreo con un filtro pasabajos (sinc con ventana de Kaiser) e interpolación lineal"""
    fc = 0.45 * rout / rin
    taps = 129
    t = np.arange(taps) - (taps - 1) / 2
    h = 2 * fc * np.sinc(2 * fc * t) * np.kaiser(taps, 8.0)
    h /= h.sum()
    y = np.convolve(x, h, mode="same")
    if n_out is None:
        n_out = int(round(len(x) * rout / rin))
    pos = np.arange(n_out) * (len(x) / n_out)
    return np.interp(pos, np.arange(len(x)), y)


def to_s8(x, gain):
    return np.clip(np.round(x * gain / 256.0), -127, 127).astype(np.int8)


def pad64(b, fill=0):
    """XGM2: largo múltiplo de 64 (el controlador cuenta de a 64) y la siguiente en un múltiplo de 256"""
    n = (-len(b)) % 64
    return b + bytes([fill & 0xFF]) * n


# ---------------------------------------------------------------- efectos
def sfx_conf():
    """kSfxNConf del decomp: con qué prioridad queda cada efecto (Sfx1/2/3_HandleCmdFromSnes)"""
    s = (SMSRC / "spc_player.c").read_text()
    out = {}
    for n in (1, 2, 3):
        body = re.search(r"kSfx%dConf\[\] = \{(.*?)\};" % n, s, re.S).group(1)
        out[n] = [int(v) for v in re.findall(r"\d+", body)]
    return out


def sfx_flags(lib, i, conf):
    """bit 0: deja la prioridad puesta; bit 1 (biblioteca 3): some_value = 2 (solo el 1 la corta);
    bit 2 (biblioteca 3): some_value = 0; bit 3: suena en Zebes (Samus y puertas); bit 4: a media velocidad"""
    c = conf[lib][i - 1] if i - 1 < len(conf[lib]) else 0
    if lib in (1, 2):
        return 1 if c in (1, 3) else 0
    f = {0: 1 | 4, 1: 2, 2: 0, 3: 0, 4: 1, 5: 1}.get(c, 0)
    return f


def is_dull(x):
    """casi sin agudos (menos del 2 % de la energía arriba de 3 kHz): alcanza con media velocidad (6650 Hz)"""
    if len(x) < 256:
        return False
    sp = np.abs(np.fft.rfft(x - x.mean())) ** 2
    fr = np.fft.rfftfreq(len(x), 1 / RATE_IN)
    tot = sp.sum()
    return tot > 0 and sp[fr > 3000].sum() / tot < 0.02


def render_sfx(spc, lib, i, song, gain):
    """graba un efecto; devuelve (pcm s8 sin lazo, pcm s8 del lazo o b'', cuadros de la parte sin lazo, media)"""
    up = f"up {song}\n" if song else ""
    frames = 1300
    a, st, sx = spc.run(f"noecho\nf 1\n{up}f 2\np {lib} {i}\nf {frames}\n", want_states=True, want_sfxst=True)
    cur = st[:, 96 + 2 + lib]
    on = np.nonzero(cur)[0]
    mono = a.mean(axis=1)
    if not len(on):
        return b"", b"", 0, False                      # no suena (los de silencio: 1/2, 0x71...)
    start_frame = on[0]
    looped = on[-1] >= frames - 2
    chans = {1: range(0, 4), 2: range(4, 6), 3: range(6, 8)}[lib]
    loop = None
    if looped:                                         # el lazo (0xFB): el primer estado que se repite
        ticks = [(at, raw) for at, raw in sx if at >= start_frame * 534]
        per = []                                       # (inicio, período) de cada canal, en ticks de efectos
        for c in chans:
            seen = {}
            for k, (at, raw) in enumerate(ticks):
                key = raw[11 * c:11 * c + 11]
                if key in seen:
                    if raw[11 * c + 10] != 0xFF:       # (un canal apagado no cuenta)
                        per.append((seen[key], k - seen[key]))
                    break
                seen[key] = k
        if per:
            k0 = max(p[0] for p in per)
            P = int(np.lcm.reduce([p[1] for p in per]))
            if P * 512 > 3 * RATE_IN:                  # canales con lazos de distinto largo: el del más largo
                k0, P = max(per, key=lambda p: p[1])
                log(f"  efecto {lib}/{i:#x}: lazos de {sorted(set(p[1] for p in per))} ticks; se toma {P}")
            if k0 + P < len(ticks):
                loop = (ticks[k0][0], ticks[k0 + P][0])
        if loop is None:
            log(f"  efecto {lib}/{i:#x}: no encuentro el lazo")
            looped = False
    if looped:
        s0, s1 = loop
        x0 = mono[:s0]
        nz = np.nonzero(np.abs(x0) > 8)[0]
        b0 = nz[0] if len(nz) else s0
        intro = x0[b0:]
        body = mono[s0:s1]
        half = is_dull(mono[b0:s1])
        ro = RATE_OUT // 2 if half else RATE_OUT
        n_body = max(64, int(round(len(body) * ro / RATE_IN / 64)) * 64)
        body8 = to_s8(lowpass_resample(np.tile(body, 3), RATE_IN, ro, 3 * n_body)[n_body:2 * n_body], gain)
        intro8 = to_s8(lowpass_resample(intro, RATE_IN, ro), gain) if len(intro) > 64 else np.zeros(0, np.int8)
        n_intro = int(np.ceil(len(intro8) / 64)) * 64
        intro_b = intro8.tobytes() + bytes(n_intro - len(intro8))
        return intro_b, body8.tobytes(), int(np.ceil(n_intro / ro * 60)), half
    end = (on[-1] + 2) * 534
    x = mono[:end]
    peak = np.abs(x).max()
    # bordes: lo que queda 40 dB debajo del pico (o debajo de lo que se oye en 8 bits) no se guarda
    thr = max(peak * 0.01, 1.5 * 256 / gain)
    nz = np.nonzero(np.abs(x) > thr)[0]
    if not len(nz):
        return b"", b"", 0, False
    x = x[nz[0]:nz[-1] + 1]
    fade = min(len(x), int(0.01 * RATE_IN))            # sin chasquido al cortar
    x[len(x) - fade:] *= np.linspace(1, 0, fade)
    half = is_dull(x)
    ro = RATE_OUT // 2 if half else RATE_OUT
    y = to_s8(lowpass_resample(x, RATE_IN, ro), gain)
    b = pad64(y.tobytes())
    return b, b"", int(np.ceil(len(b) / ro * 60)), half


def read_list(path):
    out = []
    for line in Path(path).read_text().splitlines():
        line = line.split("#")[0].split()
        if line:
            out.append((int(line[0], 0), int(line[1], 0), int(line[2], 0) if len(line) > 2 else 0))
    return out


# ---------------------------------------------------------------- música: instrumentos
YM_CLOCK, PSG_CLOCK = 7670453, 3579545
YM_RATE = YM_CLOCK / 144.0                          # 53 267 Hz


class Instr:
    """cómo suena en la Genesis un instrumento (muestra BRR) del SNES"""

    def __init__(self, kind, f0=None, ops=None, alg=7, fb=0, oct=0, vol=0.0, noise=0):
        self.kind = kind                            # "fm", "psg", "ruido", "no"
        self.f0, self.alg, self.fb, self.oct, self.vol, self.noise = f0, alg, fb, oct, vol, noise
        # ops: 4 operadores (orden de los registros: S1, S3, S2, S4) (dt, mul, tl, ks, ar, d1r, sl, d2r, rr)
        self.ops = ops

    def carriers(self):
        return {7: [0, 1, 2, 3], 6: [1, 2, 3], 5: [1, 2, 3], 4: [2, 3]}.get(self.alg, [3])

    def regs(self):
        """los 28 bytes de los operadores + B0 (algoritmo, realimentación), en el orden del XGM2 ($2x)"""
        out = []
        for base in (0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90):
            for o in self.ops:
                dt, mul, tl, ks, ar, d1r, sl, d2r, rr = o
                out.append({0x30: (dt << 4) | mul, 0x40: tl, 0x50: (ks << 6) | ar, 0x60: d1r, 0x70: d2r,
                            0x80: (sl << 4) | rr, 0x90: 0}[base])
        return out


def auto_instr(x, loopidx, f0=None):
    """FM aditiva: las 4 armónicas más fuertes de la muestra, una por operador (algoritmo 7, todos portadores)"""
    f0 = f0 or brr.fundamental(x, loopidx)
    if f0 is None:
        return Instr("ruido")
    h = brr.harmonics(x, loopidx, f0, n=15)
    ks = [k for k in np.argsort(-h)[:4] if h[k] > 0.05]
    ops = []
    for k in sorted(ks):
        tl = int(round(-20 * np.log10(h[k]) / 0.75))
        ops.append((0, k + 1, min(127, tl), 0, 31, 0, 0, 0, 15))
    while len(ops) < 4:
        ops.append((0, 1, 127, 0, 31, 0, 0, 0, 15))   # apagado
    return Instr("fm", f0=f0, ops=ops)


def read_instr_table(path):
    """tools/snd/instrumentos.txt: CONJ SRCN clave=valor..."""
    tab = {}
    for line in Path(path).read_text().splitlines():
        line = line.split("#")[0].split()
        if len(line) < 2:
            continue
        key = (int(line[0], 0) if line[0] != "*" else None, int(line[1], 0))
        kv = dict(t.split("=", 1) for t in line[2:])
        tab[key] = kv
    return tab


def make_instr(ram, song, srcn, table):
    x, loopidx = brr.decode(ram, srcn)
    kv = dict(table.get((None, srcn), {}))
    kv.update(table.get((song, srcn), {}))
    tipo = kv.get("tipo", "auto")
    f0 = float(kv["f0"]) if "f0" in kv else None   # (las armónicas se miden con esta altura)
    if tipo == "auto":
        ins = auto_instr(x, loopidx, f0)
    elif tipo == "fm":
        ins = auto_instr(x, loopidx, f0)
        if ins.f0 is None:
            ins = Instr("fm", f0=float(kv.get("f0", 440)))
        if "parche" in kv:                          # alg,fb,op1(dt:mul:tl:ks:ar:d1r:sl:d2r:rr),op2,op3,op4
            p = kv["parche"].split(",")
            ins.alg, ins.fb = int(p[0]), int(p[1])
            ins.ops = [tuple(int(v) for v in o.split(":")) for o in p[2:6]]
    elif tipo == "psg":
        ins = auto_instr(x, loopidx, f0)
        ins.kind = "psg" if ins.f0 else "ruido"
    elif tipo == "ruido":
        ins = Instr("ruido")
    else:
        ins = Instr("no")
    if "f0" in kv:
        ins.f0 = float(kv["f0"])
    ins.oct = int(kv.get("oct", 0))
    ins.vol = float(kv.get("vol", 0))
    ins.noise = int(kv.get("ruido", 0))
    return ins


# ---------------------------------------------------------------- música: del SPC a la Genesis
def find_loop(st):
    """(inicio, fin) del lazo en cuadros, o None si la pista termina. 1) la lista de patrones vuelve atrás
    (music_ptr_toplevel); 2) si no cambia (un patrón que se repite), el período de las notas"""
    tp = st[:, 96].astype(int) | st[:, 97].astype(int) << 8
    play = st[:, 98]
    first = {}
    for f in range(1, len(tp)):
        if not play[f]:
            continue
        t = tp[f]
        if t not in first:
            first[t] = f
        elif t != tp[f - 1] and first[t] > 0:
            return int(first[t]), int(f)
    on = np.nonzero(play)[0]
    if len(on) and on[-1] < len(tp) - 2:
        return None                                  # terminó
    v = st[:, :96].reshape(-1, 8, 12)
    sig = (v[:, :, 9] > 0).astype(np.int64) * (1 + v[:, :, 0].astype(np.int64) + 256 * v[:, :, 2].astype(np.int64))
    kon_f = np.nonzero(sig.any(axis=1))[0]
    if not len(kon_f):
        return None
    s = kon_f[0]
    n = len(sig)
    for P in range(30, (n - s) // 2):
        a, b = sig[s:n - P], sig[s + P:]
        same = (a == b).all(axis=1)
        # el tick del SPC no cae justo en el cuadro: se acepta una diferencia de un cuadro
        if same.mean() > 0.995:
            return int(s), int(s + P)
    return None


def ym_freq(f):
    """(bloque, fnum) del YM2612 para f Hz"""
    if f <= 0:
        return 0, 0
    for blk in range(8):
        fn = f * (1 << 20) / (YM_RATE * (1 << max(0, blk - 1))) if blk else f * (1 << 21) / YM_RATE
        if fn < 2048:
            return blk, max(1, int(round(fn)))
    return 7, 2047


class Vgm:
    def __init__(self):
        self.d = bytearray()
        self.samples = 0
        self.loop_off = None
        self.loop_samples = 0

    def ym(self, port, reg, val):
        self.d += bytes([0x52 + port, reg & 0xFF, val & 0xFF])

    def psg(self, val):
        self.d += bytes([0x50, val & 0xFF])

    def wait(self):
        self.d += b"\x62"
        self.samples += 735

    def mark_loop(self):
        self.loop_off = len(self.d)
        self.loop_at = self.samples

    def bytes(self):
        body = bytes(self.d) + b"\x66"
        h = bytearray(0x100)
        h[0:4] = b"Vgm "
        h[0x08:0x0C] = (0x170).to_bytes(4, "little")
        h[0x0C:0x10] = PSG_CLOCK.to_bytes(4, "little")
        h[0x18:0x1C] = self.samples.to_bytes(4, "little")
        if self.loop_off is not None:
            h[0x1C:0x20] = (0x100 + self.loop_off - 0x1C).to_bytes(4, "little")
            h[0x20:0x24] = (self.samples - self.loop_at).to_bytes(4, "little")
        h[0x24:0x28] = (60).to_bytes(4, "little")
        h[0x28:0x2A] = (0x0009).to_bytes(2, "little")
        h[0x2A] = 16
        h[0x2C:0x30] = YM_CLOCK.to_bytes(4, "little")
        h[0x34:0x38] = (0x100 - 0x34).to_bytes(4, "little")
        out = bytes(h) + body
        out = out[:4] + (len(out) - 4).to_bytes(4, "little") + out[8:]
        return out


def convert_track(st, instrs, loop, end, mix):
    """estados del SPC por cuadro -> VGM (YM2612: canales 0-4; PSG: 0-2 tono, 3 ruido)"""
    v = st[:, :96].reshape(-1, 8, 12)
    n = len(v) if end is None else end
    srcn = v[:, :, 0]
    pitch = v[:, :, 1].astype(np.int64) | v[:, :, 2].astype(np.int64) << 8
    volL = v[:, :, 3].astype(np.int8).astype(np.float64)
    volR = v[:, :, 4].astype(np.int8).astype(np.float64)
    env = (v[:, :, 5].astype(np.int64) | v[:, :, 6].astype(np.int64) << 8).astype(np.float64)
    envmax = (v[:, :, 7].astype(np.int64) | v[:, :, 8].astype(np.int64) << 8).astype(np.float64)
    kon = v[:, :, 9] > 0
    noise_dsp = v[:, :, 11] > 0
    vg = Vgm()
    # canales de la Genesis: ("fm", 0..4), ("psg", 0..2), ("ruido", 3)
    chans = [("fm", i) for i in range(5)] + [("psg", i) for i in range(3)] + [("ruido", 3)]
    owner = {c: None for c in chans}                  # voz del SNES que lo usa
    patch = {c: None for c in chans}
    last = {c: {} for c in chans}
    vchan = [None] * 8                                # canal de cada voz del SNES
    vlast = [None] * 8                                # el último canal que usó (para volver al mismo)
    master = mix.get("musica_db", 0.0)
    # inicio: LFO apagado, canal 3 normal, todo en silencio
    vg.ym(0, 0x22, 0)
    vg.ym(0, 0x27, 0)
    for c in range(5):
        port, ch = (0, c) if c < 3 else (1, c - 3)
        vg.ym(0, 0x28, (port << 2) | ch)
        for r in (0x40, 0x44, 0x48, 0x4C):
            vg.ym(port, r + ch, 127)
    for c in range(4):
        vg.psg(0x9F | (c << 5))

    def cls(ins, voice, f):
        if ins.kind == "ruido" or noise_dsp[f, voice]:
            return "ruido"
        return ins.kind

    def amp(voice, f):
        e = max(env[f, voice], envmax[f, voice] if kon[f, voice] else 0)
        return e / 2047.0 * max(abs(volL[f, voice]), abs(volR[f, voice])) / 127.0

    def release(c):
        kind, i = c
        if kind == "fm":
            port, ch = (0, i) if i < 3 else (1, i - 3)
            vg.ym(0, 0x28, (port << 2) | ch)
        else:
            vg.psg(0x9F | (i << 5))
        last[c].pop("att", None)
        owner[c] = None

    for f in range(n):
        if loop and f == loop[0]:
            vg.mark_loop()
            for c in chans:                          # al volver, todo se escribe de nuevo
                last[c] = {}
                patch[c] = None
        # notas nuevas
        for voice in range(8):
            if not kon[f, voice]:
                continue
            ins = instrs.get(int(srcn[f, voice]))
            if ins is None or ins.kind == "no":
                if vchan[voice]:
                    release(vchan[voice])
                    vchan[voice] = None
                continue
            k = cls(ins, voice, f)
            c = vchan[voice]
            if c is None or c[0] != k:
                if c is not None:
                    release(c)
                cand = [x for x in chans if x[0] == k]
                if k == "psg" or k == "fm":
                    cand = [x for x in chans if x[0] == k] + [x for x in chans if x[0] == ("psg" if k == "fm" else "fm")]
                free = [x for x in cand if owner[x] is None]
                pref = [x for x in free if x == vlast[voice]]
                if pref:
                    c = pref[0]
                elif free:
                    c = free[0]
                else:                                 # se le quita a la voz más baja si es más baja que esta
                    same = [x for x in cand if x[0] == cand[0][0]] or cand
                    c = min(same, key=lambda x: amp(owner[x], f))
                    if amp(owner[c], f) > amp(voice, f):
                        continue
                    vchan[owner[c]] = None
                    release(c)
                owner[c] = voice
                vchan[voice] = c
                vlast[voice] = c
            # cambio de instrumento (FM): parche
            kind, i = c
            if kind == "fm":
                port, ch = (0, i) if i < 3 else (1, i - 3)
                if patch[c] is not ins:
                    vg.ym(0, 0x28, (port << 2) | ch)
                    regs = ins.regs()
                    car = ins.carriers()
                    for j, base in enumerate((0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90)):
                        for o in range(4):
                            # el TL de los portadores lo pone después la envolvente del SNES (att); los moduladores
                            # llevan el suyo (si no, un parche con algoritmo 0-6 suena sin modulación)
                            vg.ym(port, base + 4 * o + ch, 127 if base == 0x40 and o in car else regs[j * 4 + o])
                    vg.ym(port, 0xB0 + ch, (ins.fb << 3) | ins.alg)
                    patch[c] = ins
                    last[c].pop("att", None)
                last[c]["kon"] = True
            last[c]["ins"] = ins
        # estado de cada canal en uso
        for c in chans:
            voice = owner[c]
            if voice is None:
                continue
            ins = last[c].get("ins")
            a = amp(voice, f)
            if (env[f, voice] == 0 and not kon[f, voice]) or a <= 0:
                vchan[voice] = None
                release(c)
                continue
            att_db = -20 * np.log10(max(a, 1e-5)) - ins.vol - master
            kind, i = c
            hz = (ins.f0 or 0) * pitch[f, voice] / 4096.0 * (2.0 ** ins.oct)
            L, R = abs(volL[f, voice]), abs(volR[f, voice])
            pan = 0xC0 if min(L, R) >= 0.4 * max(L, R) else (0x80 if L > R else 0x40)
            if kind == "fm":
                port, ch = (0, i) if i < 3 else (1, i - 3)
                if last[c].get("pan") != pan:
                    vg.ym(port, 0xB4 + ch, pan)
                    last[c]["pan"] = pan
                blk, fn = ym_freq(hz)
                fr = (blk << 11) | fn
                kon_now = last[c].pop("kon", False)
                if kon_now:
                    vg.ym(0, 0x28, (port << 2) | ch)
                if last[c].get("fr") != fr or kon_now:
                    vg.ym(port, 0xA4 + ch, fr >> 8)
                    vg.ym(port, 0xA0 + ch, fr & 0xFF)
                    last[c]["fr"] = fr
                att = int(round(att_db / 0.75))
                if last[c].get("att") is None or abs(att - last[c]["att"]) >= 1:
                    for o in ins.carriers():
                        tl = min(127, max(0, ins.ops[o][2] + att))
                        vg.ym(port, 0x40 + 4 * o + ch, tl)
                    last[c]["att"] = att
                if kon_now:
                    vg.ym(0, 0x28, 0xF0 | (port << 2) | ch)
            elif kind == "psg":
                nn = int(round(PSG_CLOCK / (32 * hz))) if hz > 0 else 1023
                nn = min(1023, max(1, nn))
                if last[c].get("n") != nn:
                    vg.psg(0x80 | (i << 5) | (nn & 15))
                    vg.psg(nn >> 4)
                    last[c]["n"] = nn
                att = min(15, max(0, int(round(att_db / 2.0)) + mix.get("psg_extra", 0)))
                if last[c].get("att") != att:
                    vg.psg(0x90 | (i << 5) | att)
                    last[c]["att"] = att
            else:                                    # ruido: blanco, la velocidad según la altura
                rate = ins.noise if ins.noise else (0 if pitch[f, voice] >= 0x1800 else 1 if pitch[f, voice] >= 0x0C00 else 2)
                if last[c].get("n") != rate:
                    vg.psg(0xE0 | 4 | rate)
                    last[c]["n"] = rate
                att = min(15, max(0, int(round(att_db / 2.0)) + mix.get("ruido_extra", 0)))
                if last[c].get("att") != att:
                    vg.psg(0xF0 | att)
                    last[c]["att"] = att
        vg.wait()
    for c in chans:
        if owner[c] is not None:
            release(c)
    return vg


# ---------------------------------------------------------------- todo junto
# mezcla (medida contra el SNES, docs/55): la música FM y PSG un poco más baja que el SNES a todo volumen para dejar
# lugar a los efectos PCM, que van a 8 bits.
MIX = {"musica_db": float(os.environ.get("MM_MUS_DB", -4.0)), "psg_extra": 0, "ruido_extra": 2}
SFX_GAIN = float(os.environ.get("MM_SFX_GAIN", 3.0))      # 16 bits del SNES -> 8 bits: x * SFX_GAIN / 256
# lugar de las voces en la ROM: el mismo que tenían antes (docs/40), para no correr las direcciones de lo que sigue
VOICE_RESERVE = {"kVoiceCaptivity": 37632, "kVoicePeace": 28160}
MAX_FRAMES = 16000


def read_music(path):
    out = []
    for line in Path(path).read_text().splitlines():
        p = line.split("#")[0].split()
        if p:
            out.append((int(p[0], 0), int(p[1], 0), p[2], int(p[3]) if len(p) > 3 else MAX_FRAMES))
    return out


def run_track(spc, song, track, frames, ram_path=None):
    ram = f"ram {ram_path}\n" if ram_path else ""
    a, st, _ = spc.run(f"f 1\nup {song}\n{ram}f 2\np 0 {track}\nf {frames + 2}\n", want_states=True)
    return a[2 * 534:], st[2:]


def pcm_track(a, gain):
    """una pista grabada tal cual (las voces): mono, sin el silencio de los bordes, 13 300 Hz"""
    x = a.mean(axis=1)
    nz = np.nonzero(np.abs(x) > max(np.abs(x).max() * 0.02, 1.5 * 256 / gain))[0]
    if not len(nz):
        return b""
    x = x[nz[0]:nz[-1] + 1].copy()
    fade = min(len(x), int(0.02 * RATE_IN))
    x[len(x) - fade:] *= np.linspace(1, 0, fade)
    y = to_s8(lowpass_resample(x, RATE_IN, RATE_OUT), gain)
    return y.tobytes()


def main():
    rom, gen = sys.argv[1], Path(sys.argv[2])
    work = Path(sys.argv[3] if len(sys.argv) > 3 else ROOT / "work" / "snd")
    analizar = "--analizar" in sys.argv
    spc = Spc(rom, work)
    snd = gen / "snd"
    snd.mkdir(parents=True, exist_ok=True)
    table = read_instr_table(HERE / "instrumentos.txt")
    romb = open(rom, "rb").read()
    romb = romb[len(romb) & 0x7FFF:]

    asm = ["/* MegaMetroid — sonido del cartucho, generado por tools/snd/mksnd.py desde tu ROM (docs/55) */",
           "    .section .late_rodata", "    .balign 256"]
    music_rows, voices = [], {}
    total = 0
    for song, track, tipo, frames in read_music(HERE / "musica.txt"):
        a, st = run_track(spc, song, track, frames, work / f"ram{song:02X}.bin")
        loop = find_loop(st) if tipo == "xgm2" else None
        play = np.nonzero(st[:, 98])[0]
        end = loop[1] if loop else (int(play[-1]) + 2 if len(play) and play[-1] < len(st) - 2 else len(st))
        name = f"kSndMus{song:02X}_{track}"
        if tipo == "pcm":
            b = pcm_track(a[:end * 534], SFX_GAIN)
            voices[song] = b
            log(f"música {song:#04x}/{track}: PCM {len(b)} bytes ({len(b) / RATE_OUT:.2f} s)")
            continue
        ram = (work / f"ram{song:02X}.bin").read_bytes()
        v = st[:end, :96].reshape(-1, 8, 12)
        used = sorted(set(v[:, :, 0][v[:, :, 9] > 0].tolist()))
        instrs = {s: make_instr(ram, song, s, table) for s in used}
        if analizar:
            for s in used:
                ins = instrs[s]
                m = (v[:, :, 9] > 0) & (v[:, :, 0] == s)
                x, l = brr.decode(ram, s)
                h = brr.harmonics(x, l, ins.f0, 8) if ins.f0 else []
                print(f"# {song:#04x} {s:2d}: {ins.kind:5s} f0={ins.f0 and round(ins.f0, 1)} notas={int(m.sum())} "
                      f"voces={sorted(set(np.nonzero(m)[1].tolist()))} armónicas={np.round(h, 2).tolist()}")
        vg = convert_track(st, instrs, loop, end, MIX)
        vgm = snd / f"{name}.vgm"
        vgm.write_bytes(vg.bytes())
        xgc = snd / f"{name}.xgc"
        r = subprocess.run(["java", "-jar", str(GDK / "bin" / "xgm2tool.jar"), str(vgm), str(xgc)],
                           capture_output=True, text=True)
        if r.returncode or not xgc.exists():
            raise SystemExit(f"xgm2tool: {r.stdout}\n{r.stderr}")
        n = xgc.stat().st_size
        total += n
        log(f"música {song:#04x}/{track}: {end} cuadros, lazo {loop}, {len(used)} instrumentos, VGM "
            f"{len(vg.d)} -> XGM2 {n} bytes")
        asm += ["    .balign 256", f"    .global {name}", f"{name}:", f'    .incbin "snd/{name}.xgc"']
        music_rows.append((song, track, name, n))

    # efectos
    conf = sfx_conf()
    want = {}
    for lib, i, song in read_list(HERE / "efectos.txt"):
        want.setdefault((lib, i), set()).add(song)
    zeb = sfxlist.zebes_sfx(romb)
    for k in zeb:
        want.setdefault(k, set()).add(0)
    sfx_rows, blobs = [], {}
    for (lib, i) in sorted(want):
        for song in sorted(want[(lib, i)]):
            b, lp, frames, half = render_sfx(spc, lib, i, song, SFX_GAIN)
            flags = sfx_flags(lib, i, conf) | (8 if (lib, i) in zeb else 0) | (16 if half else 0)
            key = b + b"|" + lp
            if key not in blobs:
                blobs[key] = len(blobs)
            sfx_rows.append((lib, i, song, blobs[key], len(b), len(lp), frames, flags))
    # muestras: primero en el lugar que tenían las voces de antes (docs/40: .rodata, mismo tamaño, para no correr las
    # direcciones de lo que sigue), el resto al final de lo común (.late_rodata)
    pcms = []
    for key, idx in sorted(blobs.items(), key=lambda t: t[1]):
        sep = key.rindex(b"|")
        pcms += [(f"kSndSfxPcm{idx}", key[:sep]), (f"kSndSfxLoop{idx}", key[sep + 1:])]
    for song, b in voices.items():
        pcms.append((f"kSndVoice{song:02X}", b + bytes((-len(b)) % 64)))
    holds = [[VOICE_RESERVE[k], []] for k in ("kVoiceCaptivity", "kVoicePeace")]
    late = []
    for name, b in sorted(pcms, key=lambda t: -len(t[1])):
        n = len(b) + (-len(b)) % 256
        for h in holds:
            if n <= h[0]:
                h[0] -= n
                h[1].append((name, b))
                break
        else:
            late.append((name, b))
    sfx_bytes = sum(len(b) for _, b in pcms)

    def emit(lst, out):
        for name, b in lst:
            (snd / f"{name}.pcm").write_bytes(b)
            out += ["    .balign 256", f"    .global {name}", f"{name}:"]
            if b:
                out.append(f'    .incbin "snd/{name}.pcm"')
    emit(late, asm)
    log(f"efectos: {len(sfx_rows)} ({len(blobs)} distintos); muestras {sfx_bytes} bytes "
        f"({sum(len(b) for _, b in late)} al final de lo común); música XGM2: {total} bytes")

    # tablas (las lee src/platform/md_snd.c: inc/platform/md_snd.h)
    asm += ["    .balign 4", "    .global kSndMusic", "kSndMusic:"]
    for song, track, name, n in music_rows:
        asm.append(f"    .byte {song}, {track}, 1, 0\n    .long {name}, {n}")
    for song, b in voices.items():
        asm.append(f"    .byte {song}, 5, 2, 0\n    .long kSndVoice{song:02X}, {len(b) + (-len(b)) % 64}")
    asm.append("    .byte 0, 0, 0, 0\n    .long 0, 0")
    asm += ["    .balign 4", "    .global kSndSfx", "kSndSfx:"]
    for lib, i, song, idx, n, nl, frames, flags in sfx_rows:
        asm.append(f"    .long kSndSfxPcm{idx}, {n}, kSndSfxLoop{idx}, {nl}\n"
                   f"    .short {frames}\n    .byte {lib}, {i}, {song}, {flags}, 0, 0")
    asm.append("    .long 0, 0, 0, 0\n    .short 0\n    .byte 0, 0, 0, 0, 0, 0")
    (gen / "md_snd_data.s").write_text("\n".join(asm) + "\n")

    # el lugar de las voces de antes: el mismo tamaño y los mismos símbolos (docs/40), con muestras adentro
    out = ["/* MegaMetroid — el lugar que tenían las voces de la intro (docs/40), con el mismo tamaño para no correr las",
           "   direcciones de lo que sigue: ahora lleva muestras del sonido (tools/snd/mksnd.py, docs/55). */",
           "    .section .rodata", "    .balign 256"]
    for (name, size), h in zip(VOICE_RESERVE.items(), holds):
        out += ["    .balign 256", f"    .global {name}", f"{name}:"]
        used = 0
        for pname, b in h[1]:
            (snd / f"{pname}.pcm").write_bytes(b)
            out += [f"    .global {pname}", f"{pname}:"]
            if b:
                out.append(f'    .incbin "snd/{pname}.pcm"')
            pad = (-len(b)) % 256
            if pad:
                out.append(f"    .space {pad}")
            used += len(b) + pad
        out += [f"    .space {size - used}", f"    .global {name}Len", "    .balign 2", f"{name}Len:", "    .long 0"]
    (gen / "md_voice.s").write_text("\n".join(out) + "\n")

if __name__ == "__main__":
    main()
