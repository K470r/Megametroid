# 49 — El ojo (MorphBallEye) y su haz de luz: lo que estaba mal y cómo quedó

Salas: 9A90 (Crateria), 9E9F (la bola) y 9F64. Código: `src/platform/md_cone.c`, `tools/conv/mkzkeys.py`,
`res/snes_ref/zdraw.log`.

## Lo que hace el SNES (decomp)

- `MorphBallEye_Init` ($A8:9058): dos enemigos, el ojo y su pieza (`parameter_2 & 0x8000`, `nullsub_244`).
- `MorphBallEye_Func_3` ($A8:9160): cada cuadro calcula el ángulo hacia Samus y pone la lista de instrucciones del
  ojo **calculada**: `((ángulo & 0xF0) >> 2) - 28756` = `$A8:8FAC + 4 * i` (16 listas de un cuadro, 14 spritemaps
  distintos, `$91DF`-`$923A`).
- El haz: HDMA `$88:E8EC` con la ventana 1 por línea (`CalculateXrayHdmaTableInner`, tabla 7E:9100) y
  `LayerBlendFunc_10`: TM = BG1+BG2+OBJ, TS = BG3 (un tile negro opaco), TSW = BG3 (W34SEL = 2), CGWSEL = 2 (sumar la
  subpantalla), CGADSUB 0x33. Fuera de la ventana se suma negro; dentro el BG3 sale de la subpantalla y queda el
  color fijo (COLDATA, amarillo que late). Se aclaran BG1, BG2, sprites de las paletas 4-7 y el fondo. La sala entera
  cambia de color al abrirse el ojo también en el SNES (comparado con `hlref`).

## Por qué desaparecía parte del ojo

No eran prioridades, ni sprites enmascarados, ni el límite por línea: **faltaban las claves de dibujo** de 12 de
los 14 cuadros del ojo. `mkzkeys.py` solo seguía listas con nombre (`addr_kX_Ilist_...`) y tablas `g_off_`; la lista
calculada de `Func_3` no la veía. Con Samus debajo o arriba del ojo el spritemap pedido (p. ej. `$922C`) no estaba
y el sprite del ojo (2x2, el azul) no se dibujaba. Volcado de la tabla de sprites del VDP: con `$9225` los tres
sprites del ojo, con `$922C` solo dos.

Arreglo: `mkzkeys.py` reconoce `current_instruction = ((v & máscara) >> n) +- base` en el código de la familia y
recorre todos los valores; `zdraw.log` suma las 36 claves (12 spritemaps en las 3 salas).

## Por qué el haz se veía mal

Medido en 9A90 (`tools/perf` no hizo falta: se contó desde volcados de RAM y VRAM):

| | antes | ahora |
|---|---|---|
| tiles de borde que pide un haz (7E:9100) | 57-63 (36-38 distintos) | igual |
| tiles armados disponibles | 32 (`0xFC00`) | 2 bancos de 43 a 85 (9A90: 77; 9E9F: ~70) |
| bandas que se quedaban sin tiles | las de abajo: escalones de 8 px | ninguna en las 3 salas |
| sprites del haz / por línea | 41-49 de 64; ≤10 por línea, ≤28 de 32 celdas | igual (sin recortes del VDP) |
| DMA de tiles al cambiar de haz | 1 KB de una vez | ≤ 24 tiles (768 B) por vblank, solo si sobra vblank |
| cuadros perdidos en 600 (9A90 / 9E9F / 9F64) | 0 / 3 / 0 | 0 / 3 / 0 |
| haces nuevos en 600 cuadros (9A90, caminando) | 18 | 17 |

El haz se dibuja como antes (modo sombra/luz, sprites "luz" con la forma de la tabla del SNES), pero los tiles
armados ya no se acaban:

- **Tres tramos de VRAM** vistos como lugares seguidos: los tiles libres después de los objetos de la sala
  (`zebes_obj_end()` hasta el 1279; en 9A90, 68), los 32 de `0xFC00` y los 54 de las filas 5-31 del plano Window
  (no se ven: la ventana solo muestra el HUD; los usa también la caja de mensaje, después se recarga el haz).
- **Dos bancos**: el haz que se muestra usa uno y el nuevo se arma en el otro. Así sus tiles se suben **de a poco**
  en cada vblank (`md_cone_vblank`, con `md_vb_lines_left()`: como mucho `CONE_UP_MAX` = 24 tiles y solo con 16
  líneas de margen) y el cambio no espera ni carga un vblank. Un banco toca como mucho dos tramos: `cone_build`
  (ensamblador) recibe `split`, `base_lo`, `base_hi` y no deja que un sprite de 2-4 tiles cruce el corte.
- El búfer del banco que se arma (85 tiles como mucho) y el segundo búfer de sprites van en el texto de las
  cinemáticas (`cinematic_text_tilemap`, bytes 1024-4095; en Zebes la caja de mensaje usa los bytes 512-1003).
  `md_cone_idle` solo arma mientras `md_cone_frame` pide el haz en ese cuadro (`cone_live`), para no escribir ahí
  en la muerte o en una cinemática.

Lo que sigue distinto del SNES: aclara hacia el blanco (el SNES suma amarillo), no aclara a Samus ni a los enemigos,
no late, y el haz sigue a Samus con unos cuadros de demora (se arma por pasos con el tiempo libre).

## Métodos que se evaluaron (y por qué no)

- **Plano dedicado (Window o B) con tiles por línea**: con sombra/luz un plano solo decide sombra o normal, y por
  celda de 8x8 (bit de prioridad); aclarar solo lo hacen los sprites operador. Armar copias aclaradas de los tiles
  de la sala para la zona del haz serían ~350 celdas (11 KB de VRAM y de DMA por haz): no hay lugar. El Window,
  además, reemplaza al plano A.
- **Interrupción H por línea**: un cambio de colores vale para toda la línea, no para el tramo [izquierda, derecha];
  cambiar la CRAM a mitad de línea deja puntos y el 68000 no tiene precisión de píxel. ~20 % del cuadro.
- **Haz simple de 8 px** (solo tiles llenos): barato pero con escalones; es lo que pasaba cuando se acababan los
  tiles.

## Para los rayos X

Usan la misma tabla (`CalculateXrayHdmaTableInner`, ya armada por pasos en `hdmaobj.c`/`xray_job_run`) y la misma
forma: sirven `cone_bands`/`cone_build`, los bancos y la subida por vblank tal cual. Falta lo propio de los rayos X
(los bloques ocultos dentro de la ventana: `HandleLayerBlendingXray*`).

Referencias que vio Edwin en otros juegos de la Genesis (para cuando se haga):
- *The Adventures of Batman & Robin*: enemigos con linternas que iluminan unos pocos píxeles y un helicóptero que
  persigue al Batwing con un foco grande.
- *Ranger X*: linternas que iluminan una parte del escenario al pasar, **y al personaje**. Con sombra/luz un sprite
  operador no aclara a los sprites de abajo; para aclarar a Samus dentro del haz habría que hacer algo así (p. ej.
  cambiar su paleta o sus colores mientras está dentro), como hace el SNES (OBJ, paletas 4-7).

## Regresión

`regress.sh`: intro 0 distintos. En la parte de Zebes (desde el cuadro 10650), igual que la base. En Ceres hay más
cuadros "distintos" (114 contra 19), todos en la cinemática de la explosión y el viaje (7105-9421, 10047). No es el haz,
que ahí no corre: la cinemática arma cada imagen con el vblank que sobra (`cine_vblank`, `m7_fits`) y muestra la
anterior si no llega, así que cambia con cualquier diferencia de tiempo. Control: la ROM base con **un solo `nop`**
en `md_cone_vblank` da 112 distintos; con relleno de código o de RAM que no se ejecuta, los mismos 19. Ver también
docs/48 (la misma sensibilidad en la llegada a Zebes y el pozo). Hay que mirarlos lado a lado antes de aprobar
(`UPDATE=1`, docs/41): no se aprobaron.

Resuelto en docs/50: la imagen del modo 7 de cada cuadro del motor ya no depende del tiempo.
