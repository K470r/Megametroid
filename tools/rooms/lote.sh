#!/bin/sh
# MegaMetroid — un lote de salas contra el SNES (docs/41): como gdbatch.sh, con las rutas del proyecto.
# Graba en el SNES (hlref, HL_WARP: transición de ascensor a cada sala) con Samus invencible y el patrón de disparos de
# mkwarp_e.py, compara con el motor en el PC (host/check, las mismas variables) y borra la grabación.
#
# Uso: lote.sh NOMBRE "SALA:PUERTA SALA:PUERTA ..."     (las salas y puertas: salas.py)
# Variables: PER (cuadros por sala, 1500), WORK (work/rooms), HL_ITEMS (objetos al empezar, en las dos)
# Salida: una línea "NOMBRE salas resultado del check; lo que se llamó sin portar" (y work/rooms/chk_NOMBRE.txt)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/rooms}
B=$1; PER=${PER:-1500}
mkdir -p $WORK
python3 $ROOT/tools/rooms/mkwarp_e.py "$2" 12861 $PER $WORK/in_$B.txt > /dev/null
REL=$(python3 $ROOT/tools/rooms/mkwarp_e.py "$2" 21 $PER /dev/null)
HL_GOD=1 HL_ENEMIES=1 HL_WARP=$REL setarch -R $ROOT/work/inputs/hlref $ROOT/sm/sm.smc $WORK/in_$B.txt \
  $WORK/ref_$B.bin 23500 > $WORK/rec_$B.log 2>&1 || true
cd $ROOT/host
HL_GOD=1 HL_ENEMIES=1 HL_WARP=$REL ./check $WORK/ref_$B.bin ../sm/sm.smc 1 ${UNTIL:-60000} 0 > $WORK/chk_$B.txt 2>&1 || true
rm -f $WORK/ref_$B.bin
echo "$B $2 $(tail -1 $WORK/chk_$B.txt) $(grep -h 'SIN PORTAR' $WORK/chk_$B.txt | sed 's/^cuadro [0-9]*: SIN PORTAR: //' | sort -u | tr '\n' ';')"
