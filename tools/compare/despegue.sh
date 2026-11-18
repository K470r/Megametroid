#!/bin/sh
# MegaMetroid — el despegue de la huida de Zebes (lado a lado SNES / Genesis): a la sala del aterrizaje (91F8) por la
# puerta 89B2 con la bomba puesta (como huida.sh), Samus sobre la nave (1152, 1100; la cámara la sigue) y abajo:
# entra, la nave tiembla, larga el polvo, sube y se va (GunshipTop_17-21), y el estado 38.
# Uso: SNES_DESDE=.. SNES_HASTA=.. GEN_DESDE=.. GEN_N=.. sh tools/compare/despegue.sh   (WORK: work/despegue)
HERE=$(cd "$(dirname "$0")" && pwd)
HL_WARP_ALIGN=1 HL_SETXY_SCROLL=1 HL_POKE=D820:4000,D82C:0200,943:2,A5A:E0E6 PAT=$HERE/despegue_in.txt DOOR=89B2 XY_N=200 X=1152 Y=1100 GOD=1 \
WORK=${WORK:-work/despegue} \
EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_SETXY_SCROLL -DMD_TEST_POKES={12755,0xD820,0x4000},{12755,0xD82C,0x200},{12755,0x943,2},{12755,0xA5A,0xE0E6} $DS_FL" \
sh $HERE/lado_a_lado.sh
