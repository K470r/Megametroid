# 58 — Enemigos congelados, luz de la bomba de poder y estelas de la onda

Fecha: 2026-10-29. Rama `hielo-pb-estelas` (desde `main`, b12aadf). Los tres puntos de "Lo que falta" de docs/57, en
ese orden. Decisiones de Edwin:
- congelados: un juego común que se sube a la VRAM al congelarse (no la propuesta por sala de docs/57: no entraba);
- bomba de poder: mixto, tinte de los colores cuando el óvalo cubre toda el área de juego y sprites de luz del modo
  sombra/luz mientras crece.

| Punto | Estado |
|---|---|
| 1. Enemigos congelados | hecho |
| 2. Luz de la explosión de la bomba de poder | hecho (con más cuadros perdidos que `main` en la explosión: abajo) |
| 2b. Rayo de luz de los trajes | no se hizo (abajo: cómo se haría con lo mismo) |
| 3. Estelas del rayo de onda | hecho |

## 1. Enemigos congelados

### Lo que hace el SNES (decomp)

- `WriteEnemyOams` ($A0:944A): con `frozen_timer` distinto de 0, el enemigo se dibuja con `palette_index = 0x0C00`
  (la paleta 6, la del rayo) en vez de la suya: mientras `frozen_timer >= 0x5A`, siempre; después, un cuadro sí y
  otro no (`frozen_timer & 2`). Mismos tiles, colores de la paleta del rayo equipado.
- Con el hielo equipado la paleta 6 es la de hielo (`kBeamPalettePtrs[2]`, $90:C3C9; la misma en las 6 combinaciones
  con hielo): 7FFF 7EC0 6DE0 54E0 7F6E 730A 6665 5E22 7FA0 7B60 7720 72A0 6E60 7F91 20C0.
- Se congela (`EnemyShot`, $A0:A6DE…A79A) si la vulnerabilidad al rayo con hielo (`EnemyDef.vulnerability_ptr`,
  +0x3C; sin ella `$B4:EC1C`) es 0xFF, o si el disparo lo mata y `(power & 0xF0) != 0x80`.

### Por qué no la propuesta de docs/57

Por sala (una clave 0x0C00 por cada spritemap con sus tiles en los de la sala): +~140 KB (no entraba en las páginas
11-21) y en 9CB3, 9D19 y 9E9F los tiles no entraban en la VRAM. Además en la línea 3 no queda ningún color libre: los
enemigos de cada estado usan de 13 a 57 colores distintos para 10 lugares (`MM_ZOBJ_STATS=1` en `mkzobj.py`).

### Lo que se hizo

PC (`tools/conv/mkzobj.py`, `mkzebes.py`):
- `frozen_banks`: los bancos de la población de cada estado que el hielo puede congelar (la regla de arriba). Los
  jefes (`BOSS_PALETTES`) no.
- `frozen_entry`: para cada clave de enemigo `(banco, sm, paleta, tiles)` de esos bancos, el congelado: las piezas
  del spritemap con la paleta 0x0C00 (`key_pieces`), pieza por pieza (sin `compose`), los tiles del SNES de la sala
  (`state_vram`) con los colores 1-5 del hielo en los lugares fijos 9-13 del rayo (siguen a `palette_buffer[225-229]`
  como en el SNES) y los 6-15 en el más parecido de los 1-5 (`ice_slots`). Cada congelado distinto se guarda una vez
  (por contenido): **653** en Crateria y Brinstar.
- La clave de la sala `(sm, 0x0C00, tiles)` guarda `0x8000 | número` en lugar del desplazamiento de sus registros.
- `mkzebes.py`: los tiles de los congelados van al depósito común (`kZebesPool`, sin repetir) y cada congelado guarda
  sus tramos: `kZebesFrz` (primer tramo, tramos, tiles, registros), `kZebesFrzRuns`, `kZebesFrzRecs` (los registros
  con el tile desde 0), todo en una página (`ZEBES_FRZ_PAGE`).

Genesis (`src/platform/md_zebes.c`, `md_engine.c`):
- `zebes_obj_draw`: si la clave es de un congelado, `frz_draw`: busca o toma un lugar en la VRAM (`frz_get`), dibuja
  sus registros y les suma el primer tile.
- Lugar: las filas 5-31 del plano Window (tiles 1802-1855, no se ven: la ventana solo muestra el HUD), en 3 lugares
  de 18 tiles (el 90 % de los congelados tiene 16 o menos; los más grandes, hasta 54, toman lugares seguidos). Un
  lugar se suelta si su congelado no se dibujó en 8 cuadros (el parpadeo del final). Los tiles se suben en el vblank
  (`frz_vblank`, con los tramos del depósito como al cargar la sala).
- Sin lugar, con la caja de mensaje abierta (usa esas filas; al cerrarse se vuelven a subir: `zebes_frz_reset`) o en
  las salas del ojo (las usa el haz), el congelado se dibuja con su paleta (`DrawEnemySpritemap`): no queda invisible
  ni cuenta como clave faltante.

### Comprobación

- `md_missing_ring` vacío después de disparar con el hielo en 9D19, 92FD, 93FE, 965B, 9E52, 9F11 y 9E9F
  (`lado_a_lado.sh`, `in_chargebeam.txt`, `HL_POKE=9A6:1002,9A8:1002`, `MD_TEST_BEAMS=0x1002`).
- Lado a lado con el SNES en 9D19 (mosaico para Edwin): los colores del congelado son los del SNES.
- En Crateria la entrada desde la puerta no llega a congelar ningún enemigo (92FD, 93FE, 965B): solo se comprobó que
  no falten claves. Falta probar a mano varias salas con enemigos congelados a la vez (más de 3 congelados distintos:
  los que no entran se ven con su paleta).

## 2. Luz de la explosión de la bomba de poder

### Lo que hace el SNES (decomp, banco $88)

- `SpawnPowerBombExplosion` ($88:8AA4): dos objetos de HDMA; la ventana 2 por línea desde las tablas indirectas
  `$89:9800` / `$89:A101` (una línea por fila) a `power_bomb_explosion_left_hdma` / `_right_hdma` (192 filas por
  distancia al centro), y la matemática de color (`LayerBlendPowerBombFunc_0` $88:8219: `CGADSUB = 0x37`, dentro de
  la ventana se suma `COLDATA` a BG1, BG2, sprites y fondo; afuera nada).
- La secuencia de formas es siempre la misma (`HdmaobjPreInstr_PowerBombExplode_*`, lista $88:8ACE): destello blanco
  (escalado por el radio, 3 cuadros), destello amarillo (4 formas ya escaladas), explosión amarilla (escalada, de 4 a
  134 de media anchura en ~38 cuadros), explosión blanca (17 formas ya escaladas, hasta 508 x 382) y resplandor (la
  última forma; el color baja de a 1 cada 3 cuadros). Las ya escaladas cortan en el primer 0 y dejan lo del cuadro
  anterior. Con la bomba cerca de Samus, ~100 de los ~160 cuadros cubren toda la pantalla.

### Lo que se hizo

PC (`tools/conv/mkpbomb.py`, en `make_data.sh`): simula la secuencia del SNES (las mismas cuentas) y, para cada una de
las **58 formas** (clave: etapa y radio o número de la forma ya escalada), un cuarto del óvalo (abajo a la derecha)
cubierto con cajas de hasta 4 x 4 tiles por bandas de 32 líneas: columnas llenas en toda la banda con 16 tiles llenos
fijos; las del borde, con sus tiles (color 14 de la línea 3, "aclarar"), seguidos por caja. Tablas en la parte común
(`kPbKey`, `kPbShape`, `kPbBox`); los 2511 tiles (80 KB) en la última página de los niveles (la 10, `PB_PAGE`).

Motor (`src/core/hdmaobj.c`): `nt_pb_shape` anota la forma que armó cada etapa (no es RAM del SNES; `host/check` no
la ve).

Genesis (`src/platform/md_cone.c`, junto al haz del ojo):
- `pb_decide`, en cada cuadro de la explosión: si la primera y la última línea del área de juego están enteras dentro
  (las formas se angostan hacia arriba y hacia abajo), **tinte**; si no, **luz**.
- Tinte (`md_cone_pb_tint` → `md_engine.c`): el color fijo del SNES sumado a todos los colores de la sala, Samus y los
  objetos, como la niebla (docs/46). El color y el resplandor son los del SNES. Para que no cueste tanto, tres tablas
  por canal (rojo, verde, azul → CRAM: `kCram555` es canal por canal, comprobado en los 32768 colores) y el color se
  toma uno de cada 3 cuadros (`PB_TINT_EVERY`; el resplandor del SNES baja cada 3).
- Luz (`pb_build`, `pb_emit` en ensamblador): los sprites de los cuatro cuartos con los giros alrededor del centro
  (línea de la Genesis = línea del SNES + 2), en `cone_sat` (al final de la tabla del VDP, como el haz del ojo).
  Solo se suben los tiles de las cajas que se ven (una DMA por caja, hasta 53 tiles por vblank, desde la página 10, a
  0xF000-0xFF3F); si la forma y lo que se ve no cambiaron, nada. Si no entran los sprites (44 como mucho) o los tiles,
  el tinte.
- Modo sombra/luz: el VDP oscurece lo que no tiene prioridad alta en ningún plano. Desde que se pone la bomba
  (`power_bomb_flag`, `HudSelectionHandler_MorphBall` $90:BF9D) hasta que termina la explosión, las celdas del plano A
  van con prioridad alta (`zebes_force_prio`, `tile_fix`); el plano B no hace falta. La ventana dibujada se reescribe
  de a 2 columnas por cuadro (`prio_sweep`) mientras la bomba espera, cuando sobra tiempo: de una vez costaba ~10
  cuadros perdidos. Los sprites van con prioridad alta y los dos que tapan el HUD (`eye_room_sprites`) mientras las
  celdas la tengan. La luz se enciende cuando las celdas están listas.

### Comprobación

- Lado a lado con el SNES en 9D19 (mosaico para Edwin): `lado_a_lado.sh` con `XY_N=60 X=600 Y=699` (Samus en el piso
  dentro de la vista) y `HL_POKE=9C6:A,9C8:A,9CA:A,9CC:A,9CE:A,9D0:A,9D2:3` (lo mismo que pone `MD_TEST_AMMO`). La
  posición de la bomba y de la cámara de las dos se comprobó en la RAM (la grabación de `hlref` y un volcado de la
  Genesis): (600, 921) en las dos, cámara a 0-3 píxeles. El óvalo sale en el mismo lugar y con el mismo tamaño en cada
  cuadro; el tinte sigue el color y la bajada del SNES.
- `tools/rooms/powerbomb.sh`: 0 diferencias (la lógica no cambió).
- Cuadros perdidos en la primera explosión de esa prueba (160 cuadros del motor, `md_shown_frame` repetidos):

| | perdidos |
|---|---|
| `main` (sin luz) | 44 |
| esta rama | 59 |
| esta rama sin la luz (`MD_PB_NOLIGHT`) | ~52 |
| esta rama sin luz ni tinte (`MD_PB_NOLIGHT MD_PB_NOTINT`) | ~45 |

  En `main` la explosión ya pierde ~1 de cada 3 cuadros: la lógica de la bomba (`BlockBombedReact`, ~150 líneas por
  cuadro en la etapa blanca; `ProcessEnemyPowerBombInteraction`, `BombOrPowerBomb_Func1`) llena el cuadro. La luz y el
  tinte suman ~15 más. Lo que se probó: el redibujo de la prioridad de a columnas y antes de la explosión (−20), el
  armado en ensamblador, el tinte con tablas y a 20 Hz (−5). Lo que queda es repartido (no hay un punto caliente) y
  con la explosión ya pasada de tiempo cualquier costo chico se vuelve un cuadro entero.

### Diferencias con el SNES

- Mientras crece, la luz aclara hacia el blanco (sombra/luz), no suma amarillo ni rosa; no aclara a Samus ni a los
  enemigos que quedan debajo (como el haz del ojo, docs/52).
- Cuando el óvalo cubre casi todo pero no las esquinas (o no entra en 44 sprites), el tinte ilumina también las
  esquinas.
- Mientras la bomba espera y explota, los sprites quedan delante de todas las celdas del plano A (prioridad alta).
- En los cuadros con luz, los colores 14 y 15 de la línea 3, que usan los enemigos fuera de las salas del ojo, son
  "aclarar" y "oscurecer".
- El tinte se actualiza cada 3 cuadros: en el destello (que cambia de color muy rápido) va un poco atrasado.

### El rayo de luz de los trajes (no hecho)

`InitializeSuitPickupHdma` ($91:D692) es el mismo mecanismo: una ventana por línea (`hdma_table_1`) y un color que se
suma (`CGADSUB = 0x33`, `suit_pickup_color_math_*`). Con lo de arriba: las formas del rayo por cuadro preparadas en el
PC (`mkpbomb.py`) o, como es un rayo vertical angosto, armadas en la Genesis con `cone_bands` / `cone_build` del haz del
ojo; la prioridad y los sprites, igual que con la bomba (`zebes_force_prio` mientras dura la animación).

## 3. Estelas del rayo de onda

- `SpawnProjectileTrail` ($90:B657) toma las listas de `$90:B5BB` / `$90:B609` por `projectile_type & 0x3F`. Los
  tiles 0x3C-0x3F con la paleta 5 son solo los de la onda (entradas 1 y 17); los demás rayos usan 0x38-0x3B (paleta 6)
  y los misiles 0x48-0x4B (paleta 5).
- Sus colores 5-7 (7C1D, 5814, 300A) son casi los 2-4 de la paleta de la onda (7C1F, 5816, 300C), la del rayo equipado
  cuando salen. `mkzobj.py` (`wave_trail_slots`) los pone en los lugares fijos 10-12 del rayo (los 9-13 de la línea 3,
  de `palette_buffer[225-229]`), como los tiles del rayo (docs/57). Antes salían con los colores de la sala (en 9D19,
  morado oscuro).
- Comprobado lado a lado en 9D19 (Genesis de `main`, de esta rama y SNES): magenta como en el SNES.

## Límites

| | `main` | Ahora | Límite |
|---|---|---|---|
| Fin de la parte común (`__mm_title_start`) | 0x33EAB4 | 0x341DB4 (+13 KB) | 0x380000 |
| RAM `_bend` | 0xE0FFE210 | 0xE0FFE300 | 0xE0FFE3F0 |
| Páginas de Zebes | 11-21 | 11-21 (2-18 KB libres por página) | 23 |
| Página 10 (niveles) | 168 KB libres | 86 KB libres (los tiles de la luz) | |
| ROM | 11 264 KB | 11 264 KB | |

`mkzebes.py` deja ahora 2 KB de margen al final de cada página (antes 4): medido en `rom.map`, lo real queda hasta ~1 KB
por encima de su cuenta; el enlace falla si una página pasa de 512 KB (`md_ssf2.ld`). `MM_PAGE_MARGIN` lo cambia.

## Comprobación contra el SNES

`tools/rooms/beams.sh` (11 combinaciones), `sba.sh` (4), `powerbomb.sh`, `chargebeam.sh` y la grabación Ceres → Zebes
(`host/check ref_full.bin … 97 1822 0`): **0 diferencias** en todas.

## Regresión

`sh tests/regress/regress.sh`: intro 0 distintos, Ceres 0 distintos, Zebes 0 distintos. No se aprobaron referencias
nuevas.

## Notas de las pruebas (`lado_a_lado.sh`)

- Sin `SETXY`, Samus entra a 9D19 por la puerta 8DAE en lugares distintos en el SNES y en la Genesis (también en
  `main`): (471, 699) contra (551, 790). Con `SETXY` en el aire, la bomba se pone mientras cae y cae distinto. Para
  comparar la bomba: Samus en el piso dentro de la vista (`XY_N=60 X=600 Y=699`).
- `MD_TEST_AMMO` pone 10 misiles, supermisiles y bombas de poder: en el SNES hace falta el mismo `HL_POKE` completo.
- En 9F11 los enemigos (D87F) salen celestes en la Genesis y rojos en el SNES, también en `main`: no es de esta rama.

## Lo que falta

- Rayo de luz de los trajes (arriba).
- La explosión de la bomba de poder pierde cuadros (ya en `main`): su lógica (`BlockBombedReact` y demás) necesita
  una pasada de velocidad; después, volver a medir lo que suma la luz.
- Probar a mano congelados en más salas (varios a la vez) y la bomba en las salas del ojo.
