#!/bin/sh
# MegaMetroid — las partidas guardadas (docs/65): la SRAM que escribe la Genesis contra la del SNES.
# La entrada del bot hasta Zebes (in_z2.txt): el SNES guarda al huir de Ceres y al aterrizar (GunshipTop_7). hlref
# (el decomp) deja su SRAM en saves/sm.srm; la Genesis (ROM de prueba TEST=1) la deja en la del emulador (bytes
# impares). Se comparan los 8 KB del SNES. Uso: sh tools/compare/partida.sh   (WORK: work/sram)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${WORK:-$ROOT/work/sram}
T0=10757                         # cuadro de la entrada del primer cuadro del motor (como lado_a_lado.sh)
mkdir -p $WORK/saves
cat $ROOT/tools/snes_ref/ceres_bot/in_z2.txt > $WORK/in.txt
echo "300 -" >> $WORK/in.txt
rm -f $WORK/saves/sm.srm
(cd $WORK && setarch -R $ROOT/work/inputs/hlref $ROOT/sm/sm.smc in.txt /dev/null 23800 > /dev/null 2>&1) || true
python3 - $WORK/in.txt $T0 $ROOT/res/gen/md/test_input.s <<'P'
import sys
B = {'B': 1, 'Y': 2, 's': 4, 'S': 8, 'U': 16, 'D': 32, '<': 64, '>': 128, 'A': 256, 'X': 512, 'L': 1024, 'R': 2048}
SN = [0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10]
v = []
for l in open(sys.argv[1]):
    p = l.split()
    if not p:
        continue
    x = 0
    for ch in (p[1] if len(p) > 1 else ''):
        x |= B.get(ch, 0)
    v += [sum(m for i, m in enumerate(SN) if x & (1 << i))] * int(p[0])
v = v[int(sys.argv[2]):]
with open(sys.argv[3], 'w') as o:
    o.write("    .section .rodata\n    .align 2\n    .global kTestInputLen\nkTestInputLen:\n    .word %d\n"
            "    .global kTestInput\nkTestInput:\n" % len(v))
    for i in range(0, len(v), 16):
        o.write("    .word " + ",".join("0x%04X" % x for x in v[i:i + 16]) + "\n")
P
env TEST=1 make -C $ROOT rom > $WORK/build.log 2>&1 || { echo "no compila: $WORK/build.log"; exit 1; }
cp $ROOT/out/rom.bin $WORK/gen.bin
head -c 16384 /dev/zero > $WORK/zero.srm                  # (como la del SNES: a cero)
printf "sram $WORK/zero.srm\nhold ${GEN_N:-16000} -\ndumpsram $WORK/gen.srm\nshot $WORK/gen.ppm\n" > $WORK/gen.txt
$ROOT/tools/bin/retrorun $ROOT/tools/bin/genesis_plus_gx_libretro.so $WORK/gen.bin $WORK/gen.txt > /dev/null 2>&1
python3 - $WORK/saves/sm.srm $WORK/gen.srm <<'P'
import sys
s = open(sys.argv[1], 'rb').read()
g = open(sys.argv[2], 'rb').read()
g = (g + bytes(0x4000 - len(g)))[1::2][:8192]          # (el emulador recorta el final del archivo)
d = [i for i in range(8192) if s[i] != g[i]]
print("SRAM: %d bytes distintos de 8192" % len(d))
for i in d[:40]:
    print("  %04X snes %02X genesis %02X" % (i, s[i], g[i]))
P
