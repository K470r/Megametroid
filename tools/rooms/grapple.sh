#!/bin/sh
# grapple.sh: rayo gancho contra el SNES (docs/56). Sala 9D19 (puerta 8DAE): bloques de gancho en el techo (fila 43,
# columnas 45-53). Gancho elegido (HL_ITEMS=4004, HL_POKE 9D2:4).
#   columpio:  Samus en el aire debajo de los bloques (784, 740); in_grapple.txt (enganchar arriba, columpiarse, alargar
#              y acortar, soltarse)
#   enemigos:  con enemigos, desde la puerta; in_grapple2.txt (disparar el gancho en todas las direcciones)
# Uso: sh tools/rooms/grapple.sh   (WORK: carpeta temporal; las grabaciones se borran al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/grapple}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
run() {   # run nombre entrada
  cat $BOT/in_z2.txt $BOT/$2 > $WORK/in_$1.txt
  setarch -R $HL $ROOT/sm/sm.smc $WORK/in_$1.txt $WORK/ref_$1.bin 23500 2>&1 | grep -E 'inicio|salto|cuadros' || true
  echo "== $1"; (cd $ROOT/host && ./check $WORK/ref_$1.bin ../sm/sm.smc 1 30000 0) | tail -3
  [ -n "$KEEP" ] || rm -f $WORK/ref_$1.bin
}
export HL_GOD=1 HL_WARP=21:8DAE HL_ITEMS=4004 HL_POKE=9D2:4
HL_SETXY=250:784:740 run columpio in_grapple.txt
HL_ENEMIES=1 run enemigos in_grapple2.txt
