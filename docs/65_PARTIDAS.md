# 65 — Las partidas guardadas

## La SRAM

El cartucho tiene 8 KB de SRAM en bytes impares (cabecera "RA", 0xF820, 0x200001-0x203FFF). La Genesis la pone sobre
la ROM desde 0x200000 al escribir 1 en $A130F1, así que el código que la lee (`src/platform/md_sram.c`,
`nt_sram_read` / `nt_sram_write`) va en la ROM baja (`.text.keepboot`) y con las interrupciones apagadas. En el PC
(`host/pc_stubs.c`) es memoria; con `MM_SRAM=archivo` se lee y se escribe en un archivo.

Los bytes 0x0000-0x1FFF son la SRAM del SNES tal cual (se puede pasar una partida de un lado al otro):

| dónde | qué |
|---|---|
| 0x0000 / 0x0008 | suma de cada ranura y la suma al revés |
| 0x1FF0 / 0x1FF8 | las mismas, copiadas |
| 0x1FEC / 0x1FEE | la última ranura elegida y al revés |
| 0x0010, 0x066C, 0x0CC8 | las ranuras A, B y C: la RAM del SNES 7E:D7C0-DE1B |

Cada ranura: el equipo (copia de 7E:09A2-0A01), los eventos, objetos, puertas, mapas y estaciones (7E:D820-D91B; en
D914 `loading_game_state`, en D916 la estación y en D918 el área donde se guardó) y el mapa explorado comprimido
(7E:D91C: solo los 327 bytes que pueden tener algo, las listas de $81:82D6; mkdata.py las deja en `kSaveMapIdxB`).

Después (0x2000) van las opciones de la Genesis: "MM", el idioma, cancelar ícono y caminar hacia atrás.

## En el motor

`src/core/save.c`, a mano desde el decomp: `SaveToSram` ($81:8000) y `LoadFromSram` ($81:8085). En el motor esa RAM
son variables nativas: se lee y se escribe por dirección (`nt_ram_w`, `nt_ram_set_w`) y el mapa comprimido se arma
al escribir. Se guarda donde el SNES: la estación de guardado (plm.c), la nave (gunship.c: al aterrizar y con la caja
"¿guardar?", ahora con su caja de mensaje), la huida de Ceres (game.c) y la partida nueva (intro.c).

## En la Genesis

- **Menú de partidas** (`src/front/menu.c`): cada ranura con su energía, tanques y hora como `DrawFileSelectionHealth`
  / `DrawFileSelectionTime` ($81:A087 / A14E), o NO DATA. El cursor empieza en la última ranura usada.
- **Elegir una ranura** (md_main.c): con partida, `StartLoadedGame` (save.c): `LoadFromSram` y lo que sigue en el SNES
  según `loading_game_state` (0x1F Ceres, 0x22 la huida de Ceres, 5 el mapa de la partida, docs/67, y después el
  estado 6: cargar en la estación). Sin partida, la intro (partida nueva en esa ranura).
- **Fin de juego, YES**: `LoadFromSram` como el SNES; en Ceres otra vez Ceres, si no el estado 5 (el mapa de la
  partida, dentro del motor, como en el SNES).
- **Copiar y borrar partidas** (DATA COPY / DATA CLEAR, aparecen si hay alguna partida): elegir la ranura (y el
  destino), confirmar con YES / NO; se copian o se ponen a cero los 0x65C bytes y las cuatro sumas, como
  `FileSelectMenu_13` / `_25` ($81:9A2C / $81:9C9E). Los textos, en `tools/lang/menu.txt`.

## El sonido y la SRAM (MiSTer)

Con la SRAM puesta ($A130F1 = 1) un cartucho de verdad la repite en toda la zona 0x200000-0x3FFFFF; Genesis Plus GX
la pone solo en 0x200000-0x203FFF (su comentario: "normally mirrored up to $3fffff but this breaks Sonic Megamix").
El MiSTer hace como el cartucho. El Z80 (XGM2) lee la música de la ventana 6 (0x300000) y efectos de la parte fija:
si leía mientras el 68000 tenía la SRAM puesta, recibía basura y el sonido quedaba roto (al entrar al menú de
partidas, que lee las tres ranuras, y al guardar). `md_sram_copy` (md_sram.c) ahora le quita el bus al Z80 mientras
la SRAM está puesta.

Prueba: `tools/emu_harness/gpgx_sram_mirror.patch` (Genesis Plus GX con la SRAM repetida hasta 0x3FFFFF, como el
hardware; aplicarlo en una copia de `third_party/gpgx` y compilar con `make -f Makefile.libretro`). Con la ROM de antes
el audio se aparta del normal desde que el menú lee las ranuras; con la de ahora es igual muestra por muestra.

## Pruebas

- `host/savetest`: guardar, borrar y cargar en el PC; el equipo, los eventos y el mapa vuelven iguales, y una ranura
  con un byte cambiado no carga.
- `tools/compare/partida.sh`: la entrada del bot hasta Zebes en el SNES (hlref, el decomp: `saves/sm.srm`) y en la
  Genesis (ROM de prueba, SRAM del emulador a cero): **los 8 KB iguales** salvo 7E:1FEE-1FEF (la última ranura al revés,
  que escribe el menú de partidas y la ROM de prueba salta). Para eso el motor ahora tiene `debug_flag` y 7E:09E8 (los
  pone a 1 NewSaveFile y van en la ranura) y escribe la firma de 1FE0 ("madadameyohn" al encender sin partidas,
  "supermetroid" al terminar el juego).
- En el emulador, con la ROM normal y esa SRAM: el menú muestra la ranura A (ENERGY 99, TIME 00:02), al elegirla se
  empieza junto a la nave; con la energía a 0, el fin de juego y YES vuelven ahí.
- `host/check` (intro y Ceres -> Zebes): las mismas 3 diferencias de antes (el tramo de Crocomire en
  `cinematic_text_tilemap`), ninguna nueva.

## Falta

- Elegir una partida desde el menú empieza con la RAM del comienzo (md_init) más la partida cargada: el SNES llega
  ahí desde sus menús, con la RAM que dejaron. El estado 6 vuelve a armar casi todo.
