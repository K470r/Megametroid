/* MegaMetroid — motor nativo: órdenes a Samus desde otros sistemas (CallSomeSamusCode, $90:F084, docs/31).
 * Escrito a mano desde snesrev/sm sm_90.c. */
#include "samus.h"
#include "hdmaobj.h"
#include "../gen/enums.h"

static void Samus_UpdatePreviousPose(void) {  // 0x90F0EE
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;
  samus_last_different_pose_movement_type = samus_prev_movement_type2;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type2 = samus_movement_type;
}

static uint8 SamusCode_00_LockSamus(void) {  // 0x90F109
  frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func13);
  frame_handler_beta = FUNC16(SetContactDamageIndexAndUpdateMinimap);
  return 1;
}

static uint8 SamusCode_1C(void) {  // 0x90F41E
  if (samus_movement_type == kMovementType_14_WallJumping) {
    if (sign16(samus_anim_frame - 23)) {
      if (sign16(samus_anim_frame - 13)) {
        QueueSfx1_Max9(0x31);
        return 0;
      }
      QueueSfx1_Max9(0x3E);
      return 0;
    }
  } else {
    if (samus_movement_type != kMovementType_03_SpinJumping)
      return 0;
    if (samus_pose != kPose_81_FaceR_Screwattack && samus_pose != kPose_82_FaceL_Screwattack) {
      if (samus_pose != kPose_1B_FaceR_SpaceJump && samus_pose != kPose_1C_FaceL_SpaceJump)
        QueueSfx1_Max9(0x31);
      else
        QueueSfx1_Max9(0x3E);
      return 0;
    }
  }
  QueueSfx1_Max9(0x33);
  return 0;
}

static uint8 SamusCode(uint16 a) {
  switch (a & 0x1F) {
  case 0x00: return SamusCode_00_LockSamus();
  case 0x01:                                   // 0x90F117
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
    frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    return 1;
  case 0x02:                                   // 0x90F125
    samus_pose = (samus_pose_x_dir == 4) ? kPose_02_FaceL_Normal : kPose_01_FaceR_Normal;
    samus_anim_frame_skip = 0;
    SamusFunc_F433();
    Samus_SetAnimationFrameIfPoseChanged();
    reached_ceres_elevator_fade_timer = 60;
    return SamusCode_00_LockSamus();
  case 0x07:                                   // SamusCode_07_SetupForElevator ($90:F1C8)
    MakeSamusFaceForward();
    frame_handler_beta = FUNC16(Samus_Func19);
    samus_movement_handler = FUNC16(Samus_MovementHandler_Normal);
    samus_draw_handler = FUNC16(SamusDisplayHandler_UsingElevator);
    samus_input_handler = FUNC16(Samus_InputHandler_E913);
    bomb_jump_dir = 0;
    return 1;
  case 0x11:                                   // SamusCode_11_SetupForDeath ($90:F2F8)
    DisablePaletteFx();
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func13);
    frame_handler_beta = FUNC16(EmptyFunction);
    samus_draw_handler = FUNC16(SamusDisplayHandler_SamusReceivedFatal);
    return 1;
  case 0x14:                                   // 0x90F331: sonidos al recibir daño
    if (sign16(samus_health - 31))
      QueueSfx3_Max6(2);
    if (grapple_beam_function == FUNC16(GrappleBeamFunc_Inactive)) {
      if (samus_pose_x_dir == 3) {
        if (samus_pose == kPose_81_FaceR_Screwattack || samus_pose == kPose_82_FaceL_Screwattack)
          QueueSfx1_Max6(0x33);
        else if (samus_pose == kPose_1B_FaceR_SpaceJump || samus_pose == kPose_1C_FaceL_SpaceJump)
          QueueSfx1_Max6(0x3E);
        else
          QueueSfx1_Max6(0x31);
      }
    } else {
      QueueSfx1_Max6(6);
    }
    return 0;
  case 0x1F:                                   // 0x90F4D0: soltar el gancho (al entrar al ascensor, por ejemplo)
    if (grapple_beam_function != 0xC4F0) {
      grapple_beam_unkD1E = 0;
      grapple_beam_unkD20 = 0;
      grapple_beam_direction = 0;
      grapple_beam_unkD36 = 0;
      grapple_walljump_timer = 0;
      slow_grabble_scrolling_flag = 0;
      grapple_varCF6 = 0;
      grapple_beam_flags = 0;
      if ((equipped_beams & 0xFFF) < 12)       /* LoadProjectilePalette ($90:ACFC) */
        for (uint16 i = 0; i < 16; i++) palette_buffer[224 + i] = kBeamPalettes[(equipped_beams & 0xFFF) * 16 + i];
      grapple_beam_function = 0xC4F0;
      samus_draw_handler = FUNC16(SamusDrawHandler_Default);
    }
    return 0;
  case 0x0D: return (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive)) ? 2 : 0;   // 0x90F2B8
  case 0x0E:                                   // 0x90F2CA
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
    frame_handler_beta = FUNC16(Samus_Func18);
    return 1;
  case 0x0F: frame_handler_gamma = FUNC16(Samus_Func3); return 0;   // 0x90F2D8
  case 0x10:                                   // 0x90F2E0
    if (frame_handler_beta != FUNC16(j_HandleDemoRecorder_2_0)) {
      frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
      frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    }
    return 1;
  case 0x03:                                   // 0x90F152
    if (grapple_beam_function != FUNC16(GrappleBeamFunc_Inactive)) {
      nt_unimpl("SamusCode_03 (gancho)");
      return 0;
    }
    if (samus_movement_type != 3 && samus_movement_type != 20)
      return 0;
    samus_pose = (samus_pose_x_dir == 4) ? kPose_02_FaceL_Normal : kPose_01_FaceR_Normal;
    samus_anim_frame_skip = 0;
    SamusFunc_F433();
    Samus_SetAnimationFrameIfPoseChanged();
    Samus_UpdatePreviousPose();
    return 1;
  case 0x09:                                   // SamusCode_08_SetupForZebesStart ($90:F23C)
    if (equipped_items & 0x20) {
      SpawnPalfxObject(0xE1FC);
      samus_pose = kPose_9B_FaceF_VariaGravitySuit;
    } else if (equipped_items & 1) {
      SpawnPalfxObject(0xE1F8);
      samus_pose = kPose_9B_FaceF_VariaGravitySuit;
    } else {
      SpawnPalfxObject(0xE1F4);
      samus_pose = kPose_00_FaceF_Powersuit;
    }
    Samus_LoadSuitPalette();
    SamusFunc_F433();
    samus_anim_frame_timer = 3;
    samus_anim_frame = 2;
    substate = 0;
    return 1;
  case 0x0A: samus_draw_handler = FUNC16(nullsub_152); return 0;            // 0x90F28D
  case 0x0B: samus_draw_handler = FUNC16(SamusDrawHandler_Default);         // 0x90F295
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
    frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    return 1;
  case 0x12: samus_special_super_palette_flags = 1; return 0;                // 0x90F320
  case 0x13: samus_special_super_palette_flags = 0; Samus_LoadSuitPalette(); return 0;   // 0x90F328
  case 0x1A: frame_handler_beta = FUNC16(Samus_LowHealthCheck); return 0;   // 0x90F409
  case 0x1B:                                   // 0x90F411
    if (frame_handler_beta == FUNC16(j_HandleDemoRecorder_2_0))
      return 1;
    return SamusCode_00_LockSamus();
  case 0x1C: return SamusCode_1C();
  case 0x1D:                                   // 0x90F471
    if (samus_movement_type == 3 || samus_movement_type == 20) {
      QueueSfx1_Max15(0x32);
    } else if (!(button_config_shoot_x & joypad1_lastkeys) && sign16(flare_counter - 16)) {
      QueueSfx1_Max15(2);
    }
    return 0;
  case 0x1E:                                   // 0x90F4A2
    if (game_state == 8) {
      if (samus_movement_type == 3 || samus_movement_type == 20) {
        SamusCode_1C();
        return 0;
      }
      if (!sign16(flare_counter - 16))
        QueueSfx1_Max9(0x41);
    }
    return 0;
  default:
    nt_unimpl_v("CallSomeSamusCode (orden sin portar)", a);
    return 0;
  }
}

void MakeSamusFaceForward(void) {  // 0x91E3F6
  samus_pose = ((equipped_items & 0x20) || (equipped_items & 1)) ? kPose_9B_FaceF_VariaGravitySuit : kPose_00_FaceF_Powersuit;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  Samus_UpdatePreviousPose();
  if (samus_y_radius != 24) {
    samus_y_pos -= 3;
    samus_prev_y_pos = samus_y_pos;
  }
  frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func13);
  frame_handler_beta = FUNC16(SetContactDamageIndexAndUpdateMinimap);
  samus_new_pose = -1;
  samus_new_pose_interrupted = -1;
  samus_new_pose_transitional = -1;
  samus_momentum_routine_index = 0;
  samus_special_transgfx_index = 0;
  samus_hurt_switch_index = 0;
  CallSomeSamusCode(0x1F);
  samus_x_extra_run_speed = samus_x_extra_run_subspeed = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_y_dir = 0;
  used_for_ball_bounce_on_landing = 0;
  samus_x_accel_mode = 0;
  flare_counter = 0;
  flare_animation_frame = 0;
  flare_slow_sparks_anim_frame = 0;
  flare_fast_sparks_anim_frame = 0;
  flare_animation_timer = 0;
  flare_slow_sparks_anim_timer = 0;
  flare_fast_sparks_anim_timer = 0;
  Samus_LoadSuitPalette();
}

uint16 CallSomeSamusCode(uint16 a) {  // 0x90F084
  uint16 code = SamusCode(a);
  if (!(code & 1))
    return code >> 1;
  samus_new_pose = 0xFFFF;
  samus_new_pose_interrupted = 0xFFFF;
  samus_new_pose_transitional = 0xFFFF;
  samus_momentum_routine_index = 0;
  samus_special_transgfx_index = 0;
  samus_hurt_switch_index = 0;
  return 0xFFFF;
}
