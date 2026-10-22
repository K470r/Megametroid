# 31 — Motor nativo para la Genesis (Ceres)

## Por qué

El intento anterior (docs/30) pasaba el código del decomp a la Genesis de forma mecánica: la consola corría las
funciones del SNES con una capa que imitaba la RAM, los registros, el DMA y el OAM del SNES. Funcionaba igual que
el SNES pero iba a la mitad o a un tercio de la velocidad y el dibujo traducía en cada cuadro.

Decisión (Edwin): **todo el código se reescribe a mano para la Genesis**. La lógica se lee del decomp (la biblia) y
se escribe en C para el 68000; los datos se preparan en el PC. La Genesis no interpreta ni convierte nada del SNES.
El port mecánico queda solo como referencia en el PC.

## Estructura (ver docs/47)

| Carpeta | Qué hay |
|---|---|
| `src/core/`, `inc/core/` | el motor: C escrito a mano, igual en PC y en la Genesis |
| `res/gen/engine/` | lo generado: variables, tablas de la ROM, enumeraciones (no se edita) |
| `tools/conv/` | `gen_vars.py` (variables), `mkdata.py` (tablas de la ROM del usuario) |
| `host/` | `check.c`: comprobación contra el SNES cuadro a cuadro |
| `src/platform/`, `inc/platform/` | la capa de la Genesis: dibujo, VDP, mando, sonido |

### Reglas del motor

- **Nombres del decomp.** Funciones y variables se llaman igual que en snesrev/sm, con la dirección del SNES en
  un comentario (`// 0x90A383`), para poder seguir la biblia línea por línea.
- **Variables nativas.** `gen_vars.py` busca en `src/core/` e `inc/core/` los nombres de `variables.h` del decomp y declara cada uno
  como variable global con su tipo (uint8/uint16; arreglos con su tamaño). Las que no tienen nombre en el decomp
  van en `src/core/vars_extra.txt` con su dirección (`uint8 samus_x_decel_mult_hi @0xB4D`).
- **Mismas cuentas.** Aritmética de 16 bits con desborde igual al 65816 (`uint16`/`int16`, `add_hilo`,
  `PAIR32`...). Los defectos del original se conservan.
- **Punteros del SNES.** Lo que el juego guarda como puntero de 16 bits a la ROM (p. ej. la tabla de retardos de
  una pose) sigue siendo la dirección del SNES (así se compara la RAM); `mkdata.py` emite esos datos como bloque
  con su base y el motor resta (`bank91_ptr`). Las palabras ya están en el orden del 68000.
- **Lo que no está portado** llama a `nt_unimpl("...")` (en el PC lo informa; en la Genesis se verá en pantalla).
  Así nada queda a medias en silencio.

### Tablas (`mkdata.py`)

Desde la ROM: parámetros de pose ($91:B629), transiciones por botones ($91:9EE2, aplanadas con índice), retardos
de animación ($91:B010 + bloque de bytes), velocidades ($90:9F49), spritemaps por pose ($92:9263/945D), poses de
giro, paletas de Samus ($9B), tiles de Samus por cuadro ($92:D94E), cañón del brazo ($90:C7DF). Desde el decomp
(arreglos de C): las tablas de pendientes de sm_94.c.

## Comprobación (`host/check`)

`check ref.bin rom.smc desde hasta [modo]`: carga el estado del SNES de la grabación de `hlref` (RAM por cuadro),
corre el motor con la misma entrada y compara todas sus variables con la RAM del SNES.
- modo 0 (libre): estado del SNES solo al empezar.
- modo 1: estado del SNES antes de cada cuadro.
Los sistemas aún no escritos (enemigos, paletas de la sala, HUD, cola de sonido de otros sistemas) se copian del
SNES después de cada cuadro y no se comparan (`kNotCompared`, `elem_skip`).

## Estado

| Parte | Origen | Archivo | Estado |
|---|---|---|---|
| Movimiento de Samus | $90 | samus_move.c | escrito |
| Poses, transiciones, paletas de Samus | $91 | samus_pose.c | escrito |
| Colisión con bloques y pendientes | $94 | coll.c | escrito |
| Scroll de la cámara, capa 2 | $80 | scroll.c | escrito |
| Cola de sonido, cola de música | $80/$82 | sfx.c, door.c | escrito |
| Cuadro de juego (estado 8), mando, NMI, interrupciones de línea | $80/$82 | game.c, door.c | escrito |
| Puertas y carga de salas (estados 9, 10, 11), fundidos | $80/$82/$88/$89/$8F | door.c | escrito |
| Disparo del rayo normal, estelas, explosión | $90/$93/$9B | proj.c | escrito |
| Marco de enemigos: carga, IA, instrucciones, colisiones, cola de dibujo | $A0 | enemy_main.c | escrito |
| Puertas de Ceres (enemigo E23F) | $A6 | ceres_door.c | escrito |
| Plataforma del ascensor (proyectiles enemigos), PLM B7EB | $86/$84 | eproj.c | escrito |
| HUD: energía, ítems, minimapa | $80/$90 | hud.c, samus_move.c | escrito |
| Sala de Ridley **sin Ridley**: solo la cápsula de la bebé | $A6 | ceres_ridley.c | escrito (a propósito distinto del SNES) |

Datos preparados en el PC (`mkdata.py` → `gen/tables.c`): salas de Ceres (cabeceras, estados, puertas, nivel y BTS
ya descomprimidos, FX, paletas de tileset), enemigos (cabeceras, poblaciones, sets, listas de instrucciones de la
puerta y de la bebé), proyectiles (listas de $93, estelas de $90/$9B), HUD (dígitos, mapa del área 6).

**Resultado:** grabación del bot desde que empieza el juego en Ceres (cuadro 97, Samus en el ascensor) hasta que
entra a la sala de Ridley (cuadro 1822): bajada del ascensor, las 5 salas, 4 puertas, disparos contra las puertas,
HUD y minimapa, paletas completas (fundidos de puerta incluidos), enemigos, nivel y BTS. Modo libre:
**0 diferencias en 1725 cuadros**. Desde el cuadro 1823 (sala de Ridley) la diferencia es buscada: no hay Ridley.

Sin portar todavía (no hacen falta en la estación sin la pelea ni la huida): objetos de paleta, animación de tiles,
objetos de HDMA (el haze lo imita la Genesis), la huida (temblores, escombros, balanceo), Ridley.

## Pendiente

1. La capa de la Genesis (`md/`): salas con los mapas de tiles ya hechos (docs/27), sprites directos al VDP (Samus,
   rayo y estelas, puertas, cápsula, plataforma del ascensor), tiles de Samus por DMA ya convertidos, paletas, HUD,
   mando, sonido. ROM de prueba para MiSTer.
2. Unir la intro (que ya llega hasta que la nave entra a la estación) con el motor nativo.
