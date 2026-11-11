#!/bin/sh
# MegaMetroid — paso 1 del plan de RAM (docs/60): qué bytes de la RAM del SNES se usan en las salas con enemigos.
# Graba en el SNES (hlref, HL_WARP como tools/rooms/lote.sh, Samus invencible y el patrón de disparos de mkwarp_e.py)
# lotes de 4 salas, de a P en paralelo, y cada grabación pasa por una tubería a ram_uso.py (no se guarda: cada lote
# son ~800 MB). Sale work/ram/snes_salas.npz (máscaras nz y ch de 7E:0000-7F:FFFF).
#
# Uso: medir_snes.sh [áreas...]      Variables: PER (cuadros por sala, 1500), P (lotes en paralelo, 4)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
W=$ROOT/work/ram; mkdir -p $W
PER=${PER:-1500}; P=${P:-4}
python3 $ROOT/tools/rooms/salas.py $ROOT/sm/sm.smc "$@" | awk '{print $2}' > $W/salas.list
rm -f $W/lote_*
split -l 4 -d -a 3 $W/salas.list $W/lote_
lote() {   # $1: archivo con las salas del lote
  B=$(basename $1); L="$(tr '\n' ' ' < $1)"
  python3 $ROOT/tools/rooms/mkwarp_e.py "$L" 12861 $PER $W/in_$B.txt > /dev/null
  REL=$(python3 $ROOT/tools/rooms/mkwarp_e.py "$L" 21 $PER /dev/null)
  rm -f $W/ref_$B; mkfifo $W/ref_$B
  python3 $ROOT/tools/ram/ram_uso.py $W/ref_$B $W/acc_$B.npz > $W/uso_$B.log 2>&1 &
  HL_GOD=1 HL_ENEMIES=1 HL_WARP=$REL setarch -R $ROOT/work/inputs/hlref $ROOT/sm/sm.smc $W/in_$B.txt \
    $W/ref_$B 23500 > /dev/null 2>&1
  wait; rm -f $W/ref_$B $W/in_$B.txt
  echo "$B: $L $(cat $W/uso_$B.log)"
}
export ROOT W PER
n=0
for f in $W/lote_[0-9]*; do   # de a P lotes (jobs en $(...) no ve los trabajos de esta shell: se cuentan aquí)
  lote $f &
  n=$((n + 1)); [ $((n % P)) -eq 0 ] && wait
done
wait
python3 - $W <<'P'
import sys, glob, numpy as np
w = sys.argv[1]
nz = ch = None; fr = 0
for f in sorted(glob.glob(w + '/acc_lote_*.npz')):
    z = np.load(f)
    nz = z['nz'] if nz is None else nz | z['nz']; ch = z['ch'] if ch is None else ch | z['ch']; fr += int(z['frames'])
np.savez_compressed(w + '/snes_salas.npz', nz=nz, ch=ch, frames=fr)
print('salas: %d cuadros; bytes usados 7E %d de 65536' % (fr, int((nz | ch)[:0x10000].sum())))
P
