# 30 — El juego completo portado: Ceres, la huida y la llegada a Zebes

## Resultado

El código del juego del SNES (decomp snesrev/sm) corre transformado para el 68000 y da **la misma RAM
que el SNES en cada cuadro** desde que termina la intro hasta Samus parada en Landing Site:

| Tramo | Estados de juego | Cuadros |
|---|---|---|
| Carga de la partida en Ceres, juego en Ceres, puertas y salas | 31, 6, 7, 8, 9-11 | 0-5560 |
| Ridley: pelea, se lleva a la bebé, autodestrucción | 8 | ~1930-5570 |
| Huida con el contador, subida por el pozo con el vaivén | 8, 9-11 | 5570-8220 |
| Ascensor, apagón, la nave escapa, explosión, viaje, llegada y aterrizaje en Zebes | 32, 33, 34, 6, 7 | 8222-10210 |
| Samus en Landing Site | 8 | 10210-10741 |

**10741 cuadros, 0 distintos** (los 128 KB de RAM del SNES, byte por byte).

Por ahora esto corre en PC (el mismo código C++ que irá a la Genesis, con el mismo modelo de memoria).
Para la Genesis faltan el mapa de RAM definitivo (abajo) y el dibujo.

## Cómo está hecho (igual que docs/28 y docs/29)

1. **Referencia del SNES** (`tools/snes_ref/ceres_bot/`):
   - `hlbot`: un bot que juega Ceres en el decomp y graba la entrada (`ceres_completo_in.txt`).
   - `hlref`: vuelve a correr esa entrada y graba la RAM de cada cuadro (y la RAM + VRAM iniciales).
     Son unos 1,4 GB; no se guardan, se regeneran en 10 s.
2. **Funciones que se portan** (`port/diff/game_funcs.txt`): las que el SNES ejecuta en la intro y en
   Ceres (grabadas con el decomp en PC; 1391). `filter_dispatch.py` deja solo esas en las tablas de
   despacho de 18 bancos (80, 81, 82, 84, 86, 87, 88, 89, 8B, 8D, 8F, 93, 9B, A0, A2, A6, A9, B4); el
   núcleo de Samus (90, 91, 92, 94 y `sm_extra.c`) es el ya verificado.
3. **Versión LE** (`make_le_sources.py --game`): igual que antes, más:
   - mapa de RAM del juego completo: `lo` (7E:0000-1FFF), `2k` (7E:2000-BFFF), `c0` (7E:C000-FFFF) y
     `7f` (7F:0000-FFFF). Las zonas grandes van enteras porque el juego recorre arreglos que cruzan
     límites (ej.: `SaveToSram` recorre los mapas explorados CD52+0x100 por área y la partida guardada
     D7C0-DE1B con un puntero).
   - `g_ram` con dirección variable (`g_ram + x`, `&g_ram[x]`) pasa por `mm_ram`, que busca la zona.
   - **campos lejanos de los enemigos**: la estructura de un enemigo empieza en 7E:0F78+k y sigue, con
     relleno, en 7E:7800+k, 8000+k y 8800+k. En la Genesis no están seguidos: esos campos son
     `mm_far16` (`port/le/le_mem.h`), que calculan su dirección del SNES por su propia posición y
     leen/escriben en la zona que corresponde. Sin cambiar ni una línea del código de los enemigos.
   - parches de C++ (`port/tools/game_patches.py`): saltos sobre inicializaciones, conversiones; misma
     lógica.
4. **Pegamento** (`port/genesis/game_glue.cpp`): lo que en el SNES es hardware:
   - el cuadro es `RunOneFrameOfGame` (juego + NMI del decomp) y después la pantalla línea a línea con
     el IRQ vertical (`Vector_IRQ`: el HUD y las puertas reprograman la línea siguiente), como
     `DrawFrameToPpu`;
   - registros: mando, multiplicador del modo 7 (`MPYM`), puerto de VRAM (lectura y escritura), puerto
     de WRAM ($2180) y DMA (hacia la RAM se hace de verdad; hacia la VRAM, en la Genesis irá al dibujo
     por `mm_game_dma_hook`).
5. **Comparación** (`port/host/game_md_host.c`, `port/build_game_host.sh`): corre el port con cada
   zona en su bloque (página protegida al final) y compara toda la RAM con la referencia. Opciones:
   `MM_WATCH` (variables por cuadro), `MM_DUMP` (RAM del port y del SNES en un cuadro) y
   `MM_VOLATILE` (ver abajo).

Compilado para 68000 (`m68k-elf-g++ -O2`): unos **355 KB de código**. Cabe sin problema en el
cartucho (SSF2, docs/29).

## La RAM: el problema grande de la Genesis

La Genesis tiene 64 KB de RAM; el SNES 128 KB. `MM_VOLATILE` llena una zona de basura antes de cada
cuadro: si el resto de la RAM sigue igual al SNES, el juego no necesita esa zona entre cuadros
(`port/tools/scan_ram_volatile.sh`, resultados en `port/diff/game_ram_scan.txt`):

| Zona | Uso entre cuadros | Qué es |
|---|---|---|
| 7E:0000-1FFF | sí | variables del motor, Samus, enemigos, PLMs |
| 7E:2000-27FF | sí | `tilemap_stuff` (Ridley la usa como variables) |
| 7E:2800-6FFF | no | gráficos descomprimidos para la VRAM |
| 7E:7000-8FFF | sí | datos de enemigos y su RAM extra (7800+k, 8000+k, 8800+k) |
| 7E:9000-9FFF | no | tablas HDMA (se rehacen cada cuadro) |
| 7E:A000-B7FF | solo para dibujar | tabla de bloques 16x16 → mapa del SNES (la Genesis tiene la suya) |
| 7E:C000-C3FF | dentro del cuadro | paletas (se rehacen desde las de destino) |
| 7E:C400-C7FF | sí | paletas de destino, HUD |
| 7E:C800-CBFF | no | filas/columnas de fondo por copiar a la VRAM (dibujo) |
| 7E:CC00-DFFF | sí | scrolls, mapas explorados, partida guardada, PLMs, eventos |
| 7E:E000-EBFF | no | |
| 7E:EC00-EFFF | sí | |
| 7E:F000-FFFF | no | |
| 7F:0000-6401 | sí | level data de la capa 1 (colisión) |
| 7F:6402-9601 | sí | BTS (tipo de cada bloque: pendientes, puertas...) |
| 7F:9602-... | solo para dibujar | level data de la capa 2 |

Lo que el juego necesita entre cuadros, sin dibujo: unos 26 KB fijos + la sala. La sala más grande
(Big Pink) tiene 25,6 KB de capa 1 y 12,8 KB de BTS: 64 KB justos, sin lugar para SGDK. Plan:
- **BTS disperso**: como mucho 628 bloques con BTS distinto de 0 en una sala (West Ocean). Guardado como
  tabla (~4 KB en vez de 12,8 KB), con un tipo que imita al arreglo (`BTS[i]`), igual que `mm_far16`.
- **Capa 2 en ROM** (no cambia): no se descomprime a la RAM.
- **Gráficos**: la Genesis ya los tiene convertidos; las descompresiones que solo van a la VRAM no
  se hacen.
- **Dibujo**: la Genesis dibuja desde el level data con sus bloques (como `tests/samus_play`); las
  funciones que llenan 7E:C8C8 (filas/columnas para la VRAM del SNES) no se ejecutan.
- La RAM del cartucho no sirve: en el MiSTer la SRAM es de 8 bits y ocupa 0x200000-0x3FFFFF cuando
  está activa (tapa la ROM), y el mapeador extendido de EverDrive ("SEGA SSF", SRAM de 16 bits) no
  existe en el MiSTer.

## En la Genesis: `tests/game_md` (ROM de prueba de Ceres)

El mismo código corre en la Genesis con el mapa de RAM de 64 KB (`make_le_sources.py --game-md`,
`port/build_game_md_host.sh`, `tests/game_md/build.sh`):

- **RAM**: solo las zonas que el juego usa de un cuadro al siguiente (unos 36 KB, `game_ram_map.h`). El level data
  y el BTS de cada sala están **en la ROM ya descomprimidos** y en la RAM solo va una capa de cambios (puertas que
  se abren, bloques que se rompen: tabla de 1024 entradas; en todo Ceres usa 14). Lo que era solo para la VRAM
  del SNES (descompresiones de gráficos, mapas de fondo, filas por copiar) no se hace (`port/tools/game_md_patches.py`,
  cada parche dice por qué).
- **Dibujo** (`tests/game_md/src/main.c`): fondos con los tiles y bloques ya convertidos de cada sala, desde el
  level data del juego; sprites desde el OAM del SNES (solo las entradas usadas); tiles de sprites copiados por DMA
  desde bancos de la ROM ya convertidos a 4bpp de la Genesis (`--obj-banks`); paletas y brillo desde `palette_buffer`
  e `INIDISP`; haze de Ceres con una interrupción horizontal en asm; vaivén con los registros de modo 7 del juego.
- **Comprobación en la Genesis**: `REPLAY=1 -DMM_REPLAY` compila la ROM con la entrada grabada; tras cada cuadro deja
  una suma por zona de RAM que `tools/emu_harness/game_md_replay.py` compara con el PC (`MM_CKSUM`).
  **10741 cuadros, 0 distintos**: toda la secuencia corre en el 68000 igual que en el SNES.
  Así salieron los errores que en PC no se ven (orden de bytes): los argumentos de `SpawnHdmaObject` y las
  estructuras `BabyMetroidExecState` / `MotherBrainInstrExecState` sobre la RAM.

**Velocidad** (vblanks por cuadro del juego; 1 = 60 cuadros por segundo): 1,9-2 caminando por Ceres, 2,5-3 en la
pelea con Ridley y la huida. Es decir, el juego va a la mitad o a un tercio de la velocidad del SNES. Medido con
el perfil de GPGX: unos 120 000 ciclos del 68000 por cuadro son el código del juego y unos 60 000 lo demás; un
cuadro tiene 128 000. Lo ya hecho: NMI sin registros del PPU que no vuelven a la RAM, IRQ sin recorrer líneas,
división con DIVU, multiplicación directa, fundido de paletas con una división por diferencia, cola propia de DMA
de tiles, -O3, OAM solo con las entradas usadas.

## Pendiente (en este orden)

0. **Velocidad** (lo más importante ahora): memoria del juego en orden de la Genesis (hoy cada acceso de 16 bits
   arma los bytes), dibujo de sprites directo desde los spritemaps (sin pasar por el OAM), las funciones más caras
   reescritas con la misma lógica. Meta: 1 vblank por cuadro.

1. Mapa de RAM de la Genesis con las zonas de arriba + BTS disperso; volver a comparar (tiene que dar
   0 distintos con las zonas "libres" fuera).
2. Llevar el juego portado a la ROM de la Genesis: reemplaza al núcleo de Samus solo
   (`tests/samus_play`), con el dibujo de salas que ya existe, sprites desde el OAM del SNES (enemigos,
   proyectiles, PLMs), paletas desde `palette_buffer`, HUD.
3. Las escenas de modo 7 (Ridley escapando, la nave, la explosión, la llegada a Zebes): como el vuelo
   de la intro, imágenes preparadas desde los registros que pone el juego.
4. Grabar más recorridos (Crateria, Brinstar...) y sumar sus funciones a `game_funcs.txt`.
