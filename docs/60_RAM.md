# 60 — Plan de RAM: medición (paso 1)

La RAM de la Genesis (64 KB) está casi llena: las variables llegan hasta 0xE0FFE388 y `md_ssf2.ld` no deja pasar de
0xE0FFE3F0. Por encima quedan el montón de SGDK (hasta `MEMORY_HIGH` = 0xE0FFF600) y la pila (2560 bytes). Sin RAM
no caben las cachés que harían falta para el rendimiento (docs/59). Este documento es el paso 1 del plan: medir antes
de tocar. Los informes completos: `docs/60_ram/snes_uso.md` y `docs/60_ram/md_uso.md`.

## Herramientas (`tools/ram/`)

| Script | Qué hace |
|---|---|
| `medir_snes.sh [áreas]` | Graba en el SNES (`hlref`, como `tools/rooms/lote.sh`) las 194 salas con enemigos en lotes de 4 (1500 cuadros por sala, Samus invencible, patrón de disparos); cada grabación pasa por una tubería a `ram_uso.py` (no se guarda: ~800 MB por lote). Sale `work/ram/snes_salas.npz`. |
| `ram_uso.py GRAB ACC` | Suma a un acumulado qué bytes de 7E:0000-7F:FFFF se usan (distinto de cero o cambió), en total y por estado del juego (`game_state`). También para las grabaciones largas que ya había (`work/inputs/ref_intro.bin`, `ref_full.bin`, `ref_tu.bin`). |
| `informe.py vars.h salida ACC...` | Cruza las máscaras con las variables del motor (`vars.h`: dirección del SNES y el tamaño que les dio `gen_vars.py`) y con `enemy_ram7800`. Cuenta solo los estados de juego (no la carga de salas, 6 y 11: ahí el SNES usa 7E:7000-97FF como búfer y `host/check` tampoco la compara). |
| `medir_md.sh [escenas]` + `md_uso.py` | Genesis: las ROM de la regresión (intro, ceres, zebes) con la RAM desde `_bend` por cuadro; recorre el montón de SGDK y mide hasta dónde baja la pila. |
| `md_vars.sh` + `md_ram_uso.py` + `md_informe.py` | Genesis: qué bytes cambia el motor en cada modo (`game_state` y `md_zebes`), y por variable (con los símbolos de cada ROM). |

Medido: 325.980 cuadros del SNES (las 194 salas, la intro, Ceres y el camino a Zebes) y 48.500 de la Genesis (las
tres escenas de la regresión y la pelea con Kraid).

## Resultados

### Genesis: montón de SGDK y pila (lo más grande y lo más barato)

| | Bytes |
|---|---|
| Montón (`_bend` a 0xF600) | 4624 |
| Asignado de forma fija: la cola de DMA de SGDK (80 x 16 + cabecera) | 1282 |
| Asignado un momento al arrancar (cuadro 14, liberado en el 17; por identificar) | 3082 |
| **Libre todo el juego (0xE886-0xF600)** | **3450** |
| Pila: lo más hondo (Ceres y Zebes; intro 1279, Kraid 1318) | 1615 de 2560 |

El motor no usa la cola de DMA de SGDK (hace su propio DMA). Si la asignación del arranque se quita o se hace antes en
otra parte, el límite de `_bend` puede subir ~3 KB. Hay que identificar quién pide esos 3082 bytes en el arranque (la
nota de docs/46 dice que con menos montón la consola no arrancaba: probablemente era eso).

### SNES: arreglos más largos que lo que se usa

`gen_vars.py` da a cada arreglo el largo hasta la variable siguiente del SNES; cuando después hay un hueco, el
arreglo sale más largo que lo que el juego usa. Los que importan (sin contar los que hay que dejar por las partidas
guardadas o porque el código llega al final):

| Variable | Declarado | Usado | Sobra | Nota |
|---|---|---|---|---|
| `sprite_*` (8 arreglos de objetos de sprite) | 8 x 128 | 8 x 64 | 512 | el SNES usa 32 lugares (`CreateSpriteAtPos` empieza en 62): seguro |
| `eproj_flags`, `eproj_enemy_header_ptr` | 2 x 72 | 2 x 36 | 72 | 18 proyectiles: seguro |
| `explored_map_tiles_saved` | 2048 | 1608 | 440 | el área 7 (256) nunca; confirmar en el código y las partidas |
| `g_word_7E9C46`, `scrolling_sky_*`, `mother_brain_indirect_hdma`, `power_bomb_explosion_*_hdma` | 1026 | 567 | ~460 | tablas HDMA: confirmar el largo en el código |
| `enemy_drawing_queue` | 228 | 104 | 124 | cola de dibujo: confirmar el máximo |

### SNES: RAM extra de enemigos (`enemy_ram7800`, 8 KB)

En juego (sin la carga de salas) se usan 7E:7800-8AB7 y 9000-953C y 9700-97FF. Sin usar en ninguna grabación:
7E:8AB8-8FFF (1352 bytes) y 7E:953D-96FF (451). El segundo tramo **sí** se usa: `g_byte_7E9540...9610` son los tiles
del Torizo muerto (`sm_a9.c`), que ninguna grabación llegó a ver. El primero no tiene ninguna dirección fija ni en el
port ni en el decomp: candidato a quitar (1352 bytes), con la misma prueba de los índices.

Esto muestra el límite de la medición: un caso que las grabaciones no tienen no aparece. Por eso cada recorte se
confirma con el código.

### Genesis: buffers por modo

- Solo en Zebes: `zcache` 896, `qsky_buf` 512, `qrow_buf` 432, `qcol_buf` 384, `zfade` 256 (2480 bytes).
- Solo en la intro (estados 30-31): `it_kc` 320, `it_cram_src`, `it_eyes`, `it_eyes_raw` 3 x 128 (704 bytes).
- Nunca tocados en las escenas medidas pero sí usados en otros casos: el haz de la bomba de energía (`cone_sat` 704,
  `cache_a` 352, `kRow` 324), la caja de mensajes (`mb_buf`, `mb_cells`, `mb_spr`: 592), el gancho (`gr_tiles` 160).
  Son de un momento: candidatos a compartir RAM con otros de otro momento.
- SGDK sin usar en el juego: los fundidos (`fadeR/G/B`, `fadeSR/SG/SB`, `fadeCurrentPal`: 896) y la caché de sprites
  (`vdpSpriteCache` 768): solo los usan el título y el menú.

## Lo que sigue (pasos 2 a 5), con lo medido

| Paso | Qué | Bytes | Riesgo |
|---|---|---|---|
| 2 | `sprite_*` a 32, `eproj_flags`/`enemy_header_ptr` a 18 (`ARRAY_LEN` de `gen_vars.py`) | 584 | bajo |
| 2b | tablas HDMA, `enemy_drawing_queue`, `explored_map_tiles_saved` tras mirar el código | ~1000 | medio |
| 3 | `enemy_ram7800` sin 7E:8AB8-8FFF (dos arreglos o una traducción de dirección en `ERAM`) | 1352 | medio |
| 4 | Intro con Zebes compartiendo RAM; buffers de momentos (haz, caja de mensajes) | ~1500 | medio |
| 5 | SGDK: el montón del arranque, la cola de DMA, los fundidos y la caché de sprites en RAM del juego | ~3000-5000 | alto (arranque) |

Validación de cada paso: la regresión, `host/check` en las salas (las variables se comparan con el SNES) y `pckeys.sh`
en la sala de Kraid.

## Paso 2 (hecho): arreglos recortados

`gen_vars.py` (`ARRAY_LEN`): los 8 `sprite_*` a 32, `eproj_flags` y `eproj_enemy_header_ptr` a 18 (y el borrado de
`LoadEnemies` a 18), `g_word_7E9C46` a 17, `scrolling_sky_bg2_hdma_data` a 46 y `explored_map_tiles_saved` a 7 áreas
(con una guarda para el área 7 en `gamestart.c`). Hay que volver a generar el estado inicial (`host/mkinit`, como en
`make_data.sh`): lleva el tamaño de cada variable. `_bend`: 0xE0FFE388 → 0xE0FFDF74 (**1044 bytes libres**).

Se quedan como están (mirado en el código):
- `enemy_drawing_queue` (228): son las 8 colas de capa de `kEnemyLayerToQueuePtr` ($A0:B133), de largos distintos y que
  se desbordan una en la siguiente como en el SNES; recortarla cambia lo que pasa con muchos enemigos en una capa.
- `power_bomb_explosion_left/right_hdma`: `PowerBombScaledCircle` puede escribir hasta el índice 255.
- `scrolling_sky_bg2_indirect_hdma` y `mother_brain_indirect_hdma`: su largo depende de la pantalla y del código de
  Mother Brain aún sin portar.

Validación: regresión con 0 distintos; Kraid en el PC (`pckeys.sh`) igual que antes (3 cuadros, primero 191); las
194 salas con `check` (lotes de `tools/rooms/lote.sh`): igual que con el código de antes (47 lotes sin diferencias; 2
con las de siempre: Samus sin energía, `HandleSamusOutOfHealthAndGameTile` sin portar).

Trampa encontrada: `ClearSpriteObjects` borraba 64 entradas de cada `sprite_*` (el original borra 511 palabras de
seguido) y, con los arreglos a 32, pisaba las variables de al lado (`size_of_bg2`, `sprite_object_index`): 11 lotes
fallaban en la carga de sala. Al recortar un arreglo, buscar los borrados con largo fijo del original.

## Paso 3 (hecho): RAM extra de enemigos sin 7E:8AC0-8FFF

La medición daba 7E:8AB8-8FFF sin usar, pero es la parte alta de los bloques de 64 bytes por enemigo de 7E:8800
(`ERAM(0x8800 + E_k)`, `gExtraEnemyRam8800`): se usa según el índice del enemigo, y ninguna grabación tenía enemigos
de esas especies con índice alto. Recorriendo las poblaciones de todas las salas (especie de cada índice y su
desplazamiento más alto en 7E:88xx): lo más alto es 7E:8ABF (Yapping Maw, índice 10, sala A293). Ridley, Draygon,
Botwoon, Spore Spawn, Baby Metroid y Mother Brain la usan con índice 0; los enemigos que se crean en juego (la
explosión de Ridley...) no la tocan. Se quita 7E:8AC0-8FFF (1344 bytes).

- `ERAM(a)` (`inc/core/enemies.h`, `eram_idx`): desde 7E:9000 baja 0x540 bytes; con direcciones fijas lo resuelve el
  compilador. En el PC, tocar el tramo avisa (`nt_eram_gap`, sale como "SIN PORTAR") y `check` cuenta como diferencia
  que el SNES lo cambie en juego.
- Los demás accesos pasan por `ERAM`: `nt_spawn_p`, `nt_ram_w`, `plm.c`, `eram_set_byte`; `host/check`, `mkinit`,
  `simsum` y `vardump` recorren las direcciones con lugar. El título usa el arreglo como búfer (4992 bytes): cabe.
- `_bend`: 0xE0FFDF74 → 0xE0FFDA34. Con el paso 2: **2388 bytes libres**.

Validación: las 194 salas con `check` igual que antes (47 lotes sin diferencias, 2 con Samus sin energía) y ningún
aviso del tramo; regresión 0 distintos; Kraid en el PC igual (3, primero 191); la pelea grande dura lo mismo (1833).

## Paso 4 (hecho): la intro y Zebes comparten RAM

Por modo (`docs/60_ram/md_uso.md`): los búferes de la intro (`it_kc`, `it_eyes`, `it_eyes_raw`, `it_cram_src`,
`it_slots`: 768 bytes) solo se tocan con la intro en pantalla (`intro_on`), y los grandes de Zebes (`zcache` y las
colas del vblank `qcol_buf`, `qrow_buf`, `qsky_buf`: 2224) solo en Zebes. Van en `md_ovl` (2240 bytes, `md.h`):
- Zebes (`ZebesOvl`, `md_zebes.c`): `zcache` primero (el ensamblador de `zebes_obj_draw` lo lee en `md_ovl`); al
  preparar cada sala se borra (`zo = 0` en cada entrada: la intro pudo dejar cualquier cosa). Las colas se escriben
  antes de leerse.
- Intro (`IntroOvl`, `md_intro.c`): `IntroGfx` la pone a cero al empezar (como al arrancar la consola).
- `_bend`: 0xE0FFDA34 → 0xE0FFD748 (**748 bytes**). Con los pasos 2 y 3: **3136 bytes libres**.

No se comparten (pueden coincidir con Zebes): el haz de la bomba de energía (`cone_sat`, `cache_a`, `kRow`) y la caja
de mensajes (`mb_*`). Los de SGDK del título (fundidos, `vdpSpriteCache`) van con el paso 5.

Validación: regresión 0 distintos (intro, Ceres y Zebes: los dos usos); la pelea con Kraid cuadro a cuadro contra el
paso 3: solo franjas de 1-6 líneas en los bordes de las interrupciones horizontales (degradado de Ceres, borde del
HUD), las de siempre al cambiar la velocidad del código.
