#!/bin/sh
# shinespark.sh: speed booster, shinespark (vertical, horizontal, diagonal; choque y ecos), tornillo, bola resorte y
# los ecos al frenar, contra el SNES (docs/56). Sala 9CB3 (Brinstar, la del Dachora: pasillo largo abajo y el pozo
# encima), entrando por la puerta 8DD2; Samus con speed booster, salto espacial, hi-jump, tornillo, bola y bola
# resorte (HL_ITEMS=230E) y llevada al pasillo de abajo antes de cada tramo (HL_SETXY). Entrada: in_z2.txt (hasta
# Zebes) + in_shinespark.txt (correr, agacharse con el brillo, saltar con A, A+<, R+A; saltos con el tornillo; bola
# resorte; correr y frenar).
# Uso: sh tools/rooms/shinespark.sh   (WORK: carpeta temporal; la grabación pesa ~350 MB y se borra al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/shinespark}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
cat $BOT/in_z2.txt $BOT/in_shinespark.txt > $WORK/in.txt
export HL_GOD=1 HL_WARP=21:8DD2 HL_ITEMS=230E
export HL_SETXY=200:200:1707,760:1150:1707,1150:200:1707,1450:300:1707,1800:300:1707
setarch -R $HL $ROOT/sm/sm.smc $WORK/in.txt $WORK/ref.bin 23500 2>&1 | grep -E 'inicio|salto|cuadros' || true
(cd $ROOT/host && ./check $WORK/ref.bin ../sm/sm.smc 1 30000 0) | tail -3
[ -n "$KEEP" ] || rm -f $WORK/ref.bin
