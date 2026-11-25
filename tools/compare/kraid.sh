#!/bin/sh
# MegaMetroid — Kraid lado a lado con el SNES (docs/59, README punto 8).
# Samus entra a la sala de Kraid (A59F) por la puerta de la izquierda (91B6) con 50 misiles y 20 supermisiles
# (ARMA: 1 misiles, 2 supermisiles; el ítem del HUD) y dispara hacia la derecha mientras Kraid sube y pelea.
# Uso: ARMA=1 sh tools/compare/kraid.sh   (WORK: work/kraid/m1; SNES_DESDE/SNES_HASTA/GEN_DESDE/GEN_N como lado_a_lado)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
A=${ARMA:-1}
W=${WORK:-$ROOT/work/kraid/m$A}
mkdir -p $W
{ echo "300 -"; for i in $(seq 1 60); do echo "2 X"; echo "18 -"; done; } > $W/in_pat.txt
P="{12743,0x9C6,50},{12743,0x9C8,50},{12743,0x9CA,20},{12743,0x9CC,20}"
HL_POKE=9C6:32,9C8:32,9CA:14,9CC:14,9D2:$A HL_ITEMS=1004 \
WORK=$W PAT=$W/in_pat.txt DOOR=91B6 X=0 Y=0 XY_N=999999 GOD=1 \
  SNES_DESDE=${SNES_DESDE:-23600} SNES_HASTA=${SNES_HASTA:-25000} GEN_DESDE=${GEN_DESDE:-13000} GEN_N=${GEN_N:-1500} \
  EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_ITEMS=0x1004 -DMD_TEST_HUD_ITEM=$A -DMD_TEST_POKES=$P" \
  sh $ROOT/tools/compare/lado_a_lado.sh
