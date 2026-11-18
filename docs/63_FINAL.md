# 63 — El final y los créditos (en curso)

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
