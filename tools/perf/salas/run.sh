#!/bin/sh
# run.sh: compila la ROM de prueba (zebes_shoot) y mide los cuadros perdidos en las salas $@
cd /home/claude/mm && cp tests/regress/inputs/zebes_shoot.s res/gen/md/test_input.s && TEST=1 make rom > /tmp/claude-0/b.log 2>&1 || { grep -i " error" /tmp/claude-0/b.log | head; exit 1; }
cp out/rom.bin out/rom.out out/symbol.txt /tmp/claude-0/lag/
cd /tmp/claude-0/lag && for i in ${@:-3 1 13 51 22 9 24 6 5}; do ./lagroom.sh rom.bin symbol.txt $i > r$i.txt & done; wait
for i in ${@:-3 1 13 51 22 9 24 6 5}; do cat r$i.txt; done | awk '{s+=$4; print} END {print "total", s}'
