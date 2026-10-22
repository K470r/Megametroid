# 34 — Escape de Ceres hasta el ascensor (motor nativo y Genesis)

## Qué hay

`MegaMetroid_Ceres_nativo.bin` (y `_demo.bin`) llega ahora hasta el final del escape de la estación: después de la
huida de Ridley, el texto "EMERGENCY / SELF DESTRUCT SEQUENCE ACTIVATED / EVACUATE COLONY IMMEDIATELY", la cuenta
regresiva de 60 segundos, las cinco salas en su estado de escape (con escombros y vapor), el pozo girando en modo 7
y Samus subiendo al ascensor. Ahí la pantalla se funde a negro (estado 33) y queda en negro (estado 34).

Lo que sigue (estado 34 en adelante: la nave saliendo de Ceres, la explosión de la estación y, si el tiempo llega a
cero, la explosión con Samus adentro y el fin de juego) está en `35_CINEMATICAS_CERES.md`.

## Motor (`native/src`)

| Qué | Dónde |
|---|---|
| Estados del escape de Ridley (`Ridley_Func_54`-`62`), texto con máquina de escribir, ciclo de color | `ceres_ridley.c` |
| Contador: `ProcessTimer`, `DrawTimer`, `DecrementDecimal` | `samus_move.c` |
| Vapor de Ceres (enemigo E1FF), puerta que explota | `ceres_door.c` |
| Escombros que caen (proyectiles 9734/9742), polvo E509, choque de proyectiles con bloques fiel al decomp | `eproj.c` |
| Pozo: balanceo (`g_word_89AD5F`), `Mode7Rotate` para sprites y explosiones, estado 32 al subir al ascensor | `door.c`, `nmath.c`, `proj.c` |
| Estados 32-34: fundido y negro | `game.c` |

Verificación: `check` con 0 diferencias del cuadro 97 al 8296 en las 9 grabaciones. En la Genesis
(`native_compare.py`, 9000 cuadros) las variables del motor son iguales al SNES en los 8620 cuadros comparados.
`native_compare` ahora descarta las muestras con el motor a mitad de un cuadro (`md_busy`): en un cuadro perdido
el emulador puede cortar con el cuadro siguiente a medias, y eso no es una diferencia.

## Genesis (`native/md`, `mkmd.py`)

- **Salas del escape.** Cada sala tiene otro estado en la huida (E097, E04D, E003, DFB9, DF71), con su captura de
  VRAM del SNES (`snes_ref/<sala>_<estado>.bin`, no se distribuye).
- **Texto.** No es del BG3 (HUD): el juego lo escribe en el tilemap del BG1 (0x5000) con tiles de 4 bits que carga al
  empezar el escape (`$B7:DA00` en 0x7820). `mkmd` toma esos tiles de la captura de la huida y los pone en el banco de
  la sala E0B5; `kEscText` lleva palabra del SNES → celda del plano A. Sus colores (paleta 5, que el juego pone en
  verde al empezar, y 97-99, que cambian solos) no se juntan con otros en el reparto de colores.
  Corregido de paso: `Ridley_Func_55` y `Ridley_Func_61` cambiaban `palette_buffer` sin avisar a la Genesis.
- **Rojo.** En el SNES el haze rojo (Ridley ya se fue) suma con el color math un degradado de rojo a todo, de 0 arriba
  a 15/31 abajo. La Genesis no puede cambiar todos los colores línea a línea: el color de fondo lleva el degradado
  (interrupción horizontal, como en la pelea) y el resto de los colores, menos el HUD, un rojo fijo (+2 de 7).
- **Pozo en modo 7.** Rotación con scroll horizontal por línea y vertical por columnas de 16 píxeles. Al girar, el
  SNES muestra lo que hay fuera del pozo en el mapa de modo 7: una pared de 2x2 tiles que se repite. La Genesis usa
  para esa sala un plano de 64x64: columnas 32-63 la pared, 0-31 el pozo (96 filas) en anillo de 64 filas que sigue
  a la cámara (filas nuevas por DMA en el vblank; las celdas de la puerta del fondo se vuelven a poner si su fila se
  recarga).
- **Contador.** Sprites del banco $80 (tipo de clave 8), con los tiles de la captura de la huida.
- **Fundido final.** Brillo desde `INIDISP` en los estados 32-34 (la CRAM se escala al subirla).

## Velocidad

Cuadros perdidos en la grabación del bot (Genesis Plus GX), por tramo de cuadros del motor:

| Tramo | Antes | Ahora |
|---|---|---|
| Ceres hasta Ridley (100-3000) | 6 | 6 (entradas a sala) |
| Pelea (3000-5017) | 18 | 0 |
| Texto y primeras salas (5228-6580) | 11 | 4 |
| Sala de las cápsulas (6580-6980) | 235 | 91 |
| Pozo (7100-8223) | 552 | 87 |

Cambios:

1. **Compilación**: `-O3 -funroll-loops -finline-limit=1500` en lugar de `-O2` (variable `OPT` de `build.sh`). El
   motor es C con muchas funciones chicas por objeto; integrarlas ahorra más que nada. Comparado con el SNES: 0
   diferencias.
2. **Tabla de rotación** a mano: la parte entera de cada línea con `swap` en 16.16 (37 ciclos por línea en lugar de
   ~80). VRAM idéntica.
3. Lo que queda en la sala de las cápsulas es carga real (11 vapores, ~8 escombros, disparos): ~1 cuadro de 68000 por
   cuadro. Probado y descartado: alargar la espera del vblank tardío hasta la línea 250 (gana poco y duplica los
   vblanks que se pasan a la imagen).

Mediciones: `LAGLOG=1 LAG_BASE=4000 sh build.sh` registra desde el cuadro 4000 del motor.
