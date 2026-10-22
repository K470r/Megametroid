# 32 — Ceres jugable en la Genesis con el motor nativo

## Qué hay

`native/md/out/rom.bin` (copia: `MegaMetroid_Ceres_nativo.bin`): la estación Ceres jugable, desde que Samus
baja en el ascensor hasta la sala de la cápsula de la bebé y, desde docs/33, la pelea con Ridley y su huida
(falta el escape de la estación).

- El juego es el motor nativo de docs/31: C escrito a mano para el 68000, con la lógica leída del decomp.
- Lo que el SNES hacía con su PPU lo hace `native/md/` con datos que el PC ya dejó en el formato del VDP. La
  Genesis no convierte nada del SNES: copia tiles y planos por DMA, lee tablas y escribe sprites.
- `MegaMetroid_Ceres_nativo_demo.bin`: la misma ROM, pero el mando lo maneja la grabación del bot del SNES
  (sirve para ver el recorrido completo sin jugar y para la comparación automática).

Mando (6 botones): C saltar, A correr, B disparar, X apuntar abajo, Z apuntar arriba, Y elegir ítem, MODE
cancelar ítem. Con 3 botones: START + arriba/abajo para apuntar.

## Cómo se arma

```
REF=<grabación hlref de Ceres> sh native/md/build.sh          # ROM para jugar
REF=... TEST=1 sh native/md/build.sh                         # ROM con la entrada del bot
```

1. `native/host/build.sh`: tablas del motor (`mkdata.py`, `gen_vars.py`, `gen_enums.py`), comprobación `check`
   y `mkinit`.
2. `native/host/mkinit`: el estado del motor al empezar el juego en Ceres (cuadro 97 de la grabación), ya en
   big-endian para el 68000. La Genesis solo lo copia. (Cuando se una con la intro, esto lo harán los estados
   6 y 7 del juego portados.)
3. `native/tools/mkmd.py`: todos los datos de dibujo (ver abajo).
4. gcc para 68000 + SGDK (solo arranque, espera del vblank y mando).

## Datos de dibujo (`mkmd.py` → `native/md/gen/`)

| Qué | Cómo queda en la Genesis |
|---|---|
| Salas | Planos A y B completos (64x64 celdas; el pozo, 32x128) y sus tiles. Se cargan enteros al entrar: no hay dibujo de columnas/filas. El pozo (modo 7 en el SNES) sale del mapa de modo 7 del tileset 17; la puerta del fondo del pozo alterna dos versiones de 4 celdas. La capa 2 de "biblioteca" es la del SNES tal cual (`native/md/snes_ref/<sala>.bin`, capturas de VRAM hechas con `hlref HL_VRAM`). |
| Colores | Cada color de la CRAM dice de qué índice de `palette_buffer` sale; `kCram555` (32768 entradas) da el color de la Genesis. Así los fundidos y ciclos de color del motor se ven igual. Solo se recalculan los colores que cambiaron. |
| Samus | Tiles por definición ($92) ya intercalados (columna c, fila r → 2c + r): un sprite de 16x16 del SNES es uno de 2x2 de la Genesis. Las 2283 spritemaps convertidas a registros de sprite. |
| Puertas, rayo, cápsula, plataforma | Por sala: registros de sprite por spritemap y sus tiles. El rayo normal en las 10 direcciones y su explosión se sacan de las listas del banco $93. |
| HUD | Plano Window (4 filas). Tabla de 8192 palabras: palabra del HUD del SNES → celda de la Genesis. |
| Paletas | PAL0-1 fondo, PAL2 Samus, PAL3 HUD; los objetos van donde quepan. Si una sala necesita más de 60 colores, se funden los más parecidos (pozo y sala de computadoras: 10). |

## Verificación

- Motor contra el SNES en el PC (`native/host/check`): recorrido del bot (1725 cuadros), entrada al azar (7900
  cuadros) y tres recorridos del bot con ráfagas al azar (`hlbot HL_NOISE`): 0 diferencias.
- ROM en Genesis Plus GX (`tools/emu_harness/native_compare.py`): las variables del motor en la Genesis son las
  mismas que en el SNES en los 2250 cuadros; imagen contra el SNES sin el color math: diferencia media de 0,3
  niveles (de 7) en los pasillos, 2,6 en el pozo (colores fundidos). La imagen va 2 líneas más abajo que en el
  SNES: así el HUD queda igual respecto del juego (en el SNES el juego empieza en la línea 30; aquí, debajo de
  la ventana de 32 líneas).
- Velocidad: 60 cuadros por segundo; se pierde un cuadro al entrar a cada sala (carga de la VRAM). Con entrada al
  azar, menos de 1 cuadro perdido cada 250.
- Optimizaciones que no cambian el resultado (comprobadas con `check`): fundido de paleta con tabla en vez de
  división, interrupciones de línea sin recorrer 225 líneas, minimapa que no se recalcula si nada cambió, y en la
  Genesis el fundido calcula cuadro a cuadro solo los colores que se ven (al terminar, todos quedan igual que en
  el SNES).

## Corrección: colores que cambiaban al moverse (8 de octubre)

En salas con desplazamiento vertical a veces el fondo se ponía rojo, verde o negro. Causa: la interrupción
horizontal del degradado (escribe el color de fondo en la CRAM) se volvía a activar antes de terminar las
escrituras del vblank. En los cuadros pesados ese trabajo se alargaba hasta la imagen; la interrupción cambiaba la
dirección del VDP a la CRAM y las palabras siguientes (filas del HUD, scroll) caían como colores. Ahora md_vblank
corre con las interrupciones enmascaradas y la interrupción horizontal se activa al final. Comprobado leyendo la
CRAM del VDP en cada cuadro (`retrorun ctrace`): 0 cuadros con colores distintos a los del motor en el recorrido
del bot y en 4 recorridos con entrada al azar.

## Lo que todavía no es igual al SNES

- **El haze de Ceres.** En el SNES, el color math suma azul a toda la imagen por bandas de 8 líneas (más azul
  hacia abajo). La Genesis no tiene color math y cambiar todos los colores a mitad de pantalla deja puntos (las
  escrituras a la CRAM fuera del borrado horizontal). Por ahora solo el color de fondo lleva el degradado (lo
  negro de la estación), con la interrupción horizontal cada 8 líneas. Es la diferencia más visible.
- Sonido: la cola del motor está, falta el driver de la Genesis.
- Ridley, la pelea, el temblor y la huida: docs/33. Falta el escape (escombros, balanceo del pozo, cuenta
  regresiva).
