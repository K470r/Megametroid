/* MegaMetroid — motor nativo: tomar el traje Varia o el de gravedad (docs/56).
 * Escrito a mano desde snesrev/sm: VariaSuitPickup / GravitySuitPickup ($91:D4E4/D5BA, los llama la PLM del
 * objeto), el objeto de HDMA del rayo de luz ($91:D5A2/D67A, InitializeSuitPickupHdma $91:D692) y sus
 * pre-instrucciones ($88:E026-E3B5). El rayo es una ventana por línea (hdma_table_1: izquierda/derecha) con la
 * matemática de color (reg_COLDATA); la Genesis lo dibuja con eso. */
#pragma GCC optimize("Os")                      /* se usa dos veces en el juego (docs/53) */
#include "samus.h"

#include "hdmaobj.h"
#include "nmath.h"
#include "game.h"

static void Suit_SetLightBeamTable(uint16 v) {   /* las 256 líneas (512 bytes) con el mismo valor */
  for (int i = 255; i >= 0; i--)
    hdma_table_1[i] = v;
}

static void SuitPickup_Common(uint16 other_suit) {
  Samus_CancelSpeedBoost();
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_y_subspeed = 0;
  samus_y_speed = 0;
  samus_y_dir = 0;
  used_for_ball_bounce_on_landing = 0;
  samus_x_accel_mode = 0;
  elevator_status = 0;
  substate = 0;
  suit_pickup_light_beam_pos = 0;
  suit_pickup_light_beam_widening_speed = 256;
  Suit_SetLightBeamTable(255);
  if (samus_movement_type == kMovementType_03_SpinJumping || samus_movement_type == kMovementType_14_WallJumping)
    QueueSfx1_Max9(0x32);
  samus_pose = (equipped_items & other_suit) ? kPose_9B_FaceF_VariaGravitySuit : kPose_00_FaceF_Powersuit;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  CallSomeSamusCode(0x15);
  samus_x_pos = layer1_x_pos + 120;
  samus_prev_x_pos = layer1_x_pos + 120;
  samus_y_pos = layer1_y_pos + 136;
  samus_prev_y_pos = layer1_y_pos + 136;
  QueueSfx2_Max6(0x56);
}

void VariaSuitPickup(void) {  // 0x91D4E4
  suit_pickup_color_math_R = 48;
  suit_pickup_color_math_G = 80;
  suit_pickup_color_math_B = 0x80;
  suit_pickup_palette_transition_color = 0;
  SuitPickup_Common(0x20);
  SpawnHdmaObject(0x91, 0xD5A2);
}

void GravitySuitPickup(void) {  // 0x91D5BA
  suit_pickup_color_math_R = 48;
  suit_pickup_color_math_G = 73;
  suit_pickup_color_math_B = 0x90;
  suit_pickup_palette_transition_color = 1;
  SuitPickup_Common(1);
  SpawnHdmaObject(0x91, 0xD67A);
}

void InitializeSuitPickupHdma(void) {  // 0x91D692 (registros del SNES: la ventana y la matemática de color)
  reg_TM = 19;
  reg_TMW = 19;
  reg_TS = 4;
  reg_TSW = 4;
  next_gameplay_CGWSEL = 16;
  reg_W12SEL = 0;
  reg_W34SEL = 2;
  reg_WOBJSEL = 32;
  next_gameplay_CGADSUB = 51;
  reg_COLDATA[0] = suit_pickup_color_math_R;
  reg_COLDATA[1] = suit_pickup_color_math_G;
  reg_COLDATA[2] = suit_pickup_color_math_B;
  /* mov24(&hdma_ptr_1, 0x9800E4), hdma_ptr_2 = 0x98C8E4, hdma_ptr_3 = 0x999098, hdma_var_1 = 0: esos bytes
     (7E:0A88-0A92) son los de las variables de la demo */
  demo_enable = 0x00E4;
  demo_num_input_frames = 0xE498;
  demo_input_prev = 0x98C8;
  demo_input_prev_new = 0x9098;
  demo_backup_prev_controller_input = 0x0099;
  demo_backup_prev_controller_input_new &= 0xFF00;
}

static void lb_set_lo(uint8 v) { suit_pickup_light_beam_pos = (suit_pickup_light_beam_pos & 0xFF00) | v; }
static void lb_set_hi(uint8 v) { suit_pickup_light_beam_pos = (suit_pickup_light_beam_pos & 0xFF) | v << 8; }
static void tb_set(uint16 b, uint8 v) {          /* byte b de hdma_table_1 (orden del SNES: el bajo primero) */
  uint16 *w = &hdma_table_1[b >> 1];
  *w = (b & 1) ? (uint16)((*w & 0xFF) | v << 8) : (uint16)((*w & 0xFF00) | v);
}

static uint8 VariaSuitPickup_0_LightBeamAppears(void) {  // 0x88E092
  suit_pickup_light_beam_pos += 8;
  int16 v0 = suit_pickup_light_beam_pos;
  uint16 v1 = 0;
  do {
    hdma_table_1[v1 >> 1] = 0x7878;              // g_word_88E0D5
    v1 += 2;
  } while (--v0 > 0);
  uint16 v2 = 510;
  int16 v3 = suit_pickup_light_beam_pos;
  do {
    hdma_table_1[v2 >> 1] = 0x7878;
    v2 -= 2;
  } while (--v3 > 0);
  if (!sign16(suit_pickup_light_beam_pos - 128)) {
    ++substate;
    suit_pickup_light_beam_pos = 30840;
  }
  return 1;
}

static uint8 VariaSuitPickup_1(void) {  // 0x88E0D7
  uint8 sp = HI8(suit_pickup_light_beam_widening_speed);
  lb_set_lo(LO8(suit_pickup_light_beam_pos) - sp);
  lb_set_hi(HI8(suit_pickup_light_beam_pos) + sp);
  Suit_SetLightBeamTable(suit_pickup_light_beam_pos);
  if (sign16((uint8)suit_pickup_light_beam_pos - 97)) {
    ++substate;
    suit_pickup_light_beam_pos = 0x846C;
  }
  return 1;
}

static void AdvanceSuitPickupColorMathSubscreenToWhite(void) {  // 0x88E2B4
  if (suit_pickup_color_math_R != 63) {
    suit_pickup_color_math_R += 2;
    if (!sign8(suit_pickup_color_math_R - 64))
      suit_pickup_color_math_R = 63;
  }
  if (suit_pickup_color_math_G != 95) {
    suit_pickup_color_math_G += 2;
    if (!sign8(suit_pickup_color_math_G - 96))
      suit_pickup_color_math_G = 95;
  }
  if (suit_pickup_color_math_B != 0x9F) {
    suit_pickup_color_math_B += 2;
    if (!sign8(suit_pickup_color_math_B + 96))
      suit_pickup_color_math_B = 0x9F;
  }
}

static uint8 VariaSuitPickup_2_LightBeamWidens(void) {  // 0x88E113
  AdvanceSuitPickupColorMathSubscreenToWhite();
  uint8 sp = HI8(suit_pickup_light_beam_widening_speed);
  bool neg = (int8)(LO8(suit_pickup_light_beam_pos) - sp) < 0;
  lb_set_lo(LO8(suit_pickup_light_beam_pos) - sp);
  if (!LO8(suit_pickup_light_beam_pos) || neg) {
    suit_pickup_light_beam_pos = 0xFF00;
  } else {
    uint16 t = sp + HI8(suit_pickup_light_beam_pos);
    lb_set_hi(t > 0xFF ? 0xFF : t);
  }
  uint16 b = 0, i = 0;
  uint8 lo = LO8(suit_pickup_light_beam_pos), hi = HI8(suit_pickup_light_beam_pos);
  do {                                           /* mitad de arriba: g_byte_88E3C9 hacia adelante */
    uint8 d = B88B(0xE3C9 + i);
    int8 l = lo - d;
    tb_set(b++, l < 0 ? 0 : l);
    uint16 r = d + hi;
    tb_set(b++, r > 0xFF ? 0xFF : r);
    ++i;
  } while (b < 0x100);
  i--;
  do {                                           /* mitad de abajo: hacia atrás */
    uint8 d = B88B(0xE3C9 + i);
    int8 l = lo - d;
    tb_set(b++, l < 0 ? 0 : l);
    uint16 r = d + hi;
    tb_set(b++, r > 0xFF ? 0xFF : r);
    --i;
  } while (b < 0x200);
  suit_pickup_light_beam_widening_speed += 96;
  if (suit_pickup_light_beam_pos == 0xFF00) {
    ++substate;
    suit_pickup_light_beam_widening_speed >>= 1;
    suit_pickup_light_beam_pos = 0;
  }
  return 1;
}

static uint8 SuitPickup_3(uint16 item) {  // VariaSuitPickup_3 $88:E320 / GravitySuitPickup_3 $88:E361
  equipped_items |= item;
  collected_items |= item;
  samus_pose = kPose_9B_FaceF_VariaGravitySuit;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;
  samus_last_different_pose_movement_type = samus_prev_movement_type2;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type2 = samus_movement_type;
  Samus_LoadSuitPalette();
  ++substate;
  return 1;
}

static uint8 VariaSuitPickup_4_LightBeamShrinks(void) {  // 0x88E1BA
  if (suit_pickup_palette_transition_color) {    /* AdvanceSuitPickupColorMathToBlue $88:E3A2 */
    if (suit_pickup_color_math_R != 48) --suit_pickup_color_math_R;
    if (suit_pickup_color_math_G != 73) --suit_pickup_color_math_G;
    if (suit_pickup_color_math_B != 0x90) --suit_pickup_color_math_B;
  } else {                                       /* AdvanceSuitPickupColorMathToOrange $88:E2F9 */
    if (suit_pickup_color_math_R != 63) --suit_pickup_color_math_R;
    if (suit_pickup_color_math_G != 77) --suit_pickup_color_math_G;
    if (suit_pickup_color_math_B != 0x83) --suit_pickup_color_math_B;
  }
  suit_pickup_light_beam_pos += (suit_pickup_light_beam_widening_speed & 0xFF00) >> 8;
  int16 v0 = suit_pickup_light_beam_pos;
  uint16 v1 = 0;
  do {
    hdma_table_1[v1 >> 1] = 255;
    v1 += 2;
  } while (--v0 > 0);
  uint16 v2 = 510;
  int16 v3 = suit_pickup_light_beam_pos - 1;
  do {
    hdma_table_1[v2 >> 1] = 255;
    v2 -= 2;
  } while (--v3 > 0);
  suit_pickup_light_beam_widening_speed -= 32;
  if (sign16(suit_pickup_light_beam_widening_speed - 256))
    suit_pickup_light_beam_widening_speed = 256;
  if (!sign16(suit_pickup_light_beam_pos - 128)) {
    ++substate;
    suit_pickup_light_beam_pos = -1793;
  }
  return 1;
}

static uint8 VariaSuitPickup_5_LightBeamDissipates(void) {  // 0x88E22B
  lb_set_lo(LO8(suit_pickup_light_beam_pos) + 8);
  lb_set_hi(HI8(suit_pickup_light_beam_pos) - 8);
  hdma_table_1[128] = suit_pickup_light_beam_pos;
  if (!sign16((uint8)suit_pickup_light_beam_pos - 112))
    ++substate;
  return 1;
}

static uint8 GravitySuitPickup_6(void) {  // 0x88E25F
  reg_COLDATA[2] = 0x80;
  reg_COLDATA[1] = 64;
  reg_COLDATA[0] = 32;
  demo_enable = 0x0001;                          /* mov24(&hdma_ptr_1, 0x980001) (7E:0A88-0A8A) */
  demo_num_input_frames = 0x0098;                /* y *(uint16 *)((uint8 *)&demo_num_input_frames + 1) = 0 */
  demo_input_prev = 0;
  demo_input_prev_new = 0;
  demo_backup_prev_controller_input = 0;
  hdma_table_1[0] = 255;
  substate = 0;
  suit_pickup_light_beam_pos = 0;
  suit_pickup_color_math_R = 0;
  suit_pickup_color_math_G = 0;
  suit_pickup_color_math_B = 0;
  suit_pickup_palette_transition_color = 0;
  int v0 = hdma_object_index >> 1;
  hdma_object_instruction_list_pointers[v0] += 2;
  hdma_object_instruction_timers[v0] = 1;
  CallSomeSamusCode(0xB);
  return 0;
}

void HdmaobjPreInstr_SuitPickup(uint16 k, bool gravity) {  // 0x88E026 (Varia) / 0x88E05C (gravedad)
  uint8 r;
  switch (substate) {
  case 0: r = VariaSuitPickup_0_LightBeamAppears(); break;
  case 1: r = VariaSuitPickup_1(); break;
  case 2: r = VariaSuitPickup_2_LightBeamWidens(); break;
  case 3: r = SuitPickup_3(gravity ? 0x20 : 1); break;
  case 4: r = VariaSuitPickup_4_LightBeamShrinks(); break;
  case 5: r = VariaSuitPickup_5_LightBeamDissipates(); break;
  case 6:
    if (!gravity)
      QueueMusic_Delayed8(3);                    // VariaSuitPickup_6 ($88:E258)
    r = GravitySuitPickup_6();
    break;
  default: nt_unimpl("HdmaobjPreInstr_SuitPickup (substate)"); r = 0; break;
  }
  if (r & 1) {
    reg_COLDATA[0] = suit_pickup_color_math_R;
    reg_COLDATA[1] = suit_pickup_color_math_G;
    reg_COLDATA[2] = suit_pickup_color_math_B;
    fx_layer_blending_config_c = 18;
  }
}
