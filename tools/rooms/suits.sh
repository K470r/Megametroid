#!/bin/sh
# suits.sh: tomar el traje Varia y el de gravedad contra el SNES (docs/56): rayo de luz (objeto de HDMA $91:D5A2 /
# D67A y sus pre-instrucciones $88:E026-E3B5), cambio de traje y de paleta.
#   Varia:    sala A6E2 (puerta 91DA), Samus en (60, 171); entrada in_varia.txt (disparar a la esfera, saltar
#             encima, cerrar la caja de mensaje).
#   Gravedad: sala CE40 (puerta 8A06) con Phantoon muerto (HL_POKE D82A:0100, bit 1 del área 3), Samus en (60, 139);
#             entrada in_gravity.txt (agachada, disparar, caminar hasta el objeto).
# Uso: sh tools/rooms/suits.sh   (WORK: carpeta temporal; las grabaciones se borran al terminar)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/suits}
HL=${HL:-$ROOT/work/inputs/hlref}
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK
run() {   # run nombre entrada
  cat $BOT/in_z2.txt $BOT/$2 > $WORK/in_$1.txt
  setarch -R $HL $ROOT/sm/sm.smc $WORK/in_$1.txt $WORK/ref_$1.bin 23500 2>&1 | grep -E 'inicio|salto|cuadros' || true
  echo "== $1"; (cd $ROOT/host && ./check $WORK/ref_$1.bin ../sm/sm.smc 1 30000 0) | tail -3
  [ -n "$KEEP" ] || rm -f $WORK/ref_$1.bin
}
export HL_GOD=1 HL_ITEMS=0004
HL_WARP=21:91DA HL_SETXY=200:60:171 run varia in_varia.txt
HL_WARP=21:8A06 HL_SETXY=200:60:139 HL_POKE=D82A:0100 run gravity in_gravity.txt
