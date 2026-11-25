#!/bin/sh
# MegaMetroid — perfil del 68000 en una sala del visor (600 cuadros disparando, como todas.sh), por función.
# Uso: perfil.sh índice [n]      (usa work/perf/rom.bin y sym.txt de todas.sh; núcleo tools/bin/gpgx_prof)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
W=${W:-$ROOT/work/perf}; I=$1
A=$(awk '$3=="viewer_debug_next"{print $1}' $W/sym.txt); A=$((0x$A & 0xFFFF))
printf "hold 11700 -\npoke %X %X 2\nhold 3 S\nhold 3 SR\nhold 3 -\nhold 400 -\n" $A $I > $W/pa$I.txt
cp $W/pa$I.txt $W/pb$I.txt; printf "hold 300 X\nhold 300 LX\n" >> $W/pb$I.txt
for x in a b; do MM_PROF=$W/p$x$I.bin $ROOT/tools/bin/retrorun $ROOT/tools/bin/gpgx_prof/genesis_plus_gx_libretro.so $W/rom.bin $W/p$x$I.txt >/dev/null 2>&1; done
python3 - $W $I <<'PY'
import struct,sys
W,I=sys.argv[1],sys.argv[2]
a=open('%s/pa%s.bin'%(W,I),'rb').read(); b=open('%s/pb%s.bin'%(W,I),'rb').read()
A=struct.unpack('<%dI'%(len(a)//4),a); B=struct.unpack('<%dI'%(len(b)//4),b)
open('%s/pd%s.bin'%(W,I),'wb').write(struct.pack('<%dI'%len(A),*[max(0,y-x) for x,y in zip(A,B)]))
PY
python3 $ROOT/tools/emu_harness/prof_report.py $W/pd$I.bin $W/sym.txt ${2:-40}
