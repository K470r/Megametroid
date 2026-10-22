/* MegaMetroid — motor nativo: PLMs (banco $84 del decomp, sm_84.c).
 *
 * Los PLMs ("post-load modifications") son los objetos de bloque de las salas: puertas y sus tapas, bloques de
 * scroll, objetos (ítems), estaciones, bloques que se rompen y vuelven, compuertas... Cada uno tiene una cabecera
 * en $84 (función de inicio, lista de instrucciones) y una lista de instrucciones que cambia bloques del nivel
 * (level_data/BTS, room.h) cada tantos cuadros. Las listas se leen del banco $84 ya en palabras del 68000 (B84W) y
 * las poblaciones de las salas del banco $8F (B8FW).
 *
 * Portado del decomp casi tal cual (las funciones con los nombres y direcciones de sm_84.c); lo que cambia:
 *  - punteros del SNES (const uint8 *) -> direcciones de 16 bits del banco ($84:p) leídas con B84W/B84B;
 *  - la parte de la VRAM de DrawPlm (los tiles de los bloques que cambian en la pantalla) la hace la Genesis:
 *    ProcessPlmDrawInstruction avisa con nt_block_changed() de cada bloque escrito;
 *  - los tiles de los objetos (PlmInstr_LoadItemPlmGfx) los sube la Genesis (g_hooks.plm_item_gfx). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "room.h"
#include "plm.h"
#include "enemies.h"
#include "../gen/enums.h"

static inline uint8 B84B(uint16 a) { return (uint8)B84W(a); }
#define LOBYTE(x) ((uint8)(x))
#define HIBYTE(x) ((uint8)((x) >> 8))
static inline uint16 abs16(uint16 t) { return (t & 0x8000) ? (uint16)-t : t; }
static uint16 PrepareBitAccess(uint16 a) {  // 0x80818E
  bitmask = 1 << (a & 7);
  return a >> 3;
}
int DisplayMessageBox_Poll(uint16 a);   /* msgbox.c */
static void nt_ram_write16(uint16 a, uint16 v) {   /* la RAM que nombra el argumento del PLM */
  if (a >= 0x7800 && a < 0x9800 && !(a & 1))
    enemy_ram7800[(a - 0x7800) >> 1] = v;           /* variables de los jefes (Draygon: sus cañones) */
  else
    nt_unimpl_v("SetPlmVarPtr (dirección de RAM sin preparar)", a);
}
/* los 16 bits del medio de un número de 24 bits guardado como sub (palabra baja) y entero (palabra alta) */
static inline uint16 get_mid16(uint16 sub, uint16 whole) { return (uint16)(sub >> 8 | whole << 8); }
static inline void set_mid16(uint16 *sub, uint16 *whole, uint16 v) {
  *sub = (*sub & 0x00FF) | (uint16)(v << 8);
  *whole = (*whole & 0xFF00) | (v >> 8);
}

static uint16 CallPlmInstr(uint32 ea, uint16 j, uint16 k);
static uint16 DrawW(uint16 a);
static uint16 PlmInstr_ABD6(uint16 p, uint16 k);
static uint16 PlmInstr_ActivateEnergyStation(uint16 p, uint16 k);
static uint16 PlmInstr_ActivateMapStation(uint16 p, uint16 k);
static uint16 PlmInstr_ActivateMissileStation(uint16 p, uint16 k);
static uint16 PlmInstr_ActivateSaveStationAndGotoIfNo(uint16 p, uint16 k);
static uint16 PlmInstr_Call(uint16 p, uint16 k);
static uint16 PlmInstr_CallFunction(uint16 p, uint16 k);
static uint16 PlmInstr_ClearChargeBeamCounter(uint16 p, uint16 k);
static uint16 PlmInstr_ClearMusicQueueAndQueueTrack(uint16 p, uint16 k);
static uint16 PlmInstr_ClearPreInstr(uint16 p, uint16 k);
static uint16 PlmInstr_ClearTrigger(uint16 p, uint16 k);
static uint16 PlmInstr_CollectAmmoMissileTank(uint16 p, uint16 k);
static uint16 PlmInstr_CollectAmmoPowerBombTank(uint16 p, uint16 k);
static uint16 PlmInstr_CollectAmmoSuperMissileTank(uint16 p, uint16 k);
static uint16 PlmInstr_CollectHealthEnergyTank(uint16 p, uint16 k);
static uint16 PlmInstr_CollectHealthReserveTank(uint16 p, uint16 k);
static uint16 PlmInstr_CopyFromRamToVram(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurret(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurret2(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurretFacingDownLeft(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurretFacingDownRight(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurretFacingUpLeft(uint16 p, uint16 k);
static uint16 PlmInstr_DamageDraygonTurretFacingUpRight(uint16 p, uint16 k);
static uint16 PlmInstr_DealDamage_2(uint16 p, uint16 k);
static uint16 PlmInstr_DecrementAndBranchNonzero(uint16 p, uint16 k);
static uint16 PlmInstr_Delete(uint16 p, uint16 k);
static uint16 PlmInstr_DiagonalEarthquake(uint16 p, uint16 k);
static uint16 PlmInstr_DisableSamusControls(uint16 p, uint16 k);
static uint16 PlmInstr_DisplayGameSavedMessageBox(uint16 p, uint16 k);
static uint16 PlmInstr_Draw0x38FramesOfLeftTreadmill(uint16 p, uint16 k);
static uint16 PlmInstr_Draw0x38FramesOfRightTreadmill(uint16 p, uint16 k);
static uint16 PlmInstr_DrawItemFrame0(uint16 p, uint16 k);
static uint16 PlmInstr_DrawItemFrame1(uint16 p, uint16 k);
static uint16 PlmInstr_DrawItemFrame_Common(uint16 p, uint16 k);
static uint16 PlmInstr_DrawPlmBlock(uint16 p, uint16 k);
static uint16 PlmInstr_E63B(uint16 p, uint16 k);
static uint16 PlmInstr_EnableMovementAndSetSaveStationUsed(uint16 p, uint16 k);
static uint16 PlmInstr_EnableSamusControls(uint16 p, uint16 k);
static uint16 PlmInstr_EnableWaterPhysics(uint16 p, uint16 k);
static uint16 PlmInstr_FxBaseYPos_0x2D2(uint16 p, uint16 k);
static uint16 PlmInstr_GiveInvincibility(uint16 p, uint16 k);
static uint16 PlmInstr_Goto(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfBossBitSet(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfChozoSet(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfDoorBitSet(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfEventSet(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfItemBitSet(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfMissilesFull(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfRoomArgLess(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfSamusHealthFull(uint16 p, uint16 k);
static uint16 PlmInstr_GotoIfSamusNear(uint16 p, uint16 k);
static uint16 PlmInstr_IncrementArgumentAndJGE(uint16 p, uint16 k);
static uint16 PlmInstr_IncrementDoorHitCounterAndJGE(uint16 p, uint16 k);
static uint16 PlmInstr_JumpIfSamusHasNoBombs(uint16 p, uint16 k);
static uint16 PlmInstr_LoadItemPlmGfx(uint16 p, uint16 k);
static uint16 PlmInstr_MovePlmDownOneBlock(uint16 p, uint16 k);
static uint16 PlmInstr_MovePlmDownOneBlock_0(uint16 p, uint16 k);
static uint16 PlmInstr_MovePlmRight4Blocks(uint16 p, uint16 k);
static uint16 PlmInstr_MoveUpAndMakeBlueDoorFacingLeft(uint16 p, uint16 k);
static uint16 PlmInstr_MoveUpAndMakeBlueDoorFacingRight(uint16 p, uint16 k);
static uint16 PlmInstr_PickupBeamAndShowMessage(uint16 p, uint16 k);
static uint16 PlmInstr_PickupEquipmentAddGrappleShowMessage(uint16 p, uint16 k);
static uint16 PlmInstr_PickupEquipmentAddXrayShowMessage(uint16 p, uint16 k);
static uint16 PlmInstr_PickupEquipmentAndShowMessage(uint16 p, uint16 k);
static uint16 PlmInstr_PlaceSamusOnSaveStation(uint16 p, uint16 k);
static uint16 PlmInstr_PreInstr(uint16 p, uint16 k);
static uint16 PlmInstr_ProcessAirScrollUpdate(uint16 p, uint16 k);
static uint16 PlmInstr_ProcessSolidScrollUpdate(uint16 p, uint16 k);
static uint16 PlmInstr_QueueMusic(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx1_Max1(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx1_Max15(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx1_Max3(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx1_Max6(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx1_Max9(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx2_Max1(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx2_Max15(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx2_Max3(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx2_Max6(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx2_Max9(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx3_Max1(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx3_Max15(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx3_Max6(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx3_Max9(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSfx_Max3(uint16 p, uint16 k);
static uint16 PlmInstr_QueueSong1MusicTrack(uint16 p, uint16 k);
static uint16 PlmInstr_Return(uint16 p, uint16 k);
static uint16 PlmInstr_Scroll_0_1_Blue(uint16 p, uint16 k);
static uint16 PlmInstr_SetBTS(uint16 p, uint16 k);
static uint16 PlmInstr_SetBtsTo1(uint16 p, uint16 k);
static uint16 PlmInstr_SetCrittersEscapedEvent(uint16 p, uint16 k);
static uint16 PlmInstr_SetEvent(uint16 p, uint16 k);
static uint16 PlmInstr_SetGreyDoorPreInstr(uint16 p, uint16 k);
static uint16 PlmInstr_SetItemBit(uint16 p, uint16 k);
static uint16 PlmInstr_SetLinkReg(uint16 p, uint16 k);
static uint16 PlmInstr_SetRoomChozoBit(uint16 p, uint16 k);
static uint16 PlmInstr_SetTimer(uint16 p, uint16 k);
static uint16 PlmInstr_ShootEyeDoorProjectileWithProjectileArg(uint16 p, uint16 k);
static uint16 PlmInstr_Sleep(uint16 p, uint16 k);
static uint16 PlmInstr_Spawn10shardsAnd6n00bs(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnEproj(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnEyeDoorSmokeProjectile(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnEyeDoorSweatEproj(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnFourMotherBrainGlass(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnN00bTubeCrackEproj(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnTorizoStatueBreaking(uint16 p, uint16 k);
static uint16 PlmInstr_SpawnTwoEyeDoorSmoke(uint16 p, uint16 k);
static uint16 PlmInstr_TransferWreckedShipChozoSpikesToSlopes(uint16 p, uint16 k);
static uint16 PlmInstr_TransferWreckedShipSlopesToChozoSpikes(uint16 p, uint16 k);
static uint16 PlmInstr_WakeEprojAtPlmPos(uint16 p, uint16 k);
static uint16 SetBtsTo0x10AdvanceRow(uint16 k);
static uint16 SetBtsTo0x10AdvanceRowUp(uint16 k);
static uint8 ActivateStationIfSamusCannonLinedUp(uint16 a, uint16 j);
static uint8 CallPlmHeaderFunc(uint32 ea, uint16 j);
static uint8 CheckIfColoredDoorCapSpawned(void);
static uint8 PlmSetup_B638_Rightwards_Extension(uint16 j);
static uint8 PlmSetup_B63F_Leftwards_Extension(uint16 j);
static uint8 PlmSetup_B643_Downwards_Extension(uint16 j);
static uint8 PlmSetup_B647_Upwards_Extension(uint16 j);
static uint8 PlmSetup_B6D3_MapStation(uint16 j);
static uint8 PlmSetup_B6E3_EnergyStationRightAccess(uint16 j);
static uint8 PlmSetup_B6E7_EnergyStationLeftAccess(uint16 j);
static uint8 PlmSetup_B6EF_MissileStationRightAccess(uint16 j);
static uint8 PlmSetup_B6F3_MissileStationLeftAccess(uint16 j);
static uint8 PlmSetup_B6FF_ScrollBlockTouch(uint16 j);
static uint8 PlmSetup_B703_ScrollPLM(uint16 j);
static uint8 PlmSetup_B707_SolidScrollPLM(uint16 j);
static uint8 PlmSetup_B70F_IcePhysics(uint16 j);
static uint8 PlmSetup_B71F_SubmergingQuicksand(uint16 j);
static uint8 PlmSetup_B723_SandfallsSlow(uint16 j);
static uint8 PlmSetup_B727_SandFallsFast(uint16 j);
static uint8 PlmSetup_B737_SubmergingQuicksand(uint16 j);
static uint8 PlmSetup_B73B_B73F_SandFalls(uint16 j);
static uint8 PlmSetup_B767_ClearShitroidInvisibleWall(uint16 j);
static uint8 PlmSetup_B76B_SaveStationTrigger(uint16 j);
static uint8 PlmSetup_B76F_SaveStation(uint16 j);
static uint8 PlmSetup_B7EB_EnableSoundsIn32Frames(uint16 j);
static uint8 PlmSetup_B974(uint16 j);
static uint8 PlmSetup_B9C1_CrittersEscapeBlock(uint16 j);
static uint8 PlmSetup_B9ED_CrittersEscapeBlock(uint16 j);
static uint8 PlmSetup_BB30_CrateriaMainstreetEscape(uint16 j);
static uint8 PlmSetup_BTS_Brinstar_0x80_Floorplant(uint16 j);
static uint8 PlmSetup_BTS_Brinstar_0x81_Ceilingplant(uint16 j);
static uint8 PlmSetup_Bts47_MapStationRightAccess(uint16 j);
static uint8 PlmSetup_Bts4_MapStationLeftAccess(uint16 j);
static uint8 PlmSetup_C73A_UpwardsGateShootblock(uint16 j);
static uint8 PlmSetup_C794_GreyDoor(uint16 j);
static uint8 PlmSetup_C806_LeftGreenGateTrigger(uint16 j);
static uint8 PlmSetup_C80A_RightGreenGateTrigger(uint16 j);
static uint8 PlmSetup_C80E_LeftRedGateTrigger(uint16 j);
static uint8 PlmSetup_C812_RightRedGateTrigger(uint16 j);
static uint8 PlmSetup_C816_LeftBlueGateTrigger(uint16 j);
static uint8 PlmSetup_C81A_RightBlueGateTrigger(uint16 j);
static uint8 PlmSetup_C81E_LeftYellowGateTrigger(uint16 j);
static uint8 PlmSetup_C822_RightYellowGateTrigger(uint16 j);
static uint8 PlmSetup_C826_DownwardsOpenGate(uint16 j);
static uint8 PlmSetup_C82A_DownwardsClosedGate(uint16 j);
static uint8 PlmSetup_C82E_UpwardsOpenGate(uint16 j);
static uint8 PlmSetup_C832_UpwardsClosedGate(uint16 j);
static uint8 PlmSetup_C836_DownwardsGateShootblock(uint16 j);
static uint8 PlmSetup_ClearShitroidInvisibleWall(uint16 j);
static uint8 PlmSetup_CrumbleBotwoonWall(uint16 j);
static uint8 PlmSetup_D028_D02C_Unused(uint16 j);
static uint8 PlmSetup_D08C_CrumbleBlock(uint16 j);
static uint8 PlmSetup_D08C_SuperMissileBlockRespawning(uint16 j);
static uint8 PlmSetup_D094_EnemyBreakableBlock(uint16 j);
static uint8 PlmSetup_D0D8_ClearVflag(uint16 j);
static uint8 PlmSetup_D0D8_SetVFlag(uint16 j);
static uint8 PlmSetup_D0DC_BreakableGrappleBlock(uint16 j);
static uint8 PlmSetup_D0E8_GiveSamusDamage(uint16 j);
static uint8 PlmSetup_D113_LowerNorfairChozoRoomPlug(uint16 j);
static uint8 PlmSetup_D127(uint16 j);
static uint8 PlmSetup_D138(uint16 j);
static uint8 PlmSetup_D6DA_LowerNorfairChozoHandTrigger(uint16 j);
static uint8 PlmSetup_D6F2_WreckedShipChozoHandTrigger(uint16 j);
static uint8 PlmSetup_D700_MakePlmAirBlock_Unused(uint16 j);
static uint8 PlmSetup_D704_AlteranateLowerNorfairChozoHand_Unused(uint16 j);
static uint8 PlmSetup_D708_LowerNorfairChozoBlockUnused(uint16 j);
static uint8 PlmSetup_D70C_NoobTube(uint16 j);
static uint8 PlmSetup_DeactivatePlm(uint16 j);
static uint8 PlmSetup_DeletePlmIfAreaTorizoDead(uint16 j);
static uint8 PlmSetup_Door_Blue(uint16 j);
static uint8 PlmSetup_Door_Colored(uint16 j);
static uint8 PlmSetup_Door_Strange(uint16 j);
static uint8 PlmSetup_DraygonCannon(uint16 j);
static uint8 PlmSetup_DraygonCannonFacingDiagonalLeft(uint16 j);
static uint8 PlmSetup_DraygonCannonFacingDiagonalRight(uint16 j);
static uint8 PlmSetup_DraygonCannonFacingLeft(uint16 j);
static uint8 PlmSetup_DraygonCannonFacingRight(uint16 j);
static uint8 PlmSetup_EyeDoor(uint16 j);
static uint8 PlmSetup_EyeDoorEye(uint16 j);
static uint8 PlmSetup_MakeBllockChozoHandTrigger(uint16 j);
static uint8 PlmSetup_MotherBrainGlass(uint16 j);
static uint8 PlmSetup_MotherBrainRoomEscapeDoor(uint16 j);
static uint8 PlmSetup_PlmB6DF_EnergyStation(uint16 j);
static uint8 PlmSetup_PlmB6EB_EnergyStation(uint16 j);
static uint8 PlmSetup_QuicksandSurface(uint16 j);
static uint8 PlmSetup_QuicksandSurfaceB(uint16 j);
static uint8 PlmSetup_RespawningBombBlock(uint16 j);
static uint8 PlmSetup_RespawningBombBlock2(uint16 j);
static uint8 PlmSetup_RespawningCrumbleBlock(uint16 j);
static uint8 PlmSetup_RespawningPowerBombBlock(uint16 j);
static uint8 PlmSetup_RespawningShotBlock(uint16 j);
static uint8 PlmSetup_RespawningSpeedBoostBlock(uint16 j);
static uint8 PlmSetup_ReturnCarryClear(uint16 j);
static uint8 PlmSetup_ReturnCarrySet(uint16 j);
static uint8 PlmSetup_SetMetroidRequiredClearState(uint16 j);
static uint8 PlmSetup_SetrupWreckedShipEntrance(uint16 j);
static uint8 PlmSetup_ShaktoolsRoom(uint16 j);
static uint8 PlmSetup_SpeedBoosterEscape(uint16 j);
static uint8 StartPlm(int16 v1, uint16 header, uint16 pre);
static uint8 UNUSED_sub_84B3E3(uint16 j);
static uint8 WakePlmIfSamusIsBelowAndRightOfTarget(uint16 k, uint16 x_r18, uint16 y_r20);
static uint8 sub_84B9F1(uint16 j);
static uint8 sub_84C63F(uint16 j);
static uint8 sub_84C647(uint16 j);
static uint8 sub_84C64C(uint16 j, uint16 a);
static uint8 sub_84EE4D(uint16 j);
static uint8 sub_84EE52(uint16 j);
static uint8 sub_84EE57(uint16 j);
static uint8 sub_84EE5C(uint16 j);
static uint8 sub_84EE5F(uint16 j, uint16 a);
static uint8 sub_84EE64(uint16 j);
static uint8 sub_84EE77(uint16 j);
static uint8 sub_84EE7C(uint16 j);
static uint8 sub_84EE81(uint16 j);
static uint8 sub_84EE86(uint16 j);
static uint8 sub_84EE89(uint16 j, uint16 a);
static uint8 sub_84EE8E(uint16 j);
static uint8 sub_84EEAB(uint16 v0);
static void CallPlmInstrFunc(uint32 ea);
static void CallPlmPreInstr(uint32 ea, uint16 k);
static void DeletePlm(uint16 j);
static void DisplayMessageBox(uint16 a);
static void DrawPlm(uint16 k);
static void PlmPreInstr_AdvanceLavaSamusMovesLeft(uint16 k);
static void PlmPreInstr_B7EB_DecTimerEnableSoundsDeletePlm(uint16 k);
static void PlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed(uint16 k);
static void PlmPreInstr_EscapeRoomBeforeOldTourianEscapeShaft(uint16 k);
static void PlmPreInstr_GoToLinkInstrIfShot(uint16 k);
static void PlmPreInstr_GoToLinkInstrIfShotWithAnyMissile(uint16 k);
static void PlmPreInstr_GoToLinkInstrIfShotWithPowerBomb(uint16 k);
static void PlmPreInstr_GoToLinkInstrIfShotWithSuperMissile(uint16 k);
static void PlmPreInstr_GoToLinkInstruction(uint16 k);
static void PlmPreInstr_GotoLinkIfBoss1Dead(uint16 k);
static void PlmPreInstr_GotoLinkIfCrittersEscaped(uint16 k);
static void PlmPreInstr_GotoLinkIfEnemyDeathQuotaOk(uint16 k);
static void PlmPreInstr_GotoLinkIfMiniBossDead(uint16 k);
static void PlmPreInstr_GotoLinkIfShotWithSuperMissile(uint16 k);
static void PlmPreInstr_GotoLinkIfTorizoDead(uint16 k);
static void PlmPreInstr_GotoLinkIfTourianStatueFinishedProcessing(uint16 k);
static void PlmPreInstr_GotoLinkIfTriggered(uint16 k);
static void PlmPreInstr_IncrementRoomArgIfShotBySuperMissile(uint16 k);
static void PlmPreInstr_OldTourianEscapeShaftEscape(uint16 k);
static void PlmPreInstr_PlayDudSound(uint16 k);
static void PlmPreInstr_PositionSamusAndInvincible(uint16 k);
static void PlmPreInstr_SetMetroidsClearState_Ev0x10(uint16 k);
static void PlmPreInstr_SetMetroidsClearState_Ev0x11(uint16 k);
static void PlmPreInstr_SetMetroidsClearState_Ev0x12(uint16 k);
static void PlmPreInstr_SetMetroidsClearState_Ev0x13(uint16 k);
static void PlmPreInstr_ShaktoolsRoom(uint16 k);
static void PlmPreInstr_WakeAndLavaIfBoosterCollected(uint16 k);
static void PlmPreInstr_WakeIfTriggered(uint16 k);
static void PlmPreInstr_WakeOnKeyPress(uint16 k);
static void PlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft(uint16 k);
static void PlmPreInstr_WakePlmIfRoomArgumentDoorIsSet(uint16 k);
static void PlmPreInstr_WakePlmIfSamusHasBombs(uint16 k);
static void PlmPreInstr_WakePlmIfTriggered(uint16 k);
static void PlmPreInstr_WakePlmIfTriggeredOrSamusAbovePlm(uint16 k);
static void PlmPreInstr_WakePlmIfTriggeredOrSamusBelowPlm(uint16 k);
static void PlmSetup_QuicksandSurface_0(uint16 j);
static void PlmSetup_QuicksandSurface_1(uint16 j);
static void PlmSetup_QuicksandSurface_2(uint16 j);
static void SetBts0x10FiveStepsDown(uint16 j);
static void SetBts0x10FiveStepsUp(uint16 j);
static void SetPlmVarPtr(uint16 k, uint16 a);
static void SkipDebugDrawInstructionsForScrollPlms(uint16 j);
static void SpawnMotherBrainGlassShatteringShard(uint16 a);
static void SpawnRoomPlmAt(uint16 header, uint8 x, uint8 y, uint16 arg);
static void WriteRowOfLevelDataBlockAndBTS(uint16 k, uint16 arg0, uint16 arg1, uint16 arg2);
static void sub_84D7EF(uint16 k);

void CalculatePlmBlockCoords(uint16 k) {  // 0x848290
  uint16 t = plm_block_indices[k >> 1] >> 1;
  uint8 w = (uint8)room_width_in_blocks;
  uint32 qr = w ? DivMod16(t, w) : 0;
  plm_y_block = (uint16)qr;
  plm_x_block = (uint16)(qr >> 16);
}

void WriteLevelDataBlockTypeAndBts(uint16 k, uint16 a) {  // 0x8482B4: tipo de bloque (4 bits altos) y BTS
  uint16 i = k >> 1;
  level_set(i, (level_get(i) & 0x0FFF) | (a & 0xF000));
  bts_set(i, (uint8)a);
  nt_block_changed(i);
}

static void WriteRowOfLevelDataBlockAndBTS(uint16 k, uint16 arg0, uint16 arg1, uint16 arg2) {  // 0x8482D6
  uint16 v8 = plm_block_indices[k >> 1] >> 1;
  for (int i = 0; i < arg2; i++) {
    level_set(v8 + i, arg0);
    bts_set(v8 + i, (uint8)arg1);
    nt_block_changed(v8 + i);
  }
}

void EnablePLMs(void) { plm_flag |= 0x8000; }    // 0x8483AD
void DisablePLMs(void) { plm_flag &= ~0x8000; }  // 0x8483B8
void ClearPLMs(void) {                            // 0x8483C3
  for (int i = 39; i >= 0; i--)
    plm_header_ptr[i] = 0;
  plm_item_gfx_index = 0;
}

static int16 FreePlmSlot(void) {
  int16 v = 78;
  while (plm_header_ptr[v >> 1]) {
    v -= 2;
    if (v < 0)
      return -1;
  }
  return v;
}

static uint8 StartPlm(int16 v1, uint16 header, uint16 pre) {   /* lo común de las altas */
  int v = v1 >> 1;
  plm_header_ptr[v] = header;
  plm_pre_instrs[v] = pre;
  plm_instr_list_ptrs[v] = B84W(header + 2);                     // PlmHeader: función, lista
  plm_instruction_timer[v] = 1;
  plm_instruction_draw_ptr[v] = addr_kDefaultPlmDrawInstruction;
  plm_timers[v] = 0;
  plm_id = v1;
  return CallPlmHeaderFunc(0x840000 | B84W(header), v1);
}

void SpawnHardcodedPlm(uint8 x, uint8 y, uint16 id) {  // 0x8483D7
  int16 v1 = FreePlmSlot();
  if (v1 < 0)
    return;
  int v = v1 >> 1;
  plm_block_indices[v] = 2 * ((uint16)y * (uint8)room_width_in_blocks + x);
  plm_room_arguments[v] = 0;
  plm_variables[v] = 0;
  StartPlm(v1, id, 0x8469);
}

static void SpawnRoomPlmAt(uint16 header, uint8 x, uint8 y, uint16 arg) {  // 0x84846A
  int16 v1 = FreePlmSlot();
  if (v1 < 0)
    return;
  int v = v1 >> 1;
  plm_block_indices[v] = 2 * ((uint16)y * (uint8)room_width_in_blocks + x);
  plm_room_arguments[v] = arg;
  plm_variables[v] = 0;
  StartPlm(v1, header, FUNC16(PlmPreInstr_Empty2));
}

void SpawnRoomPLM(uint16 k) {  // 0x84846A: entrada de la población de la sala ($8F:k: PLM, x, y, argumento)
  SpawnRoomPlmAt(B8FW(k), (uint8)B8FW(k + 2), (uint8)B8FW(k + 3), B8FW(k + 4));
}

static uint8 CheckIfColoredDoorCapSpawned(void) {  // 0x82E91C
  const DoorDefN *DD = door_def(door_def_ptr);
  uint16 v2 = 2 * ((uint16)DD->y_pos_plm * (uint8)room_width_in_blocks + DD->x_pos_plm);
  int16 v3 = 78;
  while (v2 != plm_block_indices[v3 >> 1]) {
    v3 -= 2;
    if (v3 < 0)
      return 0;
  }
  int v5 = v3 >> 1;
  if (!plm_header_ptr[v5])
    return 0;
  int16 v6 = plm_room_arguments[v5];
  if (v6 >= 0) {
    bitmask = 1 << (v6 & 7);                      // PrepareBitAccess
    if (bitmask & opened_door_bit_array[v6 >> 3])
      return 0;
  }
  plm_instruction_timer[v5] = 1;
  plm_instr_list_ptrs[v5] = B84W(plm_header_ptr[v5] + 4);   // PlmHeader_Size6: la segunda lista (cerrarse)
  return 1;
}

void SpawnDoorClosingPLM(void) {  // 0x82E8EB: la tapa de la puerta por la que se entra se cierra detrás de Samus
  if (!CheckIfColoredDoorCapSpawned()) {
    uint16 id = B8FW(addr_kDoorClosingPlmIds + 2 * door_direction);
    if (id) {
      const DoorDefN *DD = door_def(door_def_ptr);
      SpawnRoomPlmAt(id, DD->x_pos_plm, DD->y_pos_plm, 0);
    }
  }
}

void SpawnRoomPlms(uint16 header_ptr) {   /* CreatePlmsExecuteDoorAsmRoomSetup ($82:EB6C): la lista de la sala */
  for (uint16 k = header_ptr; B8FW(k); k += 6)
    SpawnRoomPLM(k);
}

uint8 SpawnPLM(uint16 a) {  // 0x8484E7: PLM del bloque que se toca (cur_block_index); 1: carry, 0x40: overflow
  int16 v1 = FreePlmSlot();
  if (v1 < 0)
    return 0;
  int v = v1 >> 1;
  plm_block_indices[v] = 2 * cur_block_index;
  plm_room_arguments[v] = 0;
  plm_variables[v] = 0;
  return StartPlm(v1, a, FUNC16(PlmPreInstr_Empty3));
}

/* palabra de una instrucción de dibujo: del banco $84 o, si la dirección es de la RAM, la instrucción de un bloque
   que arma PlmInstr_DrawPlmBlock en 7E:1E6D (custom_draw_instr_*) */
static uint16 DrawW(uint16 a) {
  if (a & 0x8000)
    return B84W(a);
  switch (a) {
  case 0x1E6D: return custom_draw_instr_num_blocks;
  case 0x1E6F: return custom_draw_instr_plm_block;
  case 0x1E71: return custom_draw_instr_zero_terminator;
  default: nt_unimpl("ProcessPlmDrawInstruction (instrucción en la RAM)"); return 0;
  }
}

void ProcessPlmDrawInstruction(uint16 v0) {  // 0x84861E: escribe los bloques de la instrucción en el nivel
  int v1 = v0 >> 1;
  uint16 p = plm_instruction_draw_ptr[v1];
  uint16 dst = plm_block_indices[v1] >> 1, dst_base = dst;
  uint16 w = room_width_in_blocks;
  for (;;) {
    uint16 n = DrawW(p);
    p += 2;
    uint16 step = (n & 0x8000) ? w : 1;           /* columna o fila */
    for (int i = 0; i < (uint8)n; i++, p += 2) {
      level_set(dst + i * step, DrawW(p));
      nt_block_changed(dst + i * step);
    }
    uint16 d = DrawW(p);
    if (!d)
      break;
    dst = dst_base + (int8)d + (int8)(d >> 8) * w;
    p += 2;
  }
}

/* DrawPlm ($84:8DAA): en el SNES copia los tiles de los bloques del PLM que se ven a la cola de la VRAM. La Genesis
   los redibuja sola (nt_block_changed); aquí queda solo la cuenta: plm_draw_tilemap_index (8 por bloque, hasta 512)
   limita cuántos bloques más se dibujan en el cuadro, y el juego la lee. */
static void DrawPlm(uint16 k) {  // 0x848DAA
  uint16 v1 = plm_instruction_draw_ptr[k >> 1];
  uint16 x = plm_x_block, y = plm_y_block;
  for (;;) {
    uint16 r26 = layer1_y_pos >> 4, r20, r22, r18 = 0;
    if (sign16(r26 + 15 - y))
      return;
    int16 v2 = DrawW(v1);
    r20 = v2 & 0x7FFF;
    if (v2 < 0) {                                       /* columna */
      if (!(layer1_x_pos >> 4 == x || (int16)((layer1_x_pos >> 4) - x) < 0))
        return;
      int16 v20 = (layer1_x_pos >> 4) + 17;
      if (v20 == x || (int16)(v20 - x) < 0)
        return;
      r22 = r26 + 16;
      if ((int16)(r26 - y) < 0) {
        r26 = y;
      } else {
        r18 = r26 - y;
        if (sign16(y + r20 - r26))
          return;
        if (r20 == r18)
          return;
        r20 -= r18;
      }
    } else {                                            /* fila */
      if (sign16(y - r26))
        return;
      r26 = y;
      r22 = ((uint16)(layer1_x_pos + 15) >> 4) - 1;
      uint16 r24 = x;
      if ((int16)(r22 - x) >= 0 && r22 != x) {
        r18 = r22 - x;
        if (x + r20 == r22 || (int16)(x + r20 - r22) < 0)
          return;
        r20 -= r18;
        r24 = r22;
      }
      r22 += 17;
      if (sign16(r22 - x))
        return;
      r22 = r20 + r24 - 1 - r22;
      if (!(r22 & 0x8000) && !(r20 -= r22))
        return;
      if (sign16(((uint16)(512 - plm_draw_tilemap_index) >> 3) - r20))
        return;
      goto count;
    }
    r22 = r20 + r26 - r22;
    if (!(r22 & 0x8000)) {
      bool neg = (int16)(r20 - r22) < 0;
      r20 -= r22;
      if (!r20 || neg)
        return;
    }
    if (sign16(((uint16)(512 - plm_draw_tilemap_index) >> 3) - r20))
      return;
count:;
    uint16 r3 = 2 * r18 + v1 + 2;
    for (;;) {
      r3 += 2;
      plm_draw_tilemap_index += 8;
      if (!sign16(plm_draw_tilemap_index - 512))
        return;
      if (!--r20)
        break;
    }
    uint16 addr = (r22 & 0x8000) ? r3 : r3 + 2 * r22;  /* siguiente trozo de la instrucción de dibujo */
    uint16 v31 = DrawW(addr);
    if (!v31)
      return;
    x = plm_x_block + (int8)v31;
    y = plm_y_block + (int8)(v31 >> 8);
    v1 = addr + 2;
  }
}

void DisplayMessageBox(uint16 a) {   /* $85:8080: el PLM espera a que se cierre (PlmHandler) */
  queued_message_box_index = a;
}

/* el PLM activo siguiente, de v para abajo (-1: no hay): casi todos los lugares están vacíos y recorrerlos aparte,
   con un puntero, le ahorra al 68000 casi todo el bucle */
static __attribute__((noinline)) int NextActivePlm(int v) {
  const uint16 *h = plm_header_ptr + v;
  for (; v >= 0; v--, h--)
    if (*h)
      return v;
  return -1;
}

uint8 DisplayMessageBox_Async(uint16 a);   /* msgbox.c */

/* corrutina como en el decomp (coroutine_state_2 = 1: esperando la caja de mensaje que pidió el PLM plm_id) */
uint8 PlmHandler(void) {  // 0x8485B4 (PlmHandler_Async)
  int v;
  uint16 p, f;
  if (coroutine_state_2 == 1) {
    v = plm_id >> 1;
    goto RESUME;
  }
  if (!(plm_flag & 0x8000))
    return 0;
  plm_draw_tilemap_index = 0;
  for (v = NextActivePlm(39); v >= 0; v = NextActivePlm(v - 1)) {
    plm_id = 2 * v;
    uint16 pre = plm_pre_instrs[v];
    if (pre != (uint16)fnPlmPreInstr_Empty && pre != (uint16)fnPlmPreInstr_Empty2 && pre != (uint16)fnPlmPreInstr_nullsub_60)
      CallPlmPreInstr(0x840000 | pre, plm_id);
    if (--plm_instruction_timer[v])
      continue;
    p = plm_instr_list_ptrs[v];
    while ((f = B84W(p)) & 0x8000) {
      p = CallPlmInstr(0x840000 | f, p + 2, plm_id);
      if (!p)
        goto NEXT_PLM;
      if (queued_message_box_index) {               /* el PLM pidió una caja de mensaje: se espera a que se cierre */
        plm_instr_list_ptrs[v] = p;
RESUME:
        if (DisplayMessageBox_Async(queued_message_box_index)) { coroutine_state_2 = 1; return 1; }
        queued_message_box_index = 0;
        p = plm_instr_list_ptrs[v];
      }
    }
    plm_instruction_timer[v] = f;
    plm_instruction_draw_ptr[v] = B84W(p + 2);
    plm_instr_list_ptrs[v] = p + 4;
    ProcessPlmDrawInstruction(plm_id);
    CalculatePlmBlockCoords(plm_id);
    DrawPlm(plm_id);
NEXT_PLM:;
  }
  plm_id = 0;
  coroutine_state_2 = 0;
  return 0;
}

static uint16 PlmInstr_Sleep(uint16 p, uint16 k) {  // 0x8486B4
  plm_instr_list_ptrs[k >> 1] = p - 2;
  return 0;
}

static uint16 PlmInstr_Delete(uint16 p, uint16 k) {  // 0x8486BC
  plm_header_ptr[k >> 1] = 0;
  return 0;
}

static uint16 PlmInstr_PreInstr(uint16 p, uint16 k) {  // 0x8486C1
  plm_pre_instrs[k >> 1] = B84W(p);
  return p + 2;
}

static uint16 PlmInstr_ClearPreInstr(uint16 p, uint16 k) {  // 0x8486CA
  plm_pre_instrs[k >> 1] = FUNC16(PlmPreInstr_Empty);
  return p;
}

static void CallPlmInstrFunc(uint32 ea) {
  switch (ea) {
  case fnVariaSuitPickup: case fnGravitySuitPickup:   /* animación de los trajes ($91): aún sin portar */
  default: nt_unimpl("PlmInstr_CallFunction (función sin portar)"); return;
  }
}

static uint16 PlmInstr_CallFunction(uint16 p, uint16 k) {  // 0x84870B
  CallPlmInstrFunc((uint32)(uint8)B84W(p + 2) << 16 | B84W(p));
  return p + 3;
}

static uint16 PlmInstr_Goto(uint16 p, uint16 k) {  // 0x848724
  return (B84W(p));
}


static uint16 PlmInstr_DecrementAndBranchNonzero(uint16 p, uint16 k) {  // 0x84873F
  if (plm_timers[k >> 1]-- == 1)
    return p + 2;
  else
    return PlmInstr_Goto(p, k);
}

static uint16 PlmInstr_SetTimer(uint16 p, uint16 k) {  // 0x84874E
  plm_timers[k >> 1] = (plm_timers[k >> 1] & 0xFF00) | B84B(p);   /* solo el byte bajo */
  return p + 1;
}

static uint16 PlmInstr_LoadItemPlmGfx(uint16 p, uint16 k) {  // 0x848764
  /* los tiles del objeto ($89:xxxx, 256 bytes) y sus 8 paletas: la Genesis los sube a la ranura v (g_hooks) */
  uint16 v2 = plm_item_gfx_index;
  plm_variables[k >> 1] = plm_item_gfx_index;
  plm_item_gfx_index = (v2 + 2) & 6;
  int v3 = v2 >> 1;
  plm_item_gfx_ptrs[v3] = p;
  if (g_hooks.plm_item_gfx) g_hooks.plm_item_gfx(v3, B84W(p), p + 2);
  return p + 10;
}

static uint16 PlmInstr_CopyFromRamToVram(uint16 p, uint16 k) {  // 0x8487E5 (copia a la VRAM: la Genesis)
  return p + 7;
}

static uint16 PlmInstr_GotoIfBossBitSet(uint16 p, uint16 k) {  // 0x84880E
  if (CheckBossBitForCurArea(B84B(p + 0)))
    return PlmInstr_Goto(p + 1, k);
  else
    return p + 3;
}

static uint16 PlmInstr_GotoIfEventSet(uint16 p, uint16 k) {  // 0x84882D
  if (CheckEventHappened(B84W(p)))
    return PlmInstr_Goto(p + 2, k);
  else
    return p + 4;
}

static uint16 PlmInstr_SetEvent(uint16 p, uint16 k) {  // 0x84883E
  SetEventHappened(B84W(p));
  return p + 2;
}

static uint16 PlmInstr_GotoIfChozoSet(uint16 p, uint16 k) {  // 0x848848
  uint16 v3 = plm_room_arguments[k >> 1];
  int idx = PrepareBitAccess(v3);
  if (!sign16(v3) && ((bitmask & room_chozo_bits[idx]) != 0))
    return PlmInstr_Goto(p, k);
  else
    return p + 2;
}

static uint16 PlmInstr_SetRoomChozoBit(uint16 p, uint16 k) {  // 0x848865
  uint16 v3 = plm_room_arguments[k >> 1];
  if (!sign16(v3)) {
    int idx = PrepareBitAccess(v3);
    room_chozo_bits[idx] |= bitmask;
  }
  return p;
}

static uint16 PlmInstr_GotoIfItemBitSet(uint16 p, uint16 k) {  // 0x84887C
  uint16 v2 = plm_room_arguments[k >> 1];
  int idx = PrepareBitAccess(v2);
  if (!sign16(v2) && (bitmask & item_bit_array[idx]) != 0)
    return PlmInstr_Goto(p, k);
  else
    return p + 2;
}

static uint16 PlmInstr_SetItemBit(uint16 p, uint16 k) {  // 0x848899
  uint16 v2 = plm_room_arguments[k >> 1];
  if (!sign16(v2)) {
    uint16 v3 = PrepareBitAccess(v2);
    item_bit_array[v3] |= bitmask;
  }
  return p;
}

static uint16 PlmInstr_PickupBeamAndShowMessage(uint16 p, uint16 k) {  // 0x8488B0
  uint16 t = B84W(p);
  collected_beams |= t;
  equipped_beams |= t;
  equipped_beams &= ~((t << 1) & 8);
  equipped_beams &= ~((t >> 1) & 4);
  UpdateBeamTilesAndPalette();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(B84B(p + 2));
  return p + 3;
}

static uint16 PlmInstr_PickupEquipmentAndShowMessage(uint16 p, uint16 k) {  // 0x8488F3
  uint16 t = B84W(p);
  equipped_items |= t;
  collected_items |= t;
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(B84B(p + 2));
  return p + 3;
}

static uint16 PlmInstr_PickupEquipmentAddGrappleShowMessage(uint16 p, uint16 k) {  // 0x84891A
  uint16 t = B84W(p);
  equipped_items |= t;
  collected_items |= t;
  AddGrappleToHudTilemap();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(5);
  return p + 2;
}

static uint16 PlmInstr_PickupEquipmentAddXrayShowMessage(uint16 p, uint16 k) {  // 0x848941
  uint16 t = B84W(p);
  equipped_items |= t;
  collected_items |= t;
  AddXrayToHudTilemap();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(6);
  return p + 2;
}

static uint16 PlmInstr_CollectHealthEnergyTank(uint16 p, uint16 k) {  // 0x848968
  samus_max_health += B84W(p);
  samus_health = samus_max_health;
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(1);
  return p + 2;
}

static uint16 PlmInstr_CollectHealthReserveTank(uint16 p, uint16 k) {  // 0x848986
  samus_max_reserve_health += B84W(p);
  if (!reserve_health_mode)
    ++reserve_health_mode;
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(0x19);
  return p + 2;
}

static uint16 PlmInstr_CollectAmmoMissileTank(uint16 p, uint16 k) {  // 0x8489A9
  uint16 t = B84W(p);
  samus_max_missiles += t;
  samus_missiles += t;
  AddMissilesToHudTilemap();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(2);
  return p + 2;
}

static uint16 PlmInstr_CollectAmmoSuperMissileTank(uint16 p, uint16 k) {  // 0x8489D2
  uint16 t = B84W(p);
  samus_max_super_missiles += t;
  samus_super_missiles += t;
  AddSuperMissilesToHudTilemap();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(3);
  return p + 2;
}

static uint16 PlmInstr_CollectAmmoPowerBombTank(uint16 p, uint16 k) {  // 0x8489FB
  uint16 t = B84W(p);
  samus_max_power_bombs += t;
  samus_power_bombs += t;
  AddPowerBombsToHudTilemap();
  PlayRoomMusicTrackAfterAFrames(0x168);
  DisplayMessageBox(4);
  return p + 2;
}

static uint16 PlmInstr_SetLinkReg(uint16 p, uint16 k) {  // 0x848A24
  plm_instruction_list_link_reg[k >> 1] = B84W(p);
  return p + 2;
}

static uint16 PlmInstr_Call(uint16 p, uint16 k) {  // 0x848A2E
  plm_instruction_list_link_reg[k >> 1] = p + 2;
  return (B84W(p));
}

static uint16 PlmInstr_Return(uint16 p, uint16 k) {  // 0x848A3A
  return (plm_instruction_list_link_reg[k >> 1]);
}

static uint16 PlmInstr_GotoIfDoorBitSet(uint16 p, uint16 k) {  // 0x848A72
  uint16 v2 = plm_room_arguments[k >> 1];
  if (sign16(v2))
    return p + 2;
  int idx = PrepareBitAccess(v2);
  if (!sign16(v2) && (bitmask & opened_door_bit_array[idx]) != 0)
    return (B84W(p));
  else
    return p + 2;
}

static uint16 PlmInstr_IncrementDoorHitCounterAndJGE(uint16 p, uint16 k) {  // 0x848A91
  int v2 = k >> 1;
  if ((uint8)++plm_variables[v2] < B84B(p + 0))
    return p + 3;
  uint16 v5 = plm_room_arguments[v2];
  if (!sign16(v5)) {
    int idx = PrepareBitAccess(v5);
    opened_door_bit_array[idx] |= bitmask;
    plm_room_arguments[v2] = (opened_door_bit_array[idx] | opened_door_bit_array[idx + 1] << 8) | 0x8000;   /* palabra */
  }
  plm_pre_instrs[v2] = addr_PlmPreInstr_Empty4;
  return PlmInstr_Goto(p + 1, k);
}

static uint16 PlmInstr_IncrementArgumentAndJGE(uint16 p, uint16 k) {  // 0x848ACD
  uint8 v2 = (uint8)plm_room_arguments[k >> 1] + 1;                 /* byte bajo */
  if (v2 >= B84B(p + 0)) {
    int v4 = k >> 1;
    plm_room_arguments[v4] = -1;
    plm_pre_instrs[v4] = addr_locret_848AE0;
    return PlmInstr_Goto(p + 1, k);
  } else {
    plm_room_arguments[k >> 1] = v2;
    return p + 3;
  }
}

static uint16 PlmInstr_SetBTS(uint16 p, uint16 k) {  // 0x848AF1
  bts_set(plm_block_indices[k >> 1] >> 1, B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_DrawPlmBlock(uint16 p, uint16 k) {  // 0x848B17
  int v2 = k >> 1;
  uint16 v3 = plm_variable[v2];
  level_set(plm_block_indices[v2] >> 1, v3);
  custom_draw_instr_plm_block = v3;
  custom_draw_instr_num_blocks = 1;
  custom_draw_instr_zero_terminator = 0;
  plm_instruction_timer[v2] = 1;
  plm_instruction_draw_ptr[v2] = 0x1E6D;       /* ADDR16_OF_RAM(custom_draw_instr_num_blocks): la instrucción en la RAM */
  plm_instr_list_ptrs[v2] = p;
  ProcessPlmDrawInstruction(k);
  uint16 v4 = plm_id;
  CalculatePlmBlockCoords(plm_id);
  DrawPlm(v4);
  return 0;
}

static uint16 PlmInstr_ProcessAirScrollUpdate(uint16 p, uint16 k) {  // 0x848B55
  plm_variable[k >> 1] = 0;
  for (uint16 q = plm_room_arguments[k >> 1]; !(B8FW(q) & 0x80); q += 2)   /* pares (scroll, valor) en $8F */
    scrolls[(uint8)B8FW(q)] = (uint8)B8FW(q + 1);
  int v7 = plm_block_indices[k >> 1] >> 1;
  level_set(v7, (level_get(v7) & 0xFFF) | 0x3000);
  return p;
}

static uint16 PlmInstr_ProcessSolidScrollUpdate(uint16 p, uint16 k) {  // 0x848B93
  plm_variable[k >> 1] = 0;
  for (uint16 q = plm_room_arguments[k >> 1]; !(B8FW(q) & 0x80); q += 2)   /* pares (scroll, valor) en $8F */
    scrolls[(uint8)B8FW(q)] = (uint8)B8FW(q + 1);
  int v7 = plm_block_indices[k >> 1] >> 1;
  level_set(v7, (level_get(v7) & 0xFFF) | 0xB000);
  return p;
}

static uint16 PlmInstr_QueueMusic(uint16 p, uint16 k) {  // 0x848BD1
  QueueMusic_Delayed8(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_ClearMusicQueueAndQueueTrack(uint16 p, uint16 k) {  // 0x848BDD
  for (int i = 14; i >= 0; i -= 2) {
    int v3 = i >> 1;
    music_queue_track[v3] = 0;
    music_queue_delay[v3] = 0;
  }
  music_queue_read_pos = music_queue_write_pos;
  music_timer = 0;
  music_entry = 0;
  QueueMusic_Delayed8(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx1_Max6(uint16 p, uint16 k) {  // 0x848C07
  QueueSfx1_Max6(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx2_Max6(uint16 p, uint16 k) {  // 0x848C10
  QueueSfx2_Max6(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx3_Max6(uint16 p, uint16 k) {  // 0x848C19
  QueueSfx3_Max6(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx1_Max15(uint16 p, uint16 k) {  // 0x848C22
  QueueSfx1_Max15(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx2_Max15(uint16 p, uint16 k) {  // 0x848C2B
  QueueSfx2_Max15(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx3_Max15(uint16 p, uint16 k) {  // 0x848C34
  QueueSfx3_Max15(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx1_Max3(uint16 p, uint16 k) {  // 0x848C3D
  QueueSfx1_Max3(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx2_Max3(uint16 p, uint16 k) {  // 0x848C46
  QueueSfx2_Max3(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx_Max3(uint16 p, uint16 k) {  // 0x848C4F
  QueueSfx3_Max3(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx1_Max9(uint16 p, uint16 k) {  // 0x848C58
  QueueSfx1_Max9(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx2_Max9(uint16 p, uint16 k) {  // 0x848C61
  QueueSfx2_Max9(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx3_Max9(uint16 p, uint16 k) {  // 0x848C6A
  QueueSfx3_Max9(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx1_Max1(uint16 p, uint16 k) {  // 0x848C73
  QueueSfx1_Max1(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx2_Max1(uint16 p, uint16 k) {  // 0x848C7C
  QueueSfx2_Max1(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_QueueSfx3_Max1(uint16 p, uint16 k) {  // 0x848C85
  QueueSfx3_Max1(B84B(p + 0));
  return p + 1;
}

static uint16 PlmInstr_ActivateMapStation(uint16 p, uint16 k) {  // 0x848C8F
  map_station_byte_array[area_index] = 0xFF;   /* |= 0xFF de 16 bits: el byte del área (el siguiente queda) */
  DisplayMessageBox(0x14);
  has_area_map = 1;
  return p;
}

static uint16 PlmInstr_ActivateEnergyStation(uint16 p, uint16 k) {  // 0x848CAF
  if (samus_max_health != samus_health) {
    DisplayMessageBox(0x15);
    samus_health = samus_max_health;
  }
  CallSomeSamusCode(1);
  return p;
}

static uint16 PlmInstr_ActivateMissileStation(uint16 p, uint16 k) {  // 0x848CD0
  if (samus_max_missiles != samus_missiles) {
    DisplayMessageBox(0x16);
    samus_missiles = samus_max_missiles;
  }
  CallSomeSamusCode(1);
  return p;
}

static uint16 PlmInstr_ActivateSaveStationAndGotoIfNo(uint16 p, uint16 k) {  // 0x848CF1
  int r = DisplayMessageBox_Poll(23);
  if (r < 0)
    return p - 2; // restart plm instr
  if (r == 2)
    return (B84W(p));
  SpawnEprojWithRoomGfx(addr_kEproj_SaveStationElectricity, 0);
  load_station_index = plm_room_arguments[plm_id >> 1] & 7;
  PrepareBitAccess(load_station_index);
  used_save_stations_and_elevators[2 * area_index] |= bitmask;
  SaveExploredMapTilesToSaved();   /* SaveToSram: la Genesis todavía no guarda partidas */
  return p + 2;
}

static uint16 PlmInstr_GotoIfSamusNear(uint16 p, uint16 k) {  // 0x848D41
  CalculatePlmBlockCoords(k);
  uint8 v2 = abs16((samus_x_pos >> 4) - plm_x_block);
  uint8 v4 = abs16((samus_y_pos >> 4) - plm_y_block);
  if (v2 <= B84B(p + 0) && v4 <= B84B(p + 1))
    return (B84W(p + 2));
  else
    return p + 4;
}


static uint16 PlmInstr_MovePlmDownOneBlock(uint16 p, uint16 k) {  // 0x84AB00
  plm_block_indices[k >> 1] += 2 * room_width_in_blocks;
  return p;
}

static uint8 PlmSetup_CrumbleBotwoonWall(uint16 j) {  // 0x84AB28
  plm_instruction_timer[j >> 1] = 64;
  return 0;
}

static uint16 PlmInstr_Scroll_0_1_Blue(uint16 p, uint16 k) {  // 0x84AB51
  scrolls[0] = scrolls[1] = 1;
  return p;
}

static uint16 PlmInstr_MovePlmDownOneBlock_0(uint16 p, uint16 k) {  // 0x84AB59
  plm_block_indices[k >> 1] += 2 * room_width_in_blocks;
  return p;
}

static uint16 PlmInstr_ABD6(uint16 p, uint16 k) {  // 0x84ABD6
  plm_block_indices[k >> 1] += 2;
  return p;
}

static void PlmPreInstr_PositionSamusAndInvincible(uint16 k) {  // 0x84AC89
  int v1 = k >> 1;
  samus_x_pos = plm_variable[v1];
  samus_y_pos = plm_variables[v1];
  samus_invincibility_timer |= 0x10;
}

static uint16 PlmInstr_DealDamage_2(uint16 p, uint16 k) {  // 0x84AC9D
  samus_periodic_damage += 2;
  return p;
}

static uint16 PlmInstr_GiveInvincibility(uint16 p, uint16 k) {  // 0x84ACB1
  samus_invincibility_timer = 48;
  return p;
}

static uint16 PlmInstr_Draw0x38FramesOfRightTreadmill(uint16 p, uint16 k) {  // 0x84AD43
  WriteRowOfLevelDataBlockAndBTS(k, 0x30ff, 0x8, 0x38);
  return p;
}

static uint16 PlmInstr_Draw0x38FramesOfLeftTreadmill(uint16 p, uint16 k) {  // 0x84AD58
  WriteRowOfLevelDataBlockAndBTS(k, 0x30ff, 0x9, 0x38);
  return p;
}

static uint16 PlmInstr_GotoIfSamusHealthFull(uint16 p, uint16 k) {  // 0x84AE35
  if (samus_max_health != samus_health)
    return p + 2;
  CallSomeSamusCode(1);
  return (B84W(p));
}

static uint16 PlmInstr_GotoIfMissilesFull(uint16 p, uint16 k) {  // 0x84AEBF
  if (samus_max_missiles != samus_missiles)
    return p + 2;
  CallSomeSamusCode(1);
  return (B84W(p));
}

static uint16 PlmInstr_PlaceSamusOnSaveStation(uint16 p, uint16 k) {  // 0x84B00E
  samus_x_pos = (samus_x_pos + 8) & 0xFFF0;
  MakeSamusFaceForward();
  return p;
}

static uint16 PlmInstr_DisplayGameSavedMessageBox(uint16 p, uint16 k) {  // 0x84B024
  DisplayMessageBox(0x18);
  return p;
}

static uint16 PlmInstr_EnableMovementAndSetSaveStationUsed(uint16 p, uint16 k) {  // 0x84B030
  CallSomeSamusCode(1);
  save_station_lockout_flag = 1;
  return p;
}

static uint8 PlmSetup_SetrupWreckedShipEntrance(uint16 j) {  // 0x84B04A
  int16 v2;

  uint16 v1 = plm_block_indices[j >> 1];
  v2 = 56;
  do {
    level_set(v1 >> 1, 255);
    v1 += 2;
    --v2;
  } while (v2);
  return 0;
}

static uint8 PlmSetup_BTS_Brinstar_0x80_Floorplant(uint16 j) {  // 0x84B0DC
  if (((samus_y_radius + samus_y_pos - 1) & 0xF) == 15) {
    int v1 = plm_block_indices[j >> 1] >> 1;
    level_set(v1, level_get(v1) & (0x8FFF));
    int v2 = j >> 1;
    plm_variable[v2] = samus_x_pos;
    plm_variables[v2] = samus_y_pos - 1;
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 0;
}

static uint8 PlmSetup_BTS_Brinstar_0x81_Ceilingplant(uint16 j) {  // 0x84B113
  if (((samus_y_pos - samus_y_radius) & 0xF) != 0) {
    plm_header_ptr[j >> 1] = 0;
  } else {
    int v1 = plm_block_indices[j >> 1] >> 1;
    level_set(v1, level_get(v1) & (0x8FFF));
    int v2 = j >> 1;
    plm_variable[v2] = samus_x_pos;
    plm_variables[v2] = samus_y_pos + 1;
  }
  return 0;
}

static uint8 ActivateStationIfSamusCannonLinedUp(uint16 a, uint16 j) {  // 0x84B146
  uint16 v2 = 78;
  while (a != plm_block_indices[v2 >> 1]) {
    v2 -= 2;
    if ((v2 & 0x8000) != 0)
      goto LABEL_7;
  }
  CalculatePlmBlockCoords(plm_id);
  if (((uint16)(16 * plm_y_block) | 0xB) == samus_y_pos) {
    int v3 = v2 >> 1;
    plm_instr_list_ptrs[v3] = plm_instruction_list_link_reg[v3];
    plm_instruction_timer[v3] = 1;
    CallSomeSamusCode(6);
    return 1;
  }
LABEL_7:
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static uint8 PlmSetup_B6D3_MapStation(uint16 j) {  // 0x84B18B
  int v1 = j >> 1;
  int v2 = plm_block_indices[v1] >> 1;
  level_set(v2, (level_get(v2) & 0xFFF) | 0x8000);
  if (map_station_byte_array[area_index]) {
    plm_instr_list_ptrs[v1] = addr_word_84AD76;
  } else {
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, 0xB047);
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] - 4, 0xB048);
  }
  return 0;
}

static uint8 PlmSetup_Bts47_MapStationRightAccess(uint16 j) {  // 0x84B1C8
  if ((samus_collision_direction & 0xF) == 0 && samus_pose == kPose_8A_FaceL_Ranintowall && (samus_pose_x_dir & 4) != 0)
    return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] - 2, j);
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static uint8 PlmSetup_Bts4_MapStationLeftAccess(uint16 j) {  // 0x84B1F0
  if ((samus_collision_direction & 0xF) == 1 && samus_pose == kPose_89_FaceR_Ranintowall && (samus_pose_x_dir & 8) != 0)
    return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] + 4, j);
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static uint8 PlmSetup_PlmB6DF_EnergyStation(uint16 j) {  // 0x84B21D
  int v1 = j >> 1;
  int v2 = plm_block_indices[v1] >> 1;
  level_set(v2, (level_get(v2) & 0xFFF) | 0x8000);
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, 0xB049);
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] - 2, FUNC16(PlmSetup_SetrupWreckedShipEntrance));
  return 0;
}

static uint8 PlmSetup_PlmB6EB_EnergyStation(uint16 j) {  // 0x84B245
  int v1 = j >> 1;
  int v2 = plm_block_indices[v1] >> 1;
  level_set(v2, (level_get(v2) & 0xFFF) | 0x8000);
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, 0xB04B);
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] - 2, 0xB04C);
  return 0;
}

static uint8 PlmSetup_B6E3_EnergyStationRightAccess(uint16 j) {  // 0x84B26D
  if ((samus_collision_direction & 0xF) != 0
      || samus_pose != kPose_8A_FaceL_Ranintowall
      || (samus_pose_x_dir & 4) == 0
      || samus_health == samus_max_health) {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  } else {
    return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] - 2, j);
  }
}

static uint8 PlmSetup_B6E7_EnergyStationLeftAccess(uint16 j) {  // 0x84B29D
  if ((samus_collision_direction & 0xF) == 1
      && samus_pose == kPose_89_FaceR_Ranintowall
      && (samus_pose_x_dir & 8) != 0
      && samus_health != samus_max_health) {
    return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] + 2, j);
  }
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static uint8 PlmSetup_B6EF_MissileStationRightAccess(uint16 j) {  // 0x84B2D0
  if ((samus_collision_direction & 0xF) != 0
      || samus_pose != kPose_8A_FaceL_Ranintowall
      || (samus_pose_x_dir & 4) == 0
      || samus_missiles == samus_max_missiles) {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  }
  return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] - 2, j);
}

static uint8 PlmSetup_B6F3_MissileStationLeftAccess(uint16 j) {  // 0x84B300
  if ((samus_collision_direction & 0xF) == 1
      && samus_pose == kPose_89_FaceR_Ranintowall
      && (samus_pose_x_dir & 8) != 0
      && samus_missiles != samus_max_missiles) {
    return ActivateStationIfSamusCannonLinedUp(plm_block_indices[j >> 1] + 2, j);
  } else {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  }
}

static void DeletePlm(uint16 j) {  // 0x84B333
  plm_header_ptr[j >> 1] = 0;
}

static uint8 PlmSetup_B638_Rightwards_Extension(uint16 j) {  // 0x84B33A
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x50FF);
  DeletePlm(j);
  return 0;
}

static uint8 PlmSetup_B63F_Leftwards_Extension(uint16 j) {  // 0x84B345
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x5001);
  DeletePlm(j);
  return 0;
}

static uint8 PlmSetup_B643_Downwards_Extension(uint16 j) {  // 0x84B350
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xD0FF);
  DeletePlm(j);
  return 0;
}

static uint8 PlmSetup_B647_Upwards_Extension(uint16 j) {  // 0x84B35B
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xD001);
  DeletePlm(j);
  return 0;
}

static void SkipDebugDrawInstructionsForScrollPlms(uint16 j) {  // 0x84B366
  plm_instr_list_ptrs[j >> 1] += 4;
}

static uint8 PlmSetup_B703_ScrollPLM(uint16 j) {  // 0x84B371
  int v1 = j >> 1;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0x3046);
  plm_variable[v1] = 0;
  SkipDebugDrawInstructionsForScrollPlms(j);
  return 0;
}

static uint8 PlmSetup_B707_SolidScrollPLM(uint16 j) {  // 0x84B382
  int v1 = j >> 1;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0xB046);
  plm_variable[v1] = 0;
  SkipDebugDrawInstructionsForScrollPlms(j);
  return 0;
}

static uint8 PlmSetup_B6FF_ScrollBlockTouch(uint16 j) {  // 0x84B393
  int v1 = j >> 1;
  uint16 v2 = plm_block_indices[v1];
  plm_block_indices[v1] = 0;
  uint16 v3 = 78;
  while (v2 != plm_block_indices[v3 >> 1]) {
    v3 -= 2;
    if ((v3 & 0x8000) != 0) {
      nt_unimpl("PlmSetup_B6FF_ScrollBlockTouch (sin PLM de scroll: el SNES se cuelga)");
      return 0;
    }
  }
  int v4 = v3 >> 1;
  if ((plm_variable[v4] & 0x8000) == 0) {
    plm_variable[v4] = 0x8000;
    ++plm_instr_list_ptrs[v4];
    ++plm_instr_list_ptrs[v4];
    plm_instruction_timer[v4] = 1;
  }
  return 0;
}

static uint8 PlmSetup_DeactivatePlm(uint16 j) {  // 0x84B3C1
  int v1 = plm_block_indices[j >> 1] >> 1;
  level_set(v1, level_get(v1) & (0x8FFF));
  return 0;
}

static uint8 PlmSetup_ReturnCarryClear(uint16 j) {  // 0x84B3D0
  return 0;
}

static uint8 PlmSetup_ReturnCarrySet(uint16 j) {  // 0x84B3D2
  return 1;
}

static uint8 PlmSetup_D094_EnemyBreakableBlock(uint16 j) {  // 0x84B3D4
  int v1 = plm_block_indices[j >> 1] >> 1;
  level_set(v1, level_get(v1) & (0xFFF));
  return 0;
}

static uint8 UNUSED_sub_84B3E3(uint16 j) {  // 0x84B3E3
  return 0;
}

static uint8 PlmSetup_B70F_IcePhysics(uint16 j) {  // 0x84B3EB
  if (((samus_y_radius + samus_y_pos - 1) & 0xF) == 7 || ((samus_y_radius + samus_y_pos - 1) & 0xF) == 15) {
    samus_x_decel_mult = 16; samus_x_decel_mult_hi = 0;   /* palabra de 7E:0B4C */
  }
  return 0;
}


static const uint16 g_word_84B48B[2] = { 0x200, 0x200 };
static const uint16 g_word_84B48F[2] = { 0x120, 0x100 };
static const uint16 g_word_84B493[2] = { 0x280, 0x380 };

static uint8 PlmSetup_QuicksandSurface(uint16 j) {

  samus_has_momentum_flag = 0;
  speed_boost_counter = 0;
  samus_echoes_sound_flag = 0;
  samus_x_extra_run_subspeed = 0;
  samus_x_extra_run_speed = 0;
  samus_x_base_subspeed &= ~0x8000;
  samus_x_base_speed = 0;
  uint16 v1 = 0;
  if ((equipped_items & 0x20) != 0)
    v1 = 2;
  if (!inside_block_reaction_samus_point)
    switch (samus_y_dir & 3) {                      // kPlmSetup_QuicksandSurface ($84:B408)
    case 1: PlmSetup_QuicksandSurface_1(v1); break;
    case 2: PlmSetup_QuicksandSurface_2(v1); break;
    default: PlmSetup_QuicksandSurface_0(v1); break;
    }
  return 0;
}

static void PlmSetup_QuicksandSurface_0(uint16 j) {  // 0x84B447
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  extra_samus_y_subdisplacement = 0;
  extra_samus_y_displacement = 0;
  set_mid16(&extra_samus_y_subdisplacement, &extra_samus_y_displacement, g_word_84B48F[j >> 1]);
}

static void PlmSetup_QuicksandSurface_1(uint16 j) {  // 0x84B45A
  int v1 = j >> 1;
  if (g_word_84B493[v1] < get_mid16(samus_y_subspeed, samus_y_speed)) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    set_mid16(&samus_y_subspeed, &samus_y_speed, g_word_84B493[v1]);
  }
  extra_samus_y_subdisplacement = 0;
  extra_samus_y_displacement = 0;
  set_mid16(&extra_samus_y_subdisplacement, &extra_samus_y_displacement, g_word_84B48B[v1]);
}

static void PlmSetup_QuicksandSurface_2(uint16 j) {  // 0x84B47B
  extra_samus_y_subdisplacement = 0;
  extra_samus_y_displacement = 0;
  set_mid16(&extra_samus_y_subdisplacement, &extra_samus_y_displacement, g_word_84B48B[j >> 1]);
  autojump_timer = 0;
}

static uint8 PlmSetup_B71F_SubmergingQuicksand(uint16 j) {  // 0x84B497
  autojump_timer = 0;
  extra_samus_y_subdisplacement = 0x2000;
  extra_samus_y_displacement = 1;
  return 0;
}

static uint8 PlmSetup_B723_SandfallsSlow(uint16 j) {  // 0x84B4A8
  extra_samus_y_subdisplacement = 0x4000;
  extra_samus_y_displacement = 1;
  return 0;
}

static uint8 PlmSetup_B727_SandFallsFast(uint16 j) {  // 0x84B4B6
  extra_samus_y_subdisplacement = -16384;
  extra_samus_y_displacement = 1;
  return 0;
}

extern int32 *nt_cur_coll_amt;   /* coll.c: la cantidad de la colisión en curso */
static uint8 PlmSetup_QuicksandSurfaceB(uint16 j) {  // arena de Maridia: frena la caída de la colisión en curso
  if ((samus_collision_direction & 2) == 0)
    return 0;
  if (!nt_cur_coll_amt)
    return 0;
  switch (samus_y_dir & 3) {
  case 0: case 3:
    if ((samus_collision_direction & 0xF) == 3) {
      if (samus_contact_damage_index == 1) {
        *nt_cur_coll_amt = 0;
        return 1;
      }
      if ((uint16)(*nt_cur_coll_amt >> 8) > 0x30)
        *nt_cur_coll_amt = (*nt_cur_coll_amt & 0xff0000ff) | 0x3000;
      flag_samus_in_quicksand++;
    }
    break;
  case 2:
    if (samus_contact_damage_index == 1) {
      *nt_cur_coll_amt = 0;
      return 1;
    }
    flag_samus_in_quicksand++;
    return 0;
  }
  return 0;
}

static uint8 PlmSetup_B737_SubmergingQuicksand(uint16 j) {  // 0x84B541
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_y_subaccel = 0;
  samus_y_accel = 0;
  return 0;
}

static uint8 PlmSetup_B73B_B73F_SandFalls(uint16 j) {  // 0x84B54F
  return 0;
}

static uint8 PlmSetup_ClearShitroidInvisibleWall(uint16 j) {  // 0x84B551
  int16 v2;

  uint16 v1 = plm_block_indices[j >> 1];
  v2 = 10;
  do {
    level_set(v1 >> 1, level_get(v1 >> 1) & (0xFFF));
    v1 += room_width_in_blocks * 2;
    --v2;
  } while (v2);
  return 0;
}

static uint8 PlmSetup_B767_ClearShitroidInvisibleWall(uint16 j) {  // 0x84B56F
  int16 v2;

  uint16 v1 = plm_block_indices[j >> 1];
  v2 = 10;
  do {
    level_set(v1 >> 1, (level_get(v1 >> 1) & 0xFFF) | 0x8000);
    v1 += room_width_in_blocks * 2;
    --v2;
  } while (v2);
  return 0;
}

static uint8 PlmSetup_B76B_SaveStationTrigger(uint16 j) {  // 0x84B590
  if (!power_bomb_explosion_status
      && (samus_pose == kPose_01_FaceR_Normal || samus_pose == kPose_02_FaceL_Normal)
      && !save_station_lockout_flag
      && (samus_collision_direction & 0xF) == 3) {
    CalculatePlmBlockCoords(j);
    if ((uint16)(samus_x_pos - 8) >> 4 == plm_x_block) {
      int v1 = j >> 1;
      uint16 v2 = plm_block_indices[v1];
      plm_block_indices[v1] = 0;
      plm_header_ptr[v1] = 0;
      uint16 v3 = 78;
      while (v2 != plm_block_indices[v3 >> 1]) {
        v3 -= 2;
        if ((v3 & 0x8000) != 0)
          return 1;
      }
      int v4 = v3 >> 1;
      ++plm_instr_list_ptrs[v4];
      ++plm_instr_list_ptrs[v4];
      plm_instruction_timer[v4] = 1;
    }
  }
  return 1;
}

static uint8 PlmSetup_B76F_SaveStation(uint16 j) {  // 0x84B5EE
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xB04D);
  return 0;
}

static uint8 PlmSetup_MotherBrainRoomEscapeDoor(uint16 j) {  // 0x84B5F8
  uint16 v1 = plm_block_indices[j >> 1];
  WriteLevelDataBlockTypeAndBts(v1, 0x9001);
  uint16 v2 = room_width_in_blocks * 2 + v1;
  WriteLevelDataBlockTypeAndBts(v2, 0xD0FF);
  uint16 v3 = room_width_in_blocks * 2 + v2;
  WriteLevelDataBlockTypeAndBts(v3, 0xD0FF);
  WriteLevelDataBlockTypeAndBts(
    room_width_in_blocks * 2 + v3,
    0xD0FF);
  return 0;
}

static uint8 PlmSetup_B7EB_EnableSoundsIn32Frames(uint16 j) {  // 0x84B7C3
  uint16 v1;
  if (area_index == 6)
    v1 = 32;
  else
    v1 = 240;
  int v2 = j >> 1;
  plm_timers[v2] = v1;
  plm_pre_instrs[v2] = FUNC16(PlmPreInstr_B7EB_DecTimerEnableSoundsDeletePlm);
  return 0;
}

static void PlmPreInstr_B7EB_DecTimerEnableSoundsDeletePlm(uint16 k) {  // 0x84B7DD
  int v2 = k >> 1;
  if (plm_timers[v2]-- == 1) {
    debug_disable_sounds = 0;
    plm_header_ptr[v2] = 0;
  }
}

static void PlmPreInstr_WakeAndLavaIfBoosterCollected(uint16 k) {  // 0x84B7EF
  if ((collected_items & 0x2000) != 0) {
    if ((fx_target_y_pos & 0x8000) != 0) {
      plm_header_ptr[k >> 1] = 0;
    } else {
      fx_y_vel = -128;
      int v1 = k >> 1;
      plm_instruction_timer[v1] = 1;
      ++plm_instr_list_ptrs[v1];
      ++plm_instr_list_ptrs[v1];
      plm_timers[v1] = 0;
    }
  } else {
    fx_target_y_pos = -1;
    fx_y_vel = 0;
    fx_timer = 0;
    earthquake_timer = 0;
    plm_header_ptr[k >> 1] = 0;
  }
}

static void PlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft(uint16 k) {  // 0x84B82A
  if (samus_x_pos <= 0xAE0) {
    fx_timer = 1;
    int v1 = k >> 1;
    plm_instruction_timer[v1] = 1;
    ++plm_instr_list_ptrs[v1];
    ++plm_instr_list_ptrs[v1];
    plm_timers[v1] = 0;
  }
}

static void PlmPreInstr_AdvanceLavaSamusMovesLeft(uint16 k) {  // 0x84B846
  static const uint16 g_word_84B876[10] = {
     0x72b,  0x1bf,
    0xff50,  0x50a,
     0x167, 0xff20,
     0x244,  0x100,
    0xff20, 0x8000,
  };


  int v1 = k >> 1;
  uint16 v2 = plm_timers[v1];
  int v3 = v2 >> 1;
  uint16 v4 = g_word_84B876[v3];
  if ((v4 & 0x8000) != 0) {
    SetEventHappened(0x15);
  } else if (v4 >= samus_x_pos) {
    if (g_word_84B876[v3 + 1] < fx_base_y_pos)
      fx_base_y_pos = g_word_84B876[v3 + 1];
    fx_y_vel = g_word_84B876[v3 + 2];
    plm_timers[v1] = v2 + 6;
  }
}

static uint8 PlmSetup_SpeedBoosterEscape(uint16 j) {  // 0x84B89C
  if (CheckEventHappened(0x15))
    plm_header_ptr[j >> 1] = 0;
  return 0;
}

static void PlmPreInstr_ShaktoolsRoom(uint16 k) {  // 0x84B8B0
  if (power_bomb_explosion_status) {
    scrolls[0] = scrolls[1] = scrolls[2] = scrolls[3] = 1;
  }
  if (samus_x_pos > 0x348) {
    SetEventHappened(0xD);
    plm_header_ptr[k >> 1] = 0;
  }
}

static uint8 PlmSetup_ShaktoolsRoom(uint16 j) {  // 0x84B8DC
  scrolls[0] = 1; scrolls[1] = 0;
  scrolls[2] = 0; scrolls[3] = 0;
  return 0;
}

static uint8 WakePlmIfSamusIsBelowAndRightOfTarget(uint16 k, uint16 x_r18, uint16 y_r20) {  // 0x84B8FD
  if (x_r18 < samus_x_pos && y_r20 < samus_y_pos) {
    int v2 = k >> 1;
    plm_instr_list_ptrs[v2] += 2;
    plm_instruction_timer[v2] = 1;
    return false;
  }
  return true;
}

static void PlmPreInstr_OldTourianEscapeShaftEscape(uint16 k) {  // 0x84B927
  if (!WakePlmIfSamusIsBelowAndRightOfTarget(k, 240, 2080))
    SpawnEprojWithRoomGfx(0xB4B1, 0);
}

static void PlmPreInstr_EscapeRoomBeforeOldTourianEscapeShaft(uint16 k) {  // 0x84B948
  if (!WakePlmIfSamusIsBelowAndRightOfTarget(k, 240, 1344)) {
    fx_y_vel = -104;
    fx_timer = 16;
  }
}

static uint8 PlmSetup_B974(uint16 j) {  // 0x84B96C
  // Implemented in BlockReact_ShootableAir
  return 0;
}

static uint8 PlmSetup_B9C1_CrittersEscapeBlock(uint16 j) {  // 0x84B978
  if (projectile_type[projectile_index >> 1]) {
    int v1 = j >> 1;
    int v2 = plm_block_indices[v1] >> 1;
    uint16 v3 = (level_get(v2) & 0xF000) | 0x9F;
    plm_variable[v1] = v3;
    level_set(v2, v3 & 0x8FFF);
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 0;
}

static uint16 PlmInstr_SetCrittersEscapedEvent(uint16 p, uint16 k) {  // 0x84B9B9
  SetEventHappened(0xF);
  return p;
}

static uint8 PlmSetup_B9ED_CrittersEscapeBlock(uint16 j) {  // 0x84B9C5
  uint16 v1 = plm_block_indices[j >> 1];
  WriteLevelDataBlockTypeAndBts(v1, 0xC04F);
  uint16 v2 = v1 + room_width_in_blocks * 2;
  WriteLevelDataBlockTypeAndBts(v2, 0xD0FF);
  WriteLevelDataBlockTypeAndBts(v2 + room_width_in_blocks * 2, 0xD0FF);
  return 0;
}

static uint8 sub_84B9F1(uint16 j) {  // 0x84B9F1
  uint16 v1 = plm_block_indices[j >> 1];
  level_set(v1 >> 1, (level_get(v1 >> 1) & 0xFFF) | 0x8000);
  uint16 v2 = room_width_in_blocks * 2 + v1;
  level_set(v2 >> 1, (level_get(v2 >> 1) & 0xFFF) | 0x8000);
  uint16 v3 = room_width_in_blocks + room_width_in_blocks + v2;
  level_set(v3 >> 1, (level_get(v3 >> 1) & 0xFFF) | 0x8000);
  int v4 = (uint16)(room_width_in_blocks * 2 + v3) >> 1;
  level_set(v4, (level_get(v4) & 0xFFF) | 0x8000);
  return 0;
}

static uint16 PlmInstr_JumpIfSamusHasNoBombs(uint16 p, uint16 k) {  // 0x84BA6F
  if ((collected_items & 0x1000) != 0)
    return p + 2;
  else
    return (B84W(p));
}

static uint8 PlmSetup_BB30_CrateriaMainstreetEscape(uint16 j) {  // 0x84BB09
  if (!CheckEventHappened(0xF))
    plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint16 PlmInstr_MovePlmRight4Blocks(uint16 p, uint16 k) {  // 0x84BB25
  plm_block_indices[k >> 1] += 8;
  return p;
}

static void PlmPreInstr_WakePlmIfTriggered(uint16 k) {  // 0x84BB52
  int v1 = k >> 1;
  if (plm_timers[v1]) {
    ++plm_instr_list_ptrs[v1];
    ++plm_instr_list_ptrs[v1];
    plm_instruction_timer[v1] = 1;
    plm_pre_instrs[v1] = addr_locret_84BB6A;
  }
}

static void PlmPreInstr_WakePlmIfTriggeredOrSamusBelowPlm(uint16 k) {  // 0x84BB6B
  CalculatePlmBlockCoords(k);
  if ((samus_x_pos >> 4 == plm_x_block && (uint16)((samus_y_pos >> 4) - plm_y_block) < 5)
      || plm_timers[k >> 1]) {
    int v1 = k >> 1;
    ++plm_instr_list_ptrs[v1];
    ++plm_instr_list_ptrs[v1];
    plm_instruction_timer[v1] = 1;
    plm_pre_instrs[v1] = FUNC16(PlmPreInstr_Empty5);
  }
}

static void PlmPreInstr_WakePlmIfTriggeredOrSamusAbovePlm(uint16 k) {  // 0x84BBA4
  CalculatePlmBlockCoords(k);
  if ((samus_x_pos >> 4 == plm_x_block && (uint16)((samus_y_pos >> 4) - plm_y_block) >= 0xFFFC)
      || plm_timers[k >> 1]) {
    int v1 = k >> 1;
    ++plm_instr_list_ptrs[v1];
    ++plm_instr_list_ptrs[v1];
    plm_instruction_timer[v1] = 1;
    plm_pre_instrs[v1] = addr_locret_84BBDC;
  }
}

static uint16 PlmInstr_ClearTrigger(uint16 p, uint16 k) {  // 0x84BBDD
  plm_timers[k >> 1] = 0;
  return p;
}

static uint16 PlmInstr_SpawnEproj(uint16 p, uint16 k) {  // 0x84BBE1
  SpawnEprojWithRoomGfx(B84W(p), 0);
  return p + 2;
}

static uint16 PlmInstr_WakeEprojAtPlmPos(uint16 p, uint16 k) {  // 0x84BBF0
  int i;

  uint16 v2 = plm_block_indices[k >> 1];
  for (i = 34; i >= 0; i -= 2) {
    if (v2 == eproj_E[i >> 1])
      break;
  }
  int v4 = i >> 1;
  eproj_instr_timers[v4] = 1;
  eproj_instr_list_ptr[v4] += 2;
  return p + 2;
}

static void PlmPreInstr_GoToLinkInstrIfShot(uint16 k) {  // 0x84BD0F
  int v2 = k >> 1;
  if (plm_timers[v2]) {
    plm_timers[v2] = 0;
    plm_instr_list_ptrs[v2] = plm_instruction_list_link_reg[v2];
    plm_instruction_timer[v2] = 1;
  }
}

static void PlmPreInstr_GoToLinkInstrIfShotWithPowerBomb(uint16 k) {  // 0x84BD26
  int v2 = k >> 1;
  uint16 v3 = plm_timers[v2];
  if (v3) {
    if ((v3 & 0xF00) == 768) {
      plm_timers[v2] = 0;
      plm_instr_list_ptrs[v2] = plm_instruction_list_link_reg[v2];
      plm_instruction_timer[v2] = 1;
      return;
    }
    QueueSfx2_Max6(0x57);
  }
  plm_timers[v2] = 0;
}

static void PlmPreInstr_GoToLinkInstrIfShotWithAnyMissile(uint16 k) {  // 0x84BD50
  int16 v3;

  uint16 v2 = plm_timers[k >> 1];
  if (v2) {
    v3 = v2 & 0xF00;
    if (v3 == 512) {
      plm_variables[k >> 1] = 119;
      goto LABEL_4;
    }
    if (v3 == 256) {
LABEL_4:;
      int v4 = k >> 1;
      plm_timers[v4] = 0;
      plm_instr_list_ptrs[v4] = plm_instruction_list_link_reg[v4];
      plm_instruction_timer[v4] = 1;
      return;
    }
    QueueSfx2_Max6(0x57);
  }
  plm_timers[k >> 1] = 0;
}

static void PlmPreInstr_GoToLinkInstrIfShotWithSuperMissile(uint16 k) {  // 0x84BD88
  int v2 = k >> 1;
  uint16 v3 = plm_timers[v2];
  if (v3) {
    if ((v3 & 0xF00) == 512) {
      plm_timers[v2] = 0;
      plm_instr_list_ptrs[v2] = plm_instruction_list_link_reg[v2];
      plm_instruction_timer[v2] = 1;
      return;
    }
    QueueSfx2_Max6(0x57);
  }
  plm_timers[v2] = 0;
}

static void PlmPreInstr_GoToLinkInstruction(uint16 k) {  // 0x84BDB2
  int v2 = k >> 1;
  plm_timers[v2] = 0;
  plm_instr_list_ptrs[v2] = plm_instruction_list_link_reg[v2];
  plm_instruction_timer[v2] = 1;
}

static void PlmPreInstr_PlayDudSound(uint16 k) {  // 0x84BE1C
  int v2 = k >> 1;
  if (plm_timers[v2])
    QueueSfx2_Max6(0x57);
  plm_timers[v2] = 0;
}

static void PlmPreInstr_GotoLinkIfBoss1Dead(uint16 k) {  // 0x84BDD4
  if (CheckBossBitForCurArea(1) & 1)
    PlmPreInstr_GoToLinkInstruction(k);
  else
    PlmPreInstr_PlayDudSound(k);
}

static void PlmPreInstr_GotoLinkIfMiniBossDead(uint16 k) {  // 0x84BDE3
  if (CheckBossBitForCurArea(2) & 1)
    PlmPreInstr_GoToLinkInstruction(k);
  else
    PlmPreInstr_PlayDudSound(k);
}

static void PlmPreInstr_GotoLinkIfTorizoDead(uint16 k) {  // 0x84BDF2
  if (CheckBossBitForCurArea(4) & 1)
    PlmPreInstr_GoToLinkInstruction(k);
  else
    PlmPreInstr_PlayDudSound(k);
}

static void PlmPreInstr_GotoLinkIfEnemyDeathQuotaOk(uint16 k) {  // 0x84BE01
  if (num_enemies_killed_in_room < num_enemy_deaths_left_to_clear) {
    PlmPreInstr_PlayDudSound(k);
  } else {
    SetEventHappened(0);
    PlmPreInstr_GoToLinkInstruction(k);
  }
}

static void PlmPreInstr_GotoLinkIfTourianStatueFinishedProcessing(uint16 k) {  // 0x84BE1F
  if ((tourian_entrance_statue_finished & 0x8000) == 0)
    PlmPreInstr_PlayDudSound(k);
  else
    PlmPreInstr_GoToLinkInstruction(k);
}

static void PlmPreInstr_GotoLinkIfCrittersEscaped(uint16 k) {  // 0x84BE30
  if (CheckEventHappened(0xF) & 1)
    PlmPreInstr_GoToLinkInstruction(k);
  else
    PlmPreInstr_PlayDudSound(k);
}

static uint16 PlmInstr_SetGreyDoorPreInstr(uint16 p, uint16 k) {  // 0x84BE3F
  plm_pre_instrs[k >> 1] = B84W(0xBE4B + (plm_variable[k >> 1] & ~1));
  return p;
}

static uint8 PlmSetup_C806_LeftGreenGateTrigger(uint16 j) {  // 0x84C54D
  if ((projectile_type[projectile_index >> 1] & 0xFFF) == 512)
    return sub_84C63F(j);
  QueueSfx2_Max6(0x57);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C80A_RightGreenGateTrigger(uint16 j) {  // 0x84C56C
  if ((projectile_type[projectile_index >> 1] & 0xFFF) == 512)
    return sub_84C647(j);
  QueueSfx2_Max6(0x57);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C80E_LeftRedGateTrigger(uint16 j) {  // 0x84C58B
  int16 v1;

  v1 = projectile_type[projectile_index >> 1] & 0xFFF;
  if (v1 == 256 || v1 == 512)
    return sub_84C63F(j);
  QueueSfx2_Max6(0x57);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C812_RightRedGateTrigger(uint16 j) {  // 0x84C5AF
  int16 v1;

  v1 = projectile_type[projectile_index >> 1] & 0xFFF;
  if (v1 == 256 || v1 == 512)
    return sub_84C647(j);
  QueueSfx2_Max6(0x57);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C822_RightYellowGateTrigger(uint16 j) {  // 0x84C5F1
  if ((projectile_type[projectile_index >> 1] & 0xFFF) != 768)
    return sub_84C647(j);
  QueueSfx2_Max6(0x57);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C81E_LeftYellowGateTrigger(uint16 j) {  // 0x84C5D3
  if ((projectile_type[projectile_index >> 1] & 0xFFF) == 768)
    return sub_84C63F(j);
  QueueSfx2_Max6(0x57);
  int v1 = j >> 1;
  plm_header_ptr[v1] = 0;
  return PlmSetup_C822_RightYellowGateTrigger(j);
}

static uint8 PlmSetup_C81A_RightBlueGateTrigger(uint16 j) {  // 0x84C627
  if ((projectile_type[projectile_index >> 1] & 0xFFF) != 768)
    return sub_84C647(j);
  plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_C816_LeftBlueGateTrigger(uint16 j) {  // 0x84C610
  if ((projectile_type[projectile_index >> 1] & 0xFFF) != 768)
    return sub_84C63F(j);
  plm_header_ptr[j >> 1] = 0;
  return PlmSetup_C81A_RightBlueGateTrigger(j);
}

static uint8 sub_84C63F(uint16 j) {  // 0x84C63F
  return sub_84C64C(j, plm_block_indices[j >> 1] + 2);
}

static uint8 sub_84C647(uint16 j) {  // 0x84C647
  return sub_84C64C(j, plm_block_indices[j >> 1] - 2);
}

static uint8 sub_84C64C(uint16 j, uint16 a) {  // 0x84C64C
  uint16 v2 = 78;
  while (a != plm_block_indices[v2 >> 1]) {
    v2 -= 2;
    if ((v2 & 0x8000) != 0)
      goto LABEL_7;
  }
  int v3;
  v3 = v2 >> 1;
  if (!plm_timers[v3])
    ++plm_timers[v3];
LABEL_7:
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static void SetBts0x10FiveStepsDown(uint16 j) {  // 0x84C66A
  uint16 v1 = SetBtsTo0x10AdvanceRow(plm_block_indices[j >> 1] >> 1);
  uint16 v2 = SetBtsTo0x10AdvanceRow(v1);
  uint16 v3 = SetBtsTo0x10AdvanceRow(v2);
  uint16 v4 = SetBtsTo0x10AdvanceRow(v3);
  SetBtsTo0x10AdvanceRow(v4);
}

static uint16 SetBtsTo0x10AdvanceRow(uint16 k) {  // 0x84C67F
  bts_set(k, 0x10);                               /* palabra: BTS[k] = 0x10, BTS[k+1] igual */
  return room_width_in_blocks + k;
}

static void SetBts0x10FiveStepsUp(uint16 j) {  // 0x84C694
  uint16 v1 = SetBtsTo0x10AdvanceRowUp(plm_block_indices[j >> 1] >> 1);
  uint16 v2 = SetBtsTo0x10AdvanceRowUp(v1);
  uint16 v3 = SetBtsTo0x10AdvanceRowUp(v2);
  uint16 v4 = SetBtsTo0x10AdvanceRowUp(v3);
  SetBtsTo0x10AdvanceRowUp(v4);
}

static uint16 SetBtsTo0x10AdvanceRowUp(uint16 k) {  // 0x84C6A9
  bts_set(k, 0x10);                               /* palabra: BTS[k] = 0x10, BTS[k+1] igual */
  return k - room_width_in_blocks;
}

static uint8 PlmSetup_C82A_DownwardsClosedGate(uint16 j) {  // 0x84C6BE
  SpawnEprojWithRoomGfx(0xE659, 0);
  SetBts0x10FiveStepsDown(j);
  return 0;
}

static uint8 PlmSetup_C832_UpwardsClosedGate(uint16 j) {  // 0x84C6CB
  SpawnEprojWithRoomGfx(0xE675, 0);
  SetBts0x10FiveStepsUp(j);
  return 0;
}

static uint8 PlmSetup_C826_DownwardsOpenGate(uint16 j) {  // 0x84C6D8
  SetBts0x10FiveStepsDown(j);
  return 0;
}

static uint8 PlmSetup_C82E_UpwardsOpenGate(uint16 j) {  // 0x84C6DC
  SetBts0x10FiveStepsUp(j);
  return 0;
}

static uint8 PlmSetup_C836_DownwardsGateShootblock(uint16 j) {  // 0x84C6E0
  int v1 = j >> 1;
  plm_instr_list_ptrs[v1] = B84W(0xC70A + (plm_room_arguments[v1] & ~1));
  uint16 v2 = B84W(0xC71A + (plm_room_arguments[v1] & ~1));
  if (v2)
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] - 2, v2);
  uint16 v3 = B84W(0xC72A + (plm_room_arguments[v1] & ~1));
  if (v3)
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, v3);
  return 0;
}

static uint8 PlmSetup_C73A_UpwardsGateShootblock(uint16 j) {  // 0x84C73A
  int v1 = j >> 1;
  plm_instr_list_ptrs[v1] = B84W(0xC764 + (plm_room_arguments[v1] & ~1));
  uint16 v2 = B84W(0xC774 + (plm_room_arguments[v1] & ~1));
  if (v2)
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] - 2, v2);
  uint16 v3 = B84W(0xC784 + (plm_room_arguments[v1] & ~1));
  if (v3)
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, v3);
  return 0;
}

static uint8 PlmSetup_C794_GreyDoor(uint16 j) {  // 0x84C794
  int v1 = j >> 1;
  plm_variable[v1] = (uint16)(HIBYTE(plm_room_arguments[v1]) & 0x7C) >> 1;
  plm_room_arguments[v1] &= ~0x7C00;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0xC044);
  return 0;
}

static uint8 PlmSetup_Door_Colored(uint16 j) {  // 0x84C7B1
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xC044);
  return 0;
}

static uint8 PlmSetup_Door_Blue(uint16 j) {  // 0x84C7BB
//  if (sign16(projectile_index))
//    printf("BUG: projectile_index invalid\n");
  if (!sign16(projectile_index) && (projectile_type[projectile_index >> 1] & 0xF00) == 768) {
    plm_header_ptr[j >> 1] = 0;
  } else {
    uint16 v1 = plm_block_indices[j >> 1];
    level_set(v1 >> 1, (level_get(v1 >> 1) & 0xFFF) | 0x8000);
  }
  return 0;
}

static uint8 PlmSetup_Door_Strange(uint16 j) {  // 0x84C7E2
  int v1 = j >> 1;
  uint16 v2 = plm_block_indices[v1];
  plm_block_indices[v1] = 0;
  uint16 v3 = 78;
  while (v2 != plm_block_indices[v3 >> 1]) {
    v3 -= 2;
    if ((v3 & 0x8000) != 0)
      return 0;
  }
  plm_timers[v3 >> 1] = (projectile_type[(int16)projectile_index >> 1] & 0x1FFF) | 0x8000;
  return 0;
}

static uint16 PlmInstr_SetBtsTo1(uint16 p, uint16 k) {  // 0x84CD93
  bts_set(plm_block_indices[k >> 1] >> 1, 1);
  return p;
}

static uint8 PlmSetup_D028_D02C_Unused(uint16 j) {  // 0x84CDC2
  if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack) {
    int v1 = j >> 1;
    int v2 = plm_block_indices[v1] >> 1;
    uint16 v3 = level_get(v2);
    plm_variable[v1] = v3;
    level_set(v2, v3 & 0xFFF);
    return 0;
  } else {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  }
}

static uint8 PlmSetup_RespawningSpeedBoostBlock(uint16 j) {  // 0x84CDEA
  if ((speed_boost_counter & 0xF00) == 1024
      || samus_pose == kPose_C9_FaceR_Shinespark_Horiz
      || samus_pose == kPose_CA_FaceL_Shinespark_Horiz
      || samus_pose == kPose_CB_FaceR_Shinespark_Vert
      || samus_pose == kPose_CC_FaceL_Shinespark_Vert
      || samus_pose == kPose_CD_FaceR_Shinespark_Diag
      || samus_pose == kPose_CE_FaceL_Shinespark_Diag) {
    int v1 = j >> 1;
    int v2 = plm_block_indices[v1] >> 1;
    uint16 v3 = (level_get(v2) & 0xF000) | 0xB6;
    plm_variable[v1] = v3;
    level_set(v2, v3 & 0xFFF);
    return 0;
  } else {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  }
}

static uint8 PlmSetup_RespawningCrumbleBlock(uint16 j) {  // 0x84CE37
  if ((samus_collision_direction & 0xF) == 3) {
    int v1 = j >> 1;
    int v2 = plm_block_indices[v1] >> 1;
    uint16 v3 = (level_get(v2) & 0xF000) | 0xBC;
    plm_variable[v1] = v3;
    level_set(v2, v3 & 0x8FFF);
    plm_instruction_timer[v1] = 4;
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 1;
}

static uint8 PlmSetup_RespawningShotBlock(uint16 j) {  // 0x84CE6B
  int v1 = j >> 1;
  int v2 = plm_block_indices[v1] >> 1;
  uint16 v3 = (level_get(v2) & 0xF000) | 0x52;
  plm_variable[v1] = v3;
  level_set(v2, v3 & 0x8FFF);
  return 0;
}

static uint8 PlmSetup_RespawningBombBlock(uint16 j) {  // 0x84CE83
  if ((speed_boost_counter & 0xF00) == 1024
      || samus_pose == kPose_81_FaceR_Screwattack
      || samus_pose == kPose_82_FaceL_Screwattack
      || samus_pose == kPose_C9_FaceR_Shinespark_Horiz
      || samus_pose == kPose_CA_FaceL_Shinespark_Horiz
      || samus_pose == kPose_CB_FaceR_Shinespark_Vert
      || samus_pose == kPose_CC_FaceL_Shinespark_Vert
      || samus_pose == kPose_CD_FaceR_Shinespark_Diag
      || samus_pose == kPose_CE_FaceL_Shinespark_Diag) {
    int v2 = j >> 1;
    int v3 = plm_block_indices[v2] >> 1;
    uint16 v4 = (level_get(v3) & 0xF000) | 0x58;
    plm_variable[v2] = v4;
    level_set(v3, v4 & 0xFFF);
    return 0;
  } else {
    plm_header_ptr[j >> 1] = 0;
    return 1;
  }
}

static uint8 PlmSetup_RespawningBombBlock2(uint16 j) {  // 0x84CEDA
  int16 v1;

  v1 = projectile_type[projectile_index >> 1] & 0xF00;
  if (v1 == 1280) {
    int v5 = j >> 1;
    plm_instr_list_ptrs[v5] += 3;
    int v6 = plm_block_indices[v5] >> 1;
    uint16 v7 = (level_get(v6) & 0xF000) | 0x58;
    plm_variable[v5] = v7;
    level_set(v6, v7 & 0x8FFF);
  } else if (v1 == 768) {
    int v2 = j >> 1;
    int v3 = plm_block_indices[v2] >> 1;
    uint16 v4 = (level_get(v3) & 0xF000) | 0x58;
    plm_variable[v2] = v4;
    level_set(v3, v4 & 0x8FFF);
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 0;
}

static uint8 PlmSetup_RespawningPowerBombBlock(uint16 j) {  // 0x84CF2E
  int16 v1;

  v1 = projectile_type[projectile_index >> 1] & 0xF00;
  if (v1 == 1280) {
    plm_instr_list_ptrs[j >> 1] = addr_kPlmInstrList_C91C;
  } else if (v1 == 768) {
    int v2 = j >> 1;
    int v3 = plm_block_indices[v2] >> 1;
    uint16 v4 = (level_get(v3) & 0xF000) | 0x57;
    plm_variable[v2] = v4;
    level_set(v3, v4 & 0x8FFF);
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 0;
}

static uint8 PlmSetup_D08C_SuperMissileBlockRespawning(uint16 j) {  // 0x84CF67
  int16 v1;

  v1 = projectile_type[projectile_index >> 1] & 0xF00;
  if (v1 == 1280) {
    plm_instr_list_ptrs[j >> 1] = addr_kPlmInstrList_C922;
  } else if (v1 == 512) {
    int v2 = j >> 1;
    int v3 = plm_block_indices[v2] >> 1;
    uint16 v4 = (level_get(v3) & 0xF000) | 0x9F;
    plm_variable[v2] = v4;
    level_set(v3, v4 & 0x8FFF);
  } else {
    plm_header_ptr[j >> 1] = 0;
  }
  return 0;
}

static uint8 PlmSetup_D08C_CrumbleBlock(uint16 j) {  // 0x84CFA0
  if ((projectile_type[projectile_index >> 1] & 0xF00) != 1280)
    plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_D0DC_BreakableGrappleBlock(uint16 j) {  // 0x84CFB5
  int v1 = j >> 1;
  uint16 v2 = plm_block_indices[v1];
  plm_variable[v1] = level_get(v2 >> 1);
  bts_set(v2 >> 1, 0);                            /* palabra: BTS[k] = 0, BTS[k+1] igual */
  return 0x41;
}

static uint8 PlmSetup_D0D8_SetVFlag(uint16 j) {  // 0x84CFCD
  return 0x41;
}

static uint8 PlmSetup_D0D8_ClearVflag(uint16 j) {  // 0x84CFD1
  return 1;
}

static uint8 PlmSetup_D0E8_GiveSamusDamage(uint16 j) {  // 0x84CFD5
  ++samus_periodic_damage;
  return 0x41;
}

static uint8 PlmSetup_D113_LowerNorfairChozoRoomPlug(uint16 j) {  // 0x84D108
  level_set(plm_block_indices[j >> 1] >> 1, 0);   /* A sin definir en el original (el decomp usa 0) */
  return 0;
}

static uint8 PlmSetup_D127(uint16 j) {  // 0x84D117
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xC000);
  return 0;
}

static uint8 PlmSetup_D138(uint16 j) {  // 0x84D12B
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xE000);
  return 0;
}

static uint16 PlmInstr_FxBaseYPos_0x2D2(uint16 p, uint16 k) {  // 0x84D155
  fx_base_y_pos = 722;
  return p;
}

static void PlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed(uint16 k) {  // 0x84D15C
  uint16 prod = 8 * (uint8)room_width_in_blocks;
  uint16 v1 = 2 * (prod + 4);
  if (level_get(v1 >> 1) == 255) {
    WriteLevelDataBlockTypeAndBts(v1, 0xB083);
    plm_header_ptr[plm_id >> 1] = 0;
  }
}

static uint8 PlmSetup_D6DA_LowerNorfairChozoHandTrigger(uint16 j) {  // 0x84D18F
  if ((collected_items & 0x200) != 0
      && (samus_collision_direction & 0xF) == 3
      && (samus_pose == kPose_1D_FaceR_Morphball_Ground
          || samus_pose == kPose_79_FaceR_Springball_Ground
          || samus_pose == kPose_7A_FaceL_Springball_Ground)) {
    SetEventHappened(0xC);
    enemy_data[0].parameter_1 = 1;
    int v2 = plm_block_indices[j >> 1] >> 1;
    level_set(v2, level_get(v2) & (0xFFF));
    CallSomeSamusCode(0);
    SpawnHardcodedPlm(0x0c, 0x1d, 0xd113);
  }
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static void PlmPreInstr_IncrementRoomArgIfShotBySuperMissile(uint16 k) {  // 0x84D1E6
  int v1 = k >> 1;
  uint16 v2 = plm_timers[v1];
  if ((v2 & 0xF00) == 512 || (v2 & 0xF00) == 256)
    plm_room_arguments[v1]++;
  plm_timers[v1] = 0;
}

static uint16 PlmInstr_GotoIfRoomArgLess(uint16 p, uint16 k) {  // 0x84D2F9
  if (plm_room_arguments[k >> 1] >= B84W(p))
    return p + 4;
  else
    return (B84W(p + 2));
}

static uint16 PlmInstr_SpawnFourMotherBrainGlass(uint16 p, uint16 k) {  // 0x84D30B
  QueueSfx3_Max15(0x2E);
  SpawnMotherBrainGlassShatteringShard(B84W(p + 0));
  SpawnMotherBrainGlassShatteringShard(B84W(p + 2));
  SpawnMotherBrainGlassShatteringShard(B84W(p + 4));
  SpawnMotherBrainGlassShatteringShard(B84W(p + 6));
  return p + 8;
}

static void SpawnMotherBrainGlassShatteringShard(uint16 a) {  // 0x84D331
  SpawnEprojWithRoomGfx(0xCEFC, a);
}

static void PlmPreInstr_WakePlmIfSamusHasBombs(uint16 k) {  // 0x84D33B
  if ((collected_items & 0x1000) != 0) {
    int v1 = k >> 1;
    plm_instruction_timer[v1] = 1;
    plm_instr_list_ptrs[v1] += 2;
    plm_pre_instrs[v1] = FUNC16(nullsub_351);
  }
}

static uint16 PlmInstr_SpawnTorizoStatueBreaking(uint16 p, uint16 k) {  // 0x84D357
  SpawnEprojWithRoomGfx(0xA993, B84W(p));
  return p + 2;
}

static uint16 PlmInstr_QueueSong1MusicTrack(uint16 p, uint16 k) {  // 0x84D3C7
  QueueMusic_Delayed8(6);
  return p;
}

static uint16 PlmInstr_TransferWreckedShipChozoSpikesToSlopes(uint16 p, uint16 k) {  // 0x84D3D7
  WriteLevelDataBlockTypeAndBts(0x1608, 0x1012);
  WriteLevelDataBlockTypeAndBts(0x160A, 0x1013);
  return p;
}

static uint16 PlmInstr_TransferWreckedShipSlopesToChozoSpikes(uint16 p, uint16 k) {  // 0x84D3F4
  WriteLevelDataBlockTypeAndBts(0x1608, 0xA000);
  WriteLevelDataBlockTypeAndBts(0x160A, 0xA000);
  return p;
}

static void PlmPreInstr_WakeOnKeyPress(uint16 k) {  // 0x84D4BF
  if ((joypad1_newkeys & (uint16)(kButton_B | kButton_Y | kButton_Left | kButton_Right | kButton_A | kButton_X)) != 0) {
    int v2 = k >> 1;
    plm_instruction_timer[v2] = 1;
    ++plm_instr_list_ptrs[v2];
    ++plm_instr_list_ptrs[v2];
  }
}

static uint16 PlmInstr_EnableWaterPhysics(uint16 p, uint16 k) {  // 0x84D525
  fx_liquid_options &= ~4;
  return p;
}

static uint16 PlmInstr_SpawnN00bTubeCrackEproj(uint16 p, uint16 k) {  // 0x84D52C
  SpawnEprojWithRoomGfx(0xD904, 0);
  return p;
}

static uint16 PlmInstr_DiagonalEarthquake(uint16 p, uint16 k) {  // 0x84D536
  earthquake_type = 11;
  earthquake_timer = 64;
  return p;
}

static uint16 PlmInstr_Spawn10shardsAnd6n00bs(uint16 p, uint16 k) {  // 0x84D543
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 2);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 4);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 6);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 8);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0xA);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0xC);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0xE);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0x10);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeShards, 0x12);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 0);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 2);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 4);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 6);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 8);
  SpawnEprojWithRoomGfx(addr_kEproj_N00bTubeReleasedAirBubbles, 0xA);
  return p;
}

static uint16 PlmInstr_DisableSamusControls(uint16 p, uint16 k) {  // 0x84D5E6
  CallSomeSamusCode(0);
  return p;
}

static uint16 PlmInstr_EnableSamusControls(uint16 p, uint16 k) {  // 0x84D5EE
  CallSomeSamusCode(1);
  return p;
}

static uint8 PlmSetup_MotherBrainGlass(uint16 j) {  // 0x84D5F6
  int v1 = j >> 1;
  plm_room_arguments[v1] = 0;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0x8044);
  return 0;
}

static uint8 PlmSetup_DeletePlmIfAreaTorizoDead(uint16 j) {  // 0x84D606
  if (CheckBossBitForCurArea(4) & 1)
    plm_header_ptr[j >> 1] = 0;
  return 0;
}

static uint8 PlmSetup_MakeBllockChozoHandTrigger(uint16 j) {  // 0x84D616
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xB080);
  return 0;
}

static uint8 PlmSetup_D6F2_WreckedShipChozoHandTrigger(uint16 j) {  // 0x84D620
  if (CheckBossBitForCurArea(1) & 1
      && (samus_collision_direction & 0xF) == 3
      && (samus_pose == kPose_1D_FaceR_Morphball_Ground
          || samus_pose == kPose_79_FaceR_Springball_Ground
          || samus_pose == kPose_7A_FaceL_Springball_Ground)) {
    enemy_data[0].parameter_1 = 1;
    scrolls[7] = 2; scrolls[8] = 2;
    scrolls[13] = 1; scrolls[14] = 1;
    int v1 = plm_block_indices[j >> 1] >> 1;
    level_set(v1, level_get(v1) & (0xFFF));
    CallSomeSamusCode(0);
    SpawnHardcodedPlm(0x17, 0x1d, 0xd6f8);
  }
  plm_header_ptr[j >> 1] = 0;
  return 1;
}

static uint8 PlmSetup_D700_MakePlmAirBlock_Unused(uint16 j) {  // 0x84D67F
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x44);
  return 0;
}

static uint8 PlmSetup_D704_AlteranateLowerNorfairChozoHand_Unused(uint16 j) {  // 0x84D689
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x8044);
  return 0;
}

static uint8 PlmSetup_D708_LowerNorfairChozoBlockUnused(uint16 j) {  // 0x84D693
  int v1 = j >> 1;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0x8044);
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1] + 2, 0x50FF);
  WriteLevelDataBlockTypeAndBts(
    room_width_in_blocks
    + room_width_in_blocks
    + plm_block_indices[v1],
    0xD0FF);
  WriteLevelDataBlockTypeAndBts(
    room_width_in_blocks
    + room_width_in_blocks
    + plm_block_indices[v1]
    + 2,
    0xD0FF);
  return 0;
}

static uint8 PlmSetup_D70C_NoobTube(uint16 j) {  // 0x84D6CC
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x8044);
  return 0;
}

static void PlmPreInstr_WakePlmIfRoomArgumentDoorIsSet(uint16 k) {  // 0x84D753
  int v2 = k >> 1;
  int idx = PrepareBitAccess(plm_room_arguments[v2]);
  if ((bitmask & opened_door_bit_array[idx]) != 0) {
    int v3 = k >> 1;
    plm_pre_instrs[v3] = addr_locret_84D779;
    plm_instr_list_ptrs[v3] = plm_instruction_list_link_reg[v3];
    plm_instruction_timer[v3] = 1;
  }
}

static uint16 PlmInstr_ShootEyeDoorProjectileWithProjectileArg(uint16 p, uint16 k) {  // 0x84D77A
  SpawnEprojWithRoomGfx(addr_kEproj_EyeDoorProjectile, B84W(p));
  QueueSfx2_Max6(0x4C);
  return p + 2;
}

static uint16 PlmInstr_SpawnEyeDoorSweatEproj(uint16 p, uint16 k) {  // 0x84D790
  SpawnEprojWithRoomGfx(addr_kEproj_EyeDoorSweat, B84W(p));
  return p + 2;
}

static uint16 PlmInstr_SpawnTwoEyeDoorSmoke(uint16 p, uint16 k) {  // 0x84D79F
  SpawnEprojWithRoomGfx(0xE517, 0x30A);
  SpawnEprojWithRoomGfx(0xE517, 0x30A);
  return p;
}

static uint16 PlmInstr_SpawnEyeDoorSmokeProjectile(uint16 p, uint16 k) {  // 0x84D7B6
  SpawnEprojWithRoomGfx(addr_kEproj_EyeDoorSmoke, 0xB);
  return p;
}

static uint16 PlmInstr_MoveUpAndMakeBlueDoorFacingRight(uint16 p, uint16 k) {  // 0x84D7C3
  int v2 = k >> 1;
  uint16 blk = (plm_block_indices[v2] -= room_width_in_blocks * 2);
  WriteLevelDataBlockTypeAndBts(blk, 0xC041);
  sub_84D7EF(blk);
  return p;
}

static uint16 PlmInstr_MoveUpAndMakeBlueDoorFacingLeft(uint16 p, uint16 k) {  // 0x84D7DA
  int v2 = k >> 1;
  uint16 blk = (plm_block_indices[v2] -= room_width_in_blocks * 2);
  WriteLevelDataBlockTypeAndBts(blk, 0xC040);
  sub_84D7EF(blk);
  return p;
}

static void sub_84D7EF(uint16 k) {  // 0x84D7EF
  uint16 v1 = room_width_in_blocks * 2 + k;
  WriteLevelDataBlockTypeAndBts(v1, 0xD0FF);
  uint16 v2 = room_width_in_blocks * 2 + v1;
  WriteLevelDataBlockTypeAndBts(v2, 0xD0FE);
  WriteLevelDataBlockTypeAndBts(room_width_in_blocks * 2 + v2, 0xD0FD);
}

static uint8 PlmSetup_EyeDoorEye(uint16 j) {  // 0x84DA8C
  int idx = PrepareBitAccess(plm_room_arguments[j >> 1]);
  if ((bitmask & opened_door_bit_array[idx]) == 0) {
    int v1 = j >> 1;
    WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0xC044);
    WriteLevelDataBlockTypeAndBts(2 * room_width_in_blocks + plm_block_indices[v1], 0xD0FF);
  }
  return 0;
}

static uint8 PlmSetup_EyeDoor(uint16 j) {  // 0x84DAB9
  int idx = PrepareBitAccess(plm_room_arguments[j >> 1]);
  if ((bitmask & opened_door_bit_array[idx]) == 0)
    WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0xA000);
  return 0;
}

static void PlmPreInstr_SetMetroidsClearState_Ev0x10(uint16 k) {  // 0x84DADE
  if (num_enemies_killed_in_room >= num_enemy_deaths_left_to_clear)
    SetEventHappened(0x10);
}

static void PlmPreInstr_SetMetroidsClearState_Ev0x11(uint16 k) {  // 0x84DAEE
  if (num_enemies_killed_in_room >= num_enemy_deaths_left_to_clear)
    SetEventHappened(0x11);
}

static void PlmPreInstr_SetMetroidsClearState_Ev0x12(uint16 k) {  // 0x84DAFE
  if (num_enemies_killed_in_room >= num_enemy_deaths_left_to_clear)
    SetEventHappened(0x12);
}

static void PlmPreInstr_SetMetroidsClearState_Ev0x13(uint16 k) {  // 0x84DB0E
  if (num_enemies_killed_in_room >= num_enemy_deaths_left_to_clear)
    SetEventHappened(0x13);
}

static uint8 PlmSetup_SetMetroidRequiredClearState(uint16 j) {  // 0x84DB1E
  plm_pre_instrs[j >> 1] = B84W(0xDB28 + (plm_room_arguments[j >> 1] & ~1));
  return 0;
}

static void PlmPreInstr_GotoLinkIfShotWithSuperMissile(uint16 k) {  // 0x84DB64
  int16 v1;

  v1 = plm_timers[k >> 1] & 0xF00;
  if (v1 == 512) {
    plm_room_arguments[k >> 1] = 119;
  } else if (v1 != 256) {
    return;
  }
  int v2 = k >> 1;
  plm_timers[v2] = 0;
  plm_instr_list_ptrs[v2] = plm_instruction_list_link_reg[v2];
  plm_instruction_timer[v2] = 1;
}

static void SetPlmVarPtr(uint16 k, uint16 a) {   /* palabra de la RAM que nombra plm_variable (cañones de Draygon) */
  nt_ram_write16(plm_variable[k >> 1], a);
}

static uint16 PlmInstr_DamageDraygonTurret(uint16 p, uint16 k) {  // 0x84DB8E
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(room_width_in_blocks * 2 + v2, 0xA003);
  return p;
}

static uint16 PlmInstr_DamageDraygonTurretFacingDownRight(uint16 p, uint16 k) {  // 0x84DBB8
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(v2 + 2, 0xA003);
  uint16 v3 = plm_block_indices[plm_id >> 1];
  uint16 v4 = room_width_in_blocks * 2 + v3;
  WriteLevelDataBlockTypeAndBts(v4, 0xA003);
  WriteLevelDataBlockTypeAndBts(v4 + 2, 0);
  return p;
}

static uint16 PlmInstr_DamageDraygonTurretFacingUpRight(uint16 p, uint16 k) {  // 0x84DBF7
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(v2 + 2, 0);
  uint16 v4 = room_width_in_blocks * 2 + v2;
  WriteLevelDataBlockTypeAndBts(v4, 0xA003);
  WriteLevelDataBlockTypeAndBts(v4 + 2, 0xA003);
  return p;
}

static uint16 PlmInstr_DamageDraygonTurret2(uint16 p, uint16 k) {  // 0x84DC36
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(room_width_in_blocks * 2 + v2, 0xA003);
  return p;
}

static uint16 PlmInstr_DamageDraygonTurretFacingDownLeft(uint16 p, uint16 k) {  // 0x84DC60
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(v2 - 2, 0xA003);
  uint16 v4 = room_width_in_blocks * 2 + v2;
  WriteLevelDataBlockTypeAndBts(v4, 0xA003);
  WriteLevelDataBlockTypeAndBts(v4 - 2, 0);
  return p;
}

static uint16 PlmInstr_DamageDraygonTurretFacingUpLeft(uint16 p, uint16 k) {  // 0x84DC9F
  SetPlmVarPtr(k, 1);
  uint16 v2 = plm_block_indices[k >> 1];
  WriteLevelDataBlockTypeAndBts(v2, 0xA003);
  WriteLevelDataBlockTypeAndBts(v2 - 2, 0);
  uint16 v4 = v2 + room_width_in_blocks * 2;
  WriteLevelDataBlockTypeAndBts(v4, 0xA003);
  WriteLevelDataBlockTypeAndBts(v4 - 2, 0xA003);
  return p;
}

static uint8 PlmSetup_DraygonCannonFacingRight(uint16 j) {  // 0x84DE94
  int v1 = j >> 1;
  plm_variable[v1] = plm_room_arguments[v1];
  plm_room_arguments[v1] = 0;
  uint16 v2 = plm_block_indices[v1];
  WriteLevelDataBlockTypeAndBts(v2, 0xC044);
  WriteLevelDataBlockTypeAndBts(room_width_in_blocks * 2 + v2, 0xD0FF);
  return 0;
}

static uint8 PlmSetup_DraygonCannonFacingDiagonalRight(uint16 j) {  // 0x84DEB9
  int v1 = j >> 1;
  plm_variable[v1] = plm_room_arguments[v1];
  plm_room_arguments[v1] = 0;
  uint16 v2 = plm_block_indices[v1];
  WriteLevelDataBlockTypeAndBts(v2, 0xC044);
  WriteLevelDataBlockTypeAndBts(v2 + 2, 0x50FF);
  uint16 v3 = room_width_in_blocks * 2 + v2;
  WriteLevelDataBlockTypeAndBts(v3, 0xD0FF);
  WriteLevelDataBlockTypeAndBts(v3 + 2, 0xD0FF);
  return 0;
}

static uint8 PlmSetup_DraygonCannonFacingLeft(uint16 j) {  // 0x84DEF0
  int v1 = j >> 1;
  plm_variable[v1] = plm_room_arguments[v1];
  plm_room_arguments[v1] = 0;
  uint16 v2 = plm_block_indices[v1];
  WriteLevelDataBlockTypeAndBts(v2, 0xC044);
  WriteLevelDataBlockTypeAndBts(room_width_in_blocks * 2 + v2, 0xD0FF);
  return 0;
}

static uint8 PlmSetup_DraygonCannonFacingDiagonalLeft(uint16 j) {  // 0x84DF15
  int v1 = j >> 1;
  plm_variable[v1] = plm_room_arguments[v1];
  plm_room_arguments[v1] = 0;
  uint16 v2 = plm_block_indices[v1];
  WriteLevelDataBlockTypeAndBts(v2, 0xC044);
  WriteLevelDataBlockTypeAndBts(v2 - 2, 0x5001);
  uint16 v3 = room_width_in_blocks * 2 + v2;
  WriteLevelDataBlockTypeAndBts(v3, 0xD0FF);
  WriteLevelDataBlockTypeAndBts(v3 - 2, 0xD0FF);
  return 0;
}

static uint8 PlmSetup_DraygonCannon(uint16 j) {  // 0x84DF4C
  int v1 = j >> 1;
  plm_variable[v1] = plm_room_arguments[v1];
  plm_room_arguments[v1] = 3;
  return 0;
}

static void PlmPreInstr_GotoLinkIfTriggered(uint16 k) {  // 0x84DF89
  int v1 = k >> 1;
  if (LOBYTE(plm_timers[v1]) == 255) {
    plm_pre_instrs[v1] = FUNC16(PlmPreInstr_nullsub_301);
    plm_instr_list_ptrs[v1] = plm_instruction_list_link_reg[v1];
    plm_instruction_timer[v1] = 1;
  }
}

static void PlmPreInstr_WakeIfTriggered(uint16 k) {  // 0x84DFE6
  int v1 = k >> 1;
  uint16 v2 = plm_timers[v1];
  if (v2 != 768 && (uint8)v2 == 255) {
    plm_timers[v1] = 0;
    ++plm_instr_list_ptrs[v1];
    ++plm_instr_list_ptrs[v1];
    plm_instruction_timer[v1] = 1;
  }
}

static uint16 PlmInstr_DrawItemFrame_Common(uint16 p, uint16 k) {  // 0x84E07F
  int v2 = k >> 1;
  plm_instruction_timer[v2] = 4;
  plm_instr_list_ptrs[v2] = p;
  ProcessPlmDrawInstruction(k);
  uint16 v3 = plm_id;
  CalculatePlmBlockCoords(plm_id);
  DrawPlm(v3);
  return 0;
}

static uint16 PlmInstr_DrawItemFrame0(uint16 p, uint16 k) {  // 0x84E04F
  plm_instruction_draw_ptr[k >> 1] = B84W(0xE05F + (plm_variables[k >> 1] & ~1));
  return PlmInstr_DrawItemFrame_Common(p, k);
}

static uint16 PlmInstr_DrawItemFrame1(uint16 p, uint16 k) {  // 0x84E067
  plm_instruction_draw_ptr[k >> 1] = B84W(0xE077 + (plm_variables[k >> 1] & ~1));
  return PlmInstr_DrawItemFrame_Common(p, k);
}

static uint16 PlmInstr_ClearChargeBeamCounter(uint16 p, uint16 k) {  // 0x84E29D
  flare_counter = 0;
  return p;
}

static uint16 PlmInstr_E63B(uint16 p, uint16 k) {  // 0x84E63B
  fx_y_vel = -32;
  return p;
}

static uint8 sub_84EE4D(uint16 j) {  // 0x84EE4D
  return sub_84EE5F(j, 8);
}

static uint8 sub_84EE52(uint16 j) {  // 0x84EE52
  return sub_84EE5F(j, 0xA);
}

static uint8 sub_84EE57(uint16 j) {  // 0x84EE57
  return sub_84EE5F(j, 0xC);
}

static uint8 sub_84EE5C(uint16 j) {  // 0x84EE5C
  return sub_84EE5F(j, 0xE);
}

static uint8 sub_84EE5F(uint16 j, uint16 a) {  // 0x84EE5F
  plm_variables[j >> 1] = a;
  return sub_84EE64(j);
}

static uint8 sub_84EE64(uint16 j) {  // 0x84EE64
  WriteLevelDataBlockTypeAndBts(plm_block_indices[j >> 1], 0x45);
  ++global_number_of_items_loaded_ctr;
  return 0;
}

static uint8 sub_84EE77(uint16 j) {  // 0x84EE77
  return sub_84EE89(j, 8);
}

static uint8 sub_84EE7C(uint16 j) {  // 0x84EE7C
  return sub_84EE89(j, 0xA);
}

static uint8 sub_84EE81(uint16 j) {  // 0x84EE81
  return sub_84EE89(j, 0xC);
}

static uint8 sub_84EE86(uint16 j) {  // 0x84EE86
  return sub_84EE89(j, 0xE);
}

static uint8 sub_84EE89(uint16 j, uint16 a) {  // 0x84EE89
  plm_variables[j >> 1] = a;
  return sub_84EE8E(j);
}

static uint8 sub_84EE8E(uint16 j) {  // 0x84EE8E
  int v1 = j >> 1;
  WriteLevelDataBlockTypeAndBts(plm_block_indices[v1], 0xC045);
  plm_variable[v1] = level_get(plm_block_indices[v1] >> 1);
  ++global_number_of_items_loaded_ctr;
  return 0;
}

static uint8 sub_84EEAB(uint16 v0) {  // 0x84EEAB
  int i;

  if (time_is_frozen_flag) {
    plm_header_ptr[v0 >> 1] = 0;
  } else {
    int v1 = v0 >> 1;
    uint16 v2 = plm_block_indices[v1];
    plm_block_indices[v1] = 0;
    for (i = 78; i >= 0; i -= 2) {
      if (v2 == plm_block_indices[i >> 1])
        break;
    }
    plm_timers[i >> 1] = 255;
  }
  return 0;
}



static uint8 CallPlmHeaderFunc(uint32 ea, uint16 j) {
  switch (ea) {
  case fnnullsub_67:
  case fnnullsub_290:
  case fnnullsub_68:
  case fnnullsub_84BAFA:
  case fnnullsub_71:
  case fnnullsub_72:
  case fnnullsub_69: return 0;
  case fnPlmSetup_CrumbleBotwoonWall: return PlmSetup_CrumbleBotwoonWall(j);
  case fnPlmSetup_SetrupWreckedShipEntrance: return PlmSetup_SetrupWreckedShipEntrance(j);
  case fnPlmSetup_BTS_Brinstar_0x80_Floorplant: return PlmSetup_BTS_Brinstar_0x80_Floorplant(j);
  case fnPlmSetup_BTS_Brinstar_0x81_Ceilingplant: return PlmSetup_BTS_Brinstar_0x81_Ceilingplant(j);
  case fnPlmSetup_B6D3_MapStation: return PlmSetup_B6D3_MapStation(j);
  case fnPlmSetup_Bts47_MapStationRightAccess: return PlmSetup_Bts47_MapStationRightAccess(j);
  case fnPlmSetup_Bts4_MapStationLeftAccess: return PlmSetup_Bts4_MapStationLeftAccess(j);
  case fnPlmSetup_PlmB6DF_EnergyStation: return PlmSetup_PlmB6DF_EnergyStation(j);
  case fnPlmSetup_PlmB6EB_EnergyStation: return PlmSetup_PlmB6EB_EnergyStation(j);
  case fnPlmSetup_B6E3_EnergyStationRightAccess: return PlmSetup_B6E3_EnergyStationRightAccess(j);
  case fnPlmSetup_B6E7_EnergyStationLeftAccess: return PlmSetup_B6E7_EnergyStationLeftAccess(j);
  case fnPlmSetup_B6EF_MissileStationRightAccess: return PlmSetup_B6EF_MissileStationRightAccess(j);
  case fnPlmSetup_B6F3_MissileStationLeftAccess: return PlmSetup_B6F3_MissileStationLeftAccess(j);
  case fnPlmSetup_B638_Rightwards_Extension: return PlmSetup_B638_Rightwards_Extension(j);
  case fnPlmSetup_B63F_Leftwards_Extension: return PlmSetup_B63F_Leftwards_Extension(j);
  case fnPlmSetup_B643_Downwards_Extension: return PlmSetup_B643_Downwards_Extension(j);
  case fnPlmSetup_B647_Upwards_Extension: return PlmSetup_B647_Upwards_Extension(j);
  case fnPlmSetup_B703_ScrollPLM: return PlmSetup_B703_ScrollPLM(j);
  case fnPlmSetup_B707_SolidScrollPLM: return PlmSetup_B707_SolidScrollPLM(j);
  case fnPlmSetup_B6FF_ScrollBlockTouch: return PlmSetup_B6FF_ScrollBlockTouch(j);
  case fnPlmSetup_DeactivatePlm: return PlmSetup_DeactivatePlm(j);
  case fnPlmSetup_ReturnCarryClear: return PlmSetup_ReturnCarryClear(j);
  case fnPlmSetup_ReturnCarrySet: return PlmSetup_ReturnCarrySet(j);
  case fnPlmSetup_D094_EnemyBreakableBlock: return PlmSetup_D094_EnemyBreakableBlock(j);
  case fnUNUSED_sub_84B3E3: return UNUSED_sub_84B3E3(j);
  case fnPlmSetup_B70F_IcePhysics: return PlmSetup_B70F_IcePhysics(j);
  case fnPlmSetup_QuicksandSurface: return PlmSetup_QuicksandSurface(j);
  case fnPlmSetup_B71F_SubmergingQuicksand: return PlmSetup_B71F_SubmergingQuicksand(j);
  case fnPlmSetup_B723_SandfallsSlow: return PlmSetup_B723_SandfallsSlow(j);
  case fnPlmSetup_B727_SandFallsFast: return PlmSetup_B727_SandFallsFast(j);
  case fnPlmSetup_QuicksandSurfaceB: return PlmSetup_QuicksandSurfaceB(j);
  case fnPlmSetup_B737_SubmergingQuicksand: return PlmSetup_B737_SubmergingQuicksand(j);
  case fnPlmSetup_B73B_B73F_SandFalls: return PlmSetup_B73B_B73F_SandFalls(j);
  case fnPlmSetup_ClearShitroidInvisibleWall: return PlmSetup_ClearShitroidInvisibleWall(j);
  case fnPlmSetup_B767_ClearShitroidInvisibleWall: return PlmSetup_B767_ClearShitroidInvisibleWall(j);
  case fnPlmSetup_B76B_SaveStationTrigger: return PlmSetup_B76B_SaveStationTrigger(j);
  case fnPlmSetup_B76F_SaveStation: return PlmSetup_B76F_SaveStation(j);
  case fnPlmSetup_MotherBrainRoomEscapeDoor: return PlmSetup_MotherBrainRoomEscapeDoor(j);
  case fnPlmSetup_B7EB_EnableSoundsIn32Frames: return PlmSetup_B7EB_EnableSoundsIn32Frames(j);
  case fnPlmSetup_SpeedBoosterEscape: return PlmSetup_SpeedBoosterEscape(j);
  case fnPlmSetup_ShaktoolsRoom: return PlmSetup_ShaktoolsRoom(j);
  case fnPlmSetup_B974: return PlmSetup_B974(j);
  case fnPlmSetup_B9C1_CrittersEscapeBlock: return PlmSetup_B9C1_CrittersEscapeBlock(j);
  case fnPlmSetup_B9ED_CrittersEscapeBlock: return PlmSetup_B9ED_CrittersEscapeBlock(j);
  case fnsub_84B9F1: return sub_84B9F1(j);
  case fnPlmSetup_BB30_CrateriaMainstreetEscape: return PlmSetup_BB30_CrateriaMainstreetEscape(j);
  case fnPlmSetup_C806_LeftGreenGateTrigger: return PlmSetup_C806_LeftGreenGateTrigger(j);
  case fnPlmSetup_C80A_RightGreenGateTrigger: return PlmSetup_C80A_RightGreenGateTrigger(j);
  case fnPlmSetup_C80E_LeftRedGateTrigger: return PlmSetup_C80E_LeftRedGateTrigger(j);
  case fnPlmSetup_C812_RightRedGateTrigger: return PlmSetup_C812_RightRedGateTrigger(j);
  case fnPlmSetup_C81E_LeftYellowGateTrigger: return PlmSetup_C81E_LeftYellowGateTrigger(j);
  case fnPlmSetup_C822_RightYellowGateTrigger: return PlmSetup_C822_RightYellowGateTrigger(j);
  case fnPlmSetup_C816_LeftBlueGateTrigger: return PlmSetup_C816_LeftBlueGateTrigger(j);
  case fnPlmSetup_C81A_RightBlueGateTrigger: return PlmSetup_C81A_RightBlueGateTrigger(j);
  case fnPlmSetup_C82A_DownwardsClosedGate: return PlmSetup_C82A_DownwardsClosedGate(j);
  case fnPlmSetup_C832_UpwardsClosedGate: return PlmSetup_C832_UpwardsClosedGate(j);
  case fnPlmSetup_C826_DownwardsOpenGate: return PlmSetup_C826_DownwardsOpenGate(j);
  case fnPlmSetup_C82E_UpwardsOpenGate: return PlmSetup_C82E_UpwardsOpenGate(j);
  case fnPlmSetup_C836_DownwardsGateShootblock: return PlmSetup_C836_DownwardsGateShootblock(j);
  case fnPlmSetup_C73A_UpwardsGateShootblock: return PlmSetup_C73A_UpwardsGateShootblock(j);
  case fnPlmSetup_C794_GreyDoor: return PlmSetup_C794_GreyDoor(j);
  case fnPlmSetup_Door_Colored: return PlmSetup_Door_Colored(j);
  case fnPlmSetup_Door_Blue: return PlmSetup_Door_Blue(j);
  case fnPlmSetup_Door_Strange: return PlmSetup_Door_Strange(j);
  case fnPlmSetup_D028_D02C_Unused: return PlmSetup_D028_D02C_Unused(j);
  case fnPlmSetup_RespawningSpeedBoostBlock: return PlmSetup_RespawningSpeedBoostBlock(j);
  case fnPlmSetup_RespawningCrumbleBlock: return PlmSetup_RespawningCrumbleBlock(j);
  case fnPlmSetup_RespawningShotBlock: return PlmSetup_RespawningShotBlock(j);
  case fnPlmSetup_RespawningBombBlock: return PlmSetup_RespawningBombBlock(j);
  case fnPlmSetup_RespawningBombBlock2: return PlmSetup_RespawningBombBlock2(j);
  case fnPlmSetup_RespawningPowerBombBlock: return PlmSetup_RespawningPowerBombBlock(j);
  case fnPlmSetup_D08C_SuperMissileBlockRespawning: return PlmSetup_D08C_SuperMissileBlockRespawning(j);
  case fnPlmSetup_D08C_CrumbleBlock: return PlmSetup_D08C_CrumbleBlock(j);
  case fnPlmSetup_D0DC_BreakableGrappleBlock: return PlmSetup_D0DC_BreakableGrappleBlock(j);
  case fnPlmSetup_D0D8_SetVFlag: return PlmSetup_D0D8_SetVFlag(j);
  case fnPlmSetup_D0D8_ClearVflag: return PlmSetup_D0D8_ClearVflag(j);
  case fnPlmSetup_D0E8_GiveSamusDamage: return PlmSetup_D0E8_GiveSamusDamage(j);
  case fnPlmSetup_D113_LowerNorfairChozoRoomPlug: return PlmSetup_D113_LowerNorfairChozoRoomPlug(j);
  case fnPlmSetup_D127: return PlmSetup_D127(j);
  case fnPlmSetup_D138: return PlmSetup_D138(j);
  case fnPlmSetup_D6DA_LowerNorfairChozoHandTrigger: return PlmSetup_D6DA_LowerNorfairChozoHandTrigger(j);
  case fnPlmSetup_MotherBrainGlass: return PlmSetup_MotherBrainGlass(j);
  case fnPlmSetup_DeletePlmIfAreaTorizoDead: return PlmSetup_DeletePlmIfAreaTorizoDead(j);
  case fnPlmSetup_MakeBllockChozoHandTrigger: return PlmSetup_MakeBllockChozoHandTrigger(j);
  case fnPlmSetup_D6F2_WreckedShipChozoHandTrigger: return PlmSetup_D6F2_WreckedShipChozoHandTrigger(j);
  case fnPlmSetup_D700_MakePlmAirBlock_Unused: return PlmSetup_D700_MakePlmAirBlock_Unused(j);
  case fnPlmSetup_D704_AlteranateLowerNorfairChozoHand_Unused: return PlmSetup_D704_AlteranateLowerNorfairChozoHand_Unused(j);
  case fnPlmSetup_D708_LowerNorfairChozoBlockUnused: return PlmSetup_D708_LowerNorfairChozoBlockUnused(j);
  case fnPlmSetup_D70C_NoobTube: return PlmSetup_D70C_NoobTube(j);
  case fnPlmSetup_EyeDoorEye: return PlmSetup_EyeDoorEye(j);
  case fnPlmSetup_EyeDoor: return PlmSetup_EyeDoor(j);
  case fnPlmSetup_SetMetroidRequiredClearState: return PlmSetup_SetMetroidRequiredClearState(j);
  case fnPlmSetup_DraygonCannonFacingRight: return PlmSetup_DraygonCannonFacingRight(j);
  case fnPlmSetup_DraygonCannonFacingDiagonalRight: return PlmSetup_DraygonCannonFacingDiagonalRight(j);
  case fnPlmSetup_DraygonCannonFacingLeft: return PlmSetup_DraygonCannonFacingLeft(j);
  case fnPlmSetup_DraygonCannonFacingDiagonalLeft: return PlmSetup_DraygonCannonFacingDiagonalLeft(j);
  case fnPlmSetup_DraygonCannon: return PlmSetup_DraygonCannon(j);
  case fnsub_84EE4D: return sub_84EE4D(j);
  case fnsub_84EE52: return sub_84EE52(j);
  case fnsub_84EE57: return sub_84EE57(j);
  case fnsub_84EE5C: return sub_84EE5C(j);
  case fnsub_84EE64: return sub_84EE64(j);
  case fnsub_84EE77: return sub_84EE77(j);
  case fnsub_84EE7C: return sub_84EE7C(j);
  case fnsub_84EE81: return sub_84EE81(j);
  case fnsub_84EE86: return sub_84EE86(j);
  case fnsub_84EE8E: return sub_84EE8E(j);
  case fnsub_84EEAB: return sub_84EEAB(j);
  default: nt_unimpl("CallPlmHeaderFunc (inicio de PLM sin portar)"); return 0;
  }
}

static void CallPlmPreInstr(uint32 ea, uint16 k) {
  switch (ea) {
  case fnPlmPreInstr_nullsub_60: return;
  case fnPlmPreInstr_nullsub_301: return;
  case fnPlmPreInstr_Empty: return;
  case fnPlmPreInstr_Empty2: return;
  case fnPlmPreInstr_Empty3: return;
  case 0x848AA6: return;                          /* PlmPreInstr_Empty4 */
  case fnPlmPreInstr_Empty5: return;
  case fnnullsub_351: return;
  case fnnullsub_84BAFA: return;
  case fnPlmSetup_BTS_Brinstar_0x80_Floorplant: PlmSetup_BTS_Brinstar_0x80_Floorplant(k); return;
  case fnPlmSetup_BTS_Brinstar_0x81_Ceilingplant: PlmSetup_BTS_Brinstar_0x81_Ceilingplant(k); return;
  case fnPlmPreInstr_B7EB_DecTimerEnableSoundsDeletePlm: PlmPreInstr_B7EB_DecTimerEnableSoundsDeletePlm(k); return;
  case fnPlmPreInstr_WakeAndLavaIfBoosterCollected: PlmPreInstr_WakeAndLavaIfBoosterCollected(k); return;
  case fnPlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft: PlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft(k); return;
  case fnPlmPreInstr_AdvanceLavaSamusMovesLeft: PlmPreInstr_AdvanceLavaSamusMovesLeft(k); return;
  case fnPlmPreInstr_ShaktoolsRoom: PlmPreInstr_ShaktoolsRoom(k); return;
  case fnPlmPreInstr_OldTourianEscapeShaftEscape: PlmPreInstr_OldTourianEscapeShaftEscape(k); return;
  case fnPlmPreInstr_EscapeRoomBeforeOldTourianEscapeShaft: PlmPreInstr_EscapeRoomBeforeOldTourianEscapeShaft(k); return;
  case fnPlmPreInstr_WakePlmIfTriggered: PlmPreInstr_WakePlmIfTriggered(k); return;
  case fnPlmPreInstr_WakePlmIfTriggeredOrSamusBelowPlm: PlmPreInstr_WakePlmIfTriggeredOrSamusBelowPlm(k); return;
  case fnPlmPreInstr_WakePlmIfTriggeredOrSamusAbovePlm: PlmPreInstr_WakePlmIfTriggeredOrSamusAbovePlm(k); return;
  case fnPlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed: PlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed(k); return;
  case fnPlmPreInstr_IncrementRoomArgIfShotBySuperMissile: PlmPreInstr_IncrementRoomArgIfShotBySuperMissile(k); return;
  case fnPlmPreInstr_WakePlmIfSamusHasBombs: PlmPreInstr_WakePlmIfSamusHasBombs(k); return;
  case fnPlmPreInstr_WakePlmIfRoomArgumentDoorIsSet: PlmPreInstr_WakePlmIfRoomArgumentDoorIsSet(k); return;
  case fnPlmPreInstr_GotoLinkIfShotWithSuperMissile: PlmPreInstr_GotoLinkIfShotWithSuperMissile(k); return;
  case fnPlmPreInstr_GotoLinkIfTriggered: PlmPreInstr_GotoLinkIfTriggered(k); return;
  case fnPlmPreInstr_WakeIfTriggered: PlmPreInstr_WakeIfTriggered(k); return;
  case fnPlmPreInstr_GoToLinkInstrIfShot: PlmPreInstr_GoToLinkInstrIfShot(k); return;
  case fnPlmPreInstr_GoToLinkInstrIfShotWithPowerBomb: PlmPreInstr_GoToLinkInstrIfShotWithPowerBomb(k); return;
  case fnPlmPreInstr_GoToLinkInstrIfShotWithAnyMissile: PlmPreInstr_GoToLinkInstrIfShotWithAnyMissile(k); return;
  case fnPlmPreInstr_GoToLinkInstrIfShotWithSuperMissile: PlmPreInstr_GoToLinkInstrIfShotWithSuperMissile(k); return;
  case fnPlmPreInstr_GoToLinkInstruction: PlmPreInstr_GoToLinkInstruction(k); return;
  case fnPlmPreInstr_PlayDudSound: PlmPreInstr_PlayDudSound(k); return;
  case fnPlmPreInstr_GotoLinkIfBoss1Dead: PlmPreInstr_GotoLinkIfBoss1Dead(k); return;
  case fnPlmPreInstr_GotoLinkIfMiniBossDead: PlmPreInstr_GotoLinkIfMiniBossDead(k); return;
  case fnPlmPreInstr_GotoLinkIfTorizoDead: PlmPreInstr_GotoLinkIfTorizoDead(k); return;
  case fnPlmPreInstr_GotoLinkIfEnemyDeathQuotaOk: PlmPreInstr_GotoLinkIfEnemyDeathQuotaOk(k); return;
  case fnPlmPreInstr_GotoLinkIfTourianStatueFinishedProcessing: PlmPreInstr_GotoLinkIfTourianStatueFinishedProcessing(k); return;
  case fnPlmPreInstr_GotoLinkIfCrittersEscaped: PlmPreInstr_GotoLinkIfCrittersEscaped(k); return;
  case fnPlmPreInstr_PositionSamusAndInvincible: PlmPreInstr_PositionSamusAndInvincible(k); return;
  case fnPlmPreInstr_WakeOnKeyPress: PlmPreInstr_WakeOnKeyPress(k); return;
  case fnnullsub_359: return;
  case fnnullsub_73:
  case fnnullsub_74:
  case fnnullsub_75:
  case fnnullsub_76:
  case fnnullsub_77:
  case fnnullsub_78:
  case fnnullsub_79:
  case fnnullsub_80:
  case fnnullsub_81:
  case fnlocret_848AE0: return;
  case fnPlmPreInstr_SetMetroidsClearState_Ev0x10: PlmPreInstr_SetMetroidsClearState_Ev0x10(k); return;
  case fnPlmPreInstr_SetMetroidsClearState_Ev0x11: PlmPreInstr_SetMetroidsClearState_Ev0x11(k); return;
  case fnPlmPreInstr_SetMetroidsClearState_Ev0x12: PlmPreInstr_SetMetroidsClearState_Ev0x12(k); return;
  case fnPlmPreInstr_SetMetroidsClearState_Ev0x13: PlmPreInstr_SetMetroidsClearState_Ev0x13(k); return;
  default: nt_unimpl("CallPlmPreInstr (pre-instrucción de PLM sin portar)"); return;
  }
}

static uint16 CallPlmInstr(uint32 ea, uint16 j, uint16 k) {
  switch (ea) {
  case fnPlmInstr_Sleep: return PlmInstr_Sleep(j, k);
  case fnPlmInstr_Delete: return PlmInstr_Delete(j, k);
  case fnPlmInstr_PreInstr: return PlmInstr_PreInstr(j, k);
  case fnPlmInstr_ClearPreInstr: return PlmInstr_ClearPreInstr(j, k);
  case fnPlmInstr_CallFunction: return PlmInstr_CallFunction(j, k);
  case fnPlmInstr_Goto: return PlmInstr_Goto(j, k);
  case fnPlmInstr_DecrementAndBranchNonzero: return PlmInstr_DecrementAndBranchNonzero(j, k);
  case fnPlmInstr_SetTimer: return PlmInstr_SetTimer(j, k);
  case fnPlmInstr_LoadItemPlmGfx: return PlmInstr_LoadItemPlmGfx(j, k);
  case fnPlmInstr_CopyFromRamToVram: return PlmInstr_CopyFromRamToVram(j, k);
  case fnPlmInstr_GotoIfBossBitSet: return PlmInstr_GotoIfBossBitSet(j, k);
  case fnPlmInstr_GotoIfEventSet: return PlmInstr_GotoIfEventSet(j, k);
  case fnPlmInstr_SetEvent: return PlmInstr_SetEvent(j, k);
  case fnPlmInstr_GotoIfChozoSet: return PlmInstr_GotoIfChozoSet(j, k);
  case fnPlmInstr_SetRoomChozoBit: return PlmInstr_SetRoomChozoBit(j, k);
  case fnPlmInstr_GotoIfItemBitSet: return PlmInstr_GotoIfItemBitSet(j, k);
  case fnPlmInstr_SetItemBit: return PlmInstr_SetItemBit(j, k);
  case fnPlmInstr_PickupBeamAndShowMessage: return PlmInstr_PickupBeamAndShowMessage(j, k);
  case fnPlmInstr_PickupEquipmentAndShowMessage: return PlmInstr_PickupEquipmentAndShowMessage(j, k);
  case fnPlmInstr_PickupEquipmentAddGrappleShowMessage: return PlmInstr_PickupEquipmentAddGrappleShowMessage(j, k);
  case fnPlmInstr_PickupEquipmentAddXrayShowMessage: return PlmInstr_PickupEquipmentAddXrayShowMessage(j, k);
  case fnPlmInstr_CollectHealthEnergyTank: return PlmInstr_CollectHealthEnergyTank(j, k);
  case fnPlmInstr_CollectHealthReserveTank: return PlmInstr_CollectHealthReserveTank(j, k);
  case fnPlmInstr_CollectAmmoMissileTank: return PlmInstr_CollectAmmoMissileTank(j, k);
  case fnPlmInstr_CollectAmmoSuperMissileTank: return PlmInstr_CollectAmmoSuperMissileTank(j, k);
  case fnPlmInstr_CollectAmmoPowerBombTank: return PlmInstr_CollectAmmoPowerBombTank(j, k);
  case fnPlmInstr_SetLinkReg: return PlmInstr_SetLinkReg(j, k);
  case fnPlmInstr_Call: return PlmInstr_Call(j, k);
  case fnPlmInstr_Return: return PlmInstr_Return(j, k);
  case fnPlmInstr_GotoIfDoorBitSet: return PlmInstr_GotoIfDoorBitSet(j, k);
  case fnPlmInstr_IncrementDoorHitCounterAndJGE: return PlmInstr_IncrementDoorHitCounterAndJGE(j, k);
  case fnPlmInstr_IncrementArgumentAndJGE: return PlmInstr_IncrementArgumentAndJGE(j, k);
  case fnPlmInstr_SetBTS: return PlmInstr_SetBTS(j, k);
  case fnPlmInstr_DrawPlmBlock: return PlmInstr_DrawPlmBlock(j, k);
  case fnPlmInstr_DrawPlmBlock_: return PlmInstr_DrawPlmBlock(j, k);
  case fnPlmInstr_ProcessAirScrollUpdate: return PlmInstr_ProcessAirScrollUpdate(j, k);
  case fnPlmInstr_ProcessSolidScrollUpdate: return PlmInstr_ProcessSolidScrollUpdate(j, k);
  case fnPlmInstr_QueueMusic: return PlmInstr_QueueMusic(j, k);
  case fnPlmInstr_ClearMusicQueueAndQueueTrack: return PlmInstr_ClearMusicQueueAndQueueTrack(j, k);
  case fnPlmInstr_QueueSfx1_Max6: return PlmInstr_QueueSfx1_Max6(j, k);
  case fnPlmInstr_QueueSfx2_Max6: return PlmInstr_QueueSfx2_Max6(j, k);
  case fnPlmInstr_QueueSfx3_Max6: return PlmInstr_QueueSfx3_Max6(j, k);
  case fnPlmInstr_QueueSfx1_Max15: return PlmInstr_QueueSfx1_Max15(j, k);
  case fnPlmInstr_QueueSfx2_Max15: return PlmInstr_QueueSfx2_Max15(j, k);
  case fnPlmInstr_QueueSfx3_Max15: return PlmInstr_QueueSfx3_Max15(j, k);
  case fnPlmInstr_QueueSfx1_Max3: return PlmInstr_QueueSfx1_Max3(j, k);
  case fnPlmInstr_QueueSfx2_Max3: return PlmInstr_QueueSfx2_Max3(j, k);
  case fnPlmInstr_QueueSfx_Max3: return PlmInstr_QueueSfx_Max3(j, k);
  case fnPlmInstr_QueueSfx1_Max9: return PlmInstr_QueueSfx1_Max9(j, k);
  case fnPlmInstr_QueueSfx2_Max9: return PlmInstr_QueueSfx2_Max9(j, k);
  case fnPlmInstr_QueueSfx3_Max9: return PlmInstr_QueueSfx3_Max9(j, k);
  case fnPlmInstr_QueueSfx1_Max1: return PlmInstr_QueueSfx1_Max1(j, k);
  case fnPlmInstr_QueueSfx2_Max1: return PlmInstr_QueueSfx2_Max1(j, k);
  case fnPlmInstr_QueueSfx3_Max1: return PlmInstr_QueueSfx3_Max1(j, k);
  case fnPlmInstr_ActivateMapStation: return PlmInstr_ActivateMapStation(j, k);
  case fnPlmInstr_ActivateEnergyStation: return PlmInstr_ActivateEnergyStation(j, k);
  case fnPlmInstr_ActivateMissileStation: return PlmInstr_ActivateMissileStation(j, k);
  case fnPlmInstr_ActivateSaveStationAndGotoIfNo: return PlmInstr_ActivateSaveStationAndGotoIfNo(j, k);
  case fnPlmInstr_GotoIfSamusNear: return PlmInstr_GotoIfSamusNear(j, k);
  case fnPlmInstr_MovePlmDownOneBlock: return PlmInstr_MovePlmDownOneBlock(j, k);
  case fnPlmInstr_Scroll_0_1_Blue: return PlmInstr_Scroll_0_1_Blue(j, k);
  case fnPlmInstr_MovePlmDownOneBlock_0: return PlmInstr_MovePlmDownOneBlock_0(j, k);
  case fnPlmInstr_DealDamage_2: return PlmInstr_DealDamage_2(j, k);
  case fnPlmInstr_GiveInvincibility: return PlmInstr_GiveInvincibility(j, k);
  case fnPlmInstr_Draw0x38FramesOfRightTreadmill: return PlmInstr_Draw0x38FramesOfRightTreadmill(j, k);
  case fnPlmInstr_Draw0x38FramesOfLeftTreadmill: return PlmInstr_Draw0x38FramesOfLeftTreadmill(j, k);
  case fnPlmInstr_GotoIfSamusHealthFull: return PlmInstr_GotoIfSamusHealthFull(j, k);
  case fnPlmInstr_GotoIfMissilesFull: return PlmInstr_GotoIfMissilesFull(j, k);
  case fnPlmInstr_PlaceSamusOnSaveStation: return PlmInstr_PlaceSamusOnSaveStation(j, k);
  case fnPlmInstr_DisplayGameSavedMessageBox: return PlmInstr_DisplayGameSavedMessageBox(j, k);
  case fnPlmInstr_EnableMovementAndSetSaveStationUsed: return PlmInstr_EnableMovementAndSetSaveStationUsed(j, k);
  case fnPlmInstr_SetCrittersEscapedEvent: return PlmInstr_SetCrittersEscapedEvent(j, k);
  case fnPlmInstr_JumpIfSamusHasNoBombs: return PlmInstr_JumpIfSamusHasNoBombs(j, k);
  case fnPlmInstr_MovePlmRight4Blocks: return PlmInstr_MovePlmRight4Blocks(j, k);
  case fnPlmInstr_ClearTrigger: return PlmInstr_ClearTrigger(j, k);
  case fnPlmInstr_SpawnEproj: return PlmInstr_SpawnEproj(j, k);
  case fnPlmInstr_WakeEprojAtPlmPos: return PlmInstr_WakeEprojAtPlmPos(j, k);
  case fnPlmInstr_SetGreyDoorPreInstr: return PlmInstr_SetGreyDoorPreInstr(j, k);
  case fnPlmInstr_FxBaseYPos_0x2D2: return PlmInstr_FxBaseYPos_0x2D2(j, k);
  case fnPlmInstr_GotoIfRoomArgLess: return PlmInstr_GotoIfRoomArgLess(j, k);
  case fnPlmInstr_SpawnFourMotherBrainGlass: return PlmInstr_SpawnFourMotherBrainGlass(j, k);
  case fnPlmInstr_SpawnTorizoStatueBreaking: return PlmInstr_SpawnTorizoStatueBreaking(j, k);
  case fnPlmInstr_QueueSong1MusicTrack: return PlmInstr_QueueSong1MusicTrack(j, k);
  case fnPlmInstr_TransferWreckedShipChozoSpikesToSlopes: return PlmInstr_TransferWreckedShipChozoSpikesToSlopes(j, k);
  case fnPlmInstr_TransferWreckedShipSlopesToChozoSpikes: return PlmInstr_TransferWreckedShipSlopesToChozoSpikes(j, k);
  case fnPlmInstr_EnableWaterPhysics: return PlmInstr_EnableWaterPhysics(j, k);
  case fnPlmInstr_SpawnN00bTubeCrackEproj: return PlmInstr_SpawnN00bTubeCrackEproj(j, k);
  case fnPlmInstr_DiagonalEarthquake: return PlmInstr_DiagonalEarthquake(j, k);
  case fnPlmInstr_Spawn10shardsAnd6n00bs: return PlmInstr_Spawn10shardsAnd6n00bs(j, k);
  case fnPlmInstr_ShootEyeDoorProjectileWithProjectileArg: return PlmInstr_ShootEyeDoorProjectileWithProjectileArg(j, k);
  case fnPlmInstr_SpawnEyeDoorSweatEproj: return PlmInstr_SpawnEyeDoorSweatEproj(j, k);
  case fnPlmInstr_SpawnTwoEyeDoorSmoke: return PlmInstr_SpawnTwoEyeDoorSmoke(j, k);
  case fnPlmInstr_SpawnEyeDoorSmokeProjectile: return PlmInstr_SpawnEyeDoorSmokeProjectile(j, k);
  case fnPlmInstr_MoveUpAndMakeBlueDoorFacingRight: return PlmInstr_MoveUpAndMakeBlueDoorFacingRight(j, k);
  case fnPlmInstr_MoveUpAndMakeBlueDoorFacingLeft: return PlmInstr_MoveUpAndMakeBlueDoorFacingLeft(j, k);
  case fnPlmInstr_DamageDraygonTurret: return PlmInstr_DamageDraygonTurret(j, k);
  case fnPlmInstr_DamageDraygonTurretFacingDownRight: return PlmInstr_DamageDraygonTurretFacingDownRight(j, k);
  case fnPlmInstr_DamageDraygonTurretFacingUpRight: return PlmInstr_DamageDraygonTurretFacingUpRight(j, k);
  case fnPlmInstr_DamageDraygonTurret2: return PlmInstr_DamageDraygonTurret2(j, k);
  case fnPlmInstr_DamageDraygonTurretFacingDownLeft: return PlmInstr_DamageDraygonTurretFacingDownLeft(j, k);
  case fnPlmInstr_DamageDraygonTurretFacingUpLeft: return PlmInstr_DamageDraygonTurretFacingUpLeft(j, k);
  case fnPlmInstr_DrawItemFrame0: return PlmInstr_DrawItemFrame0(j, k);
  case fnPlmInstr_DrawItemFrame1: return PlmInstr_DrawItemFrame1(j, k);
  case fnPlmInstr_DrawItemFrame_Common: return PlmInstr_DrawItemFrame_Common(j, k);
  case fnPlmInstr_ClearChargeBeamCounter: return PlmInstr_ClearChargeBeamCounter(j, k);
  case fnPlmInstr_ABD6: return PlmInstr_ABD6(j, k);
  case fnPlmInstr_E63B: return PlmInstr_E63B(j, k);
  case fnPlmInstr_SetBtsTo1: return PlmInstr_SetBtsTo1(j, k);
  case fnPlmInstr_DisableSamusControls: return PlmInstr_DisableSamusControls(j, k);
  case fnPlmInstr_EnableSamusControls: return PlmInstr_EnableSamusControls(j, k);
  default: nt_unimpl("CallPlmInstr (instrucción de PLM sin portar)"); plm_header_ptr[k >> 1] = 0; return 0;
  }
}

