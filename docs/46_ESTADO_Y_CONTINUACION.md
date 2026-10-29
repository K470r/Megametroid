# 46 — Estado del proyecto y cómo continuar (resumen para empezar otro chat)

Fecha: 2026-10-21. ROM actual: `MegaMetroid_crateria13.bin` (ver md5 en la entrega).
Leer esto primero; para el detalle de cada parte, el documento que se nombra.

## Qué es

Port de Super Metroid (SNES) a la Sega Genesis / Mega Drive. Destinos: MiSTer, Mega EverDrive X3 y una placa
fabricable, mapeador SSF2 (ROM de ~10 MB, páginas en la ventana 0x380000).

Reglas de Edwin (no negociables):
- Todo el código del juego se escribe para el 68000 desde el decomp (snesrev/sm, "la biblia"), con sus nombres y la
  dirección del SNES en un comentario. Nada se interpreta ni se convierte en la Genesis: todo se prepara en el PC.
- Jugar exactamente como el SNES, a 60 cuadros sin bajones. Intro y cinemáticas pueden variar en tiempos.
- No se distribuye nada de la ROM de Nintendo (ni ROM, ni extraído, ni ROM compiladas). Ver docs/43 y docs/45.
- Juego en inglés con tablas por idioma; español después. Título: "MEGA METROID", "Síndrome" en lugar de Nintendo.
- Efectos al estilo de Thunder Force IV sin bajones. Dejar lugar para música y enemigos.

## Estado (lo que anda)

- Encendido, título, menú, intro completa con voces (docs/29, 36, 38).
- Ceres entero: salas, Ridley, huida con alarma y tiempo, cinemática de la explosión, viaje a Zebes (docs/32-35).
- Zebes: aterrizaje, **todas las salas de Crateria con sus enemigos** y el camino de Brinstar hasta la bola y los
  primeros misiles (7 salas), ascensor, lluvia, paralaje, FX. Visor de salas (START + izquierda/derecha).
- Bola morfo: se toma (ícono en su pedestal, docs/45 → `mkzebes.py room_items`), se usa: rodar en el suelo y caer
  (`Samus_Movement_04/08`, iguales al SNES: 0 diferencias en `check`).
- Ojos (MorphBallEye) y su haz de luz (crateria10, `src/platform/md_cone.c`): la forma es la tabla por línea del SNES
  (`CalculateXrayHdmaTableInner`), línea por línea (tiles de los bordes armados en cada cuadro, llenos fijos en el
  medio), con el modo sombra/luz del VDP (color 14 de la línea 3: aclarar). Para que el modo no oscurezca la sala, en
  los estados de sala con el ojo todas las celdas tienen prioridad alta (`mkzebes.py`, campo `eye` de `ZebesGfx`), los
  sprites se pasan a prioridad alta y dos sprites al principio de la lista enmascaran las líneas del HUD. Los colores
  14 y 15 de la línea 3 no los usan los enemigos en esas salas. Diferencias con el SNES: aclara hacia el blanco (el
  SNES suma amarillo), no aclara a los sprites y no late (al apagarse, un cuadro sí y otro no).
  Velocidad (crateria12): el haz ya no le quita cuadros al juego. El haz nuevo (la tabla del SNES y sus sprites)
  se arma **de a pasos cortos** en el otro búfer mientras se muestra el anterior (corrido con la cámara), y se cambia
  cuando está: `md_cone_idle` (el tiempo que la consola espera al vblank, `md_wait_free` y `pipe_wait`) y un paso
  corto por cuadro en `md_cone_frame` (uno cada dos si el cuadro no tuvo tiempo libre). La tabla se escribe por
  partes con las mismas cuentas: con `xjob_rec` las funciones de relleno de `hdmaobj.c` anotan qué escribir y
  `xray_job_run` lo escribe (rampas en ensamblador, `xray_ramp_fast`; prueba `MD_TABCHK`: igual a la de una vez).
  `cone_build` se puede cortar a mitad de banda (tope de tiles por paso), usa los tiles llenos fijos en las columnas
  llenas en las 8 líneas y saca la fila de cada tile de una tabla (`kBuildIdx`); si se acaban los 32 tiles armados,
  el resto del haz va con columnas llenas (borde de a 8 píxeles) en vez de cortarse. Los sprites del haz van en los
  últimos lugares de la tabla de sprites del VDP (`md_cone_sat`, enlazados desde el último del juego) y si no cambió
  la forma solo se suman x e y. Cuadros del motor en 300: 300 quieta, caminando y dando vueltas bajo el haz (antes
  294); barridos en las 3 salas: 72 de 72. Lo que se paga: el haz sigue a Samus con unos cuadros de demora (cada
  ~8-10 cuadros uno nuevo; más en la sala 9A90, con el haz casi horizontal: cada uno cuesta ~250 líneas).
- **Ojo y haz arreglados** (eye-beam, docs/52): faltaban las claves de 12 de los 14 cuadros del ojo (lista calculada
  de `MorphBallEye_Func_3`, ahora en `mkzkeys.py`): por eso desaparecía parte del ojo. Los bordes del haz ya no se
  vuelven escalones de 8 px: los tiles armados van en dos bancos (el que se muestra y el que se arma) sobre tres tramos
  de VRAM (libres después de los objetos, `0xFC00`, filas ocultas del Window) y se suben de a poco en cada vblank.
- **Enemigos con spritemaps extendidos** (crateria11: los piratas de la sala del pozo y otras): `mkzkeys.py` ahora
  expande las partes (`extra_properties & 4` de la población, como `WriteEnemyOams`). Eran los invisibles que hacían
  daño al volver de Brinstar con la bola y los misiles.
- **Misiles** (crateria8): proyectil, explosión y estelas, con los colores de cada sala. Las claves salen de la ROM
  (`mkmd.zebes_proj_keys`: listas de `$93:83F1`, explosión de `KillProjectileInner` en `$93:867F`, estelas de
  `$90:B5BB/B609`) y se convierten en cada estado de sala de Zebes junto con los enemigos (`mkzobj.py`, claves
  `(sm, 0, 0xC000)` y `(tile, 0, 0xC001)`); pesan poco al juntar colores (ceden ellos, no los enemigos).
- **Bombas** (crateria9): poner bombas en la bola (`HudSelectionHandler_MorphBall`, `InitializeInstrForMissile`,
  `ProjPreInstr_Bomb`, `Bomb_Func2`), su explosión (`InitializeBombExplosion`, `$93:8683`), bloques que rompen
  (`BombOrPowerBomb_Func1`, `BlockColl_BombExplosion`, `BlockBombedReact`), daño a enemigos (`EnemyBombCollHandler`
  y `_Multibox`), salto con bomba (`SamusProjectileInteractionHandler` → `SetupBombJump` → `Samus_HandleTransitionsB_3`
  → `Samus_MoveHandler_BombJumpStart/Main`) y el rebote de la bola al caer (`Samus_HandleTransFromBlockColl_1_1..3`,
  `Samus_MorphBallBounce*`). `check` contra el SNES: 0 diferencias (700 cuadros con bombas, saltos y bloques rotos en
  el aterrizaje; 3600 cuadros al azar en 4 salas). Faltan: bombas de poder (las bombas en abanico: crateria14).
- **Claves de enemigos de todas las salas de Crateria y Brinstar** desde la ROM (`mkzkeys.py` sobre todas las salas
  alcanzables por puertas, áreas 0 y 1; antes solo las jugadas): 31 salas no tenían ninguna (enemigos invisibles).
  Norfair, Maridia y demás: cuando se llegue (inflan la ROM).
- **Ojos** (MorphBallEye): las salas salen de la población de cada estado en la ROM (`mkzobj.has_eye`, init
  `$A8:9058`): 9A90, 9E9F y 9F64.
- **crateria13** (2026-10-21):
  - **Supermisiles**: `ProjPreInstr_SuperMissile` y sus `Func0459/1368/27`, `ProjPreInstr_Func1`, `Missile_Func2`
    (el acompañante), `InitializeInstrForSuperMissile` ($93:842B), `SuperMissileBlockCollDetect_X/Y`,
    `ProjectileReflection`; la explosión y el temblor ya estaban (`KillProjectileInner`). Claves en
    `mkmd.zebes_proj_keys` (índice 2 y `$93:8693`). HUD: misil / supermisil / nada resaltados como en el SNES
    (`kHudTilemaps_Missiles` entero en las celdas del HUD). `check`: 0 diferencias disparando supermisiles.
  - **Cajas de mensaje** (banco $85, `src/core/msgbox.c`): corrutinas como en el decomp (`coroutine_state_3/4`;
    `GameState_8_MainGameplay` y `PlmHandler` ahora se reanudan, `coroutine_state_1/2`). La Genesis las dibuja con
    sprites (`md_msgbox_post`, tiles `kMsgTiles` de `mkmd.py` en 0xF000 y en las filas libres de la ventana) y en
    sus líneas la línea 3 vuelve a los colores del HUD (interrupción `md_hint_msg`); los colores 25/26 de la CGRAM
    van en `MB_SLOT25/26`. Se abre desde el medio como el HDMA del SNES. Los cuadros que el SNES espera sin NMI
    (`WaitForNMI_NoUpdate_Async`) repiten la lista de sprites anterior (`nt_frame_noupdate`).
  - **Bomb Torizo**: la pelea entera contra el SNES con 0 diferencias en 10000 cuadros (despertar, ataques, orbes,
    caja de mensaje de las bombas, muerte). Portado: `EprojPreInstr_A977`, `sub_86A887`, `EprojInstr_SpawnEnemyDrops`,
    `HandleEprojCollWithProj`, `sub_82DAF7/DB0C/DB41` (door.c), instrucciones comunes de cada banco
    (`Enemy_SetAiPreInstr_*`... en roots.txt) y `CallHitboxTouch/Shot` dentro de la tabla de `CallEnemyAi` (port.py).
    Gráficos: `mkzkeys.py` reconoce los spritemaps extendidos que pone el código (`extra_properties |= 4` en
    `Torizo_Init`), toma toda la tira de tablas ($AA:A4F0-B096) y la paleta 0 (`palette_index = 0`). Los objetos de
    una sala pueden empezar justo después de sus tiles (`ZebesObj.base`) si no entran desde 880.
  - **Matemática de color** (`md_engine.c`, `tint15`): la Genesis no suma capas; se suma/resta un color a todos los
    de la sala (no al HUD): niebla FX 0xC (96BA y 975C antes de tener los misiles, A5ED: el color medio de su BG3,
    `kFogW` de `mkmd.py` desde `kFxTypeTilemapPtrs`), neblina del Bomb Torizo (COLDATA medio de `$88:DD74`) y las
    restas de `LayerBlendFunc_28/2A` (97B5). Las nubes en sí no se dibujan.
  - Spore Spawn (9DC7): sus instrucciones ($A5:E7xx, `Draygon_Instr_*` en el decomp) traducidas.
  - Mando (`md_main.c`): botones en el lugar de los del SNES (6 botones: X cancelar, Y disparar, Z apuntar arriba,
    A correr, B saltar, C apuntar abajo, MODE elegir ítem; 3 botones: A correr, B disparar, C saltar, START corto =
    elegir ítem, START mantenido 20 cuadros = pausa).
  - ROM: los bancos del SNES en la parte fija van como bytes (`kBankXXB`; antes 84/87/88/8F/A6 como palabras: 192 KB
    menos); `mkzebes` reparte las páginas con primer hueco decreciente y usa lo libre de las de la intro.
  - RAM: 7E:3000-3FFF es un solo arreglo (`cinematic_text_tilemap[2048]`; `menu_tilemap` y `cinematic_bg_tilemap`
    son alias, `alias` en `vars_extra.txt`): 2 KB menos. `md_ssf2.ld` falla si `_bend` pasa de 0xE0FFE3F0 (con
    menos memoria para SGDK la consola no arranca).
- **crateria14** (2026-10-22, rama `charge-beam`, docs/49): **rayo cargado** (`HudSelectionHandler_NothingOrPowerBombs`
  entera, `FireChargedBeam`, `FireSba` sin los rayos que faltan), **destello de la carga** (`HandleChargingBeamGfxAudio`,
  `DrawFlareAnimationComponent`) y **bombas en abanico** (`BombSpread`, `ProjPreInstr_SpreadBomb`, `BlockCollSpreadBomb`).
  Claves del rayo cargado y del destello en `mkmd.zebes_proj_keys`; la VRAM de `mkzobj` lleva los tiles y la paleta
  del rayo. `check` en la sala del Charge Beam (`tools/rooms/chargebeam.sh`): 0 diferencias en 2126 cuadros.
  Regresión: 62 cuadros "distintos" en Ceres (antes 19), todos corridos en el tiempo como los de docs/48 (docs/49).
- **spore-spawn** (2026-10-22, docs/50): Spore Spawn giraba a toda velocidad por un error del traductor (`jp[n]` leía
  `jp + 4n`; también afectaba a `Draygon_Instr_1/14`, `Torizo_Instr_25/26/45` y `SpacePirates_Instr_15`). Portado lo
  que faltaba para la pelea (`EprojInstr_DC5A`, `EprojInstr_SpawnEnemyDrops_0`, `CreateADudShot`; `kraid_unk9000` =
  `ERAM(0x9000)`) y el cuelgue al morir en la Genesis (palabra en dirección impar en `boss_bits_for_area`). Pelea
  entera contra el SNES con `tools/rooms/spore_fight.py`: 0 diferencias hasta la muerte.
- **crateria14** (2026-10-27, docs/51): el **Bomb Torizo se dibuja como en el SNES** (cuerpo entero, colores de
  estatua, despertar, destellos, ojo, trozos de la estatua, orbes). Jefes con colores propios (`mkzobj.BOSS_PALETTES`):
  sus paletas salen de las funciones del decomp que las escriben; en sus estados la sala va en una línea y la línea 1 es
  de los objetos (`ZebesObj.src` de 30). `port.py` marca `nt_pal_dirty` cuando el código escribe `palette_buffer`.
  `mkzkeys.py`: proyectiles de PLMs (`plm_walk`), tablas de listas, pre-instrucciones puestas por lista. Comparación
  lado a lado con el SNES cuadro a cuadro: `tools/compare/lado_a_lado.sh` (ROM de prueba con `MD_TEST_WARP_AT`).
- **Estación de mapa, pausa, mapa y equipo** (map-station, 2026-10-22; docs/53): el cuelgue al usar la estación de
  mapa era la pausa sin portar (la caja "MAP DATA ACCESS COMPLETED" abre la pausa: `game_state = 12`) y
  `CallSomeSamusCode(6)`. Portados los estados 12-18 del banco $82 (`src/core/pause.c`): mapa con scroll, íconos y
  destinos, pantalla de equipo (equipar, reserva), L/R/START; `check`: 0 diferencias. Genesis: `md_pause.c` y
  `tools/conv/mkpause.py` (tiles, celdas, colores y sprites de la pausa desde la ROM; tiles en la página 18). El
  minimapa del HUD muestra el mapa cargado. START ya abre la pausa en cualquier sala (también en el visor: START solo).
  Regresión: intro 0; Ceres/Zebes 97 (antes 19): las mismas clases de cuadros (pozo que gira, círculo de la
  explosión), sensibles al ciclo: la base con +40 ciclos por cuadro en `md_post` ya da 76. Falta aprobarla a ojo.
- **Caras de Brinstar (9F11, 9F64, 9E9F, 9A44) con su brillo** (2026-10-29): con la bola, `BlueBrinstarFaceBlock_Func_1`
  ($A8:E86E, gancho de dibujo) cicla `palette_buffer[línea + 9..12]` en rojo ($A8:E7CC). La Genesis ya lo hacía, pero
  `mkzobj.py` armaba los colores con la paleta fija y juntaba esos índices con otros de igual color (en 9F11 la línea 7
  es de los Roach D87F y de las caras EA7F: la lista de tiles carga las dos paletas ahí y gana la de las caras). Ahora
  los ciclos de color van como momentos (`HOOK_PALETTES`, `hook_moments`), la fusión pesa con la raíz de los píxeles y
  conserva el índice que cicla. Además las claves de enemigos de otro estado de la misma sala ya no ocupan colores
  (`tileset_pals`: paletas que son líneas de otro estado y no de este). Quedan 2 de los 4 tonos del brillo (10 lugares).
- Pruebas: regresión de imagen (intro, Ceres, Zebes: 0 diferencias), comparación cuadro a cuadro con el SNES en el
  PC (`host/check`), 41 salas con Samus invencible (docs/41, 44).

## Lo que falta (en el orden acordado)

0. (Hecho en crateria10) **Ojo (haz de luz)**. Análisis del SNES (decomp): el HDMA `$88:E8EC` escribe la ventana 1 por línea
   (`CalculateXrayHdmaTableInner`); `LayerBlendFunc_10` quita el BG3 (tile negro opaco `0x184E`) de la subpantalla
   dentro de la ventana y ahí se **suma un color fijo** (R=G=16..8, B=0, late en 16 cuadros: `$88:EA8B`) a BG1, BG2,
   sprites y fondo (CGADSUB 0x33). Se abre con aceleración (`HdmaobjPreInstr_E9E6`) y se apaga bajando el color
   (`EACB`). Propuesta Genesis: modo sombra/luz con sprites "luz" (color 14 de la línea 3) en la forma de la tabla;
   mientras está encendido, prioridad alta en el plano del fondo y en los sprites (si no, todo queda en sombra).
1. (Hecho en crateria13) Caja de mensaje, pelea con Bomb Torizo, niebla/neblina como tinte. Falta: las nubes de la
   niebla (forma), el degradado de la neblina (hoy un color medio), la muerte de Samus (c11).
2. Ojos de las estatuas (paleta/2 líneas del BG), agua del FX 0x26 (BG3), Spore Spawn en BG2 sin probar.
3. **Bombas de poder** (las bombas normales y las de abanico ya están). Rayos de onda, hielo, spazer y plasma (y sus
   ataques especiales `FireSba_Fire*`): `nt_unimpl` en proj.c.
2. Ojos de las estatuas (paleta/2 líneas del BG), agua del FX 0x26 (BG3). (Spore Spawn: hecho, docs/50.)
3. **Bombas de poder** y bombas en abanico (las bombas normales ya están).
4. Secuencia de muerte (`HandleSamusOutOfHealthAndGameTile`). (Pausa y mapa: hechos, docs/53; falta el mapa del menú
   de partidas y los ganchos de pausa de Kraid/Draygon/Mother Brain.)
5. **Sonido** (docs/55, rama `intro-audio`): XGM2 con la música y los efectos de la intro a la llegada a Zebes; en Zebes
   solo Samus y puertas. Falta: ajustar la tabla de instrumentos a oído, la música del resto (ventana 6), los efectos
   de los enemigos (ej.: el ojo pide el efecto 0x17 de la biblioteca 2). Regresión: 0 distintos (huida determinista;
   el borde del círculo se cuenta aparte, docs/55).
6. **Música en CD (Mega CD, modo 1)**: al encender se busca el BIOS del Mega CD; si está, se carga un programa en
   su procesador que revisa el disco; con el disco de MegaMetroid la música sale del CD, si no (sin Mega CD o disco
   no válido) suena la del cartucho. Protocolo: MSU-MD salvo que aparezca algo mejor. Detalle y condiciones (música
   del cartucho primero, ventana 6 para el Z80, permisos de los covers): docs/48. La X3 no tiene CD.
6b. **Tamaño de la ROM** (docs/48): hecho el depósito común de tiles (12,0 → 10,5 MB). Lo urgente es la parte fija
   (~290 KB libres para todo el código que falta): cinemáticas de Ceres a páginas, título comprimido, recortar las
   copias de bancos.
7. Rendimiento: salas pesadas pierden ~61 cuadros cada 600 (docs/44: 92B3 la peor). El haz del ojo ya no quita
   cuadros (crateria12). Ver "Lo que queda" en docs/44. Los rayos X (más adelante) podrán usar el mismo armado por pasos.
8. Después de Crateria: el resto de Zebes (la sala de Kraid y otras se ven mal en el visor: esperado).
9. Repositorio: copia limpia lista (abajo), licencia MIT a nombre de Edwin Trujillo y Claude (Anthropic); falta
   crear el repo (preguntar antes a Edwin).

## Cómo se trabaja (comandos)

    sh host/build.sh                       # motor en el PC: check, mkinit... y res/gen/engine/
    make                                          # ROM (rápido, con los datos ya generados)
    make full                                     # completo (necesita REF y REF_INTRO: . work/inputs/env.sh)
    MM_MAX_PALS=2 python3 tools/conv/mkzebes.py sm/sm.smc res/gen/engine   # gráficos de Zebes (y objetos: mkzobj)
    python3 tools/port/port.py                    # traducir del decomp lo que piden los enemigos (docs/42)
    sh tests/regress/regress.sh  /  UPDATE=1 ...  # regresión de imagen / aprobar (comparar a ojo antes)
    host/check ref.bin sm/sm.smc 1 30000 0 # comparar el motor con una grabación del SNES
    tools/bin/retrorun tools/bin/genesis_plus_gx_libretro.so rom.bin guion.txt   # hold N botones, poke, shot, dumpram

`hlref` (el SNES de referencia, `tools/snes_ref/ceres_bot/hlref.c`): `hlref rom entrada.txt salida.bin desde [hasta]`,
con `HL_WARP` (saltar a una sala), `HL_SETXY` (con `HL_SETXY_SCROLL` la cámara la sigue; con `HL_OPENSCROLLS` todas las
pantallas de la sala abiertas), `HL_GOD`, `HL_ENEMIES`, `HL_ITEMS=4` (con la bola), `HL_SHOTS`,
`HL_VRAM`, `HL_VDUMP`, `HL_NOM7`, `HL_ANY`. Las entradas del bot están en `tools/snes_ref/ceres_bot/`.
Medición de cuadros perdidos y perfil del 68000: `tools/perf/salas/` (docs/44; `--prof` en setup_tools.sh).

Probar en la Genesis una sala: ROM con `TEST=1` (entrada de `tests/regress/inputs/zebes_shoot.s`), en retrorun
`hold 11700 -`, `poke <viewer_debug_next> <índice> 2`, `hold 3 S`, `hold 3 SR`: el visor salta a esa sala; con
`poke` a `collected_items`/`equipped_items` se dan objetos, y a `samus_x_pos`/`samus_y_pos` se mueve a Samus (las
direcciones salen de `out/symbol.txt`, `& 0xFFFF`). La sala de la bola es el índice 48 (0x30).

Entregar a Edwin: la ROM y las fuentes van a `C:\proyectos\MegaMetroid` (el escritorio enlazado); las ROM se llaman
`MegaMetroid_crateriaN.bin`.

## Trampas conocidas

- **Lecturas de palabra impares**: GCC junta `p[1] | p[2] << 8` en un `move.w` → ADDRESS ERROR en el 68000. Se
  compila con `-fdisable-tree-bswap` (docs/44). `-mstrict-align` no sirve.
- **Páginas SSF2**: los datos de Zebes llegan a la 23 (`md_ssf2.ld`) y usan lo que dejan libre las de la intro (19-20).
- **`make`** (sin `data`) no rehace el estado inicial: si cambian las variables del motor (gen_vars), correr `mkinit`
  (ver `Makefile`) o la ROM no arranca.
  La cuenta de bytes por página deja 4 KB de margen (alineaciones).
- **Parte fija + título ≤ 4 MB**: con el título (730 KB) al final, lo libre hasta los 4 MB era ~80 KB antes de la
  pausa (docs/53: `pause.c` con `-Os`, sus tiles en la página 18). Código poco usado: `#pragma GCC optimize("Os")`.
- **Parte fija de la ROM ≤ 4 MB** (lo demás va en páginas SSF2). `-O3` con desenrollado infla funciones con bucles:
  el cono pasó de 1.5 KB a 88 KB; ahí se usa `#pragma GCC optimize("O2", ...)`.
- **RAM**: la pila queda entre `_bend` (symbol.txt) y 0xE1000000; con 5 KB más de variables la ROM no arrancó
  (pantalla negra). Para búferes grandes usar RAM que la escena no usa (el cono usa `cinematic_bg_tilemap`).
- **VRAM**: tiles 0-1279 ocupados (sala desde 448, objetos desde 880 o `ZebesObj.base`); libre 0xF000-0xFFFF
  (tiles 1920-2047): ahí van los del cono y, con la caja de mensaje, los de la caja (después se recarga el cono).
  0xEA00-0xEAFF (tiles 1872-1879, entre la tabla de sprites y el scroll horizontal): los 8 tiles del rayo equipado
  (docs/57). Mapa en `inc/platform/md.h`.
- **Colores**: línea 3 debajo del HUD = objetos de la sala (10 colores) + rayo (9-13); la cambia una interrupción.
  En los estados con un jefe de `BOSS_PALETTES` (hoy el Torizo) la sala usa solo la línea 0 y la línea 1 es del jefe
  (docs/51). Toda escritura a `palette_buffer` fuera de las funciones del motor tiene que marcar `nt_pal_dirty`.
- **Una llamada más por cuadro** en el camino común (`md_post`) corre los tiempos de Ceres y la regresión lo marca
  (19 → 99 cuadros, todos corrimientos): lo que depende de la sala se decide al cargarla.
- **Perfil**: los `poke` dependen de las direcciones de cada compilación: regenerar el guion al recompilar.
- **H32**: 64 sprites, 16 por línea; el cono va al final de la lista (detrás de todo, se recorta primero).
- **VRAM del haz** (docs/52): además de 0xF000-0xFFFF usa los tiles libres después de los objetos de la sala y las
  filas 5-31 del Window (como la caja de mensaje). RAM: `cinematic_text_tilemap` desde el byte 1024.
- La regresión marca "corrimientos" cuando cambia la velocidad del código: comparar a ojo (lado a lado con la ROM
  anterior) antes de aprobar.

## Repositorio (2026-10-21)

Copia limpia en `/home/claude/MegaMetroid_repo` (y en el escritorio de Edwin), sin `git init`. Qué entra y qué no:
docs/43; cómo se rehace todo desde la ROM: docs/45; `README.md` en la raíz. Lo que quedó fuera sigue en la carpeta de
trabajo (nada se borró).

## Para ahorrar contexto en el próximo chat

- Empezar por este documento y el de la parte que se toque (no leer todo `docs/`).
- No volcar archivos grandes: `grep -n` y leer por rangos; `port_gen.c` (generado, 370 KB) y `gen/` no se leen.
- Mirar imágenes como mosaicos de varias capturas en una sola (PIL), no una por una.
- Las grabaciones del SNES pesan 1-2 GB: borrarlas al terminar; el disco de la máquina es de ~12 GB libres.
