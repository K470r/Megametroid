#!/bin/sh
# chargebeam.sh: rayo cargado y bombas en abanico contra el SNES (docs/49). Sala 9D19 (Brinstar, la del Charge Beam,
# por la puerta 8DAE) con la bola, las bombas y el rayo cargado (HL_ITEMS / HL_POKE); entrada: in_z2.txt (hasta Zebes)
# + in_chargebeam.txt (cargar y soltar en varias direcciones, en el aire, sin llegar a 60, bombas en abanico).
# Uso: sh tools/rooms/chargebeam.sh   (WORK: carpeta temporal; la grabación pesa ~280 MB y se borra al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/chargebeam}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_chargebeam.txt > $WORK/in_cb.txt
export HL_GOD=1 HL_ENEMIES=1 HL_WARP=21:8DAE
HL_ITEMS=1004 HL_POKE=9A6:1000,9A8:1000 setarch -R $HL $ROOT/sm/sm.smc $WORK/in_cb.txt $WORK/ref_cb.bin 23500 2>&1 \
  | grep -E 'inicio|salto|cuadros' || true
(cd $ROOT/host && ./check $WORK/ref_cb.bin ../sm/sm.smc 1 30000 0) | tail -3
[ -n "$KEEP" ] || rm -f $WORK/ref_cb.bin
