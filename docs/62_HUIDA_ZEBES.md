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

Con 2 líneas (`MM_MAX_PALS=2`, como en `make`) las salas de la huida juntan algunos colores más (91F8 9261: 3
fusiones, Tourian: 3 a 6), sin que se note. Prueba: `huida.sh` con `SALA=91F8` y `SALA=DE4D`.

## Lo que falta

- **El estado 39** (el final y los créditos, $8B:D443): `nt_unimpl` (docs/46, siguiente bloque).
- **Se acaba el tiempo** en la huida de Zebes (`GameState_36` con el evento 0x0E): `nt_unimpl`.
- Las pruebas con salto no sirven para puertas que se entran desde abajo (Climb → Parlor): sin el impulso hacia arriba,
  Samus cae también en el SNES. Para probar el camino entero hace falta una grabación de la huida jugada.
