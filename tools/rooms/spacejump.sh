#!/bin/sh
# spacejump.sh: salto espacial con el tornillo (saltos encadenados hacia la derecha y la izquierda) contra el SNES
# (docs/55). Sala 91F8 (Landing Site, entrando por la puerta 8946), Samus en el suelo abierto de abajo (720, 1211)
# con HL_ITEMS=230E. Entrada: in_z2.txt + in_spacejump.txt (A 28 cuadros, soltar 34: el siguiente A cae en la
# ventana de velocidad del salto espacial, $90:A436).
# Uso: sh tools/rooms/spacejump.sh   (WORK: carpeta temporal; la grabación se borra al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/spacejump}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_spacejump.txt > $WORK/in.txt
export HL_GOD=1 HL_WARP=21:8946 HL_SETXY=200:720:1211 HL_ITEMS=230E
setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $WORK/ref.bin 23500 2>&1 | grep -E 'inicio|salto|cuadros' || true
(cd $ROOT/host && ./check $WORK/ref.bin ../sm/sm.smc 1 30000 0) | tail -3
[ -n "$KEEP" ] || rm -f $WORK/ref.bin
