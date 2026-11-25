#!/bin/sh
# MegaMetroid — Kraid lado a lado con el SNES (docs/59, README punto 8).
# Samus entra a la sala de Kraid (A59F) por la puerta de la izquierda (91B6) con 50 misiles y 20 supermisiles
# (ARMA: 1 misiles, 2 supermisiles; el ítem del HUD) y dispara hacia la derecha mientras Kraid sube y pelea.
# Uso: ARMA=1 [GRANDE=1] sh tools/compare/kraid.sh   (WORK: work/kraid/m1; SNES_DESDE/SNES_HASTA/GEN_DESDE/GEN_N como lado_a_lado)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
A=${ARMA:-1}
W=${WORK:-$ROOT/work/kraid/m$A${GRANDE:+g}}
mkdir -p $W
{ echo "300 -"; for i in $(seq 1 ${DISPAROS:-60}); do echo "2 X"; echo "18 -"; done; } > $W/in_pat.txt
P="{12743,0x9C6,50},{12743,0x9C8,50},{12743,0x9CA,20},{12743,0x9CC,20}"
# GRANDE=1: Kraid se hace grande (su vida, 7E:0F8C, a 0x35C en el cuadro 1100 de la grabación: docs/59), con las
# plataformas de la panza
HPA=""
if [ -n "$GRANDE" ]; then P="$P,{13843,0xF8C,0x35C}"; HPA="1100:F8C:35C"; fi
# (la puerta deja a Samus en x = -41, adentro de la puerta: se la pone en la sala, en el piso, con la cámara siguiéndola)
HL_POKE_AT=$HPA HL_SETXY_SCROLL=1 HL_POKE=9C6:32,9C8:32,9CA:14,9CC:14,9D2:$A HL_ITEMS=1004 \
WORK=$W PAT=$W/in_pat.txt DOOR=91B6 X=80 Y=443 XY_N=170 GOD=1 \
  SNES_DESDE=${SNES_DESDE:-23600} SNES_HASTA=${SNES_HASTA:-25000} GEN_DESDE=${GEN_DESDE:-13000} GEN_N=${GEN_N:-1500} \
  EXTRA_FL="-DMD_TEST_SETXY_SCROLL -DMD_TEST_GOD -DMD_TEST_ITEMS=0x1004 -DMD_TEST_HUD_ITEM=$A -DMD_TEST_POKES=$P" \
  sh $ROOT/tools/compare/lado_a_lado.sh
