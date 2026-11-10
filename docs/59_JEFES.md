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

1. (Hecho, 2026-11-06) **El destello al recibir daño**: el SNES dibuja al jefe con la paleta 0 en los cuadros alternos
   de `flash_timer` ($A0:9929). En las salas de jefes (`room_is_boss`, md_engine.c) la Genesis dibuja esas piezas con
   su paleta normal y ese cuadro pone en blanco los colores de los objetos (línea 3 debajo del HUD: 1-8, 14, 15; y la
   línea 1 en las salas con línea propia del jefe: Ridley, Golden Torizo); al cuadro siguiente vuelven
   (`boss_flash_post`). Sin tiles ni sprites nuevos. Lo que comparte colores con el jefe en esa sala también parpadea.
   Probado forzando `flash_timer` (retrorun `poke`) con Crocomire y Ridley. El cuerpo de fondo no parpadea (en el
   SNES tampoco: el destello es de los sprites).
2. **El ácido de fondo** (en curso, 2026-11-06): la capa 3 de efectos (lava, ácido, agua) no estaba hecha en la
   Genesis. Inventario (salas alcanzables): agua 59, lava 40, ácido 34, esporas 4, luciérnagas 3, niebla 3 (tinte, sin
   nubes), lluvia 1, agua de la estatua de Tourian 1. En el SNES el líquido es el BG3 en la pantalla principal y la sala
   en la subpantalla, sumados (`LayerBlendFunc_18`, CGADSUB 0x24); dos HDMA mueven la superficie (BG3VOFS) y el fondo
   (BG2VOFS) por línea. Hecho: **lava y ácido como tinte** (md_engine.c, `lq_prepare`/`md_hint_lq`): desde la línea
   de la superficie (`lava_acid_y_pos`) una interrupción horizontal pone una paleta con el color medio del líquido
   sumado (de `palette_buffer` con los pesos de su tilemap: lava 27/26/25, ácido 3/2/1); `md_vint_top` repone la
   normal y los pasos en cada vblank (también en los cuadros atrasados). Pasos dobles (`lq_next` -> `lq_steps`: el
   bucle principal no los pisa mientras la interrupción los lee). La paleta va en `cinematic_text_tilemap` + 3200.
   Superficie: una franja de 8 líneas con 1,5 veces el color (medido en el SNES; una fila de sprites ondulada no
   entra: 8 sprites de 32 llenan el límite de 16 por línea y Samus y los enemigos desaparecerían). La interrupción no
   se apaga: después del último paso salta cada 32 líneas sin hacer nada y en el cuadro siguiente vuelve a empezar
   sola en la línea 31 (así los cuadros atrasados también lo llevan); el primer paso repone la paleta normal. La
   línea: `lava_acid_y_pos - layer1_y_pos - 2` (medido contra el SNES: coincide a una línea). Costo: medido con el
   juego quieto (sin entradas), el mismo ritmo con y sin líquido (`-DMD_NO_LQ`). Queda: el primer cuadro después de
   una racha de atraso puede salir sin el líquido (el vblank llega tarde a la línea 31); sin la onda horizontal.
   La regresión de Zebes se volvió a aprobar (2026-11-06): con el código nuevo cambian unos ciclos y en la pelea con
   Ridley de Ceres y en la huida el atraso cae en otros cuadros (imágenes comparadas píxel a píxel: correctas, un
   cuadro antes o después). Ojo: **la sala de Crocomire corre a ~24 cuadros por segundo en la Genesis** (243 de cada
   600, con o sin líquido): rendimiento pendiente.
   **Agua** (FX 6, 59 salas): mismo mecanismo con `fx_y_pos`, color de los índices 26 (74 %) y 25 (26 %) de su
   tilemap; mezcla 0x14/0x16 (Maridia, 44 salas) **resta** (CGADSUB 0xB3/0xB1), 0x18 (15 salas: Crateria, Brinstar)
   suma. Probado en CEFB y D017 (Maridia): el fondo toma el tono del SNES.
   La regresión cuenta aparte los cuadros de ritmo (5124-5127 y 8034, ver regress.sh).
   Agua de la estatua de Tourian (FX 0x26, A66A): el mismo tilemap y la mezcla 18 (suma): como el agua. Probado.
   **Lluvia** (FX 0x0A, la sala de aterrizaje): 16 sprites de 8x32 con los tiles de las motas del BG3 ($9A:B700,
   tiles 0x50-0x53, `kRainSnesTiles` en mkdata) en lugares fijos de un dibujo de 256x256 que cae 6 píxeles por cuadro
   y deriva (como `HdmaobjPreInstr_RainBg3Scroll`); tiles en 0xF000 (2039-2042). Color: uno libre de la línea 3 o, si
   no hay (la nave los usa todos), el más parecido de la CRAM (como el gancho), buscado como mucho cada 16 cuadros.
   Sin la bruma (suma G+1 B+2: casi nada en la Genesis, y en esa sala la paleta cambia casi cada cuadro: atrasaba).
   La regresión de Ceres y Zebes se aprobó de nuevo (la llegada a Zebes ahora tiene lluvia: comparada imagen a imagen).
   Esporas (4) y las nubes de la niebla (3) quedan así por ahora (acordado); luciérnagas (3) ya se ven casi igual.
3. **La muerte** (en curso, 2026-11-06): prueba con `HL_POKE_AT=800:F7A:640` (nuevo en hlref.c y check.c: una
   palabra de la RAM antes del cuadro grabado n; Crocomire al puente, x = 0x640, `Crocomire_Func_37`) y lote.sh: la
   muerte entera en 0 diferencias con el SNES (3000 cuadros). Hizo falta: el traductor pasa `*(uint16 *)&a[i] = v`
   sobre arreglos de bytes de la RAM del SNES a `nt_ram_set_w(dirección + i, v)` (xlate.py, `U8_ARRAYS`): en el
   68000 la palabra quedaba al revés (`scrolls[3] = 256` ponía el 1 en scrolls[3]: los bloqueos de la cámara de las
   salas de jefes) y `Crocomire_Func_60` (borra croco_vline_height hasta el byte 129) pisaba `croco_target_0688` en el
   PC. Falta: la preinstrucción de su objeto de HDMA (943C, la onda) y verlo en la Genesis. Antes: el puente que se rompe, la caída al ácido, el derretirse (`Crocomire_Func_67` borra líneas
   verticales de los tiles por la cola de VRAM, `crocomire_bg2_scroll_hdma_data` mueve el BG2 por línea: la "onda") y
   el esqueleto (claves con la paleta 0A00). Hace falta una prueba que llegue ahí (bajarle la vida al empezar).
   **Vista en la Genesis (2026-11-10)**: `MD_TEST_POKES` (md_main.c: varios pokes, como HL_POKE_AT) con Crocomire al
   puente, `scrolls[4..7]` en azul, Samus en x = 1480 y la cámara (`layer1_x_pos`) en 1360: si no, la cámara queda a la
   izquierda (la muerte pasa fuera de la pantalla). Encontrado y arreglado:
   - `crocomire_bg2_scroll_hdma_data` (vars_extra.txt) estaba en `cinematic_text_tilemap + 0x6E0` **palabras** (byte
     3520), no bytes: al morir, la tabla de la onda pisaba las tablas del líquido (`lq_tab`, `lq_norm`: colores cian y
     blancos bajo el ácido) y `bb_rsum` (el cuerpo se reconvertía de más: la muerte iba más lenta). Ahora en + 0x370
     palabras (bytes 1760-2319), el hueco que dejan las tablas de la caché (md_zebes.c).
   - claves de la muerte en `zdraw.log` (registradas en el PC con `MM_DRAWLOG` y la misma prueba, 0 diferencias con
     el SNES): la nube de polvo (`DustCloudExplosion`, $8D:AC9B-AD08, g = 0) y el humo (objetos de sprite $B4:C838-C8A5).
   - el fondo del SNES en esa prueba queda viejo (dibuja una columna por cuadro y la cámara salta 500 píxeles); la
     Genesis redibuja la ventana: allí la diferencia es de la prueba.
   - Pendiente: el humo sale marrón o azulado (el SNES: gris): sus colores se funden con otros (42 colores de la sala
     en 25 lugares); y el ritmo de la Genesis en la muerte.
   **El derretido y la onda (2026-11-10)**:
   - Derretido: `Crocomire_Func_60` copia a 7E:4000 filas de tiles del banco $A4 (fase 1: 6 filas desde $A4:A07D; fase
     2: 7 desde $A4:AC7D; listas de $A4:9BC5) y `Crocomire_Func_67` las sube como tiles r * 16 + c (c < 11) del BG2,
     borrando cada vez píxeles de columnas al azar hasta `croco_vline_height`. mkzebes pone esos 143 tiles en la caché
     del cuerpo con números de tile que la sala no usa (`kCrocoMelt[fase][r * 11 + c]`; la caché pasa a 400 tiles,
     `BC_K`); md_zebes.c los usa en vez de los de la VRAM según la fase (`croco_word_7E069A`), reconvierte el cuerpo
     al cambiar de fase y, mientras dura (HDMA $A4:943C activo), arma 4 de esos tiles por cuadro desde la ROM con las
     columnas borradas y los sube (`melt_frame`/`melt_vblank`). Antes se veían los tiles normales del cuerpo en esos
     números: un rompecabezas.
   - Onda: el HDMA $A4:943C (tabla indirecta {0xFF -> 7E:CAF0, 0xE1 -> 7E:CBEE}: el BG2VOFS de cada línea) es el
     estirón del cuerpo al hundirse (1 a ~25 píxeles desde la línea ~145). Va en la interrupción horizontal del
     líquido: `lq_prepare` mezcla, ordenados por línea, los pasos del líquido y los de la onda (una palabra a la VSRAM
     del plano B en cada línea en que cambia la tabla, las que quepan; línea del SNES + 2). Las listas pasaron a
     `cinematic_text_tilemap` (10 pasos: `lq_steps` + 3968, `lq_next` + 3520) y `md_vint_top` repone el scroll de
     arriba. El HDMA reemplaza al BG2VOFS en todas las líneas: `scroll_by` usa la tabla.
   - RAM (cinematic_text_tilemap, sala de Crocomire): 0-1023 filas del cuerpo, 1024-1823 `bc_k2s`, 1840-2399
     `crocomire_bg2_scroll_hdma_data` (vars_extra.txt: + 0x398 palabras), 2400-3121 tablas de la caché, 3128 y 3840
     tiles del derretido, 3200-3455 colores del líquido, 3456-3519 valores de la onda, 3520 `lq_next`, 3648 `lq_tab`,
     3904 `bb_rsum`, 3968 `lq_steps`.
4. **La sala vencida** (2026-11-10): con `boss_bits_for_area[2] |= 2` (poke a 7E:D82A antes del salto) la Genesis
   elegía bien el estado (A9B9) pero Crocomire salía vivo: `Crocomire_Init` lee `*(uint16 *)&boss_bits_for_area[area_index]`
   y en el 68000 esa palabra mira el byte de al lado. xlate.py ahora pasa también las **lecturas** de palabras sobre
   arreglos de bytes de la RAM del SNES a `nt_ram_w(dirección)`, y las escrituras, RMW y lecturas sobre variables de un
   byte (`U8_SCALARS`: `reg_TM |= 2`, `reg_BG2SC &= 0xFFFC`, `wrecked_ship_robot_palanim_table_index`) a
   `nt_ram_w`/`nt_ram_set_w`. Arregla también los otros jefes que miran su bit así (`& 1`: Phantoon y otros, 3 lugares).
   Ahora la sala vencida se ve como en el SNES (el esqueleto en x = 576, con sus colores). PC: la muerte (3000 cuadros)
   y la sala vencida (2000) en 0 diferencias. Regresión: sin cambios.

## Rendimiento de las salas de jefes (2026-11-06)

Perfil del 68000: núcleo GPGX con el parche `tools/emu_harness/gpgx_mm_profile.patch` (solo la parte de
`m68kcpu.c`: lo demás ya está en `third_party/gpgx`; se compila con `make -f Makefile.libretro`) y `MM_PROF`; dos
corridas (hasta 13600 y hasta 14200 con el juego quieto: sin entradas el juego hace lo mismo por cuadro del motor y se
puede comparar) y la diferencia por función (`prof_report.py`, `tools/perf/salas/hot.py`). Ojo: las interrupciones
escritas en ensamblador no tienen tamaño en la tabla de símbolos; usar `nm` (no `objdump -t` filtrado por `F`).

Sala de Crocomire (quieto, 600 cuadros de la consola): **243 -> 462 cuadros del motor** (con disparos: 424).
- El mapa del cuerpo: no se reconvierte si no cambió, y solo las filas que cambiaron (suma por fila, `bb_rsum`); el
  vblank sube solo esas filas.
- El líquido: sus paletas solo se rehacen enteras si cambió su color o muchos índices; si cambiaron pocos
  (`nt_palette_lo/hi`), solo los colores que salen de ellos (`cram_rev`). La interrupción escribe menos colores (127
  en vez de 192: escribir en la CRAM durante la imagen frena al 68000).
- `BankW` (enemy_main.c) recuerda el comienzo de la copia del banco (`port_bank_base`, generado por port.py): las cajas
  de golpe de los spritemaps extendidos leen cientos de palabras por cuadro y el `switch` era el 12 %.
Lo que queda caro: `DrawEnemyQueue` / `WriteEnemyOams` (las piezas de Crocomire) y la IA (`EnemyMain`).

Las demás salas de jefes (quieto, cuadros del motor de cada 600): Phantoon 583, Draygon 593, Spore Spawn 591, Golden
Torizo 542, Mother Brain 502, Ridley 454, Botwoon 340 -> **445** (`nt_ram_w` lee directo la RAM extra de los enemigos,
7E:7800-97FF: Botwoon lee la historia de su cuerpo en 7E:9000 + índice y la búsqueda era el 25 %; en el PC sigue en
0 diferencias). Pendiente: Ridley (`md_post`, `zebes_obj_draw`), Mother Brain (`EnemyMain`) y lo que queda de Crocomire.

## Pendiente: la sala de las estatuas de Tourian (A66A)

Anotado a pedido de Edwin (2026-11-06): en la Genesis no se ven todas las estatuas de los jefes (Kraid, Phantoon,
Draygon, Ridley) con su gema iluminada; en el SNES se ven las cuatro (comparar con `lado_a_lado.sh`, `DOOR=91F2`).
Hay que revisar en el decomp (la biblia) qué hacen: las estatuas son enemigos `TourianEntranceStatue` ($AA, init
$AA:D7C8: escribe las líneas 2 y 7 de los sprites; ya en `mkzobj.init_palettes`) y sus partes en el BG; con cada jefe
vencido (eventos 6-9, `HdmaobjPreInstr_CheckLotsOfEventsHappened` $88:DBD7) la estatua de ese jefe se apaga, y con
los cuatro el PLM ($84:B777) abre el paso (`FxTypeFunc_26`: el agua baja). Definir con eso qué falta: el dibujo de las
estatuas y las gemas, y comprobar que al vencer a cada jefe pasa lo mismo que en el SNES.

## Lo que falta

- El dibujo: Mother Brain (su cuerpo no pasó por `tilemap_stuff` en la prueba); la onda de Phantoon y de Crocomire
  (desplazamiento por línea); los colores propios de cada jefe (`mkzobj.BOSS_PALETTES`, como el Bomb Torizo); la lava/ácido de fondo de Crocomire y Ridley (efecto de la sala).
- `LoadFxEntry` (Mother Brain), el pudrirse de los cadáveres (`InitializeEnemyCorpseRotting`, `ProcessCorpseRotting`:
  escriben tiles del SNES), `DeadTorizo_Func_1` y `MotherBrain_AddSpritemapToOam` (dibujo en la cola de VRAM y la OAM
  del SNES).
- Kraid.

## Spore Spawn: HUD y esporas (2026-11-10)

- El bloqueo que se reportó no aparece en la ROM de esta rama (confirmado por el usuario y en una pelea de 4500
  cuadros en la ROM de prueba).
- HUD: en el SNES el HUD (BG3 con prioridad, BGMODE bit 3) tapa todos los sprites de las líneas 0-31; en la Genesis
  la Window no tapa a los sprites, así que el tallo de Spore Spawn (y Ridley en Ceres) asomaba encima del HUD.
  `md_logic` pone en todo el juego (estados 7-11) los dos sprites de máscara que ya usaban las salas del ojo
  (`md_cone.c`): 8x32 en las líneas del HUD, uno con x = 1 y otro con x = 0, que enmascara a los que siguen en esas
  líneas. Regresión de Ceres y Zebes re-aprobada: solo cambian las líneas 0-31 (Ridley de Ceres ya no asoma).
- Esporas: no tenían claves (las crea la IA, no una lista: `SporeSpawners` → `Spores` $86:DE7A, spritemaps
  A9B0/A9B7/A9BE) ni la nube de polvo (`SpawnEprojWithRoomGfx(addr_kEproj_DustCloudExplosion, 0x18)`, sm_a5.c, B023-B082
  con g = 0x0200): se sumaron a `zdraw.log` (lo que pidió `md_missing_ring`). Y salían negras: usan la paleta 7
  (`palette_buffer` 249-252), que llena `SporeSpawn_Init` desde `kSporeSpawn_Palette` ($A5:E359). `mkzobj.init_palettes`
  ahora reconoce los arreglos con `#define kX ((uint16*)RomFixedPtr(0x...))` y el índice `(i >> 1) + N`.
- Pendiente: la "sombra" del cuerpo (plano B) bajo el HUD que se ve al subir el jefe.

## Pendientes reportados (2026-11-10)

- Probado por el usuario hasta vencer a Spore Spawn: esporas y HUD bien en todo el juego.
- Bomb Torizo (sala 9804): durante la pelea nunca abrió los ojos (en el SNES los abre: brillo de los ojos, paleta /
  piezas de la cara) y al terminar quedaron dos círculos marrones cerca de la puerta (¿restos de proyectiles u
  objetos de sprite que no se borran, o tiles de la explosión con los colores de la sala?). Revisar contra la biblia.

## Kraid (2026-11-10, en curso)

- Sala de los mini Kraid (A521, `FakeKraid`): sus púas ($86:9DBE/9DCC: spritemaps $8D:8427 y 842E, g = 0x0660) no
  tenían clave: `FakeKraid_Instr` guarda la cabecera en una variable (`v6 = addr_kEproj_MiniKraidSpikesLeft`) antes de
  `SpawnEprojWithGfx` y mkzkeys solo miraba los argumentos. mkzkeys ahora toma también esas asignaciones; con todas
  las salas (salas.py, áreas 0-5) solo faltaban esas dos claves en todo el juego. PC: A521 en 0 diferencias.
- Sala de Kraid (A59F): Kraid nunca aparece porque su IA ($A7: Kraid, brazo, pelusas, pie, uñas; rocas en $86) no
  está portada. Al agregarlo a roots.txt el traductor deja sin resolver: corrutinas (`Kraid_UnpauseHook_IsSinking`),
  ganchos de pausa (`pause_hook`/`unpause_hook`), la cola de lectura de VRAM y copias por DMA (`PauseHook_Kraid`,
  `UnpauseHook_Kraid_*`), `ram4000`/`g_ram` (`Kraid_SetupGfxWithTilePrioClear`: mapas descomprimidos a 7E:4000 y
  7E:2000) y `Kraid_Instr_9`. Hay que escribirlos a mano (bosses.c) y dibujar su cuerpo en el plano B (como
  Crocomire: BOSS_BG2_ROOMS, zbg2.log).
- **Portado (paso 1)**: roots.txt con Kraid y sus partes; el traductor ahora resuelve los ganchos de la pausa puestos por
  partes (`.bank`/`.addr`), los punteros a la ROM declarados aparte (`Kraid_Shot_Mouth`), `ERAM_ARR` (definida en
  enemies.h) y el despacho de disparo con `j` (`Kraid_Arm_Shot(j)`: `collision_detection_index * 2`). A mano en bosses.c:
  `Kraid_SetupGfxWithTilePrioClear` (mapas ya descomprimidos: mkdata `kKraidMap2000`/`kKraidMap4000`, página de los
  jefes), `Kraid_Death_Fadeout`, `Kraid_TransferTopHalfToVram` y los ganchos de la pausa (solo VRAM: vacíos). A59F en
  BOSS_ROOMS. viewer.c sacaba a propósito los enemigos de la sala de Kraid (sin portar): ya no.
  PC contra el SNES (3000 cuadros): 3 cuadros distintos (la uña mala, enemigo 7, se dispara un cuadro antes: pendiente).
  La ROM de prueba no entraba en la parte común: su entrada va por tramos (tools/conv/rle_test_input.py, md_main.c).
- Pendiente: el cuerpo en el plano B (ahora salen tiles de la sala), el fondo de la sala distinto, claves de las rocas
  ($8D:8268/826F, g = 0x0600), el polvo ($B4:C838-C8A5) y una pieza ($8D:804D, 0E00).
