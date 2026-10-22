#!/bin/sh
# Compila la comprobación en PC del motor nativo (docs/31) y mkinit (estado inicial para la Genesis, docs/32).
# Genera antes las tablas del motor en res/gen/engine (enums, variables y tablas de la ROM).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$HERE/..
SMSRC=${SMSRC:-$ROOT/sm/src}
GEN=$ROOT/res/gen/engine
mkdir -p $GEN
python3 $ROOT/tools/conv/gen_enums.py $SMSRC $GEN
python3 $ROOT/tools/conv/gen_vars.py $SMSRC/variables.h $ROOT/src/core:$ROOT/inc/core $GEN
python3 $ROOT/tools/conv/mkdata.py ${SMROM:-$ROOT/sm/sm.smc} $SMSRC $GEN > /dev/null
CF="-O1 -g -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -I$HERE -I$ROOT/inc/core -I$GEN"
SRC="$ROOT/src/core/*.c $GEN/vars.c $GEN/vars_map.c $GEN/tables.c"
gcc $CF -o $HERE/check $HERE/check.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/tour $HERE/tour.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/simsum $HERE/simsum.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/vardump $HERE/vardump.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/zcover $HERE/zcover.c $HERE/pc_stubs.c $SRC
gcc $CF -o $HERE/mkinit $HERE/mkinit.c $HERE/pc_stubs.c $SRC
echo OK $HERE/check $HERE/mkinit
