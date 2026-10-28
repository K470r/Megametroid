# 38 — La intro con el motor nativo

La intro del SNES (estado 30: "The last Metroid is in captivity", las páginas del diario, los recuerdos de Mother
Brain, la larva, el científico y el laboratorio, el vuelo a Ceres y "SPACE COLONY") corre con el motor nativo y se
dibuja en la Genesis. Al terminar (estado 31) empieza la partida en Ceres.

Flujo de la ROM: título → menú → **START GAME** → intro (los botones pasan las páginas, como en el SNES) → Ceres.

## Lógica

`src/core/intro.c` es el código del SNES ($8B) portado: funciones de cinemática (`IntroCinematicFunction`),
objetos de fondo (texto que se escribe letra por letra, con su brillo), objetos de sprite (Mother Brain, Rinkas,
huevo, larva...), Samus en los recuerdos con la entrada de demostración y los fundidos cruzados de paleta.

Comprobación: `host/check` contra la grabación del SNES desde que empieza el estado 30: 0 diferencias en los
8456 cuadros de la intro (y desde el cuadro de `CinematicFunction_Intro_Initial`).

El estado inicial de la intro en la Genesis es la RAM del SNES al elegir partida nueva (`mkinit ... -1`, cabecera de
la grabación `REF_INTRO`): `md_init(1)` lo copia y el motor ejecuta `Intro_Initial`.

## Dibujo (`src/platform/md_intro.c`, datos de `tools/conv/mkintro.py`)

La Genesis sigue la escena por los registros del SNES (`IntroScene`, en `intro.c` porque la usan también la Genesis y
el registro de dibujo del PC):

| Escena | Plano A | Plano B | Sprites |
|---|---|---|---|
| CAPT | BG3 (texto) | — (el mismo mapa que A) | — |
| TEXT (diario) | BG3 (texto, cursor) | BG2: el casco, con los ojos que cambia el juego | cursor |
| MB / BABY | BG3 (franjas negras, texto) | BG1: el recuerdo | objetos de la escena, Samus, rayo, cañón |
| SCIENTIST / LAB | BG3 | BG1 | objetos |
| FLY | la nave (dos búferes: uno se ve, el otro se arma) | | estación, asteroides, estrellas (spritemaps grandes compuestos y cubiertos con sprites de hasta 32x32) |
| SPACE | BG1 ("SPACE COLONY") | — | los del vuelo |

- **Plano A**: se sigue lo que el SNES tiene en ese mapa de la VRAM: el mapa inicial ($97:8D12) y, cuando
  `HandleCinematicBgObjects` lo manda, las filas 0-29 de `cinematic_text_tilemap`. Solo se compara cuando el juego
  escribió el mapa (`cinematic_text_dirty`) y solo se convierten las filas que cambian (en el vblank, las que entran).
- **Grupos de gráficos** (CAPT, TEXT, MB, BABY, SCIENTIST, LAB, FLIGHT): cada uno con sus tiles y sus 4 líneas de
  CRAM. Cada color de la Genesis apunta a un índice de `palette_buffer` (fundidos y efectos de paleta tal cual); los
  índices que en el grupo tienen siempre el mismo color comparten uno (`intro_usage.json`). Con Samus la línea 2 es su
  paleta, como en el juego. El cambio de grupo se hace con la pantalla en negro.
- **Fundido cruzado**: el SNES mezcla el texto y la escena con color math. La Genesis muestra una escena a la vez:
  la que se va se oscurece hasta negro y después la que llega se aclara (`IntroScene` da el brillo). En la mitad del
  diario no se dibujan los sprites del recuerdo, y en la del recuerdo no se ve el plano del texto (el juego ya
  escribe allí la página siguiente). No es igual al SNES en tiempo, pero las escenas no se mezclan.
- **Voces** ("The last Metroid is in captivity. The galaxy is at peace."): docs/40.
- **La nave**: cada juego de registros de modo 7 del vuelo (385) ya está dibujado como lo haría el PPU del SNES, en
  tiles (página 20 del SSF2). Cuatro cuadros con más de 512 tiles distintos quedan sin dibujar (se ve el anterior).
- Todo va 2 líneas más arriba que en el juego (sin la ventana del HUD): medido contra capturas del SNES, la imagen
  coincide (diferencia media de 1-2 por píxel fuera de los fundidos).

VRAM durante la intro: planos de 64x32 en $A000 y $C000; tiles del grupo en 1-222, 256-1279 y en los huecos que dejan
los planos chicos y la ventana apagada; 223 el cañón, 224-255 Samus.

## Datos (se generan con tu ROM; no se distribuyen)

1. Grabación de la intro con hlref (`HL_ANY=1`, desde el cuadro en que empieza el estado 30) y la de la intro entera.
2. `tools/snes_ref/intro_usage.py ref_intro.bin data/extracted_full res/snes_ref/intro_usage.json`
3. Registro de dibujo: `MM_DRAWLOG=... host/check ref_intro.bin sm.smc 1 8456 0`, las líneas `C18x` de
   `spr8c`, `proj` y `trail` en `res/snes_ref/intro_draw.log`.
4. `Makefile` con `REF_INTRO=<grabación desde el estado 30>`: `mkinit -1` y `mkintro.py`.

Pruebas: `TEST=1 TEST_INTRO=1` arranca la intro con la entrada grabada (cuadro a cuadro igual que el SNES);
`STOP_AT=N` la congela en el cuadro N para comparar capturas.

## Velocidad

Con los cuadros en cadena (docs/39) la intro entera pierde 2 cuadros (antes ~37: el fundido cruzado del recuerdo de
Mother Brain y el comienzo del vuelo). `ComposeFadingPalettes` solo recompone los colores que cambiaron.
