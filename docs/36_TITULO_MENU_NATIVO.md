# 36 — Título, menú y juego en una sola ROM (motor nativo)

## Qué hay

`MegaMetroid_Ceres_nativo.bin` arranca ahora con la secuencia completa de antes del juego y sigue con el motor
nativo:

1. Logo "Síndrome", letras rojas, tomas del laboratorio, alejamiento y "MEGA METROID" (`front/title.c`).
2. SAMUS DATA y OPTION MODE con los textos en inglés o español (`front/menu.c`).
3. START GAME → Ceres (motor nativo) → escape → cinemáticas.
4. Fin de juego: YES vuelve a empezar Ceres; NO vuelve al título.

Las ROMs `_demo.bin` y `_demo_tiempo.bin` (entrada de la grabación del SNES) empiezan directo en Ceres, para poder
compararlas cuadro a cuadro.

## Cómo está armado (`native/md`)

| Qué | Dónde |
|---|---|
| Título y menú (de la ROM de prueba `tests/samus_play`, sin cambios de lógica) | `front/title.c`, `front/menu.c`, `front/sound.c`, `front/front.c` |
| Datos del título y del menú (de tu ROM; no se distribuyen) | `gen/front/` con `tools/genesis_convert/make_title_data.py` y `make_menu_data.py` |
| Grabaciones del SNES que usan (no se distribuyen) | `snes_ref/title.rec`, `snes_ref/menu_snaps/` |
| Textos del menú por idioma | `tools/lang/menu.txt` |
| Secuencia y vuelta a empezar | `md_main.c` |
| Mapa de la ROM y de la RAM | `md_ssf2.ld` |

El título y el menú usan SGDK (VDP, sprites, mando). Al empezar el juego, `md_init` arma el VDP a su manera y al
volver al título `VDP_init` lo devuelve a SGDK.

**Volver a empezar sin reiniciar la consola.** `md_ssf2.ld` junta en la RAM todas las variables del juego (las de
todo objeto que no sea de SGDK ni `front_*`): `_game_data_start/_end` y `_game_bss_start/_end`. `play_game()` las
pone como al encender (copia de la ROM y ceros) y llama a `md_init`. Así YES empieza otra partida limpia, y las de
SGDK (mando, `vtimer`, interrupciones) y el idioma elegido en el menú no se pierden.

**Mapa de la ROM.** 3 MB: el juego y, al final, los datos del título (`.rodata_title`). Está preparado el mapeador
SSF2 (páginas de 512 KB más allá de 4 MB en la ventana 0x380000) para cuando llegue la intro.

## Traducción

Todo texto nuevo va por tablas por idioma (`tools/lang/`). El menú ya está así (`g_lang`). Los textos que el juego
dibuja con tiles del SNES (EMERGENCY..., PLANET ZEBES, GAME OVER) se traducirán con tiles propios por idioma en
la etapa de traducción.
