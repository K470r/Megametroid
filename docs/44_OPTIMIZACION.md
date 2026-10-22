# 44 — Optimización del motor en las salas con enemigos

Objetivo: el mismo juego que el SNES sin perder cuadros en la Genesis. Se mide en cada sala con enemigos (34 de
Crateria + 7 de Brinstar) con la ROM de prueba (entrada grabada `zebes_shoot`) y el visor de salas.

## Herramientas (`tools/perf/salas/`, rutas de esta máquina: ajustar)

- `run.sh [índices]`: compila la ROM de prueba y mide en cada sala (índice de `kViewerRooms`) los cuadros perdidos en
  600 cuadros disparando (300 `X`, 300 `LX`), 400 cuadros después de entrar (la transición de puerta no cuenta).
  También muestra si se llamó algo sin portar (`md_unimpl_count`, mensaje).
- `prof.sh idx [n]`: perfil del 68000 (Genesis Plus GX con `MM_PROF`) en esa sala, por función.
- `hot.py perfil símbolos función rom.out`: dentro de una función, los tramos de 32 bytes que más ciclos llevan, con
  el desensamblado.
- `tl.sh idx`: cuadros perdidos cada 10 cuadros (para ver si es parejo o un pico).

Ojo: el perfil cuenta los ciclos en que el 68000 está parado por una DMA dentro de la instrucción que la arranca.

## Lo que se hizo (2026-10-20)

Cuadros perdidos en las 11 peores salas: 143 → 58 (la peor, 990D: 41 → 5).

1. **Proyectiles del caballo de mar sin portar** (`EprojInit/PreInstr_WalkingLavaSeahorseFireball`): el decomp nombra
   su cabecera `addr_loc_A89E90` (otro banco), el traductor no la reconocía. Las bolas de fuego quedaban quietas en
   (0, 0) para siempre y cada cuadro pasaba por el aviso de "sin portar". Ahora `port.py` toma como cabecera de
   proyectil cualquier dirección que se pase a `SpawnEproj*`. (93AA: 30 → 0 perdidos.)
2. **Tiles de Samus**: el SNES los copia a la VRAM cada cuadro; aquí solo si cambiaron (la DMA para al 68000).
3. **Bloques cambiados** (`lvl_find`): búsqueda directa en su casilla, en línea (antes: llamada + recorrido).
4. **Multiplicaciones y divisiones de 16 bits**: en C, `uint16 * uint16` es una multiplicación de 32 bits por software
   (`__mulsi3`); `Mul16` / `DivMod16` (nt.h) usan `mulu.w` / `divu.w`. También en lo traducido (xlate.py).
5. **Despachos de lo traducido**: tabla ordenada + búsqueda con las últimas recordadas; los proyectiles recuerdan su
   función por casilla y las funciones vacías (nullsub) no se llaman. Las IA de enemigos, igual.
6. `enemy_def` en línea con su caché; colores de los objetos con sus orígenes calculados una vez por sala; el HUD
   convierte solo las celdas que cambiaron; las claves de los proyectiles enemigos usan la misma caché que los
   enemigos.
7. El banco $8C (solo la intro) pasó a bytes: la parte fija de la ROM no entraba en 4 MB.

## Comprobación

- Todas las grabaciones del SNES siguen en 0 diferencias (Crateria, Brinstar, Ceres, ascensor).
- Grabaciones nuevas con `HL_GOD=1` (hlref y check): Samus no muere, 1500 cuadros por sala; antes Samus moría en
  algunas salas a los pocos cuadros y lo de esa sala casi no se probaba (así se escapó el caballo de mar).

## Lo que queda (en orden de peso, sala 92B3 con 4 Yapping Maw y 16 proyectiles de cuerpo)

- Recorrido de los 18 proyectiles (`EprojRunAll`), su dibujo (`md_draw_eprojs_asm`, `md_putrecs`).
- Colisión de enemigos con bloques (`Enemy_MoveRight/MoveDown`, reacciones por tipo de bloque).
- `HdmaObjectHandler`, `md_vblank` (DMA de la lista de sprites), conversión del HUD.

## Lecturas de palabra en direcciones impares (crateria6)

GCC (13, m68k) junta dos lecturas de byte como `p[1] | p[2] << 8` en una lectura de palabra y un `ror.w #8` (paso
"bswap"), aunque la dirección pueda ser impar: en el 68000 eso es un ADDRESS ERROR (pasaba al hacerse bola:
`Samus_AnimDelayFunc_9/12`, tabla de bytes $91:B010). `-mstrict-align` no lo evita; se compila con
`-fdisable-tree-bswap` (`Makefile`). Costo medido: 58 → 61 cuadros perdidos en las 9 salas más pesadas.
Para buscar casos: en el desensamblado, `move.w/move.l (aN)` seguido de `ror.w #8`.
