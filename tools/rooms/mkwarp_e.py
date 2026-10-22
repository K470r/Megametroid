import sys
# mkwarp_e.py lista inicio_n por_sala salida_in.txt -> HL_WARP; patrón con disparos (para los enemigos)
doors=[x.split(':')[1] for x in sys.argv[1].split()]
n0=int(sys.argv[2]); per=int(sys.argv[3])
base=open(__import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'snes_ref', 'ceres_bot', 'in_z2.txt')).read()
pat=['30 >X','20 >AX','10 -','30 <X','20 <AX','10 -','40 X','20 UX','20 DX','30 >','20 >A','30 <BX','20 AX','30 -','20 X','20 >X','30 <X']
assert sum(int(p.split()[0]) for p in pat)==400 or True
tot=sum(int(p.split()[0]) for p in pat)
with open(sys.argv[4],'w') as f:
    f.write(base)
    for i in range(len(doors)):
        f.write('\n'.join(pat)+'\n')
        if per>tot: f.write('%d -\n'%(per-tot))
print(','.join('%d:%s'%(n0+per*i,d) for i,d in enumerate(doors)))
