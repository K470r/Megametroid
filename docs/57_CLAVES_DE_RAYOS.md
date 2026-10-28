# 57 — Rayos en la Genesis: juego de claves común y tiles del rayo equipado

Fecha: 2026-10-28. Rama `beam-keys` (desde `main`). Propuesta de docs/56 ("Lo que falta en la Genesis"), aprobada
por Edwin.

## Problema

La lógica de los rayos de onda, hielo, spazer y plasma, sus combinaciones y los 4 ataques especiales (SBA) ya daba 0
diferencias contra el SNES (docs/56), pero en la Genesis no se veían: sus spritemaps no estaban en las claves de
dibujo. Ponerlos en las claves de cada sala de Zebes (`mkmd.beam_sm_tiles`, cada uno con los tiles de su combinación)
no entraba en las páginas del SSF2.

## Lo que hace el SNES (decomp)

- `UpdateBeamTilesAndPalette` ($90:AC8D): encola una copia de 256 bytes (8 tiles) desde `kBeamTilePtrs[equipped_beams
  & 0xFFF]` ($90:C3B1, banco $9A) a la VRAM $6300 (los tiles 0x30-0x37 de los sprites) y escribe la paleta 7
  (`palette_buffer[224-239]`, `kBeamPalettePtrs` $90:C3C9; `WriteBeamPalette_Y`). Se llama al cargar cada sala
  (`DoorTransitionFunction_LoadMoreThings_Async`), al empezar la partida, al tomar un rayo (PLM), al salir de la pausa
  (`UpdateBeamTilesAndPalette_Unpause` $82:A377) y al cambiar de rayo en la pantalla de equipo.
- Los proyectiles toman sus listas de instrucciones ($93) **solo** de tablas: `kProjectileData_UnchargedBeams`
  ($93:83C1), `_ChargedBeams` ($93:83D9), `_NonBeams` ($93:83F1), `kShinesparkEchoSpazer_ProjectileData` ($93:8403),
  `g_off_938413` (SBA), `kRunInstrForSuperMissile` ($93:842B) y las explosiones de `KillProjectileInner` /
  `InitializeBombExplosion` ($93:867B rayo, 867F misil, 8683 bomba, 8693 supermisil). `RunProjectileInstructions`
  ($93:81E9) deja el spritemap en `projectile_spritemap_pointers`.
- `DrawProjectileSpritemap` ($81:8A4B) → `DrawGrappleOrProjectileSpritemap` ($81:8A5F): copia las piezas del
  spritemap tal cual (x, y, tile y atributo) a la OAM.

Recorriendo todas esas listas (`mkmd.samus_proj_lists`): **416 spritemaps** con piezas (y uno vacío, `$A117`, un cuadro
sin dibujo de la onda), y de ellos **370 están hechos solo con los 8 tiles del rayo** (tiles 0x30-0x37, paleta 6, todas las piezas de 8x8): los rayos de las 12 combinaciones,
cargados o no, los SBA y los ecos. Los otros 46 (misiles, supermisiles, bombas, bombas de poder, explosiones, las
partes de los ecos y del SBA con tiles comunes) usan los sprites comunes ($9A:D200 → VRAM $6000). Ningún spritemap
mezcla tiles del rayo con tiles comunes.

## Lo que se hizo

### 1. Juego de claves común (`mkmd.beam_common`, en la parte fija)

- `kBeamKeySm[371]` (ordenados; con el vacío `$A117`, para que no cuente como clave faltante), `kBeamKeyRec[371]` y
  `kBeamRecs` (12 165 palabras, 24 KB; los registros iguales se
  comparten: muchos spritemaps se repiten entre combinaciones). Cada pieza es un sprite de 8x8 en `BEAM_TILE + (tile
  - 0x30)`, línea 3, con su volteo y su prioridad (`(atributo >> 12) & 3 == 3` → prioridad alta, como `mkzobj`).
- `kBeamTiles`: los 8 tiles de cada una de las 12 combinaciones ya en el formato de la Genesis (3 KB). El color c
  (1-5) de la paleta del rayo va al lugar fijo 8 + c de la línea 3 (los colores 9-13, que la Genesis ya copiaba de
  `palette_buffer[225-229]` en cada cuadro: `zebes_obj_colors`). Los pocos píxeles de otros colores (el 6 en las
  combinaciones con onda e hielo, el 15 en las de plasma) van al más parecido de los 1-5 de la paleta de esa
  combinación (se elige en el PC).
- `DrawGrappleOrProjectileSpritemap` (`md_engine.c`): en Zebes busca primero en el juego común (búsqueda binaria,
  sin cambiar de página) y lo dibuja con `md_put_recs`; si no está, las claves de la sala (`zebes_obj_draw`) como
  antes. En Ceres y en la intro no cambia nada.

### 2. Lugar fijo de la VRAM: `BEAM_TILE` = 1872 (0xEA00-0xEAFF)

Entre la tabla de sprites (64 sprites en H32: 0xE800-0xE9FF) y el scroll horizontal (0xEC00) hay 512 bytes libres;
el rayo usa la mitad. No se cruza con 0xF000-0xFFFF (haz del ojo, caja de mensaje, gancho `GRAPPLE_TILE`), así que el
rayo se ve también en las salas del ojo y con el gancho. Mapa en `inc/platform/md.h`.

- Motor (`door.c`): `UpdateBeamTilesAndPalette` llama a `BeamTiles(equipped_beams & 0xFFF)` (capa de la Genesis; en
  `host/` no hace nada), como el SNES encola su copia. Como se llama al cargar cada sala, al tomar un rayo y al salir
  de la pausa, los tiles se vuelven a subir aunque algo haya usado la VRAM entretanto.
- Genesis: `BeamTiles` anota el origen (`kBeamTiles + combinación * 256`); `pipe_take` lo pasa al vblank y el vblank
  lo copia con una DMA de 256 bytes (como el cañón del brazo y el gancho).

### 3. Lo que ya se veía

- Rayo normal (sin cargar y cargado), destello de la carga, misiles, supermisiles, bombas y gancho: el rayo normal
  y el cargado pasan al juego común (son spritemaps de tiles 0x30-0x37); el resto queda como estaba. El rayo cargado
  normal ya no va en las claves de cada sala (`zebes_proj_keys` sin `$93:83D9`) y `mkzobj` ya no arma una VRAM por
  combinación (`vram_for`).
- La bomba de poder (el sprite de la bomba, `$93:8671`: spritemaps AB97, AB9E, ABA5) no estaba en ninguna clave: se
  agrega a `zebes_proj_keys` (tiles comunes, colores de la sala, como la bomba).

## Comprobación

### Contra el SNES (`host/check`)

`tools/rooms/beams.sh` (las 11 combinaciones con el cargado), `sba.sh` (onda, hielo, spazer, plasma), `powerbomb.sh`
y `chargebeam.sh`: **0 diferencias** en todas (la lógica no cambió; `BeamTiles` en el PC no hace nada).

### En la Genesis, lado a lado con el SNES

`tools/compare/lado_a_lado.sh` en la sala 9D19 (puerta 8DAE, `XY_N=250 X=784 Y=740`, como el gancho en docs/56),
patrón propio: disparos sin cargar a la derecha, arriba, izquierda y derecha, y dos cargados (al lado y arriba).
`EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_ITEMS=0x1004 -DMD_TEST_BEAMS=0x10xx"` y `HL_ITEMS=1004 HL_POKE=9A6:10xx,9A8:10xx`.
Nuevas en `md_main.c` (solo en las ROM de prueba): `MD_TEST_GOD` (como `HL_GOD`: la energía al máximo en el estado 8;
sin eso Samus moría en la Genesis y las dos máquinas se separaban) y `MD_TEST_AMMO=n` (misiles, supermisiles y bombas
de poder, como `HL_POKE=9C6:n,...,9D0:n`).

- Las capturas de cada cuadro del motor quedan corridas un cuadro (docs/56): se compara la Genesis con la captura del
  SNES `T0 + m` (no `T0 + 1 + m`). Así coinciden la posición, la animación y el parpadeo de los rayos (el SNES dibuja
  los rayos sin cargar un cuadro sí y otro no según el índice del proyectil: `DrawPlayerExplosions2`).
- Las 12 combinaciones (con la normal), los 4 SBA, la bomba de poder, misiles, supermisiles y bombas: mosaicos para
  Edwin (resultados abajo).
- Los primeros cuadros después de poner a Samus (cae desde y=740) la cámara de las dos se acomoda distinto; desde que
  toca el suelo coinciden.

Resultados:

| Qué | Resultado |
|---|---|
| Las 12 combinaciones (normal, onda, hielo, spazer, plasma y las 7 mezclas), sin cargar y cargadas, al lado y arriba | la forma, el color, la animación y el parpadeo son los del SNES (mosaicos) |
| Los 4 SBA (onda, hielo, spazer, plasma) | las mismas figuras que el SNES; la cámara de las dos queda corrida en horizontal en esa prueba (la colocación con `SETXY` mientras Samus camina hacia la pared: no es el dibujo) |
| Rayo normal y cargado (que pasaron al juego común): Genesis nueva contra la de `main`, misma entrada | **0 píxeles distintos** en 1001 cuadros del motor |
| Misiles, supermisiles, bombas y bomba de poder: Genesis nueva contra la de `main` | iguales píxel por píxel salvo 59 cuadros: los de la bomba de poder, que ahora se ve (en `main` faltaban sus claves AB97/AB9E/ABA5) |
| Claves que faltaron (`md_missing_ring`, al final de cada corrida) | ninguna de proyectiles; solo las de los enemigos congelados (abajo) |

### Límites

| | Antes | Ahora | Límite |
|---|---|---|---|
| Fin de la parte común (`__mm_title_start`) | 0x3378B4 | 0x33EAB4 (+29 KB) | 0x380000 |
| RAM `_bend` | 0xE0FFE208 | 0xE0FFE210 | 0xE0FFE3F0 |
| Páginas de Zebes (`mkzebes.py`) | 11-22, 435 KB libres | 11-21, 130 KB libres | sin aviso |
| ROM | 11 392 KB (0xB20000) | 11 264 KB (0xB00000) | |

Las claves de cada sala pesan menos sin el rayo cargado (que en cada estado de sala llevaba sus tiles y registros):
una página menos.

### Regresión

`sh tests/regress/regress.sh`: intro 0 distintos, Ceres 0 distintos, Zebes 0 distintos (la de Zebes dispara el rayo
normal después de aterrizar: con el juego común la imagen es la misma que la referencia). No se aprobaron referencias
nuevas.

## Lo que falta

- **La explosión de la bomba de poder** (el círculo que crece): en el SNES es una ventana por HDMA con matemática de
  color (`Hdmaobj_PowerBomb*`, banco $88); la lógica está (docs/56) pero la Genesis no la dibuja. Es el mismo trabajo
  que el rayo de luz de los trajes: una forma por línea con el modo sombra/luz o con sprites, como el haz del ojo
  (docs/52).

- **Enemigos congelados**: el SNES dibuja al enemigo congelado con la paleta 6 (la del rayo de hielo: `pal = 0x0C00`).
  Esas claves no están en las salas (`md_missing_ring`: `(1, AF19, 0C00, 20)` en 9D19 con el hielo): el congelado no se
  ve. Propuesta: en `mkzobj`, por cada spritemap de enemigo una clave con la paleta 0x0C00 que use los mismos tiles con
  los colores del rayo (los 9-13 fijos y los demás de la paleta de hielo en lugares que no usen los enemigos de la
  sala); mide primero cuánto suma en las páginas.
- **Estelas de los rayos** (`projectiletrail`, tiles 0x3C-0x3F con la paleta 5): se dibujan con los colores de la sala
  (pesan poco al juntar colores, docs/46) y el magenta del SNES queda como el color más parecido de la sala (en 9D19,
  morado oscuro). Las de los tiles 0x38-0x3B (paleta 6) ya usan los colores fijos del rayo.
- **El rayo de luz de los trajes** (`hdma_table_1` y la matemática de color, docs/56): no se hizo en esta rama.
