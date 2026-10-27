# 49 — Spore Spawn (sala 9DC7, Brinstar)

Problema (Edwin): en la Genesis el jefe Spore Spawn se movía muchísimo más rápido que en el SNES.

## Causa: el traductor leía mal `jp[n]`

Las instrucciones de las listas de enemigos reciben `jp`, un puntero a las palabras que siguen a la instrucción. En el
decomp `jp[1]` es la segunda palabra (2 bytes más adelante); en el motor `jp` es la dirección de 16 bits del banco
y se lee con `IW_XX(jp + 2)`. `tools/port/xlate.py` cambiaba primero `jp[n]` por `IW_XX(jp + 2n)` y después la regla
de `jp + n` (avanzar n palabras) volvía a duplicar ese número: `jp[1]` leía `jp + 4`, `jp[2]` leía `jp + 8`.
Solo `jp[0]` salía bien.

`Draygon_Instr_21` ($A5:E82D) lo usa en la lista de Spore Spawn `$A5:E6D5: E82D 0040 0001` (radio 0x40, paso del
ángulo 1). Con el error, el paso (`ssn_var_0C`, ERAM 7E:7818) tomaba la palabra siguiente, `E8BA`: en
`SporeSpawn_Func_2` ($A5:EB52) el ángulo sumaba 0xBA (−70) por cuadro, casi una vuelta cada 4 cuadros.

Arreglo: se aplica primero `jp + n` y después `jp[n]`. Al regenerar cambian 7 instrucciones, todas para quedar como el
decomp:

| Instrucción | Qué leía mal |
|---|---|
| `Draygon_Instr_1` $A5:94DD | listas de los enemigos 1-3 de Draygon |
| `Draygon_Instr_14` $A5:9E0A | segunda palabra (7E:883E) |
| `Draygon_Instr_21` $A5:E82D | paso del ángulo de Spore Spawn |
| `Torizo_Instr_25` $AA:C2D9 | salto fuera de Crateria (Gold Torizo) |
| `Torizo_Instr_26` $AA:C5A4, `Torizo_Instr_45` $AA:D526 | el segundo destino del salto |
| `SpacePirates_Instr_15` $B2:F564 | tamaño del rectángulo de la garra |

La pelea con Bomb Torizo seguía en 0 diferencias porque en Crateria (`area_index` 0) y con el bot no se tomaban esas
ramas.

## Lo que faltaba para la pelea entera

- `EprojInstr_DC5A` ($86:DC5A, propiedades del tallo) y `EprojInstr_SpawnEnemyDrops_0` ($86:DC61, lo que suelta al
  morir), en `roots.txt`.
- `CreateADudShot` ($A0:8046, copiado en $A5:8046): el disparo que pega en la concha (cajas con disparo 8046) hace el
  humo, el sonido 0x3D y se apaga.
- `kraid_unk9000` (7E:9000) era una variable aparte, pero en el SNES es la misma RAM que la de enemigos:
  ahora es `ERAM(0x9000)` (`inc/core/enemies.h`; `gen_vars.py` no la declara). `Draygon_Instr_22` ($A5:E872) la pone en 1
  mientras Spore Spawn está abierto y `EprojPreInstr_SporeSpawners` ($86:DD46) no suelta esporas.

- **Cuelgue al morir, solo en la Genesis**: `SporeSpawn_Func_6` ($A5:EDF3) hace
  `*(uint16 *)&boss_bits_for_area[area_index] |= 2`. En Brinstar `area_index` es 1: escribe una palabra en una
  dirección impar (7E:D829 → FFD377) y el 68000 da ADDRESS ERROR (en el PC no pasa nada). En el SNES (bajo-alto), un OR
  de 16 bits con un valor < 0x100 solo cambia ese byte: el traductor ahora deja `boss_bits_for_area[area_index] |= 2`
  (regla en `xlate.py`; era el único caso).

## Comprobación

`host/check` contra la pelea grabada en el SNES (`hlref`), con la misma entrada:

    python3 tools/rooms/spore_fight.py          # graba (work/spore) y corre check

- La entrada es la del bot hasta Zebes y después un salto (`HL_WARP=21:8D2A`) a 9DC7, `HL_GOD`, 999 misiles elegidos
  (`HL_POKE=9C6:3E7,9C8:3E7,9D2:1`) y un misil cada 8 cuadros mirando a la derecha.
- Spore Spawn solo recibe daño en el núcleo abierto (caja (−15,−24)-(14,23), disparo `SporeSpawn_Shot`); desde abajo
  y en diagonal lo tapa la concha (cajas con disparo 8046). El guion lleva a Samus con `HL_SETXY` a la izquierda del
  núcleo en cada tramo abierto (x − 70, y + 9) hasta que muere. `HL_SETXY` admite ahora 256 entradas (antes 32) en
  `hlref.c` y `check.c`.

Resultado: Spore Spawn muere en el cuadro grabado 7261 (8 golpes de misil: 960 → 0); **0 diferencias en los cuadros
2-8761** (toda la pelea, la muerte, la caída del cuerpo, la PLM de la salida y 1500 cuadros más). La IA, las listas,
las esporas, el tallo, las paletas de daño (`SporeSpawn_Func_7`) y el bit de jefe quedan iguales al SNES.

En la Genesis (retrorun) también se probó la muerte: energía de Spore Spawn puesta en 1, Samus llevada a la izquierda del
núcleo abierto y disparo (`hold 3 X` en retrorun): muere, queda con la paleta marrón y el bit de jefe
del área 1 puesto; antes del arreglo de `boss_bits_for_area` la consola se colgaba ahí (ADDRESS ERROR).

En la Genesis (retrorun, ROM `TEST=1`, visor: `poke <viewer_debug_next> 2d 2` — 9DC7 es el índice 45 —, Samus a
(200,700)): la trayectoria del enemigo 0 cuadro a cuadro (por `md_shown_frame`) es la del SNES en los 1268 cuadros
capturados (bajada, vueltas, tramo abierto en (185,586), siguiente vuelta). Las únicas diferencias son de un cuadro, por
cómo cae la captura cuando se pierde un cuadro.

Dibujo: el cuerpo (spritemaps extendidos $A5:EE6F-EF61), el tallo y las esporas son sprites; con el fondo de la sala
(BG2) se ven bien en el visor (capturas durante la bajada, las vueltas y el tramo abierto). No se comparó contra una
captura del SNES.

## Regresión

`regress.sh`: intro 0. Ceres y Zebes: 22 cuadros "distintos" (antes 19), los mismos en las dos escenas: 7288-8187 y
10047 (huida de Ceres) más 7448, 8650-8699 (alarma de la huida, explosión), y ya no 7105, 7310, 7330, 7350. No es la
lógica: la base con **dos funciones que nunca se llaman** agregadas a `port_gen.c` da exactamente la misma lista de 22.
Son los cuadros que dependen de la velocidad del código (docs/41, 48). A ojo, lado a lado con la base: diferencias de un
cuadro en el destello de la alarma y en los bordes de la explosión. Falta que Edwin lo vea y apruebe
(`UPDATE=1 sh tests/regress/regress.sh ceres zebes`).
