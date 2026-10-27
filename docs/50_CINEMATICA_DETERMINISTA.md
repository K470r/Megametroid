# 50 — Cinemática de Ceres: la misma imagen en cada cuadro del motor

La explosión de Ceres y el viaje a Zebes (cuadros del motor ~7100-9900 de la escena `ceres`, docs/41) arman cada
imagen de la capa del modo 7 en la mitad oculta del plano A, en varios vblanks (docs/35). Antes, cuándo se cambiaba
a la imagen nueva dependía de cuánto vblank sobraba (`m7_fits` → `vb_lines_left()`): con un solo `nop` más en el
vblank, la regresión de Ceres pasaba de 19 a 112 cuadros distintos (docs/49, "Regresión"), y no servía para ver si un
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
| cuadros perdidos en la cinemática (7000-9900, `perdidos`) | 106 | 97 |
| cuadros perdidos en el resto de Ceres / Zebes | 65 / 29 | 66 / 29 |
| secuencia de imágenes del modo 7 con 1, 2, 50, 300 `nop` en el vblank | cambia | igual (0 esperas) |

Cuadros perdidos: cuadros de la consola que repiten el cuadro del motor (la grabación de `regress.sh`).

Costo de una fila: en Genesis Plus GX cada DMA corta (una fila de ~30 celdas) tarda 2-3 líneas, y escribirla con el
68000 lo mismo; 16 tiles (256 palabras), unas 5. Por eso `M7_ROW_UNITS` es 128 y no los 48 de `M7_ROW_COST` (que
sigue siendo el margen de `m7_fits`). Presupuestos probados (esperas con 0 / 300 / 600 `nop`): 700/450 → 0/1/14;
650/400 → 0/0/3; 600/400 → 0/0/1, pero sigue peor a la imagen pedida (268).

## Regresión: lo que todavía cambia con el tiempo

Con un `nop` en `md_cone_vblank` (`EXPFL=-DMD_VB_NOPS=1`), contra la referencia nueva, quedan 9-40 cuadros
"distintos" según cómo quede alineado el código (antes 112). Ninguno es de la imagen del modo 7:

- **Borde derecho del círculo de la explosión** (8628-8700): la interrupción horizontal que escribe el scroll vertical
  de cada línea llega unos ciclos antes o después según qué instrucción corre el 68000, y en algunas líneas el borde
  derecho (x ≥ 190) sale con el de la línea vecina (hasta 19 píxeles). Es el mismo tipo de corrimiento que docs/41
  acepta para las líneas, pero de lado; `regress.sh` lo cuenta como distinto porque son muchas filas.
- **Alarma roja de la huida** (7497, 7548, 7956-7958): un vblank que empieza tarde (`md_publish`, camino "tarde")
  termina dentro de la imagen (medido: hasta la línea 0x26) y ese cuadro no tiene el degradado del haze.
- Filas sueltas del degradado (8002-8178): corrimientos de una línea, ya aceptados.

Lo mismo pasa en la intro: el vuelo a Ceres (`md_intro.c`, `ship_vblank`) también carga con lo que sobra del vblank
(`vb_lines_left`), y con este cambio 7780-7785 muestran un tamaño de la nave un cuadro antes.

## Herramientas

- `tools/perf/m7.sh [salida.csv]` (`XFL=...`): por cuadro de la consola, el cuadro del motor, la imagen pedida y la
  que se ve, el paso del armado, las líneas del vblank al entrar y salir del armado, las esperas y cómo empezó el
  vblank (`MD_M7LOG`).
- `EXPFL=-DMD_VB_NOPS=n`: el vblank tarda n instrucciones más (prueba de sensibilidad al tiempo).
- `M7_BUDGET`, `M7_BUDGET_CIRC`, `M7_ROW_UNITS` se pueden cambiar con `EXPFL`/`XFL` para medir.

## Aprobación

Las imágenes nuevas no están aprobadas: después de mirarlas lado a lado (rama de hoy contra la nueva), `UPDATE=1 sh
tests/regress/regress.sh intro ceres zebes` (docs/41). Contra la referencia aprobada: intro 5 distintos (7780-7785),
ceres y zebes 240 (5118 y la cinemática 7105-9868; 5118 es el primer cuadro del degradado al empezar la pelea con
Ridley, que ahora sale un cuadro antes).
