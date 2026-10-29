# MegaMetroid

Super Metroid para la Sega Genesis / Mega Drive. El código del juego está reescrito a mano para el 68000 a partir del
decomp de snesrev (`sm/`), y todo lo que viene de la ROM se convierte en el PC desde **tu propia ROM**: este
repositorio no contiene nada de Nintendo (ni la ROM, ni datos extraídos, ni ROM compiladas).

Estado (2026-10-29): se juega desde el encendido hasta Brinstar. Lo que anda, lo que falta y el plan, más abajo
([Estado del port](#estado-del-port)). Detalle técnico: [docs/46](docs/46_ESTADO_Y_CONTINUACION.md). Índice de la
documentación: [docs/README.md](docs/README.md).

## Qué hace falta

- Linux (Debian/Ubuntu probado) o Windows con WSL (Ubuntu). En una sola línea:

      sudo apt update && sudo apt install git build-essential gcc-m68k-linux-gnu python3 python3-numpy python3-pil util-linux default-jre-headless

  (Java lo usan rescomp, lz4w y xgm2tool de SGDK.) Si `apt` da "404 Not Found", falta el `sudo apt update`.
- Tu volcado de **Super Metroid (Japón / EE. UU.)**, 3 MB, sin cabecera de copiadora (la quita `check_rom.py`):
  SHA-1 `da957f0d63d14cb441d215462904c4fa8519c613`, MD5 `21f3e98df4780ee1c667b84e57d88675`.

## Compilar la ROM de la Genesis

En Linux, o en Windows con WSL (Ubuntu). La primera vez tarda un rato (las herramientas, las grabaciones del SNES y
`make full`, este solo de 8 a 10 minutos) y usa ~5 GB de disco; todo queda dentro de la carpeta del proyecto.

1. Instalar lo necesario (una vez): el comando de "Qué hace falta", arriba.
2. Bajar el proyecto y entrar en la carpeta:

       git clone https://github.com/K470r/Megametroid
       cd Megametroid

3. Bajar y compilar las herramientas (decomp, SGDK, emuladores):

       sh tools/setup/setup_tools.sh

4. Poner tu volcado de Super Metroid (lo comprueba y lo copia a `sm/sm.smc`):

       python3 tools/check_rom.py /ruta/a/tu_volcado.smc

5. Extraer y grabar desde tu ROM lo que se necesita (~2 min, ~4 GB en `work/`):

       sh tools/inputs/make_inputs.sh

6. Compilar:

       . work/inputs/env.sh && make full

La ROM queda en **`out/rom.bin`** (11 MB, mapeador SSF2): se carga en MiSTer, en la Mega EverDrive o en un emulador
con SSF2 (Genesis Plus GX, BlastEm). Sale igual byte a byte para cualquiera que use la misma ROM (ver
[docs/45](docs/45_REPRODUCIBLE.md)): con `md5sum out/rom.bin` se puede comparar con la de otro.

### Actualizar a lo último

    cd Megametroid
    git checkout main
    git pull
    . work/inputs/env.sh && make full

`make full` (8-10 min) cuando cambian los conversores (`tools/`); si solo cambió el código, basta `make`. Si cambió
`tools/setup/` o `tools/inputs/`, repetir antes los pasos 3 y 5.

### Comandos

| Comando | Qué hace |
|---|---|
| `make full` | todo: los datos desde tu ROM (`make data`) y la ROM |
| `make` | solo recompila el código (los datos de `res/gen/` ya hechos) |
| `make data` | solo los datos desde tu ROM (`tools/conv/make_data.sh`) |
| `make snd` | rehace la música y los efectos (`tools/snd/mksnd.py`, con `tools/snd/instrumentos.txt`) y la ROM |
| `make fur` | las canciones del juego como proyectos de Furnace en `work/furnace/` (para elegir instrumentos a oído) |
| `make host` | el motor en el PC (`host/check`: comparación cuadro a cuadro con el SNES) |
| `make TEST=1` | la ROM de prueba con la entrada de mando grabada (la usa la regresión) |
| `make clean` | borra `out/` |

Detalle de la estructura y del `Makefile`: [docs/47](docs/47_ESTRUCTURA_SGDK.md).

### Instrumentos de la música (Furnace)

La música sale de las secuencias del SNES de tu ROM, con un instrumento FM por muestra (`tools/snd/instrumentos.txt`,
en git: solo números). Para ajustarlos a oído ([docs/55](docs/55_SONIDO_XGM2.md)):

    make fur                                                       # proyectos en work/furnace/ (+ muestras/ y la lista)
    # en Furnace: abrir el .fur, cambiar los instrumentos FM (nombres CONJ_MUESTRA, p. ej. 03_25), guardar
    python3 tools/snd/fur2tabla.py work/furnace/03_5_titulo.fur --envolvente-snes --escribir
    make snd                                                       # la ROM con los instrumentos nuevos

## Probar

    sh tests/regress/regress.sh          # la imagen de la intro, Ceres y Zebes contra la aprobada (0 distintos)
    sh tools/rooms/beams.sh              # rayos contra el SNES (también sba.sh, powerbomb.sh, chargebeam.sh...)
    make host                            # el motor en el PC (host/check: comparación con el SNES)

La regresión usa la ROM de prueba (`TEST=1`), que empieza sin el título: después hay que probar también el arranque
normal (`out/rom.bin`) en un emulador.

## Estado del port

**Anda:**
- Encendido (logo Síndrome), título con su música y los chillidos del bebé metroide, menú de partidas y opciones.
- Intro completa con voces, música y efectos; Ceres entero (Ridley, la huida, la explosión) y el viaje a Zebes.
- Zebes: todas las salas a las que se llega por puertas están convertidas (6 áreas); Crateria con sus enemigos y el
  camino de Brinstar hasta la bola morfo y los primeros misiles; pausa y mapa ([docs/53](docs/53_PAUSA_Y_MAPA.md)).
- Samus completa: movimientos, trajes, gancho, salto espacial, shinespark ([docs/56](docs/56_SAMUS_COMPLETA.md));
  rayos y sus combinaciones, rayo cargado, ataques especiales ([docs/49](docs/49_RAYO_CARGADO.md),
  [57](docs/57_CLAVES_DE_RAYOS.md)); misiles, bombas, bomba de poder con su luz, enemigos congelados
  ([docs/58](docs/58_HIELO_BOMBA_ESTELAS.md)).
- Spore Spawn ([docs/50](docs/50_SPORE_SPAWN.md)), Torizo ([docs/51](docs/51_TORIZO.md)), ojos y su haz de luz
  ([docs/52](docs/52_OJO_HAZ.md)).

**Falta, en este orden** (decidido el 2026-10-29):
1. Música y efectos de sonido de todo el juego (hoy, del título a la llegada a Zebes): hacerles lugar en la ROM
   (una ventana de la ROM para el sonido, por área: [docs/48](docs/48_TAMANO_ROM.md)). En curso.
2. Los enemigos de todas las salas y sus particularidades (bloques con el ícono del arma que los rompe), por áreas.
3. Los efectos de las salas: lava, ácido, agua, lluvia, niebla.
4. Los jefes, que se puedan vencer como en el SNES.
5. El juego de principio a fin, con los rayos X y las salidas secretas.

Después: la música del Mega CD (si hay Mega CD y el disco de MegaMetroid, la música sale del CD; si no, la del
cartucho: [docs/48](docs/48_TAMANO_ROM.md)), la traducción al español y los instrumentos ajustados a oído.

## Estructura

Organizada como un proyecto de SGDK (`src/`, `inc/`, `res/`, `out/`):

| Ruta | Qué es |
|---|---|
| `Makefile` | compila la ROM (ver "Comandos") |
| `src/core/`, `inc/core/` | el motor (C sin SGDK, igual en el PC y en la Genesis); `port_gen.c/.h` los genera `tools/port` del decomp |
| `src/platform/`, `inc/platform/` | la capa de la Genesis: VDP, dibujo, mando, sonido |
| `src/front/`, `inc/front/` | título y menú (con SGDK) |
| `src/boot/` | cabecera del cartucho (`rom_header.c`) y enlace SSF2 (`md_ssf2.ld`) |
| `res/` | recursos: `resources.res` (rescomp, arte propio), `snes_ref/` (claves de dibujo), `gen/` (lo que sale de tu ROM, fuera de git) |
| `out/` | lo compilado: `rom.bin`, `rom.out`, `rom.map`, `symbol.txt` |
| `host/` | el motor en el PC: comparación con el SNES (`check`), estado inicial (`mkinit`) |
| `tools/conv/` | conversores de la ROM a datos de la Genesis (`mkdata`, `mkmd`, `mkzebes`, `mkzobj`, `mkintro`...) |
| `tools/snd/` | el sonido: el N-SPC del decomp en el PC (`spcrun.c`), `mksnd.py` (XGM2), `mkfur.py` y `fur2tabla.py` (Furnace) |
| `tools/` | traductor del decomp (`port/`), extractor, grabador del SNES (`snes_ref/`), arnés de emulador, medición |
| `tests/regress/` | regresión de imagen: entradas de mando grabadas y hashes por línea de la imagen aprobada |
| `docs/` | documentación (en español) |

## Licencia

MIT ([LICENSE](LICENSE)): se puede usar, copiar, modificar y distribuir sin restricción, siempre que se conserve el
aviso con los autores: **S í n d r o m e y Claude (Anthropic)**. Lo derivado del decomp (snesrev/sm) conserva también su
aviso MIT. SGDK, Genesis Plus GX y snes9x tienen sus propias licencias y no se incluyen (los baja `setup_tools.sh`).
