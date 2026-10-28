#!/usr/bin/env python3
"""spore_fight.py — la pelea contra Spore Spawn (sala 9DC7) grabada en el SNES hasta su muerte (docs/50).

El bot no apunta: la entrada es la del bot hasta Zebes (in_z2.txt), después Samus mira a la derecha y dispara un misil
cada 8 cuadros. Spore Spawn se mueve igual haga lo que haga Samus hasta que recibe un golpe, así que en cada tramo en que
queda abierto (IA nullsub_223 $A5:EB1A, lista $A5:E719) se lleva a Samus con HL_SETXY a su izquierda, a la altura del
núcleo (x - 70, y + 9: el misil sale a y - 7 y el núcleo es la caja (-15,-24)-(14,23) con disparo SporeSpawn_Shot;
desde abajo o en diagonal lo tapa la concha de abajo, cajas con disparo $A5:8046). Si queda a menos de 123 no hay lugar
a la izquierda y se espera al tramo siguiente. Se repite la grabación con cada tramo nuevo hasta el bit de jefe.

Uso: python3 tools/rooms/spore_fight.py [carpeta]   (por defecto work/spore; ~2 GB de grabación)
     HLREF=... (por defecto work/inputs/hlref, de tools/inputs/make_inputs.sh)
Al final corre host/check con las mismas variables (0 diferencias esperadas) y deja setxy.txt.
"""
import mmap, os, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
WORK = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'work', 'spore'))
HLREF = os.environ.get('HLREF', os.path.join(ROOT, 'work', 'inputs', 'hlref'))
ROM = os.path.join(ROOT, 'sm', 'sm.smc')
FROM = 23500                     # cuadro del SNES en que empieza la grabación (in_z2.txt tiene 23520)
H, FS = 0x30000, 2 + 0x20000      # cabecera (RAM + VRAM) y cuadro (entrada + RAM) de hlref
os.makedirs(WORK, exist_ok=True)
IN, REF = os.path.join(WORK, 'in_spore.txt'), os.path.join(WORK, 'ref_spore.bin')

base = open(os.path.join(ROOT, 'tools', 'snes_ref', 'ceres_bot', 'in_z2.txt')).read()
with open(IN, 'w') as f:
    f.write(base if base.endswith('\n') else base + '\n')
    f.write('300 -\n' + '1 >\n9 -\n' * 20 + '2 X\n6 -\n' * 6000)

tele, done = [(250, 200, 700)], set()   # (cuadro grabado, x, y): primero a la arena
def env():
    return dict(os.environ, HL_GOD='1', HL_ENEMIES='1', HL_WARP='21:8D2A', HL_POKE='9C6:3E7,9C8:3E7,9D2:1',
                HL_SETXY=','.join('%d:%d:%d' % t for t in tele))

end = 2600
while True:
    subprocess.run(['setarch', '-R', HLREF, ROM, IN, REF, str(FROM), str(FROM + end)], env=env(),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    with open(REF, 'rb') as f:
        m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        n = (len(m) - H) // FS
        w = lambda fr, a: m[H + fr * FS + 2 + a] | m[H + fr * FS + 3 + a] << 8
        prev = new = dead = None
        for fr in range(n):
            ai = w(fr, 0xFA8)                                    # enemigo 0: ai_var_A
            if dead is None and w(fr, 0xD828) >> 8 & 2:          # boss_bits_for_area[1] (7E:D829) & 2
                dead = fr
            if new is None and ai != prev and ai == 0xEB1A and w(fr, 0xF92) == 0xE719 and fr not in done:
                new = (fr, w(fr, 0xF7A), w(fr, 0xF7E))
            prev = ai
        hp = w(n - 1, 0xF8C)
        m.close()
    print('hasta %d: energía %d, tramo abierto %s%s' % (end, hp, new, ', muerto en %d' % dead if dead else ''), flush=True)
    if dead is not None:
        break
    if new is None:
        end += 1000
        continue
    s, ex, ey = new
    done.add(s)
    if ex - 70 >= 53:
        tele += [(s + 6 + 6 * i, ex - 70, ey + 9) for i in range(16)]
    end = s + 1100
if end < dead + 1500:            # la caída del cuerpo, la PLM de la salida y unos cuadros más
    end = dead + 1500
    subprocess.run(['setarch', '-R', HLREF, ROM, IN, REF, str(FROM), str(FROM + end)], env=env(),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
open(os.path.join(WORK, 'setxy.txt'), 'w').write(env()['HL_SETXY'] + '\n')
print('muerto en el cuadro grabado %d; check:' % dead, flush=True)
r = subprocess.run([os.path.join(ROOT, 'host', 'check'), REF, ROM, '1', str(end), '0'], env=env(),
                   cwd=os.path.join(ROOT, 'host'), capture_output=True, text=True)
print((r.stdout + r.stderr).strip().splitlines()[-1])
