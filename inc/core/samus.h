/* MegaMetroid — motor nativo: Samus (docs/31).
 * Reescrito a mano desde el decomp (bancos $90, $91 y $94 de snesrev/sm). Los nombres de funciones y
 * variables son los del decomp, para poder seguir la biblia línea por línea. */
#ifndef SAMUS_H
#define SAMUS_H
#include "nt.h"
#include "enums.h"
#include "fn.h"
#include "tables.h"

typedef struct { bool flag; int32 amt; } Pair_Bool_Amt;
typedef struct { uint16 collision; int32 amt; } CheckEnemyColl_Result;
typedef struct { uint16 k, j; } PairU16;

/* motor (lo que el SNES no tiene: aviso de caminos sin portar) */
void nt_unimpl(const char *what);
extern uint32 nt_unimpl_val;               /* el valor que falta (dirección de la función, tipo...): para el aviso */
static inline void nt_unimpl_v(const char *what, uint32 v) { nt_unimpl_val = v; nt_unimpl(what); nt_unimpl_val = 0; }
void nt_clear_ram(uint16 from, uint16 to);   /* nt_ram.c: pone a cero las variables de 7E:from-7E:to-1 */

/* ---- samus_move.c (banco $90: movimiento) */
void Samus_Animate(void);
void Samus_Draw(void);
void Samus_DrawWhenNotAnimatingOrDying(void);
void Samus_HandleMovement_X(void);
void Samus_MoveX(int32 amt);
void Samus_MoveY_WithSpeedCalc(void);
void Samus_MoveLeft(int32 amt);
void Samus_MoveRight(int32 amt);
void Samus_MoveUp(int32 amt);
void Samus_MoveDown(int32 amt);
void Samus_InitJump(void);
void Samus_DetermineAccel_Y(void);
void Samus_MovementHandler_Normal(void);
void RunSamusMovementHandler(void);
void Samus_CancelSpeedBoost(void);
void MainScrollingRoutine(void);
uint16 Samus_GetBottom_R18(void);
uint16 Samus_GetTop_R20(void);
void Samus_SetRadius(void);
void Samus_UpdateSuitPaletteIndex(void);
void Samus_HandleCooldown(void);
void Samus_Func10(void);
void Samus_JumpCheck(void);
void Samus_HandlePeriodicDamage(void);
void Samus_PauseCheck(void);
void Samus_LowHealthCheck_(void);
void Samus_HitInterruption(void);
void UpdateMinimap(void);
void HandleControllerInputForGamePhysics(void);
void HandleSamusMovementAndPause(void);
void DrawSamusAndProjectiles(void);
uint16 CallSomeSamusCode(uint16 a);
void MakeSamusFaceForward(void);               /* $91:E3F6 */
void Samus_RestoreHealth(uint16 a);
void Samus_RestoreMissiles(uint16 a);
void Samus_RestoreSuperMissiles(uint16 a);
void Samus_RestorePowerBombs(uint16 a);
PairU16 Samus_CalcSpritemapPos(uint16 k);
void Samus_ClearXSpeedIfColl(void);
void Samus_MoveUp_SetPoseCalcInput(void);
void Samus_MoveDown_SetPoseCalcInput(void);
int32 Samus_CalcDisplacementMoveLeft(int32 amt);
int32 Samus_CalcDisplacementMoveRight(int32 amt);
void Samus_HandleExtraRunspeedX(void);
uint16 Samus_DetermineSpeedTableEntryPtr_X(void);

/* ---- samus_pose.c (banco $91: entrada, poses, transiciones, paletas) */
void Samus_InputHandler(void);
void Samus_LookupTransitionTable(void);
void Samus_HandleTransitions(void);
void Samus_HandleTransFromBlockColl(void);
void Samus_HandlePalette(void);
void SamusFunc_F433(void);
void Samus_SetAnimationFrameIfPoseChanged(void);
void HandleJumpTransition(void);
void Samus_LoadSuitPalette(void);
void WriteBeamPalette_A(uint16 a);
void Samus_Copy10PalColors(uint16 v0);
void Samus_Copy6PalColors(uint16 j);
void Samus_Initialize(void);                   /* $91:E00D */
void Samus_LoadSuitTargetPalette(void);
void CopyToSamusSuitPalette(uint16 k);
void Samus_Func20(void);
void Samus_AlignBottomWithPrevPose(void);

/* ---- coll.c (banco $94: colisión con bloques) */
void Samus_AlignYPosSlope(void);
int32 WallJumpBlockCollDetect(int32 amt);
int32 Samus_CollDetectChangedPose(int32 amt);
int32 Samus_MoveRight_NoSolidColl(int32 amt);
int32 Samus_MoveDown_NoSolidColl(int32 amt);
void BlockInsideDetection(void);
void CalculateBlockAt(uint16 r26, uint16 r28, uint16 r30, uint16 r32);
uint8 BlockCollNoWaveBeamHoriz(uint16 k);
uint8 BlockCollNoWaveBeamVert(uint16 k);
uint8 BlockCollWaveBeamHoriz(uint16 k);
uint8 BlockCollWaveBeamVert(uint16 k);
uint8 BlockCollMissileHoriz(uint16 k);
uint8 BlockCollMissileVert(uint16 k);

/* ---- enemies.c */
CheckEnemyColl_Result Samus_CheckSolidEnemyColl(int32 amt);

/* ---- proj.c (proyectiles) */
void HandleProjectile(void);
void ResetProjectileData(void);
void HudSelectionHandler_Normal(void);
void HudSelectionHandler_JumpEtc(void);
void HudSelectionHandler_MorphBall(void);
void HudSelectionHandler_TurningAround(void);
void HudSelectionHandler_CrouchEtcTrans(void);
void HandleSwitchingHudSelection(void);
void HandleProjectileTrails(void);
void KillProjectile(uint16 k);
void ClearProjectile(uint16 k);
void DrawProjectiles(void);
void HandleArmCannonOpenState(void);
void Samus_ArmCannon_Draw(void);
void HandleChargingBeamGfxAudio(void);

/* ---- sonido (cola; la Genesis la traduce a su driver) */
void QueueSfx1_Max3(uint16 v);
void QueueSfx2_Max3(uint16 v);
void QueueSfx3_Max3(uint16 v);
void QueueSfx1_Max6(uint16 v);
void QueueSfx1_Max9(uint16 v);
void QueueSfx1_Max15(uint16 v);
void QueueSfx2_Max6(uint16 v);
void QueueSfx2_Max15(uint16 v);
void QueueSfx2_Max9(uint16 v);
void QueueSfx1_Max1(uint16 v);
void QueueSfx2_Max1(uint16 v);
void QueueSfx3_Max1(uint16 v);
void QueueSfx3_Max6(uint16 v);
void QueueSfx3_Max9(uint16 v);
void QueueSfx3_Max15(uint16 v);

/* ---- otros */
uint8 CheckEventHappened(uint16 a);
void DrawPlayerExplosions2(void);
void ClearFlareAnimationState(void);
void Samus_HandleHudSpecificBehaviorAndProjs(void);
void HandleSwitchingHudSelection(void);
void Samus_InitWallJump(void);

/* ---- dibujo (md/ en la Genesis; en el PC no hace nada) */
void DrawSamusFrontExtra(void);              /* el tile extra de Samus de frente ($90:868D) */
void samus_tiles_changed(void);              /* nmi_copy_samus_* cambió: subir sus tiles en el vblank */
void hud_minimap_update(uint16 r34, uint16 r22, uint16 x);
void DrawSamusSpritemap(uint16 idx, uint16 x, uint16 y);
void SetSamusTilesDefsForCurAnim(void);
void HandleAtmosphericEffects(void);
void Samus_DrawEchoes(void);
void GrappleBeamHandler(void);                   /* grapple.c ($9B:C490) */
void SamusDrawHandler_Grapple(void);
void Samus_DrawEcho(uint16 j);
void Samus_DrawShinesparkCrashEchoProjectiles(void);
void Projectile_Func7_Shinespark(void);
Point16U Projectile_SinLookup(uint16 j, uint16 a);
void InitializeShinesparkEchoOrSpazerSba(uint16 k);

#endif
