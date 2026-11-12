#!/bin/sh
# MegaMetroid — paso 1 del plan de RAM (docs/60), lado de la Genesis: cuánto usan de verdad el montón de SGDK y la
# pila (lo que queda arriba de _bend). Compila las ROM de prueba de la regresión (intro, ceres, zebes), las corre
# en Genesis Plus GX guardando cada cuadro la RAM desde _bend hasta el final y lo resume md_uso.py.
#
# Uso: medir_md.sh [escena...]      (sin nada: intro ceres zebes)     Sale: work/ram/md_<escena>.txt
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
W=$ROOT/work/ram; mkdir -p $W
RETRORUN=${RETRORUN:-$ROOT/tools/bin/retrorun}
CORE=${CORE:-$ROOT/tools/bin/genesis_plus_gx_libretro.so}
IN=$ROOT/tests/regress/inputs
for sc in ${*:-intro ceres zebes}; do
  case $sc in
    intro) I=intro.s; E="TEST_INTRO=1"; FR=9000 ;;
    ceres) I=full.s; E=""; FR=10900 ;;
    zebes) I=zebes_shoot.s; E=""; FR=11400 ;;
    *) echo "escena desconocida: $sc"; exit 2 ;;
  esac
  cp $IN/$I $ROOT/res/gen/md/test_input.s
  env TEST=1 $E make -C $ROOT rom > $W/build_$sc.log 2>&1
  cp $ROOT/out/rom.bin $W/md_$sc.bin
  B=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="_bend"{print $1}')
  A=$(python3 -c "print('%04X' % (int('$B',16) & 0xFFFE))"); N=$((0x10000 - 0x$A))
  printf "trace $W/md_$sc.trace $A:$N\nhold $FR -\nuntrace\n" > $W/md_$sc.cmd
  $RETRORUN $CORE $W/md_$sc.bin $W/md_$sc.cmd > /dev/null 2>&1
  python3 $ROOT/tools/ram/md_uso.py $W/md_$sc.trace $A $sc | tee $W/md_$sc.txt
  [ -n "$KEEP" ] || rm -f $W/md_$sc.trace
done
