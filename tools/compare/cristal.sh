#!/bin/sh
# MegaMetroid — el destello de cristal lado a lado con el SNES (docs/69).
#
# En el Parlor (puerta 8916) Samus en bola en (1200, 187), con 28 de energía (40 en la Genesis, ver HL_POKE_AT), sin
# tanques de reserva y 10 misiles / 10 superbombas / 11 bombas de poder (una se gasta al ponerla). La entrada
# (tools/snes_ref/ceres_bot/in_crystal.txt) pone la bomba de poder y mantiene L+R+abajo+X hasta que termina.
# Salida: work/crystal/lado (pares Genesis | SNES con mont.py).
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
P="{12743,0x9C6,10},{12743,0x9C8,10},{12743,0x9CA,10},{12743,0x9CC,10},{12743,0x9CE,11},{12743,0x9D0,11},{12843,0x9C2,40}"
export HL_SETXY_SCROLL=1 HL_POKE_AT=100:9C2:28 HL_ITEMS=1004 HL_POKE=9C6:A,9C8:A,9CA:A,9CC:A,9CE:B,9D0:B,9D2:3
WORK=${WORK:-$ROOT/work/crystal/lado} PAT=$ROOT/tools/snes_ref/ceres_bot/in_crystal.txt DOOR=8916 X=1200 Y=187 \
  XY_N=150 GOD=0 SNES_DESDE=23900 SNES_HASTA=24300 GEN_DESDE=13000 GEN_N=1600 \
  EXTRA_FL="-DMD_TEST_SETXY_SCROLL -DMD_TEST_ITEMS=0x1004 -DMD_TEST_HUD_ITEM=3 -DMD_TEST_POKES=$P" \
  sh $ROOT/tools/compare/lado_a_lado.sh
echo "pares: cd ${WORK:-$ROOT/work/crystal/lado} && python3 $ROOT/tools/compare/mont.py pares.png 13518 13584 13616 13645 13705 13855"
