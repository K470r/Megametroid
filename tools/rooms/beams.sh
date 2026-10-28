#!/bin/sh
# beams.sh: los rayos de onda, hielo, spazer y plasma y sus combinaciones (con el rayo cargado) contra el SNES
# (docs/56). Sala 9D19 (la del Charge Beam, puerta 8DAE) con enemigos y la entrada del rayo cargado (in_z2.txt +
# in_chargebeam.txt: disparar y cargar en todas las direcciones, en el aire, bombas en abanico). Una grabación por
# combinación (equipped_beams/collected_beams en 7E:09A6/09A8 con HL_POKE).
# Uso: sh tools/rooms/beams.sh [combinaciones en hex]   (por omisión las 11 válidas con el rayo cargado)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/beams}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_chargebeam.txt > $WORK/in.txt
export HL_GOD=1 HL_ENEMIES=1 HL_WARP=21:8DAE HL_ITEMS=1004
for b in ${*:-1001 1002 1004 1008 1003 1005 1006 1007 1009 100A 100B}; do
  HL_POKE=9A6:$b,9A8:$b setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $WORK/ref_$b.bin 23500 > /dev/null 2>&1 || true
  echo "== rayos $b: $( (cd $ROOT/host && HL_POKE=9A6:$b,9A8:$b ./check $WORK/ref_$b.bin ../sm/sm.smc 1 30000 0) | tail -1)"
  (cd $ROOT/host && HL_POKE=9A6:$b,9A8:$b ./check $WORK/ref_$b.bin ../sm/sm.smc 1 30000 0) | grep 'SIN PORTAR' | sort | uniq -c | head -5
  [ -n "$KEEP" ] || rm -f $WORK/ref_$b.bin
done
