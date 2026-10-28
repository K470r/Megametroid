#!/bin/sh
# sba.sh: ataques especiales (rayo cargado con la bomba de poder elegida: FireSba_FireWave/Ice/Spazer/Plasma
# $90:CD1A-CE98 y sus pre-instrucciones) contra el SNES (docs/55). Sala 9D19 (puerta 8DAE) con enemigos; 10 bombas de
# poder y la bomba de poder elegida (HL_POKE 9CE/9D0/9D2); entrada in_z2.txt + in_sba.txt (cargar 160 cuadros y
# soltar, a los dos lados). Una grabación por rayo (onda, hielo, spazer, plasma, con el rayo cargado).
# Uso: sh tools/rooms/sba.sh   (WORK: carpeta temporal; las grabaciones se borran al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/sba}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_sba.txt > $WORK/in.txt
export HL_GOD=1 HL_ENEMIES=1 HL_WARP=21:8DAE HL_ITEMS=1004
for b in 1001 1002 1004 1008; do
  export HL_POKE=9A6:$b,9A8:$b,9CE:A,9D0:A,9D2:3
  setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $WORK/ref_$b.bin 23500 > /dev/null 2>&1 || true
  echo "== rayos $b: $( (cd $ROOT/host && ./check $WORK/ref_$b.bin ../sm/sm.smc 1 30000 0) | tail -1)"
  [ -n "$KEEP" ] || rm -f $WORK/ref_$b.bin
done
