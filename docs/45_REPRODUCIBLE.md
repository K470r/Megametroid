# 45 — Compilación reproducible desde la ROM de cada quien

Desde el repositorio (solo nuestras fuentes) cualquiera con su propio cartucho rehace todo lo que no se distribuye y
compila la ROM de la Genesis del estado actual. Comprobado el 2026-10-21: en una copia limpia, con solo `sm.smc`, los
pasos de abajo dan la misma ROM **byte a byte** que la carpeta de trabajo (md5 `0b544cb73e823b471e932ddef1679f3d`,
crateria7) y la regresión da 0 diferencias.

## Pasos

    sudo apt install git build-essential gcc-m68k-linux-gnu python3 python3-numpy python3-pil util-linux
    sh tools/setup/setup_tools.sh                 # decomp (sm/), SGDK, Genesis Plus GX, snes9x, retrorun
    python3 tools/check_rom.py ~/super_metroid.smc   # comprueba el hash y la deja en sm/sm.smc
    sh tools/inputs/make_inputs.sh                # ~2 min y ~4 GB en work/: extracción, grabaciones, capturas
    . work/inputs/env.sh                          # REF y REF_INTRO
    sh native/md/build.sh                         # -> native/md/out/rom.bin
    sh tests/regress/regress.sh                   # 0 distintos = la imagen aprobada

Las voces de la intro salen de las muestras del SPC extraídas (`data/extracted_full/audio`, songsets 3F y 42:
`native/tools/mkvoice.py`, docs/40). No hace falta ningún archivo aparte.

## Qué rehace `make_inputs.sh` y con qué

| Archivo | Cómo | Comprobado |
|---|---|---|
| `data/extracted_full/` | `tools/sm_extract/extract_all.py` | igual (salvo `summary.json`, con fechas; `audio/SuperMetroid.sf2` no lo usa nadie) |
| `ref_full.bin` (REF) | `hlref` con `ceres_completo_in.txt`, desde el cuadro 10600 (empieza en el 10659, estado 31) | byte a byte |
| `ref_intro0.bin` (REF_INTRO) | `HL_ANY=1 hlref ... 2201 2400` (la cabecera = estado inicial de la intro) | byte a byte |
| `ref_intro.bin` | `HL_ANY=1 hlref ... 2202 10720` (la intro entera) | byte a byte |
| `ref_tu.bin` | `hlref` con `in_timeup.txt` (Ceres con el tiempo agotado) | sale igual `cine_traj.json` |
| `snes_ref/<sala>.bin` (12) | `HL_VRAM` en los cuadros de `regs.json` | iguales |
| `CINE1 19098, CINE1_M7 19400, CINE2_M1 19654, CINE2_M7 19760` | `HL_VRAM` en la corrida de `ref_full` | iguales (con OAM) |
| `CINE1_TU 20400, MENU 20750` | `HL_VRAM` en la de `ref_tu` | iguales |
| `E0B5_pal.bin` | CGRAM del cuadro 13247 | igual |
| `regs.json` | `snap_regs.py` | igual |
| `cine_traj.json` | `snap_cine.py ref_full ref_tu` | igual |
| `intro_usage.json` | `tools/snes_ref/intro_usage.py ref_intro.bin` | igual |
| `getaway.npz` | 113 capturas desde el cuadro 15776, con y sin la capa del modo 7 (`HL_NOM7`) | igual |
| `title.rec`, `title.rec.grad` | `record_title.py` con snes9x | iguales |
| `menu_snaps/` | `record_menus.py` con snes9x | iguales |
| `snes_ref/*.log` | se distribuyen (solo direcciones y claves de dibujo) | — |

Los cuadros de las capturas se ubicaron con `HL_VDUMP` (VRAM de cada cuadro) y `tools/inputs/find_frames.py`.
`HL_NOM7` está en `tools/snes_ref/viewer_check/patch_decomp.py` (apaga BG1 en el modo 7 al dibujar).

## Las entradas de prueba

`tests/regress/inputs/*.s` son datos nuestros (el mando grabado, sin nada de la ROM): no se rehacen. `full.s` sale
de `ref_full.bin` (cuadros 98..). `zebes_shoot.s` salió de una versión anterior de `in_z2.txt`; no hace falta
rehacerla.

## Cuidado al cambiar el motor

`native/md/build.sh` sin `SKIP_DATA` vuelve a generar el estado inicial (`mkinit`) con la lista de variables del
motor de ese momento. Hasta el 2026-10-21 la carpeta de trabajo compilaba siempre con `SKIP_DATA=1` y el estado
inicial era del 14 de octubre (le faltaban las variables nuevas); ahora es el de las grabaciones. Si se agregan
variables al motor, conviene una compilación completa de vez en cuando y aprobar la regresión.
