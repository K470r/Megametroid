#!/bin/sh
# nave.sh [salida.csv]: vuelo de la nave en la intro (escena intro de la regresión): en cada cuadro de la consola, el
# cuadro del motor que se ve, el cuadro de la nave que pide el motor (ship_now), el que se ve (ship_shown), el que se
# prepara (ship_load) y los cuadros en que el cambio esperó a la carga (md_ship_wait: tiene que quedar en 0)
# (MD_M7LOG). docs/54. Variables: XFL (más opciones de compilación, p. ej. "-DMD_VB_NOPS=1"), FR (9000)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RETRORUN=${RETRORUN:-$ROOT/tools/bin/retrorun}
CORE=${CORE:-$ROOT/tools/bin/genesis_plus_gx_libretro.so}
W=$ROOT/work/m7; mkdir -p $W
OUT=${1:-$W/nave.csv}
cp $ROOT/tests/regress/inputs/intro.s $ROOT/res/gen/md/test_input.s
(cd $ROOT && TEST=1 TEST_INTRO=1 EXPFL="-DMD_M7LOG $XFL" make rom > $W/build.log 2>&1) || { echo "no compila (ver $W/build.log)"; exit 1; }
cp $ROOT/out/rom.bin $W/nave.bin
sym() { m68k-linux-gnu-nm $ROOT/out/rom.out | awk -v n=$1 '$3==n{print substr($1,5)}'; }
T=""
for v in md_shown_frame md_frame_count ship_now ship_shown ship_load md_ship_wait; do
  a=$(sym $v); [ -n "$a" ] || a=$(sym md_shown_frame)   # (una versión sin md_ship_wait: repite el cuadro)
  T="$T,$a:2"
done
printf "trace $W/nave.tr ${T#,}\nhold ${FR:-9000} -\nuntrace\n" > $W/nave.txt
(cd $W && $RETRORUN $CORE nave.bin nave.txt > /dev/null 2>&1)
python3 - $W/nave.tr $OUT <<'PY'
import sys, struct
d = open(sys.argv[1], 'rb').read()
with open(sys.argv[2], 'w') as f:
    f.write('emu,shown_frame,frame,now,shown,load,wait\n')
    for i in range(len(d) // 12):
        w = struct.unpack_from('<6h', d, 12 * i)
        f.write('%d,' % i + ','.join(map(str, w)) + '\n')
PY
echo "listo: $OUT"
