# 47 — Estructura del proyecto (SGDK) y recursos

Fecha: 2026-10-22. El repositorio pasó de `native/` a la estructura de un proyecto SGDK (`src/`, `inc/`, `res/`,
`out/`, un `Makefile` en la raíz). El código no cambió: solo de lugar, y los `#include "../src/x.h"` pasaron a
`#include "x.h"` con las carpetas en `-I`. La ROM sale igual byte a byte (ver "Cómo se comprobó").

## Dónde quedó cada cosa

| Antes | Ahora | Qué es |
|---|---|---|
| `native/src/*.c` | `src/core/` | el motor (sin SGDK; compila igual en el PC) |
| `native/src/*.h` | `inc/core/` | sus cabeceras (`port_gen.h` lo escribe aquí `tools/port/port.py`) |
| `native/src/vars_extra.txt` | `src/core/vars_extra.txt` | variables propias del motor (`gen_vars.py`) |
| `native/md/md_*.c` | `src/platform/` | la capa de la Genesis (`md_intro.c` lo incluye `md_engine.c`) |
| `native/md/*.h` | `inc/platform/` | |
| `native/md/front/` | `src/front/`, `inc/front/` | título y menú (con SGDK) |
| `native/md/rom_header.c`, `md_ssf2.ld` | `src/boot/` | cabecera del cartucho y enlace SSF2 |
| `native/md/build.sh` | `Makefile` + `tools/conv/make_data.sh` | compilar / generar los datos |
| `native/md/snes_ref/` | `res/snes_ref/` | claves de dibujo (`*.log`, en git) y capturas del SNES (fuera de git) |
| `native/gen/` | `res/gen/engine/` | tablas del motor generadas (fuera de git) |
| `native/md/gen/` | `res/gen/md/` | datos de la Genesis generados desde la ROM (fuera de git) |
| `native/md/out/` | `out/` (`out/obj/` los objetos) | `rom.bin`, `rom.out`, `rom.map`, `symbol.txt` |
| `native/host/` | `host/` | el motor en el PC (`check`, `mkinit`...) |
| `native/tools/` | `tools/conv/` | conversores de la ROM |

## Compilar

| Antes | Ahora |
|---|---|
| `sh native/md/build.sh` | `make full` (= `make data` + `make`) |
| `cd native/md && SKIP_DATA=1 sh build.sh` | `make` |
| `SKIP_DATA=1 TEST=1 sh build.sh` | `make TEST=1` |
| `... OPT=-O2 LAGLOG=1 sh build.sh` | `make OPT=-O2 LAGLOG=1` (o en el entorno, como antes) |
| `sh native/host/build.sh` | `make host` (o `sh host/build.sh`) |
| `rm -rf native/md/out` | `make clean` |

Las variables son las mismas (`GDK`, `CROSS`, `OPT`, `TEST`, `TEST_INTRO`, `NOHAZE`, `LAGLOG`, `LAG_BASE`, `LATE_MAX`,
`STOP_AT`, `EXPFL`; para `make data`: `REF`, `REF_INTRO`, `SMROM`, `SNAP`, `TEST_FRAMES`, `SKIP_FRONT`).

Diferencias con `build.sh`:
- Compila solo lo que cambió (dependencias de los `.h` con `-MMD`). Si cambian las opciones (`TEST=1`, `OPT`...)
  recompila todo (`out/obj/flags.txt`). Tras tocar algo de SGDK, `make clean`.
- `make` sin los datos de `res/gen/` avisa en vez de fallar a mitad.
- `make` nunca rehace el estado inicial (`mkinit`): eso lo hace `make data` / `make full` (ver docs/45 y docs/46).

### Por qué no `include $(GDK)/makefile.gen`

El `makefile.gen` de SGDK compila todo `src/` con las mismas opciones y enlaza con su `md.ld`. Este proyecto
necesita: el enlace SSF2 propio (`md_ssf2.ld`: páginas en 0x380000, variables del juego juntas para reiniciar la
partida), el motor sin las cabeceras de SGDK, `-fdisable-tree-bswap` y `-fno-store-merging` (lecturas impares,
docs/44), `intro.c` a `-O2` (4 MB de parte fija) y el orden de enlace de siempre (`ls` de los objetos). Con
`makefile.gen` la ROM cambiaría. El `Makefile` sigue sus convenciones (GDK, `src/ inc/ res/ out/`, rescomp,
`libmd.a`, el `sega.s` de SGDK) sin esas diferencias.

## Recursos (`res/`)

| Carpeta | En git | Qué va |
|---|---|---|
| `res/resources.res` | sí | arte **propio** para rescomp (logos, fuentes, gráficos nuevos sin nada de la ROM) |
| `res/snes_ref/*.log` | sí | listas de claves de dibujo (solo direcciones) |
| `res/gen/` | no | todo lo que los conversores sacan de la ROM de cada quien |
| `res/private/` | no | redibujos hechos a partir de la ROM (ver abajo) |

`res/resources.res` empieza vacío: mientras no declare nada, `make` no lo compila y la ROM no cambia. Cuando se le
agregue algo (`IMAGE`, `SPRITE`, `PALETTE`, `XGM2`, `WAV`...), `make` corre rescomp (`$(GDK)/bin/rescomp.jar`) y
enlaza `out/obj/res_resources.o`; su cabecera es `res_resources.h` (en `-I` para `src/platform/md_main.c`,
`md_snd.c` y `src/front/`). Sus datos van en la parte fija de la ROM (máximo 4 MB). Las reglas de la VDP siguen:
PNG indexado de 16 colores (el índice 0 transparente), tiles de 8×8 y sprites hasta 32×32 por pieza.

### Redibujos hechos sobre la ROM

Un redibujo de un tile o sprite del juego es una **obra derivada**: el trazo nuevo es tuyo, pero el personaje y el
diseño siguen siendo de Nintendo, así que la regla de docs/43 también vale para ellos: no van al repositorio
público. Para tenerlos versionados y que el `Makefile` los use:

1. Un **repositorio privado aparte** (p. ej. `MegaMetroid-private`) con solo esos archivos, montado como submódulo
   en `res/private/` o simplemente clonado ahí (la carpeta está en `.gitignore`).
2. Uno o más `.res` dentro de `res/private/` (p. ej. `res/private/samus.res`, con rutas relativas a esa carpeta).
   `make` los compila solo si existen y declaran algo: `out/obj/res_private_samus.o` y su cabecera
   `res_private_samus.h`. Sin `res/private/` (un colaborador que no lo tiene) la ROM sale como siempre.

Las ROM compiladas con esos recursos tampoco se distribuyen (docs/43, docs/45).

## Cómo se comprobó (2026-10-22)

Sin la ROM del SNES no se puede compilar la ROM entera, así que se compararon el árbol viejo (`build.sh`) y el nuevo
(`Makefile`) con las cabeceras generadas reemplazadas por archivos de prueba (con los mismos `#include` que
escriben los conversores), y las variables y enumeraciones generadas de verdad desde el decomp:
- las 53 unidades de compilación, preprocesadas (`-E -P`): iguales;
- las opciones de cada una (sin las `-I`): iguales;
- la línea de enlace (objetos y su orden): igual, salvo la ruta del `.map`;
- en 5 variantes: normal, `TEST=1`, `TEST=1 TEST_INTRO=1`, `NOHAZE=1 LAGLOG=1 LAG_BASE=40`, `OPT=-O1`;
- `gen_vars.py` / `gen_enums.py`: la misma salida (cambian solo las líneas `#include`);
- `host/`: las mismas unidades preprocesadas (cambia solo el texto que escribe `mkinit`);
- rescomp: un `.res` de prueba compila y enlaza.

Falta lo que solo se puede hacer con la ROM: `make full` y `sh tests/regress/regress.sh` (0 distintos), y comparar
el md5 de `out/rom.bin` con el de la última entrega.
