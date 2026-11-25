# 62 — La huida de Zebes

Pruebas: `tools/compare/huida.sh` (sala por sala, con salto: `SALA=DE4D|DE7A|DEA7|DEDE|96BA|92FD|91F8`) y
`tools/compare/despegue.sh` (la nave).

## El camino

Mother Brain (DD58) → DE4D → DE7A → DEA7 → DEDE (Tourian, salas solo de la huida) → Climb (96BA) → Parlor (92FD) →
aterrizaje (91F8). En Crateria la huida es otro estado de sala (evento 0x0E: 91F8 9261, 92FD 9348, 96BA 9705, 9804
984F, 9879 98C4). El contador lo pone Mother Brain al morir (`timer_status = 2`, `CallSomeSamusCode(0xF)`:
`frame_handler_gamma = Samus_Func3`, que lo baja y lo dibuja); las pruebas lo ponen con pokes.

## Lo arreglado (2026-11-18)

- **Las explosiones al azar** (`RoomCode_GenRandomExplodes` $8F:C1A9: objetos de sprite de las listas 3, 9, 12, 18,
  21 de $B4:BDA8) no se dibujaban: sus spritemaps no estaban en el registro de dibujo (nadie jugó la huida en el PC).
  `mkzobj.explode_keys` las agrega en los estados cuyo código principal las genera (`EXPLODE_CODES`: C120, C124,
  C131, C183, E57C, E5A0, E5A4).
- **El despegue** (`GunshipTop_17-21`, gunship.c): faltaba. Al entrar a la nave con la bomba puesta: los tiles de la
  nave encendida (`$94:C800` → 7600-7FFF del SNES, cinco cuadros: `nt_enemy_vram_copy`, convertidos en el PC con
  `mkzobj.ROOM_VRAM_OPS` del estado 9261, sin copia al cargar), el temblor, el polvo (`stru_86A379`), la subida, la
  aceleración y el estado 38 (`GameState_38_SamusEscapesFromZebes`, game.c: el fundido y el 39).
- **Claves de un estado** (`res/snes_ref/zdraw_estados.log`, `mkzobj.read_state_logs`): las del despegue (el polvo,
  las explosiones) van solo en 9261. En todo 91F8 cambiaban la fusión de colores y las piezas de la nave al llegar de
  Ceres (la regresión daba 867 cuadros distintos).
- `host/check` y `MD_TEST_SETXY_SCROLL` (md_main.c): como `HL_SETXY_SCROLL` de hlref (la cámara sigue a Samus).

En el PC (`host/check` con la grabación del SNES) el despegue da igual al SNES cuadro a cuadro (3 cuadros distintos,
al final, al pasar al 39).

## Los colores que cambian (2026-11-25)

El cielo del aterrizaje en la huida salía de un solo color (el SNES: franjas y nubes moradas) y en Tourian la alarma
ponía azules partes que en el SNES son rojas. La causa era la misma: `mkzebes` junta en un lugar de la paleta de la
Genesis los índices del SNES que al cargar dan el mismo color, y el motor copia a ese lugar el color de uno solo de
ellos; pero los efectos de paleta del estado (palfx) cambian unos y no otros. En Tourian la alarma pone rojos los
índices 56-59 (paleta 3) y no los 5-7 (paleta 0), del mismo azul al cargar; en el aterrizaje la huida cambia 65-71 y
81-91 (el cielo).

`mkzebes.palfx_traj` hace lo del SNES con los datos de la ROM: los bits `palette_fx_bitset` de los efectos del estado
(`FxDef`, byte 13 de cada entrada de `$83`; `LoadFXHeader` $89:AB82) eligen objetos de la lista del área
(`kAreaPalFxListPointers` $83:AC46), y de cada uno se sigue su lista de instrucciones de `$8D` como
`PalFx_ProcessOne` ($8D:C54A): el temporizador, los colores seguidos desde su índice, los saltos de índice
(`ColorPlus`), `Goto`, `DecTimerGoto`, `SetColorIndex`... (1200 cuadros; las preinstrucciones no se siguen: alcanza con
saber qué índices cambian y a qué colores). Cada índice que cambia va con su trayectoria en colores de la Genesis
(`IdxCol`): comparte lugar solo con otro del mismo color que cambie igual. La trayectoria va en la clave del estado
(los estados con efectos distintos no comparten la conversión: 91F8 tiene ahora 9261, 9247 y 922D, el de la lluvia).

Solo separa un índice si en la Genesis cambia de verdad (un efecto que no sale del nivel del DAC del color de la
carga no separa: si no, se agregaban tiles sin nada que ganar). Al juntar colores (con 2 líneas, `MM_MAX_PALS=2` como
en `make`), la distancia entre dos colores es el promedio en el tiempo (`tdist`: un índice negro al cargar que la
lluvia pone gris no se junta con el negro fijo) y los que cambian se juntan primero entre ellos (`side`: juntado con
uno fijo, el lugar sigue al que cambia; el negro de las rocas de la cueva de la huida salía morado).

## El cielo de la superficie (2026-11-25)

En la llegada con lluvia (91F8, estados 9213 y 922D) y en la huida, el cielo (`RoomMainAsm_ScrollingSky`, las filas de
`$8A`) son franjas lisas de grises o morados muy cercanos: en el SNES van de 8 en 8 y en la Genesis, con un nivel del
DAC cada ~30 en lo oscuro, caían casi todos en el mismo color y el cielo salía liso. Tres cosas:

- **La bruma de la lluvia** (fx 0x0A): en el SNES el BG3 de la lluvia se suma a todo (G+1 B+2 de 5 bits en el
  aterrizaje), justo lo que separa las franjas. Estaba apagada (rehacer las tablas del líquido cada cuadro atrasaba
  el juego); ahora va en el tinte (`tint_on`, md_engine.c, como la niebla), que se rehace solo cuando cambia.
- **Los colores del cielo pesan al juntar**: en las salas con cielo `convert` cuenta los píxeles del nivel y del cielo
  (`usage`, `sky`), así las franjas no se juntan antes que colores que casi no se ven.
- **`kCramFine`** (página 7, `mkzebes.cram_fine_table`): para los lugares que usan los tiles del cielo
  (`ZebesGfx.skym`), cuánto sube o baja cada canal respecto del más cercano. Los casi neutros eligen entre gris,
  gris azulado y marrón/oliva por brillo (probados: con verdes las montañas del fondo salían verdes; con lila, rosada
  la franja de arriba); los demás, el brillo más parecido sin torcer el matiz (los morados de la huida). Lo que da
  negro queda negro. `fine_fix` (md_engine.c) rehace esos lugares después de convertir la paleta (con el tinte),
  recordando el último color de cada uno (la tabla solo se lee cuando cambia).

Costo: en la bajada de la nave (el fundido de la llegada, la paleta cambia cada cuadro) el motor tarda unos 16 cuadros
más en ~220; con Samus en la sala, igual que antes.

Prueba: `huida.sh` con `SALA=91F8` y `SALA=DE4D`; la llegada, con la ROM de la regresión (escena zebes, cuadros del
motor 10083-11017) contra hlref con la entrada del bot (`in_z2.txt`, cuadro del SNES = 10757 + cuadro del motor).

## Lo que falta

- **El estado 39** (el final y los créditos, $8B:D443): `nt_unimpl` (docs/46, siguiente bloque).
- **Se acaba el tiempo** en la huida de Zebes (`GameState_36` con el evento 0x0E): `nt_unimpl`.
- Las pruebas con salto no sirven para puertas que se entran desde abajo (Climb → Parlor): sin el impulso hacia arriba,
  Samus cae también en el SNES. Para probar el camino entero hace falta una grabación de la huida jugada.
