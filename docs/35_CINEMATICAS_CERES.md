# 35 — Ceres explota: huida en la nave, tiempo agotado y fin de juego

## Qué hay

Las dos salidas del escape de Ceres, completas:

- **Samus sube al ascensor a tiempo** (estados 33 → 34/37): la nave sale de la estación, Ceres explota (el círculo
  blanco/amarillo de la explosión), la nave se acerca a la pantalla, "PLANET ZEBES" y el viaje hasta Zebes (modo 7 y
  mosaico). Termina al empezar a cargar Zebes (estado 6), que es lo próximo a portar.
- **El contador llega a cero** (estados 35/36 → 37 → 25/26): fundido a blanco, la estación explota con Samus
  adentro, fundido a negro y el menú "GAME OVER / FIND THE METROID LARVA! / TRY AGAIN? / YES / NO" con la bebé
  metroide y el misil que marca la opción. Por ahora YES y NO reinician la consola (todavía no hay título ni
  partidas guardadas).

ROMs: `MegaMetroid_Ceres_nativo.bin` (para jugar), `_demo.bin` (entrada de la grabación del escape completo, hasta
Zebes) y `_demo_tiempo.bin` (la misma partida, pero sin tocar el control al final del escape: se acaba el tiempo).

## Motor (`native/src`)

| Qué | Dónde |
|---|---|
| Objetos de HDMA del SNES: lista de instrucciones, pre-instrucciones, haze, scroll del BG3, la bomba de poder (pre-explosión, explosión, resplandor, círculo escalado) | `hdmaobj.c` |
| Efectos de paleta (banco $8D) | `palfx.c` |
| Cinemáticas del banco $8B: objetos-sprite, funciones de la cinemática, matriz del modo 7, fundidos | `cinematic.c` |
| Fin de juego (estados 25/26) y el menú | `gameover.c` |
| Estados 33-37, 25, 26, 31 y 0xFFFF | `game.c` |
| Tiempo agotado (estado 35, paleta a blanco) | `samus_move.c` |

La RAM 7E:198D-1C1E es de las cinemáticas en unos estados y de los proyectiles de enemigos en otros
(`variables_extra.h`). En el motor son variables separadas; `check` compara la "dueña" según el estado del juego al
empezar el cuadro. `nt_clear_ram(desde, hasta)` borra por direcciones del SNES con la tabla generada `g_ram_vars`.

Verificación: `check` con 0 diferencias en las 10 grabaciones (`ref_full` hasta el cuadro 10103, donde empieza la
carga de Zebes; `ref_timeup` hasta el final del menú).

## Genesis (`native/md`)

### Salas sin nivel

Las cinemáticas y el menú usan "salas" preparadas por `mkmd.py` solo con sus sprites: C001 (Ceres explota), C002
(viaje a Zebes) y C0FF (menú: además el BG1 fijo del texto). No tienen HUD: los sprites van 2 líneas más arriba y los
planos con scroll vertical +2 (medido contra el SNES). La línea 0 de la CRAM queda libre para la capa del modo 7
(`free_line`). Con más de 4 piezas por sprite se juntan las de la misma paleta (la Genesis tiene 64 sprites en H32).

### Capa del modo 7: imágenes ya dibujadas

`mkm7.py` dibuja en el PC, desde la trayectoria del SNES (`snap_cine.py` → `snes_ref/cine_traj.json`), cada imagen
de la capa que hace falta: la estación, la nave al explotar, Ceres explotando con Samus, el viaje a Zebes y Zebes con
mosaico. Una imagen sirve mientras la escala y el ángulo cambian poco (`ESCALA`, `ANGULO`; la nave al explotar crece
rápido: `ESCALA_VAR`). Cada imagen: 14 colores, como mucho 200 tiles (`TILES_MAX`, se juntan los parecidos) y su
mapa de celdas. La Genesis la busca por la matriz del motor (`m7_lookup`).

**Doble búfer.** El plano A (64x64) tiene dos mitades de 32 filas. La imagen que se ve está en una; la siguiente se
arma en la otra en uno o varios vblanks: sus tiles (en la otra mitad de la zona de tiles), después se borra lo que
quedó de la anterior y se escriben sus celdas. Cuando está lista, se cambia de mitad con el scroll vertical (+256) y
sus colores en el mismo vblank: no hay cuadros a medio cambiar. La fila lógica Y de la mitad b es la física
`(Y + 32 b) & 63`; las filas que pisarían la imagen que se ve no se escriben (`m7_row_ok`).

El trabajo del vblank se mide con el contador de líneas en cada paso (`m7_fits`): el 68000 queda parado en cada DMA,
así que el contador ya incluye lo hecho. Con el presupuesto calculado de antemano el vblank se pasaba hasta 60 líneas
a la imagen.

### Círculo de la explosión: scroll por línea

En el SNES el círculo es una ventana por línea (tablas de bordes izquierdo y derecho) con color math. En la Genesis:

- El plano B tiene un dibujo fijo: el píxel (X, Y) tiene el color 15 si X <= Y (un triángulo de 512x512, con 2
  tiles: lleno y diagonal).
- Con el scroll horizontal por línea, cada línea muestra su fila desde la columna "izquierda"; la fila elige el
  ancho (Y = derecha - izquierda). Las líneas sin círculo muestran las columnas 256-511 de una fila < 256 (vacías).
- La fila de cada línea se elige con el scroll vertical del plano B, que cambia una interrupción horizontal en cada
  línea. La interrupción es código armado en RAM (`cv_code`): copia una palabra de la tabla al puerto del VDP y
  avanza la dirección (~128 ciclos). La tabla es doble (una se muestra, otra se arma) y la interrupción vertical
  vuelve al principio si el cuadro se atrasa.
- El color 15 de la línea 0 es COLDATA. Bordes exactos, sin tiles por cuadro. Las 2 líneas vacías junto a cada
  tramo toman la fila del borde, para que un desfase de una línea entre los dos scrolls no deje una raya.

Lo que no se puede igual: en el SNES el círculo suma color a todo lo que tiene debajo (sprites incluidos); en la
Genesis es un color sólido detrás de los sprites y de la capa del modo 7.

### Menú de fin de juego

BG1 fijo de la sala C0FF y el tilemap del motor (`menu_tilemap`); el degradado verde-azul de abajo con la misma
interrupción horizontal del haze (`kMenuTeal`, aproximado: 3 bits por color).

## Velocidad

Cuadros perdidos (Genesis Plus GX), de la salida del ascensor a la carga de Zebes: 83 → 9.

1. Fundidos: el brillo de la CRAM dividía por 15 con la división de 32 bits de libgcc (192 por cuadro, ~50 líneas):
   ahora una tabla `kBright[16][8]`.
2. Círculo: de tiles armados por cuadro (hasta 570 líneas de 68000 con el círculo grande) al scroll por línea
   (~30 líneas para la tabla más la interrupción). La tabla no se rehace si el radio, el centro y el scroll son los
   mismos.
3. Bomba de poder en el motor: el relleno de las líneas sin círculo con `memset` (mismo resultado).
4. Vblank que se pasaba a la imagen: ver `m7_fits`.

Quedan ~7 cuadros perdidos en el pico de la explosión (el motor calcula las dos bombas de poder y ~40 sprites).

Medición: `tools/emu_harness/gpgx_mm_profile.patch` (perfil de PC del 68000 en Genesis Plus GX: `MM_PROF=archivo`).

## Lo que sigue

La carga de Zebes (estado 6) y el aterrizaje en Crateria.
