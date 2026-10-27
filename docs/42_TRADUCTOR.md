# 42 — Traductor del decomp (enemigos de Crateria)

Los enemigos y sus proyectiles no se escriben a mano uno por uno: `tools/port` traduce las funciones del decomp
(`sm/src`, la biblia) al motor nativo de 68000. El resultado es `src/core/port_gen.c` / `port_gen.h` (generados:
no se editan; se cambia el traductor o se escribe la función a mano en otro archivo y deja de generarse).

## Uso

    python3 tools/port/port.py          # lee tools/port/roots.txt, escribe port_gen.c/.h y tools/port/banks.txt
    sh host/build.sh             # mkdata.py saca de la ROM de cada quien los bancos de banks.txt (kBankXXB)

## Piezas

- `cgraph.py`: grafo de llamadas del decomp (también las tablas de punteros a funciones).
- `xlate.py`: las reglas de traducción (estructuras `Enemy_X` -> `EnemyData` y la RAM extra `ERAM`, tablas de la ROM
  -> lecturas `BW_XX/BB_XX` por dirección, macros, listas de instrucciones con punteros -> direcciones de 16 bits,
  tablas que no están en la ROM -> se copian del decomp tal cual).
- `port.py`: cierra el conjunto de funciones desde `roots.txt` (familias `Prefijo_*` o nombres), arma los despachos
  (`port_CallEnemyAi`, `port_CallEnemyInstr`, `port_CallEnemyPreInstr`, `port_CallEprojInit/PreInstr/Instr`,
  `port_call_void`) y declara las funciones escritas a mano que usa lo traducido (con su prototipo nativo).
  Lo de bancos que no se traducen y no está escrito a mano queda vacío y avisa (SIN PORTAR).
- `inc/core/dcompat.h`: LOBYTE/HIBYTE, multiplicaciones y divisiones del SNES, etc. para el 68000.
- `src/core/port_lib.c`: funciones pequeñas de otros bancos (Multiply16x16...).

## Detalles que importan

- Las instrucciones comunes de los enemigos (`EnemyInstr_*`, copiadas al principio de cada banco) leen la lista del
  banco del enemigo: `IW_EN(a)`.
- Bancos guardados como bytes (`kBankXXB`): la palabra se arma con dos lecturas y un `asm` vacío en medio; si no, gcc
  las junta en una lectura de palabra que en el 68000 falla con direcciones impares (Address Error). Los arreglos de
  bytes van alineados a 2.
- Bancos poco usados (81, 82, 86, 8B, 8D) están como bytes y no como palabras: la parte fija de la ROM (4 MB) no
  alcanzaba.

## Verificación

`tools/rooms/crall.sh` (ver docs/41): las 34 salas de Crateria con enemigos, grabadas en el SNES con un patrón de
disparos, comparadas cuadro a cuadro con el motor (`check`). Hoy: 0 diferencias en todas, salvo las 3 en que Samus
muere (la secuencia de muerte, estados 19-24, falta portar). Las grabaciones de Ceres siguen en 0.

## Gráficos de los enemigos en la Genesis (claves de dibujo)

Cada sala lleva ya convertidos los spritemaps que sus enemigos pueden dibujar (`mkzobj.py`, dentro de `mkzebes.py`).
La lista sale de `res/snes_ref/zdraw.log`, que junta:

- `zcover` (fuzz en el PC con `MM_DRAWLOG`): entra a cada sala y juega con un mando al azar, Samus sin morir;
- `mkzkeys.py` (recorrido estático de las listas de instrucciones): todo lo que una lista puede mostrar, también las
  explosiones al morir (`death_anim` del enemigo: listas $86:EFD5; los tipos 3 y 4 crean objetos de sprite con
  `CreateSpriteAtPos`, listas de $B4:BDA8), el humo al recibir un disparo y lo que pueden soltar.
- `merge.py` junta los registros (las claves de destello se quitan si son pocas).
- (docs/49) `mkzkeys.py` también agrega los proyectiles que crean los PLMs de la sala (recorre sus listas: `plm_walk`;
  p. ej. los trozos de la estatua del Bomb Torizo, la estación de guardado), sigue tablas de listas
  (`BW_86(0xA7AB + 2 * i)`, `Eproj_InitXYVelRandom`) y las pre-instrucciones puestas por una lista
  (`EprojInstr_SetPreInstr_`). Las familias que ponen `palette_index = 0` en su inicio dan proyectiles con g = 0.
- `port.py` agrega `nt_pal_dirty` después de cada escritura a `palette_buffer` del código traducido (la Genesis solo
  sube los colores marcados).

En la Genesis, `md_missing_ring` (RAM) guarda las últimas claves que no se encontraron: el recorrido de prueba de todas
las salas con enemigos tiene que dejarlo vacío.
