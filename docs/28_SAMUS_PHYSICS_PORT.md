# 28 — Física de Samus: port mecánico del decomp y verificación 1:1 contra el SNES

## Objetivo

Que Samus en la Genesis se comporte **exactamente** como en el Super Metroid del SNES:
caminar, correr, salto normal y giratorio, agacharse, morfosfera, pendientes, wall jump,
speed booster, space jump, hi-jump, spring ball y caídas. Fuera de alcance por ahora:
disparo, grapple y shinespark (el código está, pero no se probó).

En vez de reescribir la física a mano (el motor anterior de `src/samus` y `port_sources`),
se toma el código C del decomp **snesrev/sm** (licencia MIT, `port/decomp/LICENSE_snesrev_sm.txt`)
y se transforma de forma mecánica para el 68000. Luego se comprueba frame a frame contra el
juego real, que corre en un emulador de SNES.

## Resultado

| Escenario (Landing Site) | Frames | PC, C | PC, C++ "LE" | **Genesis (68000, GPGX)** |
|---|---|---|---|---|
| `landing_basic`, todos los ítems | 1693 | 1693 idénticos | 1693 idénticos | **1693 idénticos** |
| `landing_walljump`, todos los ítems | 2103 | 2103 idénticos | 2103 idénticos | **2103 idénticos** |
| `landing_noitems`, sin ítems | 1693 | 1693 idénticos | 1693 idénticos | **1693 idénticos** |

- En cada frame se comparan **228 variables de Samus**: posición, subposición, velocidades,
  poses, tipo de movimiento, temporizadores, estado del speed booster, ecos, colisiones,
  scroll de la cámara, etc. (`port/diff/samus_mask.txt`).
- Las columnas de PC tienen dos modos:
  - **Resincronizado:** cada frame parte del estado real del SNES.
  - **Libre:** solo el frame 0 viene del SNES y después el port corre solo.

  Ambos modos dan 100 %.
- La Genesis corre en modo libre durante toda la grabación: el estado inicial viene del SNES y
  en cada frame recibe solo lo que el NMI del SNES habría actualizado (mando y contadores).
- **Costo en el 68000:** tras las optimizaciones (ver "ROM jugable"), en promedio unas 86
  scanlines por frame con el minimapa incluido, con un máximo de unas 160 de las 262 de un frame
  NTSC. La ROM jugable, que no calcula el minimapa, corre a 60 fps en todas las pruebas.

## Cómo funciona

### 1. Extracción del núcleo (`port/tools/extract_funcs.py`, `port/diff/core_funcs.txt`)

- **Núcleo:** 406 funciones de `sm_90/91/92/94` alcanzables desde el frame de Samus, más
  34 auxiliares copiadas tal cual (matemática, aleatorio, eventos, scroll) en `sm_extra.c`.
- **Stubs:** todo lo que queda fuera (enemigos, PLMs, audio, HUD) se genera como stub que
  cuenta sus llamadas (`port/tools/gen_stubs.py`). Así se ve si la física depende de algo que
  no está.

### 2. Orden del frame (de `GameState_8_MainGameplay`)

1. `HandleControllerInputForGamePhysics`
2. `HandleSamusMovementAndPause`
3. `MainScrollingRoutine`
4. `DrawSamusAndProjectiles`
5. `HandleSamusOutOfHealthAndGameTile`
6. `DecrementSamusTimers`

### 3. Memoria "little-endian" para el 68000 (`port/le/le_mem.h`, `port/tools/make_le_sources.py`)

El código del decomp lee y escribe la RAM y la ROM del SNES como bytes en orden
little-endian. En lugar de reescribirlo:

- La RAM y la ROM del juego conservan **la disposición exacta de bytes del SNES**.
- Todo acceso de 16 o 32 bits a esa memoria pasa por tipos C++ `le16`, `le16s`, `le32` y `le32s`,
  que leen y escriben byte a byte. Así no hay accesos a direcciones impares y el orden de bytes
  es el correcto.
- `LOBYTE` y `HIBYTE` se resuelven según sean variables nativas o memoria del SNES.
- El núcleo se compila como C++. La transformación es un script, no una edición a mano. Los
  pocos casos que no puede resolver solo están en `port/tools/le_patches.py`, cada uno con su
  motivo.

### 4. Mapa de RAM en la Genesis

| RAM del SNES | Buffer en Genesis | Contenido |
|---|---|---|
| 7E:0000–1FFF | `g_ram_lo` (8 KB) | variables del motor y de Samus |
| 7E:9800–9BFF | `g_ram_98` (1 KB) | tabla HDMA de los rayos X |
| 7E:C000–C7FF | `g_ram_pal` (2 KB) | paletas y tilemap del HUD |
| 7E:CD00–CDFF | `g_ram_cd` | scrolls |
| 7E:D600–D91F | `g_ram_d8` | estelas de proyectil, eventos, jefes |
| 7F:0000–9601 | `g_ram_7f` (37.5 KB, del heap) | level_data y BTS de la sala |

Cualquier otra dirección se convierte en un símbolo indefinido (`mm_unmapped_ram_0xNNNN`).
Si el núcleo llegara a usar una zona no mapeada, **falla el enlace** en vez de corromper memoria.

La RAM de la Genesis (64 KB) no alcanza para todo esto más el búfer DMA por defecto de SGDK
(8 KB). Por eso `main()` reduce ese búfer a 2 KB (`DMA_setBufferSize`) y reserva
`level_data`/BTS con `MEM_alloc`.

### 5. Prueba contra el SNES real

- `tools/emu_harness/retrorun.c` ejecuta cualquier núcleo libretro sin pantalla, a partir de un
  guion.
- `tools/snes_ref/make_sram.py` crea una partida guardada en la nave, con todos los ítems o sin
  ninguno.
- `tools/snes_ref/scenarios.py` define las secuencias de mando.
- **En snes9x:** se graba la RAM completa (128 KB) en cada frame.
- **En PC:** `port/host/diff_host.c` corre el núcleo y compara.
- **En Genesis:**
  - `tests/samus_diff` es una ROM con el núcleo, el estado inicial y los mandos grabados.
  - `tools/emu_harness/md_samus_diff.c` la corre en Genesis Plus GX y compara la RAM del 68000
    contra la grabación, frame por frame. También mide cuántas scanlines tarda cada frame.

Variables excluidas de la comparación (`port/diff/exclude.txt`), porque no son física de Samus
y el resto del juego también las modifica dentro del frame:

- `random_number`
- `oam_next_ptr`
- `vram_write_queue_tail`
- `active_enemy_indexes`

### Cambios al código de snesrev (`port/diff/decomp_changes.md`)

| Archivo | Cambio | Motivo |
|---|---|---|
| sm_90.c `Samus_HandleSpeedBoosterAnimDelay` | Tras `QueueSfx3_Max6(3)` se fija `v2 = 0x0003` | snesrev "corrigió" un bug del juego original. El SNES real se comporta como byte alto 0: con 0x0003 no hay diferencias; con la versión de snesrev, 2 frames distintos. |
| sm_90.c `Samus_HandleAnimDelay` | Se desactiva un `printf` de depuración | Sin efecto en la lógica. |

## Hallazgos importantes para todo el proyecto

1. **Compilador:** hay que usar un **`m68k-elf-gcc` configurado para 68000**, como el que trae
   SGDK. El `m68k-linux-gnu-gcc` de las distribuciones Linux supone un 68020 y genera `move.w`
   y `move.l` a direcciones impares, incluso con `-m68000 -mstrict-align`. En la Genesis eso
   termina en *ADDRESS ERROR*. En la nube se compiló GCC 13.3 `m68k-elf` (`--with-cpu=68000`)
   y SGDK con ese mismo compilador.
2. **Cabecera de ROM:** la cabecera por defecto de SGDK declara SRAM en 0x200000. Con una ROM de
   más de 2 MB se solapa con los datos. `tests/samus_diff/src/rom_header.c` no declara SRAM.
3. **Memoria:** ver el punto 4 de "Cómo funciona".

## Cómo reproducir

Necesitas `sm.smc` (tu propio volcado; no se distribuye), snes9x y Genesis Plus GX en versión
libretro, y SGDK con su `m68k-elf-gcc`.

```sh
# 1) grabar las trazas del SNES (una vez)
python3 tools/snes_ref/scenarios.py landing_basic > ref/landing_basic.inp
retrorun snes9x_libretro.so sm.smc guion.txt   # guion: load ship.state / hold 1 - / trace ref/landing_basic.trace 0:131072 / input ref/landing_basic.inp

# 2) prueba en PC (C++ "LE")
SMSRC=<decomp>/src port/build_le_host.sh
port/build_le/host/diff_le sm.smc ref/landing_basic.trace port/diff/samus_mask.txt \
    port/diff/inject_nmi.txt port/diff/exclude.txt 100000 libre

# 3) prueba en Genesis
gcc -O2 -I<libretro-common/include> tools/emu_harness/md_samus_diff.c -ldl -o md_samus_diff
GDK=<SGDK> GPGX=genesis_plus_gx_libretro.so tests/samus_diff/run_diff.sh ref
```

Los archivos de apoyo de la grabación están en `tests/snes_ref/`: estados guardados en la nave,
SRAM, mandos por frame y el guion de arranque. Las trazas no se incluyen porque ocupan unos
200 MB cada una; se regeneran con los pasos anteriores.

## ROM jugable: Samus en las 243 salas (244 entradas) (`tests/samus_play`)

Samus corre con el núcleo verificado en cualquier sala del juego, a 60 fps.

**Qué hace la capa Genesis** (`tests/samus_play/src/main.c` y `port/genesis/samus_play_glue.cpp`). Es
el equivalente del NMI y del PPU del SNES; la física es la del núcleo, sin cambios:

- **Mando.** Traduce el mando de Genesis a botones del SNES y repite `ReadJoypadInputs` (sm_80.c)
  tal cual, con autorrepetición y teclas nuevas.
- **Salas.**
  - Carga cada sala como `LoadRoomHeader` / `LoadStateHeader` (copiadas del decomp) más la parte
    de scrolls y level data de `LoadLevelDataAndOtherThings`.
  - El level data y el BTS van comprimidos con un LZ propio (`port/genesis/mm_lz.c`).
- **Fondos.**
  - Dibuja los planos A y B desde el level data real del SNES (bloques de 16x16) con una tabla de
    metatiles por sala.
  - Esa tabla guarda las 4 variantes de flip ya calculadas y reproduce celda por celda el mapa
    convertido (el generador lo comprueba en las 243 salas).
  - El plano se actualiza como máximo con una columna y una fila por capa y por frame; cada
    columna va en una sola DMA.
- **Sprites de Samus.**
  - `DrawSamusSpritemap` entrega el spritemap a la capa Genesis (`mm_samus_spr_hook`), que usa
    registros precalculados de sprites Genesis.
  - Un sprite SNES de 16x16 se dibuja como dos sprites de 16x8.
  - Los tiles de Samus se copian igual que `NmiTransferSamusToVram`, desde sus gráficos
    convertidos a 4bpp de Genesis con los mismos desplazamientos.
  - La paleta de sprites 4 del SNES va a `PAL3`; otra paleta, si aparece, va a `PAL2`.
- **ROM del SNES.** Solo se incluyen los bancos que lee el núcleo (`port/tools/rom_banks.py`). Si
  alguna vez leyera otro, la pantalla muestra `FALTA BANCO ROM xx`.

**Controles**

| Mando | Acción | Botón del SNES |
|---|---|---|
| C | saltar | A |
| A | correr | B |
| B | disparar (todavía no dispara) | X |
| X (6 botones) | apuntar abajo | L |
| Z (6 botones) | apuntar arriba | R |
| Y (6 botones) | seleccionar ítem | Select |
| MODE (6 botones) | cancelar | Y |
| START + arriba/abajo (3 botones) | apuntar arriba/abajo | R / L |
| START + izquierda/derecha | sala anterior / siguiente | — |
| START + A | cambiar entre esquema de 3 y 6 botones | — |

El esquema de 6 botones se elige solo si el mando lo es.

**Verificación**

| Prueba | Resultado |
|---|---|
| `tools/emu_harness/md_play_e2e.py`: las grabaciones del SNES reproducidas como mando Genesis de 6 botones desde el arranque de la ROM jugable | `landing_basic` 1693/1693 y `landing_walljump` 2103/2103 frames idénticos; ninguna vuelta por debajo de 60 fps |
| Recorrido automático de las 243 salas: entrar, caminar, saltar | Sin bancos faltantes; Samus siempre dentro de la sala |
| Carrera larga con speed booster y ecos en Landing Site (la sala más pesada: dos capas) | 60 fps |

**Optimizaciones.** Todas se verificaron con las pruebas diferenciales:

- Variables de RAM en dirección par y tablas fijas de ROM leídas con un acceso de palabra
  (`le16a`, `le32a`).
- Acceso directo a bancos de ROM constantes.
- `-O3` y LTO del núcleo en un solo objeto.
- El minimapa del HUD no se calcula (`mm_skip_minimap`); todavía no hay HUD.

Resultado: el núcleo pasó de unas 123 a unas 86 scanlines por frame en promedio, con minimapa.

**Medición.** `tools/emu_harness/gpgx_mm_profile.patch` agrega a Genesis Plus GX un perfil de
ciclos por instrucción y por sección del bucle (variable `mm_prof_section`).
`prof_report.py` lo resume por función.

**Compilar.**

- **Windows:** `tests\samus_play\build.bat`. Usa el gcc de SGDK, la biblioteca ya compilada
  `lib\libmm_samus_core.a` y los datos de `data\`.
- **Linux:** `tests/samus_play/build.sh` regenera todo, incluido el núcleo C++. Necesita
  `m68k-elf-g++` para 68000; el SGDK de Windows no trae compilador de C++.

**Limitaciones actuales.** Todavía faltan:

- Puertas: se cambia de sala con START + izquierda/derecha. Si Samus sale de la sala, reaparece.
- Enemigos y PLMs: la nave de Landing Site es un enemigo, por eso no se ve.
- Agua, lava y ácido (FX de capa 3).
- HUD.
- Disparos.
- El cielo con HDMA de Landing Site: por ahora la capa 2 sigue a la capa 1.

### Salas de Ceres con modo 7 (236, 237 y 242)

En el SNES dos salas de Ceres no se dibujan con el level data, y por eso salían con basura:

| Sala (índice) | En el SNES | En la Genesis |
|---|---|---|
| 236 Ceres Shaft (llegada) | BG en modo 7 durante toda la sala. La VRAM del tileset ($C0:E22A) va intercalada: byte bajo = mapa de 128x128, byte alto = píxeles 8bpp. Degradado "haze" azul por línea (HDMA de color). | Imagen del plano de modo 7 convertida a tiles (120 tiles, 76 bloques, 2 paletas). El haze se hace con una interrupción horizontal cada 8 líneas que cambia el color de fondo: se ve en todo lo negro de la torre. |
| 237 Ceres Shaft (huida) | Estado `IsBossDead`: paleta $C2:C1E3, haze rojo y la torre se balancea (`RoomCode_CeresElevatorShaft`, tabla $89:AD5F, pivote (128, 1008)). | Balanceo estilo Castlevania Bloodlines: scroll horizontal por línea + vertical por columna de 16 px (rotación de primer orden con los mismos M7A/M7B del SNES y la misma tabla). Plano de 64x64 solo en esta sala. El núcleo recibe M7A-M7Y y `ceres_status`, así Samus se dibuja rotada igual que en el SNES (`Samus_CalcPos_Mode7`). |
| 242 Ceres Ridley Boss Room | `BG12NBA = $66`: la capa 2 usa tiles de VRAM $C000 (ROM $B0:A000). Solo se ve el piso; el resto es el haze sobre negro. | Piso + degradado horneados en tiles con tramado ordenado 4x4 (la cámara de esta sala no se mueve). Comparado con el SNES: el piso coincide píxel a píxel. |

- Generador: `tools/genesis_convert/ceres_display.py` (lo llama `make_samus_play_data.py`).
  Campos nuevos de `PlayRoom`: `disp` (mapa de bloques propio), `fill` (bloque de relleno
  fuera de la sala, el plano de modo 7 lo repite) y `fx`.
- Con la interrupción horizontal activa, los accesos al VDP del bucle principal van entre
  `SYS_disableInts()` / `SYS_enableInts()` (`VDP_GUARD`).
- Las tres salas corren a 60 fps (medido con `mm_loop_count`, Samus cayendo por el pozo).
- Todavía no: la puerta y el contenedor del bebé metroide de la sala de Ridley son objetos
  (PLM / enemigo) y llegarán con esos sistemas; tampoco la huida de Ridley (el modo 7 con Ridley
  alejándose).

## Pendiente

- Puertas y transiciones entre salas (código de puertas del SNES).
- Agua, lava y ácido; HUD; disparos; enemigos.
- **Archivar el motor anterior:** `src/samus` y `port_sources` pasan a `archive/`.
- **Optimizar el costo por frame** sin cambiar el resultado. La prueba diferencial es la red de
  seguridad: cualquier optimización tiene que seguir dando 100 %.
- Más escenarios: agua, lava, pendientes pronunciadas y otras áreas.
