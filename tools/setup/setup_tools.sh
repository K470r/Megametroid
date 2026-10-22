#!/bin/sh
# MegaMetroid — baja y compila lo que el proyecto usa y no es nuestro, cada cosa en la versión con que se probó
# (docs/45). Todo queda dentro del proyecto (sm/, third_party/, tools/bin/), fuera de git (.gitignore).
#
#   sm/                 decomp de Super Metroid (snesrev/sm, MIT) 578f90b — tu ROM va en sm/sm.smc
#   third_party/SGDK    SGDK ee6870a (trae lib/libmd.a y lib/libgcc.a ya compiladas)
#   tools/bin/genesis_plus_gx_libretro.so   Genesis Plus GX 58c3414 + tools/emu_harness/gpgx_mm_harness.patch
#   tools/bin/gpgx_prof/genesis_plus_gx_libretro.so   con gpgx_mm_profile.patch (arnés + perfil del 68000, opcional)
#   tools/bin/snes9x_libretro.so            snes9x fae2fea (capturas del título y el menú)
#   tools/bin/retrorun                      tools/emu_harness/retrorun.c (corre núcleos libretro sin pantalla)
#
# Requisitos del sistema (Debian/Ubuntu): sudo apt install git build-essential gcc-m68k-linux-gnu python3 \
#   python3-numpy python3-pil util-linux default-jre-headless
# Uso: sh tools/setup/setup_tools.sh [--prof]
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $ROOT/third_party $ROOT/tools/bin
clone() {   # clone url commit dir
  if [ ! -d "$3/.git" ]; then
    git clone -q "$1" "$3.tmp"
    git -C "$3.tmp" checkout -q "$2"
    mkdir -p "$3"
    (cd "$3.tmp" && tar cf - .) | (cd "$3" && tar xf -)   # (sm/ puede tener ya sm.smc)
    rm -rf "$3.tmp"
  fi
}
echo "== decomp (sm/)";            clone ${SM_URL:-https://github.com/snesrev/sm} 578f90b3cc49557bb70060ad033bb90b8cf8ac50 $ROOT/sm
echo "== SGDK";                    clone ${SGDK_URL:-https://github.com/Stephane-D/SGDK} ee6870a6010b1e341ca36078f581a68a26c9a543 $ROOT/third_party/SGDK
echo "== Genesis Plus GX"
G=$ROOT/third_party/gpgx
clone ${GPGX_URL:-https://github.com/libretro/Genesis-Plus-GX} 58c341487e5bfcf979ea68413c7987633adb0c56 $G
if [ ! -f $ROOT/tools/bin/genesis_plus_gx_libretro.so ]; then
  (cd $G && git apply --ignore-whitespace $ROOT/tools/emu_harness/gpgx_mm_harness.patch && make -s -j4 -f Makefile.libretro > /dev/null)
  cp $G/genesis_plus_gx_libretro.so $ROOT/tools/bin/
fi
if [ "$1" = "--prof" ] && [ ! -f $ROOT/tools/bin/gpgx_prof/genesis_plus_gx_libretro.so ]; then
  (cd $G && git checkout -q -- . && git apply --ignore-whitespace $ROOT/tools/emu_harness/gpgx_mm_profile.patch \
     && make -s -j4 -f Makefile.libretro clean > /dev/null && make -s -j4 -f Makefile.libretro > /dev/null \
     && cp genesis_plus_gx_libretro.so /tmp/gpgx_prof.so && git checkout -q -- . \
     && git apply --ignore-whitespace $ROOT/tools/emu_harness/gpgx_mm_harness.patch)
  mkdir -p $ROOT/tools/bin/gpgx_prof && mv /tmp/gpgx_prof.so $ROOT/tools/bin/gpgx_prof/genesis_plus_gx_libretro.so
fi
echo "== retrorun"
gcc -O2 -I$G/libretro/libretro-common/include -o $ROOT/tools/bin/retrorun $ROOT/tools/emu_harness/retrorun.c -ldl -lm
echo "== snes9x"
S=$ROOT/third_party/snes9x
clone ${SNES9X_URL:-https://github.com/libretro/snes9x} fae2fea08f74180759ef540ee94259213f503480 $S
[ -f $ROOT/tools/bin/snes9x_libretro.so ] || { make -s -j4 -C $S/libretro > /dev/null && cp $S/libretro/snes9x_libretro.so $ROOT/tools/bin/; }
echo "listo"
