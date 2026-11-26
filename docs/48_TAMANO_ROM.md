# 48 — Tamaño de la ROM, Mega CD y traducción

Fecha: 2026-10-22. Medido sobre la ROM compilada desde la ROM del SNES (docs/45), no estimado.

## Qué ocupa la ROM

Antes del depósito de tiles: 12 032 KB (12 320 768 bytes, páginas SSF2 8-23).

| Qué | KB | Dónde | Se lee |
|---|---|---|---|
| Salas de Zebes (`zebes.o`): tiles de sala 1747, celdas/`l2`/`bg2`/paletas 1889, tiles de objetos 1394, claves y registros 825 | 5 855 | páginas | tiles: al entrar a la sala; lo demás: al jugar |
| Level data y BTS de las salas | 1 359 | páginas | al jugar (el SNES los tiene en la RAM $7F) |
| Intro (vuelo, escenas) | 427 | páginas 19-20 | en la intro |
| Parte fija: código (~1 MB), copias de bancos del SNES (782), cinemáticas de Ceres en modo 7 (360) y huida de Ridley (195), Samus (230), salas de Ceres, título (730), voces (65), estado inicial (72) | ~3 300 | 0-0x3FFFFF | |

**Todo Zebes ya está.** `mkzebes.py` convierte las salas a las que se llega por puertas: las 6 áreas, 310 estados,
284 juegos de gráficos (Crateria 567 + 525 KB de objetos, Brinstar 736 + 658, Norfair 965 + 404, Wrecked Ship 434 +
191, Maridia 686 + 314, Tourian 247 + 128). Lo que falta sumar es lo del resto del juego que no son salas: enemigos y
jefes de las otras áreas, el final, la música y el código.

**El límite que llega primero es la parte fija.** Lo común termina en 0x335E28 y tiene que quedar debajo de
0x380000 (`md_ssf2.ld`): quedan ~290 KB para el código que falta (Norfair, Maridia, jefes, Tourian, final) y la
música. Ver "Lo que sigue".

## Hecho: depósito común de tiles (puntos 2 y 3)

Las salas repiten tiles: 55 895 tiles de sala guardados, 26 374 distintos (47 %); 44 619 de objetos, 14 142 distintos
(32 %). `mkzebes.py` ahora guarda cada tile distinto una sola vez en un depósito (`kZebesPool`: 40 trozos de 1024
tiles, 32 KB, repartidos en las páginas) y cada sala / juego de objetos guarda sus tramos `(índice, cantidad)`
(`ZebesGfx.runs`, `ZebesObj.runs`). `zebes_load_vram` (`md_zebes.c`, `zebes_dma_runs`) copia los tramos a la VRAM en el
mismo orden y lugar que antes: **la VRAM queda igual byte a byte**. No usa RAM ni descomprime: DMA desde la ROM, con
la pantalla apagada, en el mismo momento que el SNES (`LoadCRETilesTilesetTilesAndPalette`, $82:E783).

Coincidencia exacta (sin volteos): con volteos el depósito sería un 2 % más chico pero habría que reescribir los
atributos de las celdas y de los sprites.

Resultado: 100 514 tiles → 40 508 en el depósito, 27 376 tramos (107 KB). ROM de 12 032 KB a **10 752 KB**
(11 010 048 bytes; páginas 8-20: tres páginas de 512 KB menos y la 18 también libre). `mkzebes.py` comprueba al
generar que los tramos rehacen los tiles de cada sala.

### Comprobación

- VRAM de los tiles de sala y objetos (448-1279) después de entrar a cada una de las 255 salas del visor, ROM anterior
  contra la nueva: 244 iguales en una corrida seguida; las otras 11, de a una: iguales o distintas solo en tiles
  animados (los que cambian solos de un cuadro al siguiente en la ROM anterior). Ningún tile distinto.
- `regress.sh`: intro 0 distintos. Ceres y Zebes: 19 cuadros "distintos" (7105-8187, 10047) y 83 corrimientos, todos
  en Ceres y en la llegada a Zebes; en las salas de Zebes de la prueba, ninguno más. Causa: al crecer las tablas de
  la parte fija se corren las direcciones de los datos (los tiles de Samus, que se copian cada cuadro: `vdp_dma_vram`
  parte las DMA que cruzan un borde de 128 KB) y, desde el cuadro 6572 del motor, algunos cuadros se muestran un
  cuadro de la consola antes o después. Comparados lado a lado, son las mismas imágenes corridas un cuadro (el reloj
  de la huida lo muestra); en el pozo que gira (7476) cambia el cuadro en que se ve el borde de la rotación.
  Control: la ROM anterior con 512 bytes de relleno no cambia nada; con el depósito sí. Falta que Edwin lo mire y
  apruebe (`UPDATE=1 sh tests/regress/regress.sh ceres zebes`, docs/41).
- La sensibilidad del pozo al tiempo ya existía: cualquier cambio que corra la parte fija la va a mostrar.

## Hecho: cinemáticas de Ceres en páginas

`kM7Tiles`, `kM7Maps` (modo 7, `mkm7.py`) y `kGetawayTiles`, `kGetawayMaps` (huida de Ridley, `mkgetaway.py`) ya no
van en la parte fija: `mkzebes.py` los reparte en las páginas como un bloque más (`CINE_BLOBS`) y escribe
`res/gen/md/md_cine.s` (las secciones `.bankN`) y `md_cine.h` (`CINE_PAGE_<bloque>`). Por eso `mkgetaway.py` y `mkm7.py`
corren ahora antes que `mkzebes.py` (`tools/conv/make_data.sh`). En `md_engine.c` cada lectura pone la página antes
(`map_page_ext`) y deja la que estaba: las DMA de tiles y mapas de la huida (`ga_vblank`, `ga_union_rows`), las de
tiles del modo 7 (`cine_vblank`) y la lectura de las celdas del modo 7 (la del cuadro). Valen en el vblank (que pone la
página del nivel y la restaura al salir) y en la lógica.

Resultado: la parte común termina en 0x2AAF28 (antes 0x335E28): **−569 KB**; quedan ~852 KB libres debajo de
0x380000. La ROM sube a 10 880 KB (la página 21 se usa: lo de Zebes que se corrió).

Comprobación: intro 0 distintos; Ceres/Zebes 99 cuadros "distintos" (antes 19), todos en Ceres. La huida de Ridley
es igual (mismos cuadros); en las cinemáticas, las dos ROM muestran las mismas imágenes del modo 7 en el mismo orden
(`m7_shown` cuadro a cuadro), pero el cargador las arma con el vblank libre y las cambia cuando están (docs/35): ahora
alguna cambia unos cuadros del motor antes o después y a veces se saltea una intermedia distinta (antes la 38, ahora
la 2). No hay datos rotos. Falta la aprobación de Edwin (docs/41).

## Hecho: título comprimido

`make_title_data.py` comprime con LZ4W (`$GDK/bin/lz4w.jar`, en paralelo) los tiles de cada escena en bloques
independientes de 3200 bytes (100 tiles: lo que el título carga por cuadro, `FRAME_BYTES`) y cada mitad de los mapas
de los tamaños del alejamiento (14 filas x 64 celdas). Los mapas de las tomas 1-3 quedan sin comprimir (se leen por
columnas en el paneo). `title.c` descomprime cada paso con `lz4w_unpack` de SGDK en `enemy_ram7800` (RAM del juego,
8 KB: mientras se ve el título el juego no corre y `reset_game_vars` la pone en cero al empezar la partida) y la lista
de DMA lo copia desde ahí en el vblank, como antes desde la ROM. En un cuadro va a lo sumo un bloque y una mitad.

Resultado: 620 KB → 270 KB; la sección del título bajó de 746 a 380 KB y toda la parte fija (título incluido)
termina en 0x30A216, debajo de 0x380000. Comprobación: los 3000 primeros cuadros de la consola (logo, letras, tomas,
alejamiento y pantalla de título: 827 imágenes distintas) son iguales cuadro a cuadro a la ROM anterior (`hashseq` de
retrorun). `make full` desde cero da el mismo md5.

## Límite de tamaño (Edwin, 2026-10-29 y 2026-11-03)

- La Mega EverDrive X3 carga hasta **56 Mbit (7 MB)** y no tiene RAM en el cartucho (lo que se lee al jugar no se
  puede descomprimir en otra RAM). La placa propia todavía no está diseñada.
- Decisión (2026-11-03): **terminar el juego primero, ocupando lo menos posible, con un tope de 16 MB** (lo que el
  enlace SSF2 permite, páginas hasta la 31). Al final se baja el tamaño todo lo que se pueda. Para llegar a 7 MB con
  todo el juego habría que bajar más de la mitad; las ideas medidas o por medir están en "Lo que sigue" (tiles
  comprimidos hacia la VRAM al entrar a la sala, gráficos y mapas por juego de tiles en vez de por estado de sala,
  copias de bancos recortadas).

## Hecho: el sonido en la ventana 6 (2026-11-03, rama `musica-juego`)

Lo que proponía la sección del Mega CD (abajo), aprobado por Edwin con tope de 16 MB (docs/55, "Todo el juego"):
- **La ventana 6 (0x300000-0x37FFFF) muestra la página de sonido del conjunto de música cargado**, cambiada solo en
  `music_upload` (`$A130FD`), como el SNES sube un conjunto (`APU_UploadBank` $80:8F0C). Páginas 6 y 24-28 (desde 2026-11-05: 6 y 26-31; la 24 y la 25 son de Zebes, con los enemigos de todas las áreas; en
  `md_ssf2.ld` hay lugar hasta la 31: 16 MB).
- Lo común tiene que quedar **debajo de 0x300000** (`ASSERT` en `md_ssf2.ld`). Para eso salieron de lo común:
  - el título → al final de la página 22 (se ve con la ventana 7 en 22, `front_screens`);
  - Ceres → la página 7 (niveles, tiles y planos de sus salas, estado inicial: 340 KB);
  - la música y los efectos de las páginas (los globales siguen en lo fijo: 502 KB).

| | antes | ahora |
|---|---|---|
| fin de lo común | 0x34C020 | 0x2F6640 (38 KB libres debajo de 0x300000) |
| `_bend` | 0xE0FFE2FC | 0xE0FFE2C0 |
| ROM | 11 392 KB | 14 592 KB (páginas hasta la 28) |

**Lo común casi no tiene lugar**: lo que falta del código (≈ 300 KB) necesita liberar antes. Lo que más da es
recortar las copias de bancos (abajo): de los 782 KB, ≈ 430 KB son rangos de funciones del decomp (código del SNES).

## Hecho: tiles de las salas comprimidos (2026-11-18, pedido por Edwin)

Los tiles de Zebes se descomprimen al cargar la sala: en la transición de la puerta y al volver de la pausa
(`md_zebes.c`, `zebes_load_ops`). Formato MZ (`tools/conv/mzpack.c` comprime con parseo óptimo, `tools/conv/mz.py`
lo llama y comprueba cada trozo; el descompresor de la Genesis está en ensamblador en `md_zebes.c`, `mz_unpack`):
LZ con códigos gamma de Elias intercalados, como ZX0, con "repetir el último desplazamiento" (los tiles repiten a
distancias fijas: 4 bytes una fila, 32 un tile) y el byte del desplazamiento con 7 bits y el primer bit del largo.
Medido: 53 % (zlib da 50 % pero con Huffman, más lento de descomprimir; transformar los tiles antes no ayudó).

Cómo se reparten (`mkzebes.py`, medido sobre el depósito de 64.244 tiles):
- Los tiles de los congelados (Kraid, Draygon...: 5.448) los lee el juego mientras corre: siguen sin comprimir en
  `kZebesPool`.
- Comprimir el depósito común por bloques no sirve: las salas comparten tiles sueltos y se descomprimía de más
  (192 tiles usados por carga, hasta 2.112 decodificados). Comprimir cada sala con todos sus tiles pierde lo que se
  ahorraba al no repetirlos (1,66 MB).
- Lo que se hizo: los tiles que usa una sola sala (todos sus estados) o un solo juego de objetos van en trozos
  propios de 64 tiles (2 KB); los que usan varias, ordenados por quiénes los usan, en trozos de 16. Cada carga es una
  lista de operaciones por trozo (cada trozo se descomprime una vez). El búfer es `md_scratch` (2 KB; `meta_idx` se
  rehace después: `meta_build`): no hace falta RAM nueva.
- Resultado: 1.837 KB de tiles -> 1.010 KB (1.777 trozos), operaciones 254 KB. La página 24 queda vacía y sobran
  221 KB en las demás (antes 31 KB): unos 730 KB libres.

Tiempo: se descomprime en el vblank de la carga, con la pantalla apagada. En promedio cada sala descomprime 11 KB
(máximo 21) y sus objetos 8 (máximo 23); la sala del aterrizaje pasa de 11 a 25 cuadros. Las transiciones de puerta
de la Genesis eran ~50 cuadros más cortas que las del SNES: siguen siéndolo. Mientras descomprime, el XGM2 no queda
avisado de DMA (la música sigue: `md_snd_protect`). La pantalla se enciende al empezar el vblank siguiente (md_engine.c,
`vb_display_next`, en `md_vint`): encendida al terminar la carga, si esta se pasaba a la imagen el HUD y los colores
se escribían con la imagen empezada, y según dónde terminara el primer cuadro salía o no (la regresión cambiaba). Así
el primer cuadro después de cargar es siempre negro (dentro del negro de la transición); mientras tanto
`vb_lines_left` da todo el tiempo (la pantalla está apagada). Regresión aprobada de nuevo (ceres, zebes) por ese cuadro.
Validación: regresión 0 distintos (intro, Ceres, Zebes: la imagen es la misma) y la VRAM de la sala igual antes y
después de la pausa.

## Hecho: fondos de biblioteca comprimidos (2026-11-25, punto 9)

Los mapas del BG2 de las salas (fondos de biblioteca, 64x32 celdas o 64x64 en Kraid) se suben enteros a la VRAM al
cargar la sala: ahora van comprimidos (`mkzebes.py`: trozos MZ de 16 filas, 2 KB, en `ZebesGfx.bg2z`, cada uno con su
largo antes) y `zebes_load_vram` los descomprime en `md_scratch` (después rehace `meta_idx`: `meta_build`). Los cielos
de la superficie (`sky`) se leen por fila al jugar: quedan sin comprimir. Con los rayos X el plano B se reescribe con
otra prioridad: antes se leía el mapa de la ROM; ahora se lee la fila del plano B de la VRAM y se le pone la prioridad
del mapa (`bg2p`: todas sus celdas tienen la misma, salvo Kraid, que no pasa por ahí).
Resultado: 167 mapas, 672 KB -> 56 KB (MZ; zlib daba 47). La página 21 queda vacía y sobran 122 KB en las usadas.
Comprobación: el plano B después de entrar a las 255 salas del visor es igual con la ROM anterior en 247; en las otras
8 (agua, lava, Crocomire) la diferencia es del momento de la captura (las dos ROM llegan a otro cuadro del juego):
las capturas a la vista son iguales. Regresión: 0 distintos.

## Hecho: datos sin repetir y el sonido en las páginas libres (2026-11-26, punto 9)

Lo que se lee al jugar no se puede comprimir (no hay RAM), pero sí guardar sin repeticiones, con un acceso directo:
- **BTS de los niveles** (`mkdata.py`, `room.h` `bts_at`): cada fila de 16 bloques una sola vez y un byte por fila de la
  sala (`LevelDataN.bts_idx`). 453 KB -> 109 KB. El nivel en sí (906 KB) no: se lee cientos de veces por cuadro en las
  colisiones y la indirección costaría rendimiento.
- **Capa 2 propia** (`mkzebes.py`, `ZebesGfx.l2i`, `block2_at`): igual, por sala. 355 KB -> 105 KB.
- **Registros de sprites de los objetos**: muchos juegos repiten los mismos (el mismo enemigo con los mismos tiles). Los
  juegos se reparten en depósitos comunes (`kZR`, hasta 0x7F00 palabras: la clave guarda el desplazamiento con 15 bits),
  cada juego en el que más registros comparte; cada depósito va en una página con sus juegos. 1361 KB -> 758 KB (12
  depósitos). Sale también lo que no usaba nadie (los registros de las salas por cuadro, Kraid, Draygon...).
- **Salas especiales** (C001 Ceres explota, C002 Zebes, C0FF fin de juego): sus tiles y planos salían de la parte fija;
  ahora los reparte mkzebes en las páginas como los bloques de las cinemáticas (`CINE_BLOBS`, `md_cine.h`) y
  `md_engine.c` pone la página por sala al cargarlas (por sala: bloques de páginas distintas tienen la misma dirección).
- **El sonido en las páginas que Zebes deja vacías**: mkzebes anota cuáles (`res/gen/md/pages_free.txt`) y `mksnd.py`
  las usa antes que la 26 (`md_ssf2.ld` tiene páginas de sonido de la 8 a la 31; una página con las dos cosas da
  superposición). Con las tres libres (16, 17, 21) el sonido termina en la 27: **la ROM pasa de 16 MB a 14,4 MB**.
- `mkpbomb.py` usa la última página de los niveles solo si entra (si no, la siguiente, que cuenta mkzebes); el margen
  por página de mkzebes es 4 KB (los arreglos nuevos de bytes suman alineaciones que la cuenta no ve).

Comprobación: planos A y B de las 255 salas iguales a la ROM anterior (los tiles distintos son solo los de Samus y el
HUD, por el momento de la captura); el cadáver del Torizo igual a los 700 y 3000 cuadros; regresión 0 distintos;
lotes de salas en el PC contra el SNES (Crateria, Brinstar, Norfair, Maridia) 0 diferencias.

## Hecho: 12,5 MB (2026-11-26, punto 9, tercera tanda)

- **Celdas sin variantes**: cada bloque guardaba sus 4 variantes (sin volteo, horizontal, vertical, las dos); las otras
  tres son siempre la misma con las columnas o las filas cambiadas y los bits de volteo de la Genesis (comprobado en los
  23 150 bloques). Se guarda solo la base y `md_zebes.c` (`cell_of`) calcula la celda: 791 KB -> ~198 KB.
- **Arreglos iguales una vez por página**: al final de mkzebes, cualquier arreglo `NT_BANK` igual a otro de la misma
  página (los estados de una sala con las mismas celdas, las claves de juegos de un mismo depósito...) se emite una vez
  y las referencias van al primero (~575). Los gráficos con las mismas celdas van juntos en una página.
- **El reparto en las páginas** cuenta el tamaño exacto de lo que se emite (cada arreglo, sin repetir lo que la página
  ya tiene) y, después, intenta vaciar la página propia de Zebes menos llena moviendo sus piezas a los huecos de las
  demás. La última página de niveles y las anteriores (solo niveles) también prestan sus huecos. Margen por página
  1,5 KB; reserva de la página de los jefes 312 KB (usa ~305).
- **El sonido en 5 páginas** (`mksnd.py`: si el reparto voraz da N, prueba órdenes al azar para N-1).
- **La página de los jefes pasa de la 25 a la 21** (`port.py`, `mkdata.py`, `mkzebes.py`, `port_gen.h`): con las páginas
  14-17 para el sonido, lo último que se usa es la 24. **La ROM queda en 12,5 MB** (desde 16 MB).

Lo fijo de cada página: 7 Ceres; 8-9 niveles; 22 el título; 23 el menú; 21 los jefes. Para bajar otra página habría que
liberar ~500 KB más de Zebes (el nivel, 906 KB, es lo único grande que queda sin tocar: ver arriba por qué).

## Lo que sigue (en este orden)

| Paso | Ganancia medida | Costo / riesgo |
|---|---|---|
| Recortar las copias de bancos (`kBankXXB`, 782 KB) a lo que se lee | sin medir | todas las lecturas pasan por `A6W`/`BW_86`...: medir en `host/` con las grabaciones |
| `make size`: parte fija libre y llenado de cada página | — | que falle antes de pasarse |

No se puede comprimir lo que se lee al jugar (celdas, `l2`, claves, registros de sprites, level data: ~5 MB): no hay
RAM donde descomprimirlo (el SNES tiene 128 KB; la Genesis 64 KB, casi llenos: en 2026-11-25 quedan ~2,7 KB para la
pila). Inventario de 2026-11-25 (KB en la ROM): registros de sprites de los objetos 1361, tiles comprimidos 1038, level
data 906 + BTS 453, celdas 791, capa 2 propia 355, claves 356, depósito sin comprimir 227, operaciones de carga 253.
De lo que solo se lee al cargar la sala quedan las operaciones (`kZG*_ops`, `kZO*_ops`: 253 KB): candidatas.

## Mega CD (decidido por Edwin)

Resultado que se quiere:
1. Al encender, el juego busca el BIOS del Mega CD.
2. Si lo encuentra, carga un pequeño programa en el procesador del Mega CD (el 68000 secundario) que revisa el disco.
3. Si el disco es el de MegaMetroid, la música sale del CD. Si no hay Mega CD o el disco no es válido, suena la del
   cartucho.

Método: modo 1 (el cartucho manda; el Mega CD aparece en 0x400000-0x7FFFFF). El SSF2 solo usa direcciones debajo de
0x400000, así que convive con el modo 1. El programa del Mega CD seguirá el protocolo MSU-MD (el conocido para
cartuchos con audio de CD), salvo que se vea algo mejor para este caso; así sirve en Mega CD real con flashcart y en
emuladores y núcleos que ya lo soportan. La X3 no tiene CD: siempre la del cartucho. Hay que confirmar en cada destino
(MiSTer, EverDrive con CD) el modo 1 con SSF2 y el tamaño máximo de ROM.

Condiciones:
- Hace falta primero la música del cartucho: hecha (docs/55; antes solo las voces, docs/40). Propuesta acorde a las reglas: convertir
  las secuencias N-SPC de la ROM con una tabla de instrumentos FM/PSG/PCM nuestra (la tabla va en el repositorio; las
  secuencias salen de la ROM de cada quien).
- El Z80 no sabe qué página del SSF2 está puesta: la música del cartucho tiene que estar en una ventana que no cambie
  mientras suena: la ventana 6 con la página del conjunto de música, cambiada solo en `music_upload`. Hecho (arriba).
  La ROM con el sonido llega a 14 592 KB: con el modo 1 del Mega CD el cartucho sigue viendo sus 4 MB y las páginas.
- Los covers de terceros (`res/*.mp3` en la carpeta de trabajo) necesitan permiso de sus autores para ir en un disco.

## Traducción al español: estado

| Texto | Estado |
|---|---|
| Menú (partidas, opciones, controles) | hecho: `tools/lang/menu.txt` (clave \| inglés \| español), `g_lang`, fuente con tildes y Ñ |
| Intro | a medias: `tools/lang/intro_es.txt` existe pero vacío; `make_menu_data.py` arma las páginas en los dos idiomas, pero la intro nativa (`intro.c`) dibuja el texto con los tiles del SNES y no las usa |
| Cajas de mensaje (objetos), EMERGENCY..., PLANET ZEBES, GAME OVER, pausa, mapa, final | falta: salen de tiles del SNES (`kMsgTiles` y otros) |

Propuesta para que sea fácil: un solo archivo `tools/lang/es.txt` con todo el texto del juego por clave (número de
mensaje del SNES, escena), el inglés de referencia sacado de la ROM al compilar, y un conversor que dibuja cada texto
en tiles con la fuente del SNES más las tildes y la Ñ propias. El motor elige la tabla por `g_lang` (que hoy solo ve
el menú: pasarlo a la capa de la Genesis y a la SRAM). Nada se traduce mientras se juega.
