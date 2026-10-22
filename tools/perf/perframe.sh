#!/bin/sh
# perframe.sh A B: lo que ocupa cada cuadro de emulador A..B (perfil por diferencias)
cd /tmp/claude-0/t7
for n in $(seq $1 $2); do printf "hold $n -\n" > h$n.txt; MM_PROF=/tmp/claude-0/t7/q$n.bin RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx_prof/genesis_plus_gx_libretro.so /home/claude/mm/native/md/out/rom.bin h$n.txt >/dev/null 2>&1 & done
wait
python3 - $1 $2 <<'PY'
import numpy as np, bisect, sys
A,B=int(sys.argv[1]),int(sys.argv[2])
syms=[]
for l in open('/home/claude/mm/native/md/out/symbol.txt'):
    p=l.split()
    if len(p)>=3 and p[1] in 'tTwW': syms.append((int(p[0],16)&0xFFFFFF,p[2]))
syms.sort(); addrs=[x for x,_ in syms]
prev=None
for n in range(A,B+1):
    a=np.fromfile(f'/tmp/claude-0/t7/q{n}.bin',np.uint32).astype(np.int64)
    if prev is not None:
        d=a-prev; tot={}
        for i in np.nonzero(d)[0]:
            k=bisect.bisect_right(addrs,i*2)-1
            nm=syms[k][1] if k>=0 else '?'
            tot[nm]=tot.get(nm,0)+d[i]
        T=sum(v for k,v in tot.items() if k!='main')
        top=sorted(((v,k) for k,v in tot.items() if k!='main'),reverse=True)[:6]
        print(n, 'busy %.0f'%(T/7/488), ' '.join('%s:%.0f'%(k,v/7/488) for v,k in top))
    prev=a
PY
