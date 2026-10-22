# 39 — Velocidad: la meta es la del SNES

Objetivo del proyecto: que en la Genesis se juegue con la misma fluidez que en el SNES (60 cuadros por segundo, sin
cuadros perdidos). Este documento lleva las mediciones.

## Cómo se mide

- `LAGLOG=1` (`md_lag_log`): cuántos vblanks tardó cada cuadro del motor (1 = a tiempo).
- `EXPFL=-DMD_BUSYLOG=N`: líneas de la consola que usa cada cuadro del motor desde el cuadro N (`md_busy_log`, 512
  cuadros). El cuadro tiene 262 líneas y el trabajo del vblank va aparte: arriba de ~245 se pierde un cuadro.
- `STOP_AT=N` congela el motor en el cuadro N: con dos ROM (N = A y N = B) y el perfilador del emulador
  (`gpgx_prof`, `MM_PROF`), la diferencia es el perfil exacto de los cuadros A..B, por función. Así se comparan dos
  versiones del motor en los mismos cuadros del juego.
- Siempre con la entrada de una grabación del SNES (`TEST=1`): la lógica es la misma cuadro a cuadro.

## 14 de octubre

Al sumar Zebes, el motor creció y en Ceres se notó (la pelea con Ridley perdía cuadros que antes no):

| Causa | Arreglo |
|---|---|
| `enemy_def` buscaba en 148 definiciones (antes 10) muchas veces por cuadro | caché de las últimas por dirección |
| Objetos de sprite (explosiones sueltas) portados: dos recorridos de 32 lugares por cuadro | se saltan mientras no haya ninguno vivo |
| `PlmHandler`: el recorrido de los 40 lugares pasó a ser lento (registros) | búsqueda aparte del PLM activo siguiente; pre-instrucciones vacías sin llamada |
| Nivel en la ROM con bloques cambiados aparte: cada lectura miraba el mapa de bits | si la sala no tiene bloques cambiados, una comparación y la ROM |
| Fundidos de la intro: 256 colores cada 4 cuadros | solo los que cambiaron |
| Colisión de proyectiles enemigos con pendientes metida en el bucle (registros guardados en la pila) | las rutinas de pendiente aparte (`noinline`): −4 líneas por cuadro en la huida |

Resultado (grabaciones de los bots):

| Tramo | Versión del 9 de octubre | Antes del arreglo | Ahora |
|---|---|---|---|
| Pelea con Ridley (5450 cuadros) | 0 | 44 | 0 |
| Huida, sala con escombros (DF8D), por cuadro | ~250 líneas | ~264 | ~257 |

La sala de los escombros de la huida ya perdía cuadros en la versión anterior (los escombros son proyectiles con
colisión contra el nivel, muchos sprites). Lo que queda en ese tramo es lógica nueva y fiel al juego (tiles animados,
efectos de ambiente, el contador, la mezcla de capas): hay que bajar el costo de lo que más pesa — dibujo de sprites
(`md_drawkey`), enemigos (`EnemyMain`), proyectiles enemigos (`EprojRunAll`) — para llegar a 0.

Pruebas de opciones del compilador en la huida (líneas medias por cuadro): `-O3 -funroll-loops -finline-limit=1500`
(la de siempre) 252; `-O2` 286; `-O3` sin desenrollar 254; límites de inline 300/600: 257/255. Se queda la de siempre.

Lo que más pesa en la sala de los escombros (líneas por cuadro): dibujo de sprites `md_drawkey` 35 (18 llamadas,
~950 ciclos cada una), proyectiles enemigos 29 (los escombros: 9-11 a la vez, con colisión vertical contra el
nivel), enemigos 27, el resto del cuadro del juego 20, vblank 14, `md_frame` 13, dibujo de proyectiles 17.

## Cuadros en cadena (14 de octubre, segunda parte)

Medido con `MD_BUSYLOG`: en la huida los cuadros cuestan ~200 líneas de media, pero varían mucho (de 160 a más de
280). Con el esquema de antes (lógica → esperar el vblank → escribir en el VDP → lógica...) cada cuadro tenía que
caber entero en su cuadro de la consola, y el tiempo que le sobraba a uno liviano se perdía esperando.

Ahora el cuadro tiene dos partes y el vblank va en la interrupción vertical (como el NMI del SNES):

1. `md_logic`: la lógica del motor. Lo que escribe para el VDP (lista de sprites, colas de los ganchos: tiles de
   Ridley, texto de la huida, cañón, cuadro de la huida, bloques cambiados y tiles animados de Zebes) va a su propio
   lado: la lista de sprites tiene dos búferes, las colas una copia de la lógica.
2. `md_wait_free`: espera a que la interrupción haya tomado el cuadro anterior.
3. `md_post`: pasa las colas de la lógica a las del vblank y arma lo demás (colores, HUD, scroll, columnas de Zebes).
4. `md_publish`: el cuadro queda listo; la interrupción vertical lo escribe en el VDP (`md_vint` → `md_vblank`, con
   la página del nivel puesta y la que estaba al volver). Si el vblank ya empezó sin cuadro y queda tiempo (hasta la
   línea 234), se escribe en el momento.

Los ganchos que cambian lo que lee el vblank sin copia (sala nueva, cinemáticas, inicio de la intro) primero esperan
a que se tome el cuadro anterior (`md_sync`).

Cuándo empieza la lógica siguiente (`pipe_wait`, `md_main.c`): si el cuadro anterior costó más de `PIPE_H` (200)
líneas, en seguida (usa lo que sobró del cuadro publicado); si no, cuando el vblank toma el cuadro o faltan
`PIPE_TS` (40) líneas para el vblank. Así la entrada se lee como en el SNES (un cuadro antes de verse) y solo en los
tramos pesados un poco antes (simulado: +0,2 a +0,4 cuadros en esos tramos, a cambio de no perder imágenes).

Para que entrara en la RAM: la intro, la capa del modo 7 de las cinemáticas y las salas de Zebes comparten
`md_scratch` (solo uno de los tres se usa a la vez).

Comprobación: la lógica es la misma (`MD_STATESUM` contra el motor del PC: 256 cuadros iguales en la huida, la
pelea y Zebes) y las imágenes congeladas con `STOP_AT` son idénticas píxel a píxel a las de antes (Ceres, huida,
intro, Zebes).

Cuadros perdidos (grabación `full`, 2000 cuadros de la huida desde el 6000; `r11`; intro entera):

| Tramo | Antes | Ahora |
|---|---|---|
| Huida, cuadros 6000-8000 de `full` | 218 | 37 |
| Huida, `r11` 8000-8800 | 70 | 10 |
| Intro (8900 cuadros) | 37 | 2 |

Los que quedan son cuadros pesados después de uno liviano, o dos pesados seguidos (y los cambios de sala, con la
pantalla en negro). Lo que más pesa ahora en la huida (líneas por cuadro): `md_post` 40 (el giro del pozo: 20,
scroll por línea de los dos planos), `md_vblank` 23, `EnemyMain` 22, `game_frame` 19, `md_drawkey` 16.
