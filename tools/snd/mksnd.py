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

    def __init__(self, rom, work, exe=None):
        self.rom, self.work = rom, Path(work)
        self.work.mkdir(parents=True, exist_ok=True)
        self.exe = Path(exe) if exe else self.work / "spcrun"
        src = HERE / "spcrun.c"
        if not exe and (not self.exe.exists() or self.exe.stat().st_mtime < src.stat().st_mtime):
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
    # (el tramo centrado de la convolución entera: lo mismo que mode="same", que con menos muestras que el filtro
    # devuelve el largo del filtro)
    y = np.convolve(x, h, mode="full")[(taps - 1) // 2:(taps - 1) // 2 + len(x)]
    if n_out is None:
        n_out = max(1, int(round(len(x) * rout / rin)))
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
    ins.noise = int(kv.get("ruido", -1))            # -1: la velocidad sale de la altura (noise_rate)
    if ins.kind == "ruido":
        # la percusión es una grabación (un platillo...) que se apaga sola y, sin lazo, el DSP corta la voz al final:
        # su volumen a lo largo de la muestra (RMS cada 64 muestras) y su brillo (centroide del espectro)
        xs = np.asarray(x, dtype=np.float64)
        blk = np.array([np.sqrt(np.mean(xs[i:i + 64] ** 2)) for i in range(0, len(xs), 64)] or [0.0])
        ins.senv = blk / blk.max() if blk.max() > 0 else blk
        ins.sloop = loopidx
        sp = np.abs(np.fft.rfft((xs - xs.mean()) * np.hanning(len(xs)))) if len(xs) > 1 else np.zeros(2)
        fr = np.fft.rfftfreq(len(xs), 1 / 32000.0) if len(xs) > 1 else np.zeros(2)
        ins.cent = float((sp * fr).sum() / sp.sum()) if sp.sum() > 0 else 4000.0
    return ins


def _noise_centroids():
    """centroide del ruido blanco del PSG en sus 3 velocidades (el registro se corre a reloj/16/N, N = 16, 32, 64): el
    espectro es |sinc(f/fs)|, medido hasta 16 kHz (lo que da el DSP del SNES a 32 kHz)"""
    f = np.linspace(1, 16000, 4000)
    out = []
    for n in (16, 32, 64):
        fs = PSG_CLOCK / 16.0 / n
        m = np.abs(np.sinc(f / fs))
        out.append(float((m * f).sum() / m.sum()))
    return out


NOISE_CENT = None


def noise_rate(ins, pitch):
    """la velocidad del ruido (0 la más aguda) cuyo brillo se parece más al de la muestra tocada a esa altura"""
    global NOISE_CENT
    if ins.noise >= 0:
        return ins.noise
    if NOISE_CENT is None:
        NOISE_CENT = _noise_centroids()
    played = getattr(ins, "cent", 4000.0) * pitch / 4096.0
    return int(np.argmin([abs(np.log(max(played, 1.0) / c)) for c in NOISE_CENT]))


# ---------------------------------------------------------------- música: del SPC a la Genesis
def find_loop(st):
    """(inicio, fin) del lazo en cuadros, o None si la pista termina. 1) la lista de patrones vuelve atrás
    (music_ptr_toplevel); 2) si no cambia (un patrón que se repite), el período de las notas desde la primera nota
    o, si no, desde el último patrón (el título: una entrada que no se repite y un patrón que repite solo)"""
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
    n = len(sig)
    last = max(f for f in range(1, len(tp)) if tp[f] != tp[f - 1])
    for s in dict.fromkeys((int(kon_f[0]), last)):
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


def convert_track(st, instrs, loop, end, mix, trace=None):
    """estados del SPC por cuadro -> VGM (YM2612: canales 0-4; PSG: 0-2 tono, 3 ruido). Con `trace` (una lista)
    anota además lo que suena en cada canal y cuadro, para el proyecto de Furnace (mkfur.py):
    (cuadro, canal, instrumento, Hz, atenuación (pasos del TL o del PSG), paneo, nota nueva, ruido) o
    (cuadro, canal, None, ...) al soltarlo."""
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

    vpos = [0.0] * 8                                  # dónde va cada voz en su muestra (muestras de 32 kHz)

    def amp(voice, f):
        e = max(env[f, voice], envmax[f, voice] if kon[f, voice] else 0)
        a = e / 2047.0 * max(abs(volL[f, voice]), abs(volR[f, voice])) / 127.0
        ins = instrs.get(int(srcn[f, voice]))
        senv = getattr(ins, "senv", None)
        if senv is not None and not noise_dsp[f, voice]:   # la percusión: lo que suena de la grabación
            k = int(vpos[voice] / 64)
            if k >= len(senv):
                lp = ins.sloop
                if lp is None:
                    return 0.0                        # sin lazo: el DSP corta la voz al final de la muestra
                k = lp // 64 + (k - lp // 64) % max(1, len(senv) - lp // 64)
            a *= senv[min(k, len(senv) - 1)]
        return a

    def release(c):
        kind, i = c
        if kind == "fm":
            port, ch = (0, i) if i < 3 else (1, i - 3)
            vg.ym(0, 0x28, (port << 2) | ch)
        else:
            vg.psg(0x9F | (i << 5))
        last[c].pop("att", None)
        owner[c] = None
        if trace is not None:
            trace.append((f, c, None, 0, 0, 0, False, 0))

    for f in range(n):
        for voice in range(8):                        # avance de cada voz en su muestra
            vpos[voice] = 0.0 if kon[f, voice] else vpos[voice] + pitch[f - 1, voice] * 32000.0 / 4096.0 / 60.0
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
            kon_now = last[c].pop("kon", False)
            if kind == "fm":
                port, ch = (0, i) if i < 3 else (1, i - 3)
                if last[c].get("pan") != pan:
                    vg.ym(port, 0xB4 + ch, pan)
                    last[c]["pan"] = pan
                blk, fn = ym_freq(hz)
                fr = (blk << 11) | fn
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
                if trace is not None:
                    trace.append((f, c, ins, hz, att, pan, kon_now, 0))
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
                if trace is not None:
                    trace.append((f, c, ins, hz, att, pan, kon_now, 0))
            else:                                    # ruido: blanco, la velocidad según la altura
                rate = noise_rate(ins, pitch[f, voice])
                if last[c].get("n") != rate:
                    vg.psg(0xE0 | 4 | rate)
                    last[c]["n"] = rate
                att = min(15, max(0, int(round(att_db / 2.0)) + mix.get("ruido_extra", 0)))
                if last[c].get("att") != att:
                    vg.psg(0xF0 | att)
                    last[c]["att"] = att
                if trace is not None:
                    trace.append((f, c, ins, hz, att, pan, kon_now, rate))
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


def write_sample(path, x, loopidx, secs=2.0):
    """la muestra del SNES sola (--analizar): a su altura natural (32 kHz, pitch $1000), con su lazo hasta `secs`"""
    import wave
    x = np.asarray(x, dtype=np.int16)
    n = int(32000 * secs)
    if loopidx is not None and loopidx < len(x):
        y = list(x)
        while len(y) < n:
            y += list(x[loopidx:])
        x = np.array(y[:n], dtype=np.int16)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(32000)
        w.writeframes(x.astype("<i2").tobytes())


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


# ---------------------------------------------------------------- páginas (docs/55)
PAGES = [6, 24, 25, 26, 27, 28, 29, 30, 31]           # páginas de sonido del SSF2 (md_ssf2.ld: .snd6, .snd24...)
PAGE_SIZE = 0x80000
ANY_SET = 0xFF                                        # en las tablas: el efecto suena igual con cualquier conjunto


def sfx_cache_key(rom_bytes):
    """lo que cambia una grabación: la ROM, el N-SPC y el código de grabación de este archivo"""
    import hashlib
    import inspect
    h = hashlib.sha1(rom_bytes)
    h.update((HERE / "spcrun.c").read_bytes())
    for f in (render_sfx, lowpass_resample, to_s8, is_dull, pad64):
        h.update(inspect.getsource(f).encode())
    h.update(str((SFX_GAIN, RATE_OUT, RATE_IN)).encode())
    return h.hexdigest()[:12]


def _render_job(args):
    rom, work, exe, lib, i, song, cache = args
    import pickle
    f = Path(cache) / f"{lib}_{i:02X}_{song:02X}.pkl"
    if f.exists():
        return (lib, i, song), pickle.loads(f.read_bytes())
    spc = Spc(rom, Path(work) / f"spc{os.getpid()}", exe)   # (una carpeta por proceso)
    r = render_sfx(spc, lib, i, song, SFX_GAIN)
    f.write_bytes(pickle.dumps(r))
    return (lib, i, song), r


def render_many(rom, work, jobs):
    """graba los efectos (biblioteca, efecto, conjunto) en paralelo, con caché en work/sfxcache/<clave>"""
    from multiprocessing import Pool
    romb = open(rom, "rb").read()
    cache = Path(work) / "sfxcache" / sfx_cache_key(romb)
    cache.mkdir(parents=True, exist_ok=True)
    n = max(1, os.cpu_count() or 1)
    exe = str(Spc(rom, work).exe)
    args = [(rom, str(work), exe, lib, i, song, str(cache)) for lib, i, song in jobs]
    out = {}
    with Pool(n) as pool:
        for k, r in pool.imap_unordered(_render_job, args):
            out[k] = r
    return out


def pack_pages(sets, size_of):
    """reparte los conjuntos en páginas: cada uno va a la página donde suma menos (lo que ya está no se repite)"""
    pages = []                                        # [conjuntos, claves de lo que lleva]
    for st in sorted(sets, key=lambda x: -size_of({x}, set())):
        best = None
        for pg in pages:
            add = size_of(pg[0] | {st}, pg[1]) - size_of(pg[0], pg[1])
            if size_of(pg[0] | {st}, pg[1]) <= PAGE_SIZE and (best is None or add < best[0]):
                best = (add, pg)
        if best is None:
            pages.append([{st}, set()])
        else:
            best[1][0].add(st)
    return [pg[0] for pg in pages]


def main():
    rom, gen = sys.argv[1], Path(sys.argv[2])
    work = Path(sys.argv[3] if len(sys.argv) > 3 else ROOT / "work" / "snd")
    analizar = "--analizar" in sys.argv
    spc = Spc(rom, work)
    snd = gen / "snd"
    snd.mkdir(parents=True, exist_ok=True)
    for f in snd.glob("*"):
        f.unlink()
    table = read_instr_table(HERE / "instrumentos.txt")
    romb = open(rom, "rb").read()
    romb = romb[len(romb) & 0x7FFF:]

    # ---- música: cada pista convertida (las 1-4 son del motor: van en lo fijo y suenan con cualquier conjunto)
    tracks = {}                                        # (conjunto, pista) -> (tipo 1 XGM2 / 2 PCM, bytes)
    for song, track, tipo, frames in read_music(HERE / "musica.txt"):
        a, st = run_track(spc, song, track, frames, work / f"ram{song:02X}.bin")
        loop = find_loop(st) if tipo == "xgm2" else None
        play = np.nonzero(st[:, 98])[0]
        end = loop[1] if loop else (int(play[-1]) + 2 if len(play) and play[-1] < len(st) - 2 else len(st))
        name = f"kSndMus{song:02X}_{track}"
        if tipo == "pcm":
            b = pcm_track(a[:end * 534], SFX_GAIN)
            b += bytes((-len(b)) % 64)
            tracks[(song, track)] = (2, b)
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
                      f"voces={sorted(set(np.nonzero(m)[1].tolist()))} armónicas={np.round(h, 2).tolist()} "
                      f"pista={track}")
                write_sample(work / "muestras" / f"{song:02X}_{s}.wav", x, l)
        vg = convert_track(st, instrs, loop, end, MIX)
        vgm = snd / f"{name}.vgm"
        vgm.write_bytes(vg.bytes())
        xgc = snd / f"{name}.xgc"
        r = subprocess.run(["java", "-jar", str(GDK / "bin" / "xgm2tool.jar"), str(vgm), str(xgc)],
                           capture_output=True, text=True)
        if r.returncode or not xgc.exists():
            raise SystemExit(f"xgm2tool: {r.stdout}\n{r.stderr}")
        tracks[(song, track)] = (1, xgc.read_bytes())
        log(f"música {song:#04x}/{track}: {end} cuadros, lazo {loop}, {len(used)} instrumentos, XGM2 "
            f"{len(tracks[(song, track)][1])} bytes")

    # ---- efectos: quién los pide (tools/snd/sndmap.py) y los que agrega efectos.txt
    import sndmap
    mp = sndmap.build(rom)
    glob = set(mp["global"])
    per_set = {k: set(v) for k, v in mp["per_set"].items()}
    for lib, i, song in read_list(HERE / "efectos.txt"):
        if song:
            per_set.setdefault(song, set()).add((lib, i))
        else:
            glob.add((lib, i))
    conf = sfx_conf()
    nmax = {lib: len(conf[lib]) for lib in conf}
    glob = {k for k in glob if k[1] <= nmax[k[0]]}
    for k in list(per_set):
        per_set[k] = {x for x in per_set[k] if x[1] <= nmax[x[0]]} - glob
    zeb = sfxlist.zebes_sfx(romb)
    music_sets = {s for s, _ in tracks}
    sets = sorted((set(per_set) | music_sets) - {0})
    jobs = [(lib, i, 0) for lib, i in sorted(glob)] + \
        [(lib, i, s) for s in sets for lib, i in sorted(per_set.get(s, ()))]
    rend = render_many(rom, work, jobs)

    def blob(r):
        return r[0] + b"|" + r[1]

    def item_size(b):
        n = len(b) - 1
        return n + (-n) % 256 + 512                    # (alineado a 256 y las dos alineaciones de 256 del lazo)

    def mus_size(st):
        return sum(len(b) + 256 for (s, t), (k, b) in tracks.items() if s == st and t >= 5)

    def size_of(sts, _):
        bl = {blob(rend[(lib, i, s)]) for s in sts for lib, i in per_set.get(s, ())}
        return 4096 + sum(mus_size(s) for s in sts) + sum(item_size(b) for b in bl if len(b) > 1)
    page_sets = pack_pages(sets, size_of)
    if len(page_sets) > len(PAGES):
        raise SystemExit(f"mksnd: hacen falta {len(page_sets)} páginas de sonido; hay {len(PAGES)}")

    # ---- archivos: muestras (una vez cada una), tablas en lo fijo y en cada página
    files = {}                                        # (dónde: 0 lo fijo o la página, muestra) -> nombre
    scope = [0]                                       # dónde se está armando: lo de una página solo ve lo fijo y lo suyo

    def pcm_file(b):
        for k in ((0, b), (scope[0], b)):
            if k in files:
                return files[k]
        name = files[(scope[0], b)] = f"kSndPcm{len(files)}"
        (snd / f"{name}.pcm").write_bytes(b)
        return name

    def sfx_rows(keys, set_of):
        rows = []
        for lib, i in sorted(keys):
            variants = {}
            for s in set_of(lib, i):
                r = rend[(lib, i, s)]
                variants.setdefault(blob(r), (r, []))[1].append(s)
            for b, (r, ss) in variants.items():
                flags = sfx_flags(lib, i, conf) | (8 if (lib, i) in zeb else 0) | (16 if r[3] else 0)
                for s in (ss if len(variants) > 1 else [ANY_SET]):
                    rows.append((lib, i, s, r, flags))
        return rows

    def emit_sfx_table(name, rows, out):
        out += ["    .balign 4", f"    .global {name}", f"{name}:"]
        for lib, i, s, r, flags in rows:
            a = pcm_file(r[0]) if r[0] else "0"
            l = pcm_file(r[1]) if r[1] else "0"
            out.append(f"    .long {a}, {len(r[0])}, {l}, {len(r[1])}\n    .short {r[2]}\n"
                       f"    .byte {lib}, {i}, {s}, {flags}, 0, 0")
        out.append("    .long 0, 0, 0, 0\n    .short 0\n    .byte 0, 0, 0, 0, 0, 0")

    def emit_music_table(name, keys, out):
        out += ["    .balign 4", f"    .global {name}", f"{name}:"]
        for (s, t) in keys:
            k, b = tracks[(s, t)]
            out.append(f"    .byte {s if t >= 5 else 0}, {t}, {k}, 0\n    .long {pcm_file(b)}, {len(b)}")
        out.append("    .byte 0, 0, 0, 0\n    .long 0, 0")

    def emit_files(names, section, out):
        for b, name in names:
            out += [f"    .section {section}", "    .balign 256", f"    .global {name}", f"{name}:",
                    f'    .incbin "snd/{name}.pcm"']

    # lo fijo: los efectos globales, las pistas del motor (1-4) y qué página tiene cada conjunto
    fixed_rows = sfx_rows(glob, lambda lib, i: [0])
    engine = sorted(k for k in tracks if k[1] < 5)
    asm = ["/* MegaMetroid — sonido del cartucho, generado por tools/snd/mksnd.py desde tu ROM (docs/55) */",
           "    .section .late_rodata"]
    emit_sfx_table("kSndSfx", fixed_rows, asm)
    emit_music_table("kSndMusic", engine, asm)
    set_page = [0] * 256
    for pg, sts in zip(PAGES, page_sets):
        for s in sts:
            set_page[s] = pg
    asm += ["    .global kSndSetPage", "kSndSetPage:", "    .byte " + ", ".join(map(str, set_page))]
    fixed_files = [(b, n) for (w, b), n in files.items() if w == 0]
    # cada página: su tabla al principio (0x300000) y sus muestras
    page_asm, page_report = [], []
    for pg, sts in zip(PAGES, page_sets):
        scope[0] = pg
        rows = sfx_rows(set().union(*(per_set.get(s, set()) for s in sts)),
                        lambda lib, i: [s for s in sorted(sts) if (lib, i) in per_set.get(s, ())])
        sec = f".snd_page{pg}"
        out = [f"    .section {sec},\"a\"", "    .balign 4", f"    .global kSndPage{pg}", f"kSndPage{pg}:",
               f"    .long kSndPage{pg}Music, kSndPage{pg}Sfx"]
        emit_music_table(f"kSndPage{pg}Music", sorted(k for k in tracks if k[0] in sts and k[1] >= 5), out)
        emit_sfx_table(f"kSndPage{pg}Sfx", rows, out)
        new = [(b, n) for (w, b), n in files.items() if w == pg]
        emit_files(new, f"{sec},\"a\"", out)
        page_asm += out
        nbytes = sum(len(b) for b, _ in new)
        page_report.append(f"página {pg}: conjuntos {' '.join(f'{s:02X}' for s in sorted(sts))}: "
                           f"{len(rows)} efectos, {nbytes // 1024} KB de muestras")
    # las muestras de lo fijo: primero en el lugar que tenían las voces de antes (.rodata, mismo tamaño, para no
    # correr las direcciones de lo que sigue: docs/40), lo demás al final de lo común (.late_rodata)
    holds = [[VOICE_RESERVE[k], []] for k in ("kVoiceCaptivity", "kVoicePeace")]
    late = []
    for b, name in sorted(fixed_files, key=lambda t: -len(t[0])):
        n = len(b) + (-len(b)) % 256
        for h in holds:
            if n <= h[0]:
                h[0] -= n
                h[1].append((name, b))
                break
        else:
            late.append((b, name))
    emit_files(late, ".late_rodata", asm)
    (gen / "md_snd_data.s").write_text("\n".join(asm + page_asm) + "\n")
    log(f"sonido: fijo {sum(len(b) for b, _ in fixed_files) // 1024} KB ({len(fixed_rows)} efectos globales, "
        f"{len(engine)} pistas del motor); {len(page_sets)} páginas")
    for r in page_report:
        log("  " + r)

    # el lugar de las voces de antes: el mismo tamaño y los mismos símbolos (docs/40), con muestras adentro
    out = ["/* MegaMetroid — el lugar que tenían las voces de la intro (docs/40), con el mismo tamaño para no correr las",
           "   direcciones de lo que sigue: ahora lleva muestras del sonido (tools/snd/mksnd.py, docs/55). */",
           "    .section .rodata", "    .balign 256"]
    for (name, size), h in zip(VOICE_RESERVE.items(), holds):
        out += ["    .balign 256", f"    .global {name}", f"{name}:"]
        used = 0
        for pname, b in h[1]:
            out += [f"    .global {pname}", f"{pname}:", f'    .incbin "snd/{pname}.pcm"']
            pad = (-len(b)) % 256
            if pad:
                out.append(f"    .space {pad}")
            used += len(b) + pad
        out += [f"    .space {size - used}", f"    .global {name}Len", "    .balign 2", f"{name}Len:", "    .long 0"]
    (gen / "md_voice.s").write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
