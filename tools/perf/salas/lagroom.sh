#!/bin/sh
# lagroom.sh rom sym idx : cuadros perdidos en 600 cuadros de emulación dentro de la sala idx del visor (disparando)
cd /tmp/claude-0/lag
S=$2; I=$3; R=$1
A=$(awk '$3=="viewer_debug_next"{print $1}' $S); A=$((0x$A & 0xFFFF))
printf "hold 11700 -\npoke %X %X 2\nhold 3 S\nhold 3 SR\nhold 3 -\nhold 400 -\ndumpram l${I}a.bin\nhold 300 X\nhold 300 LX\ndumpram l${I}b.bin\n" $A $I > l$I.txt
/home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so $R l$I.txt >/dev/null 2>&1
python3 - $S $I <<'PY'
import sys
syms={l.split()[2]:int(l.split()[0],16)&0xFFFF for l in open(sys.argv[1]) if len(l.split())>=3}
def g(f,n): d=open(f,'rb').read(); a=syms[n]; return d[a]|d[a+1]<<8
i=sys.argv[2]
fa,fb=g('l%sa.bin'%i,'md_frame_count'),g('l%sb.bin'%i,'md_frame_count')
d=open('l%sb.bin'%i,'rb').read(); m=syms['md_unimpl_msg']; ptr=(d[m]|d[m+1]<<8)<<16|(d[m+2]|d[m+3]<<8)
rom=open(sys.argv[1].replace('symbol.txt','rom.bin'),'rb').read() if False else open('rom.bin','rb').read()
msg=rom[ptr:ptr+60].split(b'\0')[0].decode('latin1') if ptr and ptr<len(rom) else ''
print(i,'%04X'%g('l%sb.bin'%i,'room_ptr'),'perdidos',600-((fb-fa)&0xFFFF),'unimpl',g('l%sb.bin'%i,'md_unimpl_count'),'%04X'%g('l%sb.bin'%i,'nt_unimpl_val'),msg)
PY
