#!/bin/sh
# MegaMetroid — datos de la Genesis desde la ROM del SNES de cada quien (docs/32, docs/45). Lo llama `make data`.
#   1) tablas del motor (host/build.sh -> res/gen/engine) y estado inicial (mkinit -> res/gen/md)
#   2) gráficos, salas, intro, cinemáticas, voces, título y menú (tools/conv/*.py, tools/genesis_convert/)
# Todo lo que sale de aquí viene de la ROM: va en res/gen/ (fuera de git, docs/43).
# Variables: SMROM, REF (grabación hlref de Ceres), REF_INTRO (la de la intro), SNAP (capturas del SNES: res/snes_ref),
#            TEST_FRAMES, SKIP_FRONT=1 (no rehacer título y menú), ONLY_VOICE=1 (solo las voces, si faltan)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SMROM=${SMROM:-$ROOT/sm/sm.smc}
SNAP=${SNAP:-$ROOT/res/snes_ref}
EXT=$ROOT/data/extracted_full
GEN_E=$ROOT/res/gen/engine
GEN=$ROOT/res/gen/md
CONV=$ROOT/tools/conv
mkdir -p $GEN_E $GEN
if [ -z "$ONLY_VOICE" ]; then
  sh $ROOT/host/build.sh >/dev/null
  $ROOT/host/mkinit ${REF:?falta REF (grabación hlref)} 97 $GEN
  # la intro: la RAM del SNES al elegir partida nueva (cabecera de la grabación de la intro, hlref con HL_ANY desde el
  # cuadro en que empieza el estado 30) y sus gráficos (docs/38)
  $ROOT/host/mkinit ${REF_INTRO:?falta REF_INTRO (grabación hlref de la intro)} -1 $GEN init_state_intro
  python3 $CONV/mkintro.py $SMROM $EXT $SNAP $GEN
  python3 $CONV/mkmd.py $SMROM $EXT $SNAP $GEN_E $GEN $SNAP/draw.log $SNAP/hud.log
  # las cinemáticas antes que mkzebes.py: él reparte sus bloques en las páginas del SSF2 (md_cine.s, docs/48)
  python3 $CONV/mkgetaway.py $SNAP/getaway.npz $GEN
  python3 $CONV/mkm7.py $SNAP $GEN
  MM_MAX_PALS=2 python3 $CONV/mkzebes.py $SMROM $GEN_E 2>/dev/null
  printf '    .section .rodata\n    .align 2\n    .global kInitBlob\nkInitBlob:\n    .incbin "init_state.bin"\n    .align 2\n    .global kInitBlobIntro\nkInitBlobIntro:\n    .incbin "init_state_intro.bin"\n' > $GEN/init_blob.s
  python3 $CONV/mk_test_input.py $REF 98 ${TEST_FRAMES:-5600} $GEN/test_input.s
fi
# voces de la intro (docs/40): las muestras del SPC extraídas de la ROM (data/extracted_full/audio)
[ -f $GEN/md_voice.s ] && [ -n "$ONLY_VOICE" ] || python3 $CONV/mkvoice.py $EXT/audio $GEN
# pantallas de antes del juego (docs/36): título (grabación del SNES: snes_ref/title.rec) y menú (snes_ref/menu_snaps)
if [ -z "$ONLY_VOICE" ] && [ -z "$SKIP_FRONT" ]; then
  mkdir -p $GEN/front
  python3 $ROOT/tools/genesis_convert/make_title_data.py $SMROM $EXT $SNAP/title.rec $GEN/front --proj $GEN >/dev/null
  python3 $ROOT/tools/genesis_convert/make_menu_data.py $SMROM $EXT $SNAP/menu_snaps $GEN/front --proj $GEN >/dev/null
fi
