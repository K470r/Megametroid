#!/bin/sh
# MegaMetroid — el final (estado 39) lado a lado SNES / Genesis: el despegue (despegue.sh) con la entrada de
# final_in.txt (como final_snes.sh). Cuadro grabado k del final = cuadro k + 23500 del SNES = cuadro k + 12743 de la
# Genesis (el motor es el mismo: cada cuadro de la Genesis se compara con el mismo del SNES).
# Uso: SNES_DESDE=.. SNES_HASTA=.. GEN_DESDE=.. GEN_N=.. sh tools/compare/final.sh   (WORK: work/final_lado)
#      HORAS=5 o 12: los otros finales (el tiempo de juego, 7E:09E0, en el SNES y en la Genesis)
HERE=$(cd "$(dirname "$0")" && pwd)
HP=""; [ -n "$HORAS" ] && HP=",{12755,0x9E0,$HORAS}"
HL_WARP_ALIGN=1 HL_SETXY_SCROLL=1 HL_POKE=D820:4000,D82C:0200,943:2,A5A:E0E6${HORAS:+,9E0:$(printf %04X $HORAS)} PAT=$HERE/final_in.txt DOOR=89B2 XY_N=200 X=1152 Y=1100 GOD=1 \
WORK=${WORK:-work/final_lado} \
EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_SETXY_SCROLL -DMD_TEST_POKES={12755,0xD820,0x4000},{12755,0xD82C,0x200},{12755,0x943,2},{12755,0xA5A,0xE0E6}$HP $DS_FL" \
sh $HERE/lado_a_lado.sh
