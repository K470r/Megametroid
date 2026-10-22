#!/bin/sh
# MegaMetroid — rehace, desde la ROM de cada quien, todo lo que la compilación necesita y no se distribuye (docs/45).
#
#   1. data/extracted_full           recursos extraídos de la ROM (tools/sm_extract/extract_all.py)
#   2. grabaciones del SNES (hlref, el decomp corriendo tu ROM) con las entradas guardadas del bot:
#        ref_full.bin   Ceres -> Zebes (REF de Makefile: estado inicial, cuadro 97)
#        ref_intro0.bin la intro, desde el cuadro 2201 (REF_INTRO: su cabecera es el estado inicial de la intro)
#        ref_intro.bin  la intro entera desde el 2202 (intro_usage.json)
#        ref_tu.bin     Ceres con el tiempo agotado (cinemática y menú de fin)
#   3. res/snes_ref/           capturas de VRAM de las salas y cinemáticas, regs.json, cine_traj.json,
#                                    getaway.npz (huida de Ridley), intro_usage.json
#   4. con snes9x (SNES9X=núcleo libretro): title.rec, title.rec.grad, menu_snaps/
# Cada paso se comprobó contra los archivos con que se compiló el estado actual: salen iguales byte a byte.
#
# Uso: sh tools/inputs/make_inputs.sh
# Variables: SMROM (sm/sm.smc), SMSRC (sm/src: el decomp de snesrev), RETRORUN (tools/bin/retrorun),
#            SNES9X (tools/bin/snes9x_libretro.so), WORK (carpeta temporal: ~4 GB), KEEP_REFS=1 (no borrar las
#            grabaciones: las piden algunas pruebas y herramientas de medición)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SMROM=${SMROM:-$ROOT/sm/sm.smc}
SMSRC=${SMSRC:-$ROOT/sm/src}
RETRORUN=${RETRORUN:-$ROOT/tools/bin/retrorun}
SNES9X=${SNES9X:-$ROOT/tools/bin/snes9x_libretro.so}
WORK=${WORK:-$ROOT/work/inputs}
SNAP=$ROOT/res/snes_ref
BOT=$ROOT/tools/snes_ref/ceres_bot
mkdir -p $WORK $SNAP
[ -f "$SMROM" ] || { echo "falta la ROM: python3 tools/check_rom.py <tu volcado> (la deja en sm/sm.smc)"; exit 1; }
python3 $ROOT/tools/check_rom.py "$SMROM"

echo "== 1. recursos extraídos (data/extracted_full)"
python3 $ROOT/tools/sm_extract/extract_all.py "$SMROM" $ROOT/data/extracted_full > $WORK/extract.log

echo "== 2. hlref (decomp de $SMSRC)"
HL=$WORK/hlref
[ -x $HL ] || sh $ROOT/tools/snes_ref/viewer_check/build_hlref.sh "$SMSRC" $HL
run() { setarch -R "$@" 2>&1 | grep -v '^v1=' | grep -E 'inicio|cuadros' || true; }
cd $WORK
# Ceres -> Zebes, con las capturas de VRAM de las salas y cinemáticas (cuadro: el de regs.json / docs/45)
echo "   ref_full.bin + capturas de VRAM"
HL_VRAM="10780:$SNAP/DF45.bin,11400:$SNAP/DF8D.bin,11800:$SNAP/DFD7.bin,12150:$SNAP/E021.bin,12400:$SNAP/E06B.bin,\
12620:$SNAP/E0B5.bin,13247:$WORK/E0B5_full.bin,16220:$SNAP/E0B5_ESC.bin,16360:$SNAP/E06B_E097.bin,16660:$SNAP/E021_E04D.bin,\
16960:$SNAP/DFD7_E003.bin,17460:$SNAP/DF8D_DFB9.bin,18160:$SNAP/DF45_DF71.bin,19098:$SNAP/CINE1.bin,19400:$SNAP/CINE1_M7.bin,\
19654:$SNAP/CINE2_M1.bin,19760:$SNAP/CINE2_M7.bin" run $HL "$SMROM" $BOT/ceres_completo_in.txt $WORK/ref_full.bin 10600
python3 -c "import sys; d=open(sys.argv[1],'rb').read(); open(sys.argv[2],'wb').write(d[0x10000:0x10200])" \
  $WORK/E0B5_full.bin $SNAP/E0B5_pal.bin                     # solo los colores (CGRAM)
echo "   ref_intro0.bin (estado inicial de la intro) y ref_intro.bin (la intro entera, para intro_usage.json)"
HL_ANY=1 run $HL "$SMROM" $BOT/ceres_completo_in.txt $WORK/ref_intro0.bin 2201 2400
HL_ANY=1 run $HL "$SMROM" $BOT/ceres_completo_in.txt $WORK/ref_intro.bin 2202 10720
echo "   ref_tu.bin (tiempo agotado) + capturas"
HL_VRAM="20400:$SNAP/CINE1_TU.bin,20750:$SNAP/MENU.bin" run $HL "$SMROM" $BOT/in_timeup.txt $WORK/ref_tu.bin 10600

echo "== 3. snes_ref"
python3 $ROOT/tools/conv/snap_regs.py $WORK/ref_full.bin 10659 $SNAP DF45:10780 DF8D:11400 DFD7:11800 E021:12150 \
  E06B:12400 E0B5:12620 E0B5_ESC:16220 E06B_E097:16360 E021_E04D:16660 DFD7_E003:16960 DF8D_DFB9:17460 DF45_DF71:18160 > /dev/null
python3 $ROOT/tools/conv/snap_cine.py $SNAP/cine_traj.json $WORK/ref_full.bin $WORK/ref_tu.bin
python3 $ROOT/tools/snes_ref/intro_usage.py $WORK/ref_intro.bin $ROOT/data/extracted_full $SNAP/intro_usage.json
# huida de Ridley: la imagen con y sin la capa del modo 7 (HL_NOM7), 113 cuadros desde el 15776
mkdir -p $WORK/ga/a $WORK/ga/b
HL_SHOTS=15776,1,15890,$WORK/ga/a/s run $HL "$SMROM" $BOT/ceres_completo_in.txt /dev/null 10600 15900
HL_NOM7=1 HL_SHOTS=15776,1,15890,$WORK/ga/b/s run $HL "$SMROM" $BOT/ceres_completo_in.txt /dev/null 10600 15900
python3 $ROOT/tools/conv/snap_getaway.py $WORK/ga/a $WORK/ga/b 15776 $SNAP/getaway.npz
rm -rf $WORK/ga

echo "== 4. título y menú (snes9x)"
if [ -f "$SNES9X" ] && [ -x "$RETRORUN" ]; then
  python3 $ROOT/tools/snes_ref/record_title.py "$RETRORUN" "$SNES9X" "$SMROM" $SNAP/title.rec
  python3 $ROOT/tools/snes_ref/record_menus.py "$RETRORUN" "$SNES9X" "$SMROM" $SNAP/menu_snaps
else
  echo "   (sin $SNES9X o $RETRORUN: sh tools/setup/setup_tools.sh los compila)"; exit 1
fi

cat > $WORK/env.sh <<EOF
# para Makefile y las herramientas que leen las grabaciones
export REF=$WORK/ref_full.bin
export REF_INTRO=$WORK/ref_intro0.bin
EOF
echo "listo. Para compilar:  . $WORK/env.sh && make full"
