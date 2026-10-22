# 49 — Rayo cargado y bombas en abanico

Fecha: 2026-10-22. Rama `charge-beam` (desde `dedup-tiles`).

## Problema

Al tomar el Charge Beam (Brinstar, sala 9D19) Samus dejaba de disparar: `HudSelectionHandler_NothingOrPowerBombs`
terminaba en `nt_unimpl` en cuanto `equipped_beams & 0x1000`. Tampoco estaban la animación de la carga ni las bombas
en abanico (la bola con el rayo cargado).

## Qué se portó (decomp snesrev/sm, mismos nombres, dirección del SNES en el comentario)

| Función | Dirección | Archivo |
|---|---|---|
| `HudSelectionHandler_NothingOrPowerBombs` (entera: cargar, soltar, < 60 rayo normal, ≥ 60 cargado) | $90:B80D | proj.c |
| `FireChargedBeam` (tipo `equipped_beams & 0x100F \| 0x8010`, `charged_shot_glow_timer = 4`) | $90:B986 | proj.c |
| `FireSba` (carga llena con la bomba de poder elegida; con el rayo solo no hace nada: `FireSba_ClearCarry`) | $90:CCC0 | proj.c |
| `HandleChargingBeamGfxAudio` (carga e hiperrayo) y `DrawFlareAnimationComponent` | $90:BAFC, $90:BBE1 | proj.c |
| `DrawBeamGrappleSpritemap` (spritemaps de `g_off_93A1A1`) | $81:8A37 | proj.c → capa de la Genesis |
| `BombSpread`, `ProjPreInstr_SpreadBomb` | $90:D849, $90:D8F7 | proj.c |
| `BlockCollSpreadBomb` (con `sub_949D5D`) | $94:A621 | coll.c |

Ya estaban: las paletas de la carga y del destello de Samus (`HandleBeamChargePalettes`, samus_pose.c), los
contadores en el movimiento (samus_move.c), `Samus_CanFireBeam`, `InitializeProjectile` (lee `kProjectileData_
ChargedBeams`), las estelas y la explosión del rayo.

Notas:
- Las direcciones de `FireChargedBeam` y `FireSba` salen de la ROM (el decomp pone la dirección de la tabla anterior
  en el comentario de esas tablas: `kFireChargedBeam_Funcs` "0x90B986" es la función; `kCostOfSbaInPowerBombs` está en
  $90:CC21).
- Los tres contadores del destello (destello, chispas lentas, rápidas) van seguidos en la RAM del SNES y el decomp los
  recorre con un desplazamiento (0, 2, 4); el motor usa dos tablas de punteros (`kFlareFrame`, `kFlareTimer`).
- Falta, con su `nt_unimpl`: el rayo de onda (`WaveBeam_CheckColl`, `ProjPreInstr_WavePlasmaEtc`) y los ataques
  especiales con onda, hielo, spazer y plasma (`FireSba_Fire*`). Llegan cuando se porten esos rayos.

## Datos preparados en el PC

- `mkdata.py`: `kBank90_Flare` ($90:C481-C4B4: `kFlareAnimDelays` y sus listas de tiempos; `bank90_flare`,
  `bank90_flare_word`) y `kBank93` llega hasta $93:A20D (incluye `g_off_93A1A1`, los 54 spritemaps del destello).
- `mkmd.zebes_proj_keys`: las listas del rayo cargado normal (`$93:83D9`, índice 0, 10 direcciones) y los 54
  spritemaps del destello. Se convierten en cada estado de sala de Zebes con los demás proyectiles (`mkzobj.py`, clave
  `(sm, 0, 0xC000)`); las bombas en abanico usan las listas de la bomba (ya estaban).
- `mkzobj.enemy_vram`: los tiles 0x30-0x37 de los sprites comunes están vacíos; en el juego ahí van los 8 tiles del
  rayo (`UpdateBeamTilesAndPalette` $90:AC8D: `kBeamTilePtrs` → VRAM $6300) y la línea 6 es la paleta del rayo
  (`kBeamPalettePtrs`). Ahora la VRAM de la conversión los tiene (los del rayo normal): el rayo cargado usa los tiles
  0x33-0x34 y el destello el color 6 de la línea 6 (los colores 1-5 son los lugares fijos 9-13 del rayo). Las
  estelas del rayo cargado también salen de ahí.
- Capa de la Genesis (`md_engine.c`): `DrawGrappleOrProjectileSpritemap(sm, x, y)` dibuja un spritemap de $93
  (proyectiles y destello); `DrawProjectileSpritemap` la llama con el de cada proyectil.

## Comprobación

- `host/check` contra el SNES (`tools/rooms/chargebeam.sh`): sala 9D19 con la bola, las bombas y el rayo cargado
  (`HL_ITEMS=1004`, `HL_POKE=9A6:1000,9A8:1000`), entrada `in_chargebeam.txt` (cargar y soltar parado, caminando,
  hacia arriba, en el aire, soltar antes de 60, bombas en abanico con y sin abajo): **0 diferencias en 2126 cuadros**
  (modo libre y modo 1). Con el código anterior: diferencias desde el cuadro 260 y `SIN PORTAR: ... (rayo cargado)`.
- Ceres → Zebes (`ref_full.bin`, cuadros 97-1822): 0 diferencias, igual que antes.
- Genesis (retrorun, ROM `TEST=1`): visor a la sala 9D19 (índice 43), `poke` a `collected_items`/`equipped_items` =
  0x1004 y `collected_beams`/`equipped_beams` = 0x1000; con `X` (disparar en retrorun) se ve el destello junto al
  cañón, Samus en amarillo al soltar, el disparo cargado y su explosión, y las 5 bombas en abanico. `md_missing_ring`
  sin claves de proyectil.
- `regress.sh` (mismo entorno, gcc 13.2 de Ubuntu): intro 0 distintos (2 corrimientos de una línea). Ceres y Zebes:
  62 cuadros "distintos" (7105-8650) contra los 19 de la rama base, que en este entorno da exactamente los 19 de
  docs/48. Las salas de Zebes de la prueba no suman ninguno. Son la misma causa que docs/48: el código y `kBank93`
  crecen, se corren las direcciones de la parte fija y en la huida de Ceres y la llegada a Zebes algunos cuadros del
  motor se muestran uno o dos cuadros de la consola antes o después (en la huida, con la alarma que parpadea, eso
  cambia la imagen comparada). Comprobado: la imagen del cuadro 7330 del motor (cuadro 7474 de la consola) es
  idéntica píxel por píxel a la que muestra la rama base un cuadro de consola más tarde; las de los cuadros 8689
  y 9360 son iguales a la vista. No se aprobaron (`UPDATE=1`): lo decide Edwin.
