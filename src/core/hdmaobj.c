/* MegaMetroid — motor nativo: objetos de HDMA (banco $88 del decomp, sm_88.c; docs/35).
 *
 * En el SNES cada objeto programa un canal de HDMA (tablas por línea para la PPU) con una lista de instrucciones y
 * una "pre-instrucción" que corre en cada cuadro. Aquí se porta la parte del juego: las listas, los temporizadores y
 * lo que las pre-instrucciones calculan en la RAM (los niveles del haze en hdma_table_2, los bordes del círculo del
 * power bomb por línea, el color de COLDATA). Los registros de DMA del SNES no existen: la capa de la Genesis lee
 * esas variables y dibuja lo equivalente (degradado del fondo, círculo de la explosión).
 *
 * LayerBlendingHandler ($88:8000) arma los registros de la PPU (capas, ventanas, color math) a partir de
 * fx_layer_blending_config_c: la Genesis los lee para imitar las transparencias y el tinte (COLDATA). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "animtiles.h"
#include "plm.h"
#include "enemies.h"
#include "dcompat.h"
#include "tables.h"

static void HdmaobjInstr_InitMorphBallEyeBeamHdma(uint16 k);
static void HdmaobjPreInstr_E9E6(uint16 k);
static void HdmaobjPreInstr_EA3C(uint16 k);
static void HdmaobjPreInstr_EACB(uint16 k);

/* bytes sueltos de arreglos de palabras (el SNES los toca byte a byte; en el 68000 el orden de los bytes de una
   palabra es otro, así que se nombra el byte: bajo = el de la dirección par) */
static inline uint8 lo8(const uint16 *a, uint16 i) { return (uint8)a[i]; }
static inline void set_lo8(uint16 *a, uint16 i, uint8 v) { a[i] = (a[i] & 0xFF00) | v; }
static inline void set_byte(uint16 *a, uint16 b, uint8 v) {   /* byte b (dirección) de un arreglo de palabras */
  if (b & 1) a[b >> 1] = (a[b >> 1] & 0x00FF) | (uint16)v << 8;
  else a[b >> 1] = (a[b >> 1] & 0xFF00) | v;
}

/* ------------------------------------------------------------------ alta de objetos ($88:8435) */
static uint16 SpawnHdmaObjectInner(uint16 k, uint8 bank, uint16 list, uint16 r18, uint16 r20) {  // 0x888477
  int v = k >> 1;
  hdma_object_pre_instructions[v] = FUNC16(nullsub_293);
  hdma_object_pre_instruction_bank[v] = 0x88;
  hdma_object_instruction_list_pointers[v] = list;
  hdma_object_instruction_timers[v] = 1;
  hdma_object_timers[v] = 0;
  hdma_object_A[v] = 0;
  hdma_object_B[v] = 0;
  hdma_object_C[v] = 0;
  hdma_object_D[v] = 0;
  hdma_object_channels_bitmask[v] = (uint16)(r18 >> 8 | r18 << 8);   // swap16
  hdma_object_bank_slot[v] = (uint16)bank << 8 | r20;
  return k;
}

uint16 SpawnHdmaObject(uint8 bank, uint16 list) {  // 0x888435 (DMAP/BBAD del SNES: no hacen falta)
  uint16 r18 = 1024, r20 = 32;
  for (uint16 k = 0; k != 12; k += 2) {
    if (!hdma_object_channels_bitmask[k >> 1])
      return SpawnHdmaObjectInner(k, bank, list, r18, r20);
    if (r18 & 0x8000)
      return 0xFFFF;
    r18 <<= 1;
    r20 += 16;
  }
  return 0xFFFF;
}

static void SpawnHdmaObjectToSlot0xA(uint8 bank, uint16 list) {  // 0x88840A
  SpawnHdmaObjectInner(0xA, bank, list, 0x8000, 0x70);
}

void DisableHdmaObjects(void) { hdma_objects_enable_flag &= ~0x8000; }   // 0x888293
void EnableHdmaObjects(void) { hdma_objects_enable_flag |= 0x8000; }     // 0x888288
void WaitUntilEndOfVblankAndClearHdma(void) {                              // 0x88829E + sub_8882AC
  reg_HDMAEN = 0;
  for (int i = 5; i >= 0; i--)
    hdma_object_channels_bitmask[i] = 0;
}

/* ------------------------------------------------------------------ objetos de las salas */
void SpawnBG3ScrollHdmaObject(void) {  // 0x88D865 (scroll del BG3: el HUD; la Genesis no lo usa)
  SpawnHdmaObjectToSlot0xA(0x88, 0xD8D0);
}

/* funciones de subida del líquido (fx_rising_function_bank_88) */
#define FX_RISE_LAVA_ACID 0xB343            /* FxRisingFunction_LavaAcid */
#define FX_RISE_LAVA_ACID_WAIT 0xB367
#define FX_RISE_LAVA_ACID_RAISING 0xB382
#define FX_RISE_WATER 0xC428                /* FxRisingFunction_C428_WaterNormal */
#define FX_RISE_WATER_WAIT 0xC44C
#define FX_RISE_WATER_RISING 0xC458

static uint8 CheckBossBitForCurArea1(void) { return (boss_bits_for_area[area_index] & 1) != 0; }

void FxTypeFunc(uint8 type) {  // kFxTypeFuncPtrs ($89:AB82): los de Ceres
  switch (type) {
  case 0: break;
  case 0x28:                                    // FxTypeFunc_28_CeresRidley ($88:D8DE)
    SpawnHdmaObject(0x88, 0xD906);
    SpawnHdmaObject(0x88, 0xD91D);
    break;
  case 0x2A:                                    // FxTypeFunc_CeresElevator ($88:D928)
    SpawnHdmaObject(0x88, 0xD939);
    break;
  case 0x2C:                                    // FxTypeFunc_2C_Haze ($88:DDC7)
    SpawnHdmaObject(0x88, CheckBossBitForCurArea1() ? 0xDEEB : 0xDED3);
    break;
  case 0x08:                                    // FxTypeFunc_8_Spores ($88:DA11)
    gameplay_BG3SC = 92;
    SpawnHdmaObject(0x88, 0xDA2D);
    SpawnBG3ScrollHdmaObject();
    SpawnAnimtiles(addr_kAnimtiles_Spores);
    break;
  case 0x0A:                                    // FxTypeFunc_A_Rain ($88:D950)
    gameplay_BG3SC = 92;
    SpawnHdmaObject(0x88, 0xD96C);
    SpawnBG3ScrollHdmaObject();
    SpawnAnimtiles(addr_kAnimtiles_Rain);
    break;
  case 0x0C:                                    // FxTypeFunc_C (niebla, $88:DB08)
    gameplay_BG3SC = 92;
    SpawnHdmaObject(0x88, 0xDB19);
    break;
  case 0x20:                                    // FxTypeFunc_20 (cielo de la superficie, $88:A7D8)
    layer2_scroll_x |= 1;
    layer2_scroll_y |= 1;
    SpawnHdmaObject(0x88, 0xAD76);
    set_to_e0_by_scrolling_sky = 224;
    UNUSED_word_7E059C = 0;
    break;
  case 0x22:                                    // FxTypeFunc_22_ScrollingSky / RoomSetupAsm_ScrollingSkyOcean ($88:A800)
    reg_BG2SC = 74;
    SpawnHdmaObject(0x88, 0xAD89);
    set_to_e0_by_scrolling_sky = 224;
    UNUSED_word_7E059C = 0;
    break;
  case 0x02:                                    // FxTypeFunc_2_Lava ($88:B279)
  case 0x04:                                    // FxTypeFunc_4_Acid ($88:B2A1)
    fx_rising_function_bank_88 = FX_RISE_LAVA_ACID;
    lava_acid_y_pos = fx_base_y_pos;
    SpawnHdmaObject(0x88, 0xC3E1);
    SpawnHdmaObject(0x88, 0xC3F0);
    SpawnBG3ScrollHdmaObject();
    SpawnAnimtiles(type == 2 ? addr_kAnimtiles_Lava : addr_kAnimtiles_Acid);
    break;
  case 0x06:                                    // FxTypeFunc_6_Water ($88:C3FF)
    fx_rising_function_bank_88 = FX_RISE_WATER;
    fx_y_pos = fx_base_y_pos;
    SpawnHdmaObject(0x88, 0xD856);
    if (fx_liquid_options & 2)
      SpawnHdmaObject(0x88, 0xD847);
    SpawnBG3ScrollHdmaObject();
    break;
  case 0x24:                                    // FxTypeFunc_24 (oscuridad de las luciérnagas, $88:B07C)
    fireflea_flashing_timer = 6;
    fireflea_flashing_index = 0;
    SpawnHdmaObject(0x88, 0xB0AC);
    statue_bg2_ypos = 0;                        /* *(uint16 *)&hdma_window_1_left_pos[0] */
    UNUSED_word_7E177C = 0;
    fireflea_darkness_level = 0;
    UNUSED_word_7E1780 = 24;
    UNUSED_word_7E1782 = 0;                     /* kFirefleaFlashingShades[0] */
    break;
  case 0x26:                                    // FxTypeFunc_26_TourianEntranceStatue ($88:DB8A)
    if (CheckEventHappened(0xA) & 1) {
      SpawnHardcodedPlm(0x06, 0x0C, 0xB777);
      scrolls[0] = 2;
      scrolls[1] = 2;
    }
    reg_BG2SC = 74;
    fx_rising_function_bank_88 = FX_RISE_WATER;
    fx_y_pos = fx_base_y_pos;
    SpawnHdmaObject(0x88, 0xD856);
    SpawnHdmaObject(0x88, 0xDCFA);
    SpawnBG3ScrollHdmaObject();
    break;
  default: nt_unimpl_v("FxTypeFunc (tipo de FX sin portar)", type); break;
  }
}

/* ------------------------------------------------------------------ haze de Ceres ($88:DE10) */
static void HazeFadeStep(uint16 k) {   /* hdma_table_2: 16 bandas, nivel A, A-1, ... (con el color en los bits altos) */
  int v = k >> 1;
  uint8 r20 = lo8(hdma_object_B, v);
  int8 lv = (int8)lo8(hdma_object_A, v);
  for (int i = 15; i >= 0; --i) {
    set_byte(hdma_table_2, i, r20 | (uint8)lv);
    lv = (int8)(lv - 1);
    if (lv < 0) lv = 0;
  }
}

static void HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn(uint16 k) {  // 0x88DE2D
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  fx_layer_blending_config_c = 44;
  int v = k >> 1;
  if (hdma_object_A[v] == 16) {
    hdma_object_pre_instructions[v] = FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadedIn);
  } else {
    HazeFadeStep(k);
    ++hdma_object_A[v];
  }
}

static void HazeStart(uint16 k, uint16 a) {  // sub_88DE18
  int v = k >> 1;
  hdma_object_B[v] = a;
  hdma_object_A[v] = 0;
  if (door_transition_function == FUNC16(DoorTransition_FadeInScreenAndFinish)) {
    hdma_object_pre_instructions[v] = FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn);
    HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn(k);
  }
}

static void HdmaobjPreInstr_HazeColorMathSubscreen_FadedIn(uint16 k) {  // 0x88DE74
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  fx_layer_blending_config_c = 44;
  if (door_transition_function == FUNC16(DoorTransitionFunction_FadeOutScreen))
    hdma_object_pre_instructions[k >> 1] = FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingOut);
}

static void HdmaobjPreInstr_HazeColorMathSubscreen_FadingOut(uint16 k) {  // 0x88DE96
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  fx_layer_blending_config_c = 44;
  int v = k >> 1;
  if (hdma_object_A[v]) {
    HazeFadeStep(k);
    --hdma_object_A[v];
  }
}

/* ------------------------------------------------------------------ explosión del power bomb ($88:8AA4) */
#define kPowerBombPreExplosionRadiusSpeed 0x3000
#define kPowerBombPreExplosionRadiusAccel 0x80
#define kPowerBombAfterglowDelay 3                 /* g_word_888B96 */
#define kPowerBombExplosionRadiusSpeed 0
#define kPowerBombExplosionRadiusAccel 0x30
#define B88(a) B88B(a)
#define kPowerBombShapeScaled 0xA206               /* g_byte_88A206: forma de 1/4 de círculo (x por línea, y) */
#define kPowerBombShapeTop 0xA286                  /* g_byte_88A286 */
#define kPowerBombExplosionColors 0x8D85           /* kPowerBombExplosionColors */
#define kPowerBombPreExplosionColors 0x9079        /* g_byte_889079 */

void SpawnPowerBombExplosion(void) {  // 0x888AA4
  if (time_is_frozen_flag) {
    power_bomb_explosion_status = 0x4000;
  } else {
    QueueSfx1_Max15(1);
    power_bomb_explosion_status = 0x8000;
    SpawnHdmaObject(0x88, 0x8ACE);
    SpawnHdmaObject(0x88, 0x8B80);
  }
}

static void Hdmaobj_PreExplodeWhite(void) {  // 0x888B14
  offscreen_power_bomb_left_hdma = 0xFF;
  offscreen_power_bomb_right_hdma = 0;
  power_bomb_pre_explosion_flash_radius = 1024;
  power_bomb_pre_explosion_radius_speed = kPowerBombPreExplosionRadiusSpeed;
}
static void Hdmaobj_PreExplodeYellow(void) { pre_scaled_power_bomb_explosion_shape_def_ptr = 0x9F06; }   // 0x888B32
static void Hdmaobj_ExplodeYellow(void) {  // 0x888B39
  power_bomb_explosion_radius = 1024;
  power_bomb_pre_explosion_radius_speed = kPowerBombExplosionRadiusSpeed;
}
static void Hdmaobj_ExplodeWhite(void) { pre_scaled_power_bomb_explosion_shape_def_ptr = addr_kPowerBombExplosionShapeDef0; }   // 0x888B47

uint8 Hdmaobj_CrystalFlash(void);   /* samus_move.c ($90:D5A2) */

static void Hdmaobj_CleanUpTryCrystalFlash(uint16 k) {  // 0x888B4E
  if (samus_x_pos != power_bomb_explosion_x_pos || samus_y_pos != power_bomb_explosion_y_pos
      || (Hdmaobj_CrystalFlash() & 1))
    power_bomb_flag = 0;
  power_bomb_explosion_status = 0;
  int v = k >> 1;
  hdma_object_channels_bitmask[v] = 0;
  hdma_object_channels_bitmask[v + 1] = 0;
  power_bomb_pre_explosion_flash_radius = 0;
  power_bomb_explosion_radius = 0;
  CallSomeSamusCode(0x1E);
}

static void HdmaobjPreInstr_PowerBombExplode_SetWindowConf(uint16 k) {  // 0x888B8F
  fx_layer_blending_config_c |= 0x8000;
}

static void NextInstr(uint16 k) {   /* la pre-instrucción salta el Sleep de la lista */
  int v = k >> 1;
  hdma_object_instruction_timers[v] = 1;
  hdma_object_instruction_list_pointers[v] += 2;
  hdma_object_timers[v] = 0;
}

static void HdmaobjPreInstr_PowerBombExplode_Stage5_Afterglow(uint16 k) {  // 0x888B98
  if (power_bomb_explosion_status & 0x8000) {
    int v = k >> 1;
    if ((--hdma_object_timers[v]) & 0x8000) {
      uint8 d = lo8(hdma_object_D, v);
      set_lo8(hdma_object_D, v, d - 1);
      if (d == 1) {
        hdma_object_instruction_timers[v] = 1;
        hdma_object_instruction_list_pointers[v] += 2;
      } else {
        if (reg_COLDATA[0] & 0x1F) reg_COLDATA[0] = ((reg_COLDATA[0] & 0x1F) - 1) | 0x20;
        if (reg_COLDATA[1] & 0x1F) reg_COLDATA[1] = ((reg_COLDATA[1] & 0x1F) - 1) | 0x40;
        if (reg_COLDATA[2] & 0x1F) reg_COLDATA[2] = ((reg_COLDATA[2] & 0x1F) - 1) | 0x80;
        set_lo8(hdma_object_timers, v, kPowerBombAfterglowDelay);
      }
    }
  }
}

/* bordes por línea (k = línea 0-191) desde una forma ya escalada en la ROM (192 anchos) */
static void CalculatePowerBombHdma_LeftOfScreen(uint16 k, uint16 j) {  // 0x888BEA
  do {
    uint8 w = B88(j), c = (uint8)power_bomb_explosion_x_pos_plus_0x100;
    if ((uint16)w + c > 0xFF) {
      power_bomb_explosion_right_hdma[k] = (uint8)(w + c);
      power_bomb_explosion_left_hdma[k] = 0;
    } else {
      power_bomb_explosion_right_hdma[k] = 0;
      power_bomb_explosion_left_hdma[k] = 1;
    }
    ++j; ++k;
  } while (k != 192);
}

static void CalculatePowerBombHdma_OnScreen(uint16 k, uint16 j) {  // 0x888C12
  do {
    uint8 w = B88(j);
    if (!w) break;
    uint8 c = (uint8)power_bomb_explosion_x_pos_plus_0x100;
    power_bomb_explosion_left_hdma[k] = (c >= w) ? c - w : 0;
    power_bomb_explosion_right_hdma[k] = ((uint16)c + w > 0xFF) ? 255 : c + w;
    ++j; ++k;
  } while (k != 192);
}

static void CalculatePowerBombHdma_RightOfScreen(uint16 k, uint16 j) {  // 0x888C3A
  do {
    uint8 w = B88(j), c = (uint8)power_bomb_explosion_x_pos_plus_0x100;
    if (c < w) {
      power_bomb_explosion_left_hdma[k] = c - w;
      power_bomb_explosion_right_hdma[k] = 0xFF;
    } else {
      power_bomb_explosion_left_hdma[k] = 0xFF;
      power_bomb_explosion_right_hdma[k] = 0xFE;
    }
    ++j; ++k;
  } while (k != 192);
}

/* posición en pantalla y punteros a las tablas indirectas del SNES (se conservan: el juego los guarda) */
static void PowerBombScreenPos(uint16 k, uint16 radius) {
  uint16 v1;
  uint16 x = power_bomb_explosion_x_pos - layer1_x_pos + 256;
  if (x >= 0x300) {
    v1 = 0;
  } else {
    power_bomb_explosion_x_pos_plus_0x100 = x;
    v1 = power_bomb_explosion_y_pos - layer1_y_pos + 256;
    if (v1 >= 0x300) v1 = 0;
  }
  power_bomb_explosion_y_pos_rsub_0x1ff = (v1 ^ 0x3FF) - 256;
  if (!(radius & 0xFF00))
    power_bomb_explosion_y_pos_rsub_0x1ff = 0;
  hdma_object_table_pointers[(k >> 1) + 0] = 3 * power_bomb_explosion_y_pos_rsub_0x1ff + addr_kIndirectHdmaTable_PowerBombExplodeLeft;
  hdma_object_table_pointers[(k >> 1) + 1] = 3 * power_bomb_explosion_y_pos_rsub_0x1ff + addr_kIndirectHdmaTable_PowerBombExplodeRight;
}

static uint8 k_out;
static uint8 Mul8x8hi(uint8 a, uint8 b) { return (uint8)(((uint16)a * b) >> 8); }

/* el círculo escalado: la forma de 1/4 (g_byte_88A206: x, y por punto) por el radio, de abajo hacia arriba */
static uint16 CalcPowerBombHdmaScaled(uint8 mode, uint8 k, uint8 j, uint8 m) {  // 0x888CC6 / 8D04 / 8D46
  uint8 left = 0, right = 0;
  do {
    uint8 r20 = Mul8x8hi(m, B88(kPowerBombShapeScaled + j + 32));
    uint8 c = (uint8)power_bomb_explosion_x_pos_plus_0x100;
    uint8 reg = Mul8x8hi(m, B88(kPowerBombShapeScaled + j));
    if (mode == 0) {                              // izquierda de la pantalla
      if ((uint16)reg + c > 0xFF) { right = reg + c; left = 0; } else { right = 0; left = 0xFF; }
    } else if (mode == 1) {                       // en pantalla
      right = ((uint16)reg + c > 0xFF) ? 0xFF : (uint8)(reg + c);
      left = (c < reg) ? 0 : (uint8)(c - reg);
    } else {                                      // derecha
      if (c < reg) { left = c - reg; right = 0xFF; } else { left = 0xFF; right = 0; }
    }
    for (;;) {
      power_bomb_explosion_left_hdma[k] = left;
      power_bomb_explosion_right_hdma[k] = right;
      if (k == r20) break;
      k--;
    }
    j++;
  } while (!(j & 0x80));
  k_out = k;
  return (uint16)right << 8 | left;
}

static void PowerBombScaledCircle(uint16 radius_hi) {
  uint8 kk = Mul8x8hi((uint8)radius_hi, B88(kPowerBombShapeTop));
  uint8 hi = HI8(power_bomb_explosion_x_pos_plus_0x100);
  uint16 v2 = CalcPowerBombHdmaScaled(hi == 0 ? 0 : hi == 1 ? 1 : 2, kk, 96, (uint8)radius_hi);
  uint8 i = k_out;
  do {
    power_bomb_explosion_left_hdma[i] = (uint8)v2;
    power_bomb_explosion_right_hdma[i] = HI8(v2);
    i--;
  } while (!(i & 0x80));
  uint8 v3 = kk + 1;                            /* de kk + 1 a 0xBF (con la vuelta en 256): sin círculo */
  if (v3 > 0xC0) {
    memset(power_bomb_explosion_left_hdma + v3, 255, 256 - v3);
    memset(power_bomb_explosion_right_hdma + v3, 0, 256 - v3);
    v3 = 0;
  }
  memset(power_bomb_explosion_left_hdma + v3, 255, 0xC0 - v3);
  memset(power_bomb_explosion_right_hdma + v3, 0, 0xC0 - v3);
}

static void PowerBombShapeFromRom(void) {
  uint16 j = pre_scaled_power_bomb_explosion_shape_def_ptr;
  uint8 hi = HI8(power_bomb_explosion_x_pos_plus_0x100);
  if (hi == 0) CalculatePowerBombHdma_LeftOfScreen(0, j);
  else if (hi == 1) CalculatePowerBombHdma_OnScreen(0, j);
  else CalculatePowerBombHdma_RightOfScreen(0, j);
}

static void SetColdata(uint16 table, uint16 t) {
  reg_COLDATA[0] = B88(table + t) | 0x20;
  reg_COLDATA[1] = B88(table + t + 1) | 0x40;
  reg_COLDATA[2] = B88(table + t + 2) | 0x80;
}

static void HdmaobjPreInstr_PowerBombExplode_PreExplosionWhite(uint16 k) {  // 0x8890DF
  if (!(power_bomb_explosion_status & 0x8000)) return;
  PowerBombScreenPos(k, power_bomb_pre_explosion_flash_radius);
  PowerBombScaledCircle(HI8(power_bomb_pre_explosion_flash_radius));
  SetColdata(kPowerBombPreExplosionColors, 3 * ((HI8(power_bomb_pre_explosion_flash_radius) >> 3) & 0xF));
  power_bomb_pre_explosion_flash_radius += power_bomb_pre_explosion_radius_speed;
  if (power_bomb_pre_explosion_flash_radius < 0x9200)
    power_bomb_pre_explosion_radius_speed -= kPowerBombPreExplosionRadiusAccel;
  else
    NextInstr(k);
}

static void HdmaobjPreInstr_PowerBombExplode_PreExplosionYellow(uint16 k) {  // 0x8891A8
  if (!(power_bomb_explosion_status & 0x8000)) return;
  PowerBombScreenPos(k, power_bomb_pre_explosion_flash_radius);
  PowerBombShapeFromRom();
  SetColdata(kPowerBombPreExplosionColors, 3 * ((HI8(power_bomb_pre_explosion_flash_radius) >> 3) & 0xF));
  pre_scaled_power_bomb_explosion_shape_def_ptr += 192;
  if (pre_scaled_power_bomb_explosion_shape_def_ptr == 0xA206)
    NextInstr(k);
  if ((uint32)power_bomb_pre_explosion_radius_speed + power_bomb_pre_explosion_flash_radius < 0x10000) {
    power_bomb_pre_explosion_flash_radius += power_bomb_pre_explosion_radius_speed;
    power_bomb_pre_explosion_radius_speed -= kPowerBombPreExplosionRadiusAccel;
  }
}

static void HdmaobjPreInstr_PowerBombExplode_ExplosionYellow(uint16 k) {  // 0x888DE9
  if (!(power_bomb_explosion_status & 0x8000)) return;
  PowerBombScreenPos(k, power_bomb_explosion_radius);
  PowerBombScaledCircle(HI8(power_bomb_explosion_radius));
  SetColdata(kPowerBombExplosionColors, 3 * (HI8(power_bomb_explosion_radius) >> 3));
  power_bomb_explosion_radius += power_bomb_pre_explosion_radius_speed;
  if (power_bomb_explosion_radius < 0x8600)
    power_bomb_pre_explosion_radius_speed += kPowerBombExplosionRadiusAccel;
  else
    NextInstr(k);
}

static void HdmaobjPreInstr_PowerBombExplode_ExplosionWhite(uint16 k) {  // 0x888EB2
  if (!(power_bomb_explosion_status & 0x8000)) return;
  PowerBombScreenPos(k, power_bomb_explosion_radius);
  PowerBombShapeFromRom();
  SetColdata(kPowerBombExplosionColors, 3 * (HI8(power_bomb_explosion_radius) >> 3));
  pre_scaled_power_bomb_explosion_shape_def_ptr += 192;
  if (pre_scaled_power_bomb_explosion_shape_def_ptr == 0x9F06) {
    NextInstr(k);
    hdma_object_D[k >> 1] = 32;
  }
  if ((uint32)power_bomb_pre_explosion_radius_speed + power_bomb_explosion_radius < 0x10000) {
    power_bomb_explosion_radius += power_bomb_pre_explosion_radius_speed;
    power_bomb_pre_explosion_radius_speed += kPowerBombExplosionRadiusAccel;
  }
}

/* ------------------------------------------------------------------ lluvia, esporas, niebla (BG3, $88:D981...) */
static void HdmaobjPreInstr_RainBg3Scroll(uint16 k) {  // 0x88D9A1
  gameplay_BG3SC = 92;
  fx_layer_blending_config_c = fx_layer_blending_config_b;
  if (!time_is_frozen_flag) {
    int v = (uint8)k >> 1;
    bg3_ypos = hdma_object_E[v] - layer1_y_pos + (int8)HI8(hdma_object_A[v]);
    hdma_object_A[v] -= 1536;
    hdma_object_E[v] = layer1_y_pos;
    bg3_xpos = hdma_object_F[v] - layer1_x_pos + (int8)HI8(hdma_object_B[v]);
    hdma_object_B[v] += hdma_object_D[v];
    hdma_object_F[v] = layer1_x_pos;
  }
}

static void HdmaobjPreInstr_SporesBG3Xscroll(uint16 k) {  // 0x88DA47
  gameplay_BG3SC = 92;
  fx_layer_blending_config_c = fx_layer_blending_config_b;
  if (!time_is_frozen_flag) {
    int v = (uint8)k >> 1;
    bg3_ypos = layer1_y_pos + (int8)HI8(hdma_object_C[v]);
    hdma_object_C[v] -= 64;
    bg3_xpos = layer1_x_pos + (int8)HI8(hdma_object_D[v]);
  }
}

static void HdmaobjPreInstr_FogBG3Scroll(uint16 k) {  // 0x88DB36
  gameplay_BG3SC = 92;
  fx_layer_blending_config_c = fx_layer_blending_config_b;
  if (!time_is_frozen_flag) {
    int v = (uint8)k >> 1;
    bg3_ypos = layer1_y_pos + (int8)HI8(hdma_object_A[v]);
    hdma_object_A[v] -= 64;
    bg3_xpos = layer1_x_pos + (int8)HI8(hdma_object_B[v]);
    hdma_object_B[v] += 80;
  }
}

/* ------------------------------------------------------------------ lava, ácido y agua ($88:B279...)
   La superficie del líquido: base (fx_base_y_pos, sube o baja hacia fx_target_y_pos) + marea (tide_phase). Las
   tablas de HDMA (BG3 = el líquido, BG2 = la ondulación del fondo bajo el agua) quedan en mother_brain_indirect_hdma
   y g_word_7E9C46 para la Genesis. */

static inline void mbi_put16(uint16 off, uint16 v) {   /* palabra en la tabla de bytes (orden del SNES) */
  mother_brain_indirect_hdma[off] = (uint8)v;
  mother_brain_indirect_hdma[off + 1] = (uint8)(v >> 8);
}

static uint8 RaiseOrLowerFx(void) {  // 0x88868C
  if (fx_target_y_pos & 0x8000)
    return 1;
  uint32 d = (uint32)((int32)(int16)fx_y_vel << 8);
  uint16 v = (d + ((uint32)fx_base_y_pos << 16 | fx_base_y_subpos)) >> 16;
  fx_base_y_subpos += (uint16)d;
  if (!(fx_y_vel & 0x8000)) {
    if (v & 0x8000)
      v = 0xFFFF;
    fx_base_y_pos = v;
    if (fx_target_y_pos >= v)
      return 0;
    fx_base_y_pos = fx_target_y_pos;
    fx_base_y_subpos = 0;
    return 1;
  }
  if (v & 0x8000)
    v = 0;
  fx_base_y_pos = v;
  if (fx_target_y_pos >= v) {
    fx_base_y_pos = fx_target_y_pos;
    fx_base_y_subpos = 0;
    return 1;
  }
  return 0;
}

static const uint16 kEarthquakeSfx[17] = { 0x46, 1, 0x46, 3, 0x46, 2, 0x46, 1, 0x46, 1, 0x46, 2, 0x46, 2, 0x46, 1, 0x8000 };

static void HandleEarthquakeSoundEffect(void) {  // 0x88B21D
  if (!(earthquake_sfx_timer & 0x8000) && (--earthquake_sfx_timer & 0x8000)) {
    uint16 v0 = earthquake_sfx_index;
    int16 v1 = kEarthquakeSfx[v0 >> 1];
    if (v1 < 0) {
      v0 = 0;
      v1 = kEarthquakeSfx[0];
    }
    QueueSfx2_Max6(v1);
    earthquake_sfx_timer = kEarthquakeSfx[(v0 >> 1) + 1] + (v0 & 1) + (random_number & 3);
    earthquake_sfx_index = v0 + 4;
  }
}

static void FxRisingFunction(void) {  /* CallFxRisingFunc: lava/ácido y agua */
  uint16 f = fx_rising_function_bank_88;
  switch (f) {
  case FX_RISE_LAVA_ACID:
  case FX_RISE_WATER:                                    // espera a que haya que subir o bajar
    if (fx_y_vel) {
      bool go = (fx_y_vel & 0x8000) ? (fx_target_y_pos != fx_base_y_pos && fx_target_y_pos < fx_base_y_pos)
                                    : !(fx_target_y_pos < fx_base_y_pos || fx_target_y_pos == fx_base_y_pos);
      if (go)
        fx_rising_function_bank_88 = f == FX_RISE_WATER ? FX_RISE_WATER_WAIT : FX_RISE_LAVA_ACID_WAIT;
    }
    break;
  case FX_RISE_LAVA_ACID_WAIT:
    HandleEarthquakeSoundEffect();
    earthquake_type = 21;
    earthquake_timer |= 0x20;
    if (!--fx_timer)
      fx_rising_function_bank_88 = FX_RISE_LAVA_ACID_RAISING;
    break;
  case FX_RISE_LAVA_ACID_RAISING:
    HandleEarthquakeSoundEffect();
    earthquake_type = 21;
    earthquake_timer |= 0x20;
    if (RaiseOrLowerFx() & 1) {
      fx_y_vel = 0;
      fx_rising_function_bank_88 = FX_RISE_LAVA_ACID;
    }
    break;
  case FX_RISE_WATER_WAIT:
    if (!--fx_timer)
      fx_rising_function_bank_88 = FX_RISE_WATER_RISING;
    break;
  case FX_RISE_WATER_RISING:
    if (RaiseOrLowerFx() & 1) {
      fx_rising_function_bank_88 = FX_RISE_WATER;
      fx_y_vel = 0;
    }
    break;
  default: nt_unimpl_v("FxRisingFunction (función sin portar)", f); break;
  }
}

static void FxHandleTide(void) {  // 0x88B2C9
  /* el original lee la palabra en 7E:197D (byte alto de fx_y_vel + byte bajo de fx_liquid_options): sus bits 15 y
     14 son los bits 7 y 6 de fx_liquid_options; escribe la palabra en 7E:1971 (fx_y_suboffset alto, fx_y_offset bajo) */
  if (!(fx_liquid_options & 0xC0))
    return;
  bool big = (fx_liquid_options & 0x80) != 0;            /* marea grande / pequeña */
  fx_y_suboffset = 0;
  fx_y_offset = 0;
  int16 sc = kSinCosTable8bit_Sext[HI8(tide_phase)];
  uint16 v = big ? (uint16)(8 * sc) : (uint16)(32 * sc);
  if (sc & (big ? 0x1000 : 0x400))
    --fx_y_offset;
  fx_y_suboffset = (uint16)(v << 8);
  fx_y_offset = (fx_y_offset & 0xFF00) | (v >> 8);
  if (big)
    tide_phase += (sc & 0x8000) ? 192 : 288;
  else
    tide_phase += (sc & 0x8000) ? 128 : 224;
}

static uint16 FxSurfacePos(void) {   /* (desplazamiento + base) de 16.16 */
  uint32 t = ((uint32)fx_y_offset << 16 | fx_y_suboffset) + ((uint32)fx_base_y_pos << 16 | fx_base_y_subpos);
  return t >> 16;
}
static uint16 FxSurfaceSub(void) {
  return fx_y_suboffset + fx_base_y_subpos;
}

static const uint8 kLavaSoundEffects[8] = { 0x12, 0x13, 0x14, 0x12, 0x13, 0x14, 0x12, 0x13 };

static void HdmaobjPreInstr_LavaAcidBG3YScroll(uint16 k) {  // 0x88B3B0
  fx_layer_blending_config_c = fx_layer_blending_config_b;
  if (time_is_frozen_flag)
    return;
  FxRisingFunction();
  FxHandleTide();
  int16 v2 = FxSurfacePos();
  lava_acid_y_subpos = FxSurfaceSub();
  lava_acid_y_pos = v2;
  bg3_xpos = reg_BG1HOFS;
  mbi_put16(0, 0);
  bg3_ypos = 0;
  int16 v4 = 0;
  if (v2 >= 0) {
    int16 v3 = v2 - layer1_y_pos;
    if (v3 <= 0)
      v4 = ((v3 ^ 0x1F) & 0x1F) | 0x100;
    else if ((uint16)v3 < 0x100)
      v4 = (uint8)~v3;
  }
  mbi_put16(2, v4);
  if (fx_type == 2 && !(lava_acid_y_pos & 0x8000)) {
    int v5 = (uint8)hdma_object_index >> 1;
    if (hdma_object_C[v5]-- == 1) {
      hdma_object_C[v5] = 112;
      QueueSfx2_Max6(kLavaSoundEffects[random_number & 7]);
    }
  }
  random_number = (uint16)(random_number << 8 | random_number >> 8);
  int16 v8 = 511;
  if (!(lava_acid_y_pos & 0x8000)) {
    v8 = lava_acid_y_pos - layer1_y_pos + 256;
    if (v8 < 0)
      v8 = 255;
    else if ((uint16)v8 >= 0x200)
      v8 = 511;
  }
  hdma_object_table_pointers[(uint8)hdma_object_index >> 1] = 3 * (((v8 ^ 0x1FF) + 1) & 0x3FF) + 0xB62A;
}

static void HdmaobjPreInstr_LavaAcidBG2YScroll(uint16 k) {  // 0x88B4D5
  int v = k >> 1;
  g_word_7E9C44 = reg_BG2VOFS;
  if (!time_is_frozen_flag && (fx_liquid_options & 6)) {
    if (fx_liquid_options & 2) {                           // Handle_LavaAcidBG2YScroll_Func3
      if (hdma_object_B[v]-- == 1) {
        hdma_object_B[v] = 4;
        hdma_object_A[v] = (hdma_object_A[v] - 2) & 0x1E;
      }
      uint8 v3 = ((uint8)hdma_object_A[v] + 2 * (reg_BG2VOFS & 0xF)) & 0x1E;
      uint8 v4 = (2 * (reg_BG2VOFS & 0xF) + 30) & 0x1E;
      for (int n = 0; n < 16; n++) {
        g_word_7E9C46[v4 >> 1] = (B88W(0xB60A + v3) + reg_BG2VOFS) & 0x1FF;
        v4 = (v4 - 2) & 0x1E;
        v3 = (v3 - 2) & 0x1E;
      }
    } else {                                               // Handle_LavaAcidBG2YScroll_Func2
      if (hdma_object_B[v]-- == 1) {
        hdma_object_B[v] = 6;
        hdma_object_A[v] = (hdma_object_A[v] - 2) & 0x1E;
      }
      uint8 v3 = (uint8)hdma_object_A[v];
      uint8 v4 = 30;
      for (int n = 0; n < 16; n++) {
        g_word_7E9C46[v4 >> 1] = (B88W(0xB589 + v3) + reg_BG2HOFS) & 0x1FF;
        v3 = (v3 - 2) & 0x1E;
        v4 = (v4 - 2) & 0x1E;
      }
    }
  } else {                                                 // Handle_LavaAcidBG2YScroll_Func1
    for (int i = 0; i < 16; i++)
      g_word_7E9C46[i] = reg_BG2VOFS & 0x1FF;
  }
  hdma_object_table_pointers[(uint8)hdma_object_index >> 1] = 3 * (reg_BG2VOFS & 0xF) + 0xC0B1;
}

static const int16 kWaterWave[16] = { 0, 1, 1, 0, 0, -1, -1, 0, 0, 1, 1, 0, 0, -1, -1, 0 };   /* $88:C46E */

static void HdmaobjPreInstr_WaterBG3XScroll(uint16 k) {  // 0x88C48E
  fx_layer_blending_config_c = fx_layer_blending_config_b;
  if (time_is_frozen_flag)
    return;
  FxRisingFunction();
  FxHandleTide();
  fx_y_subpos = FxSurfaceSub();
  fx_y_pos = FxSurfacePos();
  int16 v3 = fx_y_pos - layer1_y_pos;
  if ((int16)fx_y_pos < 0)
    bg3_ypos = 0;
  else if (v3 <= 0)
    bg3_ypos = ((v3 ^ 0x1F) & 0x1F) | 0x100;
  else if ((uint16)v3 < 0x100)
    bg3_ypos = (uint8)~v3;
  else
    bg3_ypos = 0;
  int v5 = k >> 1;
  uint16 r20 = layer1_x_pos + (int8)HI8(hdma_object_C[v5]);
  if (hdma_object_B[v5]-- == 1) {
    hdma_object_B[v5] = 10;
    hdma_object_A[v5] = (hdma_object_A[v5] + 2) & 0x1E;
  }
  uint8 v7 = (uint8)hdma_object_A[v5];
  for (int i = 30; i >= 0; i -= 2) {
    mbi_put16(v7 + 4, kWaterWave[i >> 1] + r20);
    v7 = (v7 - 2) & 0x1E;
  }
  if (fx_liquid_options & 1)
    hdma_object_C[v5] += 64;
  int16 v9 = fx_y_pos - layer1_y_pos + 256;
  if ((int16)fx_y_pos < 0)
    v9 = 511;
  else if (v9 >= 0) {
    if ((uint16)v9 >= 0x200)
      v9 = 511;
  } else {
    v9 = (v9 & 0xF) | 0x100;
  }
  hdma_object_table_pointers[v5] = 3 * (((v9 ^ 0x1FF) + 1) & 0x3FF) + 0xC645;
}

static void HdmaobjPreInstr_WaterBG2XScroll(uint16 k) {  // 0x88C589
  int v = (uint8)k >> 1;
  g_word_7E9C44 = reg_BG2HOFS;
  if (!time_is_frozen_flag && (fx_liquid_options & 2)) {   // HdmaobjPreInstr_WaterBG2XScroll_Func2
    if (hdma_object_B[v]-- == 1) {
      hdma_object_B[v] = 6;
      hdma_object_A[v] = (hdma_object_A[v] + 2) & 0x1E;
    }
    uint8 v3 = ((uint8)hdma_object_A[v] + 2 * (reg_BG2VOFS & 0xF)) & 0x1E;
    uint8 v4 = (2 * (reg_BG2VOFS & 0xF) + 30) & 0x1E;
    for (int n = 0; n < 16; n++) {
      g_word_7E9C46[(v4 >> 1) + 1] = kWaterWave[v3 >> 1] + reg_BG2HOFS;
      v4 = (v4 - 2) & 0x1E;
      v3 = (v3 - 2) & 0x1E;
    }
  } else {                                                 // HdmaobjPreInstr_WaterBG2XScroll_Func1
    if (!time_is_frozen_flag)
      hdma_object_channels_bitmask[v] = 0;
    for (int i = 0; i < 16; i++)
      g_word_7E9C46[i + 1] = reg_BG2HOFS;
  }
  int16 v1 = fx_y_pos - layer1_y_pos + 256;
  if ((int16)fx_y_pos < 0)
    v1 = 511;
  else if (v1 >= 0) {
    if ((uint16)v1 >= 0x200)
      v1 = 511;
  } else {
    v1 = ((fx_y_pos - layer1_y_pos) & 0xF) | 0x100;
  }
  hdma_object_table_pointers[v] = 3 * ((v1 ^ 0x1FF) & 0x3FF) + 0xCF46;
}

/* ------------------------------------------------------------------ cielo de la superficie ($88:ADC2)
   23 franjas del BG2 (kHdmaScrollEntrys, $88:AEC1: arriba, velocidad) con su X en scrolling_sky_bg2_hdma_data
   (7E:9F80: pares fracción/píxel); la tabla indirecta (7E:9F00: líneas, dirección de la franja) dice qué franja va
   en cada línea de la pantalla. La Genesis lee ambas para el scroll por líneas del cielo. */
#define SKY_ENTRY(i, f) B88W(0xAEC1 + 8 * (i) + 2 * (f))   /* f: 0 arriba, 1 subvelocidad, 2 velocidad, 3 franja */
static void sky_put16(uint16 off, uint16 v) {
  scrolling_sky_bg2_indirect_hdma[off] = (uint8)v;
  scrolling_sky_bg2_indirect_hdma[off + 1] = (uint8)(v >> 8);
}
static void HdmaobjPreInstr_SkyLandBG2XscrollInner(void) {  // 0x88ADC2
  reg_BG2SC = 74;
  for (int v = 0; v < 23; v++) {
    int e = (uint16)(SKY_ENTRY(v, 3) - 0x9F80) >> 1;
    uint32 cur = (uint32)scrolling_sky_bg2_hdma_data[e + 1] << 16 | scrolling_sky_bg2_hdma_data[e];
    cur += (uint32)SKY_ENTRY(v, 2) << 16 | SKY_ENTRY(v, 1);
    scrolling_sky_bg2_hdma_data[e] = (uint16)cur;
    scrolling_sky_bg2_hdma_data[e + 1] = (uint16)(cur >> 16);
  }
  scrolling_sky_bg2_hdma_data[44] = 0;
  scrolling_sky_bg2_hdma_data[45] = 0;
  sky_put16(0, 31);
  sky_put16(1, 1438);
  uint16 r18 = layer1_y_pos + 32, r20 = layer1_y_pos + 224, r24;
  uint16 v5 = 0, v6 = 3;
  do {
    while ((int16)(r18 - SKY_ENTRY(v5, 0)) >= 0 && (int16)(r18 - SKY_ENTRY(v5 + 1, 0)) < 0) {
      uint16 v8 = SKY_ENTRY(v5 + 1, 0) - r18;
      r24 = v8;
      if (!sign16(v8 - 128)) {
        sky_put16(v6, 127);
        sky_put16(v6 + 1, SKY_ENTRY(v5, 3) + 2);
        v6 += 3;
        v8 = r24 - 127;
      }
      sky_put16(v6, v8);
      sky_put16(v6 + 1, SKY_ENTRY(v5, 3) + 2);
      r18 += r24;
      v6 += 3;
      if (!sign16(r18 - r20)) {
        sky_put16(v6 + 3, 0);
        return;
      }
    }
    ++v5;
  } while (sign16(v5 * 8 - 184));
  uint16 i;
  for (i = 1535 - r18;; i = r24 - 127) {
    r24 = i;
    if (sign16(i - 128))
      break;
    sky_put16(v6, 127);
    sky_put16(v6 + 1, 181);
    v6 += 3;
  }
  sky_put16(v6, i);
  sky_put16(v6 + 1, 181);
  sky_put16(v6 + 3, 0);
}

/* ------------------------------------------------------------------ luciérnagas ($88:B0BC) */
static const uint16 kFirefleaFlashingShades[12] = { 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x500, 0x400, 0x300, 0x200, 0x100 };
static const uint16 kFirefleaDarknessShades[6] = { 0, 0x600, 0xC00, 0x1200, 0x1800, 0x1900 };

static void HdmaobjPreInstr_FirefleaBG3XScroll(uint16 k) {  // 0x88B0BC: la sala se oscurece (COLDATA)
  (void)k;
  fx_layer_blending_config_c = (fx_layer_blending_config_c & 0xFF00) | 0xC;
  if (time_is_frozen_flag)
    return;
  if (!--fireflea_flashing_timer) {
    fireflea_flashing_timer = 6;
    uint16 v1;
    if (sign16(fireflea_darkness_level - 10)) {
      v1 = fireflea_flashing_index + 1;
      if (v1 >= 0xC)
        v1 = 0;
    } else {
      v1 = 6;
    }
    fireflea_flashing_index = v1;
  }
  uint8 v2 = (uint16)(kFirefleaDarknessShades[fireflea_darkness_level >> 1] + kFirefleaFlashingShades[fireflea_flashing_index]) >> 8;
  reg_COLDATA[1] = v2 | 0x80;
  reg_COLDATA[2] = v2 | 0x40;
  reg_COLDATA[0] = v2 | 0x20;
}

/* ------------------------------------------------------------------ estatua de la entrada de Tourian ($88:DBCB) */
static void StatueSetBg2(uint16 k) {  // sub_88DBCB
  statue_bg2_ypos = layer1_y_pos + hdma_object_B[k >> 1];
}

static void HdmaobjPreInstr_CheckLotsOfEventsHappened(uint16 k) {  // 0x88DBD7: los 4 jefes muertos
  if ((CheckEventHappened(6) & 1) && (CheckEventHappened(7) & 1) && (CheckEventHappened(8) & 1) &&
      (CheckEventHappened(9) & 1)) {
    tourian_entrance_statue_animstate |= 0x10;
    if (!(tourian_entrance_statue_animstate & 0x8000)) {
      hdma_object_C[k >> 1] = 300;
      int v1 = hdma_object_index >> 1;
      hdma_object_instruction_timers[v1] = 1;
      hdma_object_instruction_list_pointers[v1] += 2;
      k = hdma_object_index;
    }
  }
  StatueSetBg2(k);
}

static void HdmaobjPreInstr_DC23(uint16 k) {  // 0x88DC23: tiembla y suelta polvo
  HandleEarthquakeSoundEffect();
  earthquake_type = 13;
  earthquake_timer |= 0x20;
  int v1 = (uint8)k >> 1;
  if (--hdma_object_C[v1] & 0x8000) {
    for (int i = 0; i < 4; i++)
      SpawnEprojWithRoomGfx(addr_kEproj_TourianStatueDustClouds, 0);
    hdma_object_instruction_timers[v1] = 1;
    hdma_object_instruction_list_pointers[v1] += 2;
  }
  StatueSetBg2(k);
}

static void HdmaobjPreInstr_DC69(uint16 k) {  // 0x88DC69: la estatua baja
  HandleEarthquakeSoundEffect();
  earthquake_type = 13;
  earthquake_timer |= 0x20;
  if (!time_is_frozen_flag) {
    int v1 = k >> 1;
    uint32 t = ((uint32)hdma_object_B[v1] << 16 | hdma_object_A[v1]) - 0x4000;
    hdma_object_B[v1] = t >> 16;
    hdma_object_A[v1] = (uint16)t;
    if (hdma_object_B[v1] == 0xFF10) {
      SpawnHardcodedPlm(0x06, 0x0C, 0xB773);
      SetEventHappened(0xA);
      hdma_object_instruction_timers[v1] = 1;
      hdma_object_instruction_list_pointers[v1] += 2;
    }
    StatueSetBg2(k);
  }
}

static void HdmaobjPreInstr_DCBA(uint16 k) {  // 0x88DCBA
  tourian_entrance_statue_finished = 0x8000;
  scrolls[0] = 2;
  scrolls[1] = 2;
  StatueSetBg2(k);
}

/* ------------------------------------------------------------------ mezcla de capas ($88:8000) */
static void InitializeLayerBlending(void) {  // 0x888075
  reg_W12SEL = 0;
  reg_W34SEL = 0;
  reg_WOBJSEL = 0;
  reg_TM = 19;
  reg_TS = 4;
  reg_TMW = 0;
  reg_TSW = 0;
  next_gameplay_CGWSEL = 2;
  next_gameplay_CGADSUB = 51;
}

static uint16 LayerBlendFunc(uint8 f, uint16 j) {  /* kLayerBlendFuncTable ($88:8000 + f) */
  switch (f) {
  case 0x04: reg_TM = 17; reg_TS = 4; break;                                  // LayerBlendFunc_4_PhantoonIntro
  case 0x06: reg_TS = 20; next_gameplay_CGADSUB = 39; break;
  case 0x08: reg_TS = 20; next_gameplay_CGADSUB = 34; break;
  case 0x0A: next_gameplay_CGADSUB = 50; break;
  case 0x0C: next_gameplay_CGWSEL = 0; next_gameplay_CGADSUB = 0xA2; break;
  case 0x10: case 0x12: reg_W34SEL = 2; reg_WOBJSEL = 32; reg_TSW = 4; break;  // LayerBlendFunc_10
  case 0x14: case 0x22: next_gameplay_CGADSUB = 0xB3; break;                  // LayerBlendFunc_14
  case 0x16: j = (j & 0xFF00) | 4; reg_TM = 17; reg_TS = 6; next_gameplay_CGADSUB = 0xB1; break;
  case 0x18: case 0x1E: case 0x30:                                             // LayerBlendFunc_18
    j = 2;
    if ((reg_NMITIMEN & 0x30) == 0x30) {
      next_gameplay_CGADSUB = 36;
      reg_TS = 19;
      reg_TM = 4;
    }
    break;
  case 0x1A: j = (j & 0xFF00) | 4; reg_TM = 21; reg_TS = 2; next_gameplay_CGADSUB = 53; break;
  case 0x1C: reg_TM = 21; reg_TS = 2; next_gameplay_CGADSUB = 85; break;
  case 0x24:
    reg_W12SEL = 0; reg_W34SEL = 2; reg_WOBJSEL = 32; reg_TM = 19; reg_TS = 4; reg_TMW = 19; reg_TSW = 4;
    next_gameplay_CGWSEL = 16; next_gameplay_CGADSUB = 51;
    break;
  case 0x26: next_gameplay_CGADSUB = 119; break;
  case 0x28: case 0x2A:                                                        // tinte de color (COLDATA)
    next_gameplay_CGWSEL = 0;
    next_gameplay_CGADSUB = 0xB3;
    if (!(fx_layer_blending_config_c & 0x8000)) {
      reg_COLDATA[0] = f == 0x28 ? 37 : 38;
      reg_COLDATA[1] = f == 0x28 ? 64 : 66;
      reg_COLDATA[2] = 0x80;
    }
    break;
  case 0x2C: next_gameplay_CGWSEL = 0; break;
  case 0x2E: next_gameplay_CGADSUB = 0xB3; break;
  case 0x32: reg_TS = 68; next_gameplay_CGADSUB = 0xB2; break;
  case 0x34: j = 6; break;
  default: break;                                                              // nullsub_9/10/11
  }
  return j;
}

static void LayerBlendingHandler(void) {  // 0x888000
  uint16 j = 0;
  uint8 f = (uint8)fx_layer_blending_config_c;
  if (f) {
    InitializeLayerBlending();
    j = LayerBlendFunc(f, j);
  }
  if (!(fx_layer_blending_config_c & 0x8000)) {
    if (fx_layer_blending_config_c & 0x4000) {          // HandleLayerBlendingXrayCanShowBlocks
      reg_W12SEL = 0xC8; reg_W34SEL = 8; reg_WOBJSEL = 0x80; reg_TM = 19; reg_TS = 4; reg_TMW = 3; reg_TSW = 4;
      next_gameplay_CGWSEL = 34;
      next_gameplay_CGADSUB = (next_gameplay_CGADSUB & 0x80) | 0x73;
    } else if (fx_layer_blending_config_c & 0x2000) {   // HandleLayerBlendingXrayCantShowBlocks
      reg_W12SEL = 0; reg_W34SEL = 8; reg_WOBJSEL = 0x80; reg_TM = 19; reg_TS = 4; reg_TMW = 3; reg_TSW = 4;
      next_gameplay_CGWSEL = 34;
      next_gameplay_CGADSUB = (next_gameplay_CGADSUB & 0x80) | 0x61;
      if (room_ptr == 0xCEFB)
        reg_TM = 17;
    } else if (fx_layer_blending_config_c & 0x1000) {   // HandleLayerBlendingXrayFirefleaRoom
      reg_W12SEL = 0; reg_W34SEL = 8; reg_WOBJSEL = 0x80; reg_TM = 19; reg_TS = 0; reg_TMW = 3; reg_TSW = 4;
      next_gameplay_CGWSEL = 32;
      next_gameplay_CGADSUB = 0xB3;
    }
  } else {                                              // HandleLayerBlendingPowerBomb
    if (room_ptr == 0xA66A)
      j = 6;
    reg_W12SEL = (j >> 1) == 2 ? 0x80 : 0;
    reg_W34SEL = 8;
    reg_WOBJSEL = 0x80;
    next_gameplay_CGWSEL = 2;
    next_gameplay_CGADSUB = (j >> 1) == 3 ? 49 : 55;
    reg_TMW = 0;
    reg_TSW = (j >> 1) == 2 ? 6 : 4;
    reg_TM = (j >> 1) == 2 ? 17 : 19;
    reg_TS = (j >> 1) == 2 ? 6 : 4;
  }
}

/* ------------------------------------------------------------------ manejador ($88:84B9) */
static void CallHdmaobjPreInstr(uint16 f, uint16 k) {
  switch (f) {
  case FUNC16(nullsub_293): case addr_locret_88858A: case FUNC16(nullsub_56): break;
  case FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_CeresRidleyAlive): HazeStart(k, 0x80); break;   // 0x88DE10
  case FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_CeresRidleyDead): HazeStart(k, 0x20); break;    // 0x88DE15
  case FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn): HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn(k); break;
  case FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadedIn): HdmaobjPreInstr_HazeColorMathSubscreen_FadedIn(k); break;
  case FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingOut): HdmaobjPreInstr_HazeColorMathSubscreen_FadingOut(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_SetWindowConf): HdmaobjPreInstr_PowerBombExplode_SetWindowConf(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_PreExplosionWhite): HdmaobjPreInstr_PowerBombExplode_PreExplosionWhite(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_PreExplosionYellow): HdmaobjPreInstr_PowerBombExplode_PreExplosionYellow(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_ExplosionYellow): HdmaobjPreInstr_PowerBombExplode_ExplosionYellow(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_ExplosionWhite): HdmaobjPreInstr_PowerBombExplode_ExplosionWhite(k); break;
  case FUNC16(HdmaobjPreInstr_PowerBombExplode_Stage5_Afterglow): HdmaobjPreInstr_PowerBombExplode_Stage5_Afterglow(k); break;
  case FUNC16(HdmaobjPreInstr_RainBg3Scroll): HdmaobjPreInstr_RainBg3Scroll(k); break;
  case FUNC16(HdmaobjPreInstr_SporesBG3Xscroll): HdmaobjPreInstr_SporesBG3Xscroll(k); break;
  case FUNC16(HdmaobjPreInstr_FogBG3Scroll): HdmaobjPreInstr_FogBG3Scroll(k); break;
  case FUNC16(HdmaobjPreInstr_IntroCutsceneCrossfade):                       // 0x88EC1D
    if (cinematic_var13 == 1) {                 /* eproj_x_pos[0] del decomp: 7E:1A4B */
      int v1 = hdma_object_index >> 1;
      hdma_object_instruction_list_pointers[v1] += 2;
      hdma_object_instruction_timers[v1] = 1;
    }
    break;
  case FUNC16(HdmaobjPreInstr_FirefleaBG3XScroll): HdmaobjPreInstr_FirefleaBG3XScroll(k); break;
  case FUNC16(HdmaobjPreInstr_CheckLotsOfEventsHappened): HdmaobjPreInstr_CheckLotsOfEventsHappened(k); break;
  case FUNC16(HdmaobjPreInstr_DC23): HdmaobjPreInstr_DC23(k); break;
  case FUNC16(HdmaobjPreInstr_DC69): HdmaobjPreInstr_DC69(k); break;
  case FUNC16(HdmaobjPreInstr_DCBA): HdmaobjPreInstr_DCBA(k); break;
  case FUNC16(HdmaobjPreInstr_LavaAcidBG3YScroll): HdmaobjPreInstr_LavaAcidBG3YScroll(k); break;
  case FUNC16(HdmaobjPreInstr_LavaAcidBG2YScroll): HdmaobjPreInstr_LavaAcidBG2YScroll(k); break;
  case FUNC16(HdmaobjPreInstr_WaterBG3XScroll): HdmaobjPreInstr_WaterBG3XScroll(k); break;
  case FUNC16(HdmaobjPreInstr_WaterBG2XScroll): HdmaobjPreInstr_WaterBG2XScroll(k); break;
  case FUNC16(HdmaobjPreInstr_SkyLandBG2Xscroll): case FUNC16(HdmaobjPreInstr_SkyLandBG2Xscroll2):
    if (!time_is_frozen_flag) HdmaobjPreInstr_SkyLandBG2XscrollInner();
    break;
  case FUNC16(HdmaobjPreInstr_E9E6): HdmaobjPreInstr_E9E6(k); break;     /* haz del ojo de la sala de la bola */
  case FUNC16(HdmaobjPreInstr_EA3C): HdmaobjPreInstr_EA3C(k); break;
  case FUNC16(HdmaobjPreInstr_EACB): HdmaobjPreInstr_EACB(k); break;
  case FUNC16(HdmaobjPreInstr_BombTorizoHazeColorMathBgColor): fx_layer_blending_config_c = 44; break;   // 0x88DD43
  case FUNC16(HdmaobjPreInstr_VariaSuitPickup): HdmaobjPreInstr_SuitPickup(k, false); break;   // 0x88E026
  case FUNC16(HdmaobjPreInstr_GravitySuitPickup): HdmaobjPreInstr_SuitPickup(k, true); break;  // 0x88E05C
  default: nt_unimpl_v("HdmaobjPreInstr (pre-instrucción sin portar)", f); break;
  }
}

static void CallHdmaobjFarFunc(uint32 ea, uint16 k) {   /* HdmaobjInstr_CallFarFunc */
  switch (ea) {
  case fnHdmaobj_PreExplodeWhite: Hdmaobj_PreExplodeWhite(); break;
  case fnHdmaobj_PreExplodeYellow: Hdmaobj_PreExplodeYellow(); break;
  case fnHdmaobj_ExplodeYellow: Hdmaobj_ExplodeYellow(); break;
  case fnHdmaobj_ExplodeWhite: Hdmaobj_ExplodeWhite(); break;
  case fnHdmaobj_CleanUpTryCrystalFlash: Hdmaobj_CleanUpTryCrystalFlash(k); break;
  case fnInitializeSuitPickupHdma: InitializeSuitPickupHdma(); break;
  default: nt_unimpl_v("HdmaobjInstr_CallFarFunc (función sin portar)", ea); break;
  }
}

static uint16 ListW(uint8 bank, uint16 a) {
  switch (bank) {
  case 0x88: return B88W(a);
  case 0x81: return B81W(a);
  case 0x91: {                                   /* el rayo de luz de los trajes ($91:D5A2-D691) */
    const uint8 *q = kBank91_SuitHdma + (uint16)(a - 0xD5A2);
    uint16 lo = q[0];
    __asm__("" : "+d"(lo));
    return lo | q[1] << 8;
  }
  default: nt_unimpl("objeto de HDMA (banco sin preparar)"); return FUNC16(HdmaobjInstr_Delete);
  }
}

/* una instrucción: devuelve el puntero siguiente o 0 (el objeto espera / se borró) */
static uint16 HdmaobjInstr(uint16 f, uint16 k, uint16 p, uint8 bank) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(HdmaobjInstr_Delete): hdma_object_channels_bitmask[v] = 0; return 0;                              // Delete
  case FUNC16(HdmaobjInstr_SetPreInstr):                                                                             // SetPreInstr
    hdma_object_pre_instructions[v] = ListW(bank, p);
    set_lo8(hdma_object_pre_instruction_bank, v, (uint8)ListW(bank, p + 2));
    return p + 3;
  case FUNC16(HdmaobjInstr_ClearPreInstr): hdma_object_pre_instructions[v] = addr_locret_88858A; return p;          // ClearPreInstr
  case FUNC16(HdmaobjInstr_CallFarFunc):                                                                             // CallFarFunc
    CallHdmaobjFarFunc((uint32)(uint8)ListW(bank, p + 2) << 16 | ListW(bank, p), k);
    return p + 3;
  case FUNC16(HdmaobjInstr_Goto): return ListW(bank, p);                                                      // Goto
  case FUNC16(HdmaobjInstr_GotoRel): return p + (int8)ListW(bank, p);                                            // GotoRel
  case FUNC16(HdmaobjInstr_DecrementAndGoto): return (hdma_object_timers[v]-- == 1) ? p + 2 : ListW(bank, p);             // DecrementAndGoto
  case FUNC16(HdmaobjInstr_DecrementAndGotoRel): return (hdma_object_timers[v]-- == 1) ? p + 1 : p + (int8)ListW(bank, p);   // DecrementAndGotoRel
  case FUNC16(HdmaobjInstr_SetTimer): set_lo8(hdma_object_timers, v, (uint8)ListW(bank, p)); return p + 1;        // SetTimer
  case FUNC16(HdmaobjInstr_SetHdmaControl): case FUNC16(HdmaobjInstr_SetHdmaTarget):
  case FUNC16(HdmaobjInstr_SetHdmaTableBank): case FUNC16(HdmaobjInstr_SetIndirectHdmaDataBank):
    return p + 1;                                /* registros de DMA del SNES */
  case FUNC16(HdmaobjInstr_SetHdmaTablePtr): hdma_object_table_pointers[v] = ListW(bank, p); return p + 2;               // SetHdmaTablePtr
  case FUNC16(HdmaobjInstr_Sleep): hdma_object_instruction_list_pointers[v] = p - 2; return 0;                 // Sleep
  case FUNC16(sub_88D916): case FUNC16(HdmaobjInstr_SetVideoMode1): hdma_data_table_in_ceres = 9; return p;
  case FUNC16(HdmaobjInstr_InitMorphBallEyeBeamHdma): HdmaobjInstr_InitMorphBallEyeBeamHdma(k); return p;   // $88:E917
  case FUNC16(nullsub_112): return p;                                                                        // $88:DB2E
  case FUNC16(HdmaobjInstr_GotoIfEventHappened):                                                             // $88:DCCB
    hdma_object_C[v] = 0;
    hdma_object_A[v] = 0;
    if (CheckEventHappened(0xA)) {
      hdma_object_B[v] = -240;
      statue_bg2_ypos = -240;
      return ListW(bank, p);
    }
    hdma_object_B[v] = 0;
    statue_bg2_ypos = 0;
    scrolls[0] = 1;
    scrolls[1] = 0;
    return p + 2;
  case FUNC16(HdmaobjInstr_B3A9): hdma_object_C[v] = 112; return p;                                          // $88:B3A9
  case FUNC16(HdmaobjInstr_SetFlagB_Copy): case FUNC16(HdmaobjInstr_SetFlagB_Copy2):
  case FUNC16(HdmaobjInstr_SetFlagB_Copy3): case FUNC16(HdmaobjInstr_SetFlagB):
    hdma_object_B[v] = 1;
    return p;
  case FUNC16(HdmaobjInstr_1938_RandomNumber):                                                                     // $88:D981
    hdma_object_D[v] = B88W(0xD992 + ((random_number >> 1) & 6));
    return p;
  default:
    nt_unimpl_v("HdmaobjInstr (instrucción sin portar)", f);
    hdma_object_channels_bitmask[v] = 0;
    return 0;
  }
}

static void HdmaobjInstructionHandler(uint16 k) {  // 0x88851C
  int v = k >> 1;
  CallHdmaobjPreInstr(hdma_object_pre_instructions[v], k);
  if (hdma_object_instruction_timers[v]-- == 1) {
    uint8 bank = HI8(hdma_object_bank_slot[v]);
    uint16 p = hdma_object_instruction_list_pointers[v];
    uint16 f;
    while ((f = ListW(bank, p)) & 0x8000) {
      p = HdmaobjInstr(f, k, p + 2, bank);
      if (!p) return;
    }
    hdma_object_instruction_timers[v] = f;
    hdma_object_table_pointers[v] = ListW(bank, p + 2);
    hdma_object_instruction_list_pointers[v] = p + 4;
  }
}

void HdmaObjectHandler(void) {  // 0x8884B9 (HandleMusicQueue va antes, en game_frame)
  if (!time_is_frozen_flag && (power_bomb_explosion_status & 0x4000)) {
    power_bomb_explosion_status = 0x8000;
    SpawnHdmaObject(0x88, 0x8ACE);
    SpawnHdmaObject(0x88, 0x8B80);
  }
  fx_layer_blending_config_c = fx_layer_blending_config_a;
  if (hdma_objects_enable_flag & 0x8000) {
    reg_HDMAEN = 0;
    for (uint16 k = 0; k != 12; k += 2) {
      hdma_object_index = k;
      uint8 ch = (uint8)hdma_object_channels_bitmask[k >> 1];
      if (ch) {
        reg_HDMAEN |= ch;
        HdmaobjInstructionHandler(k);
      }
    }
    LayerBlendingHandler();
  }
}

void SpawnBombTorizoHaze(void) {  // 0x88DD32: neblina del Bomb Torizo (color de fondo con la matemática de color)
  SpawnHdmaObject(0x88, 0xDD4A);
  SpawnHdmaObject(0x88, 0xDD62);
}

/* ------------------------------------------------------------------ cono de luz (rayos X, $91:BE11; ojo de la bola, $88:E8D9)
 * La tabla de la ventana del SNES (7E:9100, una palabra por línea: izquierda en el byte bajo, derecha en el alto;
 * 0x00FF sin cono) la calcula el motor igual que el SNES; la Genesis la dibuja con sprites (md_cone.c). */
/* (con -O3 y el desenrollado del motor estas funciones ocupaban 88 KB: se compilan normales) */
#pragma GCC optimize("O2", "no-unroll-loops", "no-inline-functions")
#ifdef NT_SSF2
/* Genesis: la tabla se escribe por partes, en el tiempo que le sobra a cada cuadro (md_cone.c). Mientras
   xjob_rec está puesto, las cuatro funciones de relleno no escriben: anotan qué escribir (dónde empiezan, con qué
   valores y pasos) y xray_job_run lo escribe después, de a unas líneas, con las mismas cuentas. Lo que el SNES
   escribe directo (la línea del ojo) se escribe al anotar. */
typedef struct { uint16 *dest; int pos; uint8 ramp, up, fin; int adjust; uint16 fill; uint32 l, ls, r, rs; } XrayJob;
static XrayJob xjob[3];
static uint8 xjob_n, xjob_i, xjob_rec;
static void xjob_add_const(uint16 *dest, uint8 up, int v1, uint16 fill) {
  XrayJob *j = &xjob[xjob_n++];
  /* XrayFillUp/Down escriben v1 y siguen (do-while): igual que empezar un paso antes con el chequeo primero */
  j->dest = dest; j->ramp = 0; j->up = up; j->fin = 1; j->fill = fill; j->pos = up ? v1 + 2 : v1 - 2;
}
static void xjob_add_ramp(uint16 *dest, uint8 up, int pos, uint32 l, uint32 ls, uint32 r, uint32 rs, int adjust) {
  XrayJob *j = &xjob[xjob_n++];
  j->dest = dest; j->ramp = 1; j->up = up; j->fin = 0; j->fill = 0xff; j->pos = pos;
  j->l = l; j->ls = ls; j->r = r; j->rs = rs; j->adjust = adjust;
}
/* xray_ramp_fast(&f, n): hasta n líneas de una rampa con las mismas cuentas que XrayFill*Ramped mientras al menos
   un borde quede dentro (0 a 0xFFFF; el otro se recorta igual que allí) o los dos estén fuera sin terminar. La
   línea en que termina la rampa (los dos fuera y alejándose) la deja sin sumar para C. Vuelve cuántas escribió. */
typedef struct { uint16 *d; uint32 l, r, ls, rs; int16 step; } XrayFast;
uint16 xray_ramp_fast(XrayFast *f, uint16 n);
__asm__(
  "    .text\n"
  "    .global xray_ramp_fast\n"
  "xray_ramp_fast:\n"
  "    movem.l %d2-%d7/%a2,-(%sp)\n"
  "    move.l 32(%sp),%a1\n"
  "    move.w 38(%sp),%d7\n"
  "    move.l (%a1),%a0\n"
  "    move.l 4(%a1),%d0\n"
  "    move.l 8(%a1),%d1\n"
  "    move.l 12(%a1),%d2\n"
  "    move.l 16(%a1),%d3\n"
  "    move.w 20(%a1),%d6\n"
  "    move.w %d7,%a2\n"
  "    subq.w #1,%d7\n"
  "1:  add.l %d2,%d0\n"
  "    add.l %d3,%d1\n"
  "    move.l %d0,%d4\n"
  "    or.l %d1,%d4\n"
  "    swap %d4\n"
  "    tst.w %d4\n"
  "    bne 10f\n"
  "    move.w %d1,(%a0)\n"                    /* los dos dentro: alto = derecha >> 8, bajo = izquierda >> 8 */
  "    move.w %d0,-(%sp)\n"
  "    move.b (%sp)+,1(%a0)\n"
  "    adda.w %d6,%a0\n"
  "    dbra %d7,1b\n"
  "    bra 9f\n"
  "10: move.l %d0,%d4\n"
  "    swap %d4\n"
  "    tst.w %d4\n"
  "    bne 3f\n"                              /* izquierda fuera */
  "    move.w %d0,%d4\n"
  "    lsr.w #8,%d4\n"
  "2:  move.l %d1,%d5\n"
  "    swap %d5\n"
  "    tst.w %d5\n"
  "    bne 5f\n"                              /* derecha fuera */
  "    move.w %d1,%d5\n"
  "    clr.b %d5\n"
  "6:  or.w %d5,%d4\n"
  "7:  move.w %d4,(%a0)\n"
  "    adda.w %d6,%a0\n"
  "    dbra %d7,1b\n"
  "    bra 9f\n"
  "3:  bmi 4f\n"                              /* izquierda >= 0x10000: 0xFF (si la derecha está dentro) */
  "    move.l %d1,%d5\n"
  "    swap %d5\n"
  "    tst.w %d5\n"
  "    bne 8f\n"                              /* las dos fuera: en C (puede terminar) */
  "    move.w #0xFF,%d4\n"
  "    bra 7b\n"
  "4:  move.l %d1,%d5\n"                      /* izquierda < 0: 0 */
  "    swap %d5\n"
  "    tst.w %d5\n"
  "    bne 8f\n"
  "    moveq #0,%d4\n"
  "    bra 2b\n"
  "5:  bmi 51f\n"                             /* derecha >= 0x10000: 0xFF00 */
  "    move.w #0xFF00,%d5\n"
  "    bra 6b\n"
  "51: move.w #0xFF,%d4\n"                    /* derecha < 0: 0xFF */
  "    bra 7b\n"
  "8:  move.l %d2,%d4\n"                      /* las dos fuera: ¿termina? (los dos se alejan de la pantalla) */
  "    eor.l %d0,%d4\n"
  "    move.l %d3,%d5\n"
  "    eor.l %d1,%d5\n"
  "    or.l %d5,%d4\n"
  "    bpl 88f\n"
  "    tst.l %d1\n"                          /* no: derecha < 0 o izquierda >= 0x10000 -> 0xFF; si no, 0xFF00 */
  "    bmi 81f\n"
  "    tst.l %d0\n"
  "    bpl 81f\n"
  "    move.w #0xFF00,%d4\n"
  "    bra 7b\n"
  "81: move.w #0xFF,%d4\n"
  "    bra 7b\n"
  "88: sub.l %d2,%d0\n"                      /* termina: esa línea, en C */
  "    sub.l %d3,%d1\n"
  "9:  move.l %d0,4(%a1)\n"
  "    move.l %d1,8(%a1)\n"
  "    move.w %a2,%d0\n"
  "    sub.w %d7,%d0\n"
  "    subq.w #1,%d0\n"
  "    movem.l (%sp)+,%d2-%d7/%a2\n"
  "    rts\n");
/* xray_fill_fast(d, v, n, paso): n palabras v desde d, de a paso bytes */
void xray_fill_fast(uint16 *d, uint16 v, uint16 n, int16 step);
__asm__(
  "    .text\n"
  "    .global xray_fill_fast\n"
  "xray_fill_fast:\n"
  "    movea.l 4(%sp),%a0\n"
  "    move.w 10(%sp),%d1\n"
  "    move.w 14(%sp),%d0\n"
  "    move.w 18(%sp),%a1\n"
  "    subq.w #1,%d0\n"
  "    bmi 2f\n"
  "1:  move.w %d1,(%a0)\n"
  "    adda.w %a1,%a0\n"
  "    dbra %d0,1b\n"
  "2:  rts\n");
/* hasta budget líneas de lo anotado; 1 si ya está toda la tabla */
uint8 xray_job_run(uint16 budget) {
  while (xjob_i < xjob_n) {
    XrayJob *j = &xjob[xjob_i];
    uint16 *dest = j->dest;
    int pos = j->pos;
    if (!j->fin) {                     /* XrayFillUpRamped / XrayFillDownRamped */
      uint32 left = j->l, right = j->r, left_step = j->ls, right_step = j->rs;
      uint8 out = 0;
      for (;;) {
        /* las líneas con algún borde dentro de la pantalla (casi todas), en ensamblador */
        int nb = j->up ? (pos >> 1) + 1 : ((458 - pos) >> 1) + 1;   /* líneas hasta el borde de la tabla */
        if (pos >= 0 && pos <= 458) {
          uint16 n = nb < budget ? nb : budget;
          XrayFast f = { &dest[pos >> 1], left, right, left_step, right_step, j->up ? -2 : 2 };
          uint16 done = xray_ramp_fast(&f, n);
          budget -= done;
          left = f.l; right = f.r;
          pos += j->up ? -2 * done : 2 * done;
          if (done == nb) { out = 1; break; }       /* llegó al borde de la tabla */
        }
        if (!budget) { j->pos = pos; j->l = left; j->r = right; return 0; }
        budget--;
        left += left_step;
        right += right_step;
        uint32 left_val = (left & ~0xffff) ? (((int32)left < 0) ? 0 : 0xffff) : left;
        uint32 right_val = (right & ~0xffff) ? (((int32)right < 0) ? 0 : 0xffff) : right;
        bool finished = (left & ~0xffff) && (right & ~0xffff) && (((left_step ^ left) | (right_step ^ right)) >> 31) == 0;
        dest[pos >> 1] = ((int32)right < 0 || (int32)left >= 0x10000) ? 0xff : (left_val >> 8 | (right_val & ~0xff));
        if (finished) { pos += j->adjust; j->fin = 1; break; }
        if (j->up ? (pos -= 2) < 0 : (pos += 2) >= 460) { out = 1; break; }
      }
      if (out) { xjob_i++; continue; }
    }
    /* el resto con un valor fijo (XrayFillUp / XrayFillDown, o el final de las rampas) */
    {
      int first = j->up ? pos - 2 : pos + 2;           /* la primera que se escribe */
      int left_n = j->up ? (first >= 0 ? (first >> 1) + 1 : 0) : (first < 460 ? ((458 - first) >> 1) + 1 : 0);
      if (left_n > budget) {
        xray_fill_fast(&dest[first >> 1], j->fill, budget, j->up ? -2 : 2);
        j->pos = pos + (j->up ? -2 : 2) * budget;
        return 0;
      }
      if (left_n) xray_fill_fast(&dest[first >> 1], j->fill, left_n, j->up ? -2 : 2);
      budget -= left_n;
    }
    xjob_i++;
  }
  return 1;
}
#endif
typedef struct XrayHdmaCtx {
  uint16 r22, r24, r26, r28, r30, r32, r18_avoid;
  uint16 *dest;
} XrayHdmaCtx;

static __attribute__((noinline)) void XrayFillUp(uint16 *dest, uint16 fill_with, uint16 v1) {
#ifdef NT_SSF2
  if (xjob_rec) { xjob_add_const(dest, 1, (int16)v1, fill_with); return; }
#endif
  do dest[v1 >> 1] = fill_with; while (!sign16(v1 -= 2));
}
static __attribute__((noinline)) void XrayFillDown(uint16 *dest, uint16 fill_with, uint16 v1) {
#ifdef NT_SSF2
  if (xjob_rec) { xjob_add_const(dest, 0, v1, fill_with); return; }
#endif
  do dest[v1 >> 1] = fill_with; while ((v1 += 2) < 460);
}
static __attribute__((noinline)) void XrayFillUpRamped(uint16 *dest, int pos, uint32 left, uint32 left_step, uint32 right, uint32 right_step, int adjust) {
#ifdef NT_SSF2
  if (xjob_rec) { xjob_add_ramp(dest, 1, pos, left, left_step, right, right_step, adjust); return; }
#endif
  do {
    left += left_step;
    right += right_step;
    uint32 left_val = (left & ~0xffff) ? (((int32)left < 0) ? 0 : 0xffff) : left;
    uint32 right_val = (right & ~0xffff) ? (((int32)right < 0) ? 0 : 0xffff) : right;
    bool finished = (left & ~0xffff) && (right & ~0xffff) && (((left_step ^ left) | (right_step ^ right)) >> 31) == 0;
    dest[pos >> 1] = ((int32)right < 0 || (int32)left >= 0x10000) ? 0xff : (left_val >> 8 | (right_val & ~0xff));
    if (finished) {
      pos += adjust;
      while ((pos -= 2) >= 0) dest[pos >> 1] = 0xff;
      return;
    }
  } while ((pos -= 2) >= 0);
}
static __attribute__((noinline)) void XrayFillDownRamped(uint16 *dest, int pos, uint32 left, uint32 left_step, uint32 right, uint32 right_step, int adjust) {
#ifdef NT_SSF2
  if (xjob_rec) { xjob_add_ramp(dest, 0, pos, left, left_step, right, right_step, adjust); return; }
#endif
  do {
    left += left_step;
    right += right_step;
    uint32 left_val = (left & ~0xffff) ? (((int32)left < 0) ? 0 : 0xffff) : left;
    uint32 right_val = (right & ~0xffff) ? (((int32)right < 0) ? 0 : 0xffff) : right;
    bool finished = (left & ~0xffff) && (right & ~0xffff) && (((left_step ^ left) | (right_step ^ right)) >> 31) == 0;
    dest[pos >> 1] = ((int32)right < 0 || (int32)left >= 0x10000) ? 0xff : (left_val >> 8 | (right_val & ~0xff));
    if (finished) {
      pos += adjust;
      while ((pos += 2) < 460) dest[pos >> 1] = 0xff;
      return;
    }
  } while ((pos += 2) < 460);
}

static void XrayHdmaFunc_BeamAimedR(XrayHdmaCtx *c) {  // 0x91BEC2
  int p = 2 * c->r24;
  XrayFillUpRamped(c->dest, p - 2, c->r22 - 0x10000, c->r30, 65535, 1, 2);
  XrayFillDownRamped(c->dest, p, c->r22 - 0x10000, c->r32, 65535, 1, -2);
}
static void XrayHdmaFunc_BeamAimedL(XrayHdmaCtx *c) {  // 0x91BF72
  int t = 2 * c->r24;
  XrayFillUpRamped(c->dest, t - 2, 0, -1, c->r22 + 0x10000, -c->r32, 2);
  XrayFillDownRamped(c->dest, t, 0, -1, c->r22 + 0x10000, -c->r30, -2);
}
static void XrayHdmaFunc_BeamAimedU(XrayHdmaCtx *c) {
  int p = 2 * c->r24;
  if (sign16(c->r26 - 192))
    XrayFillUpRamped(c->dest, p - 2, c->r22 - 0x10000, c->r30, c->r22 - 0x10000, c->r32, 0);
  else if (sign16(c->r28 - 192))
    XrayFillUpRamped(c->dest, p - 2, c->r22 + 0x10000, -c->r30, c->r22 - 0x10000, c->r32, 0);
  else
    XrayFillUpRamped(c->dest, p - 2, c->r22 + 0x10000, -c->r30, c->r22 + 0x10000, -c->r32, 2);
  XrayFillDown(c->dest, 0xff, p);
}
static void XrayHdmaFunc_BeamAimedD(XrayHdmaCtx *c) {
  int p = 2 * c->r24;
  if (sign16(c->r28 - 128))
    XrayFillDownRamped(c->dest, p, c->r22 - 0x10000, c->r32, c->r22 - 0x10000, c->r30, 0);
  else if (sign16(c->r26 - 128))
    XrayFillDownRamped(c->dest, p, c->r22 + 0x10000, -c->r32, c->r22 - 0x10000, c->r30, 0);
  else
    XrayFillDownRamped(c->dest, p, c->r22 + 0x10000, -c->r32, c->r22 + 0x10000, -c->r30, 0);
  XrayFillUp(c->dest, 0xff, p - 2);
}
static void XrayHdmaFunc_BeamHoriz(XrayHdmaCtx *c) {  // 0x91C505
  uint16 v0 = 2 * (c->r24 - 1);
  c->dest[v0 >> 1] = 0xff00;
  XrayFillUp(c->dest, 0xff, v0 - 2);
  XrayFillDown(c->dest, 0xff, v0 + 2);
}
static void XrayHdmaOnScreen_BeamAimedR(XrayHdmaCtx *c) {  // 0x91C5FF
  int p = 2 * c->r24;
  c->dest[(p - 2) >> 1] = c->r22 >> 8 | 0xff00;
  XrayFillUpRamped(c->dest, p - 4, c->r22, c->r30, 65535, 1, 2);
  XrayFillDownRamped(c->dest, p, c->r22, c->r32, 65535, 1, -2);
}
static void XrayHdmaOnScreen_BeamAimedL(XrayHdmaCtx *c) {  // 0x91C660
  int v0 = 2 * c->r24;
  c->dest[(v0 - 2) >> 1] = (c->r22 >> 8) << 8;
  XrayFillUpRamped(c->dest, v0 - 4, 0, -1, c->r22, -c->r32, 2);
  XrayFillDownRamped(c->dest, v0, 0, -1, c->r22, -c->r30, -2);
}
static void XrayHdmaOnScreen_BeamAimedU(XrayHdmaCtx *c) {  // 0x91C6C1
  int v0 = 2 * c->r24;
  c->dest[(v0 - 2) >> 1] = (c->r22 >> 8) * 0x101;
  if (sign16(c->r26 - 192))
    XrayFillUpRamped(c->dest, v0 - 4, c->r22, c->r30, c->r22, c->r32, 0);
  else if (sign16(c->r28 - 192))
    XrayFillUpRamped(c->dest, v0 - 4, c->r22, -c->r30, c->r22, c->r32, 0);
  else
    XrayFillUpRamped(c->dest, v0 - 4, c->r22, -c->r30, c->r22, -c->r32, 2);
  XrayFillDown(c->dest, 0xff, v0);
}
static void XrayHdmaOnScreen_BeamAimedD(XrayHdmaCtx *c) {  // 0x91C822
  int v0 = 2 * c->r24;
  c->dest[(v0 - 2) >> 1] = (c->r22 >> 8) * 0x101;
  if (sign16(c->r28 - 128))
    XrayFillDownRamped(c->dest, v0, c->r22, c->r32, c->r22, c->r30, 0);
  else if (sign16(c->r26 - 128))
    XrayFillDownRamped(c->dest, v0, c->r22, -c->r32, c->r22, c->r30, 0);
  else
    XrayFillDownRamped(c->dest, v0, c->r22, -c->r32, c->r22, -c->r30, 0);
  XrayFillUp(c->dest, 0xff, v0 - 4);
}
static void XrayHdmaOnScreen_BeamHoriz(XrayHdmaCtx *c) {  // 0x91C998
  uint16 v0 = 2 * (c->r24 - 1);
  uint32 t = (c->r22 >> 8);
  c->dest[v0 >> 1] = (c->r18_avoid == 64) ? t | 0xFF00 : t << 8;
  XrayFillUp(c->dest, 0xff, v0 - 2);
  XrayFillDown(c->dest, 0xff, v0 + 2);
}

void CalculateXrayHdmaTableInner(uint16 k, uint16 j, uint16 r18, uint16 r20, bool off_screen, uint16 *dest_addr) {  // 0x91BE11
  int v3;
  XrayHdmaCtx c[1];
  c->dest = dest_addr;
  c->r18_avoid = r18;
  c->r22 = k << 8;
  c->r24 = j;
  c->r26 = r18 - r20;
  c->r26 += (c->r26 & 0x8000) != 0 ? 0x100 : 0;
  c->r28 = r20 + r18;
  if (!sign16(r20 + r18 - 257))
    c->r28 = r20 + r18 - 256;
  c->r30 = kTanTable[sign16(c->r26 - 128) ? c->r26 : c->r26 - 128];
  c->r32 = kTanTable[sign16(c->r28 - 128) ? c->r28 : c->r28 - 128];
  if (!r20 && (r18 == 64 || r18 == 192)) v3 = 8;
  else if (sign16(c->r26 - 128)) v3 = !sign16(c->r26 - 64) ? 4 : !sign16(c->r28 - 64) ? 0 : 2;
  else v3 = !sign16(c->r26 - 192) ? 2 : !sign16(c->r28 - 192) ? 6 : 4;
  if (off_screen) {
    switch (v3 >> 1) {
    case 0: XrayHdmaFunc_BeamAimedR(c); break;
    case 1: XrayHdmaFunc_BeamAimedU(c); break;
    case 2: XrayHdmaFunc_BeamAimedD(c); break;
    case 3: XrayHdmaFunc_BeamAimedL(c); break;
    default: XrayHdmaFunc_BeamHoriz(c); break;
    }
  } else {
    switch (v3 >> 1) {
    case 0: XrayHdmaOnScreen_BeamAimedR(c); break;
    case 1: XrayHdmaOnScreen_BeamAimedU(c); break;
    case 2: XrayHdmaOnScreen_BeamAimedD(c); break;
    case 3: XrayHdmaOnScreen_BeamAimedL(c); break;
    default: XrayHdmaOnScreen_BeamHoriz(c); break;
    }
  }
}

/* --- el ojo de la sala de la bola (MorphBallEye, $A8:9058): su haz es una ventana con color sumado (COLDATA) */
#define BEAM_TAB (&ERAM(0x9100))
static uint16 beam_memo[4];                       /* MorphBallEyeBeamTable */
uint16 beam_tab_version;                          /* cambia cada vez que se reescribe la tabla (md_cone.c) */
static uint8 beam_pending;
/* la tabla con los últimos valores (CalculateXrayHdmaTableInner, como en el SNES) si aún no se calculó */
void beam_table_flush(void) {
  uint16 v2 = beam_memo[0], v1 = beam_memo[1];
  CalculateXrayHdmaTableInner(v2, v1, beam_memo[2], beam_memo[3], !((int16)v2 >= 0 && sign16(v2 - 256)), &ERAM(0x9100));
  beam_pending = 0;
}
uint8 beam_table_pending(void) { return beam_pending; }
#ifdef NT_SSF2
/* la tabla por partes (md_cone.c): beam_job_start anota con los últimos valores; xray_job_run la escribe */
#ifdef MD_TABCHK
static uint16 xjob_par[4];
volatile uint16 tabchk_bad, tabchk_n;
void beam_job_check(void) {   /* prueba: la tabla por partes == la de una vez */
  uint16 *t = cinematic_text_tilemap;
  CalculateXrayHdmaTableInner(xjob_par[0], xjob_par[1], xjob_par[2], xjob_par[3], !((int16)xjob_par[0] >= 0 && sign16(xjob_par[0] - 256)), t);
  for (uint16 i = 0; i < 230; i++) if (t[i] != ERAM(0x9100 + 2 * i)) { tabchk_bad++; break; }
  tabchk_n++;
}
#endif
void beam_job_start(void) {
  uint16 v2 = beam_memo[0], v1 = beam_memo[1];
#ifdef MD_TABCHK
  for (uint16 i = 0; i < 4; i++) xjob_par[i] = beam_memo[i];
#endif
  xjob_n = 0; xjob_i = 0; xjob_rec = 1;
  CalculateXrayHdmaTableInner(v2, v1, beam_memo[2], beam_memo[3], !((int16)v2 >= 0 && sign16(v2 - 256)), &ERAM(0x9100));
  xjob_rec = 0;
  beam_pending = 0;
}
#endif
void beam_params(uint16 *p) { for (uint16 i = 0; i < 4; i++) p[i] = beam_memo[i]; }   /* md_cone.c */
static uint8 beam_memo_ok;
static inline uint8 get_byte(const uint16 *a, uint16 b) { return (b & 1) ? a[b >> 1] >> 8 : (uint8)a[b >> 1]; }
static void eram_set_byte(uint16 addr, uint8 v) { set_byte(enemy_ram7800, addr - 0x7800, v); }

void SpawnMorphBallEyeBeamHdma(void) {  // 0x88E8D9 (DMAP 0x41, BBAD 0x26: ventana 1 del SNES)
  SpawnHdmaObject(0x88, 0xE8EC);
}

static void HdmaobjInstr_InitMorphBallEyeBeamHdma(uint16 k) {  // 0x88E917
  set_byte(hdma_object_A, k, 48);     reg_COLDATA[0] = 48;
  set_byte(hdma_object_A, k + 1, 80); reg_COLDATA[1] = 80;
  set_byte(hdma_object_B, k, 0x80);   reg_COLDATA[2] = 0x80;
  fx_layer_blending_config_c = 16;
  /* tabla indirecta del HDMA (7E:9000): 100 líneas desde 9100, 100 desde 91C8, 24 desde 9290 */
  static const uint8 kTab[11] = { 0xE4, 0x00, 0x91, 0xE4, 0xC8, 0x91, 0x98, 0x90, 0x92, 0x00, 0x00 };
  for (uint16 i = 0; i < 11; i++) eram_set_byte(0x9000 + i, kTab[i]);
  gEnemyData(64)->ai_var_C = 1;
  beam_memo_ok = 0;
  beam_pending = 0;
  hdma_object_C[k >> 1] = 0;
  hdma_object_D[k >> 1] = 0;
  ERAM(0x9080) = 0;
  ERAM(0x9082) = 0;
  ERAM(0x9090) = 0;
}

/* la tabla solo depende de estos cuatro valores: si no cambiaron desde la última vez, ya está escrita (en la
   Genesis calcularla cuesta un cuarto de cuadro; el resultado es el mismo) */
static void MorphBallEyeBeamTable(uint16 v0) {  // 0x88E987
  EnemyData *E = gEnemyData(64);
  uint16 r18 = E->ai_var_D, r20 = hdma_object_C[v0 >> 1];
  uint16 v1 = E->y_pos - layer1_y_pos, v2 = E->x_pos - layer1_x_pos;
  if (!beam_memo_ok || beam_memo[0] != v2 || beam_memo[1] != v1 || beam_memo[2] != r18 || beam_memo[3] != r20) {
    beam_memo[0] = v2; beam_memo[1] = v1; beam_memo[2] = r18; beam_memo[3] = r20;
    beam_memo_ok = 1;
    beam_tab_version++;
#ifdef NT_SSF2
    beam_pending = 1;                 /* en la Genesis la tabla solo la lee md_cone.c: la calcula cuando la usa */
#else
    beam_table_flush();
#endif
  }
  reg_COLDATA[0] = get_byte(hdma_object_A, v0);
  reg_COLDATA[1] = get_byte(hdma_object_A, v0 + 1);
  reg_COLDATA[2] = get_byte(hdma_object_B, v0);
}

static void BeamNextInstr(void) {
  int v = hdma_object_index >> 1;
  hdma_object_instruction_list_pointers[v] += 2;
  hdma_object_instruction_timers[v] = 1;
}

static void HdmaobjPreInstr_E9E6(uint16 k) {  // 0x88E9E6: el haz se abre
  int v = k >> 1;
  fx_layer_blending_config_c = 16;
  AddToHiLo(&ERAM(0x9080), &ERAM(0x9082), 0x4000);
  AddToHiLo(&hdma_object_C[v], &hdma_object_D[v], (uint32)ERAM(0x9080) << 16 | ERAM(0x9082));
  if (!sign16(hdma_object_C[v] - 4)) {
    hdma_object_C[v] = 4;
    BeamNextInstr();
  }
  MorphBallEyeBeamTable(k);
}

static void HdmaobjPreInstr_EA3C(uint16 k) {  // 0x88EA3C: abierto, el color late
  fx_layer_blending_config_c = 16;
  if (gEnemyData(64)->ai_var_C) {
    MorphBallEyeBeamTable(k);
    uint16 i = 4 * ERAM(0x9090);
    set_byte(hdma_object_A, k, B88B(0xEA8B + i));
    set_byte(hdma_object_A, k + 1, B88B(0xEA8B + i + 1));
    set_byte(hdma_object_B, k, B88B(0xEA8B + i + 2));
    ERAM(0x9090) = (ERAM(0x9090) + 1) & 0xF;
  } else {
    BeamNextInstr();
  }
}

static void HdmaobjPreInstr_EACB(uint16 k) {  // 0x88EACB: se apaga
  fx_layer_blending_config_c = 16;
  if (get_byte(hdma_object_A, k + 1) == 64) {
    reg_COLDATA[0] = 32;
    reg_COLDATA[1] = 64;
    reg_COLDATA[2] = 0x80;
    hdma_object_D[k >> 1] = 0;
    hdma_object_C[k >> 1] = 0;
    for (uint16 i = 0; i < 256; i++) BEAM_TAB[i] = 255;
    beam_memo_ok = 0;
    beam_pending = 0;
    beam_tab_version++;
    for (uint16 a = 0x9000; a <= 0x9008; a += 2) ERAM(a) = 0;
    BeamNextInstr();
  } else {
    MorphBallEyeBeamTable(k);
    uint16 v4 = hdma_object_index;
    if (get_byte(hdma_object_A, v4) != 32) set_byte(hdma_object_A, v4, get_byte(hdma_object_A, v4) - 1);
    if (get_byte(hdma_object_A, v4 + 1) != 64) set_byte(hdma_object_A, v4 + 1, get_byte(hdma_object_A, v4 + 1) - 1);
    if (get_byte(hdma_object_B, v4) != 0x80) set_byte(hdma_object_B, v4, get_byte(hdma_object_B, v4) - 1);
  }
}
