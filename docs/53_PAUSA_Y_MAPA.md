# 53 — Estación de mapa, pausa, mapa y pantalla de equipo

Fecha: 2026-10-22. Rama `map-station` (desde `dedup-tiles`).

## El cuelgue

Al entrar a la sala de la estación de mapa de Crateria (9994, cerca del aterrizaje) y tocar la estación, el juego
quedaba colgado después de que Samus "cargaba" el mapa. Dos cosas faltaban, en este orden:

1. `CallSomeSamusCode(6)` (`SamusCode_06_LockToStation`, $90:F1AA): la estación ($84:B146,
   `ActivateStationIfSamusCannonLinedUp`) deja a Samus quieta con `frame_handler_beta = j_HandleDemoRecorder_2`.
   Ni la orden ni ese manejador estaban.
2. La caja "MAP DATA ACCESS COMPLETED" (`PlmInstr_ActivateMapStation`, $84:8C8F → `DisplayMessageBox(0x14)`): al
   cerrarla, `DisplayMessageBox_Async` ($85:8080) pone `game_state = 12` y **el SNES abre la pausa en el mapa**. Los
   estados 12-18 (la pausa) no estaban portados: `game_frame` caía en "estado de juego sin portar" y no volvía.
   Lo mismo pasaba con START en cualquier sala (`Samus_PauseCheck`, $90:EA45).

Así que la estación de mapa necesita la pausa entera: se portó la pausa con el mapa y la pantalla de equipo.

## Motor (`src/core/pause.c`, banco $82)

Escrito a mano desde `sm_82.c` con los nombres y direcciones del SNES. Estados 13-17 en `pause.c`
(`PauseStates`); 12 y 18 en `game.c` (llaman a `GameState_8_MainGameplay`, como en el SNES).

- Entrar ($82:8CEF): copia de la paleta del juego en 7E:3300, paleta de la pausa ($B6:F000), mapas base de 7E:3400 y
  7E:3800 ($B6:E400/E800), silueta de Samus, reserva, tilemaps del equipo, límites y scroll del mapa
  (`DetermineMapScrollLimits`, `SetupMapScrollingForPauseMenu`).
- Menú ($82:90E8): `MainPauseRoutine` (los 8 `PauseMenu_*`), `ReleaseButtonsFilter` ($80:8146), HUD, paleta animada.
  Mapa: flechas y scroll (`HandleMapScrollArrows`, `MapScrolling`), indicador de Samus, íconos (jefes, estaciones,
  puntos de guardado), destinos de los ascensores. Equipo: las cuatro categorías, equipar y desequipar (rayos con la
  regla spazer/plasma), reserva AUTO/MANUAL (con el `_BUGGY` del original), pasar energía de la reserva, flecha que
  brilla, silueta según el traje.
- Salir ($82:9324/9367): paleta del juego otra vez, `ResumeGameplay` ($80:A149, en `gamestart.c`), `LoadRoomPlmGfx`
  ($84:8232, en `plm.c`), `CallSomeSamusCode(0xC)` (`SamusCode_0C_UnlockFromMapStation`, con `SamusFunc_E633` de $91).
- También: `CallSomeSamusCode` 4, 6 y 0xC (`samus_codes.c`).
- Sin portar (no hacen falta en Crateria): los ganchos de pausa de Kraid, Draygon y Mother Brain (`pause_hook`,
  `unpause_hook`), el hiperrayo en el equipo.

La RAM es la del SNES (7E:3000-3FFF: `cinematic_text_tilemap` y su alias `cinematic_bg_tilemap`). Lo que el SNES
arma en 7E:4000 (el mapa del área, que `QueueClearingOfFxTilemap` pisa enseguida) no se guarda: `PauseMapCell` /
`PauseMapCells8` dan cada celda como `LoadPauseMenuMapTilemap` ($82:943D) y la Genesis la dibuja. Lo que el SNES copia
a la VRAM lo pide `PauseGfx(PAUSE_GFX_ENTER / MAP / EQUIPMENT)` (en el PC no hace nada). Datos de la ROM: `kBank82B`
(tablas del banco $82) y `kBankB6_Pause` (`mkdata.py`: $B6:E000-F1FF, de a palabras).

### Comprobación contra el SNES

`host/check` compara ahora el scroll del BG1 en la pausa (`reg_BG1HOFS/VOFS`, estados 13-17) y deja de comparar
`vram_write_queue_tail` (el motor no usa la cola de la VRAM; el SNES la deja llena en los cuadros de la caja de
mensaje). Grabaciones (`tools/snes_ref/ceres_bot/in_map.txt`, `in_map3.txt`):

    cd work/map
    HL_SETXY=140:100:171 HL_WARP=21:8BDA hlref sm.smc in_map.txt ref_map.bin 23500
    HL_SETXY=140:100:171 HL_WARP=21:8BDA host/check ref_map.bin sm.smc 1 1594 0
    # con objetos, reserva y rayos (equipo):
    HL_ITEMS=1004 HL_POKE=9A6:1003,9A8:1003,9D4:64,9D6:32,9C0:1 ... in_map.txt ref_map2.bin
    HL_ITEMS=1004 HL_POKE=9C2:1E,9D4:64,9D6:32,9C0:2 ... in_map3.txt ref_map3.bin

Samus entra por la puerta 8BDA, se pone en (100, 171), camina hasta la estación, la caja, el mapa, scroll en las
cuatro direcciones, R al equipo, mover el selector, equipar/desequipar (carga) y AUTO/MANUAL, L al mapa, START, y otra
pausa con START. **0 diferencias** en las tres (1594, 1594 y 1300 cuadros). La grabación de Ceres → Zebes
(`ref_full.bin`, desde el cuadro 98) sigue en 0.

## Genesis (`src/platform/md_pause.c`, `tools/conv/mkpause.py`)

`md_pause.c` lo incluye `md_engine.c` (usa su lista de sprites, la CRAM y el HUD). `md_post` y `md_vblank` son los de
siempre: `md_post` tiene un solo desvío al principio (`pz_dim`, estados 12-18) a `md_pause_frame`, y `md_vblank` no
cambia (ver "Regresión" abajo: los tiempos de la intro y de Ceres dependen del ciclo exacto de esas funciones).
- Estados 13-17 (`pz_on`): `md_pause_frame` arma el cuadro (colores, brillo, sprites, HUD) y escribe en la VRAM lo propio
  de la pausa en ese momento (el vblank anterior ya pasó; sin interrupciones mientras escribe). El `md_vblank` de siempre
  escribe sprites, colores, HUD y scroll; la sala de Zebes no se dibuja (`md_zebes = 0` desde que se entra; al salir,
  `zebes_use` lo vuelve a poner).
- Estados 12 y 18 (el juego oscureciéndose o aclarándose; `PauseGfx(PAUSE_GFX_FADE)`): el `md_post` de siempre y
  después el brillo de INIDISP.

| SNES (pausa) | Genesis |
|---|---|
| BG1 64x32 en VRAM 0x3000 (el mapa; o el equipo desde 7E:3800) con `reg_BG1HOFS/VOFS` | plano A 64x32 con el mismo scroll. El BG1 de 64x32 del SNES son dos pantallas de 32x32 seguidas (celda i: pantalla `i >> 10`) |
| BG2 32x32 ($B6:E000) + nombre del área (VRAM 0x38AA) + filas 25-26 desde 7E:3640 | plano B |
| BG3 (el HUD arriba) | la ventana, como en el juego |
| `DrawMenuSpritemap` ($82:C569) | sprites ya convertidos (`kPzSprIdx`/`kPzSprRecs`) |
| tiles de $B6:8000 y $B6:C000 | `kPzTiles` (539 tiles, en la página 18 del SSF2, en los tiles 256-794 de la sala) |
| `kPauseScreenPalettes` | `palette_buffer` con `kPzSrc` (color de la CRAM → índice), igual que las salas |

`mkpause.py` junta todas las palabras que pueden aparecer en el BG1/BG2 (mapas de las 7 áreas explorados o no,
marco, nombres, textos del equipo y sus paletas apagadas, flecha que brilla, dígitos, AUTO/MANUAL, siluetas) y los
spritemaps que dibuja el código con su paleta (íconos, indicador, destinos, flechas, L/R/START, selector, tanques),
y arma tiles, la tabla palabra → celda (`kPzLutT/P/V`: por tile, sus paletas) y los sprites.

**Colores.** La pausa usa 47 colores distintos (por su valor en la CRAM) y la Genesis tiene 45 + 3 libres en la
línea 3 (los otros 12 son del HUD; los colores de la pausa iguales a los del HUD los usan). Se busca un reparto en que
cada tile tenga sus colores en una línea; no hay, así que los colores fijos menos usados pasan al más parecido hasta
que entra: hoy 8 cambios, todos en íconos del mapa (`040->022`, `2A0->4E0`, `022->222`, `200->400`, `842->822`,
`00A->02A`, `CCA->CCC`, `6EE->2EE`; los imprime `mkpause.py`). Los colores que el código cambia (176-191: la paleta
animada; 102 y 107: la flecha de la reserva) tienen su lugar propio y cambian solos.

**Lo que se dibuja de golpe** (entrar, volver del equipo al mapa) va con la pantalla oscura, como en el SNES: los
tiles y el marco con la pantalla apagada; el mapa de a 3 filas por cuadro (`md_pause_post` →
`PauseMapCells8`). El equipo: cada cuadro se comparan las 1024 palabras de 7E:3800 con las mostradas (`cmpm.l`, casi
siempre iguales) y solo las filas distintas pasan a celdas. Así la pausa va a 60 sin perder cuadros; solo pierde
algunos con la pantalla negra (entrar: ~9; salir: lo que tarda en cargarse la sala, ~12).

**Brillo.** Los estados 12-18 se oscurecen con INIDISP (`HandleFadeOut/In`). La línea 3 (HUD arriba, enemigos desde
la línea 32 con la interrupción) la escribe `md_vblank` sin el brillo: mientras no es 15, `md_pause_frame` oscurece
esos colores antes y pone los del HUD también debajo (los enemigos los usan unos cuadros, con la pantalla casi negra;
si no, el fondo del HUD se veía lila un cuadro sí y otro no).

**Al salir**, `ResumeGameplay` llama a `g_hooks.room_gfx`: la Genesis vuelve a cargar la sala entera (tiles, planos,
objetos, haz del ojo) como al entrar por una puerta.

**ROM.** La parte fija estaba casi llena (80 KB libres hasta los 4 MB, docs/48): `pause.c` va con
`#pragma GCC optimize("Os")` (37 → 11 KB) y los tiles en la página 18 (vacía; `mkzebes.py` la cuenta como ocupada si
algún día la necesita). Lo nuevo va **al final de lo común** (`md_ssf2.ld`: `pause.o`, `md_pause_data.o` y las secciones
`.late_text/.late_rodata`, macros `NT_LATE`/`NT_LATE_DATA` de `nt.h`), para no correr las direcciones de lo que ya
estaba; por lo mismo `pause.c` y `md_pause.c` tienen copias de funciones chicas que en el resto quedan inlinadas
(`CancelSoundEffects`, `ClearFlareAnimationState`, `vdp_dma_vram`...: llamarlas las sacaría como funciones aparte).
El tamaño de la ROM no cambia (11 010 048 bytes); lo común termina en 0x340154 (antes 0x335E28).

### Regresión de imagen (`tests/regress/regress.sh`)

- **intro: 0 distintos** (como antes).
- **Ceres / Zebes: 97 distintos** (antes 19), todos de las dos clases que ya estaban en los 19: el pozo que gira
  (7105-7950: filas 88-223, el borde de la rotación) y el círculo de la explosión (8631-8702: los bordes por línea los
  pone una interrupción horizontal) y algunos cuadros mostrados un cuadro de la consola antes o después. Dependen del
  ciclo exacto en que corre el 68000, no del dibujo.
- Prueba de que es eso: la ROM de antes (`dedup-tiles`) **con solo dos incrementos de una variable en `md_post`** (unos
  40 ciclos por cuadro, sin cambiar nada de lo que se dibuja) da Ceres 76 distintos, en los mismos cuadros (7105, 7234,
  7288, 7310, 7350, 7442, 7637, 7676, 7786...). Cualquier código nuevo en el cuadro de juego mueve esos números; mover
  datos de lugar también (solo `kBankB6_Pause` y la página 18, sin código: 18 distintos, otros cuadros).
- Lo que se hizo para que no empeore más: `md_post`/`md_vblank` sin cambios salvo el desvío, lo nuevo al final de la
  ROM, sin funciones nuevas fuera de línea en el medio. Falta que Edwin compare a ojo y apruebe
  (`UPDATE=1 sh tests/regress/regress.sh ceres zebes`, docs/41).

### Mando

6 botones: como en el SNES (Z = R, C = L, START = START). **3 botones** (no tiene L ni R): en la pausa A = L, B = R,
C = A (equipar), START = START (en el juego START sigue siendo corto = SELECT, mantenido = pausa). (El emulador de
prueba conecta un mando de 6 botones: el de 3 botones en la pausa no se probó en retrorun.) En el visor de
salas (`viewer.c`) START + izquierda/derecha cambia de sala; START solo (al soltarlo) abre la pausa.

### Cómo se probó en la Genesis

ROM de prueba (`make TEST=1` con `tests/regress/inputs/zebes_shoot.s`), retrorun: `hold 11700 -`,
`poke <viewer_debug_next> 001B 2` (sala 27 = 9994), `hold 3 SR`, Samus a (100, 171) con `poke`, `hold 150 R`
(llega a la estación), un botón para cerrar la caja, y en la pausa `L`/`U` (scroll), `r` (Z: R), `B` (equipar),
`A` (C: L), `S`. Cuadros perdidos: solo con la pantalla negra (entrar ~11, salir ~12 mientras se carga la sala) y 2-4
en los fundidos de cambio de pantalla (mapa ↔ equipo, mientras se arma el plano); en el mapa y el equipo, ninguno. Comparado con capturas del SNES (`hlref` con `HL_SHOTS`): la misma imagen 2 líneas más abajo (como
todo el juego en la Genesis), íconos y flechas en su lugar. El minimapa del HUD muestra el mapa del área después de la
estación (como el SNES; `UpdateMinimap` ya usaba `has_area_map`).

## Pendiente

- Los ganchos de pausa de Kraid, Draygon y Mother Brain (cuando se llegue a esas salas) y el hiperrayo.
- El mapa del menú de partidas (`GameState_5_FileSelectMap`, `DrawRoomSelectMap`): usa las mismas piezas.
- Sonidos de la pausa (`QueueSfx*`: la cola existe, falta el controlador, docs/46 punto 5).
- La estación de energía y la de misiles (`PlmInstr_ActivateEnergyStation/MissileStation`) usan
  `CallSomeSamusCode(1)` y la caja de mensaje (ya están); la de guardado necesita la SRAM.
