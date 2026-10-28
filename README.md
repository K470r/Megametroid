# MegaMetroid

Super Metroid para la Sega Genesis / Mega Drive. El código del juego está reescrito a mano para el 68000 a partir del
decomp de snesrev (`sm/`), y todo lo que viene de la ROM se convierte en el PC desde **tu propia ROM**: este
repositorio no contiene nada de Nintendo (ni la ROM, ni datos extraídos, ni ROM compiladas).

Estado: intro, título y menú; Ceres completo; Crateria y el camino de Brinstar a la bola morfo. Detalle y lo que
falta: [docs/46](docs/46_ESTADO_Y_CONTINUACION.md). Índice de la documentación: [docs/README.md](docs/README.md).

## Qué hace falta

- Linux (Debian/Ubuntu probado): `sudo apt install git build-essential gcc-m68k-linux-gnu python3 python3-numpy
  python3-pil util-linux`
- Tu volcado de **Super Metroid (Japón / EE. UU.)**, 3 MB, sin cabecera de copiadora (la quita `check_rom.py`):
  SHA-1 `da957f0d63d14cb441d215462904c4fa8519c613`, MD5 `21f3e98df4780ee1c667b84e57d88675`.

## Compilar

    sh tools/setup/setup_tools.sh                     # baja y compila: decomp (sm/), SGDK, Genesis Plus GX, snes9x, retrorun
    python3 tools/check_rom.py /ruta/a/tu_volcado.smc # comprueba la ROM y la copia a sm/sm.smc
    sh tools/inputs/make_inputs.sh                    # extrae y graba desde tu ROM lo que se necesita (~2 min, ~4 GB en work/)
    . work/inputs/env.sh
    make full                                         # datos (res/gen/) + ROM -> out/rom.bin

La ROM sale igual byte a byte que la del estado actual (ver [docs/45](docs/45_REPRODUCIBLE.md)).
Después de la primera vez, para recompilar solo el código: `make` (`make TEST=1` con la entrada grabada,
`make clean` para empezar de cero). Detalle de la estructura y del `Makefile`: [docs/47](docs/47_ESTRUCTURA_SGDK.md).

## Probar

    sh tests/regress/regress.sh          # la imagen de la intro, Ceres y Zebes contra la aprobada (0 distintos)
    make host                            # el motor en el PC (host/check: comparación con el SNES)

## Estructura

Organizada como un proyecto de SGDK (`src/`, `inc/`, `res/`, `out/`):

| Ruta | Qué es |
|---|---|
| `Makefile` | compila la ROM (`make`, `make data`, `make full`, `make host`, `make clean`) |
| `src/core/`, `inc/core/` | el motor (C sin SGDK, igual en el PC y en la Genesis); `port_gen.c/.h` los genera `tools/port` del decomp |
| `src/platform/`, `inc/platform/` | la capa de la Genesis: VDP, dibujo, mando, sonido |
| `src/front/`, `inc/front/` | título y menú (con SGDK) |
| `src/boot/` | cabecera del cartucho (`rom_header.c`) y enlace SSF2 (`md_ssf2.ld`) |
| `res/` | recursos: `resources.res` (rescomp, arte propio), `snes_ref/` (claves de dibujo), `gen/` (lo que sale de tu ROM, fuera de git) |
| `out/` | lo compilado: `rom.bin`, `rom.out`, `rom.map`, `symbol.txt` |
| `host/` | el motor en el PC: comparación con el SNES (`check`), estado inicial (`mkinit`) |
| `tools/conv/` | conversores de la ROM a datos de la Genesis (`mkdata`, `mkmd`, `mkzebes`, `mkzobj`, `mkintro`...) |
| `tools/` | traductor del decomp (`port/`), extractor, grabador del SNES (`snes_ref/`), arnés de emulador, medición |
| `tests/regress/` | regresión de imagen: entradas de mando grabadas y hashes por línea de la imagen aprobada |
| `docs/` | documentación (en español) |

## Licencia

MIT ([LICENSE](LICENSE)): se puede usar, copiar, modificar y distribuir sin restricción, siempre que se conserve el
aviso con los autores: **S í n d r o m e y Claude (Anthropic)**. Lo derivado del decomp (snesrev/sm) conserva también su
aviso MIT. SGDK, Genesis Plus GX y snes9x tienen sus propias licencias y no se incluyen (los baja `setup_tools.sh`).
