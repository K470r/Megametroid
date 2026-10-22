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

## Lo que sigue (en este orden)

| Paso | Ganancia medida | Costo / riesgo |
|---|---|---|
| Recortar las copias de bancos (`kBankXXB`, 782 KB) a lo que se lee | sin medir | todas las lecturas pasan por `A6W`/`BW_86`...: medir en `host/` con las grabaciones |
| LZ4W sobre el depósito (bloques de 4 KB) | depósito 1266 → ~940 KB | búfer de 4 KB en la puerta (`cinematic_text_tilemap`); peor sala ~49 bloques: 0,2-0,4 s |
| `make size`: parte fija libre y llenado de cada página | — | que falle antes de pasarse |

No se puede comprimir lo que se lee al jugar (celdas, `l2`, claves, level data: ~5 MB): no hay RAM donde
descomprimirlo (el SNES tiene 128 KB; la Genesis 64 KB, casi llenos).

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
- Hace falta primero la música del cartucho (hoy solo las voces, docs/40). Propuesta acorde a las reglas: convertir
  las secuencias N-SPC de la ROM con una tabla de instrumentos FM/PSG/PCM nuestra (la tabla va en el repositorio; las
  secuencias salen de la ROM de cada quien).
- El Z80 no sabe qué página del SSF2 está puesta: la música del cartucho tiene que estar en una ventana que no cambie
  mientras suena. Propuesta: la ventana 6 (0x300000-0x37FFFF) con la página del conjunto de música, cambiada solo en
  `music_upload` (como el SNES sube un conjunto por área). Para eso hay que liberar esa parte de la parte fija (pasos
  de arriba).
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
