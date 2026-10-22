#!/bin/sh
# gdbatch.sh NN [c|br]: como crbatch.sh, con Samus que no muere (HL_GOD) y más cuadros por sala
cd /tmp/claude-0/warp
K=${2:-c}; B=$K$1; PER=${PER:-1500}
python3 mkwarp_e.py "$(cat rooms_$B.txt)" 12861 $PER gin_$B.txt > /dev/null
python3 mkwarp_e.py "$(cat rooms_$B.txt)" 21 $PER /dev/null > g${B}_rel.txt
[ -f gref_$B.bin ] || HL_GOD=1 HL_ENEMIES=1 HL_WARP=$(cat g${B}_rel.txt) setarch -R /tmp/claude-0/hlb/hlref_e /home/claude/mm/sm/sm.smc gin_$B.txt gref_$B.bin 23500 > grec_$B.log 2>&1
cd /home/claude/mm/host
HL_GOD=1 HL_ENEMIES=1 HL_WARP=$(cat /tmp/claude-0/warp/g${B}_rel.txt) ./check /tmp/claude-0/warp/gref_$B.bin ../../sm/sm.smc 1 30000 0 > /tmp/claude-0/warp/gchk_$B.txt 2>&1
rm -f /tmp/claude-0/warp/gref_$B.bin
echo "$B $(cat rooms_$B.txt 2>/dev/null) $(tail -1 /tmp/claude-0/warp/gchk_$B.txt) $(grep -h 'SIN PORTAR' /tmp/claude-0/warp/gchk_$B.txt | sed 's/^cuadro [0-9]*: SIN PORTAR: //' | sort -u | tr '\n' ';')"
