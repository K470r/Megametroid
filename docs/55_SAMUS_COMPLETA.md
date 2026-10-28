# 55 — Samus completa: inventario de lo que falta portar

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

## Estado

| Mecánica | Estado |
|---|---|
| Inventario | hecho (este documento) |
