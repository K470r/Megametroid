#!/bin/sh
# sprab.sh BASE REF "XFL_A" "XFL_B": compara la lista de sprites (MD_SPRSUM) de dos variantes de la ROM, 256 cuadros
B=$1; REF=$2; A=$3; Bf=$4
cd /home/claude/mm/native/md && python3 ../tools/mk_test_input.py $REF 98 $((B + 400)) gen/test_input.s >/dev/null
for v in a b; do
  F=$A; [ $v = b ] && F=$Bf
  SKIP_DATA=1 TEST=1 EXPFL="-DMD_SPRSUM=$B $F" sh build.sh >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/sp_$v.bin && cp out/rom.out /tmp/claude-0/it/sp_$v.out || { echo FALLO $v; exit 1; }
done
cd /tmp/claude-0/it
E=$(( B * 13 / 10 + 900 ))
for v in a b; do printf "hold $E -\ndumpram sp_$v.ram\n" > sp_$v.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so sp_$v.bin sp_$v.txt >/dev/null 2>&1 & done
wait
python3 - <<'PY'
import subprocess
L={}
for v in 'ab':
    syms={}
    for l in subprocess.run(['m68k-linux-gnu-nm',f'/tmp/claude-0/it/sp_{v}.out'],capture_output=True,text=True).stdout.splitlines():
        p=l.split()
        if len(p)==3: syms[p[2]]=int(p[0],16)
    ram=open(f'/tmp/claude-0/it/sp_{v}.ram','rb').read()
    rb=lambda a: ram[(a&0xFFFF)^1]
    L[v]=[(rb(syms['md_spr_sum']+2*i)<<8)|rb(syms['md_spr_sum']+2*i+1) for i in range(256)]
a,b=L['a'],L['b']
bad=[i for i in range(256) if a[i]!=b[i]]
print('cuadros 256, distintos %d (primero %s), ceros %d/%d'%(len(bad), bad[0] if bad else '-', a.count(0), b.count(0)))
PY
