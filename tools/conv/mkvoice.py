#!/usr/bin/env python3
"""MegaMetroid — voces de la intro ("The last Metroid is in captivity. The galaxy is at peace.", docs/40).

Entrada: las muestras del SPC que extrae tools/sm_extract/extract_all.py de tu ROM (data/extracted_full/audio):
  "The last Metroid"  songset_3F/samples/sample_18_B516.wav     "is in captivity"  songset_3F/samples/sample_19_DAEA.wav
  "The galaxy"        songset_42/samples/sample_18_B516.wav     "is at peace"      songset_42/samples/sample_19_CE54.wav
El extractor las rotula a 32 kHz; la intro las toca a la mitad: se toman como 16 kHz, tal cual (sin remuestrear).
Salida: gen/md_voice.s con dos muestras para el controlador PCM de SGDK (8 bits con signo, 16 kHz, alineadas y con
largo múltiplo de 256): kVoiceCaptivity (01 + pausa + 02) y kVoicePeace (03 + pausa + 04). Sin los WAV, las
muestras quedan vacías (largo 0) y la intro va sin voz.

Uso: mkvoice.py data/extracted_full/audio GEN_DIR
"""
import os, sys, wave, struct

RATE = 16000
GAP = 0.22          # segundos entre las dos partes de cada frase


def load(path):
    w = wave.open(path)
    n, ch, sw = w.getnframes(), w.getnchannels(), w.getsampwidth()
    fr = RATE                                        # (ver arriba: el rótulo de 32 kHz no vale)
    raw = w.readframes(n)
    if sw == 2:
        v = list(struct.unpack('<%dh' % (n * ch), raw))
    elif sw == 1:
        v = [(b - 128) * 256 for b in raw]
    else:
        raise SystemExit('%s: muestras de %d bytes no soportadas' % (path, sw))
    if ch > 1:
        v = [sum(v[i:i + ch]) // ch for i in range(0, len(v), ch)]
    if fr != RATE:                                   # interpolación lineal
        out, step = [], fr / RATE
        t = 0.0
        while t < len(v) - 1:
            i = int(t); f = t - i
            out.append(v[i] * (1 - f) + v[i + 1] * f)
            t += step
        v = out
    return v


def to_s8(v):
    peak = max(1, max(abs(x) for x in v))
    k = 127.0 / peak                                 # a todo el rango de 8 bits, sin recortar
    return bytes((max(-128, min(127, int(round(x * k)))) & 0xFF) for x in v)


def phrase(res, a, b):
    pa, pb = os.path.join(res, a), os.path.join(res, b)
    if not (os.path.exists(pa) and os.path.exists(pb)):
        return b''
    v = load(pa) + [0] * int(GAP * RATE) + load(pb)
    s = to_s8(v)
    s += bytes((256 - len(s) % 256) % 256)           # silencio hasta múltiplo de 256
    return s


def main():
    res, gen = sys.argv[1], sys.argv[2]
    os.makedirs(os.path.join(gen, 'bins'), exist_ok=True)
    out = ['    .section .rodata', '    .balign 256']
    for name, a, b in (('kVoiceCaptivity', 'songset_3F/samples/sample_18_B516.wav', 'songset_3F/samples/sample_19_DAEA.wav'),
                       ('kVoicePeace', 'songset_42/samples/sample_18_B516.wav', 'songset_42/samples/sample_19_CE54.wav')):
        s = phrase(res, a, b)
        f = 'bins/%s.pcm' % name
        open(os.path.join(gen, f), 'wb').write(s)
        out += ['    .balign 256', '    .global %s' % name, '%s:' % name]
        if s:
            out.append('    .incbin "%s"' % f)
        out += ['    .global %sLen' % name, '    .balign 2', '%sLen:' % name, '    .long %d' % len(s)]
        print('%s: %d bytes%s' % (name, len(s), '' if s else ' (sin las muestras extraídas: sin voz)'))
    open(os.path.join(gen, 'md_voice.s'), 'w').write('\n'.join(out) + '\n')


main()
