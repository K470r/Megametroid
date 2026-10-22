# MegaMetroid 26 — Extracción completa de recursos de la ROM

## Qué hace

`tools/sm_extract/extract_all.py` extrae todos los recursos de `sm.smc` (NTSC, sin header) a `data/extracted_full/`:

```text
python3 tools/sm_extract/extract_all.py sm.smc data/extracted_full
python3 tools/sm_extract/extract_all.py sm.smc data/extracted_full --only rooms,samus
```

Módulos: `blobs, tilesets, palettes, rooms, samus, enemies, text, audio`.

Requisitos: Python 3.8+, Pillow, numpy. Usa `sm/assets/restool.py` (snesrev/sm) como inventario y lee `sm/sm.smc`.

## Regla de verificación

Se mantiene la política de `data/rom/extraction_policy.md`: **ningún límite de datos se adivina**.

1. **Inventario:** `restool.visit_all()` recorre la ROM desde los mismos punteros que usa el juego (load stations → salas → puertas, tablas de tilesets, tablas de Samus, headers de enemigos, message boxes, animtiles, PLMs). Todo lo que se extrae sale de ese recorrido o de una dirección citada en el código C.
2. **Reglas de carga:** cada una está citada contra su función C (ver la tabla de abajo).
3. **ROM:** se verifica por SHA-256 antes de extraer.

## Resultado (ROM D63ED5F8)

| Módulo | Cantidad | Fuente verificada |
|---|---|---|
| blobs | 1091 bloques (404 comprimidos, con versión descomprimida) | restool `Blob`/`PackedBlob` |
| tilesets | 29 | `LoadStateHeader` 0x82DEF2, `LoadCRETilesTilesetTilesAndPalette` 0x82E783 |
| palettes | 38 | restool `visit_palettes` + paletas de tilesets |
| rooms | 262 salas, todos sus estados | `Room.parse_actual_binary`, `LoadLevelDataAndOtherThings` 0x82E7D3 |
| samus | 253 poses, 1982 frames | `SetSamusTilesDefsForCurAnim`, `NmiTransferSamusToVram`, `Samus_Draw` (generaliza el composer 23R) |
| enemies | 154 definiciones | `EnemyDef` (paleta en `ED->bank`, `sm_a0.c:522`) |
| text | 27 message boxes + fuente BG3 | `$85:869B`, `LoadStdBG3andSpriteTilesClearTilemaps` 0x8282E2 |
| audio | 25 song sets, 115 samples únicos | `APU_UploadBank` 0x808028, `SpcPlayer_Upload`, DIR=$6D |

## Detalles por módulo

### Tilesets (`tilesets/NN/`)
- VRAM: primero va el CRE (`$B98000`) en el byte `0x5000` y después el tileset en `0x0000`, en el mismo orden que el juego. Los tilesets grandes (Ceres 17-20 y Kraid 26) pisan parte del CRE, igual que en la consola.
- Tile table: CRE (`$B9A09D`) en metatiles `0x000-0x0FF` y tileset desde `0x100`. En Ceres (área 6) el tileset ocupa toda la tabla.
- `metatiles_1024.png`: los 1024 bloques de 16×16 con paleta y flips reales.
- `vram_tiles_palN.png`: los tiles de 8×8 vistos con cada una de las 8 paletas BG.

### Salas (`rooms/<área>_<id>_<nombre>/`)
- `room.json`: header, estados (condición, tileset, música, FX, enemigos, PLM, scrolls, etc.).
- `stateN_*_level_data.bin`: level data descomprimido (`[size][layer1][BTS][layer2]`).
- `stateN_*_layer1.png` / `layer2.png`: render de la sala completa.
- `*_block_types.bin` (tipo de bloque 0-15) y `*_bts.bin`: la base para la colisión.
- Los estados que comparten level data y tileset no se duplican: su campo `render` apunta al primero.
- Double Chamber y Bowling Alley traen más level data que `w*h`. El juego solo indexa los primeros `w*h` bloques, y aquí se hace igual (queda anotado en `note`).

### Samus (`samus/`)
- `poses/pose_NNN.png`: tira con todos los frames de la pose, con la paleta Power Suit.
- `samus_poses.json`: por frame, el anim def, los spritemaps superior e inferior y las transferencias DMA (origen y tamaños).
- `palettes/`: Power (`$9B9400`), Varia (`$9B9520`) y Gravity (`$9B9800`), de `addr_kSamusPalette_*`.
- `atlas_frame0.png`: el frame 0 de todas las poses, para revisión rápida.
- Samus está en (32,36) del lienzo de 64×64 y el spritemap se desplaza en Y según `kSamus_CalcSpritemapPos` (0x908C1F): Default, Standing, Crouch y Special.
- La mitad inferior solo se dibuja cuando `kSamusIsBottomDrawnFuncs` (0x9085E2) lo permite, según el movement type (`kPoseParams` en $91:B629), la pose y el frame. Son 378 frames sin mitad inferior. En la pose 00 se agrega el OAM extra de `SamusBottomDrawn_0_Standing`.
- Las poses 0x45 y 0x46 no se usan en el juego: no tienen constante `kPose_` y la única transición que entra en ellas viene de sí mismas. Sus mitades apuntan a tiles de poses distintas, así que se ven rotas en la ROM original.

### Enemigos (`enemies/<dir>_<label>/`)
- `enemy.json` (salud, daño, radios, punteros), `tiles_4bpp.bin`, `palette_bgr555.bin` y `tiles.png` con su paleta.
- `spritemaps_index.json`: los spritemaps que encontró `enemy_instr_decode`.

### Texto (`text/`)
- En Super Metroid los textos son **gráficos** (tilemaps BG3), no cadenas. Cada message box se renderiza a PNG con la fuente BG3 estándar (`$9AB200`).
- `enemy_debug_names.json`: nombres ASCII de depuración que trae la ROM (banco `$B4`).
- `room_names.json`: nombres de salas según la decompilación. No vienen en la ROM.

### Audio (`audio/`)
- `aram_base_engine.bin`: la ARAM tras subir el motor base (`$CF8000`).
- `songset_XX/aram.bin`: la ARAM completa de cada song set (motor + bloques de la canción).
- `samples/*.brr` y `*.wav`: cada sample del directorio `$6D00`. El WAV es mono de 16 bits a 32000 Hz nominal; la altura real sale de `pitch_base` del instrumento.
- `songset.json`: bloques subidos, tabla de tracks (`$5820`), instrumentos (`$6C00`, 6 bytes: SRCN, ADSR1, ADSR2, GAIN, pitch) y samples con su punto de loop.
- El BRR se decodifica con el algoritmo de snes_spc (blargg).
- **No incluido:** conversión de secuencias a YM2612/PSG. Las secuencias quedan en `aram.bin`, apuntadas desde la tabla de tracks.

## Lo que falta para Genesis

Esto es la extracción fiel en formato SNES. La conversión a Genesis va en un paso aparte:

1. Tiles SNES 4bpp planar → Genesis 4bpp packed.
2. Paletas: de 8 paletas BGR555 a 4 paletas de 9 bits.
3. Tilemaps: tile de 10 bits y paleta de 3 bits → tile de 11 bits y paleta de 2 bits.
4. Música: secuencias N-SPC → XGM2.

## Verificación independiente

`tools/sm_extract/verify_extraction.py` comprueba todo lo extraído contra la ROM sin reutilizar el código del extractor. Usa el descompresor original de snesrev, un decodificador BRR según fullsnes y renderizadores propios para salas, metatiles, mensajes y frames de Samus.

```text
python3 tools/sm_extract/verify_extraction.py sm.smc data/extracted_full
```

Resultado: **9958/9958 comprobaciones correctas**.
- **Datos binarios:** cada bloque, paleta, tile y level data coincide byte a byte con la ROM o con su descompresión. Las ARAM de audio se reconstruyen desde los bloques de la ROM y cada BRR coincide con ellas.
- **PNGs indexados:** al volver a codificar los píxeles a 4bpp/2bpp SNES se obtienen exactamente los bytes originales.
- **PNGs compuestos** (salas, metatiles, mensajes, frames de Samus): son idénticos píxel a píxel a un render independiente.
- **WAV:** son idénticos muestra a muestra a una segunda decodificación BRR.

Qué no se verifica contra la ROM, porque no son datos de la ROM: los JSON de metadatos y las imágenes de muestras de paleta (`palette.png`).
