# 58 — Enemigos congelados, explosión de la bomba de poder y estelas de la onda

Fecha: 2026-10-28. Rama `hielo-pb-estelas` (desde `main`). Los tres puntos de "Lo que falta" de docs/57, en este
orden: enemigos congelados, la explosión de la bomba de poder (y el rayo de luz de los trajes) y las estelas de los
rayos con la paleta 5.

Estado: **punto 3 hecho**. Los puntos 1 y 2 están medidos y analizados; las propuestas esperan la decisión de Edwin
(abajo).

## 3. Estelas del rayo de onda (hecho)

### Lo que hace el SNES (decomp)

- `SpawnProjectileTrail` ($90:B657) elige las listas de la estela de cada proyectil en `off_90B5BB` (izquierda) y
  `off_90B609` (derecha) por `projectile_type & 0x3F` (rayos) o `31 + tipo` (misiles: 32, supermisiles: 33).
- Las listas de los tiles 0x3C-0x3F con la paleta 5 (atributo `0x2A3C`...) son **solo las del rayo de onda**: la
  entrada 1 (onda sin cargar) y la 17 (onda cargada). Las demás entradas con estela (2, 3, 6, 7, 10, 11, 16, 18, 19,
  22, 23, 26, 27) usan los tiles 0x38-0x3B con la paleta 6; los misiles, 0x48-0x4B con la paleta 5.
- Esos tiles usan los colores 5, 6 y 7 de la paleta 5 de los sprites (7C1D, 5814, 300A, la paleta inicial
  `$9A:8000`), que son casi los 2, 3 y 4 de la paleta de la onda (7C1F, 5816, 300C, `kBeamPalettePtrs[1]` en
  `$90:C3C9`). Cuando sale una estela de onda, la onda es el rayo equipado: su paleta es la paleta 6 de ese momento.

### Lo que se hizo (`tools/conv/mkzobj.py`)

- Las claves de esas estelas (`("t", "2A3C")`... de `mkmd.zebes_proj_keys`) ya no toman los colores de la sala.
  Cada píxel va al lugar fijo 8 + c del color c más parecido de los 1-5 de la paleta de onda (`wave_trail_slots`).
  Salen 5→10, 6→11 y 7→12: los colores 9-13 de la línea 3, que la Genesis copia de `palette_buffer[225-229]` en cada
  cuadro, igual que con los tiles del rayo (docs/57).
- Esos colores ya no entran en la cuenta de colores de la sala: hay algo menos que juntar (las páginas quedan con
  133 KB libres; antes, 130).
- Las estelas de los misiles (tiles 0x48-0x4B, paleta 5) siguen con los colores de la sala, como antes.

### Comprobación

- Lado a lado en 9D19 (`tools/compare/lado_a_lado.sh`, puerta 8DAE, `XY_N=250 X=784 Y=740`, entrada
  `in_chargebeam.txt`, `EXTRA_FL="-DMD_TEST_GOD -DMD_TEST_ITEMS=0x1004 -DMD_TEST_BEAMS=0x1001"`,
  `HL_ITEMS=1004 HL_POKE=9A6:1001,9A8:1001`): la Genesis de `main`, la de esta rama y el SNES en el mismo cuadro del
  motor (mosaico para Edwin). Antes, los anillos de la estela salían morado oscuro; ahora salen magenta, como en el
  SNES. (La cámara de la Genesis queda corrida en vertical en esta prueba, igual que en docs/57: es la colocación
  con `SETXY`, no el dibujo.)
- `make` con los datos nuevos da el mismo md5 dos veces (`312d3957…`).

## 1. Enemigos congelados (medido: la propuesta de docs/57 no entra)

### Lo que hace el SNES (decomp)

- `WriteEnemyOams` ($A0:944A): con `frozen_timer` distinto de 0, el enemigo se dibuja con `palette_index = 0x0C00`
  (la paleta 6, la del rayo) en vez de la suya; mientras `frozen_timer >= 0x5A`, siempre, y después un cuadro sí y
  otro no (`frozen_timer & 2`). Los mismos tiles con los colores de la paleta del rayo equipado.
- Con el hielo equipado, la paleta 6 es la de hielo (`kBeamPalettePtrs[2]`, la misma en las 6 combinaciones con
  hielo): 7FFF 7EC0 6DE0 54E0 7F6E 730A 6665 5E22 7FA0 7B60 7720 72A0 6E60 7F91 20C0 (15 colores).
- Se congela (`EnemyShot`, $A0:A6DE…A79A) si la vulnerabilidad al rayo con hielo (`EnemyDef.vulnerability_ptr`,
  +0x3C; sin ella `$B4:EC1C`) es 0xFF, o si el disparo lo mata y `(power & 0xF0) != 0x80`.

### Lo que se midió (`mkzobj.py` con `MM_FROZEN=1`)

Se implementó la propuesta de docs/57 para medirla. Por cada clave de un enemigo que se puede congelar
(`frozen_banks`), una clave con la paleta 0x0C00: los mismos tiles del SNES, los colores 1-5 del hielo en los lugares
fijos 9-13 del rayo y los 6-15 en lugares libres (si no hay, al más parecido). Los congelados van pieza por pieza (sin
`compose`) y nunca le quitan lugar a los enemigos normales: si con ellos no entran los tiles en la VRAM, ese estado
queda sin congelados.

| | `main` | con congelados por sala |
|---|---|---|
| Claves de objetos (292 estados) | 29 361 | 31 244 (+1 883) |
| Sprites en los registros | 81 691 | 90 649 (+8 958) |
| Tiles del depósito | 40 493 | 41 491 (+998) |
| Páginas de Zebes | 11-21, 130 KB libres | **11-22** (+~140 KB: no entra en las de hoy) |
| Estados donde no entran en la VRAM | — | 9CB3, 9D19 y 9E9F (los dos de 9E9F): ahí seguirían invisibles |
| Lugares libres de la línea 3 para los colores 6-15 del hielo | — | ninguno: los enemigos de cada estado usan entre 13 y 57 colores distintos para 10 lugares (`MM_ZOBJ_STATS=1`) |

Una primera versión que dejaba armar (`compose`) también a los congelados entraba en 11-21 (55 KB libres), pero en
9 estados los enemigos grandes perdían su armado (más sprites por línea): se descartó.

Para el resto de las pruebas, la ROM de esta rama se compila **sin** los congelados (`MM_FROZEN` vale 0 por omisión):
las páginas, la regresión y las comparaciones son las de `main` más el punto 3.

### Propuestas (para Edwin)

- **1a. La de docs/57 (por sala)**: está hecha, se enciende con `MM_FROZEN=1`. Cuesta una página más del SSF2 (la 22:
  ROM de 11,25 a 11,75 MB) y en 9CB3, 9D19 y 9E9F los congelados no se verían (VRAM llena).
- **1b. Congelados comunes por tipo de enemigo, subidos a la VRAM al congelarse** (recomendada): un juego de claves
  común en una página (no por sala), como el de los rayos. Medido sobre todas las claves de Crateria y Brinstar: 1 713
  claves de congelados, **655 distintas** por contenido, **1 115 tiles distintos** (35 KB) y ~25 KB de registros:
  ~65 KB en las páginas, entra en lo libre de hoy. En la VRAM solo se necesita el spritemap que está congelado (un
  enemigo congelado no se anima): 10 tiles en la mediana, 16 en el 90 % y 64 como mucho. Se suben en el vblank (como
  los tiles del rayo o del gancho) a un tramo fijo de ~64 tiles: los libres después de los objetos y las filas ocultas
  del Window (como el haz del ojo y la caja de mensaje; al cerrar la caja se vuelven a subir). Cada sala solo guarda
  qué clave común usa cada congelado (~14 KB en total). Lo que falta resolver: qué hacer si se congelan a la vez más
  enemigos de los que entran (lo más probable: los últimos con su paleta normal) y las salas del ojo, donde el haz usa
  esos tiles.
- **1c. Sin tiles nuevos**: dibujar al congelado con sus tiles normales y otra línea de colores. No hay ninguna libre
  (0 y 1: la sala; 2: Samus; 3: enemigos y rayo), así que no sirve sin quitarle colores a la sala o a los enemigos.

## 2. La explosión de la bomba de poder (analizada, falta elegir el método)

### Lo que hace el SNES (decomp, banco $88)

- `SpawnPowerBombExplosion` ($88:8AA4) crea dos objetos de HDMA: la ventana 2 por línea (izquierda y derecha:
  `power_bomb_explosion_left_hdma` / `_right_hdma`, 192 líneas por distancia al centro, indirectas en
  `kIndirectHdmaTable_PowerBombExplodeLeft/Right`) y la configuración.
- Matemática de color (`LayerBlendPowerBombFunc_0` $88:8219): `CGWSEL = 2` (suma la subpantalla), `CGADSUB = 0x37`
  (suma, sin la mitad, a BG1, BG2, BG3, sprites y fondo), `TS = TSW = BG3` con `W34SEL = 8`: dentro de la ventana el
  BG3 se quita de la subpantalla y se suma el color fijo (`COLDATA`). Afuera la sala queda igual. En la sala A66A
  (`Func_6`) no se aclaran BG2 ni BG3 (`CGADSUB = 0x31`).
- Las etapas (los `HdmaobjPreInstr_PowerBombExplode_*`):
  1. **Destello previo** (`PreExplosionWhite` $88:90DF): óvalo escalado por `power_bomb_pre_explosion_flash_radius`
     (`CalculatePowerBombHdmaScaled_*`: media anchura = radio × `g_byte_88A206[j]`, media altura hasta
     radio × 191/256), colores de `g_byte_889079` (gris 16 → amarillento); crece muy rápido (unos 3 cuadros).
  2. **Destello amarillo** (`PreExplosionYellow` $88:91A8): formas ya escaladas (`pre_scaled_power_bomb_explosion_shape_def_ptr`,
     192 bytes por cuadro, hasta `$88:A206`).
  3. **Explosión amarilla** (`ExplosionYellow` $88:8DE9): el óvalo crece de 4 a 134 (radio 0x0400 → 0x8600, aceleración
     0x30: ~37 cuadros); colores de `kPowerBombExplosionColors` ($88:8D85: R = G de 14 a 28, B de 10 a 0).
  4. **Explosión blanca** (`ExplosionWhite` $88:8EB2): formas ya escaladas hasta `$88:9F06`.
  5. **Resplandor** (`Stage5_Afterglow` $88:8B98): la misma forma, el color fijo baja de a 1 cada 3 cuadros hasta 0.
- Lo mismo usa el destello de cristal (`SpawnCrystalFlashHdmaObjs`) y el rayo de luz de los trajes
  (`InitializeSuitPickupHdma` $91:D692: ventana por línea de `hdma_table_1`, `CGADSUB = 0x33`, `COLDATA` de
  `suit_pickup_color_math_*`): **una forma por línea y un color que se suma**.

Mosaico del SNES para Edwin (9D19, `in_powerbomb.txt`): el óvalo es grande (media anchura de hasta 134 px, media
altura de hasta 100) y casi siempre queda cortado por un borde de la pantalla; afuera la sala no cambia.

### Lo que la Genesis puede hacer (y lo que no)

- La Genesis no suma colores. Tiene el modo sombra/luz: los sprites operador (color 14 de la línea 3) aclaran lo que
  tapan **hacia el blanco** (c/2 + 1/2), siempre en la misma medida; no pueden sumar amarillo ni bajar poco a poco
  como el resplandor.
- Cambiar los colores (el tinte de `tint15`, que ya suma un color a todos los de la sala: niebla, neblina del Torizo)
  da **el color exacto** del SNES, pero vale para la línea entera: no puede seguir el borde izquierdo y derecho del
  óvalo dentro de una línea.
- Con sprites, el óvalo grande necesita ~40 sprites (radio 120: ~38, hasta 8 por línea) además de Samus y los
  enemigos (64 en H32, 16 y 256 puntos por línea): en las líneas casi llenas no queda lugar para los demás sprites.
- Todas las formas del SNES salen de tablas de la ROM (el radio escala `g_byte_88A206`; las etapas 2 y 4 usan formas
  ya escaladas del banco $88). Se pueden preparar en el PC: para cada forma, los sprites de la luz y los tiles de sus
  bordes, sin armar nada en la Genesis (a diferencia del haz del ojo, que depende de la posición de Samus y se arma
  de a pasos).

### Métodos posibles (para Edwin)

- **2a. Sombra/luz con sprites, formas preparadas en el PC.** La forma exacta en cada cuadro; aclara hacia el blanco.
  Para el resplandor (que en el SNES baja de a poco), la luz se apaga cuando el color fijo pasa por la mitad, o se
  alterna un cuadro sí y otro no. Sirve tal cual para el rayo de luz de los trajes (otra forma por línea) y para la
  bomba de poder en las peleas. Riesgo: en las líneas casi llenas los sprites de la luz se comen el límite por línea.
- **2b. Tinte por líneas.** Las líneas que toca el óvalo cambian sus colores al tinte exacto (una interrupción
  horizontal al principio y otra al final, como la de los colores debajo del HUD). Colores y resplandor exactos,
  ningún sprite, muy barato; pero la forma es una franja de lado a lado, no un óvalo, y el cambio de 48 colores a
  mitad de pantalla deja puntos en una o dos líneas.
- **2c. Mixto.** Sprites de luz (2a) mientras el óvalo crece; en el resplandor (la forma ya no cambia y cubre casi
  todo), el tinte exacto (2b) con la forma por líneas. Más trabajo; las dos partes se ven distintas en el cambio.

Recomendación: **2a**. Es la forma del SNES cuadro a cuadro, no carga al 68000 (todo preparado en el PC) y es el
método que se reusa para el rayo de luz de los trajes y las peleas. Antes de implementarlo hay que medir cuántos
sprites usa la forma más grande que sale en pantalla y qué queda para Samus y los enemigos.

## Límites (esta rama, sin congelados)

| | `main` | Ahora | Límite |
|---|---|---|---|
| Fin de la parte común (`__mm_title_start`) | 0x33EAB4 | 0x33EAB4 | 0x380000 |
| RAM `_bend` | 0xE0FFE210 | 0xE0FFE210 | 0xE0FFE3F0 |
| Páginas de Zebes (`mkzebes.py`) | 11-21, 130 KB libres | 11-21, 133 KB libres | sin aviso |
| ROM | 11 264 KB | 11 264 KB | |

## Comprobación contra el SNES

La lógica no cambió (solo `mkzobj.py`): `tools/rooms/beams.sh` (11 combinaciones), `sba.sh` (4), `powerbomb.sh`,
`chargebeam.sh` y la grabación Ceres → Zebes (`host/check ref_full.bin … 97 1822 0`): **0 diferencias** en todas.

## Regresión

`sh tests/regress/regress.sh`: intro 0 distintos, Ceres 0 distintos, Zebes 0 distintos. No se aprobaron referencias
nuevas.

## Lo que falta

- Punto 1: elegir entre 1a y 1b (o otra) e implementarlo; comprobar en varias salas de Crateria y Brinstar lado a lado
  con el SNES y que `md_missing_ring` quede sin claves de congelados.
- Punto 2: elegir el método e implementarlo para la bomba de poder; después, el rayo de luz de los trajes. Medir los
  cuadros perdidos (`tools/perf/`, docs/44).
