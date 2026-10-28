#!/bin/sh
# MegaMetroid — capturas lado a lado SNES / Genesis de una sala, cuadro a cuadro (docs/51).
#
# La entrada es la del bot hasta Zebes (tools/snes_ref/ceres_bot/in_z2.txt) más un patrón propio ($PAT). En el SNES
# (hlref) y en la Genesis (ROM de prueba TEST=1 con MD_TEST_WARP_AT / MD_TEST_SETXY_AT, md_main.c) se salta a la
# sala por la puerta $DOOR y se pone a Samus en ($X, $Y), en el mismo cuadro de la entrada: la lógica es la misma, así
# que cada cuadro del motor de la Genesis se compara con el mismo cuadro del SNES.
#
# Uso: lado_a_lado.sh            (por omisión, la pelea con el Bomb Torizo: puerta 8BC2 y torizo_in.txt)
#      mont.py salida.png e1 e2 ...  (en $WORK: pares Genesis | SNES en los cuadros e1, e2... de la Genesis)
# Variables: PAT, DOOR, X, Y, WORK (work/lado), SNES_DESDE/SNES_HASTA (cuadros del SNES a capturar),
#            GEN_DESDE/GEN_N (cuadros de la Genesis a capturar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HERE=$ROOT/tools/compare
PAT=${PAT:-$HERE/torizo_in.txt}
DOOR=${DOOR:-8BC2}
X=${X:-60}; Y=${Y:-160}
WORK=${WORK:-$ROOT/work/lado}
FROM=23500                       # primer cuadro grabado por hlref (en Zebes, estado 8)
WARP_N=21; XY_N=170              # cuadros grabados (desde FROM) del salto y de la posición
T0=10757                         # cuadro de la entrada del primer cuadro del motor (estado inicial: ref_full, cuadro 97)
mkdir -p $WORK/snes $WORK/gen
cat $ROOT/tools/snes_ref/ceres_bot/in_z2.txt $PAT > $WORK/in.txt
# SNES
HL_WARP=$WARP_N:$DOOR HL_SETXY=$XY_N:$X:$Y HL_ENEMIES=1 HL_GOD=1 \
HL_SHOTS=${SNES_DESDE:-24000},1,${SNES_HASTA:-31400},$WORK/snes/a \
  setarch -R $ROOT/work/inputs/hlref $ROOT/sm/sm.smc $WORK/in.txt /dev/null $FROM 2>&1 | grep -E 'cuadros' || true
# Genesis: la misma entrada desde el estado inicial
python3 - $WORK/in.txt $T0 $ROOT/res/gen/md/test_input.s <<'P'
import sys
B = {'B': 1, 'Y': 2, 's': 4, 'S': 8, 'U': 16, 'D': 32, '<': 64, '>': 128, 'A': 256, 'X': 512, 'L': 1024, 'R': 2048}
SN = [0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10]
v = []
for l in open(sys.argv[1]):
    p = l.split()
    if not p:
        continue
    x = 0
    for ch in (p[1] if len(p) > 1 else ''):
        x |= B.get(ch, 0)
    v += [sum(m for i, m in enumerate(SN) if x & (1 << i))] * int(p[0])
v = v[int(sys.argv[2]):]
with open(sys.argv[3], 'w') as o:
    o.write("    .section .rodata\n    .align 2\n    .global kTestInputLen\nkTestInputLen:\n    .word %d\n"
            "    .global kTestInput\nkTestInput:\n" % len(v))
    for i in range(0, len(v), 16):
        o.write("    .word " + ",".join("0x%04X" % x for x in v[i:i + 16]) + "\n")
P
W=$((FROM + WARP_N - T0)); S=$((FROM + XY_N - T0))
env TEST=1 EXPFL="-DMD_TEST_WARP_AT=$W -DMD_TEST_WARP_DOOR=0x$DOOR -DMD_TEST_SETXY_AT=$S -DMD_TEST_SETXY_X=$X -DMD_TEST_SETXY_Y=$Y" \
  make -C $ROOT rom > $WORK/build.log 2>&1 || { echo "no compila: $WORK/build.log"; exit 1; }
cp $ROOT/out/rom.bin $WORK/gen.bin
A=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="md_shown_frame"{print substr($1,length($1)-3)}')
G0=${GEN_DESDE:-13500}
printf "trace $WORK/gen/tr.bin $A\nhold $G0 -\nshotseq $WORK/gen/g\nhold ${GEN_N:-8800} -\nunshotseq\n" > $WORK/gen.txt
$ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $WORK/gen.bin $WORK/gen.txt > /dev/null 2>&1
echo "$G0 $T0" > $WORK/gen/desde.txt
echo "listo: cd $WORK && python3 $HERE/mont.py pares.png <cuadros de la Genesis desde $G0>"
