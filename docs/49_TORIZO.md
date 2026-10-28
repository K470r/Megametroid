# 49 — Dibujo del Bomb Torizo (crateria14)

Fecha: 2026-10-27. Sala 9804 (estados 981B y 9835: con el Torizo; 984F: después). La lógica ya coincidía con el SNES
(docs/46); el dibujo estaba roto: partes que desaparecían, una pierna negra, colores de estatua pegados, trozos de la
estatua como "X" amarillas y orbes invisibles.

Las capturas comparativas (SNES | Genesis antes | Genesis ahora) no van en el repositorio: tienen gráficos de la ROM
(docs/43). Se rehacen con `tools/compare/lado_a_lado.sh` y `mont.py` (abajo); para "antes", la misma ROM de prueba
compilada desde `dedup-tiles` con el `md_main.c` de esta rama.

## Cómo se compara (`tools/compare/lado_a_lado.sh`)

La misma entrada en el SNES (`hlref`) y en la Genesis (ROM de prueba `TEST=1`): la del bot hasta Zebes
(`in_z2.txt`) más `tools/compare/torizo_in.txt` (tomar las bombas: un salto corto con disparo abre la esfera, cerrar
la caja de mensaje, pelear hasta que muere). En el SNES `HL_WARP`/`HL_SETXY`; en la Genesis lo mismo en el mismo cuadro de
la entrada con `-DMD_TEST_WARP_AT/DOOR` y `-DMD_TEST_SETXY_AT/X/Y` (`md_main.c`, solo en la ROM de prueba). Como la
lógica es la misma, el cuadro del motor que muestra la Genesis (`md_shown_frame`) se compara con el mismo cuadro del
SNES: `mont.py` arma los pares. (Con el salto por ascensor, el BG1 del SNES queda a medio dibujar en esta sala: la
estatua del fondo aparece corrida; es de `hlref`, no del juego.)

## Qué estaba mal y qué se hizo

### 1. Colores: las paletas del Torizo no se conocían al convertir (la pierna negra)

En el SNES el Torizo usa las paletas de sprites 1, 2 y 7 (y la 3), y no las carga la lista de la sala sino su código:
`Torizo_Init` ($AA:C87F) escribe la 7 (`kTorizo_Palettes_1`) y la 3, y llama a `Torizo_C250` (la 1 y la 2 de estatua,
`kTorizo_Palettes_2/3`); al despertar, `Torizo_C268` (`Palettes_4/5`); al recibir daño, `Torizo_Hurt` las pone en
blanco y `Torizo_C2B0` las vuelve. `mkzobj.init_palettes` solo reconocía arreglos `g_word_XXXXXX`: para la conversión
las paletas 2 y 7 valían negro, todos sus colores se juntaban en uno y la pierna (paleta 2) salía negra sobre el
fondo oscuro, como si faltara.

Ahora (`mkzobj.BOSS_PALETTES`, caso por caso como pide la regla): por jefe, las funciones del decomp que escriben sus
paletas, por área (Bomb Torizo: estatua `Torizo_Init + Torizo_C250`, despierto `Torizo_C268`; Golden Torizo:
`Torizo_C280`, `Torizo_C298`). `fn_palettes` lee los valores del decomp (arreglos `static const` del mismo archivo o
`g_word_` de la ROM). Los colores se juntan mirando todos los momentos a la vez (`merge_colors`: iguales si lo son en
todos; la distancia es la suma de las de cada momento). La Genesis toma cada color de `palette_buffer` en cada cuadro,
así que estatua, despertar y destellos se ven como en el SNES mientras la agrupación valga para todos.

### 2. Lugar para los colores: la línea 1 para el jefe

Despierto, el Torizo usa 42 índices del SNES que en la Genesis son 29 colores distintos; los objetos tenían 10 lugares
(línea 3 debajo del HUD). En los estados de sala con un jefe de `BOSS_PALETTES` que se dibuja en la sala
(`mkzobj.boss_line`):

- el fondo de la sala va en **una sola línea** (`mkzebes.convert(..., max_pals=1)`; en 9804 junta 8 colores). Para que
  cedan los colores que casi no se ven (bloques de reacción de todo el juego, puertas) y no los del fondo, en ese caso
  la fusión pesa por los píxeles que aparecen en el nivel (`usage`);
- la **línea 1 queda para los objetos**: 15 colores más. `ZebesObj.src` tiene 30 índices (15 de la línea 3 y 15 de la
  línea 1) y `md_zebes.c` los pone en `zsrc[17..31]` cuando la sala tiene una línea (la CRAM, los fundidos y el tinte
  los tratan como cualquier color de la sala);
- qué paletas del SNES del jefe van a la línea 1 se elige probando todos los repartos (`convert_state`: el de menor
  costo de fusión). En 9804: las paletas 1 y 2 en la línea 1 (5 fusiones), lo demás (paleta 7, comunes, proyectiles)
  en la 3. Con jefe, la fusión pesa con la raíz de los píxeles: el ojo naranja (`Palettes_4` índices 12-14, pocos
  píxeles) no se pierde contra las superficies grandes;
- `compose` (piezas juntadas en sprites grandes) hace una imagen por (prioridad, línea) y cada punto va en la de la
  pieza que queda encima (la primera del spritemap), como el SNES y la Genesis resuelven sprites encimados.
  (En 9804 los tiles juntados no entran desde 616, así que el Torizo va pieza por pieza, como antes.)

Las claves de dibujo son por sala; ahora las de enemigos se filtran por los bancos de la población de cada estado
(`pop_banks`): los de 984F (otro juego de tiles) ya no ocupan colores en 981B/9835.

### 3. Colores que no se subían a la CRAM (estatua pegada, blanco pegado)

El SNES copia la paleta entera en cada NMI; la Genesis solo los colores marcados (`nt_pal_dirty`). El código
traducido escribía `palette_buffer` directo (`Torizo_Hurt`, `Torizo_C2B0`, y también Spore Spawn, un enemigo de $A8
y dos proyectiles de $86) sin marcar: el Torizo quedaba con los colores de estatua o en blanco hasta otro cambio.
`tools/port/port.py` ahora agrega el aviso después de cada una de esas escrituras (`port_gen.c` regenerado).

### 4. Trozos de la estatua ("X" amarillas) y orbes invisibles (claves que faltaban, `mkzkeys.py`)

- **Trozos de la estatua**: los crea el PLM D6EA (la esfera de las bombas) con `PlmInstr_SpawnTorizoStatueBreaking`
  ($84:D357 → proyectil $86:A993), no el Torizo; sus listas salen de la tabla `$86:A7AB`. `mkzkeys` ahora recorre las
  listas de los PLMs de cada estado (`plm_walk`, con el tamaño de los argumentos de cada instrucción leído del decomp)
  y agrega los proyectiles que crean (`PLM_EPROJ`: también la estación de guardado y el humo de las puertas ojo), y
  sigue tablas de listas (`BW_86(0xA7AB + 2 * i)`).
- **Sus tiles**: el mismo PLM copia antes los tiles de los trozos (`PlmInstr_CopyFromRamToVram`: $AD:B200 → VRAM
  $6E00, tiles 0xE0-0xFF). `mkzobj.plm_sprite_copies` aplica esas copias a la VRAM de sprites del estado (sin eso los
  trozos salían con los tiles comunes que había antes: las "X").
- **Orbes**: `Torizo_Init` pone `palette_index = 0`, así que sus proyectiles tienen g = 0 (no la paleta del juego de
  tiles, 0x200). Sus listas al rebotar salen de las tablas de `Eproj_InitXYVelRandom`
  (`addr_kEprojInit_BombTorizosChozoOrbs_init0/1`) y de la pre-instrucción que pone `EprojInstr_SetPreInstr_`
  ($86:8161 → `sub_86A887`, listas $86:A48A/A48E): las dos se siguen ahora.
- `zdraw.log`: se corrió `mkzkeys.py` sobre las 88 salas de Crateria y Brinstar y se sumó lo nuevo (4769 → 5083
  claves; se quitaron las 19 de los proyectiles del Torizo con g = 0x200).

### 5. Fondo de biblioteca con el scroll del SNES

En Zebes el plano B se movía con `layer2_x_pos`; el SNES usa `BG2HOFS = bg2_x_scroll + layer2_x_pos` (door.c). En las
salas con fondo de biblioteca (64x32 celdas, igual que el BG2) la sala se carga con `b_mode = 1` y el plano B usa los
registros del SNES (en 9804, `bg2_x_scroll = 256`). Se decide al cargar la sala: una llamada más por cuadro corría los
tiempos de Ceres y la regresión pasaba de 19 a 99 cuadros distintos (todos corrimientos, ver docs/48).

### 6. Lista del fundido en Zebes

`zroom` heredaba la lista del fundido (`NT_FADE_LIST`) de la sala base de Ceres; ahora es esa más los colores de la sala
y de sus objetos (`md_zebes.c`).

## Límites de la Genesis en esta pelea

- Sprites: en los cuadros medidos, hasta 16 por línea (nunca más) y en los más cargados (orbes en el piso, explosiones
  de la muerte) la lista llega a los 64 de H32; aun así las capturas coinciden con el SNES.
- Colores: 25 lugares (15 + 10) para ~45 colores del Torizo, los comunes y los proyectiles: se juntan los más
  parecidos (línea 1: 5 fusiones; línea 3: 15). El fondo de 9804 pierde 8 colores en una línea.
- Velocidad (ya estaba): en la pelea se pierden entre 6 y 20 % de los cuadros (por cada 1000 de la consola, 937-794
  del motor; la base daba 934-770). Pendiente de docs/44.

## Comprobación

- `check` con la grabación de la pelea (`HL_WARP=21:8BC2 HL_SETXY=170:60:160 HL_ENEMIES=1 HL_GOD=1`, 12213 cuadros,
  hasta después de la muerte): 3 cuadros con diferencias (2894-2896), solo `vram_write_queue_tail` (la cola de copias a
  la VRAM del SNES, que el motor no usa: la llena el PLM D6EA y el NMI no la vacía en los cuadros de la caja de
  mensaje). Igual con el `port_gen.c` anterior: no es de estos cambios.
- `regress.sh`: intro 0 distintos; Ceres y Zebes 15 distintos (la base: 19), en la misma franja sensible a los tiempos
  (7288-10047, docs/48). Control: la base con 0x700 bytes de relleno (lo que se corrió la parte fija) da 20.
- `md_missing_ring` al terminar la pelea: solo la clave 5 (destello de Samus al recibir daño), que falta en todas las
  salas.
- ROM: 10 752 KB, igual que antes.

Observado y no tocado: en algunos cuadros Samus se ve en el SNES y no en la Genesis (el parpadeo de invencibilidad
desfasado un cuadro).
