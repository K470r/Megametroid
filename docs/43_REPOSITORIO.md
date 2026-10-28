# 43 — El repositorio git (qué se sube y qué no)

Se crea cuando estén terminados Crateria y la parte de Brinstar de la bola morfo y los 2 misiles. Este documento se
va completando mientras tanto: es la lista de lo que hay que subir, lo que no, y cómo arma cada colaborador el juego.

## La regla

**No se comparte nada que salga de la ROM de Nintendo.** Ni la ROM del SNES, ni lo extraído de ella, ni las ROM de
Genesis ya compiladas (llevan dentro gráficos, mapas y datos del juego). Se sube solo lo nuestro: el motor, las
herramientas que convierten y la documentación. Cada colaborador vuelca su propio cartucho, lo pone en el proyecto y
compila.

## La ROM de cada quien

- Juego: **Super Metroid (Japón / EE. UU.)**, 3 MB (3 145 728 bytes), sin cabecera de copiadora, sin parches.
- SHA-1 `da957f0d63d14cb441d215462904c4fa8519c613`
- MD5 `21f3e98df4780ee1c667b84e57d88675`
- SHA-256 `12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72`
- Dónde va: **`sm/sm.smc`** (dentro del proyecto).
- `python3 tools/check_rom.py tu_volcado.smc` comprueba el hash, quita la cabecera de 512 bytes si la tiene y la copia
  a `sm/sm.smc`.

## Qué SÍ se sube

| Ruta | Qué es |
|---|---|
| `src/core/` | el motor nativo de 68000 (C), incluido `port_gen.c/.h` (generado del decomp, sin datos de la ROM) |
| `Makefile`, `src/platform/`, `inc/platform/`, `src/front/`, `inc/front/`, `src/boot/` | la parte propia de la Genesis (VDP, dibujo, sonido, cabecera, enlace) |
| `host/*.c *.h build.sh` | el motor compilado en el PC: `check`, `mkinit`, `zcover`, `tour`... (sin los binarios) |
| `tools/conv/*.py` | conversores: `mkdata`, `mkmd`, `mkzebes`, `mkzobj`, `mkzkeys`, `mkintro`... (el sonido: `tools/snd/`, docs/55; las tablas `musica.txt`, `efectos.txt` e `instrumentos.txt` son nuestras) |
| `tools/port/` | el traductor del decomp (docs/42), `roots.txt` |
| `tools/genesis_convert/`, `tools/emu_harness/`, `tools/snes_ref/` | conversores, `retrorun`, grabador del SNES (`hlref`) — solo fuentes y scripts |
| `tools/setup/`, `tools/inputs/` | instalar las herramientas; rehacer desde la ROM todo lo que no se sube (docs/45) |
| `tools/check_rom.py` | la comprobación de la ROM |
| `tests/regress/regress.sh`, `tests/regress/inputs/` | la prueba de regresión y las entradas de mando grabadas |
| `tests/regress/golden/*.rows.gz` | hashes por línea de la imagen aprobada (no se puede reconstruir la imagen con ellos) |
| `res/snes_ref/*.log` | listas de claves de dibujo (`zdraw.log`, `draw.log`, `hud.log`...): solo direcciones, sin gráficos |
| `docs/` | toda la documentación (en español) |
| `.gitignore`, `README.md` (por escribir), licencia (por decidir) | |

El decomp de snesrev (`sm/`, licencia MIT) es la biblia: mejor como submódulo git fijado a la versión que usamos que
copiado. Ojo: hoy hay dos copias (`sm/src` y la de trabajo del traductor); antes de subir hay que dejar una sola.

## Qué NO se sube

| Ruta | Por qué |
|---|---|
| `sm/sm.smc`, `sm.smc` y cualquier `*.smc` / `*.sfc` | la ROM del SNES |
| `*.bin` de la raíz (`MegaMetroid_*.bin`, `Samus Play Concept.bin`, `Paletas nivles.bin`...), `out/`, `out/` | ROM de Genesis compiladas / datos de la ROM |
| `data/` (`extracted_full`, `genesis`, `rom`...) | todo lo extraído de la ROM |
| `res/gen/engine/`, `res/gen/md/` | tablas y gráficos generados desde la ROM (se rehacen al compilar) |
| `res/snes_ref/*.bin`, `*.rec`, `*.npz`, `*.json` de capturas, `menu_snaps/` | capturas de VRAM/RAM del SNES corriendo la ROM |
| `res/` | archivos sueltos de trabajo (WAV, covers en mp3): no los usa la compilación |
| grabaciones `hlref` (`ref*.bin`), estados del SNES | llevan RAM y VRAM del juego |
| `repomix-output-*.xml` | volcado de otro repositorio |
| binarios compilados (`host/check`, `mkinit`, `zcover`..., `retrorun`) | se compilan |

## Hecho (2026-10-21) y pendiente

Hecho: la copia limpia (`/home/claude/MegaMetroid_repo`), `README.md`, `tools/setup/setup_tools.sh` (decomp, SGDK,
emulador, snes9x, retrorun en versiones fijas), `tools/inputs/make_inputs.sh` (todas las grabaciones y capturas
desde la ROM de cada quien, comprobadas byte a byte: docs/45), rutas sin `/home/claude` en `Makefile`,
`host/build.sh` y `tests/regress/regress.sh`. Una sola copia del decomp (`sm/`, la baja setup_tools.sh).

Pendiente:
1. `git init`, primer commit y el repositorio remoto: cuando Edwin lo diga.
2. `tools/perf/` y `tools/rooms/` tienen rutas de esta máquina (/tmp/claude-0/...): herramientas de medición y de
   prueba por sala, ajustar al usarlas (tools/rooms/README.md).
3. `tools/emu_harness/native_compare.py`: rutas fijas del emulador (usar tools/bin/).

## Cómo compila un colaborador

    sh tools/setup/setup_tools.sh
    python3 tools/check_rom.py ~/volcados/super_metroid.smc     # -> sm/sm.smc
    sh tools/inputs/make_inputs.sh                                # capturas del SNES desde su ROM
    . work/inputs/env.sh && make full                 # -> out/rom.bin
    sh tests/regress/regress.sh                                   # comprobar que la imagen es la aprobada
