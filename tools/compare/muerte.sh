#!/bin/sh
# MegaMetroid — la muerte de Samus lado a lado SNES / Genesis: la pelea con el Bomb Torizo (lado_a_lado.sh por
# omisión) y en el cuadro grabado $N la energía de Samus a 0 (7E:09C2): estados 19-24 (apagón, Samus que titila, el
# traje que explota) y el menú de fin de juego (25-26). Con REF=archivo, la grabación de hlref para host/check.
# Uso: N=300 sh tools/compare/muerte.sh   (WORK: work/muerte; SNES_DESDE/SNES_HASTA/GEN_DESDE/GEN_N como lado_a_lado)
HERE=$(cd "$(dirname "$0")" && pwd)
N=${N:-300}
HL_POKE_AT=$N:9C2:0 GOD=0 WORK=${WORK:-work/muerte} SNES_DESDE=${SNES_DESDE:-$((23500 + N - 20))} \
SNES_HASTA=${SNES_HASTA:-$((23500 + N + 900))} \
EXTRA_FL="-DMD_TEST_POKES={$((12743 + N)),0x9C2,0} $DS_FL" sh $HERE/lado_a_lado.sh
