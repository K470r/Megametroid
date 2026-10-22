#!/bin/sh
# build_hlref.sh smsrc/src salida: compila hlref con el decomp parcheado para el visor
set -e
H=$(dirname "$0")
T=$(mktemp -d)
python3 $H/patch_decomp.py "$1" $T/src
S=$T/src
# -O1: con -O2 el decomp tiene comportamiento indefinido que a veces se cuelga al saltar de sala
gcc -O1 -w -iquote $S -o "$2" $H/../ceres_bot/hlref.c $(ls $S/*.c | grep -v -e main.c -e opengl.c -e glsl_shader.c -e config.c -e spc_player.c) $S/snes/*.c -lm
rm -rf $T
