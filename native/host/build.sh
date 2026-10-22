#!/bin/sh
# Compila la comprobación en PC del motor nativo (docs/31) y mkinit (estado inicial para la Genesis, docs/32).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
N=$HERE/..
SMSRC=${SMSRC:-$N/../sm/src}
python3 $N/tools/gen_enums.py $SMSRC $N/gen
python3 $N/tools/gen_vars.py $SMSRC/variables.h $N/src $N/gen
python3 $N/tools/mkdata.py ${SMROM:-$N/../sm/sm.smc} $SMSRC $N/gen > /dev/null
CF="-O1 -g -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable"
SRC="$N/src/*.c $N/gen/vars.c $N/gen/vars_map.c $N/gen/tables.c"
gcc $CF -o $HERE/check $HERE/check.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/tour $HERE/tour.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/simsum $HERE/simsum.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/vardump $HERE/vardump.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/zcover $HERE/zcover.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/mkinit $HERE/mkinit.c $HERE/pc_stubs.c $SRC
echo OK $HERE/check $HERE/mkinit
