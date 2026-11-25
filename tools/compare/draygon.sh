#!/bin/sh
# MegaMetroid — Draygon lado a lado con el SNES (docs/59, README punto 8).
# La pelea (DA60) por la puerta A924 con el patrón draygon_in.txt y las variables del Golden Torizo (rayos de onda,
# hielo, plasma y carga; 999 de energía). Uso: sh tools/compare/draygon.sh   (WORK: work/draygon)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HL_POKE=9A6:100B,9A8:100B,9C2:3E7,9C4:3E7 PAT=$ROOT/tools/compare/draygon_in.txt DOOR=A924 X=100 Y=400 \
WORK=${WORK:-$ROOT/work/draygon} GOD=1 \
  SNES_DESDE=${SNES_DESDE:-23600} SNES_HASTA=${SNES_HASTA:-26000} GEN_DESDE=${GEN_DESDE:-13000} GEN_N=${GEN_N:-2500} \
  EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_ITEMS=0 -DMD_TEST_BEAMS=0x100B -DMD_TEST_POKES={12764,0x9C2,0x3E7},{12764,0x9C4,0x3E7}" \
  sh $ROOT/tools/compare/lado_a_lado.sh
