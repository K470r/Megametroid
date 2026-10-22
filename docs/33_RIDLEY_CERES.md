# 33 — La pelea con Ridley en Ceres (motor nativo y Genesis)

## Qué hay

`MegaMetroid_Ceres_nativo.bin` (y `_demo.bin`, con la entrada del bot) llega ahora hasta la sala de Ridley
(E0B5): la pelea entera y la huida de Ridley con la bebé. Ridley se va cuando la energía de Samus baja de 30,
como en el SNES.

Lo que sigue a la huida (texto "EMERGENCY", cuenta regresiva y escape de la estación hasta el ascensor) está en
docs/34.

## Motor (`native/src`)

- `ceres_ridley.c`: Ridley, su cola (`tilemap_stuff`), la bebé, los estados de la pelea de Ceres
  (`CeresRidley_Func_2`-`27`, `Ridley_Func_44`-`53`), las instrucciones (`Ridley_Instr_*`), el daño y la huida
  (`CeresRidley_Func_28`-`35`, `HandleCeresRidleyGetawayCutscene`).
- `nmath.c`/`nmath.h`: tablas de los bancos $A6 y $86 ya en palabras del 68000 (`A6W`, `B86W`), división 16/8
  con `divu.w`, ángulos, senos y cosenos, `Samus_DealDamage`.
- `eproj.c`: bolas de fuego de Ridley y sus explosiones, choque de proyectiles con bloques.
- `enemy_main.c`: enemigos de varias partes (choques por cajas), spritemaps extendidos, `SpawnEnemy`.
- `samus_move.c`/`samus_pose.c`: empujón y retroceso al recibir daño, Samus empujada fuera del camino de Ridley.
- `game.c`: temblor de la sala (`HandleRoomShaking`); la capa de la Genesis lo suma al scroll de los planos.

Verificación: `native/host/check` contra las grabaciones del SNES (bot y bot con ráfagas al azar, `HL_NOISE`
11-14): 0 diferencias hasta el comienzo del escape. En la Genesis (`native_compare.py`), las variables del motor
son iguales al SNES en los 5445 cuadros de la grabación hasta el escape.

## Dibujo en la Genesis (`native/md`, `mkmd.py`, `mkgetaway.py`)

| Qué | Cómo |
|---|---|
| Ridley | Sus piezas del SNES (16x16 y 8x8) se juntan en el PC en sprites grandes de la Genesis (403 → 193). La línea 1 de la CRAM es de Ridley (la paleta 7 del SNES, sin mezclar). El parpadeo al recibir daño cambia los colores de esa línea (en el SNES: los mismos tiles con otra paleta). |
| Cola | Cada segmento es un spritemap de $A6 ya convertido. |
| Tiles que cambian | Tres grupos de tiles que el SNES reescribe durante la pelea, con sus variantes del banco $B0, por DMA. |
| Puerta | Mientras Ridley la dibuja (cerrada), es parte del plano A: sus celdas cambian y no gasta sprites. |
| Bolas de fuego, explosiones, bebé | Registros de sprite por spritemap, como los demás objetos. |
| Huida | 113 cuadros preparados en el PC a partir de capturas del SNES (con y sin la capa de modo 7, con el color math): cada uno es un rectángulo del plano B con prioridad alta, una paleta de 15 colores y sus tiles sin repetir. Doble búfer: se cargan los tiles del cuadro siguiente mientras se ve el actual (los tiles de Ridley de la pelea quedan libres: 2 x 463). |

`snes_ref/getaway.npz` y `E0B5_pal.bin` salen de la ROM del usuario: no se distribuyen.

## Velocidad (8 de octubre)

Cuadros perdidos en la grabación del bot (Genesis Plus GX):

| Tramo | Antes | Ahora |
|---|---|---|
| Pelea (2017 cuadros) | 180 | 18 (0 desde docs/34, con `-O3`) |
| Huida (114) | 50 | 0 |
| Pasillos (1720) | 6 (entradas a sala) | 6 |

Cambios:

1. **Sprites a mano.** Buscar la clave de un spritemap y copiar sus registros a la lista de sprites costaba
   ~1800 ciclos por dibujo en C (30 líneas por cuadro en la pelea). Ahora es una rutina del 68000
   (`md_drawkey`) con todo en registros. La lista de sprites resultante es idéntica (suma por cuadro comparada).
2. **Espera del vblank propia.** En lugar de `SYS_doVBlankProcess`: espera que cambie `vtimer` y, si el cuadro
   terminó tarde pero todavía en el comienzo del vblank (hasta la línea 240), hace igual el trabajo del VDP en ese
   vblank en lugar de perder un cuadro entero. El mando se lee con `JOY_update` después.
3. **Menos trabajo en el vblank.** El HUD se convierte a celdas en `md_frame` y en el vblank solo se copia por
   DMA lo que cambió (de 19 a 8 líneas). La comparación del HUD es un `cmpm.l`.
4. **Huida.** Los cambios de celdas se hacen con una rutina de DMA por filas a mano (`md_dma_rows`), sobre el
   rectángulo unión del cuadro anterior y el nuevo (sin reponer aparte). La huida va primero en el vblank y
   calcula cuánto DMA le cabe con la línea actual; si no cabe, espera al vblank siguiente, y los tiles de Samus
   esperan si la huida se llevó el vblank. Ya no hay trabajo del vblank que se pase a la imagen.
5. Menores: las colas de dibujo de enemigos vacías no se llaman; sin el recorrido de objetos de sprite (no hay
   en lo portado); las mitades de Samus recuerdan su última búsqueda.

Mediciones: `LAGLOG=1 sh build.sh` registra los cuadros de la consola de cada cuadro del motor
(`md_lag_log`) y los vblanks que se pasaron a la imagen (`md_vb_over`). `STOP_AT=N` congela en el cuadro N.

## Correcciones

- Al terminar la huida el último rectángulo quedaba en el plano B (tiles ya reemplazados: basura de colores a los
  costados). Ahora, si el motor deja de pedir cuadros, la Genesis pasa al último (vacío), que repone el plano B, y
  la línea 1 vuelve a los colores de la sala.
- Los cambios de celdas grandes de la huida se partían en dos vblanks y por un cuadro Ridley se veía con la
  paleta del cuadro anterior (verde). Ya no se parten.
- Samus desaparecía un cuadro al tocar el suelo: al aterrizar ($90:8CC3) el SNES lee una palabra de una tabla de
  bytes y la y de Samus queda con basura en el byte alto; en el SNES no importa (la y de un sprite es de 8 bits).
  La Genesis ahora toma solo el byte bajo cuando la y es absurda. (Venía de antes de la pelea.)
- Ridley antes de la pelea se veía como una silueta negra. En el SNES el color math del haze se suma también a los
  sprites de las paletas 4-7: negro + haze = el fondo de cada banda. Ahora la línea de Ridley lleva sumado el nivel
  de la banda donde está Ridley, y mientras su paleta es toda negra sus sprites no se dibujan.
- Rosado en los costados durante el temblor después de la huida: las columnas que tapan los bordes (sprites con
  la paleta 7, como Ridley) tenían sus tiles en la zona que usa la huida. Ahora solo las claves de Ridley van ahí.
- Explosión del rayo al chocar con una pared u obstáculo: faltaba `DrawBombAndProjectileExplosions` ($93:834D).
  Contra Ridley el rayo desaparece sin explosión, como en el SNES de Ceres (en otros enemigos sale una chispa: son
  objetos de sprite, todavía sin portar).
- Búsqueda de sprites con memoria de 16 claves (la cola de Ridley repite las mismas): VRAM idéntica con y sin ella.
