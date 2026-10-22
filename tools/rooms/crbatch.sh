#!/bin/sh
# crbatch.sh NN: Crateria con enemigos: graba (SNES) y compara (motor nativo) el lote NN (rooms_cNN.txt)
cd /tmp/claude-0/warp
B=$1; PER=${PER:-400}
python3 mkwarp_e.py "$(cat rooms_c$B.txt)" 12861 $PER in_c$B.txt > /dev/null
python3 mkwarp_e.py "$(cat rooms_c$B.txt)" 21 $PER /dev/null > c${B}_rel.txt
[ -f ref_c$B.bin ] || HL_ENEMIES=1 HL_WARP=$(cat c${B}_rel.txt) setarch -R /tmp/claude-0/hlb/hlref_e /home/claude/mm/sm/sm.smc in_c$B.txt ref_c$B.bin 23500 > rec_c$B.log 2>&1
cd /home/claude/mm/native/host
HL_ENEMIES=1 HL_WARP=$(cat /tmp/claude-0/warp/c${B}_rel.txt) ./check /tmp/claude-0/warp/ref_c$B.bin ../../sm/sm.smc 1 ${UNTIL:-20000} 0 > /tmp/claude-0/warp/chk_c$B.txt 2>&1
tail -1 /tmp/claude-0/warp/chk_c$B.txt
