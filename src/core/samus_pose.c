/* MegaMetroid — motor nativo: entrada, poses y transiciones de Samus (banco $91 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_91.c (y las funciones de $90 que solo usa esta parte). */
#include "samus.h"

/* tablas de punteros de las paletas especiales en el banco $91 (sm_91.c: RomFixedPtr) */
#define kSamusPalette_HyperBeam 0xD829
#define kSamus_SpeedBoostingPalettes 0xD998
#define kSamus_HyperBeamPalettes 0xD99E
#define kSamusPal_ScrewAttack 0xDA4A
#define kSamusPal_SpeedBoost 0xDAA9
#define kSamusPal_SpeedBoostShine 0xDB10
#define kSamusPal_Shinespark 0xDB75
#define kStru_91DC00 0xDC00   /* SamusCrystalFlashPalTable: { timer, ptr } */
#define kOff_91DC28 0xDC28

static bool in_liquid_at(uint16 y) {
  if (!(fx_y_pos & 0x8000))
    return sign16(fx_y_pos - y) && !(fx_liquid_options & 4);
  return !(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - y);
}

/* ------------------------------------------------------------------ entrada ($91:8000) */
typedef struct { uint16 r18, r20; } Pair_R18_R20;

static Pair_R18_R20 TranslateCustomControllerBindingsToDefault(void) {  // 0x9181F4
  uint16 dirs = kButton_Up | kButton_Down | kButton_Left | kButton_Right;
  uint16 r18 = joypad1_newkeys & dirs, r20 = joypad1_lastkeys & dirs;
  uint16 v0 = joypad1_newkeys;
  if (button_config_shoot_x & v0) r18 |= kButton_X;
  if (button_config_jump_a & v0) r18 |= kButton_A;
  if (button_config_run_b & v0) r18 |= kButton_B;
  if (button_config_itemcancel_y & v0) r18 |= kButton_Y;
  if ((button_config_aim_up_R & v0) && (button_config_aim_up_R & (kButton_L | kButton_R))) r18 |= kButton_R;
  if ((button_config_aim_down_L & v0) && (button_config_aim_down_L & (kButton_L | kButton_R))) r18 |= kButton_L;
  uint16 v1 = joypad1_lastkeys;
  if (button_config_shoot_x & v1) r20 |= kButton_X;
  if (button_config_jump_a & v1) r20 |= kButton_A;
  if (button_config_run_b & v1) r20 |= kButton_B;
  if (button_config_itemcancel_y & v1) r20 |= kButton_Y;
  if ((button_config_aim_up_R & v1) && (button_config_aim_up_R & (kButton_L | kButton_R))) r20 |= kButton_R;
  if ((button_config_aim_down_L & v1) && (button_config_aim_down_L & (kButton_L | kButton_R))) r20 |= kButton_L;
  return (Pair_R18_R20){ (uint16)~r18, (uint16)~r20 };
}

static uint8 Samus_Pose_Func2(void) {  // 0x918304
  static const uint8 kTab[28] = { 2, 1, 1, 0, 6, 2, 8, 2, 1, 6, 2, 2, 2, 6, 2, 2, 2, 6, 6, 6, 6, 2, 6, 2, 2, 2, 2, 2 };
  uint16 v0 = kTab[samus_movement_type];
  if (v0 == 1) {
    if (samus_x_base_speed || samus_x_base_subspeed) {
      samus_momentum_routine_index = 1;
      return 1;
    }
    v0 = 2;
  }
  samus_momentum_routine_index = v0;
  return 0;
}

static void Samus_Pose_CancelGrapple(void) {  // 0x9182D9
  if ((Samus_Pose_Func2() & 1) || kPoseParams[samus_pose].new_pose_unless_buttons == 255)
    samus_new_pose = samus_pose;
  else
    samus_new_pose = kPoseParams[samus_pose].new_pose_unless_buttons;
}

void Samus_LookupTransitionTable(void) {  // 0x9181A9
  if (joypad1_lastkeys) {
    Pair_R18_R20 pair = TranslateCustomControllerBindingsToDefault();
    const PoseEntry *pe = &kPoseTransitions[kPoseTransitionIndex[samus_pose]];
    if (pe->new_input == 0xFFFF)
      return;
    do {
      if (!(pair.r18 & pe->new_input) && !(pair.r20 & pe->cur_input)) {
        if (pe->new_pose != samus_pose) {
          samus_new_pose = pe->new_pose;
          bomb_jump_dir = 0;
        }
        return;
      }
      pe++;
    } while (pe->new_input != 0xFFFF);
  }
  UNUSED_word_7E0A18 = 0;
  Samus_Pose_CancelGrapple();
}

void Samus_InputHandler(void) {  // 0x918000
  switch (samus_movement_type) {
  case 0:                                     // 0x91804D
    if ((samus_pose && samus_pose != kPose_9B_FaceF_VariaGravitySuit) || !elevator_status)
      Samus_LookupTransitionTable();
    break;
  case 5:                                     // 0x918087
    if (time_is_frozen_flag) {
      Samus_Func20();
    } else {
      Samus_LookupTransitionTable();
      if (!samus_movement_type) {
        samus_y_pos -= 5;
        samus_prev_y_pos -= 5;
      }
    }
    break;
  case 7: case 9: case 11: case 15:
    break;
  case 10:                                    // 0x918113
    Samus_LookupTransitionTable();
    if (samus_movement_type != kMovementType_0A_KnockbackOrCrystalFlashEnding) {
      Samus_InitJump();
      samus_knockback_timer = 0;
    }
    break;
  case 21:                                    // 0x91816F
    if (time_is_frozen_flag)
      Samus_Func20();
    else
      Samus_LookupTransitionTable();
    break;
  default:
    Samus_LookupTransitionTable();
    break;
  }
}

/* ------------------------------------------------------------------ paletas ($91:D6F7) */
void CopyToSamusSuitPalette(uint16 k) {  // 0x91DD5B
  memcpy(&palette_buffer[192], bank9B_ptr(k), 32);
  nt_pal_dirty(192, 207);
}

static void CopyToSamusSuitTargetPalette(uint16 k) {  // 0x91DDD7
  memcpy(&target_palettes[192], bank9B_ptr(k), 32);
}

void Samus_LoadSuitPalette(void) {  // 0x91DEBA
  if (equipped_items & 0x20)
    CopyToSamusSuitPalette(addr_kSamusPalette_GravitySuit);
  else if (equipped_items & 1)
    CopyToSamusSuitPalette(addr_kSamusPalette_VariaSuit);
  else
    CopyToSamusSuitPalette(addr_kSamusPalette_PowerSuit);
}

void Samus_LoadSuitTargetPalette(void) {  // 0x91DEE6
  if (equipped_items & 0x20)
    CopyToSamusSuitTargetPalette(addr_kSamusPalette_GravitySuit);
  else if (equipped_items & 1)
    CopyToSamusSuitTargetPalette(addr_kSamusPalette_VariaSuit);
  else
    CopyToSamusSuitTargetPalette(addr_kSamusPalette_PowerSuit);
}

void Samus_CancelSpeedBoost(void) {  // 0x91DE53
  if (samus_has_momentum_flag) {
    samus_has_momentum_flag = 0;
    speed_boost_counter = 0;
    special_samus_palette_frame = 0;
    special_samus_palette_timer = 0;
    Samus_LoadSuitPalette();
  }
  if (!(speed_echoes_index & 0x8000)) {
    speed_echoes_index = 0xFFFF;
    if (samus_pose_x_dir == 4) {
      speed_echo_xspeed[0] = (uint16)-8;
      speed_echo_xspeed[1] = (uint16)-8;
    } else {
      speed_echo_xspeed[0] = 8;
      speed_echo_xspeed[1] = 8;
    }
  }
}

static uint8 HandleVisorPalette(void) {  // 0x91D83F
  if (timer_for_shine_timer == 8)
    return 0;
  if (fx_layer_blending_config_a == 40 || fx_layer_blending_config_a == 42) {
    uint16 v1 = samus_visor_palette_timer_index - 1;
    samus_visor_palette_timer_index = v1;
    if ((uint8)v1)
      return 0;
    samus_visor_palette_timer_index = v1 | 5;
    palette_buffer[196] = word_9BA3C0[HI8(v1) >> 1];
    nt_pal_dirty(196, 196);
    int v2 = HI8(v1) + 2;
    if (sign16(v2 - 12))
      samus_visor_palette_timer_index = (uint16)(v2 << 8) | (uint8)samus_visor_palette_timer_index;
    else
      samus_visor_palette_timer_index = (uint8)samus_visor_palette_timer_index | 0x600;
    return 0;
  }
  samus_visor_palette_timer_index = 1537;
  return 0;
}

static uint8 HandleBeamChargePalettes(void) {  // 0x91D743
  if (charged_shot_glow_timer) {
    if (hyper_beam_flag) {
      if (!(charged_shot_glow_timer & 1)) {
        if (!(charged_shot_glow_timer & 0x1E)) {
          charged_shot_glow_timer = 0;
          return 1;
        }
        CopyToSamusSuitPalette(bank91_word(kSamusPalette_HyperBeam + (charged_shot_glow_timer & 0x1E)));
      }
      --charged_shot_glow_timer;
      return 0;
    }
    if (--charged_shot_glow_timer) {
      for (int i = 28; i >= 0; i -= 2)
        palette_buffer[(i >> 1) + 193] = 1023;
      nt_pal_dirty(193, 207);
      return 0;
    }
    return 1;
  }
  if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive) && flare_counter && !sign16(flare_counter - 60)) {
    uint16 R36 = (samus_contact_damage_index == 4) ? kSamusPalette_PseudoScrew[samus_suit_palette_index >> 1]
                                                   : kSamusPalette_NonPseudoScrew[samus_suit_palette_index >> 1];
    CopyToSamusSuitPalette(bank91_word(R36 + samus_charge_palette_index));
    uint16 v1 = samus_charge_palette_index + 2;
    if (!sign16(samus_charge_palette_index - 10))
      v1 = 0;
    samus_charge_palette_index = v1;
    return 0;
  }
  samus_charge_palette_index = 0;
  return HandleVisorPalette();
}

static uint8 Samus_HandleScrewAttackSpeedBoostingPals(void) {  // 0x91D9B2
  if (!(samus_suit_palette_index & 4) && in_liquid_at(Samus_GetTop_R20()))
    return 1;
  if (samus_movement_type == kMovementType_03_SpinJumping) {
    if (!(equipped_items & 8))
      goto speed;
    if (samus_anim_frame) {
      if (!sign16(samus_anim_frame - 27))
        return 0;
      goto screw;
    }
    special_samus_palette_frame = 0;
    return 1;
  }
  if (samus_movement_type == kMovementType_14_WallJumping) {
    if (!(equipped_items & 8))
      return 1;
    if (!sign16(samus_anim_frame - 3))
      goto screw;
    special_samus_palette_frame = 0;
    return 1;
  }
speed:
  if ((speed_boost_counter & 0xFF00) != 1024)
    return 1;
  if (!--special_samus_palette_timer || (special_samus_palette_timer & 0x8000)) {
    special_samus_palette_timer = 4;
    /* el SNES no limita el cuadro: si venía del tornillo (hasta 10) lee lo que sigue a la tabla */
    CopyToSamusSuitPalette(bank91_word(bank91_word(kSamusPal_SpeedBoost + (samus_suit_palette_index & ~1)) + special_samus_palette_frame));
    special_samus_palette_frame = (special_samus_palette_frame >= 6) ? 6 : special_samus_palette_frame + 2;
  }
  return 1;
screw:
  CopyToSamusSuitPalette(bank91_word(bank91_word(kSamusPal_ScrewAttack + (samus_suit_palette_index & ~1)) + special_samus_palette_frame));
  special_samus_palette_frame = (special_samus_palette_frame >= 10) ? 0 : special_samus_palette_frame + 2;
  return 1;
}

static uint8 Samus_SpeedBoosterShinePals(void) {  // 0x91DAC7
  uint16 v0 = samus_shine_timer;
  if (samus_shine_timer == 170)
    QueueSfx3_Max9(0xC);
  samus_shine_timer = v0 - 1;
  if ((int16)(v0 - 1) <= 0) {
    special_samus_palette_frame = 0;
    timer_for_shine_timer = 0;
    return 0;
  }
  CopyToSamusSuitPalette(bank91_word(bank91_word(kSamusPal_SpeedBoostShine + (samus_suit_palette_index & ~1)) + special_samus_palette_frame));
  special_samus_palette_frame = !sign16(special_samus_palette_frame - 10) ? 0 : special_samus_palette_frame + 2;
  return 1;
}

static uint8 Samus_HandleShinesparkingPals(void) {  // 0x91DB3A
  if (!--samus_shine_timer || (samus_shine_timer & 0x8000)) {
    timer_for_shine_timer = 0;
    special_samus_palette_frame = 0;
    return 0;
  }
  CopyToSamusSuitPalette(bank91_word(bank91_word(kSamusPal_Shinespark + (samus_suit_palette_index & ~1)) + special_samus_palette_frame));
  special_samus_palette_frame = !sign16(special_samus_palette_frame - 6) ? 0 : special_samus_palette_frame + 2;
  return 1;
}

void Samus_Copy10PalColors(uint16 v0) {  // 0x91DC34
  memcpy(&palette_buffer[224], bank9B_ptr(v0), 20);
  nt_pal_dirty(224, 233);
}

void Samus_Copy6PalColors(uint16 j) {  // 0x91DC82
  memcpy(&palette_buffer[234], bank9B_ptr(j), 12);
  nt_pal_dirty(234, 239);
}

static uint8 Samus_HandleCrystalFlashPals(void) {  // 0x91DB93
  if (samus_shine_timer & 0x8000) {
    WriteBeamPalette_A(equipped_beams);
    timer_for_shine_timer = 0;
    special_samus_palette_frame = 0;
    special_samus_palette_timer = 0;
    samus_shine_timer = 0;
    return 0;
  }
  if ((int16)--samus_shine_timer <= 0) {
    samus_shine_timer = 5;
    Samus_Copy6PalColors(bank91_word(kOff_91DC28 + special_samus_palette_frame));
    special_samus_palette_frame = !sign16(special_samus_palette_frame - 10) ? 0 : special_samus_palette_frame + 2;
  }
  /* suit_pickup_color_math_B y el byte siguiente (suit_pickup_palette_transition_color): contador de 16 bits */
  uint16 t = suit_pickup_color_math_B | suit_pickup_palette_transition_color << 8;
  t--;
  if (!t || (t & 0x8000)) {
    t = bank91_word(kStru_91DC00 + special_samus_palette_timer);              // .timer
    Samus_Copy10PalColors(bank91_word(kStru_91DC00 + 2 + special_samus_palette_timer));   // .ptr
    special_samus_palette_timer = !sign16(special_samus_palette_timer - 36) ? 0 : special_samus_palette_timer + 4;
  }
  suit_pickup_color_math_B = (uint8)t;
  suit_pickup_palette_transition_color = t >> 8;
  return 1;
}

static uint8 Samus_HandleXrayPals(void) {  // 0x91DCB4
  if (demo_timer_counter & 0x8000) {
    timer_for_shine_timer = 0;
    special_samus_palette_frame = 0;
    special_samus_palette_timer = 0;
    demo_timer_counter = 0;
    return 0;
  }
  if (!demo_timer_counter) {
    if (sign16(demo_input_pre_instr - 2)) {
      if (!--special_samus_palette_timer || (special_samus_palette_timer & 0x8000)) {
        special_samus_palette_timer = 5;
        palette_buffer[196] = word_9BA3C0[special_samus_palette_frame >> 1];
        nt_pal_dirty(196, 196);
        if (sign16(special_samus_palette_frame - 4))
          special_samus_palette_frame += 2;
      }
      return 1;
    }
    special_samus_palette_frame = 6;
    special_samus_palette_timer = 1;
    demo_timer_counter = 1;
  }
  if (--special_samus_palette_timer && !(special_samus_palette_timer & 0x8000))
    return 1;
  special_samus_palette_timer = 5;
  palette_buffer[196] = word_9BA3C0[special_samus_palette_frame >> 1];
  nt_pal_dirty(196, 196);
  special_samus_palette_frame = !sign16(special_samus_palette_frame - 10) ? 6 : special_samus_palette_frame + 2;
  return 1;
}

static void HandleMiscSamusPalette(void) {  // 0x91D8A5
  if (!samus_special_super_palette_flags) {
    uint16 v0 = samus_hurt_flash_counter;
    if (!samus_hurt_flash_counter)
      return;
    bool load = false;
    if (samus_hurt_flash_counter == 2) {
      if (!cinematic_function && (frame_handler_beta != FUNC16(j_HandleDemoRecorder_2_0) || samus_pose != kPose_54_FaceL_Knockback)) {
        QueueSfx1_Max6(0x35);
        load = true;
      }
      v0 = samus_hurt_flash_counter;
    }
    if (!load && sign16(v0 - 7)) {
      if (v0 & 1)
        CopyToSamusSuitPalette(addr_word_9BA380);
      else
        load = true;
    }
    if (load) {
      if (cinematic_function)
        CopyToSamusSuitPalette(addr_word_9BA3A0);
      else
        Samus_LoadSuitPalette();
    }
    uint16 v1 = samus_hurt_flash_counter + 1;
    samus_hurt_flash_counter = v1;
    if (v1 == 40) {
      if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive)) {
        if (samus_movement_type == kMovementType_03_SpinJumping || samus_movement_type == kMovementType_14_WallJumping)
          CallSomeSamusCode(0x1C);
        else if (!sign16(flare_counter - 16) && (button_config_shoot_x & joypad1_lastkeys))
          play_resume_charging_beam_sfx = 1;
      } else {
        nt_unimpl("HandleMiscSamusPalette (gancho)");
      }
    } else if (!sign16(v1 - 60)) {
      samus_hurt_flash_counter = 0;
    }
    return;
  }
  if (samus_special_super_palette_flags & 0x8000) {    /* hiperrayo (Mother Brain) */
    CopyToSamusSuitPalette(bank91_word(kSamus_HyperBeamPalettes + 2 * samus_charge_palette_index));
    if (!--special_samus_palette_timer || (special_samus_palette_timer & 0x8000)) {
      special_samus_palette_timer = special_samus_palette_frame;
      if (!sign16(++samus_charge_palette_index - 10))
        samus_charge_palette_index = 0;
    }
  } else {
    if (samus_special_super_palette_flags & 1)
      CopyToSamusSuitPalette(bank91_word(kSamus_SpeedBoostingPalettes + (samus_suit_palette_index & ~1)));
    else
      Samus_LoadSuitPalette();
    ++samus_special_super_palette_flags;
  }
}

void Samus_HandlePalette(void) {  // 0x91D6F7
  if (!(samus_special_super_palette_flags & 0x8000)) {
    bool copy;
    if (HandleBeamChargePalettes() & 1) {
      copy = true;
    } else {
      uint8 r;
      switch (timer_for_shine_timer) {
      case 0: r = Samus_HandleScrewAttackSpeedBoostingPals(); break;
      case 1: r = Samus_SpeedBoosterShinePals(); break;
      case 2: HandleMiscSamusPalette(); r = 0; break;   /* (Func_U8 *) en el decomp: el carry queda en 0 */
      case 6: r = Samus_HandleShinesparkingPals(); break;
      case 7: r = Samus_HandleCrystalFlashPals(); break;
      case 8: r = Samus_HandleXrayPals(); break;
      case 9: r = HandleVisorPalette(); break;
      case 10: r = 0; break;                           // nullsub_164
      default: nt_unimpl("Samus_HandlePalette (índice 3-5: puntero nulo en el SNES)"); r = 1; break;
      }
      copy = !(r & 1);
    }
    if (copy)
      CopyToSamusSuitPalette(off_91D727[samus_suit_palette_index >> 1]);
  }
  HandleMiscSamusPalette();
}

/* ------------------------------------------------------------------ transiciones por colisión ($91:E8B6) */
static uint8 Samus_HandleTransFromBlockColl_1_0(void) {  // 0x91E95D
  if (samus_prev_movement_type2 == kMovementType_03_SpinJumping || samus_prev_movement_type2 == kMovementType_14_WallJumping) {
    samus_new_pose = (samus_pose_x_dir == 4) ? kPose_A7_FaceL_LandSpinJump : kPose_A6_FaceR_LandSpinJump;
    return 0;
  }
  uint16 v0 = kPoseParams[samus_pose].direction_shots_fired;
  if (v0 == 255) {
    samus_new_pose = (samus_pose_x_dir == 4) ? kPose_A5_FaceL_LandJump : kPose_A4_FaceR_LandJump;
    return 0;
  }
  if ((v0 == 2 || v0 == 7) && (button_config_shoot_x & joypad1_lastkeys)) {
    samus_new_pose = (samus_pose_x_dir == 4) ? kPose_E7_FaceL_LandJump_Fire : kPose_E6_FaceR_LandJump_Fire;
    return 0;
  }
  samus_new_pose = word_91E9F3[v0];
  return 0;
}

/* morfosfera al tocar el suelo ($91:EA07..$91:EA63, $91:F1FC..$91:F2CE) */
static const uint16 g_word_909EB5 = 1;   /* como el decomp (sm_91.c) */
static const uint16 g_word_909EB7 = 0;
static uint8 Samus_HandleTransFromBlockColl_1_1(void) {  // 0x91EA07
  uint16 v0 = 2 * used_for_ball_bounce_on_landing;
  if (v0) {
    if (v0 == 2) { samus_new_pose = samus_pose; return 0; }
    if (v0 != 4) nt_unimpl("Samus_HandleTransFromBlockColl_1_1 (Unreachable)");
  } else if (!sign16(samus_y_speed - 3)) {
    samus_new_pose = samus_pose;
    return 0;
  }
  samus_new_pose = (samus_pose_x_dir == 4) ? kPose_41_FaceL_Morphball_Ground : kPose_1D_FaceR_Morphball_Ground;
  return 0;
}
static uint8 Samus_HandleTransFromBlockColl_1_2(void) {  // 0x91EA48
  samus_new_pose = (samus_pose_x_dir == 4) ? 66 : 32;
  return 0;
}
static uint8 Samus_HandleTransFromBlockColl_1_3(void) {  // 0x91EA63
  if (button_config_jump_a & joypad1_lastkeys) { samus_new_pose = samus_pose; return 0; }
  uint16 v1 = 2 * (uint8)used_for_ball_bounce_on_landing;
  if (v1) {
    if (v1 == 2) { samus_new_pose = samus_pose; return 0; }
    if (v1 != 4) nt_unimpl("Samus_HandleTransFromBlockColl_1_3 (Unreachable)");
  } else if (!sign16(samus_y_speed - 3)) {
    samus_new_pose = samus_pose;
    return 0;
  }
  samus_new_pose = (samus_pose_x_dir == 4) ? kPose_7A_FaceL_Springball_Ground : 121;
  return 0;
}
static uint8 Samus_MorphBallBounceNoSpringballTrans(void) {  // 0x91F1FC
  uint16 v0 = 2 * used_for_ball_bounce_on_landing;
  if (v0) {
    if (v0 == 2) {
      ++used_for_ball_bounce_on_landing;
      samus_y_dir = 1;
      samus_y_subspeed = g_word_909EB7;
      samus_y_speed = g_word_909EB5 - 1;
      return 1;
    }
    if (v0 != 4) nt_unimpl("Samus_MorphBallBounceNoSpringballTrans (Unreachable)");
  } else if (!sign16(samus_y_speed - 3)) {
    ++used_for_ball_bounce_on_landing;
    samus_y_dir = 1;
    samus_y_subspeed = g_word_909EB7;
    samus_y_speed = g_word_909EB5;
    return 1;
  }
  used_for_ball_bounce_on_landing = 0;
  samus_y_dir = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  return 0;
}
static uint8 Samus_MorphBallBounceSpringballTrans(void) {  // 0x91F25E
  if (button_config_jump_a & joypad1_lastkeys) {
    used_for_ball_bounce_on_landing = 0;
    Samus_InitJump();
    return 1;
  }
  uint16 v1 = 2 * (uint8)used_for_ball_bounce_on_landing;
  if (v1) {
    if (v1 == 2) {
      used_for_ball_bounce_on_landing = 1538;
      samus_y_dir = 1;
      samus_y_subspeed = g_word_909EB7;
      samus_y_speed = g_word_909EB5 - 1;
      return 1;
    }
    if (v1 != 4) nt_unimpl("Samus_MorphBallBounceSpringballTrans (Unreachable)");
  } else if (!sign16(samus_y_speed - 3)) {
    used_for_ball_bounce_on_landing = 1537;
    samus_y_dir = 1;
    samus_y_subspeed = g_word_909EB7;
    samus_y_speed = g_word_909EB5;
    return 1;
  }
  used_for_ball_bounce_on_landing = 0;
  samus_y_dir = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  return 0;
}

void Samus_HandleTransFromBlockColl(void) {  // 0x91E8B6
  if (!input_to_pose_calc)
    return;
  uint8 hi = HI8(input_to_pose_calc);
  switch ((uint8)input_to_pose_calc) {
  case 1:                                     // 0x91E931
    if (hi != 4) {
      uint8 r;
      switch (hi) {
      case 0: r = Samus_HandleTransFromBlockColl_1_0(); break;
      case 1: r = Samus_HandleTransFromBlockColl_1_1(); break;
      case 2: r = Samus_HandleTransFromBlockColl_1_2(); break;
      case 3: r = Samus_HandleTransFromBlockColl_1_3(); break;
      case 5: samus_new_pose = samus_pose; r = 0; break;
      default: nt_unimpl("Samus_HandleTransFromBlockColl_1 (índice)"); r = 0; break;
      }
      samus_momentum_routine_index = (r & 1) ? 0 : 5;
    }
    break;
  case 2:                                     // 0x91E8FE
    if (hi != 4) {
      uint16 v0 = 4 * hi;
      samus_new_pose = (samus_pose_x_dir == 4) ? word_91E921[(v0 >> 1) + 1] : word_91E921[v0 >> 1];
      samus_momentum_routine_index = 5;
    }
    break;
  case 3: case 4:                             // 0x91E8D8 / 0x91E8E5
    samus_new_pose = samus_pose;
    samus_momentum_routine_index = 5;
    break;
  case 5:                                     // 0x91EABE
    samus_new_pose = (samus_pose_x_dir == 4) ? kPose_84_FaceL_Walljump : kPose_83_FaceR_Walljump;
    samus_momentum_routine_index = 5;
    break;
  default:
    break;
  }
}

static void Samus_CheckWalkedIntoSomething(void) {  // 0x91EADE
  if (samus_collides_with_solid_enemy && samus_movement_type == 1) {
    samus_new_pose = word_91EB74[kPoseParams[samus_pose].direction_shots_fired];
    samus_collides_with_solid_enemy = 0;
    return;
  }
  if (samus_new_pose == 0xFFFF || kPoseParams[samus_new_pose].movement_type != 1) {
    samus_collides_with_solid_enemy = 0;
    return;
  }
  int32 amt;
  if (samus_pose_x_dir != 4) {
    samus_collision_direction = 1;
    if (Samus_CheckSolidEnemyColl(SHL16(1)).collision)
      goto blocked;
    samus_collision_direction = 1;
    amt = SHL16(1);
  } else {
    samus_collision_direction = 0;
    if (Samus_CheckSolidEnemyColl(SHL16(1)).collision)
      goto blocked;
    amt = SHL16(-1);
    samus_collision_direction = 0;
  }
  Samus_MoveRight_NoSolidColl(amt);
  if (samus_collision_flag)
    goto blocked;
  samus_collides_with_solid_enemy = 0;
  return;
blocked:
  samus_new_pose = word_91EB74[kPoseParams[samus_new_pose].direction_shots_fired];
  samus_collides_with_solid_enemy = 0;
}

/* ------------------------------------------------------------------ transiciones ($91:EB88) */
static void SamusFunc_EC80(void) {  // 0x91FB8E
  if (samus_prev_movement_type2 != kMovementType_06_Falling && samus_movement_type == kMovementType_06_Falling) {
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 2;
  }
}

static void Samus_HandleTransitionsA_2(void) {  // 0x91ECD0
  samus_x_accel_mode = 0;
  Samus_CancelSpeedBoost();
}

static void Samus_HandleTransitionsA_8(void) {  // 0x91EC8E
  Samus_CancelSpeedBoost();
  set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 0);
  SamusFunc_EC80();
}

static void SamusFunc_F1D3(void) {  // 0x91F1D3
  samus_is_falling_flag = 0;
  UNUSED_word_7E0B1A = 0;
  UNUSED_word_7E0B2A = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_y_dir = 0;
  UNUSED_word_7E0B38 = 0;
  used_for_ball_bounce_on_landing = 0;
}

/* polvo, salpicaduras o chispas al caer, según el área ($91:F0A5): Maridia (agua) y Norfair (polvo) */
static int LandingOutOfLiquid(uint16 bottom) {   /* 1: hay que dibujarlo (fuera del líquido o con física normal) */
  if (!(fx_y_pos & 0x8000))
    return !(sign16(fx_y_pos - bottom) && !(fx_liquid_options & 4));
  return (lava_acid_y_pos & 0x8000) || !sign16(lava_acid_y_pos - bottom);
}
static void HandleLandingGraphics_Ceres(void) {  // 0x91F0BE
  atmospheric_gfx_frame_and_type[2] = 0;
  atmospheric_gfx_frame_and_type[3] = 0;
}
static void HandleLandingGraphics_Maridia(void) {  // 0x91F116
  uint16 bottom = Samus_GetBottom_R18();
  if (!LandingOutOfLiquid(bottom))
    return;
  atmospheric_gfx_frame_and_type[2] = atmospheric_gfx_frame_and_type[3] = 256;
  atmospheric_gfx_anim_timer[2] = atmospheric_gfx_anim_timer[3] = 3;
  atmospheric_gfx_x_pos[2] = samus_x_pos + 4;
  atmospheric_gfx_x_pos[3] = samus_x_pos - 3;
  atmospheric_gfx_y_pos[2] = atmospheric_gfx_y_pos[3] = bottom - 4;
}
static void HandleLandingGraphics_Norfair(void) {  // 0x91F166
  uint16 bottom = Samus_GetBottom_R18();
  if (!LandingOutOfLiquid(bottom))
    return;
  atmospheric_gfx_frame_and_type[2] = atmospheric_gfx_frame_and_type[3] = 1536;
  atmospheric_gfx_anim_timer[2] = atmospheric_gfx_anim_timer[3] = 3;
  atmospheric_gfx_x_pos[2] = samus_x_pos + 8;
  atmospheric_gfx_x_pos[3] = samus_x_pos - 8;
  atmospheric_gfx_y_pos[2] = atmospheric_gfx_y_pos[3] = bottom;
}
static void HandleLandingGraphics_Tourian(void) {  // 0x91F1BA
  if (!sign16(room_index - 5) && (sign16(room_index - 9) || room_index == 11))
    HandleLandingGraphics_Norfair();
  else
    HandleLandingGraphics_Ceres();
}
/* salas de Crateria con agua o lluvia (bit 0: lluvia si hay FX de lluvia; 2: agua abajo de y 944; 4: agua) */
static const uint8 kCrateriaWetRooms[17] = { 1, 0, 0, 0, 0, 2, 0, 4, 0, 4, 4, 4, 4, 0, 4, 0, 0 };
static void HandleLandingGraphics(void) {  // 0x91F0A5
  switch (area_index) {
  case 0:                                          // HandleLandingGraphics_Crateria
    if (cinematic_function) { HandleLandingGraphics_Ceres(); return; }
    if (room_index == 28) { HandleLandingGraphics_Norfair(); return; }
    if ((int16)(room_index - 16) >= 0) { HandleLandingGraphics_Ceres(); return; }
    if (kCrateriaWetRooms[room_index] & 1) {
      if (fx_type != 10) HandleLandingGraphics_Ceres(); else HandleLandingGraphics_Maridia();
    } else if (kCrateriaWetRooms[room_index] & 2) {
      if (!sign16(samus_y_pos - 944)) HandleLandingGraphics_Maridia(); else HandleLandingGraphics_Ceres();
    } else if (kCrateriaWetRooms[room_index] & 4) {
      HandleLandingGraphics_Maridia();
    } else {
      HandleLandingGraphics_Ceres();
    }
    return;
  case 1: if (room_index == 8) HandleLandingGraphics_Norfair(); else HandleLandingGraphics_Tourian(); return;  // Brinstar
  case 2: case 3: HandleLandingGraphics_Norfair(); return;
  case 4: HandleLandingGraphics_Maridia(); return;
  case 5: HandleLandingGraphics_Tourian(); return;
  default: HandleLandingGraphics_Ceres(); return;
  }
}

static void HandleLandingSoundEffectsAndGfx(void) {  // 0x91F046
  if ((samus_prev_movement_type2 == kMovementType_03_SpinJumping || samus_prev_movement_type2 == kMovementType_14_WallJumping)
      && !cinematic_function) {
    if (samus_prev_pose == kPose_81_FaceR_Screwattack || samus_prev_pose == kPose_82_FaceL_Screwattack)
      QueueSfx1_Max6(0x34);
    else
      QueueSfx1_Max6(0x32);
  }
  if (samus_y_speed && !sign16(samus_y_speed - 5)) {
    if (!cinematic_function)
      QueueSfx3_Max6(4);
  } else {
    if (!samus_y_subspeed)
      return;
    if (!cinematic_function)
      QueueSfx3_Max6(5);
  }
  HandleLandingGraphics();
}

static void Samus_HandleTransitionsA_5(void) {  // 0x91EFC4
  switch ((uint8)input_to_pose_calc) {
  case 0: case 3:
    break;
  case 1: {                                   // 0x91F010
    HandleLandingSoundEffectsAndGfx();
    uint8 hi = HI8(input_to_pose_calc);
    if (hi == 4) {
      SamusFunc_F1D3();
      break;
    }
    uint8 r;
    switch (hi) {
    case 0:                                   // 0x91F1EC
      if (samus_input_handler != FUNC16(Samus_InputHandler_E91D))
        samus_input_handler = FUNC16(HandleAutoJumpHack);
      r = 0;
      break;
    case 1: r = Samus_MorphBallBounceNoSpringballTrans(); break;
    case 2: used_for_ball_bounce_on_landing = 0; enable_horiz_slope_coll = 3; r = 0; break;   // 0x91F253
    case 3: r = Samus_MorphBallBounceSpringballTrans(); break;
    case 5: used_for_ball_bounce_on_landing = 0; r = 0; break;   // 0x91F2CE
    default: nt_unimpl("Samus_HandleTransitionsA_5_1 (índice)"); r = 0; break;
    }
    if (!(r & 1)) {
      UNUSED_word_7E0A18 = 0;
      samus_x_accel_mode = 0;
      samus_x_base_speed = 0;
      samus_x_base_subspeed = 0;
      SamusFunc_F1D3();
    }
    break;
  }
  case 2:                                     // 0x91EFEF
    if (samus_y_dir != 1) {
      used_for_ball_bounce_on_landing = 0;
      samus_y_subspeed = 0;
      samus_y_speed = 0;
      samus_is_falling_flag = 1;
      samus_y_dir = 2;
    }
    UNUSED_word_7E0A18 = 0;
    break;
  case 4:                                     // 0x91EFDF
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    UNUSED_word_7E0B1A = 0;
    samus_y_dir = 2;
    break;
  case 5:                                     // 0x91F2D3
    samus_x_accel_mode = 0;
    samus_collides_with_solid_enemy = 0;
    samus_is_falling_flag = 0;
    UNUSED_word_7E0B1A = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    UNUSED_word_7E0A18 = 0;
    QueueSfx3_Max6(5);
    break;
  case 6:                                     // 0x91F2F0
    if (samus_collides_with_solid_enemy && samus_prev_movement_type2 == 9) {
      enable_horiz_slope_coll = samus_pose_x_dir != 4;
      UNUSED_word_7E0A18 = 0;
    }
    break;
  }
}

static void Samus_HandleTransitionsA_7(void) {  // 0x91ECDA
  static const uint16 word_91ED36[12] = { 5, 5, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0 };
  int32 amt;
  if (sign16(samus_pose - kPose_DB)) {
    amt = SHL16(word_91ED36[samus_pose - 53]);
    if (amt) {
      samus_y_radius = kPoseParams[samus_pose].y_radius;
      amt = Samus_CollDetectChangedPose(amt);
    }
  } else if (!sign16(samus_pose - kPose_F1_FaceR_CrouchTrans_AimU) && sign16(samus_pose - kPose_F7_FaceR_StandTrans_AimU)) {
    amt = SHL16(5);
    samus_y_radius = kPoseParams[samus_pose].y_radius;
    amt = Samus_CollDetectChangedPose(amt);
  } else {
    return;
  }
  samus_y_pos += (uint16)((uint32)amt >> 16);
  samus_prev_y_pos = samus_y_pos;
  if (used_for_ball_bounce_on_landing) {
    used_for_ball_bounce_on_landing = 0;
    samus_y_subspeed = 0;
    samus_y_speed = 0;
    samus_y_dir = 0;
  }
}

static void Samus_HandleTransitionsA(void) {  // 0x91EB88
  switch (samus_momentum_routine_index) {
  case 0: case 3: case 4: break;
  case 1:                                     // 0x91EC50
    if (samus_x_base_speed || samus_x_base_subspeed) {
      add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(samus_x_extra_run_speed, samus_x_extra_run_subspeed));
      samus_x_accel_mode = 2;
      Samus_CancelSpeedBoost();
      set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 0);
      SamusFunc_EC80();
    } else {
      Samus_HandleTransitionsA_2();
    }
    break;
  case 2: Samus_HandleTransitionsA_2(); break;
  case 5: Samus_HandleTransitionsA_5(); break;
  case 6:                                     // 0x91EC85
    samus_x_accel_mode = 0;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    Samus_HandleTransitionsA_8();
    break;
  case 7: Samus_HandleTransitionsA_7(); break;
  case 8: Samus_HandleTransitionsA_8(); break;
  }
}

static void Samus_HandleTransitionsB_1(void) {  // 0x91ED4E: golpe -> retroceso
  /* kSamus_HandleTransitionsB_1 por tipo de movimiento anterior: 0 (_0), 4 (_4), 6 (_6), 7 (_7), 10/11 (nada) */
  static const uint8 kKind[28] = { 0, 0, 0, 0, 4, 0, 6, 7, 4, 4, 10, 11, 11, 0, 11, 11, 0, 4, 4, 4, 0, 0, 10, 11, 11, 11, 11, 11 };
  uint8 k = kKind[samus_prev_movement_type2];
  if (k == 6 && frame_handler_gamma == FUNC16(Samus_Func9))
    k = 10;
  if (k == 0 || k == 6) {                       // Samus_HandleTransitionsB_1_0
    uint16 dir_key = (samus_pose_x_dir == 4) ? 0x200 : 0x100;
    if (knockback_x_dir)
      knockback_dir = (joypad1_lastkeys & dir_key) ? 5 : 2;
    else
      knockback_dir = (joypad1_lastkeys & dir_key) ? 4 : 1;
    samus_movement_handler = FUNC16(Samus_MoveHandler_Knockback);
  } else if (k == 4 || k == 7) {                // _4 / _7
    knockback_dir = (samus_pose_x_dir == 4) ? 1 : 2;
    samus_movement_handler = FUNC16(Samus_MoveHandler_Knockback);
  }
  /* Samus_SetSpeedForKnockback_Y ($90:99D6) */
  {
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
    static const uint16 kSpeed[3] = { 5, 2, 2 };
    samus_y_subspeed = 0;
    samus_y_speed = kSpeed[v0 >> 1];
    grapple_walljump_timer = 0;
    reached_ceres_elevator_fade_timer = 0;
    samus_y_dir = 1;
  }
  bomb_jump_dir = 0;
  samus_contact_damage_index = 0;
  samus_hurt_flash_counter = 1;
}

static void Samus_HandleTransitionsB(void) {  // kSamus_HandleTransitionsB
  switch (samus_special_transgfx_index) {
  case 0: case 6: case 7: break;
  case 1: Samus_HandleTransitionsB_1(); break;
  case 2:                                     // 0x91EE69
    knockback_dir = 0;
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
    samus_y_speed = 0;
    samus_y_subspeed = 0;
    samus_y_dir = 0;
    Samus_AlignBottomWithPrevPose();
    break;
  case 3:                                     // 0x91EE80
    bomb_jump_dir = (uint8)bomb_jump_dir | 0x800;
    samus_movement_handler = FUNC16(Samus_MoveHandler_BombJumpStart);
    if (samus_input_handler != FUNC16(Samus_InputHandler_E91D))
      samus_input_handler = FUNC16(nullsub_152);
    break;
  case 4: Samus_InitJump(); break;            // 0x91EEA1
  case 8:                                     // 0x91EF3B
    samus_y_pos -= 5;
    samus_prev_y_pos = samus_y_pos;
    frame_handler_alfa = FUNC16(EmptyFunction);
    break;
  default: nt_unimpl("Samus_HandleTransitionsB (daño/bomba/rayos X/gancho)"); break;
  }
}

static void Samus_HandleTransitionsC_1(void) {  // 0x91F31D
  knockback_dir = 0;
  samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
  used_for_ball_bounce_on_landing = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_is_falling_flag = 1;
  samus_y_dir = 2;
  Samus_AlignBottomWithPrevPose();
  if (samus_input_handler != FUNC16(Samus_InputHandler_E91D))
    samus_input_handler = FUNC16(Samus_InputHandler_E913);
}

static void Samus_HandleTransitionsC_3(void) {  // 0x91F36E
  used_for_ball_bounce_on_landing = 0;
  samus_anim_frame_timer += samus_anim_frame_buffer;
}

static void clamp_prev_pos(void) {  // 0x91F3AA
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

static void Samus_HandleTransitionsC(void) {  // kSamus_HandleTransitionsC
  switch (samus_hurt_switch_index) {
  case 0: break;
  case 1: Samus_HandleTransitionsC_1(); break;
  case 2:                                     // 0x91F34E
    Samus_AlignBottomWithPrevPose();
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
    samus_input_handler = (game_state == 42) ? FUNC16(Samus_InputHandler_E91D) : FUNC16(Samus_InputHandler_E913);
    break;
  case 3: Samus_HandleTransitionsC_3(); break;
  case 4:                                     // 0x91F37C
    samus_x_pos = layer1_x_pos + 128;
    samus_prev_x_pos = layer1_x_pos + 128;
    samus_y_pos = layer1_y_pos + 128;
    samus_prev_y_pos = layer1_y_pos + 128;
    break;
  case 5:                                     // 0x91F397
    samus_y_pos += 5;
    samus_prev_y_pos = samus_y_pos;
    break;
  case 6: HandleJumpTransition(); break;
  case 7: clamp_prev_pos(); break;
  case 8: Samus_HandleTransitionsC_1(); Samus_HandleTransitionsC_3(); break;
  }
}

static void HandleCollDueToChangedPose(void);

static uint8 SamusFunc_F404(void) {  // 0x91F404
  uint16 v1 = samus_pose;
  if (samus_pose != samus_prev_pose) {
    HandleCollDueToChangedPose();
    SamusFunc_F433();
    HandleJumpTransition();
    Samus_SetAnimationFrameIfPoseChanged();
    samus_anim_frame_skip = 0;
  }
  return v1 != samus_pose;
}

static void save_prev_pose(void) {
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;
  samus_last_different_pose_movement_type = samus_prev_movement_type2;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type2 = samus_movement_type;
}

void Samus_HandleTransitions(void) {  // 0x91EB88
  if (!(samus_new_pose_transitional & 0x8000)) {
    if (samus_hurt_switch_index == 1) {
      Samus_HandleTransitionsC();
      save_prev_pose();
      goto done;
    }
    if (samus_hurt_switch_index != 3 || samus_special_transgfx_index != 9) {
      samus_pose = samus_new_pose_transitional;
      SamusFunc_F433();
      Samus_SetAnimationFrameIfPoseChanged();
      Samus_HandleTransitionsC();
      save_prev_pose();
      goto done;
    }
  }
  if (!(samus_new_pose_interrupted & 0x8000)) {
    samus_pose = samus_new_pose_interrupted;
    if (!(SamusFunc_F404() & 1))
      Samus_HandleTransitionsB();
    save_prev_pose();
    goto done;
  }
  Samus_CheckWalkedIntoSomething();
  if (!(samus_new_pose & 0x8000)) {
    samus_pose = samus_new_pose;
    if (!(SamusFunc_F404() & 1))
      Samus_HandleTransitionsA();
    save_prev_pose();
  }
done:
  input_to_pose_calc = 0;
}

/* ------------------------------------------------------------------ cambio de pose ($91:F433) */
static void set_pose_x_dir_and_type(void) {
  samus_pose_x_dir = kPoseParams[samus_pose].pose_x_dir;
  samus_movement_type = kPoseParams[samus_pose].movement_type;
}

static uint8 SamusFunc_F468_Standing(void) {  // 0x91F4DC
  uint8 a = kPoseParams[samus_pose].direction_shots_fired, b = kPoseParams[samus_prev_pose].direction_shots_fired;
  if ((!a || a == 9) && (!b || b == 9))
    samus_anim_frame_skip = 1;
  return 0;
}

static uint8 SamusFunc_F468_Running(void) {  // 0x91F50C
  if (samus_prev_movement_type2 == 1)
    samus_anim_frame_skip = FUNC16(Samus_InputHandler);
  if (!UNUSED_word_7E0DF8)
    return 0;
  if (samus_pose == (kPose_44_FaceL_Turn_Crouch | kPose_01_FaceR_Normal)) {
    samus_pose = kPose_25_FaceR_Turn_Stand;
    return 1;
  }
  if (samus_pose == (kPose_44_FaceL_Turn_Crouch | kPose_02_FaceL_Normal)) {
    samus_pose = kPose_26_FaceL_Turn_Stand;
    return 1;
  }
  return 0;
}

static uint8 SamusFunc_F468_NormalJump(void) {  // 0x91F543
  if (samus_pose == kPose_4E_FaceL_Jump_NoAim_NoMove_NoGun || samus_pose == kPose_16_FaceL_Jump_AimU
      || samus_pose == kPose_6A_FaceL_Jump_AimUL || samus_pose == kPose_4D_FaceR_Jump_NoAim_NoMove_NoGun
      || samus_pose == kPose_15_FaceR_Jump_AimU || samus_pose == kPose_69_FaceR_Jump_AimUR) {
    if (samus_shine_timer) {
      nt_unimpl("SamusFunc_F468_NormalJump (shinespark)");
      return 1;
    }
  }
  if (samus_x_extra_run_speed || samus_x_extra_run_subspeed)
    samus_x_accel_mode = 2;
  else
    samus_x_accel_mode = 0;
  if ((samus_pose == kPose_15_FaceR_Jump_AimU || samus_pose == kPose_16_FaceL_Jump_AimU)
      && (samus_prev_pose == kPose_55_FaceR_Jumptrans_AimU || samus_prev_pose == kPose_56_FaceL_Jumptrans_AimU))
    samus_anim_frame_skip = 1;
  if (button_config_shoot_x & joypad1_newkeys)
    new_projectile_direction_changed_pose = kPoseParams[samus_pose].direction_shots_fired | 0x8000;
  return 0;
}

static uint8 SamusFunc_F468_Crouching(void) {  // 0x91F5EB
  if ((samus_pose == kPose_85_FaceR_Crouch_AimU || samus_pose == kPose_86_FaceL_Crouch_AimU)
      && (samus_prev_pose == kPose_F1_FaceR_CrouchTrans_AimU || samus_prev_pose == kPose_F2_FaceL_CrouchTrans_AimU))
    samus_anim_frame_skip = 1;
  return 0;
}

static uint8 SamusFunc_F468_Falling(void) {  // 0x91F60D
  samus_x_accel_mode = (samus_x_extra_run_speed || samus_x_extra_run_subspeed) ? 2 : 0;
  return 0;
}

static uint8 SamusFunc_F468_SpinJump(void) {  // 0x91F624
  if (samus_prev_movement_type2 == kMovementType_03_SpinJumping || samus_prev_movement_type2 == kMovementType_14_WallJumping) {
    samus_anim_frame_skip = 1;
    uint16 cur = samus_pose_x_dir | samus_movement_type << 8;
    if (((samus_prev_pose_x_dir & 0xF) == 8 && cur == 772) || ((samus_prev_pose_x_dir & 0xF) == 4 && cur == 776)) {
      add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(samus_x_extra_run_speed, samus_x_extra_run_subspeed));
      Samus_CancelSpeedBoost();
      set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 0);
      samus_x_accel_mode = 1;
    }
  }
  if (!(equipped_items & 0x20) && in_liquid_at(Samus_GetTop_R20()))
    return 0;
  if (equipped_items & 8) {
    samus_pose = (samus_pose_x_dir == 4) ? kPose_82_FaceL_Screwattack : kPose_81_FaceR_Screwattack;
    if (!samus_anim_frame_skip)
      QueueSfx1_Max6(0x33);
    return 0;
  }
  if (equipped_items & 0x200) {
    QueueSfx1_Max6(0x3E);
    samus_pose = (samus_pose_x_dir == 4) ? kPose_1C_FaceL_SpaceJump : kPose_1B_FaceR_SpaceJump;
    return 0;
  }
  if (!samus_anim_frame_skip && !cinematic_function)
    QueueSfx1_Max6(0x31);
  return 0;
}

static uint8 Samus_CrouchTrans(void) {  // 0x91F7B0
  if (!sign16((speed_boost_counter & 0xFF00) - 1024)) {
    samus_shine_timer = 180;
    timer_for_shine_timer = 1;
    special_samus_palette_frame = 0;
  }
  return 0;
}

static uint8 Samus_MorphTrans(void) {  // 0x91F7CE
  if (equipped_items & 4) {
    if (samus_prev_movement_type2 == kMovementType_03_SpinJumping)
      samus_x_accel_mode = 2;
    bomb_spread_charge_timeout_counter = 0;
    return 0;
  }
  samus_pose = samus_prev_pose;
  return 1;
}

static uint8 SamusFunc_F468_CrouchTransEtc(void) {  // 0x91F758
  if (sign16(samus_pose - kPose_F1_FaceR_CrouchTrans_AimU)) {
    if (sign16(samus_pose - kPose_DB)) {
      samus_momentum_routine_index = 7;
      switch ((uint16)(samus_pose - 53)) {     // off_91F790
      case 0: case 1: return Samus_CrouchTrans();
      case 2: case 3: return Samus_MorphTrans();
      case 4: case 5: {                        // MaybeUnused_sub_91F7F4 / F840: a la bola (en el suelo o en el aire)
        uint8 L = samus_pose - 53 == 5, air = samus_prev_movement_type2 == kMovementType_08_MorphBallFalling ||
                                               samus_prev_movement_type2 == kMovementType_13_SpringBallFalling;
        if (air) samus_pose = (equipped_items & 2) ? (L ? kPose_7E_FaceL_Springball_Fall : kPose_7D_FaceR_Springball_Fall)
                                                   : (L ? kPose_32_FaceL_Morphball_Air : kPose_31_FaceR_Morphball_Air);
        else samus_pose = (equipped_items & 2) ? (L ? kPose_7A_FaceL_Springball_Ground : kPose_79_FaceR_Springball_Ground)
                                               : (L ? kPose_41_FaceL_Morphball_Ground : kPose_1D_FaceR_Morphball_Ground);
        return 1;
      }
      default: return 0;                       // Samus_StandOrUnmorphTrans
      }
    }
    if (samus_pose - 219 < 2)                  // off_91F7A8
      return Samus_MorphTrans();
    return 0;
  }
  samus_momentum_routine_index = 7;
  if (sign16(samus_pose - kPose_F7_FaceR_StandTrans_AimU))
    return Samus_CrouchTrans();
  return 0;
}

static uint8 SamusFunc_F468_Moonwalking(void) {  // 0x91F88C
  if (moonwalk_flag)
    return 0;
  samus_pose = (samus_pose_x_dir == 4) ? kPose_25_FaceR_Turn_Stand : kPose_26_FaceL_Turn_Stand;
  return 1;
}

static void turn_keep_speed(void) {
  add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(samus_x_extra_run_speed, samus_x_extra_run_subspeed));
  set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 0);
  samus_x_accel_mode = 1;
}

static uint8 SamusFunc_F468_TurningAroundOnGround(void) {  // 0x91F8D3
  if (samus_prev_pose && samus_prev_pose != kPose_9B_FaceF_VariaGravitySuit) {
    uint16 v0 = kPoseParams[samus_prev_pose].direction_shots_fired;
    if (samus_prev_movement_type2 == kMovementType_10_Moonwalking) {
      new_projectile_direction_changed_pose = v0 | 0x100;
      samus_pose = (button_config_jump_a & joypad1_lastkeys) ? kSamusTurnPose_Moonwalk[v0] : kSamusTurnPose_Standing[v0];
    } else if (samus_prev_movement_type2 == 5) {
      samus_pose = kSamusTurnPose_Crouching[v0];
    } else {
      samus_pose = kSamusTurnPose_Standing[v0];
    }
  }
  turn_keep_speed();
  return 1;
}

static uint8 SamusFunc_F468_WallJumping(void) {  // 0x91FA76
  uint16 bottom = Samus_GetBottom_R18();
  if (in_liquid_at(bottom))
    return 0;
  atmospheric_gfx_frame_and_type[3] = 1536;
  atmospheric_gfx_anim_timer[3] = 3;
  atmospheric_gfx_y_pos[3] = bottom;
  atmospheric_gfx_x_pos[3] = (samus_pose_x_dir != 8) ? samus_x_pos + 6 : samus_x_pos - 6;
  return 0;
}

static void SamusFunc_FA0A(void) {  // 0x91FA0F: al cambiar de lado, la velocidad extra pasa a la base
  if (samus_prev_pose_x_dir == 8 ? samus_pose_x_dir == 4 : samus_pose_x_dir == 8) {
    add_hilo(&samus_x_base_speed, &samus_x_base_subspeed, IPAIR32(samus_x_extra_run_speed, samus_x_extra_run_subspeed));
    Samus_CancelSpeedBoost();
    set_hilo(&samus_x_extra_run_speed, &samus_x_extra_run_subspeed, 0);
    samus_x_accel_mode = 1;
  }
}

static uint8 SamusFunc_F468_MorphBall(void) {  // 0x91F9F4
  if (samus_prev_movement_type2 == kMovementType_04_MorphBallOnGround ||
      samus_prev_movement_type2 == kMovementType_08_MorphBallFalling)
    samus_anim_frame_skip = FUNC16(Samus_InputHandler);
  SamusFunc_FA0A();
  return 0;
}

static uint8 SamusFunc_F468_call(uint8 mt) {
  switch (mt) {
  case 0: return SamusFunc_F468_Standing();
  case 1: return SamusFunc_F468_Running();
  case 2: return SamusFunc_F468_NormalJump();
  case 3: return SamusFunc_F468_SpinJump();
  case 4: case 8: return SamusFunc_F468_MorphBall();
  case 5: return SamusFunc_F468_Crouching();
  case 6: return SamusFunc_F468_Falling();
  case 7: case 9: case 10: case 11: case 12: case 13: case 21: case 22: case 26: return 0;
  case 14: return SamusFunc_F468_TurningAroundOnGround();
  case 15: return SamusFunc_F468_CrouchTransEtc();
  case 16: return SamusFunc_F468_Moonwalking();
  case 20: return SamusFunc_F468_WallJumping();
  case 23:                                     // 0x91F952
    samus_pose = kSamusTurnPose_Jumping[kPoseParams[samus_prev_pose].direction_shots_fired];
    turn_keep_speed();
    return 1;
  case 24:                                     // 0x91F98A
    samus_pose = kSamusTurnPose_Falling[kPoseParams[samus_prev_pose].direction_shots_fired];
    turn_keep_speed();
    return 1;
  case 25:                                     // 0x91F8CB
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
    return 0;
  default:
    nt_unimpl("SamusFunc_F468 (morfosfera/shinespark)");
    return 0;
  }
}

static void SamusFunc_F468(void) {
  if (SamusFunc_F468_call(samus_movement_type) & 1) {
    set_pose_x_dir_and_type();
    if (samus_movement_type == 14) {
      SamusFunc_F468_TurningAroundOnGround();
      set_pose_x_dir_and_type();
    }
  }
}

void SamusFunc_F433(void) {  // 0x91F433
  set_pose_x_dir_and_type();
  SamusFunc_F468();
  if ((samus_prev_movement_type2 == kMovementType_03_SpinJumping || samus_prev_movement_type2 == kMovementType_14_WallJumping)
      && (equipped_items & 8))
    Samus_LoadSuitPalette();
}

void Samus_SetAnimationFrameIfPoseChanged(void) {  // 0x91FB08
  uint16 t;
  if (equipped_items & 0x20) {
    t = samus_x_speed_divisor;
  } else {
    uint16 r = samus_y_pos + kPoseParams[samus_pose].y_radius - 1;
    if (!(fx_y_pos & 0x8000))
      t = (sign16(fx_y_pos - r) && !(fx_liquid_options & 4)) ? 3 : samus_x_speed_divisor;
    else if ((lava_acid_y_pos & 0x8000) || !sign16(lava_acid_y_pos - r))
      t = samus_x_speed_divisor;
    else
      t = 2;
  }
  if (!(samus_anim_frame_skip & 0x8000) && samus_pose != samus_prev_pose) {
    samus_anim_frame = samus_anim_frame_skip;
    samus_anim_frame_timer = t + bank91_ptr(kSamusAnimationDelayData[samus_pose])[samus_anim_frame_skip];
  }
}

void Samus_InitWallJump(void);

void HandleJumpTransition(void) {  // 0x91FBBB
  switch (samus_movement_type) {
  case 2:                                     // 0x91FC66
    if (samus_pose == kPose_4B_FaceR_Jumptrans || samus_pose == kPose_4C_FaceL_Jumptrans
        || (!sign16(samus_pose - kPose_55_FaceR_Jumptrans_AimU) && sign16(samus_pose - kPose_5B))) {
      if (samus_prev_pose == kPose_27_FaceR_Crouch || samus_prev_pose == kPose_28_FaceL_Crouch)
        samus_y_pos -= 10;
      Samus_InitJump();
    }
    break;
  case 3:                                     // 0x91FC99
    if (samus_prev_movement_type2 != kMovementType_03_SpinJumping && samus_prev_movement_type2 != kMovementType_14_WallJumping)
      Samus_InitJump();
    break;
  case 13:                                    // 0x91FC42
    if ((samus_pose == 0x65 && samus_prev_pose == 100) || (samus_pose == 0x66 && samus_prev_pose == 99))
      Samus_InitJump();
    break;
  case 18: nt_unimpl("HandleJumpTransition_SpringBallInAir"); break;
  case 20:                                    // 0x91FC08
    if (samus_prev_movement_type2 != kMovementType_14_WallJumping)
      Samus_InitWallJump();
    break;
  default:
    break;
  }
}

void Samus_Func20(void) {  // 0x91FCAF
  nt_unimpl("Samus_Func20 (rayos X)");
}

/* ------------------------------------------------------------------ colisión al cambiar de pose ($91:FDAE) */
static uint8 HandleCollDueToChangedPose_Block_CollBoth(void) {  // 0x91FFA7
  if (sign16(samus_y_radius - 8))
    return 1;
  samus_pose = (samus_pose_x_dir == 4) ? kPose_28_FaceL_Crouch : kPose_27_FaceR_Crouch;
  uint16 r18 = kPoseParams[samus_pose].y_radius;
  if (sign16(samus_y_radius - r18)) {
    samus_y_pos += samus_y_radius - r18;
    samus_prev_y_pos = samus_y_pos;
  }
  return 0;
}

static uint8 solid_coll_check(uint16 dir, int16 v) {
  uint16 v1 = samus_y_radius;
  samus_y_radius = kPoseParams[samus_pose].y_radius;
  samus_collision_direction = dir;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(SHL16(v));
  samus_y_radius = v1;
  samus_collision_flag = cres.collision;
  if (samus_collision_flag)
    return 1;
  return 2 | (uint8)0;  /* sin colisión: el resultado lo guarda quien llama */
}

static void HandleCollDueToChangedPose(void) {  // 0x91FDAE
  if (!samus_pose || samus_pose == kPose_9B_FaceF_VariaGravitySuit)
    return;
  solid_enemy_collision_flags = 0;
  block_collision_flags = 0;
  int16 v0 = kPoseParams[samus_prev_pose].y_radius;
  if (!sign16(v0 - kPoseParams[samus_pose].y_radius))
    return;
  samus_y_radius = kPoseParams[samus_prev_pose].y_radius;
  samus_y_radius_diff = kPoseParams[samus_pose].y_radius - v0;
  samus_collision_direction = 2;
  CheckEnemyColl_Result cres = Samus_CheckSolidEnemyColl(SHL16(samus_y_radius_diff));
  samus_collision_flag = cres.collision;
  if (samus_collision_flag)
    solid_enemy_collision_flags = 1;
  samus_space_to_move_up_enemy = (uint16)((uint32)cres.amt >> 16);
  samus_collision_direction = 3;
  cres = Samus_CheckSolidEnemyColl(SHL16(samus_y_radius_diff));
  samus_collision_flag = cres.collision;
  if (samus_collision_flag)
    solid_enemy_collision_flags |= 2;
  samus_space_to_move_down_enemy = (uint16)((uint32)cres.amt >> 16);
  uint8 r = 0;
  switch (solid_enemy_collision_flags) {
  case 1: {                                   // 0x91FE9E
    uint16 v1 = samus_y_radius;
    samus_y_radius = kPoseParams[samus_pose].y_radius;
    samus_collision_direction = 3;
    cres = Samus_CheckSolidEnemyColl(SHL16(samus_y_radius_diff - samus_space_to_move_up_enemy));
    samus_y_radius = v1;
    samus_collision_flag = cres.collision;
    if (samus_collision_flag) r = 1; else samus_space_to_move_up_enemy = (uint16)((uint32)cres.amt >> 16);
    break;
  }
  case 2: {                                   // 0x91FEDF
    uint16 v1 = samus_y_radius;
    samus_y_radius = kPoseParams[samus_pose].y_radius;
    samus_collision_direction = 2;
    cres = Samus_CheckSolidEnemyColl(SHL16(samus_y_radius_diff - samus_space_to_move_down_enemy));
    samus_y_radius = v1;
    samus_collision_flag = cres.collision;
    if (samus_collision_flag) r = 1; else samus_space_to_move_down_enemy = (uint16)((uint32)cres.amt >> 16);
    break;
  }
  default: break;
  }
  if (r) {
    samus_pose = samus_prev_pose;
    return;
  }
  int32 amt = Samus_CollDetectChangedPose(SHL16(-samus_y_radius_diff));
  if (samus_collision_flag)
    block_collision_flags = 1;
  samus_space_to_move_up_blocks = (uint16)((uint32)amt >> 16);
  amt = Samus_CollDetectChangedPose(SHL16(samus_y_radius_diff));
  if (samus_collision_flag)
    block_collision_flags |= 2;
  samus_space_to_move_down_blocks = (uint16)((uint32)amt >> 16);
  switch (block_collision_flags) {
  case 0:                                     // 0x91FF76
    switch (solid_enemy_collision_flags) {
    case 1: samus_y_pos += samus_space_to_move_up_enemy; samus_prev_y_pos = samus_y_pos; break;
    case 2: samus_y_pos -= samus_space_to_move_down_enemy; samus_prev_y_pos = samus_y_pos; break;
    case 3: r = HandleCollDueToChangedPose_Block_CollBoth(); break;
    }
    break;
  case 1:                                     // 0x91FF20
    amt = Samus_CollDetectChangedPose(SHL16(samus_y_radius_diff - samus_space_to_move_up_blocks));
    if (samus_collision_flag) r = 1;
    else if (solid_enemy_collision_flags & 2) r = HandleCollDueToChangedPose_Block_CollBoth();
    else { samus_y_pos += (uint16)((uint32)amt >> 16); samus_prev_y_pos = samus_y_pos; }
    break;
  case 2:                                     // 0x91FF49
    amt = Samus_CollDetectChangedPose(SHL16(samus_space_to_move_down_blocks - samus_y_radius_diff));
    if (samus_collision_flag) r = 1;
    else if (solid_enemy_collision_flags & 1) r = HandleCollDueToChangedPose_Block_CollBoth();
    else { samus_y_pos -= (uint16)((uint32)amt >> 16); samus_prev_y_pos = samus_y_pos; }
    break;
  case 3: r = HandleCollDueToChangedPose_Block_CollBoth(); break;
  }
  if (r)
    samus_pose = samus_prev_pose;
}

void Samus_AlignBottomWithPrevPose(void) {  // 0x90EC7E
  uint16 r18 = kPoseParams[samus_prev_pose].y_radius - kPoseParams[samus_pose].y_radius;
  samus_y_pos += r18;
  samus_prev_y_pos += r18;
}

/* ------------------------------------------------------------------ Samus al empezar la partida (docs/37) */
void nt_clear_ram(uint16 from, uint16 to);
void UpdateBeamTilesAndPalette(void);
void Samus_Initialize(void) {  // 0x91E00D
  uint16 r18 = debug_invincibility;
  nt_clear_ram(0xA02, 0xE0C);                     /* 7E:0A02-0E0B: las variables de Samus */
  if (game_state != 40) {                         // kGameState_40_TransitionToDemo
    frame_handler_alfa = FUNC16(EmptyFunction);
    if (loading_game_state == 34) {               // kGameState_34_CeresGoesBoom
      frame_handler_beta = FUNC16(SetContactDamageIndexAndUpdateMinimap);
      samus_draw_handler = FUNC16(SamusDrawHandler_Default);
      samus_momentum_routine_index = -1;
      samus_special_transgfx_index = 0;
      samus_hurt_switch_index = 0;
      Samus_LoadSuitPalette();
      samus_input_handler = FUNC16(Samus_InputHandler_E913);
    } else {
      frame_handler_beta = FUNC16(Samus_Func16);
      samus_draw_handler = FUNC16(SamusDrawHandler_Default);
      samus_momentum_routine_index = 0;
      samus_special_transgfx_index = 0;
      samus_hurt_switch_index = 0;
      samus_input_handler = FUNC16(Samus_InputHandler_E913);
      debug_invincibility = r18;
    }
  }
  samus_new_pose = -1;
  samus_new_pose_interrupted = -1;
  samus_new_pose_transitional = -1;
  frame_handler_gamma = area_index == 6 ? FUNC16(Samus_Func3) : FUNC16(nullsub_152);
  samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
  UNUSED_word_7E0A5E = -2764;
  samus_prev_health_for_flash = 50;
  samus_visor_palette_timer_index = 1537;
  for (int i = 0; i < 10; i++)
    projectile_bomb_pre_instructions[i] = FUNC16(ProjPreInstr_Empty);
  grapple_beam_function = FUNC16(GrappleBeamFunc_Inactive);
  enable_horiz_slope_coll = 3;
  samus_hurt_flash_counter = 0;
  samus_special_super_palette_flags = 0;
  absolute_moved_last_frame_x_fract = 0;
  absolute_moved_last_frame_x = 1;
  absolute_moved_last_frame_y_fract = 0;
  absolute_moved_last_frame_y = 1;
  for (int i = 0; i < 256; i++)
    hdma_table_1[i] = 255;
  samus_y_subaccel = 0x1C00;
  samus_y_accel = 0;
  fx_y_pos = -1;
  lava_acid_y_pos = -1;
  UpdateBeamTilesAndPalette();
  cinematic_function = 0;
  samus_pose = 0;                                 // kPose_00_FaceF_Powersuit
  samus_pose_x_dir = 0; samus_movement_type = 0;
  samus_prev_pose = 0;
  samus_prev_pose_x_dir = 0; samus_prev_movement_type2 = 0;
  samus_last_different_pose = 0;
  samus_last_different_pose_x_dir = 0; samus_last_different_pose_movement_type = 0;
  enemy_index_to_shake = -1;
  hud_item_index = 0;
  samus_auto_cancel_hud_item_index = 0;
  samus_invincibility_timer = 0;
  samus_knockback_timer = 0;
  samus_hurt_flash_counter = 0;
  debug_invincibility = 0;
  samus_prev_health_for_flash = samus_health;
}

/* ------------------------------------------------------------------ recargas ($91:DF12...) */
void Samus_RestoreHealth(uint16 a) {  // 0x91DF12
  uint16 v1 = a + samus_health;
  samus_health = v1;
  if ((int16)(v1 - samus_max_health) >= 0) {
    uint16 v2 = samus_reserve_health + v1 - samus_max_health;
    if ((int16)(v2 - samus_max_reserve_health) >= 0)
      v2 = samus_max_reserve_health;
    samus_reserve_health = v2;
    if (v2 && !reserve_health_mode)
      reserve_health_mode = 1;
    samus_health = samus_max_health;
  }
}

void Samus_RestoreMissiles(uint16 a) {  // 0x91DF80
  uint16 v1 = samus_missiles + a;
  samus_missiles = v1;
  if ((int16)(v1 - samus_max_missiles) >= 0) {
    samus_reserve_missiles += v1 - samus_max_missiles;
    if (sign16(samus_max_missiles - 99)) {
      if ((int16)(samus_reserve_missiles - samus_max_missiles) >= 0)
        samus_reserve_missiles = samus_max_missiles;
    } else if (!sign16(samus_reserve_missiles - 99)) {
      samus_reserve_missiles = 99;
    }
    samus_missiles = samus_max_missiles;
  }
}

void Samus_RestoreSuperMissiles(uint16 a) {  // 0x91DFD3
  uint16 v1 = samus_super_missiles + a;
  samus_super_missiles = v1;
  if ((int16)(v1 - samus_max_super_missiles) >= 0 && v1 != samus_max_super_missiles)
    samus_super_missiles = samus_max_super_missiles;
}

void Samus_RestorePowerBombs(uint16 a) {  // 0x91DFF0
  uint16 v1 = samus_power_bombs + a;
  samus_power_bombs = v1;
  if ((int16)(v1 - samus_max_power_bombs) >= 0 && v1 != samus_max_power_bombs)
    samus_power_bombs = samus_max_power_bombs;
}
