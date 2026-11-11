#!/bin/sh
# MegaMetroid — paso 1 del plan de RAM (docs/60): qué variables de la Genesis se tocan en cada modo (game_state y
# md_zebes). Corre las ROM de prueba de la regresión (las compila medir_md.sh: work/ram/md_<escena>.bin; aquí se
# vuelven a compilar) con la RAM entera por cuadro por una tubería a md_ram_uso.py. Sale work/ram/md_<escena>.npz.
#
# Uso: md_vars.sh [escena...]       (sin nada: intro ceres zebes)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
W=$ROOT/work/ram; mkdir -p $W
RETRORUN=${RETRORUN:-$ROOT/tools/bin/retrorun}
CORE=${CORE:-$ROOT/tools/bin/genesis_plus_gx_libretro.so}
for sc in ${*:-intro ceres zebes}; do
  case $sc in
    intro) I=intro.s; E="TEST_INTRO=1"; FR=9000 ;;
    ceres) I=full.s; E=""; FR=10900 ;;
    zebes) I=zebes_shoot.s; E=""; FR=11400 ;;
    *) echo "escena desconocida: $sc"; exit 2 ;;
  esac
  cp $ROOT/tests/regress/inputs/$I $ROOT/res/gen/md/test_input.s
  env TEST=1 $E make -C $ROOT rom > $W/build_$sc.log 2>&1
  cp $ROOT/out/rom.bin $W/md_$sc.bin; m68k-linux-gnu-nm $ROOT/out/rom.out > $W/md_$sc.sym
  rm -f $W/md_$sc.npz $W/md_$sc.fifo; mkfifo $W/md_$sc.fifo
  printf "trace $W/md_$sc.fifo 0000:65536\nhold $FR -\nuntrace\n" > $W/md_$sc.cmd
  python3 $ROOT/tools/ram/md_ram_uso.py $W/md_$sc.fifo $W/md_$sc.sym $W/md_$sc.npz &
  $RETRORUN $CORE $W/md_$sc.bin $W/md_$sc.cmd > /dev/null 2>&1
  wait; rm -f $W/md_$sc.fifo
done
