#!/bin/sh
# MegaMetroid — prueba de los rayos X (lado a lado SNES / Genesis): Parlor (92FD, Crateria) por la puerta 8916, Samus
# bajo los bloques de bomba ocultos (61-66, 11), con los rayos X tomados y elegidos (9D2 = 5); xray_in.txt: correr (B)
# con arriba y abajo.
# Uso: SNES_DESDE=.. SNES_HASTA=.. GEN_DESDE=.. GEN_N=.. sh tools/compare/xray.sh   (WORK: work/xray)
HERE=$(cd "$(dirname "$0")" && pwd)
HL_ITEMS=8000 HL_POKE=9D2:5 PAT=$HERE/xray_in.txt DOOR=8916 X=${X:-1000} Y=${Y:-200} \
WORK=${WORK:-work/xray} \
EXTRA_FL="-DMD_TEST_ITEMS=0x8000 -DMD_TEST_HUD_ITEM=5 $XR_FL" \
sh $HERE/lado_a_lado.sh
