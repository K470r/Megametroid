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
- **Paso 2**: claves de la sala de Kraid (zcover en A59F, 12000 cuadros; y mkzkeys ya con Kraid traducido): las rocas
  ($8D:8268/826F, g = 0x0600), el polvo ($B4:C838-C8A5) y $A7:804D (vacío). Ya se ven en la Genesis. Quedan fuera
  $A7:9608/9632/965C (mkzkeys las encuentra en las listas; 32 tiles cada una): con ellas los objetos del estado con
  Kraid vivo (A5B1, la sala ya usa 304 tiles) no entran en la VRAM.
  PC (host/pc_stubs.c): la cola de VRAM de Kraid (sus dos mitades de 7E:2000/2800) se registra como el BG2 de los
  otros jefes; zbg2.log tiene sus 509 palabras y colores (zcover). mkzebes todavía no las usa.
- **El cuerpo (pendiente, el paso grande)**: su BG2 es de 64x64 (`reg_BG2SC = 0x43`), el fondo de biblioteca llena las
  cuatro pantallas desde $B9:FA38 y $B9:FE3E y Kraid sube su cuerpo a la columna izquierda (VRAM $4000 y $4800). Con
  el cuerpo convertido como los otros jefes la sala más el cuerpo dan 680 tiles y los objetos de Kraid (brazo, pies,
  uñas, pelusas: ~460) ya no entran: hace falta un reparto de la VRAM propio de esta sala (el cuerpo en una caché como
  Crocomire, el fondo de biblioteca aparte, o menos tiles de la sala). Borrador del generador en
  tools/conv/wip/mkzebes_kraid_bg2.py (library_bg de 64x64, BOSS_BG2_64).
- El fondo de la sala (plano A) se ve distinto del SNES ya en la ROM anterior: en esta sala las capas no son las de
  siempre (en el BG1 del SNES aparece textura del cuerpo); revisar cómo las reparte antes de comparar.

## Kraid: plan (2026-11-10)

Medido en la VRAM del SNES en plena pelea (hlref `HL_VDUMP`):

| | SNES | Genesis (con los volteos de sus tiles) |
|---|---|---|
| BG2 (pared del fondo + cuerpo de Kraid, 64x64, tiles desde la palabra 0x2000: `BG12NBA = 0x0200`) | 490 tiles | **287** únicos; como mucho **253** en una pantalla |
| BG1 (la sala, 64x32, tiles desde 0) | 167 | **150** |
| Sprites de Kraid (brazo, pies, uñas, pelusas, rocas, polvo; todos sus cuadros) | — | 458 (mkzobj) |

Lugar en la VRAM de la Genesis: 1280 tiles (0x0000-0x9FFF), de los que 448 son comunes (Samus, HUD, rayo); más 123 en
0xF000 y 54 en las filas libres de la Window: **~1010 para la sala, el cuerpo y los objetos**. Con volteos: 150 + 287 +
458 = 895: entra sin redibujar nada. Hoy no entra (680 tiles de sala y cuerpo) porque el generador lee el BG2 como
el de los otros jefes (64x32 en $4800, tiles desde 0): Kraid lo tiene en otro lugar y de otro tamaño.

Paleta: el cuerpo (BG2) y todas las piezas (sprites, `0E00`) usan la paleta 7 del SNES: una línea de la Genesis
(`BOSS_LINE`) con sus 15 colores exactos para el cuerpo y las piezas a la vez, sin juntar colores.

Pasos:
1. **Capas y fases** en el SNES: subida (el techo se rompe, la cámara), pelea, muerte (se hunde: `kKraidSinkEntry`
   borra filas por la cola de VRAM, fundido de la paleta 6) y sala vencida. Qué escribe Kraid en el mapa del BG2: las
   dos mitades (7E:2000/2800 -> $4000/$4800), la boca y el ojo (4 cuadros de 352 palabras desde $A7:97C8-A0C8 a
   $4000) y el borrado al hundirse.
2. **Generador**: el BG2 de 64x64 desde $4000 con los tiles desde 0x2000 (borrador: tools/conv/wip/), los tiles con
   volteos, las palabras de la boca y del ojo también, el cuerpo en `BOSS_LINE`; reparto propio de la VRAM de la sala:
   comunes | sala | cuerpo | objetos, más 0xF000 y la Window. Si no alcanza: los sprites de Kraid subidos por cuadro
   (solo los del cuadro de animación que se ve, como Samus) o el cuerpo en la caché de Crocomire (253 a la vez).
3. **Motor**: la cola de VRAM de Kraid (sus entradas a $4000/$4800 y las de la boca) convierte palabras al plano B
   como `bb_convert`; el scroll del BG2 (la subida); los fundidos de su paleta.
4. **Pruebas** contra el SNES: lado_a_lado y los planos armados por separado desde la VRAM de las dos consolas (como
   con Crocomire); la muerte con `HL_POKE_AT`/`MD_TEST_POKES` (vida de Kraid: 7E:0F8C); la sala vencida.
5. **Terminar**: las rocas (salen amarillentas), la uña mala (un cuadro antes en el PC), el rendimiento.

Redibujar tiles: con los volteos no hace falta. Fusionar tiles parecidos casi no ahorra (8 de 287 difieren en 6
píxeles o menos). Si alguna vez hiciera falta, la reducción se hace en el generador con la ROM de cada uno (nada del
juego en el repositorio, docs/43), no a mano.

### Kraid, paso 1: capas y fases en el SNES (2026-11-10)

Medido con hlref (`HL_POKE_AT=1100:F8C:35C` para que se haga grande, `2000:F8C:0,2000:FA8:C360` para la muerte;
`tools/compare/fases_snes.py` lista los cambios de estado y de registros).

Registros en la pelea: `BG1SC = 0x51` (BG1: mapa 64x32 en $5000), `BG2SC = 0x43` (BG2: 64x64 en $4000), `BG12NBA =
0x0200` (tiles del BG1 desde 0, del BG2 desde la palabra 0x2000), `TM = 0x13` (BG1, BG2 y sprites; sin BG3).

| Fase | Estados (`kraid_var_A`) | Qué se ve |
|---|---|---|
| Espera | `RestrictSamusXtoFirstScreen` (300 cuadros) | la sala; detrás **negro**: el mapa del BG1 ya tiene la pared de la cueva (tiles 1008-1023, paleta 6) pero la paleta 6 está en negro |
| Sube | `RaiseKraidThroughFloor`, `Raise_LoadTilemapBottomAndShake`, `Raise_SpawnRandomEarthquakeProjs16/8`, `Raise_Handler` | el cuerpo es el BG2 (paleta 7): sube con su scroll vertical (`BG2VOFS`), el horizontal sigue a Kraid; temblor y rocas |
| Primera fase | `KraidFoot_FirstPhase_*`, `KraidsFoot_PrepareToLungeForward` | el pie embiste; la boca y el ojo: 4 cuadros de 352 palabras ($A7:97C8, 9AC8, 9DC8, A0C8) por la cola de VRAM a $4000 (las 11 filas de arriba del mapa del BG2) |
| Se hace grande (vida < 7/8) | `GetsBig_ReleaseCamera`, `GetsBig_BreakCeilingPlatforms` (PLMs rompen las plataformas del techo, rocas), `GetsBig_SetBG2TilemapPrioBits` (prioridad alta en todo el BG2, sube la mitad de arriba), `GetsBig_FinishUpdateBg2Tilemap` (la de abajo), `GetsBig_DrawRoomBg` (`kKraidsRoomBg`: 16 tiles a los 1008-1023 del BG1; paleta 6 = `kKraid_Palette2`), `GetsBig_FadeInRoomBg` (fundido de la paleta 6) | el BG2 sube 160 píxeles (`BG2VOFS` 0xFFD0 -> 0x0070): el cuerpo entero; la cámara se suelta; aparece la pared de la cueva |
| Pelea | `GetsBig_Thinking`, `Mainloop_Thinking`, `Main_AttackWithMouthOpen`, `Shot_*` (el ojo), pelusas, uñas, `KraidsFoot_SecondPhase_*` | camina (el BG2 sigue a Kraid), boca y ojo por la cola de VRAM |
| Muerte | `Death_Init` (paleta 7 = `kKraid_BgPalette7_KraidDeath`, la 6 a negro), `Death_Fadeout` (al terminar: 512 bytes de ceros a los tiles 1008-1023), `Death_UpdateBG2Tilemap{Top,Bottom}Half`, `Death_SinkThroughFloor` (se hunde: `kKraidSinkEntry` borra filas del mapa del BG2 por la cola de VRAM) | la pared se apaga; Kraid se hunde |
| Sala vencida | `FadeInBg_ClearBg2Tilemap{Top,Bottom}Half` (0x338), `FadeInBg_LoadBg3Tiles1..4of4`, `FadeInBg_FadeInBp6`, `FadeInBg_SetEnemyDead_*` | el BG2 vuelve a 64x32 en $4800 (`BG2SC = 0x48`); la pared de la cueva otra vez, fundida. `Kraid_Init` con Kraid muerto lleva directo a este estado |

Lo que implica para la Genesis (pasos 2 y 3):
- El **"fondo roto"** de hoy: la Genesis dibuja la pared de la cueva desde el principio (sus colores se juntan con otros
  y no siguen la paleta 6 en negro). Los tiles 1008-1023 del BG1 son de Kraid (`kKraidsRoomBg`) y se cargan, se
  borran y se funden por fases: en la Genesis, sus colores tienen que salir de la paleta 6 en vivo (fundidos) y sus
  tiles tienen que cambiar cuando el SNES los sube.
- El **cuerpo** es todo el BG2 de 64x64 (paleta 7), movido con su scroll: en la Genesis, el plano B (también de 64x64)
  con sus tiles; la boca y el ojo son reescrituras del mapa (filas de arriba) por la cola de VRAM; al hundirse, filas
  borradas por la cola; al final el BG2 vuelve a ser el fondo de biblioteca (64x32).
- La prioridad de todo el BG2 cambia a alta al hacerse grande (`SetBG2TilemapPrioBits`).

### Kraid, paso 2: corrección de las cuentas (2026-11-11)

**Error en el plan**: leí `BG12NBA` (7E:005D) junto con el byte siguiente (`BG34NBA` = 0x02). En verdad `BG12NBA =
0x00`: el BG2 de Kraid toma los tiles desde 0, como la sala (su cuerpo está en el tileset de la sala; en la palabra
0x2000 está la fuente del BG3). Medido de nuevo: el cuerpo son **~445 tiles de la Genesis** (los volteos casi no
ahorran) y casi no comparte con la sala.

| | tiles |
|---|---|
| comunes (Samus, HUD, rayo) | 448 |
| sala | 209 |
| cuerpo | ~445 |
| sprites de Kraid, todos sus cuadros | 458 |
| total | ~1560 |
| lugar (0-1279, 0xF000, Window) | ~1457 |

Faltan ~100. Opciones:
- **Sprites de Kraid subidos por cuadro** (como Samus): a la vez usa ~150-180 tiles (brazo ~34, pie ~34, Kraid ~27,
  pelusas y uñas ~45, rocas y polvo ~30) en vez de 458. Sin pérdida; sirve también para Ridley, Mother Brain y
  Draygon. Trabajo: una caché de tiles de sprites (como la del cuerpo de Crocomire) y subirlos en el vblank.
- **Juntar tiles parecidos del cuerpo** (~100 de 445): en el generador, más rápido, pero con pérdida visible en sus
  escamas.

Ya hecho (desactivado: A59F fuera de `MM_BOSS_BG2_ROOMS` por omisión): el BG2 de 64x64 en $4000 (`library_bg`
full64, `bg_loads_4000`), las palabras de la boca y del ojo ($A7:97C8-A0C8) en la tabla del cuerpo, el mapa inicial
como parte del cuerpo, los tiles del cuerpo en 0xF000 y en las filas libres de la Window (`BOSS_WIN_TILE`,
`bt_dma` en dos partes) y el plano B de 64x64 (`ZebesGfx.bg2h`). Con A59F activado el cuerpo ya aparece en su lugar
(falta lugar para los sprites).

### Kraid, paso 2: sprites por cuadro (2026-11-11)

Elegido por el usuario: subir los sprites de Kraid por cuadro (sin pérdida). Hecho:
- **Generador** (`mkzobj.STREAM_ROOMS = {A59F}`): en esas salas los objetos se convierten con base 0 y cada clave
  pasa a ser una entrada del juego común de los congelados (tiles propios desde 0 y registros; `stream_entries`). La
  sala ya no tiene tiles fijos de objetos (`ZebesObj.stream`). También los proyectiles enemigos (`STREAM_EPROJ`).
- **Motor** (md_zebes.c): el depósito de los congelados se configura al entrar a la sala (`frz_setup`): normal, 3
  lugares de 18 tiles en la Window; con `stream`, hasta 48 lugares de 8 tiles desde el final de la sala (Kraid vivo:
  928-1279), con sus tablas en `cinematic_text_tilemap + 3200`. Un lugar se libera si no se dibujó en `FRZ_KEEP`
  cuadros; el vblank sube los tiles pedidos. Los proyectiles enemigos (`ep_lookup`, que copia registros) usan
  `zebes_obj_ep_recs`: sus registros con el lugar ya sumado en un búfer (`+ 3488`), sin recordarlos.
  `zebes_load_vram` reinicia el depósito (la pausa pisa sus tablas).
- Con esto entran el cuerpo de Kraid (plano B de 64x64, `BOSS_BG2_ROOMS` con A59F) y todos sus sprites: Kraid se ve
  entero en la Genesis, sube y se mueve como en el SNES, con brazo, rocas y polvo. Sin claves que falten.
  Regresión: 0 distintos.

Lo que falta (paso 3): la sala (BG1: las púas, la pared de la cueva y sus colores; la paleta 6 en vivo y los tiles
1008-1023 que Kraid sube), lo que Kraid escribe en su BG2 (boca y ojo, sus dos mitades, el hundimiento), los colores
del cuerpo en su propia línea y el rendimiento con el depósito.

### Kraid, paso 3 (2026-11-11, en curso)

- **Tileset de Kraid**: en el SNES (`DoorTransitionFunction`) el CRE se descomprime primero (7E:7000) y el tileset
  encima (7E:2000): el de Kraid (tileset 26, 0x8000 bytes) pisa el CRE y sus tiles 640-1023 son la parte baja de su
  cuerpo. `mkzebes.tileset` ponía el CRE después: la sala y el cuerpo salían con tiles equivocados (el "fondo roto").
  Ahora en el mismo orden que el SNES; la sala de Kraid ya coincide (piso, púas, puerta, colores).
- **La pared de la cueva** (BG1 1008-1023): `convert(own=...)` les da tiles propios convertidos desde `kKraidsRoomBg`
  con su paleta (`kKraid_Palette2`, $A7:86C7); `kZebesWall` lista sus tiles de la Genesis y su contenido. El motor
  los deja vacíos al entrar y los llena o los borra cuando Kraid los sube por la cola de VRAM (`wall_on`). Sus
  colores leen la paleta 6 en vivo: el fundido es el del SNES.
- **La cola de VRAM de Kraid** (`nt_vram_queue_flush`, md_zebes.c): sus escrituras al mapa del BG2 (las dos mitades,
  la boca y el ojo, el hundimiento, el borrado) pasan al plano B por filas (`kq_*`, 16 por cuadro, convertidas con la
  tabla del cuerpo), con `BG2SC` como esté (64x64 o 64x32).
- **Tabla del cuerpo sin volteos**: las palabras del mapa inicial y de la boca traen sus volteos y la prioridad; la
  tabla los tenía grabados y el motor los volvía a aplicar (la barriga salía revuelta). Ahora la tabla va sin ellos.
- **El cuerpo con BG12NBA 0**: el bloque de Kraid en `mkzebes` copiaba cada tile del cuerpo a un número "libre" (era
  para un BG2 con tiles desde otra base). Con base 0 esos números libres eran otros tiles del cuerpo aún sin copiar:
  la barriga de la fase grande salía revuelta. Con base 0 los números quedan como están.
- **El fundido de la pared no terminaba**: `AdvancePaletteFade_BgPalette6` ($82:D96C) estaba sin traducir (el stub
  devolvía 0): Kraid se quedaba en `Kraid_GetsBig_FadeInRoomBg` para siempre (no caminaba, no tiraba uñas ni pelusas,
  la pared quedaba negra). Ahora está escrita a mano en door.c (con `nt_pal_dirty(96, 111)`).
- **La boca por cuadro**: los 4 cuadros de la boca y del ojo ($A7:97C8-A0C8, 352 palabras, una entrada de 704 bytes
  en la cola de VRAM, `Kraid_ExecuteInstr`) tienen 0, 69, 71 y 65 tiles que no usa nada más. Van a un lugar común de
  71 tiles justo después de la sala (`kraid_mouth_split` en mkzebes; `ZebesWall.mouth*`), y el motor sube los del
  cuadro que Kraid pide en el mismo vblank que sus 11 filas (`kKraidMouthSrc`, `mouth_pend`). Ahorra 134 tiles: la sala
  con Kraid vivo pasó de 744 a 607 (con la boca) y el depósito de sprites de 11 a 28 lugares.
- **Depósito por cuadro**: en las salas por cuadro un lugar se suelta si no se dibujó en el cuadro anterior (antes 8:
  los cuadros viejos del pie y del brazo, de 32-37 tiles, llenaban el depósito). Arreglo: un lugar nuevo que empezaba
  a mitad de uno que se soltaba dejaba válida la cabeza del viejo.
- **Colores de las púas y del cuerpo**: el color 5 de la línea 0 salía del índice 115 (línea 7, el cuerpo, que
  cambia en vivo: `Kraid_HealthBasedPaletteHandling`, el destello al recibir daño): las púas se volvían amarillas.
  En la sala de Kraid (`convert(own=...)`) un color que solo usan las líneas 6-7 (pared y cuerpo) ya no se junta con
  uno que solo usa la sala, y un color que comparten exacto sigue al índice de la sala.
- **Pendiente, colores de la sala**: la sala, la pared y el cuerpo comparten 2 líneas (30 colores) y se juntan 21: los
  bloques azules junto a la puerta salen con puntos rosados y la puerta más magenta que en el SNES (ya pasaba antes).
- **Claves de las pelusas** (`kKraid_Ilist_8B04`: 8C6C quieta y A617-A689 volando; 9608, 9632, 965C): agregadas a
  zdraw.log a mano desde lo que faltó en la Genesis (la corrida del PC con `HL_ENEMIES` no registra enemigos; queda
  por revisar).
- **La pared muy oscura** es del SNES: sus colores (`kKraid_Palette2`) valen 1-6 de 31 y la Genesis los lleva al
  nivel del DAC más cercano (casi negro, con puntos). Fiel; se puede aclarar si se prefiere.

### Kraid, paso 3: muerte, sala vencida, pausa y rendimiento (2026-11-11)

Probado contra el SNES escribiendo `kraid_var_A = Kraid_Death_Init` ($C360) y la vida en 0 en el mismo momento de los
dos lados (`HL_POKE_AT` / `MD_TEST_POKES`), y la sala vencida al entrar con `boss_bits_for_area` (7E:D829) en 1:
- **Muerte y hundimiento**: los mismos estados y los mismos 313 cuadros de lógica; el cuerpo se borra por filas (la
  cola de VRAM, `kKraidSinkEntry`), las púas del piso (fila 27) se desmoronan de a una cada 12 cuadros, las rocas
  caen, la pared se apaga y se vuelve a encender, y la sala queda vencida igual que en el SNES.
- **Púas "fantasma" del SNES**: durante el temblor del hundimiento el SNES dibuja algunos bloques desmoronados en
  otro lugar del mapa del BG1 (la cola muestra `0x56DA` para la columna 13): en su pantalla quedan púas de las
  columnas 13-15 (sin colisión) y un bloque marrón junto a la puerta. La Genesis redibuja desde el nivel y no las
  tiene. No se copia.
- **Sala vencida** (`BG2SC = 0x48`: BG2 de 32x32, que el SNES repite): lo que Kraid escribe en esa pantalla va a las
  cuatro del plano B de 64x64 (bit 15 en la celda de la tarea; cuenta como 4 filas en la tanda del cuadro).
- **Pausa**: al volver, la Genesis rehace su VRAM y el plano B sale del mapa inicial. Los ganchos de Kraid (bosses.c)
  mandan a la cola lo que el SNES copia por DMA (`Kraid_TransferTopHalfToVram`: 7E:2000) y lo que el SNES conserva en
  su VRAM: la mitad de abajo (7E:2800) si Kraid ya es grande, y en el hundimiento las filas ya borradas
  (`Kraid_UnpauseHook_IsSinking`). `zebes_use` corre también al volver de la pausa (`game_state` 17): ahí no olvida la
  pared, el cuadro de la boca ni la cola. En los estados 12-18 los colores de los enemigos (`zobj_col`) van en negro
  (antes, los del HUD: los sprites de Kraid salían rosados sobre la pantalla todavía negra). Para salir de la pausa
  en una prueba hay que mantener Start (~40 cuadros: `timed_held_input`).
- **Rendimiento** (perfil con el núcleo de `tools/emu_harness/gpgx_mm_profile.patch`):
  - con la vida en 0, `Kraid_HurtFlash_Handling` y `Kraid_HealthBasedPaletteHandling` corren en cada cuadro;
    escritos a mano, solo escriben (y avisan, `nt_pal_dirty`) los colores que cambian: antes la CRAM entera se
    rehacía en cada cuadro;
  - la ventana dibujada (`zebes_frame`) sigue a la cámara sin el temblor (3 píxeles como mucho, entra en su margen):
    con él se volvían a dibujar columnas y filas en cada cuadro;
  - `frz_get` busca el lugar de una entrada por un índice directo (`frz_hc`, `cinematic_text_tilemap + 2880`);
  - `port_Find*` (port.py): la caché de funciones es de dos vías (las piezas de Kraid chocaban en la directa).
  El hundimiento pasó de 450 a 366 cuadros de consola (313 de lógica); la pelea grande, de 2062 a 2008 (1713): todavía
  ~17 % lenta. Lo que queda es del motor en general (`EnemyMain` con las colisiones de cada pieza, ~16 000 ciclos por
  cuadro; `zebes_obj_draw`, ~8 000; `md_post`). La regresión cambia de ritmo en la huida de Ceres (7994, 8125: el
  temblor cae un cuadro antes): agregados a las excepciones de ritmo.

### Kraid: colores de la sala (2026-11-11)

Con Kraid vivo la sala, la pared y el cuerpo piden 45 colores para las dos líneas de la Genesis (30). La fusión general
(de a pares, el error por los píxeles) juntaba colores lejanos que se ven poco: los azules de los bloques terminaban
en los rosados de la puerta, y en cadena (un azul a un gris, el gris a otro...) todo iba al verde más visto. En la sala
de Kraid (`convert(own=...)`, A5B1) ahora:
- **Dos grupos de líneas del SNES**, uno por línea de la Genesis: el cuerpo con la puerta (`KRAID_BODY_LINES` = 7, 1;
  la puerta comparte negro y blanco con el cuerpo) y la sala con la pared (0, 2-6). Cada tile va a la línea de su
  grupo.
- **k-medoides por grupo**: los 15 colores que representan a los demás (error = raíz de los píxeles x distancia al
  cuadrado), cada color al más cercano, y el grupo queda con su color de más peso. Sin cadenas.
- **El índice del SNES de cada lugar** (de donde la Genesis toma el color en vivo): uno de la sala con ese color exacto;
  si solo la pared o el cuerpo lo tienen y la sala también usa el lugar, el más usado (en la primera fase la paleta 6
  está en negro: las sombras de los bloques azules no pueden salir de ella).
- Resultado: la puerta gris rosada, los bloques azules con sus brillos celestes, las púas rojas y el cuerpo con sus
  colores, como el SNES. Lo que se cede: el gris de la pared (que casi no se ve) toma el azul de las sombras de los
  bloques. Probadas las otras dos combinaciones: con la pared en el grupo del cuerpo (7, 6) los brillos celestes de los
  bloques vuelven al rosado; con la pared y la puerta (7, 1, 6) la puerta sale verde.
- `MM_ONLY=sala` (una sola sala, sin escribir nada) y `MM_COLDBG=1` (colores, pesos, fusiones e índice de cada lugar)
  para depurar.

Queda: los sprites de la sala (polvo y rocas) con otro tono (la línea de los objetos: 41 colores en 15), y el
rendimiento de la pelea grande (60 cuadros por segundo).

### Kraid: rendimiento (2026-11-11, en curso)

El SNES no pierde ningún cuadro en toda la pelea (`nmi_frame_counter_word` contra `nmi_frame_counter_including_lag`).
Medición en la Genesis: `fightprof.sh` (perfil con `gpgx_mm_profile.patch` entre AE23 y C3F9, los mismos cuadros de
lógica en cada versión). Hecho:
- **La boca ya convertida**: los 4 cuadros ($A7:97C8-A0C8, 1408 palabras) van a la ROM como celdas de la Genesis
  (`ZebesWall.mouth_cells`, mkzebes): `kq_frame` las copia sin buscar cada palabra en la tabla.
- **Cajas de golpe y dibujo extendido** (`EnemyMain`): la base del banco se toma una vez por función (`BankBase`,
  `RW`), no en cada palabra.
- **`CallEnemyAi`**: caché de dos vías (las ~24 funciones de las piezas de Kraid chocaban); los despachadores
  `Call*(uint32 ea)` de port.py van sobre 16 bits cuando todos los casos son del mismo banco.
- **`KraidsFingernail_Fire`** a mano (la tabla del contorno sin recalcular las piezas en cada vuelta).
- **`zebes_obj_colors`** sin la llamada por color; **`map_page`** no escribe el registro si la página ya está;
  **`frz_get`** busca lugar en una pasada por bloques.
Resultado: la pelea grande (de AE23 a C3F9, 1712 cuadros de lógica) pasó de 2011 a 1886 cuadros de consola (~10 %
lenta); en la regresión ya no se pierden cuadros (antes 1-3).

**Segunda vuelta (optimización de fondo, sirve para todo el juego):**
- `md_putrecs_t` (asm): el tile del depósito se suma al escribir cada sprite; `md_putrecs`/`md_putrecs_t` guardan solo
  los registros del camino común.
- `zebes_obj_draw`: camino rápido corto (acierto de `zcache` y lugar ya puesto; sin multiplicar), el resto aparte
  (`zebes_obj_draw_slow`); sin cambiar de página si no hace falta.
- Proyectiles por cuadro: `ep_cache` los recuerda (bit 31) y `ep_lookup` los dibuja desde la página del depósito,
  sin copiar registros.
- `EprojBlockCollisition_Horiz/Vertical`: camino rápido cuando ningún bloque del borde reacciona.
- xlate.py: las palabras sobre arreglos de bytes que el port tiene (vars.h) se leen directo (`nt_u8w`), no por la
  dirección del SNES (`Kraid_CheckIfDead` en cada cuadro de la sala vencida: ~4 600 ciclos).
- Las paletas de Kraid con un puntero; `HandleSpriteObjects`/`DrawSpriteObjects` recorren con punteros.
Mediciones (cuadros de consola para los mismos de lógica): salida de Kraid (C902 a AEA4, 367) 686 -> 623; pelea
grande (AE23 a C3F9, 1712) 1886 -> 1856 (al empezar el rendimiento: 2062). La comparación del PC con el SNES en la
sala de Kraid sigue igual. Una sala tranquila (Kraid vencido) usa ~95 000 ciclos por cuadro.

**Probado y descartado: los bancos con los bytes de cada par intercambiados** (para que una palabra en dirección par
sea una lectura nativa del 68000). Las direcciones impares resultan muy comunes (las tablas del SNES caen en
cualquier byte): fuera de línea (`bankb_w_odd`) costaban ~7 000 ciclos por cuadro y la pelea quedó peor (1917 cuadros
contra 1856); en línea, el código no entra en la ROM fija (0x300000, ya casi llena: cualquier optimización que agrande
el código fijo necesita antes mover algo a las páginas).

Lo que falta: **la salida de Kraid del piso** (estados C902-C924) y su transformación (AC4D) van a la mitad de
velocidad: ~254 000 ciclos por cuadro de lógica (el presupuesto es ~127 000) con 10-11 rocas y el polvo, las mismas que
en el SNES. Unos 85 000 son el dibujo de objetos por cuadro (`zebes_obj_draw` ~80 veces por cuadro con ~300 ciclos de
sobrecosto cada una, `zebes_obj_ep_recs` copia los registros de cada roca en cada cuadro, `md_putrecs`); el resto es
la lógica traducida (`EnemyMain` ~25 000, colisiones de las rocas con el nivel ~33 000). Llevarlo a 60 cuadros por
segundo pide optimizar a fondo el dibujo de objetos y los proyectiles.

**Colisión de proyectiles con bloques en ensamblador.** `EprojBlockCollisition_Horiz/Vertical` (con `eproj_quiet`)
pasan a ensamblador en la Genesis (el C de siempre queda en el PC como referencia): sin el guardado de once
registros del C, con lecturas de byte en vez de desplazamientos de 8 y `lvl_mod` en un registro (en la sala de Kraid
hay bloques cambiados y `lvl_fast` es 0, así que cada bloque mira su bit). Cualquier caso que no sea "ningún bloque
reacciona" salta al original (`_full`). Subida de Kraid: 572 → 560 cuadros de consola (las dos funciones y el bucle,
~22k → ~17,4k ciclos por cuadro de lógica, ~9 llamadas de cada una por cuadro). Regresión: 0 distintos.

**zebes_obj_draw en ensamblador.** El camino común (clave en `zcache`, congelado por cuadro con su lugar puesto) pasa
a ensamblador en la Genesis, con `_Static_assert` sobre la disposición de `zcache` y `ZebesObj.recs`; lo demás salta a
`zebes_obj_draw_slow` con los mismos argumentos. Sin el guardado de once registros y la copia de los cinco
argumentos: ~13k → ~9,5k ciclos por cuadro de lógica (unas 10 llamadas por cuadro). Subida: 560 → 548 cuadros;
pelea grande: 1844 → 1832. La imagen de cada cuadro del motor es la misma que antes (hash por `md_shown_frame`,
script imgcmp.sh); solo cambian los cuadros de retraso, que caen en otros momentos.

`zfrz_put` (md_zebes.c, ensamblador): el dibujo de un congelado con su lugar del cuadro ya puesto, con convención de
registros, común a `zebes_obj_draw` y a `md_draw_eprojs_asm` (que ya no pasa por el C de `zebes_ep_draw` en lo común).
Neutro en ciclos (~16k por cuadro entre los dos, ~18 llamadas de ~600 ciclos: la mitad son el cambio de página al
depósito y la vuelta y el guardado), pero deja un solo camino. Lo que queda caro aquí es el cambio de página por
llamada: los datos del depósito (kZebesFrz, página 18) no caben en la parte fija.

**El destello verde del HUD (arreglado).** En algunos cuadros de la pelea el fondo y el texto del HUD salían un cuadro
con los colores de los enemigos (el verde de Kraid). Medido con `MD_HUDLOG` (líneas de la entrada al vblank, de la
reposición de los colores del HUD y del final de `md_vblank`): el vblank llega a tiempo y repone los colores en la
línea 226, pero en esos cuadros `md_vblank` termina en las líneas 0-8, ya con el HUD en pantalla; al volver a activar
la interrupción horizontal (la había apagado al empezar), salta en ese momento y no en la línea 31, y el cambio a los
colores de los enemigos cae sobre el HUD. Los manejadores de la línea 31 (`md_hint_obj`, `md_hint_msg`,
`md_hint_lq`) no hacen nada antes de la línea 30: la interrupción buena de la línea 31 llega igual. En la pelea, la
fila del HUD sale igual en todos los cuadros; la regresión, 0 distintos.

## Kraid: rendimiento, dónde quedó y qué falta (2026-11-12)

Subida de Kraid (C902 → AEA4, 367 cuadros de lógica): 686 → 496 cuadros de consola (74 % de la velocidad). Pelea
grande (AE23 → C3F9, 1712 de lógica): 2062 → 1807 (95 %). Lo hecho, en orden: ROM fija (bancos fríos a la página
7), lecturas directas, cachés de dos vías, colisiones de proyectiles en ensamblador, `zebes_obj_draw`/`zfrz_put` en
ensamblador, plan de RAM (docs/60) y con esa RAM: registros de los congelados en la zona del jefe, IA por lugar de
enemigo, caché de tramos de bloques, `EprojRunAll` en ensamblador con las rocas directas, recorridos de enemigos y
objetos de sprite hasta el lugar usado, `md_draw_eprojs_asm` con un índice, `md_putrecs_ns`.

Pendiente (lo que todavía se puede hacer):
- `EnemyMain` (~16k ciclos por cuadro): unos 760 ciclos de pegamento por enemigo y ~2k de `gEnemyData` recalculado
  en cada función en línea. Reescribir el bucle de enemigos en ensamblador (con `E` en un registro).
- `zebes_obj_draw` + `DrawOneEnemy`/`DrawEnemySpritemap` (~5k + el C de quien llama): el dibujo de las piezas de
  Kraid por un camino en ensamblador desde `DrawOneEnemy`.
- `md_putrecs_ns` (~9k): trabajo real (unos 40 sprites por cuadro); poco margen.
- Base de todas las salas: `md_post` (~6-12k; la comparación del HUD, 1,4k), `md_vblank` (~5k).
- Herramientas: perfil `segprof.sh` (estados de Kraid), `flines.py` (por línea), imagen cuadro a cuadro (`imgcmp.sh`,
  con `REF=` un commit), regresión y las 194 salas con `check`.

## Bomb Torizo (2026-11-12)

Comparado lado a lado con el SNES (`tools/compare/lado_a_lado.sh`, que por omisión es esta pelea) y la lógica cuadro a
cuadro (estado completo de la Genesis con `MD_STOP_AT` contra la grabación de `hlref`: `work/lado/stop.sh`,
`cmpstop.py`):
- **La lógica se separaba del SNES en el cuadro 16582**: en el SNES el Torizo golpea a Samus por contacto y en la
  Genesis no. Su función de contacto es `GoldTorizo_Touch` (la cabecera del Bomb Torizo apunta a ella), código de jefe
  (`NT_BOSS`, página 25), y la sala 9804 no estaba en `BOSS_ROOMS` (mkdata.py): con la ventana 7 en otra página el
  despachador no la llama ("código de jefe fuera de su página"). En el PC no pasa (no hay páginas): `check` daba 0
  diferencias. Arreglo: 9804 en `BOSS_ROOMS` (su nivel va a la página 25). Ahora la lógica es igual en toda la pelea.
  Recorrido estático de todas las salas que no son de jefe con la cabecera de sus enemigos: ninguna otra llama código
  de jefe.
- **Los ojos**: índices 156-158 de palette_buffer (rosados en la estatua; naranjas al despertar; cian en el Golden).
  Al juntar colores se iban con un marrón: `mkzobj.BOSS_SOLO` los deja para el final. Se encienden en las poses en
  que el tile del ojo va solo; en algunas (el salto) el sprite compuesto de la cabeza sale con los índices del cuerpo
  en el ojo: pendiente. El reparto de colores está bien (`MM_ZOBJ_REP=155,156,157,158` en `mkzobj.build` muestra
  156-158 a sí mismos en la línea 1); hay que comparar el tile del ojo (456, componente $AA:9786) de la Genesis con el
  del SNES en el mismo cuadro exacto (el volcado `HL_VRAM` y la captura de la Genesis quedaron a un cuadro: el Torizo
  salta y todo sale corrido 6 píxeles).
- **El ojo, resuelto (copias de tiles)**: el ojo es una animación de 4 cuadros que las listas de instrucciones suben a
  la VRAM en el momento (`EnemyInstr_CopyToVram`, $A0:814B: $AA:B279/B2B9/B2F9/B339 a 7D80, tiles 472-473; también
  borrados desde $AA:B0A5 y otras animaciones a 7300/7400). La ROM de la estatua no trae esos píxeles y la Genesis
  ignoraba la copia. Ahora `mkzobj.vram_copy_ops` busca las copias del banco del jefe (`VRAM_COPY_BANKS`: $AA, los dos
  Torizos); las piezas que usan esos tiles tienen tiles propios en la Genesis (no se comparten por contenido y no
  entran en los sprites compuestos: van sueltas, encima) y para cada copia se guardan esos tiles reconvertidos con el
  mismo reparto de colores (`kZO*_vcp`, ~3 KB por sala). El motor avisa (`nt_enemy_vram_copy`) y `md_zebes.c` sube
  los tiles en el vblank. Para que entrara, `mkzebes.py` usa también lo que dejan libre los niveles en su última
  página (la 10, `NT_LEVEL_LAST_USED` de mkdata.py): 140 KB libres en total; el ensamblador de `zebes_obj_draw` toma
  `ZEBES_FRZ_PAGE` del encabezado generado (antes fijo en 18).
- **La estatua "corrida"**: no es un fallo. La estatua es parte del nivel (bloques 12-14 de las filas 10-12, x
  192-240) y la Genesis la dibuja ahí. En la captura del SNES de la comparación sale 61 píxeles a la izquierda porque
  el salto de prueba de `hlref` (HL_WARP) deja el BG1 desalineado (`bg1_x_offset` = 0xB3B con el mapa escrito con otro
  desplazamiento); en el juego de verdad está a la derecha.
- **Los aros marrones** al final: también están en el SNES (restos de la pelea); allí se ven más tenues por la
  neblina (`SpawnBombTorizoHaze`).
- **La neblina, por franjas**: son dos objetos de HDMA al COLDATA, uno con rojo y verde ($88:DD75) y otro con azul
  ($88:DDA6): nada en las primeras 72 líneas y después crece hacia abajo (hasta 5/5/8). Antes la Genesis sumaba a
  toda la imagen el promedio de la primera tabla sola (2/2/0: amarillento, sin el azul). Ahora `lq_prepare`
  (md_engine.c) usa la interrupción horizontal del líquido: arriba la paleta normal y dos franjas (desde las líneas 72
  y 147 del SNES) con el color medio de cada una (2/2/2 y 4/4/6, `MD_TORIZO_HAZE*` de mkmd.py). Con la caja de
  mensaje (las bombas) no: sus paletas comparten la RAM con la caja; al cerrarse se rehacen.

## Golden Torizo (2026-11-12)

Lado a lado con el SNES (`tools/compare/lado_a_lado.sh` con `PAT=tools/compare/golden_in.txt DOOR=9A86 X=300 Y=370`,
`HL_POKE=9A6:100B,9A8:100B,9C2:3E7,9C4:3E7` y en la Genesis `EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_ITEMS=0
-DMD_TEST_BEAMS=0x100B -DMD_TEST_POKES={12764,0x9C2,0x3E7},{12764,0x9C4,0x3E7}"`: rayos de onda, hielo, plasma y
carga, 999 de energía; el Golden desvía los disparos sin cargar, el patrón carga y suelta) y la lógica cuadro a cuadro
(`MD_STOP_AT` contra la grabación de `hlref`) en toda la pelea, la muerte y lo que sigue:
- **`MD_TEST_GOD` desde el salto**: la Genesis hacía invencible a Samus desde el principio (también contra Ridley en
  Ceres) y `HL_GOD` de hlref solo desde el cuadro grabado: llegaban a la sala con otra posición. Ahora desde el salto.
- **Instrucciones de proyectiles sin traducir**: `port.py` solo traducía las `EprojInstr_*` debajo de $86:8300; las
  demás (22: rayo del ojo del Golden en el suelo `GotoIfFunc1`, `ResetXYpos1/2`, `SetVelTowardsSamus2`, Draygon,
  estatuas de Tourian, tubo de Maridia...) se paraban en la lista ("instrucción sin portar"). Ahora se traducen todas;
  `GotoIfFunc1` (B3B8), `sub_86B13E` (huevos), `nullsub_82` y las dos del láser de los piratas usadas como instrucción
  van a mano en `ep_instr_list` (eproj.c). Además, las 45 funciones de inicio y pre-instrucción de proyectiles que
  faltaban en los despachadores del decomp (supermisil y huevos del Golden, Mother Brain, puerta-ojo y estatuas de
  Tourian, tubo de Maridia, pelusa de Draygon 8DCA...) están en `tools/port/roots.txt`.
- `SetGoldenTorizoPalette` ($84:8000, los colores según la vida) escrita a mano (bosses.c); solo marca los colores si
  cambiaron (se llama en cada cuadro).
- `port.py`: `POST_PATCHES` (las marcas `enemy_hw` y `sprobj_note` que estaban a mano en port_gen.c: ahora el
  traductor las pone y port_gen.c sale igual al regenerarlo); el despachador de pre-instrucciones que llaman las de la
  sala del tubo es `ep_preinstr_c`. `xlate.py`: `ExtraEnemyRam8000` también en la primera línea de la función.
- Resultado: la lógica de la Genesis es igual al SNES en toda la pelea y después (0 diferencias); en el PC, `check`
  sobre la misma grabación da 0 diferencias en sus 8355 cuadros. El dibujo coincide (ataques, orbes, huevos, rayo del
  ojo, muerte, explosiones).
- **Pendiente: la velocidad**. La pelea corre al 72-76 % (unos 164 mil ciclos por cuadro del motor; el presupuesto
  es ~128 mil); sin el jefe, la sala al 91-97 %. El costo está repartido: md_post (HUD y paleta), md_putrecs_t y el
  dibujo de enemigos (los estados del Golden no entran juntando piezas: 1655 sprites sueltos), HdmaObjectHandler
  (las olas de la lava, `B88W` en un bucle de 16), las estelas de los rayos, PaletteFxHandler, los PLM. Ya hecho:
  `zebes_obj_draw` en ensamblador también fuera de las salas por cuadro (9.7k -> 5.6k ciclos). Como Kraid: los objetos
  por cuadro (`STREAM_ROOMS`) permitirían juntar piezas, pero chocan con las copias de tiles del ojo (`vram_copy_ops`).

## Phantoon (2026-11-12)

Lado a lado (`PAT=tools/compare/phantoon_in.txt DOOR=A2AC X=80 Y=150`, las mismas variables de rayos y energía que el
Golden Torizo) y la lógica cuadro a cuadro en toda la pelea y la muerte:
- **Lógica**: en la Genesis las bolas de fuego del principio salían en otro lugar: `Eproj_PhantomFireballs_Func2` lee
  bytes sueltos de una tabla de palabras (`(uint8 *)&kSinCosTable8bit_Sext[64] + k`); en el 68000 el orden de los
  bytes es el otro. `port.py` (`POST_PATCHES`) la pasa a la palabra k / 2 (byte bajo y alto). Ahora 0 diferencias en
  la Genesis y en el PC.
- **El fondo rojo**: el relleno oscuro de la sala salía de un índice de la paleta 7 del SNES (la del cuerpo, que se pone
  roja al enojarse) porque los dos eran negros al convertir. `mkzebes.convert`: los colores de las paletas del cuerpo
  del jefe en el BG2 que la sala no usa son `LiveCol` (distintos de un color igual de la sala) y no se juntan con los
  de la sala; así el cuerpo se desvanece y cambia de color como en el SNES, y la sala no.
- **Los desvanecidos**: `Phantoon_Func_8/9` escriben `palette_buffer[...] =` en varias líneas y el aviso a la Genesis
  (`nt_pal_dirty`, port.py) solo veía las asignaciones de una línea; ahora también esas y `+=`, `-=`, `--`, `++` (la
  paleta de Kraid tenía dos así).
- **Invisible**: con `LayerBlendFunc_4` el SNES saca el BG2 de las dos pantallas (TM/TS sin el bit 2): md_zebes.c
  (`bb_hide`) vacía el plano B y al volver sube el cuerpo otra vez. Con el BG2 en la subpantalla sumado (0x1A) y la
  paleta en negro tampoco se ve: lo da la paleta (ya sigue al SNES).
- **La onda** (HDMA $88:E4A8, indirecto al BG2HOFS desde la tabla de 7E:9000): md_engine.c (`phantoon_wave`) arma el
  scroll por línea del plano B (como el cielo).
- **El ojo al morir**: 8 palabras del BG2 que la prueba anterior no vio (`zbg2.log`) y las claves de sus partes
  (`zdraw.log`), registradas en el PC con `MM_DRAWLOG` sobre esta pelea. Ojo: no sumar las líneas `bg2pal` de la
  muerte (el cuerpo está rojo: sus colores quedan iguales y la conversión los junta en una silueta).
- Queda: el color de la compuerta de la puerta (el pendiente general de las puertas).

## Draygon (2026-11-12)

Lado a lado (`PAT=tools/compare/draygon_in.txt DOOR=A924 X=100 Y=400`, las variables de rayos y energía del Golden
Torizo) y la lógica cuadro a cuadro: igual al SNES en la Genesis y en el PC (13 mil cuadros, con Draygon agarrando a
Samus varias veces).
- **Samus agarrada**: `Samus_SetGrabbedByDraygonPose`, `Samus_ReleaseFromDraygon(_)`,
  `Samus_GrabbedByDraygonFrameHandler` (frame_handler_gamma) y `Samus_InputHandler_E913/E91D` escritas a mano
  (samus_codes.c, $90:E23B-E91D); antes eran avisos vacíos.
- **Objetos por cuadro**: con lo que se dibuja en la pelea (claves de `zdraw.log` y palabras de `zbg2.log` registradas en
  el PC con `MM_DRAWLOG`) los objetos del estado de pelea ya no entraban ni sueltos (503 tiles de 456). DA60 va en
  `STREAM_ROOMS` (como Kraid): el límite de tiles para base 0 es el de 11 bits (2047) y las tablas del depósito van en
  `cinematic_text_tilemap + 1024` en las salas sin la cola de Kraid (+ 3200 es del agua). Las claves `enemyext` no se
  suman: en Zebes los spritemaps extendidos se dibujan pieza por pieza (con las `enemy`).
- **Las crías**: `Draygon_Func_1` ($A5:871B) sube por la cola de VRAM sus tiles ($B1:9400, 0x600 bytes a 0xDA00):
  `mkzobj.BOSS_VRAM_COPIES`.
- **Las burbujas**: la paleta 2 de sprites la pone `Draygon_Init` (25 colores desde $A5:A217, 144-168);
  `mkzobj.init_palettes` tomaba siempre 16: ahora lee la cuenta del bucle. Salen verdosas (sus grises se juntan con los
  verdes de Draygon: 52 colores en 15 lugares); a Edwin le gustaron así.

## Botwoon (2026-11-12)

Lado a lado (`PAT=tools/compare/botwoon_in.txt DOOR=A774 X=200 Y=150`, mismas variables que el Golden Torizo) y la
lógica cuadro a cuadro: igual al SNES en la Genesis y en el PC en toda la pelea y la muerte.
- **La historia del cuerpo** (7E:9000 + índice): `nt_ram_set_w` (escritura por dirección del SNES) no llegaba a la RAM
  extra de los enemigos (7E:7800-97FF); ahora sí, y `nt_ram_b` la lee igual (palabras y bytes).
- **Disparos contra el cuerpo**: `HandleEprojCollWithProj` ($A0:99F9) lee la posición del disparo con el índice del
  proyectil enemigo (un desborde del SNES: con los índices altos del cuerpo lee la RAM que sigue al arreglo de 10);
  ahora por la dirección del SNES (`nt_ram_w(0x0B64 + k)`), como el SNES.
- Claves de dibujo de la pelea en `zdraw.log` (87, registradas con `MM_DRAWLOG`).
- `mkzebes.py`: `main()` solo al correrlo (importarlo regeneraba todo, y sin `MM_MAX_PALS=2` las salas salían con tres
  líneas de color: las tuberías con los colores de Samus).
- **Pendiente: el agua de esta sala.** El tinte del agua de la Genesis supone los colores 26 y 25 del BG3 (74 % y 26 %),
  que valen en otras salas de Maridia; en la de Botwoon el BG3 es el color 27 (84 %), negro: el SNES no resta casi nada
  y la Genesis tiñe de rojo. Hace falta el reparto de colores del BG3 por sala (como `kFogW`).
- **Pendiente (menor)**: al morir, `CrumbleBotwoonWall` ($84:B79B) deja la columna 15 en aire sin redibujar el BG1 y el
  SNES sigue mostrando la tubería; la Genesis dibuja desde el nivel y la borra.

## Pendientes de Samus (anotado a pedido de Edwin, 2026-11-12)

El rayo gancho está hecho y probado (docs/56, `tools/rooms/grapple.sh`: engancharse, columpio, soltarse, enemigos y el
salto en la pared con el gancho). Faltan, de docs/56: **los rayos X** (sus colores están; el movimiento y el cono no:
`Samus_HandleTransitionsB` 5, `SamusMovementType_Xray`), **el movimiento del destello de cristal** (sus colores están)
y **el hiperrayo** (Mother Brain).
