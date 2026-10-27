#!/bin/sh
# m7.sh [salida.csv]: cinemática de Ceres (escena ceres de la regresión): en cada cuadro de la consola, el cuadro del
# motor que se ve, la imagen del modo 7 que pide el motor (m7_want) y la que se ve (m7_shown), si se está armando
# otra (m7_busy, m7_step), las líneas del vblank al entrar y al salir de cine_vblank y los cuadros en que el cambio
# esperó al armado (md_m7_wait: tiene que quedar en 0) (MD_M7LOG). docs/50.
# Variables: XFL (más opciones de compilación, p. ej. "-DM7_NOP"), FR (cuadros de la consola, 10900)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RETRORUN=${RETRORUN:-$ROOT/tools/bin/retrorun}
CORE=${CORE:-$ROOT/tools/bin/genesis_plus_gx_libretro.so}
W=$ROOT/work/m7; mkdir -p $W
OUT=${1:-$W/m7.csv}
cp $ROOT/tests/regress/inputs/full.s $ROOT/res/gen/md/test_input.s
(cd $ROOT && TEST=1 EXPFL="-DMD_M7LOG $XFL" make rom > $W/build.log 2>&1) || { echo "no compila (ver $W/build.log)"; exit 1; }
cp $ROOT/out/rom.bin $W/m7.bin
sym() { m68k-linux-gnu-nm $ROOT/out/rom.out | awk -v n=$1 '$3==n{print substr($1,5)}'; }
T=""
for v in md_shown_frame md_frame_count m7_want m7_shown m7_busy m7_step md_m7_in md_m7_out md_m7_wait md_m7_start; do
  a=$(sym $v); [ -n "$a" ] || { echo "falta $v"; exit 1; }
  T="$T,$(printf '%X' $(( 0x$a & ~1 ))):2"
done
printf "trace $W/m7.tr ${T#,}\nhold ${FR:-10900} -\nuntrace\n" > $W/m7.txt
(cd $W && $RETRORUN $CORE m7.bin m7.txt > /dev/null 2>&1)
python3 - $W/m7.tr $OUT $(for v in m7_busy m7_step; do echo $(( 0x$(sym $v) & 1 )); done) <<'PY'
import sys, struct
d = open(sys.argv[1], 'rb').read(); odd = [int(x) for x in sys.argv[3:]]
names = 'shown_frame frame want shown busy step vb_in vb_out wait start'.split()
with open(sys.argv[2], 'w') as f:
    f.write('emu,' + ','.join(names) + '\n')
    for i in range(len(d) // 20):
        w = [d[20 * i + 2 * k] | d[20 * i + 2 * k + 1] << 8 for k in range(10)]
        w = [struct.unpack('<h', struct.pack('<H', x))[0] for x in w]
        for k, o in zip((4, 5), odd):            # bytes: alto (dirección par) o bajo (impar)
            w[k] = (w[k] & 0xFF) if o else (w[k] >> 8) & 0xFF
        f.write('%d,' % i + ','.join(map(str, w)) + '\n')
PY
echo "listo: $OUT"
