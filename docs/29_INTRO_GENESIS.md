# 29 — Intro de Super Metroid para la Genesis

Objetivo: reproducir la intro completa del juego, desde que se enciende la consola hasta que Samus
llega al sitio de aterrizaje, en la misma ROM del juego (`tests/samus_play`). Todo tiene que correr
a 60 fps.

La regla es que **en la Genesis no se calcula nada del modo 7**. Las imágenes escaladas se preparan
en la PC con los mismos gráficos del SNES, y la consola solo copia tiles y mueve el scroll.

## Decisiones acordadas

| Tema | Decisión |
|---|---|
| Acercamientos y alejamientos (laboratorio, Ridley, nave, estación) | Tantos tamaños como se puedan cargar sin perder cuadros. En el título el alejamiento va 1,4 veces más rápido que en el SNES (`ZOOM_SPEED`): 33 tamaños, uno cada ~8 cuadros, con 0 cuadros perdidos. |
| Logo de arranque | Se queda 90 cuadros más encendido antes de fundir (`LOGO_HOLD_EXTRA`); se toman del negro que le sigue, así las letras rojas salen en el mismo cuadro. |
| Regla general | Se inventa solo lo que la Genesis no puede hacer (modo 7, mezclas de color por HDMA). Todo lo demás se porta del juego desensamblado. |
| Efecto de super bomba (Power Bomb) | Opción A: anillo de sprites que crece más un cambio de paleta. Si no se ve bien, se pasa a la opción B (modo sombra/brillo). |
| Música y sonido | Van después. El juego ya pasa todos los pedidos de sonido a una cola (ver abajo). Lo primero que sonará serán los efectos de Samus. |
| Marca | Logo de arranque "Síndrome" en el óvalo y título "MEGA METROID". Los scripts de los logos vienen del proyecto `metroid_genesis`. |
| ROM | Intro y juego en la misma ROM. Después del título, START lleva al juego; por ahora directo al sitio de aterrizaje. |
| Idioma | El juego va en inglés, con traducción al español pronto. Todo texto nuevo (menú, texto de la intro, mensajes) va en tablas por idioma, no fijo en el código ni en gráficos sin separar. El título no tiene texto propio: solo los logos. |
| Bebé metroide del título | Se anima como en el SNES cuando el alejamiento terminó y la imagen está quieta. |

## Plan por etapas

1. **Secuencia de título: hecha.** Logo de arranque, letras rojas, las 3 tomas con paneo, el
   alejamiento de 39 tamaños, el logo MEGA METROID y el ©.
2. **Menú de selección de partida: hecho** (partidas, opciones, idioma, controles, ajustes especiales).
3. **Texto de la intro: hecho** ("The last Metroid is in captivity…" y las 5 páginas del diario).
   Las demos entre páginas son imágenes fijas por ahora.
4. Viaje y llegada a la estación Ceres (modo 7).
5. Juego en Ceres: puertas, ascensor, Ridley, huida con contador, escape de Ridley en la sala 242.
   El pozo con balanceo ya está.
6. Salida de Ceres: la estación se aleja (los mismos tamaños de la llegada, al revés), las
   explosiones y la super bomba.
7. Llegada a Zebes y aterrizaje, con la lluvia y el cielo.

## Cómo está hecha la secuencia de título

- **Grabación del SNES real** (`tools/snes_ref/record_title.py`).
  - Corre tu ROM en snes9x y guarda por cuadro: brillo (INIDISP), capas (TM), matemática de color, la
    cámara de modo 7 (`cinematic_var6/8/10`), el OAM y `palette_buffer`.
  - También mide el degradado del título tal como sale en pantalla, con BG1 y los sprites apagados.
    En el SNES lo arman dos HDMA juntos: COLDATA y CGADSUB.
  - Los tiempos, fundidos, letras y paletas animadas salen del juego original, sin reescribirlos.
- **Generador** (`tools/genesis_convert/make_title_data.py`):
  - Las 3 tomas se pre-renderizan con el mismo muestreo del modo 7. La toma 1 es de 584×224 (793 tiles),
    la 2 de 480×224 (685) y la 3 de 256×552 (447).
  - Alejamiento, zoom 67 → 256. El generador elige cuántos tamaños usar simulando el cargador de la
    Genesis: el siguiente se muestra en cuanto termina de cargarse mientras se ve el actual. Hoy son
    39 tamaños, uno cada ~9 cuadros, 592 KB. Cada uno tiene su propio reparto de paletas: usan 4
    líneas, salvo el último, que deja PAL3 a los sprites del logo. El máximo es 559 tiles por tamaño.
  - Bebé metroide: en el SNES es un objeto de modo 7 que reescribe los tiles 224-227 del plano con 3
    cuadros de `$95:A5E1`. En el último tamaño esas 4 celdas llevan tiles propios. La línea de tiempo
    dice qué cuadro toca, leído de los punteros del objeto grabados en el SNES, y se copian 4 tiles
    cuando cambia. La paleta del bebé ya viene en la grabación.
  - Sprites: el OAM grabado se convierte a sprites Genesis, con la hoja del título parcheada con los
    logos. Son 34 conjuntos y 246 tiles. Si un conjunto usa dos paletas del SNES (logo + ©), el © se
    pasa al color más parecido del logo, así todos los sprites entran en PAL3.
  - Línea de tiempo: 2812 cuadros de 16 bytes, más los cambios de paleta. Llega hasta que el SNES,
    sin tocar nada, funde a negro para las demos; ahí la Genesis vuelve a empezar.
- **Reproductor** (`tests/samus_play/src/title.c`):
  - Tomas: se dibujan por columnas o filas a medida que avanza el scroll, en el plano B.
  - Alejamiento: dos bancos de VRAM de 576 tiles, en el plano B y el plano A. El siguiente tamaño se
    carga en el banco libre y se cambia de plano; el plano oculto se aparta con el scroll.
    - Carga: 3200 bytes por cuadro entre tiles y mapa (`FRAME_BYTES`). El bucle espera el vblank
      por su cuenta (sin `SYS_doVBlankProcess`, que lee los mandos y vacía su cola dentro del vblank):
      así las transferencias empiezan en la línea 225 en vez de la 235.
    - Mapa: 28×64 celdas (el ancho del plano) con la base de tiles ya sumada, en dos mitades
      copiadas de una vez desde la ROM. Medido: 14 transferencias cortas de una fila costaban
      ~3 líneas cada una; una sola de 1792 bytes, ~10.
    - `TILE_USER_INDEX` tiene que ser 16, como en tu SGDK; `title.c` lo comprueba al compilar.
  - Paleta: copia de la paleta del SNES en RAM. Cada cuadro se convierte a CRAM con el brillo del
    cuadro (fundidos) y con la resta de color de las tomas 1 y 3.
  - Degradado: una interrupción horizontal cada 4 líneas cambia el color de fondo. Las tablas ya están
    en colores de CRAM, en la ROM. Hacerlo en cada línea costaría ~35 % del cuadro.
  - Todo lo del VDP se prepara antes del vblank en una lista de DMA, y en el vblank solo se transfiere.
    La memoria de trabajo (~3 KB) se pide al montón y se libera al salir, así el juego conserva su RAM.
  - Botones: A/B/C/START saltan al título con un fundido rápido. START en el título pasa al juego con el
    fundido lento del SNES.
- **Medición:** 0 cuadros perdidos en toda la secuencia, medido con `mm_title_cur` en Genesis Plus GX.
  `mm_title_vc` / `mm_title_vc_end` guardan la línea del VDP al empezar y al terminar las
  transferencias de cada cuadro (el arnés las lee para ver si alguna se sale del vblank).

Diferencia conocida: en el SNES el degradado del título también tiñe el fondo del laboratorio. Aquí
solo cambia el color de fondo, que se ve en todo lo negro.

## Menú de partidas y texto de la intro

Flujo: título → START → pantalla de partidas → opciones → START GAME → texto de la intro → juego.
EXIT en las opciones vuelve al título.

- **Grabación del SNES** (`tools/snes_ref/record_menus.py`): enciende tu ROM, pulsa los botones con
  un guion fijo y guarda VRAM, WRAM y captura de cada pantalla (partidas, opciones, páginas del
  diario, demos). Se guardan en `out/menu_snaps`.
- **Generador** (`tools/genesis_convert/make_menu_data.py`): de esas capturas saca las fuentes del
  SNES (la grande de 8×16 del menú, la chica, la verde del diario y la blanca de la primera página),
  el marco y el cursor, el casco, el planeta y las imágenes de las escenas. Escribe
  `data/menu_data.*`, `data/menu_strings.h` y `data/bins/mnu_*`.
- **Código**: `src/menu.c` (partidas y opciones). La intro con texto es el código del juego portado
  (`src/intro_md.c`, ver abajo); la intro vieja con imágenes fijas (`src/intro.c`) se quitó.
- **Opciones que llegan al juego**: cancelación de íconos (auto/manual) y caminar hacia atrás se
  escriben en la RAM del juego (`$09EA` y `$09E4`) al cargar cada sala.

Cascos de las ranuras: portados de `DrawFileSelectSlotSamusHelmet` ($81:9DE4) con los spritemaps
$82:C569 0x2C-0x33. Al elegir una ranura el casco gira de lado a frente, un cuadro cada 8, y se pasa
a las opciones cuando queda de frente o con otro botón (`FileSelectMenu_31_TurnSamusHelmet`).
B vuelve al título, como en el SNES.

Pendiente: las partidas guardadas (SRAM; por ahora todas dicen NO DATA).

### La intro es el código del juego (páginas y recuerdos)

Todo lo que pasa desde "The last Metroid is in captivity" hasta que la nave sale hacia Ceres es el
código del SNES corriendo en la Genesis: el estado de juego `GameState_37_CeresGoesBoomWithSamus_`
($8B:A35B) con sus funciones de cinemática, Samus con su motor real y la entrada de demo (Mother
Brain y larva), los objetos de sprite y de fondo (el diario también es un objeto de fondo), los
efectos de paleta y el NMI. Lo único escrito para la Genesis es cómo se ve.

**Cómo se porta** (el mismo método de docs/28):
- `port/diff/intro_funcs.txt`: las funciones que el SNES ejecuta en la intro (grabadas con el decomp
  en PC hasta que `CinematicFunction_Intro_Func73`, $8B:C100, pasa a la partida nueva).
  `port/tools/filter_dispatch.py` deja en los `Call*` solo esas, para no arrastrar medio juego.
- `port/decomp/sm_8b.c`, `sm_8d.c`, `sm_88.c`, `sm_93.c`: copias filtradas del decomp.
  `sm_intro_extra.c`: funciones sueltas de otros bancos (`port/diff/intro_extra_funcs.txt`,
  con `port/tools/extract_funcs.py`).
- `port/genesis/intro_glue.cpp`: el cuadro (`mm_intro_frame`: lo mismo que `RunOneFrameOfGame` con
  el estado 37 y el NMI), el mapa de RAM (ver abajo) y los registros del SNES que el juego lee.
- Comprobación: `port/host/intro_md_host.c` corre la intro portada en PC con el mismo mapa de RAM de
  la Genesis (con páginas protegidas al final de cada zona) y compara toda la RAM con el SNES cuadro
  a cuadro: **8458 cuadros, 0 distintos hasta el último** (hasta Func73; ver el vuelo abajo).

**El dibujo** (`tests/samus_play/src/intro_md.c`):
- `tools/snes_ref/intro_usage.py` mira, en la RAM y la VRAM del SNES de cada cuadro, qué tiles,
  entradas de mapa, sprites y colores usa cada fase: CAPT (captividad), TEXT (diario), FB50/54/58/5C
  (Mother Brain, larva, científico, laboratorio). `tools/genesis_convert/make_intro_data.py` arma
  con eso los tiles, tablas y paletas de cada fase (`data/intro_data.*`). Nada de eso se inventa:
  sale de los gráficos de tu ROM tal como el juego los pone en la VRAM.
- Cada cuadro: el texto (BG3 del SNES) va al plano A; el casco o el recuerdo (BG2/BG1), al plano B;
  los sprites de los objetos de cinemática se convierten directo desde `DrawSpritemap` ($81:879F),
  con la misma prueba de "fuera de pantalla"; los de Samus, como en el juego; el resto del OAM
  (proyectiles, explosiones) se convierte al final. Los tiles de Samus solo se cargan en las fases
  con Samus (en las otras esa VRAM es de la fase).
- Lo único que se reemplaza porque la Genesis no lo tiene: el fundido cruzado del SNES entre texto y
  recuerdo (mezcla de color) se hace como fundido a negro con el mismo contador del juego.

**RAM**: la intro pide al entrar unos 23 KB del montón de SGDK (RAM del SNES $7F usada por la intro,
mapas, tablas de conversión y de paletas) y los suelta todos al salir: en el juego queda la misma
memoria libre que antes de la intro. Las tablas de trabajo que antes eran variables fijas también
van al montón, así que la intro solo deja ~150 bytes fijos.

**Velocidad**: el diario y el científico/laboratorio van a 60 cuadros por segundo. Mother Brain y la
larva (motor de Samus + objetos + sprites) todavía pierden algunos cuadros en los momentos más
cargados (en la pelea, uno de cada 10-15); los cambios de fase se hacen con la pantalla en negro.

### El vuelo a Ceres (CinematicFunction_Intro_Func54 a Func73)

También es el código del juego: la comparación de RAM cubre hasta que Func73 ($8B:C100) pone la
partida nueva en Ceres (8458 cuadros, solo difiere el último por `SaveToSram`, que falta). En el SNES:
- **La nave** es BG1 en modo 7 (tiles $95:A82F, mapa $96:FE69; en el destello, Func56 cambia por la
  cola de modo 7 el mapa por el de la nave vista de atrás, $7F:4300). No gira: cambia de tamaño y de
  posición con dos ángulos fijos, y usa 15 colores (CGRAM 81-94).
- **La estación, los asteroides, las estrellas y el planeta** son sprites de 6 spritemaps grandes
  ($8C:8FE7...), hasta ~140 entradas por cuadro.
- **"SPACE COLONY"** es texto en BG1 en modo 1 (mapa $5C00, tiles 4bpp en $6000).
- **El destello blanco** es suma de color fijo (CGADSUB $31, COLDATA en blanco que va bajando).

En la Genesis (`tools/genesis_convert/make_intro_flight.py` e `intro_md.c`):
- Modo 7 (lo único que se reemplaza): cada juego de registros de modo 7 que usa el juego (385, en
  `port/diff/intro_usage.json`) se dibuja al compilar igual que el PPU del SNES y se guarda como tiles
  (306 KB, en la página 8 del SSF2). En el cuadro se busca el que corresponde a los registros y se
  muestra en un plano, con el otro de búfer. Los cuadros grandes (cuando la nave pasa pegada a la
  cámara) no caben en un vblank: se mandan en varios y, como la escena no depende del mando, se prepara
  el cuadro que el juego tendrá cuando termine de llegar. Tres cuadros de más de 520 tiles (en pleno
  destello blanco) no se dibujan.
- Sprites: cada spritemap grande se compone como imagen con el orden de dibujo del SNES y se cubre
  con sprites de hasta 32x32 (máximo 56 por cuadro, 9 por línea: entra en los 64 de la Genesis).
- La suma de color fijo se hace en la paleta (a todos los colores, como las capas que marca CGADSUB).
- "SPACE COLONY": el plano A toma el mapa $5C00 en vez de BG3. El paso del vuelo a esa fase no apaga
  la pantalla: las dos fases tienen los mismos tiles de sprites al principio.
- Al terminar, el juego pasa a la partida nueva. Ceres jugable todavía no está: por ahora sigue en el
  sitio de aterrizaje.

### Idiomas y traducción

"JAPANESE TEXT" se cambió por "SPANISH TEXT". El idioma elegido en las opciones cambia todo el texto.

- `tools/lang/menu.txt`: textos del menú, una línea `clave | inglés | español`.
- Texto de la intro en inglés: sale de tu ROM al compilar (no se guarda en el proyecto).
  Como referencia se escribe `data/intro_en_extraido.txt`.
- Texto de la intro en español: `tools/lang/intro_es.txt`, con secciones `[captivity]`, `[page1]`…
  `[page5]`. Cada línea del archivo es una línea en pantalla (máximo 30 letras en el diario,
  24 en la primera página). Una sección vacía usa el inglés.
- Se pueden usar Á É Í Ó Ú Ñ Ü ¿ ¡: el acento se dibuja en la fila de arriba, así que el texto va
  a doble espacio, como en el SNES.
- Después de editar, se regenera con `build.sh` sin `SKIP_MENU`.

## ROM y mapeador SSF2

La ROM pasa de 4 MB (4,4 MB con el vuelo) y usa el mapeador SSF2 estándar, el de Super Street
Fighter II: la ROM se ve en 8 ventanas de 512 KB y las ventanas 1-7 se pueden cambiar de página
escribiendo un byte en $A130F3-$A130FF. Lo tienen las placas SSF2 que se pueden fabricar, la Mega
EverDrive, MiSTer y los emuladores (Genesis Plus GX lo activa solo con una ROM de más de 4 MB). La
cabecera es la normal ("SEGA MEGA DRIVE"); la variante "SEGA SSF" de Krikzz no hace falta.

Cómo se usa (`tests/samus_play/md_ssf2.ld` y `src/mm_ssf2.h`):
- Solo se cambia la ventana 7 (0x380000-0x3FFFFF). Todo el código y los datos comunes quedan por debajo
  (el enlazador lo comprueba). En la ventana 7 normal (página 7) están los datos del título
  (sección `.rodata_title`), que no se usan fuera del título.
- Las páginas 8 y siguientes son las secciones `.bank8`, `.bank9`... (dirección 0x380000, guardadas
  desde 0x400000 en el archivo). Antes de leerlas: `mm_ssf2_map7(8)`; después, `mm_ssf2_map7(7)`.
- Hoy: salas 2,6 MB, título 730 KB, intro 180 KB, nave del vuelo 306 KB (página 8), menú 21 KB,
  código ~40 KB. Para la música, los enemigos y el resto hay lugar en más páginas.

## Sonido: preparado

- `port/decomp/stubs.c` (y `port/tools/gen_stubs.py`): `QueueSfx1/2/3_MaxN(id)` llaman a
  `mm_sound_hook(biblioteca << 8 | largo de cola, id)`, con el mismo id del SNES.
- `tests/samus_play/src/sound.c`: la cola de la Genesis. `play_sfx()` es donde irá el controlador
  (YM2612 + PSG). Con `-DMM_SOUND_DEBUG`, la línea de información muestra el último pedido, por
  ejemplo "SFX3 05", para identificar cada efecto de Samus mientras se rehacen.

## Compilar y probar

- ROM normal: `build.sh` / `build.bat`, con la secuencia de título.
- `-DMM_START_ROOM=n`: arranca en la sala n, sin título.
- `-DMM_NO_TITLE`: arranca en el sitio de aterrizaje. Es la que usa la prueba de punta a punta
  (`md_play_e2e.py`), que sigue saliendo 1693/1693 y 2103/2103 cuadros idénticos.
- Regenerar el título (Linux):
  `RETRORUN=... SNES9X=... ./build.sh`. Graba `out/title.rec` si no existe y genera `data/title_data.*`.

## Referencia del SNES de Ceres completo (juego, huida, cinemáticas, llegada a Zebes)

`tools/snes_ref/ceres_bot/`: un bot (hlbot.c + bot_logic.h) que juega Ceres en el decomp (snesrev) y graba
la entrada. `ceres_completo_in.txt` (mismo formato que g2.txt) va desde el encendido hasta Samus parada
en Landing Site (91F8), cuadro ~21400:

| cuadro | qué pasa |
|---|---|
| 10757 | empieza el juego en Ceres (estado 8), sala DF45 |
| 12589 | sala de Ridley (E0B5); Samus espera; Ridley la baja a 29 de energía |
| ~15740 | ceres_status 1: Ridley se va con la bebé; 2: autodestrucción |
| 16338-17719 | huida: E06B, E021, DFD7 (subida en zigzag), DF8D, DF45 |
| 17760 | ceres_status 0x8000 en DF45: empieza el vaivén |
| 18900 | estado 32 (ascensor), 34 (nave escapa, explosión, viaje a Zebes) |
| 20780 | estados 6-7: llegada, aterrizaje; 20870 estado 8 en Landing Site |

Vaivén (RoomCode_CeresElevatorShaft, $89:ACC3): comparado cuadro a cuadro con la Genesis (sala 237,
"Ceres Shaft (huida)"): misma tabla, mismo inicio (índice 34, espera 60), M7B de 0 a 34 y vuelta con
los mismos cuadros (1 → 2 a los 2 cuadros, 34 sostenido 61 cuadros, etc.). Inclinación máxima 34/256
(7,6°) en los dos. `sway_snes.txt`: M7A, M7B, M7C, M7X, M7Y, índice y espera por cuadro en el SNES.
