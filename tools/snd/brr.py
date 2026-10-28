"""MegaMetroid — muestras BRR del SPC (la RAM que vuelca spcrun): decodificar y medir (docs/55)."""
import numpy as np

DIR = 0x6D00                                   # DIR = 0x6D (Vector_Reset_Spc, spc_player.c)


def decode(ram, srcn, maxblocks=4000):
    """devuelve (muestras int16, índice del lazo o None)"""
    start = ram[DIR + 4 * srcn] | ram[DIR + 4 * srcn + 1] << 8
    loop = ram[DIR + 4 * srcn + 2] | ram[DIR + 4 * srcn + 3] << 8
    out, p1, p2, a, loopidx = [], 0, 0, start, None
    for _ in range(maxblocks):
        if a == loop and loopidx is None:
            loopidx = len(out)
        h = ram[a]
        sh, flt = h >> 4, (h >> 2) & 3
        for i in range(16):
            b = ram[(a + 1 + i // 2) & 0xFFFF]
            n = (b >> 4) if i % 2 == 0 else (b & 15)
            n = n - 16 if n >= 8 else n
            s = (n << sh) >> 1 if sh <= 12 else (-2048 if n < 0 else 0)
            if flt == 1:
                s += p1 + ((-p1) >> 4)
            elif flt == 2:
                s += 2 * p1 + ((-3 * p1) >> 5) - p2 + (p2 >> 4)
            elif flt == 3:
                s += 2 * p1 + ((-13 * p1) >> 6) - p2 + ((3 * p2) >> 4)
            s = max(-32768, min(32767, s))
            s = (s << 1) & 0xFFFF                   # 15 bits, como el DSP
            s = (s - 0x10000 if s >= 0x8000 else s) >> 1
            out.append(s)
            p2, p1 = p1, s
        a = (a + 9) & 0xFFFF
        if h & 1:
            break
    looped = bool(h & 1 and h & 2)
    return np.array(out, dtype=np.float64), (loopidx if looped else None)


def fundamental(x, loopidx, rate=32000.0):
    f = _fundamental(x, loopidx, rate)
    if f is None or f <= 0:
        return None
    if loopidx is not None and len(x) - loopidx >= 16:   # un lazo tiene un número entero de ciclos
        L = len(x) - loopidx
        n = max(1, round(L * f / rate))
        f = rate * n / L
    return f


def harmonics(x, loopidx, f0, n=16, rate=32000.0):
    """amplitud de las armónicas 1..n (la mayor = 1), medida en el lazo o en el cuerpo de la muestra"""
    seg = x[loopidx:] if loopidx is not None and len(x) - loopidx >= 16 else x[:min(len(x), 8192)]
    N = 16384
    t = np.tile(seg, N // max(1, len(seg)) + 1)[:N]
    sp = np.abs(np.fft.rfft((t - t.mean()) * np.hanning(N)))
    fr = np.fft.rfftfreq(N, 1 / rate)
    h = []
    for k in range(1, n + 1):
        if k * f0 >= rate / 2 - 200:
            h.append(0.0)
            continue
        i = int(round(k * f0 * N / rate))
        h.append(float(sp[max(0, i - 4):i + 5].max()))
    h = np.array(h)
    return h / h.max() if h.max() > 0 else h


def _fundamental(x, loopidx, rate=32000.0):
    """frecuencia fundamental de la muestra tocada a 32 kHz (pitch 0x1000), por autocorrelación de la parte que se
    repite (o del cuerpo); None si no tiene altura clara (percusión)"""
    seg = x[loopidx:] if loopidx is not None and len(x) - loopidx >= 32 else x[len(x) // 4:]
    if loopidx is not None and len(x) - loopidx >= 16:
        seg = np.tile(x[loopidx:], max(2, 4096 // max(1, len(x) - loopidx)))
    if len(seg) < 64:
        return None
    seg = seg - seg.mean()
    n = len(seg)
    f = np.fft.rfft(seg, 2 * n)
    ac = np.fft.irfft(f * np.conj(f))[:n]
    if ac[0] <= 0:
        return None
    ac /= ac[0]
    lo, hi = int(rate / 4000), min(n // 2, int(rate / 25))
    if hi <= lo + 2:
        return None
    k = lo + int(np.argmax(ac[lo:hi]))
    if ac[k] < 0.5:
        return None
    for d in (2, 3, 4):                         # preferir el período más corto con una correlación parecida
        kk = int(round(k / d))
        if kk > lo:
            w = ac[max(lo, kk - 2):kk + 3]
            if w.max() > ac[k] - 0.08:
                k = max(lo, kk - 2) + int(np.argmax(w))
                break
    if 0 < k < n - 1:                           # interpolación parabólica
        a, b, c = ac[k - 1], ac[k], ac[k + 1]
        den = a - 2 * b + c
        k = k + (0.5 * (a - c) / den if den else 0)
    return rate / k
