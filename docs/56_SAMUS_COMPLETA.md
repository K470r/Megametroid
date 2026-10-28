# 56 — Samus completa: inventario de lo que falta portar

Fecha: 2026-10-28. Rama `samus-completa` (desde `main`).

Objetivo (Edwin): **todas** las mecánicas de Samus y de sus proyectiles portadas y funcionando como en el SNES, para
que en el juego solo falte ir consiguiendo las habilidades. La biblia es el decomp snesrev/sm (`sm/src`, bancos $90,
$91, $9B y $93; el gancho choca con bloques en $94).

## Cómo se armó el inventario

1. `grep -rn 'nt_unimpl(' src/core` en `samus_move.c`, `samus_pose.c`, `samus_codes.c`, `samus_draw.c` y `proj.c`
   (63 llamadas) y los despachos con `nt_unimpl_v` (`CallProjPreInstr`, `CallSomeSamusCode`, `Samus_CallInputHandler`,
   `HandleControllerInputForGamePhysics`, `HandleSamusMovementAndPause`, `CallSamusDrawHandler`).
2. Las funciones del decomp de los bancos $90/$91/$9B/$93 (680 con dirección) contra las definidas en `src/core` y
   las direcciones que aparecen en los comentarios del motor: **340** no tienen nada que las porte. De esas, unas 60
   son de la demo de la intro y del arranque (ya resueltas de otra forma: `intro.c`), de Mother Brain/Draygon
   (`SomeMotherBrainScripts_*`, `Samus_*Draygon*`) o la muerte de Samus (banco $9B, otra tarea: docs/46 punto 4).

En `port_gen.c` (traductor `tools/port`) las que tocan a Samus son tres: `GrappleBeam_Func2` (lo llaman los
enemigos al soltar el gancho), `Samus_InputHandler_E913` y `Samus_InputHandler_E91D` (los nombres solo se usan como
valor de `samus_input_handler`; no se llaman). Los demás `nt_unimpl` de `port_gen.c` son de enemigos.

## Inventario por mecánica

Formato: **dónde está hoy el `nt_unimpl`** → funciones del decomp que faltan (dirección del SNES).

### Movimiento

| Mecánica | `nt_unimpl` hoy | Falta del decomp |
|---|---|---|
| Salto espacial | `samus_move.c` `Samus_Movement_03_SpinJumping` | la rama del space jump de `Samus_Movement_03_SpinJumping` $90:A436 (`Samus_SpinJumpMovement` $90:9040 ya está) |
| Speed booster | `Samus_HandleExtraRunspeedX`, `Samus_HandleSpeedBoosterAnimDelay` | `Samus_HandleExtraRunspeedX` $90:973E (rama con `equipped_items & 0x2000`), `Samus_HandleSpeedBoosterAnimDelay` $90:852C, `Samus_SpeedBoosterShinePals` $91:DAC7, rama del speed booster de `Samus_HandleScrewAttackSpeedBoostingPals` $91:D9B2, ecos `ProjPreInstr_SpeedEcho` $90:D4D2 |
| Shinespark | `SamusFunc_F468_NormalJump`, `SamusFunc_F468` (índice 27), `Samus_MovementHandler_Normal` (tipo 27), `Samus_DrawEchoes`, `DrawPlayerExplosions2` | `Samus_Movement_1B_ShinesparkEtc` $90:A7DA, `Samus_Input_1B_ShinesparkEtc` $91:81A1, la rama del shinespark de `SamusFunc_F468_NormalJump` $91:F543, `Samus_MoveHandlerShinesparkWindup` $90:D068, `…VerticalShinespark` $90:D0AB, `…_Shinespark_Diag` $90:D0D7, `…_Shinespark_Horiz` $90:D106, `Samus_ShinesparkMove_X/Y` $90:D132/D1FF, `Samus_EndSuperJump` $90:D2BA, choque `Samus_MoveHandler_ShinesparkCrash_0/1/2` $90:D383-D3CC, `…CrashEchoCircle` $90:D3F3, `…CrashFinish` $90:D40D, `Samus_DrawEchoes` $90:87BD, `Samus_DrawEcho` $90:8855, `Samus_DrawShinesparkCrashEchoes` $90:88BA, `Samus_DrawShinesparkCrashEchoProjectiles` $90:8953, `Samus_DrawManyEchoes` $90:EC03, `Samus_DrawHandler_EndOfShinespark` $90:EBF3, `Samus_HandleShinesparkingPals` $91:DB3A, `Samus_HitInterrupt_Shinespark` $90:DEBA, `Samus_Func25_ShineSpark` $90:F04B, `Samus_MoveHandler_F072` $90:F072, `Projectile_Func7_Shinespark` $90:CFFA, `InitializeShinesparkEchoOrSpazerSba` $93:8163 |
| Screw attack | `Samus_HandleScrewAttackSpeedBoostingPals` (screw attack) | rama del screw attack de $91:D9B2 (paletas `kSamusPal_ScrewAttack`); el daño a enemigos ya está en el marco de enemigos ($A0: `EnemySamusCollHandler`, `samus_contact_damage_index`) — comprobar |
| Bola resorte | `Samus_MovementHandler_Normal` (17-19), `HandleJumpTransition` (18), `Samus_MorphBallBounceSpringballTrans` | `Samus_Movement_11/12/13_SpringBall*` $90:A69F/A6F1/A703, `Samus_Input_11/12/13` $91:814F-815F, `SamusFunc_F468_Springball` $91:FA56, `HandleJumpTransition_SpringBallInAir` $91:FC18, el resto de `Samus_MorphBallBounceSpringballTrans` $91:F25E |
| Salto en la pared | — | ya está (`Samus_Movement_14_WallJumping`, `Samus_InitWallJump`); con el gancho falta `Samus_GrappleWallJumpCheck` $90:9CAC |
| Daño y retroceso | `Samus_MoveHandler_Knockback` (0), `Samus_HandleTransitionsB` (5, 9, 10), `SamusFunc_F468` (25 ya; 26) | `Samus_MoveHandler_Knockback_0` $90:DF50 (es `InvalidInterrupt_Crash`: se deja), `Samus_HandleTransitionsB_1_0/4/6/7/10/11` $91:EDA2-EE48 (comprobar cuáles faltan en `Samus_HandleTransitionsB_1`), `Samus_HandleTransitionsB_5` $91:EEA6 (rayos X), `_9`/`_9B` $91:EF4F/EF53 (gancho), `_10` $91:EFBC (Draygon), `Samus_HitInterrupt_*` $90:DEBA-DF15 |
| Trajes Varia y Gravedad | `HandleMiscSamusPalette` (paletas especiales), `CallSomeSamusCode` (0x15) | `VariaSuitPickup` $91:D4E4, `GravitySuitPickup` $91:D5BA, `InitializeSuitPickupHdma` $91:D692, `SamusCode_15_CalledBySuitAcquision` $90:F310, `SamusCode_11_15_Common` $90:F2FC; física en líquidos: revisar `Samus_GetYSpeedTable`/`samus_x_speed_table` (ya en `samus_move.c`) con FX de agua/lava/ácido; salpicaduras `AtmosphericTypeFunc_*` $90:8AC5-8B74 |
| Destello de cristal | `Hdmaobj_CrystalFlash`, `HandleMiscSamusPalette` | `SamusMoveHandler_CrystalFlashStart` $90:D678, `…Main_0/1/2` $90:D6E3…, `kSamusMoveHandler_CrystalFlashFinish` $90:D75B, `Samus_HandleCrystalFlashPals` $91:DB93 |
| Posición del spritemap | `Samus_CalcSpritemapPos_` (27) | `Samus_CalcSpritemapPos_Special` $90:8D98 |

### Armas

| Mecánica | `nt_unimpl` hoy | Falta del decomp |
|---|---|---|
| Onda, hielo, spazer, plasma y combinaciones | `FireUnchargedBeam`/`FireChargedBeam` (`WaveBeam_CheckColl`), `LoadProjectilePalette` (≥ 12), `CallProjPreInstr` | `WaveBeam_CheckColl_0459/1368/2/7` $90:BDD4-BDF2, `ProjPreInstr_WavePlasmaEtc` $90:B0C3 (`_0459/_1368/_27`), `ProjPreInstr_BeamOrIceWave` $90:B0E4, `WriteBeamPalette_A/Y` $90:ACC2/ACCD, `InitializeProjectileSpeedOfType` $90:B1DD, `Projectile_SinLookup(_Inner)` $90:CC39/CC8A; tiles y paletas de cada rayo (`kBeamTilePtrs`, `kBeamPalettePtrs` → `mkmd`/`mkzobj`) y sus claves de dibujo (`zebes_proj_keys`) |
| Ataques especiales | `FireSba` (1, 2, 4, 8) | `FireSba_FireWave/Ice/Spazer/Plasma` $90:CD1A/CD9B/CE14/CE98, `ProjPreInstr_IceSba(2)` $90:CF09/CF7A, `ProjPreInstr_PlasmaSba` $90:D793 (+`Func_0/1/2`), `ProjPreInstr_WaveSba` $90:DA08, `ProjPreInstr_SpazerSba*` $90:DB57…, `FireEndOfSpazerSba` $90:DC67, `ProjPreInstr_EndOfSpazerSba` $90:DC9C, `InitializeSbaProjectile` $93:81A4 |
| Bomba de poder | `HudSelectionHandler_MorphBall` (bomba de poder), `HudSelectionHandler_Normal` | `ProjPreInstr_PowerBomb` $90:B0AE, `PowerBomb_Func3` $90:C157, `SwitchToHudHandler_PowerBombs/SuperMissiles` $90:C577/C564, la explosión (`$88`: `Hdmaobj_PowerBomb*`, bloques `BlockColl_PowerBomb`/`PowerBombExplosion*` en $94, daño `EnemyPowerBombColl` en $A0) |
| Hiperrayo | `FireUnchargedBeam` (FireHyperBeam), `ResetProjectileData`, `HandleBeamChargePalettes` | `FireHyperBeam` $90:BCD1, `ProjPreInstr_HyperBeam` $90:B159 (solo en la pelea de Mother Brain: al final) |
| Gancho | `SamusCode` 0x03, `MainScrollingRoutine`, `HandleMiscSamusPalette` (gancho), `HudSelectionHandler_Normal`, `Samus_MovementHandler_Normal` (22), `GrappleBeam_Func2` (port_gen) | banco $9B entero (`GrappleBeamHandler` $9B:C490 y sus `GrappleBeamFunc_*`, `GrappleNext_*`, `HandleConnectingGrapple*`, `UpdateGrappleBeamTiles*`, `HandleGrappleBeamFlare`, `PropelSamusFromGrappleSwing` $9B:CA65, `CancelGrappleBeamIfIncompatiblePose`, `CheckIfGrappleIsConnectedToBlock`, `ProcessEnemyGrappleBeamColl`), `Samus_Movement_16_Grappling` $90:A780, `Samus_Input_16_Grappling` $91:8181, `HudSelectionHandler_Grappling` $90:DD6F, `Samus_MoveHandler_ReleaseFromGrapple` $90:946E, `Samus_DetermineGrappleSwingSpeed_X` $90:9C21, `Grapple_Func1` $90:D525, `Samus_PostGrappleCollisionDetect` $90:EF22, `Samus_HandleTransitionsB_9/9B`; bloques de gancho ($94) |
| Rayos X | `Samus_Func20`, `Samus_HandleTransitionsB` (5), `HudSelectionHandler_Normal` | `Samus_Func20_` $90:E918, `SamusMovementType_Xray` $90:E94F, `HudSelectionHandler_Xray` $90:DDC8, `SwitchToHudHandler_Xray` $90:C5AE, `Samus_HandleXrayPals` $91:DCB4, `Xray_Initialize` $91:E16D, `XrayRunHandler` $91:CAD6 y `Xray_Setup*`/`Xray_Func*` $91:CAF9-D0D3, `HdmaobjPreInstr_XraySetup`, `ResponsibleForXrayStandupGlitch` $91:E2AD; en la Genesis el haz por pasos de docs/52 |

### No son mecánicas (quedan como están)

`InvalidInterrupt_Crash` (daño negativo, `Samus_MoveHandler_Knockback_0`, `Samus_HandlePeriodicDamage`): el SNES se
cuelga; `nt_unimpl` es lo correcto. `Unreachable` de los despachos. `Samus_ShootCheck (depuración)`,
`UpdateMinimap (área de depuración)`, `MarkMapTileAboveSamusAsExplored (fuera del mapa)`: no pasan en el juego. Las
lecturas fuera de los datos preparados (`b93`, `b9bt`) son guardas: avisan si una tabla nueva queda fuera.

## Orden de trabajo

Un commit por mecánica, cada una con su prueba en `tools/rooms/` (grabación `hlref` con `HL_WARP`, `HL_ITEMS`,
`HL_POKE`, `HL_SETXY` y `host/check` en 0 diferencias) y una mirada en la Genesis (retrorun). Ver el estado de cada
una al final de este documento.

## Estado (2026-10-28)

Todo se comprobó con `host/check` contra el SNES (modo libre: el estado del SNES solo al empezar) con las pruebas
de `tools/rooms/`. Las entradas del bot (`tools/snes_ref/ceres_bot/in_*.txt`) se agregan a `in_z2.txt` (hasta Zebes).

| Mecánica | Estado | Prueba (0 diferencias) |
|---|---|---|
| Inventario | hecho (este documento) | |
| Salto espacial | hecho | `spacejump.sh` (Landing Site, saltos encadenados con el tornillo) |
| Speed booster, ecos | hecho | `shinespark.sh` |
| Shinespark (vertical, horizontal, diagonal, choque y ecos) | hecho | `shinespark.sh` (sala 9CB3, 2686 cuadros) |
| Tornillo (paletas; el daño ya estaba) | hecho | `spacejump.sh`, `shinespark.sh` |
| Bola resorte | hecho | `shinespark.sh` |
| Salto en la pared | ya estaba; con el gancho: hecho | `grapple.sh` |
| Daño y retroceso | ya estaba (el inventario lo daba por faltante: está escrito sin nombres de función) | |
| Trajes Varia y gravedad (rayo de luz, paleta) | hecho | `suits.sh` (A6E2 y CE40) |
| Paletas especiales (speed booster, brillo, shinespark, destello de cristal, rayos X, hiperrayo) | hecho | |
| Rayos de onda, hielo, spazer, plasma y sus combinaciones | hecho | `beams.sh` (las 11 combinaciones con el cargado) |
| Ataques especiales (SBA) | hecho | `sba.sh` (onda, hielo, spazer, plasma) |
| Bomba de poder (soltarla, explosión contra bloques y enemigos) | hecho | `powerbomb.sh` |
| Gancho (engancharse, columpio, soltarse, enemigos) | hecho | `grapple.sh` |
| Rayos X | **falta** (siguiente bloque) | |
| Destello de cristal (movimiento) | falta (las paletas están) | |
| Hiperrayo | falta (Mother Brain) | |

Errores que se encontraron de paso:
- `plm.c`: la instrucción de dibujo que arma `PlmInstr_DrawPlmBlock` está en 7E:1E67 (`custom_draw_instr_num_blocks`),
  no en 7E:1E6D: el shinespark que rompe bloques lo mostró.
- `samus_codes.c`: `speed_boost_counter = bank91_word(0xB61F)` daba 0; ahora `kSpeedBoostToCtr[0]`.
- `grapple_beam_end_angle_hi` es el byte alto de `grapple_beam_end_angle16`: se usa `HI8()` (si no, `gen_vars` la
  declara aparte y no se actualiza).

### Lo que falta en la Genesis (dibujo)

La lógica es la del SNES (comprobada); en la Genesis falta dibujar:
- **los rayos nuevos** (onda, hielo, spazer, plasma, combinaciones y SBA): sus spritemaps no están en las claves de
  dibujo. Agregarlos a las claves de cada sala de Zebes (`mkmd.beam_sm_tiles`, cada una con los tiles de su
  combinación) hace que Zebes no entre en las páginas del SSF2. Propuesta: un juego de claves común (no por sala)
  que use un lugar fijo de la VRAM donde la Genesis sube los 8 tiles de la combinación equipada (como
  `UpdateBeamTilesAndPalette` en el SNES) y los colores 9-13 de la línea 3 (ya son del rayo).
- **el rayo de luz de los trajes**: la ventana (`hdma_table_1`) y la matemática de color; hoy se ve el cambio de traje
  sin el rayo.

### En la Genesis (retrorun, `tools/compare/lado_a_lado.sh`)

`lado_a_lado.sh` acepta ahora `XY_N` (cuadro de la posición), `HL_ITEMS`/`HL_POKE` (hlref) y `EXTRA_FL` (para la
Genesis: `-DMD_TEST_ITEMS=0x... -DMD_TEST_BEAMS=0x... -DMD_TEST_HUD_ITEM=n`, md_main.c, puestos al saltar de sala).

- **Shinespark** (sala 9CB3, `in_shinespark.txt`, `XY_N=200 X=200 Y=1707`, objetos 230E): la posición y la pose de
  Samus son las del SNES en cada cuadro del motor (RAM de la Genesis contra la grabación), y la imagen (carrera, brillo,
  carga, salto vertical con sus paletas y ecos) es la del SNES. Las capturas de `mont.py` quedan un cuadro del motor
  corridas (la Genesis muestra lo del cuadro anterior del SNES), igual en posición y paleta: es la alineación de las
  capturas, no el juego.
- **Gancho** (sala 9D19, `in_grapple.txt`, `XY_N=250 X=784 Y=740`, `MD_TEST_ITEMS=0x4004 MD_TEST_HUD_ITEM=4`): ya se
  dibuja. `GrappleTiles` convierte en cada cambio los tiles de $9A (`kGrappleSnesTiles`, mkdata: el punto y los 4
  segmentos según el ángulo; solo usan el color 15 de la paleta 5) y los sube en el vblank a 5 tiles fijos al final
  de 0xF000 (`GRAPPLE_TILE`, los del haz del ojo: en esa sala no se dibuja). El color: el lugar de la CRAM que sale de
  palette_buffer[223] o, si la sala no lo usa (9D19), el más parecido a ese color (celeste, como el SNES).

### `regress.sh`

intro 0 distintos. Ceres y Zebes: **10 cuadros "distintos"** (8629, 8655-8656, 8664-8665, 8669, 8684-8686, 8690,
8700), todos en la explosión de Ceres (la nave que se va y el círculo de la explosión), los mismos desde el commit
del gancho. La lógica no cambió (`host/check` con `ref_full.bin`, cuadros 98-12000: 0 diferencias). Comprobado con
la ROM de `main` compilada aparte (que da los 0 distintos de la referencia): con este código cada uno de esos cuadros
del motor se muestra 2 cuadros de consola más tarde que en `main` (el código y los datos crecen y se corren las
direcciones de la parte fija, como en docs/48 y docs/49). Comparando la misma imagen del motor en las dos ROMs
cambian entre 0 y 340 píxeles, todos en el borde derecho del círculo de la explosión: en `main` ese borde sale
desgarrado (la tabla del círculo se cambia mientras se dibuja la pantalla) y aquí sale limpio, o al revés según el
cuadro. No se aprobaron (`UPDATE=1`): lo decide Edwin.
