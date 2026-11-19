# 67 — El mapa de la partida (estado 5)

Al elegir una partida guardada en Zebes (o YES en el fin de juego), el SNES muestra el planeta con el área de la
partida encendida, un cuadrado que se abre desde el nombre del área hasta el mapa del área con la estación titilando, y
después carga el juego (estado 6). B en el mapa del área vuelve al planeta; B en el planeta, a las opciones.

## En el motor

`src/core/filemap.c`, a mano desde el decomp: `FileSelectMap` ($81:9E3E, `menu_index` 0-22) con el cuadrado
(`expand_sq_*` y las tablas del HDMA de las ventanas en 7E:9E00, alias de `hdma_table_2`), los colores de las áreas,
el scroll del mapa y los íconos (los de la pausa: `PauseFsel_*` en pause.c). Lo no explorado sin la estación del mapa
es 0xF (no 0x1F como en la pausa: `pause_map_blank`). El código va en la página 23 del SSF2 (lo común está lleno):
`game.c` lo llama con `FileSelectMapPaged` (md_filemap.c), que pone la página.

## En la Genesis

- **El planeta**: el SNES suma el BG3 del área al BG1 (matemática de color). `tools/conv/mkfilemap.py` arma la imagen
  ya sumada, una por área, desde la ROM (comparada con una captura del SNES: igual salvo los sprites, que van aparte,
  y una línea de corrimiento del SNES). `md_filemap.c` la dibuja en el plano A por celdas, decidiendo con los mismos
  registros del SNES (TM, TMW, W12SEL y la ventana del cuadrado) si cada celda se ve.
- **Los nombres** ("PLANET ZEBES" y las áreas): sprites desde los spritemaps del menú, encendido y apagado.
- **El cuadrado**: con celdas de 8x8. Lo que el SNES muestra adentro en los pasos 7-8 y 20-21 (el marco del mapa)
  queda negro: sus colores son los de la pausa y no caben en la CRAM junto con los del planeta.
- **El mapa del área**: la pausa (`md_pause.c` con `pz_fsel`): el marco $B6:E000 con el pie de $81:B14B, sin el HUD,
  el nombre del área y los íconos. Los colores fijos salen de la paleta de la pausa (`kPzFselCol`, mkpause.py: la
  línea que usa el SNES es igual en las dos) y los animados de `palette_buffer`. El recuadro de la estación (spritemap
  0x12) queda fuera del reparto de colores de la pausa, para que la pausa no cambie.
- El código del menú de partidas también pasó a la página 23 (`md_main.c` la pone antes de llamarlo).

## Pruebas

En el emulador, con la partida guardada por `tools/compare/partida.sh`: menú → opciones → planeta (el cuadrado se
abre) → mapa del área (centrado en la nave, la estación titilando) → el juego; B en el mapa → el planeta; B en el
planeta → las opciones; muerte → YES → planeta → mapa → juego. Capturas del SNES (snes9x con la misma SRAM) para
comparar. `host/check` (intro y Ceres → Zebes) sin diferencias nuevas; la pausa con los mismos colores que antes.
