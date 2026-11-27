#!/usr/bin/env python3
"""MegaMetroid — puertas de color y barreras con interruptor, SNES contra Genesis (README: colores de las puertas).

Para cada puerta o barrera de tools/rooms/puertas.py (una por lugar): en el SNES (hlref) el salto a la sala por su puerta
de entrada (HL_WARP), Samus a 3 bloques del PLM con la cámara siguiéndola y los scrolls abiertos (HL_SETXY,
HL_SETXY_SCROLL, HL_OPENSCROLLS) y la captura 120 cuadros después (HL_SHOTS, cada PASO cuadros); en la Genesis la ROM de
prueba (TEST=1, zebes_shoot) con el visor (viewer_debug_next) y lo mismo escribiendo la posición en la RAM.

Uso: puertas.py rom.smc gen.bin gen_sym.txt salida/ [desde hasta]
Salida: salida/pNNN.png (SNES | Genesis) y salida/indice.txt (número, sala, tipo, PLM, x, y).
"""
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "conv"))
import mkmd  # noqa: E402
from PIL import Image  # noqa: E402

smc, gen, gsym, out = sys.argv[1:5]
out = Path(out)
out.mkdir(parents=True, exist_ok=True)
rom = mkmd.Rom(smc)
PASO, FROM = 300, 23500
lst = subprocess.run([sys.executable, str(ROOT / "tools/rooms/puertas.py"), smc], capture_output=True, text=True).stdout
# todas las puertas de todas las salas: (sala de destino) -> [(puerta, x de la tapa, y de la tapa)]
def doors_of(ra):
    dl = 0x8F0000 | rom.w(0x8F0000 | (ra + 9))
    out_, i = [], 0
    while True:
        d = rom.w(dl + 2 * i)
        if d < 0x8000:
            return out_
        out_.append(d)
        i += 1
into = {}
for l in lst.split("\n"):
    pass
rooms_ = {int(l.split()[1].split(":")[0], 16) for l in lst.split("\n") if len(l.split()) >= 8}
todo_, seen_r = [0x91F8], set()
while todo_:
    ra_ = todo_.pop()
    if ra_ in seen_r:
        continue
    seen_r.add(ra_)
    for d in doors_of(ra_):
        dest = rom.w(0x830000 | d)
        if dest >= 0x8000:
            into.setdefault(dest, []).append((d, rom.b(0x830000 | (d + 4)), rom.b(0x830000 | (d + 5))))
            todo_.append(dest)
tg, seen = [], set()
for l in lst.split("\n"):
    p = l.split()
    if len(p) < 8:
        continue
    ra, door = (int(v, 16) for v in p[1].split(":"))
    x, y = int(p[5]), int(p[6])
    if (ra, x, y) in seen:
        continue
    seen.add((ra, x, y))
    wb = rom.b(0x8F0000 | (ra + 4)) * 16           # ancho de la sala en bloques
    plm = int(p[4], 16)
    vert = plm in (0xC84E, 0xC854, 0xC866, 0xC86C, 0xC87E, 0xC884, 0xC896, 0xC89C)
    if vert:                                       # puerta en el piso o el techo: Samus al lado, 3 bloques adentro
        sx, sy = x * 16 + 8, (y + 4 if y < 4 else y - 3) * 16
    else:
        sx, sy = (x + 3 if x < wb // 2 else x - 3) * 16 + 8, y * 16 + 40
    # la puerta que entra a la sala por esta tapa (o la más cercana: las barreras)
    cand = into.get(ra) or [(door, x, y)]
    door = min(cand, key=lambda c: abs(c[1] - x) + abs(c[2] - y))[0]
    tg.append(dict(ra=ra, door=door, kind=p[3], plm=plm, x=x, y=y, prm=p[7], sx=sx, sy=sy))
if os.environ.get("PUERTAS_SALAS"):                 # objetivos a mano: "sala:x:y,..." (hex:bloques)
    tg = []
    for e in os.environ["PUERTAS_SALAS"].split(","):
        ra, x, y = int(e.split(":")[0], 16), int(e.split(":")[1]), int(e.split(":")[2])
        door = min(into[ra], key=lambda c: abs(c[1] - x) + abs(c[2] - y))[0]
        tg.append(dict(ra=ra, door=door, kind="a_mano", plm=0, x=x, y=y, prm="-", sx=0, sy=0))
a0, a1 = (int(sys.argv[5]), int(sys.argv[6])) if len(sys.argv) > 6 else (0, len(tg))
tg = tg[a0:a1]
with open(out / "indice.txt", "w") as f:
    for k, t in enumerate(tg):
        f.write("%03d %04X %-22s %04X %3d %3d %s\n" % (a0 + k, t["ra"], t["kind"], t["plm"], t["x"], t["y"], t["prm"]))

# SNES, en tandas de 60 saltos (hlref: 64 como mucho). Los cuadros de HL_WARP/HL_SETXY se cuentan desde el inicio de
# la grabación (el primer cuadro desde FROM en el juego), el de HL_SHOTS es el de la entrada: una corrida corta lo da
bot = (ROOT / "tools/snes_ref/ceres_bot/in_z2.txt").read_text()
(out / "in.txt").write_text(bot.rstrip("\n") + "\n600 -\n")
r_ = subprocess.run(["setarch", "-R", str(ROOT / "work/inputs/hlref"), smc, str(out / "in.txt"), "/dev/null", str(FROM)],
                    capture_output=True, text=True)
start = int(re.search(r"inicio en el cuadro (\d+)", r_.stderr).group(1))
for c0 in range(0, len(tg), 60):
    ch = tg[c0:c0 + 60]
    inp = out / "in.txt"
    inp.write_text(bot.rstrip("\n") + "\n%d -\n" % (len(ch) * PASO + 2000))
    warp = ",".join("%d:%04X" % (21 + k * PASO, t["door"]) for k, t in enumerate(ch))
    sxy = ",".join("%d:%d:%d" % (21 + k * PASO + 170, t["sx"], t["sy"]) for k, t in enumerate(ch))
    first = start + 21 + 290
    env = dict(os.environ, HL_WARP=warp, HL_GOD="1", HL_ENEMIES="1",
               HL_SHOTS="%d,%d,%d,%s" % (first, PASO, first + (len(ch) - 1) * PASO, out / ("s%03d_" % (a0 + c0))))
    subprocess.run(["setarch", "-R", str(ROOT / "work/inputs/hlref"), smc, str(inp), "/dev/null", str(FROM)], env=env,
                   capture_output=True)

# Genesis: el visor
syms = {l.split()[2]: int(l.split()[0], 16) & 0xFFFF for l in open(gsym) if len(l.split()) >= 3}
vt = (ROOT / "res/gen/engine/tables.c").read_text()
vr = [int(a, 16) for a, _ in re.findall(r'\{0x([0-9A-F]+), 0x([0-9A-F]+)\}', re.search(r'kViewerRooms\[\d+\] = \{(.*?)\};', vt).group(1))]
# (una corrida por objetivo, desde el arranque: en una sola corrida, Samus a veces cruza otra puerta de la sala
# anterior en el momento del salto y la captura sale de otra sala)
from concurrent.futures import ThreadPoolExecutor


def gen_one(k, t):
    sc = ["hold 11700 -", "poke %X %X 2" % (syms["door_def_ptr"], t["door"]),
          "poke %X E17D 2" % syms["door_transition_function"], "poke %X 9 2" % syms["game_state"], "hold 420 -",
          "shot %s" % (out / ("g%03d.ppm" % (a0 + k)))]
    f = out / ("gen%03d.txt" % (a0 + k))
    f.write_text("\n".join(sc) + "\n")
    subprocess.run([str(ROOT / "tools/bin/retrorun"), str(ROOT / "tools/bin/genesis_plus_gx_libretro.so"), gen, str(f)],
                   capture_output=True)


with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
    list(ex.map(lambda kt: gen_one(*kt), enumerate(tg)))

# pares
shots = sorted(out.glob("s*_*.ppm"))
for k, t in enumerate(tg):
    c0 = (k // 60) * 60
    sf = out / ("s%03d_%05d.ppm" % (a0 + c0, start + 21 + 290 + (k - c0) * PASO))
    gf = out / ("g%03d.ppm" % (a0 + k))
    if not sf.exists() or not gf.exists():
        print("falta", a0 + k, sf.exists(), gf.exists())
        continue
    s, g = Image.open(sf).convert("RGB"), Image.open(gf).convert("RGB")
    im = Image.new("RGB", (s.width + g.width + 4, max(s.height, g.height)))
    im.paste(s, (0, 0)); im.paste(g, (s.width + 4, 0))
    im.save(out / ("p%03d.png" % (a0 + k)))
print("listo:", len(tg), "objetivos en", out)
