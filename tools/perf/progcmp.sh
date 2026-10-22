#!/bin/sh
# progcmp.sh E...: cuadro del motor alcanzado en esos cuadros del emulador, ROM vieja (prepipe) y nueva (env: TEST_INTRO...)
(cd /tmp/claude-0/prepipe/native/md && SKIP_DATA=1 TEST=1 sh build.sh >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/old.bin; cp out/rom.out /tmp/claude-0/it/old.out)
(cd /home/claude/mm/native/md && SKIP_DATA=1 TEST=1 sh build.sh >/dev/null 2>&1; cp out/rom.bin /tmp/claude-0/it/new.bin; cp out/rom.out /tmp/claude-0/it/new.out)
for v in old new; do for e in "$@"; do printf "hold $e -\ndumpram f_${v}_$e.ram\n" > f_${v}_$e.txt; RR_PAD=6 /home/claude/m68kbin/retrorun /home/claude/gpgx/genesis_plus_gx_libretro.so $v.bin f_${v}_$e.txt >/dev/null 2>&1 & done; done; wait
for v in old new; do l="$v"; for e in "$@"; do l="$l $e:$(python3 fc.py $v.out f_${v}_$e.ram)"; done; echo $l; done
