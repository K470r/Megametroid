import sys,struct,bisect,subprocess
prof=open(sys.argv[1],'rb').read(); cyc=struct.unpack(f'<{len(prof)//4}I',prof)
syms=sorted((int(l.split()[0],16),l.split()[2]) for l in open(sys.argv[2]) if len(l.split())>=3 and l.split()[1] in 'TtWw')
fn=sys.argv[3]; a0=[a for a,n in syms if n==fn][0]; a1=min(a for a,n in syms if a>a0)
tot=sum(cyc[a0//2:a1//2])
# block per 32 bytes
blocks=[]
for b in range(a0,a1,32):
    c=sum(cyc[b//2:min(a1,b+32)//2]); blocks.append((c,b))
blocks.sort(reverse=True)
dis=subprocess.run(['m68k-linux-gnu-objdump','-d','--no-show-raw-insn','--start-address=%d'%a0,'--stop-address=%d'%a1,sys.argv[4]],capture_output=True,text=True).stdout.split('\n')
for c,b in blocks[:int(sys.argv[5]) if len(sys.argv)>5 else 6]:
    print('--- %06X %.1f%%'%(b,100*c/tot))
    for l in dis:
        try: ad=int(l.split(':')[0],16)
        except: continue
        if b<=ad<b+32: print('  %6d'%cyc[ad//2],l)
