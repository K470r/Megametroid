#!/bin/sh
# lag.sh B1 B2...: cuadros de retraso de la intro (prueba con la entrada de la grabación), ventanas de 1024 cuadros
cd /home/claude/mm/native/md
for B in "$@"; do SKIP_DATA=1 TEST=1 TEST_INTRO=1 LAGLOG=1 LAG_BASE=$B EXPFL="-DMD_LAG_N=1024" sh build.sh >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/lag_$B.bin; cp out/rom.out /tmp/claude-0/it/lag_$B.out; done
cd /tmp/claude-0/it
for B in "$@"; do E=$((B * 105 / 100 + 1100)); printf "hold $E -\ndumpram l_$B.ram\n" > l_$B.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so lag_$B.bin l_$B.txt >/dev/null 2>&1 & done
wait
python3 - "$@" <<'PY'
import subprocess, sys
for B in map(int, sys.argv[1:]):
    syms={}
    for l in subprocess.run(['m68k-linux-gnu-nm',f'/tmp/claude-0/it/lag_{B}.out'],capture_output=True,text=True).stdout.splitlines():
        p=l.split()
        if len(p)==3: syms[p[2]]=int(p[0],16)
    ram=open(f'/tmp/claude-0/it/l_{B}.ram','rb').read()
    rb=lambda a: ram[(a&0xFFFF)^1]
    rw=lambda a: rb(a)<<8|rb(a+1)
    a=syms['md_lag_log']; lag=[rb(a+i) for i in range(1023)]
    out=[]
    for s in range(0,len(lag),100):
        seg=lag[s:s+100]
        if sum(seg)==0: break
        if sum(seg)>len(seg): out.append('%d:+%d'%(B+s,sum(seg)-len(seg)))
    print(B, 'fc',rw(syms['md_frame_count']), ' '.join(out))
PY
