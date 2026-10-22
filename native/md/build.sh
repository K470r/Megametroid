#!/bin/sh
# MegaMetroid — ROM de la Genesis con el motor nativo (docs/32).
#   1) tablas del motor (native/host/build.sh) y estado inicial (mkinit)
#   2) datos de la Genesis (native/tools/mkmd.py)
#   3) compilar con gcc para 68000 y enlazar con SGDK
# Variables: GDK (SGDK con lib/libmd.a y lib/libgcc.a), CROSS (prefijo del gcc de 68000), SMROM, REF (grabación
# hlref de Ceres), SNAP (capturas de VRAM del SNES, ver docs/32), TEST=1 (entrada de la grabación), SKIP_DATA=1
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
N=$HERE/..
ROOT=$N/..
GDK=${GDK:-$ROOT/third_party/SGDK}
CROSS=${CROSS:-m68k-linux-gnu-}
CC=${CROSS}gcc
OBJCOPY=${CROSS}objcopy
SMROM=${SMROM:-$ROOT/sm/sm.smc}
SNAP=${SNAP:-$HERE/snes_ref}
OUT=$HERE/out
mkdir -p $OUT $HERE/gen
if [ -z "$SKIP_DATA" ]; then
  sh $N/host/build.sh >/dev/null
  $N/host/mkinit ${REF:?falta REF (grabación hlref)} 97 $HERE/gen
  # la intro: la RAM del SNES al elegir partida nueva (cabecera de la grabación de la intro, hlref con HL_ANY desde el
  # cuadro en que empieza el estado 30) y sus gráficos (docs/38)
  $N/host/mkinit ${REF_INTRO:?falta REF_INTRO (grabación hlref de la intro)} -1 $HERE/gen init_state_intro
  python3 $N/tools/mkintro.py $SMROM $ROOT/data/extracted_full $SNAP $HERE/gen
  python3 $N/tools/mkmd.py $SMROM $ROOT/data/extracted_full $SNAP $N/gen $HERE/gen $SNAP/draw.log $SNAP/hud.log
  MM_MAX_PALS=2 python3 $N/tools/mkzebes.py $SMROM $N/gen 2>/dev/null
  python3 $N/tools/mkgetaway.py $SNAP/getaway.npz $HERE/gen
  python3 $N/tools/mkm7.py $SNAP $HERE/gen
  printf '    .section .rodata\n    .align 2\n    .global kInitBlob\nkInitBlob:\n    .incbin "init_state.bin"\n    .align 2\n    .global kInitBlobIntro\nkInitBlobIntro:\n    .incbin "init_state_intro.bin"\n' > $HERE/gen/init_blob.s
  python3 $N/tools/mk_test_input.py $REF 98 ${TEST_FRAMES:-5600} $HERE/gen/test_input.s
fi
# voces de la intro (docs/40): las muestras del SPC extraídas de la ROM (data/extracted_full/audio)
[ -f $HERE/gen/md_voice.s ] && [ -n "$SKIP_DATA" ] || python3 $N/tools/mkvoice.py $ROOT/data/extracted_full/audio $HERE/gen
# pantallas de antes del juego (docs/36): título (grabación del SNES: snes_ref/title.rec) y menú (snes_ref/menu_snaps)
if [ -z "$SKIP_DATA" ] && [ -z "$SKIP_FRONT" ]; then
  mkdir -p $HERE/gen/front
  python3 $ROOT/tools/genesis_convert/make_title_data.py $SMROM $ROOT/data/extracted_full $SNAP/title.rec $HERE/gen/front --proj $HERE/gen >/dev/null
  python3 $ROOT/tools/genesis_convert/make_menu_data.py $SMROM $ROOT/data/extracted_full $SNAP/menu_snaps $HERE/gen/front --proj $HERE/gen >/dev/null
fi
FL="-m68000 -fdisable-tree-bswap ${OPT:--O3 -funroll-loops -finline-limit=1500 -fira-loop-pressure} -fomit-frame-pointer -ffreestanding -fno-builtin -fno-strict-aliasing -fno-store-merging -U_FORTIFY_SOURCE -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-main -DNT_FADE_LIST -DNT_SSF2"
[ -n "$TEST" ] && FL="$FL -DMD_TEST_INPUT"
[ -n "$TEST_INTRO" ] && FL="$FL -DMD_TEST_INTRO"     # con TEST: la entrada de la grabación de la intro
[ -n "$NOHAZE" ] && FL="$FL -DMD_NO_HAZE"
[ -n "$LAGLOG" ] && FL="$FL -DMD_LAGLOG -DLAG_BASE=${LAG_BASE:-0}"
[ -n "$LATE_MAX" ] && FL="$FL -DLATE_MAX=$LATE_MAX"
[ -n "$EXPFL" ] && FL="$FL $EXPFL"
[ -n "$STOP_AT" ] && FL="$FL -DMD_STOP_AT=$STOP_AT"
SG="-I$GDK/inc -I$GDK/res -DSGDK_GCC -fms-extensions"
rm -f $OUT/*.o
for f in $N/src/*.c $N/gen/vars.c $N/gen/tables.c $N/gen/zebes.c $HERE/md_engine.c $HERE/md_zebes.c $HERE/md_cone.c $HERE/gen/md_data.c $HERE/gen/md_getaway.c $HERE/gen/md_m7.c $HERE/gen/md_intro_data.c $HERE/gen/init_state.c; do
  # intro.c (la intro y sus cinemáticas, una vez por partida): -O2, el desenrollado de -O3 le sumaba 30 KB a la
  # parte fija de la ROM (4 MB)
  case $f in */intro.c) F2=$(echo "$FL" | sed 's/-O3 -funroll-loops -finline-limit=1500 -fira-loop-pressure/-O2/');; *) F2=$FL;; esac
  $CC $F2 -c $f -o $OUT/$(basename $f .c).o
done
$CC $FL $SG -c $HERE/md_main.c -o $OUT/md_main.o
$CC $FL $SG -c $HERE/md_snd.c -o $OUT/md_snd.o
# título y menú: SGDK (sus objetos se llaman front_*: md_ssf2.ld no los vuelve a poner en cero al empezar otra partida)
FF="-m68000 -fdisable-tree-bswap -O2 -fomit-frame-pointer -fno-builtin -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -I$HERE/front -I$HERE/gen/front $SG"
for f in title menu sound front; do $CC $FF -c $HERE/front/$f.c -o $OUT/front_$f.o; done
for f in title_data menu_data; do $CC $FF -c $HERE/gen/front/$f.c -o $OUT/front_${f}_c.o; done
(cd $HERE/gen && for f in title_data menu_data; do $CC -m68000 -c front/$f.s -o $OUT/front_${f}_s.o; done)
$CC $FL $SG -c $HERE/rom_header.c -o $OUT/rom_header.o
mkdir -p $HERE/out_hdr/out && $OBJCOPY -O binary $OUT/rom_header.o $HERE/out_hdr/out/rom_header.bin
cp $GDK/src/boot/sega.s $OUT/sega.s
(cd $HERE/out_hdr && $CC -x assembler-with-cpp -Wa,--register-prefix-optional,--bitwise-or -m68000 $SG -c $OUT/sega.s -o $OUT/sega.o)
(cd $HERE/gen && $CC -m68000 -c md_bins.s -o $OUT/md_bins.o && $CC -m68000 -c init_blob.s -o $OUT/init_blob.o && $CC -m68000 -c md_voice.s -o $OUT/md_voice.o)
EXTRA=""
if [ -n "$TEST" ]; then (cd $HERE/gen && $CC -m68000 -c test_input.s -o $OUT/test_input.o); EXTRA=$OUT/test_input.o; fi
$CC -m68000 -fno-use-linker-plugin -fno-lto -n -T $HERE/md_ssf2.ld -nostdlib $OUT/sega.o $(ls $OUT/*.o | grep -v -e sega.o -e rom_header.o -e test_input.o) $EXTRA \
    $GDK/lib/libmd.a $GDK/lib/libgcc.a -o $OUT/rom.out -Wl,--build-id=none -Wl,-z,noexecstack -Wl,--gc-sections -Wl,-Map,$OUT/rom.map
$OBJCOPY -O binary $OUT/rom.out $OUT/rom.bin
${CROSS}nm -n $OUT/rom.out > $OUT/symbol.txt
python3 $ROOT/tools/emu_harness/md_fix_rom.py $OUT/rom.bin
ls -l $OUT/rom.bin
