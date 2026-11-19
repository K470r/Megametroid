# 64 — La muerte de Samus

Referencia del SNES: `tools/compare/muerte.sh` (la pelea con el Bomb Torizo de `lado_a_lado.sh` y, en el cuadro
grabado `N` (300), la energía de Samus a 0 con `HL_POKE_AT`). Con hlref y `REF=archivo` se graba la RAM por cuadro
para `host/check` (el mismo `HL_POKE_AT`).

## Lo que hace el SNES

Samus sin energía (`HandleSamusOutOfHealthAndGameTile`, $82:DB69): con tanques de reserva en automático se rellena
(estado 27); si no:

| estado | qué | |
|---|---|---|
| 19 | se guarda la paleta y se preparan los fundidos | el juego sigue corriendo |
| 20 | todo se oscurece salvo Samus; al terminar quedan solo los sprites (TM = 0x10) y suena la música de la muerte | |
| 21 | espera la música; Samus gira hacia la cámara (`StartSamusDeathAnimation`) | |
| 22 | la animación (`Samus_DrawStartingDeathAnim`) | |
| 23 | titila (dos paletas) y sube a la VRAM los tiles de los pedazos del traje ($9B:8000-93FF) | |
| 24 | el traje explota (spritemaps de Samus 2076-2093) y la pantalla se pone blanca | |
| 25-26 | se apaga y el menú de fin de juego (gameover.c, el mismo del tiempo agotado en Ceres) | |

Con "YES" el SNES carga la partida guardada: eso es el guardado (pendiente).

## En el motor

- `src/core/death.c` (a mano desde el decomp, `-Os`: lo común de la ROM está lleno): la secuencia del banco $9B, las
  paletas (`kDeathPalsB`, mkdata.py, en la página fría 7) y los tiempos (`kDeathTimes`).
- `game.c`: los estados 19-24 y 27 pasan por `PauseGameState`, que ya tenía la única llamada a `RunGameState(8)`
  (otra llamada hacía que gcc copiara toda la máquina de estados: 27 KB). El borrado de 7E:1A8D-1B8C del estado 20
  va por dirección (`nt_clear_ram`): en el motor esas variables no están en el mismo orden.
- `samus_move.c`: `Samus_DrawStartingDeathAnim` y `Samus_DrawDuringDeathAnim` ($90:8976/8998).
- **Comprobación:** `host/check` del cuadro 250 al 645 (de la muerte al menú): 0 cuadros distintos.

## En la Genesis

`src/platform/md_death.c` (incluido al final de md_engine.c): al terminar el apagón (`SamusDeathGfx`) se borran los
planos, se esconde el HUD y se cargan los 160 tiles de los pedazos (`tools/conv/mkdeath.py`, página 23) donde estaban
los de la sala; la CRAM: el fondo (color 0), la línea de Samus (palette_buffer 192-207) y la línea 1 para la paleta 7
(240-255). `md_post` pasa por `md_death_frame` hasta el menú. Los spritemaps 2076-2093 salen de `kDeathRecs` (los
16x16 partidos en 8x8).

Diferencia: el último fantasma del traje es casi blanco sobre blanco (0x77BD sobre 0x7BDE); con 3 bits por canal en la
Genesis los dos son el mismo blanco.
