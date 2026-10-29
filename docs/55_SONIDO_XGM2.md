# 55 — Música y efectos del cartucho (XGM2): de la intro a la llegada a Zebes

Fecha: 2026-10-28. Rama `intro-audio`. Opción elegida por Edwin: **XGM2 de SGDK** (la A de la propuesta).

Qué suena:
- **Desde la intro hasta que la nave llega a la órbita de Zebes**: la música y todos los efectos, en los mismos
  momentos que el SNES (los pide la misma cola del motor).
- **Desde la llegada a Zebes**: sin música; solo los efectos de Samus y de las puertas.
- Las voces de la intro siguen sonando (ahora son pistas PCM del XGM2).

## Qué pide el SNES (decomp y grabaciones)

El motor ya tiene las colas del SNES: `HandleMusicQueue` ($80:8F0C, `src/core/door.c`) llama a
`music_upload(conjunto)` y `music_play(pista)`; `HandleSoundEffects` ($82:89EF, `src/core/sfx.c`) a
`sfx_play(biblioteca, efecto)`. Lo que pide cada parte se sacó corriendo el motor del PC sobre las grabaciones del
SNES de `make_inputs.sh` con `MM_SNDLOG` (nuevo en `host/pc_stubs.c`: cuadro del SNES, estado, pedido):

    MM_SNDLOG=work/snd/log_intro.txt host/check work/inputs/ref_intro.bin sm/sm.smc 0 99999
    MM_SNDLOG=work/snd/log_full.txt  host/check work/inputs/ref_full.bin  sm/sm.smc 97 99999

| Cuadro SNES | Estado | Pedido | Qué es (función del decomp) |
|---|---|---|---|
| título 363 | 1 | $03, 5 | el título (`CinematicFunctionOpening` $8B:9B68; `src/front/title.c`, ver abajo) |
| al salir del título | 1 | $03, 6 | el menú de partidas (`HandleCinematicsTransitions_1` $8B:9A83) |
| 2068 / 2167 | 30 | conjunto $3F, pista 5 | "The last Metroid is in captivity" (`CinematicFunction_Intro_Initial` $8B:A395) |
| 2375 / 2390 | 30 | $42, 5 | "The galaxy is at peace" (`CinematicFunc_Intro_QueueGalaxyIsAtPeace` $8B:A613) |
| 2678 / 2693 | 30 | $36, 5 | la intro: diario y recuerdos (`CinematicFunc_Intro_SetupTextPage2` $8B:A66F) |
| 9459 | 30 | pista 0 | silencio (`CinematicFunction_Intro_Func15b` $8B:B207) |
| 9774 / 9789 | 30 | $2D, 5 | el vuelo a Ceres (`CinematicFunction_Intro_Func54` $8B:BCA0) |
| 10617 | 8 | $2D, 6 | Ceres (sala) |
| 12413 / 13057 | 11 / 8 | $24, 5 | la pelea con Ridley |
| 15759 | 8 | $24, 7 | la huida |
| 18831 / 18846 | 34 | $2D, 7 | Ceres explota y el viaje a Zebes (`cinematic.c`) |
| 20649 | 6 | $06 | llegada a Zebes: desde aquí, sin música |

Efectos en la intro: biblioteca 3 $0D (el tecleo del diario, 337 veces), 3 $23 (la larva), 2 $0B (el huevo),
1 $32 (Samus en el recuerdo de Mother Brain). En Ceres: 28 distintos (lista en `tools/snd/efectos.txt`).

## Cómo se hace (todo en el PC, desde tu ROM)

El decomp trae el motor de sonido del SPC700 en C (`sm/src/spc_player.c`, N-SPC, con el DSP de `snes/dsp.c`).
`tools/snd/spcrun.c` lo compila y lo corre con la ROM de cada quien: sube el motor y los efectos (`$CF:8000`) y los
conjuntos de música (`$8F:E7E1`), escribe en los puertos como `RtlApuWrite` y genera 534 muestras por cuadro (como
`RtlRenderAudio`). Por cuadro anota cada voz del DSP (muestra, pitch, volumen, envolvente, KON/KOF), la lista de
patrones (`music_ptr_toplevel`), el efecto de cada biblioteca y su prioridad; y graba el audio.

`tools/snd/mksnd.py` (lo llama `make data`, en lugar del viejo `mkvoice.py`) arma `res/gen/md/md_snd_data.s` y
`md_voice.s`:

- **Música** (`tools/snd/musica.txt`: qué pistas): cada voz del SPC, cuadro a cuadro, pasa a un canal de la Genesis:
  5 FM (el 6 es el DAC del PCM), 3 tonos del PSG y el ruido. Cada nota nueva toma un canal libre de su clase (el que
  usó antes esa voz, si está libre; si no, se le quita a la más baja). La altura sale del pitch del DSP y de la altura
  medida de la muestra BRR (`tools/snd/brr.py`: autocorrelación, ajustada a un número entero de ciclos en el lazo);
  el volumen y la envolvente, del volumen y la ganancia del DSP (TL de los portadores, de a 0,75 dB); el paneo de
  VOLL/VOLR (izquierda, derecha o centro). El lazo: cuando la lista de patrones vuelve atrás o, si no cambia (un
  patrón que se repite), el período de las notas. Sale un VGM y `xgm2tool` lo pasa a XGM2.
- **Instrumentos** (`tools/snd/instrumentos.txt`, en git: es nuestra). Por defecto, **FM aditiva**: las 4 armónicas
  más fuertes de la muestra, una por operador (algoritmo 7); sin altura clara (percusión, ruido), el ruido del PSG.
  La tabla corrige a mano cualquier muestra: parche FM completo, PSG, ruido, octava, altura, volumen. Hoy tiene una
  corrección: el ambiente de Ceres (muestra 24 de $2D), que la autocorrelación medía tres octavas arriba.
- **Voces** (pistas "pcm"): la pista tal cual sale del N-SPC (con su eco), PCM de 8 bits a 13 300 Hz.
- **Efectos** (`tools/snd/efectos.txt` + `tools/snd/sfxlist.py`): cada efecto se graba solo, sin eco, como PCM de
  8 bits a 13 300 Hz (o a 6 650 Hz si casi no tiene agudos: menos del 2 % de la energía arriba de 3 kHz). Los bordes
  que quedan 40 dB debajo del pico no se guardan. Los que no terminan (el SPC los repite con $FB) se guardan en dos
  partes, la entrada y el lazo, encontrado en el estado de los canales de efectos (`sfxst`); si sus canales tienen
  lazos de distinto largo, el del más largo. Los que usan instrumentos del conjunto cargado (Ridley 2/$59, la larva
  3/$23, 3/$24) se graban con ese conjunto. La prioridad de cada uno sale de `kSfxNConf` del decomp.
- **Qué suena en Zebes** (`tools/snd/sfxlist.py`): los efectos que pide el código de Samus, de los proyectiles, del
  HUD y de las puertas (`src/core/samus_*.c`, `proj.c`, `hud.c`, `door.c`, con las tablas que leen: los disparos de
  cada rayo) y los de las listas de instrucciones de los PLM de puertas (cabeceras con `PlmSetup_Door_*` y
  `PlmSetup_C794_GreyDoor`, y las de `kDoorClosingPlmIds` $8F:E68A, recorridas con `plmscan.py`): 3/$07 y 3/$08.

Tamaños (con la ROM de Japón / EE. UU.): música XGM2 40 KB (las 6 pistas FM: 0,8-13 KB cada una); voces 69 KB;
efectos 71 (51 distintos) 285 KB.

## En la Genesis (`src/platform/md_snd.c`)

- `md_snd_init`: carga el XGM2 (en lugar del controlador PCM de antes).
- `music_upload` / `music_play`: como `Music_HandleCmdFromSnes` (la misma pista no vuelve a empezar; la 0 calla).
  Las pistas FM con `XGM2_play`; las voces con `XGM2_playPCMEx` en el canal 1. Si el conjunto no tiene música
  convertida (Zebes), silencio. El conjunto se toma también de `music_data_index` (una partida que empieza de un
  estado guardado, como las ROM de prueba, ya lo trae puesto).
- `sfx_play`: un canal PCM por biblioteca (1, 2, 3), con las reglas de `Sfx1/2/3_HandleCmdFromSnes`: un efecto con
  prioridad no se corta salvo con los de silencio (1/2 en la 1; $71 y $7E en la 2; 1 en la 3), y en la 3
  `some_value == 2` bloquea todo salvo el 1. Si el efecto tiene lazo, `md_snd_frame` (desde `md_logic`, solo si hay
  uno pendiente) empieza el lazo cuando termina la entrada.
- **Bus del Z80**: el vblank ya no le pide el bus al Z80; le avisa con la bandera del XGM2 (`XGM2_IN_DMA`, como
  `Z80_setBusProtection` de SGDK, sin tocar las interrupciones: `md_snd_protect`) antes y después de sus DMA. El Z80
  sigue tocando desde su búfer y no lee la ROM durante las DMA.
- **Dónde está**: todo en la parte fija, debajo de 0x380000 (el Z80 no ve la página del SSF2). Las muestras ocupan
  primero el lugar que tenían las voces de antes (`md_voice.s`, `.rodata`, el mismo tamaño: 37 632 + 28 160 bytes)
  para no correr las direcciones de lo que sigue; lo demás va en `.late_rodata` (después de la pausa, antes del
  título). Lo común termina en 0x318AAC (antes 0x2BED50); el título sigue terminando debajo de 4 MB.
- **Mega CD** (docs/48): `music_play` le pasa primero la pista a `snd_cd_play(conjunto, pista)`; hoy devuelve 0.
  Cuando esté el programa del Mega CD, si devuelve 1 la pista sale del CD y el XGM2 no toca música (los efectos
  siguen en el cartucho).
- `EXPFL=-DMD_SND_MUTE`: la ROM con el XGM2 cargado pero sin pedir nada (para medir).

## Lo medido

Audio: `RR_WAV=archivo.wav` en `tools/bin/retrorun` (nuevo: graba lo que suena) sobre las ROM de prueba de
`regress.sh`, contra el SNES: `tools/snd/snesref.py` toca con el N-SPC del decomp los pedidos de `MM_SNDLOG`.

| | resultado |
|---|---|
| altura (croma de 12 notas, Genesis contra SNES) | intro 0,63, Ridley 0,48, huida 0,37: el máximo siempre sin transponer |
| envolvente de volumen (Ceres, cada 0,1 s) | correlación 0,69 |
| mezcla | voces y música en la misma proporción que el SNES (la música se subió 5 dB: `MM_MUS_DB`, hoy -4) |
| el ambiente de Ceres | 0-50 Hz: 174,8 dB contra 177,1 del SNES (antes de la corrección de la tabla, 160,7) |
| Zebes | la música del viaje hasta la órbita; después silencio y solo los efectos de Samus |

Cuadros perdidos (cuadros de la consola que repiten el cuadro del motor, en las grabaciones de `regress.sh`; docs/54):

| | referencia aprobada | con sonido |
|---|---|---|
| intro | 42 | 46 |
| Ceres hasta 7000 / huida y cinemática / Zebes | 70 / 110 / 30 | 66 / 84 / 18 |
| Zebes (escena zebes) | 70 / 110 / 69 | 66 / 83 / 48 |

El vblank pierde menos que antes porque ya no espera el bus del Z80.

### Regresión de imagen

    intro: 8453 cuadros comparados, 0 distintos
    ceres: 10677 cuadros comparados, 0 distintos
    zebes: 11145 cuadros comparados, 0 distintos

Con el sonido, el Z80 lee la ROM por el bus del 68000 y le roba ciclos: dos partes de Ceres que dependían de cuánto
tardaba el 68000 dejaron de coincidir con la referencia (50 y 48 cuadros). Lo que se hizo (decidido por Edwin):

1. **La huida de Ridley, determinista** (`ga_post` / `ga_vblank` en `md_engine.c`, como el modo 7 y la nave de
   docs/54). Antes cargaba los tiles de cada cuadro con lo que sobraba del vblank y cambiaba las celdas cuando
   estaban y cabían: con el Z80 sonando terminaba un cuadro más tarde (5126-5128). Ahora lo decide la lógica en
   `md_post`: los tiles cuestan 16 palabras cada uno y cada cuadro del motor después del pedido cuenta `GA_STEP` =
   1920 (los 120 tiles que entraban en un vblank); sin tiles, el cambio va en el mismo cuadro. Las filas del cambio
   van enteras si caben en un vblank (`GA_FULL_VBLANK`) o de a partes fijas por cuadro del motor. Los tiles se siguen
   cargando con lo que sobra del vblank (van a la mitad que no se ve); si al cambiar faltara alguno, el vblank los
   termina (`EXPFL=-DMD_GAWAIT` cuenta esas esperas en `md_ga_wait`: 0, también con 50 y 300 `nop` en el vblank).
   Un cuadro que trae filas del cambio solo se toma con el vblank empezado si quedan las líneas que piden
   (`ga_job_lines` en `md_publish`).
   Con sonido, mudo (`MD_SND_MUTE`) y con 50 `nop` la huida sale igual cuadro a cuadro. Contra la referencia vieja
   cambió solo el 5127: el cuadro en que Ridley se va salía con los colores de Ridley en la línea 1 (rayas rojas en
   las paredes durante un cuadro); ahora la paleta cambia en el mismo cuadro que las celdas. Edwin aprobó la
   referencia nueva (`UPDATE=1 ... ceres zebes`).
2. **El borde derecho del círculo de la explosión** (8628-8701, docs/54): la interrupción horizontal escribe el scroll
   de cada línea y, según qué instrucción corre el 68000, llega antes o después del dibujo de la línea; el ancho de esa
   línea sale con el de la vecina. Comparadas con `main` en el mismo cuadro del motor, solo cambian los escalones del
   borde derecho; `main` sin sonido con 7 o 50 `nop` en el vblank también cambia ahí (11 y 21 cuadros). Se probó
   esperar el borrado horizontal en la interrupción (en todas las líneas o solo en las del borde): no lo vuelve
   determinista (Genesis Plus GX dibuja la línea entera de una vez) y cuesta 44-144 cuadros. Arreglarlo de raíz sería
   dibujar el círculo con tiles armados en cada cuadro (como el haz del ojo, sin interrupción por línea), pero en los
   polos el borde cambia mucho en 8 líneas: más de 100 tiles por cuadro en el vblank más cargado. Por ahora
   `regress.sh` cuenta los cuadros 8620-8705 de ceres y zebes aparte ("N en el borde del círculo de la explosión
   (no falla)"); fuera de esa ventana todo sigue contando.

Cuadros perdidos con todo esto (misma medida de arriba): intro 42 (referencia 42); Ceres 68 / 84 / 18 (70 / 110 / 30);
Zebes 68 / 86 / 48 (70 / 110 / 69).

## Herramientas

| | |
|---|---|
| `tools/snd/spcrun.c` | el N-SPC del decomp en el PC (guion: `up`, `p`, `f`, `ram`, `noecho`, `sfxst`) |
| `tools/snd/mksnd.py sm.smc res/gen/md [trabajo] [--analizar]` | todo el sonido; `--analizar` lista cada muestra (altura, armónicas, voces) |
| `tools/snd/brr.py` | muestras BRR: decodificar, altura, armónicas |
| `tools/snd/sfxlist.py sm.smc` | los efectos de Samus y de las puertas |
| `tools/snd/snesref.py sm.smc registro.txt salida.wav [desde hasta]` | el audio del SNES de referencia |
| `MM_SNDLOG=archivo host/check ...` | los pedidos al SPC por cuadro |
| `RR_WAV=archivo.wav tools/bin/retrorun ...` | graba lo que suena en la Genesis |
| `tools/snd/fui2parche.py instrumento.fui [CONJ SRCN] [--escribir] [--envolvente-snes]` | un instrumento FM de Furnace (.fui) a una línea de `instrumentos.txt` |
| `make fur` (`tools/snd/mkfur.py sm.smc [carpeta] [CONJ PISTA...]`) | las canciones del juego como proyectos de Furnace (`work/furnace/*.fur`) |
| `tools/snd/fur2tabla.py proyecto.fur [--escribir] [--envolvente-snes] [--revisar]` | los instrumentos FM de un proyecto de Furnace a `instrumentos.txt` |
| `make snd` | rehace solo el sonido (`mksnd.py`) y la ROM (~1 min) |
| `EXPFL=-DMD_GAWAIT` | cuenta en `md_ga_wait` los cambios de la huida que tuvieron que terminar de cargar tiles |
| `EXPFL=-DGA_STEP=n` | presupuesto de la huida por cuadro del motor (1920) |
| `MM_MUS_DB`, `MM_SFX_GAIN` | (entorno de `mksnd.py`) volumen de la música FM y de los efectos |

## Ajustar los instrumentos a oído con Furnace

1. Elegir el instrumento: `python3 tools/snd/mksnd.py sm/sm.smc res/gen/md work/snd --analizar` lista cada muestra
   (`# CONJ SRCN: tipo f0=... notas=... voces=[...] armónicas=[...]`) y la pista donde suena, y guarda cada muestra
   del SNES sola en `work/snd/muestras/CONJ_SRCN.wav` (en hexadecimal sin `0x`: `03_25.wav`): 2 s a su altura
   natural, con su lazo. Es lo que hay que imitar.
2. En Furnace, un instrumento **FM (OPN)** (YM2612): armarlo escuchando el original (esa muestra, o la pista entera
   del SNES: `tools/snd/snesref.py`). Guardarlo con "Save instrument" (.fui, el formato nuevo o el viejo).
3. `python3 tools/snd/fui2parche.py coro.fui 0x36 24 --escribir` escribe en `tools/snd/instrumentos.txt` la línea
   `0x36 24 tipo=fm parche=...` (reemplaza la de ese conjunto y muestra, o la agrega). Se le pueden sumar claves
   (`oct=-1 vol=-3`). Avisa lo que la tabla no usa: operadores apagados (van con TL 127), AM, SSG-EG, LFO.
4. `make snd` y escuchar `out/rom.bin`.

Detalles:
- Furnace guarda los operadores en el mismo orden que la tabla (los registros: S1, S3, S2, S4) y el detune de 0 a 7
  con el centro en 3: `fui2parche.py` lo pasa al registro con la tabla de Furnace (`dtTable`, 7 6 5 0 1 2 3 4).
- La envolvente: el SNES manda el volumen de cada nota y `mksnd.py` lo pone en el TL de los **portadores**. La
  envolvente propia de un portador (ar, d1r, sl, d2r, rr) se suma a esa: si se quiere solo la del SNES, con
  `--envolvente-snes` los portadores van con ar=31 d1r=0 sl=0 d2r=0 rr=15. Los **moduladores** llevan su TL y su
  envolvente tal cual (son el timbre).
- Arreglado al hacer esto: al cambiar de parche, `mksnd.py` ponía TL=127 en los cuatro operadores y después solo
  subía el de los portadores, así que en un parche con algoritmo 0-6 los moduladores quedaban mudos (no se notaba:
  los instrumentos automáticos son de algoritmo 7, todos portadores). Ahora el 127 va solo en los portadores. Con la
  tabla de hoy el sonido es el mismo byte a byte; con un parche de algoritmo 0 los moduladores llevan su TL.
- `ONLY_SND=1 sh tools/conv/make_data.sh` solo hace el sonido si falta (lo usa el `Makefile`): para rehacerlo después
  de cambiar la tabla, `make snd`.

## Las canciones enteras en Furnace (`make fur`)

Para elegir los instrumentos escuchando la canción completa: `make fur` escribe en `work/furnace/` un proyecto de
Furnace por canción del juego (37: las de las áreas, los jefes, las cinemáticas, el título, los créditos y las cuatro
del motor; sin las voces PCM ni las pistas que en el SPC son silencio). Salen de tu ROM: no van a git.

Qué tiene cada proyecto (`tools/snd/mkfur.py`):
- El sistema Genesis (YM2612 + PSG), 60 filas por segundo (velocidad 1: una fila por cuadro del SNES), patrones de
  256 filas y el lazo de la canción (`0Bxx` al final; `0D00` donde termina la parte que no se repite).
- Las notas **tal como las arma la ROM**: `convert_track` (`mksnd.py`) anota lo que suena en cada canal de la Genesis
  (su parámetro `trace`) y `mkfur.py` lo pasa a filas: nota e instrumento al tocar, `OFF` al soltar, el volumen del
  SNES en la columna de volumen (FM: 127 menos la atenuación en pasos del TL; PSG: 15 menos la atenuación), la altura
  fina con `E5xx` (1/128 de semitono: el vibrato y los deslizamientos), `EAFF`/`EA00` (nota ligada) cuando la altura
  se va más de un semitono sin volver a tocar, `08xy` el paneo y `2001` en el ruido (blanco, con sus 3 velocidades:
  C, C# y D).
- Un instrumento por muestra del SNES: `03_25` (conjunto $03, muestra 25), con el parche de hoy de
  `instrumentos.txt`. Los del PSG y el ruido se llaman `03_25 psg` / `03_24 ruido`.
- El formato de Furnace 0.6.8.1 (versión 228, bloque `INFO`): lo abren esa versión y las siguientes.
- Además, en `work/furnace/`: `instrumentos_por_cancion.txt` (los instrumentos de cada canción, su tipo y cuántas
  notas tocan) y `muestras/CONJ_MUESTRA.wav` (cada muestra del SNES sola, como `--analizar`).

Cómo se usa:
1. `make fur` y abrir en Furnace, por ejemplo, `work/furnace/03_5_titulo.fur`. Play: suena la canción entera.
2. Cambiar los instrumentos FM (los que se llaman `CONJ_MUESTRA`) escuchando. Las notas del proyecto no se usan
   para la ROM (salen del SPC): lo que se cambia ahí no llega al juego.
3. Guardar el proyecto (con cualquier nombre) y pasar los instrumentos a la tabla, todos de una vez:
   `python3 tools/snd/fur2tabla.py work/furnace/03_5_titulo.fur --envolvente-snes --escribir`.
4. `make snd` y escuchar `out/rom.bin`. Un `make fur` después rehace los proyectos con los instrumentos nuevos (y
   las otras canciones del mismo conjunto ya los traen).

`fur2tabla.py` lee el `.fur` comprimido o no, del formato viejo (`INFO`) o del nuevo (`INF2`, Furnace en
desarrollo), y pasa cada instrumento FM con `fui2parche.to_parche`: la misma línea que guardarlo como `.fui`.

Comprobado (2026-10-28), sin Furnace en el contenedor (compilarlo no se permitió: código de afuera):
- `fur2tabla.py --revisar` lee los proyectos campo por campo como el cargador de Furnace
  (`src/engine/fileOps/fur.cpp`, rama `INFO`): versión 228, YM2612 + PSG, 10 canales, punteros, carpetas, patrones.
- Desde los patrones, siguiendo órdenes y saltos, la nota + `E5xx`, el volumen y las notas encendidas de cada canal
  en cada cuadro dan lo mismo que la traza de la ROM: las 37 canciones, 0 distintos.
- Los parches: proyecto → `fur2tabla.py` → la misma línea que el instrumento de la tabla (detune incluido).
- `mksnd.py` con la traza: la música de la ROM sale igual byte a byte.
- Oído en Furnace por Edwin: suena como en la ROM.

## La percusión (2026-10-29)

Las muestras que van al ruido del PSG (platillos, golpes) son grabaciones que se apagan solas y no tienen lazo: el
DSP corta la voz cuando la muestra termina. Antes la Genesis tomaba solo la envolvente ADSR de la voz (que se queda
arriba mientras la tecla sigue apretada) y el ruido sonaba parejo ("lluvia"). Ahora (`convert_track`, `amp`):
- cada voz lleva su posición en la muestra (`vpos`: `pitch` × 32000 / 4096 por segundo, desde la nota) y el volumen
  se multiplica por el de la grabación en ese punto (RMS cada 64 muestras, `Instr.senv`); al terminar la muestra
  (sin lazo) la voz se suelta, como en el DSP;
- la velocidad del ruido (`noise_rate`) es la de las 3 del PSG cuyo brillo (centroide del espectro de |sinc| hasta
  16 kHz) se parece más al de la muestra tocada a esa altura. `ruido=0..2` en la tabla la fija (antes `ruido=0` no
  se podía pedir).
- En el proyecto de Furnace cada golpe es una nota nueva (las notas del PSG y del ruido no llevaban la marca de nota
  nueva y los golpes seguidos se juntaban en uno).

Cambian las pistas con percusión: título y menú ($03), la intro ($36), Ridley y la huida ($24), el vuelo a Ceres y el
viaje a Zebes ($2D). El `03_24` del título no era percusión: es un teclado grave de 100 Hz (`instrumentos.txt`).

## Lo que falta

- Escuchar y ajustar la tabla de instrumentos a oído (la FM aditiva es un punto de partida medido, no un arreglo).
- El eco del SNES (la música lo usa mucho) no está: la Genesis no tiene eco.
- La música del resto del juego: agregar líneas en `musica.txt`. Con todo el juego no entra en la parte fija: pasar
  la música a la ventana 6 (página cambiada solo en `music_upload`, docs/48), para lo que hay que bajar lo común
  debajo de 0x300000 (o el título a una página).
- Los efectos de los enemigos en Zebes (hoy, solo Samus y puertas).
- El programa del Mega CD (`snd_cd_play`).

## El título y el menú de partidas (2026-10-28)

El título no corre el motor (es `src/front/title.c`, con la línea de tiempo grabada del SNES), así que pide la
música él mismo, como el SNES:
- **Pista 5 del conjunto $03** desde el cuadro de la línea de tiempo en que el SNES la escribe en el puerto del SPC
  (`g_title_music_frame`, 363). Lo calcula `make_title_data.py` (`music_frame`) desde `title.rec`:
  `CinematicFunctionOpening` pide `QueueMusic_Delayed8(0xFF03)` y `(5)`; `HandleMusicQueue` sube el conjunto 8
  cuadros después (el contador de cuadros se detiene los 39 que tarda `APU_UploadBank`) y escribe la pista 8 más tarde.
  El logo de arranque va sin música, como en el SNES. Si se salta al título antes de ese cuadro, la música empieza al
  llegar a la pantalla de título; al volver a empezar la secuencia, vuelve a empezar la música.
- **Pista 6** (el menú de partidas) al terminar el fundido a negro de START. La intro la corta al pedir el $3F.

`find_loop` (`mksnd.py`) busca ahora el período de las notas también desde el último patrón: la pista del título es una
entrada que no se repite (cuadros 0-1511) y un patrón que repite solo (4909 cuadros). Sin eso la grababa entera hasta
`MAX_FRAMES`. Las otras pistas salen iguales. Tamaño: título 8,5 KB, menú 4,5 KB.

Comprobado: en Genesis Plus GX (`RR_WAV`), la música se oye desde el cuadro 369 del título (el 363 más la primera
nota); `regress.sh` 0 distintos; lo común termina en 0x341FCC.

