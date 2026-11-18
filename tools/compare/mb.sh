#!/bin/sh
# MegaMetroid — la pelea con Mother Brain lado a lado (docs/59): las variables de la prueba de motherbrain_in.txt.
# Uso: SNES_DESDE=.. SNES_HASTA=.. GEN_DESDE=.. GEN_N=.. sh tools/compare/mb.sh   (WORK: work/mb)
HERE=$(cd "$(dirname "$0")" && pwd)
HL_POKE=9A6:100B,9A8:100B,9C2:3E7,9C4:3E7,9C6:63,9C8:63,9CA:63,9CC:63,9CE:63,9D0:63,9D2:2,D820:4 \
HL_WARP_ALIGN=1 HL_POKE_AT=1800:FCC:12C,8450:FCC:64,8450:9C2:3E7 GOD=0 PAT=$HERE/motherbrain_in.txt DOOR=AAE0 X=28 Y=130 \
WORK=${WORK:-work/mb} \
EXTRA_FL="-DMD_TEST_ITEMS=0 -DMD_TEST_BEAMS=0x100B -DMD_TEST_AMMO=99 -DMD_TEST_HUD_ITEM=2 -DMD_TEST_POKES={12750,0xD820,4},{12764,0x9C2,0x3E7},{12764,0x9C4,0x3E7},{14543,0xFCC,0x12C},{21193,0xFCC,0x64},{21193,0x9C2,0x3E7} $MB_FL" \
sh $HERE/lado_a_lado.sh
