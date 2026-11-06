# 59 — Jefes: dónde va su código y cómo se prueban

Rama `claude/trusting-galileo-ct4zjp` (2026-11-05, al día el 2026-11-06). Jefes: Phantoon, Draygon, Botwoon, Crocomire, Ridley de
Norfair, Golden Torizo, Mother Brain y la bebé metroide (Kraid no: queda para después). También los cadáveres de
Tourian (DC65, DCB1).

## Dónde va el código (la decisión)

La parte común de la ROM (debajo de 0x300000) no alcanzaba para la IA traducida de los jefes (~980 funciones más los
bancos $91, $A4, $A9 y $AD). La ventana 7 del SSF2 (0x380000) no se puede cambiar para llamar a una función: el
código común lee el nivel de la sala por esa ventana (`level_get`, `bts_get` en `room.h`, las colisiones de todos los
enemigos) dando por hecho que muestra la página del nivel.

Por eso: **el código de cada jefe va en la misma página que los niveles de las salas de los jefes** (la 25). En esas
salas la ventana 7 muestra siempre esa página (es la del nivel), así que la IA, las instrucciones y los proyectiles del
jefe corren sin cambiar de página. Todo lo que usa la ventana 7 para otra cosa (dibujo, tiles, el vblank) ya guardaba
y reponía la página (`md_cur_page`, `vb_enter`/`vb_exit`).

- `tools/port/port.py`: lo que solo alcanzan las raíces de los jefes (lo que está después de `# jefes` en
  `roots.txt`), menos lo que alcanzan las demás raíces y lo que nombra el código escrito a mano (sin contar
  `bosses.c` ni los comentarios), lleva `NT_BOSS` (`__attribute__((section(".bank25")))`). `BOSS_PAGE = 25`.
- `tools/conv/mkdata.py`: `BOSS_ROOMS` (A98D, B283, B32E, CD13, D95E, DA60, DC65, DCB1, DD58): sus niveles van en la
  página 25; `BOSS_BANKS` ($91, $A4, $A9, $AD): las copias de bancos que solo leen los jefes, también.
- `src/core/bosses.c`: lo escrito a mano de los jefes, también con `NT_BOSS`.
- Seguridad (solo Genesis): `port_Find*` no llama a una función de la página de los jefes si la ventana 7 está en otra
  página (`NT_BOSS_AWAY`, avisa con `nt_unimpl`).
- Páginas: Zebes usa de la 11 a la 25; en la 25, lo que dejan los jefes (`BOSS_RESERVE` en `mkzebes.py`: 320 KB, hoy
  usan ~240 KB; el enlazador avisa si la página pasa de 512 KB); el sonido la 6 y de la 26 a la 31. Los trozos del
  depósito de tiles de Zebes son de 16 KB (`POOL_CHUNK` 512). La ROM
  sigue en 15,25 MB (sin pasar de 16 MB: no hace falta un mapeador extendido).

Con esto la parte común tiene ~47 KB libres y la RAM ~116 bytes (`crocomire_bg2_scroll_hdma_data`, 560 bytes, comparte
lugar con `cinematic_text_tilemap`: `vars_extra.txt`; las cachés de los despachos traducidos tienen 8 entradas).

## Lo que hizo falta en el traductor (xlate.py) y a mano

Ojo con los tipos de las tablas de funciones: en el 68000 un puntero vuelve en a0 y un entero en d0. Las funciones de
instrucción traducidas devuelven `uint16`, así que los `typedef` del decomp que devuelven `const uint16 *`
(`CrocomireFunc`) se cambian a `uint16` (`port.py`); si no, la Genesis lee basura (en el PC no se nota). Fue lo que
rompía a Crocomire en la Genesis (su lista de instrucciones saltaba a 0x0484).

Traductor: cola de escrituras a la VRAM del SNES (`VramWriteEntry` → bytes en `nt_vwq`, la NMI la pasa a la
plataforma: `nt_vram_queue_flush`), RAM extra por elemento (`enemy_ram7800[n].kraid`, `gRam7800_Default`,
`extra_enemy_ram8000`), `SpawnHdmaObject_Args`, ganchos de pausa (`nt_pause_hook`/`nt_unpause_hook`, `pause.c`),
corrutinas sin espera, funciones de una línea (`cgraph.py`), `Get_X` de otro tipo, RAM del SNES por dirección
(`nt_ram_w`, `nt_ram_set_w`), punteros a la ROM (con y sin const, asignados después, con paréntesis anidados; los de
palabras avanzan de a 2), palabras entre dos campos de `EnemyData`, las funciones `static` escritas a mano ya no
cuentan como nativas (Ridley de Norfair sale entero del decomp; Ceres sigue con su versión a mano).

A mano: `bosses.c` (listas de instrucciones de Mother Brain y de la bebé, Crocomire derritiéndose sin el borrado de
píxeles del SNES, tiles de Mother Brain por la cola, la larva en la cara de Mother Brain...), `CeresRidley_Init` en
Norfair, `DisableMinimapAndMarkBossRoomAsExplored` (tabla `kBank90_BossMap`), `ProcessExtendedTilemap` (los cuerpos en
BG2 a `tilemap_stuff`), los objetos de HDMA de Phantoon y del agua de Draygon (`hdmaobj.c`).

## Los cuerpos en el BG2 (Phantoon, Draygon y Crocomire)

El SNES sube `tilemap_stuff` (hasta 1024 bytes: 16 filas de 32 celdas) al BG2 en $4800 (`QueueEnemyBG2TilemapTransfers`
$A0:9726). En la Genesis:

- `host/pc_stubs.c` (con `MM_DRAWLOG`) registra las palabras distintas de cada subida y el color más brillante de cada
  índice del BG; `res/snes_ref/zbg2.log` tiene lo de Crocomire, Phantoon y Draygon (lote.sh de 3000 cuadros).
- `mkzebes.py`: en las salas de `BOSS_BG2_ROOMS` (CD13, DA60) esos tiles se convierten con los de la sala, con esos
  colores, y una tabla (línea del SNES -> slot, tile) -> celda va en la página de los jefes (`kZebesBossBg2`). Los
  tiles que solo usa el cuerpo (hasta 123) van a la VRAM de 0xF000 (tiles 1920-2042, libres en estas salas; la bomba
  de poder y las cajas de mensaje también la usan: al terminar se vuelven a subir, `zebes_boss_tiles_again`).
- El fondo de biblioteca de estas salas copia 7E:2000 al BG2, que el inicio del jefe llenó con 0x338 (tile vacío):
  `library_bg(..., ram_2000)`. El fondo va en la clave de los gráficos de la sala (la de Draygon tiene el mismo nivel
  en la pelea y vencido, con otro fondo) y el cuerpo solo donde el fondo copia 7E:2000 (`bg_copies_2000`).
- `md_zebes.c` (`nt_enemy_bg2_transfer`): convierte en el bucle principal a un buffer de 1 KB (el principio de
  `cinematic_text_tilemap`, libre en estas salas) y el vblank lo sube al plano B (en las filas y 32 filas más abajo).
  El plano B se mueve con BG2HOFS/VOFS (b_mode 1, como los fondos de biblioteca). Al volver de la pausa se rehace.
- Crocomire (312 tiles, ~155 a la vez, `bg2u` en el registro): caché (`BOSS_BG2_CACHE`). Todos sus tiles quedan en la
  ROM (página de los jefes) y la tabla los marca (bit 15 + número). En cada subida del mapa `bb_convert` marca los
  lugares que ya tienen tiles de esa subida y da a los que faltan otros (177 lugares: los 123 de 0xF000 y las 54
  filas de la ventana que no usa el HUD); el vblank sube los tiles pendientes antes que las filas. La RAM de la
  caché va en `cinematic_text_tilemap` (1024: tile -> lugar, 16 bits; desde 2400: los bits de lugares y lugar ->
  tile). Para que la boca no se rompa al cambiar: no reemplaza lugares del mapa que se ve (`bc_shown`) mientras haya
  otros; los pendientes de un mapa que no llegó a verse se descartan; en salas de una pantalla de alto (`cache` 3,
  la de Crocomire) usa también las filas 36-61 del plano A (281 lugares).
- 7E:2000 lleno de 0x338 en las tres salas (`BOSS_2000_FILL`: en el SNES el inicio del jefe corre antes que la copia
  del fondo, door.c). Los colores de la sala y del cuerpo se juntan pesando sus píxeles (en la de Crocomire el oscuro
  del fondo se juntaba con un rojo).
- Para la parte común: el banco $A5 (Draygon y Spore Spawn) pasó a la página 25 y la sala de Spore Spawn (9DC7)
  también (`BOSS_ROOMS`); probada en la Genesis sin avisos.

## Pruebas

En el PC, cada sala sola contra el SNES (`tools/rooms/lote.sh`, Samus invencible, `PER=3000`):

| Sala | Jefe | Lógica | En la Genesis |
|---|---|---|---|
| A98D | Crocomire | 0 diferencias | se ve como en el SNES (cuerpo en el plano B por la caché, sprites sin el destello); falta el ácido de fondo y la onda |
| B283 | Golden Torizo | 0 | corre; sin ver todavía al jefe |
| B32E | Ridley | 0 | se ve, con sus colores (claves de `mkmd.ridley_keys` sin las del destello: 624 tiles de 632; `BOSS_PALETTES`: su línea 7 de sprites se llena después de cargar la sala); falta la lava de fondo (efecto de la sala) |
| CD13 | Phantoon | 0 | se ve, con el cuerpo en el plano B (sin la onda) |
| D95E | Botwoon | 0 | se ve |
| DA60 | Draygon | igual hasta que Samus muere (secuencia de muerte sin portar) | corre; el cuerpo en el plano B con su desplazamiento (la prueba lado a lado no llega a la pelea en el SNES) |
| DC65, DCB1 | cadáveres, bebé | 0 | corren; el "pudrirse" (`*CorpseRotting`) avisa |
| DD58 | Mother Brain | 0 (lo que alcanza el patrón de prueba) | corre; `LoadFxEntry` y el pudrirse avisan |

Ojo: el patrón de prueba no llega a cada fase de cada pelea (Mother Brain, sobre todo).

## Estado al 2026-11-06

- **Lógica**: todos los jefes de la lista corren en el PC igual que el SNES en lo que alcanzan las pruebas (lote.sh,
  3000 cuadros); en la Genesis corren sin avisos (salvo lo del final de esta página).
- **Dibujo en la Genesis**:
  - Botwoon: se ve.
  - Ridley de Norfair: se ve con sus colores (sin el destello).
  - Phantoon y Draygon: el cuerpo en el plano B con sus colores y su desplazamiento.
  - Crocomire: se ve como en el SNES en la pelea (cuerpo por la caché de tiles, sprites sin el destello; la boca
    comparada cuadro a cuadro).
  - Golden Torizo y Mother Brain: corren, sin revisar todavía el dibujo.
- **Arreglos de paso que valen para todo**: tipos de las tablas de funciones (`CrocomireFunc`: a0/d0 en el 68000),
  fondo de biblioteca en la clave de los gráficos de la sala, colores juntados por peso de píxeles en las salas con
  cuerpo, banco $A5 y la sala de Spore Spawn en la página 25.
- **Cómo probar** (Genesis contra SNES): `tools/compare/lado_a_lado.sh` con `DOOR`, `X`, `Y` (para Crocomire:
  `DOOR=93D2 X=1150 Y=160`); los avisos con `aviso.py` (lee `md_unimpl_*` de la RAM). Tras cambiar `mkzebes.py` o el
  registro hay que regenerar (`make full`, o `mkzebes.py` + `make rom`).

## Crocomire: lo que falta, paso a paso

1. **El destello al recibir daño**: hoy se quitan las claves con la paleta 0000 (no entraban en la VRAM). Idea: dibujar
   las mismas piezas de la paleta 0E00 con un color blanco (sin tiles nuevos).
2. **El ácido de fondo**: la capa 3 de efectos (lava, ácido, agua) no está hecha en la Genesis para ninguna sala; en la
   de Crocomire se ve el ácido abajo. Sirve también para Ridley y las salas de Norfair y Maridia.
3. **La muerte**: el puente que se rompe, la caída al ácido, el derretirse (`Crocomire_Func_67` borra líneas
   verticales de los tiles por la cola de VRAM, `crocomire_bg2_scroll_hdma_data` mueve el BG2 por línea: la "onda") y
   el esqueleto (claves con la paleta 0A00). Hace falta una prueba que llegue ahí (bajarle la vida al empezar).
4. **La sala vencida** (estado con el esqueleto y el puente roto): comprobar que se ve bien.

## Lo que falta

- El dibujo: Mother Brain (su cuerpo no pasó por `tilemap_stuff` en la prueba); la onda de Phantoon y de Crocomire
  (desplazamiento por línea); los colores propios de cada jefe (`mkzobj.BOSS_PALETTES`, como el Bomb Torizo); la lava/ácido de fondo de Crocomire y Ridley (efecto de la sala).
- `LoadFxEntry` (Mother Brain), el pudrirse de los cadáveres (`InitializeEnemyCorpseRotting`, `ProcessCorpseRotting`:
  escriben tiles del SNES), `DeadTorizo_Func_1` y `MotherBrain_AddSpritemapToOam` (dibujo en la cola de VRAM y la OAM
  del SNES).
- Kraid.
