#!/bin/sh
# MegaMetroid — prueba de regresión de la imagen (docs/41).
#
# Para cada escena aprobada compila la ROM de prueba (entrada grabada del SNES, cuadro a cuadro igual), la corre en
# Genesis Plus GX y anota, para cada cuadro del motor (el que se ve: md_shown_frame), un hash por línea de la imagen.
# Compara con golden/<escena>.rows.gz: así no depende de la velocidad (si un cuadro tarda más, se compara el mismo
# cuadro del motor).
#
# Corrimientos: las interrupciones horizontales (degradado de Ceres, colores debajo del HUD) caen una línea antes o
# después según qué instrucción estaba corriendo el 68000; si el código cambia de velocidad, en algunos cuadros una
# franja empieza una línea más arriba o más abajo. Un cuadro con tramos de 1-3 líneas distintas (hasta 4 tramos)
# se cuenta como corrimiento y no falla, salvo que la misma línea cambie en muchos cuadros (eso es un cambio de verdad,
# como una línea de otro color debajo del HUD).
#
# Uso: regress.sh [escena...]          comparar (escenas: intro ceres zebes; sin nada, todas)
#      UPDATE=1 regress.sh [escena...] aprobar: la imagen actual pasa a ser la de referencia
# Variables: RETRORUN, CORE (Genesis Plus GX libretro)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
MD=${MD:-$HERE/../../native/md}
RETRORUN=${RETRORUN:-$HERE/../../tools/bin/retrorun}
CORE=${CORE:-$HERE/../../tools/bin/genesis_plus_gx_libretro.so}
WORK=${WORK:-$HERE/../../work/regress}
mkdir -p $WORK
scenes=${*:-intro ceres zebes}
fail=0
for sc in $scenes; do
  case $sc in
    intro) IN=intro.s; EXTRA_ENV="TEST_INTRO=1"; FR=9000 ;;
    ceres) IN=full.s; EXTRA_ENV=""; FR=10900 ;;
    zebes) IN=zebes_shoot.s; EXTRA_ENV=""; FR=11400 ;;
    *) echo "escena desconocida: $sc"; exit 2 ;;
  esac
  cp $HERE/inputs/$IN $MD/gen/test_input.s
  (cd $MD && env SKIP_DATA=1 TEST=1 $EXTRA_ENV sh build.sh > $WORK/build_$sc.log 2>&1) || { echo "$sc: no compila (ver $WORK/build_$sc.log)"; exit 1; }
  cp $MD/out/rom.bin $WORK/$sc.bin
  A=$(m68k-linux-gnu-nm $MD/out/rom.out | awk '$3=="md_shown_frame"{print $1}')
  printf "rowhashseq $WORK/$sc.rows $A\nhold $FR -\nunrowhashseq\n" > $WORK/$sc.txt
  (cd $WORK && $RETRORUN $CORE $sc.bin $sc.txt > /dev/null 2>&1)
  G=$HERE/golden/$sc.rows.gz
  if [ -n "$UPDATE" ]; then gzip -9 -c $WORK/$sc.rows > $G; echo "$sc: aprobada"; continue; fi
  [ -f $G ] || { echo "$sc: sin referencia (UPDATE=1 para aprobar)"; fail=1; continue; }
  python3 - $G $WORK/$sc.rows $sc <<'PY' || fail=1
import sys, gzip, struct
def load(data):
    out, p = {}, 0
    while p + 6 <= len(data):
        f, v, h = struct.unpack_from('<3H', data, p); p += 6
        rows = data[p:p + 2 * h]; p += 2 * h
        if v >= 2 and v not in out:              # el primer cuadro de emulación que muestra cada cuadro del motor
            out[v] = rows
    return out
g = load(gzip.open(sys.argv[1]).read())
c = load(open(sys.argv[2], 'rb').read())
bad, shift, rowcount = [], [], {}
for f in sorted(g):
    if f not in c or g[f] == c[f]:
        continue
    n = len(g[f]) // 2
    d = [i for i in range(n) if g[f][2 * i:2 * i + 2] != c[f][2 * i:2 * i + 2]]
    runs = []
    for i in d:
        if runs and i == runs[-1][1] + 1: runs[-1][1] = i
        else: runs.append([i, i])
    if len(d) <= 8 and len(runs) <= 4 and all(b - a < 3 for a, b in runs):
        shift.append(f)
        for i in d: rowcount[i] = rowcount.get(i, 0) + 1
    else:
        bad.append(f)
steady = sorted(i for i, k in rowcount.items() if k > 0.2 * len(g))
miss = [f for f in g if f not in c]
def ranges(fs):
    rng = []
    for f in fs:
        if rng and f <= rng[-1][1] + 2: rng[-1][1] = f
        else: rng.append([f, f])
    return ', '.join('%d-%d' % (a, b) if a != b else str(a) for a, b in rng[:20])
msg = '%s: %d cuadros comparados, %d distintos' % (sys.argv[3], len(g), len(bad))
if bad: msg += ' en ' + ranges(bad)
if shift: msg += '; %d con corrimientos de una línea' % len(shift)
if steady: msg += '; LÍNEAS QUE CAMBIAN SIEMPRE: %s' % steady[:10]
if miss: msg += '; %d cuadros que no se vieron' % len(miss)
print(msg)
sys.exit(1 if bad or steady else 0)
PY
done
exit $fail
