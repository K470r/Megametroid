# 41 — Lo aprobado no se rompe: prueba de regresión de la imagen

Regla: una escena que Edwin revisó y aprobó queda "congelada". Antes de entregar una ROM se corre
`tests/regress/regress.sh`; si una escena aprobada cambia, la entrega no sale hasta que el cambio esté explicado
(y aprobado) o deshecho.

## Cómo funciona

- Cada escena es una partida grabada del SNES (`tests/regress/inputs/*.s`, solo los botones de cada cuadro): la
  ROM de prueba la juega igual, cuadro a cuadro.
- Genesis Plus GX corre la ROM y `retrorun` (`hashseq`) anota en cada cuadro de la consola el hash de la imagen y
  qué cuadro del motor se ve (`md_shown_frame`, lo pone el vblank al tomar un cuadro).
- Se compara por cuadro del motor, no por cuadro de la consola: si una optimización cambia cuándo se muestra un
  cuadro, se sigue comparando la misma imagen.
- `golden/<escena>.txt`: cuadro del motor -> hash de la imagen aprobada.

| Escena | Qué cubre | Cuadros |
|---|---|---|
| `intro` | intro completa: diario, recuerdos, fundidos, vuelo a Ceres | 8453 |
| `ceres` | Ceres entera, pelea con Ridley, huida, Ceres explotando, viaje y llegada a Zebes | 10721 |
| `zebes` | lo anterior + Samus sale de la nave, camina y dispara en la sala del aterrizaje | 11203 |

## Uso

    tests/regress/regress.sh              # todas las escenas
    tests/regress/regress.sh intro        # una
    UPDATE=1 tests/regress/regress.sh X   # aprobar el estado actual de X (solo después de que Edwin lo revise)

La salida dice cuántos cuadros cambiaron y en qué tramos (cuadros del motor). Con esos números se compara con
`STOP_AT=N` la imagen de antes y la de ahora.

Lo que el título y el menú muestran no entra todavía (se juegan con el mando, no con una grabación).

Cada sala de Crateria que se apruebe se agrega como escena nueva.

## Diferencias que no son errores

El hash es de la imagen exacta. Si el código se hace un poco más lento o más rápido, en algunos cuadros cambian cosas
que dependen del momento exacto y no del cuadro del motor: la línea en que la interrupción horizontal cambia el color
del degradado de Ceres (una línea más arriba o más abajo) o una letra del texto de la intro que sube un cuadro después.
Antes de aprobar se comparan esas imágenes con las de la versión aprobada (compilada desde la copia de la carpeta del
usuario): si solo son eso, se aprueba (`UPDATE=1`).

## Formato por líneas (desde octubre)

La referencia (`golden/<escena>.rows.gz`) guarda, para cada cuadro del motor, un hash de 16 bits por cada línea de
la imagen (`retrorun rowhashseq`). Así un cuadro que cambia en 1-3 líneas sueltas (hasta 4 tramos) se informa como
"corrimiento de una línea" (la interrupción horizontal cayó una línea antes o después) y no falla; sí falla si la
misma línea cambia en más del 20 % de los cuadros (un cambio de verdad, como una línea de otro color) o si cambia más
que eso. `MD=` permite correrla con otra copia del motor (para armar la referencia de una versión aprobada).

### 2026-10-20 (crateria4): referencias aprobadas de nuevo
- intro: 9 cuadros con una fila de texto que aparece un cuadro antes o después (cambio de velocidad: el banco $8C pasó a
  bytes para que la parte fija de la ROM entre en 4 MB). Mismo contenido.
- ceres/zebes: desde Ridley, la alarma roja de la huida y algunos cuadros quedan desfasados: el motor tarda un poco más
  (manejadores nuevos de Samus: ascensor, muerte) y la Genesis perdió 1-2 cuadros en toda la partida; los efectos que
  van por cuadros de pantalla quedan corridos. En el PC, todas las grabaciones del SNES siguen con 0 diferencias.

### 2026-10-20 (crateria5, optimización, docs/44): aprobadas de nuevo
- intro: textos un cuadro antes o después (el motor va más rápido). ceres/zebes: la alarma de la huida y la explosión
  de Ceres corridas un cuadro (comparadas lado a lado con crateria4: mismo contenido). En el PC, todo en 0.

### 2026-10-20 (crateria6): aprobadas de nuevo
- intro, ceres, zebes: 2-3 cuadros corridos uno (el código compilado sin el paso bswap de GCC cambia de velocidad).

### 2026-10-21 (crateria7): aprobadas de nuevo
- intro, ceres, zebes: mismo contenido corrido unos cuadros (comparado lado a lado con crateria6: la nave de la intro,
  el reloj de la huida 41"44 contra 41"49). El código nuevo (haz del ojo, bola) cambia la velocidad.
- Después, con el estado inicial rehecho desde las grabaciones (tools/inputs/make_inputs.sh; el de antes era del 14 de
  octubre y no tenía las variables nuevas): la alarma roja de la huida corrida un cuadro. Aprobadas.
