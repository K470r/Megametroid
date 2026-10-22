#!/bin/sh
# stopcmp.sh N...: captura de la ROM vieja (prepipe) y la nueva congeladas en el cuadro N del motor
for N in "$@"; do
  (cd /tmp/claude-0/prepipe && TEST=1 STOP_AT=$N make rom >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/so_$N.bin)
  (cd /home/claude/mm && TEST=1 STOP_AT=$N make rom >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/sn_$N.bin)
  E=$((N * 12 / 10 + 300))
  for v in so sn; do printf "hold $E -\nshot ${v}_$N.ppm\n" > ${v}_$N.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so ${v}_$N.bin ${v}_$N.txt >/dev/null 2>&1 & done
  wait
  python3 -c "
from PIL import Image, ImageChops
a=Image.open('so_$N.ppm').convert('RGB'); b=Image.open('sn_$N.ppm').convert('RGB')
d=ImageChops.difference(a,b); bb=d.getbbox(); print($N, 'distinto' if bb else 'igual', bb)
a.save('so_$N.png'); b.save('sn_$N.png')"
done
