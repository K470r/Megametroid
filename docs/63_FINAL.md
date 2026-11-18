# 63 — El final y los créditos (en curso: la lógica está; el dibujo, desde los créditos)

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

## La lógica: hecha y comprobada

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
- **Comprobación:** `host/check` modo 2 con la grabación del SNES (`REF=work/final/ref.bin sh
  tools/compare/final_snes.sh`, 2,1 GB) del cuadro 899 al 16250 (todo el final): **0 cuadros distintos**.
  Corrección: una primera versión de esta nota decía que ya coincidía, pero la prueba era falsa (el modo 0 se paraba
  en el cuadro 903, las variables de la cinemática se cargaban del SNES porque el estado 39 no contaba como
  cinemática, y la función de la escena no se despachaba: el final no arrancaba). Arreglos: el despacho de las
  funciones de la cinemática (`EndingCinematicFunction`, ending_disp.c); en `host/check`, el estado 39 cuenta como
  cinemática, las estrellas (en `enemy_data`) se comparan con las del SNES en 7E:0E0C y el texto de 7E:3000 se compara
  desde que se arma; los alias de la RAM del SNES (`cinematic_var21-26` = `cinematic_glow_ptr[0-5]`; en el banco $8B
  `eproj_pre_instr[0/1]` = `cinematic_glow_ptr[6/7]`); y la onda del texto del tiempo (objeto de HDMA $88:EC8A:
  instrucción $88:EC9F y pre-instrucción $88:ECB6, hdmaobj.c).
- Regresión: 0 distintos. Código nuevo: ~12 KB a mano (ending.o, ending_disp.o) y ~1500 líneas traducidas.

## El dibujo (2026-11-19): de los créditos a la pantalla final

Escenas del final (cuadro grabado k = cuadro 23500 + k del SNES; el motor de la Genesis va por los mismos cuadros):

| cuadros | escena | SNES | Genesis |
|---|---|---|---|
| 900-1302 | la nave sale: Zebes de cerca (modo 7) y las nubes (sprites grandes) | modo 7 + OBJ | **pendiente** (negro) |
| 1303-2246 | el espacio, Zebes chico y su explosión | modo 1, color math | **pendiente** |
| 2247-3456 | el casco y el tiempo | modo 7 + texto con onda | **pendiente** |
| 3457-12566 | créditos; Samus (con traje o sin él según el tiempo) | modo 1: BG1 texto, BG2 Samus, OBJ | hecho |
| 12567-13303 | Samus chica y el símbolo que gira | OBJ (y el símbolo en modo 7 desde 13088) | sprites hechos; el modo 7, pendiente |
| 13304- | "SEE YOU NEXT MISSION", el porcentaje y **"GENESIS DOES..."** | modo 1: BG1 texto, BG2 el símbolo | hecho |

Cómo (como la pausa, docs/53):
- **Datos** (`tools/conv/mkending.py` → `md_ending_data.c`, todo en la página 24 del SSF2, 40 KB): por grupo de escenas
  que comparten la VRAM del SNES (D1 créditos y Samus, D2/D2B el símbolo, D3 la pantalla final), desde una captura de
  la VRAM del SNES con tu ROM (`snes_ref/END_*.bin`, `make_inputs.sh`) y lo que el juego dibuja
  (`snes_ref/ending_draw.log`: palabras de mapa y spritemaps, solo números; lo arma `tools/snes_ref/ending_log.py`
  desde `MM_ENDLOG` de `host/check`): tiles de la Genesis, tabla palabra del SNES → celda (por tile y paleta), la
  pantalla fija del BG2, los sprites y los colores por índice de `palette_buffer` (los fundidos y brillos del final
  cambian los colores: la Genesis los copia en cada cuadro).
- **Genesis** (`src/platform/md_ending.c`, incluido al final de md_engine.c; `md_post` lo llama en el estado 39):
  plano A = BG1 (las filas de 7E:3000 que el juego sube: `EndingTextUpload` y `UpdateCinematicBgTilemap`; solo las
  filas que cambiaron, por suma), plano B = BG2 fijo; las capas apagadas (TM) se esconden con el scroll (columnas
  32-63 vacías); los sprites, con `DrawCinematicSpritemap` y las estrellas (`EndingStar`, nuevo aviso de
  `CinematicFunction_Intro_Func149`); sin sprites si TM/TS no los tienen; los que el SNES solo suma por el color math
  van detrás del BG2.
- **El slogan:** "GENESIS DOES..." con la letra grande blanca de "SEE YOU NEXT MISSION", en las filas 16-17 (justo
  debajo del porcentaje), desde que aparece el porcentaje.
- **Comparación** lado a lado: `tools/compare/final.sh` (como despegue.sh).

Diferencias que quedan en lo hecho: el SNES suma capas con el color math (el brillo azul del traje con el BG3, Samus
sin traje que aparece sumada sobre la del traje): en la Genesis se ve una u otra.

## Lo que sigue

Las escenas en modo 7 (Zebes, el casco, el símbolo: imágenes preparadas como `mkm7.py`, con la matriz del motor), las
nubes (sprites de 64x64 del SNES: en un plano), el espacio con la explosión, y los otros finales según el tiempo
(Samus con o sin traje: capturas de cada uno).
