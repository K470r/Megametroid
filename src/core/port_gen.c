/* Generado por tools/port/port.py (docs/42): funciones del decomp (snesrev sm/src) traducidas al motor
   nativo con tools/port/xlate.py. No editar: se cambia el traductor o se escribe la función a mano en otro
   archivo (entonces deja de generarse). */
#include "port_gen.h"

typedef struct EprojCollInfo {
  uint16 eci_r20;
  uint16 eci_r24;
  uint16 eci_r26;
  uint16 eci_r28;
  uint16 eci_r30;
  uint16 eci_r32;
  uint16 eci_r34;
} EprojCollInfo;
typedef uint8 Func_EprojCollInfo_U8(EprojCollInfo *eci);
typedef struct EnemyBlockCollInfo {
  int32 ebci_r18_r20;
  uint16 ebci_r24;
  uint16 ebci_r26;
  uint16 ebci_r28;
  uint16 ebci_r30;
  uint16 ebci_r32;
} EnemyBlockCollInfo;
typedef uint8 Func_EnemyBlockCollInfo_U8(EnemyBlockCollInfo *ebci);
typedef struct PositionAndWidth {
  uint16 x_pos;
  uint16 x_subpos;
  uint16 y_pos;
  uint16 y_subpos;
  uint16 width;
  uint16 height;
} PositionAndWidth;
typedef uint8 CalculateAngleFromXYFunc(uint16 j);
typedef struct BabyMetroidExecState {
  uint16 ip;
  uint16 timer;
} BabyMetroidExecState;

uint8 sub_82DAF7(uint16 a);
uint16 SuitDamageDivision(uint16 a);
void SpawnPalfxObject(uint16 j);
uint8 SpawnPLM(uint16 a);
void SpawnMorphBallEyeBeamHdma(void);
void SpawnHardcodedPlm(uint8 x, uint8 y, uint16 id);
void SpawnEprojWithRoomGfx(uint16 j, uint16 a);
void SpawnEprojWithGfx(uint16 a, uint16 k, uint16 j);
void SpawnBombTorizoHaze(void);
void SetBossBitForCurArea(uint16 a);
void Samus_ScrollFinishedHook_SporeSpawnFight(void);
void Samus_RestoreSuperMissiles(uint16 a);
void Samus_RestorePowerBombs(uint16 a);
void Samus_RestoreMissiles(uint16 a);
void Samus_RestoreHealth(uint16 a);
void Samus_DealDamage(uint16 a);
void ResetProjectileData(void);
void QueueSfx3_Max9(uint16 v);
void QueueSfx3_Max6(uint16 v);
void QueueSfx3_Max3(uint16 v);
void QueueSfx3_Max15(uint16 v);
void QueueSfx3_Max1(uint16 v);
void QueueSfx2_Max9(uint16 v);
void QueueSfx2_Max6(uint16 v);
void QueueSfx2_Max3(uint16 v);
void QueueSfx2_Max15(uint16 v);
void QueueSfx2_Max1(uint16 v);
void QueueSfx1_Max9(uint16 v);
void QueueSfx1_Max6(uint16 v);
void QueueSfx1_Max3(uint16 v);
void QueueSfx1_Max15(uint16 v);
void QueueSfx1_Max1(uint16 v);
void QueueMusic_Delayed8(uint16 a);
void ProjectileReflection(uint16 r20);
void NormalEnemyTouchAiSkipDeathAnim(void);
uint16 NextRandom(void);
uint32 Multiply16x16(uint16 a, uint16 j);
uint16 MoveEprojWithVelocity(uint16 k);
uint16 Math_MultBySin(uint16 a, uint16 ang);
uint16 Math_MultByCos(uint16 a, uint16 ang);
uint8 EprojBlockCollisition_Vertical(uint16 k);
uint8 EprojBlockCollisition_Horiz(uint16 k);
uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal);
uint16 CheckIfEprojIsOffScreen(uint16 k);
uint8 CheckEventHappened(uint16 a);
uint8 CheckBossBitForCurArea(uint16 a);
uint16 CallSomeSamusCode(uint16 a);
void CallEnemyAi(uint8 bank, uint16 f);
void CalculatePlmBlockCoords(uint16 k);
uint16 CalculateAngleFromXY(uint16 x, uint16 y);
uint16 EprojInstr_Delete(uint16 k, uint16 epjp);
uint16 EprojInstr_Sleep(uint16 k, uint16 epjp);
uint16 EprojInstr_SetPreInstr_(uint16 k, uint16 epjp);
uint16 EprojInstr_ClearPreInstr(uint16 k, uint16 epjp);
void CallEprojFunc(uint32 ea, uint32 k);
uint16 EprojInstr_CallFunc(uint16 k, uint16 epjp);
uint16 EprojInstr_Goto(uint16 k, uint16 epjp);
uint16 EprojInstr_GotoRel(uint16 k, uint16 epjp);
uint16 EprojInstr_DecTimerAndGotoIfNonZero(uint16 k, uint16 epjp);
uint16 EprojInstr_DecTimerAndGotoRelIfNonZero(uint16 k, uint16 epjp);
uint16 EprojInstr_SetTimer(uint16 k, uint16 epjp);
uint16 EprojInstr_MoveRandomlyWithinRadius(uint16 k, uint16 epjp);
uint16 EprojInstr_SetProjectileProperties(uint16 k, uint16 epjp);
uint16 EprojInstr_ClearProjectileProperties(uint16 k, uint16 epjp);
uint16 EprojInstr_EnableCollisionWithSamusProj(uint16 k, uint16 epjp);
uint16 EprojInstr_DisableCollisionWithSamusProj(uint16 k, uint16 epjp);
uint16 EprojInstr_DisableCollisionWithSamus(uint16 k, uint16 epjp);
uint16 EprojInstr_EnableCollisionWithSamus(uint16 k, uint16 epjp);
uint16 EprojInstr_SetToNotDieOnContact(uint16 k, uint16 epjp);
uint16 EprojInstr_SetToDieOnContact(uint16 k, uint16 epjp);
uint16 EprojInstr_SetLowPriority(uint16 k, uint16 epjp);
uint16 EprojInstr_SetHighPriority(uint16 k, uint16 epjp);
uint16 EprojInstr_SetXyRadius(uint16 k, uint16 epjp);
uint16 EprojInstr_SetXyRadiusZero(uint16 k, uint16 epjp);
uint16 EprojInstr_CalculateDirectionTowardsSamus(uint16 k, uint16 epjp);
uint16 EprojInstr_WriteColorsToPalette(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueMusic(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx1_Max6(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_Max6(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx3_Max6(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx1_Max15(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_Max15(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx3_Max15(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx1_Max3(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_Max3(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx3_Max3(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx1_Max9(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_Max9(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx3_Max9(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx1_Max1(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_Max1(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx3_Max1(uint16 k, uint16 epjp);
static void Eproj_Init_0x8bc2_SkreeDownRight(uint16 j);
static void Eproj_Init_0x8bd0_SkreeUpRight(uint16 j);
static void Eproj_Init_0x8bde_SkreeDownLeft(uint16 j);
static void Eproj_Init_0x8bec_SkreeUpLeft(uint16 j);
void Eproj_PreInstr_SkreeParticle(uint16 k);
static void EprojInit_DraygonsGunk(uint16 j);
void EprojPowerBombCollision(uint16 k);
void EprojPreInstr_DraygonsGunk_8E0F(uint16 k);
static void EprojInit_MiniKraidSpit(uint16 j);
void EprojPreInit_MiniKraidSpit(uint16 k);
static void EprojInit_MiniKraidSpikes(uint16 j, uint16 a);
static void EprojInit_MiniKraidSpikesLeft(uint16 j);
static void EprojInit_MiniKraidSpikesRight(uint16 j);
void EprojPreInstr_MiniKraidSpikes(uint16 k);
static void EprojInit_WalkingLavaSeahorseFireball(uint16 j);
void EprojPreInstr_WalkingLavaSeahorseFireball(uint16 k);
static void EprojInit_PirateMotherBrainLaser(uint16 j);
uint16 EprojInstr_SetPreInstrAndRun(uint16 k, uint16 epjp);
void EprojPreInstr_PirateMotherBrainLaser_MoveLeft(uint16 k);
void EprojPreInstr_PirateMotherBrainLaser_MoveRight(uint16 k);
static void EprojInit_PirateClaw(uint16 j);
uint16 EprojInstr_A3BE(uint16 k, uint16 epjp);
uint16 EprojInstr_GotoWithProbability25(uint16 k, uint16 epjp);
static void EprojInit_BombTorizoLowHealthDrool(uint16 j);
static void EprojInit_BombTorizoLowHealthInitialDrool(uint16 j);
static void EprojInit_BombTorizoExplosiveSwipe(uint16 j);
static void EprojInit_BombTorizoStatueBreaking(uint16 j);
static void EprojInit_BombTorizoLowHealthExplode(uint16 j);
static void EprojInit_BombTorizoDeathExplosion(uint16 j);
void sub_86A887(uint16 v0);
void EprojPreInstr_A977(uint16 k);
uint16 EprojInstr_SpawnEnemyDrops(uint16 k, uint16 epjp);
static void Eproj_InitXYVelRandom(uint16 j, uint16 k, Point16U pt);
static void EprojInit_BombTorizosChozoOrbs(uint16 j);
static void EprojInit_GoldenTorizosChozoOrbs(uint16 j);
void EprojPreInstr_BombTorizosChozoOrbs(uint16 k);
void EprojPreInstr_GoldenTorizosChozoOrbs(uint16 k);
static void EprojInit_TorizoSonicBoom(uint16 j);
void EprojPreInstr_TorizoSonicBoom(uint16 k);
static void EprojInit_TourianStatueDustClouds(uint16 j);
static void EprojInit_TourianLandingDustCloudsRightFoot(uint16 j);
uint16 EprojInstr_MoveY_Minus4(uint16 k, uint16 epjp);
static void EprojInit_TorizoLandingDustCloudLeftFoot(uint16 j);
static void EprojInit_GoldenTorizoEgg(uint16 j);
void EprojPreInstr_GoldenTorizoEgg(uint16 k);
static void EprojInit_GoldenTorizoSuperMissile(uint16 j);
void EprojPreInstr_GoldenTorizoSuperMissile(uint16 k);
static void sub_86B279(uint16 k, uint16 a);
uint16 EprojInstr_SetVelTowardsSamus1(uint16 k, uint16 epjp);
static void EprojInit_GoldenTorizoEyeBeam(uint16 j);
void EprojPreInstr_GoldenTorizoEyeBeam(uint16 k);
static void EprojInit_TourianStatueUnlockingParticleWaterSplash(uint16 j);
static void EprojInit_TourianStatueEyeGlow(uint16 j);
static void EprojInit_TourianStatueUnlockingParticle(uint16 j);
static void EprojInit_TourianStatueSoul(uint16 j);
static void EprojInit_TourianStatueBaseDecoration(uint16 j);
static void EprojInit_TourianStatueRidley(uint16 j);
static void EprojInit_TourianStatuePhantoon(uint16 j);
static void EprojPreInstr_BA42(uint16 k);
void EprojPreInstr_TourianStatueStuff(uint16 k);
static void EprojInit_MotherBrainDeathBeemFired(uint16 j);
void SpawnMotherBrainDeathBeam(uint16 x);
static void EprojInit_KiHunterAcidSpitCommon(uint16 k, uint16 j);
static void EprojInit_KiHunterAcidSpitLeft(uint16 j);
static void EprojInit_KiHunterAcidSpitRight(uint16 j);
void sub_86CFD5(uint16 k);
void sub_86CFE6(uint16 k);
void sub_86CFF8(uint16 k);
static void EprojInit_KagosBugs(uint16 j);
void EprojPreInstr_KagosBugs_Func1(uint16 k);
void EprojPreInstr_KagosBugs_Func2(uint16 k);
void EprojPreInstr_KagosBugs(uint16 k);
void EprojPreInstr_D0EC(uint16 k);
void EprojPreInstr_D128(uint16 v0);
uint16 EprojInstr_D15C(uint16 k, uint16 epjp);
uint16 EprojInstr_D1B6(uint16 k, uint16 epjp);
void sub_86D992(uint16 v0);
void EprojPreInstr_SpikeShootingPlantSpikes_MoveY1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_MoveY2(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_MoveX1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_MoveX2(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_0_MoveX1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_2_MoveX2(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_1_MoveY1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_4_MoveY2(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_6_MoveX1Y1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_8_MoveX1Y2(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_7_MoveX2Y1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes_9_MoveX2Y2(uint16 k);
void EprojPreInstrHelper_SpikeShootingPlantSpikes_Func1(uint16 k);
void EprojPreInstr_SpikeShootingPlantSpikes(uint16 k);
uint16 EprojPreInstrHelper_SpikeShootingPlantSpikes_Func2(uint16 k);
uint16 EprojInstr_DC5A(uint16 k, uint16 epjp);
uint16 EprojInstr_SpawnEnemyDrops_0(uint16 k, uint16 epjp);
uint16 EprojInstr_SpawnSporesEproj(uint16 k, uint16 epjp);
static void EprojInit_Spores(uint16 j);
static void EprojInit_SporeSpawnStalk(uint16 j);
static void EprojInit_SporeSpawners(uint16 j);
void EprojPreInstr_Spores(uint16 k);
void EprojPreInstr_SporeSpawners(uint16 k);
static void EprojInit_DustCloudOrExplosion(uint16 v0);
void EprojPreInstr_DustCloudOrExplosion(uint16 k);
uint16 Eproj_FuncE722(uint16 k);
void Eproj_FuncE73E_MoveXY(uint16 k);
static void EprojInit_YappingMawsBody(uint16 j);
uint16 EprojInstr_ECE3(uint16 k, uint16 epjp);
uint16 EprojInstr_ED17(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_9(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_24(uint16 k, uint16 epjp);
uint16 EprojInstr_QueueSfx2_B(uint16 k, uint16 epjp);
uint16 EprojInstr_EEAF(uint16 k, uint16 epjp);
uint16 EprojInstr_HandleRespawningEnemy(uint16 k, uint16 epjp);
static void EprojInit_F337(uint16 j);
static void EprojInit_EnemyDeathExplosion(uint16 j);
void EprojPreInstr_Pickup(uint16 k);
void Eproj_Pickup_SmallHealth(void);
void Eproj_Pickup_BigHealth(void);
void Eproj_Pickup_PowerBombs(void);
void Eproj_Pickup_Missiles(void);
void Eproj_Pickup_SuperMissiles(void);
uint16 RandomDropRoutine(uint16 k);
void RespawnEnemy(uint16 v0);
void CreateADudShot(void);
uint16 EnemyInstr_SetAiPreInstr(uint16 k, uint16 jp);
uint16 EnemyInstr_ClearAiPreInstr(uint16 k, uint16 jp);
uint16 EnemyInstr_StopScript(uint16 k, uint16 jp);
uint16 EnemyInstr_Goto(uint16 k, uint16 jp);
uint16 EnemyInstr_DecTimerAndGoto(uint16 k, uint16 jp);
uint16 EnemyInstr_SetTimer(uint16 k, uint16 jp);
uint16 EnemyInstr_Skip2bytes(uint16 k, uint16 jp);
uint16 EnemyInstr_EnableOffScreenProcessing(uint16 k, uint16 jp);
uint16 EnemyInstr_DisableOffScreenProcessing(uint16 k, uint16 jp);
void SpawnEnemyDrops(uint16 a, uint16 k, uint16 varE20);
void NormalEnemyFrozenAI(void);
void EnemyDeathAnimation(uint16 k, uint16 a);
void NormalEnemyTouchAi(void);
void NormalEnemyTouchAiSkipDeathAnim_CurEnemy(void);
void NormalEnemyPowerBombAi(void);
void NormalEnemyPowerBombAiSkipDeathAnim(void);
void NormalEnemyShotAi(void);
void NormalEnemyShotAiSkipDeathAnim_CurEnemy(void);
void EnemyFunc_A6B4_UsedBySporeSpawn(void);
uint16 NormalEnemyShotAiSkipDeathAnim(void);
void CreateDudShot(void);
uint16 CheckIfEnemyTouchesSamus(uint16 k);
uint16 EnemyFunc_AC67(uint16 k);
PairU16 EnemyFunc_ACA8(Point16U base_pt, Point16U samus_pt);
uint16 CheckIfEnemyIsOnScreen(void);
uint16 DetermineDirectionOfSamusFromEnemy(void);
uint16 GetSamusEnemyDelta_Y(uint16 k);
uint16 GetSamusEnemyDelta_X(uint16 k);
uint16 IsSamusWithinEnemy_Y(uint16 k, uint16 a);
uint16 IsSamusWithinEnemy_X(uint16 k, uint16 a);
uint16 SignExtend8(uint16 a);
uint16 Abs16(uint16 a);
uint16 SubtractThenAbs16(uint16 k, uint16 j);
static uint32 SineMult8bitInner(uint16 varE32, uint16 varE34);
uint16 CosineMult8bit(uint16 a, uint16 varE32);
uint16 SineMult8bit(uint16 a, uint16 varE32);
uint32 CosineMult8bitFull(uint16 a, uint16 varE32);
uint32 SineMult8bitFull(uint16 a, uint16 varE32);
Point32 ConvertAngleToXy(uint16 r18, uint16 r20);
void EnemyFunc_B691(uint16 varE20, Point32 pt);
void EnemyFunc_B7A1(void);
void Enemy_ItemDrop_MiniKraid(uint16 k);
void Enemy_ItemDrop_LowerNorfairSpacePirate(uint16 k);
void Enemy_ItemDrop_BombTorizo(uint16 k);
void Enemy_ItemDrop_GoldenTorizo(uint16 k);
void Enemy_ItemDrop_SporeSpawn(uint16 k);
uint8 CompareDistToSamus_X(uint16 k, uint16 a);
uint8 EnemyFunc_BBBF(uint16 k, int32 amt);
uint8 EnemyFunc_BF8A(uint16 k, uint16 a, int32 amt);
uint16 CalculateAngleOfSamusFromEproj(uint16 k);
uint16 CalculateAngleOfSamusFromEnemy(uint16 k);
uint8 IsEnemyLeavingScreen(uint16 k);
static uint8 ClearCarry_13(EnemyBlockCollInfo *ebci);
static uint8 SetCarry_4(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollReact_Spike(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollHorizReact_Slope(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollVertReact_Slope(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollHorizReact_Slope_Square(EnemyBlockCollInfo *ebci, uint16 k, uint16 a);
static uint8 EnemyBlockCollVertReact_Slope_Square(EnemyBlockCollInfo *ebci, uint16 a, uint16 k);
static uint8 EnemyBlockCollHorizReact_Slope_NonSquare(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollVertReact_Slope_NonSquare(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollReact_HorizExt(EnemyBlockCollInfo *ebci);
static uint8 EnemyBlockCollReact_VertExt(EnemyBlockCollInfo *ebci);
uint8 Enemy_MoveRight_ProcessSlopes(uint16 k, int32 amt);
uint8 Enemy_MoveRight_IgnoreSlopes(uint16 k, int32 amt);
static uint8 Enemy_MoveRight_IgnoreSlopes_Inner(uint16 k, int32 amount32, uint16 r32);
uint8 Enemy_MoveDown(uint16 k, int32 amount32);
static uint8 EnemyBlockCollReact_Horiz(EnemyBlockCollInfo *ebci, uint16 k);
static uint8 EnemyBlockCollReact_Vert(EnemyBlockCollInfo *ebci, uint16 k);
void CalculateBlockContainingPixelPos(uint16 xpos, uint16 ypos);
uint8 EnemyFunc_C8AD(uint16 k);
void Enemy_NormalFrozenAI_A2(void);
void Enemy_NormalTouchAI_A2(void);
void Enemy_NormalShotAI_A2(void);
void BouncingGoofball_Init(void);
void BouncingGoofball_Func1(uint16 k);
void BouncingGoofball_Main(void);
void BouncingGoofball_State0(void);
void BouncingGoofball_State1(void);
uint16 BouncingGoofball_SamusCloseX(void);
void BouncingGoofball_Func2(void);
void BouncingGoofball_Func3(void);
uint16 BouncingGoofball_Instr_88C5(uint16 k, uint16 jp);
uint16 BouncingGoofball_Instr_88C6(uint16 k, uint16 jp);
void ThinHoppingBlobs_Init(void);
void sub_A29A6C(uint16 a);
void CallThinHoppingBlobs(uint32 ea);
void ThinHoppingBlobs_Main(void);
void ThinHoppingBlobs_Func1(uint16 k);
void TwinHoppingBlobs_Func2(void);
void ThinHoppingBlobs_Func3(void);
uint16 ThinHoppingBlobs_Func4(void);
void ThinHoppingBlobs_Func5(void);
void ThinHoppingBlobs_Func6(uint16 k);
void CallThinHoppingBlobsVarE(uint32 ea);
void ThinHoppingBlobs_Func7(void);
void ThinHoppingBlobs_Func8(void);
void ThinHoppingBlobs_Func9(void);
void ThinHoppingBlobs_Func10(void);
void ThinHoppingBlobs_Func11(void);
void ThinHoppingBlobs_Func12(void);
void ThinHoppingBlobs_Func13(void);
void ThinHoppingBlobs_Func14(void);
void ThinHoppingBlobs_Func15(void);
void ThinHoppingBlobs_Func16(void);
uint16 SpikeShootingPlant_Instr_9F2A(uint16 k, uint16 jp);
void SpikeShootingPlant_Init(void);
void SpikeShootingPlant_Main(void);
void SpikeShootingPlant_2(uint16 k);
void SpikeShootingPlant_3(uint16 k);
void SpikeShootingPlant_4(void);
void SpikeShootingPlant_5(void);
void SpikeShootingPlant_6(void);
void SpikeShootingPlant_7(void);
void SpikeShootingPlant_8(void);
uint16 SpikeShootingPlant_Instr_A095(uint16 k, uint16 jp);
uint16 SpikeShootingPlant_Instr_A0A7(uint16 k, uint16 jp);
void Flies_Init(void);
bool CarryAdd16(uint16 *rv, uint16 v);
void Flies_1(uint16 k);
uint16 Flies_2(uint16 k);
void Flies_Main(void);
void Flies_3(uint16 k);
void Flies_4(uint16 k);
void Flies_5(uint16 k);
void Flies_6(uint16 k);
void Flies_7(uint16 k);
uint16 EnemyInstr_Rio_Instr_1(uint16 k, uint16 jp);
void Rio_Init(void);
void CallRioFunc(uint32 ea);
void Rio_Main(void);
void Rio_1(uint16 k);
void Rio_2(uint16 k);
void Rio_3(uint16 k);
void Rio_4(uint16 k);
void Rio_5(uint16 k);
void Rio_6(uint16 a);
static uint16 HirisingSlowfalling_Func_1(uint16 k);
void HirisingSlowfalling_Init(void);
void HirisingSlowfalling_Func_2(uint16 k, uint16 r18);
void HirisingSlowfalling_Func_3(uint16 a);
void CallHirisingSlowfalling(uint32 ea);
void HirisingSlowfalling_Main(void);
void HirisingSlowfalling_Func_4(void);
void HirisingSlowfalling_Func_5(void);
void HirisingSlowfalling_Func_6(void);
void HirisingSlowfalling_Func_7(void);
void Gripper_Init(void);
void Gripper_Main(void);
uint8 Gripper_Func_1(uint16 k);
uint8 Gripper_Func_2(uint16 k);
void Gripper_Func_3(void);
void Gripper_Func_4(void);
void JetPowerRipper_Init(void);
void JetPowerRipper_Main(void);
void Ripper_Func_1(void);
void JetPowerRipper_Shot(void);
void Ripper_Init(void);
void Ripper_Main(void);
void TimedShutter_Init(void);
void CallTimedShutterFunc(uint32 ea, uint16 k);
void TimedShutter_Main(void);
void TimedShutter_Func_1(uint16 k);
void TimedShutter_Func_2(uint16 k);
void TimedShutter_Func_3(uint16 k);
void TimedShutter_Func_4(uint16 k);
void TimedShutter_Func_Null(void);
void TimedShutter_Func_5(uint16 k);
void TimedShutter_Func_6(void);
void TimedShutter_Func_7(void);
void TimedShutter_Func_8(void);
void TimedShutter_Func_9(void);
void TimedShutter_Func_Null2(void);
void TimedShutter_Func_10(uint16 k);
void TimedShutter_Func_11(void);
void TimedShutter_Func_12(void);
void TimedShutter_Func_13(void);
void TimedShutter_Func_14(void);
void TimedShutter_Func_15(void);
void RisingFallingPlatform_Init(void);
void ShootableShutter_Init(void);
void RisingFallingPlatform_Func_1(uint16 k);
void CallRisingFallingPlatformFunc(uint32 ea);
void RisingFallingPlatform_Main(void);
void RisingFallingPlatform_Func_2(void);
void RisingFallingPlatform_Func_3(void);
void RisingFallingPlatform_Func_4(void);
void RisingFallingPlatform_Func_5(void);
void RisingFallingPlatform_Func_6(void);
void RisingFallingPlatform_Func_7(uint16 k);
void RisingFallingPlatform_Func_8(void);
void RisingFallingPlatform_Func_9(void);
void RisingFallingPlatform_Func_10(void);
void RisingFallingPlatform_Func_11(void);
void RisingFallingPlatform_Func_12(void);
void RisingFallingPlatform_Func_13(void);
void RisingFallingPlatform_Touch(void);
void RisingFallingPlatform_Shot(void);
void ShootableShutter_Shot(void);
void RisingFallingPlatform_Powerbomb(void);
void HorizontalShootableShutter_Func_2(void);
void HorizontalShootableShutter_Func_3(void);
void HorizontalShootableShutter_Func_4(void);
void HorizontalShootableShutter_Func_5(void);
void HorizontalShootableShutter_Func_6(void);
void HorizontalShootableShutter_Func_7(uint16 k);
void HorizontalShootableShutter_Func_8(void);
void HorizontalShootableShutter_Func_9(void);
void HorizontalShootableShutter_Func_10(uint16 k);
void HorizontalShootableShutter_Func_11(uint16 k);
void HorizontalShootableShutter_Func_12(void);
void HorizontalShootableShutter_Func_13(void);
void HorizontalShootableShutter_Func_14(void);
void Enemy_NormalTouchAI_A3(void);
void Enemy_NormalShotAI_A3(void);
void Enemy_NormalShotAI_SkipSomeParts_A3(void);
void Enemy_NormalPowerBombAI_A3(void);
void Enemy_NormalFrozenAI_A3(void);
uint16 Waver_Instr_1(uint16 k, uint16 jp);
void Waver_Init(void);
void Waver_Main(void);
void Waver_Func_1(void);
uint16 Metalee_Instr_1(uint16 k, uint16 jp);
void Metalee_Init(void);
void CallMetaleeFunc(uint32 ea);
void Metalee_Main(void);
void Metalee_Func_1(void);
void Metalee_Func_2(uint16 k);
void Metalee_Func_3(uint16 k);
void Metalee_Func_4(void);
void Metalee_Func_5(void);
void Metalee_Func_6(void);
void Metalee_Shot(void);
void Fireflea_Init(void);
void Fireflea_Func_1(uint16 k);
void Fireflea_Func_2(uint16 k);
void Fireflea_Func_3(uint16 k);
void Fireflea_Func_4(uint16 k);
void Fireflea_Func_5(uint16 k);
void Fireflea_Func_6(uint16 k);
void Fireflea_Main(void);
void Fireflea_Touch(uint16 k);
void Fireflea_Powerbomb(void);
void Fireflea_Shot(void);
void Fireflea_Common(void);
uint16 MaridiaFish_Instr_3(uint16 k, uint16 jp);
uint16 MaridiaFish_Instr_1(uint16 k, uint16 jp);
uint16 MaridiaFish_Instr_2(uint16 k, uint16 jp);
void MaridiaFish_Init(void);
void MaridiaFish_Func_1(void);
void MaridiaFish_Func_2(void);
void MaridiaFish_Func_3(void);
void MaridiaFish_Func_4(void);
void CallMaridiaFishFunc(uint32 ea);
void MaridiaFish_Main(void);
void Elevator_Init(void);
void Elevator_Frozen(void);
void Elevator_Func_1(void);
void Elevator_Func_2(void);
void Elevator_Func_3(void);
void Elevator_Func3b(void);
void Elevator_Func_4(void);
void Crab_Init(void);
void Crab_Func_1(void);
void Slug_Init(void);
void Slug_Func_1(void);
uint16 PlatformThatFallsWithSamus_Instr_3(uint16 k, uint16 jp);
uint16 PlatformThatFallsWithSamus_Instr_4(uint16 k, uint16 jp);
uint16 PlatformThatFallsWithSamus_Instr_1(uint16 k, uint16 jp);
uint16 PlatformThatFallsWithSamus_Instr_2(uint16 k, uint16 jp);
void PlatformThatFallsWithSamus_Init(void);
void FastMovingSlowSinkingPlatform_Init(void);
void PlatformThatFalls_Init(uint16 k, uint16 j);
void PlatformThatFallsWithSamus_Main(void);
void PlatformThatFallsWithSamus_Func_1(void);
void PlatformThatFallsWithSamus_Func_2(void);
void PlatformThatFallsWithSamus_Func_3(void);
void PlatformThatFallsWithSamus_Func_4(void);
void PlatformThatFallsWithSamus_Func_5(void);
void PlatformThatFallsWithSamus_Func_6(void);
void PlatformThatFallsWithSamus_Func_7(void);
void PlatformThatFallsWithSamus_Func_8(void);
void PlatformThatFallsWithSamus_Func_9(void);
void PlatformThatFallsWithSamus_Func_10(void);
void FastMovingSlowSinkingPlatform_Shot(void);
void Roach_Func_1(void);
void Roach_Init(void);
void Roach_Func_2(uint16 k);
void Roach_Func_3(uint16 k);
void Roach_Func_4(uint16 k);
void Roach_Func_5(uint16 k);
void Roach_Func_6(uint16 k);
void Roach_Func_7(uint16 k);
void Roach_Func_8(uint16 k);
void CallRoachFunc(uint32 ea);
void Roach_Main(void);
void Roach_Func_9(void);
void Roach_Func_10(void);
void Roach_Func_11(void);
void Roach_Func_12(void);
void Roach_Func_13(void);
void Roach_Func_14(void);
void Roach_Func_15(void);
void Roach_Func_16(void);
void Roach_Func_17(uint16 k);
void Roach_Func_18(uint16 k);
void Roach_Func_19(void);
void Roach_Func_20(void);
void Roach_Func_21(void);
void Roach_Func_22(void);
void Roach_Func_23(void);
void Roach_Func_24(void);
void Roach_Func_25(void);
void Roach_Func_26(void);
void Roach_Func_27(void);
void Roach_Func_28(void);
void Roach_Func_29(uint16 k);
void Roach_Func_30(uint16 k);
void Roach_Func_31(uint16 k);
void Roach_Func_32(uint16 k);
void Roach_Func_33(uint16 k);
uint16 Sidehopper_Func_1(uint16 k, uint16 jp);
uint16 Sidehopper_Instr_1(uint16 k, uint16 jp);
void Sidehopper_Init(void);
uint16 Sidehopper_Func_2(uint16 r22, uint16 r24);
void Sidehopper_Func_3(void);
void Sidehopper_Main(void);
void Sidehopper_Func_4(uint16 k);
void Sidehopper_Func_5(void);
void Sidehopper_Func_6(void);
void Sidehopper_Func_7(void);
void Sidehopper_Func_8(void);
void Sidehopper_Func_9(void);
void Sidehopper_Func_10(void);
void Sidehopper_Func_11(void);
void Sidehopper_Func_12(void);
void Sidehopper_Func_13(void);
void Sidehopper_Func_14(void);
void Sidehopper_Func_15(void);
void Sidehopper_Func_16(void);
void Sidehopper_Func_17(void);
void Sidehopper_Func_18(void);
static void CallSidehopperFunc(uint32 ea);
void Sidehopper_Func_19(void);
void Sidehopper_Func_20(void);
void Sidehopper_Func_21(void);
void Sidehopper_Func_22(void);
uint16 Skree_Instr_1(uint16 k, uint16 jp);
void Skree_Init(void);
void CallSkreeFunc(uint32 ea);
void Skree_Main(void);
void Skree_Func_1(void);
void Skree_Func_2(uint16 k);
void Skree_Func_3(void);
void Skree_Func_4(void);
void Skree_Func_5(void);
void Skree_Shot(void);
static uint32 Shift8AddMagn(int16 a, int s);
uint16 WreckedShipOrangeZoomer_Func_1(uint16 k, uint16 jp);
void WreckedShipOrangeZoomer_Init(void);
void WreckedShipOrangeZoomer_Main(void);
void WreckedShipOrangeZoomer_Func_2(uint16 k);
void sub_A3E168(uint16 k);
void BigEyeBugs_Init(void);
void FireZoomer_Init(void);
uint16 Zoomer_Instr_SetPreinstr(uint16 k, uint16 jp);
void StoneZoomer_Init(void);
void StoneZoomer_E67A(uint16 k);
void StoneZoomer_Main(void);
void FireZoomer_Func_1(uint16 k);
void FireZoomer_Func_2(uint16 k);
void FireZoomer_Func_3(uint16 k);
int32 FireZoomer_E8A5(uint16 k);
uint16 Draygon_Instr_1(uint16 k, uint16 jp);
uint16 Draygon_Instr_13(uint16 k, uint16 jp);
static Point16U Draygon_Func_47(void);
uint16 Draygon_Instr_8(uint16 k, uint16 jp);
uint16 Draygon_Instr_7(uint16 k, uint16 jp);
uint16 Draygon_Instr_6(uint16 k, uint16 jp);
uint16 Draygon_Instr_9(uint16 k, uint16 jp);
uint16 Draygon_Instr_2(uint16 k, uint16 jp);
uint16 Draygon_Instr_11(uint16 k, uint16 jp);
uint16 Draygon_Instr_5(uint16 k, uint16 jp);
uint16 Draygon_Instr_15(uint16 k, uint16 jp);
uint16 Draygon_Instr_17(uint16 k, uint16 jp);
uint16 Draygon_Instr_14(uint16 k, uint16 jp);
uint16 Draygon_Instr_16(uint16 k, uint16 jp);
uint16 Draygon_Instr_10(uint16 k, uint16 jp);
uint16 Draygon_Instr_4(uint16 k, uint16 jp);
uint16 Draygon_Instr_12(uint16 k, uint16 jp);
uint16 Draygon_Instr_18(uint16 k, uint16 jp);
uint16 Draygon_Instr_3(uint16 k, uint16 jp);
uint16 Draygon_Instr_25(uint16 k, uint16 jp);
uint16 Draygon_Instr_24(uint16 k, uint16 jp);
uint16 Draygon_Instr_21(uint16 k, uint16 jp);
uint16 Draygon_Instr_53(uint16 k, uint16 j);
uint16 Draygon_Instr_22(uint16 k, uint16 jp);
uint16 Draygon_Instr_27(uint16 k, uint16 jp);
uint16 Draygon_Instr_23(uint16 k, uint16 jp);
uint16 Draygon_Instr_30(uint16 k, uint16 jp);
uint16 Draygon_Instr_20(uint16 k, uint16 jp);
uint16 Draygon_Instr_29(uint16 k, uint16 jp);
uint16 Draygon_Instr_19(uint16 k, uint16 jp);
uint16 Draygon_Instr_28(uint16 k, uint16 jp);
uint16 Draygon_Instr_26(uint16 k, uint16 jp);
void sub_A5E9F5(void);
void SporeSpawn_Init(void);
void CallSporeSpawnFunc(uint32 ea);
void SporeSpawn_Main(void);
void SporeSpawn_Func_1(void);
void SporeSpawn_Func_2(uint16 k);
void SporeSpawn_Func_3(void);
void SporeSpawn_Func_4(void);
void SporeSpawn_Func_5(void);
void SporeSpawn_Shot(void);
void SporeSpawn_Touch(void);
void SporeSpawn_Func_6(void);
void SporeSpawn_Func_7(uint16 a);
void Boulder_Init(void);
void CallBoulderFunc(uint32 ea);
void Boulder_Main(void);
void Boulder_Func_1(void);
void Boulder_Func_2(void);
void Boulder_Func_3(void);
void Boulder_Func_4(void);
void Boulder_Func_5(void);
void Boulder_Func_6(void);
void Boulder_Func_7(uint16 j);
void Boulder_Func_8(uint16 j);
void Boulder_Func_9(uint16 j);
void FakeKraid_Init(void);
void FakeKraid_Main(void);
void FakeKraid_Func_1(uint16 k, uint16 j);
uint16 FakeKraid_Instr_2(uint16 k, uint16 jp);
uint16 FakeKraid_Instr_1(uint16 k, uint16 jp);
uint16 FakeKraid_Instr_3(uint16 k, uint16 jp);
uint16 FakeKraid_Instr_4(uint16 k, uint16 jp);
void FakeKraid_InstrHelper_45(uint16 k, uint16 j, uint16 a);
uint16 FakeKraid_Instr_5(uint16 k, uint16 jp);
void FakeKraid_Touch(void);
void FakeKraid_Shot(void);
void FakeKraid_9C50(void);
void Etecoon_Init(void);
void Etecoon_Main(void);
void Etecoon_Func_1(uint16 k);
uint8 Etecoon_Func_2(uint16 k);
uint8 Etecoon_Func_3(uint16 k);
void Etecoon_Func_4(uint16 k);
void Etecoon_Func_5(uint16 k);
void Etecoon_Func_6(uint16 k);
void Etecoon_Func_7(uint16 k);
void Etecoon_Func_8(uint16 k);
void Etecoon_Func_9(uint16 k);
void Etecoon_Func_10(uint16 k);
void Etecoon_Func_11(uint16 k);
void Etecoon_Func_12(uint16 k);
void Etecoon_Func_13(uint16 j);
void Etecoon_Func_14(uint16 j);
void Etecoon_Func_15(uint16 j);
void Etecoon_Func_16(uint16 k);
void Etecoon_Func_17(uint16 k);
void Etecoon_Func_18(uint16 k);
void Etecoon_Func_19(uint16 k);
void Etecoon_Func_20(uint16 k);
void Etecoon_Func_21(uint16 k);
void Etecoon_Func_22(uint16 k);
void Etecoon_Func_23(uint16 k);
void Etecoon_Func_24(uint16 k);
void Etecoon_Func_25(uint16 k);
void Etecoon_Func_26(uint16 k);
void Dachora_Init(void);
void Dachora_Main(void);
void Dachora_Func_1(uint16 j, uint16 k);
void Dachora_Func_2(uint16 k);
void Dachora_Func_3(uint16 k);
void Dachora_Func_4(uint16 k);
void Dachora_Func_5(uint16 k);
int32 Dachora_Func_6(uint16 k);
void Dachora_Func_7(uint16 k);
void Dachora_Func_8(uint16 k);
void Dachora_Func_9(uint16 k);
void Dachora_Func_10(uint16 k);
void Dachora_Func_11(uint16 k);
void Dachora_Func_12(uint16 k);
void Enemy_NormalTouchAI_A8(void);
void Enemy_NormalShotAI_A8(void);
void Enemy_NormalFrozenAI_A8(void);
void MorphBallEye_Init(void);
void MorphBallEye_Main(void);
void MorphBallEye_Func_1(uint16 k);
void MorphBallEye_Func_2(uint16 k);
void MorphBallEye_Func_3(uint16 k);
void MorphBallEye_Func_4(uint16 k);
uint16 YappingMaw_Instr_2(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_4(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_5(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_7(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_3(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_6(uint16 k, uint16 jp);
uint16 YappingMaw_Instr_1(uint16 k, uint16 jp);
void YappingMaw_Init(void);
void CallYappingMawFunc(uint32 ea);
void YappingMaw_Main(void);
void YappingMaw_Func_1(void);
void YappingMaw_Func_2(void);
void YappingMaw_Func_3(void);
void YappingMaw_Func_4(void);
void YappingMaw_Func_5(void);
void YappingMaw_Func_6(void);
void YappingMaw_Func_7(void);
void YappingMaw_Func_8(void);
void YappingMaw_Func_9(uint16 k);
void YappingMaw_Func_10(void);
void YappingMaw_Func_11(void);
void YappingMaw_Func_12(void);
void YappingMaw_Func_13(void);
void YappingMaw_Func_14(void);
void YappingMaw_Func_15(void);
uint16 YappingMaw_Func_16(uint16 a, uint16 varE32);
uint16 YappingMaw_Func_17(uint16 a, uint16 varE32);
void YappingMaw_Touch(void);
void YappingMaw_Shot(void);
void YappingMaw_Frozen(void);
void YappingMaw_Func_18(uint16 j);
void YappingMaw_Func_19(void);
void Kago_Init(void);
void CallKagoFunc(uint32 ea, uint16 k);
void Kago_Main(void);
void Kago_Func_1(uint16 k);
void Kago_Shot(void);
uint16 Beetom_Instr_1(uint16 k, uint16 jp);
void Beetom_Func_1(void);
void Beetom_Init(void);
uint16 Beetom_Func_2(uint16 r22, uint16 r24);
void Beetom_Func_3(void);
void Beetom_Func_4(void);
void Beetom_Func_5(void);
void Beetom_Func_6(void);
void Beetom_Func_7(void);
void Beetom_Func_8(void);
void Beetom_Func_9(void);
void Beetom_Func_10(void);
void Beetom_Func_11(void);
void Beetom_Func_12(void);
void Beetom_Func_13(void);
void Beetom_Func_14(void);
void Beetom_Func_15(void);
void Beetom_Func_16(void);
void Beetom_Func_17(void);
void Beetom_Func_18(void);
void Beetom_Func_19(void);
void Beetom_Func_22(uint16 k);
void Beetom_Func_23(uint16 k);
void Beetom_Func_20(void);
void Beetom_Func_21(void);
void Beetom_Func_26(uint16 k);
void Beetom_Func_27(uint16 k);
void Beetom_Func_24(void);
void Beetom_Func_25(void);
void Beetom_Func_30(uint16 k);
void Beetom_Func_31(uint16 k);
void Beetom_Func_28(void);
void Beetom_Func_29(void);
void Beetom_Func_34(void);
void Beetom_Func_32(void);
void Beetom_Func_33(void);
void Beetom_Func_35(void);
void Beetom_Func_37(uint16 k);
void Beetom_Func_36(void);
void CallBeetomFunc(uint32 ea);
void Beetom_Main(void);
void Beetom_Touch(void);
void Beetom_Shot(void);
void WalkingLavaSeahorse_Init(void);
void WalkingLavaSeahorse_Func_1(uint16 k);
uint16 WalkingLavaSeahorse_Func_2(uint16 k);
void CallWalkingLavaSeahorseFunc(uint32 ea, uint16 k);
void WalkingLavaSeahorse_Main(void);
void WalkingLavaSeahorse_Func_3(uint16 k);
void WalkingLavaSeahorse_Func_4(uint16 k);
void sub_A8DDDE(uint16 k);
void WalkingLavaSeahorse_Func_5(uint16 k);
void WalkingLavaSeahorse_Func_6(uint16 k);
void WalkingLavaSeahorse_Func_7(uint16 k);
void WalkingLavaSeahorse_Func_8(uint16 k);
void WalkingLavaSeahorse_Func_9(uint16 k);
uint16 WalkingLavaSeahorse_Instr_4(uint16 k, uint16 jp);
void WalkingLavaSeahorse_DF20(uint16 a);
uint16 WalkingLavaSeahorse_Instr_3(uint16 k, uint16 jp);
uint16 WalkingLavaSeahorse_Instr_5(uint16 k, uint16 jp);
uint16 WalkingLavaSeahorse_Instr_6(uint16 k, uint16 jp);
uint16 WalkingLavaSeahorse_Instr_2(uint16 k, uint16 jp);
uint16 WalkingLavaSeahorse_Instr_1(uint16 k, uint16 jp);
void WalkingLavaSeahorse_Func_10(void);
void BlueBrinstarFaceBlock_Init(void);
void BlueBrinstarFaceBlock_Func_1(void);
void BlueBrinstarFaceBlock_Main(void);
void BlueBrinstarFaceBlock_Shot(void);
void KiHunter_Init(void);
void KiHunterWings_Init(void);
void CallKiHunterFunc(uint32 ea, uint16 k);
void KiHunter_Main(void);
void KiHunterWings_Main(void);
void KiHunter_Func_1(uint16 k);
void KiHunter_Func_2(uint16 k);
void KiHunter_Func_3(uint16 k);
uint16 KiHunter_Instr_1(uint16 k, uint16 jp);
void KiHunter_Func_4(uint16 k);
void KiHunter_Func_5(uint16 k);
uint16 KiHunter_Instr_2(uint16 k, uint16 jp);
void KiHunter_Func_6(uint16 k);
uint16 KiHunter_Instr_3(uint16 k, uint16 jp);
void KiHunter_Func_7(uint16 k);
void KiHunter_Func_8(uint16 k);
uint16 KiHunter_Instr_4(uint16 k, uint16 jp);
uint16 KiHunter_Instr_5(uint16 k, uint16 jp);
void sub_A8F6DC(uint16 k, uint16 j);
void KiHunter_Func_9(uint16 k);
void KiHunter_Shot(void);
void CallKiHunterBFunc(uint32 ea);
void KiHunter_Func_10(uint16 k);
void KiHunter_Func_11(void);
void KiHunter_Func_12(void);
void KiHunter_Func_13(void);
void KiHunter_Func_14(void);
void KiHunter_Func_15(uint16 k);
void KiHunter_Func_16(uint16 k);
void KiHunter_Func_17(void);
void Enemy_NormalFrozenAI_AA(void);
uint16 Enemy_ClearAiPreInstr_AA(uint16 k, uint16 jp);
uint16 Enemy_SetAiPreInstr_AA(uint16 k, uint16 jp);
uint16 Torizo_Instr_3(uint16 k, uint16 jp);
uint16 Torizo_Instr_31(uint16 k, uint16 jp);
uint16 Torizo_Instr_33(uint16 k, uint16 jp);
uint16 Torizo_Instr_36(uint16 k, uint16 jp);
uint16 Torizo_Instr_37(uint16 k, uint16 jp);
uint16 Torizo_Instr_35(uint16 k, uint16 jp);
uint16 Torizo_Instr_38(uint16 k, uint16 jp);
uint16 Torizo_Instr_6(uint16 k, uint16 jp);
uint16 Torizo_Instr_5(uint16 k, uint16 jp);
uint16 Torizo_Instr_9(uint16 k, uint16 jp);
void Torizo_C20A(uint16 k);
void Torizo_C22D(uint16 k);
void Torizo_C250(void);
void Torizo_C268(void);
void Torizo_C280(void);
void Torizo_C298(void);
void Torizo_C2B0(void);
uint16 Torizo_Instr_7(uint16 k, uint16 jp);
uint16 Torizo_Instr_2(uint16 k, uint16 jp);
uint16 Torizo_Instr_8(uint16 k, uint16 jp);
uint16 Torizo_Instr_25(uint16 k, uint16 jp);
uint16 Torizo_Instr_22(uint16 k, uint16 jp);
uint16 Torizo_Instr_19(uint16 k, uint16 jp);
uint16 Torizo_Instr_32(uint16 k, uint16 jp);
uint16 Torizo_Instr_30(uint16 k, uint16 jp);
uint16 Torizo_Instr_34(uint16 k, uint16 jp);
uint16 Torizo_Instr_24(uint16 k, uint16 jp);
uint16 Torizo_Instr_12(uint16 k, uint16 jp);
uint16 Torizo_Instr_10(uint16 k, uint16 jp);
uint16 Torizo_Instr_11(uint16 k, uint16 jp);
uint16 Torizo_Instr_29(uint16 k, uint16 jp);
uint16 Torizo_Instr_1(uint16 k, uint16 jp);
uint16 Torizo_Instr_28(uint16 k, uint16 jp);
uint16 Torizo_Instr_4(uint16 k, uint16 jp);
uint16 Torizo_Instr_40(uint16 k, uint16 jp);
uint16 Torizo_Instr_16(uint16 k, uint16 jp);
uint16 Torizo_Instr_27(uint16 k, uint16 jp);
uint16 Torizo_Instr_23(uint16 k, uint16 jp);
uint16 Torizo_Instr_14(uint16 k, uint16 jp);
uint16 Torizo_Instr_15(uint16 k, uint16 jp);
uint16 Torizo_Instr_26(uint16 k, uint16 jp);
uint16 Torizo_Instr_18(uint16 k, uint16 jp);
uint16 Torizo_Instr_20(uint16 k, uint16 jp);
uint16 Torizo_Instr_44(uint16 k, uint16 jp);
uint16 Torizo_Instr_21(uint16 k, uint16 jp);
uint16 Torizo_Instr_17(uint16 k, uint16 jp);
uint16 Torizo_Instr_13(uint16 k, uint16 jp);
void Torizo_C620(uint16 k);
void Torizo_C643(uint16 k);
void Torizo_Hurt(void);
void CallTorizoFunc(uint32 ea, uint16 k);
void Torizo_Main(void);
void Torizo_Func_2(uint16 k);
void Torizo_Func_3(uint16 k);
void Torizo_Func_4(uint16 k);
void Torizo_Func_1(uint16 k);
void Torizo_Func_5(uint16 k);
void Torizo_Func_6(uint16 k);
void Torizo_Func_7(uint16 k);
void Torizo_Init(void);
void GoldTorizo_Touch(void);
void Torizo_Shot(void);
void Torizo_Func_8(void);
uint16 Torizo_Instr_39(uint16 k, uint16 jp);
uint16 Torizo_Instr_41(uint16 k, uint16 jp);
uint16 Torizo_Instr_42(uint16 k, uint16 jp);
uint16 Torizo_Instr_48(uint16 k, uint16 jp);
uint16 Torizo_Instr_57(uint16 k, uint16 jp);
uint16 Torizo_Instr_58(uint16 k, uint16 jp);
uint16 Torizo_Instr_59(uint16 k, uint16 jp);
uint16 Torizo_Instr_62(uint16 k, uint16 jp);
uint16 Torizo_Instr_63(uint16 k, uint16 jp);
uint16 Torizo_Instr_56(uint16 k, uint16 jp);
uint16 Torizo_Instr_60(uint16 k, uint16 jp);
uint16 Torizo_Instr_46(uint16 k, uint16 jp);
uint16 Torizo_Func_12(uint16 k);
void Torizo_Func_11(uint16 k);
uint16 Torizo_Instr_47(uint16 k, uint16 jp);
uint16 Torizo_Instr_49(uint16 k, uint16 jp);
uint16 Torizo_Instr_61(uint16 k, uint16 jp);
uint16 Torizo_Instr_53(uint16 k, uint16 jp);
uint16 Torizo_Instr_55(uint16 k, uint16 jp);
uint16 Torizo_Instr_52(uint16 k, uint16 jp);
uint16 Torizo_Instr_50(uint16 k, uint16 jp);
uint16 Torizo_Instr_43(uint16 k, uint16 jp);
uint16 Torizo_Instr_51(uint16 k, uint16 jp);
uint16 Torizo_Instr_45(uint16 k, uint16 jp);
uint16 Torizo_Instr_54(uint16 k, uint16 jp);
void Torizo_Func_9(uint16 k);
void Torizo_Func_10(uint16 k);
void Torizo_D5E6(uint16 k);
void Torizo_D5ED(uint16 k);
void Torizo_D5F1(uint16 k);
void Torizo_D658(void);
void GoldTorizo_Shot(void);
void Torizo_D6A6(void);
void Torizo_D6D1(uint16 k, uint16 j);
void Torizo_D6F7(uint16 k, uint16 j);
void TourianEntranceStatue_Init(void);
void ChozoStatue_Init(void);
void sub_AAE784(void);
void ChozoStatue_Main(void);
void Enemy_NormalFrozenAI_B2(void);
void WalkingSpacePirates_Powerbomb(void);
void WalkingSpacePirates_Touch(void);
void WalkingSpacePirates_Shot(void);
void WalkingSpacePirates_8789(void);
void WalkingSpacePirates_87C8(void);
void WalkingSpacePirates_883E(void);
uint16 SpacePirates_Instr_MovePixelsDownAndChangeDirFaceRight(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_MovePixelsDownAndChangeDirFaceLeft(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_RandomNewDirFaceR(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_RandomNewDirFaceL(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_PrepareWallJumpR(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_PrepareWallJumpL(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_FireLaserL(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_FireLaserR(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_SetEnemyFunc(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_PlaySfx(uint16 k, uint16 jp);
void WallSpacePirates_Init(void);
void CallSpacePiratesEnemyFunc(uint32 ea, uint16 k);
void WallSpacePirates_Main(void);
void SpacePirates_Func_1(uint16 k);
void SpacePirates_Func_2(uint16 k);
void SpacePirates_Func_3(uint16 k);
void SpacePirates_Func_4(uint16 k);
uint16 SpacePirates_Instr_20(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_16(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_15(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_18(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_17(uint16 k, uint16 jp);
void NinjaSpacePirates_Init(void);
void NinjaSpacePirates_Main(void);
void SpacePirates_Func_5(uint16 k);
void SpacePirates_Func_6(uint16 k);
void SpacePirates_Func_7(uint16 k);
uint16 SpacePirates_Func_8(void);
uint16 SpacePirates_Func_9(void);
uint16 SpacePirates_Func_10(void);
void SpacePirates_Func_11(uint16 k);
void SpacePirates_Func_12(uint16 k);
void SpacePirates_Func_13(uint16 k);
void SpacePirates_Func_14(uint16 k);
void SpacePirates_Func_15(uint16 k);
void SpacePirates_Func_16(void);
uint16 SpacePirates_Instr_19(uint16 k, uint16 jp);
void SpacePirates_F985(uint16 k);
void SpacePirates_F9C1(uint16 k);
void SpacePirates_FA15(uint16 k);
uint16 SpacePirates_Instr_21(uint16 k, uint16 jp);
void SpacePirates_FA59(uint16 k);
void SpacePirates_FA95(uint16 k);
void SpacePirates_FAE9(uint16 k);
void SpacePirates_FB11(uint16 k);
uint16 SpacePirates_Instr_12(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_14(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_11(uint16 k, uint16 jp);
uint16 SpacePirates_Instr_13(uint16 k, uint16 jp);
void WalkingSpacePirates_Init(void);
void WalkingSpacePirates_Main(void);
void WalkingSpacePirates_FD44(void);
void WalkingSpacePirates_FDCE(void);
void WalkingSpacePirates_FE4B(void);
void Enemy_NormalTouchAI_B3(void);
void Enemy_NormalShotAI_B3(void);
void Enemy_NormalFrozenAI_B3(void);
uint16 Enemy_SetAiPreInstr_B3(uint16 k, uint16 jp);
uint16 Enemy_ClearAiPreInstr_B3(uint16 k, uint16 jp);
void BrinstarPipeBug_Init(void);
void BrinstarPipeBug_Main(void);
void BrinstarPipeBug_PreInstr_1(uint16 k);
void BrinstarPipeBug_PreInstr_2(uint16 k);
void BrinstarPipeBug_PreInstr_3(uint16 k);
void BrinstarPipeBug_PreInstr_4(uint16 k);
void BrinstarPipeBug_PreInstr_5(uint16 k);
void BrinstarPipeBug_Func_1(void);
void CallBugInstr(uint32 ea, uint16 k);
void NorfairPipeBug_Func_1(void);
void NorfairPipeBug_Func_2(void);
void NorfairPipeBug_Func_3(uint16 k);
void NorfairPipeBug_Func_4(void);
void NorfairPipeBug_Func_5(uint16 k);
void NorfairPipeBug_Func_6(void);
void NorfairPipeBug_Func_7(void);
void NorfairPipeBug_Func_8(void);
void NorfairPipeBug_Func_9(void);
void NorfairPipeBug_Func_10(void);
void NorfairPipeBug_Func_11(void);
void NorfairPipeBug_Func_12(void);
void BrinstarYellowPipeBug_Init(void);
void BrinstarYellowPipeBug_Main(void);
void BrinstarYellowPipeBug_Func_1(void);
void BrinstarYellowPipeBug_Func_2(uint16 k);
void BrinstarYellowPipeBug_Func_3(void);
void BrinstarYellowPipeBug_Func_4(uint16 k);
void BrinstarYellowPipeBug_Func_5(void);
void BrinstarYellowPipeBug_Func_6(uint16 k);
void BrinstarYellowPipeBug_Func_7(void);
void BrinstarYellowPipeBug_Func_8(void);
void BrinstarYellowPipeBug_Func_9(uint16 k);
void BrinstarYellowPipeBug_Func_10(uint16 k);
uint16 EscapeEtecoon_Instr_1(uint16 k, uint16 jp);
uint16 EscapeEtecoon_Instr_2(uint16 k, uint16 jp);
void EscapeEtecoon_Main(void);
void EscapeEtecoon_E65C(uint16 k);
void EscapeEtecoon_E670(uint16 k);
void EscapeEtecoon_E680(uint16 k);
void EscapeEtecoon_Init(void);
uint16 EscapeDachora_Instr_2(uint16 k, uint16 jp);
uint16 EscapeDachora_Instr_3(uint16 k, uint16 jp);
uint16 EscapeDachora_Instr_1(uint16 k, uint16 jp);
uint16 EscapeDachora_Instr_4(uint16 k, uint16 jp);
void EscapeDachora_Init(void);
static Func_Y_V *const funcs_A2A79[3];
static Func_Y_V *const kEprojPreInstr_SpikeShootingPlantSpikes[10];
static Func_V *const off_86F0AD[7];
static Func_EnemyBlockCollInfo_U8 *const off_A0C859[16];
static Func_EnemyBlockCollInfo_U8 *const off_A0C88D[16];
static Func_V *const off_A28718[2];
static Func_V *const off_A2EB1A[5];
static Func_V *const off_A2EC3A[5];
static Func_V *const off_A2EDFB[5];
static Func_V *const off_A2F107[5];
static Func_V *const off_A39540[4];
static Func_V *const off_A39C97[2];
static Func_V *const off_A39C9B[2];
const uint8 * EprojInstr_GotoIfFunc1(uint16 k, const uint8 *epjp);
void GrappleBeam_Func2(void);
void Samus_InputHandler_E913(void);
void Samus_InputHandler_E91D(void);
void SetGoldenTorizoPalette(uint16 a);
void Torizo_CorpseRottingCopyFunc(uint16 j, uint16 k);
void Torizo_CorpseRottingInitFunc(void);
void Torizo_CorpseRottingMoveFunc(uint16 j, uint16 k);
void WriteReg(uint16 reg, uint8 value);

const uint8 * EprojInstr_GotoIfFunc1(uint16 k, const uint8 *epjp) { nt_unimpl("EprojInstr_GotoIfFunc1"); return 0; }
void GrappleBeam_Func2(void) { nt_unimpl("GrappleBeam_Func2"); }
void Samus_InputHandler_E913(void) { nt_unimpl("Samus_InputHandler_E913"); }
void Samus_InputHandler_E91D(void) { nt_unimpl("Samus_InputHandler_E91D"); }
void SetGoldenTorizoPalette(uint16 a) { nt_unimpl("SetGoldenTorizoPalette"); }
void Torizo_CorpseRottingCopyFunc(uint16 j, uint16 k) { nt_unimpl("Torizo_CorpseRottingCopyFunc"); }
void Torizo_CorpseRottingInitFunc(void) { nt_unimpl("Torizo_CorpseRottingInitFunc"); }
void Torizo_CorpseRottingMoveFunc(uint16 j, uint16 k) { nt_unimpl("Torizo_CorpseRottingMoveFunc"); }
void WriteReg(uint16 reg, uint8 value) { nt_unimpl("WriteReg"); }

static Func_Y_V *const funcs_A2A79[3] = { Etecoon_Func_13, Etecoon_Func_14, Etecoon_Func_15 };
static Func_Y_V *const kEprojPreInstr_SpikeShootingPlantSpikes[10] = {  // 0x86D9DB
  EprojPreInstr_SpikeShootingPlantSpikes_0_MoveX1,
  EprojPreInstr_SpikeShootingPlantSpikes_1_MoveY1,
  EprojPreInstr_SpikeShootingPlantSpikes_2_MoveX2,
  EprojPreInstr_SpikeShootingPlantSpikes_0_MoveX1,
  EprojPreInstr_SpikeShootingPlantSpikes_4_MoveY2,
  EprojPreInstr_SpikeShootingPlantSpikes_2_MoveX2,
  EprojPreInstr_SpikeShootingPlantSpikes_6_MoveX1Y1,
  EprojPreInstr_SpikeShootingPlantSpikes_7_MoveX2Y1,
  EprojPreInstr_SpikeShootingPlantSpikes_8_MoveX1Y2,
  EprojPreInstr_SpikeShootingPlantSpikes_9_MoveX2Y2,
};
static Func_V *const off_86F0AD[7] = {  // 0x86EFE0
  0,
  Eproj_Pickup_SmallHealth,
  Eproj_Pickup_BigHealth,
  Eproj_Pickup_PowerBombs,
  Eproj_Pickup_Missiles,
  Eproj_Pickup_SuperMissiles,
  0,
};
static Func_EnemyBlockCollInfo_U8 *const off_A0C859[16] = {  // 0xA0C845
  ClearCarry_13,
  EnemyBlockCollHorizReact_Slope,
  ClearCarry_13,
  ClearCarry_13,
  ClearCarry_13,
  EnemyBlockCollReact_HorizExt,
  ClearCarry_13,
  ClearCarry_13,
  SetCarry_4,
  SetCarry_4,
  EnemyBlockCollReact_Spike,
  SetCarry_4,
  SetCarry_4,
  EnemyBlockCollReact_VertExt,
  SetCarry_4,
  SetCarry_4,
};
static Func_EnemyBlockCollInfo_U8 *const off_A0C88D[16] = {  // 0xA0C879
  ClearCarry_13,
  EnemyBlockCollVertReact_Slope,
  ClearCarry_13,
  ClearCarry_13,
  ClearCarry_13,
  EnemyBlockCollReact_HorizExt,
  ClearCarry_13,
  ClearCarry_13,
  SetCarry_4,
  SetCarry_4,
  EnemyBlockCollReact_Spike,
  SetCarry_4,
  SetCarry_4,
  EnemyBlockCollReact_VertExt,
  SetCarry_4,
  SetCarry_4,
};
static Func_V *const off_A28718[2] = { BouncingGoofball_State0, BouncingGoofball_State1 };
static Func_V *const off_A2EB1A[5] = { TimedShutter_Func_6, TimedShutter_Func_7, TimedShutter_Func_8, TimedShutter_Func_9, TimedShutter_Func_Null };
static Func_V *const off_A2EC3A[5] = {  // 0xA2EC13
  TimedShutter_Func_11,
  TimedShutter_Func_12,
  TimedShutter_Func_13,
  TimedShutter_Func_14,
  TimedShutter_Func_Null2,
};
static Func_V *const off_A2EDFB[5] = {  // 0xA2EF09
  RisingFallingPlatform_Func_3,
  RisingFallingPlatform_Func_4,
  RisingFallingPlatform_Func_5,
  RisingFallingPlatform_Func_6,
  RisingFallingPlatform_Func_6,
};
static Func_V *const off_A2F107[5] = {  // 0xA2F224
  HorizontalShootableShutter_Func_3,
  HorizontalShootableShutter_Func_4,
  HorizontalShootableShutter_Func_5,
  HorizontalShootableShutter_Func_6,
  HorizontalShootableShutter_Func_6,
};
static Func_V *const off_A39540[4] = { Elevator_Func_1, Elevator_Func_2, Elevator_Func_3, Elevator_Func3b };
static Func_V *const off_A39C97[2] = { PlatformThatFallsWithSamus_Func_1, PlatformThatFallsWithSamus_Func_2 };
static Func_V *const off_A39C9B[2] = { PlatformThatFallsWithSamus_Func_3, PlatformThatFallsWithSamus_Func_4 };

static const int16 kEprojInit_BombTorizoStatueBreaking_F[8] = { 16, 16, 16, 16, 16, 16, 16, 16 };
static const int16 kEprojInit_BombTorizoStatueBreaking_Yvel[8] = { 256, 256, 256, 256, 256, 256, 256, 256 };
static const int16 kEprojInit_MiniKraidSpikes_Tab0[3] = { -2, 12, 24 };
static const int16 kEprojInit_SporeSpawners_X[4] = { 0x20, 0x60, 0xa0, 0xe0 };
uint16 EprojInstr_Delete(uint16 k, uint16 epjp) {  // 0x868154
  eproj_id[k >> 1] = 0;
  return 0;
}
uint16 EprojInstr_Sleep(uint16 k, uint16 epjp) {  // 0x868159
  eproj_instr_list_ptr[k >> 1] = epjp - 2;
  return 0;
}
uint16 EprojInstr_SetPreInstr_(uint16 k, uint16 epjp) {  // 0x868161
  eproj_pre_instr[k >> 1] = BW_86(epjp);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_ClearPreInstr(uint16 k, uint16 epjp) {  // 0x86816A
  eproj_pre_instr[k >> 1] = 0x8170;
  return epjp;
}
void CallEprojFunc(uint32 ea, uint32 k) {
  switch (ea) {
  case 0x86C7FB: SpawnMotherBrainDeathBeam(k); return;
  default: nt_unimpl("Unreachable");
  }
}
uint16 EprojInstr_CallFunc(uint16 k, uint16 epjp) {  // 0x868171
  CallEprojFunc((BW_86(epjp) | (uint32)BB_86((uint16)((uint16)(epjp + 2))) << 16), k);
  return (uint16)(epjp + 3);
}
uint16 EprojInstr_Goto(uint16 k, uint16 epjp) {  // 0x8681AB
  return (uint16)(BW_86(epjp));
}
uint16 EprojInstr_GotoRel(uint16 k, uint16 epjp) {  // 0x8681B0
  return epjp + (int8)BB_86(epjp);
}
uint16 EprojInstr_DecTimerAndGotoIfNonZero(uint16 k, uint16 epjp) {  // 0x8681C6
  int v2 = k >> 1;
  if (eproj_timers[v2]-- == 1)
    return (uint16)(epjp + 2);
  else
    return EprojInstr_Goto(k, epjp);
}
uint16 EprojInstr_DecTimerAndGotoRelIfNonZero(uint16 k, uint16 epjp) {  // 0x8681CE
  int v2 = k >> 1;
  if (eproj_timers[v2]-- == 1)
    return (uint16)(epjp + 1);
  else
    return EprojInstr_GotoRel(k, epjp);
}
uint16 EprojInstr_SetTimer(uint16 k, uint16 epjp) {  // 0x8681D5
  eproj_timers[k >> 1] = BW_86(epjp);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_MoveRandomlyWithinRadius(uint16 k, uint16 epjp) {  // 0x8681DF
  int8 Random;
  int8 v4;
  int16 v6;

  uint16 r18 = NextRandom();
  do {
    Random = NextRandom();
    v4 = (BB_86(epjp) & Random) - BB_86((uint16)(epjp + 1));
  } while (v4 < 0);
  eproj_x_pos[k >> 1] += sign16(r18) ? -(uint8)v4 : (uint8)v4;
  do {
    LOBYTE(v6) = NextRandom();
    LOBYTE(v6) = (BB_86((uint16)(epjp + 2)) & v6) - BB_86((uint16)(epjp + 3));
  } while ((v6 & 0x80) != 0);
  v6 = (uint8)v6;
  if ((r18 & 0x4000) != 0)
    v6 = -(uint8)v6;
  eproj_y_pos[k >> 1] += v6;
  return (uint16)(epjp + 4);
}
uint16 EprojInstr_SetProjectileProperties(uint16 k, uint16 epjp) {  // 0x868230
  eproj_properties[k >> 1] |= BW_86(epjp);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_ClearProjectileProperties(uint16 k, uint16 epjp) {  // 0x86823C
  eproj_properties[k >> 1] &= BW_86(epjp);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_EnableCollisionWithSamusProj(uint16 k, uint16 epjp) {  // 0x868248
  eproj_properties[k >> 1] |= 0x8000;
  return epjp;
}
uint16 EprojInstr_DisableCollisionWithSamusProj(uint16 k, uint16 epjp) {  // 0x868252
  eproj_properties[k >> 1] &= ~0x8000;
  return epjp;
}
uint16 EprojInstr_DisableCollisionWithSamus(uint16 k, uint16 epjp) {  // 0x86825C
  eproj_properties[k >> 1] |= 0x2000;
  return epjp;
}
uint16 EprojInstr_EnableCollisionWithSamus(uint16 k, uint16 epjp) {  // 0x868266
  eproj_properties[k >> 1] &= ~0x2000;
  return epjp;
}
uint16 EprojInstr_SetToNotDieOnContact(uint16 k, uint16 epjp) {  // 0x868270
  eproj_properties[k >> 1] |= 0x4000;
  return epjp;
}
uint16 EprojInstr_SetToDieOnContact(uint16 k, uint16 epjp) {  // 0x86827A
  eproj_properties[k >> 1] &= ~0x4000;
  return epjp;
}
uint16 EprojInstr_SetLowPriority(uint16 k, uint16 epjp) {  // 0x868284
  eproj_properties[k >> 1] |= 0x1000;
  return epjp;
}
uint16 EprojInstr_SetHighPriority(uint16 k, uint16 epjp) {  // 0x86828E
  eproj_properties[k >> 1] &= ~0x1000;
  return epjp;
}
uint16 EprojInstr_SetXyRadius(uint16 k, uint16 epjp) {  // 0x868298
  eproj_radius[k >> 1] = BW_86(epjp);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_SetXyRadiusZero(uint16 k, uint16 epjp) {  // 0x8682A1
  eproj_radius[k >> 1] = 0;
  return epjp;
}
uint16 EprojInstr_CalculateDirectionTowardsSamus(uint16 k, uint16 epjp) {  // 0x8682A5
  int v2 = k >> 1;
  uint16 x = samus_x_pos - eproj_x_pos[v2];
  uint16 y = samus_y_pos - eproj_y_pos[v2];
  uint16 v3 = 2 * CalculateAngleFromXY(x, y);
  int v4 = eproj_index >> 1;
  eproj_E[v4] = v3;
  int v5 = v3 >> 1;
  eproj_x_vel[v4] = kSinCosTable8bit_Sext[v5 + 64];
  eproj_y_vel[v4] = kSinCosTable8bit_Sext[v5];
  return epjp;
}
uint16 EprojInstr_WriteColorsToPalette(uint16 k, uint16 epjp) {  // 0x8682D5
  uint16 v3 = BW_86((uint16)(epjp + 2));
  int n = BB_86((uint16)(epjp + 4));
  uint16 v4 = BW_86(epjp);
  do {
    palette_buffer[v3 >> 1] = BW_86(v4);
    v4 += 2;
    v3 += 2;
  } while ((--n & 0x8000) == 0);
  return (uint16)(epjp + 5);
}
uint16 EprojInstr_QueueMusic(uint16 k, uint16 epjp) {  // 0x8682FD
  QueueMusic_Delayed8(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx1_Max6(uint16 k, uint16 epjp) {  // 0x868309
  QueueSfx1_Max6(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx2_Max6(uint16 k, uint16 epjp) {  // 0x868312
  QueueSfx2_Max6(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx3_Max6(uint16 k, uint16 epjp) {  // 0x86831B
  QueueSfx3_Max6(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx1_Max15(uint16 k, uint16 epjp) {  // 0x868324
  QueueSfx1_Max15(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx2_Max15(uint16 k, uint16 epjp) {  // 0x86832D
  QueueSfx2_Max15(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx3_Max15(uint16 k, uint16 epjp) {  // 0x868336
  QueueSfx3_Max15(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx1_Max3(uint16 k, uint16 epjp) {  // 0x86833F
  QueueSfx1_Max3(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx2_Max3(uint16 k, uint16 epjp) {  // 0x868348
  QueueSfx2_Max3(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx3_Max3(uint16 k, uint16 epjp) {  // 0x868351
  QueueSfx3_Max3(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx1_Max9(uint16 k, uint16 epjp) {  // 0x86835A
  QueueSfx1_Max9(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx2_Max9(uint16 k, uint16 epjp) {  // 0x868363
  QueueSfx2_Max9(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx3_Max9(uint16 k, uint16 epjp) {  // 0x86836C
  QueueSfx3_Max9(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx1_Max1(uint16 k, uint16 epjp) {  // 0x868375
  QueueSfx1_Max1(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx2_Max1(uint16 k, uint16 epjp) {  // 0x86837E
  QueueSfx2_Max1(BB_86(epjp));
  return (uint16)(epjp + 1);
}
uint16 EprojInstr_QueueSfx3_Max1(uint16 k, uint16 epjp) {  // 0x868387
  QueueSfx3_Max1(BB_86(epjp));
  return (uint16)(epjp + 1);
}
static void Eproj_Init_0x8bc2_SkreeDownRight(uint16 j) {  // 0x868ACD
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos;
  eproj_y_vel[v2] = -769;
  eproj_x_pos[v2] = v1->x_pos + 6;
  eproj_x_vel[v2] = 320;
}
static void Eproj_Init_0x8bd0_SkreeUpRight(uint16 j) {  // 0x868AF1
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos;
  eproj_y_vel[v2] = -1025;
  eproj_x_pos[v2] = v1->x_pos + 6;
  eproj_x_vel[v2] = 96;
}
static void Eproj_Init_0x8bde_SkreeDownLeft(uint16 j) {  // 0x868B15
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos;
  eproj_y_vel[v2] = -769;
  eproj_x_pos[v2] = v1->x_pos - 6;
  eproj_x_vel[v2] = -320;
}
static void Eproj_Init_0x8bec_SkreeUpLeft(uint16 j) {  // 0x868B39
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos;
  eproj_y_vel[v2] = -1025;
  eproj_x_pos[v2] = v1->x_pos - 6;
  eproj_x_vel[v2] = -96;
}
void Eproj_PreInstr_SkreeParticle(uint16 k) {  // 0x868B5D
  int v1 = k >> 1;
  
  AddToHiLo(&eproj_x_pos[v1], &eproj_x_subpos[v1], INT16_SHL8(eproj_x_vel[v1]));
  AddToHiLo(&eproj_y_pos[v1], &eproj_y_subpos[v1], INT16_SHL8(eproj_y_vel[v1]));

  eproj_y_vel[v1] += 80;
  if (CheckIfEprojIsOffScreen(k))
    eproj_id[v1] = 0;
}
static void EprojInit_DraygonsGunk(uint16 j) {  // 0x868D04
  int v1 = j >> 1;
  eproj_x_pos[v1] = eproj_spawn_x;
  eproj_y_pos[v1] = eproj_spawn_y;
  ERAM(0x97DC + 2 * (v1)) = eproj_unk1995;
  Point32 pt = ConvertAngleToXy(eproj_unk1995, eproj_init_param_1);
  eproj_x_vel[v1] = pt.x >> 16;
  eproj_E[v1] = pt.x;
  eproj_y_vel[v1] = pt.y >> 16;
  eproj_F[v1] = pt.y;
  eproj_gfx_idx[v1] = 1024;
}
void EprojPowerBombCollision(uint16 k) {  // 0x868D5C
  uint16 r18 = HIBYTE(power_bomb_explosion_radius);
  if (HIBYTE(power_bomb_explosion_radius)) {
    uint16 r20 = (uint16)(r18 + (HIBYTE(power_bomb_explosion_radius) & 1) + (power_bomb_explosion_radius >> 9)) >> 1;
    int v1 = k >> 1;
    if (abs16(power_bomb_explosion_x_pos - eproj_x_pos[v1]) < r18
        && abs16(power_bomb_explosion_y_pos - eproj_y_pos[v1]) < r20) {
      eproj_id[v1] = 0;
      samus_x_speed_divisor = 0;
    }
  }
}
void EprojPreInstr_DraygonsGunk_8E0F(uint16 k) {  // 0x868E0F
  uint16 v3;

  EprojPowerBombCollision(k);
  Eproj_FuncE73E_MoveXY(k);
  int v1 = k >> 1;
  uint16 v2 = abs16(samus_x_pos - eproj_x_pos[v1]);
  if (sign16(v2 - 16) && (v3 = abs16(samus_y_pos - eproj_y_pos[v1]), sign16(v3 - 20))) {
    eproj_instr_list_ptr[v1] = 0x8C38;
    eproj_instr_timers[v1] = 1;
  } else {
    if (Eproj_FuncE722(k))
      eproj_id[v1] = 0;
  }
}
static void EprojInit_MiniKraidSpit(uint16 j) {  // 0x869DEC

  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v3 = j >> 1;
  eproj_x_pos[v3] = ERAM(0x7804 + (uint16)(cur_enemy_index)) + v1->x_pos;
  eproj_y_pos[v3] = v1->y_pos - 16;
  eproj_y_subpos[v3] = 0;
  eproj_x_subpos[v3] = 0;
  eproj_x_vel[v3] = ERAM(0x7800 + (uint16)(cur_enemy_index));
  eproj_y_vel[v3] = ERAM(0x7802 + (uint16)(cur_enemy_index));
}
void EprojPreInit_MiniKraidSpit(uint16 k) {  // 0x869E1E
  int16 v2;

  if (EprojBlockCollisition_Horiz(k) & 1 || EprojBlockCollisition_Vertical(k) & 1) {
    eproj_id[k >> 1] = 0;
  } else {
    int v1 = k >> 1;
    v2 = eproj_y_vel[v1] + 64;
    if (v2 >= 0 && !sign16(eproj_y_vel[v1] - 960))
      v2 = 1024;
    eproj_y_vel[v1] = v2;
  }
}
static void EprojInit_MiniKraidSpikes(uint16 j, uint16 a) {  // 0x869E4E
  int v2 = j >> 1;
  eproj_x_vel[v2] = a;
  uint16 r18 = kEprojInit_MiniKraidSpikes_Tab0[ERAM(0x780C + (uint16)(cur_enemy_index)) >> 1];
  EnemyData *v3 = gEnemyData(cur_enemy_index);
  eproj_x_pos[v2] = v3->x_pos;
  eproj_y_pos[v2] = r18 + v3->y_pos;
  eproj_y_subpos[v2] = 0;
  eproj_x_subpos[v2] = 0;
  eproj_y_vel[v2] = 0;
}
static void EprojInit_MiniKraidSpikesLeft(uint16 j) {  // 0x869E46
  EprojInit_MiniKraidSpikes(j, -0x200);
}
static void EprojInit_MiniKraidSpikesRight(uint16 j) {  // 0x869E4B
  EprojInit_MiniKraidSpikes(j, 0x200);
}
void EprojPreInstr_MiniKraidSpikes(uint16 k) {  // 0x869E83
  if (EprojBlockCollisition_Horiz(k) & 1)
    eproj_id[k >> 1] = 0;
}
static void EprojInit_WalkingLavaSeahorseFireball(uint16 j) {  // 0x869EB2

  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos - 12;
  eproj_x_vel[v2] = -1024;
  eproj_x_pos[v2] = v1->x_pos - 16;
  if ((v1->ai_var_D & 0x8000) == 0) {
    eproj_x_vel[v2] = 1024;
    eproj_x_pos[v2] = v1->x_pos + 16;
  }
  eproj_y_vel[v2] = ((int16)BW_86(0x9EF9 + 2 * (eproj_init_param_1 >> 1)));
  eproj_y_subpos[v2] = 0;
  eproj_x_subpos[v2] = 0;
}
void EprojPreInstr_WalkingLavaSeahorseFireball(uint16 k) {  // 0x869EFF
  uint16 v2;
  if (EprojBlockCollisition_Vertical(k) & 1 || EprojBlockCollisition_Horiz(k) & 1) {
    eproj_id[k >> 1] = 0;
  } else {
    int v1 = k >> 1;
    if ((eproj_x_vel[v1] & 0x8000) == 0) {
      v2 = eproj_x_vel[v1] - 64;
      eproj_x_vel[v1] = v2;
      if (sign16(v2 - 512))
        v2 = 512;
    } else {
      v2 = eproj_x_vel[v1] + 64;
      eproj_x_vel[v1] = v2;
      if (!sign16(v2 + 512))
        v2 = -512;
    }
    eproj_x_vel[v1] = v2;
  }
}
static void EprojInit_PirateMotherBrainLaser(uint16 j) {  // 0x86A009
  int v2 = j >> 1;
  eproj_instr_list_ptr[v2] = eproj_spawn_r22 ? addr_word_869F7D : addr_word_869F41;
  eproj_pre_instr[v2] = 0xA05B /* nullsub_87 */;
  eproj_x_pos[v2] = eproj_spawn_x;
  eproj_y_pos[v2] = eproj_spawn_y;
  EnemyData *v3 = gEnemyData(cur_enemy_index);
  eproj_properties[v2] = BW_A0((uint16)((v3->enemy_ptr) + 6)) | 0x1000;
  eproj_E[v2] = v3->parameter_1;
  QueueSfx2_Max6(0x67);
}
uint16 EprojInstr_SetPreInstrAndRun(uint16 k, uint16 epjp) {  // 0x86A050
  eproj_pre_instr[k >> 1] = BW_86(epjp);
  return epjp;
}
void EprojPreInstr_PirateMotherBrainLaser_MoveLeft(uint16 k) {  // 0x86A05C
  int v1 = k >> 1;
  eproj_x_pos[v1] -= 2;
  if ((eproj_E[v1] & 0x8000) == 0)
    eproj_x_pos[v1] -= 2;
  if (CheckIfEprojIsOffScreen(k))
    eproj_id[v1] = 0;
}
void EprojPreInstr_PirateMotherBrainLaser_MoveRight(uint16 k) {  // 0x86A07A
  int v1 = k >> 1;
  eproj_x_pos[v1] += 2;
  if ((eproj_E[v1] & 0x8000) == 0)
    eproj_x_pos[v1] += 2;
  if (CheckIfEprojIsOffScreen(k))
    eproj_id[v1] = 0;
}
static void EprojInit_PirateClaw(uint16 j) {  // 0x86A098
  Rect16U r = eproj_spawn_rect;
  int v1 = j >> 1;
  eproj_y_pos[v1] = r.y + r.h;
  eproj_x_pos[v1] = r.x + r.w;
  eproj_instr_list_ptr[v1] = eproj_init_param_1 ? addr_off_869FE1 : addr_off_869FB9;
  eproj_pre_instr[v1] = 0xA05B /* nullsub_87 */;
  eproj_E[v1] = 2048;
  eproj_F[v1] = 1;
}
uint16 EprojInstr_A3BE(uint16 k, uint16 epjp) {  // 0x86A3BE
  int v1 = k >> 1;
  eproj_x_pos[v1] = eproj_E[v1];
  eproj_y_pos[v1] = eproj_F[v1];
  return epjp;
}
uint16 EprojInstr_GotoWithProbability25(uint16 k, uint16 epjp) {  // 0x86A456
  if ((NextRandom() & 0xC000) == 0xC000)
    return (uint16)(BW_86(epjp));
  else
    return (uint16)(epjp + 2);
}
static void EprojInit_BombTorizoLowHealthDrool(uint16 j) {  // 0x86A5D3
  int16 parameter_1; // dx
  uint16 v4, v5, v8;
  int v1 = j >> 1;
  eproj_gfx_idx[v1] = 0;
  eproj_instr_list_ptr[v1] = ((uint16)BW_86(0xA64D + 2 * ((uint16)((NextRandom() >> 1) & 0xE) >> 1)));
  NextRandom();
  EnemyData *v2 = gEnemyData(cur_enemy_index);
  eproj_y_pos[v1] = v2->y_pos - 5;
  parameter_1 = v2->parameter_1;
  if ((parameter_1 & 0x4000) != 0) {
    v4 = random_number & 0x1FE;
  } else {
    if (parameter_1 < 0)
      v5 = 32;
    else
      v5 = 224;
    v4 = 2 * (v5 + (random_number & 0xF) - 8);
  }
  int v6 = v4 >> 1;
  eproj_x_vel[v1] = kSinCosTable8bit_Sext[v6 + 64];
  eproj_y_vel[v1] = kSinCosTable8bit_Sext[v6];
  EnemyData *v7 = gEnemyData(cur_enemy_index);
  if ((v7->parameter_1 & 0x8000) != 0)
    v8 = v7->x_pos + 8;
  else
    v8 = v7->x_pos - 8;
  eproj_x_pos[v1] = v8;
}
static void EprojInit_BombTorizoLowHealthInitialDrool(uint16 j) {  // 0x86A65D
  int16 v3;
  int16 parameter_1; // dx

  int v1 = j >> 1;
  eproj_gfx_idx[v1] = 0;
  NextRandom();
  EnemyData *v2 = gEnemyData(cur_enemy_index);
  eproj_y_pos[v1] = v2->y_pos + (random_number & 3) - 5;
  eproj_y_vel[v1] = (random_number & 0x1F) + 48;
  NextRandom();
  v3 = random_number & 3;
  parameter_1 = v2->parameter_1;
  if ((parameter_1 & 0x4000) != 0) {
    eproj_x_pos[v1] = v2->x_pos + v3;
    eproj_x_vel[v1] = 0;
  } else {
    if (parameter_1 < 0)
      eproj_x_pos[v1] = v2->x_pos + v3 + 8;
    else
      eproj_x_pos[v1] = v2->x_pos + v3 - 8;
    eproj_x_vel[v1] = 0;
  }
}
static void EprojInit_BombTorizoExplosiveSwipe(uint16 j) {  // 0x86A6F6
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  if ((v1->parameter_1 & 0x8000) != 0) {
    int v4 = eproj_init_param_1 >> 1;
    int v5 = j >> 1;
    eproj_x_pos[v5] = v1->x_pos - ((int16)BW_86(0xA738 + 2 * (v4)));
    eproj_y_pos[v5] = v1->y_pos + ((int16)BW_86(0xA74E + 2 * (v4)));
  } else {
    int v2 = eproj_init_param_1 >> 1;
    int v3 = j >> 1;
    eproj_x_pos[v3] = v1->x_pos + ((int16)BW_86(0xA738 + 2 * (v2)));
    eproj_y_pos[v3] = v1->y_pos + ((int16)BW_86(0xA74E + 2 * (v2)));
  }
}
static void EprojInit_BombTorizoStatueBreaking(uint16 j) {  // 0x86A764
  int8 v1;

  CalculatePlmBlockCoords(plm_id);
  v1 = eproj_init_param_1;
  int v2 = eproj_init_param_1 >> 1;
  int v3 = j >> 1;
  eproj_instr_list_ptr[v3] = ((uint16)BW_86(0xA7AB + 2 * (v2)));
  eproj_x_pos[v3] = ((int16)BW_86(0xA7CB + 2 * (v2))) + 16 * plm_x_block;
  int v4 = (v1 & 0xF) >> 1;
  eproj_y_pos[v3] = ((int16)BW_86(0xA7EB + 2 * (v4))) + 16 * plm_y_block;
  eproj_y_vel[v3] = kEprojInit_BombTorizoStatueBreaking_Yvel[v4];
  eproj_F[v3] = kEprojInit_BombTorizoStatueBreaking_F[v4];
  eproj_properties[v3] |= 0x1000;
}
static void EprojInit_BombTorizoLowHealthExplode(uint16 j) {  // 0x86A81B
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_x_pos[v2] = v1->x_pos;
  eproj_y_pos[v2] = v1->y_pos;
  if ((v1->parameter_1 & 0x8000) == 0)
    eproj_init_param_1 += 2;
  eproj_init_param_1 += 2;
  int v3 = eproj_init_param_1 >> 1;
  uint16 v4 = ((int16)BW_86(0xA859 + 2 * (v3))) + eproj_x_pos[v2];
  eproj_x_pos[v2] = v4;
  eproj_E[v2] = v4;
  uint16 v5 = ((int16)BW_86(0xA865 + 2 * (v3))) + eproj_y_pos[v2];
  eproj_y_pos[v2] = v5;
  eproj_F[v2] = v5;
}
static void EprojInit_BombTorizoDeathExplosion(uint16 j) {  // 0x86A871
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  uint16 x_pos = v1->x_pos;
  int v3 = j >> 1;
  eproj_x_pos[v3] = x_pos;
  eproj_E[v3] = x_pos;
  uint16 y_pos = v1->y_pos;
  eproj_y_pos[v3] = y_pos;
  eproj_F[v3] = y_pos;
}
void sub_86A887(uint16 v0) {  // 0x86A887
  int16 v2;
  int16 v3;
  int16 v4;

  if (EprojBlockCollisition_Horiz(v0) & 1) {
    int v6 = v0 >> 1;
    eproj_instr_list_ptr[v6] = 0xA48A;
    eproj_instr_timers[v6] = 1;
  } else {
    int v1 = v0 >> 1;
    v2 = eproj_x_vel[v1];
    if (v2 >= 0) {
      v4 = v2 - 4;
      if (v4 < 0)
        v4 = 3;
      eproj_x_vel[v1] = v4;
    } else {
      v3 = v2 + 4;
      if (v3 >= 0)
        v3 = 3;
      eproj_x_vel[v1] = v3;
    }
    uint8 carry = EprojBlockCollisition_Vertical(v0);
    if ((eproj_y_vel[v1] & 0x8000) != 0 || !carry) {
      uint16 v5 = eproj_y_vel[v1] + 16;
      eproj_y_vel[v1] = v5;
      if ((v5 & 0xF000) == 4096)
        eproj_id[v1] = 0;
    } else {
      int v7 = v0 >> 1;
      eproj_y_pos[v7] -= 3;
      eproj_instr_list_ptr[v7] = 0xA48E;
      eproj_instr_timers[v7] = 1;
    }
  }
}
void EprojPreInstr_A977(uint16 k) {  // 0x86A8EF
  uint8 carry = EprojBlockCollisition_Vertical(k);
  int v1 = k >> 1;
  if ((eproj_y_vel[v1] & 0x8000) != 0 || !carry) {
    uint16 v2 = eproj_F[0] + eproj_y_vel[v1];
    eproj_y_vel[v1] = v2;
    if ((v2 & 0xF000) == 4096)
      eproj_y_vel[v1] = 4096;
  } else {
    eproj_pre_instr[v1] = 0xA918;
  }
}
uint16 EprojInstr_SpawnEnemyDrops(uint16 k, uint16 epjp) {  // 0x86AB8A
  int v2 = k >> 1;
  eproj_spawn_x = eproj_x_pos[v2]; eproj_spawn_y = eproj_y_pos[v2];
  SpawnEnemyDrops(area_index ? BW_86((uint16)(epjp + 2)) : BW_86(epjp), k, 0);
  return (uint16)(epjp + 4);
}
static void Eproj_InitXYVelRandom(uint16 j, uint16 k, Point16U pt) {  // 0x86ABAE
  uint16 v2 = (k);
  int v3 = j >> 1;
  eproj_instr_list_ptr[v3] = BW_86(v2);
  eproj_x_pos[v3] = BW_86(v2 + 2 * (1)) + pt.x;
  eproj_x_vel[v3] = BW_86(v2 + 2 * (2)) + (uint8)NextRandom() - 128;
  eproj_y_pos[v3] = BW_86(v2 + 2 * (3)) + pt.y;
  eproj_y_vel[v3] = BW_86(v2 + 2 * (4)) + (uint8)NextRandom() - 128;
}
static void EprojInit_BombTorizosChozoOrbs(uint16 j) {  // 0x86ABEB
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 v2 = ((E->parameter_1 & 0x8000) != 0) ? addr_kEprojInit_BombTorizosChozoOrbs_init0 : addr_kEprojInit_BombTorizosChozoOrbs_init1;
  Eproj_InitXYVelRandom(j, v2, (Point16U) { E->x_pos, E->y_pos });
}
static void EprojInit_GoldenTorizosChozoOrbs(uint16 j) {  // 0x86AC7C
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 v2 = ((E->parameter_1 & 0x8000) != 0) ? addr_kEprojInit_GoldenTorizosChozoOrbs_init0 : addr_kEprojInit_GoldenTorizosChozoOrbs_init1;
  Eproj_InitXYVelRandom(j, v2, (Point16U) { E->x_pos, E->y_pos });
}
void EprojPreInstr_BombTorizosChozoOrbs(uint16 k) {  // 0x86ACAD
  if (EprojBlockCollisition_Horiz(k)) {
    int v3 = k >> 1;
    eproj_instr_list_ptr[v3] = addr_off_86AB25;
    eproj_instr_timers[v3] = 1;
  } else {
    uint8 carry = EprojBlockCollisition_Vertical(k);
    int v1 = k >> 1;
    if ((eproj_y_vel[v1] & 0x8000) != 0 || !carry) {
      uint16 v2 = eproj_y_vel[v1] + 18;
      eproj_y_vel[v1] = v2;
      if ((v2 & 0xF000) == 4096)
        eproj_id[v1] = 0;
    } else {
      int v4 = k >> 1;
      eproj_y_pos[v4] = (eproj_y_pos[v4] & 0xFFF0 | 8) - 2;
      eproj_instr_list_ptr[v4] = addr_off_86AB41;
      eproj_instr_timers[v4] = 1;
    }
  }
}
void EprojPreInstr_GoldenTorizosChozoOrbs(uint16 k) {  // 0x86ACFA
  int v1;
  int16 v2;
  uint16 v3;
  uint16 v4;

  if (EprojBlockCollisition_Horiz(k) & 1)
    eproj_x_vel[k >> 1] = -eproj_x_vel[k >> 1];
  if (EprojBlockCollisition_Vertical(k) & 1
      && (v1 = k >> 1, (eproj_y_vel[v1] & 0x8000) == 0)
      && ((v2 = eproj_x_vel[v1], v2 >= 0) ? (v3 = v2 - 64) : (v3 = v2 + 64),
          eproj_x_vel[v1] = v3,
          v4 = -(eproj_y_vel[v1] >> 1),
          eproj_y_vel[v1] = v4,
          (v4 & 0xFF80) == 0xFF80)) {
    int v5 = k >> 1;
    eproj_y_pos[v5] = (eproj_y_pos[v5] & 0xFFF0 | 8) - 2;
    eproj_instr_list_ptr[v5] = addr_off_86AB41;
    eproj_instr_timers[v5] = 1;
  } else {
    eproj_y_vel[k >> 1] += 24;
  }
}
static void EprojInit_TorizoSonicBoom(uint16 j) {  // 0x86AE15
  int v1 = ((NextRandom() & 1) != 0) ? -12 : 20;
  EnemyData *v2 = gEnemyData(cur_enemy_index);
  int v3 = j >> 1;
  eproj_y_pos[v3] = v2->y_pos + v1;
  eproj_y_vel[v3] = 0;
  if ((v2->parameter_1 & 0x8000) != 0) {
    eproj_x_pos[v3] = v2->x_pos + 32;
    eproj_x_vel[v3] = 624;
    eproj_instr_list_ptr[v3] = addr_off_86ADD2;
  } else {
    eproj_x_pos[v3] = v2->x_pos - 32;
    eproj_x_vel[v3] = -624;
    eproj_instr_list_ptr[v3] = addr_off_86ADBF;
  }
}
void EprojPreInstr_TorizoSonicBoom(uint16 k) {  // 0x86AE6C
  if (EprojBlockCollisition_Horiz(k) & 1) {
    int v4 = k >> 1;
    eproj_instr_list_ptr[v4] = addr_off_86ADE5;
    eproj_instr_timers[v4] = 1;
    eproj_E[v4] = eproj_x_pos[v4];
    eproj_F[v4] = eproj_y_pos[v4];
  } else {
    int v1 = k >> 1;
    eproj_x_vel[v1] += sign16(eproj_x_vel[v1]) ? -16 : 16;
    if ((eproj_x_vel[v1] & 0xF000) == 4096)
      eproj_id[v1] = 0;
  }
}
static void EprojInit_TourianStatueDustClouds(uint16 j) {  // 0x86AF43
  int v1 = j >> 1;
  eproj_E[v1] = 128;
  eproj_F[v1] = 188;
}
static void EprojInit_TourianLandingDustCloudsRightFoot(uint16 j) {  // 0x86AF50
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos + 48;
  eproj_x_pos[v2] = v1->x_pos + 24;
}
uint16 EprojInstr_MoveY_Minus4(uint16 k, uint16 epjp) {  // 0x86AF92
  eproj_y_pos[k >> 1] -= 4;
  return epjp;
}
static void EprojInit_TorizoLandingDustCloudLeftFoot(uint16 j) {  // 0x86AFCD
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_y_pos[v2] = v1->y_pos + 48;
  eproj_x_pos[v2] = v1->x_pos - 24;
}
static void EprojInit_GoldenTorizoEgg(uint16 j) {  // 0x86B001
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_F[v2] = (0xe2 & 0x1F) + 64;  //  bug
  eproj_E[v2] = v1->parameter_1;
  uint16 v4 = sign16(v1->parameter_1) ? addr_kEprojInit_GoldenTorizoEgg0 : addr_kEprojInit_GoldenTorizoEgg1;
  Eproj_InitXYVelRandom(j, v4, (Point16U) { v1->x_pos, v1->y_pos });
}
void EprojPreInstr_GoldenTorizoEgg(uint16 k) {  // 0x86B043
  int v1 = k >> 1;
  if ((--eproj_F[v1] & 0x8000) != 0) {
    eproj_instr_list_ptr[v1] += 2;
    eproj_instr_timers[v1] = 1;
    eproj_x_vel[v1] = ((eproj_E[v1] & 0x8000) != 0) ? 256 : -256;
  } else {
    if (EprojBlockCollisition_Horiz(k) & 1) {
      eproj_x_vel[v1] = -eproj_x_vel[v1];
      eproj_E[v1] ^= 0x8000;
    }
    if (EprojBlockCollisition_Vertical(k) & 1 && (eproj_y_vel[v1] & 0x8000) == 0) {
      eproj_x_vel[v1] += sign16(eproj_x_vel[v1]) ? 32 : -32;
      eproj_y_vel[v1] = -eproj_y_vel[v1];
    }
    eproj_y_vel[v1] += 48;
    if ((eproj_y_vel[v1] & 0xF000) == 4096)
      eproj_id[v1] = 0;
  }
}
static void EprojInit_GoldenTorizoSuperMissile(uint16 j) {  // 0x86B1CE
  int v2 = j >> 1;
  eproj_E[v2] = cur_enemy_index;
  EnemyData *v3 = gEnemyData(cur_enemy_index);
  int v4 = ((v3->parameter_1 & 0x8000) != 0) ? 1 : 0;
  eproj_x_pos[v2] = v3->x_pos + ((int16)BW_86(0xB205 + 2 * (v4)));
  eproj_y_pos[v2] = v3->y_pos - 52;
  eproj_instr_list_ptr[v2] = ((uint16)BW_86(0xB209 + 2 * (v4)));
}
void EprojPreInstr_GoldenTorizoSuperMissile(uint16 k) {  // 0x86B20D
  int v1 = k >> 1;
  EnemyData *v2 = gEnemyData(eproj_E[v1]);
  int v3 = ((v2->parameter_1 & 0x8000) != 0) ? 32 : -32;
  eproj_x_pos[v1] = v2->x_pos + v3;
  eproj_y_pos[v1] = v2->y_pos - 52;
}
static void sub_86B279(uint16 k, uint16 a) {  // 0x86B279
  int v3 = k >> 1;
  eproj_x_vel[v3] = 4 * kSinCosTable8bit_Sext[a + 64];
  eproj_y_vel[v3] = 4 * kSinCosTable8bit_Sext[a];
}
uint16 EprojInstr_SetVelTowardsSamus1(uint16 k, uint16 epjp) {  // 0x86B269
  sub_86B279(k, CalculateAngleOfSamusFromEproj(k) & 0x7F);
  return epjp;
}
static void EprojInit_GoldenTorizoEyeBeam(uint16 j) {  // 0x86B328
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 v2 = ((E->parameter_1 & 0x8000) != 0) ? addr_stru_86B376 : addr_stru_86B380;
  Eproj_InitXYVelRandom(j, v2, (Point16U) { E->x_pos, E->y_pos });
  uint16 v3 = (NextRandom() & 0x1E) - 16 + 192;
  if ((E->parameter_1 & 0x8000) == 0)
    v3 += 128;
  int v4 = v3 >> 1;
  int v5 = j >> 1;
  eproj_x_vel[v5] = 8 * kSinCosTable8bit_Sext[v4 + 64];
  eproj_y_vel[v5] = 8 * kSinCosTable8bit_Sext[v4];
}
void EprojPreInstr_GoldenTorizoEyeBeam(uint16 k) {  // 0x86B38A
  if (EprojBlockCollisition_Horiz(k) & 1) {
    eproj_instr_list_ptr[k >> 1] = addr_off_86B3CD;
    eproj_instr_timers[k >> 1] = 1;
    return;
  }
  if (EprojBlockCollisition_Vertical(k) & 1) {
    int v1 = k >> 1;
    eproj_y_pos[v1] = (eproj_y_pos[v1] & 0xFFF0 | 8) - 2;
    eproj_instr_list_ptr[v1] = addr_off_86B3E5;
    eproj_instr_timers[v1] = 1;
    return;
  }
}
static void EprojInit_TourianStatueUnlockingParticleWaterSplash(uint16 j) {  // 0x86B87A
  int v1 = j >> 1;
  eproj_x_pos[v1] = eproj_x_pos[eproj_init_param_1 >> 1];
  eproj_y_pos[v1] = fx_y_pos - 4;
}
static void EprojInit_TourianStatueEyeGlow(uint16 j) {  // 0x86B88E
  uint16 v1 = eproj_init_param_1;
  int v2 = eproj_init_param_1 >> 1;
  int v3 = j >> 1;
  eproj_x_pos[v3] = ((uint16)BW_86(0xB90E + 2 * (v2)));
  eproj_y_pos[v3] = ((uint16)BW_86(0xB916 + 2 * (v2)));
  uint16 v4 = 4 * v1;
  for (int i = 498; i != 506; i += 2) {
    palette_buffer[i >> 1] = ((uint16)BW_86(0xB91E + 2 * (v4 >> 1)));
    v4 += 2;
  }
}
static void EprojInit_TourianStatueUnlockingParticle(uint16 j) {  // 0x86B8B5
  int v1 = eproj_init_param_1 >> 1;
  int v2 = j >> 1;
  eproj_x_pos[v2] = eproj_x_pos[v1];
  eproj_y_pos[v2] = eproj_y_pos[v1];
  uint16 v3 = 2 * (uint8)((NextRandom() & 0x3F) - 32);
  eproj_E[v2] = v3;
  int v4 = v3 >> 1;
  eproj_x_vel[v2] = kSinCosTable8bit_Sext[v4 + 64];
  eproj_y_vel[v2] = 4 * kSinCosTable8bit_Sext[v4];
}
static void EprojInit_TourianStatueSoul(uint16 j) {  // 0x86B8F8
  int v1 = eproj_init_param_1 >> 1;
  int v2 = j >> 1;
  eproj_x_pos[v2] = ((uint16)BW_86(0xB90E + 2 * (v1)));
  eproj_y_pos[v2] = ((uint16)BW_86(0xB916 + 2 * (v1)));
  eproj_y_vel[v2] = -1024;
}
static void EprojInit_TourianStatueBaseDecoration(uint16 j) {  // 0x86B93E
  int v1 = j >> 1;
  eproj_E[v1] = 120;
  eproj_x_pos[v1] = 120;
  eproj_F[v1] = 184;
  eproj_y_pos[v1] = 184;
}
static void EprojInit_TourianStatueRidley(uint16 j) {  // 0x86B951
  int v1 = j >> 1;
  eproj_E[v1] = 142;
  eproj_x_pos[v1] = 142;
  eproj_F[v1] = 85;
  eproj_y_pos[v1] = 85;
}
static void EprojInit_TourianStatuePhantoon(uint16 j) {  // 0x86B964
  int v1 = j >> 1;
  eproj_E[v1] = 132;
  eproj_x_pos[v1] = 132;
  eproj_F[v1] = 136;
  eproj_y_pos[v1] = 136;
}
static void EprojPreInstr_BA42(uint16 k) {  // 0x86BA42
  int v1 = k >> 1;
  eproj_x_pos[v1] = eproj_E[v1];
  eproj_y_pos[v1] = eproj_F[v1] + layer1_y_pos - statue_bg2_ypos;
}
void EprojPreInstr_TourianStatueStuff(uint16 k) {  // 0x86BA37
  if (!tourian_entrance_statue_animstate)
    tourian_entrance_statue_finished |= 0x8000;
  EprojPreInstr_BA42(k);
}
static void EprojInit_MotherBrainDeathBeemFired(uint16 j) {  // 0x86C684
  int v1 = j >> 1;

  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  eproj_x_pos[v1] = ERAM(0x8008 + E_k);
  eproj_x_subpos[v1] = ERAM(0x8006 + E_k);
  eproj_y_pos[v1] = ERAM(0x800C + E_k);
  eproj_y_subpos[v1] = ERAM(0x800A + E_k);
  eproj_x_vel[v1] = ERAM(0x800E + E_k);
  eproj_y_vel[v1] = ERAM(0x8010 + E_k);
  MoveEprojWithVelocity(j);
  ERAM(0x8008 + E_k) = eproj_x_pos[v1];
  ERAM(0x8006 + E_k) = eproj_x_subpos[v1];
  ERAM(0x800C + E_k) = eproj_y_pos[v1];
  ERAM(0x800A + E_k) = eproj_y_subpos[v1];
  uint16 r18 = (uint8)(LOBYTE(ERAM(0x8012 + E_k)) + NextRandom());
  uint16 rv = NextRandom();
  eproj_x_vel[v1] = Math_MultBySin(rv & 0x700, r18);
  eproj_y_vel[v1] = Math_MultByCos(random_number & 0x700, r18);
  MoveEprojWithVelocity(j);
  if (sign16(eproj_y_pos[v1] - 34)
      || !sign16(eproj_y_pos[v1] - 206)
      || sign16(eproj_x_pos[v1] - 2)
      || !sign16(eproj_x_pos[v1] - 238)) {
    eproj_id[v1] = 0;
    eproj_spawn_x = eproj_x_pos[v1]; eproj_spawn_y = eproj_y_pos[v1];
    SpawnEprojWithRoomGfx(addr_kEproj_DustCloudExplosion, 0x1D);
    QueueSfx3_Max6(0x13);
    earthquake_timer = 10;
    earthquake_type = 5;
  } else {
    eproj_E[v1] = (eproj_E[v1] + 1) & 3;
    eproj_F[v1] = 0;
    eproj_x_vel[v1] = 0;
    eproj_y_vel[v1] = 0;
  }
}
void SpawnMotherBrainDeathBeam(uint16 x) {  // 0x86C7FB
  SpawnEprojWithRoomGfx(addr_kEproj_MotherBrainDeathBeamFired, eproj_E[x >> 1]);
}
static void EprojInit_KiHunterAcidSpitCommon(uint16 k, uint16 j) {  // 0x86CFBA
  int v2 = j >> 1;
  eproj_y_vel[v2] = 0;
  eproj_y_pos[v2] = gEnemyData(k)->y_pos - 16;
  eproj_y_subpos[v2] = 0;
  eproj_x_subpos[v2] = 0;
}
static void EprojInit_KiHunterAcidSpitLeft(uint16 j) {  // 0x86CF90
  int v2 = j >> 1;
  eproj_x_vel[v2] = -768;
  eproj_x_pos[v2] = gEnemyData(cur_enemy_index)->x_pos - 22;
  EprojInit_KiHunterAcidSpitCommon(cur_enemy_index, j);
}
static void EprojInit_KiHunterAcidSpitRight(uint16 j) {  // 0x86CFA6
  int v2 = j >> 1;
  eproj_x_vel[v2] = 768;
  eproj_x_pos[v2] = gEnemyData(cur_enemy_index)->x_pos + 22;
  EprojInit_KiHunterAcidSpitCommon(cur_enemy_index, j);
}
void sub_86CFD5(uint16 k) {  // 0x86CFD5
  int v1 = k >> 1;
  eproj_pre_instr[v1] = 0xCFF8 /* sub_86CFF8 */;
  eproj_x_pos[v1] -= 19;
}
void sub_86CFE6(uint16 k) {  // 0x86CFE6
  int v1 = k >> 1;
  eproj_pre_instr[v1] = 0xCFF8 /* sub_86CFF8 */;
  eproj_x_pos[v1] += 19;
}
void sub_86CFF8(uint16 k) {  // 0x86CFF8
  int v1 = k >> 1;
  if (EprojBlockCollisition_Vertical(k) & 1) {
    eproj_instr_list_ptr[v1] = addr_off_86CF56;
    eproj_instr_timers[v1] = 1;
  } else if (EprojBlockCollisition_Horiz(k) & 1) {
    eproj_x_vel[k >> 1] = 0;
  } else {
    uint16 v2 = eproj_y_vel[v1] + 16;
    eproj_y_vel[v1] = v2;
    if (!sign16(v2 - 512))
      v2 = 512;
    eproj_y_vel[v1] = v2;
  }
}
static void EprojInit_KagosBugs(uint16 j) {  // 0x86D088
  int v2 = j >> 1;
  eproj_F[v2] = cur_enemy_index;
  EnemyData *v3 = gEnemyData(cur_enemy_index);
  eproj_x_pos[v2] = v3->x_pos;
  eproj_y_pos[v2] = v3->y_pos;
  uint16 v4 = (random_number & 7) + 1;
  eproj_G[v2] = v4;
  eproj_E[v2] = v4 + 4;
  eproj_pre_instr[v2] = 0xD0CA /* EprojPreInstr_KagosBugs */;
}
void EprojPreInstr_KagosBugs_Func1(uint16 k) {  // 0x86D0B3
  int v1 = k >> 1;
  uint16 v2 = eproj_E[v1];
  if (v2) {
    uint16 v3 = v2 - 1;
    eproj_E[v1] = v3;
    if (!v3)
      QueueSfx2_Max6(0x6C);
  }
}
void EprojPreInstr_KagosBugs_Func2(uint16 k) {  // 0x86D1E4
  int v1 = k >> 1;
  EnemyData *v2 = gEnemyData(eproj_F[v1]);
  uint16 v3 = abs16(v2->x_pos - eproj_x_pos[v1]);
  if (!sign16(v3 - 23))
    eproj_properties[v1] |= 0x8000;
}
void EprojPreInstr_KagosBugs(uint16 k) {  // 0x86D0CA
  EprojPreInstr_KagosBugs_Func1(k);
  EprojPreInstr_KagosBugs_Func2(k);
  int v1 = k >> 1;
  if (eproj_G[v1]) {
    --eproj_G[v1];
  } else {
    eproj_instr_list_ptr[v1] = addr_word_86D052;
    eproj_instr_timers[v1] = 1;
    eproj_pre_instr[v1] = 0xD0EB /* nullsub_302 */;
  }
}
void EprojPreInstr_D0EC(uint16 k) {  // 0x86D0EC
  EprojPreInstr_KagosBugs_Func1(k);
  EprojPreInstr_KagosBugs_Func2(k);
  if (EprojBlockCollisition_Horiz(k)) {
    eproj_x_vel[k >> 1] = 0;
    goto LABEL_6;
  }
  if (EprojBlockCollisition_Vertical(k)) {
LABEL_6:
    eproj_y_vel[k >> 1] = 256;
LABEL_7:;
    eproj_pre_instr[k >> 1] = 0xD128 /* EprojPreInstr_D128 */;
    eproj_instr_list_ptr[k >> 1] = addr_word_86D04A;
    eproj_instr_timers[k >> 1] = 1;
    return;
  }
  uint16 v2 = eproj_y_vel[k >> 1];
  bool v3 = (int16)(((uint16)BW_86(0xD082 + 2 * (0))) + v2) < 0;
  eproj_y_vel[k >> 1] = ((uint16)BW_86(0xD082 + 2 * (0))) + v2;
  if (!v3) {
    goto LABEL_7;
  }
}
void EprojPreInstr_D128(uint16 v0) {  // 0x86D128
  EprojPreInstr_KagosBugs_Func1(v0);
  EprojPreInstr_KagosBugs_Func2(v0);
  if (EprojBlockCollisition_Horiz(v0) & 1) {
    eproj_x_vel[v0 >> 1] = 0;
  } else if (EprojBlockCollisition_Vertical(v0) & 1) {
    int v1 = v0 >> 1;
    eproj_pre_instr[v1] = 0xD0EB /* nullsub_302 */;
    eproj_instr_list_ptr[v1] = addr_word_86D03C;
    eproj_instr_timers[v1] = 1;
  } else {
    eproj_y_vel[v0 >> 1] += ((uint16)BW_86(0xD082 + 2 * (0)));
  }
}
uint16 EprojInstr_D15C(uint16 k, uint16 epjp) {  // 0x86D15C
  EprojPreInstr_KagosBugs_Func1(k);
  EprojPreInstr_KagosBugs_Func2(k);
  int v2 = k >> 1;

  uint16 t = (random_number & 0x300) + 2048;
  eproj_y_vel[v2] = -t;
  EnemyData *v3 = gEnemyData(eproj_F[v2]);
  uint16 t2 = v3->x_pos - eproj_x_pos[v2];

  if ((int16)(abs16(t2) - ((uint16)BW_86(0xD084 + 2 * (0)))) >= 0)
    t = sign16(t2) ? -1 : 0;

  uint16 v4 = ((uint16)BW_86(0xD086 + 2 * (0)));
  if (t & 0x100)
    v4 = -v4;
  eproj_x_vel[v2] = v4;
  eproj_pre_instr[v2] = 0xD0EC /* EprojPreInstr_D0EC */;
  return epjp;
}
uint16 EprojInstr_D1B6(uint16 k, uint16 epjp) {  // 0x86D1B6
  int v1 = k >> 1;
  eproj_G[v1] = (random_number & 0x1F) + 1;
  eproj_pre_instr[v1] = 0xD0CA /* EprojPreInstr_KagosBugs */;
  return epjp;
}
void sub_86D992(uint16 v0) {  // 0x86D992
  uint16 v1 = eproj_init_param_1;
  int v2 = v0 >> 1;
  eproj_E[v2] = eproj_init_param_1;
  eproj_instr_list_ptr[v2] = ((uint16)BW_86(0xD96A + 2 * (v1 >> 1)));
  EnemyData *v3 = gEnemyData(cur_enemy_index);
  eproj_x_pos[v2] = v3->x_pos;
  eproj_x_subpos[v2] = v3->x_subpos;
  eproj_y_pos[v2] = v3->y_pos;
  eproj_y_subpos[v2] = v3->y_subpos;
  eproj_y_vel[v2] = -512;
  eproj_x_vel[v2] = 512;
  if (!sign16(eproj_init_param_1 - 12)) {
    eproj_y_vel[v2] = -384;
    eproj_x_vel[v2] = 384;
  }
}
void EprojPreInstr_SpikeShootingPlantSpikes_MoveY1(uint16 k) {  // 0x86D9E6
  int v1 = k >> 1;
  AddToHiLo(&eproj_y_pos[v1], &eproj_y_subpos[v1], INT16_SHL8(eproj_y_vel[v1]));
}
void EprojPreInstr_SpikeShootingPlantSpikes_MoveY2(uint16 k) {  // 0x86DA10
  int v1 = k >> 1;
  AddToHiLo(&eproj_y_pos[v1], &eproj_y_subpos[v1], INT16_SHL8(eproj_x_vel[v1]));
}
void EprojPreInstr_SpikeShootingPlantSpikes_MoveX1(uint16 k) {  // 0x86DA3A
  int v1 = k >> 1;
  AddToHiLo(&eproj_x_pos[v1], &eproj_x_subpos[v1], INT16_SHL8(eproj_y_vel[v1]));
}
void EprojPreInstr_SpikeShootingPlantSpikes_MoveX2(uint16 k) {  // 0x86DA64
  int v1 = k >> 1;
  AddToHiLo(&eproj_x_pos[v1], &eproj_x_subpos[v1], INT16_SHL8(eproj_x_vel[v1]));
}
void EprojPreInstr_SpikeShootingPlantSpikes_0_MoveX1(uint16 k) {  // 0x86DA8E
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX1(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_2_MoveX2(uint16 k) {  // 0x86DA93
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX2(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_1_MoveY1(uint16 k) {  // 0x86DA98
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY1(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_4_MoveY2(uint16 k) {  // 0x86DA9D
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY2(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_6_MoveX1Y1(uint16 k) {  // 0x86DAA2
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX1(k);
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY1(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_8_MoveX1Y2(uint16 k) {  // 0x86DAAA
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX1(k);
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY2(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_7_MoveX2Y1(uint16 k) {  // 0x86DAB2
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX2(k);
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY1(k);
}
void EprojPreInstr_SpikeShootingPlantSpikes_9_MoveX2Y2(uint16 k) {  // 0x86DABA
  EprojPreInstr_SpikeShootingPlantSpikes_MoveX2(k);
  EprojPreInstr_SpikeShootingPlantSpikes_MoveY2(k);
}
void EprojPreInstrHelper_SpikeShootingPlantSpikes_Func1(uint16 k) {  // 0x86DAC2
  if (EprojPreInstrHelper_SpikeShootingPlantSpikes_Func2(k))
    eproj_id[k >> 1] = 0;
}
void EprojPreInstr_SpikeShootingPlantSpikes(uint16 k) {
  kEprojPreInstr_SpikeShootingPlantSpikes[eproj_E[k >> 1] >> 1](k);
  EprojPreInstrHelper_SpikeShootingPlantSpikes_Func1(k);
}
uint16 EprojPreInstrHelper_SpikeShootingPlantSpikes_Func2(uint16 k) {  // 0x86DACE
  int v1 = k >> 1;
  return (int16)(eproj_x_pos[v1] - layer1_x_pos) < 0
    || (int16)(layer1_x_pos + 256 - eproj_x_pos[v1]) < 0
    || (int16)(eproj_y_pos[v1] - layer1_y_pos) < 0
    || (int16)(layer1_y_pos + 256 - eproj_y_pos[v1]) < 0;
}
uint16 EprojInstr_DC5A(uint16 k, uint16 epjp) {  // 0x86DC5A
  eproj_properties[k >> 1] = 12288;
  return epjp;
}
uint16 EprojInstr_SpawnEnemyDrops_0(uint16 k, uint16 epjp) {  // 0x86DC61
  int v2 = k >> 1;
  eproj_spawn_x = eproj_x_pos[v2]; eproj_spawn_y = eproj_y_pos[v2];
  SpawnEnemyDrops(addr_kEnemyDef_DF7F, k, 0);
  return epjp;
}
uint16 EprojInstr_SpawnSporesEproj(uint16 k, uint16 epjp) {  // 0x86DC77
  int v2 = k >> 1;
  eproj_spawn_x = eproj_x_pos[v2]; eproj_spawn_y = eproj_y_pos[v2];
  SpawnEprojWithRoomGfx(addr_kEproj_Spores, 0);
  return epjp;
}
static void EprojInit_Spores(uint16 j) {  // 0x86DC8D
  int v2 = j >> 1;
  eproj_F[v2] = eproj_x_pos[v2] = eproj_spawn_x;
  eproj_y_pos[v2] = eproj_spawn_y;
  eproj_gfx_idx[v2] = 512;
}
static void EprojInit_SporeSpawnStalk(uint16 j) {  // 0x86DCA3
  int v1 = j >> 1;
  eproj_y_pos[v1] = enemy_data[0].y_pos + ((int16)BW_86(0xDCB9 + 2 * (eproj_init_param_1)));
  eproj_x_pos[v1] = enemy_data[0].x_pos;
}
static void EprojInit_SporeSpawners(uint16 j) {  // 0x86DCD4
  int v1 = j >> 1;
  eproj_x_pos[v1] = kEprojInit_SporeSpawners_X[eproj_init_param_1];
  eproj_y_pos[v1] = 520;
}
void EprojPreInstr_Spores(uint16 k) {  // 0x86DCEE
  int v1 = k >> 1;
  uint16 v2 = LOBYTE(eproj_E[v1]);
  uint16 r18 = SignExtend8(((uint8)BB_86(0xDD6C + (LOBYTE(eproj_E[v1])))));
  if ((eproj_F[v1] & 0x80) != 0)
    r18 = -r18;
  eproj_x_pos[v1] += r18;
  r18 = SignExtend8(((uint8)BB_86(0xDD6C + (v2 + 1))));
  uint16 v3 = r18 + eproj_y_pos[v1] + r18;
  eproj_y_pos[v1] = v3;
  if (!sign16(v3 - 768))
    eproj_id[v1] = 0;
  eproj_E[v1] = (uint8)(LOBYTE(eproj_E[v1]) + 2);
}
void EprojPreInstr_SporeSpawners(uint16 k) {  // 0x86DD46
  if (!kraid_unk9000) {
    int v1 = k >> 1;
    if (!eproj_F[v1]) {
      eproj_instr_list_ptr[v1] = addr_word_86DC06;
      eproj_instr_timers[v1] = 1;
      eproj_F[v1] = NextRandom() & 0x1FF;
    }
    --eproj_F[v1];
  }
}
static void EprojInit_DustCloudOrExplosion(uint16 v0) {  // 0x86E468
  int v1 = v0 >> 1;
  eproj_instr_list_ptr[v1] = ((uint16)BW_86(0xE42C + 2 * (eproj_init_param_1)));
  eproj_x_pos[v1] = eproj_spawn_x;
  eproj_y_pos[v1] = eproj_spawn_y;
}
void EprojPreInstr_DustCloudOrExplosion(uint16 k) {  // 0x86E4FE
  if (CheckIfEprojIsOffScreen(k))
    eproj_id[k >> 1] = 0;
}
uint16 Eproj_FuncE722(uint16 k) {  // 0x86E722
  int16 v2;
  int16 v3;

  int v1 = k >> 1;
  v2 = eproj_x_pos[v1];
  uint16 result = 1;
  if (v2 >= 0) {
    if (sign16(v2 - 512)) {
      v3 = eproj_y_pos[v1];
      if (v3 >= 0) {
        if (sign16(v3 - 512))
          return 0;
      }
    }
  }
  return result;
}
void Eproj_FuncE73E_MoveXY(uint16 k) {  // 0x86E73E
  int v1 = k >> 1;
  if (((ERAM(0x97DC + 2 * (v1)) + 64) & 0x80) != 0) {
    AddToHiLo(&eproj_x_pos[v1], &eproj_x_subpos[v1], -IPAIR32(eproj_x_vel[v1], eproj_E[v1]));
  } else {
    AddToHiLo(&eproj_x_pos[v1], &eproj_x_subpos[v1], __PAIR32__(eproj_x_vel[v1], eproj_E[v1]));
  }
  if (((ERAM(0x97DC + 2 * (v1)) + 128) & 0x80) != 0) {
    AddToHiLo(&eproj_y_pos[v1], &eproj_y_subpos[v1], -IPAIR32(eproj_y_vel[v1], eproj_F[v1]));
  } else {
    AddToHiLo(&eproj_y_pos[v1], &eproj_y_subpos[v1], __PAIR32__(eproj_y_vel[v1], eproj_F[v1]));
  }
}
static void EprojInit_YappingMawsBody(uint16 j) {  // 0x86EC62
  EnemyData *v2 = gEnemyData(cur_enemy_index);
  int v3 = j >> 1;
  eproj_x_pos[v3] = v2->x_pos;
  eproj_y_pos[v3] = v2->y_pos;
  eproj_instr_list_ptr[v3] = addr_word_86EC5C;
  if (!v2->parameter_2)
    eproj_instr_list_ptr[v3] = addr_word_86EC56;
  ERAM(0x8800 + (uint16)((uint16)(cur_enemy_index
                                                 + 2 * ERAM(0x8800 + (uint16)(cur_enemy_index + 8))))) = j;
}
uint16 EprojInstr_ECE3(uint16 k, uint16 epjp) {  // 0x86ECE3
  int v2 = k >> 1;
  uint16 x = eproj_x_pos[v2] + (NextRandom() & 0x3F) - 32;
  uint16 y = eproj_y_pos[v2] + ((uint16)(random_number & 0x3F00) >> 8) - 32;
  CreateSpriteAtPos(x, y, BW_86(epjp), 0);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_ED17(uint16 k, uint16 epjp) {  // 0x86ED17
  int v2 = k >> 1;
  uint16 x = eproj_x_pos[v2] + (NextRandom() & 0x1F) - 16;
  uint16 y = eproj_y_pos[v2] + ((uint16)(random_number & 0x1F00) >> 8) - 16;
  CreateSpriteAtPos(x, y, BW_86(epjp), 0);
  return (uint16)(epjp + 2);
}
uint16 EprojInstr_QueueSfx2_9(uint16 k, uint16 epjp) {  // 0x86EE8B
  QueueSfx2_Max1(9);
  return epjp;
}
uint16 EprojInstr_QueueSfx2_24(uint16 k, uint16 epjp) {  // 0x86EE97
  QueueSfx2_Max1(0x24);
  return epjp;
}
uint16 EprojInstr_QueueSfx2_B(uint16 k, uint16 epjp) {  // 0x86EEA3
  QueueSfx2_Max1(0xB);
  return epjp;
}
uint16 EprojInstr_EEAF(uint16 k, uint16 epjp) {  // 0x86EEAF
  uint16 v2 = RandomDropRoutine(k);
  if (k != 0 && sign16(v2 - 6)) {
    uint16 v3 = 2 * v2;
    int v4 = k >> 1;
    eproj_E[v4] = v3;
    eproj_instr_list_ptr[v4] = ((uint16)BW_86(0xEF04 + 2 * (v3 >> 1)));
    eproj_instr_timers[v4] = 1;
    eproj_F[v4] = 400;
    eproj_pre_instr[v4] = 0xEFE0 /* EprojPreInstr_Pickup */;
    eproj_properties[v4] &= ~0x4000;
    return (uint16)(eproj_instr_list_ptr[v4]);
  } else {
    int v6 = k >> 1;
    eproj_instr_timers[v6] = 1;
    eproj_properties[v6] = 12288;
    eproj_pre_instr[v6] = 0xEFDF /* EprojPreInstr_Empty */;
    eproj_instr_list_ptr[v6] = 0xECA3;
    return (uint16)(0xECA3);
  }
}
uint16 EprojInstr_HandleRespawningEnemy(uint16 k, uint16 epjp) {  // 0x86EF10
  if ((int16)eproj_killed_enemy_index[k >> 1] <= -2)
    RespawnEnemy(eproj_killed_enemy_index[k >> 1] & 0x7fff);
  return epjp;
}
static void EprojInit_F337(uint16 j) {  // 0x86EF29
  int v2 = j >> 1;
  eproj_x_pos[v2] = eproj_spawn_x;
  eproj_y_pos[v2] = eproj_spawn_y;
  eproj_gfx_idx[v2] = 0;
  eproj_enemy_header_ptr[v2] = eproj_spawn_varE24; // this is X?!
  uint16 v3 = RandomDropRoutine(j);
  if (v3 != 0 && sign16(v3 - 6)) { // bug, why does it compare x here.
    uint16 v4 = 2 * v3;
    eproj_E[v2] = v4;
    eproj_instr_list_ptr[v2] = ((uint16)BW_86(0xEF04 + 2 * (v4 >> 1)));
    eproj_instr_timers[v2] = 1;
    eproj_F[v2] = 400;
    eproj_killed_enemy_index[j >> 1] = -1;
  } else {
    int v5 = j >> 1;
    eproj_instr_list_ptr[v5] = addr_word_86ECA3;
    eproj_instr_timers[v5] = 1;
    eproj_properties[v5] = 12288;
    eproj_pre_instr[v5] = 0xEFDF /* EprojPreInstr_Empty */;
  }
}
static void EprojInit_EnemyDeathExplosion(uint16 j) {  // 0x86EF89
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  int v2 = j >> 1;
  eproj_x_pos[v2] = v1->x_pos;
  eproj_y_pos[v2] = v1->y_pos;
  eproj_killed_enemy_index[v2] = cur_enemy_index;
  if ((v1->properties & 0x4000) != 0)
    eproj_killed_enemy_index[v2] = cur_enemy_index | 0x8000;
  eproj_enemy_header_ptr[v2] = v1->enemy_ptr;
  eproj_gfx_idx[v2] = 0;
  eproj_instr_list_ptr[v2] = ((uint16)BW_86(0xEFD5 + 2 * (eproj_init_param_1)));
  eproj_instr_timers[v2] = 1;
}
void EprojPreInstr_Pickup(uint16 k) {
  int v1 = k >> 1;
  if (!--eproj_F[v1])
    goto LABEL_7;
  if (CallSomeSamusCode(0xD)) {
    if (sign16(eproj_F[v1] - 384)) {
      uint16 v2, v3;
      v2 = abs16(eproj_x_pos[v1] - grapple_beam_end_x_pos);
      if (sign16(v2 - 16)) {
        v3 = abs16(eproj_y_pos[v1] - grapple_beam_end_y_pos);
        if (sign16(v3 - 16)) {
          off_86F0AD[eproj_E[v1] >> 1]();
LABEL_7:;
          int v4 = k >> 1;
          eproj_instr_list_ptr[v4] = addr_word_86ECA3;
          eproj_instr_timers[v4] = 1;
          eproj_properties[v4] = 12288;
          eproj_pre_instr[v4] = 0xEFDF /* EprojPreInstr_Empty */;
          return;
        }
      }
    }
  }
  int v5 = k >> 1;
  uint16 varE20 = LOBYTE(eproj_radius[v5]);
  uint16 varE22 = HIBYTE(eproj_radius[v5]);
  uint16 v6 = abs16(samus_x_pos - eproj_x_pos[v5]);
  bool v7 = v6 < samus_x_radius;
  uint16 v8 = v6 - samus_x_radius;
  if (v7 || v8 < varE20) {
    uint16 v9 = abs16(samus_y_pos - eproj_y_pos[v5]);
    v7 = v9 < samus_y_radius;
    uint16 v10 = v9 - samus_y_radius;
    if (v7 || v10 < varE22) {
      off_86F0AD[eproj_E[v5] >> 1]();
      int v11 = k >> 1;
      eproj_instr_list_ptr[v11] = addr_word_86ECA3;
      eproj_instr_timers[v11] = 1;
      eproj_properties[v11] = 12288;
      eproj_pre_instr[v11] = 0xEFDF /* EprojPreInstr_Empty */;
    }
  }
}
void Eproj_Pickup_SmallHealth(void) {  // 0x86F0BB
  Samus_RestoreHealth(5);
  QueueSfx2_Max1(1);
}
void Eproj_Pickup_BigHealth(void) {  // 0x86F0CA
  Samus_RestoreHealth(0x14);
  QueueSfx2_Max1(2);
}
void Eproj_Pickup_PowerBombs(void) {  // 0x86F0D9
  Samus_RestorePowerBombs(1);
  QueueSfx2_Max1(5);
}
void Eproj_Pickup_Missiles(void) {  // 0x86F0E8
  Samus_RestoreMissiles(2);
  QueueSfx2_Max1(3);
}
void Eproj_Pickup_SuperMissiles(void) {  // 0x86F0F7
  Samus_RestoreSuperMissiles(1);
  QueueSfx2_Max1(4);
}
uint16 RandomDropRoutine(uint16 k) {  // 0x86F106
  int8 v9; // cf
  uint8 r18, r22, r20;
  uint16 r24;

  int v1 = k >> 1;
  //varE2A = eproj_killed_enemy_index[v1] & 0x7FFF;
  uint16 varE28 = eproj_enemy_header_ptr[v1];
  if (varE28 == 0)
    goto LABEL_30;
  uint16 v2;
  v2 = get_EnemyDef_A2(varE28)->item_drop_chances_ptr;
  if (!v2)
    goto LABEL_30;
  uint8 Random;
  do
    Random = NextRandom();
  while (!Random);
  r20 = 255;
  r24 = 0;
  uint16 v5;
  v5 = 1;
  if ((uint16)(samus_reserve_health + samus_health) >= 0x1E) {
    if ((uint16)(samus_reserve_health + samus_health) < 0x32)
      goto LABEL_7;
    v5 = 0;
  }
  health_drop_bias_flag = v5;
LABEL_7:;
  uint16 v7 = (v2);
  if ((uint8)health_drop_bias_flag) {
    r18 = BB_B4(v7 + 1 * (1)) + BB_B4(v7);
    r22 = 3;
  } else {
    r18 = BB_B4(v7 + 1 * (3));
    r22 = 8;
    if (samus_health != samus_max_health || samus_reserve_health != samus_max_reserve_health) {
      r18 = BB_B4(v7 + 1 * (1)) + BB_B4(v7) + r18;
      r22 = r22 | 3;
    }
    if (samus_missiles != samus_max_missiles) {
      r18 = BB_B4(v7 + 1 * (2)) + r18;
      r22 = r22 | 4;
    }
    if (samus_super_missiles != samus_max_super_missiles) {
      r20 = r20 - BB_B4(v7 + 1 * (4));
      r22 = r22 | 0x10;
    }
    if (samus_power_bombs != samus_max_power_bombs) {
      r20 = r20 - BB_B4(v7 + 1 * (5));
      r22 = r22 | 0x20;
    }
  }
  int i;
  for (i = 0; i != 4; ++i) {
    if (!(uint8)r18) {
      r22 = (uint8)r22 >> 4;
      v2 += 4;
      i = 4;
      goto LABEL_26;
    }
    v9 = r22 & 1;
    r22 = (uint8)r22 >> 1;
    if (v9) {
      uint16 v10 = (v2);
      uint16 RegWord = Mult8x8(r20, BB_B4(v10));
      int divved = SnesDivide(RegWord, r18);
      uint16 v12 = r24;
      uint16 v13 = divved + v12;
      if (v13 >= Random)
        return ((uint8)BB_86(0xF25E + (i)));
      r24 = v13;
    }
    ++v2;
  }
  do {
LABEL_26:
    v9 = r22 & 1;
    r22 = (uint8)r22 >> 1;
    if (v9) {
      uint16 v14 = (v2);
      if ((uint16)(r24 + BB_B4(v14)) >= Random)
        return ((uint8)BB_86(0xF25E + (i)));
      r24 += BB_B4(v14);
    }
    ++v2;
    ++i;
  } while (i != 6);
LABEL_30:
  i = 3;
  return ((uint8)BB_86(0xF25E + (i)));
}
void RespawnEnemy(uint16 v0) {  // 0x86F264
  cur_enemy_index = v0;
  uint16 v1 = (room_enemy_population_ptr + (v0 >> 2));
  EnemyData *E = gEnemyData(v0);
  E->enemy_ptr = BW_A1(v1);
  E->x_pos = BW_A1(v1 + 2);
  E->y_pos = BW_A1(v1 + 4);
  E->current_instruction = BW_A1(v1 + 6);
  E->properties = BW_A1(v1 + 8);
  E->extra_properties = BW_A1(v1 + 10);
  E->parameter_1 = BW_A1(v1 + 12);
  E->parameter_2 = BW_A1(v1 + 14);
  EnemySpawnData *ESD = gEnemySpawnData(v0);
  E->palette_index = ESD->palette_index;
  E->vram_tiles_index = ESD->vram_tiles_index;
  E->frozen_timer = 0;
  E->flash_timer = 0;
  E->invincibility_timer = 0;
  E->timer = 0;
  E->frame_counter = 0;
  E->ai_var_A = 0;
  E->ai_var_B = 0;
  E->ai_var_C = 0;
  E->ai_var_D = 0;
  E->ai_var_E = 0;
  E->ai_preinstr = 0;
  E->instruction_timer = 1;
  EnemyDef *Edef = get_EnemyDef_A2(E->enemy_ptr);
  E->x_width = Edef->x_radius;
  E->y_height = Edef->y_radius;
  E->health = Edef->health;
  E->layer = Edef->layer;
  E->bank = Edef->bank; E->field_2F = Edef->hurt_ai_time;
  CallEnemyAi(Edef->bank, Edef->ai_init);
}
void CreateADudShot(void) {  // 0xA08046
  CreateDudShot();
}
uint16 EnemyInstr_SetAiPreInstr(uint16 k, uint16 jp) {  // 0xA0806B
  gEnemyData(k)->ai_preinstr = IW_EN((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 EnemyInstr_ClearAiPreInstr(uint16 k, uint16 jp) {  // 0xA08074
  gEnemyData(k)->ai_preinstr = 0x807B /* nullsub_171 */;
  return jp;
}
uint16 EnemyInstr_StopScript(uint16 k, uint16 jp) {  // 0xA0807C
  EnemyData *v2 = gEnemyData(k);
  v2->properties |= kEnemyProps_Deleted;
  return 0;
}
uint16 EnemyInstr_Goto(uint16 k, uint16 jp) {  // 0xA280ED
  return (uint16)(IW_EN(jp));
}
uint16 EnemyInstr_DecTimerAndGoto(uint16 k, uint16 jp) {  // 0xA08108
  EnemyData *v2 = gEnemyData(k);
  if (v2->timer-- == 1)
    return (uint16)(jp + 2);
  else
    return EnemyInstr_Goto(k, jp);
}
uint16 EnemyInstr_SetTimer(uint16 k, uint16 jp) {  // 0xA08123
  gEnemyData(k)->timer = IW_EN(jp);
  return (uint16)(jp + 2);
}
uint16 EnemyInstr_Skip2bytes(uint16 k, uint16 jp) {  // 0xA0812C
  return (uint16)(jp + 2);
}
uint16 EnemyInstr_EnableOffScreenProcessing(uint16 k, uint16 jp) {  // 0xA08173
  EnemyData *v2 = gEnemyData(k);
  v2->properties |= 0x800;
  return jp;
}
uint16 EnemyInstr_DisableOffScreenProcessing(uint16 k, uint16 jp) {  // 0xA0817D
  EnemyData *v2 = gEnemyData(k);
  v2->properties &= ~0x800;
  return jp;
}
void SpawnEnemyDrops(uint16 a, uint16 k, uint16 varE20) {  // 0xA0920E
  eproj_spawn_varE24 = a;
  SpawnEprojWithGfx(varE20, k, addr_kEproj_Pickup);
}
void NormalEnemyFrozenAI(void) {  // 0xA0957E
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  v0->flash_timer = 0;
  if (!v0->frozen_timer || (--v0->frozen_timer, (equipped_beams & 2) == 0)) {
    uint16 v1 = v0->ai_handler_bits & 0xFFFB;
    v0->ai_handler_bits = v1;
    v0->frozen_timer = v1;
  }
}
void EnemyDeathAnimation(uint16 k, uint16 a) {  // 0xA0A3AF
  EnemyData *E = gEnemyData(cur_enemy_index);
  if (E->ai_handler_bits == 1)
    grapple_beam_function = 0xC8C5 /* GrappleBeam_Func2 */;
  if (!sign16(a - 5))
    a = 0;
//  varE20 = a;
  SpawnEprojWithGfx(a, cur_enemy_index, addr_kEproj_EnemyDeathExplosion);
  uint16 r18 = E->properties & 0x4000;
  memset(E, 0, 64);
  if (r18) {
    E->enemy_ptr = addr_kEnemyDef_DAFF;
    E->bank = 0xa3;
  }
  num_enemies_killed_in_room++;
}
void NormalEnemyTouchAi(void) {  // 0xA0A477
  NormalEnemyTouchAiSkipDeathAnim();
  if (!gEnemyData(cur_enemy_index)->health) {
    gEnemySpawnData(cur_enemy_index)->cause_of_death = 6;
    EnemyDeathAnimation(cur_enemy_index, 1);
  }
}
void NormalEnemyTouchAiSkipDeathAnim_CurEnemy(void) {  // 0xA0A497
  NormalEnemyTouchAiSkipDeathAnim();
}
void NormalEnemyPowerBombAi(void) {  // 0xA0A597
  NormalEnemyPowerBombAiSkipDeathAnim();
  if (!gEnemyData(cur_enemy_index)->health) {
    gEnemySpawnData(cur_enemy_index)->cause_of_death = 3;
    EnemyDeathAnimation(cur_enemy_index, 0);
  }
}
void NormalEnemyPowerBombAiSkipDeathAnim(void) {  // 0xA0A5C1
  EnemyDef *EnemyDef_A2;
  int16 hurt_ai_time;

  EnemyData *v0 = gEnemyData(cur_enemy_index);
  uint16 vulnerability_ptr = get_EnemyDef_A2(v0->enemy_ptr)->vulnerability_ptr;
  if (!vulnerability_ptr)
    vulnerability_ptr = -5092;
  uint8 power_bomb = get_Vulnerability(vulnerability_ptr)->power_bomb;
  if (power_bomb != 255) {
    uint16 varE32 = power_bomb & 0x7F;
    if ((power_bomb & 0x7F) != 0) {
      uint32 ttt = 100 * varE32;
      if (ttt) {
        gEnemyData(cur_enemy_index)->invincibility_timer = 48;
        EnemyData *j = gEnemyData(cur_enemy_index);
        EnemyDef_A2 = get_EnemyDef_A2(j->enemy_ptr);
        hurt_ai_time = EnemyDef_A2->hurt_ai_time;
        if (!EnemyDef_A2->hurt_ai_time)
          hurt_ai_time = 4;
        EnemyData *v6 = gEnemyData(cur_enemy_index);
        v6->flash_timer = hurt_ai_time + 8;
        v6->ai_handler_bits |= 2;
        uint16 health = v6->health;
        bool v9 = health < ttt;
        uint16 v8 = health - ttt;
        v9 = !v9;
        if (!v8 || !v9)
          v8 = 0;
        v6->health = v8;
      }
    }
  }
}
void NormalEnemyShotAi(void) {  // 0xA0A63D
  uint16 varE2E = NormalEnemyShotAiSkipDeathAnim();
  EnemyData *E = gEnemyData(cur_enemy_index);
  if (varE2E) {
    CreateSpriteAtPos(E->x_pos, E->y_pos, 55, 0);
  }
  if (!E->health) {
    uint16 j = HIBYTE(projectile_type[collision_detection_index]) & 0xF;
    gEnemySpawnData(cur_enemy_index)->cause_of_death = j;
    uint16 death_anim = 2;
    if (j == 2) {
      EnemyDef *ED = get_EnemyDef_A2(E->enemy_ptr);
      if (!sign16(ED->death_anim - 3))
        death_anim = ED->death_anim;
    } else {
      death_anim = get_EnemyDef_A2(E->enemy_ptr)->death_anim;
    }
    EnemyDeathAnimation(E->enemy_ptr, death_anim);
  }
}
void NormalEnemyShotAiSkipDeathAnim_CurEnemy(void) {  // 0xA0A6A7
  NormalEnemyShotAiSkipDeathAnim();
}
void EnemyFunc_A6B4_UsedBySporeSpawn(void) {  // 0xA0A6B4
  uint16 varE2E = NormalEnemyShotAiSkipDeathAnim();
  if (varE2E) {
    EnemyData *ED = gEnemyData(cur_enemy_index);
    CreateSpriteAtPos(ED->x_pos, ED->y_pos, 55, 0);
  }
}
uint16 NormalEnemyShotAiSkipDeathAnim(void) {  // 0xA0A6DE
  uint16 varE2E = 0;
  int16 v5;
  int16 v6;
  int16 v9;
  EnemyDef *EnemyDef_A2;
  int16 hurt_ai_time;
  int16 v20;
  uint16 varE32;

  int v0 = collision_detection_index;
  uint16 pd = projectile_damage[v0];
  uint16 r18 = projectile_type[v0];
  EnemyData *j = gEnemyData(cur_enemy_index);
  uint16 vulnerability_ptr = get_EnemyDef_A2(j->enemy_ptr)->vulnerability_ptr;
  if (!vulnerability_ptr)
    vulnerability_ptr = addr_stru_B4EC1C;
  uint16 r20 = vulnerability_ptr;
  if ((r18 & 0xF00) != 0) {
    v5 = r18 & 0xF00;
    if ((r18 & 0xF00) == 256 || v5 == 512) {
      v6 = (r18 & 0xF00) >> 8;
      varE32 = get_Vulnerability(r20 + v6)->plasma_ice_wave & 0x7F;
    } else if (v5 == 1280) {
      varE32 = get_Vulnerability(r20)->bomb & 0x7F;
    } else {
      if (v5 != 768)
        goto LABEL_18;
      varE32 = get_Vulnerability(r20)->power_bomb & 0x7F;
    }
LABEL_9:;
    uint32 ttt = (uint32)(pd >> 1) * (uint32)varE32;
    uint16 r42 = ttt;
    if (r42) {
      pd = r42;
      EnemyData *v10 = gEnemyData(cur_enemy_index);
      EnemyDef_A2 = get_EnemyDef_A2(v10->enemy_ptr);
      hurt_ai_time = EnemyDef_A2->hurt_ai_time;
      if (!EnemyDef_A2->hurt_ai_time)
        hurt_ai_time = 4;
      EnemyData *v13 = gEnemyData(cur_enemy_index);
      v13->flash_timer = hurt_ai_time + 8;
      v13->ai_handler_bits |= 2;
      if (!v13->frozen_timer) {
        uint16 hurt_sfx = get_EnemyDef_A2(v13->enemy_ptr)->hurt_sfx;
        if (hurt_sfx)
          QueueSfx2_Max3(hurt_sfx);
        ++varE2E;
      }
      uint16 v15 = cur_enemy_index;
      if ((projectile_type[collision_detection_index] & 8) != 0)
        gEnemyData(cur_enemy_index)->invincibility_timer = 16;
      EnemyData *v16 = gEnemyData(v15);
      uint16 health = v16->health;
      bool v19 = health < pd;
      uint16 v18 = health - pd;
      v19 = !v19;
      if (!v18 || !v19) {
        if ((projectile_type[collision_detection_index] & 2) != 0
            && (last_enemy_power & 0xF0) != 128
            && !v16->frozen_timer) {
          v20 = 400;
          if (area_index == 2)
            v20 = 300;
          v16->frozen_timer = v20;
          v16->ai_handler_bits |= 4;
          v16->invincibility_timer = 10;
          QueueSfx3_Max3(0xA);
          return varE2E;
        }
        v18 = 0;
      }
      v16->health = v18;
      return varE2E;
    }
LABEL_18:;
    int v7 = collision_detection_index;
    projectile_dir[v7] |= 0x10;
    CreateSpriteAtPos(projectile_x_pos[v7], projectile_y_pos[v7], 6, 0);
    QueueSfx1_Max3(0x3D);
    return varE2E;
  }
  last_enemy_power = get_Vulnerability(r20 + (r18 & 0xF))->power;
  varE32 = last_enemy_power & 0x7F;
  if (last_enemy_power != 255) {
    if ((r18 & 0x10) != 0) {
      uint8 charged_beam = get_Vulnerability(r20)->charged_beam;
      if (charged_beam == 255)
        goto LABEL_18;
      uint16 v4 = charged_beam & 0xF;
      if (!v4)
        goto LABEL_18;
      varE32 = v4;
    }
    goto LABEL_9;
  }
  EnemyData *v8 = gEnemyData(cur_enemy_index);
  if (!v8->frozen_timer)
    QueueSfx3_Max3(0xA);
  v9 = 400;
  if (area_index == 2)
    v9 = 300;
  v8->frozen_timer = v9;
  v8->ai_handler_bits |= 4;
  v8->invincibility_timer = 10;
  return varE2E;
}
void CreateDudShot(void) {  // 0xA0A8BC
  int v0 = collision_detection_index;
  CreateSpriteAtPos(projectile_x_pos[v0], projectile_y_pos[v0], 6, 0);
  QueueSfx1_Max3(0x3D);
  projectile_dir[collision_detection_index] |= 0x10;
}
uint16 CheckIfEnemyTouchesSamus(uint16 k) {  // 0xA0ABE7
  EnemyData *E = gEnemyData(k);
  uint16 i = abs16(samus_x_pos - E->x_pos);
  bool v3 = i < samus_x_radius;
  uint16 v4 = i - samus_x_radius;
  if (!v3 && v4 >= E->x_width)
    return 0;
  if ((int16)(samus_y_pos + 3 - E->y_pos) < 0) {
    uint16 v6 = E->y_pos - (samus_y_pos + 3);
    uint16 v7 = v6 - samus_y_radius;
    if (v6 < samus_y_radius || v7 == E->y_height || v7 < E->y_height)
      return -1;
  }
  return 0;
}
uint16 EnemyFunc_AC67(uint16 k) {  // 0xA0AC67
  EnemyData *E = gEnemyData(k);
  uint16 i = abs16(samus_x_pos - E->x_pos);
  bool v3 = i < samus_x_radius;
  uint16 v4 = i - samus_x_radius;
  if (!v3 && v4 >= E->x_width && v4 >= 8)
    return 0;
  uint16 v6 = abs16(samus_y_pos - E->y_pos);
  v3 = v6 < samus_y_radius;
  uint16 v7 = v6 - samus_y_radius;
  if (v3 || v7 < E->y_height)
    return -1;
  else
    return 0;
}
PairU16 EnemyFunc_ACA8(Point16U base_pt, Point16U samus_pt) {  // 0xA0ACA8
  uint16 varE3C = samus_pt.x - base_pt.x;
  uint16 result = abs16(varE3C);
  if (sign16(result - 255)) {
    uint16 x = result;
    uint16 varE20 = result;
    uint16 varE3E = samus_pt.y - base_pt.y;
    result = abs16(varE3E);
    if (sign16(result - 255)) {
      uint16 y = result;
      uint16 varE22 = result;
      uint16 varE24 = CalculateAngleFromXY(x, y);
      uint16 dist = abs16(SineMult8bit(varE24, varE20)) + abs16(CosineMult8bit(varE24, varE22));
      uint16 varE3A = CalculateAngleFromXY(varE3C, varE3E);
      return (PairU16) {dist, varE3A };
    }
  }
  return (PairU16) { result, 0 };
}
uint16 CheckIfEnemyIsOnScreen(void) {  // 0xA0AD70
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  return (int16)(v0->x_pos - layer1_x_pos) < 0 || (int16)(layer1_x_pos + 256 - v0->x_pos) < 0 || 
      (int16)(v0->y_pos - layer1_y_pos) < 0 || (int16)(layer1_y_pos + 256 - v0->y_pos) < 0;
}
uint16 DetermineDirectionOfSamusFromEnemy(void) {  // 0xA0AE29
  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x20)) {
    if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) != 0)
      return 7;
    return 2;
  } else if (IsSamusWithinEnemy_X(cur_enemy_index, 0x20)) {
    if ((GetSamusEnemyDelta_Y(cur_enemy_index) & 0x8000) != 0)
      return 0;
    return 4;
  } else if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) != 0) {
    if ((GetSamusEnemyDelta_Y(cur_enemy_index) & 0x8000) != 0)
      return 8;
    return 6;
  } else {
    if ((GetSamusEnemyDelta_Y(cur_enemy_index) & 0x8000) != 0)
      return 1;
    return 3;
  }
}
uint16 GetSamusEnemyDelta_Y(uint16 k) {  // 0xA0AEDD
  return samus_y_pos - gEnemyData(k)->y_pos;
}
uint16 GetSamusEnemyDelta_X(uint16 k) {  // 0xA0AEE5
  return samus_x_pos - gEnemyData(k)->x_pos;
}
uint16 IsSamusWithinEnemy_Y(uint16 k, uint16 a) {  // 0xA0AEED
  return (int16)(SubtractThenAbs16(gEnemyData(k)->y_pos, samus_y_pos) - a) < 0;
}
uint16 IsSamusWithinEnemy_X(uint16 k, uint16 a) {  // 0xA0AF0B
  return (int16)(SubtractThenAbs16(gEnemyData(k)->x_pos, samus_x_pos) - a) < 0;
}
uint16 SignExtend8(uint16 a) {  // 0xA0AFEA
  // not quite sign extend
  return (a & 0x80) ? a | 0xff00 : a;
}
uint16 Abs16(uint16 a) {  // 0xA0B067
  return ((a & 0x8000) != 0) ? -a : a;
}
uint16 SubtractThenAbs16(uint16 k, uint16 j) {  // 0xA0B07D
  return Abs16(j - k);
}
static uint32 SineMult8bitInner(uint16 varE32, uint16 varE34) {  // 0xA0B0DA
  uint16 mult = Mult8x8(((uint8)BB_A0(0xB143 + (varE34 & 0x7F))), varE32);
  uint16 a = mult >> 8;
  uint16 b = mult << 8;
  if ((varE34 & 0x80) != 0) {
    a = -a;
    b = -b;
  }
  return __PAIR32__(a, b);
}
uint16 CosineMult8bit(uint16 a, uint16 varE32) {  // 0xA0B0B2
  return SineMult8bitInner(varE32, (uint8)(a + 64)) >> 16;
}
uint16 SineMult8bit(uint16 a, uint16 varE32) {  // 0xA0B0C6
  return SineMult8bitInner(varE32, (uint8)(a + 0x80)) >> 16;
}
uint32 CosineMult8bitFull(uint16 a, uint16 varE32) {
  return SineMult8bitInner(varE32, (uint8)(a + 64));
}
uint32 SineMult8bitFull(uint16 a, uint16 varE32) {
  return SineMult8bitInner(varE32, (uint8)(a + 0x80));
}
Point32 ConvertAngleToXy(uint16 r18, uint16 r20) {  // 0xA0B643
  uint32 ss = ((uint16)BW_A0(0xB7EE + 2 * ((r18 + 0x40) & 0x7F))) * r20;
  uint32 tt = ((uint16)BW_A0(0xB7EE + 2 * ((r18 + 0x80) & 0x7F))) * r20;
  return (Point32) { ss, tt };
}
void EnemyFunc_B691(uint16 varE20, Point32 pt) {  // 0xA0B691
  EnemyData *E = gEnemyData(cur_enemy_index);
  if (((varE20 + 64) & 0x80) != 0)
    AddToHiLo(&E->x_pos, &E->x_subpos, -(int32)pt.x);
  else
    AddToHiLo(&E->x_pos, &E->x_subpos, pt.x);
  if (((varE20 + 128) & 0x80) != 0)
    AddToHiLo(&E->y_pos, &E->y_subpos, -(int32)pt.y);
  else
    AddToHiLo(&E->y_pos, &E->y_subpos, pt.y);
}
void EnemyFunc_B7A1(void) {  // 0xA0B7A1
  uint16 yd = samus_y_pos - samus_prev_y_pos;
  uint16 v0 = Abs16(samus_y_pos - samus_prev_y_pos);
  if (!sign16(v0 - 12))
    samus_prev_y_pos = samus_y_pos + ((yd & 0x8000) == 0 ? 12 : -12);
  uint16 xd = samus_x_pos - samus_prev_x_pos;
  uint16 i = Abs16(samus_x_pos - samus_prev_x_pos);
  if (!sign16(i - 12))
    samus_prev_x_pos = samus_x_pos + ((xd & 0x8000) == 0 ? 12 : -12);
}
void Enemy_ItemDrop_MiniKraid(uint16 k) {  // 0xA0B8EE
  int n = 4;
  do {
    eproj_spawn_x = special_death_item_drop_x_origin_pos + (NextRandom() & 0x1F) - 16;
    eproj_spawn_y = special_death_item_drop_y_origin_pos + ((random_number & 0x1F00) >> 8) - 16;
    SpawnEnemyDrops(addr_kEnemyDef_E0FF, k, 0);
  } while (--n);
}
void Enemy_ItemDrop_LowerNorfairSpacePirate(uint16 k) {  // 0xA0B92B
  int n = 5;
  do {
    eproj_spawn_x = special_death_item_drop_x_origin_pos + (NextRandom() & 0x1F) - 16;
    eproj_spawn_y = special_death_item_drop_y_origin_pos + ((random_number & 0x1F00) >> 8) - 16;
    SpawnEnemyDrops(addr_kEnemyDef_F593, k, 0);
  } while (--n);
}
void Enemy_ItemDrop_BombTorizo(uint16 k) {  // 0xA0BAA4
  int n = 16;
  do {
    eproj_spawn_x = (NextRandom() & 0x7F) + 64;
    eproj_spawn_y = ((random_number & 0x3F00) >> 8) + 96;
    SpawnEnemyDrops(addr_kEnemyDef_EEFF, k, 0);
  } while (--n);
}
void Enemy_ItemDrop_GoldenTorizo(uint16 k) {  // 0xA0BAD7
  int n = 16;
  do {
    eproj_spawn_x = (uint8)NextRandom() + 128;
    eproj_spawn_y = ((random_number & 0x3F00) >> 8) + 288;
    SpawnEnemyDrops(addr_kEnemyDef_EEFF, k, 0);
  } while (--n);
}
void Enemy_ItemDrop_SporeSpawn(uint16 k) {  // 0xA0BB0A
  int n = 16;
  do {
    eproj_spawn_x = (NextRandom() & 0x7F) + 64;
    eproj_spawn_y = ((random_number & 0x3F00) >> 8) + 528;
    SpawnEnemyDrops(addr_kEnemyDef_DF3F, k, 0);
  } while (--n);
}
uint8 CompareDistToSamus_X(uint16 k, uint16 a) {  // 0xA0BB9B
  EnemyData *i = gEnemyData(k);
  return abs16(samus_x_pos - i->x_pos) >= a;
}
uint8 EnemyFunc_BBBF(uint16 k, int32 amt) {  // 0xA0BBBF
  EnemyData *E = gEnemyData(k);
  uint16 R26 = (E->y_pos - E->y_height) & 0xFFF0;
  R26 = (uint16)(E->y_height + E->y_pos - 1 - R26) >> 4;
  uint16 prod = Mult8x8((uint16)(E->y_pos - E->y_height) >> 4, room_width_in_blocks);
  uint16 i = (amt + __PAIR32__(E->x_pos, E->x_subpos)) >> 16;
  uint16 j;
  if (sign32(amt))
    j = i - E->x_width;
  else
    j = E->x_width + i - 1;
  uint16 R34 = j;
  uint16 v4 = 2 * (prod + (j >> 4));
  while ((level_get(v4 >> 1) & 0x8000) == 0) {
    v4 += room_width_in_blocks * 2;
    if ((--R26 & 0x8000) != 0)
      return 0;
  }
  return 1;
}
uint8 EnemyFunc_BF8A(uint16 k, uint16 a, int32 amt) {  // 0xA0BF8A
  uint16 v4;

  EnemyData *E = gEnemyData(k);
  uint16 R26 = (E->x_pos - E->x_width) & 0xFFF0;
  R26 = (uint16)(E->x_width + E->x_pos - 1 - R26) >> 4;

  if (a & 1) {
    uint16 j = (amt + __PAIR32__(E->y_pos, E->y_subpos)) >> 16;
    v4 = E->y_height + j - 1;
  } else {
    uint16 v5 = (__PAIR32__(E->y_pos, E->y_subpos) - amt) >> 16;
    v4 = v5 - E->y_height;
  }
  uint16 R34 = v4;
  uint16 prod = Mult8x8(v4 >> 4, room_width_in_blocks);
  int16 v6 = (uint16)(E->x_pos - E->x_width) >> 4;
  for (int i = 2 * (prod + v6); (level_get(i >> 1) & 0x8000) == 0; i += 2) {
    if ((--R26 & 0x8000) != 0)
      return 0;
  }
  return 1;
}
uint16 CalculateAngleOfSamusFromEproj(uint16 k) {  // 0xA0C04E
  int j = k >> 1;
  return CalculateAngleFromXY(samus_x_pos - eproj_x_pos[j], samus_y_pos - eproj_y_pos[j]);
}
uint16 CalculateAngleOfSamusFromEnemy(uint16 k) {  // 0xA0C066
  EnemyData *E = gEnemyData(k);
  return CalculateAngleFromXY(samus_x_pos - E->x_pos, samus_y_pos - E->y_pos);
}
uint8 IsEnemyLeavingScreen(uint16 k) {  // 0xA0C18E
  int16 x_pos;
  int16 v3;

  EnemyData *E = gEnemyData(k);
  x_pos = E->x_pos;
  if (x_pos >= 0) {
    v3 = E->x_width + x_pos - layer1_x_pos;
    if (v3 >= 0 && (int16)(v3 - 256 - E->x_width) < 0)
      return 0;
  }
  return 1;
}
static uint8 ClearCarry_13(EnemyBlockCollInfo *ebci) {  // 0xA0C2BC
  return 0;
}
static uint8 SetCarry_4(EnemyBlockCollInfo *ebci) {  // 0xA0C2BE
  return 1;
}
static uint8 EnemyBlockCollReact_Spike(EnemyBlockCollInfo *ebci) {  // 0xA0C2C0
  uint16 v0 = ((uint16)BW_A0(0xC2DA + 2 * (bts_get(cur_block_index) & 0x7F)));
  if (!v0)
    return 1;
  SpawnPLM(v0);
  return 0;
}
static uint8 EnemyBlockCollHorizReact_Slope(EnemyBlockCollInfo *ebci) {  // 0xA0C2FA
  if ((bts_get(cur_block_index) & 0x1F) >= 5) {
    current_slope_bts = bts_get(cur_block_index);
    return EnemyBlockCollHorizReact_Slope_NonSquare(ebci);
  } else {
    return EnemyBlockCollHorizReact_Slope_Square(ebci, cur_block_index, bts_get(cur_block_index) & 0x1F);
  }
}
static uint8 EnemyBlockCollVertReact_Slope(EnemyBlockCollInfo *ebci) {  // 0xA0C319
  if ((bts_get(cur_block_index) & 0x1F) >= 5)
    return EnemyBlockCollVertReact_Slope_NonSquare(ebci);
  else
    return EnemyBlockCollVertReact_Slope_Square(ebci, bts_get(cur_block_index) & 0x1F, cur_block_index);
}
static uint8 EnemyBlockCollHorizReact_Slope_Square(EnemyBlockCollInfo *ebci, uint16 k, uint16 a) {
  EnemyData *E = gEnemyData(cur_enemy_index);

  uint16 temp_collision_DD4 = 4 * a;
  uint16 temp_collision_DD6 = bts_get(k) >> 6;
  uint16 i = 4 * a + (temp_collision_DD6 ^ ((ebci->ebci_r26 & 8) >> 3));
  if (!ebci->ebci_r28) {
    if (((LOBYTE(E->y_height) + LOBYTE(E->y_pos) - 1) & 8) == 0)
      return ((((uint8)BB_A0(0xC435 + ((i)))) & 0x80 ? -1 : 0)) < 0;
    goto LABEL_7;
  }
  if (ebci->ebci_r28 != ebci->ebci_r30 || ((E->y_pos - E->y_height) & 8) == 0) {
LABEL_7:
    if (((((uint8)BB_A0(0xC435 + ((i)))) & 0x80 ? -1 : 0)) < 0)
      return 1;
  }
  return ((((uint8)BB_A0(0xC435 + ((i ^ 2)))) & 0x80 ? -1 : 0)) < 0;
}
static uint8 EnemyBlockCollVertReact_Slope_Square(EnemyBlockCollInfo *ebci, uint16 a, uint16 k) {  // 0xA0C3B2
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 temp_collision_DD4 = 4 * a;
  uint16 temp_collision_DD6 = bts_get(k) >> 6;
  uint16 i = 4 * a + (temp_collision_DD6 ^ ((ebci->ebci_r26 & 8) >> 2));
  if (!ebci->ebci_r28) {
    if (((LOBYTE(E->x_width) + LOBYTE(E->x_pos) - 1) & 8) == 0)
      return ((((uint8)BB_A0(0xC435 + ((i)))) & 0x80 ? -1 : 0)) < 0;
    goto LABEL_7;
  }
  if (ebci->ebci_r28 != ebci->ebci_r30 || ((E->x_pos - E->x_width) & 8) == 0) {
LABEL_7:
    if (((((uint8)BB_A0(0xC435 + ((i)))) & 0x80 ? -1 : 0)) < 0)
      return 1;
  }
  return ((((uint8)BB_A0(0xC435 + ((i ^ 1)))) & 0x80 ? -1 : 0)) < 0;
}
static uint8 EnemyBlockCollHorizReact_Slope_NonSquare(EnemyBlockCollInfo *ebci) {  // 0xA0C449
  if ((ebci->ebci_r32 & 0x8000) == 0)
    return (ebci->ebci_r32 & 0x4000) != 0;
  int i = 2 * (current_slope_bts & 0x1F);
  if (ebci->ebci_r18_r20 >= 0) {
    ebci->ebci_r18_r20 = (ebci->ebci_r18_r20 >> 8) * ((uint16)BW_A0(0xC49F + 2 * (i + 1)));
  } else {
    ebci->ebci_r18_r20 = -(-(int16)(ebci->ebci_r18_r20 >> 8) * ((uint16)BW_A0(0xC49F + 2 * (i + 1))));
  }
  return 0;
}
static uint8 EnemyBlockCollVertReact_Slope_NonSquare(EnemyBlockCollInfo *ebci) {  // 0xA0C51F
  int16 v3;
  int16 v5;
  int16 v6;
  uint16 v7;
  int16 v8;
  int16 v11;
  int16 v12;
  int16 x_pos;
  uint16 v14;
  int16 v15;
  uint16 mod = SnesModulus(cur_block_index, room_width_in_blocks);
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 v1 = cur_block_index;

  if (ebci->ebci_r18_r20 < 0) {
    v11 = E->x_pos >> 4;
    if (v11 != mod)
      return 0;
    uint16 temp_collision_DD4 = (ebci->ebci_r24 - E->y_height) & 0xF ^ 0xF;
    uint16 temp_collision_DD6 = 16 * (bts_get(v1) & 0x1F);
    v12 = bts_get(v1) << 8;
    if (v12 < 0
        && ((v12 & 0x4000) != 0 ? (x_pos = E->x_pos ^ 0xF) : (x_pos = E->x_pos),
        (v14 = temp_collision_DD6 + (x_pos & 0xF),
        v15 = (((uint8)BB_94(0x8B2B + (v14))) & 0x1F) - temp_collision_DD4 - 1, v15 <= 0))) {
      E->y_pos = ebci->ebci_r24 - v15;
      E->y_subpos = 0;
      return 1;
    } else {
      return 0;
    }
  } else {
    v3 = E->x_pos >> 4;
    if (v3 != mod)
      return 0;
    uint16 temp_collision_DD4 = (E->y_height + ebci->ebci_r24 - 1) & 0xF;
    uint16 temp_collision_DD6 = 16 * (bts_get(v1) & 0x1F);
    v5 = bts_get(v1) << 8;
    if (v5 >= 0
      && ((v5 & 0x4000) != 0 ? (v6 = E->x_pos ^ 0xF) : (v6 = E->x_pos),
      (v7 = temp_collision_DD6 + (v6 & 0xF),
      v8 = (((uint8)BB_94(0x8B2B + (v7))) & 0x1F) - temp_collision_DD4 - 1,
      (((uint8)BB_94(0x8B2B + (v7))) & 0x1F) - temp_collision_DD4 == 1) || v8 < 0)) {
      E->y_pos = ebci->ebci_r24 + v8;
      E->y_subpos = -1;
      return 1;
    } else {
      return 0;
    }
  }
}
static uint8 EnemyBlockCollReact_HorizExt(EnemyBlockCollInfo *ebci) {  // 0xA0C619
  uint8 t = bts_get(cur_block_index);
  if (t) {
    cur_block_index += (int8)t;
    return 0xff;
  }
  return 0;
}
static uint8 EnemyBlockCollReact_VertExt(EnemyBlockCollInfo *ebci) {  // 0xA0C64F
  if (bts_get(cur_block_index)) {
    cur_block_index += Mul16((int8)bts_get(cur_block_index), room_width_in_blocks);
    return 0xff;
  }
  return 0;
}
uint8 Enemy_MoveRight_ProcessSlopes(uint16 k, int32 amt) {  // 0xA0C6A4
  return Enemy_MoveRight_IgnoreSlopes_Inner(k, amt, 0x8000);
}
uint8 Enemy_MoveRight_IgnoreSlopes(uint16 k, int32 amt) {  // 0xA0C6AB
  return Enemy_MoveRight_IgnoreSlopes_Inner(k, amt, 0);
}
static uint8 Enemy_MoveRight_IgnoreSlopes_Inner(uint16 k, int32 amount32, uint16 r32) {  // 0xA0C6AD
  if (!amount32)
    return 0;
  EnemyData *E = gEnemyData(k);
  uint16 r28 = (uint16)(E->y_height + E->y_pos - 1 - ((E->y_pos - E->y_height) & 0xFFF0)) >> 4;
  uint16 r30 = r28;
  uint16 prod = Mult8x8((uint16)(E->y_pos - E->y_height) >> 4, room_width_in_blocks);
  uint32 new_pos = amount32 + __PAIR32__(E->x_pos, E->x_subpos);
  uint16 v5;
  uint16 r24 = new_pos >> 16;  // read by EnemyBlockCollVertReact_Slope_NonSquare
  if (sign32(amount32))
    v5 = (new_pos >> 16) - E->x_width;
  else
    v5 = E->x_width + (new_pos >> 16) - 1;
  EnemyBlockCollInfo ebci = { .ebci_r18_r20 = amount32, .ebci_r24 = r24, .ebci_r26 = v5, .ebci_r28 = r28, .ebci_r30 = r30, .ebci_r32 = r32};
  uint16 v6 = 2 * (prod + (v5 >> 4));
  while (!(EnemyBlockCollReact_Horiz(&ebci, v6) & 1)) {
    v6 += room_width_in_blocks * 2;
    if ((--ebci.ebci_r28 & 0x8000) != 0) {
      AddToHiLo(&E->x_pos, &E->x_subpos, ebci.ebci_r18_r20);
      return 0;
    }
  }
  if (sign32(amount32)) {
    uint16 v12 = E->x_width + 1 + (ebci.ebci_r26 | 0xF);
    if (v12 <= E->x_pos)
      E->x_pos = v12;
    E->x_subpos = 0;
    return 1;
  } else {
    uint16 v10 = (ebci.ebci_r26 & 0xFFF0) - E->x_width;
    if (v10 >= E->x_pos)
      E->x_pos = v10;
    E->x_subpos = -1;
    return 1;
  }
}
uint8 Enemy_MoveDown(uint16 k, int32 amount32) {  // 0xA0C786
  int16 v6;
  uint16 v5;
  if (!amount32)
    return 0;
  EnemyData *E = gEnemyData(k);
  uint16 r28 = (uint16)(E->x_width + E->x_pos - 1 - ((E->x_pos - E->x_width) & 0xFFF0)) >> 4;
  uint16 r30 = r28;
  uint32 new_pos = amount32 + __PAIR32__(E->y_pos, E->y_subpos);
  uint16 r24 = new_pos >> 16;  // read by EnemyBlockCollVertReact_Slope_NonSquare
  if (sign32(amount32))
    v5 = (new_pos >> 16) - E->y_height;
  else
    v5 = E->y_height + (new_pos >> 16) - 1;
  uint16 prod = Mult8x8(v5 >> 4, room_width_in_blocks);
  v6 = (uint16)(E->x_pos - E->x_width) >> 4;
  EnemyBlockCollInfo ebci = { .ebci_r18_r20 = amount32, .ebci_r24 = r24, .ebci_r26 = v5, .ebci_r28 = r28, .ebci_r30 = r30, .ebci_r32 = 0 };
  for (int i = 2 * (prod + v6); !(EnemyBlockCollReact_Vert(&ebci, i) & 1); i += 2) {
    if ((--ebci.ebci_r28 & 0x8000) != 0) {
      E->y_subpos = new_pos, E->y_pos = new_pos >> 16;
      return 0;
    }
  }
  if (sign32(amount32)) {
    uint16 v13 = E->y_height + 1 + (ebci.ebci_r26 | 0xF);
    if (v13 <= E->y_pos)
      E->y_pos = v13;
    E->y_subpos = 0;
    return 1;
  } else {
    uint16 v10 = (ebci.ebci_r26 & 0xFFF0) - E->y_height;
    if (v10 >= E->y_pos)
      E->y_pos = v10;
    E->y_subpos = -1;
    return 1;
  }
}
static uint8 EnemyBlockCollReact_Horiz(EnemyBlockCollInfo *ebci, uint16 k) {
  cur_block_index = k >> 1;
  uint8 rv = 0;
  do {
    rv = off_A0C859[(level_get(cur_block_index) & 0xf000) >> 12](ebci);
  } while (rv & 0x80);
  return rv;
}
static uint8 EnemyBlockCollReact_Vert(EnemyBlockCollInfo *ebci, uint16 k) {
  cur_block_index = k >> 1;
  uint8 rv = 0;
  do {
    rv = off_A0C88D[(level_get(cur_block_index) & 0xf000) >> 12](ebci);
  } while (rv & 0x80);
  return rv;
}
void CalculateBlockContainingPixelPos(uint16 xpos, uint16 ypos) {
  uint16 prod = Mult8x8(ypos >> 4, room_width_in_blocks);
  cur_block_index = prod + (xpos >> 4);
}
uint8 EnemyFunc_C8AD(uint16 k) {  // 0xA0C8AD
  uint8 result = 0;

  EnemyData *E = gEnemyData(k);
  CalculateBlockContainingPixelPos(E->x_pos, E->y_pos + E->y_height - 1);
  if ((level_get(cur_block_index) & 0xF000) == 4096 && (bts_get(cur_block_index) & 0x1F) >= 5) {
    result = 1;
    uint16 temp_collision_DD4 = (E->y_height + E->y_pos - 1) & 0xF;
    uint16 temp_collision_DD6 = 16 * (bts_get(cur_block_index) & 0x1F);
    if ((bts_get(cur_block_index) & 0x80) == 0) {
      uint16 j = (bts_get(cur_block_index) & 0x40) != 0 ? E->x_pos ^ 0xF : E->x_pos;
      int16 v4 = (((uint8)BB_94(0x8B2B + (temp_collision_DD6 + (j & 0xF)))) & 0x1F) - temp_collision_DD4 - 1;
      if (v4 < 0)
        E->y_pos += v4;
    }
  }
  CalculateBlockContainingPixelPos(E->x_pos, E->y_pos - E->y_height);
  if ((level_get(cur_block_index) & 0xF000) == 4096 && (bts_get(cur_block_index) & 0x1F) >= 5) {
    result = 1;
    uint16 temp_collision_DD4 = (E->y_pos - E->y_height) & 0xF ^ 0xF;
    uint16 temp_collision_DD6 = 16 * (bts_get(cur_block_index) & 0x1F);
    if (bts_get(cur_block_index) & 0x80) {
      uint16 j = (bts_get(cur_block_index) & 0x40) != 0 ? E->x_pos ^ 0xF : E->x_pos;
      int16 v7 = (((uint8)BB_94(0x8B2B + (temp_collision_DD6 + (j & 0xF)))) & 0x1F) - temp_collision_DD4 - 1;
      if (v7 < 0)
        E->y_pos -= v7;
    }
  }
  return result;
}
void Enemy_NormalFrozenAI_A2(void) {  // 0xA28041
  NormalEnemyFrozenAI();
}
void Enemy_NormalTouchAI_A2(void) {  // 0xA28023
  NormalEnemyTouchAi();
}
void Enemy_NormalShotAI_A2(void) {  // 0xA2802D
  NormalEnemyShotAi();
}
void BouncingGoofball_Init(void) {  // 0xA2871C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->spritemap_pointer = addr_kSpritemap_Nothing_A2;
  BouncingGoofball_Func2();
  ERAM(0x7802 + E_k) = 1;
  E->ai_var_A = ((uint16)BW_A2(0x86DF + 2 * ((uint16)(2
                                                           * LOBYTE(E->parameter_1)) >> 1)));
  E->ai_var_C = ((uint16)BW_A2(0x86EF + 2 * ((uint16)(2
                                                           * HIBYTE(E->parameter_1)) >> 1)));
  E->ai_var_D = 0;
  E->ai_var_E = 0;
  E->ai_preinstr = 0;
}
void BouncingGoofball_Func1(uint16 k) {  // 0xA28755
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v5;
  do {
    uint16 bgl_var_E = E->ai_var_E;
    uint8 v3 = ((uint8)BB_A2(0x8701 + (bgl_var_E)));
    if ((int16)(bgl_var_E - 23) >= 0)
      v3 = -1;
    uint16 prod = Mult8x8(v3, E->ai_var_A);
    uint16 RegWord = prod;
    E->ai_var_B = RegWord;
    v5 = E->ai_var_D + RegWord;
    E->ai_var_D = v5;
    ++E->ai_var_E;
  } while ((int16)(v5 - E->ai_var_C) < 0);
  ERAM(0x7800 + E_k) = E->ai_var_E;
  ERAM(0x7804 + E_k) = 1;
  ERAM(0x7808 + E_k) = 1;
}
void BouncingGoofball_Main(void) {  // 0xA2879C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x780A + E_k)) {
    BouncingGoofball_Func1(cur_enemy_index);
    ERAM(0x780A + E_k) = 1;
    return;
  }
  ERAM(0x7806 + E_k) = E->ai_var_A;
  if (BouncingGoofball_SamusCloseX()) {
    ERAM(0x7804 + E_k) = 0;
    ERAM(0x7808 + E_k) = 0;
    if (!E->ai_preinstr) {
      E->ai_preinstr = 1;
      BouncingGoofball_Func3();
    }
    goto LABEL_9;
  }
  if (!ERAM(0x7804 + E_k)) {
LABEL_9:
    E->ai_var_A = ERAM(0x7806 + E_k);
    off_A28718[ERAM(0x7802 + E_k)]();
    return;
  }
  if (!ERAM(0x7808 + E_k)) {
    ERAM(0x7808 + E_k) = 1;
    BouncingGoofball_Func2();
  }
}
void BouncingGoofball_State0(void) {  // 0xA28801
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v1 = ++E->ai_var_E;
  uint8 v2 = ((uint8)BB_A2(0x8701 + (v1)));
  if ((int16)(v1 - 23) >= 0)
    v2 = -1;
  uint16 RegWord = Mult8x8(v2, E->ai_var_A);
  E->ai_var_B = RegWord;
  E->y_pos += HIBYTE(RegWord);
  if ((int16)(E->ai_var_E - ERAM(0x7800 + E_k)) >= 0) {
    ERAM(0x7802 + E_k) = 1;
    ERAM(0x7804 + E_k) = 1;
    E->ai_preinstr = 0;
  }
}
void BouncingGoofball_State1(void) {  // 0xA28850
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 bgl_var_E = E->ai_var_E;
  uint8 v2 = ((uint8)BB_A2(0x8701 + (bgl_var_E)));
  if ((int16)(bgl_var_E - 23) >= 0)
    v2 = -1;
  uint16 RegWord = Mult8x8(v2, E->ai_var_A);
  E->ai_var_B = RegWord;
  E->y_pos -= HIBYTE(RegWord);
  if ((--E->ai_var_E & 0x8000) != 0)
    ERAM(0x7802 + E_k) = 0;
}
uint16 BouncingGoofball_SamusCloseX(void) {  // 0xA28894
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  return IsSamusWithinEnemy_X(cur_enemy_index, E->parameter_2);
}
void BouncingGoofball_Func2(void) {  // 0xA2889F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kBouncingGoofball_Ilist_86A7;
  E->instruction_timer = 1;
  E->timer = 0;
}
void BouncingGoofball_Func3(void) {  // 0xA288B2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kBouncingGoofball_Ilist_86BF;
  E->instruction_timer = 1;
  E->timer = 0;
}
uint16 BouncingGoofball_Instr_88C5(uint16 k, uint16 jp) {  // 0xA288C5
  return jp;
}
uint16 BouncingGoofball_Instr_88C6(uint16 k, uint16 jp) {  // 0xA288C6
  ERAM(0x7804 + (cur_enemy_index)) = 0;
  QueueSfx2_Max6(0xE);
  return jp;
}
void ThinHoppingBlobs_Init(void) {  // 0xA29A3F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->spritemap_pointer = addr_kSpritemap_Nothing_A2;
  E->ai_var_A = 0;
  sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99AD);
  ERAM(0x7800 + E_k) = 0;
  E->ai_var_D = 0x9B65 /* ThinHoppingBlobs_Func6 */;
  E->ai_var_C = E->parameter_1;
  ERAM(0x7808 + E_k) = 0;
}
void sub_A29A6C(uint16 a) {  // 0xA29A6C
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  v1->current_instruction = a;
  v1->instruction_timer = 1;
  v1->timer = 0;
}
void CallThinHoppingBlobs(uint32 ea) {
  switch (ea) {
  case 0xA29B65: ThinHoppingBlobs_Func6(cur_enemy_index); return;
  case 0xA29B81: ThinHoppingBlobs_Func7(); return;
  default: nt_unimpl("Unreachable");
  }
}
void ThinHoppingBlobs_Main(void) {  // 0xA29A7D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallThinHoppingBlobs(E->ai_var_D | 0xA20000);
}
void ThinHoppingBlobs_Func1(uint16 k) {  // 0xA29A84
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (sign16(ERAM(0x7800 + E_k) - 3))
    TwinHoppingBlobs_Func2();
  ThinHoppingBlobs_Func3();
  ThinHoppingBlobs_Func5();
}
void TwinHoppingBlobs_Func2(void) {  // 0xA29A9B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = IsSamusWithinEnemy_X(cur_enemy_index, E->parameter_2);
}
void ThinHoppingBlobs_Func3(void) {  // 0xA29AAA
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7804 + E_k) = 1;
  if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) == 0)
    ERAM(0x7804 + E_k) = 0;
  if (ERAM(0x7808 + E_k))
    ERAM(0x7804 + E_k) = ERAM(0x780A + E_k);
  ERAM(0x7808 + E_k) = 0;
  uint16 R28 = ThinHoppingBlobs_Func4();
  uint16 thbs_var_00 = ERAM(0x7800 + E_k);
  if (sign16(thbs_var_00 - 3)) {
    if (!thbs_var_00)
      R28 &= 1;
    thbs_var_00 = R28;
    if (!sign16(R28 - 2))
      thbs_var_00 = 2;
  }
  uint16 v3 = 8 * thbs_var_00;
  E->ai_preinstr = v3;
  E->ai_var_E = ((uint16)BW_A2(0x9A0D + (v3)));
}
uint16 ThinHoppingBlobs_Func4(void) {  // 0xA29B06
  NextRandom();
  return (gEnemyData(cur_enemy_index)->frame_counter + random_number) & 7;
}
void ThinHoppingBlobs_Func5(void) {  // 0xA29B1A
  uint16 v2;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 thbs_var_F = E->ai_preinstr;
  uint16 r22 = 0;
  uint16 r24 = 0;
  do {
    r22 += ((uint16)BW_A2(0x9A09 + (thbs_var_F)));
    r24 += ((uint16)BW_A2(0x838F + ((8 * ((r22 & 0xFF00) >> 8)) + 1)));
    thbs_var_F = E->ai_preinstr;
    v2 = swap16(((uint16)BW_A2(0x9A07 + (thbs_var_F))));
  } while (!sign16(v2 - r24));
  E->ai_var_B = r22;
  ERAM(0x7806 + E_k) = 0;
  uint16 v3 = r22 >> 1;
  ERAM(0x780E + E_k) = r22 >> 1;
  ERAM(0x780C + E_k) = ERAM(0x780E + E_k) + (v3 >> 1);
}
void ThinHoppingBlobs_Func6(uint16 k) {  // 0xA29B65
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((--E->ai_var_C & 0x8000) != 0) {
    E->ai_var_D = 0x9B81 /* ThinHoppingBlobs_Func7 */;
    E->ai_var_C = E->parameter_1;
    ERAM(0x7802 + E_k) = 1;
    ThinHoppingBlobs_Func1(k);
  }
}
void CallThinHoppingBlobsVarE(uint32 ea) {
  switch (ea) {
  case 0xA29D0B: ThinHoppingBlobs_Func11(); return;
  case 0xA29D2B: ThinHoppingBlobs_Func12(); return;
  case 0xA29D4B: ThinHoppingBlobs_Func13(); return;
  case 0xA29D6B: ThinHoppingBlobs_Func14(); return;
  case 0xA29D98: ThinHoppingBlobs_Func15(); return;
  case 0xA29DCD: ThinHoppingBlobs_Func16(); return;
  default: nt_unimpl("Unreachable");
  }
}
void ThinHoppingBlobs_Func7(void) {  // 0xA29B81
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallThinHoppingBlobsVarE(E->ai_var_E | 0xA20000);
}
void ThinHoppingBlobs_Func8(void) {  // 0xA29B88
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 thbs_var_B = E->ai_var_B;
  if (!sign16(thbs_var_B - 0x4000))
    thbs_var_B = 0x4000;
  uint16 v6 = 8 * (thbs_var_B >> 8);
  if (!ERAM(0x7806 + E_k))
    v6 += 4;
  if (Enemy_MoveDown(cur_enemy_index, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v6 >> 2)) | (uint32)BW_A0(0xCBC7 + 4 * (v6 >> 2) + 2) << 16)))) {
    if (ERAM(0x7806 + E_k)) {
      E->ai_var_D = 0x9B65 /* ThinHoppingBlobs_Func6 */;
    } else {
      ERAM(0x7808 + E_k) = 1;
      ERAM(0x780A + E_k) = ERAM(0x7804 + E_k) ^ 1;
      ERAM(0x7800 + E_k) = 4;
      E->ai_var_E = 0x9D98 /* ThinHoppingBlobs_Func15 */;
    }
    ERAM(0x7802 + E_k) = 0;
  } else {
    uint16 thbs_var_F = E->ai_preinstr;
    if (ERAM(0x7806 + E_k)) {
      ThinHoppingBlobs_Func10();
      E->ai_var_B += ((uint16)BW_A2(0x9A0B + (thbs_var_F)));
    } else {
      ThinHoppingBlobs_Func9();
      E->ai_var_B -= ((uint16)BW_A2(0x9A0B + (thbs_var_F)));
    }
    if (sign16(E->ai_var_B)) {
      ERAM(0x7806 + E_k) = 1;
      E->ai_var_B = 0;
    }
    uint16 R20 = (((uint16)BW_A2(0x9A09 + (E->ai_preinstr))) & 0xFF00) >> 8;
    if (ERAM(0x7804 + E_k))
      R20 = -R20;
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(R20))) {
      ERAM(0x7808 + E_k) = 1;
      ERAM(0x780A + E_k) = ERAM(0x7804 + E_k) ^ 1;
      ERAM(0x7802 + E_k) = 0;
      ERAM(0x7800 + E_k) = 4;
      E->ai_var_E = 0x9D98 /* ThinHoppingBlobs_Func15 */;
    }
  }
}
void ThinHoppingBlobs_Func9(void) {  // 0xA29C71
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v2;
  if (ERAM(0x7804 + E_k)) {
    uint16 thbs_var_B = E->ai_var_B;
    if ((int16)(thbs_var_B - ERAM(0x780C + E_k)) >= 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_9A01;
    } else if ((int16)(thbs_var_B - ERAM(0x780E + E_k)) >= 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99FB;
    } else {
      v2 = addr_kThinHoppingBlobs_Ilist_99F5;
    }
  } else {
    uint16 v1 = E->ai_var_B;
    if ((int16)(v1 - ERAM(0x780C + E_k)) >= 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99E9;
    } else if ((int16)(v1 - ERAM(0x780E + E_k)) >= 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99EF;
    } else {
      v2 = addr_kThinHoppingBlobs_Ilist_99F5;
    }
  }
  sub_A29A6C(v2);
}
void ThinHoppingBlobs_Func10(void) {  // 0xA29CBE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v2;
  if (ERAM(0x7804 + E_k)) {
    uint16 thbs_var_B = E->ai_var_B;
    if ((int16)(thbs_var_B - ERAM(0x780E + E_k)) < 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99F5;
    } else if ((int16)(thbs_var_B - ERAM(0x780C + E_k)) < 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99EF;
    } else {
      v2 = addr_kThinHoppingBlobs_Ilist_99E9;
    }
  } else {
    uint16 v1 = E->ai_var_B;
    if ((int16)(v1 - ERAM(0x780E + E_k)) < 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99F5;
    } else if ((int16)(v1 - ERAM(0x780C + E_k)) < 0) {
      v2 = addr_kThinHoppingBlobs_Ilist_99FB;
    } else {
      v2 = addr_kThinHoppingBlobs_Ilist_9A01;
    }
  }
  sub_A29A6C(v2);
}
void ThinHoppingBlobs_Func11(void) {  // 0xA29D0B

  ThinHoppingBlobs_Func8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7808 + E_k) || !ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99D5);
  }
}
void ThinHoppingBlobs_Func12(void) {  // 0xA29D2B

  ThinHoppingBlobs_Func8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7808 + E_k) || !ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99C1);
  }
}
void ThinHoppingBlobs_Func13(void) {  // 0xA29D4B

  ThinHoppingBlobs_Func8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7808 + E_k) || !ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99AD);
  }
}
void ThinHoppingBlobs_Func14(void) {  // 0xA29D6B

  ThinHoppingBlobs_Func8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x7808 + E_k)) {
    if (ERAM(0x7802 + E_k))
      return;
    ERAM(0x7800 + E_k) = 0;
    E->ai_var_D = 0x9B65 /* ThinHoppingBlobs_Func6 */;
  }
  ERAM(0x7802 + E_k) = 0;
  sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99D5);
}
void ThinHoppingBlobs_Func15(void) {  // 0xA29D98
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 thbs_var_F = E->ai_preinstr;
  uint16 R20 = (((uint16)BW_A2(0x9A0B + (thbs_var_F))) & 0xFF00) >> 8;
  uint16 R18 = BB_A2((uint16)(0x9A0B + (thbs_var_F))) << 8;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(R20, R18))) {
    uint16 R28 = ThinHoppingBlobs_Func4();
    ERAM(0x7800 + E_k) = (R28 & 1) + 5;
    E->ai_var_D = 0x9B65 /* ThinHoppingBlobs_Func6 */;
  }
}
void ThinHoppingBlobs_Func16(void) {  // 0xA29DCD

  ThinHoppingBlobs_Func8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    ERAM(0x7800 + E_k) = 3;
    E->ai_var_D = 0x9B65 /* ThinHoppingBlobs_Func6 */;
    sub_A29A6C(addr_kThinHoppingBlobs_Ilist_99D5);
  }
}
uint16 SpikeShootingPlant_Instr_9F2A(uint16 k, uint16 jp) {  // 0xA29F2A
  QueueSfx2_Max6(0x34);
  return jp;
}
void SpikeShootingPlant_Init(void) {  // 0xA29F48
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->spritemap_pointer = addr_kSpritemap_Nothing_A2;
  if (HIBYTE(E->parameter_1))
    SpikeShootingPlant_5();
  else
    SpikeShootingPlant_7();
  uint16 sspt_parameter_1_low = LOBYTE(E->parameter_1);
  E->ai_var_E = sspt_parameter_1_low;
  E->ai_preinstr = ((uint16)BW_A2(0x9F42 + 2 * (sspt_parameter_1_low)));
  int v2 = LOBYTE(E->parameter_2);
  ERAM(0x7802 + E_k) = ((uint16)BW_A2(0x9F36 + 2 * (v2))) + E->x_pos;
  ERAM(0x7800 + E_k) = E->x_pos - ((uint16)BW_A2(0x9F36 + 2 * (v2)));
  int v3 = (8 * HIBYTE(E->parameter_2)) >> 1;
  E->ai_var_B = ((uint16)BW_A2(0x8187 + 2 * (v3)));
  E->ai_var_A = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
  E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
  E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
}
void SpikeShootingPlant_Main(void) {  // 0xA29FB3
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xA20000);
}
void SpikeShootingPlant_2(uint16 k) {  // 0xA29FBA
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(E->ai_var_D, E->ai_var_C));
  if ((int16)(E->x_pos - ERAM(0x7800 + E_k)) < 0) {
    E->ai_preinstr = 0x9FEC /* SpikeShootingPlant_3 */;
    E->ai_var_E = 1;
  }
  SpikeShootingPlant_4();
}
void SpikeShootingPlant_3(uint16 k) {  // 0xA29FEC
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(E->ai_var_B, E->ai_var_A));
  if ((int16)(E->x_pos - ERAM(0x7802 + E_k)) >= 0) {
    E->ai_preinstr = 0x9FBA /* SpikeShootingPlant_2 */;
    E->ai_var_E = 0;
  }
  SpikeShootingPlant_4();
}
void SpikeShootingPlant_4(void) {  // 0xA2A01C

  NextRandom();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (sign16((uint8)(LOBYTE(E->frame_counter) + random_number) - 3)) {
    E->ai_preinstr = 0xA01B /* nullsub_182 */;
    if (HIBYTE(E->parameter_1))
      SpikeShootingPlant_6();
    else
      SpikeShootingPlant_8();
  }
}
void SpikeShootingPlant_5(void) {  // 0xA2A049
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kSpikeShootingPlant_Ilist_9E8A;
  E->instruction_timer = 1;
  E->timer = 0;
}
void SpikeShootingPlant_6(void) {  // 0xA2A05C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kSpikeShootingPlant_Ilist_9EB0;
  E->instruction_timer = 1;
  E->timer = 0;
}
void SpikeShootingPlant_7(void) {  // 0xA2A06F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kSpikeShootingPlant_Ilist_9EDA;
  E->instruction_timer = 1;
  E->timer = 0;
}
void SpikeShootingPlant_8(void) {  // 0xA2A082
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kSpikeShootingPlant_Ilist_9F00;
  E->instruction_timer = 1;
  E->timer = 0;
}
uint16 SpikeShootingPlant_Instr_A095(uint16 k, uint16 jp) {  // 0xA2A095
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_preinstr = 0x9FBA /* SpikeShootingPlant_2 */;
  if (E->ai_var_E)
    E->ai_preinstr = 0x9FEC /* SpikeShootingPlant_3 */;
  return jp;
}
uint16 SpikeShootingPlant_Instr_A0A7(uint16 k, uint16 jp) {  // 0xA2A0A7
  SpawnEprojWithGfx(IW_A2(jp), cur_enemy_index, addr_kEproj_SpikeShootingPlantSpikes);
  return (uint16)(jp + 2);
}
void Flies_Init(void) {  // 0xA2B06B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_E = 0;
  E->ai_preinstr = 0xB14E /* Flies_4 */;
  E->current_instruction = addr_kFlies_Ilist_B013;
  E->spritemap_pointer = addr_kSpritemap_Nothing_A2;
  E->instruction_timer = 1;
  E->spritemap_pointer = addr_kSpritemap_Nothing_A2;
}
bool CarryAdd16(uint16 *rv, uint16 v) {
  int i = *rv + v;
  *rv = i;
  return (i >> 16);
}
void Flies_1(uint16 k) {  // 0xA2B090
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = E->ai_var_E >> 1;
  bool carry = CarryAdd16(&E->x_subpos, LOBYTE(kSinCosTable8bit_Sext[v2 + 64]) << 8);
  E->x_pos += (int8)HIBYTE(kSinCosTable8bit_Sext[v2 + 64]) + carry;
  carry = CarryAdd16(&E->y_subpos, LOBYTE(kSinCosTable8bit_Sext[v2]) << 8);
  E->y_pos += (int8)HIBYTE(kSinCosTable8bit_Sext[v2]) + carry;
}
uint16 Flies_2(uint16 k) {  // 0xA2B0DC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool carry = CarryAdd16(&E->x_subpos, LOBYTE(E->ai_var_B) << 8);
  E->x_pos += (int8)HIBYTE(E->ai_var_B) + carry;
  carry = CarryAdd16(&E->y_subpos, LOBYTE(E->ai_var_C) << 8);
  uint16 result = E->y_pos + (int8)HIBYTE(E->ai_var_C) + carry;
  E->y_pos = result;
  return result;
}
void Flies_Main(void) {  // 0xA2B11F
  NextRandom();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xA20000);
}
void Flies_3(uint16 k) {  // 0xA2B129
  int v1 = CalculateAngleOfSamusFromEnemy(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_B = 2 * kSinCosTable8bit_Sext[v1 + 64];
  E->ai_var_C = 4 * kSinCosTable8bit_Sext[v1];
  E->ai_var_D = samus_y_pos;
  E->ai_preinstr = 0xB1AA /* Flies_6 */;
}
void Flies_4(uint16 k) {  // 0xA2B14E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 flies_var_A = E->ai_var_A;
  if (flies_var_A) {
    E->ai_var_A = flies_var_A - 1;
  } else if (!(CompareDistToSamus_X(k, 0x70) & 1)) {
    Flies_3(k);
    return;
  }
  Flies_1(k);
  bool v3 = ((E->ai_var_E + 32) & 0x1FF) == 0;
  E->ai_var_E = (E->ai_var_E + 32) & 0x1FF;
  if (v3)
    E->ai_preinstr = 0xB17C /* Flies_5 */;
}
void Flies_5(uint16 k) {  // 0xA2B17C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 flies_var_A = E->ai_var_A;
  if (flies_var_A) {
    E->ai_var_A = flies_var_A - 1;
  } else if (!(CompareDistToSamus_X(k, 0x70) & 1)) {
    Flies_3(k);
    return;
  }
  Flies_1(k);
  bool v3 = ((E->ai_var_E - 32) & 0x1FF) == 0;
  E->ai_var_E = (E->ai_var_E - 32) & 0x1FF;
  if (v3)
    E->ai_preinstr = 0xB14E /* Flies_4 */;
}
void Flies_6(uint16 k) {  // 0xA2B1AA
  uint16 v1 = Flies_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ++E->ai_var_A;
  if ((E->ai_var_C & 0x8000) == 0) {
    if (v1 < E->ai_var_D)
      return;
    goto LABEL_5;
  }
  if (v1 < E->ai_var_D) {
LABEL_5:
    E->ai_var_C = -E->ai_var_C;
    E->ai_preinstr = 0xB1D2 /* Flies_7 */;
  }
}
void Flies_7(uint16 k) {  // 0xA2B1D2
  Flies_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((--E->ai_var_A & 0x8000) != 0) {
    E->ai_var_A = 24;
    E->ai_preinstr = 0xB14E /* Flies_4 */;
  }
}
uint16 EnemyInstr_Rio_Instr_1(uint16 k, uint16 jp) {  // 0xA2BBC3
  gEnemyData(cur_enemy_index)->ai_var_E = 1;
  return jp;
}
void Rio_Init(void) {  // 0xA2BBCD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_E = 0;
  E->ai_preinstr = 0;
  E->current_instruction = addr_kRio_Ilist_BB4B;
  E->ai_var_B = 0xBBED /* Rio_1 */;
}
void CallRioFunc(uint32 ea) {
  uint16 k = cur_enemy_index;
  switch (ea) {
  case 0xA2BBED: Rio_1(k); return;  // 0x7c30a
  case 0xA2BC32: Rio_2(k); return;  // 0x7c396
  case 0xA2BC48: Rio_3(k); return;  // 0x7c3d4
  case 0xA2BCB7: Rio_4(k); return;  // 0x7c4dc
  case 0xA2BCFF: Rio_5(k); return;  // 0x7c599
  default: nt_unimpl("Unreachable");
  }
}
void Rio_Main(void) {  // 0xA2BBE3
  NextRandom();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallRioFunc(E->ai_var_B | 0xA20000);
}
void Rio_1(uint16 k) {  // 0xA2BBED
  if (!(CompareDistToSamus_X(k, 0xA0) & 1)) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->ai_var_C = ((uint16)BW_A2(0xBBBB + 2 * (0)));
    E->ai_var_D = ((uint16)BW_A2(0xBBBF + 2 * (0)));
    if ((int16)(samus_x_pos - E->x_pos) < 0)
      E->ai_var_D = -E->ai_var_D;
    Rio_6(addr_kRio_Ilist_BB7F);
    E->ai_var_B = 0xBC48 /* Rio_3 */;
    if (!CheckIfEnemyIsOnScreen())
      QueueSfx2_Max6(0x65);
  }
}
void Rio_2(uint16 k) {  // 0xA2BC32
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_E) {
    E->ai_var_E = 0;
    Rio_6(addr_stru_A2BB53);
    E->ai_var_B = 0xBBED /* Rio_1 */;
  }
}
void Rio_3(uint16 k) {  // 0xA2BC48
  int16 v4;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ;
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL8(E->ai_var_D))) {
    E->ai_var_D = -E->ai_var_D;
LABEL_13:
    E->ai_var_C = -E->ai_var_C;
    E->ai_var_B = 0xBCB7 /* Rio_4 */;
    return;
  }
  if (Enemy_MoveDown(k, INT16_SHL8(E->ai_var_C)))
    goto LABEL_13;
  v4 = E->ai_var_C - 24;
  E->ai_var_C = v4;
  if (v4 < 0) {
    E->ai_var_A = E->ai_var_D;
    E->ai_var_D = 0;
    E->ai_var_C = 0;
    E->ai_var_B = 0xBCFF /* Rio_5 */;
  } else if (E->ai_var_E) {
    E->ai_var_E = 0;
    Rio_6(addr_kRio_Ilist_BB97);
  }
}
void Rio_4(uint16 k) {  // 0xA2BCB7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL8(E->ai_var_D)))
    E->ai_var_D = -E->ai_var_D;
  if (Enemy_MoveDown(k, INT16_SHL8(E->ai_var_C))) {
    Rio_6(addr_kRio_Ilist_BBA3);
    E->ai_var_B = 0xBC32 /* Rio_2 */;
  } else {
    E->ai_var_C -= 24;
  }
}
void Rio_5(uint16 k) {  // 0xA2BCFF
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((int16)(gEnemyData(k)->y_pos - samus_y_pos) >= 0) {
    E->ai_var_D = E->ai_var_A;
    E->ai_var_C = -1;
    E->ai_var_B = 0xBCB7 /* Rio_4 */;
  } else {
    int v1 = CalculateAngleOfSamusFromEnemy(k);
    E->ai_var_D = kSinCosTable8bit_Sext[v1 + 64];
    E->ai_var_C = kSinCosTable8bit_Sext[v1];
    Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL8(E->ai_var_D));
    Enemy_MoveDown(k, INT16_SHL8(E->ai_var_C));
  }
}
void Rio_6(uint16 a) {  // 0xA2BD54
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (a != E->ai_preinstr) {
    E->ai_preinstr = a;
    E->current_instruction = a;
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
static uint16 HirisingSlowfalling_Func_1(uint16 k) {  // 0xA2DFCE
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  return Mult8x8(E->parameter_1, E->ai_preinstr);
}
void HirisingSlowfalling_Init(void) {  // 0xA2DF76
  HirisingSlowfalling_Func_3(addr_kHirisingSlowfalling_Ilist_D82C);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = 0xE035 /* HirisingSlowfalling_Func_4 */;
  ERAM(0x7800 + E_k) = E->x_pos;
  ERAM(0x7802 + E_k) = E->y_pos;
  int v2 = HIBYTE(E->parameter_1);
  E->ai_var_E = ((uint16)BW_A2(0xDF5E + 2 * (v2)));
  E->ai_preinstr = BW_A2(((uint16)BW_A2(0xDF6A + 2 * (v2))));
  uint16 r18 = HirisingSlowfalling_Func_1(cur_enemy_index);
  HirisingSlowfalling_Func_2(cur_enemy_index, r18);
  ERAM(0x7806 + E_k) = ERAM(0x7802 + E_k) - r18;
  ERAM(0x7804 + E_k) = ERAM(0x7800 + E_k);
  E->ai_var_B = ERAM(0x780C + E_k);
}
void HirisingSlowfalling_Func_2(uint16 k, uint16 r18) {  // 0xA2DFE9
  uint32 v = 0;
  uint16 r24 = 0;
  do {
    r24 += 512;
    int v1 = 4 * (r24 >> 8);
    v += __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v1 + 1))), ((uint16)BW_A2(0x838F + 2 * (v1))));
  } while (sign16((v >> 16) - r18));
  ERAM(0x780C + (k)) = r24;
}
void HirisingSlowfalling_Func_3(uint16 a) {  // 0xA2E01E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = a;
  E->instruction_timer = 1;
  E->timer = 0;
}
void CallHirisingSlowfalling(uint32 ea) {
  switch (ea) {
  case 0xA2E035: HirisingSlowfalling_Func_4(); return;
  case 0xA2E04F: HirisingSlowfalling_Func_5(); return;
  case 0xA2E06A: HirisingSlowfalling_Func_6(); return;
  case 0xA2E0CD: HirisingSlowfalling_Func_7(); return;
  default: nt_unimpl("Unreachable");
  }
}
void HirisingSlowfalling_Main(void) {  // 0xA2E02E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallHirisingSlowfalling(E->ai_var_A | 0xA20000);
}
void HirisingSlowfalling_Func_4(void) {  // 0xA2E035
  if (IsSamusWithinEnemy_X(cur_enemy_index, 0x50)) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    ERAM(0x780E + E_k) = E->parameter_2;
    E->ai_var_A = 0xE04F /* HirisingSlowfalling_Func_5 */;
  }
}
void HirisingSlowfalling_Func_5(void) {  // 0xA2E04F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int16 v1 = ERAM(0x780E + E_k) - 1;
  ERAM(0x780E + E_k) = v1;
  if (v1 < 0) {
    HirisingSlowfalling_Func_3(addr_kHirisingSlowfalling_Ilist_D834);
    E->ai_var_A = 0xE06A /* HirisingSlowfalling_Func_6 */;
  }
}
void HirisingSlowfalling_Func_6(void) {  // 0xA2E06A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = 4 * HIBYTE(E->ai_var_B);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 3))), ((uint16)BW_A2(0x838F + 2 * (v2 + 2)))));
  int16 v5 = E->ai_var_B - 512;
  E->ai_var_B = v5;
  if (v5 < 0) {
    E->x_pos = ERAM(0x7808 + E_k) = ERAM(0x7804 + E_k);
    E->y_pos = ERAM(0x780A + E_k) = ERAM(0x7806 + E_k);
    E->ai_var_C = 0;
    E->ai_var_D = LOBYTE(E->parameter_1) - 1;
    HirisingSlowfalling_Func_3(addr_kHirisingSlowfalling_Ilist_D840);
    E->ai_var_A = 0xE0CD /* HirisingSlowfalling_Func_7 */;
  }
}
void HirisingSlowfalling_Func_7(void) {  // 0xA2E0CD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v1 = (E->ai_var_E + 4 * HIBYTE(E->ai_var_C));
  if (BW_A2(v1) == 0x8000) {
    ERAM(0x780A + E_k) += E->ai_preinstr;
    E->ai_var_C = 0;
    if ((--E->ai_var_D & 0x8000) != 0) {
      E->ai_var_B = ERAM(0x780C + E_k);
      E->x_pos = ERAM(0x7800 + E_k);
      E->x_subpos = 0;
      E->y_pos = ERAM(0x7802 + E_k);
      E->y_subpos = 0;
      HirisingSlowfalling_Func_3(addr_kHirisingSlowfalling_Ilist_D82C);
      E->ai_var_A = 0xE035 /* HirisingSlowfalling_Func_4 */;
    }
  } else {
    E->x_pos = BW_A2(v1) + ERAM(0x7808 + E_k);
    E->y_pos = BW_A2(v1 + 2) + ERAM(0x780A + E_k);
    E->ai_var_C += 256;
  }
}
void Gripper_Init(void) {  // 0xA2E1D3
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v1 = 8 * LOBYTE(E->current_instruction);
  E->ai_var_E = v1;
  int v2 = v1 >> 1;
  uint16 v3;
  if ((E->current_instruction & 0xFEFF) != 0) {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2)));
    v3 = ((uint16)BW_A2(0x8187 + 2 * (v2 + 1)));
  } else {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2 + 2)));
    v3 = ((uint16)BW_A2(0x8187 + 2 * (v2 + 3)));
  }
  E->ai_var_C = v3;
  uint16 v4 = addr_kGripper_Ilist_E19B;
  if ((E->ai_var_D & 0x8000) == 0)
    v4 = addr_kGripper_Ilist_E1AF;
  E->current_instruction = v4;
  E->ai_var_A = E->parameter_1;
  E->ai_var_B = E->parameter_2;
}
void Gripper_Main(void) {  // 0xA2E221
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_D, E->ai_var_C)) ||
      Gripper_Func_1(cur_enemy_index) & 1 || Gripper_Func_2(cur_enemy_index) & 1) {
    int v3 = E->ai_var_E >> 1;
    uint16 v4;
    if ((E->ai_var_D & 0x8000) == 0) {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
      v4 = addr_kGripper_Ilist_E19B;
    } else {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
      v4 = addr_kGripper_Ilist_E1AF;
    }
    E->current_instruction = v4;
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
uint8 Gripper_Func_1(uint16 k) {  // 0xA2E279
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  return (E->ai_var_D & 0x8000) != 0 && (int16)(E->x_pos - E->ai_var_A) < 0;
}
uint8 Gripper_Func_2(uint16 k) {  // 0xA2E28A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  return (E->ai_var_D & 0x8000) == 0 && (int16)(E->x_pos - E->ai_var_B) >= 0;
}
void Gripper_Func_3(void) {  // 0xA2E29B
  Enemy_NormalFrozenAI_A2();
}
void Gripper_Func_4(void) {  // 0xA2E2A4
  NormalEnemyShotAi();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->frozen_timer) {
    uint16 v1 = addr_kGripper_Sprmap_E43F;
    if ((E->ai_var_D & 0x8000) == 0)
      v1 = addr_kGripper_Sprmap_E44B;
    E->spritemap_pointer = v1;
  }
}
void JetPowerRipper_Init(void) {  // 0xA2E318
  uint16 v0 = addr_kRipper_Ilist_E2E0;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->parameter_2)
    v0 = addr_kRipper_Ilist_E2F4;
  E->current_instruction = v0;
  uint16 v2 = 8 * E->parameter_1;
  E->ai_var_E = v2;
  int v3 = v2 >> 1;
  uint16 v4;
  if (E->parameter_2) {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3)));
    v4 = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
  } else {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
    v4 = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
  }
  E->ai_var_C = v4;
}
void JetPowerRipper_Main(void) {  // 0xA2E353
  int16 v3;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_D, E->ai_var_C))) {
    int v2 = E->ai_var_E >> 1;
    if ((E->ai_var_D & 0x8000) == 0) {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2 + 2)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v2 + 3)));
      v3 = -7456;
    } else {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v2 + 1)));
      v3 = -7436;
    }
    E->current_instruction = v3;
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
void Ripper_Func_1(void) {  // 0xA2E3A0
  Enemy_NormalFrozenAI_A2();
}
void JetPowerRipper_Shot(void) {  // 0xA2E3A9
  int16 v1;

  NormalEnemyShotAi();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->frozen_timer) {
    v1 = -7105;
    if ((E->ai_var_D & 0x8000) == 0)
      v1 = -7093;
    E->spritemap_pointer = v1;
  }
}
void Ripper_Init(void) {  // 0xA2E49F
  uint16 v0 = addr_kRipper_Ilist_E477;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->parameter_2)
    v0 = addr_kRipper_Ilist_E48B;
  E->current_instruction = v0;
  uint16 v2 = 8 * E->parameter_1;
  E->ai_var_E = v2;
  int v3 = v2 >> 1;
  uint16 v4;
  if (E->parameter_2) {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3)));
    v4 = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
  } else {
    E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
    v4 = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
  }
  E->ai_var_C = v4;
}
void Ripper_Main(void) {  // 0xA2E4DA
  int16 v3;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_D, E->ai_var_C))) {
    int v2 = E->ai_var_E >> 1;
    if ((E->ai_var_D & 0x8000) == 0) {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2 + 2)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v2 + 3)));
      v3 = -7029;
    } else {
      E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v2)));
      E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v2 + 1)));
      v3 = -7049;
    }
    E->current_instruction = v3;
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
void TimedShutter_Init(void) {  // 0xA2E9DA
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = ((uint16)BW_A2(0xEA4E + 2 * (E->current_instruction + 2 * E->extra_properties)));
  uint16 y_pos = E->y_pos;
  uint16 v3;
  if (E->extra_properties) {
    E->ai_var_B = y_pos;
    uint16 v2 = y_pos - 8;
    E->ai_var_C = v2;
    v2 -= 8;
    E->ai_var_D = v2;
    v3 = v2 - 8;
  } else {
    E->ai_var_B = y_pos;
    uint16 v4 = y_pos + 8;
    E->ai_var_C = v4;
    v4 += 8;
    E->ai_var_D = v4;
    v3 = v4 + 8;
  }
  E->ai_var_E = v3;
  E->extra_properties = 0;
  E->ai_preinstr = 0;
  E->current_instruction = addr_kTimedShutter_Ilist_E998;
  int v5 = (uint16)(4 * LOBYTE(E->parameter_2)) >> 1;
  ERAM(0x7800 + E_k) = ((uint16)BW_A2(0xEA56 + 2 * (v5)));
  ERAM(0x7802 + E_k) = ((uint16)BW_A2(0xEA56 + 2 * (v5 + 1)));
}
void CallTimedShutterFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xA2EABD: TimedShutter_Func_1(k); return;
  case 0xA2EAD1: TimedShutter_Func_2(k); return;
  case 0xA2EAE7: TimedShutter_Func_3(k); return;
  case 0xA2EAFD: TimedShutter_Func_4(k); return;
  case 0xA2EB11: TimedShutter_Func_5(k); return;
  case 0xA2EC13: TimedShutter_Func_10(k); return;
  default: nt_unimpl("Unreachable");
  }
}
void TimedShutter_Main(void) {  // 0xA2EAB6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallTimedShutterFunc(E->ai_var_A | 0xA20000, cur_enemy_index);
}
void TimedShutter_Func_1(uint16 k) {  // 0xA2EABD
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 tsr_parameter_1 = E->parameter_1;
  if (tsr_parameter_1) {
    E->parameter_1 = tsr_parameter_1 - 1;
  } else {
    RisingFallingPlatform_Func_8();
    E->ai_var_A = 0xEC13 /* TimedShutter_Func_10 */;
  }
}
void TimedShutter_Func_2(uint16 k) {  // 0xA2EAD1
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (IsSamusWithinEnemy_X(k, E->parameter_1)) {
    RisingFallingPlatform_Func_8();
    E->ai_var_A = 0xEC13 /* TimedShutter_Func_10 */;
  }
}
void TimedShutter_Func_3(uint16 k) {  // 0xA2EAE7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (IsSamusWithinEnemy_X(k, E->parameter_1)) {
    RisingFallingPlatform_Func_8();
    E->ai_var_A = 0xEB11 /* TimedShutter_Func_5 */;
  }
}
void TimedShutter_Func_4(uint16 k) {  // 0xA2EAFD
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 tsr_parameter_1 = E->parameter_1;
  if (tsr_parameter_1) {
    E->parameter_1 = tsr_parameter_1 - 1;
  } else {
    RisingFallingPlatform_Func_8();
    E->ai_var_A = 0xEB11 /* TimedShutter_Func_5 */;
  }
}
void TimedShutter_Func_Null(void) {
  ;
}
void TimedShutter_Func_5(uint16 k) {  // 0xA2EB11
  int v1 = gEnemyData(k)->ai_preinstr;
  off_A2EB1A[v1]();
}
void TimedShutter_Func_6(void) {  // 0xA2EB25
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  if ((int16)(E->ai_var_B + 16 - E->y_pos) < 0) {
    E->y_pos = E->ai_var_B + 9;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E99E;
    E->y_height = 16;
  }
}
void TimedShutter_Func_7(void) {  // 0xA2EB66
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  if ((int16)(E->ai_var_C + 16 - E->y_pos) < 0) {
    E->y_pos = E->ai_var_C + 9;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E9A4;
    E->y_height = 24;
  }
}
void TimedShutter_Func_8(void) {  // 0xA2EBA7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  if ((int16)(E->ai_var_D + 16 - E->y_pos) < 0) {
    E->y_pos = E->ai_var_D + 9;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E9AA;
    E->y_height = 32;
  }
}
void TimedShutter_Func_9(void) {  // 0xA2EBE8
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  uint16 v3 = E->ai_var_E + 16;
  if ((int16)(v3 - E->y_pos) < 0) {
    E->y_pos = v3;
    ++E->ai_preinstr;
  }
}
void TimedShutter_Func_Null2(void) {

}
void TimedShutter_Func_10(uint16 k) {
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x8800 + E_k) = E->y_pos;
  uint16 v2 = 2 * E->ai_preinstr;
  off_A2EC3A[v2 >> 1]();
  if (CheckIfEnemyTouchesSamus(k)) {
    int16 v4 = E->y_pos - ERAM(0x8800 + E_k);
    if (v4 < 0)
      extra_samus_y_displacement += v4;
  }
}
void TimedShutter_Func_11(void) {  // 0xA2EC45
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, -IPAIR32(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  int16 v3 = E->ai_var_B - 16;
  if ((int16)(v3 - E->y_pos) >= 0) {
    E->y_pos = v3 + 7;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E99E;
    E->y_height = 16;
  }
}
void TimedShutter_Func_12(void) {  // 0xA2EC86
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, -IPAIR32(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  int16 v3 = E->ai_var_C - 16;
  if ((int16)(v3 - E->y_pos) >= 0) {
    E->y_pos = v3 + 7;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E9A4;
    E->y_height = 24;
  }
}
void TimedShutter_Func_13(void) {  // 0xA2ECC7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, -IPAIR32(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  int16 v3 = E->ai_var_D - 16;
  if ((int16)(v3 - E->y_pos) >= 0) {
    E->y_pos = v3 + 7;
    ++E->ai_preinstr;
    E->instruction_timer = 1;
    E->current_instruction = addr_kTimedShutter_Ilist_E9AA;
    E->y_height = 32;
  }
}
void TimedShutter_Func_14(void) {  // 0xA2ED08
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  AddToHiLo(&E->y_pos, &E->y_subpos, -IPAIR32(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  uint16 v3 = E->ai_var_E - 16;
  if ((int16)(v3 - E->y_pos) >= 0) {
    E->y_pos = v3;
    ++E->ai_preinstr;
  }
}
void TimedShutter_Func_15(void) {  // 0xA2ED33
  ;
}
void RisingFallingPlatform_Init(void) {  // 0xA2EE05
  RisingFallingPlatform_Func_1(cur_enemy_index);
  gEnemyData(cur_enemy_index)->current_instruction = addr_kRisingFallingPlatform_Ilist_EDE7;
}
void ShootableShutter_Init(void) {  // 0xA2EE12
  RisingFallingPlatform_Func_1(cur_enemy_index);
  gEnemyData(cur_enemy_index)->current_instruction = addr_kTimedShutter_Ilist_E9AA;
}
void RisingFallingPlatform_Func_1(uint16 k) {  // 0xA2EE1F
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 current_instruction_low = LOBYTE(E->current_instruction);
  ERAM(0x7800 + E_k) = current_instruction_low;
  int v3 = (8 * current_instruction_low) >> 1;
  E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3)));
  E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
  E->ai_preinstr = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
  E->ai_var_E = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
  uint16 current_instruction_high = HIBYTE(E->current_instruction);
  ERAM(0x7802 + E_k) = current_instruction_high;
  ERAM(0x8000 + E_k) = current_instruction_high;
  uint16 extra_properties_low = LOBYTE(E->extra_properties);
  ERAM(0x7804 + E_k) = extra_properties_low;
  ERAM(0x7810 + E_k) = 16 * extra_properties_low;
  uint16 extra_properties_high = HIBYTE(E->extra_properties);
  ERAM(0x7806 + E_k) = extra_properties_high;
  ERAM(0x7812 + E_k) = 16 * extra_properties_high;
  uint16 rfpm_parameter_1_low = LOBYTE(E->parameter_1);
  ERAM(0x7808 + E_k) = rfpm_parameter_1_low;
  ERAM(0x780E + E_k) = 2 * rfpm_parameter_1_low;
  ERAM(0x780A + E_k) = HIBYTE(E->parameter_1);
  uint16 rfpm_parameter_2 = E->parameter_2;
  ERAM(0x780C + E_k) = rfpm_parameter_2;
  E->ai_var_B = rfpm_parameter_2;
  uint16 y_pos = E->y_pos;
  ERAM(0x781E + E_k) = y_pos;
  ERAM(0x7820 + E_k) = ERAM(0x780A + E_k) + y_pos;
  if (!ERAM(0x7802 + E_k)) {
    uint16 v10 = E->y_pos;
    ERAM(0x7820 + E_k) = v10;
    ERAM(0x781E + E_k) = v10 - ERAM(0x780A + E_k);
  }
  E->ai_var_A = 0xEF09 /* RisingFallingPlatform_Func_2 */;
  E->extra_properties = 0;
  ERAM(0x7814 + E_k) = 0;
}
void CallRisingFallingPlatformFunc(uint32 ea) {
  switch (ea) {
  case 0xA2EF09: RisingFallingPlatform_Func_2(); return;
  case 0xA2EF28: RisingFallingPlatform_Func_4(); return;
  case 0xA2EF68: RisingFallingPlatform_Func_9(); return;
  case 0xA2EFD4: RisingFallingPlatform_Func_10(); return;
  case 0xA2F040: RisingFallingPlatform_Func_11(); return;
  case 0xA2F072: RisingFallingPlatform_Func_12(); return;
  case 0xA2F099: RisingFallingPlatform_Func_13(); return;
  case 0xA2F224: HorizontalShootableShutter_Func_2(); return;
  case 0xA2F243: HorizontalShootableShutter_Func_4(); return;
  case 0xA2F272: HorizontalShootableShutter_Func_8(); return;
  case 0xA2F2E4: HorizontalShootableShutter_Func_9(); return;
  case 0xA2F38C: HorizontalShootableShutter_Func_12(); return;
  case 0xA2F3B0: HorizontalShootableShutter_Func_13(); return;
  case 0xA2F3D4: HorizontalShootableShutter_Func_14(); return;
  default: nt_unimpl("Unreachable");
  }
}
void RisingFallingPlatform_Main(void) {  // 0xA2EED1
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallRisingFallingPlatformFunc(E->ai_var_A | 0xA20000);
  if (E->ai_var_A != 0xEF68 /* RisingFallingPlatform_Func_9 */
      && E->ai_var_A != 0xEFD4 /* RisingFallingPlatform_Func_10 */
      && (enemy_index_colliding_dirs[3] & (uint16)(enemy_index_colliding_dirs[2] & enemy_index_colliding_dirs[1] & enemy_index_colliding_dirs[0])) != 0xFFFF
      && (enemy_index_colliding_dirs[3] & (uint16)(enemy_index_colliding_dirs[2] & enemy_index_colliding_dirs[1] & enemy_index_colliding_dirs[0])) == cur_enemy_index) {
    if (samus_contact_damage_index)
      RisingFallingPlatform_Powerbomb();
  }
}
void RisingFallingPlatform_Func_2(void) {

  int v0 = ERAM(0x780E + (cur_enemy_index)) >> 1;
  off_A2EDFB[v0]();
}
void RisingFallingPlatform_Func_3(void) {  // 0xA2EF15
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_B-- == 1) {
    E->ai_var_B = ERAM(0x780C + E_k);
    RisingFallingPlatform_Func_7(cur_enemy_index);
  }
}
void RisingFallingPlatform_Func_4(void) {  // 0xA2EF28
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (IsSamusWithinEnemy_X(cur_enemy_index, ERAM(0x780C + E_k)))
    RisingFallingPlatform_Func_7(cur_enemy_index);
}
void RisingFallingPlatform_Func_5(void) {  // 0xA2EF39
  RisingFallingPlatform_Func_7(cur_enemy_index);
}
void RisingFallingPlatform_Func_6(void) {  // 0xA2EF40
  ;
}
void RisingFallingPlatform_Func_7(uint16 k) {  // 0xA2EF44
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = 0xEF68 /* RisingFallingPlatform_Func_9 */;
  if (ERAM(0x7802 + E_k))
    E->ai_var_A = 0xEFD4 /* RisingFallingPlatform_Func_10 */;
  RisingFallingPlatform_Func_8();
}
void RisingFallingPlatform_Func_8(void) {  // 0xA2EF5A
  if (!CheckIfEnemyIsOnScreen())
    QueueSfx3_Max6(0xE);
}
void RisingFallingPlatform_Func_9(void) {  // 0xA2EF68
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781C + E_k) = E->y_pos;
  ERAM(0x7814 + E_k) = 0;
  if (CheckIfEnemyTouchesSamus(cur_enemy_index))
    ERAM(0x7814 + E_k) = 1;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_preinstr, E->ai_var_E));
  if (ERAM(0x7814 + E_k))
    extra_samus_y_displacement = E->y_pos - ERAM(0x781C + E_k);
  if ((int16)(ERAM(0x781E + E_k) - E->y_pos) >= 0) {
    if (ERAM(0x7810 + E_k) == 4080) {
      E->ai_var_A = 0xF099 /* RisingFallingPlatform_Func_13 */;
    } else {
      E->ai_var_B = ERAM(0x7810 + E_k);
      E->ai_var_A = 0xF040 /* RisingFallingPlatform_Func_11 */;
    }
  }
}
void RisingFallingPlatform_Func_10(void) {  // 0xA2EFD4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781C + E_k) = E->y_pos;
  ERAM(0x7814 + E_k) = 0;
  if (CheckIfEnemyTouchesSamus(cur_enemy_index))
    ERAM(0x7814 + E_k) = 1;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_var_D, E->ai_var_C));
  if (ERAM(0x7814 + E_k))
    extra_samus_y_displacement = E->y_pos - ERAM(0x781C + E_k);
  if ((int16)(E->y_pos - ERAM(0x7820 + E_k)) >= 0) {
    if (ERAM(0x7812 + E_k) == 4080) {
      E->ai_var_A = 0xF099 /* RisingFallingPlatform_Func_13 */;
    } else {
      E->ai_var_B = ERAM(0x7812 + E_k);
      E->ai_var_A = 0xF072 /* RisingFallingPlatform_Func_12 */;
    }
  }
}
void RisingFallingPlatform_Func_11(void) {  // 0xA2F040
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_B & 0x8000) != 0) {
    E->ai_var_A = 0xEFD4 /* RisingFallingPlatform_Func_10 */;
    if (ERAM(0x7808 + E_k) == 1 && ERAM(0x7802 + E_k))
      E->ai_var_A = 0xEF28 /* RisingFallingPlatform_Func_4 */;
    if (gEnemyData(cur_enemy_index)->enemy_ptr != 0xD83F)
      RisingFallingPlatform_Func_8();
  }
}
void RisingFallingPlatform_Func_12(void) {  // 0xA2F072
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_B & 0x8000) != 0) {
    RisingFallingPlatform_Func_8();
    E->ai_var_A = 0xEF68 /* RisingFallingPlatform_Func_9 */;
    if (ERAM(0x7808 + E_k) == 1 && !ERAM(0x7802 + E_k))
      E->ai_var_A = 0xEF28 /* RisingFallingPlatform_Func_4 */;
  }
}
void RisingFallingPlatform_Func_13(void) {  // 0xA2F099
  ;
}
void RisingFallingPlatform_Touch(void) {  // 0xA2F09D
  RisingFallingPlatform_Powerbomb();
}
void RisingFallingPlatform_Shot(void) {  // 0xA2F0A2
  RisingFallingPlatform_Powerbomb();
}
void ShootableShutter_Shot(void) {  // 0xA2F0AA
  NormalEnemyShotAi();
  RisingFallingPlatform_Powerbomb();
}
void RisingFallingPlatform_Powerbomb(void) {  // 0xA2F0B6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (sign16(ERAM(0x780E + E_k) - 6)) {
LABEL_10:
    RisingFallingPlatform_Func_8();
    return;
  }
  if (ERAM(0x780E + E_k) != 8) {
    if (ERAM(0x7818 + E_k))
      return;
    ERAM(0x7818 + E_k) = 1;
  }
  if (E->ai_var_A != 0xEF68 /* RisingFallingPlatform_Func_9 */
      && E->ai_var_A != 0xEFD4 /* RisingFallingPlatform_Func_10 */) {
    E->ai_var_A = 0xEF68 /* RisingFallingPlatform_Func_9 */;
    if (ERAM(0x8000 + E_k))
      E->ai_var_A = 0xEFD4 /* RisingFallingPlatform_Func_10 */;
    ERAM(0x8000 + E_k) ^= 1;
    goto LABEL_10;
  }
}
void HorizontalShootableShutter_Func_2(void) {

  int v0 = ERAM(0x780E + (cur_enemy_index)) >> 1;
  off_A2F107[v0]();
}
void HorizontalShootableShutter_Func_3(void) {  // 0xA2F230
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_B-- == 1) {
    E->ai_var_B = ERAM(0x780C + E_k);
    HorizontalShootableShutter_Func_7(cur_enemy_index);
  }
}
void HorizontalShootableShutter_Func_4(void) {  // 0xA2F243
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (IsSamusWithinEnemy_X(cur_enemy_index, ERAM(0x780C + E_k)))
    HorizontalShootableShutter_Func_7(cur_enemy_index);
}
void HorizontalShootableShutter_Func_5(void) {  // 0xA2F254
  HorizontalShootableShutter_Func_7(cur_enemy_index);
}
void HorizontalShootableShutter_Func_6(void) {  // 0xA2F25B
  ;
}
void HorizontalShootableShutter_Func_7(uint16 k) {  // 0xA2F25F
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = 0xF272 /* HorizontalShootableShutter_Func_8 */;
  if (ERAM(0x7802 + E_k))
    E->ai_var_A = 0xF2E4 /* HorizontalShootableShutter_Func_9 */;
}
void HorizontalShootableShutter_Func_8(void) {  // 0xA2F272
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781A + E_k) = E->x_pos;
  ERAM(0x7826 + E_k) = 0;
  if (EnemyFunc_AC67(cur_enemy_index) && (int16)(samus_x_pos - E->x_pos) < 0)
    ERAM(0x7826 + E_k) = 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(E->ai_preinstr, E->ai_var_E));
  if (ERAM(0x7826 + E_k)) {
    extra_samus_x_subdisplacement = E->ai_var_E;
    extra_samus_x_displacement = E->ai_preinstr;
    HorizontalShootableShutter_Func_10(cur_enemy_index);
  }
  if ((int16)(ERAM(0x7822 + E_k) - E->x_pos) >= 0) {
    if (ERAM(0x7810 + E_k) == 4080) {
      E->ai_var_A = 0xF3D4 /* HorizontalShootableShutter_Func_14 */;
    } else {
      E->ai_var_B = ERAM(0x7810 + E_k);
      E->ai_var_A = 0xF38C /* HorizontalShootableShutter_Func_12 */;
    }
  }
}
void HorizontalShootableShutter_Func_9(void) {  // 0xA2F2E4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781A + E_k) = E->x_pos;
  ERAM(0x7826 + E_k) = 0;
  if (EnemyFunc_AC67(cur_enemy_index) && (int16)(samus_x_pos - E->x_pos) >= 0)
    ERAM(0x7826 + E_k) = 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(E->ai_var_D, E->ai_var_C));
  if (ERAM(0x7826 + E_k)) {
    extra_samus_x_subdisplacement = E->ai_var_C;
    extra_samus_x_displacement = E->ai_var_D;
    HorizontalShootableShutter_Func_11(cur_enemy_index);
  }
  if ((int16)(E->x_pos - ERAM(0x7824 + E_k)) >= 0) {
    if (ERAM(0x7812 + E_k) == 4080) {
      E->ai_var_A = 0xF3D4 /* HorizontalShootableShutter_Func_14 */;
    } else {
      E->ai_var_B = ERAM(0x7812 + E_k);
      E->ai_var_A = 0xF3B0 /* HorizontalShootableShutter_Func_13 */;
    }
  }
}
void HorizontalShootableShutter_Func_10(uint16 k) {  // 0xA2F356
  if (ERAM(0x7826 + (k))) {
    if ((joypad1_lastkeys & 0x100) != 0) {
      extra_samus_x_displacement -= 4;
      extra_samus_y_subdisplacement = 0;
    }
  }
}
void HorizontalShootableShutter_Func_11(uint16 k) {  // 0xA2F371
  if (ERAM(0x7826 + (k))) {
    if ((joypad1_lastkeys & 0x200) != 0) {
      extra_samus_x_displacement += 4;
      extra_samus_y_subdisplacement = 0;
    }
  }
}
void HorizontalShootableShutter_Func_12(void) {  // 0xA2F38C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_B & 0x8000) != 0) {
    E->ai_var_A = 0xF2E4 /* HorizontalShootableShutter_Func_9 */;
    if (ERAM(0x7808 + E_k) == 1) {
      if (ERAM(0x7802 + E_k))
        E->ai_var_A = 0xF243 /* HorizontalShootableShutter_Func_4 */;
    }
  }
}
void HorizontalShootableShutter_Func_13(void) {  // 0xA2F3B0
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_B & 0x8000) != 0) {
    E->ai_var_A = 0xF272 /* HorizontalShootableShutter_Func_8 */;
    if (ERAM(0x7808 + E_k) == 1 && !ERAM(0x7802 + E_k))
      E->ai_var_A = 0xF243 /* HorizontalShootableShutter_Func_4 */;
  }
}
void HorizontalShootableShutter_Func_14(void) {  // 0xA2F3D4
  ;
}
void Enemy_NormalTouchAI_A3(void) {  // 0xA38023
  NormalEnemyTouchAi();
}
void Enemy_NormalShotAI_A3(void) {  // 0xA3802D
  NormalEnemyShotAi();
}
void Enemy_NormalShotAI_SkipSomeParts_A3(void) {  // 0xA38032
  NormalEnemyShotAiSkipDeathAnim_CurEnemy();
}
void Enemy_NormalPowerBombAI_A3(void) {  // 0xA38037
  NormalEnemyPowerBombAi();
}
void Enemy_NormalFrozenAI_A3(void) {  // 0xA38041
  NormalEnemyFrozenAI();
}
uint16 Waver_Instr_1(uint16 k, uint16 jp) {  // 0xA386E3
  gEnemyData(cur_enemy_index)->ai_var_E = 1;
  return jp;
}
void Waver_Init(void) {  // 0xA386ED
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = 1;
  E->ai_var_A = 0x8000;
  if ((E->parameter_1 & 1) == 0) {
    E->ai_var_B = SignExtend8(0xFE);
    E->ai_var_A = SignExtend8(0x8000);
  }
  E->ai_preinstr = 0;
  E->ai_var_C = 0;
  E->ai_var_E = 0;
  E->current_instruction = addr_kWaver_Ilist_86A7;
  E->ai_preinstr = E->parameter_1 & 1;
  Waver_Func_1();
}
void Waver_Main(void) {  // 0xA3874C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_B, E->ai_var_A))) {
    uint16 r18 = ((uint16)(HIBYTE(E->ai_var_A) | LOBYTE(E->ai_var_B) << 8));
    E->ai_var_B = SignExtend8((-r18 & 0xFF00) >> 8);
    E->ai_var_A = -(int8)r18 << 8;
    E->ai_preinstr = (E->ai_preinstr ^ 1) & 1;
    Waver_Func_1();
  } else {
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(SineMult8bit(LOBYTE(E->ai_var_D), 4)))) {
      E->ai_var_D = (uint8)(E->ai_var_D + 0x80);
    } else {
      E->ai_var_D += 2;
    }
  }
  if ((E->ai_var_D & 0x7F) == 56) {
    E->ai_preinstr |= 2;
    Waver_Func_1();
  }
  if (E->ai_var_E) {
    E->ai_var_E = 0;
    E->ai_preinstr = E->ai_preinstr & 1;
    Waver_Func_1();
  }
}
void Waver_Func_1(void) {  // 0xA387FE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 waver_var_F = E->ai_preinstr;
  if (waver_var_F != E->ai_var_C) {
    E->ai_var_C = waver_var_F;
    E->current_instruction = ((uint16)BW_A3(0x86DB + 2 * (waver_var_F)));
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
uint16 Metalee_Instr_1(uint16 k, uint16 jp) {  // 0xA38956
  gEnemyData(cur_enemy_index)->ai_var_E = 1;
  return jp;
}
void Metalee_Init(void) {  // 0xA38960
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = 0;
  E->ai_var_D = 0;
  E->ai_var_E = 0;
  E->current_instruction = addr_kMetalee_Ilist_8910;
  E->ai_var_B = 0x8987 /* Metalee_Func_1 */;
}
void CallMetaleeFunc(uint32 ea) {
  switch (ea) {
  case 0xA38987: Metalee_Func_1(); return;
  case 0xA389D4: Metalee_Func_3(cur_enemy_index); return;
  case 0xA389F3: Metalee_Func_4(); return;
  case 0xA38A5C: Metalee_Func_5(); return;
  default: nt_unimpl("Unreachable");
  }
}
void Metalee_Main(void) {  // 0xA38979
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallMetaleeFunc(E->ai_var_B | 0xA30000);
}
void Metalee_Func_1(void) {  // 0xA38987
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (abs16(E->x_pos - samus_x_pos) < 0x48) {
    Metalee_Func_2(cur_enemy_index);
    ++E->ai_var_C;
    Metalee_Func_6();
    E->ai_var_B = 0x89D4 /* Metalee_Func_3 */;
  }
}
void Metalee_Func_2(uint16 k) {  // 0xA389AC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 div = (uint8)SnesDivide(samus_y_pos - E->y_pos, 24);
  E->ai_preinstr = div + 4;
}
void Metalee_Func_3(uint16 k) {  // 0xA389D4
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_E) {
    E->ai_var_E = 0;
    ++E->ai_var_C;
    Metalee_Func_6();
    E->ai_var_B = 0x89F3 /* Metalee_Func_4 */;
    QueueSfx2_Max6(0x5B);
  }
}
void Metalee_Func_4(void) {  // 0xA389F3
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = 21;
  uint16 v1 = E->properties | 3;
  E->properties = v1;
  if (EnemyFunc_BF8A(cur_enemy_index, v1, INT16_SHL16(E->ai_preinstr)) & 1) {
    E->instruction_timer = 1;
    E->timer = 0;
    E->ai_var_B = 0x8A5C /* Metalee_Func_5 */;
    QueueSfx2_Max6(0x5C);
  } else {
    E->y_pos += E->ai_preinstr;
    E->x_pos += ((int16)(E->x_pos - samus_x_pos) >= 0) ? -2 : 2;
  }
}
void Metalee_Func_5(void) {  // 0xA38A5C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_A-- == 1) {
    gEnemySpawnData(cur_enemy_index)->vram_tiles_index = E->vram_tiles_index | E->palette_index;
    E->palette_index = 2560;
    E->vram_tiles_index = 0;
    E->properties |= kEnemyProps_Deleted;
  } else {
    if (E->ai_var_A == 8) {
      uint16 v2 = cur_enemy_index;
      SpawnEprojWithGfx(0, v2, addr_stru_868BFA);
      SpawnEprojWithGfx(0, v2, addr_stru_868C08);
      SpawnEprojWithGfx(0, v2, addr_stru_868C16);
      SpawnEprojWithGfx(0, v2, addr_stru_868C24);
    }
    ++E->y_pos;
  }
}
void Metalee_Func_6(void) {  // 0xA38AB2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 metalee_var_C = E->ai_var_C;
  if (metalee_var_C != E->ai_var_D) {
    E->ai_var_D = metalee_var_C;
    E->current_instruction = ((uint16)BW_A3(0x894E + 2 * (metalee_var_C)));
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
void Metalee_Shot(void) {  // 0xA38B0F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 varE2A = E->vram_tiles_index;
  uint16 varE2C = E->palette_index;
  Enemy_NormalShotAI_A3();
  if (!gEnemyData(cur_enemy_index)->health) {
    E->vram_tiles_index = varE2A;
    E->palette_index = varE2C;
    uint16 v2 = cur_enemy_index;
    SpawnEprojWithGfx(E->ai_var_A, cur_enemy_index, addr_stru_868BFA);
    SpawnEprojWithGfx(0, v2, addr_stru_868C08);
    SpawnEprojWithGfx(0, v2, addr_stru_868C16);
    SpawnEprojWithGfx(0, v2, addr_stru_868C24);
    E->vram_tiles_index = 0;
    E->palette_index = 0;
  }
}
void Fireflea_Init(void) {  // 0xA38D2D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kFireflea_Ilist_8C2F;
  if ((E->parameter_1 & 2) != 0) {
    Fireflea_Func_1(cur_enemy_index);
    Fireflea_Func_2(cur_enemy_index);
    Fireflea_Func_3(cur_enemy_index);
    Fireflea_Func_4(cur_enemy_index);
    Fireflea_Func_5(cur_enemy_index);
  } else {
    Fireflea_Func_3(cur_enemy_index);
    Fireflea_Func_4(cur_enemy_index);
    Fireflea_Func_6(cur_enemy_index);
  }
}
void Fireflea_Func_1(uint16 k) {  // 0xA38D5D
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_E = E->x_pos;
  E->ai_preinstr = E->y_pos;
}
void Fireflea_Func_2(uint16 k) {  // 0xA38D6A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_D = HIBYTE(E->parameter_1) << 8;
}
void Fireflea_Func_3(uint16 k) {  // 0xA38D75
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v2 = 8 * LOBYTE(E->parameter_2);
  if ((E->parameter_1 & 1) == 0)
    v2 += 4;
  ERAM(0x7804 + E_k) = v2;
  int v3 = v2 >> 1;
  E->ai_var_B = ((uint16)BW_A2(0x8187 + 2 * (v3)));
  E->ai_var_A = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
}
void Fireflea_Func_4(uint16 k) {  // 0xA38D9C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_C = (uint8)(((uint16)BW_A3(0x8D1D + 2 * (HIBYTE(E->parameter_2)))));
}
void Fireflea_Func_5(uint16 k) {  // 0xA38DAE
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos = E->ai_var_E + CosineMult8bit(E->ai_var_D, E->ai_var_C);
  E->y_pos = E->ai_preinstr + SineMult8bit(E->ai_var_D, E->ai_var_C);
}
void Fireflea_Func_6(uint16 k) {  // 0xA38DD7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = E->y_pos - E->ai_var_C;
  ERAM(0x7802 + E_k) = E->ai_var_C + E->y_pos;
}
void Fireflea_Main(void) {  // 0xA38DEE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((E->parameter_1 & 2) != 0) {
    E->x_pos = E->ai_var_E + CosineMult8bit(HIBYTE(E->ai_var_D), E->ai_var_C);
    E->y_pos = E->ai_preinstr + SineMult8bit(HIBYTE(E->ai_var_D), E->ai_var_C);
    E->ai_var_D += ((uint16)(HIBYTE(E->ai_var_A) | LOBYTE(E->ai_var_B) << 8));
  } else {
    int v2 = ERAM(0x7804 + E_k) >> 1;
    AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (v2))), ((uint16)BW_A2(0x8187 + 2 * (v2 + 1)))));
    if ((int16)(E->y_pos - ERAM(0x7800 + E_k)) < 0
        || (int16)(E->y_pos - ERAM(0x7802 + E_k)) >= 0) {
      ERAM(0x7804 + E_k) ^= 4;
    }
  }
}
void Fireflea_Touch(uint16 k) {  // 0xA38E6B
  uint16 v1 = 0;

  Enemy_NormalTouchAI_A3();
  EnemyDeathAnimation(k, v1);
  if (sign16(fireflea_darkness_level - 12))
    fireflea_darkness_level += 2;
}
void Fireflea_Powerbomb(void) {  // 0xA38E83
  Enemy_NormalPowerBombAI_A3();
  Fireflea_Common();
}
void Fireflea_Shot(void) {  // 0xA38E89
  Enemy_NormalShotAI_A3();
  Fireflea_Common();
}
void Fireflea_Common(void) {  // 0xA38E8D
  if (!gEnemyData(cur_enemy_index)->health) {
    if (sign16(fireflea_darkness_level - 12))
      fireflea_darkness_level += 2;
  }
}
uint16 MaridiaFish_Instr_3(uint16 k, uint16 jp) {  // 0xA39096
  gEnemyData(cur_enemy_index)->layer = 6;
  return jp;
}
uint16 MaridiaFish_Instr_1(uint16 k, uint16 jp) {  // 0xA390A0
  gEnemyData(cur_enemy_index)->layer = 2;
  return jp;
}
uint16 MaridiaFish_Instr_2(uint16 k, uint16 jp) {  // 0xA390AA
  ERAM(0x7802 + (cur_enemy_index)) = 1;
  return jp;
}
void MaridiaFish_Init(void) {  // 0xA390B5
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = addr_kMaridiaFish_Ilist_902A;
  E->ai_var_A = 0x9132 /* MaridiaFish_Func_1 */;
  if (!HIBYTE(E->parameter_1)) {
    E->current_instruction = addr_kMaridiaFish_Ilist_9060;
    E->ai_var_A = 0x91AB /* MaridiaFish_Func_2 */;
  }
  int v1 = (8 * LOBYTE(E->parameter_1)) >> 1;
  E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v1)));
  E->ai_var_B = ((uint16)BW_A2(0x8187 + 2 * (v1 + 1)));
  E->ai_var_E = ((uint16)BW_A2(0x8187 + 2 * (v1 + 2)));
  E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v1 + 3)));
  ERAM(0x7800 + E_k) = LOBYTE(E->parameter_2);
  ERAM(0x7804 + E_k) = HIBYTE(E->parameter_2);
  E->ai_preinstr = 0;
  ERAM(0x7802 + E_k) = 0;
  ERAM(0x7806 + E_k) = SineMult8bit(E->ai_preinstr, ERAM(0x7800 + E_k));
}
void MaridiaFish_Func_1(void) {  // 0xA39132
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_E, E->ai_var_D))) {
    E->ai_var_A = 0x9224 /* MaridiaFish_Func_3 */;
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kMaridiaFish_Ilist_903C;
  } else {
    uint16 v2 = SineMult8bit(E->ai_preinstr, ERAM(0x7800 + E_k));
    ERAM(0x7808 + E_k) = v2;
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(v2 - ERAM(0x7806 + E_k)))) {
      E->ai_var_A = 0x9224 /* MaridiaFish_Func_3 */;
      E->instruction_timer = 1;
      E->timer = 0;
      E->current_instruction = addr_kMaridiaFish_Ilist_903C;
    } else {
      E->ai_preinstr = (uint8)(LOBYTE(ERAM(0x7804 + E_k)) + E->ai_preinstr);
    }
  }
  ERAM(0x7806 + E_k) = ERAM(0x7808 + E_k);
}
void MaridiaFish_Func_2(void) {  // 0xA391AB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_C, E->ai_var_B))) {
    E->ai_var_A = 0x9256 /* MaridiaFish_Func_4 */;
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kMaridiaFish_Ilist_9072;
  } else {
    uint16 v2 = SineMult8bit(E->ai_preinstr, ERAM(0x7800 + E_k));
    ERAM(0x7808 + E_k) = v2;
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(v2 - ERAM(0x7806 + E_k)))) {
      E->ai_var_A = 0x9256 /* MaridiaFish_Func_4 */;
      E->instruction_timer = 1;
      E->timer = 0;
      E->current_instruction = addr_kMaridiaFish_Ilist_9072;
    } else {
      E->ai_preinstr = (uint8)(LOBYTE(ERAM(0x7804 + E_k)) + E->ai_preinstr);
    }
  }
  ERAM(0x7806 + E_k) = ERAM(0x7808 + E_k);
}
void MaridiaFish_Func_3(void) {  // 0xA39224
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    E->ai_var_A = 0x91AB /* MaridiaFish_Func_2 */;
    ERAM(0x7804 + E_k) = -ERAM(0x7804 + E_k);
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kMaridiaFish_Ilist_9060;
  }
}
void MaridiaFish_Func_4(void) {  // 0xA39256
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7802 + E_k)) {
    ERAM(0x7802 + E_k) = 0;
    E->ai_var_A = 0x9132 /* MaridiaFish_Func_1 */;
    ERAM(0x7804 + E_k) = -ERAM(0x7804 + E_k);
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kMaridiaFish_Ilist_902A;
  }
}
void CallMaridiaFishFunc(uint32 ea) {
  switch (ea) {
  case 0xA39132: MaridiaFish_Func_1(); return;
  case 0xA391AB: MaridiaFish_Func_2(); return;
  case 0xA39224: MaridiaFish_Func_3(); return;
  case 0xA39256: MaridiaFish_Func_4(); return;
  default: nt_unimpl("Unreachable");
  }
}
void MaridiaFish_Main(void) {  // 0xA3912B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallMaridiaFishFunc(E->ai_var_A | 0xA30000);
}
void Elevator_Init(void) {  // 0xA394E6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->spritemap_pointer = addr_kSpritemap_Nothing_A3;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = addr_kElevator_Ilist_94D6;
  E->parameter_1 *= 2;
  E->ai_var_A = E->y_pos;
  if (elevator_status != 2)
    elevator_flags = elevator_status = 0;
  if (__PAIR32__(elevator_status, elevator_flags)) {
    E->y_pos = E->parameter_2;
    Elevator_Func_4();
  }
}
void Elevator_Frozen(void) {  // 0xA3952A
  if (!door_transition_flag_elevator_zebetites) {
    if (__PAIR32__(elevator_status, elevator_flags))
      off_A39540[elevator_status]();
  }
}
void Elevator_Func_1(void) {  // 0xA39548
  if ((((uint16)BW_A3(0x94E2 + 2 * (gEnemyData(cur_enemy_index)->parameter_1 >> 1))) & joypad1_newkeys) != 0) {
    QueueSfx3_Max6(0xB);
    QueueSfx1_Max6(0x32);
    CallSomeSamusCode(7);
    ResetProjectileData();
    Elevator_Func_4();
    ++elevator_status;
  } else {
    elevator_flags = 0;
  }
}
void Elevator_Func_2(void) {  // 0xA39579
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->parameter_1) {
    elevator_direction = 0x8000;
    AddToHiLo(&E->y_pos, &E->y_subpos, -0x18000);
  } else {
    elevator_direction = 0;
    AddToHiLo(&E->y_pos, &E->y_subpos, 0x18000);
  }
  Elevator_Func_4();
}
void Elevator_Func_3(void) {  // 0xA395B9
  ++elevator_status;
  Elevator_Func3b();
}
void Elevator_Func3b(void) {  // 0xA395BC
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->parameter_1) {
    AddToHiLo(&E->y_pos, &E->y_subpos, 0x18000);
    if (E->y_pos < E->ai_var_A) {
      Elevator_Func_4();
      return;
    }
  } else {
    AddToHiLo(&E->y_pos, &E->y_subpos, -0x18000);
    if (E->y_pos >= E->ai_var_A) {
      Elevator_Func_4();
      return;
    }
  }
  elevator_status = 0;
  elevator_flags = 0;
  QueueSfx3_Max6(0x25);
  E->y_pos = E->ai_var_A;
  CallSomeSamusCode(0xB);
  Elevator_Func_4();
}
void Elevator_Func_4(void) {  // 0xA39612
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  samus_y_pos = E->y_pos - 26;
  samus_y_subpos = 0;
  samus_x_pos = E->x_pos;
  samus_y_speed = 0;
  samus_y_subspeed = 0;
}
void Crab_Init(void) {  // 0xA396E3
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  v1->parameter_2 = 8;
  v1->current_instruction = ((uint16)BW_A3(0x96DB + 2 * (v1->current_instruction & 3)));
  StoneZoomer_E67A(cur_enemy_index);
}
void Crab_Func_1(void) {  // 0xA396FD
  ;
}
void Slug_Init(void) {  // 0xA3993B
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  v1->parameter_2 = 10;
  v1->current_instruction = ((uint16)BW_A3(0x992B + 2 * (v1->current_instruction & 3)));
  StoneZoomer_E67A(cur_enemy_index);
}
void Slug_Func_1(void) {  // 0xA39955
  ;
}
uint16 PlatformThatFallsWithSamus_Instr_3(uint16 k, uint16 jp) {  // 0xA39C6B
  ERAM(0x7804 + (cur_enemy_index)) = 0;
  return jp;
}
uint16 PlatformThatFallsWithSamus_Instr_4(uint16 k, uint16 jp) {  // 0xA39C76
  ERAM(0x7804 + (cur_enemy_index)) = 1;
  return jp;
}
uint16 PlatformThatFallsWithSamus_Instr_1(uint16 k, uint16 jp) {  // 0xA39C81
  ERAM(0x7804 + (cur_enemy_index)) = 0;
  return jp;
}
uint16 PlatformThatFallsWithSamus_Instr_2(uint16 k, uint16 jp) {  // 0xA39C8C
  ERAM(0x7804 + (cur_enemy_index)) = 1;
  return jp;
}
void PlatformThatFallsWithSamus_Init(void) {  // 0xA39C9F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x783E + E_k) = -1;
  uint16 ptfwss_parameter_1 = E->parameter_1;
  ERAM(0x7804 + E_k) = ptfwss_parameter_1;
  if (ptfwss_parameter_1)
    PlatformThatFalls_Init(cur_enemy_index, addr_kPlatformThatFallsWithSamus_Ilist_9BFD);
  else
    PlatformThatFalls_Init(cur_enemy_index, addr_kPlatformThatFallsWithSamus_Ilist_9BE7);
}
void FastMovingSlowSinkingPlatform_Init(void) {  // 0xA39CBA
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 ptfwss_parameter_1 = E->parameter_1;
  ERAM(0x7804 + E_k) = ptfwss_parameter_1;
  if (ptfwss_parameter_1)
    PlatformThatFalls_Init(cur_enemy_index, addr_kPlatformThatFallsWithSamus_Ilist_9C55);
  else
    PlatformThatFalls_Init(cur_enemy_index, addr_kPlatformThatFallsWithSamus_Ilist_9C3F);
}
void PlatformThatFalls_Init(uint16 k, uint16 j) {  // 0xA39CCC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->current_instruction = j;
  int v3 = (8 * LOBYTE(E->parameter_2)) >> 1;
  E->ai_var_C = ((uint16)BW_A2(0x8187 + 2 * (v3)));
  E->ai_var_B = ((uint16)BW_A2(0x8187 + 2 * (v3 + 1)));
  E->ai_var_E = ((uint16)BW_A2(0x8187 + 2 * (v3 + 2)));
  E->ai_var_D = ((uint16)BW_A2(0x8187 + 2 * (v3 + 3)));
  ERAM(0x7800 + E_k) = 0;
  ERAM(0x7806 + E_k) = 0;
  ERAM(0x7808 + E_k) = 0;
  E->ai_var_A = E->y_pos + 1;
  E->ai_preinstr = 0;
  ERAM(0x780A + E_k) = HIBYTE(E->parameter_2);
}
void PlatformThatFallsWithSamus_Main(void) {  // 0xA39D16
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = 0;
  if (CheckIfEnemyTouchesSamus(cur_enemy_index))
    ERAM(0x7800 + E_k) = 1;
  off_A39C97[ERAM(0x7804 + E_k)]();
  int v3 = ERAM(0x7800 + (cur_enemy_index));
  off_A39C9B[v3]();
  if (ERAM(0x7800 + E_k) != ERAM(0x780C + E_k))
    E->ai_preinstr = 0;
  ERAM(0x780C + E_k) = ERAM(0x7800 + E_k);
}
void PlatformThatFallsWithSamus_Func_1(void) {  // 0xA39D5E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7802 + E_k) = E->x_pos;
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_E, E->ai_var_D))) {
    ERAM(0x7804 + E_k) = 1;
    PlatformThatFallsWithSamus_Func_8();
  }
}
void PlatformThatFallsWithSamus_Func_2(void) {  // 0xA39D83
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7802 + E_k) = E->x_pos;
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(E->ai_var_C, E->ai_var_B))) {
    ERAM(0x7804 + E_k) = 0;
    PlatformThatFallsWithSamus_Func_7();
  }
}
void PlatformThatFallsWithSamus_Func_3(void) {  // 0xA39DA8
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(E->y_pos - E->ai_var_A) < 0)
    goto LABEL_5;
  PlatformThatFallsWithSamus_Func_9();
  uint16 ptfwss_var_05;
  ptfwss_var_05 = ++E->ai_preinstr;
  if ((int16)(ptfwss_var_05 - ERAM(0x780A + E_k)) >= 0) {
    ptfwss_var_05 = ERAM(0x780A + E_k);
    E->ai_preinstr = ptfwss_var_05;
  }
  int v3;
  v3 = (8 * ptfwss_var_05) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v3 + 3))), ((uint16)BW_A2(0x838F + 2 * (v3 + 2)))))) {
LABEL_5:
    E->ai_preinstr = 0;
    PlatformThatFallsWithSamus_Func_10();
  }
}
void PlatformThatFallsWithSamus_Func_4(void) {  // 0xA39DE4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(++E->ai_preinstr - ERAM(0x780A + E_k)) >= 0)
    E->ai_preinstr = ERAM(0x780A + E_k);
  extra_samus_x_displacement += E->x_pos - ERAM(0x7802 + E_k);
  ERAM(0x7802 + E_k) = E->y_pos;
  int v2 = (8 * E->ai_preinstr) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 1))), ((uint16)BW_A2(0x838F + 2 * (v2)))))) {
    E->ai_preinstr = 0;
    PlatformThatFallsWithSamus_Func_10();
  }
  extra_samus_y_displacement += E->y_pos - ERAM(0x7802 + E_k);
}
void PlatformThatFallsWithSamus_Func_5(void) {  // 0xA39E47
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v0 = addr_kPlatformThatFallsWithSamus_Ilist_9C13;
  if ((ERAM(0x783E + E_k) & 0x8000) != 0)
    v0 = addr_kPlatformThatFallsWithSamus_Ilist_9BBB;
  E->current_instruction = v0;
  E->instruction_timer = 1;
  E->timer = 0;
}
void PlatformThatFallsWithSamus_Func_6(void) {  // 0xA39E64
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v0 = addr_kPlatformThatFallsWithSamus_Ilist_9C29;
  if ((ERAM(0x783E + E_k) & 0x8000) != 0)
    v0 = addr_kPlatformThatFallsWithSamus_Ilist_9BD1;
  E->current_instruction = v0;
  E->instruction_timer = 1;
  E->timer = 0;
}
void PlatformThatFallsWithSamus_Func_7(void) {  // 0xA39E81
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v0 = addr_kPlatformThatFallsWithSamus_Ilist_9C3F;
  if ((ERAM(0x783E + E_k) & 0x8000) != 0)
    v0 = addr_kPlatformThatFallsWithSamus_Ilist_9BE7;
  E->current_instruction = v0;
  E->instruction_timer = 1;
  E->timer = 0;
}
void PlatformThatFallsWithSamus_Func_8(void) {  // 0xA39E9E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v0 = addr_kPlatformThatFallsWithSamus_Ilist_9C55;
  if ((ERAM(0x783E + E_k) & 0x8000) != 0)
    v0 = addr_kPlatformThatFallsWithSamus_Ilist_9BFD;
  E->current_instruction = v0;
  E->instruction_timer = 1;
  E->timer = 0;
}
void PlatformThatFallsWithSamus_Func_9(void) {  // 0xA39EBB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x7806 + E_k)) {
    ERAM(0x7806 + E_k) = 1;
    if (ERAM(0x7804 + E_k))
      PlatformThatFallsWithSamus_Func_6();
    else
      PlatformThatFallsWithSamus_Func_5();
  }
  ERAM(0x7808 + E_k) = 0;
}
void PlatformThatFallsWithSamus_Func_10(void) {  // 0xA39EE1
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x7808 + E_k)) {
    ERAM(0x7808 + E_k) = 1;
    if (ERAM(0x7804 + E_k))
      PlatformThatFallsWithSamus_Func_8();
    else
      PlatformThatFallsWithSamus_Func_7();
  }
  ERAM(0x7806 + E_k) = 0;
}
void FastMovingSlowSinkingPlatform_Shot(void) {  // 0xA39F08
  Enemy_NormalShotAI_A3();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->frozen_timer) {
    if (ERAM(0x7804 + E_k))
      E->spritemap_pointer = addr_kPlatformThatFallsWithSamus_Sprmap_A015;
    else
      E->spritemap_pointer = addr_kPlatformThatFallsWithSamus_Sprmap_A009;
  }
}
void Roach_Func_1(void) {  // 0xA3A12F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 roach_var_24 = ERAM(0x8008 + E_k);
  if (roach_var_24 != ERAM(0x800A + E_k)) {
    ERAM(0x800A + E_k) = roach_var_24;
    E->current_instruction = roach_var_24;
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
void Roach_Init(void) {  // 0xA3A14D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x800A + E_k) = 0;
  ERAM(0x8008 + E_k) = 0;
  Roach_Func_2(cur_enemy_index);
  Roach_Func_4(cur_enemy_index);
  Roach_Func_5(cur_enemy_index);
  Roach_Func_6(cur_enemy_index);
  Roach_Func_7(cur_enemy_index);
  Roach_Func_8(cur_enemy_index);
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8000 + E_k) >> 1)));
  Roach_Func_1();
  E->ai_var_B = 0xA2D7 /* Roach_Func_9 */;
}
void Roach_Func_2(uint16 k) {  // 0xA3A183
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  Point32 pt = ConvertAngleToXy(HIBYTE(E->parameter_1), LOBYTE(E->parameter_1));
  SetHiLo(&ERAM(0x7802 + E_k), &ERAM(0x7800 + E_k), pt.x);
  SetHiLo(&ERAM(0x7806 + E_k), &ERAM(0x7804 + E_k), pt.y);
}
void Roach_Func_3(uint16 k) {  // 0xA3A1B0
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  SetHiLo(&ERAM(0x7802 + E_k), &ERAM(0x7800 + E_k), CosineMult8bitFull(HIBYTE(E->parameter_1), LOBYTE(E->parameter_1)));
  SetHiLo(&ERAM(0x7806 + E_k), &ERAM(0x7804 + E_k), SineMult8bitFull(HIBYTE(E->parameter_1), LOBYTE(E->parameter_1)));
}
void Roach_Func_4(uint16 k) {  // 0xA3A1F3
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  SetHiLo(&ERAM(0x780A + E_k), &ERAM(0x7808 + E_k), CosineMult8bitFull((uint8)(HIBYTE(E->parameter_1) - 32), LOBYTE(E->parameter_1)));
  SetHiLo(&ERAM(0x780E + E_k), &ERAM(0x780C + E_k), SineMult8bitFull((uint8)(HIBYTE(E->parameter_1) - 32), LOBYTE(E->parameter_1)));
}
void Roach_Func_5(uint16 k) {  // 0xA3A23E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  SetHiLo(&ERAM(0x7812 + E_k), &ERAM(0x7810 + E_k), CosineMult8bitFull((uint8)(HIBYTE(E->parameter_1) + 32), LOBYTE(E->parameter_1)));
  SetHiLo(&ERAM(0x7816 + E_k), &ERAM(0x7814 + E_k), SineMult8bitFull((uint8)(HIBYTE(E->parameter_1) + 32), LOBYTE(E->parameter_1)));
}
void Roach_Func_6(uint16 k) {  // 0xA3A289
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x8000 + E_k) = 2 * ((uint8)(HIBYTE(E->parameter_1) - 48) >> 5);
}
void Roach_Func_7(uint16 k) {  // 0xA3A29E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x8002 + E_k) = 2 * ((uint8)(HIBYTE(E->parameter_1) - 80) >> 5);
}
void Roach_Func_8(uint16 k) {  // 0xA3A2B7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x8004 + E_k) = 2 * ((uint8)(HIBYTE(E->parameter_1) - 48 + 32) >> 5);
}
void CallRoachFunc(uint32 ea) {
  switch (ea) {
  case 0xA3A2D7: Roach_Func_9(); return;  // 0xa3a2d7
  case 0xA3A301: Roach_Func_10(); return;  // 0xa3a301
  case 0xA3A30B: Roach_Func_11(); return;  // 0xa3a30b
  case 0xA3A315: Roach_Func_12(); return;  // 0xa3a315
  case 0xA3A325: Roach_Func_13(); return;  // 0xa3a325
  case 0xA3A33B: Roach_Func_14(); return;  // 0xa3a33b
  case 0xA3A34B: Roach_Func_15(); return;  // 0xa3a34b
  case 0xA3A380: Roach_Func_16(); return;  // 0xa3a380
  case 0xA3A407: Roach_Func_19(); return;  // 0xa3a407
  case 0xA3A40E: Roach_Func_20(); return;  // 0xa3a40e
  case 0xA3A440: Roach_Func_21(); return;  // 0xa3a440
  case 0xA3A447: Roach_Func_22(); return;  // 0xa3a447
  case 0xA3A44E: Roach_Func_23(); return;  // 0xa3a44e
  case 0xA3A462: Roach_Func_24(); return;  // 0xa3a462
  case 0xA3A476: Roach_Func_25(); return;  // 0xa3a476
  case 0xA3A4B6: Roach_Func_26(); return;  // 0xa3a4b6
  case 0xA3A4F0: Roach_Func_27(); return;  // 0xa3a4f0
  default: nt_unimpl("Unreachable");
  }
}
void Roach_Main(void) {  // 0xA3A2D0
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallRoachFunc(E->ai_var_B | 0xA30000);
}
void Roach_Func_9(void) {  // 0xA3A2D7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (IsSamusWithinEnemy_X(cur_enemy_index, LOBYTE(E->parameter_2))) {
    if (IsSamusWithinEnemy_Y(cur_enemy_index, LOBYTE(E->parameter_2)))
      E->ai_var_B = ((uint16)BW_A3(0xA121 + 2 * (HIBYTE(E->parameter_2))));
  }
}
void Roach_Func_10(void) {  // 0xA3A301
  gEnemyData(cur_enemy_index)->ai_var_B = 0xA407 /* Roach_Func_19 */;
}
void Roach_Func_11(void) {  // 0xA3A30B
  gEnemyData(cur_enemy_index)->ai_var_B = 0xA40E /* Roach_Func_20 */;
}
void Roach_Func_12(void) {  // 0xA3A315
  random_number = 11;
  gEnemyData(cur_enemy_index)->ai_var_B = 0xA4F0 /* Roach_Func_27 */;
}
void Roach_Func_13(void) {  // 0xA3A325
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = 512;
  random_number = 11;
  E->ai_var_B = 0xA4B6 /* Roach_Func_26 */;
}
void Roach_Func_14(void) {  // 0xA3A33B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_preinstr = 32;
  E->ai_var_B = 0xA44E /* Roach_Func_23 */;
}
void Roach_Func_15(void) {  // 0xA3A34B
  uint8 v1 = 64 - CalculateAngleOfSamusFromEnemy(cur_enemy_index);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = v1;
  Roach_Func_18(cur_enemy_index);
  Roach_Func_17(cur_enemy_index);
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8006 + E_k) >> 1)));
  Roach_Func_1();
  E->ai_var_B = 0xA440 /* Roach_Func_21 */;
}
void Roach_Func_16(void) {  // 0xA3A380
  uint16 v1 = (uint8)(64 - CalculateAngleOfSamusFromEnemy(cur_enemy_index) + 0x80);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = v1;
  Roach_Func_18(cur_enemy_index);
  Roach_Func_17(cur_enemy_index);
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8006 + E_k) >> 1)));
  Roach_Func_1();
  E->ai_var_B = 0xA447 /* Roach_Func_22 */;
}
void Roach_Func_17(uint16 k) {  // 0xA3A3B5
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x8006 + E_k) = 2 * ((uint8)(E->ai_var_C - 48) >> 5);
}
void Roach_Func_18(uint16 k) {  // 0xA3A3CA
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  SetHiLo(&ERAM(0x781A + E_k), &ERAM(0x7818 + E_k), CosineMult8bitFull(E->ai_var_C, LOBYTE(E->parameter_1)));
  SetHiLo(&ERAM(0x781E + E_k), &ERAM(0x781C + E_k), SineMult8bitFull(E->ai_var_C, LOBYTE(E->parameter_1)));
}
void Roach_Func_19(void) {  // 0xA3A407
  Roach_Func_29(cur_enemy_index);
}
void Roach_Func_20(void) {  // 0xA3A40E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((E->frame_counter & 0x10) != 0) {
    ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8004 + E_k) >> 1)));
    Roach_Func_1();
    Roach_Func_31(cur_enemy_index);
  } else {
    ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8002 + E_k) >> 1)));
    Roach_Func_1();
    Roach_Func_30(cur_enemy_index);
  }
}
void Roach_Func_21(void) {  // 0xA3A440
  Roach_Func_32(cur_enemy_index);
}
void Roach_Func_22(void) {  // 0xA3A447
  Roach_Func_32(cur_enemy_index);
}
void Roach_Func_23(void) {  // 0xA3A44E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_preinstr & 0x8000) != 0)
    E->ai_var_B = 0xA2D7 /* Roach_Func_9 */;
  else
    Roach_Func_29(cur_enemy_index);
}
void Roach_Func_24(void) {  // 0xA3A462
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_preinstr & 0x8000) != 0)
    E->ai_var_B = 0xA4F0 /* Roach_Func_27 */;
  else
    Roach_Func_33(cur_enemy_index);
}
void Roach_Func_25(void) {  // 0xA3A476
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  bool v2 = (--E->ai_var_A & 0x8000) != 0;
  if (v2) {
LABEL_6:
    Roach_Func_32(cur_enemy_index);
    return;
  }
  v2 = (--E->ai_preinstr & 0x8000) != 0;
  if (!v2) {
    uint16 v3 = Abs16(E->x_pos - samus_x_pos);
    if (!sign16(v3 - 96)) {
      uint16 v4 = Abs16(E->y_pos - samus_y_pos);
      if (!sign16(v4 - 96))
        Roach_Func_28();
    }
    goto LABEL_6;
  }
  E->ai_var_B = 0xA4B6 /* Roach_Func_26 */;
}
void Roach_Func_26(void) {  // 0xA3A4B6
  uint8 v1 = NextRandom() - 64;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C += v1;
  Roach_Func_18(cur_enemy_index);
  Roach_Func_17(cur_enemy_index);
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8006 + E_k) >> 1)));
  Roach_Func_1();
  E->ai_preinstr = 32;
  E->ai_var_B = 0xA476 /* Roach_Func_25 */;
}
void Roach_Func_27(void) {  // 0xA3A4F0
  uint8 v1 = NextRandom() - 64;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C += v1;
  Roach_Func_18(cur_enemy_index);
  Roach_Func_17(cur_enemy_index);
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (ERAM(0x8006 + E_k) >> 1)));
  Roach_Func_1();
  E->ai_preinstr = 32;
  E->ai_var_B = 0xA462 /* Roach_Func_24 */;
}
void Roach_Func_28(void) {  // 0xA3A52A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781A + E_k) = -ERAM(0x781A + E_k);
  ERAM(0x7818 + E_k) = -ERAM(0x7818 + E_k);
  ERAM(0x781E + E_k) = -ERAM(0x781E + E_k);
  ERAM(0x781C + E_k) = -ERAM(0x781C + E_k);
  uint16 v1 = (ERAM(0x8006 + E_k) + 4) & 7;
  ERAM(0x8006 + E_k) = v1;
  ERAM(0x8008 + E_k) = ((uint16)BW_A3(0xA111 + 2 * (v1 >> 1)));
  Roach_Func_1();
}
void Roach_Func_29(uint16 k) {  // 0xA3A578
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 varE20 = HIBYTE(E->parameter_1);
  EnemyFunc_B691(varE20, (Point32) { __PAIR32__(ERAM(0x7802 + E_k), ERAM(0x7800 + E_k)), __PAIR32__(ERAM(0x7806 + E_k), ERAM(0x7804 + E_k)) });
}
void Roach_Func_30(uint16 k) {  // 0xA3A5A3
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(ERAM(0x780A + E_k), ERAM(0x7808 + E_k)));
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x780E + E_k), ERAM(0x780C + E_k)));
}
void Roach_Func_31(uint16 k) {  // 0xA3A5DA
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(ERAM(0x7812 + E_k), ERAM(0x7810 + E_k)));
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x7816 + E_k), ERAM(0x7814 + E_k)));
}
void Roach_Func_32(uint16 k) {  // 0xA3A611
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(ERAM(0x781A + E_k), ERAM(0x7818 + E_k)));
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(ERAM(0x781E + E_k), ERAM(0x781C + E_k)));
}
void Roach_Func_33(uint16 k) {  // 0xA3A648
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(ERAM(0x781A + E_k), ERAM(0x7818 + E_k)))) {
    E->ai_var_B = 0xA2D7 /* Roach_Func_9 */;
  } else {
    if (Enemy_MoveDown(k, __PAIR32__(ERAM(0x781E + E_k), ERAM(0x781C + E_k))))
      E->ai_var_B = 0xA2D7 /* Roach_Func_9 */;
  }
}
uint16 Sidehopper_Func_1(uint16 k, uint16 jp) {  // 0xA3AA68
  QueueSfx2_Max3(IW_A3(jp));
  return (uint16)(jp + 2);
}
uint16 Sidehopper_Instr_1(uint16 k, uint16 jp) {  // 0xA3AAFE
  ERAM(0x7808 + (cur_enemy_index)) = 1;
  return jp;
}
void Sidehopper_Init(void) {  // 0xA3AB09
  random_number = 37;
  NextRandom();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7806 + E_k) = 0;
  ERAM(0x7808 + E_k) = 0;
  ERAM(0x780A + E_k) = 0;
  uint16 v2 = 2 * get_EnemyDef_A2(E->enemy_ptr)->field_2A;
  ERAM(0x780C + E_k) = v2;
  uint16 v4;
  if (E->parameter_1)
    v4 = ((uint16)BW_A3(0xAACA + 2 * (ERAM(0x780C + E_k) >> 1)));
  else
    v4 = ((uint16)BW_A3(0xAAC2 + 2 * (ERAM(0x780C + E_k) >> 1)));
  ERAM(0x7800 + E_k) = v4;
  Sidehopper_Func_3();
  if (get_EnemyDef_A2(E->enemy_ptr)->field_2A)
    ERAM(0x780A + E_k) = 2;
  int v6 = ERAM(0x780A + E_k) >> 1;
  ERAM(0x7802 + E_k) = Sidehopper_Func_2(((uint16)BW_A3(0xAAEE + 2 * (v6))), ((uint16)BW_A3(0xAAE6 + 2 * (v6))));
  int v8 = ERAM(0x780A + E_k) >> 1;
  ERAM(0x7804 + E_k) = Sidehopper_Func_2(((uint16)BW_A3(0xAAFA + 2 * (v8))), ((uint16)BW_A3(0xAAF2 + 2 * (v8))));
  E->ai_var_B = 0xABD6 /* Sidehopper_Func_4 */;
}
uint16 Sidehopper_Func_2(uint16 r22, uint16 r24) {  // 0xA3AB9D
  uint16 r18 = 0, r20 = 0;
  do {
    r18 += r24;
    r20 += ((uint16)BW_A2(0x838F + ((8 * r18) + 1)));
  } while (sign16(r20 - r22));
  return r18;
}
void Sidehopper_Func_3(void) {  // 0xA3ABBB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = ERAM(0x7800 + E_k);
  E->instruction_timer = 1;
  E->timer = 0;
}
void Sidehopper_Main(void) {  // 0xA3ABCF
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallSidehopperFunc(E->ai_var_B | 0xA30000);
}
void Sidehopper_Func_4(uint16 k) {  // 0xA3ABD6
  uint16 v1 = ((uint16)BW_A3(0xAAE2 + 2 * (NextRandom() & 1)));
  gEnemyData(k)->ai_var_B = v1;
}
void Sidehopper_Func_5(void) {  // 0xA3ABE6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x780A + E_k) >> 1;
  E->ai_var_E = ((uint16)BW_A3(0xAAE6 + 2 * (v1)));
  E->ai_var_D = ((uint16)BW_A3(0xAAEA + 2 * (v1)));
  E->ai_var_C = ERAM(0x7802 + E_k);
  E->ai_var_B = 0xAC40 /* Sidehopper_Func_7 */;
  if (E->parameter_1)
    E->ai_var_B = 0xAC56 /* Sidehopper_Func_8 */;
}
void Sidehopper_Func_6(void) {  // 0xA3AC13
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x780A + E_k) >> 1;
  E->ai_var_E = ((uint16)BW_A3(0xAAF2 + 2 * (v1)));
  E->ai_var_D = ((uint16)BW_A3(0xAAF6 + 2 * (v1)));
  E->ai_var_C = ERAM(0x7804 + E_k);
  E->ai_var_B = 0xAC40 /* Sidehopper_Func_7 */;
  if (E->parameter_1)
    E->ai_var_B = 0xAC56 /* Sidehopper_Func_8 */;
}
void Sidehopper_Func_7(void) {  // 0xA3AC40
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = 0xAC6C /* Sidehopper_Func_9 */;
  if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) == 0)
    E->ai_var_B = 0xAC8F /* Sidehopper_Func_10 */;
}
void Sidehopper_Func_8(void) {  // 0xA3AC56
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = 0xACA8 /* Sidehopper_Func_11 */;
  if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) == 0)
    E->ai_var_B = 0xACCB /* Sidehopper_Func_12 */;
}
void Sidehopper_Func_9(void) {  // 0xA3AC6C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_D = -E->ai_var_D;
  ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAAD2 + 2 * (ERAM(0x780C + E_k) >> 1)));
  Sidehopper_Func_3();
  E->ai_var_B = 0xAD0E /* Sidehopper_Func_14 */;
}
void Sidehopper_Func_10(void) {  // 0xA3AC8F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAAD2 + 2 * (ERAM(0x780C + E_k) >> 1)));
  Sidehopper_Func_3();
  E->ai_var_B = 0xAD20 /* Sidehopper_Func_15 */;
}
void Sidehopper_Func_11(void) {  // 0xA3ACA8
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_D = -E->ai_var_D;
  ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAADA + 2 * (ERAM(0x780C + E_k) >> 1)));
  Sidehopper_Func_3();
  E->ai_var_B = 0xAD32 /* Sidehopper_Func_16 */;
}
void Sidehopper_Func_12(void) {  // 0xA3ACCB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAADA + 2 * (ERAM(0x780C + E_k) >> 1)));
  Sidehopper_Func_3();
  E->ai_var_B = 0xAD44 /* Sidehopper_Func_17 */;
}
void Sidehopper_Func_13(void) {  // 0xA3ACE4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAAC2 + 2 * (ERAM(0x780C + E_k) >> 1)));
  if (E->parameter_1)
    ERAM(0x7800 + E_k) = ((uint16)BW_A3(0xAACA + 2 * (ERAM(0x780C + E_k) >> 1)));
  Sidehopper_Func_3();
  E->ai_var_B = 0xAD56 /* Sidehopper_Func_18 */;
}
void Sidehopper_Func_14(void) {  // 0xA3AD0E
  if (ERAM(0x7806 + (cur_enemy_index)))
    Sidehopper_Func_20();
  else
    Sidehopper_Func_19();
}
void Sidehopper_Func_15(void) {  // 0xA3AD20
  if (ERAM(0x7806 + (cur_enemy_index)))
    Sidehopper_Func_20();
  else
    Sidehopper_Func_19();
}
void Sidehopper_Func_16(void) {  // 0xA3AD32
  if (ERAM(0x7806 + (cur_enemy_index)))
    Sidehopper_Func_22();
  else
    Sidehopper_Func_21();
}
void Sidehopper_Func_17(void) {  // 0xA3AD44
  if (ERAM(0x7806 + (cur_enemy_index)))
    Sidehopper_Func_22();
  else
    Sidehopper_Func_21();
}
void Sidehopper_Func_18(void) {  // 0xA3AD56
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x7808 + E_k)) {
    ERAM(0x7808 + E_k) = 0;
    E->ai_var_B = 0xABD6 /* Sidehopper_Func_4 */;
  }
}
static void CallSidehopperFunc(uint32 ea) {
  switch (ea) {
  case 0xA3ABD6: Sidehopper_Func_4(cur_enemy_index); return;  // 0xa3abd6
  case 0xA3ABE6: Sidehopper_Func_5(); return;  // 0xa3abe6
  case 0xA3AC13: Sidehopper_Func_6(); return;  // 0xa3ac13
  case 0xA3AC40: Sidehopper_Func_7(); return;  // 0xa3ac40
  case 0xA3AC56: Sidehopper_Func_8(); return;  // 0xa3ac56
  case 0xA3AC6C: Sidehopper_Func_9(); return;  // 0xa3ac6c
  case 0xA3AC8F: Sidehopper_Func_10(); return;  // 0xa3ac8f
  case 0xA3ACA8: Sidehopper_Func_11(); return;  // 0xa3aca8
  case 0xA3ACCB: Sidehopper_Func_12(); return;  // 0xa3accb
  case 0xA3ACE4: Sidehopper_Func_13(); return;  // 0xa3ace4
  case 0xA3AD0E: Sidehopper_Func_14(); return;  // 0xa3ad0e
  case 0xA3AD20: Sidehopper_Func_15(); return;  // 0xa3ad20
  case 0xA3AD32: Sidehopper_Func_16(); return;  // 0xa3ad32
  case 0xA3AD44: Sidehopper_Func_17(); return;  // 0xa3ad44
  case 0xA3AD56: Sidehopper_Func_18(); return;  // 0xa3ad56
  default: nt_unimpl("Unreachable");
  }
}
void Sidehopper_Func_19(void) {  // 0xA3AD6D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = (8 * E->ai_var_C) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 3))), ((uint16)BW_A2(0x838F + 2 * (v2 + 2)))))) {
    E->ai_var_D = -E->ai_var_D;
    ERAM(0x7806 + E_k) = 1;
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_D))) {
      E->ai_var_D = -E->ai_var_D;
      ERAM(0x7806 + E_k) = 1;
    } else {
      int16 v3 = E->ai_var_C - E->ai_var_E;
      E->ai_var_C = v3;
      if (v3 < 0) {
        ERAM(0x7806 + E_k) = 1;
        E->ai_var_C = 0;
      }
    }
  }
}
void Sidehopper_Func_20(void) {  // 0xA3ADD4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = (8 * E->ai_var_C) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 1))), ((uint16)BW_A2(0x838F + 2 * (v2)))))) {
    ERAM(0x7806 + E_k) = 0;
    E->ai_var_B = 0xACE4 /* Sidehopper_Func_13 */;
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_D)))
      E->ai_var_D = -E->ai_var_D;
    int16 v3 = E->ai_var_E + E->ai_var_C;
    if (!sign16(v3 - 64))
      v3 = 64;
    E->ai_var_C = v3;
  }
}
void Sidehopper_Func_21(void) {  // 0xA3AE27
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = (8 * E->ai_var_C) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 1))), ((uint16)BW_A2(0x838F + 2 * (v2)))))) {
    E->ai_var_D = -E->ai_var_D;
    ERAM(0x7806 + E_k) = 1;
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_D))) {
      E->ai_var_D = -E->ai_var_D;
      ERAM(0x7806 + E_k) = 1;
    } else {
      int16 v3 = E->ai_var_C - E->ai_var_E;
      E->ai_var_C = v3;
      if (v3 < 0) {
        ERAM(0x7806 + E_k) = 1;
        E->ai_var_C = 0;
      }
    }
  }
}
void Sidehopper_Func_22(void) {  // 0xA3AE8E
  int16 v3;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = (8 * E->ai_var_C) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 3))), ((uint16)BW_A2(0x838F + 2 * (v2 + 2)))))) {
    ERAM(0x7806 + E_k) = 0;
    E->ai_var_B = 0xACE4 /* Sidehopper_Func_13 */;
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_D)))
      E->ai_var_D = -E->ai_var_D;
    v3 = E->ai_var_E + E->ai_var_C;
    if (!sign16(v3 - 64))
      v3 = 64;
    E->ai_var_C = v3;
  }
}
uint16 Skree_Instr_1(uint16 k, uint16 jp) {  // 0xA3C6A4
  gEnemyData(cur_enemy_index)->ai_var_E = 1;
  return jp;
}
void Skree_Init(void) {  // 0xA3C6AE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = 0;
  E->ai_var_D = 0;
  E->ai_var_E = 0;
  E->current_instruction = addr_kSkree_Ilist_C65E;
  E->ai_var_B = 0xC6D5 /* Skree_Func_1 */;
}
void CallSkreeFunc(uint32 ea) {
  switch (ea) {
  case 0xA3C6D5: Skree_Func_1(); return;
  case 0xA3C6F7: Skree_Func_2(cur_enemy_index); return;
  case 0xA3C716: Skree_Func_3(); return;
  case 0xA3C77F: Skree_Func_4(); return;
  default: nt_unimpl("Unreachable");
  }
}
void Skree_Main(void) {  // 0xA3C6C7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallSkreeFunc(E->ai_var_B | 0xA30000);
}
void Skree_Func_1(void) {  // 0xA3C6D5
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (abs16(E->x_pos - samus_x_pos) < 0x30) {
    ++E->ai_var_C;
    Skree_Func_5();
    E->ai_var_B = 0xC6F7 /* Skree_Func_2 */;
  }
}
void Skree_Func_2(uint16 k) {  // 0xA3C6F7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_E) {
    E->ai_var_E = 0;
    ++E->ai_var_C;
    Skree_Func_5();
    E->ai_var_B = 0xC716 /* Skree_Func_3 */;
    QueueSfx2_Max6(0x5B);
  }
}
void Skree_Func_3(void) {  // 0xA3C716
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = 21;
  uint16 v1 = E->properties | 3;
  E->properties = v1;
  if (EnemyFunc_BF8A(cur_enemy_index, v1, INT16_SHL16(6)) & 1) {
    E->instruction_timer = 1;
    E->timer = 0;
    E->ai_var_B = 0xC77F /* Skree_Func_4 */;
    QueueSfx2_Max6(0x5C);
  } else {
    E->y_pos += 6;
    E->x_pos += ((int16)(E->x_pos - samus_x_pos) >= 0) ? -1 : 1;
  }
}
void Skree_Func_4(void) {  // 0xA3C77F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_A-- == 1) {
    gEnemySpawnData(cur_enemy_index)->vram_tiles_index = E->vram_tiles_index | E->palette_index;
    E->palette_index = 2560;
    E->vram_tiles_index = 0;
    E->properties |= kEnemyProps_Deleted;
  } else {
    if (E->ai_var_A == 8) {
      uint16 v2 = cur_enemy_index;
      SpawnEprojWithGfx(8, cur_enemy_index, addr_stru_868BC2);
      SpawnEprojWithGfx(0, v2, addr_stru_868BD0);
      SpawnEprojWithGfx(0, v2, addr_stru_868BDE);
      SpawnEprojWithGfx(0, v2, addr_stru_868BEC);
    }
    ++E->y_pos;
  }
}
void Skree_Func_5(void) {  // 0xA3C7D5
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 skree_var_C = E->ai_var_C;
  if (skree_var_C != E->ai_var_D) {
    E->ai_var_D = skree_var_C;
    E->current_instruction = ((uint16)BW_A3(0xC69C + 2 * (skree_var_C)));
    E->instruction_timer = 1;
    E->timer = 0;
  }
}
void Skree_Shot(void) {  // 0xA3C7F5
  Enemy_NormalShotAI_SkipSomeParts_A3();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->health) {
    uint16 v1 = cur_enemy_index;
    SpawnEprojWithGfx(E->ai_var_A, cur_enemy_index, addr_stru_868BC2);
    SpawnEprojWithGfx(0, v1, addr_stru_868BD0);
    SpawnEprojWithGfx(0, v1, addr_stru_868BDE);
    SpawnEprojWithGfx(0, v1, addr_stru_868BEC);
    uint16 v5 = 2;
    if ((projectile_type[collision_detection_index] & 0xF00) != 512)
      v5 = 0;
    EnemyDeathAnimation(2 * collision_detection_index, v5);
  }
}
static uint32 Shift8AddMagn(int16 a, int s) {
  int32 r = (a << 8);
  r += (r < 0) ? -(s << 16) : (s << 16);
  return r;
}
uint16 WreckedShipOrangeZoomer_Func_1(uint16 k, uint16 jp) {  // 0xA3DFC2
  gEnemyData(k)->ai_preinstr = IW_A3((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
void WreckedShipOrangeZoomer_Init(void) {  // 0xA3E043
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = ((uint16)BW_A3(0xE03B + 2 * (E->current_instruction & 3)));
  E->ai_preinstr = addr_locret_A3E08A;
  uint16 v1 = ((uint16)BW_A3(0xE5F0 + 2 * (E->parameter_1)));
  E->ai_var_A = v1;
  E->ai_var_B = v1;
  if ((E->properties & 3) != 0) {
    if ((E->properties & 3) == 2)
      E->ai_var_B = -E->ai_var_B;
  } else {
    E->ai_var_A = -E->ai_var_A;
  }
}
void WreckedShipOrangeZoomer_Main(void) {  // 0xA3E08B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xA30000);
}
void WreckedShipOrangeZoomer_Func_2(uint16 k) {  // 0xA3E091
  int16 v7;
  uint16 v9;
  uint16 v12;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);

  if (earthquake_timer == 30 && earthquake_type == 20) {
    ERAM(0x7806 + E_k) = E->ai_preinstr;
    E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
  }
  if (Enemy_MoveRight_ProcessSlopes(k, Shift8AddMagn(E->ai_var_A, 1))) {
    ERAM(0x7808 + E_k) = 0;
    EnemyFunc_C8AD(k);
    if (!(Enemy_MoveDown(k, INT16_SHL8(E->ai_var_B)))) {
      if ((int16)(samus_y_pos - E->y_pos) >= 0) {
        v7 = E->ai_var_B;
        if (v7 < 0)
          LABEL_17:
        v7 = -v7;
      } else {
        v7 = E->ai_var_B;
        if (v7 >= 0)
          goto LABEL_17;
      }
      E->ai_var_B = v7;
      return;
    }
    E->ai_var_A = -E->ai_var_A;
    v9 = addr_kWreckedShipOrangeZoomer_Ilist_E01F;
    if ((E->ai_var_B & 0x8000) != 0)
      v9 = addr_kWreckedShipOrangeZoomer_Ilist_E003;
    E->current_instruction = v9;
    E->instruction_timer = 1;
  } else {
    uint16 v11 = ERAM(0x7808 + E_k) + 1;
    ERAM(0x7808 + E_k) = v11;
    if (sign16(v11 - 4)) {
      E->ai_var_B = -E->ai_var_B;
      v12 = addr_kWreckedShipOrangeZoomer_Ilist_E01F;
      if ((E->ai_var_B & 0x8000) != 0)
        v12 = addr_kWreckedShipOrangeZoomer_Ilist_E003;
      E->current_instruction = v12;
      E->instruction_timer = 1;
    } else {
      ERAM(0x7806 + E_k) = E->ai_preinstr;
      E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
    }
  }
}
void sub_A3E168(uint16 k) {  // 0xA3E168
  int16 v7;
  uint16 v9;
  uint16 v12;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);

  if (earthquake_timer == 30 && earthquake_type == 20) {
    ERAM(0x7806 + E_k) = E->ai_preinstr;
    E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
  }
  if (Enemy_MoveDown(k, Shift8AddMagn(E->ai_var_B, 1))) {
    ERAM(0x7808 + E_k) = 0;
    if (!(Enemy_MoveRight_ProcessSlopes(k, INT16_SHL8(E->ai_var_A)))) {
      EnemyFunc_C8AD(k);
      if ((int16)(samus_x_pos - E->x_pos) >= 0) {
        v7 = E->ai_var_A;
        if (v7 < 0)
          v7 = -v7;
      } else {
        v7 = E->ai_var_A;
        if (v7 >= 0)
          v7 = -v7;
      }
      E->ai_var_A = v7;
      return;
    }
    E->ai_var_B = -E->ai_var_B;
    v9 = addr_kWreckedShipOrangeZoomer_Ilist_DFE7;
    if ((E->ai_var_A & 0x8000) != 0)
      v9 = addr_kWreckedShipOrangeZoomer_Ilist_DFCB;
    E->current_instruction = v9;
    E->instruction_timer = 1;
  } else {
    uint16 v11 = ERAM(0x7808 + E_k) + 1;
    ERAM(0x7808 + E_k) = v11;
    if (sign16(v11 - 4)) {
      E->ai_var_A = -E->ai_var_A;
      v12 = addr_kWreckedShipOrangeZoomer_Ilist_DFE7;
      if ((E->ai_var_A & 0x8000) != 0)
        v12 = addr_kWreckedShipOrangeZoomer_Ilist_DFCB;
      E->current_instruction = v12;
      E->instruction_timer = 1;
    } else {
      ERAM(0x7806 + E_k) = E->ai_preinstr;
      E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
    }
  }
}
void BigEyeBugs_Init(void) {  // 0xA3E2D4
  EnemyData *v1 = gEnemyData(cur_enemy_index);
  v1->current_instruction = ((uint16)BW_A3(0xE2CC + 2 * (v1->current_instruction & 3)));
  StoneZoomer_E67A(cur_enemy_index);
}
void FireZoomer_Init(void) {  // 0xA3E59C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = ((uint16)BW_A3(0xE2CC + 2 * ((E->current_instruction & 3))));
  StoneZoomer_E67A(cur_enemy_index);
}
uint16 Zoomer_Instr_SetPreinstr(uint16 k, uint16 jp) {  // 0xA3E660
  gEnemyData(k)->ai_preinstr = IW_A3((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
void StoneZoomer_Init(void) {  // 0xA3E669
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = ((uint16)BW_A3(0xE2CC + 2 * ((E->current_instruction & 3))));
  StoneZoomer_E67A(cur_enemy_index);
}
void StoneZoomer_E67A(uint16 k) {  // 0xA3E67A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->spritemap_pointer = addr_kSpritemap_Nothing_A3;
  E->instruction_timer = 1;
  E->ai_preinstr = 0xE6C1 /* nullsub_304 */;
  if (E->parameter_1 != 255) {
    uint16 v2 = ((uint16)BW_A3(0xE5F0 + 2 * (E->parameter_1)));
    E->ai_var_A = v2;
    E->ai_var_B = v2;
  }
  if ((E->properties & 3) != 0) {
    if ((E->properties & 3) == 2)
      E->ai_var_B = -E->ai_var_B;
  } else {
    E->ai_var_A = -E->ai_var_A;
  }
}
void StoneZoomer_Main(void) {  // 0xA3E6C2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xA30000);
}
void FireZoomer_Func_1(uint16 k) {  // 0xA3E6C8
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (earthquake_timer == 30 && earthquake_type == 20) {
    ERAM(0x7806 + E_k) = E->ai_preinstr;
    E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
  }
  if (Enemy_MoveRight_IgnoreSlopes(k, Shift8AddMagn(E->ai_var_A, 1))) {
    ERAM(0x7808 + E_k) = 0;
    EnemyFunc_C8AD(k);
    if (Enemy_MoveDown(k, INT16_SHL8(E->ai_var_B))) {
      E->ai_var_A = -E->ai_var_A;
      uint16 fzr_parameter_2 = E->parameter_2, v7;
      if ((E->ai_var_B & 0x8000) == 0)
        v7 = ((uint16)BW_A3(0xE63C + 2 * (fzr_parameter_2 >> 1)));
      else
        v7 = ((uint16)BW_A3(0xE630 + 2 * (fzr_parameter_2 >> 1)));
      E->current_instruction = v7;
      E->instruction_timer = 1;
    }
  } else {
    uint16 v8 = ERAM(0x7808 + E_k) + 1;
    ERAM(0x7808 + E_k) = v8;
    if (sign16(v8 - 4)) {
      E->ai_var_B = -E->ai_var_B;
      uint16 v9 = E->parameter_2, v10;
      if ((E->ai_var_B & 0x8000) == 0)
        v10 = ((uint16)BW_A3(0xE63C + 2 * (v9 >> 1)));
      else
        v10 = ((uint16)BW_A3(0xE630 + 2 * (v9 >> 1)));
      E->current_instruction = v10;
      E->instruction_timer = 1;
    } else {
      ERAM(0x7806 + E_k) = E->ai_preinstr;
      E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
    }
  }
}
void FireZoomer_Func_2(uint16 k) {  // 0xA3E785
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, __PAIR32__(ERAM(0x7804 + E_k), ERAM(0x7802 + E_k)))) {
    if (E->parameter_1 == 255) {
      E->ai_var_A = 128;
      E->ai_var_B = 128;
    }
    ERAM(0x7802 + E_k) = 0;
    ERAM(0x7804 + E_k) = 0;
    ERAM(0x7808 + E_k) = 0;
    E->ai_preinstr = ERAM(0x7806 + E_k);
  } else {
    if (sign16(ERAM(0x7804 + E_k) - 4))
      AddToHiLo(&ERAM(0x7804 + E_k), &ERAM(0x7802 + E_k), 0x8000);
    if (!ERAM(0x7802 + E_k) && !ERAM(0x7804 + E_k))
      E->ai_preinstr = 0xE6C8 /* FireZoomer_Func_1 */;
  }
}
void FireZoomer_Func_3(uint16 k) {  // 0xA3E7F2
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);

  if (earthquake_timer == 30 && earthquake_type == 20) {
    ERAM(0x7806 + E_k) = E->ai_preinstr;
    E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
  }
  if (Enemy_MoveDown(k, Shift8AddMagn(E->ai_var_B, 1))) {
    ERAM(0x7808 + E_k) = 0;
    int32 amt = FireZoomer_E8A5(k);
    if (Enemy_MoveRight_IgnoreSlopes(k, amt)) {
      E->ai_var_B = -E->ai_var_B;
      uint16 fzr_parameter_2 = E->parameter_2, v6;
      if ((E->ai_var_A & 0x8000) == 0)
        v6 = ((uint16)BW_A3(0xE654 + 2 * (fzr_parameter_2 >> 1)));
      else
        v6 = ((uint16)BW_A3(0xE648 + 2 * (fzr_parameter_2 >> 1)));
      E->current_instruction = v6;
      E->instruction_timer = 1;
    } else {
      EnemyFunc_C8AD(k);
    }
  } else {
    uint16 v7 = ERAM(0x7808 + E_k) + 1;
    ERAM(0x7808 + E_k) = v7;
    if (sign16(v7 - 4)) {
      E->ai_var_A = -E->ai_var_A;
      uint16 v8 = E->parameter_2, v9;
      if ((E->ai_var_A & 0x8000) == 0)
        v9 = ((uint16)BW_A3(0xE654 + 2 * (v8 >> 1)));
      else
        v9 = ((uint16)BW_A3(0xE648 + 2 * (v8 >> 1)));
      E->current_instruction = v9;
      E->instruction_timer = 1;
    } else {
      ERAM(0x7806 + E_k) = E->ai_preinstr;
      E->ai_preinstr = 0xE785 /* FireZoomer_Func_2 */;
    }
  }
}
int32 FireZoomer_E8A5(uint16 k) {  // 0xA3E8A5
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);

  uint16 xpos = E->x_pos;
  uint16 ypos = sign16(E->ai_var_B) ?
    E->y_pos - E->y_height :
    E->y_pos + E->y_height - 1;
  CalculateBlockContainingPixelPos(xpos, ypos);

  if ((level_get(cur_block_index) & 0xF000) == 4096 && (bts_get(cur_block_index) & 0x1F) >= 5) {
    uint16 v1 = ((uint16)BW_A3(0xE931 + 2 * ((uint16)(4 * (bts_get(cur_block_index) & 0x1F)) >> 1)));
    if ((int16)E->ai_var_A >= 0)
      return Multiply16x16(E->ai_var_A, v1);
    else
      return -(int32)Multiply16x16(-E->ai_var_A, v1);
  } else {
    return INT16_SHL8(E->ai_var_A);
  }
}
uint16 Draygon_Instr_1(uint16 k, uint16 jp) {  // 0xA594DD
  gEnemyData(0)->current_instruction = IW_A5((uint16)(jp + 0));
  gEnemyData(0x40)->current_instruction = IW_A5((uint16)(jp + 2));
  gEnemyData(0x80)->current_instruction = IW_A5((uint16)(jp + 4));
  gEnemyData(0xC0)->current_instruction = IW_A5((uint16)(jp + 6));
  gEnemyData(0)->instruction_timer = 1;
  gEnemyData(0x40)->instruction_timer = 1;
  gEnemyData(0x80)->instruction_timer = 1;
  gEnemyData(0xC0)->instruction_timer = 1;
  return (uint16)(jp + 8);
}
uint16 Draygon_Instr_13(uint16 k, uint16 jp) {  // 0xA59736
  gEnemyData(k)->ai_var_A = IW_A5((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
static Point16U Draygon_Func_47(void) {  // 0xA5978B
  NextRandom();
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  uint16 r18 = (random_number & 0x7F) - 64;
  uint16 r20 = ((random_number & 0x7F00) >> 8) - 64;
  return (Point16U) {r18 + E->x_pos, r20 + E->y_pos};
}
uint16 Draygon_Instr_8(uint16 k, uint16 jp) {  // 0xA5973F
  Point16U pt = Draygon_Func_47();
  CreateSpriteAtPos(pt.x, pt.y, 21, 0);
  return jp;
}
uint16 Draygon_Instr_7(uint16 k, uint16 jp) {  // 0xA59752
  Point16U pt = Draygon_Func_47();
  CreateSpriteAtPos(pt.x, pt.y, 3, 0);
  return jp;
}
uint16 Draygon_Instr_6(uint16 k, uint16 jp) {  // 0xA59765
  Point16U pt = Draygon_Func_47();
  CreateSpriteAtPos(pt.x, pt.y, 29, 0);
  return jp;
}
uint16 Draygon_Instr_9(uint16 k, uint16 jp) {  // 0xA59778
  Point16U pt = Draygon_Func_47();
  CreateSpriteAtPos(pt.x, pt.y, 24, 0);
  return jp;
}
uint16 Draygon_Instr_2(uint16 k, uint16 jp) {  // 0xA59895
  room_loading_irq_handler = 12;
  return jp;
}
uint16 Draygon_Instr_11(uint16 k, uint16 jp) {  // 0xA598D3
  EnemyData *E2 = gEnemyData(0x80); const uint16 E2_k = (0x80);
  E2->instruction_timer = 1;
  E2->current_instruction = addr_kDraygon_Ilist_97B9;

  EnemyData *E3 = gEnemyData(0xC0); const uint16 E3_k = (0xC0);
  E3->instruction_timer = 1;
  E3->current_instruction = addr_kDraygon_Ilist_97B9;
  return jp;
}
uint16 Draygon_Instr_5(uint16 k, uint16 jp) {  // 0xA598EF
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  E->properties |= kEnemyProps_Tangible;
  return jp;
}
uint16 Draygon_Instr_15(uint16 k, uint16 jp) {  // 0xA59B9A
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  ERAM(0x781E + E_k) = 24;
  uint16 v3 = (E->enemy_ptr);
  Samus_DealDamage(SuitDamageDivision(BW_A0(v3 + 6)));
  earthquake_timer = 32;
  earthquake_type = 7;
  CreateSpriteAtPos(samus_x_pos, samus_y_pos + 16, 21, 0);
  return jp;
}
uint16 Draygon_Instr_17(uint16 k, uint16 jp) {  // 0xA59C8A
  room_loading_irq_handler = 12;
  return jp;
}
uint16 Draygon_Instr_14(uint16 k, uint16 jp) {  // 0xA59E0A
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  ERAM(0x883C + E_k) = IW_A5((uint16)(jp + 0));
  ERAM(0x883E + E_k) = IW_A5((uint16)(jp + 2));
  return (uint16)(jp + 4);
}
uint16 Draygon_Instr_16(uint16 k, uint16 jp) {  // 0xA59F57
  gEnemyData(0)->ai_var_A = IW_A5((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_10(uint16 k, uint16 jp) {  // 0xA59F60
  QueueSfx2_Max6(IW_A5((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_4(uint16 k, uint16 jp) {  // 0xA59F6E
  QueueSfx3_Max6(IW_A5((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_12(uint16 k, uint16 jp) {  // 0xA59F7C
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  eproj_spawn_x = E->x_pos - 28; eproj_spawn_y = E->y_pos - 16;
  eproj_unk1995 = (NextRandom() & 0x3F) + 128;
  SpawnEprojWithGfx(2, cur_enemy_index, addr_stru_868E50);
  return jp;
}
uint16 Draygon_Instr_18(uint16 k, uint16 jp) {  // 0xA59FAE
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  eproj_spawn_x = E->x_pos + 24; eproj_spawn_y = E->y_pos - 16;
  eproj_unk1995 = (NextRandom() & 0x3F) + 192;
  SpawnEprojWithGfx(2, cur_enemy_index, addr_stru_868E50);
  return jp;
}
uint16 Draygon_Instr_3(uint16 k, uint16 jp) {  // 0xA5C47B
  gEnemyData(0x40)->ai_var_A = IW_A5((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_25(uint16 k, uint16 jp) {  // 0xA5E75F
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  if (sign16(ERAM(0x7816 + E_k) - 40))
    ERAM(0x7816 + E_k) += 8;
  return jp;
}
uint16 Draygon_Instr_24(uint16 k, uint16 jp) {  // 0xA5E771
  ERAM(0x801E + (0)) = 0;
  return jp;
}
uint16 Draygon_Instr_21(uint16 k, uint16 jp) {  // 0xA5E82D
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  ERAM(0x7816 + E_k) = IW_A5((uint16)(jp + 0));
  ERAM(0x7818 + E_k) = IW_A5((uint16)(jp + 2));
  return (uint16)(jp + 4);
}
uint16 Draygon_Instr_53(uint16 k, uint16 j) {  // 0xA5E854
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  ERAM(0x7816 + E_k) += BW_A5(j);
  return j + 2;
}
uint16 Draygon_Instr_22(uint16 k, uint16 jp) {  // 0xA5E872
  kraid_unk9000 = IW_A5((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_27(uint16 k, uint16 jp) {  // 0xA5E87C
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  E->x_pos = 128;
  E->y_pos = 624;
  E->properties = E->properties & 0x5BFF | 0xA000;
  return jp;
}
uint16 Draygon_Instr_23(uint16 k, uint16 jp) {  // 0xA5E895
  QueueSfx2_Max6(IW_A5((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_30(uint16 k, uint16 jp) {  // 0xA5E8B1
  Enemy_ItemDrop_SporeSpawn(k);
  return jp;
}
uint16 Draygon_Instr_20(uint16 k, uint16 jp) {  // 0xA5E8BA
  gEnemyData(cur_enemy_index)->ai_var_A = IW_A5((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_29(uint16 k, uint16 jp) {  // 0xA5E8CA
  uint16 v2 = IW_A5((uint16)(jp + 0));
  for (int i = 0; i != 32; i += 2)
    palette_buffer[(i >> 1) + 144] = ((uint16)BW_A5(0xE3F9 + 2 * ((v2 + i) >> 1)));
  for (int i = 0; i != 32; i += 2)
    palette_buffer[(i >> 1) + 64] = ((uint16)BW_A5(0xE4F9 + 2 * ((v2 + i) >> 1)));
  for (int i = 0; i != 32; i += 2)
    palette_buffer[(i >> 1) + 112] = ((uint16)BW_A5(0xE5D9 + 2 * ((v2 + i) >> 1)));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_19(uint16 k, uint16 jp) {  // 0xA5E91C
  uint16 v2 = IW_A5((uint16)(jp + 0));
  for (int i = 0; i != 32; i += 2)
    target_palettes[(i >> 1) + 144] = ((uint16)BW_A5(0xE3F9 + 2 * ((v2 + i) >> 1)));
  for (int i = 0; i != 32; i += 2)
    target_palettes[(i >> 1) + 64] = ((uint16)BW_A5(0xE4F9 + 2 * ((v2 + i) >> 1)));
  for (int i = 0; i != 32; i += 2)
    target_palettes[(i >> 1) + 112] = ((uint16)BW_A5(0xE5D9 + 2 * ((v2 + i) >> 1)));
  return (uint16)(jp + 2);
}
uint16 Draygon_Instr_28(uint16 k, uint16 jp) {  // 0xA5E96E
  NextRandom();
  uint16 r18 = (random_number & 0x7F) - 64;
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  r18 += E->x_pos;
  uint16 r20 = ((uint16)(random_number & 0x7F00) >> 8) - 64;
  r20 += E->y_pos;
  eproj_spawn_x = r18; eproj_spawn_y = r20;
  SpawnEprojWithRoomGfx(0xE509, 0x15);
  QueueSfx2_Max6(0x29);
  return jp;
}
uint16 Draygon_Instr_26(uint16 k, uint16 jp) {  // 0xA5E9B1
  NextRandom();
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  uint16 x = E->x_pos + (random_number & 0x7F) - 64;
  uint16 y = E->y_pos + ((uint16)(random_number & 0x3F00) >> 8) - 32;
  CreateSpriteAtPos(x, y, 3, 0);
  QueueSfx2_Max6(0x25);
  return jp;
}
void sub_A5E9F5(void) {  // 0xA5E9F5
  if ((nmi_frame_counter_word & 0xF) == 0) {
    NextRandom();
    uint16 x = (random_number & 0x3F) + 96;
    uint16 y = ((uint16)(random_number & 0xF00) >> 8) + 480;
    CreateSpriteAtPos(x, y, 21, 0);
  }
}
void SporeSpawn_Init(void) {  // 0xA5EA2A
  uint16 v0 = 0;
  for (int i = 0; i != 32; i += 2) {
    target_palettes[(i >> 1) + 240] = ((uint16)BW_A5(0xE359 + 2 * (v0 >> 1)));
    v0 += 2;
  }
  SpawnEprojWithGfx(0, cur_enemy_index, addr_kEproj_SporeSpawnsStalk);
  SpawnEprojWithGfx(1, cur_enemy_index, addr_kEproj_SporeSpawnsStalk);
  SpawnEprojWithGfx(2, cur_enemy_index, addr_kEproj_SporeSpawnsStalk);
  SpawnEprojWithGfx(3, cur_enemy_index, addr_kEproj_SporeSpawnsStalk);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
  ERAM(0x7808 + E_k) = E->x_pos;
  ERAM(0x780A + E_k) = E->y_pos - 72;
  E->ai_var_C = E->x_pos;
  E->ai_var_D = E->y_pos;
  E->ai_preinstr = 0;
  if ((boss_bits_for_area[area_index] & 2) != 0) {
    E0->current_instruction = addr_kDraygon_Ilist_E6B9;
    E0->ai_var_A = 0xEB1A /* nullsub_223 */;
    E0->properties |= 0x8000;
    SporeSpawn_Func_5();
    SpawnHardcodedPlm(0x07, 0x1e, 0xb793);
    scrolling_finished_hook = 0;
  } else {
    E0->current_instruction = addr_kDraygon_Ilist_E6C7;
    flag_process_all_enemies = -1;
    uint16 v5 = cur_enemy_index;
    E->ai_var_A = 0xEB1A /* nullsub_223 */;
    scrolling_finished_hook = 0x9589 /* Samus_ScrollFinishedHook_SporeSpawnFight */;
    E->y_pos -= 128;
    SpawnEprojWithGfx(0, v5, addr_kEproj_SporeSpawners);
    SpawnEprojWithGfx(1, v5, addr_kEproj_SporeSpawners);
    SpawnEprojWithGfx(2, v5, addr_kEproj_SporeSpawners);
    SpawnEprojWithGfx(3, v5, addr_kEproj_SporeSpawners);
    SporeSpawn_Func_5();
  }
}
void CallSporeSpawnFunc(uint32 ea) {
  switch (ea) {
  case 0xA5EB1A: return;
  case 0xA5EB1B: SporeSpawn_Func_1(); return;
  case 0xA5EB52: SporeSpawn_Func_2(cur_enemy_index); return;
  case 0xA5EB9B: SporeSpawn_Func_3(); return;
  case 0xA5EBEE: SporeSpawn_Func_4(); return;
  default: nt_unimpl("Unreachable");
  }
}
void SporeSpawn_Main(void) {  // 0xA5EB13
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallSporeSpawnFunc(E->ai_var_A | 0xA50000);
}
void SporeSpawn_Func_1(void) {  // 0xA5EB1B
  SporeSpawn_Func_5();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v1 = E->y_pos + 1;
  E->y_pos = v1;
  if (!sign16(v1 - 624)) {
    E->current_instruction = addr_kDraygon_Ilist_E6D5;
    E->instruction_timer = 1;
  }
  EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
  ERAM(0x7816 + E0_k) = 48;
  ERAM(0x7818 + E0_k) = 1;
  ERAM(0x7814 + E0_k) = 192;
}
void SporeSpawn_Func_2(uint16 k) {  // 0xA5EB52
  SporeSpawn_Func_5();
  EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos = E->ai_var_C + CosineMult8bit(ERAM(0x7814 + E0_k), ERAM(0x7816 + E0_k));
  E->y_pos = E->ai_var_D + SineMult8bit(2 * (ERAM(0x7814 + E0_k) - 64), ERAM(0x7816 + E0_k) - 16);
  ERAM(0x7814 + E0_k) = (uint8)(LOBYTE(ERAM(0x7818 + E0_k)) + ERAM(0x7814 + E0_k));
}
void SporeSpawn_Func_3(void) {  // 0xA5EB9B
  EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
  ERAM(0x8806 + E0_k) = (uint8)(64 - CalculateAngleFromXY(128 - E0->x_pos, 624 - E0->y_pos));
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  Point32 pt = ConvertAngleToXy(LOBYTE(ERAM(0x8806 + E0_k)), 1);
  ERAM(0x8010 + E_k) = pt.x >> 16;
  ERAM(0x8012 + E_k) = pt.x;
  ERAM(0x8014 + E_k) = pt.y >> 16;
  ERAM(0x8016 + E_k) = pt.y;
}
void SporeSpawn_Func_4(void) {  // 0xA5EBEE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
  uint16 varE20 = LOBYTE(ERAM(0x8806 + E0_k));
  EnemyFunc_B691(varE20, (Point32) { __PAIR32__(ERAM(0x8010 + E_k), ERAM(0x8012 + E_k)), __PAIR32__(ERAM(0x8014 + E_k), ERAM(0x8016 + E_k)) });
  uint16 v2 = Abs16(E0->x_pos - 128);
  if (sign16(v2 - 8)) {
    uint16 v3 = Abs16(E0->y_pos - 624);
    if (sign16(v3 - 8))
      E0->ai_var_A = 0xEB1A /* nullsub_223 */;
  }
  SporeSpawn_Func_5();
  sub_A5E9F5();
}
void SporeSpawn_Func_5(void) {  // 0xA5EC49
  EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
  uint16 v1 = E->x_pos - ERAM(0x7808 + E_k), v2;
  uint16 r18, r20, r22;
  if ((v1 & 0x8000) == 0) {
    r18 = v1 >> 1, r20 = v1 >> 2;
    r22 = (v1 >> 1) + (v1 >> 2);
    eproj_x_pos[14] = 128;
    eproj_x_pos[15] = ERAM(0x7808 + E_k) + (v1 >> 2);
    ERAM(0x8000 + E_k) = eproj_x_pos[15];
    eproj_x_pos[16] = ERAM(0x7808 + E_k) + r18;
    ERAM(0x8002 + E_k) = eproj_x_pos[16];
    v2 = ERAM(0x7808 + E_k) + r22;
  } else {
    r18 = (uint16)(ERAM(0x7808 + E_k) - E->x_pos) >> 1;
    r20 = (uint16)-v1 >> 2;
    r22 = r18 + r20;
    eproj_x_pos[14] = 128;
    eproj_x_pos[15] = ERAM(0x7808 + E_k) - r20;
    ERAM(0x8000 + E_k) = eproj_x_pos[15];
    eproj_x_pos[16] = ERAM(0x7808 + E_k) - r18;
    ERAM(0x8002 + E_k) = eproj_x_pos[16];
    v2 = ERAM(0x7808 + E_k) - r22;
  }
  eproj_x_pos[17] = v2;
  ERAM(0x8004 + E_k) = v2;
  uint16 v3 = E->y_pos - 40 - ERAM(0x780A + E_k), v4;
  if ((v3 & 0x8000) == 0) {
    r18 = v3 >> 1, r20 = v3 >> 2;
    r22 = (v3 >> 1) + (v3 >> 2);
    eproj_y_pos[14] = 560;
    eproj_y_pos[15] = ERAM(0x780A + E_k) + (v3 >> 2);
    ERAM(0x8006 + E_k) = eproj_y_pos[15];
    eproj_y_pos[16] = ERAM(0x780A + E_k) + r18;
    ERAM(0x8008 + E_k) = eproj_y_pos[16];
    v4 = ERAM(0x780A + E_k) + r22;
  } else {
    r18 = (uint16)(ERAM(0x780A + E_k) - (E->y_pos - 40)) >> 1;
    r20 = (uint16)-v3 >> 2;
    r22 = r18 + r20;
    eproj_y_pos[14] = 560;
    eproj_y_pos[15] = ERAM(0x780A + E_k) - r20;
    ERAM(0x8006 + E_k) = eproj_y_pos[15];
    eproj_y_pos[16] = ERAM(0x780A + E_k) - r18;
    ERAM(0x8008 + E_k) = eproj_y_pos[16];
    v4 = ERAM(0x780A + E_k) - r22;
  }
  eproj_y_pos[17] = v4;
  ERAM(0x800A + E_k) = v4;
}
void SporeSpawn_Shot(void) {  // 0xA5ED5A
  int16 v4;

  uint16 v0 = 2 * collision_detection_index;
  uint16 v1 = projectile_type[collision_detection_index];
  if ((v1 & 0x700) != 0 || (v1 & 0x10) != 0) {
    EnemyFunc_A6B4_UsedBySporeSpawn();
    v0 = cur_enemy_index;
    EnemyData *EK = gEnemyData(v0); const uint16 EK_k = (v0);
    if (EK->flash_timer) {
      EnemyData *E = gEnemyData(0); const uint16 E_k = (0);
      EK->ai_var_A = 0xEB52 /* SporeSpawn_Func_2 */;
      v4 = 2;
      if (sign16(EK->health - 400)) {
        if ((ERAM(0x7818 + E_k) & 0x8000) != 0)
          v4 = -2;
        ERAM(0x7818 + E_k) = v4;
      }
      if (!ERAM(0x801E + E_k)) {
        ERAM(0x7818 + E_k) = -ERAM(0x7818 + E_k);
        ERAM(0x801E + E_k) = 1;
        EK->current_instruction = addr_stru_A5E729;
        EK->instruction_timer = 1;
        uint16 v7 = 96;
        uint16 health = EK->health;
        if (!sign16(health - 70)) {
          v7 = 64;
          if (!sign16(health - 410)) {
            v7 = 32;
            if (!sign16(health - 770))
              v7 = 0;
          }
        }
        if (health != ERAM(0x8800 + E_k)) {
          ERAM(0x8800 + E_k) = health;
          SporeSpawn_Func_7(v7);
        }
      }
    }
    SporeSpawn_Func_6();
  }
}
void SporeSpawn_Touch(void) {  // 0xA5EDEC
  NormalEnemyTouchAiSkipDeathAnim_CurEnemy();
  SporeSpawn_Func_6();
}
void SporeSpawn_Func_6(void) {  // 0xA5EDF3
  if (!gEnemyData(cur_enemy_index)->health) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    EnemyData *E0 = gEnemyData(0); const uint16 E0_k = (0);
    ERAM(0x781C + E0_k) = 0;
    E->invincibility_timer = 0;
    E->flash_timer = 0;
    E->ai_handler_bits = 0;
    E->properties |= kEnemyProps_Tangible;
    for (int i = 26; i >= 0; i -= 2)
      eproj_id[(uint16)((uint16)i) >> 1] = 0;
    E0->current_instruction = addr_kDraygon_Ilist_E77D;
    E0->instruction_timer = 1;
    boss_bits_for_area[area_index] |= 2;
    scrolling_finished_hook = 0;
    SpawnHardcodedPlm(0x07, 0x1e, 0xb78f);
  }
}
void SporeSpawn_Func_7(uint16 a) {  // 0xA5EE4A
  uint16 v1 = a;
  for (int i = 0; i != 32; i += 2) {
    palette_buffer[(i >> 1) + 144] = ((uint16)BW_A5(0xE379 + 2 * (v1 >> 1)));
    v1 += 2;
  }
}
void Boulder_Init(void) {  // 0xA686F5
  int16 boulder_parameter_2_high;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = 0;
  E->ai_var_B = 0;
  E->ai_var_D = 2;
  E->ai_var_A = 0x879A /* Boulder_Func_1 */;
  ERAM(0x7808 + E_k) = E->x_pos;
  uint16 y_pos = E->y_pos;
  ERAM(0x780A + E_k) = y_pos;
  ERAM(0x7806 + E_k) = y_pos;
  ERAM(0x7804 + E_k) = E->y_subpos;
  boulder_parameter_2_high = HIBYTE(E->parameter_2);
  if (!HIBYTE(E->parameter_2)) {
    boulder_parameter_2_high = 1;
    ERAM(0x780E + E_k) = 1;
  }
  E->y_pos = ERAM(0x780A + E_k) - boulder_parameter_2_high;
  ERAM(0x7800 + E_k) = -LOBYTE(E->parameter_2);
  ERAM(0x780C + E_k) = LOBYTE(E->current_instruction);
  E->current_instruction = addr_kBoulder_Ilist_86A7;
  uint16 boulder_parameter_1_high = HIBYTE(E->parameter_1);
  E->ai_var_E = boulder_parameter_1_high;
  if (!boulder_parameter_1_high) {
    ERAM(0x7800 + E_k) = -ERAM(0x7800 + E_k);
    E->current_instruction = addr_kBoulder_Ilist_86CB;
  }
  ERAM(0x7802 + E_k) = 2;
  if (LOBYTE(E->parameter_1))
    ERAM(0x7802 + E_k) = 5;
}
void CallBoulderFunc(uint32 ea) {
  switch (ea) {
  case 0xA6879A: Boulder_Func_1(); return;
  case 0xA687ED: Boulder_Func_2(); return;
  case 0xA68832: Boulder_Func_3(); return;
  case 0xA6888B: Boulder_Func_4(); return;
  case 0xA68942: Boulder_Func_5(); return;
  case 0xA689FC: Boulder_Func_6(); return;
  default: nt_unimpl("Unreachable");
  }
}
void Boulder_Main(void) {  // 0xA68793
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallBoulderFunc(E->ai_var_A | 0xA60000);
}
void Boulder_Func_1(void) {  // 0xA6879A
  int16 SamusEnemyDelta_Y;
  int16 v3;
  int16 SamusEnemyDelta_X;

  SamusEnemyDelta_Y = GetSamusEnemyDelta_Y(cur_enemy_index);
  if (SamusEnemyDelta_Y >= 0) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    if ((int16)(SamusEnemyDelta_Y - ERAM(0x780C + E_k)) < 0) {
      if (E->ai_var_E) {
        SamusEnemyDelta_X = GetSamusEnemyDelta_X(cur_enemy_index);
        if (SamusEnemyDelta_X < 0 && (int16)(SamusEnemyDelta_X - ERAM(0x7800 + E_k)) >= 0) {
          E->ai_var_A = 0x87ED /* Boulder_Func_2 */;
          if (ERAM(0x780E + E_k))
            E->ai_var_A = 0x8942 /* Boulder_Func_5 */;
        }
      } else {
        v3 = GetSamusEnemyDelta_X(cur_enemy_index);
        if (v3 >= 0 && (int16)(v3 - ERAM(0x7800 + E_k)) < 0) {
          E->ai_var_A = 0x87ED /* Boulder_Func_2 */;
          if (ERAM(0x780E + E_k))
            E->ai_var_A = 0x8942 /* Boulder_Func_5 */;
        }
      }
    }
  }
}
void Boulder_Func_2(void) {  // 0xA687ED
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  Boulder_Func_9(8 * HIBYTE(E->ai_var_C));
  if ((int16)(E->y_pos - ERAM(0x780A + E_k)) < 0) {
    uint16 v1 = E->ai_var_C + 256;
    E->ai_var_C = v1;
    if (!sign16(v1 - 20480))
      E->ai_var_C = 20480;
  } else {
    E->y_pos = ERAM(0x780A + E_k);
    E->ai_var_A = 0x8832 /* Boulder_Func_3 */;
    E->ai_var_C = 0x2000;
  }
}
void Boulder_Func_3(void) {  // 0xA68832
  int16 v1;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  Boulder_Func_9(8 * HIBYTE(E->ai_var_C) + 4);
  v1 = E->ai_var_C - 256;
  E->ai_var_C = v1;
  if (v1 >= 0) {
    uint16 v2 = 8 * HIBYTE(E->ai_var_B);
    if (E->ai_var_E)
      v2 += 4;
    Boulder_Func_7(v2);
    uint16 v3 = E->ai_var_B + 32;
    E->ai_var_B = v3;
    if (!sign16(v3 - 20480))
      E->ai_var_B = 20480;
  } else {
    E->ai_var_C = 0;
    E->ai_var_A = 0x888B /* Boulder_Func_4 */;
  }
}
void Boulder_Func_4(void) {  // 0xA6888B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = (8 * HIBYTE(E->ai_var_C)) >> 1;
  if (Enemy_MoveDown(cur_enemy_index, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v1 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v1 >> 1) + 2) << 16)))) {
    QueueSfx2_Max6(0x42);
    if (E->ai_var_E == 2) {
      E->properties |= kEnemyProps_Deleted;
      eproj_spawn_x = E->x_pos; eproj_spawn_y = E->y_pos;
      SpawnEprojWithRoomGfx(addr_kEproj_DustCloudExplosion, 0x11);
      QueueSfx2_Max6(0x43);
    } else {
      E->ai_var_A = 0x8832 /* Boulder_Func_3 */;
      E->ai_var_C = ((uint16)BW_A6(0x86F1 + 2 * (E->ai_var_D - 1)));
      if ((--E->ai_var_D & 0x8000) != 0) {
        ERAM(0x7806 + E_k) = E->y_pos;
        ERAM(0x7804 + E_k) = E->y_subpos;
        E->ai_var_A = 0x8942 /* Boulder_Func_5 */;
      }
    }
  } else {
    E->ai_var_C += 256;
    uint16 v3 = 8 * HIBYTE(E->ai_var_B);
    if (E->ai_var_E)
      v3 += 4;
    Boulder_Func_7(v3);
    uint16 v4 = E->ai_var_B + 32;
    E->ai_var_B = v4;
    if (!sign16(v4 - 20480))
      E->ai_var_B = 20480;
  }
}
void Boulder_Func_5(void) {  // 0xA68942
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 v2 = 8 * HIBYTE(E->ai_var_B);
  int v3 = v2 >> 1;
  Enemy_MoveDown(cur_enemy_index, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v3 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v3 >> 1) + 2) << 16)) + (ERAM(0x7802 + E_k) << 16));
  E->y_pos -= ERAM(0x7802 + E_k);
  if (E->ai_var_E)
    v2 += 4;
  int v4 = v2 >> 1;
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v4 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v4 >> 1) + 2) << 16)))) {
    E->properties |= 0x300;
    E->ai_var_A = 0x89FC /* Boulder_Func_6 */;
    QueueSfx2_Max6(0x42);
    eproj_spawn_x = E->x_pos; eproj_spawn_y = E->y_pos;
    SpawnEprojWithRoomGfx(addr_kEproj_DustCloudExplosion, 0x11);
    QueueSfx2_Max6(0x43);
  } else {
    uint16 v5 = E->ai_var_B + 64;
    E->ai_var_B = v5;
    if (!sign16(v5 - 0x4000))
      E->ai_var_B = 0x4000;
    if (E->y_pos == ERAM(0x7806 + E_k) && E->y_subpos == ERAM(0x7804 + E_k))
      ERAM(0x7802 + E_k) = 0;
  }
  ERAM(0x7806 + E_k) = E->y_pos;
  ERAM(0x7804 + E_k) = E->y_subpos;
}
void Boulder_Func_6(void) {  // 0xA689FC
  ;
}
void Boulder_Func_7(uint16 j) {  // 0xA68A00
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v3 = j >> 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v3 + 1))), ((uint16)BW_A2(0x838F + 2 * (v3)))));
}
void Boulder_Func_8(uint16 j) {  // 0xA68A1D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v3 = j >> 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (v3 + 1))), ((uint16)BW_A2(0x8187 + 2 * (v3)))));
}
void Boulder_Func_9(uint16 j) {  // 0xA68A3A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v3 = j >> 1;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v3 + 1))), ((uint16)BW_A2(0x838F + 2 * (v3)))));
}
void FakeKraid_Init(void) {  // 0xA69A58
  uint16 v0 = (random_number & 3) + 2;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_D = v0;
  E->ai_var_E = v0;
  v0 += 64;
  ERAM(0x7806 + E_k) = v0;
  v0 += 32;
  ERAM(0x7808 + E_k) = v0;
  ERAM(0x780A + E_k) = v0 - 48;
  ERAM(0x780E + E_k) = 0;
  E->properties |= kEnemyProps_DisableSamusColl;
  E->instruction_timer = 1;
  E->timer = 0;
  E->ai_var_B = -4;
  E->ai_var_C = -4;
  uint16 v2 = addr_stru_A699AE;
  if ((int16)(E->x_pos - samus_x_pos) < 0) {
    E->ai_var_B = 4;
    E->ai_var_C = 4;
    v2 = addr_stru_A699FC;
  }
  E->current_instruction = v2;
}
void FakeKraid_Main(void) {  // 0xA69AC2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 fkd_var_07 = ERAM(0x780E + E_k);
  uint16 v3 = fkd_var_07 + 2;
  if (!sign16(fkd_var_07 - 4))
    v3 = 0;
  ERAM(0x780E + E_k) = v3;
  FakeKraid_Func_1(cur_enemy_index, fkd_var_07);
}
void FakeKraid_Func_1(uint16 k, uint16 j) {  // 0xA69ADC
  EnemyData *ET = gEnemyData(k + j); const uint16 ET_k = (k + j);
  uint16 fkd_var_03 = ERAM(0x7806 + ET_k);
  if (fkd_var_03) {
    ERAM(0x7806 + ET_k) = fkd_var_03 - 1;
  } else {
    ERAM(0x7806 + ET_k) = (random_number & 0x3F) + 16;
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    ERAM(0x780C + E_k) = j;
    uint16 v6 = addr_kEproj_MiniKraidSpikesLeft;
    int16 fkd_var_C = E->ai_var_C;
    if (fkd_var_C >= 0)
      v6 = addr_kEproj_MiniKraidSpikesRight;
    SpawnEprojWithGfx(0, k, v6);
    if (!CheckIfEnemyIsOnScreen())
      QueueSfx2_Max6(0x3F);
  }
}
uint16 FakeKraid_Instr_2(uint16 k, uint16 jp) {  // 0xA69B26
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_E)
    --E->ai_var_E;
  if (E->ai_var_D-- == 1) {
    E->ai_var_D = (random_number & 3) + 7;
  } else {
    if (!(Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_B))))
      goto LABEL_7;
  }
  E->ai_var_B = -E->ai_var_B;
LABEL_7:
  E->ai_var_C = -4;
  if ((int16)(E->x_pos - samus_x_pos) < 0)
    E->ai_var_C = 4;
  return jp;
}
uint16 FakeKraid_Instr_1(uint16 k, uint16 jp) {  // 0xA69B74
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_E) {
    if ((E->ai_var_C & 0x8000) != 0) {
      if ((gEnemyData(cur_enemy_index)->ai_var_B & 0x8000) == 0)
        return (uint16)(addr_stru_A699C6);
      return (uint16)(addr_stru_A699AE);
    } else {
      if ((E->ai_var_B & 0x8000) != 0)
        return (uint16)(addr_stru_A69A14);
      return (uint16)(addr_stru_A699FC);
    }
  } else {
    E->ai_var_E = (random_number & 3) + 3;
    if ((E->ai_var_C & 0x8000) != 0)
      return (uint16)(addr_kFakeKraid_Ilist_99DC);
    return (uint16)(addr_kFakeKraid_Ilist_9A2A);
  }
}
uint16 FakeKraid_Instr_3(uint16 k, uint16 jp) {  // 0xA69BB2
  if (CheckIfEnemyIsOnScreen() == 0)
    QueueSfx2_Max6(0x16);
  return jp;
}
uint16 FakeKraid_Instr_4(uint16 k, uint16 jp) {  // 0xA69BC4
  FakeKraid_InstrHelper_45(k, 0, 0xFFFC);
  return jp;
}
void FakeKraid_InstrHelper_45(uint16 k, uint16 j, uint16 a) {  // 0xA69BCB
  uint16 ka = cur_enemy_index;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7804 + E_k) = a;
  int v5 = j >> 1;
  ERAM(0x7800 + E_k) = ((uint16)BW_A6(0x9A48 + 2 * (v5)));
  uint16 v6 = ((uint16)BW_A6(0x9A48 + 2 * (v5 + 1)));
  ERAM(0x7802 + E_k) = v6;
  SpawnEprojWithGfx(v6, cur_enemy_index, addr_kEproj_MiniKraidSpit);
  ERAM(0x7800 + E_k) = ((uint16)BW_A6(0x9A48 + 2 * (v5 + 2)));
  uint16 v8 = ((uint16)BW_A6(0x9A48 + 2 * (v5 + 3)));
  ERAM(0x7802 + E_k) = v8;
  SpawnEprojWithGfx(v8, ka, addr_kEproj_MiniKraidSpit);
}
uint16 FakeKraid_Instr_5(uint16 k, uint16 jp) {  // 0xA69C02
  FakeKraid_InstrHelper_45(k, 8, 4);
  return jp;
}
void FakeKraid_Touch(void) {  // 0xA69C22
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  special_death_item_drop_x_origin_pos = E->x_pos;
  special_death_item_drop_y_origin_pos = E->y_pos;
  NormalEnemyTouchAiSkipDeathAnim_CurEnemy();
  FakeKraid_9C50();
}
void FakeKraid_Shot(void) {  // 0xA69C39
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  special_death_item_drop_x_origin_pos = E->x_pos;
  special_death_item_drop_y_origin_pos = E->y_pos;
  NormalEnemyShotAiSkipDeathAnim_CurEnemy();
  FakeKraid_9C50();
}
void FakeKraid_9C50(void) {  // 0xA69C50
  if (!gEnemyData(cur_enemy_index)->health) {
    EnemyDeathAnimation(cur_enemy_index, 3);
    Enemy_ItemDrop_MiniKraid(cur_enemy_index);
  }
}
void Etecoon_Init(void) {  // 0xA7E912
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  E->spritemap_pointer = addr_kSpritemap_Nothing_A7;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = addr_kEtecoon_Ilist_E8CE;
  E->ai_preinstr = 0xE9AF /* Etecoon_Func_4 */;
  E->ai_var_E = -1;
}
void Etecoon_Main(void) {  // 0xA7E940
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (HIBYTE(E->parameter_2))
    E->parameter_2 -= 256;
  else
    CallEnemyPreInstr(E->ai_preinstr | 0xA70000);
}
void Etecoon_Func_1(uint16 k) {  // 0xA7E958
  if (earthquake_timer) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->parameter_2 = LOBYTE(E->parameter_2) | 0x8000;
    E->instruction_timer += 128;
  }
}
uint8 Etecoon_Func_2(uint16 k) {  // 0xA7E974
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  return Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(E->ai_var_C, E->ai_var_D));
}
uint8 Etecoon_Func_3(uint16 k) {  // 0xA7E983
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int32 amt = __PAIR32__(E->ai_var_A, E->ai_var_B);
  if (sign16(E->ai_var_A - 5))
    AddToHiLo(&E->ai_var_A, &E->ai_var_B, __PAIR32__(samus_y_accel, samus_y_subaccel));
  return Enemy_MoveDown(k, amt);
}
void Etecoon_Func_4(uint16 k) {  // 0xA7E9AF
  if (!door_transition_flag_enemies) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if ((E->ai_var_E & 0x8000) == 0) {
      bool v2 = E->ai_var_E == 1;
      bool v3 = (--E->ai_var_E & 0x8000) != 0;
      if (v2 || v3) {
        E->current_instruction = addr_kEtecoon_Ilist_E854;
        E->ai_preinstr = 0xEA00 /* Etecoon_Func_5 */;
        E->ai_var_E = 11;
      }
    } else if (IsSamusWithinEnemy_Y(k, 0x80)) {
      if ((E->parameter_2 & 3) == 0)
        QueueSfx2_Max15(0x35);
      E->instruction_timer = 1;
      E->current_instruction = addr_kEtecoon_Ilist_E8D6;
      E->ai_var_E = 256;
    }
  }
}
void Etecoon_Func_5(uint16 k) {  // 0xA7EA00
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
    E->current_instruction += 2;
    E->instruction_timer = 1;
    E->ai_preinstr = 0xEA37 /* Etecoon_Func_6 */;
    if (!sign16(samus_x_pos - 256))
      QueueSfx2_Max6(0x33);
  }
}
void Etecoon_Func_6(uint16 k) {  // 0xA7EA37
  if (Etecoon_Func_3(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if ((E->ai_var_A & 0x8000) == 0) {
      if (IsSamusWithinEnemy_Y(k, 0x40) && IsSamusWithinEnemy_X(k, ((uint16)BW_A7(0xE910 + 2 * (0))))) {
        uint16 v2 = DetermineDirectionOfSamusFromEnemy();
        if (sign16(v2 - 5)) {
          E->current_instruction = addr_kEtecoon_Ilist_E81E;
          E->parameter_1 = 0;
        } else {
          E->current_instruction = addr_kEtecoon_Ilist_E876;
          E->parameter_1 = 1;
        }
        E->ai_var_E = 32;
        E->instruction_timer = 1;
        E->ai_preinstr = 0xEAB5 /* Etecoon_Func_7 */;
      } else {
        E->ai_var_E = 11;
        E->ai_preinstr = 0xEA00 /* Etecoon_Func_5 */;
        E->instruction_timer = 1;
        E->current_instruction = addr_kEtecoon_Ilist_E854;
      }
    } else {
      E->ai_var_A = 0;
      E->ai_var_B = 0;
      E->instruction_timer = 3;
      E->current_instruction = addr_stru_A7E862;
    }
  }
}
void Etecoon_Func_7(uint16 k) {  // 0xA7EAB5
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    E->current_instruction += 2;
    E->instruction_timer = 1;
    if (E->parameter_1) {
      E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
      E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
      E->ai_preinstr = 0xEB2C /* Etecoon_Func_9 */;
    } else {
      E->ai_var_C = ((uint16)BW_A7(0xE90C + 2 * (0)));
      E->ai_var_D = ((uint16)BW_A7(0xE90E + 2 * (0)));
      E->ai_preinstr = 0xEB02 /* Etecoon_Func_8 */;
    }
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
  }
}
void Etecoon_Func_8(uint16 k) {  // 0xA7EB02
  if (Etecoon_Func_2(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
    E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
    E->ai_preinstr = 0xEB2C /* Etecoon_Func_9 */;
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E880;
    E->parameter_1 = 1;
  }
}
void Etecoon_Func_9(uint16 k) {  // 0xA7EB2C
  if (EnemyFunc_BBBF(k, __PAIR32__(32, 0))) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E898;
    E->ai_preinstr = 0xEB50 /* Etecoon_Func_10 */;
  } else {
    Etecoon_Func_2(k);
  }
}
void Etecoon_Func_10(uint16 k) {  // 0xA7EB50
  Etecoon_Func_1(k);
  if (Etecoon_Func_2(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if (E->parameter_1) {
      E->current_instruction = addr_kEtecoon_Ilist_E870;
      E->parameter_1 = 0;
    } else {
      E->current_instruction = addr_kEtecoon_Ilist_E8C8;
      E->parameter_1 = 1;
    }
    E->instruction_timer = 1;
    E->ai_preinstr = 0xEBCD /* Etecoon_Func_11 */;
    E->ai_var_E = 8;
    if (!sign16(samus_x_pos - 256))
      QueueSfx2_Max6(0x32);
  } else if (Etecoon_Func_3(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if (E->parameter_1)
      E->current_instruction = addr_kEtecoon_Ilist_E854;
    else
      E->current_instruction = addr_kEtecoon_Ilist_E8AC;
    E->instruction_timer = 1;
    E->ai_var_E = 11;
    E->ai_preinstr = 0xEC1B /* Etecoon_Func_12 */;
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
  }
}
void Etecoon_Func_11(uint16 k) {  // 0xA7EBCD
  Etecoon_Func_1(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    if (E->parameter_1) {
      E->current_instruction = addr_kEtecoon_Ilist_E894;
      E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
      E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
    } else {
      E->current_instruction = addr_kEtecoon_Ilist_E83C;
      E->ai_var_C = ((uint16)BW_A7(0xE90C + 2 * (0)));
      E->ai_var_D = ((uint16)BW_A7(0xE90E + 2 * (0)));
    }
    E->instruction_timer = 1;
    E->ai_preinstr = 0xEB50 /* Etecoon_Func_10 */;
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
  }
}
void Etecoon_Func_12(uint16 k) {  // 0xA7EC1B
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
    funcs_A2A79[LOBYTE(E->parameter_2)](k);
    gEnemyData(k)->instruction_timer = 1;
  }
}
void Etecoon_Func_13(uint16 j) {  // 0xA7EC47
  EnemyData *E = gEnemyData(j); const uint16 E_k = (j);
  E->ai_preinstr = 0xEC97 /* Etecoon_Func_16 */;
  E->current_instruction = addr_stru_A7E828;
  E->ai_var_C = ((uint16)BW_A7(0xE90C + 2 * (0)));
  E->ai_var_D = ((uint16)BW_A7(0xE90E + 2 * (0)));
}
void Etecoon_Func_14(uint16 j) {  // 0xA7EC61
  EnemyData *E = gEnemyData(j); const uint16 E_k = (j);
  E->ai_preinstr = 0xECBB /* Etecoon_Func_17 */;
  E->current_instruction = addr_stru_A7E880;
  E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
  E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
}
void Etecoon_Func_15(uint16 j) {  // 0xA7EC7B
  EnemyData *E = gEnemyData(j); const uint16 E_k = (j);
  E->ai_preinstr = 0xED75 /* Etecoon_Func_22 */;
  E->current_instruction += 2;
  E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
  E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
}
void Etecoon_Func_16(uint16 k) {  // 0xA7EC97
  Etecoon_Func_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (sign16(E->x_pos - 537)) {
    E->ai_var_E = 11;
    E->ai_preinstr = 0xEDC7 /* Etecoon_Func_23 */;
    E->instruction_timer = 1;
    E->current_instruction = addr_kEtecoon_Ilist_E854;
  }
}
void Etecoon_Func_17(uint16 k) {  // 0xA7ECBB
  Etecoon_Func_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (!sign16(E->x_pos - 600)) {
    E->ai_var_E = 11;
    E->ai_preinstr = 0xEDC7 /* Etecoon_Func_23 */;
    E->instruction_timer = 1;
    E->current_instruction = addr_kEtecoon_Ilist_E854;
  }
}
void Etecoon_Func_18(uint16 k) {  // 0xA7ECDF
  Etecoon_Func_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (!sign16(E->x_pos - 600)) {
    E->ai_preinstr = 0xED09 /* Etecoon_Func_19 */;
    E->ai_var_A = ((uint16)BW_A7(0xE904 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE906 + 2 * (0)));
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E898;
  }
}
void Etecoon_Func_19(uint16 k) {  // 0xA7ED09
  Etecoon_Func_2(k);
  Etecoon_Func_3(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (!sign16(E->x_pos - 680)) {
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E880;
    E->ai_preinstr = 0xED2A /* Etecoon_Func_20 */;
  }
}
void Etecoon_Func_20(uint16 k) {  // 0xA7ED2A
  Etecoon_Func_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (!sign16(E->x_pos - 840)) {
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E898;
    E->ai_preinstr = 0xED54 /* Etecoon_Func_21 */;
    E->ai_var_A = -1;
    E->ai_var_B = ((uint16)BW_A7(0xE906 + 2 * (0)));
  }
}
void Etecoon_Func_21(uint16 k) {  // 0xA7ED54
  Etecoon_Func_2(k);
  if (Etecoon_Func_3(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->ai_var_E = 11;
    E->instruction_timer = 1;
    E->current_instruction = addr_kEtecoon_Ilist_E854;
    E->ai_preinstr = 0xEDC7 /* Etecoon_Func_23 */;
  }
}
void Etecoon_Func_22(uint16 k) {  // 0xA7ED75
  Etecoon_Func_1(k);
  if (Etecoon_Func_3(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if ((E->ai_var_A & 0x8000) == 0) {
      E->ai_var_E = 11;
      E->instruction_timer = 1;
      E->current_instruction = addr_kEtecoon_Ilist_E854;
      if ((E->parameter_2 & 2) != 0 && (sign16(E->x_pos - 832)))
        E->ai_preinstr = 0xEE3E /* Etecoon_Func_24 */;
      else
        E->ai_preinstr = 0xEDC7 /* Etecoon_Func_23 */;
    } else {
      E->ai_var_A = 0;
      E->ai_var_B = 0;
      E->instruction_timer = 3;
      E->current_instruction = addr_stru_A7E862;
    }
  }
}
void Etecoon_Func_23(uint16 k) {  // 0xA7EDC7
  Etecoon_Func_1(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    uint16 v4 = E->parameter_1 + 256;
    E->parameter_1 = v4;
    if (sign16((v4 & 0xFF00) - 1024)
        || (E->parameter_1 = LOBYTE(E->parameter_1), (E->parameter_2 & 2) != 0)) {
      E->ai_preinstr = 0xED75 /* Etecoon_Func_22 */;
      E->current_instruction += 2;
    } else {
      E->ai_preinstr = 0xEE9A /* Etecoon_Func_25 */;
      E->current_instruction = addr_stru_A7E880;
      E->ai_var_C = ((uint16)BW_A7(0xE908 + 2 * (0)));
      E->ai_var_D = ((uint16)BW_A7(0xE90A + 2 * (0)));
    }
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
    E->instruction_timer = 1;
    if (!sign16(samus_x_pos - 256))
      QueueSfx2_Max6(0x33);
  }
}
void Etecoon_Func_24(uint16 k) {  // 0xA7EE3E
  Etecoon_Func_1(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    if (IsSamusWithinEnemy_Y(k, 0x40) && IsSamusWithinEnemy_X(k, 0x30)) {
      E->current_instruction = addr_stru_A7E880;
      E->ai_preinstr = 0xECDF /* Etecoon_Func_18 */;
    } else {
      E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
      E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
      E->current_instruction += 2;
      E->ai_preinstr = 0xED75 /* Etecoon_Func_22 */;
      if (!sign16(samus_x_pos - 256))
        QueueSfx2_Max6(0x33);
    }
    E->instruction_timer = 1;
  }
}
void Etecoon_Func_25(uint16 k) {  // 0xA7EE9A
  Etecoon_Func_2(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (!sign16(E->x_pos - 600)) {
    E->ai_preinstr = 0xEEB8 /* Etecoon_Func_26 */;
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E898;
  }
}
void Etecoon_Func_26(uint16 k) {  // 0xA7EEB8
  Etecoon_Func_2(k);
  if (Etecoon_Func_3(k) & 1) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->ai_var_C = ((uint16)BW_A7(0xE90C + 2 * (0)));
    E->ai_var_D = ((uint16)BW_A7(0xE90E + 2 * (0)));
    E->ai_preinstr = 0xEB02 /* Etecoon_Func_8 */;
    E->ai_var_A = ((uint16)BW_A7(0xE900 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xE902 + 2 * (0)));
    E->instruction_timer = 1;
    E->current_instruction = addr_stru_A7E828;
  }
}
void Dachora_Init(void) {  // 0xA7F4DD
  int16 dachor_parameter_1;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  E->spritemap_pointer = addr_kSpritemap_Nothing_A7;
  E->instruction_timer = 1;
  E->timer = 0;
  dachor_parameter_1 = E->parameter_1;
  if (dachor_parameter_1 < 0) {
    if ((dachor_parameter_1 & 1) != 0)
      E->current_instruction = addr_kDachora_Ilist_F4B9;
    else
      E->current_instruction = addr_kDachora_Ilist_F3F7;
    E->ai_preinstr = 0xF98C /* Dachora_Func_12 */;
  } else {
    if (dachor_parameter_1)
      E->current_instruction = addr_kDachora_Ilist_F45B;
    else
      E->current_instruction = addr_kDachora_Ilist_F399;
    E->ai_preinstr = 0xF570 /* Dachora_Func_2 */;
  }
}
void Dachora_Main(void) {  // 0xA7F52E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xA70000);
}
void Dachora_Func_1(uint16 j, uint16 k) {  // 0xA7F535
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v3 = swap16(E->palette_index);
  nt_ram_copy_rom((uint16)(((uint16)BW_A7(0xF55F + 2 * (v3 >> 1)))), BW_A7, (uint16)(j), 32);
}
void Dachora_Func_2(uint16 k) {  // 0xA7F570
  Enemy_MoveDown(k, __PAIR32__(1, 0));
  if (IsSamusWithinEnemy_Y(k, 0x40) && IsSamusWithinEnemy_X(k, ((uint16)BW_A7(0xF4C9 + 2 * (0))))) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    if (E->parameter_1)
      E->current_instruction = addr_kDachora_Ilist_F48B;
    else
      E->current_instruction = addr_kDachora_Ilist_F3C9;
    E->instruction_timer = 1;
    E->ai_preinstr = 0xF5BC /* Dachora_Func_3 */;
    E->ai_var_A = ((uint16)BW_A7(0xF4CD + 2 * (0)));
    QueueSfx2_Max15(0x1D);
  }
}
void Dachora_Func_3(uint16 k) {  // 0xA7F5BC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_A-- == 1) {
    if (E->parameter_1) {
      E->current_instruction = addr_kDachora_Ilist_F407;
      E->ai_preinstr = 0xF65E /* Dachora_Func_5 */;
    } else {
      E->current_instruction = addr_kDachora_Ilist_F345;
      E->ai_preinstr = 0xF5ED /* Dachora_Func_4 */;
    }
    E->instruction_timer = 1;
    E->ai_var_E = 1;
  }
}
void Dachora_Func_4(uint16 k) {  // 0xA7F5ED
  int32 amt = -Dachora_Func_6(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, amt) || (EnemyFunc_C8AD(k), sign16(E->x_pos - 96))) {
    E->current_instruction = addr_kDachora_Ilist_F407;
    E->ai_preinstr = 0xF65E /* Dachora_Func_5 */;
    E->ai_var_E = 1;
    E->parameter_1 = 1;
    E->instruction_timer = 1;
    E->ai_var_A = 0;
    E->ai_var_B = 0;
    Dachora_Func_1(addr_kDachora_Palette, k);
  }
}
void Dachora_Func_5(uint16 k) {  // 0xA7F65E
  int32 amt = Dachora_Func_6(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, amt)) {
    QueueSfx2_Max15(0x71);
    E->current_instruction = addr_kDachora_Ilist_F345;
    E->ai_preinstr = 0xF5ED /* Dachora_Func_4 */;
    E->parameter_1 = 0;
    E->ai_var_A = 0;
    Dachora_Func_1(addr_kDachora_Palette, k);
LABEL_4:
    E->ai_var_E = 0;
    E->ai_var_B = 0;
    E->instruction_timer = 1;
    return;
  }
  EnemyFunc_C8AD(k);
  if (!sign16(E->x_pos - 1152)) {
    E->current_instruction = addr_kDachora_Ilist_F4B3;
    E->ai_preinstr = 0xF78F /* Dachora_Func_7 */;
    E->ai_var_A = ((uint16)BW_A7(0xF4CF + 2 * (0)));
    E->y_pos += 8;
    QueueSfx2_Max6(0x3D);
    goto LABEL_4;
  }
}
int32 Dachora_Func_6(uint16 k) {  // 0xA7F6D5
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((int16)(E->ai_var_A - ((uint16)BW_A7(0xF4D5 + 2 * (0)))) >= 0) {
    if (E->ai_var_E == 1)
      QueueSfx2_Max6(0x39);
    uint16 v2 = E->ai_var_E - 1;
    E->ai_var_E = v2;
    if (!(uint8)v2) {
      Dachora_Func_1(((uint16)BW_A7(0xF787 + 2 * (HIBYTE(E->ai_var_E)))), k);
      uint16 v3 = E->ai_var_E + 272;
      E->ai_var_E = v3;
      if (!sign16(v3 - 1040))
        E->ai_var_E = 784;
    }
  }
  Enemy_MoveDown(k, INT16_SHL16(1));
  if ((int16)(E->ai_var_A - ((uint16)BW_A7(0xF4D5 + 2 * (0)))) >= 0 && (int16)(E->ai_var_B - ((uint16)BW_A7(0xF4D7 + 2 * (0)))) >= 0) {
    E->ai_var_A = ((uint16)BW_A7(0xF4D5 + 2 * (0)));
    E->ai_var_B = ((uint16)BW_A7(0xF4D7 + 2 * (0)));
    return __PAIR32__(((uint16)BW_A7(0xF4D5 + 2 * (0))), ((uint16)BW_A7(0xF4D7 + 2 * (0))));
  }
  AddToHiLo(&E->ai_var_A, &E->ai_var_B, __PAIR32__(((uint16)BW_A7(0xF4D9 + 2 * (0))), ((uint16)BW_A7(0xF4DB + 2 * (0)))));
  if ((E->ai_var_A == 4 || E->ai_var_A == 8) && !E->ai_var_B)
    E->current_instruction += 28;
  return __PAIR32__(E->ai_var_A, E->ai_var_B);
}
void Dachora_Func_7(uint16 k) {  // 0xA7F78F
  Dachora_Func_10(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_A-- == 1) {
    E->current_instruction += 2;
    E->instruction_timer = 1;
    E->ai_preinstr = 0xF806 /* Dachora_Func_8 */;
    EnemyData *E1 = gEnemyData(k + 64); const uint16 E1_k = (k + 64);
    E1->ai_var_A = 0;
    E1->ai_var_D = 0;
    gEnemyData(k + 128)->ai_var_D = 0;
    gEnemyData(k + 192)->ai_var_D = 0;
    gEnemyData(k + 256)->ai_var_D = 0;
    E->ai_var_C = 0;
    E->ai_var_D = 0;
    E->y_pos -= 8;
    QueueSfx2_Max6(0x3B);
    if (E->parameter_1) {
      gEnemyData(k + 64)->current_instruction = addr_kDachora_Ilist_F4B9;
      gEnemyData(k + 128)->current_instruction = addr_kDachora_Ilist_F4B9;
      gEnemyData(k + 192)->current_instruction = addr_kDachora_Ilist_F4B9;
      gEnemyData(k + 256)->current_instruction = addr_kDachora_Ilist_F4B9;
    } else {
      gEnemyData(k + 64)->current_instruction = addr_kDachora_Ilist_F3F7;
      gEnemyData(k + 128)->current_instruction = addr_kDachora_Ilist_F3F7;
      gEnemyData(k + 192)->current_instruction = addr_kDachora_Ilist_F3F7;
      gEnemyData(k + 256)->current_instruction = addr_kDachora_Ilist_F3F7;
    }
    gEnemyData(k + 64)->instruction_timer = 1;
    gEnemyData(k + 128)->instruction_timer = 1;
    gEnemyData(k + 192)->instruction_timer = 1;
    gEnemyData(k + 256)->instruction_timer = 1;
  }
}
void Dachora_Func_8(uint16 k) {  // 0xA7F806
  Dachora_Func_10(k);
  Dachora_Func_9(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->ai_var_C, &E->ai_var_D, __PAIR32__(samus_y_accel, samus_y_subaccel));
  AddToHiLo(&E->ai_var_A, &E->ai_var_B, __PAIR32__(E->ai_var_C, E->ai_var_D));
  uint16 v6 = E->ai_var_A;
  if (!sign16(v6 - 15))
    v6 = 15;
  int32 amt = __PAIR32__(v6, E->ai_var_B);
  if (Enemy_MoveDown(k, -amt)) {
    if (E->parameter_1) {
      E->current_instruction = addr_kDachora_Ilist_F3FF;
      E->parameter_1 = 0;
    } else {
      E->current_instruction = addr_kDachora_Ilist_F4C1;
      E->parameter_1 = 1;
    }
    E->ai_preinstr = 0xF935 /* Dachora_Func_11 */;
    E->instruction_timer = 1;
    E->ai_var_A = 0;
    E->ai_var_B = 0;
    E->ai_var_E = 0;
    Dachora_Func_1(addr_kDachora_Palette, k);
    QueueSfx2_Max6(0x3C);
  }
}
void Dachora_Func_9(uint16 k) {  // 0xA7F89A

  EnemyData *EK = gEnemyData(k); const uint16 EK_k = (k);
  EnemyData *E1 = gEnemyData(k + 64); const uint16 E1_k = (k + 64);
  uint16 dachor_var_A = E1->ai_var_A;
  if (dachor_var_A) {
    E1->ai_var_A = dachor_var_A - 1;
  } else {
    E1->ai_var_A = ((uint16)BW_A7(0xF4D1 + 2 * (0)));
    if (E1->ai_var_D) {
      EnemyData *E2 = gEnemyData(k + 128); const uint16 E2_k = (k + 128);
      if (E2->ai_var_D) {
        EnemyData *E3 = gEnemyData(k + 192); const uint16 E3_k = (k + 192);
        if (E3->ai_var_D) {
          EnemyData *E4 = gEnemyData(k + 256); const uint16 E4_k = (k + 256);
          if (!E4->ai_var_D) {
            E4->x_pos = EK->x_pos;
            E4->y_pos = EK->y_pos;
            E4->ai_var_D = ((uint16)BW_A7(0xF4D3 + 2 * (0)));
          }
        } else {
          E3->x_pos = EK->x_pos;
          E3->y_pos = EK->y_pos;
          E3->ai_var_D = ((uint16)BW_A7(0xF4D3 + 2 * (0)));
        }
      } else {
        E2->x_pos = EK->x_pos;
        E2->y_pos = EK->y_pos;
        E2->ai_var_D = ((uint16)BW_A7(0xF4D3 + 2 * (0)));
      }
    } else {
      E1->x_pos = EK->x_pos;
      E1->y_pos = EK->y_pos;
      E1->ai_var_D = ((uint16)BW_A7(0xF4D3 + 2 * (0)));
    }
  }
}
void Dachora_Func_10(uint16 k) {  // 0xA7F90A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  Dachora_Func_1(((uint16)BW_A7(0xF92D + 2 * (HIBYTE(E->ai_var_E)))), k);
  uint16 v2 = E->ai_var_E + 256;
  E->ai_var_E = v2;
  if (!sign16(v2 - 1024))
    E->ai_var_E = 0;
}
void Dachora_Func_11(uint16 k) {  // 0xA7F935
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->ai_var_A, &E->ai_var_B, __PAIR32__(samus_y_accel, samus_y_subaccel));
  int32 amt = __PAIR32__(E->ai_var_A, E->ai_var_B);
  if (!sign16((amt >> 16) - 10))
    amt = INT16_SHL16(10);
  if (Enemy_MoveDown(k, amt)) {
    if (E->parameter_1) {
      E->current_instruction = addr_kDachora_Ilist_F407;
      E->ai_preinstr = 0xF65E /* Dachora_Func_5 */;
    } else {
      E->current_instruction = addr_kDachora_Ilist_F345;
      E->ai_preinstr = 0xF5ED /* Dachora_Func_4 */;
    }
    E->instruction_timer = 1;
    E->ai_var_A = 0;
    E->ai_var_B = 0;
  }
}
void Dachora_Func_12(uint16 k) {  // 0xA7F98C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 dachor_var_D = E->ai_var_D;
  if (!dachor_var_D)
    goto LABEL_6;
  E->ai_var_D = dachor_var_D - 1;
  if ((k & 0x40) != 0) {
    if ((nmi_frame_counter_word & 1) == 0)
      goto LABEL_6;
  } else if ((nmi_frame_counter_word & 1) != 0) {
LABEL_6:
    E->properties |= kEnemyProps_Invisible;
    return;
  }
  E->properties &= ~kEnemyProps_Invisible;
}
void Enemy_NormalTouchAI_A8(void) {  // 0xA88023
  NormalEnemyTouchAi();
}
void Enemy_NormalShotAI_A8(void) {  // 0xA8802D
  NormalEnemyShotAi();
}
void Enemy_NormalFrozenAI_A8(void) {  // 0xA88041
  NormalEnemyFrozenAI();
}
void MorphBallEye_Init(void) {  // 0xA89058
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  E->spritemap_pointer = addr_kSpritemap_Nothing_A8;
  E->instruction_timer = 1;
  E->timer = 0;
  if ((E->parameter_2 & 0x8000) != 0) {
    int v1 = E->parameter_2 & 0xF;
    E->x_pos += ((int16)BW_A8(0x90CA + 2 * (v1)));
    E->y_pos += ((int16)BW_A8(0x90D2 + 2 * (v1)));
    E->ai_preinstr = 0x91DC /* nullsub_244 */;
    E->current_instruction = ((uint16)BW_A8(0x90DA + 2 * (v1)));
    for (int i = 510; i >= 0; i -= 2)
      ERAM(0x9100 + (uint16)((uint16)i)) = 255;
  } else {
    E->ai_preinstr = 0x90F1 /* MorphBallEye_Func_1 */;
    if ((E->parameter_1 & 1) != 0)
      E->current_instruction = addr_stru_A8900E;
    else
      E->current_instruction = addr_stru_A88FFC;
  }
}
void MorphBallEye_Main(void) {  // 0xA890E2
  if ((collected_items & 4) != 0) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    CallEnemyPreInstr(E->ai_preinstr | 0xA80000);
  }
}
void MorphBallEye_Func_1(uint16 k) {  // 0xA890F1
  if (IsSamusWithinEnemy_Y(k, ((uint16)BW_A8(0x9054 + 2 * (0)))) && IsSamusWithinEnemy_X(k, ((uint16)BW_A8(0x9050 + 2 * (0))))) {
    EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
    E->ai_var_E = 32;
    E->instruction_timer = 1;
    if ((E->parameter_1 & 1) != 0)
      E->current_instruction = addr_kMorphBallEye_Ilist_9026;
    else
      E->current_instruction = addr_kMorphBallEye_Ilist_9014;
    E->ai_preinstr = 0x912E /* MorphBallEye_Func_2 */;
  }
}
void MorphBallEye_Func_2(uint16 k) {  // 0xA8912E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3) {
    QueueSfx2_Max6(0x17);
    SpawnMorphBallEyeBeamHdma();
    E->ai_preinstr = 0x9160 /* MorphBallEye_Func_3 */;
    E->ai_var_D = CalculateAngleFromXY(samus_x_pos - E->x_pos, samus_y_pos - E->y_pos);
  }
}
void MorphBallEye_Func_3(uint16 k) {  // 0xA89160
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (IsSamusWithinEnemy_Y(k, ((uint16)BW_A8(0x9056 + 2 * (0)))) && IsSamusWithinEnemy_X(k, ((uint16)BW_A8(0x9052 + 2 * (0))))) {
    uint16 v3 = CalculateAngleFromXY(samus_x_pos - E->x_pos, samus_y_pos - E->y_pos);
    E->ai_var_D = v3;
    E->current_instruction = ((v3 & 0xF0) >> 2) - 28756;
  } else {
    QueueSfx2_Max6(0x71);
    E->ai_var_C = 0;
    E->ai_var_E = 32;
    if ((E->parameter_1 & 1) != 0)
      E->current_instruction = addr_kMorphBallEye_Ilist_9002;
    else
      E->current_instruction = addr_kMorphBallEye_Ilist_8FF0;
    E->ai_preinstr = 0x91CE /* MorphBallEye_Func_4 */;
  }
  E->instruction_timer = 1;
}
void MorphBallEye_Func_4(uint16 k) {  // 0xA891CE
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  bool v2 = E->ai_var_E == 1;
  bool v3 = (--E->ai_var_E & 0x8000) != 0;
  if (v2 || v3)
    E->ai_preinstr = 0x90F1 /* MorphBallEye_Func_1 */;
}
uint16 YappingMaw_Instr_2(uint16 k, uint16 jp) {  // 0xA8A0C7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0AB + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0AD + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_4(uint16 k, uint16 jp) {  // 0xA8A0D9
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0C3 + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0C5 + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_5(uint16 k, uint16 jp) {  // 0xA8A0EB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0B3 + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0B5 + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_7(uint16 k, uint16 jp) {  // 0xA8A0FD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0BB + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0BD + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_3(uint16 k, uint16 jp) {  // 0xA8A10F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0A7 + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0A9 + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_6(uint16 k, uint16 jp) {  // 0xA8A121
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0B7 + 2 * (0)));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0B9 + 2 * (0)));
  return jp;
}
uint16 YappingMaw_Instr_1(uint16 k, uint16 jp) {  // 0xA8A133
  if (!ERAM(0x802C + (cur_enemy_index)))
    QueueSfx2_Max6(0x2F);
  return jp;
}
void YappingMaw_Init(void) {  // 0xA8A148
  int16 v1;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7818 + E_k) = E->x_pos;
  ERAM(0x781A + E_k) = E->y_pos;
  ERAM(0x7800 + E_k) = 0;
  ERAM(0x7802 + E_k) = 0;
  ERAM(0x7804 + E_k) = 0;
  ERAM(0x7806 + E_k) = 0;
  ERAM(0x7808 + E_k) = 0;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x780C + E_k) = 0;
  ERAM(0x780E + E_k) = 0;
  ERAM(0x8020 + E_k) = 0;
  E->ai_preinstr = E->parameter_1;
  E->ai_var_E = 64;
  E->current_instruction = addr_kYappingMaw_Ilist_9F6F;
  uint16 R36 = 57;
  uint16 R34 = 8;
  if (!E->parameter_2) {
    E->current_instruction = addr_kYappingMaw_Ilist_9FC7;
    R36 = 56;
    R34 = -8;
  }
  E->instruction_timer = 1;
  E->timer = 0;
  E->ai_var_A = 0xA235 /* YappingMaw_Func_1 */;
  ERAM(0x880A + E_k) = E->palette_index & 0xE00;
  v1 = 3;
  ERAM(0x8808 + E_k) = 3;
  do {
    SpawnEprojWithGfx(v1, cur_enemy_index, addr_kEproj_YappingMawsBody);
    v1 = ERAM(0x8808 + E_k) - 1;
    ERAM(0x8808 + E_k) = v1;
  } while (v1 >= 0);
  uint16 v3 = E->vram_tiles_index | E->palette_index;
  ERAM(0x880E + E_k) = v3;
  ERAM(0x880C + E_k) = CreateSpriteAtPos(E->x_pos, R34 + E->y_pos, R36, v3);
}
void CallYappingMawFunc(uint32 ea) {
  switch (ea) {
  case 0xA8A235: YappingMaw_Func_1(); return;  // 0xa8a235
  case 0xA8A28C: YappingMaw_Func_2(); return;  // 0xa8a28c
  case 0xA8A445: YappingMaw_Func_8(); return;  // 0xa8a445
  case 0xA8A68A: YappingMaw_Func_11(); return;  // 0xa8a68a
  default: nt_unimpl("Unreachable");
  }
}
void YappingMaw_Main(void) {  // 0xA8A211
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  --ERAM(0x802A + E_k);
  ERAM(0x802C + E_k) = CheckIfEnemyIsOnScreen();
  CallYappingMawFunc(E->ai_var_A | 0xA80000);
  YappingMaw_Func_15();
  YappingMaw_Func_14();
  YappingMaw_Func_13();
  YappingMaw_Func_12();
}
void YappingMaw_Func_1(void) {  // 0xA8A235
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  PairU16 pair = EnemyFunc_ACA8((Point16U) { E->x_pos, E->y_pos}, (Point16U) { samus_x_pos, samus_y_pos });
  uint16 v2 = Abs16(pair.k);
  ERAM(0x7810 + E_k) = v2;
  if (sign16(v2 - 32)) {
    ERAM(0x802A + E_k) = 48;
  } else if ((int16)(v2 - E->ai_preinstr) < 0) {
    if (!sign16(ERAM(0x7810 + E_k) - 64))
      ERAM(0x7810 + E_k) = 64;
    ERAM(0x7814 + E_k) = pair.j;
    E->ai_var_A = 0xA28C /* YappingMaw_Func_2 */;
  }
}
void YappingMaw_Func_2(void) {  // 0xA8A28C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = 0;
  E->ai_var_C = 0;
  E->ai_var_D = 0;
  ERAM(0x7812 + E_k) = ERAM(0x7810 + E_k) >> 1;
  uint16 v1 = (uint8)(64 - ERAM(0x7814 + E_k));
  ERAM(0x7816 + E_k) = v1;
  if (sign16(v1 - 128))
    ERAM(0x801E + E_k) = 0;
  else
    ERAM(0x801E + E_k) = 1;
  YappingMaw_Func_3();
  uint16 v2 = 2 * ((uint8)(ERAM(0x7814 + E_k) + 16) >> 5);
  ERAM(0x8028 + E_k) = v2;
  E->current_instruction = ((uint16)BW_A8(0xA097 + 2 * (v2 >> 1)));
  E->instruction_timer = 1;
  E->timer = 0;
  ERAM(0x8024 + E_k) = ((uint16)BW_A8(0xA0A7 + ((uint16)(v2))));
  ERAM(0x8026 + E_k) = ((uint16)BW_A8(0xA0A9 + ((uint16)(v2))));
  E->ai_var_A = 0xA445 /* YappingMaw_Func_8 */;
}
void YappingMaw_Func_3(void) {  // 0xA8A310
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x781C + E_k) = YappingMaw_Func_16(0x80, ERAM(0x7812 + E_k));
  ERAM(0x781E + E_k) = YappingMaw_Func_17(0x80, ERAM(0x7812 + E_k) >> 1);
}
void YappingMaw_Func_4(void) {  // 0xA8A339
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 varE32 = ERAM(0x7800 + E_k);
  ERAM(0x7820 + E_k) = YappingMaw_Func_16(0, varE32);
  ERAM(0x7822 + E_k) = YappingMaw_Func_17(0, varE32);
  ERAM(0x800E + E_k) = YappingMaw_Func_16(ERAM(0x7816 + E_k), varE32) - ERAM(0x7820 + E_k);
  ERAM(0x8010 + E_k) = YappingMaw_Func_17(ERAM(0x7816 + E_k), varE32) - ERAM(0x7822 + E_k);
}
void YappingMaw_Func_5(void) {  // 0xA8A37C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 varE32 = ERAM(0x7804 + E_k);
  ERAM(0x7820 + E_k) = YappingMaw_Func_16(0, varE32);
  ERAM(0x7822 + E_k) = YappingMaw_Func_17(0, varE32);
  ERAM(0x8012 + E_k) = YappingMaw_Func_16(ERAM(0x7816 + E_k), varE32) - ERAM(0x7820 + E_k);
  ERAM(0x8014 + E_k) = YappingMaw_Func_17(ERAM(0x7816 + E_k), varE32) - ERAM(0x7822 + E_k);
}
void YappingMaw_Func_6(void) {  // 0xA8A3BF
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 varE32 = ERAM(0x7808 + E_k);
  ERAM(0x7820 + E_k) = YappingMaw_Func_16(0, varE32);
  ERAM(0x7822 + E_k) = YappingMaw_Func_17(0, varE32);
  ERAM(0x8016 + E_k) = YappingMaw_Func_16(ERAM(0x7816 + E_k), varE32) - ERAM(0x7820 + E_k);
  ERAM(0x8018 + E_k) = YappingMaw_Func_17(ERAM(0x7816 + E_k), varE32) - ERAM(0x7822 + E_k);
}
void YappingMaw_Func_7(void) {  // 0xA8A402
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 varE32 = ERAM(0x780C + E_k);
  ERAM(0x7820 + E_k) = YappingMaw_Func_16(0, varE32);
  ERAM(0x7822 + E_k) = YappingMaw_Func_17(0, varE32);
  ERAM(0x801A + E_k) = YappingMaw_Func_16(ERAM(0x7816 + E_k), varE32) - ERAM(0x7820 + E_k);
  ERAM(0x801C + E_k) = YappingMaw_Func_17(ERAM(0x7816 + E_k), varE32) - ERAM(0x7822 + E_k);
}
void YappingMaw_Func_8(void) {  // 0xA8A445
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 r20 = E->ai_var_B >> 2;
  if (ERAM(0x801E + E_k)) {
    ERAM(0x8002 + E_k) = 128 + r20;
    ERAM(0x8004 + E_k) = 128 + r20 * 2;
    ERAM(0x8006 + E_k) = 128 + r20 * 3;
    ERAM(0x8008 + E_k) = 128 + r20 * 4;
  } else {
    ERAM(0x8002 + E_k) = 128 - r20;
    ERAM(0x8004 + E_k) = 128 - r20 * 2;
    ERAM(0x8006 + E_k) = 128 - r20 * 3;
    ERAM(0x8008 + E_k) = 128 - r20 * 4;
  }
  uint16 varE32 = ERAM(0x7812 + E_k);
  ERAM(0x7800 + E_k) = YappingMaw_Func_16(ERAM(0x8002 + E_k), varE32) - ERAM(0x781C + E_k);
  ERAM(0x7804 + E_k) = YappingMaw_Func_16(ERAM(0x8004 + E_k), varE32) - ERAM(0x781C + E_k);
  ERAM(0x7808 + E_k) = YappingMaw_Func_16(ERAM(0x8006 + E_k), varE32) - ERAM(0x781C + E_k);
  ERAM(0x780C + E_k) = YappingMaw_Func_16(ERAM(0x8008 + E_k), varE32) - ERAM(0x781C + E_k);
  varE32 = ERAM(0x7812 + E_k) >> 1;
  ERAM(0x7802 + E_k) = YappingMaw_Func_17(ERAM(0x8002 + E_k), varE32) - ERAM(0x781E + E_k);
  ERAM(0x7806 + E_k) = YappingMaw_Func_17(ERAM(0x8004 + E_k), varE32) - ERAM(0x781E + E_k);
  ERAM(0x780A + E_k) = YappingMaw_Func_17(ERAM(0x8006 + E_k), varE32) - ERAM(0x781E + E_k);
  ERAM(0x780E + E_k) = YappingMaw_Func_17(ERAM(0x8008 + E_k), varE32) - ERAM(0x781E + E_k);
  YappingMaw_Func_4();
  YappingMaw_Func_5();
  YappingMaw_Func_6();
  YappingMaw_Func_7();
  ERAM(0x7800 + E_k) += ERAM(0x800E + E_k);
  ERAM(0x7802 + E_k) += ERAM(0x8010 + E_k);
  ERAM(0x7804 + E_k) += ERAM(0x8012 + E_k);
  ERAM(0x7806 + E_k) += ERAM(0x8014 + E_k);
  ERAM(0x7808 + E_k) += ERAM(0x8016 + E_k);
  ERAM(0x780A + E_k) += ERAM(0x8018 + E_k);
  ERAM(0x780C + E_k) += ERAM(0x801A + E_k);
  ERAM(0x780E + E_k) += ERAM(0x801C + E_k);
  E->x_pos = ERAM(0x780C + E_k) + ERAM(0x7818 + E_k);
  E->y_pos = ERAM(0x780E + E_k) + ERAM(0x781A + E_k);
  YappingMaw_Func_9(cur_enemy_index);
  if (!sign16(E->ai_var_B)) {
    if (!sign16(E->ai_var_B - 128)) {
      E->ai_var_B = 128;
      E->ai_var_C = 0;
      E->ai_var_D += 4;
    }
LABEL_19:
    if (ERAM(0x8020 + E_k))
      YappingMaw_Func_10();
    return;
  }
  E->ai_var_A = 0xA68A /* YappingMaw_Func_11 */;
  ERAM(0x802A + E_k) = 48;
  if (ERAM(0x8028 + E_k) == 4) {
    E->current_instruction = addr_kYappingMaw_Ilist_A01F;
  } else if (ERAM(0x8028 + E_k) == 12) {
    E->current_instruction = addr_kYappingMaw_Ilist_A03D;
  } else {
    E->current_instruction = addr_off_A8A025;
  }
  if (!E->parameter_2) {
    if (ERAM(0x8028 + E_k) == 4) {
      E->current_instruction = addr_kYappingMaw_Ilist_A05B;
    } else {
      if (ERAM(0x8028 + E_k) != 12) {
        E->current_instruction = addr_off_A8A061;
        return;
      }
      E->current_instruction = addr_kYappingMaw_Ilist_A079;
    }
    goto LABEL_19;
  }
}
void YappingMaw_Func_9(uint16 k) {  // 0xA8A63E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = E->ai_var_D >> 1;
  AddToHiLo(&E->ai_var_B, &E->ai_var_C, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v2 + 1))), ((uint16)BW_A2(0x838F + 2 * (v2)))));
  E->ai_var_D += 8;
}
void YappingMaw_Func_10(void) {  // 0xA8A665
  CallSomeSamusCode(3);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  samus_x_pos = ERAM(0x8024 + E_k) + E->x_pos;
  samus_y_pos = ERAM(0x8026 + E_k) + E->y_pos;
  EnemyFunc_B7A1();
}
void YappingMaw_Func_11(void) {  // 0xA8A68A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x8020 + E_k))
    YappingMaw_Func_10();
  if ((--E->ai_var_E & 0x8000) != 0 && samus_input_handler != 0xE91D /* Samus_InputHandler_E91D */) {
    samus_input_handler = 0xE913 /* Samus_InputHandler_E913 */;
    ERAM(0x8020 + E_k) = 0;
    ERAM(0x802A + E_k) = 48;
    E->ai_var_E = 64;
    E->ai_var_A = 0xA235 /* YappingMaw_Func_1 */;
  }
}
void YappingMaw_Func_12(void) {  // 0xA8A6C4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x8800 + E_k) >> 1;
  eproj_x_pos[v1] = ERAM(0x7818 + E_k);
  eproj_y_pos[v1] = ERAM(0x781A + E_k);
}
void YappingMaw_Func_13(void) {  // 0xA8A6DB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x8802 + E_k) >> 1;
  eproj_x_pos[v1] = ERAM(0x7800 + E_k) + ERAM(0x7818 + E_k);
  eproj_y_pos[v1] = ERAM(0x7802 + E_k) + ERAM(0x781A + E_k);
}
void YappingMaw_Func_14(void) {  // 0xA8A6FC
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x8804 + E_k) >> 1;
  eproj_x_pos[v1] = ERAM(0x7804 + E_k) + ERAM(0x7818 + E_k);
  eproj_y_pos[v1] = ERAM(0x7806 + E_k) + ERAM(0x781A + E_k);
}
void YappingMaw_Func_15(void) {  // 0xA8A71D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v1 = ERAM(0x8806 + E_k) >> 1;
  eproj_x_pos[v1] = ERAM(0x7808 + E_k) + ERAM(0x7818 + E_k);
  eproj_y_pos[v1] = ERAM(0x780A + E_k) + ERAM(0x781A + E_k);
}
uint16 YappingMaw_Func_16(uint16 a, uint16 varE32) {  // 0xA8A73E
  return YappingMaw_Func_17(a - 64, varE32);
}
uint16 YappingMaw_Func_17(uint16 a, uint16 varE32) {  // 0xA8A742
  int16 v1;
  uint16 r20 = 0;

  uint16 r22 = 0;
  v1 = ((uint16)BW_A0(0xB1C3 + 2 * ((uint8)-a)));
  if (v1 < 0) {
    v1 = -v1;
    ++r20;
  }
  (void)0;
  if ((uint8)varE32) {
    uint16 prod = Mult8x8(HIBYTE(v1), varE32);
    r22 = 2 * ((prod & 0xFF00) >> 8);
    if (r20)
      r22 = (2 * ((uint16)-prod >> 8)) | 0xFF00;
  }
  return r22;
}
void YappingMaw_Touch(void) {  // 0xA8A799
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((ERAM(0x802A + E_k) & 0x8000) != 0 && !ERAM(0x8020 + E_k)) {
    ERAM(0x802A + E_k) = 0;
    ERAM(0x8020 + E_k) = 1;
    samus_input_handler = 0xE90E /* nullsub_152 */;
  }
}
void YappingMaw_Shot(void) {  // 0xA8A7BD
  NormalEnemyShotAi();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->health) {
    if (E->frozen_timer) {
      if (samus_input_handler != 0xE91D /* Samus_InputHandler_E91D */)
        samus_input_handler = 0xE913 /* Samus_InputHandler_E913 */;
      ERAM(0x8020 + E_k) = 0;
    }
  } else {
    eproj_id[(uint16)(ERAM(0x8800 + E_k)) >> 1] = 0;
    eproj_id[(uint16)(ERAM(0x8802 + E_k)) >> 1] = 0;
    eproj_id[(uint16)(ERAM(0x8804 + E_k)) >> 1] = 0;
    eproj_id[(uint16)(ERAM(0x8806 + E_k)) >> 1] = 0;
    sprite_instr_list_ptrs[ERAM(0x880C + E_k) >> 1] = 0;
    if (samus_input_handler != 0xE91D /* Samus_InputHandler_E91D */)
      samus_input_handler = 0xE913 /* Samus_InputHandler_E913 */;
    ERAM(0x8020 + (cur_enemy_index)) = 0;
  }
}
void YappingMaw_Frozen(void) {  // 0xA8A835
  Enemy_NormalFrozenAI_A8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  YappingMaw_Func_18(ERAM(0x8800 + E_k));
  YappingMaw_Func_18(ERAM(0x8802 + E_k));
  YappingMaw_Func_18(ERAM(0x8804 + E_k));
  YappingMaw_Func_18(ERAM(0x8806 + E_k));
  YappingMaw_Func_19();
}
void YappingMaw_Func_18(uint16 j) {  // 0xA8A85D
  int v1 = j >> 1;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_gfx_idx[v1] = ERAM(0x880A + E_k) | eproj_gfx_idx[v1] & 0xF1FF;
  if (E->frozen_timer) {
    eproj_gfx_idx[v1] = eproj_gfx_idx[v1] & 0xF1FF | 0xC00;
    uint16 frozen_timer = E->frozen_timer;
    if (sign16(frozen_timer - 90)) {
      if ((frozen_timer & 2) == 0)
        eproj_gfx_idx[v1] = ERAM(0x880A + E_k) | eproj_gfx_idx[v1] & 0xF1FF;
    }
  }
}
void YappingMaw_Func_19(void) {  // 0xA8A899
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 r18 = ERAM(0x880A + E_k);
  uint16 r20 = ERAM(0x880C + E_k);
  sprite_palettes[r20 >> 1] = r18 | sprite_palettes[r20 >> 1] & 0xF1FF;
  if (E->frozen_timer) {
    sprite_palettes[r20 >> 1] = sprite_palettes[r20 >> 1] & 0xF1FF | 0xC00;
    uint16 frozen_timer = E->frozen_timer;
    if (sign16(frozen_timer - 90)) {
      if ((frozen_timer & 2) == 0)
        sprite_palettes[r20 >> 1] = r18 | sprite_palettes[r20 >> 1] & 0xF1FF;
    }
  }
}
void Kago_Init(void) {  // 0xA8AB46
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  E->instruction_timer = 1;
  E->timer = 0;
  E->ai_var_E = 0;
  E->current_instruction = addr_kKago_Ilist_AB1E;
  E->ai_var_A = 0xAB7B /* Kago_Func_1 */;
  E->ai_preinstr = 0;
  ERAM(0x7808 + E_k) = E->parameter_1;
}
void CallKagoFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xA8AB7B: Kago_Func_1(k); return;
  case 0xA8AB81: return;
  default: nt_unimpl("Unreachable");
  }
}
void Kago_Main(void) {  // 0xA8AB75
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallKagoFunc(E->ai_var_A | 0xA80000, cur_enemy_index);
}
void Kago_Func_1(uint16 k) {  // 0xA8AB7B
  gEnemyData(k)->ai_var_A = 0xAB81 /* nullsub_306 */;
}
void Kago_Shot(void) {  // 0xA8AB83
  int16 v2;

  NormalEnemyShotAi();
  earthquake_type = 2;
  earthquake_timer = 16;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->ai_var_B) {
    E->ai_var_B = 1;
    E->current_instruction = addr_kKago_Ilist_AB32;
    E->instruction_timer = 1;
  }
  v2 = ERAM(0x7808 + E_k) - 1;
  ERAM(0x7808 + E_k) = v2;
  if (v2 < 0) {
    EnemyDeathAnimation(cur_enemy_index, 4);
    E->ai_preinstr = 1;
  }
  SpawnEprojWithGfx(E->y_pos, cur_enemy_index, addr_loc_A8D02E);
}
uint16 Beetom_Instr_1(uint16 k, uint16 jp) {  // 0xA8B75E
  return jp;
}
void Beetom_Func_1(void) {  // 0xA8B762
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = ERAM(0x7800 + E_k);
  E->instruction_timer = 1;
  E->timer = 0;
}
void Beetom_Init(void) {  // 0xA8B776
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_D = 0;
  E->ai_var_C = 0;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x7810 + E_k) = 0;
  E->ai_var_A = 0;
  E->ai_var_E = 64;
  E->ai_preinstr = joypad1_lastkeys;
  random_number = 23;
  ERAM(0x7804 + E_k) = Beetom_Func_2(12288, 4);
  ERAM(0x7806 + E_k) = Beetom_Func_2(0x4000, 5);
  ERAM(0x7808 + E_k) = Beetom_Func_2(12288, 3);
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6F2;
  if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) != 0)
    ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B696;
  Beetom_Func_1();
  E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
}
uint16 Beetom_Func_2(uint16 r22, uint16 r24) {  // 0xA8B7EF
  uint16 r18 = 0, r20 = 0;
  do {
    r18 += r24;
    r20 += ((uint16)BW_A2(0x838F + ((8 * r18) + 1)));
  } while (sign16(r20 - r22));
  return r18;
}
void Beetom_Func_3(void) {  // 0xA8B814
  if (IsSamusWithinEnemy_X(cur_enemy_index, 0x60))
    gEnemyData(cur_enemy_index)->ai_var_C = 0xB90F /* Beetom_Func_12 */;
  else
    gEnemyData(cur_enemy_index)->ai_var_C = 0xB82F /* Beetom_Func_4 */;
}
void Beetom_Func_4(void) {  // 0xA8B82F
  NextRandom();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = ((uint16)BW_A8(0xB74E + 2 * (random_number & 7)));
  ERAM(0x7812 + E_k) = random_number & 1;
}
void Beetom_Func_5(void) {  // 0xA8B84F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_D = 32;
  E->ai_var_C = 0xB9B2 /* Beetom_Func_17 */;
}
void Beetom_Func_6(void) {  // 0xA8B85F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = 0xB9C1 /* Beetom_Func_18 */;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B696;
  Beetom_Func_1();
}
void Beetom_Func_7(void) {  // 0xA8B873
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C = 0xBA24 /* Beetom_Func_19 */;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6F2;
  Beetom_Func_1();
}
void Beetom_Func_8(void) {  // 0xA8B887
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = ERAM(0x7804 + E_k);
  E->ai_var_C = 0xBA84 /* Beetom_Func_20 */;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6AC;
  Beetom_Func_1();
}
void Beetom_Func_9(void) {  // 0xA8B8A9
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = ERAM(0x7804 + E_k);
  E->ai_var_C = 0xBAB7 /* Beetom_Func_21 */;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B708;
  Beetom_Func_1();
}
void Beetom_Func_10(void) {  // 0xA8B8CB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = ERAM(0x7806 + E_k);
  E->ai_var_C = 0xBB55 /* Beetom_Func_24 */;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6AC;
  Beetom_Func_1();
}
void Beetom_Func_11(void) {  // 0xA8B8ED
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = ERAM(0x7806 + E_k);
  E->ai_var_C = 0xBB88 /* Beetom_Func_25 */;
  ERAM(0x780A + E_k) = 0;
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B708;
  Beetom_Func_1();
}
void Beetom_Func_12(void) {  // 0xA8B90F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = ERAM(0x7808 + E_k);
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B708;
  E->ai_var_C = 0xBC5A /* Beetom_Func_29 */;
  ERAM(0x7812 + E_k) = 1;
  if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) != 0) {
    ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6AC;
    E->ai_var_C = 0xBC26 /* Beetom_Func_28 */;
    ERAM(0x7812 + E_k) = 0;
  }
  Beetom_Func_1();
  ERAM(0x780A + E_k) = 0;
}
void Beetom_Func_13(void) {  // 0xA8B952
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6CC;
  Beetom_Func_1();
  E->ai_var_C = 0xBCF8 /* Beetom_Func_32 */;
}
void Beetom_Func_14(void) {  // 0xA8B966
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B728;
  Beetom_Func_1();
  E->ai_var_C = 0xBD42 /* Beetom_Func_33 */;
}
void Beetom_Func_15(void) {  // 0xA8B97A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B696;
  if (ERAM(0x7812 + E_k))
    ERAM(0x7800 + E_k) = addr_kBeetom_Ilist_B6F2;
  Beetom_Func_1();
  ERAM(0x780A + E_k) = 0;
  E->ai_var_C = 0xBD9D /* Beetom_Func_35 */;
}
void Beetom_Func_16(void) {  // 0xA8B9A2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = 0;
  E->ai_var_C = 0xBDC5 /* Beetom_Func_36 */;
}
void Beetom_Func_17(void) {  // 0xA8B9B2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_D & 0x8000) != 0)
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
}
void Beetom_Func_18(void) {  // 0xA8B9C1
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_D & 0x8000) != 0) {
    E->ai_var_D = 64;
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    E->x_pos -= 8;
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1))) {
      E->x_pos += 8;
      if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, -16384))
        E->ai_var_C = 0xB873 /* Beetom_Func_7 */;
    } else {
      E->ai_var_C = 0xB873 /* Beetom_Func_7 */;
      --E->y_pos;
      E->x_pos += 8;
    }
  }
}
void Beetom_Func_19(void) {  // 0xA8BA24
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((--E->ai_var_D & 0x8000) != 0) {
    E->ai_var_D = 64;
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    E->x_pos += 8;
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1))) {
      E->x_pos -= 8;
      if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, 0x4000))
        E->ai_var_C = 0xB85F /* Beetom_Func_6 */;
    } else {
      E->ai_var_C = 0xB85F /* Beetom_Func_6 */;
      --E->y_pos;
      E->x_pos -= 8;
    }
  }
}
void Beetom_Func_22(uint16 k) {  // 0xA8BAE7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  } else {
    E->ai_var_B -= 4;
    if (sign16(E->ai_var_B)) {
      E->ai_var_B = 0;
      ERAM(0x780A + E_k) = 1;
    }
  }
}
void Beetom_Func_23(uint16 k) {  // 0xA8BB20
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    E->ai_var_B += 4;
    if (!sign16(E->ai_var_B - 64))
      E->ai_var_B = 64;
  }
}
void Beetom_Func_20(void) {  // 0xA8BA84
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_23(cur_enemy_index);
  else
    Beetom_Func_22(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, -16384)) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_21(void) {  // 0xA8BAB7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_23(cur_enemy_index);
  else
    Beetom_Func_22(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, 0x4000)) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_26(uint16 k) {  // 0xA8BBB8
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  } else {
    E->ai_var_B -= 5;
    if (sign16(E->ai_var_B)) {
      E->ai_var_B = 0;
      ERAM(0x780A + E_k) = 1;
    }
  }
}
void Beetom_Func_27(uint16 k) {  // 0xA8BBF1
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    E->ai_var_B += 5;
    if (!sign16(E->ai_var_B - 64))
      E->ai_var_B = 64;
  }
}
void Beetom_Func_24(void) {  // 0xA8BB55
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_27(cur_enemy_index);
  else
    Beetom_Func_26(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, -16384)) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_25(void) {  // 0xA8BB88
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_27(cur_enemy_index);
  else
    Beetom_Func_26(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, 0x4000)) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_30(uint16 k) {  // 0xA8BC8A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * ((v2 + 2) >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  } else {
    int16 v3 = E->ai_var_B - 5;
    E->ai_var_B = v3;
    if (v3 < 0) {
      E->ai_var_B = 0;
      ERAM(0x780A + E_k) = 1;
    }
  }
}
void Beetom_Func_31(uint16 k) {  // 0xA8BCC3
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    uint16 v3 = E->ai_var_B + 3;
    E->ai_var_B = v3;
    if (!sign16(v3 - 64))
      E->ai_var_B = 64;
  }
}
void Beetom_Func_28(void) {  // 0xA8BC26
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_31(cur_enemy_index);
  else
    Beetom_Func_30(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(-3, 0))) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_29(void) {  // 0xA8BC5A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (ERAM(0x780A + E_k))
    Beetom_Func_31(cur_enemy_index);
  else
    Beetom_Func_30(cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(3, 0))) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_34(void) {  // 0xA8BD8C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (joypad1_lastkeys != E->ai_preinstr) {
    E->ai_preinstr = joypad1_lastkeys;
    --E->ai_var_E;
  }
}
void Beetom_Func_32(void) {  // 0xA8BCF8
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_E) {
    E->x_pos = samus_x_pos;
    E->y_pos = samus_y_pos - 4;
    Beetom_Func_34();
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(16, 0))) {
      ERAM(0x7812 + E_k) = 1;
      Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(-32, 0));
    }
    ERAM(0x7810 + E_k) = 0;
    E->ai_var_C = 0xB9A2 /* Beetom_Func_16 */;
  }
}
void Beetom_Func_33(void) {  // 0xA8BD42
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->ai_var_E) {
    E->x_pos = samus_x_pos;
    E->y_pos = samus_y_pos - 4;
    Beetom_Func_34();
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(-16, 0))) {
      ERAM(0x7812 + E_k) = 0;
      Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, __PAIR32__(32, 0));
    }
    ERAM(0x7810 + E_k) = 0;
    E->ai_var_C = 0xB9A2 /* Beetom_Func_16 */;
  }
}
void Beetom_Func_35(void) {  // 0xA8BD9D
  if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(3, 0))) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    if (ERAM(0x7812 + E_k))
      E->ai_var_C = 0xB873 /* Beetom_Func_7 */;
    else
      E->ai_var_C = 0xB85F /* Beetom_Func_6 */;
  }
}
void Beetom_Func_37(uint16 k) {  // 0xA8BDCC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  int v2 = (8 * E->ai_var_B) >> 1;
  if (Enemy_MoveDown(k, ((uint32)((uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1)) | (uint32)BW_A0(0xCBC7 + 4 * (v2 >> 1) + 2) << 16)))) {
    E->ai_var_C = 0xB814 /* Beetom_Func_3 */;
  } else {
    uint16 v3 = E->ai_var_B + 1;
    E->ai_var_B = v3;
    if (!sign16(v3 - 64))
      E->ai_var_B = 64;
  }
  uint16 v4 = (ERAM(0x7812 + E_k)) ? -2 : 2;
  if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(v4, 0))) {
    ERAM(0x7812 + E_k) ^= 1;
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
}
void Beetom_Func_36(void) {  // 0xA8BDC5
  Beetom_Func_37(cur_enemy_index);
}
void CallBeetomFunc(uint32 ea) {
  switch (ea) {
  case 0xA8B814: Beetom_Func_3(); return;  // 0xa8b814
  case 0xA8B82F: Beetom_Func_4(); return;  // 0xa8b82f
  case 0xA8B84F: Beetom_Func_5(); return;  // 0xa8b84f
  case 0xA8B85F: Beetom_Func_6(); return;  // 0xa8b85f
  case 0xA8B873: Beetom_Func_7(); return;  // 0xa8b873
  case 0xA8B887: Beetom_Func_8(); return;  // 0xa8b887
  case 0xA8B8A9: Beetom_Func_9(); return;  // 0xa8b8a9
  case 0xA8B8CB: Beetom_Func_10(); return;  // 0xa8b8cb
  case 0xA8B8ED: Beetom_Func_11(); return;  // 0xa8b8ed
  case 0xA8B90F: Beetom_Func_12(); return;  // 0xa8b90f
  case 0xA8B952: Beetom_Func_13(); return;  // 0xa8b952
  case 0xA8B966: Beetom_Func_14(); return;  // 0xa8b966
  case 0xA8B97A: Beetom_Func_15(); return;  // 0xa8b97a
  case 0xA8B9A2: Beetom_Func_16(); return;  // 0xa8b9a2
  case 0xA8B9B2: Beetom_Func_17(); return;  // 0xa8b9b2
  case 0xA8B9C1: Beetom_Func_18(); return;  // 0xa8b9c1
  case 0xA8BA24: Beetom_Func_19(); return;  // 0xa8ba24
  case 0xA8BA84: Beetom_Func_20(); return;  // 0xa8ba84
  case 0xA8BAB7: Beetom_Func_21(); return;  // 0xa8bab7
  case 0xA8BB55: Beetom_Func_24(); return;  // 0xa8bb55
  case 0xA8BB88: Beetom_Func_25(); return;  // 0xa8bb88
  case 0xA8BC26: Beetom_Func_28(); return;  // 0xa8bc26
  case 0xA8BC5A: Beetom_Func_29(); return;  // 0xa8bc5a
  case 0xA8BCF8: Beetom_Func_32(); return;  // 0xa8bcf8
  case 0xA8BD42: Beetom_Func_33(); return;  // 0xa8bd42
  case 0xA8BD9D: Beetom_Func_35(); return;  // 0xa8bd9d
  case 0xA8BDC5: Beetom_Func_36(); return;  // 0xa8bdc5
  default: nt_unimpl("Unreachable");
  }
}
void Beetom_Main(void) {  // 0xA8B80D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallBeetomFunc(E->ai_var_C | 0xA80000);
}
void Beetom_Touch(void) {  // 0xA8BE2E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!ERAM(0x7810 + E_k)) {
    if (ERAM(0x7812 + E_k))
      E->ai_var_C = 0xB966 /* Beetom_Func_14 */;
    else
      E->ai_var_C = 0xB952 /* Beetom_Func_13 */;
    ERAM(0x7810 + E_k) = 1;
    E->ai_var_E = 64;
    E->layer = 2;
    ERAM(0x780C + E_k) = samus_x_pos - E->x_pos;
    ERAM(0x780E + E_k) = samus_y_pos - E->y_pos;
  }
  if (samus_contact_damage_index)
    goto LABEL_11;
  if ((random_enemy_counter & 7) == 7 && !sign16(samus_health - 30))
    QueueSfx3_Max6(0x2D);
  if ((E->frame_counter & 0x3F) == 63) {
LABEL_11:
    Enemy_NormalTouchAI_A8();
    samus_invincibility_timer = 0;
    samus_knockback_timer = 0;
  }
}
void Beetom_Shot(void) {  // 0xA8BEAC
  Enemy_NormalShotAI_A8();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->frozen_timer
      && (E->ai_var_C == 0xBD42 /* Beetom_Func_33 */
          || E->ai_var_C == 0xBCF8 /* Beetom_Func_32 */)) {
    E->ai_var_C = 0xB97A /* Beetom_Func_15 */;
  }
  ERAM(0x7810 + E_k) = 0;
}
void WalkingLavaSeahorse_Init(void) {  // 0xA8DCCD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7808 + E_k) = 0;
  E->ai_preinstr = E->y_pos;
  ERAM(0x7804 + E_k) = E->x_pos;
  E->properties |= kEnemyProps_DisableSamusColl;
  E->current_instruction = addr_kWalkingLavaSeahorse_Ilist_DBE7;
  E->ai_var_A = 0xDD71 /* WalkingLavaSeahorse_Func_3 */;
  WalkingLavaSeahorse_Func_1(cur_enemy_index);
  do {
    AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_var_B, E->ai_var_C));
  } while ((WalkingLavaSeahorse_Func_2(cur_enemy_index) & 0x8000) != 0);
  while (1) {
    if (Enemy_MoveDown(cur_enemy_index, __PAIR32__(E->ai_var_B, E->ai_var_C)))
      break;
    WalkingLavaSeahorse_Func_2(cur_enemy_index);
  }
  ERAM(0x7806 + E_k) = E->y_pos;
  E->y_pos = E->ai_preinstr;
}
void WalkingLavaSeahorse_Func_1(uint16 k) {  // 0xA8DD37
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_B = -12;
  E->ai_var_C = 0;
  E->ai_var_D = 0;
  E->ai_var_E = 0;
  ERAM(0x7800 + E_k) = 0;
  ERAM(0x7802 + E_k) = 0x8000;
}
uint16 WalkingLavaSeahorse_Func_2(uint16 k) {  // 0xA8DD55
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->ai_var_B, &E->ai_var_C, __PAIR32__(ERAM(0x7800 + E_k), ERAM(0x7802 + E_k)));
  return E->ai_var_B;
}
void CallWalkingLavaSeahorseFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xA8DECC: return;
  case 0xA8DD71: WalkingLavaSeahorse_Func_3(k); return;
  case 0xA8DDC6: WalkingLavaSeahorse_Func_4(k); return;
  case 0xA8DE05: WalkingLavaSeahorse_Func_6(k); return;
  case 0xA8DE4B: WalkingLavaSeahorse_Func_7(k); return;
  case 0xA8DECD: WalkingLavaSeahorse_Func_8(k); return;
  case 0xA8DEEC: WalkingLavaSeahorse_Func_9(k); return;
  default: nt_unimpl("Unreachable");
  }
}
void WalkingLavaSeahorse_Main(void) {  // 0xA8DD6B
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallWalkingLavaSeahorseFunc(E->ai_var_A | 0xA80000, cur_enemy_index);
}
void WalkingLavaSeahorse_Func_3(uint16 k) {  // 0xA8DD71
  uint16 v4;
  int16 v5;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 xxx;
  uint16 v3 = abs16(ERAM(0x7806 + E_k) - samus_y_pos);
  if (sign16(v3 - 32) && (int16)(abs16((xxx = samus_x_pos - E->x_pos)) - ((uint16)BW_A8(0xDCC7 + 2 * (0)))) < 0) {
    WalkingLavaSeahorse_Func_1(k);
    v4 = addr_stru_A8DC4B;
    v5 = -2;
    if (!sign16(xxx)) {
      v4 = addr_stru_A8DCBB;
      v5 = 2;
    }
    E->ai_var_D = v5;
    E->current_instruction = v4;
    E->instruction_timer = 1;
    E->ai_var_A = 0xDDC6 /* WalkingLavaSeahorse_Func_4 */;
    QueueSfx2_Max6(0x5E);
  }
}
void WalkingLavaSeahorse_Func_4(uint16 k) {  // 0xA8DDC6
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_var_B, E->ai_var_C));
  if ((WalkingLavaSeahorse_Func_2(k) & 0x8000) == 0)
    sub_A8DDDE(k);
}
void sub_A8DDDE(uint16 k) {  // 0xA8DDDE
  EnemyData *v1 = gEnemyData(k);
  v1->ai_var_A = 0xDE05 /* WalkingLavaSeahorse_Func_6 */;
  uint16 v2 = addr_kWalkingLavaSeahorse_Ilist_DC51;
  if ((v1->ai_var_D & 0x8000) == 0)
    v2 = addr_kWalkingLavaSeahorse_Ilist_DCC1;
  v1->current_instruction = v2;
  v1->instruction_timer = 1;
}
void WalkingLavaSeahorse_Func_5(uint16 k) {  // 0xA8DDFA
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_B = 0;
  E->ai_var_C = 0;
  sub_A8DDDE(k);
}
void WalkingLavaSeahorse_Func_6(uint16 k) {  // 0xA8DE05
  int16 v3;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, __PAIR32__(E->ai_var_B, E->ai_var_C))) {
    int t = samus_x_pos - E->x_pos;
    uint16 v2 = addr_kWalkingLavaSeahorse_Ilist_DBE7;
    v3 = -2;
    if (!sign16(t)) {
      v3 = 2;
      v2 = addr_kWalkingLavaSeahorse_Ilist_DC57;
    }
    E->ai_var_D = v3;
    E->current_instruction = v2;
    E->instruction_timer = 1;
    E->ai_var_A = 0xDE4B /* WalkingLavaSeahorse_Func_7 */;
    ERAM(0x7808 + E_k) = 1;
  } else {
    WalkingLavaSeahorse_Func_2(k);
  }
}
void WalkingLavaSeahorse_Func_7(uint16 k) {  // 0xA8DE4B
  Enemy_MoveDown(k, INT16_SHL16(2));
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((int16)(abs16(ERAM(0x7804 + E_k) - E->x_pos) - ((uint16)BW_A8(0xDCCB + 2 * (0)))) >= 0) {
    E->ai_var_A = 0xDECD /* WalkingLavaSeahorse_Func_8 */;
    E->ai_var_B = -4;
    E->ai_var_C = 0;
    uint16 v4 = addr_kWalkingLavaSeahorse_Ilist_DC51;
    if ((E->ai_var_D & 0x8000) == 0)
      v4 = addr_kWalkingLavaSeahorse_Ilist_DCC1;
    E->current_instruction = v4;
    E->instruction_timer = 1;
  } else {
    if (ERAM(0x7808 + E_k))
      return;
    int t = samus_x_pos - E->x_pos;
    uint16 v3;
    if (!sign16(t)) {
      v3 = addr_kWalkingLavaSeahorse_Ilist_DC73;
      if ((E->ai_var_D & 0x8000) != 0)
        return;
    } else {
      v3 = addr_kWalkingLavaSeahorse_Ilist_DC03;
      if ((E->ai_var_D & 0x8000) == 0)
        return;
    }
    E->current_instruction = v3;
    E->instruction_timer = 1;
    E->ai_var_A = 0xDECC /* nullsub_257 */;
  }
}
void WalkingLavaSeahorse_Func_8(uint16 k) {  // 0xA8DECD
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_var_B, E->ai_var_C));
  if ((WalkingLavaSeahorse_Func_2(k) & 0x8000) == 0)
    E->ai_var_A = 0xDEEC /* WalkingLavaSeahorse_Func_9 */;
}
void WalkingLavaSeahorse_Func_9(uint16 k) {  // 0xA8DEEC
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(E->ai_var_B, E->ai_var_C));
  if ((int16)(E->y_pos - E->ai_preinstr) >= 0) {
    E->y_pos = E->ai_preinstr;
    E->x_pos = ERAM(0x7804 + E_k);
    E->ai_var_A = 0xDD71 /* WalkingLavaSeahorse_Func_3 */;
  } else {
    WalkingLavaSeahorse_Func_2(k);
  }
}
uint16 WalkingLavaSeahorse_Instr_4(uint16 k, uint16 jp) {  // 0xA8DF1C
  WalkingLavaSeahorse_DF20(0);
  return jp;
}
void WalkingLavaSeahorse_DF20(uint16 a) {  // 0xA8DF20
  SpawnEprojWithGfx(a, cur_enemy_index, addr_loc_A89E90);
  QueueSfx2_Max6(0x3F);
}
uint16 WalkingLavaSeahorse_Instr_3(uint16 k, uint16 jp) {  // 0xA8DF33
  WalkingLavaSeahorse_DF20(2);
  return jp;
}
uint16 WalkingLavaSeahorse_Instr_5(uint16 k, uint16 jp) {  // 0xA8DF39
  WalkingLavaSeahorse_DF20(4);
  return jp;
}
uint16 WalkingLavaSeahorse_Instr_6(uint16 k, uint16 jp) {  // 0xA8DF3F
  int16 v3;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_A = 0xDE4B /* WalkingLavaSeahorse_Func_7 */;
  v3 = random_number & 3;
  if ((random_number & 3) == 0)
    v3 = 2;
  ERAM(0x7808 + E_k) = v3;
  if ((E->ai_var_D & 0x8000) == 0)
    return (uint16)(addr_kWalkingLavaSeahorse_Ilist_DC57);
  return (uint16)(addr_kWalkingLavaSeahorse_Ilist_DBE7);
}
uint16 WalkingLavaSeahorse_Instr_2(uint16 k, uint16 jp) {  // 0xA8DF63
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  uint16 wlse_var_04 = ERAM(0x7808 + E_k);
  if (wlse_var_04)
    ERAM(0x7808 + E_k) = wlse_var_04 - 1;
  return WalkingLavaSeahorse_Instr_1(cur_enemy_index, jp);
}
uint16 WalkingLavaSeahorse_Instr_1(uint16 k, uint16 jp) {  // 0xA8DF71
  int16 wlse_var_D;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(E->ai_var_D))) {
    jp = (uint16)(addr_stru_A8DBE9);
    wlse_var_D = E->ai_var_D;
    if (wlse_var_D < 0)
      jp = (uint16)(addr_stru_A8DC59);
    E->ai_var_D = -wlse_var_D;
  } else {
    EnemyFunc_C8AD(cur_enemy_index);
  }
  return jp;
}
void WalkingLavaSeahorse_Func_10(void) {  // 0xA8DF9D
  NormalEnemyShotAi();
}
void BlueBrinstarFaceBlock_Init(void) {  // 0xA8E82E
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  v0->current_instruction = addr_kBlueBrinstarFaceBlock_Ilist_E828;
  uint16 v1 = 0xE86E /* BlueBrinstarFaceBlock_Func_1 */;
  if ((collected_items & 4) == 0)
    v1 = 0x804C /* nullsub_170_A8 */;
  nt_gfx_hook_addr = v1;
  nt_gfx_hook_bank = 168;
  variables_for_enemy_graphics_drawn_hook[0] = ((16 * v0->palette_index) & 0xFF00) >> 8;
  variables_for_enemy_graphics_drawn_hook[2] = 16;
  v0->parameter_2 = ((v0->parameter_2 & 1) >> 2) | ((v0->parameter_2 & 1) << 15);
}
void BlueBrinstarFaceBlock_Func_1(void) {  // 0xA8E86E
  if (!door_transition_flag_enemies && !--variables_for_enemy_graphics_drawn_hook[2]) {
    variables_for_enemy_graphics_drawn_hook[2] = 16;
    uint16 v0 = variables_for_enemy_graphics_drawn_hook[0];
    uint16 v1 = 8 * variables_for_enemy_graphics_drawn_hook[1];
    int n = 4;
    do {
      palette_buffer[(v0 >> 1) + 137] = ((uint16)BW_A8(0xE7CC + 2 * (v1 >> 1)));
      v1 += 2;
      v0 += 2;
    } while (--n);
    variables_for_enemy_graphics_drawn_hook[1] = (LOBYTE(variables_for_enemy_graphics_drawn_hook[1]) + 1) & 7;
  }
}
void BlueBrinstarFaceBlock_Main(void) {  // 0xA8E8AE
  if ((collected_items & 4) != 0) {
    nt_gfx_hook_addr = 0xE86E /* BlueBrinstarFaceBlock_Func_1 */;
    EnemyData *v1 = gEnemyData(cur_enemy_index);
    variables_for_enemy_graphics_drawn_hook[0] = ((16 * v1->palette_index) & 0xFF00) >> 8;
    if (!v1->ai_var_A) {
      uint16 SamusEnemyDelta_Y = GetSamusEnemyDelta_Y(cur_enemy_index);
      if ((int16)(Abs16(SamusEnemyDelta_Y) - v1->parameter_1) < 0) {
        uint16 SamusEnemyDelta_X = GetSamusEnemyDelta_X(cur_enemy_index);
        v1->ai_var_B = SamusEnemyDelta_X;
        if ((int16)(Abs16(SamusEnemyDelta_X) - v1->parameter_1) < 0 && (v1->ai_var_B & 0x8000) != v1->parameter_2) {
          uint16 v4 = addr_kBlueBrinstarFaceBlock_Ilist_E80C;
          if ((v1->ai_var_B & 0x8000) == 0)
            v4 = addr_kBlueBrinstarFaceBlock_Ilist_E81A;
          v1->current_instruction = v4;
          v1->instruction_timer = 1;
          v1->ai_var_A = 1;
          variables_for_enemy_graphics_drawn_hook[2] = 16;
        }
      }
    }
  }
}
void BlueBrinstarFaceBlock_Shot(void) {  // 0xA8E91D
  projectile_dir[collision_detection_index] &= ~0x10;
}
void KiHunter_Init(void) {  // 0xA8F188
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  ERAM(0x7828 + E_k) = 0;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = addr_kKiHunter_Ilist_E9FA;
  E->ai_preinstr = 0;
  E->ai_var_A = 0xF268 /* KiHunter_Func_1 */;
  ERAM(0x7810 + E_k) = 0;
  ERAM(0x7812 + E_k) = 1;
  ERAM(0x780C + E_k) = 0;
  ERAM(0x780E + E_k) = -1;
  uint16 v1 = E->y_pos - 16;
  ERAM(0x7814 + E_k) = v1;
  ERAM(0x7816 + E_k) = v1 + 32;
  ERAM(0x7818 + E_k) = E->x_pos;
  ERAM(0x781A + E_k) = E->y_pos;
  if ((E->parameter_1 & 0x8000) != 0) {
    ERAM(0x7828 + E_k) = 1;
    E->ai_var_A = 0xF55A /* KiHunter_Func_4 */;
    ERAM(0x7810 + E_k) = 0;
    ERAM(0x7812 + E_k) = 1;
  }
}
void KiHunterWings_Init(void) {  // 0xA8F214
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties |= kEnemyProps_DisableSamusColl;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = addr_kKiHunter_Ilist_EA4E;
  int v2 = cur_enemy_index >> 1;
  E->y_pos = gEnemyData((uint16)(cur_enemy_index + (-64)))->y_pos;
  E->x_pos = gEnemyData((uint16)(cur_enemy_index + (-64)))->x_pos;
  E->ai_var_A = 0xF6F3 /* KiHunter_Func_9 */;
  E->palette_index = gEnemyData((uint16)(cur_enemy_index + (-64)))->palette_index;
  E->vram_tiles_index = gEnemyData((uint16)(cur_enemy_index + (-64)))->vram_tiles_index;
  if ((gEnemyData((uint16)(cur_enemy_index + (-64)))->parameter_1 & 0x8000) != 0)
    E->properties |= kEnemyProps_Deleted;
}
void CallKiHunterFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xA8F268: KiHunter_Func_1(k); return;  // 0xa8f268
  case 0xA8F3B8: KiHunter_Func_2(k); return;  // 0xa8f3b8
  case 0xA8F4ED: KiHunter_Func_3(k); return;  // 0xa8f4ed
  case 0xA8F55A: KiHunter_Func_4(k); return;  // 0xa8f55a
  case 0xA8F58B: KiHunter_Func_5(k); return;  // 0xa8f58b
  case 0xA8F5F0: KiHunter_Func_6(k); return;  // 0xa8f5f0
  case 0xA8F68B: KiHunter_Func_7(k); return;  // 0xa8f68b
  case 0xA8F6B3: KiHunter_Func_8(k); return;
  case 0xA8F6F3: KiHunter_Func_9(k); return;  // 0xa8f6f3
  case 0xA8F7CF: KiHunter_Func_10(k); return;  // 0xa8f7cf
  case 0xA8F5E3: return;
  default: nt_unimpl("Unreachable");
  }
}
void KiHunter_Main(void) {  // 0xA8F25C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallKiHunterFunc(E->ai_var_A | 0xA80000, cur_enemy_index);
}
void KiHunterWings_Main(void) {  // 0xA8F262
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallKiHunterFunc(E->ai_var_A | 0xA80000, cur_enemy_index);
}
void KiHunter_Func_1(uint16 k) {  // 0xA8F268
  int16 v4;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, __PAIR32__(ERAM(0x7812 + E_k), ERAM(0x7810 + E_k)))) {
    v4 = -ERAM(0x7812 + E_k);
  } else {
    uint16 y_pos = E->y_pos;
    if ((int16)(y_pos - ERAM(0x7814 + E_k)) < 0) {
      v4 = 1;
    } else {
      if ((int16)(y_pos - ERAM(0x7816 + E_k)) < 0)
        goto LABEL_8;
      v4 = -1;
    }
  }
  ERAM(0x7812 + E_k) = v4;
LABEL_8:
  if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(ERAM(0x780E + E_k), ERAM(0x780C + E_k)))) {
    uint16 v5 = 0;
    bool v6 = (-ERAM(0x780E + E_k) & 0x8000) != 0;
    ERAM(0x780E + E_k) = -ERAM(0x780E + E_k);
    if (!v6)
      v5 = 4;
    int v7 = v5 >> 1;
    E->current_instruction = ((uint16)BW_A8(0xF3B0 + 2 * (v7)));
    EnemyData *E1 = gEnemyData(k + 64); const uint16 E1_k = (k + 64);
    E1->current_instruction = ((uint16)BW_A8(0xF3B0 + 2 * (v7 + 1)));
    E->instruction_timer = 1;
    E1->instruction_timer = 1;
  }
  EnemyFunc_C8AD(k);
  uint16 r18 = samus_x_pos - E->x_pos;
  uint16 r20 = abs16(r18);
  if ((int16)(r20 - ((uint16)BW_A8(0xF180 + 2 * (0)))) < 0 && !sign16(samus_y_pos - E->y_pos - 32)) {
    uint16 r24 = samus_y_pos - E->y_pos;
    uint16 v9;
    if ((r18 & 0x8000) != 0) {
      ERAM(0x7800 + E_k) = -2;
      ERAM(0x7804 + E_k) = 0;
      ERAM(0x7806 + E_k) = 0;
      ERAM(0x7808 + E_k) = -1;
      ERAM(0x780E + E_k) = -1;
      ERAM(0x780A + E_k) = -8192;
      E->ai_preinstr = 255;
      ERAM(0x781C + E_k) = 240;
      v9 = 0;
    } else {
      ERAM(0x7800 + E_k) = 2;
      ERAM(0x7804 + E_k) = 0;
      ERAM(0x7806 + E_k) = 0;
      ERAM(0x7808 + E_k) = 0;
      ERAM(0x780A + E_k) = 0x2000;
      E->ai_preinstr = 128;
      ERAM(0x780E + E_k) = 1;
      ERAM(0x781C + E_k) = 144;
      v9 = 4;
    }
    int v10 = v9 >> 1;
    E->current_instruction = ((uint16)BW_A8(0xF3B0 + 2 * (v10)));
    EnemyData *E1 = gEnemyData(k + 64); const uint16 E1_k = (k + 64);
    E1->current_instruction = ((uint16)BW_A8(0xF3B0 + 2 * (v10 + 1)));
    E->instruction_timer = 1;
    E1->instruction_timer = 1;
    E->ai_var_B = r18 + E->x_pos;
    E->ai_var_C = E->y_pos;
    E->ai_var_A = 0xF3B8 /* KiHunter_Func_2 */;
    ERAM(0x7824 + E_k) = r24;
    ERAM(0x7822 + E_k) = r20;
    ERAM(0x7820 + E_k) = 0;
  }
}
void KiHunter_Func_2(uint16 k) {  // 0xA8F3B8
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((ERAM(0x7808 + E_k) & 0x8000) == 0) {
    if ((int16)(E->ai_preinstr - ERAM(0x781C + E_k)) < 0)
      goto LABEL_9;
  } else if ((int16)(E->ai_preinstr - ERAM(0x781C + E_k)) >= 0) {
    goto LABEL_9;
  }
  if (!ERAM(0x7820 + E_k)) {
    ERAM(0x7820 + E_k) = 1;
    uint16 v2 = addr_kKiHunter_Ilist_EA32;
    if ((ERAM(0x7808 + E_k) & 0x8000) != 0)
      v2 = addr_kKiHunter_Ilist_EA08;
    E->current_instruction = v2;
    E->instruction_timer = 1;
  }
LABEL_9:
  if ((ERAM(0x7808 + E_k) & 0x8000) == 0) {
    AddToHiLo(&ERAM(0x7804 + E_k), &ERAM(0x7806 + E_k), __PAIR32__(ERAM(0x7808 + E_k), ERAM(0x780A + E_k)));
    if ((int16)(ERAM(0x7804 + E_k) - ERAM(0x7800 + E_k)) >= 0)
      ERAM(0x7804 + E_k) = ERAM(0x7800 + E_k);
    E->ai_preinstr += ERAM(0x7804 + E_k);
    if (!sign16(E->ai_preinstr - 256)) {
      E->ai_var_A = 0xF268 /* KiHunter_Func_1 */;
      return;
    }
  } else {
    AddToHiLo(&ERAM(0x7804 + E_k), &ERAM(0x7806 + E_k), __PAIR32__(ERAM(0x7808 + E_k), ERAM(0x780A + E_k)));
    if ((int16)(ERAM(0x7804 + E_k) - ERAM(0x7800 + E_k)) < 0)
      ERAM(0x7804 + E_k) = ERAM(0x7800 + E_k);
    E->ai_preinstr += ERAM(0x7804 + E_k);
    if (sign16(E->ai_preinstr - 128)) {
      E->ai_var_A = 0xF268 /* KiHunter_Func_1 */;
      return;
    }
  }
  uint16 r20 = E->ai_var_B + CosineMult8bit(E->ai_preinstr, ERAM(0x7822 + E_k)) - E->x_pos;
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL16(r20))) {
    if ((ERAM(0x7808 + E_k) & 0x8000) == 0) {
      ERAM(0x780C + E_k) = 0;
      ERAM(0x780E + E_k) = -1;
    } else {
      ERAM(0x780C + E_k) = 0;
      ERAM(0x780E + E_k) = 1;
    }
    goto LABEL_24;
  }
  EnemyFunc_C8AD(k);
  r20 = E->ai_var_C + SineMult8bit(E->ai_preinstr, ERAM(0x7824 + E_k)) - E->y_pos;
  if (Enemy_MoveDown(k, INT16_SHL16(r20))) {
LABEL_24:
    E->ai_var_A = 0xF4ED /* KiHunter_Func_3 */;
    ERAM(0x7810 + E_k) = 0;
    ERAM(0x7812 + E_k) = -1;
  }
}
void KiHunter_Func_3(uint16 k) {  // 0xA8F4ED
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(ERAM(0x780E + E_k), ERAM(0x780C + E_k)))
      || (EnemyFunc_C8AD(k), Enemy_MoveDown(k, __PAIR32__(ERAM(0x7812 + E_k), ERAM(0x7810 + E_k))))
      || (int16)(E->y_pos - ERAM(0x781A + E_k)) < 0) {
    E->ai_var_A = 0xF268 /* KiHunter_Func_1 */;
  }
}
uint16 KiHunter_Instr_1(uint16 k, uint16 jp) {  // 0xA8F526
  uint16 result = addr_kKiHunter_Ilist_E9FA;
  uint16 r18 = addr_kKiHunter_Ilist_EA4E;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((ERAM(0x780E + E_k) & 0x8000) == 0) {
    result = addr_kKiHunter_Ilist_EA24;
    r18 = addr_kKiHunter_Ilist_EA5E;
  }
  E->current_instruction = r18;
  E->instruction_timer = 1;
  EnemyData *E1 = gEnemyData(k + 64); const uint16 E1_k = (k + 64);
  if (E1->ai_var_A == 0xF6F3 /* KiHunter_Func_9 */) {
    E1->current_instruction = r18;
    E1->instruction_timer = 1;
  }
  return (uint16)(result);
}
void KiHunter_Func_4(uint16 k) {  // 0xA8F55A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, __PAIR32__(ERAM(0x7812 + E_k), ERAM(0x7810 + E_k)))) {
    E->ai_var_A = 0xF58B /* KiHunter_Func_5 */;
  } else {
    AddToHiLo(&ERAM(0x7812 + E_k), &ERAM(0x7810 + E_k), __PAIR32__(((uint16)BW_A8(0xF184 + 2 * (0))), ((uint16)BW_A8(0xF182 + 2 * (0)))));
  }
}
void KiHunter_Func_5(uint16 k) {  // 0xA8F58B
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = addr_locret_A8F5E3;
  ERAM(0x7810 + E_k) = 0;
  ERAM(0x7812 + E_k) = (random_number & 1) - 8;
  if ((int16)(E->x_pos - samus_x_pos) >= 0) {
    ERAM(0x780C + E_k) = 0;
    ERAM(0x780E + E_k) = -2;
    E->current_instruction = addr_kKiHunter_Ilist_EA8A;
  } else {
    ERAM(0x780C + E_k) = 0;
    ERAM(0x780E + E_k) = 2;
    E->current_instruction = addr_kKiHunter_Ilist_EAA6;
  }
  E->instruction_timer = 1;
}
uint16 KiHunter_Instr_2(uint16 k, uint16 jp) {  // 0xA8F5E4
  gEnemyData(cur_enemy_index)->ai_var_A = 0xF5F0 /* KiHunter_Func_6 */;
  return jp;
}
void KiHunter_Func_6(uint16 k) {  // 0xA8F5F0
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, __PAIR32__(ERAM(0x7812 + E_k), ERAM(0x7810 + E_k)))) {
    if ((ERAM(0x7812 + E_k) & 0x8000) != 0) {
      ERAM(0x7812 + E_k) = 1;
    } else {
      ERAM(0x7810 + E_k) = 0;
      ERAM(0x7812 + E_k) = -4;
      E->ai_var_A = addr_locret_A8F5E3;
      ERAM(0x781E + E_k) = 12;
      uint16 v4 = addr_kKiHunter_Ilist_EAC2;
      if (!sign16(E->current_instruction + 0x155A))
        v4 = addr_kKiHunter_Ilist_EADA;
      E->current_instruction = v4;
      E->instruction_timer = 1;
    }
  } else {
    if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(ERAM(0x780E + E_k), ERAM(0x780C + E_k)))) {
      ERAM(0x780E + E_k) = -ERAM(0x780E + E_k);
    } else {
      EnemyFunc_C8AD(k);
      AddToHiLo(&ERAM(0x7812 + E_k), &ERAM(0x7810 + E_k), __PAIR32__(((uint16)BW_A8(0xF184 + 2 * (0))), ((uint16)BW_A8(0xF182 + 2 * (0)))));
    }
  }
}
uint16 KiHunter_Instr_3(uint16 k, uint16 jp) {  // 0xA8F67F
  gEnemyData(cur_enemy_index)->ai_var_A = 0xF68B /* KiHunter_Func_7 */;
  return jp;
}
void KiHunter_Func_7(uint16 k) {  // 0xA8F68B
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v2 = ERAM(0x781E + E_k) - 1;
  ERAM(0x781E + E_k) = v2;
  if (!v2) {
    uint16 v3 = 0xF58B /* KiHunter_Func_5 */;
    uint16 v4 = abs16(E->x_pos - samus_x_pos);
    if (sign16(v4 - 96))
      v3 = 0xF6B3 /* KiHunter_Func_8 */;
    E->ai_var_A = v3;
  }
}
void KiHunter_Func_8(uint16 k) {  // 0xA8F6B3
  uint16 v1 = addr_kKiHunter_Ilist_EAF2;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((int16)(E->x_pos - samus_x_pos) < 0)
    v1 = addr_kKiHunter_Ilist_EB10;
  E->current_instruction = v1;
  E->instruction_timer = 1;
  E->ai_var_A = addr_locret_A8F5E3;
}
uint16 KiHunter_Instr_4(uint16 k, uint16 jp) {  // 0xA8F6D2
  sub_A8F6DC(k, addr_loc_A8CF18);
  return jp;
}
uint16 KiHunter_Instr_5(uint16 k, uint16 jp) {  // 0xA8F6D8
  sub_A8F6DC(k, addr_loc_A8CF26);
  return jp;
}
void sub_A8F6DC(uint16 k, uint16 j) {  // 0xA8F6DC
  QueueSfx2_Max6(0x4C);
  SpawnEprojWithGfx(0, cur_enemy_index, j);
  ERAM(0x781E + (cur_enemy_index)) = 24;
}
void KiHunter_Func_9(uint16 k) {  // 0xA8F6F3
  int v1 = k >> 1;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos = gEnemyData((uint16)(k + (-64)))->x_pos;
  E->y_pos = gEnemyData((uint16)(k + (-64)))->y_pos;
}
void KiHunter_Shot(void) {  // 0xA8F701
  NormalEnemyShotAi();
  uint16 old_cur_enemy_index = cur_enemy_index;
  EnemyData *EK = gEnemyData(cur_enemy_index); const uint16 EK_k = (cur_enemy_index);
  EnemyData *E1 = gEnemyData(cur_enemy_index + 64); const uint16 E1_k = (cur_enemy_index + 64);
  uint16 health = EK->health;
  if (health) {
    if (health == E1->parameter_1 || (int16)(health - E1->parameter_1) < 0) {
      if (!ERAM(0x7828 + EK_k)) {
        ERAM(0x7828 + EK_k) = 1;
        EK->ai_var_A = 0xF55A /* KiHunter_Func_4 */;
        ERAM(0x7810 + EK_k) = 0;
        ERAM(0x7812 + EK_k) = 1;
        cur_enemy_index = old_cur_enemy_index + 64;
        if (E1->ai_var_A != 0xF7CF /* KiHunter_Func_10 */) {
          ERAM(0x780E + E1_k) = E1->y_pos;
          ERAM(0x7810 + E1_k) = E1->x_pos;
          KiHunter_Func_17();
          KiHunter_Func_12();
          KiHunter_Func_13();
          E1->ai_preinstr = -8192;
          E1->ai_var_A = 0xF7CF /* KiHunter_Func_10 */;
          ERAM(0x7800 + E1_k) = 0xF7DB /* KiHunter_Func_11 */;
          ERAM(0x780C + E1_k) = ERAM(0x780E + E1_k) - ERAM(0x7816 + E1_k);
          ERAM(0x780A + E1_k) = E1->x_pos;
          E1->ai_var_B = ERAM(0x7814 + E1_k);
          E1->current_instruction = addr_kKiHunter_Ilist_EA7E;
          E1->spritemap_pointer = addr_kSpritemap_Nothing_A8;
          E1->instruction_timer = 1;
          E1->timer = 0;
          E1->properties |= kEnemyProps_ProcessedOffscreen;
        }
        cur_enemy_index = old_cur_enemy_index;
      }
    } else {
      E1->ai_handler_bits = EK->ai_handler_bits;
      E1->frozen_timer = EK->frozen_timer;
      E1->invincibility_timer = EK->invincibility_timer;
      E1->flash_timer = EK->flash_timer;
    }
  } else {
    E1->properties = 512;
  }
}
void CallKiHunterBFunc(uint32 ea) {
  switch (ea) {
  case 0xA8F7DB: KiHunter_Func_11(); return;
  case 0xA8F8AD: KiHunter_Func_14(); return;
  default: nt_unimpl("Unreachable");
  }
}
void KiHunter_Func_10(uint16 k) {  // 0xA8F7CF
  uint16 r18 = ERAM(0x7800 + (cur_enemy_index));
  CallKiHunterBFunc(r18 | 0xA80000);
}
void KiHunter_Func_11(void) {  // 0xA8F7DB
  int16 v2;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_preinstr += ((uint16)BW_A2(0x838F + ((2) + (8 * HIBYTE(E->ai_var_B)) + 1)));
  E->y_pos = ERAM(0x780C + E_k) + SineMult8bit(HIBYTE(E->ai_preinstr), ((uint8)BB_A8(0xF186 + (0)))) - ERAM(0x7808 + E_k);
  E->x_pos = ERAM(0x780A + E_k) + CosineMult8bit(HIBYTE(E->ai_preinstr), ((uint8)BB_A8(0xF186 + (0)))) - ERAM(0x7806 + E_k);
  if (sign16(E->ai_preinstr + 0x4000)) {
    KiHunter_Func_16(cur_enemy_index);
  } else {
    v2 = E->ai_var_B - 384;
    if (v2 < 0)
      v2 = 256;
    E->ai_var_B = v2;
  }
}
void KiHunter_Func_12(void) {  // 0xA8F851
  uint16 v1 = CosineMult8bit(0xE0, ((uint8)BB_A8(0xF186 + (0))));
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7806 + E_k) = v1;
  ERAM(0x7808 + E_k) = SineMult8bit(0xE0, ((uint8)BB_A8(0xF186 + (0))));
}
void KiHunter_Func_13(void) {  // 0xA8F87F
  uint16 v1 = CosineMult8bit(0xA0, ((uint8)BB_A8(0xF186 + (0))));
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7802 + E_k) = v1;
  ERAM(0x7804 + E_k) = SineMult8bit(0xA0, ((uint8)BB_A8(0xF186 + (0))));
}
void KiHunter_Func_14(void) {  // 0xA8F8AD
  int16 khr_var_07;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_preinstr += ((uint16)BW_A2(0x838F + ((8 * HIBYTE(E->ai_var_B)) + 1)));
  uint16 r20 = ERAM(0x780C + E_k) + SineMult8bit(HIBYTE(E->ai_preinstr), ((uint8)BB_A8(0xF186 + (0)))) - ERAM(0x7804 + E_k) - E->y_pos;
  if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(r20))) {
    E->properties |= kEnemyProps_Deleted;
    E->x_pos = ERAM(0x7810 + E_k);
    khr_var_07 = ERAM(0x780E + E_k);
    E->y_pos = khr_var_07;
  } else {
    E->x_pos = ERAM(0x780A + E_k) + CosineMult8bit(HIBYTE(E->ai_preinstr), ((uint8)BB_A8(0xF186 + (0)))) - ERAM(0x7802 + E_k);
    if (!sign16(E->ai_preinstr + 0x4000)) {
      KiHunter_Func_15(cur_enemy_index);
      return;
    }
    khr_var_07 = E->ai_var_B - 384;
    if (khr_var_07 < 0)
      khr_var_07 = 256;
  }
  gEnemyData(cur_enemy_index)->ai_var_B = khr_var_07;
}
void KiHunter_Func_15(uint16 k) {  // 0xA8F947
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = 0xF7DB /* KiHunter_Func_11 */;
  E->ai_var_B = ERAM(0x7814 + E_k);
  E->ai_preinstr = -8192;
  ERAM(0x780A + E_k) = E->x_pos;
  ERAM(0x780C + E_k) = E->y_pos;
}
void KiHunter_Func_16(uint16 k) {  // 0xA8F96A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = 0xF8AD /* KiHunter_Func_14 */;
  E->ai_var_B = ERAM(0x7814 + E_k);
  E->ai_preinstr = -24576;
  ERAM(0x780A + E_k) = E->x_pos;
  ERAM(0x780C + E_k) = E->y_pos;
}
void KiHunter_Func_17(void) {  // 0xA8F98D
  uint16 v5;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x7814 + E_k) = 0;
  E->ai_var_B = 0;
  do {
    uint16 v3 = ERAM(0x7814 + E_k) + 384;
    ERAM(0x7814 + E_k) = v3;
    v3 >>= 8;
    v5 = ((uint16)BW_A2(0x838F + ((8 * v3) + 1))) + E->ai_var_D;
    E->ai_var_D = v5;
  } while (sign16(v5 - 0x2000));
}
void Enemy_NormalFrozenAI_AA(void) {  // 0xAA8041
  NormalEnemyFrozenAI();
}
uint16 Enemy_ClearAiPreInstr_AA(uint16 k, uint16 jp) {  // 0xAA8074
  gEnemyData(k)->ai_preinstr = 0x807B /* nullsub_171_AA */;
  return jp;
}
uint16 Enemy_SetAiPreInstr_AA(uint16 k, uint16 jp) {  // 0xAA806B
  gEnemyData(k)->ai_preinstr = IW_AA((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_3(uint16 k, uint16 jp) {  // 0xAAB09C
  gEnemyData(k)->ai_var_E = IW_AA((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_31(uint16 k, uint16 jp) {  // 0xAAB11D
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_2 = E->parameter_2 | 0x8000;
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  SpawnEprojWithGfx(0, k, addr_stru_86A95B);
  return jp;
}
uint16 Torizo_Instr_33(uint16 k, uint16 jp) {  // 0xAAB1BE
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_2 |= 0x4000;
  return jp;
}
uint16 Torizo_Instr_36(uint16 k, uint16 jp) {  // 0xAAB224
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->properties &= ~kEnemyProps_Invisible;
  return jp;
}
uint16 Torizo_Instr_37(uint16 k, uint16 jp) {  // 0xAAB22E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->properties |= kEnemyProps_Invisible;
  return jp;
}
uint16 Torizo_Instr_35(uint16 k, uint16 jp) {  // 0xAAB238
  for (int i = 30; i >= 0; i -= 2) {
    int v3 = i >> 1;
    target_palettes[v3 + 160] = 0;
    target_palettes[v3 + 144] = 0;
  }
  return jp;
}
uint16 Torizo_Instr_38(uint16 k, uint16 jp) {  // 0xAAB24D
  SetBossBitForCurArea(4);
  QueueMusic_Delayed8(((uint16)BW_AA(0xB09A + 2 * (0))));
  if (area_index)
    Enemy_ItemDrop_GoldenTorizo(k);
  else
    Enemy_ItemDrop_BombTorizo(k);
  return jp;
}
uint16 Torizo_Instr_6(uint16 k, uint16 jp) {  // 0xAAB271
  sub_82DAF7(0x600);
  return jp;
}
uint16 Torizo_Instr_5(uint16 k, uint16 jp) {  // 0xAAB94D
  Torizo_C268();
  return jp;
}
uint16 Torizo_Instr_9(uint16 k, uint16 jp) {  // 0xAAB951
  QueueMusic_Delayed8(((uint16)BW_AA(0xB098 + 2 * (0))));
  SpawnPalfxObject(addr_stru_8DF759);
  return jp;
}
void Torizo_C20A(uint16 k) {  // 0xAAC20A
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((E->parameter_1 & 0x8000) != 0)
    v2 = 512;
  else
    v2 = -512;
  E->ai_var_A = v2;
  E->ai_var_B = -1472;
  E->ai_var_C = 40;
  E->instruction_timer = 1;
}
void Torizo_C22D(uint16 k) {  // 0xAAC22D
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((E->parameter_1 & 0x8000) != 0)
    v2 = -768;
  else
    v2 = 768;
  E->ai_var_A = v2;
  E->ai_var_B = -1152;
  E->ai_var_C = 40;
  E->instruction_timer = 1;
}
void Torizo_C250(void) {  // 0xAAC250
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    target_palettes[v1 + 160] = ((uint16)BW_AA(0x86E7 + 2 * (v1)));
    target_palettes[v1 + 144] = ((uint16)BW_AA(0x86C7 + 2 * (v1)));
  }
}
void Torizo_C268(void) {  // 0xAAC268
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    target_palettes[v1 + 160] = ((uint16)BW_AA(0x8727 + 2 * (v1)));
    target_palettes[v1 + 144] = ((uint16)BW_AA(0x8707 + 2 * (v1)));
  }
}
void Torizo_C280(void) {  // 0xAAC280
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    target_palettes[v1 + 160] = ((uint16)BW_AA(0x8767 + 2 * (v1)));
    target_palettes[v1 + 144] = ((uint16)BW_AA(0x8747 + 2 * (v1)));
  }
}
void Torizo_C298(void) {  // 0xAAC298
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    target_palettes[v1 + 160] = ((uint16)BW_AA(0x87A7 + 2 * (v1)));
    target_palettes[v1 + 144] = ((uint16)BW_AA(0x8787 + 2 * (v1)));
  }
}
void Torizo_C2B0(void) {  // 0xAAC2B0
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    palette_buffer[v1 + 160] = ((uint16)BW_AA(0x8727 + 2 * (v1)));
    palette_buffer[v1 + 144] = ((uint16)BW_AA(0x8707 + 2 * (v1)));
  }
}
uint16 Torizo_Instr_7(uint16 k, uint16 jp) {  // 0xAAC2C8
  return jp;
}
uint16 Torizo_Instr_2(uint16 k, uint16 jp) {  // 0xAAC2C9
  ERAM(0x7808 + (k)) = 30583;
  return jp;
}
uint16 Torizo_Instr_8(uint16 k, uint16 jp) {  // 0xAAC2D1
  ERAM(0x7808 + (k)) = 0;
  return jp;
}
uint16 Torizo_Instr_25(uint16 k, uint16 jp) {  // 0xAAC2D9
  if ((gEnemyData(k)->parameter_2 & 0x4000) != 0)
    return (uint16)(IW_AA((uint16)(jp + 0)));
  if (area_index)
    return (uint16)(IW_AA((uint16)(jp + 2)));
  else
    return (uint16)(jp + 4);
}
uint16 Torizo_Instr_22(uint16 k, uint16 jp) {  // 0xAAC2ED
  ERAM(0x7800 + (k)) = IW_AA((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_19(uint16 k, uint16 jp) {  // 0xAAC2F7
  return (uint16)(ERAM(0x7800 + (k)));
}
uint16 Torizo_Instr_32(uint16 k, uint16 jp) {  // 0xAAC2FD
  return (uint16)(ERAM(0x7802 + (k)));
}
uint16 Torizo_Instr_30(uint16 k, uint16 jp) {  // 0xAAC303
  uint16 a = IW_AA((uint16)(jp + 0));
  for (int i = 5; i >= 0; --i)
    SpawnEprojWithRoomGfx(addr_kEproj_BombTorizoLowHealthExplode, a);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->current_instruction = ((uint16)(jp + 2));
  E->flash_timer = 40;
  E->instruction_timer = 40;
  return 0;
}
uint16 Torizo_Instr_34(uint16 k, uint16 jp) {  // 0xAAC32F
  SpawnEprojWithRoomGfx(addr_kEproj_BombTorizoDeathExplosion, 0);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->current_instruction = (jp);
  E->flash_timer = 1;
  E->instruction_timer = 1;
  return 0;
}
uint16 Torizo_Instr_24(uint16 k, uint16 jp) {  // 0xAAC34A
  SpawnEprojWithRoomGfx(addr_kEproj_TourianLandingDustCloudsRightFoot, 0);
  SpawnEprojWithRoomGfx(addr_stru_86AFF3, 0);
  return jp;
}
uint16 Torizo_Instr_12(uint16 k, uint16 jp) {  // 0xAAC35B
  uint16 health = gEnemyData(k)->health;
  if (health < 0x15E)
    SpawnEprojWithGfx(health, k, addr_kEproj_BombTorizoLowHealthInitialDrool);
  return jp;
}
uint16 Torizo_Instr_10(uint16 k, uint16 jp) {  // 0xAAC36D
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_1 |= 0x4000;
  return jp;
}
uint16 Torizo_Instr_11(uint16 k, uint16 jp) {  // 0xAAC377
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_1 &= 0x1FFF;
  ++ERAM(0x7812 + E_k);
  return jp;
}
uint16 Torizo_Instr_29(uint16 k, uint16 jp) {  // 0xAAC38A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_1 = E->parameter_1 & 0x1FFF | 0x8000;
  ++ERAM(0x7812 + E_k);
  return jp;
}
uint16 Torizo_Instr_1(uint16 k, uint16 jp) {  // 0xAAC3A0
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_1 = E->parameter_1 & 0x1FFF | 0x2000;
  ++ERAM(0x7812 + E_k);
  return jp;
}
uint16 Torizo_Instr_28(uint16 k, uint16 jp) {  // 0xAAC3B6
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_1 = E->parameter_1 & 0x1FFF | 0xA000;
  ++ERAM(0x7812 + E_k);
  return jp;
}
uint16 Torizo_Instr_4(uint16 k, uint16 jp) {  // 0xAAC3CC
  uint16 v2 = IW_AA((uint16)(jp + 0));
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos += ((int16)BW_AA(0xC3EE + 2 * (v2 >> 1)));
  E->y_pos += ((int16)BW_AA(0xC40E + 2 * ((v2 & 0xF) >> 1)));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_40(uint16 k, uint16 jp) {  // 0xAAC41E
  uint16 v2 = IW_AA((uint16)(jp + 0));
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos -= ((int16)BW_AA(0xC440 + 2 * (v2 >> 1)));
  E->y_pos -= ((int16)BW_AA(0xC460 + 2 * ((v2 & 0xF) >> 1)));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_16(uint16 k, uint16 jp) {  // 0xAAC470
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = ((int16)BW_AA(0xC4BD + 2 * (IW_AA((uint16)(jp + 0)) >> 1)));
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL16(E->ai_var_A))) {
    ERAM(0x7806 + E_k) = 0;
    if ((E->parameter_1 & 0x8000) != 0)
      return (uint16)(addr_kTorizo_Ilist_B962);
    else
      return (uint16)(addr_kTorizo_Ilist_BDD8);
  } else {
    EnemyFunc_C8AD(k);
    if (((E->parameter_1 ^ (samus_x_pos - E->x_pos)) & 0x8000) == 0) {
      if (!ERAM(0x7806 + E_k))
        ERAM(0x7806 + E_k) = 72;
      return (uint16)(jp + 2);
    } else {
      return (uint16)(jp + 2);
    }
  }
}
uint16 Torizo_Instr_27(uint16 k, uint16 jp) {  // 0xAAC4E5
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = ((int16)BW_AA(0xC532 + 2 * (IW_AA((uint16)(jp + 0)) >> 1)));
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL16(E->ai_var_A))) {
    ERAM(0x7806 + E_k) = 0;
    if ((E->parameter_1 & 0x8000) != 0)
      return (uint16)(addr_kTorizo_Ilist_BD0E);
    else
      return (uint16)(addr_kTorizo_Ilist_C188);
  } else {
    EnemyFunc_C8AD(k);
    if (((E->parameter_1 ^ (samus_x_pos - E->x_pos)) & 0x8000) == 0) {
      if (!ERAM(0x7806 + E_k))
        ERAM(0x7806 + E_k) = 72;
      return (uint16)(jp + 2);
    } else {
      return (uint16)(jp + 2);
    }
  }
}
uint16 Torizo_Instr_23(uint16 k, uint16 jp) {  // 0xAAC55A
  if ((gEnemyData(k)->ai_var_B & 0x8000) != 0)
    return (uint16)(IW_AA((uint16)(jp + 0)));
  else
    return (uint16)(jp + 2);
}
uint16 Torizo_Instr_14(uint16 k, uint16 jp) {  // 0xAAC567
  if (CompareDistToSamus_X(k, 0x38) & 1)
    return (uint16)(jp + 2);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (((E->parameter_1 ^ (samus_x_pos - E->x_pos)) & 0x8000) == 0)
    return (uint16)(jp + 2);
  ERAM(0x7800 + E_k) = ((uint16)(jp + 2));
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_15(uint16 k, uint16 jp) {  // 0xAAC58B
  if (CompareDistToSamus_X(k, 0x20) & 1 || (Torizo_Func_12(k) & 0x8000) != 0)
    return (uint16)(jp + 2);
  Torizo_C22D(k);
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_26(uint16 k, uint16 jp) {  // 0xAAC5A4
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = ((uint16)(jp + 4));
  if (samus_missiles < 5 || ((nmi_frame_counter_word + (samus_x_pos & 1) + (samus_x_pos >> 1)) & 8) != 0)
    return (uint16)(IW_AA((uint16)(jp + 0)));
  else
    return (uint16)(IW_AA((uint16)(jp + 2)));
}
uint16 Torizo_Instr_18(uint16 k, uint16 jp) {  // 0xAAC5CB
  SpawnEprojWithGfx(0, k, addr_kEproj_BombTorizosChozoOrbs);
  SpawnEprojWithGfx(0, k, addr_kEproj_BombTorizosChozoOrbs);
  SpawnEprojWithGfx(0, k, addr_kEproj_BombTorizosChozoOrbs);
  return jp;
}
uint16 Torizo_Instr_20(uint16 k, uint16 jp) {  // 0xAAC5E3
  SpawnEprojWithGfx(IW_AA((uint16)(jp + 0)), k, addr_kEproj_BombTorizoSonicBoom);
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_44(uint16 k, uint16 jp) {  // 0xAAC5F2
  SpawnEprojWithGfx(IW_AA((uint16)(jp + 0)), k, addr_kEproj_GoldenTorizoSonicBoom);
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_21(uint16 k, uint16 jp) {  // 0xAAC601
  SpawnEprojWithRoomGfx(addr_kEproj_BombTorizoExplosiveSwipe, IW_AA((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_17(uint16 k, uint16 jp) {  // 0xAAC610
  QueueSfx2_Max6(0x27);
  return jp;
}
uint16 Torizo_Instr_13(uint16 k, uint16 jp) {  // 0xAAC618
  QueueSfx2_Max6(0x4B);
  return jp;
}
void Torizo_C620(uint16 k) {  // 0xAAC620
  if (!area_index && (random_number & 0x8142) == 0) {
    uint16 health = gEnemyData(k)->health;
    if (health && health < 0x15E)
      SpawnEprojWithGfx(health, k, addr_kEproj_BombTorizoLowHealthInitialDrool);
  }
}
void Torizo_C643(uint16 k) {  // 0xAAC643
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveDown(k, INT16_SHL8(E->ai_var_B))) {
    int16 v3 = E->ai_var_B;
    if (v3 >= 0 && v3 != 256) {
      earthquake_type = 4;
      earthquake_timer = 32;
      E->ai_var_B = 256;
    }
  } else {
    E->ai_var_B += 40;
  }
}
void Torizo_Hurt(void) {  // 0xAAC67E
  Torizo_C620(cur_enemy_index);
  if (gEnemyData(cur_enemy_index)->flash_timer & 1) {
    for (int i = 30; i >= 0; i -= 2) {
      int v2 = i >> 1;
      palette_buffer[v2 + 160] = 0x7FFF;
      palette_buffer[v2 + 144] = 0x7FFF;
    }
  } else {
    Torizo_C2B0();
  }
}
void CallTorizoFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xAAC6AB: return;
  case 0xAAC6BF: Torizo_Func_3(k); return;
  case 0xAAC6C6: Torizo_Func_4(k); return;
  case 0xAAC6FF: Torizo_Func_1(k); return;
  case 0xAAD5C2: Torizo_Func_9(k); return;
  case 0xAAD5DF: Torizo_Func_10(k); return;
  case 0xAAD5E6: Torizo_D5E6(k); return;
  default: nt_unimpl("Unreachable");
  }
}
void Torizo_Main(void) {  // 0xAAC6A4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallTorizoFunc(E->ai_var_E | 0xAA0000, cur_enemy_index);
}
void Torizo_Func_2(uint16 k) {  // 0xAAC6AC
  Torizo_C643(k);
  if (!(sub_82DAF7(0x600) & 1))
    gEnemyData(k)->ai_var_E = 0xC6AB /* nullsub_270 */;
}
void Torizo_Func_3(uint16 k) {  // 0xAAC6BF
  Torizo_C620(k);
  Torizo_C643(k);
}
void Torizo_Func_4(uint16 k) {  // 0xAAC6C6
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->properties |= kEnemyProps_Tangible;
  uint16 v2 = 78;
  while (plm_header_ptr[v2 >> 1] != addr_kPlmHeader_D6EA) {
    v2 -= 2;
    if ((v2 & 0x8000) != 0) {
      QueueMusic_Delayed8(((uint16)BW_AA(0xB096 + 2 * (0))));
      E->properties &= ~kEnemyProps_Tangible;
      E->current_instruction += 2;
      E->instruction_timer = 1;
      return;
    }
  }
}
void Torizo_Func_1(uint16 k) {  // 0xAAC6FF
  Torizo_C620(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((E->parameter_2 & 0x8000) != 0 || E->health >= 0x15E) {
    if ((E->parameter_2 & 0x4000) != 0 || E->health >= 0x64) {
      CallEnemyPreInstr(E->ai_preinstr | 0xAA0000);
    } else {
      uint16 v2;
      if ((E->parameter_1 & 0x8000) != 0)
        v2 = addr_kTorizo_Ilist_BD0E;
      else
        v2 = addr_kTorizo_Ilist_C188;
      ERAM(0x7800 + E_k) = v2;
      E->current_instruction = addr_kTorizo_Ilist_B155;
      E->instruction_timer = 1;
    }
  } else {
    ERAM(0x7802 + E_k) = E->current_instruction;
    E->current_instruction = addr_kTorizo_Ilist_B0E5;
    E->instruction_timer = 1;
  }
}
void Torizo_Func_5(uint16 k) {  // 0xAAC752
  uint16 v3;
  uint16 v8;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((E->parameter_2 & 0x4000) != 0) {
    uint16 torizo_var_03 = ERAM(0x7806 + E_k);
    if (!torizo_var_03 || (v8 = torizo_var_03 - 1, (ERAM(0x7806 + E_k) = v8) != 0)) {
      uint16 v10 = abs16(E->ai_var_A) + 1;
      if (v10 >= 0x10)
        v10 = 15;
      if (!(Enemy_MoveDown(k, INT16_SHL16(v10)))) {
        uint16 v11;
        if ((E->parameter_1 & 0x8000) != 0)
          v11 = addr_off_AAC0F2;
        else
          v11 = addr_off_AABC78;
        E->current_instruction = v11;
        E->instruction_timer = 1;
        E->ai_var_B = 256;
        E->ai_var_A = 0;
      }
    } else {
      ERAM(0x7806 + E_k) = 0;
      uint16 v9;
      if ((E->parameter_1 & 0x8000) != 0)
        v9 = addr_kTorizo_Ilist_BD0E;
      else
        v9 = addr_kTorizo_Ilist_C188;
      E->current_instruction = v9;
      E->instruction_timer = 1;
    }
  } else {
    uint16 v2 = ERAM(0x7806 + E_k);
    if (!v2 || (v3 = v2 - 1, (ERAM(0x7806 + E_k) = v3) != 0)) {
      uint16 v5 = abs16(E->ai_var_A) + 1;
      if (v5 >= 0x10)
        v5 = 15;
      if (!(Enemy_MoveDown(k, INT16_SHL16(v5)))) {
        uint16 v6;
        if ((E->parameter_1 & 0x8000) != 0)
          v6 = addr_off_AAC0F2;
        else
          v6 = addr_off_AABC78;
        E->current_instruction = v6;
        E->instruction_timer = 1;
        E->ai_var_B = 256;
        E->ai_var_A = 0;
      }
    } else {
      ERAM(0x7806 + E_k) = 0;
      uint16 v4;
      if ((E->parameter_1 & 0x8000) != 0)
        v4 = addr_kTorizo_Ilist_B962;
      else
        v4 = addr_kTorizo_Ilist_BDD8;
      E->current_instruction = v4;
      E->instruction_timer = 1;
    }
  }
}
void Torizo_Func_6(uint16 k) {  // 0xAAC828
  Torizo_C643(k);
}
void Torizo_Func_7(uint16 k) {  // 0xAAC82C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL8(E->ai_var_A));
  EnemyFunc_C8AD(k);
  if (Enemy_MoveDown(k, INT16_SHL8(E->ai_var_B))) {
    E->current_instruction = ERAM(0x7800 + E_k);
    E->instruction_timer = 1;
    E->ai_var_B = 256;
    earthquake_type = 4;
    earthquake_timer = 32;
  } else {
    E->ai_var_B += E->ai_var_C;
  }
}
void Torizo_Init(void) {  // 0xAAC87F
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (CheckBossBitForCurArea(4) & 1) {
    E->properties |= kEnemyProps_Deleted;
  } else {
    int v2 = area_index >> 1;
    E->properties |= ((int16)BW_AA(0xC96B + 2 * (v2)));
    E->extra_properties |= 4;
    E->x_width = ((int16)BW_AA(0xC96F + 2 * (v2)));
    E->y_height = ((int16)BW_AA(0xC973 + 2 * (v2)));
    E->ai_var_E = 0xC6BF /* Torizo_Func_3 */;
    E->instruction_timer = 1;
    E->timer = 0;
    E->palette_index = 0;
    E->ai_preinstr = addr_locret_AAC95E;
    E->current_instruction = ((uint16)BW_AA(0xC967 + 2 * (v2)));
    E->spritemap_pointer = addr_kTorizo_ExtSprmap_87D0;
    E->x_pos = ((int16)BW_AA(0xC95F + 2 * (v2)));
    E->y_pos = ((int16)BW_AA(0xC963 + 2 * (v2)));
    E->ai_var_A = 0;
    E->ai_var_B = 256;
    for (int i = 30; i >= 0; i -= 2) {
      int v4 = i >> 1;
      target_palettes[v4 + 240] = ((uint16)BW_AA(0x86A7 + 2 * (v4)));
      target_palettes[v4 + 176] = ((uint16)BW_AA(0x8687 + 2 * (v4)));
    }
    if (area_index) {
      Torizo_C280();
      if (joypad1_lastkeys == 0xC0C0) {
        samus_health = 700;
        samus_max_health = 700;
        samus_max_reserve_health = 300;
        samus_reserve_health = 300;
        samus_missiles = 100;
        samus_max_missiles = 100;
        samus_super_missiles = 20;
        samus_max_super_missiles = 20;
        samus_power_bombs = 20;
        samus_max_power_bombs = 20;
        equipped_items = -3273;
        collected_items = -3273;
        equipped_beams = 4111;
        collected_beams = 4111;
      }
    } else {
      Torizo_C250();
      SpawnBombTorizoHaze();
    }
  }
}
void GoldTorizo_Touch(void) {  // 0xAAC977
  NormalEnemyTouchAiSkipDeathAnim_CurEnemy();
}
void Torizo_Shot(void) {  // 0xAAC97C
  if (area_index) {
    GoldTorizo_Shot();
  } else {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    if (!(ERAM(0x7808 + E_k) | E->flash_timer)) {
      NormalEnemyShotAiSkipDeathAnim_CurEnemy();
      if (!E->health) {
        E->ai_var_E = 0xC6AB /* nullsub_270 */;
        E->current_instruction = addr_kTorizo_Ilist_B1C8;
        E->instruction_timer = 1;
        E->parameter_2 |= 0xC000;
        E->properties |= kEnemyProps_Tangible;
      }
    }
  }
}
void Torizo_Func_8(void) {  // 0xAAC9C2
  if (area_index)
    Torizo_D658();
}
uint16 Torizo_Instr_39(uint16 k, uint16 jp) {  // 0xAACACE
  if (gEnemyData(k)->y_pos == 375)
    return (uint16)(jp + 2);
  else
    return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_41(uint16 k, uint16 jp) {  // 0xAACADE
  Torizo_C298();
  return jp;
}
uint16 Torizo_Instr_42(uint16 k, uint16 jp) {  // 0xAACAE2
  QueueMusic_Delayed8(5);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_width = 18;
  E->y_height = 48;
  SpawnPalfxObject(addr_stru_8DF75D);
  return jp;
}
uint16 Torizo_Instr_48(uint16 k, uint16 jp) {  // 0xAACDD7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_2 &= ~0x1000;
  return jp;
}
uint16 Torizo_Instr_57(uint16 k, uint16 jp) {  // 0xAAD0E9
  SpawnEprojWithRoomGfx(addr_stru_86B1C0, 0);
  return jp;
}
uint16 Torizo_Instr_58(uint16 k, uint16 jp) {  // 0xAAD0F3
  int16 v2;

  v2 = 34;
  while (eproj_id[(uint16)((uint16)v2) >> 1] != 0xB1C0) {
    v2 -= 2;
    if (v2 < 0)
      return (uint16)(jp + 2);
  }
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_59(uint16 k, uint16 jp) {  // 0xAAD17B
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x780C + E_k) &= ~0x8000;
  return jp;
}
uint16 Torizo_Instr_62(uint16 k, uint16 jp) {  // 0xAAD187
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x780C + E_k) |= 0x8000;
  return jp;
}
uint16 Torizo_Instr_63(uint16 k, uint16 jp) {  // 0xAAD1E7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->parameter_2 &= ~0x2000;
  return jp;
}
uint16 Torizo_Instr_56(uint16 k, uint16 jp) {  // 0xAAD38F
  QueueSfx2_Max6(0x34);
  return jp;
}
uint16 Torizo_Instr_60(uint16 k, uint16 jp) {  // 0xAAD397
  QueueSfx2_Max6(0x67);
  return jp;
}
uint16 Torizo_Instr_46(uint16 k, uint16 jp) {  // 0xAAD39F
  QueueSfx2_Max6(0x48);
  return jp;
}
uint16 Torizo_Func_12(uint16 k) {  // 0xAAD3A7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  return E->parameter_1 ^ (E->x_pos - samus_x_pos);
}
void Torizo_Func_11(uint16 k) {  // 0xAAD3B2
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  SetGoldenTorizoPalette(E->health);
}
uint16 Torizo_Instr_47(uint16 k, uint16 jp) {  // 0xAAD3E0
  SpawnEprojWithRoomGfx(addr_kEproj_GoldenTorizoSuperMissile, 0);
  return jp;
}
uint16 Torizo_Instr_49(uint16 k, uint16 jp) {  // 0xAAD3EA
  if ((Torizo_Func_12(k) & 0x8000) == 0
      || !(CompareDistToSamus_X(k, 4) & 1)
      || CompareDistToSamus_X(k, 0x28) & 1
      || samus_pose != kPose_1D_FaceR_Morphball_Ground
      && samus_pose != kPose_1E_MoveR_Morphball_Ground
      && samus_pose != kPose_1F_MoveL_Morphball_Ground
      && samus_pose != kPose_79_FaceR_Springball_Ground
      && samus_pose != kPose_7A_FaceL_Springball_Ground
      && samus_pose != kPose_7B_MoveR_Springball_Ground
      && samus_pose != kPose_7C_MoveL_Springball_Ground) {
    return (uint16)(jp + 2);
  }
  ERAM(0x7812 + (k)) = 0;
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_61(uint16 k, uint16 jp) {  // 0xAAD436
  SpawnEprojWithRoomGfx(addr_kEproj_GoldenTorizoEyeBeam, IW_AA((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 Torizo_Instr_53(uint16 k, uint16 jp) {  // 0xAAD445
  if ((Torizo_Func_12(k) & 0x8000) != 0
      || !(CompareDistToSamus_X(k, 0x20) & 1)
      || CompareDistToSamus_X(k, 0x60) & 1
      || (NextRandom() & 0x110) != 0) {
    return (uint16)(jp + 2);
  }
  ERAM(0x7800 + (k)) = ((uint16)(jp + 2));
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_55(uint16 k, uint16 jp) {  // 0xAAD474
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->health > 0x788 || (NextRandom() & 0x102) != 0)
    return (uint16)((uint16)(jp + 2));
  ERAM(0x7812 + E_k) = 0;
  ERAM(0x7800 + E_k) = ((uint16)(jp + 2));
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_52(uint16 k, uint16 jp) {  // 0xAAD49B
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->health <= 0x2A30 || (E->parameter_2 & 0x2000) == 0)
    return (uint16)(jp + 2);
  ERAM(0x7800 + E_k) = ((uint16)(jp + 2));
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_50(uint16 k, uint16 jp) {  // 0xAAD4BA
  if (!(CompareDistToSamus_X(k, 0x70) & 1))
    return (uint16)(jp + 2);
  if ((Torizo_Func_12(k) & 0x8000) != 0)
    return (uint16)(jp + 2);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (ERAM(0x780E + E_k) <= 0x168 && ((joypad1_lastkeys & 0x300) == 0 || (NextRandom() & 0x101) == 0))
    return (uint16)(jp + 2);
  ERAM(0x7812 + E_k) = 0;
  Torizo_C20A(k);
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_43(uint16 k, uint16 jp) {  // 0xAAD4F3
  SpawnEprojWithGfx(0, k, addr_kEproj_GoldenTorizosChozoOrbs);
  return jp;
}
uint16 Torizo_Instr_51(uint16 k, uint16 jp) {  // 0xAAD4FD
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (ERAM(0x7812 + E_k) < 8 && (CompareDistToSamus_X(k, 0x20) & 1 || (Torizo_Func_12(k) & 0x8000) != 0))
    return (uint16)(jp + 2);
  ERAM(0x7812 + E_k) = 0;
  Torizo_C22D(k);
  return (uint16)(IW_AA((uint16)(jp + 0)));
}
uint16 Torizo_Instr_45(uint16 k, uint16 jp) {  // 0xAAD526
  ERAM(0x7800 + (k)) = ((uint16)(jp + 4));
  if (samus_missiles < 0x20
      || ((nmi_frame_counter_word + (samus_x_pos & 1) + (samus_x_pos >> 1)) & 8) != 0) {
    return (uint16)(IW_AA((uint16)(jp + 0)));
  } else {
    return (uint16)(IW_AA((uint16)(jp + 2)));
  }
}
uint16 Torizo_Instr_54(uint16 k, uint16 jp) {  // 0xAAD54D
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_A = ((int16)BW_AA(0xD59A + 2 * (IW_AA((uint16)(jp + 0)) >> 1)));
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL16(E->ai_var_A))) {
    ERAM(0x7806 + E_k) = 0;
    if ((E->parameter_1 & 0x8000) != 0)
      return (uint16)(addr_kTorizo_Ilist_D203);
    else
      return (uint16)(addr_kTorizo_Ilist_D2BF);
  } else {
    EnemyFunc_C8AD(k);
    if (((E->parameter_1 ^ (samus_x_pos - E->x_pos)) & 0x8000) == 0) {
      if (!ERAM(0x7806 + E_k))
        ERAM(0x7806 + E_k) = 16;
      return (uint16)(jp + 2);
    } else {
      return (uint16)(jp + 2);
    }
  }
}
void Torizo_Func_9(uint16 k) {  // 0xAAD5C2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (samus_y_pos > 0x140 && samus_x_pos > 0x170) {
    ++E->current_instruction;
    ++E->current_instruction;
    E->instruction_timer = 1;
  }
}
void Torizo_Func_10(uint16 k) {  // 0xAAD5DF
  Torizo_C620(k);
  Torizo_C643(k);
}
void Torizo_D5E6(uint16 k) {  // 0xAAD5E6
  Torizo_C620(k);
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  CallEnemyPreInstr(E->ai_preinstr | 0xAA0000);
}
void Torizo_D5ED(uint16 k) {  // 0xAAD5ED
  Torizo_C643(k);
}
void Torizo_D5F1(uint16 k) {  // 0xAAD5F1
  uint16 v3;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 torizo_var_03 = ERAM(0x7806 + E_k);
  if (!torizo_var_03 || (v3 = torizo_var_03 - 1, (ERAM(0x7806 + E_k) = v3) != 0)) {
    uint16 v5 = abs16(E->ai_var_A) + 1;
    if (v5 >= 0x10)
      v5 = 15;
    if (!(Enemy_MoveDown(k, INT16_SHL16(v5)))) {
      uint16 v6;
      if ((E->parameter_1 & 0x8000) != 0)
        v6 = addr_off_AAC0F2;
      else
        v6 = addr_off_AABC78;
      E->current_instruction = v6;
      E->instruction_timer = 1;
      E->ai_var_B = 256;
      E->ai_var_A = 0;
    }
  } else {
    ERAM(0x7806 + E_k) = 0;
    uint16 v4;
    if ((E->parameter_1 & 0x8000) != 0)
      v4 = addr_kTorizo_Ilist_D203;
    else
      v4 = addr_kTorizo_Ilist_D2BF;
    E->current_instruction = v4;
    E->instruction_timer = 1;
  }
}
void Torizo_D658(void) {  // 0xAAD658
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->flash_timer && !ERAM(0x7808 + E_k))
    Torizo_D6A6();
}
void GoldTorizo_Shot(void) {  // 0xAAD667
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->flash_timer) {
    if (ERAM(0x7808 + E_k)) {
      Torizo_D6A6();
      return;
    }
    if ((E->parameter_2 & 0x1000) != 0)
      goto LABEL_11;
    uint16 v2, v3;
    v2 = 2 * collision_detection_index;
    v3 = projectile_type[collision_detection_index] & 0xF00;
    ERAM(0x780A + E_k) = v3;
    if (v3 == 256) {
      Torizo_D6D1(cur_enemy_index, v2);
      return;
    }
    if (v3 != 512) {
LABEL_11:
      E->parameter_2 |= 0x2000;
      Torizo_D6A6();
    } else {
      Torizo_D6F7(cur_enemy_index, v2);
    }
  }
}
void Torizo_D6A6(void) {  // 0xAAD6A6
  NormalEnemyShotAiSkipDeathAnim_CurEnemy();
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (!E->health) {
    E->current_instruction = addr_kTorizo_Ilist_B1C8;
    E->instruction_timer = 1;
    E->parameter_2 |= 0xC000;
    E->properties |= kEnemyProps_Tangible;
  }
}
void Torizo_D6D1(uint16 k, uint16 j) {  // 0xAAD6D1
  projectile_dir[j >> 1] &= ~0x10;
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->ai_var_E = 0xC6AB /* nullsub_270 */;
  E->instruction_timer = 1;
  uint16 v3;
  if ((E->parameter_1 & 0x8000) != 0)
    v3 = addr_kTorizo_Ilist_D2AD;
  else
    v3 = addr_kTorizo_Ilist_D1F1;
  E->current_instruction = v3;
}
void Torizo_D6F7(uint16 k, uint16 j) {  // 0xAAD6F7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);

  if ((Torizo_Func_12(k) & 0x8000) == 0) {
    E->parameter_2 |= 0x1000;
    E->ai_var_E = 0xC6AB /* nullsub_270 */;
    projectile_dir[j >> 1] |= 0x10;
    E->instruction_timer = 1;
    uint16 v3;
    if ((E->parameter_1 & 0x2000) != 0) {
      if ((E->parameter_1 & 0x8000) != 0)
        v3 = addr_kTorizo_Ilist_CEFF;
      else
        v3 = addr_kTorizo_Ilist_CE43;
    } else if ((E->parameter_1 & 0x8000) != 0) {
      v3 = addr_kTorizo_Ilist_CEA5;
    } else {
      v3 = addr_kTorizo_Ilist_CDE1;
    }
    E->current_instruction = v3;
  } else {
    Torizo_D6A6();
  }
}
void TourianEntranceStatue_Init(void) {  // 0xAAD7C8
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  v0->palette_index = 0;
  v0->instruction_timer = 1;
  v0->timer = 0;
  uint16 v1 = ((uint16)BW_AA(0xD810 + 2 * (v0->parameter_1 >> 1)));
  v0->current_instruction = v1;
  if (!v0->parameter_1) {
    SpawnEprojWithRoomGfx(addr_kEproj_TourianStatueBaseDecoration, 0);
    SpawnEprojWithRoomGfx(addr_kEproj_TourianStatueRidley, 0);
    SpawnEprojWithRoomGfx(addr_kEproj_TourianStatuePhantoon, 0);
  }
  for (int i = 30; i >= 0; i -= 2) {
    int v5 = i >> 1;
    target_palettes[v5 + 240] = ((uint16)BW_AA(0xD785 + 2 * (v5)));
    target_palettes[v5 + 160] = ((uint16)BW_AA(0xD765 + 2 * (v5)));
  }
}
void ChozoStatue_Init(void) {  // 0xAAE725
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  v0->properties |= kEnemyProps_DisableSamusColl | kEnemyProps_ProcessedOffscreen | 0x8000;
  v0->spritemap_pointer = addr_kSpritemap_Nothing_AA;
  v0->instruction_timer = 1;
  v0->timer = 0;
  v0->ai_preinstr = 0xE7A6 /* nullsub_276 */;
  v0->parameter_1 = 0;
  v0->palette_index = 0;
  gEnemyData(0)->layer = 0;
  uint16 parameter_2 = v0->parameter_2;
  v0->current_instruction = ((uint16)BW_AA(0xE7A2 + 2 * (parameter_2 >> 1)));
  if (parameter_2) {
    sub_AAE784();
  } else {
    for (int i = 30; i >= 0; i -= 2) {
      int v3 = i >> 1;
      target_palettes[v3 + 160] = ((int16)BW_AA(0xE33D + 2 * (v3)));
      target_palettes[v3 + 144] = ((uint16)BW_AA(0xE31D + 2 * (v3)));
    }
    SpawnHardcodedPlm(0x4a, 0x17, 0xd6ee);
    SpawnHardcodedPlm(0x17, 0x1d, 0xd6fc);
  }
}
void sub_AAE784(void) {  // 0xAAE784
  for (int i = 30; i >= 0; i -= 2) {
    int v1 = i >> 1;
    target_palettes[v1 + 160] = ((int16)BW_AA(0xE37D + 2 * (v1)));
    target_palettes[v1 + 144] = ((int16)BW_AA(0xE35D + 2 * (v1)));
  }
  SpawnHardcodedPlm(0x0c, 0x1d, 0xd6d6);
}
void ChozoStatue_Main(void) {  // 0xAAE7A7
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  CallEnemyPreInstr(v0->ai_preinstr | 0xAA0000);
}
void Enemy_NormalFrozenAI_B2(void) {  // 0xB28041
  NormalEnemyFrozenAI();
}
void WalkingSpacePirates_Powerbomb(void) {  // 0xB28767
  NormalEnemyPowerBombAi();
}
void WalkingSpacePirates_Touch(void) {  // 0xB2876C
  if (!gEnemyData(cur_enemy_index)->frozen_timer)
    NormalEnemyTouchAi();
}
void WalkingSpacePirates_Shot(void) {  // 0xB28779
  if (gEnemyData(cur_enemy_index)->enemy_ptr == addr_kEnemyDef_F593)
    WalkingSpacePirates_8789();
  else
    NormalEnemyShotAi();
}
void WalkingSpacePirates_8789(void) {  // 0xB28789
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  special_death_item_drop_x_origin_pos = E->x_pos;
  special_death_item_drop_y_origin_pos = E->y_pos;
  NormalEnemyShotAiSkipDeathAnim_CurEnemy();
  if (!E->health) {
    uint16 v1 = cur_enemy_index;
    if (E->enemy_ptr == addr_kEnemyDef_F593) {
      E->ai_var_B = 0;
      EnemyDeathAnimation(v1, 4);
      Enemy_ItemDrop_LowerNorfairSpacePirate(v1);
    } else {
      E->ai_var_B = 0;
      EnemyDeathAnimation(v1, 4);
    }
  }
}
void WalkingSpacePirates_87C8(void) {  // 0xB287C8
  Vulnerability *v2;
  int16 v3;
  Vulnerability *v;

  if (gEnemyData(cur_enemy_index)->enemy_ptr != addr_kEnemyDef_F593)
    goto LABEL_2;
  uint16 r18 = projectile_type[collision_detection_index];
  if (sign16((r18 & kProjectileType_TypeMask) - kProjectileType_PowerBomb)) {
    uint16 enemy_ptr;
    uint16 vulnerability_ptr;
    enemy_ptr = gEnemyData(cur_enemy_index)->enemy_ptr;
    vulnerability_ptr = get_EnemyDef_A2(enemy_ptr)->vulnerability_ptr;
    if (!vulnerability_ptr)
      vulnerability_ptr = addr_stru_B4EC1C;
    uint16 r20 = vulnerability_ptr;
    if ((r18 & kProjectileType_TypeMask) != 0) {
      if ((r18 & kProjectileType_TypeMask) != kProjectileType_Missile && (r18 & kProjectileType_TypeMask) != kProjectileType_SuperMissile)
        goto LABEL_2;
      v3 = (uint16)(r18 & kProjectileType_TypeMask) >> 8;
      v = get_Vulnerability(r20 + v3);
      if ((v->plasma_ice_wave & 0xF) != 0 && (v->plasma_ice_wave & 0xF) != 15)
        goto LABEL_2;
    } else {
      v2 = get_Vulnerability(r20 + (r18 & 0xF));
      if ((v2->power & 0xF) != 0 && (v2->power & 0xF) != 15) {
LABEL_2:
        WalkingSpacePirates_8789();
        return;
      }
    }
    WalkingSpacePirates_883E();
  }
}
void WalkingSpacePirates_883E(void) {  // 0xB2883E
  if (gEnemyData(cur_enemy_index)->enemy_ptr != addr_kEnemyDef_F593) {
    WalkingSpacePirates_8789();
    return;
  }
  uint16 v0 = 2 * collision_detection_index;
  int v1 = collision_detection_index;
  uint16 r18 = projectile_type[v1];
  if ((r18 & kProjectileType_TypeMask) == kProjectileType_SuperMissile) {
    if (!projectile_variables[v1])
      return;
  } else if (!sign16((r18 & kProjectileType_TypeMask) - kProjectileType_PowerBomb)) {
    return;
  }
  gEnemyData(cur_enemy_index)->invincibility_timer = 10;
  uint16 v2;
  if ((projectile_dir[v1] & 0xF) == 7) {
    v2 = 1;
  } else if ((projectile_dir[v1] & 0xF) == 2) {
    v2 = 8;
  } else {
    v2 = 5;
  }
  projectile_dir[v1] = v2;
  ProjectileReflection(v0);
  QueueSfx2_Max6(0x66);
}
uint16 SpacePirates_Instr_MovePixelsDownAndChangeDirFaceRight(uint16 k, uint16 jp) {  // 0xB2EE40
  if (!(Enemy_MoveDown(cur_enemy_index, INT16_SHL16(IW_B2((uint16)(jp + 0))))))
    return (uint16)(jp + 2);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C ^= 1;
  if (E->ai_var_C)
    return (uint16)(addr_off_B2ECEC);
  return (uint16)(addr_kSpacePirates_Ilist_ED36);
}
uint16 SpacePirates_Instr_MovePixelsDownAndChangeDirFaceLeft(uint16 k, uint16 jp) {  // 0xB2EE72
  if (!(Enemy_MoveDown(cur_enemy_index, INT16_SHL16(IW_B2((uint16)(jp + 0))))))
    return (uint16)(jp + 2);
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_C ^= 1;
  if (E->ai_var_C)
    return (uint16)(addr_kSpacePirates_Ilist_EDF6);
  return (uint16)(addr_off_B2EDAC);
}
uint16 SpacePirates_Instr_RandomNewDirFaceR(uint16 k, uint16 jp) {  // 0xB2EEA4
  uint16 result = addr_kSpacePirates_Ilist_ED36;
  uint16 v4 = NextRandom() & 1;
  gEnemyData(cur_enemy_index)->ai_var_C = v4;
  if (v4)
    return (uint16)(addr_off_B2ECEC);
  return (uint16)(result);
}
uint16 SpacePirates_Instr_RandomNewDirFaceL(uint16 k, uint16 jp) {  // 0xB2EEBC
  uint16 result = addr_off_B2EDAC;
  uint16 v4 = NextRandom() & 1;
  gEnemyData(cur_enemy_index)->ai_var_C = v4;
  if (v4)
    return (uint16)(addr_kSpacePirates_Ilist_EDF6);
  return (uint16)(result);
}
uint16 SpacePirates_Instr_PrepareWallJumpR(uint16 k, uint16 jp) {  // 0xB2EED4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = E->x_pos + E->parameter_2;
  E->ai_var_D = E->x_pos + (E->parameter_2 >> 1);
  E->ai_var_E = E->y_pos;
  E->ai_preinstr = 64;
  return jp;
}
uint16 SpacePirates_Instr_PrepareWallJumpL(uint16 k, uint16 jp) {  // 0xB2EEFD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->ai_var_B = E->x_pos - E->parameter_2;
  E->ai_var_D = E->x_pos - (E->parameter_2 >> 1);
  E->ai_var_E = E->y_pos;
  E->ai_preinstr = 192;
  return jp;
}
uint16 SpacePirates_Instr_FireLaserL(uint16 k, uint16 jp) {  // 0xB2EF2A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_init_param_1 = BW_A0((uint16)((E->enemy_ptr) + 6));
  eproj_spawn_x = E->x_pos - 24; eproj_spawn_y = E->y_pos - 16;
  eproj_spawn_r22 = 0;
  SpawnEprojWithRoomGfx(addr_stru_86A17B, 0);
  return jp;
}
uint16 SpacePirates_Instr_FireLaserR(uint16 k, uint16 jp) {  // 0xB2EF5D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_spawn_x = E->x_pos + 24; eproj_spawn_y = E->y_pos - 16;
  eproj_spawn_r22 = 1;
  SpawnEprojWithRoomGfx(addr_stru_86A17B, 1);
  return jp;
}
uint16 SpacePirates_Instr_SetEnemyFunc(uint16 k, uint16 jp) {  // 0xB2EF83
  gEnemyData(cur_enemy_index)->ai_var_A = IW_B2((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_PlaySfx(uint16 k, uint16 jp) {  // 0xB2EF93
  QueueSfx2_Max6(0x66);
  return jp;
}
void WallSpacePirates_Init(void) {  // 0xB2EF9F
  uint16 v0 = addr_kSpacePirates_Ilist_ED36;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((E->parameter_1 & 1) != 0)
    v0 = addr_off_B2EDAC;
  E->current_instruction = v0;
  ERAM(0x8000 + E_k) = 190;
  ERAM(0x8002 + E_k) = 66;
  ERAM(0x8004 + E_k) = 2;
  if ((E->parameter_1 & 0x8000) == 0) {
    ERAM(0x8000 + E_k) += 2;
    ERAM(0x8002 + E_k) -= 2;
    ERAM(0x8004 + E_k) += 2;
  }
  uint16 v2 = 0xF034 /* SpacePirates_Func_1 */;
  if ((E->parameter_1 & 1) != 0)
    v2 = 0xF0C8 /* SpacePirates_Func_3 */;
  E->ai_var_A = v2;
  uint16 v3;
  if (sign16((E->x_pos & 0xF) - 11))
    v3 = E->x_pos & 0xFFF8;
  else
    v3 = (E->x_pos & 0xFFF0) + 16;
  E->x_pos = v3;
}
void CallSpacePiratesEnemyFunc(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xB2804B: return;  // 0xb2804b
  case 0xB2F034: SpacePirates_Func_1(k); return;  // 0xb2f034
  case 0xB2F04F: return;  // 0xb2f04f
  case 0xB2F050: SpacePirates_Func_2(k); return;  // 0xb2f050
  case 0xB2F0C8: SpacePirates_Func_3(k); return;  // 0xb2f0c8
  case 0xB2F0E3: return;  // 0xb2f0e3
  case 0xB2FE4A: return;
  case 0xB2F0E4: SpacePirates_Func_4(k); return;  // 0xb2f0e4
  case 0xB2F6A9: SpacePirates_Func_5(k); return;  // 0xb2f6a9
  case 0xB2F6E4: SpacePirates_Func_6(k); return;  // 0xb2f6e4
  case 0xB2F817: SpacePirates_Func_11(k); return;  // 0xb2f817
  case 0xB2F84C: SpacePirates_Func_12(k); return;
  case 0xB2F890: SpacePirates_Func_13(k); return;  // 0xb2f890
  case 0xB2F8C5: SpacePirates_Func_14(k); return;
  case 0xB2F909: SpacePirates_Func_15(k); return;  // 0xb2f909
  case 0xB2F985: SpacePirates_F985(k); return;  // 0xb2f985
  case 0xB2F9C1: SpacePirates_F9C1(k); return;  // 0xb2f9c1
  case 0xB2FA15: SpacePirates_FA15(k); return;  // 0xb2fa15
  case 0xB2FA59: SpacePirates_FA59(k); return;  // 0xb2fa59
  case 0xB2FA95: SpacePirates_FA95(k); return;  // 0xb2fa95
  case 0xB2FAE9: SpacePirates_FAE9(k); return;  // 0xb2fae9
  case 0xB2FD44: WalkingSpacePirates_FD44(); return;
  case 0xB2FDCE: WalkingSpacePirates_FDCE(); return;
  default: nt_unimpl("Unreachable");
  }
}
void WallSpacePirates_Main(void) {  // 0xB2F02D
  EnemyData *v0 = gEnemyData(cur_enemy_index);
  CallSpacePiratesEnemyFunc(v0->ai_var_A | 0xB20000, cur_enemy_index);
}
void SpacePirates_Func_1(uint16 k) {  // 0xB2F034
  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x20)) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    E->current_instruction = addr_kSpacePirates_Ilist_ED80;
    E->instruction_timer = 1;
  }
}
void SpacePirates_Func_2(uint16 k) {  // 0xB2F050
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->x_pos = E->ai_var_D + SineMult8bit(E->ai_preinstr, E->parameter_2 >> 1);
  E->y_pos = E->ai_var_E - CosineMult8bit(E->ai_preinstr, E->parameter_2 >> 2);
  uint16 v1 = (uint8)(LOBYTE(E->ai_preinstr) - LOBYTE(ERAM(0x8004 + E_k)));
  E->ai_preinstr = v1;
  if (v1 == ERAM(0x8000 + E_k)) {
    E->current_instruction = addr_kSpacePirates_Ilist_EDA4;
    E->instruction_timer = 1;
    uint16 v2;
    if (sign16((E->x_pos & 0xF) - 11))
      v2 = E->x_pos & 0xFFF8;
    else
      v2 = (E->x_pos & 0xFFF0) + 16;
    E->x_pos = v2;
  }
}
void SpacePirates_Func_3(uint16 k) {  // 0xB2F0C8
  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x20)) {
    EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
    E->current_instruction = addr_kSpacePirates_Ilist_ECC0;
    E->instruction_timer = 1;
  }
}
void SpacePirates_Func_4(uint16 k) {  // 0xB2F0E4
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->x_pos = E->ai_var_D + SineMult8bit(E->ai_preinstr, E->parameter_2 >> 1);
  E->y_pos = E->ai_var_E - CosineMult8bit(E->ai_preinstr, E->parameter_2 >> 2);
  uint16 v1 = (uint8)(LOBYTE(ERAM(0x8004 + E_k)) + LOBYTE(E->ai_preinstr));
  E->ai_preinstr = v1;
  if (v1 == ERAM(0x8002 + E_k)) {
    E->current_instruction = addr_kSpacePirates_Ilist_ECE4;
    E->instruction_timer = 1;
    uint16 v2;
    if (sign16((E->x_pos & 0xF) - 11))
      v2 = E->x_pos & 0xFFF8;
    else
      v2 = (E->x_pos & 0xFFF0) + 16;
    E->x_pos = v2;
  }
}
uint16 SpacePirates_Instr_20(uint16 k, uint16 jp) {  // 0xB2F536
  gEnemyData(cur_enemy_index)->palette_index = IW_B2((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_16(uint16 k, uint16 jp) {  // 0xB2F546
  QueueSfx2_Max6(IW_B2((uint16)(jp + 0)));
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_15(uint16 k, uint16 jp) {  // 0xB2F564
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_spawn_rect = (Rect16U){ E->x_pos, E->y_pos, IW_B2((uint16)(jp + 2)), IW_B2((uint16)(jp + 4)) };
  SpawnEprojWithGfx(IW_B2((uint16)(jp + 0)), cur_enemy_index, addr_kEproj_PirateClaw);
  return (uint16)(jp + 6);
}
uint16 SpacePirates_Instr_18(uint16 k, uint16 jp) {  // 0xB2F590
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->instruction_timer = 1;
  E->ai_var_C = (((E->x_pos - samus_x_pos) & 0x8000) != 0) ? addr_kSpacePirates_Ilist_F420 : addr_kSpacePirates_Ilist_F22E;
  return (uint16)(E->ai_var_C);
}
uint16 SpacePirates_Instr_17(uint16 k, uint16 jp) {  // 0xB2F5D6
  ERAM(0x7800 + (k)) = 0;
  return jp;
}
void NinjaSpacePirates_Init(void) {  // 0xB2F5DE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->current_instruction = (E->parameter_1 & 1) != 0 ? addr_kSpacePirates_Ilist_F4CC : addr_kSpacePirates_Ilist_F2DA;
  E->ai_var_C = E->current_instruction;
  uint16 x_pos = E->x_pos;
  if ((E->parameter_1 & 1) != 0) {
    E->ai_var_E = x_pos;
    E->ai_preinstr = E->parameter_2 + x_pos;
  } else {
    E->ai_preinstr = x_pos;
    E->ai_var_E = x_pos - E->parameter_2;
  }
  uint16 r20 = (uint16)(E->ai_preinstr - E->ai_var_E) >> 1;
  E->ai_var_D = E->ai_var_E + r20;
  uint16 r18 = 0;
  uint16 R22 = 0;
  r20 = (uint8)r20 << 8;
  do {
    r18 += 32;
    R22 += r18;
  } while (sign16(R22 - r20));
  E->ai_var_B = r18;
  uint16 R24 = (R22 & 0xFF00) >> 8;
  E->ai_preinstr = E->ai_var_D + R24;
  E->ai_var_E = E->ai_var_D - R24;
  uint16 sps_var_E = E->ai_var_E;
  if ((E->parameter_1 & 1) == 0)
    sps_var_E = E->ai_preinstr;
  E->x_pos = sps_var_E;
  E->ai_var_A = 0x804B /* nullsub_169_B2 */;
  ERAM(0x7810 + E_k) = E->y_pos;
  uint16 v6 = 0;
  uint16 v7 = 0;
  int n = 15;
  do {
    target_palettes[(v7 >> 1) + 240] = ((uint16)BW_B2(0x8727 + 2 * (v6 >> 1)));
    v6 += 2;
    v7 += 2;
  } while (--n >= 0);
}
void NinjaSpacePirates_Main(void) {  // 0xB2F6A2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallSpacePiratesEnemyFunc(E->ai_var_A | 0xB20000, cur_enemy_index);
}
void SpacePirates_Func_5(uint16 k) {  // 0xB2F6A9
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(E->x_pos - samus_x_pos) - 128) >= 0) {
    SpacePirates_Func_8();
  } else {
    E->current_instruction = ((E->x_pos - samus_x_pos) & 0x8000) ? addr_kSpacePirates_Ilist_F420 : addr_kSpacePirates_Ilist_F22E;
    E->ai_var_C = E->current_instruction;
    E->instruction_timer = 1;
  }
}
void SpacePirates_Func_6(uint16 k) {  // 0xB2F6E4
  if (!SpacePirates_Func_8() && !SpacePirates_Func_10() && !SpacePirates_Func_9())
    SpacePirates_Func_7(k);
}
void SpacePirates_Func_7(uint16 k) {  // 0xB2F6F7
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((E->frame_counter & 0x3F) == 0) {
    if (E->x_pos == E->ai_var_E) {
      if ((int16)(E->x_pos - samus_x_pos) < 0)
        return;
      E->current_instruction = addr_kSpacePirates_Ilist_F15C;
    } else {
      if ((int16)(E->x_pos - samus_x_pos) >= 0)
        return;
      E->current_instruction = addr_kSpacePirates_Ilist_F34A;
    }
    E->instruction_timer = 1;
  }
}
uint16 SpacePirates_Func_8(void) {  // 0xB2F72E
  uint16 v0 = 8;
  while (!projectile_type[v0 >> 1]) {
    v0 -= 2;
    if ((v0 & 0x8000) != 0)
      return 0;
  }
  int v1 = v0 >> 1;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(projectile_x_pos[v1] - E->x_pos) - 32) >= 0
      || (int16)(abs16(projectile_y_pos[v1] - E->y_pos) - 32) >= 0) {
    return 0;
  }
  E->current_instruction = (E->x_pos - samus_x_pos & 0x8000) ? addr_kSpacePirates_Ilist_F462 : addr_kSpacePirates_Ilist_F270;
  E->instruction_timer = 1;
  return 1;
}
uint16 SpacePirates_Func_9(void) {  // 0xB2F78D
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(E->ai_var_D - samus_x_pos) - 32) >= 0)
    return 0;
  uint16 v1 = addr_kSpacePirates_Ilist_F1C4;
  if (E->x_pos == E->ai_var_E)
    v1 = addr_kSpacePirates_Ilist_F3B2;
  E->current_instruction = v1;
  E->instruction_timer = 1;
  return 1;
}
uint16 SpacePirates_Func_10(void) {  // 0xB2F7C6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(samus_x_pos - E->x_pos) - 40) >= 0
      || (int16)(abs16(samus_y_pos - E->y_pos) - 40) >= 0) {
    return 0;
  }
  E->current_instruction = ((E->x_pos - samus_x_pos) & 0x8000) != 0 ? addr_kSpacePirates_Ilist_F51A : addr_kSpacePirates_Ilist_F32E;
  E->instruction_timer = 1;
  return 1;
}
void SpacePirates_Func_11(uint16 k) {  // 0xB2F817
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos -= HIBYTE(ERAM(0x7800 + E_k));
  --E->y_pos;
  --E->y_pos;
  ERAM(0x7800 + E_k) += 32;
  if ((int16)(E->x_pos - E->ai_var_D) < 0)
    E->ai_var_A = 0xF84C /* SpacePirates_Func_12 */;
}
void SpacePirates_Func_12(uint16 k) {  // 0xB2F84C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos -= HIBYTE(ERAM(0x7800 + E_k));
  ++E->y_pos;
  ++E->y_pos;
  uint16 v2 = ERAM(0x7800 + E_k) - 32;
  ERAM(0x7800 + E_k) = v2;
  if (!v2) {
    E->ai_var_A = 0x804B /* nullsub_169_B2 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F2F8;
    E->instruction_timer = 1;
    E->x_pos = E->ai_var_E;
    SpacePirates_FB11(k);
  }
}
void SpacePirates_Func_13(uint16 k) {  // 0xB2F890
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos += HIBYTE(ERAM(0x7800 + E_k));
  --E->y_pos;
  --E->y_pos;
  ERAM(0x7800 + E_k) += 32;
  if ((int16)(E->x_pos - E->ai_var_D) >= 0)
    E->ai_var_A = 0xF8C5 /* SpacePirates_Func_14 */;
}
void SpacePirates_Func_14(uint16 k) {  // 0xB2F8C5
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos += HIBYTE(ERAM(0x7800 + E_k));
  ++E->y_pos;
  ++E->y_pos;
  uint16 v2 = ERAM(0x7800 + E_k) - 32;
  ERAM(0x7800 + E_k) = v2;
  if (!v2) {
    E->ai_var_A = 0x804B /* nullsub_169_B2 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F4EA;
    E->instruction_timer = 1;
    E->x_pos = E->ai_preinstr;
    SpacePirates_FB11(k);
  }
}
void SpacePirates_Func_15(uint16 k) {  // 0xB2F909
  if (!SpacePirates_Func_8() && !SpacePirates_Func_10())
    SpacePirates_Func_16();
}
void SpacePirates_Func_16(void) {  // 0xB2F917
  int16 v3;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(E->ai_var_D - samus_x_pos) - 32) < 0) {
    uint16 v2;
    do
      v2 = NextRandom() & 3;
    while (!v2);
    v3 = 0;
    if (E->x_pos == E->ai_var_E)
      v3 = 4;
    E->current_instruction = ((uint16)BW_B2(0xF959 + 2 * (v2 + v3)));
    E->instruction_timer = 1;
  }
}
uint16 SpacePirates_Instr_19(uint16 k, uint16 jp) {  // 0xB2F969
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = 1536;
  ERAM(0x7806 + E_k) = E->ai_var_D
    + ((uint16)(E->ai_preinstr - E->ai_var_D) >> 1);
  return jp;
}
void SpacePirates_F985(uint16 k) {  // 0xB2F985
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->y_pos -= HIBYTE(ERAM(0x7800 + E_k));
  v2 = ERAM(0x7800 + E_k) - 64;
  ERAM(0x7800 + E_k) = v2;
  if (v2 < 0) {
    E->ai_var_A = 0xF9C1 /* SpacePirates_F9C1 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F2A0;
    E->instruction_timer = 1;
    ERAM(0x7800 + E_k) = 1536;
  }
}
void SpacePirates_F9C1(uint16 k) {  // 0xB2F9C1
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos -= 5;
  if (Enemy_MoveDown(k, __PAIR32__(HIBYTE(ERAM(0x7800 + E_k)), LOBYTE(ERAM(0x7800 + E_k))))
      || (v2 = ERAM(0x7800 + E_k) - 64, ERAM(0x7800 + E_k) = v2, v2 < 0)
      || (v2 & 0xFF00) == 0) {
    E->ai_var_A = 0xFA15 /* SpacePirates_FA15 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F2B2;
    E->instruction_timer = 1;
    E->y_pos = ERAM(0x7810 + E_k);
    SpacePirates_FB11(k);
  }
}
void SpacePirates_FA15(uint16 k) {  // 0xB2FA15
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v2 = E->x_pos - 2;
  E->x_pos = v2;
  if ((int16)(v2 - E->ai_var_E) < 0) {
    E->x_pos = E->ai_var_E;
    E->current_instruction = addr_kSpacePirates_Ilist_F2F8;
    E->instruction_timer = 1;
    E->ai_var_A = 0x804B /* nullsub_169_B2 */;
  }
}
uint16 SpacePirates_Instr_21(uint16 k, uint16 jp) {  // 0xB2FA3D
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ERAM(0x7800 + E_k) = 1536;
  ERAM(0x7806 + E_k) = E->ai_var_E
    + ((uint16)(E->ai_var_D - E->ai_var_E) >> 1);
  return jp;
}
void SpacePirates_FA59(uint16 k) {  // 0xB2FA59
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->y_pos -= HIBYTE(ERAM(0x7800 + E_k));
  v2 = ERAM(0x7800 + E_k) - 64;
  ERAM(0x7800 + E_k) = v2;
  if (v2 < 0) {
    E->ai_var_A = 0xFA95 /* SpacePirates_FA95 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F492;
    E->instruction_timer = 1;
    ERAM(0x7800 + E_k) = 1536;
  }
}
void SpacePirates_FA95(uint16 k) {  // 0xB2FA95
  int16 v2;

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos += 5;
  if (Enemy_MoveDown(k, __PAIR32__(HIBYTE(ERAM(0x7800 + E_k)), LOBYTE(ERAM(0x7800 + E_k))))
      || (v2 = ERAM(0x7800 + E_k) - 64, ERAM(0x7800 + E_k) = v2, v2 < 0)
      || (v2 & 0xFF00) == 0) {
    E->ai_var_A = 0xFAE9 /* SpacePirates_FAE9 */;
    E->current_instruction = addr_kSpacePirates_Ilist_F4A4;
    E->instruction_timer = 1;
    E->y_pos = ERAM(0x7810 + E_k);
    SpacePirates_FB11(k);
  }
}
void SpacePirates_FAE9(uint16 k) {  // 0xB2FAE9
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v2 = E->x_pos + 2;
  E->x_pos = v2;
  if ((int16)(v2 - E->ai_preinstr) >= 0) {
    E->x_pos = E->ai_preinstr;
    E->current_instruction = addr_kSpacePirates_Ilist_F4EA;
    E->instruction_timer = 1;
    E->ai_var_A = 0x804B /* nullsub_169_B2 */;
  }
}
void SpacePirates_FB11(uint16 k) {  // 0xB2FB11
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  CreateSpriteAtPos(E->x_pos - 8, E->y_pos + 28, 10, 0);
  CreateSpriteAtPos(E->x_pos + 8, E->y_pos + 28, 10, 0);
}
uint16 SpacePirates_Instr_12(uint16 k, uint16 jp) {  // 0xB2FC68
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_spawn_x = E->x_pos - 24; eproj_spawn_y = E->y_pos - IW_B2((uint16)(jp + 0));
  eproj_spawn_r22 = 0;
  SpawnEprojWithRoomGfx(addr_stru_86A17B, 0);
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_14(uint16 k, uint16 jp) {  // 0xB2FC90
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  eproj_spawn_x = E->x_pos + 24; eproj_spawn_y = E->y_pos - IW_B2((uint16)(jp + 0));
  eproj_spawn_r22 = 1;
  SpawnEprojWithRoomGfx(addr_stru_86A17B, 1);
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_11(uint16 k, uint16 jp) {  // 0xB2FCB8
  gEnemyData(cur_enemy_index)->ai_var_A = IW_B2((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 SpacePirates_Instr_13(uint16 k, uint16 jp) {  // 0xB2FCC8
  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x10)) {
    if ((int16)(samus_x_pos - gEnemyData(cur_enemy_index)->x_pos) >= 0)
      return (uint16)(addr_kSpacePirates_Ilist_FC0E);
    return (uint16)(addr_kSpacePirates_Ilist_FB8C);
  } else {
    if ((int16)(samus_x_pos - gEnemyData(cur_enemy_index)->x_pos) >= 0)
      return (uint16)(addr_kSpacePirates_Ilist_FB64);
    return (uint16)(addr_kSpacePirates_Ilist_FBE6);
  }
}
void WalkingSpacePirates_Init(void) {  // 0xB2FD02
  uint16 v0 = addr_kSpacePirates_Ilist_FB64;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((E->parameter_1 & 1) != 0)
    v0 = addr_kSpacePirates_Ilist_FBE6;
  E->current_instruction = v0;
  E->ai_var_A = 0x804B /* nullsub_169_B2 */;
  E->ai_preinstr = E->parameter_2 + E->x_pos;
  E->ai_var_E = E->x_pos - E->parameter_2;
}
void WalkingSpacePirates_Main(void) {  // 0xB2FD32
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallSpacePiratesEnemyFunc(E->ai_var_A | 0xB20000, cur_enemy_index);
  if ((E->parameter_1 & 0x8000) != 0)
    WalkingSpacePirates_FE4B();
}
void WalkingSpacePirates_FD44(void) {  // 0xB2FD44
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);

  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x10)) {
    uint16 v1 = addr_kSpacePirates_Ilist_FB8C;
    if ((int16)(samus_x_pos - E->x_pos) >= 0)
      v1 = addr_kSpacePirates_Ilist_FC0E;
    E->current_instruction = v1;
    E->instruction_timer = 1;
  } else {
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1))) {
      uint16 x_pos = E->x_pos;
      ERAM(0x7800 + E_k) = x_pos;
      E->x_pos = x_pos - 17;
      uint8 result = Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1));
      E->x_pos = ERAM(0x7800 + E_k);

      if (result) {
        EnemyFunc_BBBF(cur_enemy_index, INT16_SHL16(-9));
        if (!Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, -14337)) {
          if ((int16)(E->x_pos - E->ai_var_E) >= 0)
            return;
        }
      }
      E->current_instruction = addr_kSpacePirates_Ilist_FBC6;
      E->instruction_timer = 1;
    }
  }
}
void WalkingSpacePirates_FDCE(void) {  // 0xB2FDCE
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);

  if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x10)) {
    uint16 v1 = addr_kSpacePirates_Ilist_FB8C;
    if ((int16)(samus_x_pos - E->x_pos) >= 0)
      v1 = addr_kSpacePirates_Ilist_FC0E;
    E->current_instruction = v1;
    E->instruction_timer = 1;
  } else {
    if (Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1))) {
      uint16 x_pos = E->x_pos;
      ERAM(0x7800 + E_k) = x_pos;
      E->x_pos = x_pos + 16;
      uint8 result = Enemy_MoveDown(cur_enemy_index, INT16_SHL16(1));
      E->x_pos = ERAM(0x7800 + E_k);
      if (!result ||
          (Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, 14336)) ||
          (int16)(E->x_pos - E->ai_preinstr) >= 0) {
        E->current_instruction = addr_kSpacePirates_Ilist_FC48;
        E->instruction_timer = 1;
      }
    }
  }
}
void WalkingSpacePirates_FE4B(void) {  // 0xB2FE4B
  uint16 v0 = 8;
  while (!projectile_type[v0 >> 1]) {
    v0 -= 2;
    if ((v0 & 0x8000) != 0)
      return;
  }
  int v1 = v0 >> 1;
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(abs16(projectile_x_pos[v1] - E->x_pos) - 32) < 0
      && (int16)(abs16(projectile_y_pos[v1] - E->y_pos) - 32) < 0) {
    E->current_instruction = ((E->x_pos - samus_x_pos) & 0x8000) ? addr_kSpacePirates_Ilist_FB58 : addr_kSpacePirates_Ilist_FB4C;
    E->instruction_timer = 1;
  }
}
void Enemy_NormalTouchAI_B3(void) {  // 0xB38023
  NormalEnemyTouchAi();
}
void Enemy_NormalShotAI_B3(void) {  // 0xB3802D
  NormalEnemyShotAi();
}
void Enemy_NormalFrozenAI_B3(void) {  // 0xB38041
  NormalEnemyFrozenAI();
}
uint16 Enemy_SetAiPreInstr_B3(uint16 k, uint16 jp) {  // 0xB3806B
  gEnemyData(k)->ai_preinstr = IW_B3((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
uint16 Enemy_ClearAiPreInstr_B3(uint16 k, uint16 jp) {  // 0xB38074
  gEnemyData(k)->ai_preinstr = 0x807B /* nullsub_171_B3 */;
  return jp;
}
void BrinstarPipeBug_Init(void) {  // 0xB3883B
  EnemyData *PipeBug = gEnemyData(cur_enemy_index); const uint16 PipeBug_k = (cur_enemy_index);
  PipeBug->ai_var_B = PipeBug->x_pos;
  uint16 y_pos = PipeBug->y_pos;
  PipeBug->ai_var_C = y_pos;
  ERAM(0x7800 + PipeBug_k) = y_pos - 16;
  PipeBug->ai_preinstr = 0x8880 /* BrinstarPipeBug_PreInstr_1 */;
  PipeBug->ai_var_D = 48;
  PipeBug->ai_var_E = 0;
  ERAM(0x7802 + PipeBug_k) = 0;
  PipeBug->current_instruction = addr_kBrinstarPipeBug_Ilist_87AB;
  if (PipeBug->parameter_1)
    PipeBug->current_instruction = addr_kBrinstarPipeBug_Ilist_8A1D;
}
void BrinstarPipeBug_Main(void) {  // 0xB3887A
  EnemyData *PipeBug = gEnemyData(cur_enemy_index); const uint16 PipeBug_k = (cur_enemy_index);
  CallEnemyPreInstr(PipeBug->ai_preinstr | 0xB30000);
}
void BrinstarPipeBug_PreInstr_1(uint16 k) {  // 0xB38880
  if (!CheckIfEnemyIsOnScreen())
    gEnemyData(cur_enemy_index)->ai_preinstr = 0x8890 /* BrinstarPipeBug_PreInstr_2 */;
}
void BrinstarPipeBug_PreInstr_2(uint16 k) {  // 0xB38890
  int16 SamusEnemyDelta_Y;

  SamusEnemyDelta_Y = GetSamusEnemyDelta_Y(k);
  if (SamusEnemyDelta_Y < 0 && !sign16(SamusEnemyDelta_Y + 96)) {
    EnemyData *PipeBug = gEnemyData(k); const uint16 PipeBug_k = (k);

    uint16 a = GetSamusEnemyDelta_X(k);
    PipeBug->ai_var_A = PipeBug->ai_var_A & 0x7fff | sign16(a);

    if (abs16(PipeBug->x_pos - samus_x_pos) < 0x40) {
      PipeBug->ai_preinstr = 0x88E3 /* BrinstarPipeBug_PreInstr_3 */;
      PipeBug->properties &= ~kEnemyProps_Invisible;
      PipeBug->timer = 0;
      uint16 v4;
      if ((PipeBug->ai_var_A & 0x8000) != 0)
        v4 = 0;
      else
        v4 = 2;
      PipeBug->ai_var_E = v4;
      BrinstarPipeBug_Func_1();
    }
  }
}
void BrinstarPipeBug_PreInstr_3(uint16 k) {  // 0xB388E3
  EnemyData *PipeBug = gEnemyData(k); const uint16 PipeBug_k = (k);
  bool v2 = PipeBug->y_subpos-- != 0;
  PipeBug->y_pos += v2 - 2;
  if ((int16)(ERAM(0x7800 + PipeBug_k) - PipeBug->y_pos) >= 0 && PipeBug->y_pos < samus_y_pos) {
    PipeBug->ai_var_E |= 1;
    BrinstarPipeBug_Func_1();
    PipeBug->ai_preinstr = 0x891C /* BrinstarPipeBug_PreInstr_4 */;
  }
}
void BrinstarPipeBug_PreInstr_4(uint16 k) {  // 0xB3891C
  EnemyData *PipeBug = gEnemyData(k); const uint16 PipeBug_k = (k);
  if ((PipeBug->ai_var_A & 0x8000) == 0) {
    PipeBug->x_subpos = PipeBug->x_subpos;
    PipeBug->x_pos = PipeBug->x_pos + 2;
  } else {
    PipeBug->x_subpos = PipeBug->x_subpos;
    PipeBug->x_pos = PipeBug->x_pos - 2;
  }
  if (IsEnemyLeavingScreen(k) & 1) {
    PipeBug->x_pos = PipeBug->ai_var_B;
    PipeBug->x_subpos = 0;
    uint16 pbg_var_C = PipeBug->ai_var_C;
    PipeBug->y_pos = pbg_var_C;
    PipeBug->y_subpos = pbg_var_C;
    PipeBug->ai_var_E = 0;
    BrinstarPipeBug_Func_1();
    PipeBug->properties |= kEnemyProps_Invisible;
    PipeBug->ai_var_D = 48;
    PipeBug->ai_preinstr = 0x897E /* BrinstarPipeBug_PreInstr_5 */;
  }
}
void BrinstarPipeBug_PreInstr_5(uint16 k) {  // 0xB3897E
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (E->ai_var_D-- == 1)
    E->ai_preinstr = 0x8890 /* BrinstarPipeBug_PreInstr_2 */;
}
void BrinstarPipeBug_Func_1(void) {  // 0xB3898B
  EnemyData *PipeBug = gEnemyData(cur_enemy_index); const uint16 PipeBug_k = (cur_enemy_index);
  uint16 pbg_var_E = PipeBug->ai_var_E;
  if (pbg_var_E != ERAM(0x7802 + PipeBug_k)) {
    ERAM(0x7802 + PipeBug_k) = pbg_var_E;
    PipeBug->current_instruction = PipeBug->parameter_1 ?
      ((uint16)BW_B3(0x8833 + 2 * (pbg_var_E))) : ((uint16)BW_B3(0x882B + 2 * (pbg_var_E)));
    PipeBug->instruction_timer = 1;
    PipeBug->timer = 0;
  }
}
void CallBugInstr(uint32 ea, uint16 k) {
  switch (ea) {
  case 0xB38BCD: NorfairPipeBug_Func_1(); return;  // 0xb38bcd
  case 0xB38BFF: NorfairPipeBug_Func_2(); return;  // 0xb38bff
  case 0xB38CA6: NorfairPipeBug_Func_4(); return;  // 0xb38ca6
  case 0xB38CFF: NorfairPipeBug_Func_5(k); return;  // 0xb38cff
  case 0xB38D0C: NorfairPipeBug_Func_6(); return;  // 0xb38d0c
  case 0xB38D4E: NorfairPipeBug_Func_7(); return;  // 0xb38d4e
  case 0xB38D90: NorfairPipeBug_Func_8(); return;  // 0xb38d90
  case 0xB38DD2: NorfairPipeBug_Func_9(); return;  // 0xb38dd2
  case 0xB38E14: NorfairPipeBug_Func_10(); return;  // 0xb38e14
  case 0xB38E35: NorfairPipeBug_Func_11(); return;  // 0xb38e35
  case 0xB38E5A: NorfairPipeBug_Func_12(); return;  // 0xb38e5a
  case 0xB38FB5: BrinstarYellowPipeBug_Func_1(); return;  // 0xb38fb5
  case 0xB38FF5: BrinstarYellowPipeBug_Func_2(cur_enemy_index); return;  // 0xb38ff5
  case 0xB39028: BrinstarYellowPipeBug_Func_3(); return;  // 0xb39028
  case 0xB390BD: BrinstarYellowPipeBug_Func_5(); return;  // 0xb390bd
  case 0xB3915A: BrinstarYellowPipeBug_Func_7(); return;  // 0xb3915a
  case 0xB391D8: BrinstarYellowPipeBug_Func_8(); return;  // 0xb391d8
  default: nt_unimpl("Unreachable");
  }
}
void NorfairPipeBug_Func_1(void) {  // 0xB38BCD
  if (LOBYTE(gEnemyData(cur_enemy_index)->parameter_2)
      && gEnemyData(cur_enemy_index + 64)->ai_var_A == 0x8BCD /* NorfairPipeBug_Func_1 */
      && gEnemyData(cur_enemy_index + 128)->ai_var_A == 0x8BCD /* NorfairPipeBug_Func_1 */
      && gEnemyData(cur_enemy_index + 192)->ai_var_A == 0x8BCD /* NorfairPipeBug_Func_1 */
      && gEnemyData(cur_enemy_index + 256)->ai_var_A == 0x8BCD /* NorfairPipeBug_Func_1 */) {
    gEnemyData(cur_enemy_index)->ai_var_A = 0x8BFF /* NorfairPipeBug_Func_2 */;
  }
}
void NorfairPipeBug_Func_2(void) {  // 0xB38BFF
  int k = cur_enemy_index;
  EnemyData *PipeBug = gEnemyData(cur_enemy_index); const uint16 PipeBug_k = (cur_enemy_index);
  if (IsSamusWithinEnemy_X(cur_enemy_index, LOBYTE(PipeBug->parameter_2))
      && (int16)(PipeBug->y_pos - samus_y_pos) >= 0) {
    ++PipeBug->ai_var_C;
    PipeBug->instruction_timer = 1;
    PipeBug->timer = 0;
    if ((GetSamusEnemyDelta_X(k) & 0x8000) == 0) {
      gEnemyData(k)->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
      gEnemyData(k + 64)->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
      gEnemyData(k + 128)->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
      gEnemyData(k + 192)->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
      gEnemyData(k + 256)->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
    } else {
      PipeBug->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
      gEnemyData(k + 64)->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
      gEnemyData(k + 128)->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
      gEnemyData(k + 192)->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
      gEnemyData(k + 256)->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
    }
    NorfairPipeBug_Func_3(k);
  }
}
void NorfairPipeBug_Func_3(uint16 k) {  // 0xB38C52
  gEnemyData(k)->ai_var_A = 0x8CA6 /* NorfairPipeBug_Func_4 */;
  gEnemyData(k + 64)->ai_var_A = 0x8CA6 /* NorfairPipeBug_Func_4 */;
  gEnemyData(k + 128)->ai_var_A = 0x8CA6 /* NorfairPipeBug_Func_4 */;
  gEnemyData(k + 192)->ai_var_A = 0x8CA6 /* NorfairPipeBug_Func_4 */;
  gEnemyData(k + 256)->ai_var_A = 0x8CA6 /* NorfairPipeBug_Func_4 */;
  ERAM(0x7804 + (k)) = 104;
  ERAM(0x7804 + (k + 64)) = 96;
  ERAM(0x7804 + (k + 128)) = 88;
  ERAM(0x7804 + (k + 192)) = 112;
  ERAM(0x7804 + (k + 256)) = 120;
  gEnemyData(k)->ai_preinstr = 0x8CFF /* NorfairPipeBug_Func_5 */;
  gEnemyData(k + 64)->ai_preinstr = 0x8D0C /* NorfairPipeBug_Func_6 */;
  gEnemyData(k + 128)->ai_preinstr = 0x8D4E /* NorfairPipeBug_Func_7 */;
  gEnemyData(k + 192)->ai_preinstr = 0x8D90 /* NorfairPipeBug_Func_8 */;
  gEnemyData(k + 256)->ai_preinstr = 0x8DD2 /* NorfairPipeBug_Func_9 */;
}
void NorfairPipeBug_Func_4(void) {  // 0xB38CA6
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->properties &= ~kEnemyProps_Invisible;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (66))), ((uint16)BW_A2(0x8187 + 2 * (67)))));
  if ((int16)(E->y_pos - samus_y_pos) < 0) {
    E->ai_var_A = E->ai_preinstr;
    ERAM(0x7802 + E_k) = E->y_pos;
    E->instruction_timer = 1;
    E->timer = 0;
    if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) == 0)
      E->current_instruction = addr_kNorfairPipeBug_Ilist_8B21;
    else
      E->current_instruction = addr_kNorfairPipeBug_Ilist_8AE1;
  }
}
void NorfairPipeBug_Func_5(uint16 k) {  // 0xB38CFF
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ++E->ai_var_C;
  E->ai_var_A = 0x8E5A /* NorfairPipeBug_Func_12 */;
}
void NorfairPipeBug_Func_6(void) {  // 0xB38D0C
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ++E->ai_var_C;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (66))), ((uint16)BW_A2(0x8187 + 2 * (67)))));
  if ((int16)(E->y_pos + 16 - ERAM(0x7802 + E_k)) < 0) {
    E->y_pos = ERAM(0x7802 + E_k) - 16;
    E->y_subpos = 0;
    E->ai_var_A = 0x8E5A /* NorfairPipeBug_Func_12 */;
  }
}
void NorfairPipeBug_Func_7(void) {  // 0xB38D4E
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ++E->ai_var_C;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (66))), ((uint16)BW_A2(0x8187 + 2 * (67)))));
  if ((int16)(E->y_pos + 32 - ERAM(0x7802 + E_k)) < 0) {
    E->y_pos = ERAM(0x7802 + E_k) - 32;
    E->y_subpos = 0;
    E->ai_var_A = 0x8E5A /* NorfairPipeBug_Func_12 */;
  }
}
void NorfairPipeBug_Func_8(void) {  // 0xB38D90
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ++E->ai_var_C;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (64))), ((uint16)BW_A2(0x8187 + 2 * (65)))));
  if ((int16)(E->y_pos - 16 - ERAM(0x7802 + E_k)) >= 0) {
    E->y_pos = ERAM(0x7802 + E_k) + 16;
    E->y_subpos = 0;
    E->ai_var_A = 0x8E5A /* NorfairPipeBug_Func_12 */;
  }
}
void NorfairPipeBug_Func_9(void) {  // 0xB38DD2
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ++E->ai_var_C;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (64))), ((uint16)BW_A2(0x8187 + 2 * (65)))));
  if ((int16)(E->y_pos - 32 - ERAM(0x7802 + E_k)) >= 0) {
    E->y_pos = ERAM(0x7802 + E_k) + 32;
    E->y_subpos = 0;
    E->ai_var_A = 0x8E5A /* NorfairPipeBug_Func_12 */;
  }
}
void NorfairPipeBug_Func_10(void) {  // 0xB38E14
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = E->ai_var_B >> 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (v2 + 2))), ((uint16)BW_A2(0x8187 + 2 * (v2 + 3)))));
}
void NorfairPipeBug_Func_11(void) {  // 0xB38E35
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  int v2 = E->ai_var_B >> 1;
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(((uint16)BW_A2(0x8187 + 2 * (v2))), ((uint16)BW_A2(0x8187 + 2 * (v2 + 1)))));
}
void NorfairPipeBug_Func_12(void) {  // 0xB38E5A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if ((int16)(++E->ai_var_C - ERAM(0x7804 + E_k)) >= 0) {
    E->ai_var_C = 0;
    E->instruction_timer = 1;
    E->timer = 0;
    E->ai_var_A = 0x8E14 /* NorfairPipeBug_Func_10 */;
    E->current_instruction = addr_kNorfairPipeBug_Ilist_8B05;
    if ((GetSamusEnemyDelta_X(cur_enemy_index) & 0x8000) == 0) {
      E->ai_var_A = 0x8E35 /* NorfairPipeBug_Func_11 */;
      E->current_instruction = addr_kNorfairPipeBug_Ilist_8B45;
    }
  }
}
void BrinstarYellowPipeBug_Init(void) {  // 0xB38F4C

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  ERAM(0x780E + E_k) = E->x_pos;
  ERAM(0x7810 + E_k) = E->y_pos;
  E->x_subpos = 0;
  E->y_subpos = 0;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8EFC;
  if (!E->parameter_1)
    E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F24;
  int v1 = (8 * E->parameter_2) >> 1;
  ERAM(0x7802 + E_k) = ((uint16)BW_A2(0x8187 + 2 * (v1)));
  ERAM(0x7800 + E_k) = ((uint16)BW_A2(0x8187 + 2 * (v1 + 1)));
  ERAM(0x7806 + E_k) = ((uint16)BW_A2(0x8187 + 2 * (v1 + 2)));
  ERAM(0x7804 + E_k) = ((uint16)BW_A2(0x8187 + 2 * (v1 + 3)));
  E->ai_var_A = 0x8FB5 /* BrinstarYellowPipeBug_Func_1 */;
  ERAM(0x780C + E_k) = 0;
}
void BrinstarYellowPipeBug_Main(void) {  // 0xB38FAE

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallBugInstr(E->ai_var_A | 0xB30000, cur_enemy_index);
}
void BrinstarYellowPipeBug_Func_1(void) {  // 0xB38FB5
  int16 v2;
  int16 SamusEnemyDelta_X;

  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (E->parameter_1) {
    SamusEnemyDelta_X = GetSamusEnemyDelta_X(cur_enemy_index);
    if (SamusEnemyDelta_X < 0 && !sign16(SamusEnemyDelta_X + 192))
      goto LABEL_7;
  } else {
    v2 = GetSamusEnemyDelta_X(cur_enemy_index);
    if (v2 >= 0 && sign16(v2 - 192)) {
LABEL_7:
      if (IsSamusWithinEnemy_Y(cur_enemy_index, 0x30)) {
        E->properties &= ~kEnemyProps_Invisible;
        ERAM(0x8000 + E_k) = 24;
        E->ai_var_A = 0x8FF5 /* BrinstarYellowPipeBug_Func_2 */;
      }
    }
  }
}
void BrinstarYellowPipeBug_Func_2(uint16 k) {  // 0xB38FF5

  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  uint16 v2 = ERAM(0x8000 + E_k) - 1;
  ERAM(0x8000 + E_k) = v2;
  if (!v2) {
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8EFC;
    E->ai_var_A = 0x9028 /* BrinstarYellowPipeBug_Func_3 */;
    if (!E->parameter_1) {
      E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F24;
      E->ai_var_A = 0x90BD /* BrinstarYellowPipeBug_Func_5 */;
    }
  }
}
void BrinstarYellowPipeBug_Func_3(void) {  // 0xB39028
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  BrinstarYellowPipeBug_Func_4(cur_enemy_index);
  if (CheckIfEnemyIsOnScreen()) {
    E->x_pos = ERAM(0x780E + E_k);
    E->y_pos = ERAM(0x7810 + E_k);
    E->x_subpos = 0;
    E->y_subpos = 0;
    E->ai_var_A = 0x8FB5 /* BrinstarYellowPipeBug_Func_1 */;
    ERAM(0x780C + E_k) = 0;
    E->properties |= kEnemyProps_Invisible;
  } else {
    if (!ERAM(0x780C + E_k)) {
      if (sign16(E->x_pos - samus_x_pos - 48)) {
        E->ai_var_A = 0x915A /* BrinstarYellowPipeBug_Func_7 */;
        E->ai_var_C = 0;
        E->ai_var_D = 0;
        E->ai_var_E = 40;
        E->ai_preinstr = 1;
        ERAM(0x7808 + E_k) = E->x_pos;
        E->instruction_timer = 1;
        E->timer = 0;
        E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F10;
      }
    }
  }
}
void BrinstarYellowPipeBug_Func_4(uint16 k) {  // 0xB390A1
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(ERAM(0x7806 + E_k), ERAM(0x7804 + E_k)));
}
void BrinstarYellowPipeBug_Func_5(void) {  // 0xB390BD
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);

  BrinstarYellowPipeBug_Func_6(cur_enemy_index);
  if (CheckIfEnemyIsOnScreen()) {
    E->x_pos = ERAM(0x780E + E_k);
    E->y_pos = ERAM(0x7810 + E_k);
    E->x_subpos = 0;
    E->y_subpos = 0;
    E->ai_var_A = 0x8FB5 /* BrinstarYellowPipeBug_Func_1 */;
    ERAM(0x780C + E_k) = 0;
    E->properties |= kEnemyProps_Invisible;
  } else {
    if (!ERAM(0x780C + E_k)) {
      if (sign16(samus_x_pos - E->x_pos - 48)) {
        E->ai_var_A = 0x91D8 /* BrinstarYellowPipeBug_Func_8 */;
        E->ai_var_C = 0;
        E->ai_var_D = 0;
        E->ai_var_E = 40;
        E->ai_preinstr = 1;
        ERAM(0x7808 + E_k) = E->x_pos;
        E->instruction_timer = 1;
        E->timer = 0;
        E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F38;
      }
    }
  }
}
void BrinstarYellowPipeBug_Func_6(uint16 k) {  // 0xB3913A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, __PAIR32__(ERAM(0x7802 + E_k), ERAM(0x7800 + E_k)));
}
void BrinstarYellowPipeBug_Func_7(void) {  // 0xB3915A
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (CheckIfEnemyIsOnScreen()) {
    E->x_pos = ERAM(0x780E + E_k);
    E->y_pos = ERAM(0x7810 + E_k);
    E->x_subpos = 0;
    E->y_subpos = 0;
    E->ai_var_A = 0x8FB5 /* BrinstarYellowPipeBug_Func_1 */;
    ERAM(0x780C + E_k) = 0;
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8EFC;
    E->properties |= kEnemyProps_Invisible;
  } else {
    BrinstarYellowPipeBug_Func_4(cur_enemy_index);
    if (E->ai_preinstr) {
      BrinstarYellowPipeBug_Func_10(cur_enemy_index);
    } else {
      BrinstarYellowPipeBug_Func_9(cur_enemy_index);
      if ((int16)(E->y_pos - ERAM(0x7810 + E_k)) < 0) {
        ERAM(0x780C + E_k) = 1;
        E->ai_preinstr = 1;
        E->ai_var_A = 0x9028 /* BrinstarYellowPipeBug_Func_3 */;
        E->instruction_timer = 1;
        E->timer = 0;
        E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8EFC;
      }
    }
  }
}
void BrinstarYellowPipeBug_Func_8(void) {  // 0xB391D8
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (CheckIfEnemyIsOnScreen()) {
    E->x_pos = ERAM(0x780E + E_k);
    E->y_pos = ERAM(0x7810 + E_k);
    E->x_subpos = 0;
    E->y_subpos = 0;
    E->ai_var_A = 0x8FB5 /* BrinstarYellowPipeBug_Func_1 */;
    ERAM(0x780C + E_k) = 0;
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F24;
    E->properties |= kEnemyProps_Invisible;
  } else {
    BrinstarYellowPipeBug_Func_6(cur_enemy_index);
    if (E->ai_preinstr) {
      BrinstarYellowPipeBug_Func_10(cur_enemy_index);
    } else {
      BrinstarYellowPipeBug_Func_9(cur_enemy_index);
      if ((int16)(ERAM(0x7810 + E_k) - E->y_pos) >= 0) {
        ERAM(0x780C + E_k) = 1;
        E->ai_preinstr = 1;
        E->instruction_timer = 1;
        E->timer = 0;
        E->current_instruction = addr_kBrinstarYellowPipeBug_Ilist_8F24;
        E->ai_var_A = 0x90BD /* BrinstarYellowPipeBug_Func_5 */;
      }
    }
  }
}
void BrinstarYellowPipeBug_Func_9(uint16 k) {  // 0xB39256
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  ++E->ai_var_E;
  int v3 = (8 * E->ai_var_E) >> 1;
  AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v3 + 3))), ((uint16)BW_A2(0x838F + 2 * (v3 + 2)))));
}
void BrinstarYellowPipeBug_Func_10(uint16 k) {  // 0xB3927A
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if ((--E->ai_var_E & 0x8000) != 0) {
    E->ai_var_E = 0;
    E->ai_preinstr = 0;
  } else {
    int v4 = (8 * E->ai_var_E) >> 1;
    AddToHiLo(&E->y_pos, &E->y_subpos, __PAIR32__(((uint16)BW_A2(0x838F + 2 * (v4 + 1))), ((uint16)BW_A2(0x838F + 2 * (v4)))));
  }
}
uint16 EscapeEtecoon_Instr_1(uint16 k, uint16 jp) {  // 0xB3E545
  if (lava_acid_y_pos >= 0xCE)
    return (uint16)(jp + 2);
  else
    return (uint16)(IW_B3((uint16)(jp + 0)));
}
uint16 EscapeEtecoon_Instr_2(uint16 k, uint16 jp) {  // 0xB3E610
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  E->x_pos += IW_B3((uint16)(jp + 0));
  return (uint16)(jp + 2);
}
void EscapeEtecoon_Main(void) {  // 0xB3E655
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  CallEnemyPreInstr(E->ai_preinstr | 0xB30000);
}
void EscapeEtecoon_E65C(uint16 k) {  // 0xB3E65C
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  AddToHiLo(&E->x_pos, &E->x_subpos, 0x38000);
}
void EscapeEtecoon_E670(uint16 k) {  // 0xB3E670
  if (CheckEventHappened(0xF))
    gEnemyData(k)->current_instruction = addr_kEscapeEtecoon_Ilist_E5DA;
}
void EscapeEtecoon_E680(uint16 k) {  // 0xB3E680
  EnemyData *E = gEnemyData(k); const uint16 E_k = (k);
  if (Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL8(E->ai_var_A))) {
    E->instruction_timer = 1;
    bool v3 = (-E->ai_var_A & 0x8000) != 0;
    E->ai_var_A = -E->ai_var_A;
    E->current_instruction = v3 ?
      addr_kEscapeEtecoon_Ilist_E556 : addr_kEscapeEtecoon_Ilist_E582;
    if (CheckEventHappened(0xF))
      E->current_instruction = addr_kEscapeEtecoon_Ilist_E5AE;
  }
  Enemy_MoveDown(k, INT16_SHL16(1));
}
void EscapeEtecoon_Init(void) {  // 0xB3E6CB
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (CheckEventHappened(0xF)) {
    E->properties |= kEnemyProps_Deleted;
  } else {
    E->properties |= kEnemyProps_DisableSamusColl | kEnemyProps_Tangible | 0x8000;
    E->instruction_timer = 1;
    E->timer = 0;
    E->palette_index = 0;
    int v3 = E->parameter_1 >> 1;
    E->x_pos = ((uint16)BW_B3(0xE718 + 2 * (v3)));
    E->y_pos = ((uint16)BW_B3(0xE71E + 2 * (v3)));
    E->ai_preinstr = ((uint16)BW_B3(0xE724 + 2 * (v3)));
    E->current_instruction = ((uint16)BW_B3(0xE72A + 2 * (v3)));
    E->ai_var_A = ((uint16)BW_B3(0xE730 + 2 * (v3)));
  }
}
uint16 EscapeDachora_Instr_2(uint16 k, uint16 jp) {  // 0xB3EAA8
  if (lava_acid_y_pos >= 0xCE)
    return (uint16)(jp + 2);
  else
    return (uint16)(IW_B3((uint16)(jp + 0)));
}
uint16 EscapeDachora_Instr_3(uint16 k, uint16 jp) {  // 0xB3EAB8
  if (CheckEventHappened(0xF))
    return (uint16)(IW_B3((uint16)(jp + 0)));
  else
    return (uint16)(jp + 2);
}
uint16 EscapeDachora_Instr_1(uint16 k, uint16 jp) {  // 0xB3EAC9
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->x_pos -= 6;
  return jp;
}
uint16 EscapeDachora_Instr_4(uint16 k, uint16 jp) {  // 0xB3EAD7
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  E->x_pos += 6;
  return jp;
}
void EscapeDachora_Init(void) {  // 0xB3EAE5
  EnemyData *E = gEnemyData(cur_enemy_index); const uint16 E_k = (cur_enemy_index);
  if (CheckEventHappened(0xF)) {
    E->properties |= kEnemyProps_Deleted;
  } else {
    E->properties |= kEnemyProps_DisableSamusColl;
    E->spritemap_pointer = addr_kSpritemap_Nothing_B3;
    E->instruction_timer = 1;
    E->timer = 0;
    E->current_instruction = addr_kEscapeDachora_Ilist_E964;
  }
}
void port_nop_v(void) {}
void port_nop_k(uint16 k) { (void)k; }   /* nullsubs: quien llama puede saltarlas */
static void port_w_Fireflea_Touch(void) { Fireflea_Touch(cur_enemy_index); }
static const struct { uint32 ea; PortFnV f; } kPortCallEnemyAi[187] = {
  {0xA28023, Enemy_NormalTouchAI_A2},
  {0xA2802D, Enemy_NormalShotAI_A2},
  {0xA28041, Enemy_NormalFrozenAI_A2},
  {0xA2804C, port_nop_v},
  {0xA2804C, port_nop_v},
  {0xA2871C, BouncingGoofball_Init},
  {0xA2879C, BouncingGoofball_Main},
  {0xA29A3F, ThinHoppingBlobs_Init},
  {0xA29A7D, ThinHoppingBlobs_Main},
  {0xA29F48, SpikeShootingPlant_Init},
  {0xA29FB3, SpikeShootingPlant_Main},
  {0xA2B06B, Flies_Init},
  {0xA2B11F, Flies_Main},
  {0xA2BBCD, Rio_Init},
  {0xA2BBE3, Rio_Main},
  {0xA2DF76, HirisingSlowfalling_Init},
  {0xA2E02E, HirisingSlowfalling_Main},
  {0xA2E1D3, Gripper_Init},
  {0xA2E221, Gripper_Main},
  {0xA2E318, JetPowerRipper_Init},
  {0xA2E353, JetPowerRipper_Main},
  {0xA2E3A9, JetPowerRipper_Shot},
  {0xA2E49F, Ripper_Init},
  {0xA2E4DA, Ripper_Main},
  {0xA2E9DA, TimedShutter_Init},
  {0xA2EAB6, TimedShutter_Main},
  {0xA2EE05, RisingFallingPlatform_Init},
  {0xA2EE12, ShootableShutter_Init},
  {0xA2EED1, RisingFallingPlatform_Main},
  {0xA2F09D, RisingFallingPlatform_Touch},
  {0xA2F0A2, RisingFallingPlatform_Shot},
  {0xA2F0AA, ShootableShutter_Shot},
  {0xA2F0B6, RisingFallingPlatform_Powerbomb},
  {0xA38023, Enemy_NormalTouchAI_A3},
  {0xA3802D, Enemy_NormalShotAI_A3},
  {0xA38041, Enemy_NormalFrozenAI_A3},
  {0xA3804C, port_nop_v},
  {0xA386ED, Waver_Init},
  {0xA3874C, Waver_Main},
  {0xA38960, Metalee_Init},
  {0xA38979, Metalee_Main},
  {0xA38B0F, Metalee_Shot},
  {0xA38D2D, Fireflea_Init},
  {0xA38DEE, Fireflea_Main},
  {0xA38E6B, port_w_Fireflea_Touch},
  {0xA38E83, Fireflea_Powerbomb},
  {0xA38E89, Fireflea_Shot},
  {0xA390B5, MaridiaFish_Init},
  {0xA3912B, MaridiaFish_Main},
  {0xA394E6, Elevator_Init},
  {0xA3952A, Elevator_Frozen},
  {0xA396E3, Crab_Init},
  {0xA3993B, Slug_Init},
  {0xA39C9F, PlatformThatFallsWithSamus_Init},
  {0xA39CBA, FastMovingSlowSinkingPlatform_Init},
  {0xA39D16, PlatformThatFallsWithSamus_Main},
  {0xA39F07, port_nop_v},
  {0xA39F08, FastMovingSlowSinkingPlatform_Shot},
  {0xA3A14D, Roach_Init},
  {0xA3A2D0, Roach_Main},
  {0xA3AB09, Sidehopper_Init},
  {0xA3ABCF, Sidehopper_Main},
  {0xA3C6AE, Skree_Init},
  {0xA3C6C7, Skree_Main},
  {0xA3C7F5, Skree_Shot},
  {0xA3DC1B, port_nop_v},
  {0xA3E043, WreckedShipOrangeZoomer_Init},
  {0xA3E08B, WreckedShipOrangeZoomer_Main},
  {0xA3E2D4, BigEyeBugs_Init},
  {0xA3E59C, FireZoomer_Init},
  {0xA3E669, StoneZoomer_Init},
  {0xA3E6C2, StoneZoomer_Main},
  {0xA4804C, port_nop_v},
  {0xA4B950, port_nop_v},
  {0xA4B950, port_nop_v},
  {0xA58046, CreateADudShot},
  {0xA5804C, port_nop_v},
  {0xA5804C, port_nop_v},
  {0xA5804C, port_nop_v},
  {0xA5C5AA, port_nop_v},
  {0xA5C5C4, port_nop_v},
  {0xA5EA2A, SporeSpawn_Init},
  {0xA5EB13, SporeSpawn_Main},
  {0xA5ED5A, SporeSpawn_Shot},
  {0xA5ED5A, SporeSpawn_Shot},
  {0xA5EDEC, SporeSpawn_Touch},
  {0xA5EDEC, SporeSpawn_Touch},
  {0xA5EDF2, port_nop_v},
  {0xA6804C, port_nop_v},
  {0xA6804C, port_nop_v},
  {0xA686F5, Boulder_Init},
  {0xA68793, Boulder_Main},
  {0xA69A58, FakeKraid_Init},
  {0xA69AC2, FakeKraid_Main},
  {0xA69C22, FakeKraid_Touch},
  {0xA69C39, FakeKraid_Shot},
  {0xA6F920, port_nop_v},
  {0xA7804C, port_nop_v},
  {0xA7804C, port_nop_v},
  {0xA7804C, port_nop_v},
  {0xA7948F, port_nop_v},
  {0xA794B5, port_nop_v},
  {0xA794B5, port_nop_v},
  {0xA7DD9A, port_nop_v},
  {0xA7E011, port_nop_v},
  {0xA7E912, Etecoon_Init},
  {0xA7E940, Etecoon_Main},
  {0xA7F4DD, Dachora_Init},
  {0xA7F52E, Dachora_Main},
  {0xA88023, Enemy_NormalTouchAI_A8},
  {0xA8802D, Enemy_NormalShotAI_A8},
  {0xA88041, Enemy_NormalFrozenAI_A8},
  {0xA8804C, port_nop_v},
  {0xA89058, MorphBallEye_Init},
  {0xA890E2, MorphBallEye_Main},
  {0xA8A148, YappingMaw_Init},
  {0xA8A211, YappingMaw_Main},
  {0xA8A799, YappingMaw_Touch},
  {0xA8A7BD, YappingMaw_Shot},
  {0xA8A835, YappingMaw_Frozen},
  {0xA8AB46, Kago_Init},
  {0xA8AB75, Kago_Main},
  {0xA8AB83, Kago_Shot},
  {0xA8B776, Beetom_Init},
  {0xA8B80D, Beetom_Main},
  {0xA8BE2E, Beetom_Touch},
  {0xA8BEAC, Beetom_Shot},
  {0xA8CC66, port_nop_v},
  {0xA8DCCD, WalkingLavaSeahorse_Init},
  {0xA8DD6B, WalkingLavaSeahorse_Main},
  {0xA8E82E, BlueBrinstarFaceBlock_Init},
  {0xA8E8AE, BlueBrinstarFaceBlock_Main},
  {0xA8E91D, BlueBrinstarFaceBlock_Shot},
  {0xA8F188, KiHunter_Init},
  {0xA8F214, KiHunterWings_Init},
  {0xA8F25C, KiHunter_Main},
  {0xA8F262, KiHunterWings_Main},
  {0xA8F701, KiHunter_Shot},
  {0xA9804C, port_nop_v},
  {0xA9B5C5, port_nop_v},
  {0xA9B5C5, port_nop_v},
  {0xAA8041, Enemy_NormalFrozenAI_AA},
  {0xAA804C, port_nop_v},
  {0xAAC67E, Torizo_Hurt},
  {0xAAC6A4, Torizo_Main},
  {0xAAC87F, Torizo_Init},
  {0xAAC977, GoldTorizo_Touch},
  {0xAAC977, GoldTorizo_Touch},
  {0xAAC97C, Torizo_Shot},
  {0xAAC97C, Torizo_Shot},
  {0xAAC9C1, port_nop_v},
  {0xAAC9C2, Torizo_Func_8},
  {0xAAD667, GoldTorizo_Shot},
  {0xAAD7C7, port_nop_v},
  {0xAAD7C8, TourianEntranceStatue_Init},
  {0xAAE725, ChozoStatue_Init},
  {0xAAE7A7, ChozoStatue_Main},
  {0xAAE7DB, port_nop_v},
  {0xAAE7DC, port_nop_v},
  {0xB28041, Enemy_NormalFrozenAI_B2},
  {0xB2804C, port_nop_v},
  {0xB28767, WalkingSpacePirates_Powerbomb},
  {0xB2876C, WalkingSpacePirates_Touch},
  {0xB2876C, WalkingSpacePirates_Touch},
  {0xB28779, WalkingSpacePirates_Shot},
  {0xB28779, WalkingSpacePirates_Shot},
  {0xB287C8, WalkingSpacePirates_87C8},
  {0xB2883E, WalkingSpacePirates_883E},
  {0xB2EF9F, WallSpacePirates_Init},
  {0xB2F02D, WallSpacePirates_Main},
  {0xB2F5DE, NinjaSpacePirates_Init},
  {0xB2F6A2, NinjaSpacePirates_Main},
  {0xB2FD02, WalkingSpacePirates_Init},
  {0xB2FD32, WalkingSpacePirates_Main},
  {0xB38023, Enemy_NormalTouchAI_B3},
  {0xB3802D, Enemy_NormalShotAI_B3},
  {0xB38037, NormalEnemyPowerBombAi},
  {0xB38041, Enemy_NormalFrozenAI_B3},
  {0xB3804C, port_nop_v},
  {0xB3883B, BrinstarPipeBug_Init},
  {0xB3887A, BrinstarPipeBug_Main},
  {0xB38F4C, BrinstarYellowPipeBug_Init},
  {0xB38FAE, BrinstarYellowPipeBug_Main},
  {0xB3E655, EscapeEtecoon_Main},
  {0xB3E6CB, EscapeEtecoon_Init},
  {0xB3EAE5, EscapeDachora_Init},
  {0xB3EB1A, port_nop_v}
};
PortFnV port_FindCallEnemyAi(uint32 ea) {  /* 0: no está traducida; las últimas, recordadas */
  static uint32 cea[16]; static PortFnV cf[16];
  uint16 h = ((uint16)ea ^ ((uint16)ea >> 4)) & 15;
  if (cea[h] == ea) return cf[h];
  PortFnV f = 0;
  int lo = 0, hi = 186;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    uint32 e = kPortCallEnemyAi[m].ea;
    if (e == ea) { f = kPortCallEnemyAi[m].f; break; }
    if (e < ea) lo = m + 1; else hi = m - 1;
  }
  cea[h] = ea; cf[h] = f;
  return f;
}
uint8 port_CallEnemyAi(uint32 ea) {  /* 0: no está traducida */
  PortFnV f = port_FindCallEnemyAi(ea);
  if (!f) return 0;
  f();
  return 1;
}

uint16 port_CallEnemyInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok) {  /* ok = 0: no está traducida */
  *ok = 1;
  switch (ea) {
  case 0xA280ED: return EnemyInstr_Goto(k, jp);
  case 0xA28110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xA28123: return EnemyInstr_SetTimer(k, jp);
  case 0xA28173: return EnemyInstr_EnableOffScreenProcessing(k, jp);
  case 0xA2817D: return EnemyInstr_DisableOffScreenProcessing(k, jp);
  case 0xA288C5: return BouncingGoofball_Instr_88C5(k, jp);
  case 0xA288C6: return BouncingGoofball_Instr_88C6(k, jp);
  case 0xA29F2A: return SpikeShootingPlant_Instr_9F2A(k, jp);
  case 0xA2A095: return SpikeShootingPlant_Instr_A095(k, jp);
  case 0xA2A0A7: return SpikeShootingPlant_Instr_A0A7(k, jp);
  case 0xA2BBC3: return EnemyInstr_Rio_Instr_1(k, jp);
  case 0xA380ED: return EnemyInstr_Goto(k, jp);
  case 0xA38173: return EnemyInstr_EnableOffScreenProcessing(k, jp);
  case 0xA3817D: return EnemyInstr_DisableOffScreenProcessing(k, jp);
  case 0xA386E3: return Waver_Instr_1(k, jp);
  case 0xA38956: return Metalee_Instr_1(k, jp);
  case 0xA39096: return MaridiaFish_Instr_3(k, jp);
  case 0xA390A0: return MaridiaFish_Instr_1(k, jp);
  case 0xA390AA: return MaridiaFish_Instr_2(k, jp);
  case 0xA39C6B: return PlatformThatFallsWithSamus_Instr_3(k, jp);
  case 0xA39C76: return PlatformThatFallsWithSamus_Instr_4(k, jp);
  case 0xA39C81: return PlatformThatFallsWithSamus_Instr_1(k, jp);
  case 0xA39C8C: return PlatformThatFallsWithSamus_Instr_2(k, jp);
  case 0xA3AA68: return Sidehopper_Func_1(k, jp);
  case 0xA3AAFE: return Sidehopper_Instr_1(k, jp);
  case 0xA3C6A4: return Skree_Instr_1(k, jp);
  case 0xA3DFC2: return WreckedShipOrangeZoomer_Func_1(k, jp);
  case 0xA3E660: return Zoomer_Instr_SetPreinstr(k, jp);
  case 0xA480ED: return EnemyInstr_Goto(k, jp);
  case 0xA5807C: return EnemyInstr_StopScript(k, jp);
  case 0xA580ED: return EnemyInstr_Goto(k, jp);
  case 0xA58110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xA58123: return EnemyInstr_SetTimer(k, jp);
  case 0xA594DD: return Draygon_Instr_1(k, jp);
  case 0xA59736: return Draygon_Instr_13(k, jp);
  case 0xA5973F: return Draygon_Instr_8(k, jp);
  case 0xA59752: return Draygon_Instr_7(k, jp);
  case 0xA59765: return Draygon_Instr_6(k, jp);
  case 0xA59778: return Draygon_Instr_9(k, jp);
  case 0xA59895: return Draygon_Instr_2(k, jp);
  case 0xA598D3: return Draygon_Instr_11(k, jp);
  case 0xA598EF: return Draygon_Instr_5(k, jp);
  case 0xA59B9A: return Draygon_Instr_15(k, jp);
  case 0xA59C8A: return Draygon_Instr_17(k, jp);
  case 0xA59E0A: return Draygon_Instr_14(k, jp);
  case 0xA59F57: return Draygon_Instr_16(k, jp);
  case 0xA59F60: return Draygon_Instr_10(k, jp);
  case 0xA59F6E: return Draygon_Instr_4(k, jp);
  case 0xA59F7C: return Draygon_Instr_12(k, jp);
  case 0xA59FAE: return Draygon_Instr_18(k, jp);
  case 0xA5C47B: return Draygon_Instr_3(k, jp);
  case 0xA5E75F: return Draygon_Instr_25(k, jp);
  case 0xA5E771: return Draygon_Instr_24(k, jp);
  case 0xA5E82D: return Draygon_Instr_21(k, jp);
  case 0xA5E872: return Draygon_Instr_22(k, jp);
  case 0xA5E87C: return Draygon_Instr_27(k, jp);
  case 0xA5E895: return Draygon_Instr_23(k, jp);
  case 0xA5E8B1: return Draygon_Instr_30(k, jp);
  case 0xA5E8BA: return Draygon_Instr_20(k, jp);
  case 0xA5E8CA: return Draygon_Instr_29(k, jp);
  case 0xA5E91C: return Draygon_Instr_19(k, jp);
  case 0xA5E96E: return Draygon_Instr_28(k, jp);
  case 0xA5E9B1: return Draygon_Instr_26(k, jp);
  case 0xA680ED: return EnemyInstr_Goto(k, jp);
  case 0xA69B26: return FakeKraid_Instr_2(k, jp);
  case 0xA69B74: return FakeKraid_Instr_1(k, jp);
  case 0xA69BB2: return FakeKraid_Instr_3(k, jp);
  case 0xA69BC4: return FakeKraid_Instr_4(k, jp);
  case 0xA69C02: return FakeKraid_Instr_5(k, jp);
  case 0xA780ED: return EnemyInstr_Goto(k, jp);
  case 0xA78110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xA78123: return EnemyInstr_SetTimer(k, jp);
  case 0xA880ED: return EnemyInstr_Goto(k, jp);
  case 0xA88110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xA88123: return EnemyInstr_SetTimer(k, jp);
  case 0xA88173: return EnemyInstr_EnableOffScreenProcessing(k, jp);
  case 0xA8817D: return EnemyInstr_DisableOffScreenProcessing(k, jp);
  case 0xA8A0C7: return YappingMaw_Instr_2(k, jp);
  case 0xA8A0D9: return YappingMaw_Instr_4(k, jp);
  case 0xA8A0EB: return YappingMaw_Instr_5(k, jp);
  case 0xA8A0FD: return YappingMaw_Instr_7(k, jp);
  case 0xA8A10F: return YappingMaw_Instr_3(k, jp);
  case 0xA8A121: return YappingMaw_Instr_6(k, jp);
  case 0xA8A133: return YappingMaw_Instr_1(k, jp);
  case 0xA8B75E: return Beetom_Instr_1(k, jp);
  case 0xA8DF1C: return WalkingLavaSeahorse_Instr_4(k, jp);
  case 0xA8DF33: return WalkingLavaSeahorse_Instr_3(k, jp);
  case 0xA8DF39: return WalkingLavaSeahorse_Instr_5(k, jp);
  case 0xA8DF3F: return WalkingLavaSeahorse_Instr_6(k, jp);
  case 0xA8DF63: return WalkingLavaSeahorse_Instr_2(k, jp);
  case 0xA8DF71: return WalkingLavaSeahorse_Instr_1(k, jp);
  case 0xA8F526: return KiHunter_Instr_1(k, jp);
  case 0xA8F5E4: return KiHunter_Instr_2(k, jp);
  case 0xA8F67F: return KiHunter_Instr_3(k, jp);
  case 0xA8F6D2: return KiHunter_Instr_4(k, jp);
  case 0xA8F6D8: return KiHunter_Instr_5(k, jp);
  case 0xAA806B: return Enemy_SetAiPreInstr_AA(k, jp);
  case 0xAA8074: return Enemy_ClearAiPreInstr_AA(k, jp);
  case 0xAA807C: return EnemyInstr_StopScript(k, jp);
  case 0xAA80ED: return EnemyInstr_Goto(k, jp);
  case 0xAA8110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xAA8123: return EnemyInstr_SetTimer(k, jp);
  case 0xAAB09C: return Torizo_Instr_3(k, jp);
  case 0xAAB11D: return Torizo_Instr_31(k, jp);
  case 0xAAB1BE: return Torizo_Instr_33(k, jp);
  case 0xAAB224: return Torizo_Instr_36(k, jp);
  case 0xAAB22E: return Torizo_Instr_37(k, jp);
  case 0xAAB238: return Torizo_Instr_35(k, jp);
  case 0xAAB24D: return Torizo_Instr_38(k, jp);
  case 0xAAB271: return Torizo_Instr_6(k, jp);
  case 0xAAB94D: return Torizo_Instr_5(k, jp);
  case 0xAAB951: return Torizo_Instr_9(k, jp);
  case 0xAAC2C8: return Torizo_Instr_7(k, jp);
  case 0xAAC2C9: return Torizo_Instr_2(k, jp);
  case 0xAAC2D1: return Torizo_Instr_8(k, jp);
  case 0xAAC2D9: return Torizo_Instr_25(k, jp);
  case 0xAAC2ED: return Torizo_Instr_22(k, jp);
  case 0xAAC2F7: return Torizo_Instr_19(k, jp);
  case 0xAAC2FD: return Torizo_Instr_32(k, jp);
  case 0xAAC303: return Torizo_Instr_30(k, jp);
  case 0xAAC32F: return Torizo_Instr_34(k, jp);
  case 0xAAC34A: return Torizo_Instr_24(k, jp);
  case 0xAAC35B: return Torizo_Instr_12(k, jp);
  case 0xAAC36D: return Torizo_Instr_10(k, jp);
  case 0xAAC377: return Torizo_Instr_11(k, jp);
  case 0xAAC38A: return Torizo_Instr_29(k, jp);
  case 0xAAC3A0: return Torizo_Instr_1(k, jp);
  case 0xAAC3B6: return Torizo_Instr_28(k, jp);
  case 0xAAC3CC: return Torizo_Instr_4(k, jp);
  case 0xAAC41E: return Torizo_Instr_40(k, jp);
  case 0xAAC470: return Torizo_Instr_16(k, jp);
  case 0xAAC4E5: return Torizo_Instr_27(k, jp);
  case 0xAAC55A: return Torizo_Instr_23(k, jp);
  case 0xAAC567: return Torizo_Instr_14(k, jp);
  case 0xAAC58B: return Torizo_Instr_15(k, jp);
  case 0xAAC5A4: return Torizo_Instr_26(k, jp);
  case 0xAAC5CB: return Torizo_Instr_18(k, jp);
  case 0xAAC5E3: return Torizo_Instr_20(k, jp);
  case 0xAAC5F2: return Torizo_Instr_44(k, jp);
  case 0xAAC601: return Torizo_Instr_21(k, jp);
  case 0xAAC610: return Torizo_Instr_17(k, jp);
  case 0xAAC618: return Torizo_Instr_13(k, jp);
  case 0xAACACE: return Torizo_Instr_39(k, jp);
  case 0xAACADE: return Torizo_Instr_41(k, jp);
  case 0xAACAE2: return Torizo_Instr_42(k, jp);
  case 0xAACDD7: return Torizo_Instr_48(k, jp);
  case 0xAAD0E9: return Torizo_Instr_57(k, jp);
  case 0xAAD0F3: return Torizo_Instr_58(k, jp);
  case 0xAAD17B: return Torizo_Instr_59(k, jp);
  case 0xAAD187: return Torizo_Instr_62(k, jp);
  case 0xAAD1E7: return Torizo_Instr_63(k, jp);
  case 0xAAD38F: return Torizo_Instr_56(k, jp);
  case 0xAAD397: return Torizo_Instr_60(k, jp);
  case 0xAAD39F: return Torizo_Instr_46(k, jp);
  case 0xAAD3E0: return Torizo_Instr_47(k, jp);
  case 0xAAD3EA: return Torizo_Instr_49(k, jp);
  case 0xAAD436: return Torizo_Instr_61(k, jp);
  case 0xAAD445: return Torizo_Instr_53(k, jp);
  case 0xAAD474: return Torizo_Instr_55(k, jp);
  case 0xAAD49B: return Torizo_Instr_52(k, jp);
  case 0xAAD4BA: return Torizo_Instr_50(k, jp);
  case 0xAAD4F3: return Torizo_Instr_43(k, jp);
  case 0xAAD4FD: return Torizo_Instr_51(k, jp);
  case 0xAAD526: return Torizo_Instr_45(k, jp);
  case 0xAAD54D: return Torizo_Instr_54(k, jp);
  case 0xB280ED: return EnemyInstr_Goto(k, jp);
  case 0xB28110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xB28123: return EnemyInstr_SetTimer(k, jp);
  case 0xB2EE40: return SpacePirates_Instr_MovePixelsDownAndChangeDirFaceRight(k, jp);
  case 0xB2EE72: return SpacePirates_Instr_MovePixelsDownAndChangeDirFaceLeft(k, jp);
  case 0xB2EEA4: return SpacePirates_Instr_RandomNewDirFaceR(k, jp);
  case 0xB2EEBC: return SpacePirates_Instr_RandomNewDirFaceL(k, jp);
  case 0xB2EED4: return SpacePirates_Instr_PrepareWallJumpR(k, jp);
  case 0xB2EEFD: return SpacePirates_Instr_PrepareWallJumpL(k, jp);
  case 0xB2EF2A: return SpacePirates_Instr_FireLaserL(k, jp);
  case 0xB2EF5D: return SpacePirates_Instr_FireLaserR(k, jp);
  case 0xB2EF83: return SpacePirates_Instr_SetEnemyFunc(k, jp);
  case 0xB2EF93: return SpacePirates_Instr_PlaySfx(k, jp);
  case 0xB2F536: return SpacePirates_Instr_20(k, jp);
  case 0xB2F546: return SpacePirates_Instr_16(k, jp);
  case 0xB2F564: return SpacePirates_Instr_15(k, jp);
  case 0xB2F590: return SpacePirates_Instr_18(k, jp);
  case 0xB2F5D6: return SpacePirates_Instr_17(k, jp);
  case 0xB2F969: return SpacePirates_Instr_19(k, jp);
  case 0xB2FA3D: return SpacePirates_Instr_21(k, jp);
  case 0xB2FC68: return SpacePirates_Instr_12(k, jp);
  case 0xB2FC90: return SpacePirates_Instr_14(k, jp);
  case 0xB2FCB8: return SpacePirates_Instr_11(k, jp);
  case 0xB2FCC8: return SpacePirates_Instr_13(k, jp);
  case 0xB3806B: return Enemy_SetAiPreInstr_B3(k, jp);
  case 0xB38074: return Enemy_ClearAiPreInstr_B3(k, jp);
  case 0xB380ED: return EnemyInstr_Goto(k, jp);
  case 0xB38110: return EnemyInstr_DecTimerAndGoto(k, jp);
  case 0xB38123: return EnemyInstr_SetTimer(k, jp);
  case 0xB3E545: return EscapeEtecoon_Instr_1(k, jp);
  case 0xB3E610: return EscapeEtecoon_Instr_2(k, jp);
  case 0xB3EAA8: return EscapeDachora_Instr_2(k, jp);
  case 0xB3EAB8: return EscapeDachora_Instr_3(k, jp);
  case 0xB3EAC9: return EscapeDachora_Instr_1(k, jp);
  case 0xB3EAD7: return EscapeDachora_Instr_4(k, jp);
  }
  *ok = 0;
  return 0;
}

static const struct { uint32 ea; PortFnK f; } kPortCallEnemyPreInstr[76] = {
  {0xA0807B, port_nop_k},
  {0xA2807B, port_nop_k},
  {0xA28A75, port_nop_k},
  {0xA29FBA, SpikeShootingPlant_2},
  {0xA29FEC, SpikeShootingPlant_3},
  {0xA2A01B, port_nop_k},
  {0xA2A7D7, port_nop_k},
  {0xA2B14E, Flies_4},
  {0xA2B17C, Flies_5},
  {0xA2B1AA, Flies_6},
  {0xA2B1D2, Flies_7},
  {0xA2E781, port_nop_k},
  {0xA3CF5F, port_nop_k},
  {0xA3E08A, port_nop_k},
  {0xA3E091, WreckedShipOrangeZoomer_Func_2},
  {0xA3E168, sub_A3E168},
  {0xA3E6C1, port_nop_k},
  {0xA3E6C8, FireZoomer_Func_1},
  {0xA3E785, FireZoomer_Func_2},
  {0xA3E7F2, FireZoomer_Func_3},
  {0xA7D4A8, port_nop_k},
  {0xA7E9AF, Etecoon_Func_4},
  {0xA7EA00, Etecoon_Func_5},
  {0xA7EA37, Etecoon_Func_6},
  {0xA7EAB5, Etecoon_Func_7},
  {0xA7EB02, Etecoon_Func_8},
  {0xA7EB2C, Etecoon_Func_9},
  {0xA7EB50, Etecoon_Func_10},
  {0xA7EBCD, Etecoon_Func_11},
  {0xA7EC1B, Etecoon_Func_12},
  {0xA7EC97, Etecoon_Func_16},
  {0xA7ECBB, Etecoon_Func_17},
  {0xA7ECDF, Etecoon_Func_18},
  {0xA7ED09, Etecoon_Func_19},
  {0xA7ED2A, Etecoon_Func_20},
  {0xA7ED54, Etecoon_Func_21},
  {0xA7ED75, Etecoon_Func_22},
  {0xA7EDC7, Etecoon_Func_23},
  {0xA7EE3E, Etecoon_Func_24},
  {0xA7EE9A, Etecoon_Func_25},
  {0xA7EEB8, Etecoon_Func_26},
  {0xA7F570, Dachora_Func_2},
  {0xA7F5BC, Dachora_Func_3},
  {0xA7F5ED, Dachora_Func_4},
  {0xA7F65E, Dachora_Func_5},
  {0xA7F78F, Dachora_Func_7},
  {0xA7F806, Dachora_Func_8},
  {0xA7F935, Dachora_Func_11},
  {0xA7F98C, Dachora_Func_12},
  {0xA890F1, MorphBallEye_Func_1},
  {0xA8912E, MorphBallEye_Func_2},
  {0xA89160, MorphBallEye_Func_3},
  {0xA891CE, MorphBallEye_Func_4},
  {0xA891DC, port_nop_k},
  {0xA8C568, port_nop_k},
  {0xAA807B, port_nop_k},
  {0xAAC752, Torizo_Func_5},
  {0xAAC828, Torizo_Func_6},
  {0xAAC82C, Torizo_Func_7},
  {0xAAC95E, port_nop_k},
  {0xAAD5ED, Torizo_D5ED},
  {0xAAD5F1, Torizo_D5F1},
  {0xAADCAA, port_nop_k},
  {0xAADCAB, port_nop_k},
  {0xAAE7A6, port_nop_k},
  {0xAAE7DA, port_nop_k},
  {0xB3807B, port_nop_k},
  {0xB38880, BrinstarPipeBug_PreInstr_1},
  {0xB38890, BrinstarPipeBug_PreInstr_2},
  {0xB388E3, BrinstarPipeBug_PreInstr_3},
  {0xB3891C, BrinstarPipeBug_PreInstr_4},
  {0xB3897E, BrinstarPipeBug_PreInstr_5},
  {0xB38CFF, NorfairPipeBug_Func_5},
  {0xB3E65C, EscapeEtecoon_E65C},
  {0xB3E670, EscapeEtecoon_E670},
  {0xB3E680, EscapeEtecoon_E680}
};
PortFnK port_FindCallEnemyPreInstr(uint32 ea) {  /* 0: no está traducida; las últimas, recordadas */
  static uint32 cea[16]; static PortFnK cf[16];
  uint16 h = ((uint16)ea ^ ((uint16)ea >> 4)) & 15;
  if (cea[h] == ea) return cf[h];
  PortFnK f = 0;
  int lo = 0, hi = 75;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    uint32 e = kPortCallEnemyPreInstr[m].ea;
    if (e == ea) { f = kPortCallEnemyPreInstr[m].f; break; }
    if (e < ea) lo = m + 1; else hi = m - 1;
  }
  cea[h] = ea; cf[h] = f;
  return f;
}
uint8 port_CallEnemyPreInstr(uint32 ea) {  /* 0: no está traducida */
  PortFnK f = port_FindCallEnemyPreInstr(ea);
  if (!f) return 0;
  f(cur_enemy_index);
  return 1;
}

static const struct { uint32 ea; PortFnK f; } kPortCallEprojInit[45] = {
  {0x868ACD, Eproj_Init_0x8bc2_SkreeDownRight},
  {0x868AF1, Eproj_Init_0x8bd0_SkreeUpRight},
  {0x868B15, Eproj_Init_0x8bde_SkreeDownLeft},
  {0x868B39, Eproj_Init_0x8bec_SkreeUpLeft},
  {0x868D04, EprojInit_DraygonsGunk},
  {0x869DEC, EprojInit_MiniKraidSpit},
  {0x869E46, EprojInit_MiniKraidSpikesLeft},
  {0x869E4B, EprojInit_MiniKraidSpikesRight},
  {0x869EB2, EprojInit_WalkingLavaSeahorseFireball},
  {0x86A009, EprojInit_PirateMotherBrainLaser},
  {0x86A098, EprojInit_PirateClaw},
  {0x86A5D3, EprojInit_BombTorizoLowHealthDrool},
  {0x86A65D, EprojInit_BombTorizoLowHealthInitialDrool},
  {0x86A6F6, EprojInit_BombTorizoExplosiveSwipe},
  {0x86A764, EprojInit_BombTorizoStatueBreaking},
  {0x86A81B, EprojInit_BombTorizoLowHealthExplode},
  {0x86A871, EprojInit_BombTorizoDeathExplosion},
  {0x86ABEB, EprojInit_BombTorizosChozoOrbs},
  {0x86AC7C, EprojInit_GoldenTorizosChozoOrbs},
  {0x86AE15, EprojInit_TorizoSonicBoom},
  {0x86AF43, EprojInit_TourianStatueDustClouds},
  {0x86AF50, EprojInit_TourianLandingDustCloudsRightFoot},
  {0x86AFCD, EprojInit_TorizoLandingDustCloudLeftFoot},
  {0x86B001, EprojInit_GoldenTorizoEgg},
  {0x86B1CE, EprojInit_GoldenTorizoSuperMissile},
  {0x86B328, EprojInit_GoldenTorizoEyeBeam},
  {0x86B87A, EprojInit_TourianStatueUnlockingParticleWaterSplash},
  {0x86B88E, EprojInit_TourianStatueEyeGlow},
  {0x86B8B5, EprojInit_TourianStatueUnlockingParticle},
  {0x86B8F8, EprojInit_TourianStatueSoul},
  {0x86B93E, EprojInit_TourianStatueBaseDecoration},
  {0x86B951, EprojInit_TourianStatueRidley},
  {0x86B964, EprojInit_TourianStatuePhantoon},
  {0x86C684, EprojInit_MotherBrainDeathBeemFired},
  {0x86CF90, EprojInit_KiHunterAcidSpitLeft},
  {0x86CFA6, EprojInit_KiHunterAcidSpitRight},
  {0x86D088, EprojInit_KagosBugs},
  {0x86D992, sub_86D992},
  {0x86DC8D, EprojInit_Spores},
  {0x86DCA3, EprojInit_SporeSpawnStalk},
  {0x86DCD4, EprojInit_SporeSpawners},
  {0x86E468, EprojInit_DustCloudOrExplosion},
  {0x86EC62, EprojInit_YappingMawsBody},
  {0x86EF29, EprojInit_F337},
  {0x86EF89, EprojInit_EnemyDeathExplosion}
};
PortFnK port_FindCallEprojInit(uint32 ea) {  /* 0: no está traducida; las últimas, recordadas */
  static uint32 cea[16]; static PortFnK cf[16];
  uint16 h = ((uint16)ea ^ ((uint16)ea >> 4)) & 15;
  if (cea[h] == ea) return cf[h];
  PortFnK f = 0;
  int lo = 0, hi = 44;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    uint32 e = kPortCallEprojInit[m].ea;
    if (e == ea) { f = kPortCallEprojInit[m].f; break; }
    if (e < ea) lo = m + 1; else hi = m - 1;
  }
  cea[h] = ea; cf[h] = f;
  return f;
}
uint8 port_CallEprojInit(uint32 ea, uint16 j) {  /* 0: no está traducida */
  PortFnK f = port_FindCallEprojInit(ea);
  if (!f) return 0;
  f(j);
  return 1;
}

static const struct { uint32 ea; PortFnK f; } kPortCallEprojPreInstr[64] = {
  {0x868170, port_nop_k},
  {0x8684FB, port_nop_k},
  {0x868B5D, Eproj_PreInstr_SkreeParticle},
  {0x868D54, port_nop_k},
  {0x868E0F, EprojPreInstr_DraygonsGunk_8E0F},
  {0x86950C, port_nop_k},
  {0x869A44, port_nop_k},
  {0x869E1E, EprojPreInit_MiniKraidSpit},
  {0x869E83, EprojPreInstr_MiniKraidSpikes},
  {0x869EFF, EprojPreInstr_WalkingLavaSeahorseFireball},
  {0x86A05B, port_nop_k},
  {0x86A05C, EprojPreInstr_PirateMotherBrainLaser_MoveLeft},
  {0x86A07A, EprojPreInstr_PirateMotherBrainLaser_MoveRight},
  {0x86A327, port_nop_k},
  {0x86A887, sub_86A887},
  {0x86A8EF, EprojPreInstr_A977},
  {0x86A918, port_nop_k},
  {0x86A919, port_nop_k},
  {0x86ACAD, EprojPreInstr_BombTorizosChozoOrbs},
  {0x86ACFA, EprojPreInstr_GoldenTorizosChozoOrbs},
  {0x86AE6C, EprojPreInstr_TorizoSonicBoom},
  {0x86B043, EprojPreInstr_GoldenTorizoEgg},
  {0x86B20D, EprojPreInstr_GoldenTorizoSuperMissile},
  {0x86B38A, EprojPreInstr_GoldenTorizoEyeBeam},
  {0x86B4B0, port_nop_k},
  {0x86BA37, EprojPreInstr_TourianStatueStuff},
  {0x86BA42, EprojPreInstr_BA42},
  {0x86BB4F, port_nop_k},
  {0x86BBC6, port_nop_k},
  {0x86C76D, port_nop_k},
  {0x86CAA3, port_nop_k},
  {0x86CFD5, sub_86CFD5},
  {0x86CFE6, sub_86CFE6},
  {0x86CFF7, port_nop_k},
  {0x86CFF8, sub_86CFF8},
  {0x86D0B3, EprojPreInstr_KagosBugs_Func1},
  {0x86D0CA, EprojPreInstr_KagosBugs},
  {0x86D0EB, port_nop_k},
  {0x86D0EC, EprojPreInstr_D0EC},
  {0x86D128, EprojPreInstr_D128},
  {0x86D1E4, EprojPreInstr_KagosBugs_Func2},
  {0x86D9DB, EprojPreInstr_SpikeShootingPlantSpikes},
  {0x86D9E6, EprojPreInstr_SpikeShootingPlantSpikes_MoveY1},
  {0x86DA10, EprojPreInstr_SpikeShootingPlantSpikes_MoveY2},
  {0x86DA3A, EprojPreInstr_SpikeShootingPlantSpikes_MoveX1},
  {0x86DA64, EprojPreInstr_SpikeShootingPlantSpikes_MoveX2},
  {0x86DA8E, EprojPreInstr_SpikeShootingPlantSpikes_0_MoveX1},
  {0x86DA93, EprojPreInstr_SpikeShootingPlantSpikes_2_MoveX2},
  {0x86DA98, EprojPreInstr_SpikeShootingPlantSpikes_1_MoveY1},
  {0x86DA9D, EprojPreInstr_SpikeShootingPlantSpikes_4_MoveY2},
  {0x86DAA2, EprojPreInstr_SpikeShootingPlantSpikes_6_MoveX1Y1},
  {0x86DAAA, EprojPreInstr_SpikeShootingPlantSpikes_8_MoveX1Y2},
  {0x86DAB2, EprojPreInstr_SpikeShootingPlantSpikes_7_MoveX2Y1},
  {0x86DABA, EprojPreInstr_SpikeShootingPlantSpikes_9_MoveX2Y2},
  {0x86DCEE, EprojPreInstr_Spores},
  {0x86DD44, port_nop_k},
  {0x86DD46, EprojPreInstr_SporeSpawners},
  {0x86E4FE, EprojPreInstr_DustCloudOrExplosion},
  {0x86E508, port_nop_k},
  {0x86E604, port_nop_k},
  {0x86E6D1, port_nop_k},
  {0x86EC94, port_nop_k},
  {0x86EFDF, port_nop_k},
  {0x86EFE0, EprojPreInstr_Pickup}
};
PortFnK port_FindCallEprojPreInstr(uint32 ea) {  /* 0: no está traducida; las últimas, recordadas */
  static uint32 cea[16]; static PortFnK cf[16];
  uint16 h = ((uint16)ea ^ ((uint16)ea >> 4)) & 15;
  if (cea[h] == ea) return cf[h];
  PortFnK f = 0;
  int lo = 0, hi = 63;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    uint32 e = kPortCallEprojPreInstr[m].ea;
    if (e == ea) { f = kPortCallEprojPreInstr[m].f; break; }
    if (e < ea) lo = m + 1; else hi = m - 1;
  }
  cea[h] = ea; cf[h] = f;
  return f;
}
uint8 port_CallEprojPreInstr(uint32 ea, uint16 k) {  /* 0: no está traducida */
  PortFnK f = port_FindCallEprojPreInstr(ea);
  if (!f) return 0;
  f(k);
  return 1;
}

uint16 port_CallEprojInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok) {  /* ok = 0: no está traducida */
  *ok = 1;
  switch (ea) {
  case 0x868154: return EprojInstr_Delete(k, jp);
  case 0x868159: return EprojInstr_Sleep(k, jp);
  case 0x868161: return EprojInstr_SetPreInstr_(k, jp);
  case 0x86816A: return EprojInstr_ClearPreInstr(k, jp);
  case 0x868171: return EprojInstr_CallFunc(k, jp);
  case 0x8681AB: return EprojInstr_Goto(k, jp);
  case 0x8681B0: return EprojInstr_GotoRel(k, jp);
  case 0x8681C6: return EprojInstr_DecTimerAndGotoIfNonZero(k, jp);
  case 0x8681CE: return EprojInstr_DecTimerAndGotoRelIfNonZero(k, jp);
  case 0x8681D5: return EprojInstr_SetTimer(k, jp);
  case 0x8681DF: return EprojInstr_MoveRandomlyWithinRadius(k, jp);
  case 0x868230: return EprojInstr_SetProjectileProperties(k, jp);
  case 0x86823C: return EprojInstr_ClearProjectileProperties(k, jp);
  case 0x868248: return EprojInstr_EnableCollisionWithSamusProj(k, jp);
  case 0x868252: return EprojInstr_DisableCollisionWithSamusProj(k, jp);
  case 0x86825C: return EprojInstr_DisableCollisionWithSamus(k, jp);
  case 0x868266: return EprojInstr_EnableCollisionWithSamus(k, jp);
  case 0x868270: return EprojInstr_SetToNotDieOnContact(k, jp);
  case 0x86827A: return EprojInstr_SetToDieOnContact(k, jp);
  case 0x868284: return EprojInstr_SetLowPriority(k, jp);
  case 0x86828E: return EprojInstr_SetHighPriority(k, jp);
  case 0x868298: return EprojInstr_SetXyRadius(k, jp);
  case 0x8682A1: return EprojInstr_SetXyRadiusZero(k, jp);
  case 0x8682A5: return EprojInstr_CalculateDirectionTowardsSamus(k, jp);
  case 0x8682D5: return EprojInstr_WriteColorsToPalette(k, jp);
  case 0x8682FD: return EprojInstr_QueueMusic(k, jp);
  case 0x868309: return EprojInstr_QueueSfx1_Max6(k, jp);
  case 0x868312: return EprojInstr_QueueSfx2_Max6(k, jp);
  case 0x86831B: return EprojInstr_QueueSfx3_Max6(k, jp);
  case 0x868324: return EprojInstr_QueueSfx1_Max15(k, jp);
  case 0x86832D: return EprojInstr_QueueSfx2_Max15(k, jp);
  case 0x868336: return EprojInstr_QueueSfx3_Max15(k, jp);
  case 0x86833F: return EprojInstr_QueueSfx1_Max3(k, jp);
  case 0x868348: return EprojInstr_QueueSfx2_Max3(k, jp);
  case 0x868351: return EprojInstr_QueueSfx3_Max3(k, jp);
  case 0x86835A: return EprojInstr_QueueSfx1_Max9(k, jp);
  case 0x868363: return EprojInstr_QueueSfx2_Max9(k, jp);
  case 0x86836C: return EprojInstr_QueueSfx3_Max9(k, jp);
  case 0x868375: return EprojInstr_QueueSfx1_Max1(k, jp);
  case 0x86837E: return EprojInstr_QueueSfx2_Max1(k, jp);
  case 0x868387: return EprojInstr_QueueSfx3_Max1(k, jp);
  case 0x86A050: return EprojInstr_SetPreInstrAndRun(k, jp);
  case 0x86A05C: EprojPreInstr_PirateMotherBrainLaser_MoveLeft(k); return jp;
  case 0x86A07A: EprojPreInstr_PirateMotherBrainLaser_MoveRight(k); return jp;
  case 0x86A3BE: return EprojInstr_A3BE(k, jp);
  case 0x86A456: return EprojInstr_GotoWithProbability25(k, jp);
  case 0x86AB8A: return EprojInstr_SpawnEnemyDrops(k, jp);
  case 0x86AF92: return EprojInstr_MoveY_Minus4(k, jp);
  case 0x86B269: return EprojInstr_SetVelTowardsSamus1(k, jp);
  case 0x86D15C: return EprojInstr_D15C(k, jp);
  case 0x86D1B6: return EprojInstr_D1B6(k, jp);
  case 0x86DC5A: return EprojInstr_DC5A(k, jp);
  case 0x86DC61: return EprojInstr_SpawnEnemyDrops_0(k, jp);
  case 0x86DC77: return EprojInstr_SpawnSporesEproj(k, jp);
  case 0x86ECE3: return EprojInstr_ECE3(k, jp);
  case 0x86ED17: return EprojInstr_ED17(k, jp);
  case 0x86EE8B: return EprojInstr_QueueSfx2_9(k, jp);
  case 0x86EE97: return EprojInstr_QueueSfx2_24(k, jp);
  case 0x86EEA3: return EprojInstr_QueueSfx2_B(k, jp);
  case 0x86EEAF: return EprojInstr_EEAF(k, jp);
  case 0x86EF10: return EprojInstr_HandleRespawningEnemy(k, jp);
  }
  *ok = 0;
  return 0;
}

void CallEnemyPreInstr(uint32 ea) { if (!port_CallEnemyPreInstr(ea)) { nt_unimpl_v("CallEnemyPreInstr", (uint16)(ea >> 8)); nt_unimpl_v("CallEnemyPreInstr lo", (uint16)ea); } }

uint8 port_call_void(uint32 ea) {  /* cualquier función void(void) traducida, por su dirección */
  switch (ea) {
  case 0x86F0BB: Eproj_Pickup_SmallHealth(); return 1;
  case 0x86F0CA: Eproj_Pickup_BigHealth(); return 1;
  case 0x86F0D9: Eproj_Pickup_PowerBombs(); return 1;
  case 0x86F0E8: Eproj_Pickup_Missiles(); return 1;
  case 0x86F0F7: Eproj_Pickup_SuperMissiles(); return 1;
  case 0xA08046: CreateADudShot(); return 1;
  case 0xA0957E: NormalEnemyFrozenAI(); return 1;
  case 0xA0A477: NormalEnemyTouchAi(); return 1;
  case 0xA0A497: NormalEnemyTouchAiSkipDeathAnim_CurEnemy(); return 1;
  case 0xA0A597: NormalEnemyPowerBombAi(); return 1;
  case 0xA0A5C1: NormalEnemyPowerBombAiSkipDeathAnim(); return 1;
  case 0xA0A63D: NormalEnemyShotAi(); return 1;
  case 0xA0A6A7: NormalEnemyShotAiSkipDeathAnim_CurEnemy(); return 1;
  case 0xA0A6B4: EnemyFunc_A6B4_UsedBySporeSpawn(); return 1;
  case 0xA0A8BC: CreateDudShot(); return 1;
  case 0xA0B7A1: EnemyFunc_B7A1(); return 1;
  case 0xA28041: Enemy_NormalFrozenAI_A2(); return 1;
  case 0xA28023: Enemy_NormalTouchAI_A2(); return 1;
  case 0xA2802D: Enemy_NormalShotAI_A2(); return 1;
  case 0xA2871C: BouncingGoofball_Init(); return 1;
  case 0xA2879C: BouncingGoofball_Main(); return 1;
  case 0xA28801: BouncingGoofball_State0(); return 1;
  case 0xA28850: BouncingGoofball_State1(); return 1;
  case 0xA2889F: BouncingGoofball_Func2(); return 1;
  case 0xA288B2: BouncingGoofball_Func3(); return 1;
  case 0xA29A3F: ThinHoppingBlobs_Init(); return 1;
  case 0xA29A7D: ThinHoppingBlobs_Main(); return 1;
  case 0xA29A9B: TwinHoppingBlobs_Func2(); return 1;
  case 0xA29AAA: ThinHoppingBlobs_Func3(); return 1;
  case 0xA29B1A: ThinHoppingBlobs_Func5(); return 1;
  case 0xA29B81: ThinHoppingBlobs_Func7(); return 1;
  case 0xA29B88: ThinHoppingBlobs_Func8(); return 1;
  case 0xA29C71: ThinHoppingBlobs_Func9(); return 1;
  case 0xA29CBE: ThinHoppingBlobs_Func10(); return 1;
  case 0xA29D0B: ThinHoppingBlobs_Func11(); return 1;
  case 0xA29D2B: ThinHoppingBlobs_Func12(); return 1;
  case 0xA29D4B: ThinHoppingBlobs_Func13(); return 1;
  case 0xA29D6B: ThinHoppingBlobs_Func14(); return 1;
  case 0xA29D98: ThinHoppingBlobs_Func15(); return 1;
  case 0xA29DCD: ThinHoppingBlobs_Func16(); return 1;
  case 0xA29F48: SpikeShootingPlant_Init(); return 1;
  case 0xA29FB3: SpikeShootingPlant_Main(); return 1;
  case 0xA2A01C: SpikeShootingPlant_4(); return 1;
  case 0xA2A049: SpikeShootingPlant_5(); return 1;
  case 0xA2A05C: SpikeShootingPlant_6(); return 1;
  case 0xA2A06F: SpikeShootingPlant_7(); return 1;
  case 0xA2A082: SpikeShootingPlant_8(); return 1;
  case 0xA2B06B: Flies_Init(); return 1;
  case 0xA2B11F: Flies_Main(); return 1;
  case 0xA2BBCD: Rio_Init(); return 1;
  case 0xA2BBE3: Rio_Main(); return 1;
  case 0xA2DF76: HirisingSlowfalling_Init(); return 1;
  case 0xA2E02E: HirisingSlowfalling_Main(); return 1;
  case 0xA2E035: HirisingSlowfalling_Func_4(); return 1;
  case 0xA2E04F: HirisingSlowfalling_Func_5(); return 1;
  case 0xA2E06A: HirisingSlowfalling_Func_6(); return 1;
  case 0xA2E0CD: HirisingSlowfalling_Func_7(); return 1;
  case 0xA2E1D3: Gripper_Init(); return 1;
  case 0xA2E221: Gripper_Main(); return 1;
  case 0xA2E29B: Gripper_Func_3(); return 1;
  case 0xA2E2A4: Gripper_Func_4(); return 1;
  case 0xA2E318: JetPowerRipper_Init(); return 1;
  case 0xA2E353: JetPowerRipper_Main(); return 1;
  case 0xA2E3A0: Ripper_Func_1(); return 1;
  case 0xA2E3A9: JetPowerRipper_Shot(); return 1;
  case 0xA2E49F: Ripper_Init(); return 1;
  case 0xA2E4DA: Ripper_Main(); return 1;
  case 0xA2E9DA: TimedShutter_Init(); return 1;
  case 0xA2EAB6: TimedShutter_Main(); return 1;
  case 0xA2EB25: TimedShutter_Func_6(); return 1;
  case 0xA2EB66: TimedShutter_Func_7(); return 1;
  case 0xA2EBA7: TimedShutter_Func_8(); return 1;
  case 0xA2EBE8: TimedShutter_Func_9(); return 1;
  case 0xA2EC45: TimedShutter_Func_11(); return 1;
  case 0xA2EC86: TimedShutter_Func_12(); return 1;
  case 0xA2ECC7: TimedShutter_Func_13(); return 1;
  case 0xA2ED08: TimedShutter_Func_14(); return 1;
  case 0xA2ED33: TimedShutter_Func_15(); return 1;
  case 0xA2EE05: RisingFallingPlatform_Init(); return 1;
  case 0xA2EE12: ShootableShutter_Init(); return 1;
  case 0xA2EED1: RisingFallingPlatform_Main(); return 1;
  case 0xA2EF15: RisingFallingPlatform_Func_3(); return 1;
  case 0xA2EF28: RisingFallingPlatform_Func_4(); return 1;
  case 0xA2EF39: RisingFallingPlatform_Func_5(); return 1;
  case 0xA2EF40: RisingFallingPlatform_Func_6(); return 1;
  case 0xA2EF5A: RisingFallingPlatform_Func_8(); return 1;
  case 0xA2EF68: RisingFallingPlatform_Func_9(); return 1;
  case 0xA2EFD4: RisingFallingPlatform_Func_10(); return 1;
  case 0xA2F040: RisingFallingPlatform_Func_11(); return 1;
  case 0xA2F072: RisingFallingPlatform_Func_12(); return 1;
  case 0xA2F099: RisingFallingPlatform_Func_13(); return 1;
  case 0xA2F09D: RisingFallingPlatform_Touch(); return 1;
  case 0xA2F0A2: RisingFallingPlatform_Shot(); return 1;
  case 0xA2F0AA: ShootableShutter_Shot(); return 1;
  case 0xA2F0B6: RisingFallingPlatform_Powerbomb(); return 1;
  case 0xA2F230: HorizontalShootableShutter_Func_3(); return 1;
  case 0xA2F243: HorizontalShootableShutter_Func_4(); return 1;
  case 0xA2F254: HorizontalShootableShutter_Func_5(); return 1;
  case 0xA2F25B: HorizontalShootableShutter_Func_6(); return 1;
  case 0xA2F272: HorizontalShootableShutter_Func_8(); return 1;
  case 0xA2F2E4: HorizontalShootableShutter_Func_9(); return 1;
  case 0xA2F38C: HorizontalShootableShutter_Func_12(); return 1;
  case 0xA2F3B0: HorizontalShootableShutter_Func_13(); return 1;
  case 0xA2F3D4: HorizontalShootableShutter_Func_14(); return 1;
  case 0xA38023: Enemy_NormalTouchAI_A3(); return 1;
  case 0xA3802D: Enemy_NormalShotAI_A3(); return 1;
  case 0xA38032: Enemy_NormalShotAI_SkipSomeParts_A3(); return 1;
  case 0xA38037: Enemy_NormalPowerBombAI_A3(); return 1;
  case 0xA38041: Enemy_NormalFrozenAI_A3(); return 1;
  case 0xA386ED: Waver_Init(); return 1;
  case 0xA3874C: Waver_Main(); return 1;
  case 0xA387FE: Waver_Func_1(); return 1;
  case 0xA38960: Metalee_Init(); return 1;
  case 0xA38979: Metalee_Main(); return 1;
  case 0xA38987: Metalee_Func_1(); return 1;
  case 0xA389F3: Metalee_Func_4(); return 1;
  case 0xA38A5C: Metalee_Func_5(); return 1;
  case 0xA38AB2: Metalee_Func_6(); return 1;
  case 0xA38B0F: Metalee_Shot(); return 1;
  case 0xA38D2D: Fireflea_Init(); return 1;
  case 0xA38DEE: Fireflea_Main(); return 1;
  case 0xA38E83: Fireflea_Powerbomb(); return 1;
  case 0xA38E89: Fireflea_Shot(); return 1;
  case 0xA38E8D: Fireflea_Common(); return 1;
  case 0xA390B5: MaridiaFish_Init(); return 1;
  case 0xA39132: MaridiaFish_Func_1(); return 1;
  case 0xA391AB: MaridiaFish_Func_2(); return 1;
  case 0xA39224: MaridiaFish_Func_3(); return 1;
  case 0xA39256: MaridiaFish_Func_4(); return 1;
  case 0xA3912B: MaridiaFish_Main(); return 1;
  case 0xA394E6: Elevator_Init(); return 1;
  case 0xA3952A: Elevator_Frozen(); return 1;
  case 0xA39548: Elevator_Func_1(); return 1;
  case 0xA39579: Elevator_Func_2(); return 1;
  case 0xA395B9: Elevator_Func_3(); return 1;
  case 0xA395BC: Elevator_Func3b(); return 1;
  case 0xA39612: Elevator_Func_4(); return 1;
  case 0xA396E3: Crab_Init(); return 1;
  case 0xA396FD: Crab_Func_1(); return 1;
  case 0xA3993B: Slug_Init(); return 1;
  case 0xA39955: Slug_Func_1(); return 1;
  case 0xA39C9F: PlatformThatFallsWithSamus_Init(); return 1;
  case 0xA39CBA: FastMovingSlowSinkingPlatform_Init(); return 1;
  case 0xA39D16: PlatformThatFallsWithSamus_Main(); return 1;
  case 0xA39D5E: PlatformThatFallsWithSamus_Func_1(); return 1;
  case 0xA39D83: PlatformThatFallsWithSamus_Func_2(); return 1;
  case 0xA39DA8: PlatformThatFallsWithSamus_Func_3(); return 1;
  case 0xA39DE4: PlatformThatFallsWithSamus_Func_4(); return 1;
  case 0xA39E47: PlatformThatFallsWithSamus_Func_5(); return 1;
  case 0xA39E64: PlatformThatFallsWithSamus_Func_6(); return 1;
  case 0xA39E81: PlatformThatFallsWithSamus_Func_7(); return 1;
  case 0xA39E9E: PlatformThatFallsWithSamus_Func_8(); return 1;
  case 0xA39EBB: PlatformThatFallsWithSamus_Func_9(); return 1;
  case 0xA39EE1: PlatformThatFallsWithSamus_Func_10(); return 1;
  case 0xA39F08: FastMovingSlowSinkingPlatform_Shot(); return 1;
  case 0xA3A12F: Roach_Func_1(); return 1;
  case 0xA3A14D: Roach_Init(); return 1;
  case 0xA3A2D0: Roach_Main(); return 1;
  case 0xA3A2D7: Roach_Func_9(); return 1;
  case 0xA3A301: Roach_Func_10(); return 1;
  case 0xA3A30B: Roach_Func_11(); return 1;
  case 0xA3A315: Roach_Func_12(); return 1;
  case 0xA3A325: Roach_Func_13(); return 1;
  case 0xA3A33B: Roach_Func_14(); return 1;
  case 0xA3A34B: Roach_Func_15(); return 1;
  case 0xA3A380: Roach_Func_16(); return 1;
  case 0xA3A407: Roach_Func_19(); return 1;
  case 0xA3A40E: Roach_Func_20(); return 1;
  case 0xA3A440: Roach_Func_21(); return 1;
  case 0xA3A447: Roach_Func_22(); return 1;
  case 0xA3A44E: Roach_Func_23(); return 1;
  case 0xA3A462: Roach_Func_24(); return 1;
  case 0xA3A476: Roach_Func_25(); return 1;
  case 0xA3A4B6: Roach_Func_26(); return 1;
  case 0xA3A4F0: Roach_Func_27(); return 1;
  case 0xA3A52A: Roach_Func_28(); return 1;
  case 0xA3AB09: Sidehopper_Init(); return 1;
  case 0xA3ABBB: Sidehopper_Func_3(); return 1;
  case 0xA3ABCF: Sidehopper_Main(); return 1;
  case 0xA3ABE6: Sidehopper_Func_5(); return 1;
  case 0xA3AC13: Sidehopper_Func_6(); return 1;
  case 0xA3AC40: Sidehopper_Func_7(); return 1;
  case 0xA3AC56: Sidehopper_Func_8(); return 1;
  case 0xA3AC6C: Sidehopper_Func_9(); return 1;
  case 0xA3AC8F: Sidehopper_Func_10(); return 1;
  case 0xA3ACA8: Sidehopper_Func_11(); return 1;
  case 0xA3ACCB: Sidehopper_Func_12(); return 1;
  case 0xA3ACE4: Sidehopper_Func_13(); return 1;
  case 0xA3AD0E: Sidehopper_Func_14(); return 1;
  case 0xA3AD20: Sidehopper_Func_15(); return 1;
  case 0xA3AD32: Sidehopper_Func_16(); return 1;
  case 0xA3AD44: Sidehopper_Func_17(); return 1;
  case 0xA3AD56: Sidehopper_Func_18(); return 1;
  case 0xA3AD6D: Sidehopper_Func_19(); return 1;
  case 0xA3ADD4: Sidehopper_Func_20(); return 1;
  case 0xA3AE27: Sidehopper_Func_21(); return 1;
  case 0xA3AE8E: Sidehopper_Func_22(); return 1;
  case 0xA3C6AE: Skree_Init(); return 1;
  case 0xA3C6C7: Skree_Main(); return 1;
  case 0xA3C6D5: Skree_Func_1(); return 1;
  case 0xA3C716: Skree_Func_3(); return 1;
  case 0xA3C77F: Skree_Func_4(); return 1;
  case 0xA3C7D5: Skree_Func_5(); return 1;
  case 0xA3C7F5: Skree_Shot(); return 1;
  case 0xA3E043: WreckedShipOrangeZoomer_Init(); return 1;
  case 0xA3E08B: WreckedShipOrangeZoomer_Main(); return 1;
  case 0xA3E2D4: BigEyeBugs_Init(); return 1;
  case 0xA3E59C: FireZoomer_Init(); return 1;
  case 0xA3E669: StoneZoomer_Init(); return 1;
  case 0xA3E6C2: StoneZoomer_Main(); return 1;
  case 0xA5E9F5: sub_A5E9F5(); return 1;
  case 0xA5EA2A: SporeSpawn_Init(); return 1;
  case 0xA5EB13: SporeSpawn_Main(); return 1;
  case 0xA5EB1B: SporeSpawn_Func_1(); return 1;
  case 0xA5EB9B: SporeSpawn_Func_3(); return 1;
  case 0xA5EBEE: SporeSpawn_Func_4(); return 1;
  case 0xA5EC49: SporeSpawn_Func_5(); return 1;
  case 0xA5ED5A: SporeSpawn_Shot(); return 1;
  case 0xA5EDEC: SporeSpawn_Touch(); return 1;
  case 0xA5EDF3: SporeSpawn_Func_6(); return 1;
  case 0xA686F5: Boulder_Init(); return 1;
  case 0xA68793: Boulder_Main(); return 1;
  case 0xA6879A: Boulder_Func_1(); return 1;
  case 0xA687ED: Boulder_Func_2(); return 1;
  case 0xA68832: Boulder_Func_3(); return 1;
  case 0xA6888B: Boulder_Func_4(); return 1;
  case 0xA68942: Boulder_Func_5(); return 1;
  case 0xA689FC: Boulder_Func_6(); return 1;
  case 0xA69A58: FakeKraid_Init(); return 1;
  case 0xA69AC2: FakeKraid_Main(); return 1;
  case 0xA69C22: FakeKraid_Touch(); return 1;
  case 0xA69C39: FakeKraid_Shot(); return 1;
  case 0xA69C50: FakeKraid_9C50(); return 1;
  case 0xA7E912: Etecoon_Init(); return 1;
  case 0xA7E940: Etecoon_Main(); return 1;
  case 0xA7F4DD: Dachora_Init(); return 1;
  case 0xA7F52E: Dachora_Main(); return 1;
  case 0xA88023: Enemy_NormalTouchAI_A8(); return 1;
  case 0xA8802D: Enemy_NormalShotAI_A8(); return 1;
  case 0xA88041: Enemy_NormalFrozenAI_A8(); return 1;
  case 0xA89058: MorphBallEye_Init(); return 1;
  case 0xA890E2: MorphBallEye_Main(); return 1;
  case 0xA8A148: YappingMaw_Init(); return 1;
  case 0xA8A211: YappingMaw_Main(); return 1;
  case 0xA8A235: YappingMaw_Func_1(); return 1;
  case 0xA8A28C: YappingMaw_Func_2(); return 1;
  case 0xA8A310: YappingMaw_Func_3(); return 1;
  case 0xA8A339: YappingMaw_Func_4(); return 1;
  case 0xA8A37C: YappingMaw_Func_5(); return 1;
  case 0xA8A3BF: YappingMaw_Func_6(); return 1;
  case 0xA8A402: YappingMaw_Func_7(); return 1;
  case 0xA8A445: YappingMaw_Func_8(); return 1;
  case 0xA8A665: YappingMaw_Func_10(); return 1;
  case 0xA8A68A: YappingMaw_Func_11(); return 1;
  case 0xA8A6C4: YappingMaw_Func_12(); return 1;
  case 0xA8A6DB: YappingMaw_Func_13(); return 1;
  case 0xA8A6FC: YappingMaw_Func_14(); return 1;
  case 0xA8A71D: YappingMaw_Func_15(); return 1;
  case 0xA8A799: YappingMaw_Touch(); return 1;
  case 0xA8A7BD: YappingMaw_Shot(); return 1;
  case 0xA8A835: YappingMaw_Frozen(); return 1;
  case 0xA8A899: YappingMaw_Func_19(); return 1;
  case 0xA8AB46: Kago_Init(); return 1;
  case 0xA8AB75: Kago_Main(); return 1;
  case 0xA8AB83: Kago_Shot(); return 1;
  case 0xA8B762: Beetom_Func_1(); return 1;
  case 0xA8B776: Beetom_Init(); return 1;
  case 0xA8B814: Beetom_Func_3(); return 1;
  case 0xA8B82F: Beetom_Func_4(); return 1;
  case 0xA8B84F: Beetom_Func_5(); return 1;
  case 0xA8B85F: Beetom_Func_6(); return 1;
  case 0xA8B873: Beetom_Func_7(); return 1;
  case 0xA8B887: Beetom_Func_8(); return 1;
  case 0xA8B8A9: Beetom_Func_9(); return 1;
  case 0xA8B8CB: Beetom_Func_10(); return 1;
  case 0xA8B8ED: Beetom_Func_11(); return 1;
  case 0xA8B90F: Beetom_Func_12(); return 1;
  case 0xA8B952: Beetom_Func_13(); return 1;
  case 0xA8B966: Beetom_Func_14(); return 1;
  case 0xA8B97A: Beetom_Func_15(); return 1;
  case 0xA8B9A2: Beetom_Func_16(); return 1;
  case 0xA8B9B2: Beetom_Func_17(); return 1;
  case 0xA8B9C1: Beetom_Func_18(); return 1;
  case 0xA8BA24: Beetom_Func_19(); return 1;
  case 0xA8BA84: Beetom_Func_20(); return 1;
  case 0xA8BAB7: Beetom_Func_21(); return 1;
  case 0xA8BB55: Beetom_Func_24(); return 1;
  case 0xA8BB88: Beetom_Func_25(); return 1;
  case 0xA8BC26: Beetom_Func_28(); return 1;
  case 0xA8BC5A: Beetom_Func_29(); return 1;
  case 0xA8BD8C: Beetom_Func_34(); return 1;
  case 0xA8BCF8: Beetom_Func_32(); return 1;
  case 0xA8BD42: Beetom_Func_33(); return 1;
  case 0xA8BD9D: Beetom_Func_35(); return 1;
  case 0xA8BDC5: Beetom_Func_36(); return 1;
  case 0xA8B80D: Beetom_Main(); return 1;
  case 0xA8BE2E: Beetom_Touch(); return 1;
  case 0xA8BEAC: Beetom_Shot(); return 1;
  case 0xA8DCCD: WalkingLavaSeahorse_Init(); return 1;
  case 0xA8DD6B: WalkingLavaSeahorse_Main(); return 1;
  case 0xA8DF9D: WalkingLavaSeahorse_Func_10(); return 1;
  case 0xA8E82E: BlueBrinstarFaceBlock_Init(); return 1;
  case 0xA8E86E: BlueBrinstarFaceBlock_Func_1(); return 1;
  case 0xA8E8AE: BlueBrinstarFaceBlock_Main(); return 1;
  case 0xA8E91D: BlueBrinstarFaceBlock_Shot(); return 1;
  case 0xA8F188: KiHunter_Init(); return 1;
  case 0xA8F214: KiHunterWings_Init(); return 1;
  case 0xA8F25C: KiHunter_Main(); return 1;
  case 0xA8F262: KiHunterWings_Main(); return 1;
  case 0xA8F701: KiHunter_Shot(); return 1;
  case 0xA8F7DB: KiHunter_Func_11(); return 1;
  case 0xA8F851: KiHunter_Func_12(); return 1;
  case 0xA8F87F: KiHunter_Func_13(); return 1;
  case 0xA8F8AD: KiHunter_Func_14(); return 1;
  case 0xA8F98D: KiHunter_Func_17(); return 1;
  case 0xAA8041: Enemy_NormalFrozenAI_AA(); return 1;
  case 0xAAC250: Torizo_C250(); return 1;
  case 0xAAC268: Torizo_C268(); return 1;
  case 0xAAC280: Torizo_C280(); return 1;
  case 0xAAC298: Torizo_C298(); return 1;
  case 0xAAC2B0: Torizo_C2B0(); return 1;
  case 0xAAC67E: Torizo_Hurt(); return 1;
  case 0xAAC6A4: Torizo_Main(); return 1;
  case 0xAAC87F: Torizo_Init(); return 1;
  case 0xAAC977: GoldTorizo_Touch(); return 1;
  case 0xAAC97C: Torizo_Shot(); return 1;
  case 0xAAC9C2: Torizo_Func_8(); return 1;
  case 0xAAD658: Torizo_D658(); return 1;
  case 0xAAD667: GoldTorizo_Shot(); return 1;
  case 0xAAD6A6: Torizo_D6A6(); return 1;
  case 0xAAD7C8: TourianEntranceStatue_Init(); return 1;
  case 0xAAE725: ChozoStatue_Init(); return 1;
  case 0xAAE784: sub_AAE784(); return 1;
  case 0xAAE7A7: ChozoStatue_Main(); return 1;
  case 0xB28041: Enemy_NormalFrozenAI_B2(); return 1;
  case 0xB28767: WalkingSpacePirates_Powerbomb(); return 1;
  case 0xB2876C: WalkingSpacePirates_Touch(); return 1;
  case 0xB28779: WalkingSpacePirates_Shot(); return 1;
  case 0xB28789: WalkingSpacePirates_8789(); return 1;
  case 0xB287C8: WalkingSpacePirates_87C8(); return 1;
  case 0xB2883E: WalkingSpacePirates_883E(); return 1;
  case 0xB2EF9F: WallSpacePirates_Init(); return 1;
  case 0xB2F02D: WallSpacePirates_Main(); return 1;
  case 0xB2F5DE: NinjaSpacePirates_Init(); return 1;
  case 0xB2F6A2: NinjaSpacePirates_Main(); return 1;
  case 0xB2F917: SpacePirates_Func_16(); return 1;
  case 0xB2FD02: WalkingSpacePirates_Init(); return 1;
  case 0xB2FD32: WalkingSpacePirates_Main(); return 1;
  case 0xB2FD44: WalkingSpacePirates_FD44(); return 1;
  case 0xB2FDCE: WalkingSpacePirates_FDCE(); return 1;
  case 0xB2FE4B: WalkingSpacePirates_FE4B(); return 1;
  case 0xB38023: Enemy_NormalTouchAI_B3(); return 1;
  case 0xB3802D: Enemy_NormalShotAI_B3(); return 1;
  case 0xB38041: Enemy_NormalFrozenAI_B3(); return 1;
  case 0xB3883B: BrinstarPipeBug_Init(); return 1;
  case 0xB3887A: BrinstarPipeBug_Main(); return 1;
  case 0xB3898B: BrinstarPipeBug_Func_1(); return 1;
  case 0xB38BCD: NorfairPipeBug_Func_1(); return 1;
  case 0xB38BFF: NorfairPipeBug_Func_2(); return 1;
  case 0xB38CA6: NorfairPipeBug_Func_4(); return 1;
  case 0xB38D0C: NorfairPipeBug_Func_6(); return 1;
  case 0xB38D4E: NorfairPipeBug_Func_7(); return 1;
  case 0xB38D90: NorfairPipeBug_Func_8(); return 1;
  case 0xB38DD2: NorfairPipeBug_Func_9(); return 1;
  case 0xB38E14: NorfairPipeBug_Func_10(); return 1;
  case 0xB38E35: NorfairPipeBug_Func_11(); return 1;
  case 0xB38E5A: NorfairPipeBug_Func_12(); return 1;
  case 0xB38F4C: BrinstarYellowPipeBug_Init(); return 1;
  case 0xB38FAE: BrinstarYellowPipeBug_Main(); return 1;
  case 0xB38FB5: BrinstarYellowPipeBug_Func_1(); return 1;
  case 0xB39028: BrinstarYellowPipeBug_Func_3(); return 1;
  case 0xB390BD: BrinstarYellowPipeBug_Func_5(); return 1;
  case 0xB3915A: BrinstarYellowPipeBug_Func_7(); return 1;
  case 0xB391D8: BrinstarYellowPipeBug_Func_8(); return 1;
  case 0xB3E655: EscapeEtecoon_Main(); return 1;
  case 0xB3E6CB: EscapeEtecoon_Init(); return 1;
  case 0xB3EAE5: EscapeDachora_Init(); return 1;
  }
  return 0;
}

uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w) {
  switch (bank) {
  case 0x86: *w = BW_86(a); return 1;
  case 0x94: *w = BW_94(a); return 1;
  case 0xA0: *w = BW_A0(a); return 1;
  case 0xA1: *w = BW_A1(a); return 1;
  case 0xA2: *w = BW_A2(a); return 1;
  case 0xA3: *w = BW_A3(a); return 1;
  case 0xA5: *w = BW_A5(a); return 1;
  case 0xA6: *w = BW_A6(a); return 1;
  case 0xA7: *w = BW_A7(a); return 1;
  case 0xA8: *w = BW_A8(a); return 1;
  case 0xAA: *w = BW_AA(a); return 1;
  case 0xB2: *w = BW_B2(a); return 1;
  case 0xB3: *w = BW_B3(a); return 1;
  case 0xB4: *w = BW_B4(a); return 1;
  }
  return 0;
}
