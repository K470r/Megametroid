#!/bin/sh
# MegaMetroid — el sonido de la intro y de Ceres hasta Zebes contra el SNES (docs/55), con las ROM de prueba de
# tests/regress (la entrada grabada del SNES): la Genesis con MD_SND_NOSFX (solo la música) en Genesis Plus GX con RR_WAV,
# el SNES con los pedidos de host/check (MM_SNDLOG) tocados por tools/snd/snesref.py; tools/snd/sndcmp.py compara cada
# pista desde su pedido (en orden) y cuenta los pedidos sin sonido.
# Uso: sh tools/snd/escenas.sh [intro] [ceres]
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/snd/escenas}
mkdir -p $WORK
cp $ROOT/res/gen/md/test_input.s $WORK/test_input.orig.s
for sc in ${*:-intro ceres}; do
  D=$WORK/$sc; mkdir -p $D
  case $sc in
    intro) IN=intro.s; EXTRA="TEST_INTRO=1"; FR=9000; REC=$ROOT/work/inputs/ref_intro.bin; R0=0 ;;
    ceres) IN=full.s; EXTRA=""; FR=10900; REC=$ROOT/work/inputs/ref_full.bin; R0=97 ;;
  esac
  (cd $ROOT/host && MM_SNDLOG=$D/log.txt ./check $REC ../sm/sm.smc $R0 99999 > $D/check.log 2>&1) || true
  cp $ROOT/tests/regress/inputs/$IN $ROOT/res/gen/md/test_input.s
  env TEST=1 $EXTRA EXPFL="-DMD_SND_NOSFX" make -C $ROOT rom > $D/build.log 2>&1 || { echo "$sc: no compila"; continue; }
  cp $ROOT/out/rom.bin $D/gen.bin
  A=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="md_shown_frame"{print substr($1,length($1)-3)}')
  E=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="md_snd_ev"{print substr($1,length($1)-3)}')
  printf "trace $D/tr.bin $A:2,$E:4\nhold $FR -\n" > $D/gen.txt
  RR_WAV=$D/gen.wav $ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $D/gen.bin $D/gen.txt > /dev/null 2>&1
  python3 $ROOT/tools/snd/sndcmp.py $ROOT/sm/sm.smc $ROOT/res/gen/md/md_snd_data.s $D "$sc" -100000
done
cp $WORK/test_input.orig.s $ROOT/res/gen/md/test_input.s
