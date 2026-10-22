import numpy as np, subprocess, sys
A,B=8300,8400; nf=100
F=sys.argv[1]; th=float(sys.argv[2]) if len(sys.argv)>2 else 30
a=np.fromfile(f'/tmp/claude-0/t7/pn_{A}.prof',np.uint32).astype(np.int64); b=np.fromfile(f'/tmp/claude-0/t7/pn_{B}.prof',np.uint32).astype(np.int64)
d=b-a
syms=[]
for l in open('/tmp/claude-0/it/pn_sym.txt'):
    p=l.split()
    if len(p)>=3 and p[1] in 'tTwW': syms.append((int(p[0],16)&0xFFFFFF,p[2]))
syms.sort(); addrs=[x for x,_ in syms]
lo=[x for x,n in syms if n==F][0]; hi=addrs[addrs.index(lo)+1]
out=subprocess.run(['m68k-linux-gnu-objdump','-d','/tmp/claude-0/it/pn.out','--start-address=%d'%lo,'--stop-address=%d'%hi],capture_output=True,text=True).stdout
rows=[]
for l in out.splitlines():
    p=l.split(':')
    try: ad=int(p[0],16)
    except: continue
    rows.append((d[ad//2]/7/nf,l.strip()))
print(F,'total %.0f ciclos/cuadro'%sum(r[0] for r in rows))
# group into blocks of consecutive executed instructions, report block sums
blk=[];cur=[]
for c,l in rows:
    if c>0: cur.append((c,l))
    elif cur: blk.append(cur); cur=[]
if cur: blk.append(cur)
blk.sort(key=lambda b:-sum(x[0] for x in b))
for b_ in blk[:8]:
    print('--- bloque %.0f'%sum(x[0] for x in b_))
    for c,l in b_[:40]: print('%6.0f %s'%(c,l))
