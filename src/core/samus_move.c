/* MegaMetroid — motor nativo: movimiento de Samus (banco $90 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_90.c. Mismo orden de operaciones y mismos casos que el original
 * (incluidos sus defectos), con variables nativas y tablas preparadas en el PC (gen/tables.c).
 * Las ramas que solo existen con objetos que en Ceres no hay (morfosfera, gancho, shinespark, rayos
 * especiales...) llaman a nt_unimpl() para que se vean al probar; se escriben cuando haga falta. */
#include "samus.h"
#include "cinematic.h"
#include "game.h"
#include "hdmaobj.h"
#include "nmath.h"

static inline uint16 Mult8x8(uint8 a, uint8 b) { return (uint16)a * b; }

/* ¿Samus dentro de líquido? (el decomp repite este bloque en muchas funciones) */
static bool in_liquid_at(uint16 y) {
  if (!(fx_y_pos & 0x8000))
    return sign16(fx_y_pos - y) && !(fx_liquid_options & 4);
  return !(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - y);
}

/* ------------------------------------------------------------------ animación ($90:8000) */
static void Samus_HandleAnimDelay(void);

static void Samus_SpawnAirBubbles(void) {  // 0x90813E
  uint16 r20 = Samus_GetTop_R20();
  if ((int16)(r20 - 24 - fx_y_pos) >= 0 && (nmi_frame_counter_word & 0x7F) == 0 && !atmospheric_gfx_frame_and_type[2]) {
    atmospheric_gfx_frame_and_type[2] = 1280;
    atmospheric_gfx_anim_timer[2] = 3;
    atmospheric_gfx_x_pos[2] = samus_x_pos;
    atmospheric_gfx_y_pos[2] = samus_y_pos - samus_y_radius + 6;
    QueueSfx2_Max6((NextRandom() & 1) ? 15 : 17);
  }
  if (samus_pose == kPose_00_FaceF_Powersuit || samus_pose == kPose_9B_FaceF_VariaGravitySuit || (equipped_items & 0x20))
    samus_anim_frame_buffer = 0;
}

static void Samus_SpawnWaterSplash(uint16 r18) {  // 0x9080E6
  static const uint8 kWaterSplashTypeTable[28] = {
    1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  };
  if (kWaterSplashTypeTable[samus_movement_type]) {     /* salpicadura grande: dos chorros */
    atmospheric_gfx_frame_and_type[0] = 256;
    atmospheric_gfx_frame_and_type[1] = 256;
    atmospheric_gfx_anim_timer[0] = 3;
    atmospheric_gfx_anim_timer[1] = 3;
    atmospheric_gfx_x_pos[0] = samus_x_pos + 4;
    atmospheric_gfx_x_pos[1] = samus_x_pos - 3;
    atmospheric_gfx_y_pos[0] = r18 - 4;
    atmospheric_gfx_y_pos[1] = r18 - 4;
  } else {
    atmospheric_gfx_frame_and_type[0] = 768;
    atmospheric_gfx_anim_timer[0] = 2;
    atmospheric_gfx_x_pos[0] = samus_x_pos;
    atmospheric_gfx_y_pos[0] = fx_y_pos;
  }
  Samus_SpawnAirBubbles();
}

static void Samus_Animate_NoFx(void) {  // 0x908078
  uint16 r18 = Samus_GetBottom_R18();
  samus_anim_frame_buffer = samus_x_speed_divisor;
  if (liquid_physics_type) {
    if (liquid_physics_type & 1) {
      liquid_physics_type = 0;
      QueueSfx2_Max6(0xE);
      if (!(samus_suit_palette_index & 4) && (samus_movement_type == 3 || samus_movement_type == 20))
        QueueSfx1_Max6(0x30);
      Samus_SpawnWaterSplash(r18);
    } else {
      liquid_physics_type = 0;
    }
  }
}

static void Samus_Animate_WaterFx(void) {  // 0x9080B8
  uint16 r18 = Samus_GetBottom_R18();
  if (!(fx_y_pos & 0x8000) && sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) {
    samus_anim_frame_buffer = 3;                    /* kSamusPhys_AnimDelayInWater */
    if (liquid_physics_type == 1) {                 /* kLiquidPhysicsType_Water */
      Samus_SpawnAirBubbles();
    } else {
      liquid_physics_type = 1;
      QueueSfx2_Max6(0xD);
      Samus_SpawnWaterSplash(r18);
    }
  } else {
    Samus_Animate_NoFx();
  }
}

static void Samus_Animate_SubmergedLavaAcid(void) {  // 0x90824C
  uint16 r20 = Samus_GetTop_R20();
  samus_anim_frame_buffer = 2;                      /* kSamusPhys_AnimDelayInAcid */
  liquid_physics_type = 2;
  if ((int16)(r20 - lava_acid_y_pos) < 0 && !(atmospheric_gfx_frame_and_type[0] & 0x400)) {
    for (int i = 0; i < 4; i++) {
      atmospheric_gfx_frame_and_type[i] = 1024;
      atmospheric_gfx_y_pos[i] = lava_acid_y_pos;
    }
    atmospheric_gfx_anim_timer[0] = 3;
    atmospheric_gfx_anim_timer[3] = 3;
    atmospheric_gfx_anim_timer[1] = 0x8002;
    atmospheric_gfx_anim_timer[2] = 0x8002;
    atmospheric_gfx_x_pos[0] = samus_x_pos + 6;
    atmospheric_gfx_x_pos[1] = samus_x_pos;
    atmospheric_gfx_x_pos[2] = samus_x_pos;
    atmospheric_gfx_x_pos[3] = samus_x_pos - 6;
    if (!(game_time_frames & 1))
      QueueSfx2_Max6(0x10);
  }
  if (samus_pose == kPose_00_FaceF_Powersuit || samus_pose == kPose_9B_FaceF_VariaGravitySuit || (equipped_items & 0x20))
    samus_anim_frame_buffer = 0;
}

static void Samus_AddPeriodicDamage(uint32 amt) {  /* AddToHiLo(&samus_periodic_damage, &samus_periodic_subdamage) */
  uint32 t = ((uint32)samus_periodic_damage << 16 | samus_periodic_subdamage) + amt;
  samus_periodic_damage = t >> 16;
  samus_periodic_subdamage = (uint16)t;
}

static void Samus_Animate_LavaFx(void) {  // 0x9081C0
  uint16 r18 = Samus_GetBottom_R18();
  if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) {
    if (speed_boost_counter) {
      Samus_CancelSpeedBoost();
      samus_x_extra_run_speed = 0;
      samus_x_extra_run_subspeed = 0;
    }
    if (equipped_items & 0x20) {                    /* traje de gravedad */
      samus_anim_frame_buffer = 0;
      liquid_physics_type = 2;
    } else {
      if (!(game_time_frames & 7) && !sign16(samus_health - 71))
        QueueSfx3_Max3(0x2D);
      Samus_AddPeriodicDamage(0x8000);              /* 0.5 por cuadro */
      Samus_Animate_SubmergedLavaAcid();
    }
  } else {
    Samus_Animate_NoFx();
  }
}

static void Samus_Animate_AcidFx(void) {  // 0x908219
  uint16 r18 = Samus_GetBottom_R18();
  if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) {
    if (!(game_time_frames & 7) && !sign16(samus_health - 71))
      QueueSfx3_Max3(0x2D);
    Samus_AddPeriodicDamage(0x18000);               /* 1.5 por cuadro */
    Samus_Animate_SubmergedLavaAcid();
  } else {
    Samus_Animate_NoFx();
  }
}

void Samus_Animate(void) {  // 0x908000
  switch ((fx_type & 0xF) >> 1) {
  case 0: Samus_Animate_NoFx(); break;
  case 1: Samus_Animate_LavaFx(); break;
  case 2: Samus_Animate_AcidFx(); break;
  case 3: Samus_Animate_WaterFx(); break;
  default: break;
  }
  if (samus_pose == kPose_4D_FaceR_Jump_NoAim_NoMove_NoGun || samus_pose == kPose_4E_FaceL_Jump_NoAim_NoMove_NoGun) {
    if (samus_y_dir != 2 && samus_anim_frame == 1 && samus_anim_frame_timer == 1)
      samus_anim_frame_timer = 4;
  }
  if (!--samus_anim_frame_timer || sign16(samus_anim_frame_timer)) {
    ++samus_anim_frame;
    Samus_HandleAnimDelay();
  }
}

/* instrucciones de la tabla de retardos (byte >= 0x80): devuelven true si después se lee el retardo */
static bool Samus_AnimDelayFunc_13_TransToPose(const uint8 *jp) {  // 0x9084B6
  samus_new_pose_transitional = jp[1];
  samus_hurt_switch_index = 3;
  return false;
}

static bool Samus_AnimDelayFunc_8_AutoJumpHack(const uint8 *jp) {  // 0x908370
  if (samus_input_handler == FUNC16(Samus_InputHandler_E91D))
    return Samus_AnimDelayFunc_13_TransToPose(jp);
  if (samus_new_pose != kPose_4B_FaceR_Jumptrans && samus_new_pose != kPose_4C_FaceL_Jumptrans
      && samus_new_pose != kPose_19_FaceR_SpinJump && samus_new_pose != kPose_1A_FaceL_SpinJump) {
    samus_input_handler = FUNC16(HandleAutoJumpHack);
    return Samus_AnimDelayFunc_13_TransToPose(jp);
  }
  return false;
}

static bool Samus_AnimDelayFunc_6_GotoStartIfLittleHealth(const uint8 *jp) {  // 0x908346
  samus_anim_frame = sign16(samus_health - 30) ? samus_anim_frame + 1 : 0;
  return true;
}

static bool Samus_AnimDelayFunc_7(const uint8 *jp) {  // 0x908360
  samus_movement_handler = FUNC16(Samus_HandleMovement_DrainedCrouching);
  samus_anim_frame++;
  return true;
}

static bool Samus_AnimDelayFunc_9_TransToPose(const uint8 *jp) {  // 0x90839A
  if ((uint16)(jp[1] | jp[2] << 8) & equipped_items)
    samus_new_pose_transitional = (samus_y_speed || samus_y_subspeed) ? jp[6] : jp[5];
  else
    samus_new_pose_transitional = (samus_y_speed || samus_y_subspeed) ? jp[4] : jp[3];
  samus_hurt_switch_index = 3;
  return false;
}

static bool UNUSED_Samus_AnimDelayFunc_10(const uint8 *jp) {  // 0x9083F6
  samus_new_pose_transitional = (samus_y_speed || samus_y_subspeed) ? jp[2] : jp[1];
  samus_hurt_switch_index = 3;
  return false;
}

static bool Samus_AnimDelayFunc_11_SelectDelaySequenceWalljump(const uint8 *jp) {  // 0x90841D
  if (!(equipped_items & 0x20) && in_liquid_at(Samus_GetTop_R20())) {
    QueueSfx1_Max6(0x31);
    samus_anim_frame += 1;
  } else if (equipped_items & 8) {
    QueueSfx1_Max6(0x33);
    samus_anim_frame += 21;
  } else if (!(equipped_items & 0x200)) {
    QueueSfx1_Max6(0x31);
    samus_anim_frame += 1;
  } else {
    QueueSfx1_Max6(0x3E);
    samus_anim_frame += 11;
  }
  return true;
}

static bool Samus_AnimDelayFunc_12_TransToPose(const uint8 *jp) {  // 0x90848B
  samus_new_pose_transitional = ((uint16)(jp[1] | jp[2] << 8) & equipped_items) ? jp[4] : jp[3];
  samus_hurt_switch_index = 3;
  return false;
}

static uint8 Samus_HandleSpeedBoosterAnimDelay(const uint8 *jp) {  // 0x90852C
  if (!samus_has_momentum_flag || !(button_config_run_b & joypad1_lastkeys) || samus_movement_type != 1)
    return jp[0];
  if (!(equipped_items & 0x2000)) {
    samus_anim_frame = 0;
    samus_anim_frame_timer = samus_anim_frame_buffer + bank91_ptr(kDefaultAnimFramePtr)[0];
    return 0;
  }
  if ((uint8)--speed_boost_counter)
    return jp[0];
  uint16 v2 = speed_boost_counter;
  if (!(speed_boost_counter & 0x400)) {
    v2 = speed_boost_counter + 256;
    speed_boost_counter = v2;
    if (v2 & 0x400) {                            /* velocidad máxima: ecos y sonido */
      samus_echoes_sound_flag = 1;
      QueueSfx3_Max6(3);
    }
  }
  int v3 = HI8(v2);
  speed_boost_counter = kSpeedBoostToCtr[v3] | (speed_boost_counter & 0xFF00);
  samus_anim_frame = 0;
  samus_anim_frame_timer = samus_anim_frame_buffer + bank91_ptr(kSpeedBoostToAnimFramePtr[v3])[0];
  return 0;
}

static void Samus_HandleAnimDelay(void) {
  const uint8 *p = bank91_ptr(kSamusAnimationDelayData[samus_pose]);
  if (p[samus_anim_frame] & 0x80) {
    const uint8 *jp = p + samus_anim_frame;
    uint8 v1 = Samus_HandleSpeedBoosterAnimDelay(jp);
    bool r;
    switch (v1 & 0xF) {
    case 0: case 1: case 2: case 3: case 4: case 5: r = false; break;
    case 6: r = Samus_AnimDelayFunc_6_GotoStartIfLittleHealth(jp); break;
    case 7: r = Samus_AnimDelayFunc_7(jp); break;
    case 8: r = Samus_AnimDelayFunc_8_AutoJumpHack(jp); break;
    case 9: r = Samus_AnimDelayFunc_9_TransToPose(jp); break;
    case 10: r = UNUSED_Samus_AnimDelayFunc_10(jp); break;
    case 11: r = Samus_AnimDelayFunc_11_SelectDelaySequenceWalljump(jp); break;
    case 12: r = Samus_AnimDelayFunc_12_TransToPose(jp); break;
    case 13: r = Samus_AnimDelayFunc_13_TransToPose(jp); break;
    case 14: samus_anim_frame -= jp[1]; r = true; break;          // 0x9084C7 Goto
    default: samus_anim_frame = 0; r = true; break;              // 0x9084DB GotoStart
    }
    if (r)
      samus_anim_frame_timer = samus_anim_frame_buffer + p[samus_anim_frame];
  } else {
    if (samus_has_momentum_flag && samus_movement_type == 1) {
      uint16 addr = (equipped_items & 0x2000) ? kSpeedBoostToAnimFramePtr[HI8(speed_boost_counter)] : kDefaultAnimFramePtr;
      p = bank91_ptr(addr);
    }
    samus_anim_frame_timer = samus_anim_frame_buffer + p[samus_anim_frame];
  }
}

/* ------------------------------------------------------------------ dibujo ($90:85E2) */
static uint8 bottom_hidden(void) { samus_bottom_half_spritemap_index = 0; return 0; }

static uint8 SamusBottomDrawn(void) {  // kSamusIsBottomDrawnFuncs ($90:85E2)
  switch (samus_movement_type) {
  case 0:                                    // 0x90868D
    if (samus_pose == kPose_00_FaceF_Powersuit)
      DrawSamusFrontExtra();                 // un sprite más: x-7, y-17, tile 0x3821
    return 1;
  case 3:                                    // 0x9086C6
    if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack
        || samus_pose == kPose_1B_FaceR_SpaceJump || samus_pose == kPose_1C_FaceL_SpaceJump
        || !samus_anim_frame || !sign16(samus_anim_frame - 11))
      return 1;
    return bottom_hidden();
  case 4: case 7: case 8: case 9: case 17: case 18: case 19:   // 0x908688
    return bottom_hidden();
  case 10:                                   // 0x9086EE
    if ((samus_pose != kPose_D7_FaceR_CrystalFlashEnd && samus_pose != kPose_D8_FaceL_CrystalFlashEnd)
        || !sign16(samus_anim_frame - 3))
      return 1;
    return bottom_hidden();
  case 13:                                   // 0x90874C
    if ((samus_pose != 0x65 && samus_pose != 0x66) || sign16(samus_anim_frame - 1))
      return 1;
    return bottom_hidden();
  case 15:                                   // 0x90870C
    if (!sign16(samus_pose - kPose_F1_FaceR_CrouchTrans_AimU))
      return 1;
    if (!sign16(samus_pose - 219)) {
      if (sign16(samus_pose - 221)) {
        if (!samus_anim_frame)
          return 1;
      } else if (samus_anim_frame == 2) {
        return 1;
      }
      return bottom_hidden();
    }
    if (samus_pose == kPose_35_FaceR_CrouchTrans || samus_pose == kPose_36_FaceL_CrouchTrans
        || samus_pose == kPose_3B_FaceR_StandTrans || samus_pose == kPose_3C_FaceL_StandTrans)
      return 1;
    return bottom_hidden();
  case 20:                                   // 0x908768
    if (sign16(samus_anim_frame - 3) || !sign16(samus_anim_frame - 13))
      return 1;
    return bottom_hidden();
  case 25:                                   // 0x90877C
    if (sign16(samus_anim_frame - 2) || !sign16(samus_anim_frame - 9))
      return 1;
    return bottom_hidden();
  case 27:                                   // 0x908790
    if (sign16(samus_pose - kPose_CF_FaceR_Ranintowall_AimUR)) {
      if (samus_pose != kPose_CB_FaceR_Shinespark_Vert && samus_pose != kPose_CC_FaceL_Shinespark_Vert)
        return 1;
    } else if ((samus_pose != kPose_E8_FaceR_Drained_CrouchFalling && samus_pose != kPose_E9_FaceL_Drained_CrouchFalling)
               || !sign16(samus_anim_frame - 2)) {
      return 1;
    }
    return bottom_hidden();
  default:                                   // 0x908686
    return 1;
  }
}

void Samus_Draw(void) {
  if (samus_knockback_timer || !samus_invincibility_timer || samus_shine_timer || !(nmi_frame_counter_word & 1)) {
    samus_top_half_spritemap_index = samus_anim_frame + kSamusPoseToBaseSpritemapIndexTop[samus_pose];
    PairU16 p = Samus_CalcSpritemapPos(2 * samus_pose);
    DrawSamusSpritemap(samus_top_half_spritemap_index, p.k, p.j);
    if (SamusBottomDrawn() & 1) {
      samus_bottom_half_spritemap_index = samus_anim_frame + kSamusPoseToBaseSpritemapIndexBottom[samus_pose];
      DrawSamusSpritemap(samus_bottom_half_spritemap_index, samus_spritemap_x_pos, samus_spritemap_y_pos);
    }
  }
  SetSamusTilesDefsForCurAnim();
}

static void Samus_DrawShinesparkCrashEchoes(uint16 k) {  // 0x9088BA
  if (nmi_frame_counter_word & 1) {
    uint16 a = samus_anim_frame + kSamusPoseToBaseSpritemapIndexTop[samus_pose];
    int16 v2 = speed_echo_ypos[k >> 1] - kPoseParams[samus_pose].y_offset_to_gfx - layer1_y_pos;
    if (v2 >= 0 && sign16(v2 - 248)) {
      DrawSamusSpritemap(a, speed_echo_xpos[k >> 1] - layer1_x_pos, v2);
      if (SamusBottomDrawn() & 1)
        DrawSamusSpritemap(samus_anim_frame + kSamusPoseToBaseSpritemapIndexBottom[samus_pose],
                           speed_echo_xpos[k >> 1] - layer1_x_pos, v2);
    }
  }
}

void Samus_DrawWhenNotAnimatingOrDying(void) {  // 0x908A00
  uint16 a = samus_anim_frame + kSamusPoseToBaseSpritemapIndexTop[samus_pose];
  PairU16 p = Samus_CalcSpritemapPos(2 * samus_pose);
  DrawSamusSpritemap(a, p.k, p.j);
  if (SamusBottomDrawn() & 1)    /* el original no suma la cámara a la mitad de abajo aquí: se conserva */
    DrawSamusSpritemap(samus_anim_frame + kSamusPoseToBaseSpritemapIndexBottom[samus_pose],
                       samus_spritemap_x_pos, samus_spritemap_y_pos);
  SetSamusTilesDefsForCurAnim();
}

static PairU16 Samus_CalcSpritemapPos_Default(uint16 j) {  // 0x908C94
  int v1 = (int8)kPoseParams[j >> 1].y_offset_to_gfx;
  samus_spritemap_y_pos = samus_y_pos - v1 - layer1_y_pos;
  samus_spritemap_x_pos = samus_x_pos - layer1_x_pos;
  return (PairU16){ samus_spritemap_x_pos, samus_spritemap_y_pos };
}

static PairU16 Samus_CalcSpritemapPos_Special(uint16 j) {  // 0x908D98: shinespark, destello de cristal, drenada
  static const int8 byte_908DEF[32] = {
    7, 5, -8, -8, -8, -8, -8, -5,  4, 4, 4, 4, 0, 0, 4, -3,
   -5, 0,  0,  4, -3, -5, -3,  4,  0, 0, 4, 0, 0, 4, 0,  0,
  };
  int v1 = j >> 1, v2;
  if (v1 == kPose_E8_FaceR_Drained_CrouchFalling || v1 == kPose_E9_FaceL_Drained_CrouchFalling) {
    v2 = byte_908DEF[samus_anim_frame];
  } else {
    if ((v1 != kPose_EA_FaceR_Drained_Stand && v1 != kPose_EB_FaceL_Drained_Stand) || (int16)(samus_anim_frame - 5) < 0)
      return Samus_CalcSpritemapPos_Default(j);
    v2 = -3;
  }
  samus_spritemap_y_pos = v2 + samus_y_pos - layer1_y_pos;
  samus_spritemap_x_pos = samus_x_pos - layer1_x_pos;
  return (PairU16){ samus_spritemap_x_pos, samus_spritemap_y_pos };
}

static PairU16 Samus_CalcSpritemapPos_Standing(uint16 j) {  // 0x908CC3
  static const uint8 g_byte_908D28[16] = { 3, 6, 0, 0, 3, 6, 0, 0, 3, 3, 6, 0, 3, 3, 6, 0 };
  uint16 v1 = j >> 1;
  if (!v1 || v1 == kPose_9B_FaceF_VariaGravitySuit) {
    if (!sign16(samus_anim_frame - 2)) {
      samus_spritemap_y_pos = samus_y_pos - 1 - layer1_y_pos;
      samus_spritemap_x_pos = samus_x_pos - layer1_x_pos;
      return (PairU16){ samus_spritemap_x_pos, samus_spritemap_y_pos };
    }
  } else if (!sign16(v1 - kPose_A4_FaceR_LandJump) && sign16(v1 - kPose_A8_FaceR_Grappling)) {
    /* el original lee una palabra de esta tabla de bytes (byte siguiente en el byte alto) */
    uint16 i = (uint16)(samus_anim_frame + 4 * (v1 - 164));
    uint16 r18 = g_byte_908D28[i] | (i + 1 < 16 ? g_byte_908D28[i + 1] : 0) << 8;
    samus_spritemap_y_pos = samus_y_pos - r18 - layer1_y_pos;
    samus_spritemap_x_pos = samus_x_pos - layer1_x_pos;
    return (PairU16){ samus_spritemap_x_pos, samus_spritemap_y_pos };
  }
  return Samus_CalcSpritemapPos_Default(j);
}

static PairU16 Samus_CalcSpritemapPos_Crouch(uint16 j) {  // 0x908D3C
  static const int8 byte_908D80[24] = { -8, 0, -8, 0, -4, -2, -4, -2, 0, 0, 0, 0, -4, 0, -4, 0, 5, 4, 5, 4, 0, 0, 0, 0 };
  uint16 v1 = j >> 1;
  if (sign16(v1 - kPose_35_FaceR_CrouchTrans) || !sign16(v1 - kPose_41_FaceL_Morphball_Ground))
    return Samus_CalcSpritemapPos_Default(j);
  int v4 = byte_908D80[(uint16)(samus_anim_frame + 2 * (v1 - 53))];
  samus_spritemap_y_pos = v4 + samus_y_pos - layer1_y_pos;
  samus_spritemap_x_pos = samus_x_pos - layer1_x_pos;
  return (PairU16){ samus_spritemap_x_pos, samus_spritemap_y_pos };
}

static PairU16 Samus_CalcSpritemapPos_(uint16 k) {
  switch (samus_movement_type) {
  case 0: return Samus_CalcSpritemapPos_Standing(k);
  case 15: return Samus_CalcSpritemapPos_Crouch(k);
  case 27: return Samus_CalcSpritemapPos_Special(k);
  default: return Samus_CalcSpritemapPos_Default(k);
  }
}

PairU16 Samus_CalcSpritemapPos(uint16 k) {  // 0x908C1F
  if (!(ceres_status & 0x8000))
    return Samus_CalcSpritemapPos_(k);
  uint16 old_x = samus_x_pos, old_y = samus_y_pos;  /* modo 7: Samus se dibuja donde cae en el plano girado */
  Mode7Rotate(&samus_x_pos, &samus_y_pos);           // Samus_CalcPos_Mode7
  PairU16 r = Samus_CalcSpritemapPos_(k);
  samus_y_pos = old_y;
  samus_x_pos = old_x;
  return r;
}

/* ------------------------------------------------------------------ movimiento horizontal */
static int32 Samus_CalcBaseSpeed_X(uint16 k);
static Pair_Bool_Amt Samus_CalcBaseSpeed_NoDecel_X(uint16 k);

void Samus_HandleMovement_X(void) {  // 0x908E64
  Samus_HandleExtraRunspeedX();
  Samus_MoveX(Samus_CalcBaseSpeed_X(Samus_DetermineSpeedTableEntryPtr_X()));
}

void Samus_MoveX(int32 amt) {  // 0x908EA9
  bool right;
  if (!samus_x_accel_mode || samus_x_accel_mode == 2)
    right = samus_pose_x_dir != 4;
  else
    right = samus_pose_x_dir != 8;           /* frenando: hacia el lado contrario al que mira */
  amt = right ? Samus_CalcDisplacementMoveRight(amt) : Samus_CalcDisplacementMoveLeft(amt);
  if (amt < 0)
    Samus_MoveLeft(amt);
  else
    Samus_MoveRight(amt);
}

static void Samus_MoveExtraY(void);
static void Samus_Move_NoBaseSpeed_X(void) { Samus_MoveX(0); }  // 0x909348

static void Samus_JumpingMovement(void) {  // 0x908FB3
  Samus_HandleExtraRunspeedX();
  if (samus_pose == kPose_4B_FaceR_Jumptrans || samus_pose == kPose_4C_FaceL_Jumptrans
      || (!sign16(samus_pose - kPose_55_FaceR_Jumptrans_AimU) && sign16(samus_pose - kPose_5B))) {
    samus_x_accel_mode = 0;
    Samus_Move_NoBaseSpeed_X();
    Samus_MoveExtraY();
    return;
  }
  if (samus_y_dir == 1 && (!(button_config_jump_a & joypad1_lastkeys) || (samus_y_speed & 0x8000))) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 2;
  }
  Pair_Bool_Amt pair = Samus_CalcBaseSpeed_NoDecel_X(Samus_DetermineSpeedTableEntryPtr_X());
  if (samus_movement_type == 20) {
    if (!samus_x_accel_mode)
      samus_x_accel_mode = 2;
    Samus_MoveX(pair.amt);
  } else if (samus_x_accel_mode || (joypad1_lastkeys & 0x100) || (joypad1_lastkeys & 0x200)) {
    Samus_MoveX(pair.amt);
  } else {
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_collision_flag = 0;
  }
  Samus_MoveY_WithSpeedCalc();
}

static uint8 Samus_WallJumpCheck(int32 amt);

static void Samus_SpinJumpMovement(void) {  // 0x909040
  Samus_HandleExtraRunspeedX();
  if (samus_y_dir == 1 && (!(button_config_jump_a & joypad1_lastkeys) || (samus_y_speed & 0x8000))) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 2;
  }
  Pair_Bool_Amt pair = Samus_CalcBaseSpeed_NoDecel_X(Samus_DetermineSpeedTableEntryPtr_X());
  bool keep = pair.flag || samus_x_accel_mode == 1;
  if (!keep)
    keep = (samus_pose_x_dir == 4) ? (joypad1_lastkeys & kButton_Left) != 0 : (joypad1_lastkeys & kButton_Right) != 0;
  if (keep) {
    if (!samus_x_accel_mode)
      samus_x_accel_mode = 2;
  } else {
    pair.amt = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_collision_flag = 0;
    samus_x_accel_mode = 0;
  }
  Samus_MoveX(pair.amt);
  if (!Samus_WallJumpCheck(SHL16(8)))
    Samus_MoveY_WithSpeedCalc();
}

static void Samus_CheckStartFalling(void) {  // 0x9090C4
  if (samus_y_dir == 1 && (samus_y_speed & 0x8000)) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 2;
  }
}

void Samus_MoveY_WithSpeedCalc(void) {  // 0x9090E2
  int32 amt = IPAIR32(samus_y_speed, samus_y_subspeed);
  if (samus_y_dir == 2) {
    if (samus_y_speed != 5)
      add_hilo(&samus_y_speed, &samus_y_subspeed, IPAIR32(samus_y_accel, samus_y_subaccel));
  } else {
    add_hilo(&samus_y_speed, &samus_y_subspeed, -IPAIR32(samus_y_accel, samus_y_subaccel));
  }
  if (samus_y_dir != 2)
    amt = -amt;
  amt += IPAIR32(extra_samus_y_displacement, extra_samus_y_subdisplacement);
  if (amt < 0)
    Samus_MoveUp(amt);
  else
    Samus_MoveDown(amt);
}

static void Samus_FallingMovement(void) {  // 0x909168
  Samus_HandleExtraRunspeedX();
  Pair_Bool_Amt pair = Samus_CalcBaseSpeed_NoDecel_X(Samus_DetermineSpeedTableEntryPtr_X());
  if (samus_x_accel_mode || (joypad1_lastkeys & kButton_Right) || (joypad1_lastkeys & kButton_Left)) {
    Samus_MoveX(pair.amt);
  } else {
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_collision_flag = 0;
  }
  Samus_CheckStartFalling();
  Samus_MoveY_WithSpeedCalc();
}

static void Samus_Move_NoSpeedCalc_Y(void) {  // 0x90923F
  int32 amt = IPAIR32(extra_samus_y_displacement, extra_samus_y_subdisplacement);
  if (amt == 0) {
    if (!samus_pos_adjusted_by_slope_flag)
      amt = IPAIR32(samus_total_x_speed, samus_total_x_subspeed);
    Samus_MoveDown(amt + SHL16(1));
  } else if (amt < 0) {
    Samus_MoveUp(amt);
  } else {
    Samus_MoveDown(amt + SHL16(1));
  }
}

static void Samus_MoveExtraY(void) {  // 0x909288
  int32 amt = IPAIR32(extra_samus_y_displacement, extra_samus_y_subdisplacement);
  if (amt != 0) {
    if (amt < 0)
      Samus_MoveUp(amt);
    else
      Samus_MoveDown(amt + SHL16(1));
  }
}

static uint8 Samus_CheckAndMoveY(void) {  // 0x9092B8
  if (!samus_y_dir)
    return 0;
  Samus_CheckStartFalling();
  Samus_MoveY_WithSpeedCalc();
  return 1;
}

static void Samus_MoveRight_NoColl(int32 amt) {  // 0x909826
  add_hilo(&samus_x_pos, &samus_x_subpos, amt);
  set_hilo(&projectile_init_speed_samus_moved_right, &projectile_init_speed_samus_moved_right_fract, amt);
}
static void Samus_MoveLeft_NoColl(int32 amt) {  // 0x909842
  add_hilo(&samus_x_pos, &samus_x_subpos, -amt);
  set_hilo(&projectile_init_speed_samus_moved_left, &projectile_init_speed_samus_moved_left_fract, -amt);
}
static void Samus_MoveDown_NoColl(int32 amt) {  // 0x909871
  add_hilo(&samus_y_pos, &samus_y_subpos, amt);
  set_hilo(&projectile_init_speed_samus_moved_down, &projectile_init_speed_samus_moved_down_fract, amt);
}
static void Samus_MoveUp_NoColl(int32 amt) {  // 0x90988D
  add_hilo(&samus_y_pos, &samus_y_subpos, -amt);
  set_hilo(&projectile_init_speed_samus_moved_up, &projectile_init_speed_samus_moved_up_fract, -amt);
}

void Samus_MoveLeft(int32 amt) {  // 0x909350
  amt = -amt;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(amt);
  samus_collision_flag = cres.collision, amt = cres.amt;
  if (samus_collision_flag) {
    Samus_ClearXSpeedIfColl();
    Samus_MoveLeft_NoColl(amt);
    Samus_AlignYPosSlope();
  } else {
    amt = Samus_MoveRight_NoSolidColl(-amt);
    set_hilo(&projectile_init_speed_samus_moved_left, &projectile_init_speed_samus_moved_left_fract, amt);
    if (samus_collision_direction & 1)
      samus_collision_flag = 0;
    Samus_ClearXSpeedIfColl();
    Samus_AlignYPosSlope();
  }
}

void Samus_MoveRight(int32 amt) {  // 0x9093B1
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(amt);
  samus_collision_flag = cres.collision, amt = cres.amt;
  if (samus_collision_flag) {
    Samus_ClearXSpeedIfColl();
    Samus_MoveRight_NoColl(amt);
    Samus_AlignYPosSlope();
  } else {
    amt = Samus_MoveRight_NoSolidColl(amt);
    set_hilo(&projectile_init_speed_samus_moved_right, &projectile_init_speed_samus_moved_right_fract, amt);
    if (!(samus_collision_direction & 1))
      samus_collision_flag = 0;
    Samus_ClearXSpeedIfColl();
    Samus_AlignYPosSlope();
  }
}

void Samus_MoveUp(int32 amt) {  // 0x9093EC
  samus_collision_direction = 2;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(-amt);
  samus_collision_flag = cres.collision, amt = cres.amt;
  if (samus_collision_flag) {
    Samus_MoveUp_SetPoseCalcInput();
    Samus_MoveUp_NoColl(amt);
  } else {
    amt = Samus_MoveDown_NoSolidColl(-amt);
    set_hilo(&projectile_init_speed_samus_moved_up, &projectile_init_speed_samus_moved_up_fract, amt);
    Samus_MoveUp_SetPoseCalcInput();
  }
}

void Samus_MoveDown(int32 amt) {  // 0x909440
  samus_collision_direction = 3;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(amt);
  samus_collision_flag = cres.collision, amt = cres.amt;
  if (samus_collision_flag) {
    Samus_MoveDown_SetPoseCalcInput();
    Samus_MoveDown_NoColl(amt);
  } else {
    amt = Samus_MoveDown_NoSolidColl(amt);
    projectile_init_speed_samus_moved_down_fract = (uint16)amt;
    projectile_init_speed_samus_moved_down = (uint16)((uint32)amt >> 16);
    Samus_MoveDown_SetPoseCalcInput();
  }
}

/* ------------------------------------------------------------------ scroll de la cámara ($90:94EC) */
void HandleAutoscrolling_X(void);
void HandleAutoscrolling_Y(void);
void HandleScrollingWhenTriggeringScrollLeft(void);
void HandleScrollingWhenTriggeringScrollRight(void);
void HandleScrollingWhenTriggeringScrollUp(void);
void HandleScrollingWhenTriggeringScrollDown(void);

static void Samus_CalcDistanceMoved_X(void) {  // 0x9096C0
  if ((int16)(samus_x_pos - samus_prev_x_pos) >= 0)
    set_hilo(&absolute_moved_last_frame_x, &absolute_moved_last_frame_x_fract,
             PAIR32(samus_x_pos, samus_x_subpos) - PAIR32(samus_prev_x_pos, samus_prev_x_subpos) + (1 << 16));
  else
    set_hilo(&absolute_moved_last_frame_x, &absolute_moved_last_frame_x_fract,
             PAIR32(samus_prev_x_pos, samus_prev_x_subpos) - PAIR32(samus_x_pos, samus_x_subpos) + (1 << 16));
}

static void Samus_CalcDistanceMoved_Y(void) {  // 0x9096FF
  if ((int16)(samus_y_pos - samus_prev_y_pos) >= 0)
    set_hilo(&absolute_moved_last_frame_y, &absolute_moved_last_frame_y_fract,
             PAIR32(samus_y_pos, samus_y_subpos) - PAIR32(samus_prev_y_pos, samus_prev_y_subpos) + (1 << 16));
  else
    set_hilo(&absolute_moved_last_frame_y, &absolute_moved_last_frame_y_fract,
             PAIR32(samus_prev_y_pos, samus_prev_y_subpos) - PAIR32(samus_y_pos, samus_y_subpos) + (1 << 16));
}

void Samus_ScrollFinishedHook_SporeSpawnFight(void) {  // 0x909589
  if (layer1_y_pos <= 0x1D0)
    layer1_y_pos = 464;
}

void Samus_HandleScroll_X(void) {  // 0x9095A0
  static const uint16 kFaceLeft[4] = { 0xa0, 0x50, 0x20, 0xe0 };
  static const uint16 kFaceRight[4] = { 0x60, 0x40, 0x20, 0xe0 };
  if (samus_prev_x_pos == samus_x_pos) {
    HandleAutoscrolling_X();
    return;
  }
  if ((knockback_dir || samus_movement_type == 16 || samus_x_accel_mode == 1) ^ (samus_pose_x_dir != 4))
    ideal_layer1_xpos = samus_x_pos - kFaceRight[camera_distance_index >> 1];
  else
    ideal_layer1_xpos = samus_x_pos - kFaceLeft[camera_distance_index >> 1];
  if (ideal_layer1_xpos != layer1_x_pos) {
    if ((int16)(ideal_layer1_xpos - layer1_x_pos) < 0) {
      add_hilo(&layer1_x_pos, &layer1_x_subpos, -IPAIR32(absolute_moved_last_frame_x, absolute_moved_last_frame_x_fract));
      HandleScrollingWhenTriggeringScrollLeft();
    } else {
      add_hilo(&layer1_x_pos, &layer1_x_subpos, IPAIR32(absolute_moved_last_frame_x, absolute_moved_last_frame_x_fract));
      HandleScrollingWhenTriggeringScrollRight();
    }
  }
}

static void Samus_HandleScroll_Y(void) {  // 0x90964F
  if (samus_prev_y_pos == samus_y_pos) {
    HandleAutoscrolling_Y();
    return;
  }
  if (samus_y_dir == 1)
    ideal_layer1_ypos = samus_y_pos - down_scroller;
  else
    ideal_layer1_ypos = samus_y_pos - up_scroller;
  if (ideal_layer1_ypos != layer1_y_pos) {
    if ((int16)(ideal_layer1_ypos - layer1_y_pos) < 0) {
      add_hilo(&layer1_y_pos, &layer1_y_subpos, -IPAIR32(absolute_moved_last_frame_y, absolute_moved_last_frame_y_fract));
      HandleScrollingWhenTriggeringScrollUp();
    } else {
      add_hilo(&layer1_y_pos, &layer1_y_subpos, IPAIR32(absolute_moved_last_frame_y, absolute_moved_last_frame_y_fract));
      HandleScrollingWhenTriggeringScrollDown();
    }
  }
}

void MainScrollingRoutine(void) {  // 0x9094EC
  if (slow_grabble_scrolling_flag) {
    nt_unimpl("MainScrollingRoutine (gancho)");
  } else {
    Samus_CalcDistanceMoved_X();
    Samus_HandleScroll_X();
    Samus_CalcDistanceMoved_Y();
    Samus_HandleScroll_Y();
  }
  if (scrolling_finished_hook) {               // CallScrollingFinishedHook: el único es el de Spore Spawn
    if (scrolling_finished_hook == 0x9589) Samus_ScrollFinishedHook_SporeSpawnFight();
    else nt_unimpl_v("scrolling_finished_hook", scrolling_finished_hook);
  }
  samus_prev_x_pos = samus_x_pos;
  samus_prev_x_subpos = samus_x_subpos;
  samus_prev_y_pos = samus_y_pos;
  samus_prev_y_subpos = samus_y_subpos;
}

/* ------------------------------------------------------------------ velocidad horizontal */
void Samus_HandleExtraRunspeedX(void) {  // 0x90973E
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (in_liquid_at(r18))
      goto no_run;
  }
  if (samus_movement_type != 1 || !(button_config_run_b & joypad1_lastkeys))
    goto no_run;
  if (equipped_items & 0x2000) {                 /* speed booster */
    if (!samus_has_momentum_flag) {
      samus_has_momentum_flag = 1;
      special_samus_palette_timer = 1;
      special_samus_palette_frame = 0;
      speed_boost_counter = kSpeedBoostToCtr[0];
    }
    if ((int16)(samus_x_extra_run_speed - kSamus_HandleExtraRunspeedX_Tab2) >= 0
        && (int16)(samus_x_extra_run_subspeed - kSamus_HandleExtraRunspeedX_Tab3) >= 0) {
      samus_x_extra_run_speed = kSamus_HandleExtraRunspeedX_Tab2;
      samus_x_extra_run_subspeed = kSamus_HandleExtraRunspeedX_Tab3;
      goto done;
    }
  } else {
    if (!samus_has_momentum_flag) {
      samus_has_momentum_flag = 1;
      speed_boost_counter = 0;
    }
    if ((int16)(samus_x_extra_run_speed - kSamus_HandleExtraRunspeedX_Tab4) >= 0
        && (int16)(samus_x_extra_run_subspeed - kSamus_HandleExtraRunspeedX_Tab5) >= 0) {
      samus_x_extra_run_speed = kSamus_HandleExtraRunspeedX_Tab4;
      samus_x_extra_run_subspeed = kSamus_HandleExtraRunspeedX_Tab5;
      goto done;
    }
  }
  add_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed,
           IPAIR32(kSamus_HandleExtraRunspeedX_Tab0, kSamus_HandleExtraRunspeedX_Tab1));
  goto done;
no_run:
  if (!samus_has_momentum_flag) {
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
  }
done:
  if ((speed_boost_counter & 0xFF00) == 1024)
    samus_contact_damage_index = 1;
}

void Samus_InitJump(void) {  // 0x9098BC
  static const uint16 kSpeed_Y[3] = { 4, 1, 2 };
  static const uint16 kSubspeed_Y[3] = { 0xe000, 0xc000, 0xc000 };
  static const uint16 kTab2[3] = { 6, 2, 3 };
  static const uint16 kTab3[3] = { 0, 0x8000, 0x8000 };
  uint16 v0 = 0;
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (!(fx_y_pos & 0x8000)) {
      if (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4))
        v0 = 2;
    } else if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) {
      v0 = 4;
    }
  }
  int v1 = v0 >> 1;
  if (equipped_items & 0x100) {
    samus_y_subspeed = kTab3[v1];
    samus_y_speed = kTab2[v1];
  } else {
    samus_y_subspeed = kSubspeed_Y[v1];
    samus_y_speed = kSpeed_Y[v1];
  }
  if (equipped_items & 0x2000) {
    samus_y_subspeed += samus_x_extra_run_subspeed;
    samus_y_speed += samus_x_extra_run_speed >> 1;
  }
  grapple_walljump_timer = 0;
  reached_ceres_elevator_fade_timer = 0;
  samus_y_dir = 1;
}

void Samus_InitWallJump(void) {  // 0x909949
  static const uint16 kTab0[3] = { 4, 0, 2 };
  static const uint16 kTab1[3] = { 0xa000, 0x4000, 0xa000 };
  static const uint16 kTab2[3] = { 5, 0, 3 };
  static const uint16 kTab3[3] = { 0x8000, 0x8000, 0x8000 };
  uint16 v0 = 0;
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (!(fx_y_pos & 0x8000)) {
      if (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4))
        v0 = 2;
    } else if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) {
      v0 = 4;
    }
  }
  int v1 = v0 >> 1;
  if (equipped_items & 0x100) {
    samus_y_subspeed = kTab3[v1];
    samus_y_speed = kTab2[v1];
  } else {
    samus_y_subspeed = kTab1[v1];
    samus_y_speed = kTab0[v1];
  }
  if (equipped_items & 0x2000) {
    samus_y_subspeed += samus_x_extra_run_subspeed;
    samus_y_speed += samus_x_extra_run_speed >> 1;
  }
  grapple_walljump_timer = 0;
  reached_ceres_elevator_fade_timer = 0;
  samus_y_dir = 1;
}

static bool IsGreaterThanQuirked(uint16 vhi, uint16 vlo, uint16 cmphi, uint16 cmplo) {
  if ((int16)(vhi - cmphi) >= 0) {
    if (vhi != cmphi || (((int16)(vlo - cmplo) >= 0) && vlo != cmplo))
      return true;
  }
  return false;
}

static int32 decel_delta(const SamusSpeedEntry *s) {
  if (samus_x_decel_mult)
    return IPAIR32(Mult8x8(samus_x_decel_mult, (uint8)s->decel) >> 8, Mult8x8(samus_x_decel_mult, HI8(s->decel_sub)));
  return IPAIR32(s->decel, s->decel_sub);
}

static int32 Samus_CalcBaseSpeed_X(uint16 k) {  // 0x909A7E
  const SamusSpeedEntry *s = speed_entry(k);
  if (samus_x_accel_mode) {
    add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, -decel_delta(s));
    if ((int16)samus_x_base_speed < 0) {
      samus_x_base_speed = 0;
      samus_x_base_subspeed = 0;
      samus_x_accel_mode = 0;
    }
  } else {
    add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(s->accel, s->accel_sub));
    if (IsGreaterThanQuirked(samus_x_base_speed, samus_x_base_subspeed, s->max_speed, s->max_speed_sub)) {
      samus_x_base_speed = s->max_speed;
      samus_x_base_subspeed = s->max_speed_sub;
    }
  }
  return IPAIR32(samus_x_base_speed, samus_x_base_subspeed);
}

static Pair_Bool_Amt Samus_CalcBaseSpeed_NoDecel_X(uint16 k) {  // 0x909B1F
  const SamusSpeedEntry *s = speed_entry(k);
  bool rv = false;
  if (samus_x_accel_mode & 1) {
    add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, -decel_delta(s));
    if ((int16)samus_x_base_speed < 0) {
      samus_x_base_speed = 0;
      samus_x_base_subspeed = 0;
      samus_x_accel_mode = 0;
    }
  } else {
    add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(s->accel, s->accel_sub));
    if (IsGreaterThanQuirked(samus_x_base_speed, samus_x_base_subspeed, s->max_speed, s->max_speed_sub)) {
      samus_x_base_speed = s->max_speed;
      samus_x_base_subspeed = s->max_speed_sub;
      rv = true;
    }
  }
  return (Pair_Bool_Amt){ rv, IPAIR32(samus_x_base_speed, samus_x_base_subspeed) };
}

uint16 Samus_DetermineSpeedTableEntryPtr_X(void) {  // 0x909BD1
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (fx_y_pos & 0x8000) {
      if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18))
        samus_x_speed_table_pointer = addr_kSamusSpeedTable_LavaAcid_X;
    } else if (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) {
      samus_x_speed_table_pointer = addr_kSamusSpeedTable_Water_X;
    }
  }
  return samus_x_speed_table_pointer + 12 * samus_movement_type;
}

void Samus_DetermineAccel_Y(void) {  // 0x909C5B
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (!(fx_y_pos & 0x8000)) {
      if (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) {
        samus_y_subaccel = 0x800;
        samus_y_accel = 0;
        return;
      }
    } else if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) {
      samus_y_subaccel = 0x900;
      samus_y_accel = 0;
      return;
    }
  }
  samus_y_subaccel = 0x1c00;
  samus_y_accel = 0;
}

/* ------------------------------------------------------------------ salto de pared ($90:9D35) */
static uint8 Samus_WallJumpCheck(int32 amt) {
  CheckEnemyColl_Result cres;
  if (samus_last_different_pose_movement_type != 3 && samus_last_different_pose_movement_type != 20)
    return 0;
  bool late;
  if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack)
    late = !sign16(samus_anim_frame - 27);
  else
    late = !sign16(samus_anim_frame - 11);
  if (!late) {
    if (!(joypad1_lastkeys & kButton_Left)) {
      if (!(joypad1_lastkeys & kButton_Right))
        return 0;
      samus_collision_direction = 0;
      cres = Samus_CheckSolidEnemyColl(amt);
      amt = cres.amt;
      if (!cres.collision) {
        amt = WallJumpBlockCollDetect(-amt);
        if (!samus_collision_flag)
          return 0;
      }
    } else {
      samus_collision_direction = 1;
      cres = Samus_CheckSolidEnemyColl(amt);
      amt = cres.amt;
      if (!cres.collision) {
        amt = WallJumpBlockCollDetect(amt);
        if (!samus_collision_flag)
          return 0;
      }
    }
    samus_anim_frame_timer = 1;
    if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack)
      samus_anim_frame = 26;
    else
      samus_anim_frame = 10;
    return 0;
  }
  enemy_index_to_shake = 0xFFFF;
  bool enemy;
  if (joypad1_lastkeys & kButton_Left) {
    samus_collision_direction = 1;
    cres = Samus_CheckSolidEnemyColl(amt), amt = cres.amt;
    enemy = cres.collision != 0;
    if (!enemy) {
      amt = WallJumpBlockCollDetect(amt);
      if (!(samus_collision_flag && (button_config_jump_a & joypad1_newkeys)))
        return 0;
    }
  } else {
    if (!(joypad1_lastkeys & kButton_Right))
      return 0;
    samus_collision_direction = 0;
    cres = Samus_CheckSolidEnemyColl(amt), amt = cres.amt;
    enemy = cres.collision != 0;
    if (!enemy) {
      amt = -amt;
      WallJumpBlockCollDetect(amt);           // el original no guarda el resultado aquí
      if (!(samus_collision_flag && (button_config_jump_a & joypad1_newkeys)))
        return 0;
    }
  }
  if (enemy && !(button_config_jump_a & joypad1_newkeys))
    return 0;
  if (sign16((uint16)(amt >> 16) - 8)) {
    input_to_pose_calc = 5;
    if (enemy)
      enemy_index_to_shake = collision_detection_index;
    return 1;
  }
  return 0;
}

/* ------------------------------------------------------------------ manejadores por tipo de movimiento */
static void Samus_Movement_00_Standing(void) {  // 0x90A383
  if (samus_pose && samus_pose != kPose_9B_FaceF_VariaGravitySuit) {
    if ((samus_pose == kPose_01_FaceR_Normal || samus_pose == kPose_02_FaceL_Normal)
        && (button_config_shoot_x & joypad1_lastkeys)) {
      samus_anim_frame_timer = 16;
      samus_anim_frame = 0;
    }
    Samus_Move_NoBaseSpeed_X();
    Samus_Move_NoSpeedCalc_Y();
    Samus_CancelSpeedBoost();
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_accel_mode = 0;
  } else {
    if (elevator_status) {
      samus_collision_direction = 2;
      Samus_MoveDown_NoSolidColl(SHL16(1));
    }
    input_to_pose_calc = 0;
  }
}

void Samus_FootstepGraphics(void);

static void Samus_Movement_01_Running(void) {  // 0x90A3E5
  static const uint8 kSamusFootstepFrame[10] = { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 };
  Samus_HandleMovement_X();
  Samus_Move_NoSpeedCalc_Y();
  if (samus_anim_frame_timer == 1 && kSamusFootstepFrame[samus_anim_frame]) {
    Samus_FootstepGraphics();
    if (!cinematic_function && !boss_id && !samus_shine_timer && !(speed_boost_counter & 0x400))
      QueueSfx3_Max6(6);
  }
}

static void Samus_Movement_03_SpinJumping(void) {  // 0x90A436
  uint16 r18 = 0;
  if (!(samus_suit_palette_index & 4)) {
    uint16 r20 = Samus_GetTop_R20();
    if (in_liquid_at(r20))
      r18 = 1;
  }
  if (!r18) {
    if ((equipped_items & 0x200) && samus_y_dir == 2) {   /* space jump: se puede volver a saltar al caer */
      /* la palabra en samus_y_subspeed + 1: byte bajo de la velocidad y alto de la subvelocidad */
      uint16 v = (uint16)(samus_y_speed << 8 | samus_y_subspeed >> 8);
      uint16 vmin = liquid_physics_type ? 0x80 : 0x280;   // kSamusPhys_JumpMinYVelWater / Air
      if (!sign16(v - vmin) && sign16(v - 0x500)) {        // kSamusPhys_JumpMaxYVelWater / Air
        UNUSED_word_7E0DFA = (UNUSED_word_7E0DFA & 0xFF00) | 1;
        if (button_config_jump_a & joypad1_newkeys)
          Samus_InitJump();
      }
    }
    if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack)
      samus_contact_damage_index = 3;
    else if (!sign16(flare_counter - 60))
      samus_contact_damage_index = 4;
  } else {
    if (samus_anim_frame_timer == 1 && kSamusFramesForUnderwaterSfx[samus_anim_frame])
      QueueSfx1_Max6(0x2F);
  }
  Samus_SpinJumpMovement();
}

static void Samus_Movement_05_Crouching(void) {  // 0x90A573
  Samus_Move_NoBaseSpeed_X();
  Samus_Move_NoSpeedCalc_Y();
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_x_accel_mode = 0;
}

static void Samus_Movement_06_Falling(void) {  // 0x90A58D
  Samus_FallingMovement();
  if ((samus_pose == kPose_29_FaceR_Fall || samus_pose == kPose_2A_FaceL_Fall
       || samus_pose == kPose_67_FaceR_Fall_Gun || samus_pose == kPose_68_FaceL_Fall_Gun)
      && !sign16(samus_y_speed - 5)) {
    if (sign16(samus_anim_frame - 5)) {
      samus_anim_frame_timer = 8;
      samus_anim_frame = 5;
    }
  }
}

uint8 Samus_MoveY_Simple_(void);

static void Samus_Movement_0F_CrouchingEtcTransition(void) {
  if (sign16(samus_pose - kPose_F1_FaceR_CrouchTrans_AimU)) {
    if (!sign16(samus_pose - kPose_DB))
      goto done;
    uint16 idx = samus_pose - 53;                 /* kSamusCrouchingEtcFuncs ($90:A61C) */
    if (idx == 4 || idx == 5 || idx == 10 || idx == 11) {
      enable_horiz_slope_coll = 3;                 // SamusCrouchingEtcFunc ($90:A672)
      UNUSEDword_7E0AA4 = 0;
    } else if (idx >= 12) {
      nt_unimpl("Samus_Movement_0F (pose fuera de la tabla)");
    }
  }
  Samus_Move_NoBaseSpeed_X();
  if (!(Samus_MoveY_Simple_() & 1))
    Samus_Move_NoSpeedCalc_Y();
done:
  if (input_to_pose_calc == 1025) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 0;
    used_for_ball_bounce_on_landing = 0;
  }
  input_to_pose_calc = 0;
}

uint8 Samus_MoveY_Simple_(void) {  // 0x90A61E
  if (samus_y_dir != 1)
    return 0;
  Samus_MoveY_WithSpeedCalc();
  return 1;
}

static void Samus_Movement_0E_TurningAroundOnGround(void) {  // 0x90A67C
  Samus_HandleMovement_X();
  Samus_Move_NoSpeedCalc_Y();
  Samus_CancelSpeedBoost();
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  input_to_pose_calc = 0;
}

static void Samus_Movement_10_Moonwalking(void) {  // 0x90A694
  Samus_HandleMovement_X();
  Samus_Move_NoSpeedCalc_Y();
}

static void Samus_Movement_14_WallJumping(void) {  // 0x90A734
  if (sign16(samus_anim_frame - 23)) {
    if (!sign16(samus_anim_frame - 3) && !sign16(flare_counter - 60))
      samus_contact_damage_index = 4;
  } else {
    samus_contact_damage_index = 3;
  }
  Samus_JumpingMovement();
}

static void Samus_Movement_15_RanIntoWall(void) {  // 0x90A75F
  Samus_Move_NoBaseSpeed_X();
  Samus_Move_NoSpeedCalc_Y();
  Samus_CancelSpeedBoost();
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_x_accel_mode = 0;
}

static void Samus_Movement_TurningAroundInAir(void) {  // 0x90A790 / 0x90A7AD
  Samus_HandleMovement_X();
  if (!(Samus_CheckAndMoveY() & 1))
    Samus_Move_NoSpeedCalc_Y();
  Samus_CancelSpeedBoost();
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  input_to_pose_calc = 0;
}

/* ------------------------------------------------------------------ la bola ($90:919F, $90:A521) */
static void Samus_MorphedStopIfNoInput(Pair_Bool_Amt *pair) {
  if (!samus_x_accel_mode && !(joypad1_lastkeys & kButton_Right) && !(joypad1_lastkeys & kButton_Left)) {
    pair->amt = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_collision_flag = 0;
  }
}

static void Samus_MorphedFallingMovement(void) {  // 0x90919F
  Pair_Bool_Amt pair = Samus_CalcBaseSpeed_NoDecel_X(Samus_DetermineSpeedTableEntryPtr_X());
  Samus_MorphedStopIfNoInput(&pair);
  Samus_MoveX(pair.amt);
  Samus_CheckStartFalling();
  Samus_MoveY_WithSpeedCalc();
}

static void Samus_MorphedBouncingMovement(void) {  // 0x9091D1
  Pair_Bool_Amt pair = Samus_CalcBaseSpeed_NoDecel_X(Samus_DetermineSpeedTableEntryPtr_X());
  Samus_MorphedStopIfNoInput(&pair);
  Samus_MoveX(pair.amt);
  if (!knockback_dir) {
    if (extra_samus_y_displacement || extra_samus_y_subdisplacement) {
      samus_y_dir = 2;
      samus_y_subspeed = 0;
      samus_y_speed = 0;
      int32 amt = IPAIR32(extra_samus_y_displacement, extra_samus_y_subdisplacement);
      if (amt < 0) Samus_MoveUp(amt);
      else Samus_MoveDown(amt + SHL16(1));
    } else {
      Samus_CheckStartFalling();
      Samus_MoveY_WithSpeedCalc();
    }
  }
}

static void Samus_Movement_04_MorphBallOnGround(void) {  // 0x90A521
  if (!samus_x_accel_mode &&
      ((samus_pose_x_dir == 4) ? samus_pose == kPose_41_FaceL_Morphball_Ground : samus_pose == kPose_1D_FaceR_Morphball_Ground)) {
    Samus_Move_NoBaseSpeed_X();
    if (!(Samus_CheckAndMoveY() & 1)) {
      Samus_Move_NoSpeedCalc_Y();
      Samus_CancelSpeedBoost();
      samus_x_extra_run_speed = 0;
      samus_x_extra_run_subspeed = 0;
      samus_x_base_speed = 0;
      samus_x_base_subspeed = 0;
      samus_x_accel_mode = 0;
    }
    return;
  }
  Samus_HandleMovement_X();
  if (!(Samus_CheckAndMoveY() & 1))
    Samus_Move_NoSpeedCalc_Y();
}

static void Samus_Movement_08_MorphBallFalling(void) {  // 0x90A5CA
  if (!(joypad1_lastkeys & (kButton_Left | kButton_Right)) && !samus_x_accel_mode) {
    Samus_CancelSpeedBoost();
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_accel_mode = 0;
  }
  if (used_for_ball_bounce_on_landing)
    Samus_MorphedBouncingMovement();
  else
    Samus_MorphedFallingMovement();
}

void Samus_UpdateSpeedEchoPos(void);

/* ------------------------------------------------------------------ bola resorte ($90:A69F-$90:A733) */
static void Samus_Movement_11_SpringBallOnGround(void) {  // 0x90A69F
  if (!samus_x_accel_mode
      && ((samus_pose_x_dir == 4) ? samus_pose == kPose_7A_FaceL_Springball_Ground : samus_pose == kPose_79_FaceR_Springball_Ground)) {
    Samus_Move_NoBaseSpeed_X();
    if (!(Samus_CheckAndMoveY() & 1)) {
      Samus_Move_NoSpeedCalc_Y();
      Samus_CancelSpeedBoost();
      samus_x_extra_run_speed = 0;
      samus_x_extra_run_subspeed = 0;
      samus_x_base_speed = 0;
      samus_x_base_subspeed = 0;
      samus_x_accel_mode = 0;
    }
    return;
  }
  Samus_HandleMovement_X();
  if (!(Samus_CheckAndMoveY() & 1))
    Samus_Move_NoSpeedCalc_Y();
}

static void Samus_Movement_12_SpringBallInAir(void) {  // 0x90A6F1
  if (used_for_ball_bounce_on_landing)
    Samus_MorphedBouncingMovement();
  else
    Samus_JumpingMovement();
}

static void Samus_Movement_13_SpringBallFalling(void) {  // 0x90A703
  if (!(joypad1_lastkeys & (kButton_Left | kButton_Right)) && !samus_x_accel_mode) {
    Samus_CancelSpeedBoost();
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_accel_mode = 0;
  }
  if (used_for_ball_bounce_on_landing)
    Samus_MorphedBouncingMovement();
  else
    Samus_MorphedFallingMovement();
}

void Samus_MovementHandler_Normal(void) {  // 0x90A337
  if (time_is_frozen_flag)
    return;
  switch (samus_movement_type) {
  case 0: Samus_Movement_00_Standing(); break;
  case 1: Samus_Movement_01_Running(); break;
  case 2: Samus_JumpingMovement(); break;
  case 3: Samus_Movement_03_SpinJumping(); break;
  case 4: Samus_Movement_04_MorphBallOnGround(); break;
  case 5: Samus_Movement_05_Crouching(); break;
  case 6: Samus_Movement_06_Falling(); break;
  case 7: case 11: input_to_pose_calc = 0; break;
  case 8: Samus_Movement_08_MorphBallFalling(); break;
  case 9: break;
  case 10: input_to_pose_calc = 0; Samus_Move_NoSpeedCalc_Y(); break;   // _0A_KnockbackOrCrystalFlashEnding
  case 14: Samus_Movement_0E_TurningAroundOnGround(); break;
  case 15: Samus_Movement_0F_CrouchingEtcTransition(); break;
  case 16: Samus_Movement_10_Moonwalking(); break;
  case 20: Samus_Movement_14_WallJumping(); break;
  case 21: Samus_Movement_15_RanIntoWall(); break;
  case 23: case 24: Samus_Movement_TurningAroundInAir(); break;
  case 12: input_to_pose_calc = 0; break;                              // _0C_Unused
  case 13: break;                                                      // _0D_Unused
  case 17: Samus_Movement_11_SpringBallOnGround(); break;
  case 18: Samus_Movement_12_SpringBallInAir(); break;
  case 19: Samus_Movement_13_SpringBallFalling(); break;
  case 22: if (input_to_pose_calc != 5) input_to_pose_calc = 0; break; // _16_Grappling ($90:A780)
  case 25: Samus_JumpingMovement(); break;                             // _19_DamageBoost
  case 26: case 27: input_to_pose_calc = 0; break;                     // _1A_GrabbedByDraygon, _1B_ShinesparkEtc
  default: nt_unimpl("Samus_MovementHandler_Normal (tipo de movimiento sin portar)"); break;
  }
  Samus_UpdateSpeedEchoPos();
}

static void Samus_MoveHandler_Knockback(void);
static void Samus_MoveHandler_BombJumpStart(void);
static void Samus_MoveHandler_BombJumpMain(void);

/* ------------------------------------------------------------------ shinespark ($90:CFFA-$90:D4D1) */
void Projectile_Func7_Shinespark(void) {  // 0x90CFFA
  samus_movement_handler = FUNC16(Samus_MoveHandlerShinesparkWindup);
  samus_y_dir = 1;
  speed_boost_counter = 1024;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  knockback_dir = 0;
  samus_x_extra_run_speed = 8;
  samus_x_extra_run_subspeed = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  substate = 7;
  suit_pickup_light_beam_pos = 0;
  cooldown_timer = 0;
  timer_for_shinesparks_startstop = 30;
  samus_shine_timer = 60;
  timer_for_shine_timer = 6;
  special_samus_palette_frame = 0;
  bomb_jump_dir = 0;
  if (flare_counter) {
    if (!sign16(flare_counter - 16))
      QueueSfx1_Max9(2);
    flare_counter = 0;
    ClearFlareAnimationState();
  }
}

static void Samus_MoveHandlerShinesparkWindup(void) {  // 0x90D068
  if (!--timer_for_shinesparks_startstop || (timer_for_shinesparks_startstop & 0x8000)) {
    samus_new_pose_interrupted = (samus_pose_x_dir == 4) ? 204 : 203;
    samus_movement_handler = FUNC16(Samus_MoveHandlerVerticalShinespark);
    samus_input_handler = FUNC16(nullsub_152);
    speed_echoes_index = 0;
    speed_echo_xspeed[0] = 0;
    speed_echo_xspeed[1] = 0;
    speed_echo_xpos[0] = 0;
    speed_echo_xpos[1] = 0;
    QueueSfx3_Max9(0xF);
  }
}

static int32 Samus_ClampSpeedHi(int32 amt, int val) {
  if (!sign16((amt >> 16) - val))
    amt = (uint32)val << 16 | (amt & 0xffff);
  return amt;
}

static void Samus_ShinesparkMove_X(void) {  // 0x90D132
  samus_shine_timer = 15;
  add_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, IPAIR32(samus_y_accel, samus_y_subaccel));
  if (!sign16(samus_x_extra_run_speed - 15))
    set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 15 << 16);
  int32 amt;
  CheckEnemyColl_Result cres;
  if (samus_pose_x_dir == 4) {
    amt = Samus_ClampSpeedHi(-Samus_CalcDisplacementMoveLeft(0), 15);
    cres = Samus_CheckSolidEnemyColl(amt);
    amt = cres.amt;
    if (cres.collision) {
      samus_collision_flag = cres.collision;
      goto clamp;
    }
    amt = -amt;
  } else {
    amt = Samus_ClampSpeedHi(Samus_CalcDisplacementMoveRight(0), 25);   /* (sic: 25 a la derecha) */
    cres = Samus_CheckSolidEnemyColl(amt);
    amt = cres.amt;
    if (cres.collision) {
      samus_collision_flag = cres.collision;
      goto clamp;
    }
  }
  Samus_MoveRight_NoSolidColl(amt);
  Samus_AlignYPosSlope();
clamp:;
  int16 v4 = samus_x_pos - samus_prev_x_pos;
  if (v4 < 0) {
    if (sign16(v4 + 15))
      samus_prev_x_pos = samus_x_pos + 15;
  } else if (!sign16(v4 - 16)) {
    samus_prev_x_pos = samus_x_pos - 15;
  }
}

static void Samus_ShinesparkMove_Y(void) {  // 0x90D1FF
  samus_shine_timer = 15;
  /* substate:suit_pickup_light_beam_pos es la aceleración del vuelo vertical */
  add_hilo(&substate, &suit_pickup_light_beam_pos, IPAIR32(samus_y_accel, samus_y_subaccel));
  add_hilo(&samus_y_speed, &samus_y_subspeed, IPAIR32(substate, suit_pickup_light_beam_pos));
  int32 amt = Samus_ClampSpeedHi(IPAIR32(samus_y_speed, samus_y_subspeed), 14);
  amt -= IPAIR32(extra_samus_y_displacement, extra_samus_y_subdisplacement);
  samus_collision_direction = 2;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(Samus_ClampSpeedHi(amt, 15));
  amt = cres.amt;
  if (cres.collision)
    samus_collision_flag = cres.collision;
  else
    Samus_MoveDown_NoSolidColl(-amt);
  if (sign16(samus_y_pos - samus_prev_y_pos + 14))
    samus_prev_y_pos = samus_y_pos + 14;
}

static uint8 Samus_EndSuperJump(void) {  // 0x90D2BA
  if (!sign16(samus_health - 30) && !samus_collision_flag)
    return 0;
  if (samus_pose_x_dir == 4) {
    speed_echo_xspeed[0] = 32;
    speed_echo_xspeed[1] = 160;
    speed_echo_xpos[2] = 4;
  } else {
    speed_echo_xspeed[0] = 224;
    speed_echo_xspeed[1] = 96;
    speed_echo_xpos[2] = -4;
  }
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  speed_boost_counter = 0;
  UNUSED_word_7E0B1A = 0;
  samus_y_dir = 0;
  samus_movement_handler = FUNC16(Samus_MoveHandler_ShinesparkCrash);
  samus_draw_handler = FUNC16(Samus_DrawHandler_EndOfShinespark);
  speed_echoes_index = 0;
  speed_echo_xpos[0] = samus_x_pos;
  speed_echo_xpos[1] = samus_x_pos;
  speed_echo_ypos[0] = samus_y_pos;
  speed_echo_ypos[1] = samus_y_pos;
  speed_echo_xspeed[2] = 0;
  samus_hurt_flash_counter = 0;
  QueueSfx1_Max6(0x35);
  QueueSfx3_Max6(0x10);
  return 1;
}

static void Samus_ShinesparkDrain(void) {   /* el final común de los tres vuelos */
  Samus_EndSuperJump();
  if (!sign16(samus_health - 30) && (--samus_health & 0x8000))
    samus_health = 0;
}

static void Samus_MoveHandlerVerticalShinespark(void) {  // 0x90D0AB
  samus_contact_damage_index = 2;
  samus_hurt_flash_counter = 8;
  Samus_UpdateSpeedEchoPos();
  Samus_ShinesparkMove_Y();
  Samus_ShinesparkDrain();
}

static void Samus_MoveHandler_Shinespark_Diag(void) {  // 0x90D0D7
  samus_contact_damage_index = 2;
  samus_hurt_flash_counter = 8;
  Samus_UpdateSpeedEchoPos();
  Samus_ShinesparkMove_X();
  Samus_ShinesparkMove_Y();
  Samus_ShinesparkDrain();
}

static void Samus_MoveHandler_Shinespark_Horiz(void) {  // 0x90D106
  samus_contact_damage_index = 2;
  samus_hurt_flash_counter = 8;
  Samus_UpdateSpeedEchoPos();
  Samus_ShinesparkMove_X();
  Samus_ShinesparkDrain();
}

static uint16 Projectile_SinLookup_Inner(uint16 k, uint16 r24) {  // 0x90CC8A
  /* bytes de kSinCosTable8bit_Sext desde el elemento 64 (palabras little-endian en el SNES) */
  uint16 w = kSinCosTable8bit_Sext[64 + (k >> 1)];
  uint8 b0 = (k & 1) ? HI8(w) : LO8(w);
  uint8 b1 = (k & 1) ? LO8(kSinCosTable8bit_Sext[64 + (k >> 1) + 1]) : HI8(w);
  return ((uint16)(b0 * (uint8)r24) >> 8) + (uint16)(b1 * (uint8)r24);
}

Point16U Projectile_SinLookup(uint16 j, uint16 a) {  // 0x90CC39
  uint16 v2, v4;
  if (sign16(j - 128))
    v2 = Projectile_SinLookup_Inner(2 * j, a);
  else
    v2 = -Projectile_SinLookup_Inner(2 * (uint8)(j + 0x80), a);
  int16 v3 = (uint8)(j - 64);
  if (sign16(v3 - 128))
    v4 = Projectile_SinLookup_Inner(2 * v3, a);
  else
    v4 = -Projectile_SinLookup_Inner(2 * (uint8)(v3 + 0x80), a);
  return (Point16U){ v2, v4 };
}

static void Samus_MoveHandler_ShinesparkCrash(void) {  // 0x90D346
  samus_shine_timer = 15;
  switch (HI8(speed_echoes_index)) {
  case 0: {                                     // _0 ($90:D383): los ecos se separan
    uint16 v0 = speed_echoes_index + 4;
    if (!sign16(speed_echoes_index - 12))
      v0 |= 0x100;
    speed_echoes_index = v0;
    break;
  }
  case 1:                                       // _1 ($90:D396): giran
    speed_echo_xspeed[0] = (uint8)(LO8(speed_echo_xpos[2]) + LO8(speed_echo_xspeed[0]));
    speed_echo_xspeed[1] = (uint8)(LO8(speed_echo_xpos[2]) + LO8(speed_echo_xspeed[1]));
    speed_echo_ypos[2] += 4;
    if (!sign16(speed_echo_ypos[2] - 128))
      speed_echoes_index = (uint8)speed_echoes_index | 0x200;
    break;
  case 2:                                       // _2 ($90:D3CC): vuelven
    speed_echoes_index -= 4;
    if (!(uint8)speed_echoes_index) {
      samus_movement_handler = FUNC16(Samus_MoveHandler_ShinesparkCrashEchoCircle);
      timer_for_shinesparks_startstop = 30;
      speed_echoes_index = 0;
      speed_echo_xspeed[0] = 0;
      speed_echo_xspeed[1] = 0;
    }
    break;
  default: nt_unimpl("Samus_MoveHandler_ShinesparkCrash (índice)"); break;
  }
  for (int i = 1; i >= 0; i--) {
    Point16U pt = Projectile_SinLookup(speed_echo_xspeed[i], (uint8)speed_echoes_index);
    speed_echo_xpos[i] = pt.x + samus_x_pos;
    speed_echo_ypos[i] = pt.y + samus_y_pos;
  }
}

static void Samus_MoveHandler_ShinesparkCrashEchoCircle(void) {  // 0x90D3F3
  samus_shine_timer = 15;
  if (!--timer_for_shinesparks_startstop || (timer_for_shinesparks_startstop & 0x8000)) {
    samus_movement_handler = FUNC16(Samus_MoveHandler_ShinesparkCrashFinish);
    samus_draw_handler = FUNC16(SamusDrawHandler_Default);
  }
}

static void Samus_MoveHandler_ShinesparkCrashFinish(void) {  // 0x90D40D
  static const uint8 kShinesparkCrashFinish_Tab0[12] = { 0, 0x80, 0, 0x80, 0x40, 0xc0, 0x40, 0xc0, 0xe0, 0x60, 0x20, 0xa0 };
  speed_echoes_index = 0;
  if (sign16(projectile_counter - 5)) {         /* los dos ecos que salen volando son proyectiles 3 y 4 */
    if (sign16(projectile_counter - 4)) {
      ++projectile_counter;
      speed_echo_xspeed[2] = 64;
      speed_echo_xpos[2] = samus_x_pos;
      speed_echo_ypos[2] = samus_y_pos;
      projectile_type[3] = addr_loc_908029;
      InitializeShinesparkEchoOrSpazerSba(6);
      projectile_bomb_pre_instructions[3] = FUNC16(ProjPreInstr_SpeedEcho);
      projectile_variables[3] = kShinesparkCrashFinish_Tab0[(uint16)(2 * (samus_pose - 201))];
      projectile_bomb_x_speed[3] = 0;
    }
    ++projectile_counter;
    speed_echo_xspeed[3] = 64;
    speed_echo_xpos[3] = samus_x_pos;
    speed_echo_ypos[3] = samus_y_pos;
    projectile_type[4] = addr_loc_908029;
    InitializeShinesparkEchoOrSpazerSba(8);
    projectile_bomb_pre_instructions[4] = FUNC16(ProjPreInstr_SpeedEcho);
    projectile_variables[4] = kShinesparkCrashFinish_Tab0[(uint16)(2 * (samus_pose - 201)) + 1];
    projectile_bomb_x_speed[4] = 0;
  }
  cooldown_timer = 0;
  samus_shine_timer = 1;
  samus_new_pose_transitional = (samus_pose_x_dir == 4) ? 2 : 1;
  samus_hurt_switch_index = 2;
  substate = 0;
  suit_pickup_light_beam_pos = 0;
}

void RunSamusMovementHandler(void) {  // 0x90E94B
  switch (samus_movement_handler) {
  case FUNC16(Samus_MovementHandler_Normal): Samus_MovementHandler_Normal(); break;
  case FUNC16(nullsub_152): break;
  case FUNC16(Samus_MoveHandler_Knockback): Samus_MoveHandler_Knockback(); break;
  case FUNC16(Samus_MoveHandler_BombJumpStart): Samus_MoveHandler_BombJumpStart(); break;
  case FUNC16(Samus_MoveHandler_BombJumpMain): Samus_MoveHandler_BombJumpMain(); break;
  case FUNC16(Samus_MoveHandlerShinesparkWindup): Samus_MoveHandlerShinesparkWindup(); break;
  case FUNC16(Samus_MoveHandlerVerticalShinespark): Samus_MoveHandlerVerticalShinespark(); break;
  case FUNC16(Samus_MoveHandler_Shinespark_Diag): Samus_MoveHandler_Shinespark_Diag(); break;
  case FUNC16(Samus_MoveHandler_Shinespark_Horiz): Samus_MoveHandler_Shinespark_Horiz(); break;
  case FUNC16(Samus_MoveHandler_ShinesparkCrash): Samus_MoveHandler_ShinesparkCrash(); break;
  case FUNC16(Samus_MoveHandler_ShinesparkCrashEchoCircle): Samus_MoveHandler_ShinesparkCrashEchoCircle(); break;
  case FUNC16(Samus_MoveHandler_ShinesparkCrashFinish): Samus_MoveHandler_ShinesparkCrashFinish(); break;
  default: nt_unimpl_v("RunSamusMovementHandler", samus_movement_handler); break;
  }
}

/* ------------------------------------------------------------------ minimapa del HUD ($90:A91B)
 * Igual que el SNES: marca lo explorado y arma las 3 filas de 5 celdas del minimapa en hud_tilemap con el mapa
 * del área (preparado en el PC: kAreaMapData, kAreaMapTilemaps). La Genesis copia
 * hud_tilemap a su plano. */
static const uint8 kShr0x80[8] = { 0x80, 0x40, 0x20, 0x10, 8, 4, 2, 1 };
static const uint16 kShr0xFc00[8] = { 0xfc00, 0x7e00, 0x3f00, 0x1f80, 0xfc0, 0x7e0, 0x3f0, 0x1f8 };

static inline uint16 mte16(uint16 i) { return (uint16)(map_tiles_explored[i & 0xFF] << 8 | map_tiles_explored[(i + 1) & 0xFF]); }

static void UpdateMinimapInside(uint16 r18, uint16 r22, uint16 r34, uint16 r30, uint16 r32,
                                uint16 r38, uint16 r24, uint16 r40, uint16 r26, uint16 r42, uint16 r28) {  // 0x90AA43
  uint16 v0 = (uint16)(((uint16)(r34 + r22) >> 8) | (uint16)((uint8)(r34 + r22) << 8));
  uint16 t = r18 + (v0 >> 3);
  int16 v1 = (r34 && sign16((t & 0x1F) - 2)) ? t - 1026 : t - 34;
  uint16 v2 = 2 * v1;
  const uint16 *r0 = kAreaMapTilemaps + 2048 * area_index, *r3 = r0 + 32, *r6 = r0 + 64;
  #define MAPW(p, i) ((uint16)((i) >> 1) < 2048 - ((p) - r0) ? (p)[(uint16)(i) >> 1] : 0)
  int n = 5;
  uint16 v3 = 0;
  do {
    int v6 = v3 >> 1;
    bool b = r38 >> 15; r38 <<= 1;
    uint16 v5 = (!b || !has_area_map) ? 31 : MAPW(r0, v2);
    hud_tilemap[v6 + 26] = (v5 & 0xC3FF) | 0x2C00;
    b = r24 >> 15; r24 <<= 1;
    if (b) hud_tilemap[v6 + 26] = (MAPW(r0, v2) & 0xC3FF) | 0x2800;
    b = r40 >> 15; r40 <<= 1;
    uint16 v7 = (!b || !has_area_map) ? 31 : MAPW(r3, v2);
    hud_tilemap[v6 + 58] = (v7 & 0xC3FF) | 0x2C00;
    b = r26 >> 15; r26 <<= 1;
    if (b) {
      hud_tilemap[v6 + 58] = (MAPW(r3, v2) & 0xC3FF) | 0x2800;
      if (n == 3 && (hud_tilemap[v6 + 58] & 0x1FF) == 40) {
        /* MarkMapTileAboveSamusAsExplored: el original suma a music_data_index (7E:07F3), 4 bytes antes del mapa */
        if (r30 >= 4) map_tiles_explored[r30 - 4] |= kShr0x80[r32];
        else nt_unimpl("MarkMapTileAboveSamusAsExplored (fuera del mapa)");
      }
    }
    b = r42 >> 15; r42 <<= 1;
    uint16 v8 = (!b || !has_area_map) ? 31 : MAPW(r6, v2);
    hud_tilemap[v6 + 90] = (v8 & 0xC3FF) | 0x2C00;
    b = r28 >> 15; r28 <<= 1;
    if (b) hud_tilemap[v6 + 90] = (MAPW(r6, v2) & 0xC3FF) | 0x2800;
    v3 += 2;
    v2 += 2;
    if (!(v2 & 0x3F))
      v2 += 1984;
  } while (--n);
  #undef MAPW
  if (!(nmi_frame_counter_byte & 8))
    hud_tilemap[60] |= 0x1C00;
}

static void UpdateMinimapFull(void);

/* El SNES recalcula el minimapa en cada cuadro. El resultado solo depende de la celda del mapa en que está Samus,
   de has_area_map, del parpadeo (nmi_frame_counter_byte & 8) y de map_tiles_explored (que solo cambia aquí, y de
   forma que repetirlo no cambia nada). Si nada de eso cambió y las 15 celdas del HUD siguen como quedaron, el
   resultado sería el mismo: la Genesis se ahorra el cálculo. */
static uint16 mm_key[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
static uint16 mm_cells[16];
static const uint8 kMinimapCells[16] = { 26, 27, 28, 29, 30, 58, 59, 60, 61, 62, 90, 91, 92, 93, 94, 60 };

void UpdateMinimap(void) {  // 0x90A91B
  if (debug_disable_minimap || (samus_x_pos >> 4) >= room_width_in_blocks || (samus_y_pos >> 4) >= room_height_in_blocks)
    return;
  uint16 k0 = (uint16)(room_x_coordinate_on_map + HI8(samus_x_pos)), k1 = (uint16)(room_y_coordinate_on_map + HI8(samus_y_pos));
  uint16 k2 = has_area_map, k3 = nmi_frame_counter_byte & 8;
  if (k0 == mm_key[0] && k1 == mm_key[1] && k2 == mm_key[2] && k3 == mm_key[3]) {
    int same = 1;
    for (int i = 0; i < 15 && same; i++) same = hud_tilemap[kMinimapCells[i]] == mm_cells[i];
    if (same) return;
  }
  UpdateMinimapFull();
  mm_key[0] = k0; mm_key[1] = k1; mm_key[2] = k2; mm_key[3] = k3;
  for (int i = 0; i < 15; i++) mm_cells[i] = hud_tilemap[kMinimapCells[i]];
}

static void UpdateMinimapFull(void) {
  if (area_index > 6) { nt_unimpl("UpdateMinimap (área de depuración)"); return; }
  uint16 r46 = 0;
  uint8 v0 = HI8(samus_x_pos);
  uint16 r34 = (room_x_coordinate_on_map + v0) & 0x20;
  uint16 r18 = (room_x_coordinate_on_map + v0) & 0x1F;
  uint16 v1 = (room_x_coordinate_on_map + v0) & 7;
  uint16 r20 = ((room_x_coordinate_on_map + v0) & 0x1F) >> 3;
  uint16 r22 = room_y_coordinate_on_map + HI8(samus_y_pos) + 1;
  uint16 v2 = r20 + 4 * (r34 + r22);
  map_tiles_explored[v2 & 0xFF] |= kShr0x80[v1];
  uint16 r32 = v1, r30 = v2;
  uint16 v3 = v2 - 4;
  int16 v4 = v1 - 2;
  if (v4 < 0) {
    v4 &= 7;
    --v3;
    ++r46;
  }
  uint16 R50 = v3;
  int r52 = 2 * v4;
  int v6 = r52 >> 1;
  uint16 r24 = kShr0xFc00[v6] & mte16(v3);
  uint16 r26 = kShr0xFc00[v6] & mte16(v3 + 4);
  uint16 r28 = kShr0xFc00[v6] & mte16(v3 + 8);
  const uint8 *r15 = kAreaMapData + 256 * area_index;
  #define MD16(i) ((uint16)(r15[(i) & 0xFF] << 8 | r15[((i) + 1) & 0xFF]))
  uint16 r38 = MD16(v3), r40 = MD16(v3 + 4), r42 = MD16(v3 + 8);
  #undef MD16
  if ((R50 & 3) == 3) {
    int16 v10 = r46 ? r52 >> 1 : r32;
    if (!sign16(v10 - 6)) {
      uint8 R48 = r34 ? (uint8)(R50 - 124) : (uint8)(R50 + 125);
      for (int row = 0; row < 3; row++) {
        uint8 lo = map_tiles_explored[(uint8)(R48 + 4 * row)];
        uint8 hi = r15[(uint8)(R48 + 4 * row)];
        uint16 *rm = row == 0 ? &r38 : row == 1 ? &r40 : &r42;
        uint16 *re = row == 0 ? &r24 : row == 1 ? &r26 : &r28;
        if ((uint8)r34 == 32) {
          *rm = (*rm & 0x00FF) | hi << 8;
          *re = (*re & 0x00FF) | lo << 8;
        } else {
          *rm = (*rm & 0xFF00) | hi;
          *re = (*re & 0xFF00) | lo;
        }
      }
    }
  }
  for (int n = r52 >> 1; n; n--) {
    r24 <<= 1; r38 <<= 1; r26 <<= 1; r40 <<= 1; r28 <<= 1; r42 <<= 1;
  }
  UpdateMinimapInside(r18, r22, r34, r30, r32, r38, r24, r40, r26, r42, r28);
}

/* ------------------------------------------------------------------ disparo ($90:AC1C) */
void Samus_HandleCooldown(void) {  // 0x90AC1C
  if (time_is_frozen_flag) {
    cooldown_timer = 32;
  } else if (cooldown_timer) {
    if ((cooldown_timer & 0x8000) || (--cooldown_timer, (cooldown_timer & 0x8000)))
      cooldown_timer = 0;
  }
}

void Samus_HandleHudSpecificBehaviorAndProjs(void) {  // 0x90DCF2
  if (samus_pose != kPose_00_FaceF_Powersuit && samus_pose != kPose_9B_FaceF_VariaGravitySuit) {
    Samus_HandleCooldown();
    HandleSwitchingHudSelection();
    switch (samus_movement_type) {
    case 0: case 1: case 2: case 5: case 6: case 16: case 21: HudSelectionHandler_Normal(); break;
    case 3: case 10: case 13: case 20: case 25: case 27: HudSelectionHandler_JumpEtc(); break;
    case 14: case 23: case 24: HudSelectionHandler_TurningAround(); break;
    case 15: HudSelectionHandler_CrouchEtcTrans(); break;
    case 4: case 7: case 8: case 9: case 17: case 18: case 19: HudSelectionHandler_MorphBall(); break;
    default: nt_unimpl("kHandleHudSpecificBehaviorAndProjs"); break;
    }
    if (time_is_frozen_flag)
      return;
  }
  HandleProjectile();
}

/* ------------------------------------------------------------------ golpes ($90:DE00) */
/* kSamusHitInterrupts ($90:DDE9) por tipo de movimiento: 0 Stand, 1 Ball, 2 Falling, 3 Unused, 4 KnockbackGrapple,
   5 Turning, 6 Shinespark */
static const uint8 kSamusHitInterruptKind[28] = {
  0, 0, 0, 0, 1, 0, 2, 3, 1, 1, 4, 5, 5, 0, 5, 5, 0, 1, 1, 1, 0, 0, 4, 5, 5, 5, 5, 6,
};

static uint8 Samus_HitInterrupt_Stand(void) {  // 0x90DEFA
  samus_new_pose_interrupted = (samus_pose_x_dir == 4) ? 84 : 83;
  return 1;
}

static uint8 Samus_HitInterrupt(void) {
  switch (kSamusHitInterruptKind[samus_movement_type]) {
  case 0: return Samus_HitInterrupt_Stand();
  case 1: samus_new_pose_interrupted = samus_pose; return 1;               // Samus_HitInterrupt_Ball
  case 2:                                                                   // Samus_HitInterrupt_Falling
    if (frame_handler_gamma != FUNC16(Samus_Func9))
      return Samus_HitInterrupt_Stand();
    samus_special_transgfx_index = 0;
    knockback_dir = 0;
    return 0;
  case 3: samus_new_pose_interrupted = (samus_pose_x_dir == 4) ? 52 : 51; return 1;   // Samus_HitInterrupt_Unused
  case 4: samus_special_transgfx_index = 0; return 0;                      // Samus_HitInterrupt_KnockbackGrapple
  case 5: samus_special_transgfx_index = 0; knockback_dir = 0; return 0;   // Samus_HitInterrupt_Turning
  default:                                                                  // Samus_HitInterrupt_Shinespark
    if (samus_pose == kPose_E8_FaceR_Drained_CrouchFalling || samus_pose == kPose_E9_FaceL_Drained_CrouchFalling) {
      samus_anim_frame_timer = 17;
      samus_anim_frame = 26;
    }
    samus_special_transgfx_index = 0;
    knockback_dir = 0;
    return 0;
  }
}

/* ------------------------------------------------------------------ salto con bomba ($90:DF99) */
static uint8 SetupBombJump_1(void) {  // 0x90DFF7
  samus_new_pose_interrupted = (samus_pose_x_dir == 4) ? kPose_52_FaceL_Jump_NoAim_MoveF : kPose_51_FaceR_Jump_NoAim_MoveF;
  return 1;
}
static uint8 SetupBombJump_StandCrouch(void) {  // 0x90DFED
  if (!time_is_frozen_flag)
    return SetupBombJump_1();
  bomb_jump_dir = 0;
  return 0;
}
static uint8 SetupBombJump_4(void) {  // 0x90E012
  samus_new_pose_interrupted = samus_pose;
  return 1;
}
static uint8 SetupBombJump_1A(void) {  // 0x90E020
  bomb_jump_dir = 0;
  return 0;
}
static uint8 SetupBombJump_2(void) {  // 0x90E01A
  samus_input_handler = FUNC16(Samus_InputHandler_E913);
  return SetupBombJump_1A();
}
static uint8 (*const kSetupBombJump[28])(void) = {  // 0x90DF99
  SetupBombJump_StandCrouch, SetupBombJump_1, SetupBombJump_2, SetupBombJump_2, SetupBombJump_4,
  SetupBombJump_StandCrouch, SetupBombJump_1, SetupBombJump_4, SetupBombJump_4, SetupBombJump_4,
  SetupBombJump_4, SetupBombJump_1, SetupBombJump_1, SetupBombJump_1, SetupBombJump_2,
  SetupBombJump_2, SetupBombJump_1, SetupBombJump_4, SetupBombJump_4, SetupBombJump_4,
  SetupBombJump_1, SetupBombJump_1, SetupBombJump_1, SetupBombJump_2, SetupBombJump_2,
  SetupBombJump_2, SetupBombJump_1A, SetupBombJump_1A,
};
static void SetupBombJump(void) {
  if (!HI8(bomb_jump_dir)) {
    if (kSetupBombJump[samus_movement_type]() & 1)
      samus_special_transgfx_index = 3;
  }
}

void Samus_HitInterruption(void) {  // 0x90DD8C
  if (samus_knockback_timer) {
    if (sign16(debug_invincibility - 7)) {
      if (!time_is_frozen_flag && !knockback_dir) {
        if (Samus_HitInterrupt() & 1)
          samus_special_transgfx_index = 1;
      }
    } else {
      samus_invincibility_timer = 0;
      samus_knockback_timer = 0;
    }
  } else if (knockback_dir) {
    if (samus_movement_type == kPose_0A_MoveL_NoAim) {
      if (!sign16(flare_counter - 16))
        QueueSfx1_Max6(0x41);
      samus_new_pose_transitional = (samus_pose_x_dir == 4) ? kPose_2A_FaceL_Fall : kPose_29_FaceR_Fall;
    } else {
      if (samus_hurt_switch_index == 3) {
        samus_hurt_switch_index = 8;
        return;
      }
      samus_new_pose_transitional = samus_pose;
    }
    samus_hurt_switch_index = 1;
  } else if (bomb_jump_dir) {
    SetupBombJump();
  }
}

/* ------------------------------------------------------------------ retroceso ($90:DF38) */
static void Samus_BombJumpRisingXMovement_(void) {  // 0x90E0E6 (rama del retroceso)
  int32 amt;
  if (knockback_dir) {
    amt = Samus_CalcBaseSpeed_X(Samus_DetermineSpeedTableEntryPtr_X());
    amt = knockback_x_dir ? Samus_CalcDisplacementMoveRight(amt) : Samus_CalcDisplacementMoveLeft(amt);
  } else {
    amt = Samus_CalcBaseSpeed_X(0x9F25);                              // addr_stru_909F25
    amt = ((uint8)bomb_jump_dir != 1) ? Samus_CalcDisplacementMoveRight(amt) : Samus_CalcDisplacementMoveLeft(amt);
  }
  if (amt < 0)
    Samus_MoveLeft(amt);
  else
    Samus_MoveRight(amt);
}

static void Samus_ClearMoveVars(void) {  // 0x90DF6E
  if (samus_collision_flag) {
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
    Samus_AlignBottomWithPrevPose();
  }
}

static void Samus_MoveHandler_Knockback(void) {  // 0x90DF38
  switch (knockback_dir) {
  case 1: case 2:                                                     // _Up
    Samus_BombJumpRisingXMovement_();
    Samus_MoveY_WithSpeedCalc();
    Samus_ClearMoveVars();
    break;
  case 3:                                                             // _3
    Samus_MoveY_WithSpeedCalc();
    Samus_ClearMoveVars();
    break;
  case 4: case 5:                                                     // _Down
    Samus_BombJumpRisingXMovement_();
    Samus_Move_NoSpeedCalc_Y();
    Samus_ClearMoveVars();
    break;
  default: nt_unimpl("Samus_MoveHandler_Knockback_0 (InvalidInterrupt_Crash)"); break;
  }
  input_to_pose_calc = 0;
}

static void Samus_BombJumpRisingYMovement_(void) {
  if (samus_y_dir == 1) {
    if (samus_y_speed & 0x8000) {
      samus_y_subspeed = 0;
      samus_y_speed = 0;
      samus_y_dir = 2;
      if ((uint8)bomb_jump_dir != 2)
        samus_x_accel_mode = 2;
    } else if (sign16(samus_y_speed - 1)) {
      if (samus_input_handler != FUNC16(Samus_InputHandler_E91D))
        samus_input_handler = FUNC16(Samus_InputHandler_E913);
    }
  }
}

static void Samus_InitBombJump(void) {  // 0x909A2C
  static const uint16 kSamus_InitBombJump_Speed[3] = { 2, 0, 0 };
  static const uint16 kSamus_InitBombJump_Subspeed[3] = { 0xc000, 0x1000, 0x1000 };
  uint16 v0 = 0;
  if (!(equipped_items & 0x20)) {
    uint16 r18 = Samus_GetBottom_R18();
    if (fx_y_pos & 0x8000) {
      if (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18))
        v0 = 4;
    } else if (sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) {
      v0 = 2;
    }
  }
  int v1 = v0 >> 1;
  samus_y_subspeed = kSamus_InitBombJump_Subspeed[v1];
  samus_y_speed = kSamus_InitBombJump_Speed[v1];
  grapple_walljump_timer = 0;
  reached_ceres_elevator_fade_timer = 0;
  samus_y_dir = 1;
}

static void Samus_MoveHandler_BombJumpFunc1(void) {  // 0x90E07D
  samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
  if (samus_input_handler != FUNC16(Samus_InputHandler_E91D))
    samus_input_handler = FUNC16(Samus_InputHandler_E913);
  bomb_jump_dir = 0;
}

static void Samus_HorizontalBombJump(void) {  // 0x90E04C
  Samus_BombJumpRisingXMovement_();
  Samus_BombJumpRisingYMovement_();
  if (samus_y_dir == 2 || (Samus_MoveY_WithSpeedCalc(), samus_collision_flag))
    Samus_MoveHandler_BombJumpFunc1();
}

static void Samus_VerticalBombJump(void) {  // 0x90E066
  Samus_BombJumpRisingYMovement_();
  if (samus_y_dir == 2 || (Samus_MoveY_WithSpeedCalc(), samus_collision_flag))
    Samus_MoveHandler_BombJumpFunc1();
}

static void Samus_MoveHandler_BombJumpMain(void) {  // 0x90E032
  if (bomb_jump_dir) {
    switch ((uint8)bomb_jump_dir) {
    case 1: case 3: Samus_HorizontalBombJump(); break;
    case 2: Samus_VerticalBombJump(); break;
    default: nt_unimpl("Samus_MoveHandler_BombJumpMain (Samus_MoveHandler_Knockback_0: InvalidInterrupt_Crash)"); break;
    }
  } else {
    Samus_MoveHandler_BombJumpFunc1();
  }
}

static void Samus_MoveHandler_BombJumpStart(void) {  // 0x90E025
  Samus_InitBombJump();
  samus_movement_handler = FUNC16(Samus_MoveHandler_BombJumpMain);
  input_to_pose_calc = 0;
}

/* ------------------------------------------------------------------ desplazamiento con límites ($90:E464) */
static int32 Samus_ClampSpeed(int32 amt) {
  uint16 r18 = (uint16)((uint32)amt >> 16), r20 = (uint16)amt;
  int16 v0 = r18;
  if (v0 < 0) {
    if (sign16(v0 + 15))
      r18 = -15;
  } else if (!sign16(v0 - 16)) {
    r18 = 15;
  }
  return (int32)PAIR32(r18, r20);
}

static uint32 Samus_CalcSpeed_X(uint32 amt) {  // 0x90E4E6
  amt += PAIR32(samus_x_extra_run_speed, samus_x_extra_run_subspeed);
  amt >>= (samus_x_speed_divisor <= 4) ? samus_x_speed_divisor : 4;
  set_hilo(&samus_total_x_speed, &samus_total_x_subspeed, amt);
  return amt;
}

int32 Samus_CalcDisplacementMoveLeft(int32 amt) {  // 0x90E464
  amt = Samus_CalcSpeed_X(amt);
  samus_collision_direction = 0;
  amt = IPAIR32(extra_samus_x_displacement, extra_samus_x_subdisplacement) - amt;
  return Samus_ClampSpeed(amt);
}

int32 Samus_CalcDisplacementMoveRight(int32 amt) {  // 0x90E4AD
  amt = Samus_CalcSpeed_X(amt);
  samus_collision_direction = 1;
  amt += IPAIR32(extra_samus_x_displacement, extra_samus_x_subdisplacement);
  return Samus_ClampSpeed(amt);
}

void Samus_ClearXSpeedIfColl(void) {  // 0x90E5CE
  if (samus_collision_flag) {
    samus_collides_with_solid_enemy = samus_collision_direction ? 8 : 4;
    Samus_CancelSpeedBoost();
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_accel_mode = 0;
  } else {
    input_to_pose_calc = 0;
    samus_collides_with_solid_enemy = 0;
  }
}

void Samus_MoveUp_SetPoseCalcInput(void) {  // 0x90E606
  input_to_pose_calc = samus_collision_flag ? 4 : 0;
}

void Samus_MoveDown_SetPoseCalcInput(void) {  // 0x90E61B
  static const uint8 kTab0[28] = { 0, 0, 4, 4, 1, 0, 4, 2, 4, 4, 0, 0, 0, 0, 4, 4, 0, 3, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4 };
  static const uint8 kTab1[28] = { 4, 4, 0, 0, 4, 4, 0, 4, 1, 2, 0, 4, 4, 0, 4, 4, 4, 4, 3, 3, 0, 4, 4, 4, 4, 0, 4, 4 };
  if (samus_collision_flag)
    input_to_pose_calc = 1 | kTab1[samus_movement_type] << 8;
  else if ((uint8)input_to_pose_calc != 5)
    input_to_pose_calc = 2 | kTab0[samus_movement_type] << 8;
}

/* ------------------------------------------------------------------ manejadores del cuadro ($90:E692) */
static void Samus_CallInputHandler(void) {  // 0x90E90F
  switch (samus_input_handler) {
  case FUNC16(nullsub_152): break;
  case FUNC16(Samus_InputHandler_E913): Samus_InputHandler(); break;
  case FUNC16(Samus_Func20_): Samus_Func20(); break;
  case FUNC16(HandleAutoJumpHack): {     // 0x90E926
    uint16 v0 = joypad1_newkeys;
    if (autojump_timer && sign16(autojump_timer - 9)) {
      joypad1_newkeys |= button_config_jump_a;
      autojump_timer = 0;
    }
    Samus_InputHandler();
    joypad1_newkeys = v0;
    samus_input_handler = FUNC16(Samus_InputHandler_E913);
    break;
  }
  case FUNC16(Samus_InputHandler_E91D):         // 0x90E91D: entrada de demo (intro)
    DemoObjectInputHandler();
    Samus_InputHandler();
    break;
  default: nt_unimpl_v("Samus_CallInputHandler", samus_input_handler); break;
  }
}

void HandleControllerInputForGamePhysics(void) {  // 0x90E692
  switch (frame_handler_alfa) {
  case FUNC16(Samus_FrameHandlerAlfa_Func11):   // 0x90E695
    samus_new_pose = 0xFFFF;
    samus_new_pose_interrupted = 0xFFFF;
    samus_new_pose_transitional = 0xFFFF;
    samus_momentum_routine_index = 0;
    samus_special_transgfx_index = 0;
    samus_hurt_switch_index = 0;
    Samus_SetRadius();
    Samus_CallInputHandler();
    Samus_UpdateSuitPaletteIndex();
    Samus_DetermineAccel_Y();
    BlockInsideDetection();
    Samus_HandleHudSpecificBehaviorAndProjs();
    Samus_Func10();
    break;
  case FUNC16(Samus_FrameHandlerAlfa_Func12):   // 0x90E6C9: intro (entrada de demo)
    samus_new_pose = 0xFFFF;
    samus_new_pose_interrupted = 0xFFFF;
    samus_new_pose_transitional = 0xFFFF;
    samus_momentum_routine_index = 0;
    samus_special_transgfx_index = 0;
    samus_hurt_switch_index = 0;
    controller1_input_for_demo = joypad1_lastkeys;
    controller1_new_input_for_demo = joypad1_newkeys;
    demo_backup_prev_controller_input = joypad1_input_samusfilter;
    demo_backup_prev_controller_input_new = joypad1_newinput_samusfilter;
    Samus_SetRadius();
    Samus_UpdateSuitPaletteIndex();
    Samus_CallInputHandler();
    Samus_DetermineAccel_Y();
    BlockInsideDetection();
    Samus_HandleHudSpecificBehaviorAndProjs();
    Samus_Func10();
    break;
  case FUNC16(Samus_FrameHandlerAlfa_Func13):   // 0x90E713
    HandleProjectile();
    Samus_Func10();
    break;
  case FUNC16(EmptyFunction):
    break;
  default: nt_unimpl_v("HandleControllerInputForGamePhysics", frame_handler_alfa); break;
  }
}

/* ------------------------------------------------------------------ Ridley de Ceres empuja a Samus al irse ($90:E119) */
void Samus_SetPushedOutOfCeresRidley(void) {  // 0x90E119
  samus_movement_handler = FUNC16(nullsub_152);
  frame_handler_gamma = FUNC16(Samus_PushOutOfRidleysWay);
}

/* ------------------------------------------------------------------ contador de la huida ($80:9DE7-A05F) */
void DrawTimerSpritemap(uint16 sm, uint16 x, uint16 y);   /* capa de la Genesis: DrawSpritemap($80, sm, x, y, 0x0A00) */

static void ClearTimerRam(void) {  // 0x809E93
  timer_x_pos = 0x8000;
  timer_y_pos = 0x8000;
  timer_centiseconds = timer_seconds = timer_minutes = 0;
  timer_status = 0;
}

static uint8 DecrementDecimal(uint8 *number, uint8 value) {   /* SBC decimal del 65816; 1 si no hubo préstamo */
  int result = (*number & 0xf) + (~value & 0xf) + 1;
  if (result < 0x10) result = (result - 0x6) & ((result - 0x6 < 0) ? 0xf : 0x1f);
  result = (*number & 0xf0) + (~value & 0xf0) + result;
  if (result < 0x100) result -= 0x60;
  *number = result;
  return result < 0x100;
}

static uint8 ProcessTimer_Decrement(void) {  // 0x809EA9
  static const uint8 kTimerCentisecondDecrements[128] = {
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 2, 1, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 2, 1, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 2, 1, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2,
  };
  if (DecrementDecimal(&timer_centiseconds, kTimerCentisecondDecrements[nmi_frame_counter_word & 0x7f])) {
    if (DecrementDecimal(&timer_seconds, 1)) {
      if (DecrementDecimal(&timer_minutes, 1)) {
        timer_centiseconds = timer_seconds = timer_minutes = 0;
      } else {
        timer_seconds = 0x59;
      }
    }
  }
  return (timer_centiseconds | timer_seconds | timer_minutes) == 0;
}

uint8 ProcessTimer(void) {  // 0x809DE7
  switch ((uint8)timer_status) {
  case 0: return 0;                                                   // ProcessTimer_Empty
  case 1:                                                             // ProcessTimer_CeresStart
    ClearTimerRam();
    timer_centiseconds = 0; timer_seconds = 0; timer_minutes = 1;     // SetTimerMinutes(0x100)
    timer_status = 0x8003;
    return 0;
  case 3:                                                             // ProcessTimer_InitialDelay
    timer_x_pos = (timer_x_pos & 0xFF00) | (uint8)(timer_x_pos + 1);
    if ((uint8)timer_x_pos >= 0x10)
      timer_status = (timer_status & 0xFF00) | (uint8)(timer_status + 1);
    return 0;
  case 4:                                                             // ProcessTimer_MovementDelayed
    timer_x_pos = (timer_x_pos & 0xFF00) | (uint8)(timer_x_pos + 1);
    if ((uint8)timer_x_pos >= 0x60) {
      timer_x_pos &= 0xFF00;
      timer_status = (timer_status & 0xFF00) | (uint8)(timer_status + 1);
    }
    return ProcessTimer_Decrement();
  case 5: {                                                           // ProcessTimer_MovingIntoPlace
    int v0 = 0;
    uint16 v1 = timer_x_pos + 224;
    if (v1 >= 0xDC00) { v0 = 1; v1 = 0xDC00; }
    timer_x_pos = v1;
    uint16 v2 = timer_y_pos - 193;
    if (v2 < 0x3000) { ++v0; v2 = 0x3000; }
    timer_y_pos = v2;
    if (v0 == 2)
      ++timer_status;
    return ProcessTimer_Decrement();
  }
  case 6: return ProcessTimer_Decrement();
  default: nt_unimpl("ProcessTimer (otro contador)"); return 0;
  }
}

static void DrawTwoTimerDigits(uint8 a, uint16 k) {  // 0x809F95
  DrawTimerSpritemap(0x9FE8 + 12 * (a >> 4), k + (timer_x_pos >> 8), timer_y_pos >> 8);    // kTimerDigitsSpritemapPtr
  DrawTimerSpritemap(0x9FE8 + 12 * (a & 0xF), k + 8 + (timer_x_pos >> 8), timer_y_pos >> 8);
}

void DrawTimer(void) {  // 0x809F6C
  DrawTimerSpritemap(0xA060, timer_x_pos >> 8, timer_y_pos >> 8);
  DrawTwoTimerDigits(timer_minutes, 0xFFE4);
  DrawTwoTimerDigits(timer_seconds, 0xFFFC);
  DrawTwoTimerDigits(timer_centiseconds, 0x14);
}

static void Samus_ProcessTimerAndDraw(void) {
  ProcessTimer();
  if (timer_status)
    DrawTimer();
}

static void Samus_PushOutOfRidleysWay(void) {  // 0x90E12E
  samus_pose = (samus_pose_x_dir == 4) ? kPose_54_FaceL_Knockback : kPose_53_FaceR_Knockback;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;          /* el original copia dos bytes: x_dir y tipo */
  samus_last_different_pose_movement_type = samus_prev_movement_type2;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type2 = samus_movement_type;
  samus_y_pos -= 21 - samus_y_radius;
  samus_var62 = sign16(samus_x_pos - layer1_x_pos - 128) ? 1 : 2;
  samus_y_speed = 5;
  samus_y_subspeed = 0;
  bomb_jump_dir = 0;
  frame_handler_gamma = FUNC16(Samus_Func4);
  samus_new_pose = -1;
  samus_new_pose_interrupted = -1;
  samus_new_pose_transitional = -1;
  samus_momentum_routine_index = 0;
  samus_special_transgfx_index = 0;
  samus_hurt_switch_index = 0;
  Samus_ProcessTimerAndDraw();
}

static void Samus_Func5_6(void) {  // 0x90E1FD / 0x90E21C (iguales)
  int32 amt = Samus_CalcBaseSpeed_X(Samus_DetermineSpeedTableEntryPtr_X());   // Samus_BombJumpFallingXMovement_
  amt = (samus_var62 == 1) ? Samus_CalcDisplacementMoveLeft(amt) : Samus_CalcDisplacementMoveRight(amt);
  if (amt < 0)
    Samus_MoveLeft(amt);
  else
    Samus_MoveRight(amt);
  if (samus_collision_flag) {
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
    frame_handler_gamma = FUNC16(Samus_Func3);
    samus_var62 = 0;
    Samus_ClearMoveVars();
  } else {                                                                    // Samus_BombJumpFallingYMovement_
    if (sign16(samus_y_speed - 5))
      add_hilo(&samus_y_speed, &samus_y_subspeed, IPAIR32(samus_y_accel, samus_y_subaccel));
    Samus_MoveDown(IPAIR32(samus_y_speed, samus_y_subspeed));
  }
}

static void Samus_Func4(void) {  // 0x90E1C8
  if (samus_new_pose == kPose_4F_FaceL_Dmgboost || samus_new_pose == kPose_50_FaceR_Dmgboost) {
    samus_new_pose = -1;
    samus_momentum_routine_index = 0;
  }
  if (samus_var62 == 1 || samus_var62 == 2)
    Samus_Func5_6();
  else
    nt_unimpl("Samus_Func4 (samus_var62)");
  input_to_pose_calc = 0;
  Samus_ProcessTimerAndDraw();
}

static void RunFrameHandlerGamma(void) {  // 0x90E097
  switch (frame_handler_gamma) {
  case FUNC16(nullsub_151): case FUNC16(nullsub_152): break;
  case FUNC16(Samus_Func3):                     // 0x90E0E6
    if (ProcessTimer() & 1) {                   /* se acabó el tiempo: todo se funde a blanco (estado 35) */
      game_state = 35;
      for (int i = 255; i >= 0; i--)
        target_palettes[i] = 0x7FFF;
      frame_handler_gamma = FUNC16(DrawTimer_);
      DisablePaletteFx();
    }
    if (timer_status)
      DrawTimer();
    break;
  case FUNC16(DrawTimer_): DrawTimer(); break;
  case FUNC16(Samus_PushOutOfRidleysWay): Samus_PushOutOfRidleysWay(); break;
  case FUNC16(Samus_Func4): Samus_Func4(); break;
  default: nt_unimpl("RunFrameHandlerGamma"); break;
  }
}

static void Samus_FrameHandlerBeta_Func17(void) {  // 0x90E725
  samus_contact_damage_index = 0;
  RunSamusMovementHandler();
  UpdateMinimap();
  RunFrameHandlerGamma();
  Samus_Animate();
  Samus_HitInterruption();
  Samus_HandleTransFromBlockColl();
  Samus_HandleTransitions();
  Samus_HandlePalette();
  Samus_HandlePeriodicDamage();
  Samus_PauseCheck();
  Samus_LowHealthCheck_();
}

void HandleSamusMovementAndPause(void) {  // 0x90E722
  switch (frame_handler_beta) {
  case FUNC16(Samus_FrameHandlerBeta_Func17):
    Samus_FrameHandlerBeta_Func17();
    break;
  case FUNC16(Samus_Func18):                    // 0x90E8AA
    Samus_FrameHandlerBeta_Func17();
    if (frame_handler_gamma == FUNC16(DrawTimer_) && game_state != 35)
      game_state = 35;
    break;
  case FUNC16(SetContactDamageIndexAndUpdateMinimap):  // 0x90E8DC
    samus_contact_damage_index = 0;
    UpdateMinimap();
    break;
  case FUNC16(Samus_Func15):                    // 0x90E833: intro (entrada de demo): sin minimapa ni daño
    samus_contact_damage_index = 0;
    RunSamusMovementHandler();
    Samus_Animate();
    Samus_HitInterruption();
    Samus_HandleTransFromBlockColl();
    Samus_HandleTransitions();
    Samus_HandlePalette();
    joypad1_lastkeys = controller1_input_for_demo;
    joypad1_newkeys = controller1_new_input_for_demo;
    joypad1_input_samusfilter = demo_backup_prev_controller_input;
    joypad1_newinput_samusfilter = demo_backup_prev_controller_input_new;
    break;
  case FUNC16(Samus_Func19):                    // 0x90E8EC: en el ascensor
    samus_contact_damage_index = 0;
    RunSamusMovementHandler();
    UpdateMinimap();
    Samus_Animate();
    break;
  case FUNC16(EmptyFunction):
    break;
  default:                                      /* j_HandleDemoRecorder_2 ($90:E8D6): Samus quieta (estaciones) */
    if (frame_handler_beta != FUNC16(j_HandleDemoRecorder_2)) nt_unimpl_v("HandleSamusMovementAndPause", frame_handler_beta);
    break;
  }
}

void Samus_HandlePeriodicDamage(void) {  // 0x90E9CE
  if (time_is_frozen_flag) {
    samus_periodic_damage = samus_periodic_subdamage = 0;
    return;
  }
  int32 t = IPAIR32(samus_periodic_damage, samus_periodic_subdamage);
  if (t < 0) {
    nt_unimpl("InvalidInterrupt_Crash");
    return;
  }
  if (equipped_items & 0x20)
    t = (t >> 2) & 0xffff00;
  else if (equipped_items & 1)
    t = (t >> 1) & 0xffff00;
  add_hilo(&samus_health, &samus_subunit_health, -t);
  if ((int16)samus_health < 0)
    samus_health = samus_subunit_health = 0;
  samus_periodic_damage = samus_periodic_subdamage = 0;
}

void Samus_PauseCheck(void) {  // 0x90EA45
  if (!power_bomb_flag && !time_is_frozen_flag && !door_transition_flag_enemies && area_index != 6
      && game_state == 8 && (joypad1_newkeys & kButton_Start)) {
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    game_state = 12;
  }
}

void Samus_LowHealthCheck_(void) {  // 0x90EA7F
  if (sign16(samus_health - 31)) {
    if (!samus_health_warning) {
      QueueSfx3_Max6(2);
      samus_health_warning = 1;
    }
  } else if (samus_health_warning) {
    samus_health_warning = 0;
    QueueSfx3_Max6(1);
  }
}

void Samus_JumpCheck(void) {  // 0x90EAB3
  if ((button_config_jump_a & joypad1_lastkeys) && (button_config_jump_a & joypad1_input_samusfilter))
    ++autojump_timer;
  else
    autojump_timer = 0;
  joypad1_input_samusfilter = joypad1_lastkeys;
  joypad1_newinput_samusfilter = joypad1_newkeys;
  if ((int16)(samus_health - samus_prev_health_for_flash) < 0) {
    if (!samus_hurt_flash_counter)
      samus_hurt_flash_counter = 1;
    if (!sign16(debug_invincibility - 7)) {
      samus_health = samus_prev_health_for_flash;
      return;
    }
  }
  samus_prev_health_for_flash = samus_health;
}

void Samus_Func10(void) {  // 0x90EB02
  projectile_init_speed_samus_moved_left = 0;
  projectile_init_speed_samus_moved_left_fract = 0;
  projectile_init_speed_samus_moved_right = 0;
  projectile_init_speed_samus_moved_right_fract = 0;
  projectile_init_speed_samus_moved_up = 0;
  projectile_init_speed_samus_moved_up_fract = 0;
  projectile_init_speed_samus_moved_down = 0;
  projectile_init_speed_samus_moved_down_fract = 0;
  samus_anim_frame_skip = 0;
  new_projectile_direction_changed_pose = 0;
  UNUSED_word_7E0DFA <<= 8;
  samus_prev_pose_x_dir_A10 = samus_pose_x_dir;
  samus_prev_movement_type = samus_movement_type;
}

void Samus_ShootCheck(void) {  // 0x90F576
  bool resume = (play_resume_charging_beam_sfx & 0x8000) != 0;
  if (!resume) {
    if (play_resume_charging_beam_sfx) {
      if (button_config_shoot_x & joypad1_lastkeys)
        QueueSfx1_Max9(0x41);
      play_resume_charging_beam_sfx = 0;
    }
    if (samus_echoes_sound_flag && !(speed_boost_counter & 0x400)) {
      samus_echoes_sound_flag = 0;
      QueueSfx3_Max15(0x25);
    }
    if ((samus_prev_movement_type == 3 || samus_prev_movement_type == 20)
        && samus_movement_type != kMovementType_03_SpinJumping && samus_movement_type != kMovementType_14_WallJumping) {
      QueueSfx1_Max15(0x32);
      if (!sign16(flare_counter - 16) && (button_config_shoot_x & joypad1_lastkeys))
        resume = true;
    }
  }
  if (resume)
    play_resume_charging_beam_sfx = 1;
  if (enable_debug)
    nt_unimpl("Samus_ShootCheck (depuración)");
  if ((CheckEventHappened(0xE) & 1) && frame_handler_gamma == FUNC16(DrawTimer_) && game_state != 35)
    game_state = 35;
}

void DrawSamusAndProjectiles(void) {  // 0x90EB35
  HandleArmCannonOpenState();
  switch (samus_draw_handler) {               // 0x90EB4B
  case FUNC16(SamusDrawHandler_Default):      // 0x90EB52
    HandleChargingBeamGfxAudio();
    if (arm_cannon_drawing_mode & 0xF) {      // 0x90EB55
      HandleAtmosphericEffects();
      if ((arm_cannon_drawing_mode & 0xF) == 2) {
        Samus_Draw();
        Samus_ArmCannon_Draw();
      } else {
        Samus_ArmCannon_Draw();
        Samus_Draw();
      }
      Samus_DrawEchoes();
    } else {
      HandleAtmosphericEffects();
      Samus_Draw();
      Samus_DrawEchoes();
    }
    break;
  case FUNC16(nullsub_152): case FUNC16(nullsub_156):
    break;
  case FUNC16(SamusDisplayHandler_UsingElevator):   // 0x90EC14: parpadea (un cuadro sí, otro no)
    if (!(nmi_frame_counter_word & 1))
      Samus_DrawWhenNotAnimatingOrDying();
    break;
  case FUNC16(Samus_DrawHandler_EndOfShinespark):   // 0x90EBF3
    Samus_Draw();
    for (int i = 2; i >= 0; i -= 2)
      Samus_DrawShinesparkCrashEchoes(i);
    break;
  case FUNC16(SamusDisplayHandler_SamusReceivedFatal):   // 0x90EC1D
    Samus_DrawWhenNotAnimatingOrDying();
    break;
  default: nt_unimpl_v("CallSamusDrawHandler", samus_draw_handler); break;
  }
  DrawPlayerExplosions2();
  Samus_JumpCheck();
  Samus_ShootCheck();
}

void Samus_SetRadius(void) {  // 0x90EC22
  samus_y_radius = kPoseParams[samus_pose].y_radius;
  samus_x_radius = 5;
}

uint16 Samus_GetBottom_R18(void) { return samus_y_pos + kPoseParams[samus_pose].y_radius - 1; }
uint16 Samus_GetTop_R20(void) { return samus_y_pos - kPoseParams[samus_pose].y_radius; }

void Samus_UpdateSuitPaletteIndex(void) {  // 0x90ECB6
  if (equipped_items & 0x20)
    samus_suit_palette_index = 4;
  else if (equipped_items & 1)
    samus_suit_palette_index = 2;
  else
    samus_suit_palette_index = 0;
}

/* pisadas: salpicaduras en el agua o la lluvia, chispas con el speed booster ($90:ED88, por área) */
static int FootstepOutOfLiquid(void) {
  uint16 r18 = Samus_GetBottom_R18();
  if (fx_y_pos & 0x8000)
    return !(!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18));
  return !(sign16(fx_y_pos - r18) && !(fx_liquid_options & 4));
}
static void FootstepPair(uint16 type) {
  if (samus_pose_x_dir == 4) {
    atmospheric_gfx_x_pos[0] = samus_x_pos - 12;
    atmospheric_gfx_x_pos[1] = samus_x_pos + 8;
  } else {
    atmospheric_gfx_x_pos[0] = samus_x_pos + 12;
    atmospheric_gfx_x_pos[1] = samus_x_pos - 8;
  }
  atmospheric_gfx_y_pos[0] = atmospheric_gfx_y_pos[1] = samus_y_pos + 16;
  atmospheric_gfx_frame_and_type[0] = atmospheric_gfx_frame_and_type[1] = type;
  atmospheric_gfx_anim_timer[0] = 0x8002;
  atmospheric_gfx_anim_timer[1] = 3;
}
static void Samus_FootstepGraphics_Maridia(void) {  // 0x90EDEC
  if (FootstepOutOfLiquid()) FootstepPair(256);
}
static void Samus_FootstepGraphics_1(void) {  // 0x90EE64
  if ((speed_boost_counter & 0xFF00) == 1024 && FootstepOutOfLiquid()) FootstepPair(1792);
}
void Samus_FootstepGraphics(void) {  // 0x90ED88
  static const uint8 kCrateriaWet[16] = { 1, 0, 0, 0, 0, 2, 0, 4, 0, 4, 4, 4, 4, 0, 4, 0 };
  if (area_index == 4) { Samus_FootstepGraphics_Maridia(); return; }
  if (area_index != 0) { Samus_FootstepGraphics_1(); return; }
  /* Samus_FootstepGraphics_Crateria */
  if (cinematic_function || (int16)(room_index - 16) >= 0) { Samus_FootstepGraphics_1(); return; }
  uint8 b = kCrateriaWet[room_index];
  if (b & 1) {
    if (fx_type != 10) Samus_FootstepGraphics_1(); else Samus_FootstepGraphics_Maridia();
  } else if (b & 2) {
    if (!sign16(samus_y_pos - 944)) Samus_FootstepGraphics_Maridia(); else Samus_FootstepGraphics_1();
  } else if (b & 4) {
    Samus_FootstepGraphics_Maridia();
  } else {
    Samus_FootstepGraphics_1();
  }
}

void Samus_UpdateSpeedEchoPos(void) {  // 0x90EEE7
  if ((speed_boost_counter & 0xFF00) == 1024 && !(speed_echoes_index & 0x8000) && !(game_time_frames & 3)) {
    uint16 v0 = speed_echoes_index;
    int v1 = speed_echoes_index >> 1;
    speed_echo_xpos[v1] = samus_x_pos;
    speed_echo_ypos[v1] = samus_y_pos;
    uint16 v2 = v0 + 2;
    if ((int16)(v2 - 4) >= 0)
      v2 = 0;
    speed_echoes_index = v2;
  }
}

/* $90:D5A2: ¿se puede hacer el destello de cristal al terminar un power bomb? (Samus quieta, poca energía, 10 de
   cada munición y los botones). El destello en sí todavía no está portado. */
uint8 Hdmaobj_CrystalFlash(void) {
  if (sign16(game_state - 40) && joypad1_lastkeys != (button_config_shoot_x | 0x430))
    return 1;
  if (samus_y_speed || samus_y_subspeed || !sign16(samus_health - 51) || samus_reserve_health
      || sign16(samus_missiles - 10) || sign16(samus_super_missiles - 10) || sign16(samus_power_bombs - 10))
    return 1;
  nt_unimpl("Hdmaobj_CrystalFlash (destello de cristal)");
  return 1;
}
