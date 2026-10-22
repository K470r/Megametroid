#!/bin/sh
# lagrun.sh BASE EMU: cuadros con imagen repetida (md_lag_log > 1) desde BASE (2048 cuadros), ROM actual
B=$1; E=$2
cd /home/claude/mm/native/md && SKIP_DATA=1 TEST=1 LAGLOG=1 LAG_BASE=$B EXPFL="-DMD_LAG_N=2048 $XFL" sh build.sh >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/lg.bin; cp out/rom.out /tmp/claude-0/it/lg.out
cd /tmp/claude-0/it && printf "hold $E -\ndumpram lg.ram\n" > lg.txt && RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so lg.bin lg.txt >/dev/null 2>&1
python3 - $B <<'PY'
import subprocess,sys
B=int(sys.argv[1])
syms={}
for l in subprocess.run(['m68k-linux-gnu-nm','/tmp/claude-0/it/lg.out'],capture_output=True,text=True).stdout.splitlines():
    p=l.split()
    if len(p)==3: syms[p[2]]=int(p[0],16)
r=open('/tmp/claude-0/it/lg.ram','rb').read()
rb=lambda a: r[(a&0xFFFF)^1]
v=[rb(syms['md_lag_log']+i) for i in range(2047)]
n=max(i for i,x in enumerate(v) if x) if any(v) else 0
bad=[(B+i,x) for i,x in enumerate(v[:n]) if x>1]
print('cuadros', n, 'con retraso', len(bad), 'perdidos', sum(x-1 for _,x in bad)); print(bad[:100])
PY
