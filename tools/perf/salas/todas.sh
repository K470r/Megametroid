#!/bin/sh
# MegaMetroid — cuadros perdidos en cada sala del visor (docs/44), con las rutas del proyecto.
# Compila la ROM de prueba con la entrada zebes_shoot, entra a cada sala (viewer_debug_next) y cuenta los cuadros del
# motor que faltan en 600 cuadros de la consola disparando (300 X, 300 LX), 400 cuadros después de entrar.
# Uso: todas.sh [índices]          (sin nada: las 255; 4 a la vez)   Salida: work/perf/salas.txt, una línea por sala:
#   índice sala perdidos quieta caminando puerta unimpl   (perdidos = quieta + caminando; si caminando cruzó una
#   puerta, la transición no cuenta: quieta x 2)
# Variables: NOBUILD=1 (usa out/rom.bin como está), J (en paralelo, 4), W (carpeta, work/perf)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
W=${W:-$ROOT/work/perf}; mkdir -p $W
if [ "$1" = "--una" ]; then
  I=$2; A=$(awk '$3=="viewer_debug_next"{print $1}' $W/sym.txt); A=$((0x$A & 0xFFFF))
  printf "hold 11700 -\npoke %X %X 2\nhold 3 S\nhold 3 SR\nhold 3 -\nhold 400 -\ndumpram $W/l${I}a.bin\nhold 300 X\ndumpram $W/l${I}m.bin\nhold 300 LX\ndumpram $W/l${I}b.bin\n" $A $I > $W/l$I.txt
  $ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $W/rom.bin $W/l$I.txt >/dev/null 2>&1
  python3 - $W $I <<'PY'
import sys
W,i=sys.argv[1],sys.argv[2]
syms={l.split()[2]:int(l.split()[0],16)&0xFFFF for l in open(W+'/sym.txt') if len(l.split())>=3}
def g(f,n): d=open(f,'rb').read(); a=syms[n]; return d[a]|d[a+1]<<8
f=lambda x: '%s/l%s%s.bin'%(W,i,x)
fa,fm,fb=[g(f(x),'md_frame_count') for x in 'amb']
ra,rb=g(f('a'),'room_ptr'),g(f('b'),'room_ptr')
q=300-((fm-fa)&0xFFFF); c=300-((fb-fm)&0xFFFF)
# columnas: índice sala perdidos(quieta+caminando, o solo quieta x2 si cruzó una puerta) quieta caminando cruzó unimpl
print(i,'%04X'%ra,q+(c if ra==rb else q),q,c,'puerta' if ra!=rb else '-',g(f('b'),'md_unimpl_count'))
PY
  rm -f $W/l${I}a.bin $W/l${I}m.bin $W/l${I}b.bin $W/l$I.txt
  exit 0
fi
if [ -z "$NOBUILD" ]; then
  cp $ROOT/tests/regress/inputs/zebes_shoot.s $ROOT/res/gen/md/test_input.s
  (cd $ROOT && TEST=1 make rom > $W/build.log 2>&1) || { echo "no compila: $W/build.log"; exit 1; }
fi
cp $ROOT/out/rom.bin $W/rom.bin; m68k-linux-gnu-nm $ROOT/out/rom.out > $W/sym.txt
for I in ${*:-$(seq 0 254)}; do echo $I; done | W=$W xargs -P ${J:-4} -I{} sh $0 --una {} > $W/salas.txt
sort -n $W/salas.txt -o $W/salas.txt
awk '{s+=$3; q+=$4} END {print "total perdidos", s, "(quieta", q ")", "en", NR, "salas"}' $W/salas.txt
