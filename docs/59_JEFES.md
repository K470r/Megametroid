# 59 — Jefes: dónde va su código y cómo se prueban

Rama `claude/trusting-galileo-ct4zjp` (2026-11-05). Jefes: Phantoon, Draygon, Botwoon, Crocomire, Ridley de
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

## Los cuerpos en el BG2 (Phantoon; Draygon y Crocomire no entran todavía)

El SNES sube `tilemap_stuff` (hasta 1024 bytes: 16 filas de 32 celdas) al BG2 en $4800 (`QueueEnemyBG2TilemapTransfers`
$A0:9726). En la Genesis:

- `host/pc_stubs.c` (con `MM_DRAWLOG`) registra las palabras distintas de cada subida y el color más brillante de cada
  índice del BG; `res/snes_ref/zbg2.log` tiene lo de Crocomire, Phantoon y Draygon (lote.sh de 3000 cuadros).
- `mkzebes.py`: en las salas de `BOSS_BG2_ROOMS` (hoy CD13) esos tiles se convierten con los de la sala, con esos
  colores, y una tabla (línea del SNES -> slot, tile) -> celda va en la página de los jefes (`kZebesBossBg2`).
- `md_zebes.c` (`nt_enemy_bg2_transfer`): convierte en el bucle principal a un buffer de 1 KB (el principio de
  `cinematic_text_tilemap`, libre en estas salas) y el vblank lo sube al plano B (en las filas y 32 filas más abajo).
  El plano B se mueve con BG2HOFS/VOFS (b_mode 1, como los fondos de biblioteca). Al volver de la pausa se rehace.
- VRAM: Draygon suma 148 tiles y Crocomire 317: con sus objetos no entran (falta liberar VRAM o repartir).
- Para la parte común: el banco $A5 (Draygon y Spore Spawn) pasó a la página 25 y la sala de Spore Spawn (9DC7)
  también (`BOSS_ROOMS`); probada en la Genesis sin avisos.

## Pruebas

En el PC, cada sala sola contra el SNES (`tools/rooms/lote.sh`, Samus invencible, `PER=3000`):

| Sala | Jefe | Lógica | En la Genesis |
|---|---|---|---|
| A98D | Crocomire | 0 diferencias | corre sin avisos; sus objetos entran sin las copias del destello (`mkzobj.FLASH_DROP`: 592 tiles de 592; el destello blanco al recibir daño no se ve); cuerpo en BG2 sin dibujar; sin verlo todavía en pantalla (la prueba lado a lado no llega a la pelea) |
| B283 | Golden Torizo | 0 | corre; sin ver todavía al jefe |
| B32E | Ridley | 0 | se ve, con sus colores (claves de `mkmd.ridley_keys` sin las del destello: 624 tiles de 632; `BOSS_PALETTES`: su línea 7 de sprites se llena después de cargar la sala); falta la lava de fondo (efecto de la sala) |
| CD13 | Phantoon | 0 | se ve, con el cuerpo en el plano B (sin la onda) |
| D95E | Botwoon | 0 | se ve |
| DA60 | Draygon | igual hasta que Samus muere (secuencia de muerte sin portar) | corre; partes en BG2 sin dibujar |
| DC65, DCB1 | cadáveres, bebé | 0 | corren; el "pudrirse" (`*CorpseRotting`) avisa |
| DD58 | Mother Brain | 0 (lo que alcanza el patrón de prueba) | corre; `LoadFxEntry` y el pudrirse avisan |

Ojo: el patrón de prueba no llega a cada fase de cada pelea (Mother Brain, sobre todo).

## Lo que falta

- El dibujo: los cuerpos en BG2 (`tilemap_stuff`: Phantoon, Crocomire, Draygon, Mother Brain) necesitan en la Genesis
  tiles, plano B y el desplazamiento por línea (la onda de Phantoon y de Crocomire); los colores propios de cada jefe (`mkzobj.BOSS_PALETTES`, como el Bomb Torizo); la lava/ácido de fondo de Crocomire y Ridley (efecto de la sala).
- `LoadFxEntry` (Mother Brain), el pudrirse de los cadáveres (`InitializeEnemyCorpseRotting`, `ProcessCorpseRotting`:
  escriben tiles del SNES), `DeadTorizo_Func_1` y `MotherBrain_AddSpritemapToOam` (dibujo en la cola de VRAM y la OAM
  del SNES).
- Kraid.
