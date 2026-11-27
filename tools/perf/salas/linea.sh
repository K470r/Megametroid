#!/bin/sh
# MegaMetroid — cuadros perdidos cada 20 cuadros en una sala del visor (300 disparando quieta, 300 caminando a la
# izquierda y disparando), como todas.sh. Uso: linea.sh índice     (work/perf/rom.bin y sym.txt de todas.sh)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
W=${W:-$ROOT/work/perf}; I=$1
A=$(awk '$3=="viewer_debug_next"{print $1}' $W/sym.txt); A=$((0x$A & 0xFFFF))
{ printf "hold 11700 -\npoke %X %X 2\nhold 10 S\nhold 3 SR\nhold 3 -\nhold 400 -\n" $A $I; for k in $(seq 0 30); do printf "dumpram $W/t${I}_%02d.bin\nhold 20 %s\n" $k $([ $k -lt 15 ] && echo X || echo LX); done; } > $W/t$I.txt
$ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $W/rom.bin $W/t$I.txt >/dev/null 2>&1
python3 - $W $I <<'PY'
import sys
W,I=sys.argv[1],sys.argv[2]
syms={l.split()[2]:int(l.split()[0],16)&0xFFFF for l in open(W+'/sym.txt') if len(l.split())>=3}
def g(k,n): d=open('%s/t%s_%02d.bin'%(W,I,k),'rb').read(); a=syms[n]; return d[a]|d[a+1]<<8
f=[g(k,'md_frame_count') for k in range(31)]
print(I,'quieta:',' '.join(str(20-((f[k+1]-f[k])&0xFFFF)) for k in range(15)),'| caminando:',' '.join(str(20-((f[k+1]-f[k])&0xFFFF)) for k in range(15,30)))
PY
rm -f $W/t${I}_*.bin $W/t$I.txt
