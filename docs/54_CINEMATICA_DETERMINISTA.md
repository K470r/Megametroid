# 54 — Cinemática de Ceres: la misma imagen en cada cuadro del motor

La explosión de Ceres y el viaje a Zebes (cuadros del motor ~7100-9900 de la escena `ceres`, docs/41) arman cada
imagen de la capa del modo 7 en la mitad oculta del plano A, en varios vblanks (docs/35). Antes, cuándo se cambiaba
a la imagen nueva dependía de cuánto vblank sobraba (`m7_fits` → `vb_lines_left()`): con un solo `nop` más en el
vblank, la regresión de Ceres pasaba de 19 a 112 cuadros distintos (docs/52, "Regresión"), y no servía para ver si un
cambio empeoraba esa parte.

## Qué cambió (`src/platform/md_engine.c`)

1. **Cuándo se cambia de imagen lo decide un presupuesto fijo.** Al empezar a armar una imagen se calcula su costo
   (`m7_cost_of`: tiles × 16 + filas de celdas y filas que hay que borrar, cada una `M7_ROW_UNITS` = 128 más su
   ancho). Cada cuadro del motor descuenta `M7_BUDGET` = 650 (con el círculo de la explosión, `M7_BUDGET_CIRC` =
   400: su tabla de scroll va en el mismo vblank). El cambio se hace cuando el presupuesto llega a 0 **y** la imagen
   está armada. El presupuesto solo depende del motor, así que la imagen de cada cuadro del motor es siempre la misma.
   El armado de verdad (`m7_build`) sigue usando todo lo que sobra del vblank (`m7_fits`) y va adelante; si alguna vez
   no llegara, el cambio espera (no debería pasar: `MD_M7LOG` lo cuenta en `md_m7_wait`).
2. **Se arma también en los vblanks sin cuadro nuevo** (`md_vblank_lag`, desde `md_vint`): cuando el motor se atrasa
   (el pico de la explosión), esos vblanks quedaban libres. Con la página del nivel y el bus del Z80 como en el vblank
   normal (`vb_enter`/`vb_exit`); no arma mientras `cine_frame` o `cine_start` cambian el estado (`m7_lock`).
3. **Si el motor pide otra imagen que cuesta menos de lo que le falta a la que se arma, se empieza la pedida.** Antes
   la nave al explotar (imágenes 46-61, ~7500 palabras cada una) se terminaba de armar aunque el motor ya pidiera el
   destello (imagen 62, sin tiles): en 8670-8690 se veía la nave gigante pixelada sobre el destello rojo. Lo que la
   dejada ya escribió en la mitad oculta se borra con lo de antes (el rectángulo `m7_bx0..m7_bh` cubre las dos).
4. **El fundido (`kBright`) se calcula en `md_post` y no en el vblank** (`cram_bright`). En los fundidos se comía
   ~25 líneas del vblank cada dos cuadros y el vblank terminaba dentro de la imagen (hasta la línea 12): ahí el armado
   no tenía nada. Si el vblank carga una sala (`room_pending`), lo calcula él como antes.

## Lo medido

| | antes | ahora |
|---|---|---|
| imagen distinta de la que pide el motor (7000-9900) | 253 cuadros | 230 |
| imagen nueva en la explosión (fin de la nave gigante) | 8691 | 8676 |
| cuadros perdidos (ver abajo, con los vblanks tardíos) | 65 / 106 / 29 | 68 / 106 / 30 |
| secuencia de imágenes del modo 7 con 1, 2, 50, 300 `nop` en el vblank | cambia | igual (0 esperas) |

Cuadros perdidos: cuadros de la consola que repiten el cuadro del motor (la grabación de `regress.sh`).

Costo de una fila: en Genesis Plus GX cada DMA corta (una fila de ~30 celdas) tarda 2-3 líneas, y escribirla con el
68000 lo mismo; 16 tiles (256 palabras), unas 5. Por eso `M7_ROW_UNITS` es 128 y no los 48 de `M7_ROW_COST` (que
sigue siendo el margen de `m7_fits`). Presupuestos probados (esperas con 0 / 300 / 600 `nop`): 700/450 → 0/1/14;
650/400 → 0/0/3; 600/400 → 0/0/1, pero sigue peor a la imagen pedida (268).

## La nave del vuelo a Ceres (intro, `md_intro.c`)

El mismo problema y la misma solución. `ship_vblank` carga el cuadro de la nave con lo que sobra del vblank y antes lo
mostraba en cuanto terminaba. Ahora `ship_update` (en `md_post`) decide cuándo se muestra: cada cuadro de la nave
cuesta sus tiles más el mapa (`ship_bytes`) y cada cuadro del motor cuenta `SHIP_STEP` = 2400 bytes (el vblank de un
cuadro que carga un grupo nuevo de la intro no cuenta). Se muestra en el vblank del cuadro en que el presupuesto
alcanza, si ya está cargado. `ship_pick` elige el cuadro que alcanza a llegar con ese presupuesto.

Medido (`tools/perf/nave.sh`): la carga real hace ~90 tiles (~2900 bytes) por vblank. Con 4000 había 5 esperas, con
3000 una, con 2400 ninguna, ni con 300 `nop`. La nave sigue igual de cerca de la que pide el motor: 344 cambios de
cuadro en el vuelo contra 363 de antes, y 383 cuadros con otra nave que la pedida contra 384.

## El haze y los vblanks tardíos

Tres arreglos (`md_engine.c`), que juntos dejan la alarma de la huida y el degradado de Ceres sin depender del tiempo:

1. **Un cuadro que llega con el vblank empezado se toma solo si alcanza.** `md_publish` lo tomaba hasta la línea 0xEA
   (`LATE_MAX`). Un 0xEA de la segunda vuelta es la línea 240 y deja 22 líneas. En el pozo de la huida, la DMA de la
   tabla de scroll (448 palabras) caía en la imagen, donde la DMA es lenta, y el vblank terminaba en la línea 0x26-0x42.
   Ahora se toma si quedan `md_vb_need` líneas: lo que tardó el último vblank más 2, que baja de a una por cuadro
   (las cargas de sala y de grupos de la intro, con la pantalla apagada, no cuentan). Si no alcanza, el cuadro va
   entero en el vblank siguiente. El armado del modo 7 en los vblanks sin cuadro deja libres esas líneas (`m7_keep`).
2. **Cuadro que se ve dos veces.** En un vblank sin cuadro nuevo, `md_vint_top` reponía el color de arriba y la tabla,
   pero no la primera interrupción. La última del cuadro deja el registro 10 en 0xFF, así que la segunda vez se veía
   sin el degradado. Pasaba en todos los cuadros repetidos con haze: en la grabación de Ceres, 106 cuadros repetidos
   tenían dos imágenes distintas; ahora 18 (las cargas de sala y el borde del círculo).
3. **Interrupción pendiente.** Al encender las interrupciones horizontales al final del vblank, una pendiente de antes
   se atendía ahí mismo y se comía la primera banda: las bandas salían corridas. `md_hint` ahora no hace nada si
   todavía es el vblank, igual que `md_hint_obj`.

## Regresión: lo que todavía cambia con el tiempo

Con 1, 2, 7 y 50 `nop` en `md_cone_vblank` (`EXPFL=-DMD_VB_NOPS=n`), contra la referencia nueva:

- intro: 0 cuadros distintos (antes, con 50 `nop`, 10 en el vuelo);
- ceres: solo el **borde derecho del círculo de la explosión** (8628-8702, 21-39 cuadros según el caso). La
  interrupción horizontal que escribe el scroll vertical de cada línea llega unos ciclos antes o después según qué
  instrucción corre el 68000, y en algunas líneas el borde derecho (x ≥ 190) sale con el de la línea vecina (hasta 19
  píxeles). Es el mismo tipo de corrimiento que docs/41 acepta para las líneas, pero de lado; `regress.sh` lo cuenta
  como distinto porque son muchas filas. Se podría contar como corrimiento: queda para que Edwin decida.

Cuadros perdidos (cuadros de la consola que repiten el cuadro del motor, grabación de `regress.sh ceres`):

| | antes | ahora |
|---|---|---|
| Ceres hasta 7000 | 65 | 68 |
| huida y cinemática (7000-9900) | 106 | 106 |
| Zebes | 29 | 30 |

Los 3+1 de más son cuadros tardíos que ya no se toman a medias.

## Herramientas

- `tools/perf/nave.sh [salida.csv]`: lo mismo para la nave del vuelo a Ceres en la intro (`md_ship_wait`).
- `tools/perf/m7.sh [salida.csv]` (`XFL=...`): por cuadro de la consola, el cuadro del motor, la imagen pedida y la
  que se ve, el paso del armado, las líneas del vblank al entrar y salir del armado, las esperas y cómo empezó el
  vblank (`MD_M7LOG`).
- `EXPFL=-DMD_VB_NOPS=n`: el vblank tarda n instrucciones más (prueba de sensibilidad al tiempo).
- `M7_BUDGET`, `M7_BUDGET_CIRC`, `M7_ROW_UNITS` se pueden cambiar con `EXPFL`/`XFL` para medir.

## Aprobación

Las imágenes nuevas no están aprobadas: después de mirarlas lado a lado (rama de hoy contra la nueva), `UPDATE=1 sh
tests/regress/regress.sh intro ceres zebes` (docs/41). Contra la referencia aprobada:

- intro: 41 distintos, todos en el vuelo (7738-7802): tamaños de la nave en otro cuadro.
- ceres y zebes: 232 distintos. 5118-5126 es el degradado al empezar la pelea con Ridley; 7105-7950 son la alarma de
  la huida (ahora en todos los cuadros); desde 8186, la cinemática.

## Después (docs/55)

La huida de Ridley (`ga_*`) también quedó determinista con el mismo método (presupuesto por cuadro del motor), y
`regress.sh` cuenta aparte el borde derecho del círculo (cuadros 8620-8705): con sonido el Z80 le roba ciclos al 68000
y esa parte ya no puede coincidir cuadro a cuadro.
