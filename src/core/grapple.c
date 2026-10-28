/* MegaMetroid — motor nativo: el rayo gancho (docs/56).
 * Escrito a mano desde snesrev/sm sm_9b.c ($9B:B861-CBFA: GrappleBeamHandler y sus funciones), con lo del gancho
 * de $90 (HudSelectionHandler_Grappling, Samus_MoveHandler_ReleaseFromGrapple, Samus_GrappleWallJumpCheck,
 * Samus_PostGrappleCollisionDetect, el manejador de dibujo sub_90EB86) y $A0 (GrappleBeam_CollDetect_Enemy). Las
 * colisiones contra bloques ($94) están en coll.c. Las tablas del banco $9B salen de la ROM (kBank9B_Grapple, mkdata). */
#include "samus.h"
#include "enemies.h"
#include "nmath.h"
#define abs16 abs16n
uint32 Multiply16x16(uint16 a, uint16 j);
void GrappleBeamFunc_FireGoToCancel(void);

uint8 BlockCollGrappleBeam(void);              /* coll.c ($94) */
uint8 CheckIfGrappleIsConnectedToBlock(void);
void BlockFunc_AC11(void);
uint8 BlockFunc_AC31(void);
uint8 HandleMovementAndCollForSamusGrapple(void);
void PostGrappleCollisionDetect_X(void);
void PostGrappleCollisionDetect_Y(void);
int32 WallJumpBlockCollDetect(int32 amt);
void HandleGrappleBeamFlare(void);             /* proj.c */
void LoadProjectilePalette(uint16 a);          /* samus_codes.c */
void Samus_Pose_CancelGrapple(void);           /* samus_pose.c */
void DrawGrappleOam(int16 x, int16 y, uint16 chr);   /* capa de la Genesis: un segmento o la punta (8x8) */
void GrappleTiles(uint16 point_src, uint16 beam_src);  /* capa de la Genesis: los tiles de $9A que sube el SNES */

/* ------------------------------------------------------------------ tablas de $9B:C0DB-C9CE */
static inline uint16 b9bgw(uint16 a) {
  const uint8 *p = kBank9B_Grapple + (uint16)(a - 0xC0DB);
  uint16 lo = p[0];
  __asm__("" : "+d"(lo));
  return lo | p[1] << 8;
}
static inline uint8 b9bgb(uint16 a) { return kBank9B_Grapple[(uint16)(a - 0xC0DB)]; }
#define kGrappleBeam_Ext_Xvel(i) b9bgw(0xC0DB + 2 * (i))
#define kGrappleBeam_Ext_Yvel(i) b9bgw(0xC0EF + 2 * (i))
#define kGrappleBeam_Init_EndAngle(i) b9bgw(0xC104 + 2 * (i))
#define kGrappleBeam_OriginX_NoRun(i) b9bgw(0xC122 + 2 * (i))
#define kGrappleBeam_OriginY_NoRun(i) b9bgw(0xC136 + 2 * (i))
#define kGrappleBeam_0x0d1a_offs_NoRun(i) b9bgw(0xC14A + 2 * (i))
#define kGrappleBeam_0x0d1c_offs_NoRun(i) b9bgw(0xC15E + 2 * (i))
#define kGrappleBeam_OriginX_Run(i) b9bgw(0xC172 + 2 * (i))
#define kGrappleBeam_OriginY_Run(i) b9bgw(0xC186 + 2 * (i))
#define kGrappleBeam_0x0d1a_offs_Run(i) b9bgw(0xC19A + 2 * (i))
#define kGrappleBeam_0x0d1c_offs_Run(i) b9bgw(0xC1AE + 2 * (i))
#define kGrappleBeam_SwingingData(i) b9bgb(0xC1C2 + (i))
#define kGrappleBeam_SwingingData2(i) b9bgb(0xC2C2 + (i))
#define kGrappleBeam_SwingingData3(i) b9bgb(0xC302 + (i))
#define g_off_9BC342 b9bgw(0xC342)
#define g_off_9BC344 b9bgw(0xC344)
#define g_off_9BC346(i) b9bgw(0xC346 + 2 * (i))
#define g_off_9BC3C6(i) b9bgw(0xC3C6 + 2 * (i))
#define g_off_9BC3EE(i) b9bgw(0xC3EE + 2 * (i))
#define g_off_9BC416(i) b9bgw(0xC416 + 2 * (i))
#define kSpecialAngles(i, f) b9bgw(0xC43E + 10 * (i) + (f))   /* grapple_beam_special_angles: 5 palabras */
#define g_byte_9BC9BA(i) b9bgb(0xC9BA + (i))
#define g_byte_9BC9C4(i) b9bgb(0xC9C4 + (i))

static const uint16 g_word_9BC118 = 24;
static const uint16 g_word_9BC11A = 0xc;
static const uint16 g_word_9BC11C = 5;
static const uint16 g_word_9BC11E = 0x480;
static const uint16 g_word_9BC120 = 0x300;
static const uint8 kIsGrappleBannedForMovementType[28] = {
  0, 0, 0, 1, 1, 0, 0, 1, 1, 1,
  1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
  1, 0, 0, 1, 1, 1, 0, 1,
};

static void GrappleClampPrevPos(void) {   /* GrappleBeamFunc_BE98 ($9B:BE98): la posición anterior a 12 px como mucho */
  int16 v0 = samus_x_pos - samus_prev_x_pos;
  if (v0 < 0) {
    if (sign16(v0 + 12))
      samus_prev_x_pos = samus_x_pos + 12;
  } else if (!sign16(v0 - 13)) {
    samus_prev_x_pos = samus_x_pos - 12;
  }
  int16 v1 = samus_y_pos - samus_prev_y_pos;
  if (v1 < 0) {
    if (sign16(v1 + 12))
      samus_prev_y_pos = samus_y_pos + 12;
  } else if (!sign16(v1 - 13)) {
    samus_prev_y_pos = samus_y_pos - 12;
  }
}

/* listas de animación de los segmentos ($94:B18B-B19E): cuatro cuadros de 5 y Goto al principio; las cuatro
   entradas (B18B, B18F, B193, B197) son el mismo ciclo corrido */
static uint16 B94W(uint16 a) {
  static const uint16 k[10] = { 5, 0x3A21, 5, 0x3A22, 5, 0x3A23, 5, 0x3A24, 0xB0F4, 0xB18B };
  uint16 i = (uint16)(a - 0xB18B) >> 1;
  if (i >= 10) { nt_unimpl("HandleGrappleBeamGfx (lista)"); return 0; }
  return k[i];
}

/* ------------------------------------------------------------------ $90 */
uint8 Samus_GrappleWallJumpCheck(int32 amt) {  // 0x909CAC
  enemy_index_to_shake = -1;
  if (samus_pose_x_dir != 4) {
    if (samus_pose_x_dir != 8)
      return 0;
    samus_collision_direction = 1;
  } else {
    samus_collision_direction = 0;
  }
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(amt);
  if (!cres.collision) {
    WallJumpBlockCollDetect(cres.amt);
    return samus_collision_flag && (button_config_jump_a & joypad1_newkeys);
  }
  if (!(button_config_jump_a & joypad1_newkeys))
    return 0;
  enemy_index_to_shake = collision_detection_index;
  return 1;
}

void Samus_PostGrappleCollisionDetect(void) {  // 0x90EF22
  PostGrappleCollisionDetect_X();
  PostGrappleCollisionDetect_Y();
  if (!distance_to_eject_samus_down || !distance_to_eject_samus_up) {
    if (distance_to_eject_samus_up) {
      samus_y_pos -= distance_to_eject_samus_up;
      if (!sign16(samus_y_radius - 16)) {
        PostGrappleCollisionDetect_Y();
        samus_y_pos -= distance_to_eject_samus_up;
      }
    }
  }
}

/* ------------------------------------------------------------------ $A0:9E9A: ¿la punta toca un enemigo? */
static PairU16 GrappleBeam_CollDetect_Enemy(void) {  // 0xA09E9A
  CallSomeSamusCode(0xD);
  collision_detection_index = 0;
  EnemyData *E;
  for (int i = 0; ; i++) {
    cur_enemy_index = interactive_enemy_indexes[i];
    if (cur_enemy_index == 0xFFFF)
      return (PairU16){ 0, 0 };
    E = gEnemyData(cur_enemy_index);
    if (!E->invincibility_timer) {
      uint16 v5 = abs16(E->x_pos - grapple_beam_end_x_pos);
      if (v5 < E->x_width || (uint16)(v5 - E->x_width) < 8) {
        uint16 v8 = abs16(E->y_pos - grapple_beam_end_y_pos);
        if (v8 < E->y_height || (uint16)(v8 - E->y_height) < 8)
          break;
      }
    }
  }
  E->ai_handler_bits = 1;
  uint16 enemy_ptr = E->enemy_ptr;
  uint16 grapple_ai = enemy_def(E->enemy_ptr)->ai_bits[1];   /* EnemyDef.grapple_ai */
  uint16 v0;
  switch (grapple_ai) {                          /* Enemy_GrappleReact_*_A0 ($A0:8000-8027) */
  case FUNC16(Enemy_GrappleReact_NoInteract_A0): v0 = 0; break;
  case FUNC16(Enemy_GrappleReact_SamusLatchesOn_A0): v0 = 1; break;
  case FUNC16(Enemy_GrappleReact_KillEnemy_A0): v0 = 2; break;
  case FUNC16(Enemy_GrappleReact_CancelBeam_A0): v0 = 3; break;
  case FUNC16(Enemy_GrappleReact_SamusLatchesNoInvinc_A0): v0 = 4; break;
  case FUNC16(Enemy_GrappleReact_SamusLatchesParalyze_A0): v0 = 5; break;
  case FUNC16(Enemy_GrappleReact_HurtSamus_A0): v0 = 6; break;
  default: v0 = 0; break;
  }
  if (v0 == 1 || v0 == 4 || v0 == 5) {
    grapple_beam_end_x_pos = E->x_pos;
    grapple_beam_end_y_pos = E->y_pos;
  }
  return (PairU16){ v0, enemy_ptr };
}

/* reacciones de los enemigos al gancho (Enemy_GrappleReact_*: $XX:8000-801E, iguales en cada banco; el traductor
   no hace las del gancho) */
void EnemyDeathAnimation(uint16 k, uint16 a);   /* port_gen.c ($A0:A3AF) */
uint8 Enemy_GrappleReact(uint16 f) {           /* devuelve 0 si f no es una de ellas */
  EnemyData *E = gEnemyData(cur_enemy_index);
  switch (f) {
  case 0x8000:                                   // SwitchEnemyAiToMainAi ($A0:9F6D)
    E->ai_handler_bits = 0;
    E->invincibility_timer = 0;
    E->frozen_timer = 0;
    E->shake_timer = 0;
    return 1;
  case 0x8005:                                   // SamusLatchesOnWithGrapple ($A0:9F7D)
    grapple_beam_end_x_pos = E->x_pos;
    grapple_beam_end_y_pos = E->y_pos;
    if (E->frozen_timer) {
      E->ai_handler_bits = 4;
    } else {
      uint8 t = enemy_def(E->enemy_ptr)->hurt_ai_time;
      E->flash_timer = t ? t : 4;
      E->ai_handler_bits = 0;
    }
    return 1;
  case 0x800A:                                   // EnemyGrappleDeath ($A0:9FC4)
    gEnemySpawnData(cur_enemy_index)->cause_of_death = 4;
    EnemyDeathAnimation(cur_enemy_index, 0);
    gEnemyData(cur_enemy_index)->ai_handler_bits = 0;
    return 1;
  case 0x800F: case 0x801E:                      // Enemy_SwitchToFrozenAi ($A0:9FDF) / SamusHurtFromGrapple ($A0:A070)
    E->ai_handler_bits = 4;
    return 1;
  case 0x8014:                                   // SamusLatchesOnWithGrappleNoInvinc ($A0:9FE9)
    if (E->frozen_timer) {
      E->ai_handler_bits = 4;
    } else {
      CallEnemyAi(E->bank, enemy_def(E->enemy_ptr)->ai_bits[0]);
      E->ai_handler_bits = 0;
    }
    grapple_beam_end_x_pos = E->x_pos;
    grapple_beam_end_y_pos = E->y_pos;
    return 1;
  case 0x8019: {                                 // SamusLatchesOnWithGrappleParalyze ($A0:A03E)
    uint8 t = enemy_def(E->enemy_ptr)->hurt_ai_time;
    E->flash_timer = t ? t : 4;
    E->ai_handler_bits = 0;
    E->extra_properties |= 1;
    return 1;
  }
  default:
    return 0;
  }
}

/* ------------------------------------------------------------------ $9B */
static void CancelGrappleBeamIfIncompatiblePose(void) {  // 0x9BB861
  if (!kIsGrappleBannedForMovementType[samus_movement_type]) {
    if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive) || !sign16(grapple_beam_function + 0x3882))
      return;
    int16 v0 = kPoseParams[samus_pose].direction_shots_fired;
    if (!(v0 & 0xF0)) {
      if (v0 == grapple_beam_direction)
        return;
      if (grapple_varCF6) {
        QueueSfx1_Max6(7);
        grapple_beam_function = FUNC16(GrappleBeamFunc_FireGoToCancel);
        return;
      }
    }
  }
  if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive))
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
}

static int ProcessEnemyGrappleBeamColl(uint16 a, uint16 r18) {  // 0x9BB907
  switch (a) {
  case 0: case 2: return -1;                     /* nada: sigue contra los bloques */
  case 3: return 1;
  case 1: case 4: case 5: return 0;
  case 6:
    r18 = enemy_def(r18)->damage;                /* *((uint16 *)RomPtr_A0(r18) + 3) */
    if (equipped_items & 0x20)
      r18 >>= 2;
    else if (equipped_items & 1)
      r18 >>= 1;
    Samus_DealDamage(r18);
    samus_invincibility_timer = 96;
    samus_knockback_timer = 5;
    knockback_x_dir = samus_pose_x_dir == 4;
    return 1;
  default:
    nt_unimpl("ProcessEnemyGrappleBeamColl (Unreachable)");
    return 0;
  }
}

static void HandleConnectingGrapple_Common(uint16 transgfx) {  // _Swinging $9B:BA61 / _StuckInPlace $9B:BA9B
  samus_special_transgfx_index = transgfx;
  uint16 a = CalculateAngleFromXY(samus_x_pos - grapple_beam_end_x_pos, samus_y_pos - grapple_beam_end_y_pos);
  uint16 v0 = (uint16)(a << 8 | a >> 8);         /* swap16 */
  grapple_beam_end_angle16 = v0;
  grapple_beam_end_angles_mirror = v0;
  grapple_beam_length_delta = 0;
  if (!sign16(grapple_beam_length - 64))
    grapple_beam_length -= 24;
  BlockFunc_AC11();
}

static void CallGrappleNextFunc(uint16 f) {   /* GrappleNext_* ($9B:B9D9-BA29): la pose y cómo queda colgada */
  switch (f) {
  case FUNC16(GrappleNext_SwingClockwise): samus_new_pose_interrupted = kPose_B2_FaceR_Grapple_Air; HandleConnectingGrapple_Common(9); return;
  case FUNC16(GrappleNext_SwingAntiClockwise): samus_new_pose_interrupted = kPose_B3_FaceL_Grapple_Air; HandleConnectingGrapple_Common(9); return;
  case FUNC16(GrappleNext_StandAimRight): samus_new_pose_interrupted = kPose_A8_FaceR_Grappling; break;
  case FUNC16(GrappleNext_StandAimDownRight): samus_new_pose_interrupted = kPose_AA_FaceR_Grappling_AimDR; break;
  case FUNC16(GrappleNext_StandAimDownLeft): samus_new_pose_interrupted = kPose_AB_FaceL_Grappling_AimDL; break;
  case FUNC16(GrappleNext_StandAimLeft): samus_new_pose_interrupted = kPose_A9_FaceL_Grappling; break;
  case FUNC16(GrappleNext_CrouchAimRight): samus_new_pose_interrupted = kPose_B4_FaceR_Grappling_Crouch; break;
  case FUNC16(GrappleNext_CrouchAimDownRight): samus_new_pose_interrupted = kPose_B6_FaceR_Grappling_Crouch_AimDR; break;
  case FUNC16(GrappleNext_CrouchAimDownLeft): samus_new_pose_interrupted = kPose_B7_FaceL_Grappling_Crouch_AimDL; break;
  case FUNC16(GrappleNext_CrouchAimLeft): samus_new_pose_interrupted = kPose_B5_FaceL_Grappling_Crouch; break;
  default: nt_unimpl("CallGrappleNextFunc (Unreachable)"); return;
  }
  HandleConnectingGrapple_Common(10);
}

static void HandleConnectingGrapple(void) {  // 0x9BB97C
  if (samus_movement_type == kMovementType_1A_GrabbedByDraygon) {
    grapple_beam_function = FUNC16(GrappleBeamFunc_ConnectedLockedInPlace);
    grapple_beam_length_delta = 0;
    return;
  }
  int v1 = 2 * grapple_beam_direction;
  if (samus_y_speed || samus_y_subspeed) {
    grapple_beam_function = g_off_9BC3EE(v1);
    CallGrappleNextFunc(g_off_9BC3EE(v1 + 1));
  } else if (samus_movement_type == 5) {
    grapple_beam_function = g_off_9BC416(v1);
    CallGrappleNextFunc(g_off_9BC416(v1 + 1));
  } else {
    grapple_beam_function = g_off_9BC3C6(v1);
    CallGrappleNextFunc(g_off_9BC3C6(v1 + 1));
  }
}

static uint8 HandleSpecialGrappleBeamAngles(void) {  // 0x9BBAD5
  uint16 v0 = 7;
  while (grapple_beam_end_angle16 != kSpecialAngles(v0, 0)) {
    if (--v0 & 0x8000)
      return 0;
  }
  samus_new_pose_interrupted = kSpecialAngles(v0, 2);
  samus_x_pos = grapple_beam_end_x_pos + kSpecialAngles(v0, 4);
  samus_y_pos = grapple_beam_end_y_pos + kSpecialAngles(v0, 6);
  grapple_beam_function = kSpecialAngles(v0, 8);
  samus_special_transgfx_index = 0;
  slow_grabble_scrolling_flag = 0;
  GrappleClampPrevPos();
  return 1;
}

static bool GrappleInWater(void) { return grapple_beam_flags && (grapple_beam_flags & 1); }

static void GrappleBeamFunc_BB64(void) {  // 0x9BBB64: arriba/abajo cambian el largo, izquierda/derecha empujan
  if (joypad1_newkeys & kButton_Up) {
    if (grapple_beam_length)
      grapple_beam_length_delta = -2;
  } else if (joypad1_newkeys & kButton_Down) {
    if (sign16(grapple_beam_length - 64))
      grapple_beam_length_delta = 2;
    else
      grapple_beam_length = 64;
  }
  uint16 hi8 = (uint16)(HI8(grapple_beam_end_angle16) << 8);
  if (sign16(hi8 - 0x4000) || !sign16(hi8 + 0x4000)) {
    grapple_beam_unkD2A = 0;
    return;
  }
  if (joypad1_lastkeys & 0x200) {
    if (hi8 == 0x8000 && !grapple_beam_unkD26)
      grapple_beam_unkD26 = 256;
    grapple_beam_unkD2A = GrappleInWater() ? g_word_9BC11A >> 1 : g_word_9BC11A;
  } else if (joypad1_lastkeys & 0x100) {
    if (hi8 == 0x8000 && !grapple_beam_unkD26)
      grapple_beam_unkD26 = -256;
    grapple_beam_unkD2A = GrappleInWater() ? -(g_word_9BC11A >> 1) : -g_word_9BC11A;
  } else {
    grapple_beam_unkD2A = 0;
  }
}

static void GrappleBeamFunc_BC1F(void) {  // 0x9BBC1F: gravedad del columpio según el ángulo
  if ((grapple_beam_end_angle16 & 0xC000) == 0xC000) {
    grapple_beam_unkD2C = -(g_word_9BC11C >> 2);
    grapple_beam_unkD28 = GrappleInWater() ? -(g_word_9BC118 >> 3) : -(g_word_9BC118 >> 2);
  } else if (sign16(grapple_beam_end_angle16)) {
    if ((uint16)(HI8(grapple_beam_end_angle16) << 8) == 0x8000) {
      grapple_beam_unkD28 = 0;
      grapple_beam_unkD2C = 0;
      uint16 v0 = grapple_beam_unkD26;
      if (v0 & 0x8000)
        v0 = ~(grapple_beam_unkD26 - 1);
      if (sign16(HI8(v0) - 1))
        grapple_beam_unkD26 = 0;
    } else {
      grapple_beam_unkD2C = -g_word_9BC11C;
      grapple_beam_unkD28 = GrappleInWater() ? -(g_word_9BC118 >> 1) : -g_word_9BC118;
    }
  } else if (grapple_beam_end_angle16 & 0x4000) {
    grapple_beam_unkD2C = g_word_9BC11C;
    grapple_beam_unkD28 = GrappleInWater() ? g_word_9BC118 >> 1 : g_word_9BC118;
  } else {
    grapple_beam_unkD2C = g_word_9BC11C >> 2;
    grapple_beam_unkD28 = GrappleInWater() ? g_word_9BC118 >> 3 : g_word_9BC118 >> 2;
  }
}

static void GrappleBeamFunc_BCFF(void) {  // 0x9BBCFF
  grapple_beam_unkD26 += grapple_beam_unkD2A + grapple_beam_unkD28;
  if ((grapple_beam_unkD26 ^ grapple_beam_end_angle16) & 0x8000)
    grapple_beam_unkD26 += grapple_beam_unkD2C;
  if (!(grapple_beam_unkD26 & 0x8000)) {
    if (grapple_beam_unkD26 >= g_word_9BC11E)
      grapple_beam_unkD26 = g_word_9BC11E;
  } else if ((uint16)-grapple_beam_unkD26 >= g_word_9BC11E) {
    grapple_beam_unkD26 = -g_word_9BC11E;
  }
}

static void GrappleBeamFunc_BD44(void) {  // 0x9BBD44: saltar en el columpio da impulso
  if (grapple_beam_unkD30 && (button_config_jump_a & joypad1_newkeys)) {
    if (grapple_beam_unkD26) {
      uint16 v = GrappleInWater() ? g_word_9BC120 >> 1 : g_word_9BC120;
      grapple_beam_unkD2E = (grapple_beam_unkD26 & 0x8000) ? -v : v;
    } else {
      grapple_beam_unkD2E = 0;
    }
  }
}

static void GrappleBeamFunc_BD95(void) {  // 0x9BBD95: Samus colgada en la punta, su cuadro según el ángulo
  uint16 v0 = abs16(grapple_beam_unkD26), v1;
  if (!sign16(v0 - 64)) {
    slow_grabble_scrolling_flag = 1;
    samus_anim_frame_timer = 15;
    v1 = kGrappleBeam_SwingingData(HI8(grapple_beam_end_angles_mirror));
    samus_anim_frame = v1;
  } else {
    slow_grabble_scrolling_flag = 0;
    if ((uint16)(HI8(grapple_beam_end_angle16) << 8) != 0x8000) {
      samus_anim_frame_timer = 15;
      v1 = kGrappleBeam_SwingingData(HI8(grapple_beam_end_angles_mirror));
      samus_anim_frame = v1;
    } else {
      if (sign16(samus_anim_frame - 64)) {
        samus_anim_frame_timer = 8;
        samus_anim_frame = 64;
      }
      v1 = kGrappleBeam_SwingingData(HI8(grapple_beam_end_angles_mirror));
    }
  }
  uint16 v2 = 2 * v1;
  if ((abs16(grapple_beam_unkD2E) & 0xFF00) == 256) {
    uint16 v3 = samus_anim_frame;
    if (!sign16(samus_anim_frame - 64))
      v3 = 16;
    samus_anim_frame = v3 + 32;
  }
  if (samus_pose_x_dir == 4) {
    samus_x_pos = x_pos_of_start_of_grapple_beam + (int8)kGrappleBeam_SwingingData2(v2);
    samus_y_pos = y_pos_of_start_of_grapple_beam + (int8)kGrappleBeam_SwingingData2(v2 + 1);
  } else {
    samus_x_pos = x_pos_of_start_of_grapple_beam + (int8)kGrappleBeam_SwingingData3(v2);
    samus_y_pos = y_pos_of_start_of_grapple_beam + (int8)kGrappleBeam_SwingingData3(v2 + 1);
  }
  x_pos_of_start_of_grapple_beam_prevframe = x_pos_of_start_of_grapple_beam;
  y_pos_of_start_of_grapple_beam_prevframe = y_pos_of_start_of_grapple_beam;
  GrappleClampPrevPos();
}

void GrappleBeamFunc_BD95_(void) { GrappleBeamFunc_BD95(); }

void GrappleBeamFunc_BEEB(void) {  // 0x9BBEEB
  int v0 = grapple_beam_direction;
  samus_x_pos = x_pos_of_start_of_grapple_beam - kGrappleBeam_OriginX_NoRun(v0);
  x_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1a_offs_NoRun(v0) + samus_x_pos;
  samus_y_pos = y_pos_of_start_of_grapple_beam - kGrappleBeam_OriginY_NoRun(v0);
  y_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1c_offs_NoRun(v0) + samus_y_pos;
}

static void GrappleBeamFunc_BF1B(void) {  // 0x9BBF1B: el origen del gancho sigue al cañón
  uint16 r22 = kPoseParams[samus_pose].y_offset_to_gfx;
  int v1 = grapple_beam_direction;
  if (samus_pose == kPose_49_FaceL_Moonwalk || samus_pose == kPose_4A_FaceR_Moonwalk || samus_movement_type != 1) {
    x_pos_of_start_of_grapple_beam = kGrappleBeam_OriginX_NoRun(v1) + samus_x_pos;
    x_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1a_offs_NoRun(v1) + samus_x_pos;
    y_pos_of_start_of_grapple_beam = kGrappleBeam_OriginY_NoRun(v1) + samus_y_pos - r22;
    y_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1c_offs_NoRun(v1) + samus_y_pos - r22;
  } else {
    x_pos_of_start_of_grapple_beam = kGrappleBeam_OriginX_Run(v1) + samus_x_pos;
    x_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1a_offs_Run(v1) + samus_x_pos;
    y_pos_of_start_of_grapple_beam = kGrappleBeam_OriginY_Run(v1) + samus_y_pos - r22;
    y_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1c_offs_Run(v1) + samus_y_pos - r22;
  }
}

static void UpdateGrappleBeamTiles(void) {  // 0x9BBFBD: el punto que late y el tile del segmento según el ángulo
  if (--grapple_point_anim_timer & 0x8000) {
    grapple_point_anim_timer = 5;
    grapple_point_anim_ptr += 512;
    if ((int16)(grapple_point_anim_ptr - g_off_9BC344) >= 0)
      grapple_point_anim_ptr = g_off_9BC342;
  }
  /* el SNES encola $9A:grapple_point_anim_ptr (32 bytes) -> VRAM $6200 y $9A:g_off_9BC346[ángulo] (128) -> $6210 */
  GrappleTiles(grapple_point_anim_ptr, g_off_9BC346(((HI8(grapple_beam_end_angle16) >> 1) & 0xFE) >> 1));
}

static void GrappleBeamFunc_Inactive(void) {  // 0x9BC4F0
  if ((button_config_shoot_x & joypad1_newkeys) || (button_config_shoot_x & joypad1_newinput_samusfilter)) {
    GrappleBeamFunc_FireGoToCancel();
  } else if (flare_counter) {
    flare_counter = 0;
    flare_animation_frame = 0;
    flare_slow_sparks_anim_frame = 0;
    flare_fast_sparks_anim_frame = 0;
    flare_animation_timer = 0;
    flare_slow_sparks_anim_timer = 0;
    flare_fast_sparks_anim_timer = 0;
    Samus_LoadSuitPalette();
  }
}

static uint16 CheckBannedDraygonGrappleDirs(uint16 a) {  // 0x9BC6B2
  if (a == kPose_BE_FaceL_Draygon_Move) {
    if (joypad1_lastkeys & kButton_Left) {
      if (joypad1_lastkeys & kButton_Down)
        return 6;
      if (joypad1_lastkeys & kButton_Up)
        return 8;
    }
    return 7;
  }
  if (!(joypad1_lastkeys & kButton_Right))
    return 2;
  if (joypad1_lastkeys & kButton_Down)
    return 3;
  if (!(joypad1_lastkeys & kButton_Up))
    return 2;
  return 1;
}

static void GrappleFunc_AF87(void) {  // 0x94AF87: las animaciones de los 16 segmentos desde el principio
  for (int i = 30; i >= 0; i -= 8) {
    int v1 = i >> 1;
    grapple_segment_anim_instr_ptrs[v1] = addr_word_94B197;
    grapple_segment_anim_instr_ptrs[v1 - 1] = addr_word_94B193;   /* (grapple_segment_anim_instr_timers[v1 + 15] etc.: */
    grapple_segment_anim_instr_ptrs[v1 - 2] = addr_word_94B18F;   /*  el decomp los nombra desde los contadores) */
    grapple_segment_anim_instr_ptrs[v1 - 3] = addr_word_94B18B;
    grapple_segment_anim_instr_timers[v1] = 1;
    grapple_segment_anim_instr_timers[v1 - 1] = 1;
    grapple_segment_anim_instr_timers[v1 - 2] = 1;
    grapple_segment_anim_instr_timers[v1 - 3] = 1;
  }
}

void GrappleBeamFunc_FireGoToCancel(void) {  // 0x9BC51E: disparar el gancho
  uint16 r22;
  int v0;
  if (samus_pose == kPose_F0_FaceR_Draygon_Move || samus_pose == kPose_BE_FaceL_Draygon_Move) {
    v0 = CheckBannedDraygonGrappleDirs(samus_pose);
    r22 = 6;
  } else {
    r22 = kPoseParams[samus_pose].y_offset_to_gfx;
    v0 = kPoseParams[samus_pose].direction_shots_fired;
    if (v0 & 0xF0) {
      grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
      return;
    }
  }
  grapple_beam_direction = v0;
  grapple_beam_extension_x_velocity = kGrappleBeam_Ext_Xvel(v0);
  grapple_beam_extension_y_velocity = kGrappleBeam_Ext_Yvel(v0);
  grapple_beam_end_angle16 = kGrappleBeam_Init_EndAngle(v0);
  grapple_beam_end_angles_mirror = grapple_beam_end_angle16;
  grapple_varCF6 = 10;
  if (samus_pose == kPose_49_FaceL_Moonwalk || samus_pose == kPose_4A_FaceR_Moonwalk || samus_movement_type != 1) {
    grapple_beam_origin_x_offset = kGrappleBeam_OriginX_NoRun(v0);
    grapple_beam_origin_y_offset = kGrappleBeam_OriginY_NoRun(v0) - r22;
    grapple_beam_end_x_pos = kGrappleBeam_OriginX_NoRun(v0) + samus_x_pos;
    x_pos_of_start_of_grapple_beam = grapple_beam_end_x_pos;
    x_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1a_offs_NoRun(v0) + samus_x_pos;
    grapple_beam_end_y_pos = kGrappleBeam_OriginY_NoRun(v0) + samus_y_pos - r22;
    y_pos_of_start_of_grapple_beam = grapple_beam_end_y_pos;
    y_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1c_offs_NoRun(v0) + samus_y_pos - r22;
  } else {
    grapple_beam_origin_x_offset = kGrappleBeam_OriginX_Run(v0);
    grapple_beam_origin_y_offset = kGrappleBeam_OriginY_Run(v0) - r22;
    grapple_beam_end_x_pos = kGrappleBeam_OriginX_Run(v0) + samus_x_pos;
    x_pos_of_start_of_grapple_beam = grapple_beam_end_x_pos;
    x_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1a_offs_Run(v0) + samus_x_pos;
    grapple_beam_end_y_pos = kGrappleBeam_OriginY_Run(v0) + samus_y_pos - r22;
    y_pos_of_start_of_grapple_beam = grapple_beam_end_y_pos;
    y_pos_of_start_of_grapple_beam_prevframe = kGrappleBeam_0x0d1c_offs_Run(v0) + samus_y_pos - r22;
  }
  grapple_beam_end_x_subpos = 0;
  grapple_beam_end_y_subpos = 0;
  grapple_beam_end_x_suboffset = 0;
  grapple_beam_end_x_offset = 0;
  grapple_beam_end_y_suboffset = 0;
  grapple_beam_end_y_offset = 0;
  grapple_beam_flags = 0;
  grapple_beam_length_delta = 12;
  grapple_beam_length = 0;
  grapple_beam_unkD26 = 0;
  grapple_beam_unkD28 = 0;
  grapple_beam_unkD2A = 0;
  grapple_beam_unkD2C = 0;
  grapple_beam_unkD2E = 0;
  grapple_beam_unkD30 = 0;
  grapple_beam_unkD1E = 0;
  grapple_beam_unkD20 = 0;
  grapple_beam_unkD3A = 2;
  grapple_beam_unkD3C = 0;
  grapple_point_anim_timer = 5;
  grapple_point_anim_ptr = g_off_9BC342;
  grapple_beam_grapple_start_x = 0;
  grapple_beam_unkD38 = 0;
  grapple_beam_unkD36 = 0;
  slow_grabble_scrolling_flag = 0;
  GrappleFunc_AF87();
  samus_draw_handler = FUNC16(sub_90EB86);
  grapple_walljump_timer = 0;
  LoadProjectilePalette(2);
  palette_buffer[223] = 32657;
  nt_pal_dirty(223, 223);
  grapple_beam_function = FUNC16(GrappleBeamFunc_Firing);
  QueueSfx1_Max1(5);
  flare_counter = 1;
  play_resume_charging_beam_sfx = 0;
  if (samus_movement_handler == FUNC16(Samus_MoveHandler_ReleaseFromGrapple))
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
}

static void GrappleBeamFunc_Firing(void) {  // 0x9BC703: el gancho sale hasta 128 px
  if (!(button_config_shoot_x & joypad1_lastkeys)) {
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
    return;
  }
  grapple_beam_length += grapple_beam_length_delta;
  if (!sign16(grapple_beam_length - 128)) {
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
    return;
  }
  PairU16 pair = GrappleBeam_CollDetect_Enemy();
  int v1 = ProcessEnemyGrappleBeamColl(pair.k, pair.j);
  if (v1 >= 0) {
    if (v1) {
      grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
      return;
    }
  } else {
    uint8 v2 = BlockCollGrappleBeam();
    if (!(v2 & 1))
      return;
    if (!(v2 & 0x40)) {
      grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
      return;
    }
  }
  QueueSfx1_Max6(6);
  HandleConnectingGrapple();
  grapple_beam_length_delta = -8;
  samus_grapple_flags |= 1;
}

static void GrappleBeamFunc_ConnectedLockedInPlace(void) {  // 0x9BC77E
  if (!(button_config_shoot_x & joypad1_lastkeys) || !(GrappleBeam_CollDetect_Enemy().k || CheckIfGrappleIsConnectedToBlock()))
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
}

void PropelSamusFromGrappleSwing(void) {  // 0x9BCA65: soltarse del columpio
  uint16 v0;
  int16 s = kSinCosTable8bit_Sext[HI8(grapple_beam_end_angle16) + 64];
  if (!(grapple_beam_unkD26 & 0x8000)) {
    v0 = 2 * grapple_beam_unkD26;
    if (s >= 0) {
      set_hilo(&samus_y_speed, &samus_y_subspeed, Multiply16x16(s, v0));
      samus_y_dir = 2;
    } else {
      set_hilo(&samus_y_speed, &samus_y_subspeed, Multiply16x16(-s, v0));
      samus_y_dir = 1;
    }
  } else {
    v0 = -2 * grapple_beam_unkD26;
    if (s < 0) {
      set_hilo(&samus_y_speed, &samus_y_subspeed, Multiply16x16(-s, v0));
      samus_y_dir = 2;
    } else {
      set_hilo(&samus_y_speed, &samus_y_subspeed, Multiply16x16(s, v0));
      samus_y_dir = 1;
    }
  }
  samus_x_accel_mode = 2;
  uint16 r18 = 64 - 3 * (v0 >> 9);
  uint16 v3 = abs16(kSinCosTable8bit_Sext[(uint8)(HI8(grapple_beam_end_angle16) - r18) + 64]);
  set_hilo(&samus_x_base_speed, &samus_x_base_subspeed, Multiply16x16(v3, v0));
}

static void GrappleBeamFunc_Connected_Swinging(void) {  // 0x9BC79D
  if (button_config_shoot_x & joypad1_lastkeys) {
    GrappleBeamFunc_BB64();
    if (grapple_beam_length_delta)
      BlockFunc_AC31();
    GrappleBeamFunc_BC1F();
    GrappleBeamFunc_BCFF();
    GrappleBeamFunc_BD44();
    HandleMovementAndCollForSamusGrapple();
    if ((grapple_beam_unkD36 & 0x8000) && (HandleSpecialGrappleBeamAngles() & 1))
      return;
    if (GrappleBeam_CollDetect_Enemy().k) {
      grapple_beam_flags |= 0x8000;
    } else if (!(CheckIfGrappleIsConnectedToBlock() & 1)) {
      goto release;
    }
    BlockFunc_AC11();
    GrappleBeamFunc_BD95();
    return;
  }
release:
  if (grapple_beam_unkD26 || grapple_beam_end_angle16 != 0x8000) {
    PropelSamusFromGrappleSwing();
    grapple_beam_function = FUNC16(GrappleBeamFunc_ReleaseFromSwing);
    samus_movement_handler = FUNC16(Samus_MoveHandler_ReleaseFromGrapple);
  } else {
    grapple_beam_function = FUNC16(GrappleBeam_Func2);
  }
}

static void GrappleBeamFunc_Wallgrab(void) {  // 0x9BC814
  if (!(button_config_shoot_x & joypad1_lastkeys) || !(CheckIfGrappleIsConnectedToBlock() & 1)) {
    grapple_walljump_timer = 30;
    grapple_beam_function = FUNC16(GrappleBeamFunc_C832);
  }
}

static void GrappleBeamFunc_C832(void) {  // 0x9BC832: un rato para saltar de la pared
  if (!(--grapple_walljump_timer & 0x8000)) {
    if (Samus_GrappleWallJumpCheck(16 << 16))
      grapple_beam_function = FUNC16(GrappleBeamFunc_C9CE);
  } else {
    grapple_beam_function = FUNC16(GrappleBeam_Func2);
  }
}

static void GrappleEnd(void) {   /* lo común del final de las funciones que sueltan el gancho ($9B:C86F...) */
  grapple_beam_unkD1E = 0;
  grapple_beam_unkD20 = 0;
  grapple_beam_direction = 0;
  grapple_beam_unkD36 = 0;
  grapple_walljump_timer = 0;
  slow_grabble_scrolling_flag = 0;
  grapple_varCF6 = 0;
  grapple_beam_flags = 0;
  flare_counter = 0;
  flare_animation_frame = 0;
  flare_slow_sparks_anim_frame = 0;
  flare_fast_sparks_anim_frame = 0;
  flare_animation_timer = 0;
  flare_slow_sparks_anim_timer = 0;
  flare_fast_sparks_anim_timer = 0;
  LoadProjectilePalette(equipped_beams);
  grapple_beam_function = FUNC16(GrappleBeamFunc_Inactive);
  samus_draw_handler = FUNC16(SamusDrawHandler_Default);
  Samus_PostGrappleCollisionDetect();
  if (samus_auto_cancel_hud_item_index) {
    hud_item_index = 0;
    samus_auto_cancel_hud_item_index = 0;
  }
}

static void GrappleBeamFunc_Cancel(void) {  // 0x9BC856
  QueueSfx1_Max15(7);
  if (samus_movement_type == kMovementType_16_Grappling)
    Samus_Pose_CancelGrapple();
  else
    CallSomeSamusCode(0x1C);
  GrappleEnd();
}

void GrappleBeam_Func2(void) {  // 0x9BC8C5: soltarse quieta
  QueueSfx1_Max15(7);
  uint8 d = kPoseParams[samus_pose].direction_shots_fired;
  if (samus_pose == kPose_B2_FaceR_Grapple_Air) {
    samus_new_pose_transitional = kPose_01_FaceR_Normal;
  } else if (samus_pose == kPose_B3_FaceL_Grapple_Air) {
    samus_new_pose_transitional = kPose_02_FaceL_Normal;
  } else if (sign16(samus_y_radius - 17)) {
    if (d & 0xF0)
      samus_new_pose_transitional = (samus_pose_x_dir == 4) ? kPose_28_FaceL_Crouch : kPose_27_FaceR_Crouch;
    else
      samus_new_pose_transitional = g_byte_9BC9C4(d);
  } else if (d & 0xF0) {
    samus_new_pose_transitional = (samus_pose_x_dir != 4) ? kPose_01_FaceR_Normal : kPose_02_FaceL_Normal;
  } else {
    samus_new_pose_transitional = g_byte_9BC9BA(d);
  }
  samus_hurt_switch_index = 0;
  input_to_pose_calc = 1;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  GrappleEnd();
}

static void GrappleBeamFunc_C9CE(void) {  // 0x9BC9CE: salto desde la pared
  QueueSfx1_Max15(7);
  samus_new_pose_transitional = (samus_pose_x_dir == 8) ? kPose_84_FaceL_Walljump : kPose_83_FaceR_Walljump;
  samus_hurt_switch_index = 6;
  samus_x_accel_mode = 0;
  samus_collides_with_solid_enemy = 0;
  samus_is_falling_flag = 0;
  UNUSED_word_7E0B1A = 0;
  UNUSED_word_7E0B2A = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_y_dir = 0;
  UNUSED_word_7E0B38 = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  GrappleEnd();
}

static void GrappleBeamFunc_ReleaseFromSwing(void) {  // 0x9BCB8B
  QueueSfx1_Max15(7);
  samus_new_pose_transitional = (grapple_beam_unkD26 & 0x8000) ? kPose_51_FaceR_Jump_NoAim_MoveF : kPose_52_FaceL_Jump_NoAim_MoveF;
  samus_hurt_switch_index = 7;
  GrappleEnd();
}

static void Grapple_Func1(void) {  // 0x90D525 (GrappleBeamFunc_FireGoToCancel en el despacho: UNUSED_sub_9BC759)
  int16 t = grapple_walljump_timer;
  --grapple_walljump_timer;
  if ((button_config_shoot_x & joypad1_lastkeys) && grapple_walljump_timer && !((int16)(t - 1) < 0)) {
    if (!(grapple_beam_length_delta & 0x8000)) {
      grapple_beam_length += grapple_beam_length_delta;
      if (!sign16(grapple_beam_length - 96))
        grapple_beam_length_delta = -16;
    }
    Point16U pt = Projectile_SinLookup(HI8(grapple_beam_end_angle16), grapple_beam_length);
    grapple_beam_end_x_pos = pt.x + x_pos_of_start_of_grapple_beam;
    grapple_beam_end_y_pos = pt.y + y_pos_of_start_of_grapple_beam;
    grapple_beam_end_angle16 += 2048;
    GrappleBeamFunc_BF1B();
  } else {
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
  }
}

static void CallGrappleBeamFunc(uint16 f) {
  switch (f) {
  case FUNC16(GrappleBeamFunc_Inactive): GrappleBeamFunc_Inactive(); return;
  case FUNC16(GrappleBeamFunc_FireGoToCancel): GrappleBeamFunc_FireGoToCancel(); return;
  case FUNC16(GrappleBeamFunc_Firing): GrappleBeamFunc_Firing(); return;
  case FUNC16(GrappleBeamFunc_ConnectedLockedInPlace): GrappleBeamFunc_ConnectedLockedInPlace(); return;
  case FUNC16(GrappleBeamFunc_Connected_Swinging): GrappleBeamFunc_Connected_Swinging(); return;
  case FUNC16(GrappleBeamFunc_Wallgrab): GrappleBeamFunc_Wallgrab(); return;
  case FUNC16(GrappleBeamFunc_C832): GrappleBeamFunc_C832(); return;
  case FUNC16(GrappleBeamFunc_Cancel): GrappleBeamFunc_Cancel(); return;
  case FUNC16(GrappleBeamFunc_C9CE): GrappleBeamFunc_C9CE(); return;
  case FUNC16(GrappleBeamFunc_ReleaseFromSwing): GrappleBeamFunc_ReleaseFromSwing(); return;
  case FUNC16(GrappleBeam_Func2): GrappleBeam_Func2(); return;
  case FUNC16(GrappleBeamFunc_BB64): GrappleBeamFunc_BB64(); return;
  case FUNC16(GrappleBeamFunc_BC1F): GrappleBeamFunc_BC1F(); return;
  case FUNC16(GrappleBeamFunc_BCFF): GrappleBeamFunc_BCFF(); return;
  case FUNC16(GrappleBeamFunc_BD44): GrappleBeamFunc_BD44(); return;
  case FUNC16(GrappleBeamFunc_BD95): GrappleBeamFunc_BD95(); return;
  case FUNC16(GrappleBeamFunc_BE98): GrappleClampPrevPos(); return;
  case FUNC16(GrappleBeamFunc_BEEB): GrappleBeamFunc_BEEB(); return;
  case FUNC16(GrappleBeamFunc_BF1B): GrappleBeamFunc_BF1B(); return;
  default: nt_unimpl_v("CallGrappleBeamFunc", f); return;
  }
}

void GrappleBeamHandler(void) {  // 0x9BC490
  if (grapple_varCF6)
    --grapple_varCF6;
  samus_grapple_flags &= ~1;
  CancelGrappleBeamIfIncompatiblePose();
  CallGrappleBeamFunc(grapple_beam_function);
  uint16 r18;
  if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive)
      && sign16(grapple_beam_function - FUNC16(GrappleBeamFunc_Cancel))
      && !(samus_suit_palette_index & 4)
      && fx_type
      && (r18 = Samus_GetBottom_R18(), !(fx_y_pos & 0x8000))
      && sign16(fx_y_pos - r18))
    grapple_beam_flags |= 1;
  else
    grapple_beam_flags &= ~1;
}

/* ------------------------------------------------------------------ dibujo ($90:EB86, $94:AFBA) */
static void HandleGrappleBeamGfx(void) {  // 0x94AFBA: los segmentos (8x8) desde el cañón hasta la punta
  int v1 = CalculateAngleFromXY(grapple_beam_end_x_pos - x_pos_of_start_of_grapple_beam_prevframe,
                                grapple_beam_end_y_pos - y_pos_of_start_of_grapple_beam_prevframe);
  uint32 dx = (uint32)((int32)(int16)kSinCosTable8bit_Sext[v1 + 64] << 11);
  uint32 dy = (uint32)((int32)(int16)kSinCosTable8bit_Sext[v1] << 11);
  uint16 chr = (HI8(grapple_beam_end_angle16) & 0x80) >> 1;
  chr |= 2 * (((HI8(grapple_beam_end_angle16) ^ chr) & 0x40) ^ 0x40);
  chr <<= 8;
  uint32 px = (uint32)(uint16)(x_pos_of_start_of_grapple_beam_prevframe - layer1_x_pos - 4) << 16;
  uint32 py = (uint32)(uint16)(y_pos_of_start_of_grapple_beam_prevframe - layer1_y_pos - 4) << 16;
  if (grapple_beam_length & 0x8000)
    return;
  int n = ((grapple_beam_length / 8) & 0xF) - 1;
  int q = 15;
  do {
    if (grapple_segment_anim_instr_timers[q]-- == 1) {   /* lista de animación del segmento ($94:B18B...) */
      uint16 i = grapple_segment_anim_instr_ptrs[q];
      uint16 w;
      while ((w = B94W(i)) & 0x8000)
        i = B94W(i + 2);                         /* GrappleInstr_Goto ($94:B0F4), la única instrucción */
      grapple_segment_anim_instr_timers[q] = w;
      grapple_segment_anim_instr_ptrs[q] = i + 4;
    }
    if ((py | px) & 0xFF000000)
      break;
    DrawGrappleOam((int16)(px >> 16), (int16)(py >> 16), chr | B94W(grapple_segment_anim_instr_ptrs[q] - 2));   // DrawGrappleOams
    px += dx;
    py += dy;
  } while (q--, --n >= 0);
  if (samus_pose == kPose_B2_FaceR_Grapple_Air || samus_pose == kPose_B3_FaceL_Grapple_Air
      || !((grapple_beam_end_y_pos - layer1_y_pos) & 0xFF00))   // DrawGrappleOams3 ($94:B14B): la punta
    DrawGrappleOam(grapple_beam_end_x_pos - layer1_x_pos - 4, grapple_beam_end_y_pos - layer1_y_pos - 4, 14880);
}

void SamusDrawHandler_Grapple(void) {  // sub_90EB86 ($90:EB86)
  if (!sign16(grapple_beam_function + 0x37AA))
    return;                                       /* (sub_90EB55: el manejador normal, lo hace quien llama) */
  if (grapple_beam_function == FUNC16(GrappleBeamFunc_Firing) || grapple_beam_function == FUNC16(UNUSED_sub_9BC759))
    GrappleBeamFunc_BF1B();
  HandleGrappleBeamFlare();
  HandleAtmosphericEffects();
  if ((arm_cannon_drawing_mode & 0xF) == 2) {
    Samus_Draw();
    Samus_ArmCannon_Draw();
  } else if (arm_cannon_drawing_mode & 0xF) {
    Samus_ArmCannon_Draw();
    Samus_Draw();
  } else {
    Samus_Draw();
  }
  UpdateGrappleBeamTiles();                       /* UpdateGrappleBeamTilesAndIncrFlameCtr ($9B:BFA5) */
  if (sign16(flare_counter - 120))
    ++flare_counter;
  if (grapple_beam_length)
    HandleGrappleBeamGfx();
}

/* ------------------------------------------------------------------ movimiento al soltarse ($90:946E) */
uint16 Samus_DetermineGrappleSwingSpeed_X(void) {  // 0x909C21
  if (equipped_items & 0x20)
    return addr_stru_909F31;
  uint16 r18 = Samus_GetBottom_R18();
  if (!(fx_y_pos & 0x8000))
    return (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) ? addr_stru_909F3D : addr_stru_909F31;
  if ((lava_acid_y_pos & 0x8000) || !sign16(lava_acid_y_pos - r18))
    return addr_stru_909F31;
  return addr_kSamusSpeedTable_Normal_X;
}
