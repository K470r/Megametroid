#!/bin/sh
# pnew.sh A B [función]: perfil exacto de los cuadros A..B del motor (versión actual, entrada de res/gen/md/test_input.s)
A=$1; B=$2; F=$3; E=$((B + B / 5 + 400))
cd /home/claude/mm
for K in $A $B; do TEST=1 STOP_AT=$K make rom >/dev/null 2>&1 && cp out/rom.bin /tmp/claude-0/it/pn_$K.bin; done
cp out/rom.out /tmp/claude-0/it/pn.out; m68k-linux-gnu-nm -n out/rom.out > /tmp/claude-0/it/pn_sym.txt
cd /tmp/claude-0/t7 && printf "hold $E -\n" > hE.txt
for K in $A $B; do MM_PROF=/tmp/claude-0/t7/pn_$K.prof RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx_prof/genesis_plus_gx_libretro.so /tmp/claude-0/it/pn_$K.bin hE.txt >/dev/null 2>&1 & done
wait
python3 - $A $B "$F" <<'PY'
import numpy as np, bisect, sys, subprocess
A,B,F=sys.argv[1],sys.argv[2],sys.argv[3]; nf=int(B)-int(A)
a=np.fromfile(f'/tmp/claude-0/t7/pn_{A}.prof',np.uint32).astype(np.int64); b=np.fromfile(f'/tmp/claude-0/t7/pn_{B}.prof',np.uint32).astype(np.int64)
d=b-a; syms=[]
for l in open('/tmp/claude-0/it/pn_sym.txt'):
    p=l.split()
    if len(p)>=3 and p[1] in 'tTwW': syms.append((int(p[0],16)&0xFFFFFF,p[2]))
syms.sort(); addrs=[x for x,_ in syms]; tot={}
for i in np.nonzero(d)[0]:
    k=bisect.bisect_right(addrs,i*2)-1; n=syms[k][1] if k>=0 else '?'
    tot[n]=tot.get(n,0)+d[i]
L=lambda c: c/7/nf/488
skip=('main',)
print('total %.1f líneas por cuadro' % sum(L(v) for k,v in tot.items() if k not in skip))
for k,v in sorted(tot.items(),key=lambda x:-x[1])[:22]:
    if k not in skip: print('%-40s %5.1f'%(k,L(v)))
if F:
    lo=[x for x,n in syms if n==F][0]; hi=addrs[addrs.index(lo)+1]
    v=d[lo//2:hi//2]
    hot=set(int(lo+2*i) for i in np.argsort(-v)[:25] if v[i])
    out=subprocess.run(['m68k-linux-gnu-objdump','-d','/tmp/claude-0/it/pn.out','--start-address=%d'%lo,'--stop-address=%d'%hi],capture_output=True,text=True).stdout
    for l in out.splitlines():
        p=l.split(':')
        try: ad=int(p[0],16)
        except: continue
        c=d[ad//2]/7/nf
        if c>=1: print('%7.0f  %s'%(c,l.strip()))
PY
