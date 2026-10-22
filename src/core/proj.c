/* MegaMetroid — motor nativo: selección del HUD, cañón del brazo y proyectiles (banco $90/$93 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_90.c y sm_93.c. */
#include "samus.h"
#include "nmath.h"

/* ------------------------------------------------------------------ selección del HUD ($90:C4B5) */
void ClearFlareAnimationState(void) {  // 0x90BCBE
  flare_animation_frame = 0;
  flare_slow_sparks_anim_frame = 0;
  flare_fast_sparks_anim_frame = 0;
  flare_animation_timer = 0;
  flare_slow_sparks_anim_timer = 0;
  flare_fast_sparks_anim_timer = 0;
}

static uint8 RunSwitchedToHudHandler(uint16 i) {  // 0x90C4B5
  switch (i) {
  case 0: break;                                   // 0x90C545
  case 1: if (!samus_missiles) return 1; break;    // 0x90C551
  case 2: if (!samus_super_missiles) return 1; break;
  case 3: if (!samus_power_bombs) return 1; break;
  case 4:                                          // 0x90C58A
    if (!(equipped_items & 0x4000))
      return 1;
    if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive)) {
      Samus_LoadSuitPalette();
      flare_counter = 0;
      ClearFlareAnimationState();
      grapple_beam_function = FUNC16(GrappleBeamFunc_Inactive);
    }
    return 0;
  case 5: if (!(equipped_items & 0x8000)) return 1; break;
  }
  flare_counter = 0;
  ClearFlareAnimationState();
  Samus_LoadSuitPalette();
  return 0;
}

void HandleSwitchingHudSelection(void) {  // 0x90C4C1
  uint16 v0, r22 = 0, r18 = hud_item_index;
  if (button_config_itemcancel_y & joypad1_newkeys) {
    samus_auto_cancel_hud_item_index = 0;
    v0 = 0;
  } else {
    r22 = (button_config_itemcancel_y & joypad1_lastkeys) != 0;
    if (!(button_config_itemswitch & joypad1_newkeys))
      goto done;
    v0 = hud_item_index + 1;
    if (!sign16(hud_item_index - 5))
      v0 = 0;
  }
  hud_item_index = v0;
  while (RunSwitchedToHudHandler(v0) & 1) {
    v0 = hud_item_index + 1;
    hud_item_index = v0;
    if (!sign16(v0 - 6)) {
      v0 = 0;
      hud_item_index = 0;
    }
  }
  samus_auto_cancel_hud_item_index = r22 ? hud_item_index : 0;
done:
  if (hud_item_index == r18) {
    uint16 v1 = hud_item_changed_this_frame + 1;
    if (!sign16(hud_item_changed_this_frame - 2))
      v1 = 2;
    hud_item_changed_this_frame = v1;
  } else {
    hud_item_changed_this_frame = 1;
  }
}

/* ------------------------------------------------------------------ disparo ($90:B80D) */
static void FireUnchargedBeam(void);
static uint8 Samus_CanFireBeam(void);

static uint8 Samus_CanFireBeam(void) {  // 0x90AC39
  if (!sign16(projectile_counter - 5) || (uint8)cooldown_timer)
    return 0;
  cooldown_timer = 1;
  ++projectile_counter;
  return 1;
}

static void FireChargedBeam(void);
static uint8 FireSba(void);

static void HudSelectionHandler_NothingOrPowerBombs(void) {  // 0x90B80D
  prev_beam_charge_counter = flare_counter;
  if (hyper_beam_flag || !(equipped_beams & 0x1000)) {
    if (button_config_shoot_x & joypad1_lastkeys)
      FireUnchargedBeam();
    return;
  }
  if (new_projectile_direction_changed_pose) {
    if (sign16(flare_counter - 60))
      goto fire_uncharged;
    goto fire_charged;
  }
  if (!(button_config_shoot_x & joypad1_lastkeys)) {
    if (!flare_counter)
      return;
    if (sign16(flare_counter - 60))
      goto fire_uncharged;
    goto fire_charged;
  }
  if (sign16(flare_counter - 120)) {
    if (++flare_counter == 1) {
      ClearFlareAnimationState();
      FireUnchargedBeam();
    }
  } else if (FireSba() & 1) {
    flare_counter = 0;
    ClearFlareAnimationState();
    Samus_LoadSuitPalette();
  }
  return;
fire_uncharged:                                            /* LABEL_14 */
  flare_counter = 0;
  ClearFlareAnimationState();
  FireUnchargedBeam();
  return;
fire_charged:                                              /* LABEL_15 */
  flare_counter = 0;
  ClearFlareAnimationState();
  FireChargedBeam();
}

/* Ataques especiales (rayo cargado + bomba de poder elegida). Con el rayo solo (sin onda, hielo,
   spazer ni plasma) no hacen nada (FireSba_ClearCarry). */
static uint8 FireSba(void) {  // 0x90CCC0
  static const uint16 kCostOfSbaInPowerBombs[12] = {  // 0x90CC21
    0, 1, 1, 0, 1, 0, 0, 0,
    1, 0, 0, 0,
  };
  if (hud_item_index != 3)
    return 0;
  uint16 v1 = 2 * (equipped_beams & 0xF);
  int16 v2 = samus_power_bombs - kCostOfSbaInPowerBombs[v1 >> 1];
  if (v2 < 0)
    v2 = 0;
  samus_power_bombs = v2;
  uint8 rv = 0;                                            /* kFireSbaFuncs: FireSba_ClearCarry ($90:CD18) */
  switch (v1 >> 1) {
  case 1: nt_unimpl("FireSba_FireWave"); break;
  case 2: nt_unimpl("FireSba_FireIce"); break;
  case 4: nt_unimpl("FireSba_FireSpazer"); break;
  case 8: nt_unimpl("FireSba_FirePlasma"); break;
  }
  if (!samus_power_bombs) {
    hud_item_index = 0;
    samus_auto_cancel_hud_item_index = 0;
  }
  return rv;
}

static void HudSelectionHandler_MissilesOrSuperMissiles(void);
static uint8 DeleteProjectileIfFarOffScreen(void);

void HudSelectionHandler_Normal(void) {  // 0x90DD3D
  uint16 v0;
  if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive))
    v0 = time_is_frozen_flag ? 10 : 2 * hud_item_index;
  else
    v0 = 8;
  switch (v0 >> 1) {
  case 0: case 3: HudSelectionHandler_NothingOrPowerBombs(); break;
  case 1: case 2: HudSelectionHandler_MissilesOrSuperMissiles(); break;
  case 6: HudSelectionHandler_TurningAround(); break;
  default: nt_unimpl("HudSelectionHandler_Normal (misiles/gancho/rayos X)"); break;
  }
}

void HudSelectionHandler_TurningAround(void) {  // 0x90DD74
  if (new_projectile_direction_changed_pose)
    HudSelectionHandler_Normal();
  else if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive))
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
}

void HudSelectionHandler_CrouchEtcTrans(void) {  // 0x90DD8C
  static const uint8 byte_90DDAA[12] = { 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1 };
  if (sign16(samus_pose - kPose_F1_FaceR_CrouchTrans_AimU)) {
    if (!sign16(samus_pose - kPose_DB))
      return;
    if (byte_90DDAA[(uint16)(samus_pose - 53)]) {
      if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive)) {
        grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
        HudSelectionHandler_Normal();
      }
      return;
    }
  }
  HudSelectionHandler_Normal();
}

/* La bola ($90:BF9D). Sin bombas ni bombas de poder (aún no se pueden tener) solo apaga la carga del rayo. */
static uint8 HudSelectionHandler_MorphBall_Helper2(void) {  // 0x90C0E7
  if (!(button_config_shoot_x & joypad1_newkeys)
      || (bomb_counter && (!sign16(bomb_counter - 5) || (uint8)cooldown_timer))) {
    if (flare_counter) {
      QueueSfx1_Max9(2);
      flare_counter = 0;
      ClearFlareAnimationState();
      Samus_LoadSuitPalette();
    }
    return 0;
  }
  ++cooldown_timer;
  ++bomb_counter;
  return 1;
}

static void InitializeInstrForMissile(uint16 v0);
static void ProjPreInstr_Bomb(uint16 k);
static void ProjPreInstr_SpreadBomb(uint16 k);
static const uint8 kNonBeamProjectileCooldowns[9];

/* bombas en abanico: la bola con el rayo cargado ($90:D849) */
static const uint16 kBombSpread_Tab0[5] = { 0x78, 0x6e, 0x64, 0x6e, 0x78 };
static const uint16 kBombSpread_Tab1[5] = { 0x8100, 0x8080, 0, 0x80, 0x100 };
static const uint16 kBombSpread_Tab2[5] = { 0, 1, 2, 1, 0 };
static const uint16 kBombSpread_Tab3[5] = { 0, 0, 0x8000, 0, 0 };

static void BombSpread(void) {  // 0x90D849
  if (bomb_functions[0] != FUNC16(ProjPreInstr_SpreadBomb)) {
    uint16 v0 = 10;
    do {
      int v1 = v0 >> 1;
      projectile_type[v1] = 0x8500;
      projectile_dir[v1] = 0;
      projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_SpreadBomb);
      InitializeInstrForMissile(v0);
      projectile_x_pos[v1] = samus_x_pos;
      projectile_bomb_x_subpos[v1] = 0;
      projectile_y_pos[v1] = samus_y_pos;
      projectile_bomb_y_subpos[v1] = 0;
      int v2 = (uint16)(v0 - 10) >> 1;
      projectile_bomb_x_speed[v1] = kBombSpread_Tab1[v2];
      projectile_timers[v1] = kBombSpread_Tab3[v2];
      uint16 v3 = -(int16)(kBombSpread_Tab2[v2] + ((bomb_spread_charge_timeout_counter >> 6) & 3));
      projectile_bomb_y_speed[v1] = v3;
      projectile_unk_A[v1] = v3;
      projectile_variables[v1] = kBombSpread_Tab0[v2];
      v0 += 2;
    } while ((int16)(v0 - 20) < 0);
    bomb_counter = 5;
    cooldown_timer = kNonBeamProjectileCooldowns[HI8(projectile_type[5]) & 0xF];
    bomb_spread_charge_timeout_counter = 0;
    flare_counter = 0;
  }
}

static uint8 HudSelectionHandler_MorphBall_Helper(void) {  // 0x90C0AB
  if (equipped_items & 0x1000) {
    if (sign16(flare_counter - 60) || bomb_counter)
      return HudSelectionHandler_MorphBall_Helper2();
    if ((joypad1_lastkeys & kButton_Down) && sign16((bomb_spread_charge_timeout_counter & 0xC0) - 192)) {
      ++bomb_spread_charge_timeout_counter;
    } else {
      BombSpread();
      Samus_LoadSuitPalette();
      QueueSfx1_Max9(2);
    }
  }
  return 0;
}

void HudSelectionHandler_MorphBall(void) {  // 0x90BF9D
  if (button_config_shoot_x & joypad1_lastkeys) {
    if (hud_item_index == 3) {
      if (!(power_bomb_flag & 0x8000) && (HudSelectionHandler_MorphBall_Helper2() & 1) && samus_power_bombs)
        nt_unimpl("HudSelectionHandler_MorphBall (bomba de poder)");
    } else if (HudSelectionHandler_MorphBall_Helper() & 1) {
      uint16 v0 = 10;
      while (projectile_type[v0 >> 1]) {
        v0 += 2;
        if ((int16)(v0 - 20) >= 0) {
          v0 -= 2;
          break;
        }
      }
      int v1 = v0 >> 1;
      projectile_type[v1] = 1280;
      projectile_dir[v1] = 0;
      projectile_x_pos[v1] = samus_x_pos;
      projectile_y_pos[v1] = samus_y_pos;
      projectile_variables[v1] = 60;
      InitializeInstrForMissile(v0);
      projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_Bomb);
      cooldown_timer = kNonBeamProjectileCooldowns[5];
    }
  } else if (flare_counter) {
    QueueSfx1_Max9(2);
    flare_counter = 0;
    ClearFlareAnimationState();
    Samus_LoadSuitPalette();
  }
}

void HudSelectionHandler_JumpEtc(void) {  // 0x90DDB6
  if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive)) {
    grapple_beam_function = FUNC16(GrappleBeamFunc_Cancel);
    HudSelectionHandler_Normal();
  }
}

/* ------------------------------------------------------------------ proyectiles ($90:AECE) */
void ClearProjectile(uint16 k) {  // 0x90ADB7
  int v1 = k >> 1;
  projectile_x_pos[v1] = 0;
  projectile_y_pos[v1] = 0;
  projectile_bomb_x_subpos[v1] = 0;
  projectile_bomb_y_subpos[v1] = 0;
  projectile_dir[v1] = 0;
  projectile_bomb_x_speed[v1] = 0;
  projectile_bomb_y_speed[v1] = 0;
  projectile_x_radius[v1] = 0;
  projectile_y_radius[v1] = 0;
  projectile_type[v1] = 0;
  projectile_damage[v1] = 0;
  projectile_bomb_instruction_ptr[v1] = 0;
  projectile_bomb_instruction_timers[v1] = 0;
  projectile_variables[v1] = 0;
  projectile_spritemap_pointers[v1] = 0;
  projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_Empty);
  if ((int16)(k - 10) >= 0) {
    if (--bomb_counter & 0x8000)
      bomb_counter = 0;
  } else if (--projectile_counter & 0x8000) {
    projectile_counter = 0;
  }
}

/* ------------------------------------------------------------------ cañón del brazo ($90:C5C4) */
static uint8 UpdateArmCannonIsOpenFlag(void) {  // 0x90C5EB
  static const int8 kFlagShouldArmCannonBeOpen[6] = { 0, 1, 1, 0, 1, 0 };
  if (sign16(hud_item_changed_this_frame - 2))
    return 0;
  uint16 r18 = kFlagShouldArmCannonBeOpen[hud_item_index];
  if (flag_arm_cannon_open_or_opening == r18)
    return 0;
  arm_cannon_frame = r18 ? 0 : 4;
  flag_arm_cannon_open_or_opening = (uint8)r18;
  flag_arm_cannon_opening_or_closing = 1;
  return 1;
}

static void AdvanceArmCannonFrame(void) {  // 0x90C627
  if (flag_arm_cannon_open_or_opening) {
    if (sign16(arm_cannon_frame - 2)) {
      ++arm_cannon_frame;
      return;
    }
    arm_cannon_frame = 3;
  } else {
    if (arm_cannon_frame != 1 && (int16)(arm_cannon_frame - 1) >= 0) {
      --arm_cannon_frame;
      return;
    }
    arm_cannon_frame = 0;
  }
  flag_arm_cannon_opening_or_closing = 0;
}

void HandleArmCannonOpenState(void) {  // 0x90C5C4
  if (flag_arm_cannon_opening_or_closing || (UpdateArmCannonIsOpenFlag() & 1))
    AdvanceArmCannonFrame();
  arm_cannon_drawing_mode = bank90_armcannon(kPlayerPoseToPtr[samus_pose])[1];
}

void DrawArmCannon(int16 x, int16 y, uint16 attr, uint16 src);   /* capa de la Genesis: sprite 8x8, tile de $9A:src */

void Samus_ArmCannon_Draw(void) {  // 0x90C663
  static const uint16 kDrawArmCannon_Char[10] = { 0x281f, 0x281f, 0x281f, 0x681f, 0xa81f, 0xe81f, 0x281f, 0x681f, 0x681f, 0x681f };
  if (!arm_cannon_frame || (samus_invincibility_timer && (nmi_frame_counter_word & 1)))
    return;
  uint16 v0 = kPlayerPoseToPtr[samus_pose];
  const uint8 *v1 = bank90_armcannon(v0);
  uint8 dir = v1[0];
  uint16 r22;
  if (dir & 0x80) {
    dir = (samus_anim_frame ? v1[2] : v1[0]) & 0x7F;
    r22 = v0 + 4;
  } else {
    r22 = v0 + 2;
  }
  uint16 attr = kDrawArmCannon_Char[dir];
  const uint8 *v4 = bank90_armcannon(r22 + 2 * samus_anim_frame);
  int16 x = (int8)v4[0] + samus_x_pos - layer1_x_pos;
  int16 y = (int8)v4[1] + samus_y_pos - kPoseParams[samus_pose].y_offset_to_gfx - layer1_y_pos;
  const uint8 *t2 = bank90_armcannon(0xC7A5 + 2 * dir);    /* kDrawArmCannon_Tab2 */
  uint16 tab = (t2[0] | t2[1] << 8) + 2 * arm_cannon_frame;
  uint16 src = bank90_armcannon(tab)[0] | bank90_armcannon(tab)[1] << 8;
  if (x >= 0 && sign16(x - 256) && y >= 0 && sign16(y - 256))
    DrawArmCannon(x, y, attr, src);
  else
    DrawArmCannon(-1, -1, attr, src);                     /* fuera de pantalla: el tile igual se copia */
}

/* destello de la carga: tres partes (el destello, las chispas lentas y las rápidas), cada una con su cuadro y su
   tiempo; en la RAM del SNES van seguidas (flare_animation_frame/timer + 0, 2, 4) */
static uint16 *const kFlareFrame[3] = { &flare_animation_frame, &flare_slow_sparks_anim_frame, &flare_fast_sparks_anim_frame };
static uint16 *const kFlareTimer[3] = { &flare_animation_timer, &flare_slow_sparks_anim_timer, &flare_fast_sparks_anim_timer };

static uint16 b93(uint16 a);
void DrawGrappleOrProjectileSpritemap(uint16 sm, uint16 x, uint16 y);   /* capa de la Genesis: spritemap de $93 */

static void DrawBeamGrappleSpritemap(uint16 a, uint16 x, uint16 y) {  // 0x818A37
  DrawGrappleOrProjectileSpritemap(b93(0xA1A1 + 2 * a), x, y);       /* g_off_93A1A1 */
}

static void DrawFlareAnimationComponent(uint16 k) {  // 0x90BBE1
  static const uint16 word_93A225[3] = { 0, 0x1e, 0x24 };
  static const uint16 word_93A22B[3] = { 0, 0x2a, 0x30 };
  static const int16 kProjectileOriginOffsets_X[13] = { 2, 18, 15, 17, 3, -4, -17, -15, -18, -2, -4, -4, -4 };
  static const int16 kProjectileOriginOffsets_Y[13] = { -28, -19, 1, 6, 17, 17, 6, 1, -20, -28, -20, -2, 8 };
  static const int16 kProjectileOriginOffsets2_X[10] = { 2, 19, 20, 18, 3, -4, -18, -20, -19, -2 };
  static const int16 kProjectileOriginOffsets2_Y[10] = { -32, -22, -3, 6, 25, 25, 6, -3, -20, -32 };
  uint16 r18 = LO8(*kFlareFrame[k >> 1]);
  uint16 v1 = (samus_pose_x_dir == 4) ? word_93A22B[k >> 1] : word_93A225[k >> 1];
  uint16 r22 = r18 + v1;
  uint16 r24 = kPoseParams[samus_pose].y_offset_to_gfx;
  uint16 v2 = kPoseParams[samus_pose].direction_shots_fired;
  if (v2 != 255 && v2 != 16) {
    int v4 = v2 & 0xF;
    uint16 old_x = samus_x_pos, old_y = samus_y_pos;
    if (ceres_status & 0x8000)
      Mode7Rotate(&samus_x_pos, &samus_y_pos);          // Samus_CalcPos_Mode7
    uint16 r20, v5;
    if (samus_movement_type == 1) {
      r20 = samus_x_pos + kProjectileOriginOffsets2_X[v4] - layer1_x_pos;
      v5 = samus_y_pos + kProjectileOriginOffsets2_Y[v4] - r24 - layer1_y_pos;
    } else {
      r20 = samus_x_pos + kProjectileOriginOffsets_X[v4] - layer1_x_pos;
      v5 = samus_y_pos + kProjectileOriginOffsets_Y[v4] - r24 - layer1_y_pos;
    }
    if (!(v5 & 0xFF00))
      DrawBeamGrappleSpritemap(r22, r20, v5);
    if (ceres_status & 0x8000)
      samus_y_pos = old_y, samus_x_pos = old_x;
  }
}

void HandleChargingBeamGfxAudio(void) {  // 0x90BAFC
  if (hyper_beam_flag) {
    if (flare_counter) {
      for (int i = 4; i >= 0; i -= 2) {
        uint16 *tm = kFlareTimer[i >> 1], *fr = kFlareFrame[i >> 1];
        bool v7 = *tm == 1;
        bool v8 = (--*tm & 0x8000) != 0;
        if (v7 || v8) {
          if ((*fr)-- == 1) {
            if (i == 4)
              flare_counter = 0;
          } else {
            *tm = 3;
          }
        }
        DrawFlareAnimationComponent(i);
      }
    }
  } else if ((int16)flare_counter > 0) {
    if (flare_counter == 1) {
      flare_animation_frame = 0;
      flare_slow_sparks_anim_frame = 0;
      flare_fast_sparks_anim_frame = 0;
      flare_animation_timer = 3;
      flare_slow_sparks_anim_timer = 5;
      flare_fast_sparks_anim_timer = 4;
    }
    if (!sign16(flare_counter - 15)) {
      if (flare_counter == 16)
        QueueSfx1_Max9(8);
      uint16 v0 = 0, v9;
      do {
        uint16 *tm = kFlareTimer[v0 >> 1], *fr = kFlareFrame[v0 >> 1];
        int16 v1 = *tm - 1;
        *tm = v1;
        if (v1 < 0) {
          uint16 v2 = *fr + 1;
          *fr = v2;
          uint16 v3 = v2;
          const uint8 *r0 = bank90_flare(bank90_flare_word(0xC481 + v0));   /* kFlareAnimDelays */
          int16 v4 = r0[v2];
          if (v4 == 255) {
            *fr = 0;
            v3 = 0;
          } else if (v4 == 254) {
            uint16 v5 = *fr - r0[v3 + 1];
            *fr = v5;
            v3 = v5;
          }
          *tm = r0[v3];
        }
        v9 = v0;
        DrawFlareAnimationComponent(v0);
        if (sign16(flare_counter - 30))
          break;
        v0 += 2;
      } while ((int16)(v9 - 4) < 0);
    }
  }
}

void ResetProjectileData(void) {  // 0x90AD22
  for (int v1 = 0; v1 < 10; v1++) {
    projectile_timers[v1] = 0;
    projectile_x_pos[v1] = 0;
    projectile_y_pos[v1] = 0;
    projectile_dir[v1] = 0;
    projectile_bomb_x_speed[v1] = 0;
    projectile_bomb_y_speed[v1] = 0;
    projectile_x_radius[v1] = 0;
    projectile_y_radius[v1] = 0;
    projectile_type[v1] = 0;
    projectile_damage[v1] = 0;
    projectile_bomb_instruction_ptr[v1] = 0;
    projectile_bomb_instruction_timers[v1] = 0;
    projectile_variables[v1] = 0;
    projectile_spritemap_pointers[v1] = 0;
    projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_Empty);
  }
  bomb_counter = 0;
  cooldown_timer = 0;
  projectile_counter = 0;
  power_bomb_flag = 0;
  if (hud_auto_cancel_flag) {
    hud_item_index = 0;
    samus_auto_cancel_hud_item_index = 0;
  }
  for (int i = 0; i < 4; i++) {
    speed_echo_xpos[i] = 0;
    speed_echo_ypos[i] = 0;
  }
  for (int i = 0; i < 4; i++)
    speed_echo_xspeed[i] = 0;
  speed_echoes_index = 0;
  if (samus_special_super_palette_flags) {
    samus_special_super_palette_flags = 0;
    Samus_LoadSuitTargetPalette();
  }
  if (hyper_beam_flag)
    nt_unimpl("ResetProjectileData (hiperrayo: SpawnPalfxObject)");
}

/* ------------------------------------------------------------------ datos del banco $93 (listas de instrucciones) */
static uint16 b93(uint16 a) {
  uint16 o = a - BANK93_LO;
  if ((o & 1) || a >= BANK93_HI) nt_unimpl("b93 (dirección fuera de los datos de proyectil preparados)");
  return kBank93[o >> 1];
}

void DrawProjectileSpritemap(uint16 k, uint16 x, uint16 y);     /* capa de la Genesis */
void DrawProjectileTrailSprite(uint16 x, uint16 y, uint16 tile_and_attribs);

void InitializeProjectile(uint16 k) {  // 0x938000
  int v1 = k >> 1;
  int r18 = projectile_dir[v1] & 0xF;
  uint16 v2 = projectile_type[v1], v3;
  if (v2 & 0xF00)
    v3 = b93(kProjectileData_NonBeams + 2 * (HI8(v2) & 0xF));
  else if (v2 & 0x10)
    v3 = b93(kProjectileData_ChargedBeams + 2 * (v2 & 0xF));
  else
    v3 = b93(kProjectileData_UnchargedBeams + 2 * (v2 & 0xF));
  uint16 damage = b93(v3);
  if (sign16(damage))
    nt_unimpl("InitializeProjectile (daño negativo: InvalidInterrupt_Crash)");
  projectile_damage[v1] = damage;
  uint16 v7 = b93(v3 + 2 + 2 * r18);
  projectile_bomb_instruction_ptr[v1] = v7;
  uint16 radii = b93(v7 + 4);
  projectile_x_radius[v1] = LO8(radii);
  projectile_y_radius[v1] = HI8(radii);
  projectile_bomb_instruction_timers[v1] = 1;
}

static void KillProjectileInner(uint16 k) {  // 0x9380CF
  int v1 = k >> 1;
  if (projectile_type[v1] & 0xF00) {
    if (!cinematic_function)
      QueueSfx2_Max6(7);
    uint16 v2 = projectile_type[v1];
    projectile_type[v1] = (v2 & 0xF0FF) | 0x800;
    if (v2 & 0x200) {
      projectile_bomb_instruction_ptr[v1] = kProjInstr_SuperMissileExplosion;
      earthquake_type = 20;
      earthquake_timer = 30;
    } else {
      projectile_bomb_instruction_ptr[v1] = kProjInstr_MissileExplosion;
    }
    if (!sign16(cooldown_timer - 21))
      cooldown_timer = 20;
  } else {
    projectile_type[v1] = (projectile_type[v1] & 0xF0FF) | 0x700;
    projectile_bomb_instruction_ptr[v1] = kProjInstr_BeamExplosion;
    QueueSfx2_Max6(0xC);
  }
  projectile_bomb_instruction_timers[v1] = 1;
  projectile_damage[v1] = 8;
}

static void InitializeInstrForMissile(uint16 v0) {  // 0x9380A0
  int v1 = v0 >> 1;
  uint16 v3 = b93(kProjectileData_NonBeams + 2 * (HI8(projectile_type[v1]) & 0xF));
  uint16 v4 = b93(v3);
  projectile_damage[v1] = v4;
  if (sign16(v4))
    nt_unimpl("InitializeInstrForMissile (daño negativo: InvalidInterrupt_Crash)");
  projectile_bomb_instruction_ptr[v1] = b93(v3 + 2);
  projectile_bomb_instruction_timers[v1] = 1;
}

static void InitializeBombExplosion(uint16 k) {  // 0x93814E
  int v1 = k >> 1;
  projectile_bomb_instruction_ptr[v1] = kProjInstr_BombExplosion;   /* HIWORD(kProjInstrList_Explosion.damages) */
  projectile_bomb_instruction_timers[v1] = 1;
}

static void Bomb_Func2(void) {  // 0x90C128
  uint16 v0 = projectile_index;
  int v1 = projectile_index >> 1;
  uint16 v2 = projectile_variables[v1];
  if (v2) {
    uint16 v3 = v2 - 1;
    projectile_variables[v1] = v3;
    if (v3) {
      if (v3 == 15)
        projectile_bomb_instruction_ptr[v1] += 28;
    } else {
      QueueSfx2_Max6(8);
      InitializeBombExplosion(v0);
    }
  }
}

void BombOrPowerBomb_Func1(uint16 v0);                     /* coll.c */
static void ProjPreInstr_Bomb(uint16 k) {  // 0x90B099
  if (projectile_dir[k >> 1] & 0xF0) {
    ClearProjectile(k);
  } else {
    Bomb_Func2();
    BombOrPowerBomb_Func1(k);
  }
}

uint8 BlockCollSpreadBomb(uint16 k);                       /* coll.c */
static void ProjPreInstr_SpreadBomb(uint16 k) {  // 0x90D8F7
  int v1 = k >> 1;
  if (projectile_dir[v1] & 0xF0) {
    ClearProjectile(k);
    return;
  }
  Bomb_Func2();
  if (projectile_variables[v1]) {
    add_hilo(&projectile_bomb_y_speed[v1], &projectile_timers[v1], IPAIR32(samus_y_accel, samus_y_subaccel));
    add_hilo(&projectile_y_pos[v1], &projectile_bomb_y_subpos[v1], IPAIR32(projectile_bomb_y_speed[v1], projectile_timers[v1]));
    if (BlockCollSpreadBomb(k) & 1) {
      add_hilo(&projectile_y_pos[v1], &projectile_bomb_y_subpos[v1], -IPAIR32(projectile_bomb_y_speed[v1], projectile_timers[v1]));
      if (projectile_bomb_y_speed[v1] & 0x8000) {
        projectile_bomb_y_speed[v1] = 0;
        projectile_y_radius[v1] = 0;
      } else {
        projectile_timers[v1] = kBombSpread_Tab3[v1 - 5];
        projectile_bomb_y_speed[v1] = projectile_unk_A[v1];
      }
      return;
    }
    uint16 t = projectile_bomb_x_speed[v1];
    if (t & 0x8000)
      add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], -(int32)((uint32)(t & 0x7fff) << 8));
    else
      add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], (int32)((uint32)t << 8));
  }
  if (BlockCollSpreadBomb(k) & 1) {
    uint16 t = projectile_bomb_x_speed[v1];
    projectile_bomb_x_speed[v1] ^= 0x8000;
    if (!(t & 0x8000))
      add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], -(int32)((uint32)t << 8));
    else
      add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], (int32)((uint32)(t & 0x7fff) << 8));
  }
}

static uint16 ProjectileInsts_GetValue(uint16 k) {  // 0x9381D1
  uint16 ip = projectile_bomb_instruction_ptr[k >> 1];
  int delta = (projectile_bomb_instruction_timers[k >> 1] == 1 && !sign16(b93(ip))) ? 0 : -8;
  return b93(ip + delta + 6);
}

static void RunProjectileInstructions(void) {  // 0x9381E9
  uint16 v0 = projectile_index;
  int v1 = projectile_index >> 1;
  if (projectile_bomb_instruction_timers[v1]-- == 1) {
    uint16 v3 = projectile_bomb_instruction_ptr[v1];
    uint16 timer;
    while ((timer = b93(v3)) & 0x8000) {
      switch (timer) {
      case 0x822F: ClearProjectile(v0); return;          // Proj93Instr_Delete
      case 0x8239: v3 = b93(v3 + 2); break;              // Proj93Instr_Goto
      default: nt_unimpl("RunProjectileInstructions (instrucción)"); return;
      }
    }
    projectile_bomb_instruction_timers[v1] = timer;
    projectile_spritemap_pointers[v1] = b93(v3 + 2);
    uint16 radii = b93(v3 + 4);
    projectile_x_radius[v1] = LO8(radii);
    projectile_y_radius[v1] = HI8(radii);
    projectile_bomb_instruction_ptr[v1] = v3 + 8;
  }
}

/* ------------------------------------------------------------------ disparo ($90:B887) */
static uint8 InitProjectilePositionDirection(uint16 r20) {  // 0x90BA56
  static const int16 kProjectileOriginOffsets3_X[10] = { 2, 13, 11, 13, 2, -5, -14, -11, -19, -2 };
  static const int16 kProjectileOriginOffsets3_Y[10] = { -8, -13, 1, 4, 13, 13, 4, 1, -19, -8 };
  static const int16 kProjectileOriginOffsets4_X[10] = { 2, 15, 15, 13, 2, -5, -13, -13, -15, -2 };
  static const int16 kProjectileOriginOffsets4_Y[10] = { -8, -16, -2, 1, 13, 13, 1, -2, -16, -8 };
  uint16 v0 = samus_pose;
  uint16 direction_shots_fired;
  if (new_projectile_direction_changed_pose) {
    direction_shots_fired = (uint8)new_projectile_direction_changed_pose;
    new_projectile_direction_changed_pose = 0;
  } else {
    direction_shots_fired = kPoseParams[v0].direction_shots_fired;
    if (direction_shots_fired & 0xF0) {
      if (direction_shots_fired != 16)
        { --projectile_counter; return 1; }
      direction_shots_fired = (direction_shots_fired & 0xFF00) | kPoseParams[samus_last_different_pose].direction_shots_fired;
      if (direction_shots_fired & 0xF0)
        { --projectile_counter; return 1; }
      direction_shots_fired = (uint8)direction_shots_fired;
      v0 = samus_pose;
    }
  }
  int v2 = r20 >> 1;
  projectile_dir[v2] = direction_shots_fired;
  uint16 r22 = (uint8)kPoseParams[v0].y_offset_to_gfx;
  int v6 = projectile_dir[v2] & 0xF;
  if (samus_pose == kPose_75_FaceL_Moonwalk_AimUL || samus_pose == kPose_76_FaceR_Moonwalk_AimUR
      || samus_movement_type == 1) {
    projectile_x_pos[v2] = samus_x_pos + kProjectileOriginOffsets4_X[v6];
    projectile_y_pos[v2] = samus_y_pos + kProjectileOriginOffsets4_Y[v6] - r22;
  } else {
    projectile_x_pos[v2] = samus_x_pos + kProjectileOriginOffsets3_X[v6];
    projectile_y_pos[v2] = samus_y_pos + kProjectileOriginOffsets3_Y[v6] - r22;
  }
  return 0;
}

/* palabra que el original lee a caballo entre dos variables: byte alto de a y byte bajo de la siguiente (b) */
static inline uint16 mid16(uint16 a, uint16 b) { return HI8(a) | (uint16)(LO8(b) << 8); }

static void InitializeProjectileSpeed(uint16 k, uint16 r22) {  // 0x90B1F3
  uint16 r18;
  int kh = k >> 1;
  uint16 left = mid16(projectile_init_speed_samus_moved_left_fract, projectile_init_speed_samus_moved_right);
  uint16 right = mid16(projectile_init_speed_samus_moved_right_fract, projectile_init_speed_samus_moved_up);
  uint16 up = mid16(projectile_init_speed_samus_moved_up_fract, projectile_init_speed_samus_moved_down);
  uint16 absy = mid16(absolute_moved_last_frame_y_fract, projectile_init_speed_samus_moved_left);
  projectile_bomb_x_subpos[kh] = 0;
  projectile_bomb_y_subpos[kh] = 0;
  r18 = LO8(projectile_init_speed_samus_moved_up) ? ((right >> 2) | 0xC000) : 0;
  switch (projectile_dir[kh] & 0xF) {
  case 0: case 9:
    projectile_bomb_y_speed[kh] = r18 - r22;
    projectile_bomb_x_speed[kh] = 0;
    break;
  case 1:
    projectile_bomb_y_speed[kh] = r18 - r22;
    projectile_bomb_x_speed[kh] = left + r22;
    break;
  case 2:
    projectile_bomb_y_speed[kh] = 0;
    projectile_bomb_x_speed[kh] = left + r22;
    break;
  case 3:
    projectile_bomb_y_speed[kh] = up + r22;
    projectile_bomb_x_speed[kh] = left + r22;
    break;
  case 4: case 5:
    projectile_bomb_y_speed[kh] = up + r22;
    projectile_bomb_x_speed[kh] = 0;
    break;
  case 6:
    projectile_bomb_y_speed[kh] = up + r22;
    projectile_bomb_x_speed[kh] = absy - r22;
    break;
  case 7:
    projectile_bomb_y_speed[kh] = 0;
    projectile_bomb_x_speed[kh] = absy - r22;
    break;
  case 8:
    projectile_bomb_y_speed[kh] = r18 - r22;
    projectile_bomb_x_speed[kh] = absy - r22;
    break;
  default:
    nt_unimpl("InitializeProjectileSpeed (dirección)");
  }
}

static void SetInitialProjectileSpeed(uint16 r20) {  // 0x90B1BF
  int v0 = r20 >> 1;
  uint16 d = projectile_dir[v0] & 0xF;
  /* kInitializeProjectileSpeed_XY_Diag: 0x400 recto, 0x2AB en diagonal, igual para todos los rayos */
  uint16 r22 = (d == 1 || d == 3 || d == 6 || d == 8) ? 0x2AB : 0x400;
  InitializeProjectileSpeed(r20, r22);
}

static void CheckBeamCollByDir(uint16 k) {  // 0x90BD64
  switch (projectile_dir[k >> 1] & 0xF) {
  case 0: case 4: case 5: case 9: BlockCollNoWaveBeamVert(projectile_index); break;
  case 1: case 3: case 6: case 8: {
    uint16 v0 = projectile_index;
    if (!(BlockCollNoWaveBeamHoriz(projectile_index) & 1))
      BlockCollNoWaveBeamVert(v0);
    break;
  }
  case 2: BlockCollNoWaveBeamHoriz(projectile_index); break;
  case 7: {
    uint16 v0 = projectile_index;
    projectile_bomb_x_speed[projectile_index >> 1] = -1;
    BlockCollNoWaveBeamHoriz(v0);
    break;
  }
  default: nt_unimpl("CheckBeamCollByDir (dirección)"); break;
  }
}

static void FireUnchargedBeam(void) {  // 0x90B887
  static const uint16 kUnchargedProjectile_Sfx[12] = { 0xb, 0xd, 0xc, 0xe, 0xf, 0x12, 0x10, 0x11, 0x13, 0x16, 0x14, 0x15 };
  static const uint8 kProjectileCooldown_Uncharged[38] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 12, 15, 0, 0, 0, 0,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0,
     0,  0,  0,  0,  0,  0,
  };
  static const uint8 kBeamAutoFireCooldowns[12] = { 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19 };
  static const uint16 kProjectileBombPreInstr[12] = {
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_BeamOrIceWave),
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_BeamOrIceWave),
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
    FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  };
  if (hyper_beam_flag) {
    nt_unimpl("FireHyperBeam");
    return;
  }
  if (Samus_CanFireBeam() & 1) {
    uint16 v0 = 0;
    while (projectile_damage[v0 >> 1]) {
      v0 += 2;
      if ((int16)(v0 - 10) >= 0) {
        v0 -= 2;
        break;
      }
    }
    if (!(InitProjectilePositionDirection(v0) & 1)) {
      projectile_invincibility_timer = 10;
      uint16 v1 = v0;
      int v2 = v0 >> 1;
      projectile_timers[v2] = 4;
      int8 v3 = equipped_beams;
      projectile_type[v2] = equipped_beams | 0x8000;
      QueueSfx1_Max15(kUnchargedProjectile_Sfx[v3 & 0xF]);
      play_resume_charging_beam_sfx = 0;
      InitializeProjectile(v1);
      uint16 v6 = projectile_type[v2];
      if ((equipped_beams & 0x1000) || (button_config_shoot_x & joypad1_newkeys)
          || (button_config_shoot_x & joypad1_newinput_samusfilter))
        cooldown_timer = kProjectileCooldown_Uncharged[v6 & 0x3F];
      else
        cooldown_timer = kBeamAutoFireCooldowns[v6 & 0x3F];
      projectile_bomb_x_speed[v2] = 0;
      projectile_bomb_y_speed[v2] = 0;
      projectile_index = v1;
      if (!(v6 & 1)) {
        CheckBeamCollByDir(v1);
        v1 = projectile_index;
        if (projectile_type[projectile_index >> 1] & 0xF00)
          return;
      } else {
        nt_unimpl("WaveBeam_CheckColl");
      }
      projectile_bomb_pre_instructions[v1 >> 1] = kProjectileBombPreInstr[projectile_type[v1 >> 1] & 0xF];
      SetInitialProjectileSpeed(v1);
      return;
    }
  }
  if (!sign16(prev_beam_charge_counter - 16)) {
    play_resume_charging_beam_sfx = 0;
    QueueSfx1_Max15(2);
  }
}

static const uint16 kFireChargedBeam_Funcs[12] = {
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
  FUNC16(ProjPreInstr_Beam_NoWaveBeam), FUNC16(ProjPreInstr_WavePlasmaEtc),
};

static void FireChargedBeam(void) {  // 0x90B986
  static const uint16 kChargedProjectile_Sfx[12] = { 0x17, 0x19, 0x18, 0x1a, 0x1b, 0x1e, 0x1c, 0x1d, 0x1f, 0x22, 0x20, 0x21 };
  static const uint8 kProjectileCooldown_Uncharged[38] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 12, 15, 0, 0, 0, 0,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0,
     0,  0,  0,  0,  0,  0,
  };
  if (Samus_CanFireBeam() & 1) {
    uint16 v0 = 0;
    while (projectile_damage[v0 >> 1]) {
      v0 += 2;
      if ((int16)(v0 - 10) >= 0) {
        v0 -= 2;
        break;
      }
    }
    uint16 r20 = v0;
    if (!(InitProjectilePositionDirection(r20) & 1)) {
      projectile_invincibility_timer = 10;
      uint16 v1 = r20;
      int v2 = r20 >> 1;
      projectile_timers[v2] = 4;
      uint16 v3 = (equipped_beams & 0x100F) | 0x8010;
      projectile_type[v2] = v3;
      QueueSfx1_Max15(kChargedProjectile_Sfx[v3 & 0xF]);
      play_resume_charging_beam_sfx = 0;
      InitializeProjectile(v1);
      uint16 v5 = projectile_type[v2];
      cooldown_timer = kProjectileCooldown_Uncharged[v5 & 0x3F];
      projectile_bomb_x_speed[v2] = 0;
      projectile_bomb_y_speed[v2] = 0;
      projectile_index = v1;
      if (v5 & 1) {
        nt_unimpl("WaveBeam_CheckColl");
      } else {
        CheckBeamCollByDir(v1);
        v1 = projectile_index;
        if (projectile_type[projectile_index >> 1] & 0xF00) {
          charged_shot_glow_timer = 4;                     /* LABEL_14 */
          return;
        }
      }
      r20 = v1;
      projectile_bomb_pre_instructions[v1 >> 1] = kFireChargedBeam_Funcs[projectile_type[v1 >> 1] & 0xF];
      SetInitialProjectileSpeed(r20);
      charged_shot_glow_timer = 4;
      return;
    }
  }
  if (!sign16(prev_beam_charge_counter - 16)) {
    play_resume_charging_beam_sfx = 0;
    QueueSfx1_Max15(2);
  }
}

/* ------------------------------------------------------------------ estelas ($90:B657, $9B:A3CC) */
static uint16 b90t(uint16 a) { return kBank90_Trails[(uint16)(a - BANK90_TRAILS_LO) >> 1]; }
static uint16 b9bt(uint16 a) { return kBank9B_TrailPtrs[(uint16)(a - BANK9B_TRAILPTRS_LO) >> 1]; }

static void ProjectileTrail_Func5(uint16 k, uint16 j) {  // 0x9BA3CC
  uint16 R22 = ProjectileInsts_GetValue(k);
  int v2 = k >> 1;
  uint16 r18 = projectile_x_pos[v2];
  uint16 r20 = projectile_y_pos[v2];
  if (ceres_status & 0x8000)                        /* CalcExplosion_Mode7 + cámara: el punto girado en la sala */
    Mode7Rotate(&r18, &r20);
  uint16 v4 = projectile_type[v2], v5;
  if (v4 & 0x20)
    v5 = b9bt(0xA4E3 + 2 * (v4 & 0xF)) + 2 * (projectile_dir[v2] & 0xF);
  else if (v4 & 0x10)
    v5 = b9bt(0xA4CB + 2 * (v4 & 0xF)) + 2 * (projectile_dir[v2] & 0xF);
  else
    v5 = b9bt(0xA4B3 + 2 * (v4 & 0xF)) + 2 * (projectile_dir[v2] & 0xF);
  uint16 v6 = b9bt(v5) + 4 * R22;
  const int8 *p = (const int8 *)&kBank9B_TrailOffsets[(uint16)(v6 - 0xA56F)];
  int v7 = j >> 1;
  projectiletrail_left_y_pos[v7] = r20 + p[1] - 4;
  projectiletrail_left_x_pos[v7] = r18 + p[0] - 4;
  projectiletrail_right_y_pos[v7] = r20 + p[3] - 4;
  projectiletrail_right_x_pos[v7] = r18 + p[2] - 4;
}

static void SpawnProjectileTrail(uint16 k) {  // 0x90B657
  uint16 v1 = projectile_type[k >> 1];
  int16 v2;
  if (v1 & 0xF00) {
    uint16 v3 = HI8(v1) & 0xF;
    if (v3 >= 3)
      return;
    v2 = v3 + 31;
  } else {
    v2 = v1 & 0x3F;
  }
  uint16 v4 = 34;
  while (projectiletrail_left_instr_timer[v4 >> 1]) {
    v4 -= 2;
    if (v4 & 0x8000)
      return;
  }
  int v5 = v4 >> 1;
  projectiletrail_left_instr_timer[v5] = 1;
  projectiletrail_right_instr_timer[v5] = 1;
  projectiletrail_left_instr_list_ptr[v5] = off_90B5BB[v2];
  projectiletrail_right_instr_list_ptr[v5] = off_90B609[v2];
  ProjectileTrail_Func5(projectile_index, v4);
}

static void ProjInstr_Trail(uint16 f, uint16 j) {
  switch (f) {
  case 0xB525: ++projectiletrail_left_y_pos[j >> 1]; break;    // ProjInstr_MoveLeftProjectileTrailDown
  case 0xB587: ++projectiletrail_right_y_pos[j >> 1]; break;   // ProjInstr_MoveRightProjectileTrailDown
  case 0xB5B3: --projectiletrail_left_y_pos[j >> 1]; break;    // ProjInstr_MoveLeftProjectileTrailUp
  default: nt_unimpl("HandleProjectileTrails (instrucción)"); break;
  }
}

static void DrawTrail(uint16 x, uint16 y, uint16 tile) {   /* el sprite de 8x8 de la estela (OAM del SNES) */
  if ((int16)(oam_next_ptr - 512) < 0) {
    uint16 sx = x - layer1_x_pos;
    if (!(sx & 0xFF00)) {
      uint16 sy = y - layer1_y_pos;
      if (!(sy & 0xFF00)) {
        DrawProjectileTrailSprite(sx, sy, tile);
        oam_next_ptr += 4;
      }
    }
  }
}

void HandleProjectileTrails(void) {  // 0x90B6A9
  if (!time_is_frozen_flag) {
    for (uint16 v0 = 34; !(v0 & 0x8000); v0 -= 2) {
      int v1 = v0 >> 1;
      if (projectiletrail_left_instr_timer[v1]) {
        if (--projectiletrail_left_instr_timer[v1]) {
          DrawTrail(projectiletrail_left_x_pos[v1], projectiletrail_left_y_pos[v1], projectiletrail_left_tile_and_attribs[v1]);
        } else {
          uint16 i = projectiletrail_left_instr_list_ptr[v1], v6;
          while ((v6 = b90t(i)) & 0x8000) {
            ProjInstr_Trail(v6, v0);
            i += 2;
          }
          projectiletrail_left_instr_timer[v1] = v6;
          if (v6) {
            projectiletrail_left_tile_and_attribs[v1] = b90t(i + 2);
            projectiletrail_left_instr_list_ptr[v1] = i + 4;
            DrawTrail(projectiletrail_left_x_pos[v1], projectiletrail_left_y_pos[v1], projectiletrail_left_tile_and_attribs[v1]);
          }
        }
      }
      if (projectiletrail_right_instr_timer[v1]) {
        if (--projectiletrail_right_instr_timer[v1]) {
          DrawTrail(projectiletrail_right_x_pos[v1], projectiletrail_right_y_pos[v1], projectiletrail_right_tile_and_attribs[v1]);
        } else {
          uint16 j = projectiletrail_right_instr_list_ptr[v1], v18;
          while ((v18 = b90t(j)) & 0x8000) {
            ProjInstr_Trail(v18, v0);
            j += 2;
          }
          projectiletrail_right_instr_timer[v1] = v18;
          if (v18) {
            projectiletrail_right_tile_and_attribs[v1] = b90t(j + 2);
            projectiletrail_right_instr_list_ptr[v1] = j + 4;
            DrawTrail(projectiletrail_right_x_pos[v1], projectiletrail_right_y_pos[v1], projectiletrail_right_tile_and_attribs[v1]);
          }
        }
      }
    }
  } else {
    for (uint16 k = 34; !(k & 0x8000); k -= 2) {
      int v = k >> 1;
      if (projectiletrail_left_instr_timer[v])
        DrawTrail(projectiletrail_left_x_pos[v], projectiletrail_left_y_pos[v], projectiletrail_left_tile_and_attribs[v]);
      if (projectiletrail_right_instr_timer[v])
        DrawTrail(projectiletrail_right_x_pos[v], projectiletrail_right_y_pos[v], projectiletrail_right_tile_and_attribs[v]);
    }
  }
}

/* ------------------------------------------------------------------ misiles ($90:BE62) */
static const uint16 kNonBeamProjectile_Sfx[9] = { 0, 3, 4, 0, 0, 0, 0, 0, 0 };
static const uint8 kNonBeamProjectileCooldowns[9] = { 0, 0xa, 0x14, 0x28, 0, 0x10, 0, 0, 0 };

static uint8 Samus_CanFireSuperMissile(void) {  // 0x90AC5A
  if (hud_item_index == 2 ? !sign16(projectile_counter - 4) : !sign16(projectile_counter - 5))
    return 0;
  if ((uint8)cooldown_timer)
    return 0;
  cooldown_timer = 1;
  ++projectile_counter;
  return 1;
}

static void HudSelectionHandler_MissilesOrSuperMissiles(void) {  // 0x90BE62
  if ((!(button_config_shoot_x & joypad1_newkeys) && !(button_config_shoot_x & joypad1_newinput_samusfilter)) ||
      !(Samus_CanFireSuperMissile() & 1))
    return;
  if (hud_item_index == 2 ? !samus_super_missiles : !samus_missiles) {
    --projectile_counter;
    return;
  }
  uint16 r20 = 0;
  while (projectile_damage[r20 >> 1]) {
    r20 += 2;
    if ((int16)(r20 - 10) >= 0) {
      --projectile_counter;
      return;
    }
  }
  if (InitProjectilePositionDirection(r20) & 1)
    return;
  projectile_invincibility_timer = 20;
  if (hud_item_index == 2)
    --samus_super_missiles;
  else
    --samus_missiles;
  int v1 = r20 >> 1;
  projectile_timers[v1] = 4;
  uint16 v3 = hud_item_index;
  projectile_type[v1] |= (uint16)(v3 << 8 | v3 >> 8) | 0x8000;
  if (!cinematic_function)
    QueueSfx1_Max6(kNonBeamProjectile_Sfx[v3 & 0xF]);
  InitializeProjectileSpeed(r20, 0);                      /* InitializeProjectileSpeedOfType */
  InitializeProjectile(r20);
  uint16 v7 = projectile_type[v1];
  projectile_bomb_pre_instructions[v1] = (v7 & 0x200) ? FUNC16(ProjPreInstr_SuperMissile) : FUNC16(ProjPreInstr_Missile);
  cooldown_timer = kNonBeamProjectileCooldowns[HI8(v7) & 0xF];
  if (samus_auto_cancel_hud_item_index) {
    hud_item_index = 0;
    samus_auto_cancel_hud_item_index = 0;
    return;
  }
  if (hud_item_index == 2 ? samus_super_missiles : samus_missiles)
    return;
  hud_item_index = 0;
}

static void InitializeInstrForSuperMissile(uint16 v0) {  // 0x938071
  int v1 = v0 >> 1;
  uint16 v3 = b93(0x842B + 2 * (HI8(projectile_type[v1]) & 0xF));   /* kRunInstrForSuperMissile $93:842B */
  uint16 v4 = b93(v3);
  projectile_damage[v1] = v4;
  if (sign16(v4))
    nt_unimpl("InitializeInstrForSuperMissile (daño negativo: InvalidInterrupt_Crash)");
  projectile_bomb_instruction_ptr[v1] = b93(v3 + 2);
  projectile_bomb_instruction_timers[v1] = 1;
}

/* Super misil: el proyectil "estela" que lo acompaña ($90:BF46) */
static void Missile_Func2(void) {  // 0x90BF46
  uint16 v0 = 0;
  while (projectile_damage[v0 >> 1]) {
    v0 += 2;
    if ((int16)(v0 - 10) >= 0)
      return;
  }
  uint16 r20 = v0;
  int v1 = v0 >> 1;
  projectile_type[v1] |= 0x8200;
  int v2 = projectile_index >> 1;
  projectile_x_pos[v1] = projectile_x_pos[v2];
  projectile_y_pos[v1] = projectile_y_pos[v2];
  projectile_dir[v1] = projectile_dir[v2];
  InitProjectilePositionDirection(r20);
  InitializeInstrForSuperMissile(v0);
  projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_Func1);
  projectile_variables[projectile_index >> 1] = v0 + (projectile_variables[projectile_index >> 1] & 0xFF00);
  ++projectile_counter;
}

static void Missile_Func1(uint16 k) {  // 0x90B2F6
  int v1 = k >> 1;
  if (projectile_variables[v1] & 0xFF00) {                /* a toda velocidad: aceleración fija por dirección */
    int base = (projectile_type[v1] & 0x200) ? 20 : 0;
    int d = projectile_dir[v1] & 0xF;
    projectile_bomb_x_speed[v1] += kSuperMissileAccel[base + 2 * d];
    projectile_bomb_y_speed[v1] += kSuperMissileAccel[base + 2 * d + 1];
  } else {
    uint16 v2 = 0x100 + projectile_variables[v1];
    projectile_variables[v1] = v2;
    if (v2 & 0xFF00) {
      InitializeProjectileSpeed(k, projectile_variables[v1]);
      if (projectile_type[v1] & 0x200)
        Missile_Func2();
    }
  }
}

static void ProjPreInstr_Missile(uint16 k) {  // 0x90AF68
  static const int16 kDirToVelMult16_X[10] = { 0, 16, 16, 16, 0, 0, -16, -16, -16, 0 };
  static const int16 kDirToVelMult16_Y[10] = { -16, -16, 0, 16, 16, 16, 16, 0, -16, -16 };
  int v1 = k >> 1;
  if (projectile_dir[v1] & 0xF0) {
    ClearProjectile(k);
    return;
  }
  if (projectile_timers[v1]-- == 1) {
    projectile_timers[v1] = 4;
    SpawnProjectileTrail(k);
    k = projectile_index;
  }
  int v3 = k >> 1;
  int v5 = projectile_dir[v3] & 0xF;
  projectile_bomb_x_speed[v3] += kDirToVelMult16_X[v5];
  projectile_bomb_y_speed[v3] += kDirToVelMult16_Y[v5];
  Missile_Func1(k);
  switch (v5) {
  case 0: case 4: case 5: case 9: BlockCollMissileVert(projectile_index); break;
  case 1: case 3: case 6: case 8: {
    uint16 v = projectile_index;
    if (!BlockCollMissileHoriz(projectile_index))
      BlockCollMissileVert(v);
    break;
  }
  default: BlockCollMissileHoriz(projectile_index); break;
  }
  DeleteProjectileIfFarOffScreen();
}

/* ------------------------------------------------------------------ super misil ($90:AFE5) */
static void SuperMissileBlockCollDetect_Y(void) {  // 0x90B366
  int v0 = projectile_index >> 1;
  if ((projectile_type[v0] & 0xF00) == 512 || (projectile_type[v0] & 0xF00) == 2048) {
    uint8 v5 = projectile_variables[v0];
    if (HI8(projectile_variables[v0])) {
      uint16 v1 = abs16n(projectile_bomb_y_speed[v0]) & 0xFF00;
      if (sign16(v1 - 2816)) {
        int v4 = projectile_index >> 1;
        if ((projectile_type[v4] & 0xF00) != 2048) {
          projectile_y_pos[v5 >> 1] = projectile_y_pos[v4];
          return;
        }
      } else {
        uint16 r18 = (v1 >> 8) - 10;
        if (!(projectile_bomb_y_speed[v0] & 0x8000)) {
          uint16 v6 = projectile_index;
          projectile_y_pos[v5 >> 1] = projectile_y_pos[v0] - r18;
          projectile_index = v5;
          BlockCollMissileVert(v5);
          projectile_index = v6;
          if ((projectile_type[v6 >> 1] & 0xF00) == 2048)
            ClearProjectile(v5);
          return;
        }
        uint16 v7 = projectile_index;
        projectile_y_pos[v5 >> 1] = r18 + projectile_y_pos[projectile_index >> 1];
        projectile_index = v5;
        BlockCollMissileVert(v5);
        projectile_index = v7;
        if ((projectile_type[v7 >> 1] & 0xF00) != 2048)
          return;
      }
      ClearProjectile(v5);
    }
  }
}

static void SuperMissileBlockCollDetect_X(void) {  // 0x90B406
  int v0 = projectile_index >> 1;
  if ((projectile_type[v0] & 0xF00) == 512 || (projectile_type[v0] & 0xF00) == 2048) {
    uint8 v5 = projectile_variables[v0];
    if (HI8(projectile_variables[v0])) {
      uint16 v1 = abs16n(projectile_bomb_x_speed[v0]) & 0xFF00;
      if (sign16(v1 - 2816)) {
        int v4 = projectile_index >> 1;
        if ((projectile_type[v4] & 0xF00) != 2048) {
          projectile_x_pos[v5 >> 1] = projectile_x_pos[v4];
          return;
        }
      } else {
        uint16 r18 = (v1 >> 8) - 10;
        if (!(projectile_bomb_x_speed[v0] & 0x8000)) {
          uint16 v6 = projectile_index;
          projectile_x_pos[v5 >> 1] = projectile_x_pos[v0] - r18;
          projectile_index = v5;
          BlockCollMissileHoriz(v5);
          projectile_index = v6;
          if ((projectile_type[v6 >> 1] & 0xF00) == 2048)
            ClearProjectile(v5);
          return;
        }
        uint16 v7 = projectile_index;
        projectile_x_pos[v5 >> 1] = r18 + projectile_x_pos[projectile_index >> 1];
        projectile_index = v5;
        BlockCollMissileHoriz(v5);
        projectile_index = v7;
        if ((projectile_type[v7 >> 1] & 0xF00) != 2048)
          return;
      }
      ClearProjectile(v5);
    }
  }
}

static void SuperMissile_ClearLinked(void) {  /* bucle LABEL_7 de $90:AFE5 / $90:B075 */
  for (int i = 8; i >= 0; i -= 2) {
    if ((projectile_type[i >> 1] & 0xFFF) == 512)
      ClearProjectile(i);
  }
}

static void ProjPreInstr_SuperMissile(uint16 k) {  // 0x90AFE5
  int v1 = k >> 1;
  if (projectile_dir[v1] & 0xF0) {
    ClearProjectile(k);
    SuperMissile_ClearLinked();
    return;
  }
  if (projectile_timers[v1]-- == 1) {
    projectile_timers[v1] = 2;
    SpawnProjectileTrail(k);
    k = projectile_index;
  }
  int v3 = projectile_dir[k >> 1] & 0xF;
  Missile_Func1(k);
  switch (v3) {
  case 0: case 4: case 5: case 9:                       // ProjPreInstr_SuperMissile_Func0459
    BlockCollMissileVert(projectile_index);
    SuperMissileBlockCollDetect_Y();
    break;
  case 1: case 3: case 6: case 8: {                     // ProjPreInstr_SuperMissile_Func1368
    uint16 v = projectile_index;
    if (BlockCollMissileHoriz(projectile_index)) {
      SuperMissileBlockCollDetect_X();
    } else {
      SuperMissileBlockCollDetect_X();
      BlockCollMissileVert(v);
      SuperMissileBlockCollDetect_Y();
    }
    break;
  }
  default:                                              // ProjPreInstr_SuperMissile_Func27
    BlockCollMissileHoriz(projectile_index);
    SuperMissileBlockCollDetect_X();
    break;
  }
  if (DeleteProjectileIfFarOffScreen() & 1)
    SuperMissile_ClearLinked();
}

/* Reflexión de misil/super misil/rayo (pirata dorado, $90:BE00) */
void ProjectileReflection(uint16 r20) {  // 0x90BE00
  uint16 v0 = r20;
  int v1 = r20 >> 1;
  uint16 v2 = projectile_type[v1];
  if (v2 & 0x100) {
    InitializeProjectile(r20);
    projectile_bomb_pre_instructions[v1] = FUNC16(ProjPreInstr_Missile);
    projectile_variables[v1] = 240;
  } else if (v2 & 0x200) {
    uint16 k = r20;
    ClearProjectile(LO8(projectile_variables[v1]));
    InitializeProjectile(k);
    int v3 = k >> 1;
    projectile_bomb_pre_instructions[v3] = FUNC16(ProjPreInstr_SuperMissile);
    projectile_variables[v3] = 240;
  } else {
    SetInitialProjectileSpeed(r20);
    InitializeProjectile(v0);
    projectile_bomb_pre_instructions[v1] = kFireChargedBeam_Funcs[projectile_type[v1] & 0xF];
  }
}

static void ProjPreInstr_Func1(uint16 k) {  // 0x90B075
  if (projectile_dir[k >> 1] & 0xF0) {
    ClearProjectile(k);
    SuperMissile_ClearLinked();
  }
}

/* ------------------------------------------------------------------ proyectiles en vuelo ($90:AECE) */
static uint8 DeleteProjectileIfFarOffScreen(void) {  // 0x90B16A
  int16 v0 = projectile_x_pos[projectile_index >> 1] - layer1_x_pos;
  if (!sign16(v0 + 64) && sign16(v0 - 320)) {
    int16 v2 = projectile_y_pos[projectile_index >> 1] - layer1_y_pos;
    if (!sign16(v2 + 64) && sign16(v2 - 320))
      return 0;
  }
  ClearProjectile(projectile_index);
  return 1;
}

static void ProjPreInstr_Beam_NoWaveBeam(uint16 k) {  // 0x90AEF3
  static const int16 kDirToVelMult16_X[10] = { 0, 16, 16, 16, 0, 0, -16, -16, -16, 0 };
  static const int16 kDirToVelMult16_Y[10] = { -16, -16, 0, 16, 16, 16, 16, 0, -16, -16 };
  int v1 = k >> 1;
  if (projectile_dir[v1] & 0xF0) {
    ClearProjectile(k);
    return;
  }
  if (projectile_timers[v1]-- == 1) {
    projectile_timers[v1] = 4;
    SpawnProjectileTrail(k);
    k = projectile_index;
  }
  int v3 = k >> 1;
  int v5 = projectile_dir[v3] & 0xF;
  projectile_bomb_x_speed[v3] += kDirToVelMult16_X[v5];
  projectile_bomb_y_speed[v3] += kDirToVelMult16_Y[v5];
  switch (v5) {
  case 0: case 4: case 5: case 9:                       // ProjPreInstr_Dir0459
    BlockCollNoWaveBeamVert(projectile_index);
    break;
  case 1: case 3: case 6: case 8: {                     // ProjPreInstr_Dir1368
    uint16 v = projectile_index;
    if (!(BlockCollNoWaveBeamHoriz(projectile_index) & 1))
      BlockCollNoWaveBeamVert(v);
    break;
  }
  default:                                              // ProjPreInstr_Dir27
    BlockCollNoWaveBeamHoriz(projectile_index);
    break;
  }
  DeleteProjectileIfFarOffScreen();
}

static void CallProjPreInstr(uint16 f, uint16 k) {
  switch (f) {
  case FUNC16(ProjPreInstr_Empty): return;
  case FUNC16(ProjPreInstr_Beam_NoWaveBeam): ProjPreInstr_Beam_NoWaveBeam(k); return;
  case FUNC16(ProjPreInstr_Missile): ProjPreInstr_Missile(k); return;
  case FUNC16(ProjPreInstr_SuperMissile): ProjPreInstr_SuperMissile(k); return;
  case FUNC16(ProjPreInstr_Func1): ProjPreInstr_Func1(k); return;
  case FUNC16(ProjPreInstr_Bomb): ProjPreInstr_Bomb(k); return;
  case FUNC16(ProjPreInstr_SpreadBomb): ProjPreInstr_SpreadBomb(k); return;
  default: nt_unimpl_v("CallProjPreInstr (proyectil sin portar)", f); return;
  }
}

void HandleProjectile(void) {  // 0x90AECE
  projectile_index = 18;
  for (int i = 18; i >= 0; projectile_index = i) {
    int v1 = i >> 1;
    if (projectile_bomb_instruction_ptr[v1]) {
      CallProjPreInstr(projectile_bomb_pre_instructions[v1], i);
      RunProjectileInstructions();
      i = projectile_index;
    }
    i -= 2;
  }
}

void KillProjectile(uint16 k) {  // 0x90AE06
  int v1 = k >> 1;
  if (projectile_type[v1] & 0xF00) {
    if (!sign16((HI8(projectile_type[v1]) & 0xF) - 3)) {
      ClearProjectile(k);
      return;
    }
  } else {
    switch (projectile_dir[v1] & 0xF) {
    case 0: case 9: projectile_y_pos[v1] -= projectile_y_radius[v1]; break;
    case 1: projectile_x_pos[v1] += projectile_x_radius[v1]; projectile_y_pos[v1] -= projectile_y_radius[v1]; break;
    case 2: projectile_x_pos[v1] += projectile_x_radius[v1]; break;
    case 3: projectile_x_pos[v1] += projectile_x_radius[v1]; projectile_y_pos[v1] += projectile_y_radius[v1]; break;
    case 4: case 5: projectile_y_pos[v1] += projectile_y_radius[v1]; break;
    case 6: projectile_x_pos[v1] -= projectile_x_radius[v1]; projectile_y_pos[v1] += projectile_y_radius[v1]; break;
    case 7: projectile_x_pos[v1] -= projectile_x_radius[v1]; break;
    case 8: projectile_x_pos[v1] -= projectile_x_radius[v1]; projectile_y_pos[v1] -= projectile_y_radius[v1]; break;
    default: nt_unimpl("KillProjectile (dirección)"); break;
    }
  }
  KillProjectileInner(k);
  projectile_bomb_pre_instructions[k >> 1] = FUNC16(ProjPreInstr_Empty);
}

/* explosiones de rayos y misiles (tipo 0x300 en adelante: la del rayo al chocar con una pared o un enemigo) */
void DrawBombAndProjectileExplosions(void) {  // 0x93834D
  for (int16 v0 = 18; v0 >= 0; v0 -= 2) {
    projectile_index = v0;
    int v1 = v0 >> 1;
    uint16 t = projectile_type[v1] & 0xF00;
    if (!projectile_bomb_instruction_ptr[v1] || (int16)(t - 0x300) < 0)
      continue;
    if (t == 0x300 && !projectile_variables[v1])
      continue;
    uint16 r20, r18;
    if (t != 0x500 && t != 0x300 && (ceres_status & 0x8000)) {
      uint16 x = projectile_x_pos[v1], y = projectile_y_pos[v1];   // CalcExplosion_Mode7
      Mode7Rotate(&x, &y);
      r20 = x - layer1_x_pos;
      r18 = y - layer1_y_pos;
    } else {
      r20 = projectile_x_pos[v1] - layer1_x_pos;
      if (!sign16(r20 - 304) || sign16(r20 + 48))
        continue;
      r18 = projectile_y_pos[v1] - layer1_y_pos;
    }
    if (!(r18 & 0xFF00))
      DrawProjectileSpritemap(v0, r20, r18);
    v0 = projectile_index;
  }
  projectile_index = 0xFFFE;
}

void DrawPlayerExplosions2(void) {  // 0x938254
  uint16 v0 = 8;
  projectile_index = 8;
  do {
    int v1 = v0 >> 1;
    if (projectile_bomb_instruction_ptr[v1]) {
      uint16 v2 = projectile_type[v1];
      bool skip;
      if (v2 & (0xF00 | 0x10))
        skip = !sign16((v2 & 0xF00) - 768);
      else if (v2 & 0xC)
        skip = (v0 & 2) ? !(nmi_frame_counter_word & 2) : (nmi_frame_counter_word & 2) != 0;
      else if (v0 & 2)
        skip = (nmi_frame_counter_word & 1) != 0;
      else
        skip = !(nmi_frame_counter_word & 1);
      if (!skip) {
        uint16 x = projectile_x_pos[v1], y = projectile_y_pos[v1];
        if (ceres_status & 0x8000)
          Mode7Rotate(&x, &y);                      // CalcExplosion_Mode7
        uint16 r20 = x - layer1_x_pos;
        uint16 r18 = y - layer1_y_pos;
        if (!(r18 & 0xFF00) && (projectile_spritemap_pointers[v1] & 0x8000))
          DrawProjectileSpritemap(v0, r20, r18);
        v0 = projectile_index;
      }
    }
    v0 -= 2;
    projectile_index = v0;
  } while (!(v0 & 0x8000));
  if (nmi_frame_counter_word & 1) {       // Samus_DrawShinesparkCrashEchoProjectiles
    if (speed_echo_xspeed[3] || speed_echo_xspeed[2])
      nt_unimpl("Samus_DrawShinesparkCrashEchoProjectiles");
  }
  HandleProjectileTrails();
}
