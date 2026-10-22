#!/bin/sh
# tl.sh idx : cuadros perdidos cada 10 cuadros (600 tras 400 de espera) y la sala
cd /tmp/claude-0/lag; I=$1
A=$(awk '$3=="viewer_debug_next"{print $1}' symbol.txt); A=$((0x$A & 0xFFFF))
{ printf "hold 11700 -\npoke %X %X 2\nhold 3 S\nhold 3 SR\nhold 3 -\nhold 400 -\n" $A $I; for k in $(seq 0 60); do printf "dumpram t${I}_%02d.bin\nhold 10 %s\n" $k $([ $k -lt 30 ] && echo X || echo LX); done; } > t$I.txt
/home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so rom.bin t$I.txt >/dev/null 2>&1
python3 - $I <<'PY'
import sys
syms={l.split()[2]:int(l.split()[0],16)&0xFFFF for l in open('symbol.txt') if len(l.split())>=3}
I=sys.argv[1]
def g(k,n): d=open('t%s_%02d.bin'%(I,k),'rb').read(); a=syms[n]; return d[a]|d[a+1]<<8
f=[g(k,'md_frame_count') for k in range(61)]
print(I,' '.join(str(10-(f[k+1]-f[k])) for k in range(60)))
print(' '.join('%04X'%g(k,'room_ptr') for k in range(0,61,10)), ' '.join(str(g(k,'game_state')) for k in range(0,61,10)))
PY
