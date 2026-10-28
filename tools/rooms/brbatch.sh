#!/bin/sh
# brbatch.sh SALA: como crbatch.sh, para las salas de Brinstar (rooms_brSALA.txt)
cd /tmp/claude-0/warp
B=br$1; PER=${PER:-400}
python3 mkwarp_e.py "$(cat rooms_$B.txt)" 12861 $PER in_$B.txt > /dev/null
python3 mkwarp_e.py "$(cat rooms_$B.txt)" 21 $PER /dev/null > ${B}_rel.txt
[ -f ref_$B.bin ] || HL_ENEMIES=1 HL_WARP=$(cat ${B}_rel.txt) setarch -R /tmp/claude-0/hlb/hlref_e /home/claude/mm/sm/sm.smc in_$B.txt ref_$B.bin 23500 > rec_$B.log 2>&1
cd /home/claude/mm/host
HL_ENEMIES=1 HL_WARP=$(cat /tmp/claude-0/warp/${B}_rel.txt) ./check /tmp/claude-0/warp/ref_$B.bin ../../sm/sm.smc 1 ${UNTIL:-20000} 0 > /tmp/claude-0/warp/chk_$B.txt 2>&1
echo "$1 $(tail -1 /tmp/claude-0/warp/chk_$B.txt) $(grep -h 'SIN PORTAR' /tmp/claude-0/warp/chk_$B.txt | sed 's/^cuadro [0-9]*: SIN PORTAR: //' | sort -u | tr '\n' ';')"
