# 66 — Textos en inglés y español: qué se puede traducir hoy

El idioma se elige en OPTION MODE (ENGLISH TEXT / SPANISH TEXT: `g_lang`, `src/front/menu.c`) y desde docs/65 queda
guardado en la SRAM (0x2000: "MM", idioma, cancelar ícono, caminar hacia atrás); al encender se lee
(`menu_load_options`). `g_lang` vive fuera de las variables del juego (`reset_game_vars` no lo toca): el juego puede
leerlo en cualquier momento.

Regla de los textos: el inglés sale de tu ROM al compilar; el español va en `tools/lang/` (en git: es texto nuestro,
no de Nintendo). Las letras con tilde, la Ñ, ¿ y ¡ se dibujan a partir de las letras de la ROM (la tilde en la fila
de arriba, como hace `make_menu_data.py`).

## Inventario

| texto | de dónde sale | ¿traducible? |
|---|---|---|
| Menú de partidas y opciones (SAMUS DATA, NO DATA, TIME, ENERGY, EXIT, OPTION MODE, controles, ajustes especiales) | `tools/lang/menu.txt` (clave \| inglés \| español) | **sí, ya funciona** |
| Intro: "THE LAST METROID IS IN CAPTIVITY..." y las 5 páginas del diario de Samus | el motor dibuja el texto del SNES (objetos de texto $8C, fuente de la ROM: `mkintro.py`) | **falta conectar**: `tools/lang/intro_es.txt` existe (lo lee `make_menu_data.py`, con el límite de letras por línea) pero está vacío y la intro nativa no lo usa |
| Cajas de mensaje (ítems obtenidos, "¿guardar?", MAP DATA, ENERGY RECHARGED, etc.: 28) | mapas de tiles del banco $85 (`kBank85_MsgBox`, msgbox.c) dibujados con la fuente del BG3 | **no todavía**: son mapas de la ROM; hace falta un `msgbox.txt` y armar los mapas en español con la misma fuente |
| Pausa: MAP / SAMUS / EQUIPMENT, los nombres de los ítems y de las áreas, START/L/R | tiles y mapas de la ROM (`mkpause.py`) | **no todavía**: mismo caso; los nombres son letras de la fuente de la pausa, así que se pueden armar |
| Fin de juego: GAME OVER, FIND THE METROID LARVA!, TRY AGAIN?, YES (RETURN TO GAME), NO (GO TO TITLE) | mapas de menú de la ROM ($81) | **no todavía**: mismo caso (fuente del menú: la misma de `menu.txt`, con tildes) |
| Créditos y pantalla final (SEE YOU NEXT MISSION, CLEAR TIME) | mapas de la ROM (`mkending.py`) | nombres del equipo: no se traducen; las frases sí, con la fuente del final |
| "GENESIS DOES..." (debajo del porcentaje) | `mkending.py` (letras grandes) | **sí**, conviene pasarlo a un `.txt` como `menu.txt` |
| HUD (energía, misiles: números e íconos) | tiles de la ROM | no tiene palabras |
| Título (© 1994 Nintendo) | imagen de la ROM | no se traduce |

## Cómo seguir (propuesta)

1. Un archivo por pantalla en `tools/lang/` con el mismo formato que `menu.txt` (clave | inglés | español). Para los
   mapas de la ROM, la columna en inglés es solo referencia: el inglés se sigue dibujando con el mapa original y el
   español se arma con las letras de la misma fuente (la tabla letra -> tile se saca de la ROM, como en
   `make_menu_data.py`).
2. Los conversores generan las dos versiones y el motor elige con `g_lang` en el punto donde hoy copia el mapa
   (msgbox.c, la pausa, gameover.c, el final, la intro).
3. Por pantalla, el límite de letras (las cajas de mensaje tienen un ancho fijo; la pausa, recuadros).

Orden sugerido: intro (ya tiene archivo y límites), cajas de mensaje, fin de juego, pausa, final.
