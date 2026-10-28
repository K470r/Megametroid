#!/bin/sh
# powerbomb.sh: bombas de poder (HudSelectionHandler_MorphBall $90:BF9D, ProjPreInstr_PowerBomb $90:B0AE,
# PowerBomb_Func3 $90:C157, bloques BlockFunc_A06A $94:A06A, enemigos ProcessEnemyPowerBombInteraction $A0:A306; la
# explosión de HDMA ya estaba) contra el SNES (docs/56). Sala 9D19 (puerta 8DAE) con enemigos, 10 bombas de poder
# elegidas (HL_POKE 9CE/9D0/9D2); entrada in_z2.txt + in_powerbomb.txt (bola y tres bombas de poder).
# Uso: sh tools/rooms/powerbomb.sh   (WORK: carpeta temporal; la grabación se borra al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/powerbomb}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_powerbomb.txt > $WORK/in.txt
export HL_GOD=1 HL_ENEMIES=1 HL_WARP=21:8DAE HL_ITEMS=1004 HL_POKE=9CE:A,9D0:A,9D2:3
setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $WORK/ref.bin 23500 2>&1 | grep -E 'inicio|salto|cuadros' || true
(cd $ROOT/host && ./check $WORK/ref.bin ../sm/sm.smc 1 30000 0) | tail -3
[ -n "$KEEP" ] || rm -f $WORK/ref.bin
