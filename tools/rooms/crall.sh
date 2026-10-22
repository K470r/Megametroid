#!/bin/sh
# crall.sh: todas las salas de Crateria con enemigos (en paralelo de a 8)
cd /tmp/claude-0/warp
for i in $(seq -w 0 33); do
  ( r=$(./crbatch.sh $i 2>&1 | tail -1); echo "c$i $(cat rooms_c$i.txt) $r $(grep -h 'SIN PORTAR' chk_c$i.txt | sed 's/^cuadro [0-9]*: SIN PORTAR: //' | sort -u | tr '\n' ';')" ) > res_c$i.txt &
  case $i in 07|15|23|31) wait;; esac
done
wait
cat res_c*.txt
