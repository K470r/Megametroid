# 63 — El final y los créditos (en curso: la lógica está; falta el dibujo)

Referencia del SNES: `tools/compare/final_snes.sh` (el despegue de docs/62 y 16000 cuadros: capturas en
`work/final/snes`; con `REF=archivo`, la RAM por cuadro para `host/check`).

## Lo que hace el SNES (estado 39, `GameState_39_EndingAndCredits_` $8B:D443)

Cada cuadro: la función de la cinemática (`cinematic_function`, desde `CinematicFunctionEscapeFromCebes` $8B:D480),
los objetos-sprite y los objetos de BG de la cinemática, los objetos de los créditos (`CreditsObject_Process`), los
efectos de paleta, los sprites del final (`DrawCinematicSpriteObjects_Ending`), `CinematicFunction_Intro_Func149` y
la matriz del modo 7. Las escenas:

1. La nave entre las nubes y al espacio; Zebes pequeño.
2. Zebes explota (anillo del modo 7 y destello blanco); la nave viene hacia la pantalla.
3. "THE OPERATION WAS COMPLETED SUCCESSFULLY" y el tiempo, sobre el casco de Samus agrandado.
4. Los créditos suben (texto de colores; terminan en "1994 NINTENDO").
5. Samus (el traje según el tiempo), el destello, el símbolo que gira, "SEE YOU NEXT MISSION" y el porcentaje de
   objetos.

## Decisión de Edwin (2026-11-18)

En la última pantalla, debajo del porcentaje de objetos, mientras gira el símbolo: el texto **"GENESIS DOES..."**
(solo eso), con la letra y los colores de esa pantalla. Lo demás, igual que el SNES.

## Plan

1. La lógica en `src/core` (las funciones del banco $8B del final, los objetos de los créditos), comprobada en el PC
   con `host/check` contra la grabación del SNES, como la intro y Ceres (docs/35, docs/38).
2. El dibujo en la Genesis con lo que ya usan las cinemáticas: imágenes del modo 7 preparadas en el PC (`mkm7.py`),
   salas solo de sprites (`mkmd.py`), el texto de los créditos en un plano.
3. El slogan.

## La lógica (2026-11-19): hecha y comprobada

- **Traducida del decomp** (tools/port, docs/42): `XLATE_BANKS` suma $8B y $8C; en `roots.txt`, antes de la sección
  de los jefes (si no, iba a la página de los jefes), las funciones de $8B:D480 en adelante, los objetos de los
  créditos (`CreditsObject_*`), `DrawCinematicSpriteObjects_Ending` y lo que llaman las escritas a mano. `xlate.py`:
  `ram3000.pause_menu_map_tilemap` es `cinematic_text_tilemap` (7E:3000). `port.py`: $8B y $8C se leen con `B8BW` /
  `B8CW` (nmath.h: bancos fríos de la página 7, `COLD_BANKS` de mkdata.py), no van a `banks.txt`.
- **Despachos** (`tools/port/ending_disp.py` → `src/core/ending_disp.c`): los casos del final de los switch del decomp
  (objetos sprite: 26 inicios, 20 pre-instrucciones, 20 instrucciones; objetos de BG: 3 instrucciones). `cinematic.c`
  e `intro.c` los llaman cuando el caso no es de la intro.
- **A mano** (`src/core/ending.c`): el cuadro del estado 39 (`GameState_39_EndingAndCredits`, game.c), lo que carga
  gráficos o toca el hardware del SNES (las escenas: `CinematicFunctionEscapeFromCebes`, `Func112`, `Func115`,
  `Func118`, `Func126`; `SetupPpu_7_Mode1`, `CinematicUpdateSomeBg`), que en la Genesis son avisos a la capa de
  dibujo (`EndingGfx`, `EndingTextUpload`, `EndingM7Queue`: todavía vacíos), los créditos (`CreditsObject_Func1`: las
  filas de `kCreditsRows`, descomprimidas de $97:EEFF por mkdata.py), el porcentaje de objetos
  (`CalcItemPercentageCount`), el efecto del texto del tiempo (`Func133`), las 40 estrellas (`Func127`, `Func149`: en
  `enemy_data`, que en el final no se usa) y los spritemaps (`DrawSpritemap*` → `DrawCinematicSpritemap`).
  `CopySuperMetroidString` (la marca de juego terminado en la partida guardada) espera al guardado.
- **Comprobación:** `host/check` con la grabación del SNES (`REF=work/final/ref.bin sh tools/compare/final_snes.sh`,
  2,1 GB) desde el cuadro 899 hasta el 16250 (todo el final): 3 cuadros distintos, los del cambio al estado 39 (la
  RAM 7E:198D-1C1E pasa de los proyectiles a la cinemática), y después igual que el SNES hasta el final.
- Regresión: 0 distintos. Código nuevo: ~12 KB a mano (ending.o, ending_disp.o) y ~1500 líneas traducidas.

## Lo que sigue: el dibujo

`EndingGfx` por escena (los gráficos de cada una preparados en el PC desde tu ROM), las imágenes del modo 7
(`mkm7.py`, con la trayectoria del motor en el PC), el texto de los créditos (`EndingTextUpload`: el plano con
`cinematic_text_tilemap`), los sprites de la cinemática (`DrawCinematicSpritemap`), las estrellas y el slogan.
