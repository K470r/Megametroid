#!/bin/sh
# bnew.sh BASE EMU [etiqueta]: líneas medias por cuadro (versión actual) desde BASE; variables del Makefile en el entorno
B=$1; E=$2
cd /home/claude/mm && TEST=1 EXPFL="-DMD_BUSYLOG=$B $XFL" make rom >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/bx.bin && cp out/rom.out /tmp/claude-0/it/bx.out || { echo FALLO; exit 1; }
cd /tmp/claude-0/it && printf "hold $E -\ndumpram bx.ram\n" > bx.txt && RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so bx.bin bx.txt >/dev/null 2>&1
python3 - $B "$3" <<'PY'
import subprocess, sys
B=int(sys.argv[1])
syms={}
for l in subprocess.run(['m68k-linux-gnu-nm','/tmp/claude-0/it/bx.out'],capture_output=True,text=True).stdout.splitlines():
    p=l.split()
    if len(p)==3: syms[p[2]]=int(p[0],16)
ram=open('/tmp/claude-0/it/bx.ram','rb').read()
rb=lambda a: ram[(a&0xFFFF)^1]
v=[(rb(syms['md_busy_log']+2*i)<<8)|rb(syms['md_busy_log']+2*i+1) for i in range(512)]
v=[x for x in v if 0<x<1000]
print(sys.argv[2], 'cuadros %d media %.1f máx %d sobre245 %d' % (len(v), sum(v)/max(1,len(v)), max(v or [0]), sum(1 for x in v if x>245)))
PY
