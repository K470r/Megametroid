#!/bin/sh
# MegaMetroid — el sonido de una sala de cada área contra el SNES (docs/55).
#
# Para cada sala: la entrada del bot hasta Zebes (in_z2.txt) más el patrón del rayo cargado (in_chargebeam.txt: disparar
# y cargar, saltar, bombas) y el salto a la sala por una puerta (HL_WARP en hlref y en host/check, MD_TEST_WARP_AT en la
# ROM de prueba, como tools/compare/lado_a_lado.sh):
#   - SNES: hlref graba; host/check con MM_SNDLOG da los pedidos al SPC y tools/snd/snesref.py los toca con el N-SPC;
#   - Genesis: la ROM de prueba (TEST=1) con la misma entrada, en Genesis Plus GX con RR_WAV.
# tools/snd/sndcmp.py compara la música (la Genesis con MD_SND_NOSFX, el SNES con solo los pedidos de música): el
# desfase entre los dos audios y la correlación de la envolvente y del croma (12 notas) cada 0,1 s; y cuenta los pedidos
# (también los efectos) sin sonido en la Genesis (sndcheck.py).
#
# Uso: sh tools/snd/salas.sh [nombre:puerta ...]   (por omisión una sala de cada área)
# Variables: WORK (work/snd/salas), N (cuadros desde el salto: 3000)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/snd/salas}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
N=${N:-3000}
FROM=23500                       # primer cuadro grabado por hlref (en Zebes, estado 8)
WARP_N=21                        # el salto: 21 cuadros grabados después
T0=10757                         # cuadro de la entrada del primer cuadro del motor (estado inicial: ref_full, cuadro 97)
SALAS=${*:-crateria:8A12 brinstar:8DAE norfair:9246 wrecked:89D6 maridia:A330 tourian:9222}
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_chargebeam.txt $BOT/in_chargebeam.txt > $WORK/in.txt
cp $ROOT/res/gen/md/test_input.s $WORK/test_input.orig.s
for s in $SALAS; do
  name=${s%%:*}; door=${s##*:}
  D=$WORK/$name; mkdir -p $D
  export HL_GOD=1 HL_ENEMIES=1 HL_WARP=$WARP_N:$door
  # SNES: la grabación, los pedidos y el audio
  setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $D/ref.bin $FROM > $D/hlref.log 2>&1 || true
  (cd $ROOT/host && MM_SNDLOG=$D/log.txt ./check $D/ref.bin ../sm/sm.smc 1 $((WARP_N + N)) 0 > $D/check.log 2>&1) || true
  unset HL_GOD HL_ENEMIES HL_WARP
  # Genesis: la misma entrada desde el estado inicial, con el salto en el mismo cuadro
  python3 $ROOT/tools/snd/mkinput.py $WORK/in.txt $T0 $ROOT/res/gen/md/test_input.s
  W=$((FROM + WARP_N - T0))
  env TEST=1 EXPFL="-DMD_TEST_WARP_AT=$W -DMD_TEST_WARP_DOOR=0x$door -DMD_SND_NOSFX" make -C $ROOT rom > $D/build.log 2>&1 \
    || { echo "$name: no compila ($D/build.log)"; continue; }
  cp $ROOT/out/rom.bin $D/gen.bin
  A=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="md_shown_frame"{print substr($1,length($1)-3)}')
  E=$(m68k-linux-gnu-nm $ROOT/out/rom.out | awk '$3=="md_snd_ev"{print substr($1,length($1)-3)}')
  printf "trace $D/tr.bin $A:2,$E:4\nhold $((W + N + 400)) -\n" > $D/gen.txt
  RR_WAV=$D/gen.wav $ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $D/gen.bin $D/gen.txt > /dev/null 2>&1
  python3 $ROOT/tools/snd/sndcmp.py $ROOT/sm/sm.smc $ROOT/res/gen/md/md_snd_data.s $D "$name" $((FROM - T0))
done
cp $WORK/test_input.orig.s $ROOT/res/gen/md/test_input.s
