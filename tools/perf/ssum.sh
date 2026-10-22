#!/bin/sh
# ssum.sh BASE REF: compara la suma del estado de la Genesis (MD_STATESUM) con el motor en el PC, 512 cuadros desde BASE
B=$1; REF=$2
cd /home/claude/mm && python3 tools/conv/mk_test_input.py $REF 98 $((B + 600)) res/gen/md/test_input.s >/dev/null
TEST=1 EXPFL="-DMD_STATESUM=$B $XFL" make rom >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/ss.bin && cp out/rom.out /tmp/claude-0/it/ss.out || { echo FALLO; exit 1; }
E=$(( B * 2 + 520 * 22 + 600 ))
cd /tmp/claude-0/it && printf "hold $E -\ndumpram ss.ram\n" > ss.txt && RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so ss.bin ss.txt >/dev/null 2>&1
/home/claude/mm/host/simsum $REF /home/claude/mm/sm/sm.smc 97 $B 256 > ss_host.txt
python3 - <<'PY'
import subprocess
syms={}
for l in subprocess.run(['m68k-linux-gnu-nm','/tmp/claude-0/it/ss.out'],capture_output=True,text=True).stdout.splitlines():
    p=l.split()
    if len(p)==3: syms[p[2]]=int(p[0],16)
ram=open('/tmp/claude-0/it/ss.ram','rb').read()
rb=lambda a: ram[(a&0xFFFF)^1]
g=[(rb(syms['md_state_log']+2*i)<<8)|rb(syms['md_state_log']+2*i+1) for i in range(256)]
h=[int(l.split()[1],16) for l in open('/tmp/claude-0/it/ss_host.txt')]
n=min(len(g),len(h)); bad=[i for i in range(n) if g[i]!=h[i]]
print('cuadros %d iguales %d; primero distinto %s' % (n, n-len(bad), bad[0] if bad else '-'))
if bad: print('genesis', hex(g[bad[0]]), 'pc', hex(h[bad[0]]))
PY
