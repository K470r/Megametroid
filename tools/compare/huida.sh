#!/bin/sh
# MegaMetroid — la huida de Zebes, sala por sala (lado a lado SNES / Genesis): se salta a la sala ($SALA) por la
# puerta que lleva a ella con la bomba puesta (evento 0x0E, el jefe de Tourian vencido: bit 2) y el contador en marcha
# (timer_status 2: ProcessTimer_MotherBrainStart), sin mover a Samus de donde la deja la puerta.
# Salas: DE4D DE7A DEA7 DEDE (Tourian), 96BA (Climb), 92FD (Parlor), 91F8 (aterrizaje). PAT: la entrada (huida_in.txt).
# Uso: SALA=DE4D SNES_DESDE=.. SNES_HASTA=.. GEN_DESDE=.. GEN_N=.. sh tools/compare/huida.sh   (WORK: work/huida)
HERE=$(cd "$(dirname "$0")" && pwd)
case ${SALA:-DE4D} in
  DE4D) D=AA8C ;; DE7A) D=AAEC ;; DEA7) D=AB04 ;; DEDE) D=AB1C ;; 96BA) D=AB34 ;; 92FD) D=8B3E ;; 91F8) D=896A ;;
  *) echo "sala desconocida"; exit 2 ;;
esac
HL_POKE=D820:4000,D82C:0200,943:2,A5A:E0E6 PAT=${PAT:-$HERE/huida_in.txt} DOOR=$D XY_N=999999 X=0 Y=0 GOD=1 \
WORK=${WORK:-work/huida} \
EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_POKES={12755,0xD820,0x4000},{12755,0xD82C,0x200},{12755,0x943,2},{12755,0xA5A,0xE0E6} $HU_FL" \
sh $HERE/lado_a_lado.sh
