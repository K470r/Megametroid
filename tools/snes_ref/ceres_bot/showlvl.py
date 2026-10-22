import struct,sys
d=open(sys.argv[1],'rb').read(); w=struct.unpack('<H',d[-2:])[0]
lv=struct.unpack('<%dH'%0x3000,d[:0x6000]); n=lv[0]//2; h=n//w
print(sys.argv[1],'w',w,'h',h)
for y in range(h): print('%4d '%(y*16)+''.join('.#/?:;<=>DEF!@$%'[(lv[1+y*w+x]>>12)] for x in range(w)))
