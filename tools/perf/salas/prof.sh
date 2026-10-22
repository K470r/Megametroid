#!/bin/sh
# prof.sh idx : perfil del 68000 en la sala idx (600 cuadros disparando)
cd /tmp/claude-0/lag; I=$1
A=$(awk '$3=="viewer_debug_next"{print $1}' symbol.txt); A=$((0x$A & 0xFFFF))
printf "hold 11700 -\npoke %X %X 2\nhold 3 S\nhold 3 SR\nhold 3 -\nhold 400 -\n" $A $I > pa.txt
cp pa.txt pb.txt; printf "hold 300 X\nhold 300 LX\n" >> pb.txt
MM_PROF=/tmp/claude-0/lag/pa.bin RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx_prof/genesis_plus_gx_libretro.so rom.bin pa.txt >/dev/null 2>&1
MM_PROF=/tmp/claude-0/lag/pb.bin RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx_prof/genesis_plus_gx_libretro.so rom.bin pb.txt >/dev/null 2>&1
python3 - <<'PY'
import struct
a=open('/tmp/claude-0/lag/pa.bin','rb').read(); b=open('/tmp/claude-0/lag/pb.bin','rb').read()
A=struct.unpack('<%dI'%(len(a)//4),a); B=struct.unpack('<%dI'%(len(b)//4),b)
open('/tmp/claude-0/lag/pd.bin','wb').write(struct.pack('<%dI'%len(A),*[max(0,y-x) for x,y in zip(A,B)]))
PY
python3 /home/claude/mm/tools/emu_harness/prof_report.py pd.bin symbol.txt ${2:-40}
