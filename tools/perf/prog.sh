#!/bin/sh
# prog.sh nombre emu1 emu2...: md_frame_count de la ROM de prueba actual en esos cuadros del emulador
cd /home/claude/mm/native/md && SKIP_DATA=1 TEST=1 $EXTRA sh build.sh >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/p.bin; cp out/rom.out /tmp/claude-0/it/p.out
cd /tmp/claude-0/it
for e in "$@"; do printf "hold $e -\ndumpram p_$e.ram\n" > p_$e.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so p.bin p_$e.txt >/dev/null 2>&1 & done
wait
for e in "$@"; do python3 - $e <<'PY'
import subprocess,sys
e=sys.argv[1]
syms={}
for l in subprocess.run(['m68k-linux-gnu-nm','/tmp/claude-0/it/p.out'],capture_output=True,text=True).stdout.splitlines():
    p=l.split()
    if len(p)==3: syms[p[2]]=int(p[0],16)
ram=open(f'/tmp/claude-0/it/p_{e}.ram','rb').read()
rb=lambda a: ram[(a&0xFFFF)^1]
fc=(rb(syms['md_frame_count'])<<8)|rb(syms['md_frame_count']+1)
print(e, fc, 'retraso', int(e)-fc-50)
PY
done
