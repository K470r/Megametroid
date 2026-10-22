# MegaMetroid 27 — Conversión de salas a Genesis (pasos 1 a 4)

## Qué hace

`tools/genesis_convert/convert_rooms.py` convierte las salas extraídas y verificadas en el paso 26 al formato del VDP de Genesis.

```text
python3 tools/genesis_convert/convert_rooms.py data/extracted_full data/genesis/rooms            # todo el juego
python3 tools/genesis_convert/convert_rooms.py data/extracted_full data/genesis/rooms --area 0   # solo Crateria
```

Opciones:
- `--pals 2`: paletas de fondo preferidas.
- `--max-pals 3`: tope de paletas cuando 2 no alcanzan sin perder colores.
- `--color dac|oklab|linear`: criterio de conversión de color.

## Los cuatro pasos

1. **Tiles.** Pasan de SNES 4bpp planar a Genesis 4bpp packed (nibble alto = píxel izquierdo, 32 bytes por tile).
2. **Color.** Cada color BGR555 se lleva a CRAM de 9 bits. El modo predeterminado (`dac`) elige, canal por canal, el nivel más cercano de la salida real del DAC del VDP (0, 52, 87, 116, 144, 172, 206, 255), que no es lineal.
3. **Paletas por sala.** Solo se toman las combinaciones (tile, subpaleta SNES) que la sala usa de verdad, y se agrupan en paletas Genesis de 15 colores más el transparente. Cada tile 8×8 tiene que caber entero en una sola paleta. El empaquetado se intenta en este orden:
   1. Por subpaleta SNES completa (búsqueda exhaustiva).
   2. Por tile: reinicios aleatorios con semilla fija más mejora local.
   3. Con una paleta más, hasta `--max-pals`.
   4. Solo si nada de lo anterior alcanza, se fusionan los colores más parecidos (distancia perceptual), quedándose siempre con la opción de menos fusiones.
4. **Tiles y mapas.** Los píxeles se reindexan a su paleta Genesis y los tiles se deduplican aprovechando los flips (tile 0 = vacío). El mapa de cada capa se escribe en big-endian, con una palabra por celda 8×8: `PRI | PAL(2) | VF | HF | TILE(11)`. Los flips de bloque 16×16 se aplican igual que `BuildBgColumn` (sm_80.c). En Ceres (área 6) la tabla de bloques es la del tileset sin CRE, como en `LoadLevelDataAndOtherThings`.

## Salida por estado de sala (`data/genesis/rooms/<sala>/`)

| Archivo | Contenido |
|---|---|
| `stateN_*_tiles.bin` | Tiles Genesis (32 bytes cada uno) |
| `stateN_*_cram.bin` | Paletas: 16 palabras CRAM big-endian por paleta. El índice 0 es el color de fondo del SNES (CGRAM[0]) |
| `stateN_*_layer1_map.bin` / `layer2_map.bin` | Mapas de plano, ancho × alto celdas |
| `stateN_*_layer*_genesis.png` | Render hecho desde los datos Genesis |
| `stateN_*_report.json` | Tiles, paletas, colores por paleta, fusiones y verificación |
| `report.json` | Resumen de todas las salas |

## Resultado

Verificación: cada estado se renderiza desde los datos Genesis y se compara con la sala original del paso 26, pasada por la misma conversión de color.

| | Estados | Idénticos píxel a píxel | Con 3 paletas | Con fusión de colores |
|---|---|---|---|---|
| **Crateria** | 36 | **36** | 2 (West Ocean, East Ocean) | 0 |
| **Todo el juego** | 268 | **266** | 50 | 2 (Ceres Shaft: 43 colores, 7 y 10 fusiones) |

- Tiles por sala: mediana ~97, máximo 297 (West Ocean). Cabe con holgura en VRAM.
- Los océanos solo tienen 28 y 25 colores, pero algunos tiles mezclan grupos de colores y por eso no entran en 2 paletas. Con 3 quedan exactos.
- Ceres Shaft (la sala que rota con Mode 7) necesitaría 4 paletas. Queda para el tratamiento especial de Ceres, junto con el truco de la torre de Castlevania Bloodlines (scroll por línea más scroll vertical por columnas) para simular el balanceo.

## Color: una decisión de estilo pendiente

El nivel más oscuro del DAC de Genesis por encima del negro (~52/255) es más claro que los tonos oscuros del SNES (8 a 33). Ninguna fórmula lo resuelve del todo. `color_modes_landing_site.png` compara tres criterios:

- **A (`dac`, predeterminado):** conserva mejor el brillo; los verdes oscuros salen más saturados.
- **B (`oklab`):** desplaza los tonos oscuros hacia azul-cian.
- **C (priorizar brillo):** descartado, vuelve rojizo el fondo.

Más adelante se pueden sobreescribir a mano las paletas de salas puntuales.

## ROM de prueba: `tests/room_viewer/`

Es un proyecto SGDK aparte que no toca `src/main.c`. Muestra las 34 salas de Crateria en modo H32 (256×224, como el SNES): capa 1 en el plano A, capa 2 en el plano B, y planos de 64×32 que se actualizan por columnas y filas al mover la cámara.

| Botón | Acción |
|---|---|
| D-pad | Mover la cámara (con B, más rápido) |
| A / C | Sala anterior / siguiente |
| START | Mostrar u ocultar la capa 2 |

- Compilar: `tests/room_viewer/build.bat` (usa `C:\sgdk`).
- ROM precompilada: `tests/room_viewer/out/rom.bin`.
- Regenerar los datos: `python3 tools/genesis_convert/make_room_viewer_data.py data/genesis/rooms tests/room_viewer/src --area 0`.

### Prueba en emulador

`tools/emu_harness/mdrun.c` ejecuta la ROM con el core libretro de Genesis Plus GX, sin pantalla. Simula el mando y guarda capturas. `tools/emu_harness/check_room_viewer.py` compara cada captura contra la composición esperada, calculada desde los datos Genesis siguiendo el orden de prioridades del VDP (fondo < B baja < A baja < B alta < A alta).

Resultado: **34 de 34 salas de Crateria idénticas píxel a píxel**, en la zona con más colores de cada sala y con scroll profundo (hasta 1984 px en Old Tourian Shaft). La hoja de capturas está en `docs/27_crateria_genesis_emulator.png`.

## Pendiente (siguientes pasos)

- Fondos de biblioteca (`bg_data`) y la capa 3 (agua, niebla, lava).
- Tiles animados (animtiles) y ciclos de paleta (palfx).
- Parallax de la capa 2: el visor mueve ambas capas a la misma velocidad.
- Sprites (Samus y enemigos) con PAL2 y PAL3, y el HUD en el plano Window con cambio de paleta por H-int.
