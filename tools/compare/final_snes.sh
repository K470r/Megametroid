#!/bin/sh
# MegaMetroid — referencia del SNES del final (estado 39: la huida en la nave, Zebes explota, el tiempo, los créditos,
# Samus y "SEE YOU NEXT MISSION"): el despegue de despegue.sh y 16000 cuadros más. Capturas cada $PASO cuadros en
# $WORK/snes; con REF=archivo, también la grabación de hlref (RAM por cuadro, para host/check).
# Uso: sh tools/compare/final_snes.sh   (WORK: work/final; DESDE/HASTA/PASO de las capturas)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/final}
mkdir -p $WORK/snes
cat $ROOT/tools/snes_ref/ceres_bot/in_z2.txt $ROOT/tools/compare/final_in.txt > $WORK/in.txt
HL_WARP_ALIGN=1 HL_SETXY_SCROLL=1 HL_POKE=D820:4000,D82C:0200,943:2,A5A:E0E6 HL_WARP=21:89B2 HL_SETXY=200:1152:1100 \
HL_ENEMIES=1 HL_GOD=1 HL_SHOTS=${DESDE:-24400},${PASO:-60},${HASTA:-40400},$WORK/snes/a \
  setarch -R $ROOT/work/inputs/hlref $ROOT/sm/sm.smc $WORK/in.txt ${REF:-/dev/null} 23500 > /dev/null 2>&1
echo "capturas en $WORK/snes"
