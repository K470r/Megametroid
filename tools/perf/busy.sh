#!/bin/sh
# busy.sh BASE EMU: líneas por cuadro del motor desde BASE (viejo y nuevo, misma entrada de prueba)
B=$1; E=$2
cd /tmp/claude-0/old && TEST=1 EXPFL="-DMD_BUSYLOG=$B" make rom >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/bo.bin && cp out/rom.out /tmp/claude-0/it/bo.out
cd /home/claude/mm && TEST=1 EXPFL="-DMD_BUSYLOG=$B" make rom >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/bn.bin && cp out/rom.out /tmp/claude-0/it/bn.out
cd /tmp/claude-0/it
for v in bo bn; do printf "hold $E -\ndumpram $v.ram\n" > $v.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so $v.bin $v.txt >/dev/null 2>&1 & done
wait
python3 - $B <<'PY'
import subprocess, sys
B=int(sys.argv[1]); D={}
for v in ('bo','bn'):
    syms={}
    for l in subprocess.run(['m68k-linux-gnu-nm',f'/tmp/claude-0/it/{v}.out'],capture_output=True,text=True).stdout.splitlines():
        p=l.split()
        if len(p)==3: syms[p[2]]=int(p[0],16)
    ram=open(f'/tmp/claude-0/it/{v}.ram','rb').read()
    rb=lambda a: ram[(a&0xFFFF)^1]
    a=syms['md_busy_log']; D[v]=[(rb(a+2*i)<<8)|rb(a+2*i+1) for i in range(512)]
import json; json.dump(D, open('/tmp/claude-0/it/busy.json','w'))
o,n=D['bo'],D['bn']
for s in range(0,512,32):
    so=[x for x in o[s:s+32] if x]; sn=[x for x in n[s:s+32] if x]
    if not so and not sn: continue
    print(B+s, 'viejo media %.0f máx %d' % (sum(so)/max(1,len(so)), max(so or [0])), '| nuevo media %.0f máx %d' % (sum(sn)/max(1,len(sn)), max(sn or [0])))
PY
