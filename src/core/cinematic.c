/* MegaMetroid — motor nativo: cinemáticas (banco $8B del decomp, sm_8b.c; docs/35).
 *
 * Lo portado: la escena de la nave que se va de Ceres (estado 34) y la de Ceres explotando con Samus adentro
 * (estado 37, cuando se acaba el tiempo), con lo que usan:
 *   - objetos sprite de cinemática: hasta 16, con lista de instrucciones en $8B, pre-instrucción por cuadro y un
 *     spritemap de $8C para dibujar (la Genesis lo dibuja con DrawCinematicSpritemap);
 *   - la matriz del modo 7 (ángulo cinematic_var5, escala cinematic_var6, desplazamiento var8/var10);
 *   - efectos de paleta ($8D, palfx.c) y la explosión del power bomb ($88, hdmaobj.c).
 * Los gráficos (tiles y mapas del modo 7, tiles de sprites) no pasan por el motor: CinematicGfx() avisa a la Genesis
 * qué juego de gráficos se carga y Mode7Transfers() qué cambio del mapa de modo 7 se hizo.
 * Sin portar todavía (nt_unimpl): objetos de modo 7 y de fondo, texto con brillo, Samus en la cinemática (no se usan
 * en estas escenas). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "cinematic.h"

void CinematicGfx(uint16 which);          /* capa de la Genesis: 1 = Ceres explotando, 2 = Zebes */
void Mode7Transfers(uint16 list);         /* capa de la Genesis: QueueMode7Transfers($8B, list) */
void DrawCinematicSpritemap(uint16 sm, uint16 x, uint16 y, uint16 chr, uint8 offscreen);   /* $8C (Genesis) */
void QueueMusic_DelayedY(uint16 a, uint16 j);

#define CINE_RAM_FROM 0x198D                 /* cinematic_var5 y los 656 bytes que siguen ($8B: "for j = 656...") */
#define CINE_RAM_TO   (0x198D + 658)
#define INT16_SHL8(x) ((int32)(int16)(x) << 8)

void HandleFadeIn(void) {  // 0x80894D
  if ((int16)(screen_fade_counter - 1) < 0) {
    screen_fade_counter = screen_fade_delay;
    if (((reg_INIDISP + 1) & 0xF) != 0)
      reg_INIDISP = (reg_INIDISP + 1) & 0xF;
  } else {
    --screen_fade_counter;
  }
}

void HandleFadeOut(void) {  // 0x808924
  if ((int16)(screen_fade_counter - 1) < 0) {
    screen_fade_counter = screen_fade_delay;
    if (reg_INIDISP & 0xF)
      reg_INIDISP = (reg_INIDISP & 0xF) == 1 ? 0x80 : (reg_INIDISP & 0xF) - 1;
  } else {
    --screen_fade_counter;
  }
}

/* ------------------------------------------------------------------ PPU ($8B:819B, $8B:8230) */
void SetupPpu_3_Mode7(void) {  // 0x8B819B
  reg_INIDISP = 0x80;
  reg_OBSEL = 3;
  reg_BGMODE = 7;
  reg_M7SEL = 0x80;
  reg_BG1SC = 0; reg_BG2SC = 0; reg_BG3SC = 0; reg_BG4SC = 0;
  reg_BG12NBA = 0; reg_BG34NBA = 0;
  reg_TM = 17; reg_TS = 0; reg_TMW = 0;
  next_gameplay_CGWSEL = 0; next_gameplay_CGADSUB = 0;
  reg_CGWSEL = 0; reg_CGADSUB = 0;
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  reg_MOSAIC = 0;
  cinematic_var7 = 0; cinematic_var8 = 0; cinematic_var9 = 0; cinematic_var10 = 0;
  g_word_7E1999 = 0; g_word_7E199B = 0; g_word_7E199D = 0; g_word_7E199F = 0;
  fx_layer_blending_config_a = 0;
}

static void SetupPpu_4_Mode1(void) {  // 0x8B8230
  reg_INIDISP = 0x80;
  reg_OBSEL = 3;
  reg_BGMODE = 1;
  reg_M7SEL = 0;
  reg_BG1SC = 92; reg_BG2SC = 0; reg_BG3SC = 0; reg_BG4SC = 0;
  reg_BG12NBA = 6; reg_BG34NBA = 0;
  reg_TM = 17; reg_TS = 0;
  reg_W12SEL = 0; reg_W34SEL = 0; reg_WOBJSEL = 0;
  reg_TMW = 0; reg_TSW = 0;
  reg_CGWSEL = 0; reg_CGADSUB = 0;
  reg_BG1HOFS = 0; reg_BG1VOFS = 0;
}

/* ------------------------------------------------------------------ matriz del modo 7 ($8B:8532) */
static void HandleMode7TransformationMatrix(void) {
  reg_M7D = reg_M7A = (uint16)(((int32)kSinCosTable8bit_Sext[(uint8)(cinematic_var5 + 64) + 64] * (int16)cinematic_var6) >> 8);
  uint16 v1 = (uint16)(((int32)kSinCosTable8bit_Sext[(uint8)cinematic_var5 + 64] * (int16)cinematic_var6) >> 8);
  reg_M7B = v1;
  reg_M7C = -v1;
  reg_BG1HOFS = cinematic_var8;
  reg_BG1VOFS = cinematic_var10;
}

/* ------------------------------------------------------------------ objetos sprite ($8B:938A) */
static uint8 SpawnCinematicSpriteObjectInner(uint16 d, uint16 j);

uint8 SpawnCinematicSpriteObject(uint16 d, uint16 a) {  // 0x8B938A
  cinematic_spawn_param = a;
  for (int i = 30; i >= 0; i -= 2)
    if (!cinematicspr_instr_ptr[i >> 1])
      return SpawnCinematicSpriteObjectInner(d, i);
  return 1;
}

uint8 SpawnCinematicSpriteObjectToR18(uint16 d, uint16 r18) {  // 0x8B93A2
  cinematic_spawn_param = 0;
  return SpawnCinematicSpriteObjectInner(d, r18);
}

/* inicios de objeto (posición y atributo: cinematicbg_arr7/8 = x, y; arr9 = paleta y prioridad del spritemap) */
static void SetSprPos(uint16 j, uint16 x, uint16 y, uint16 chr) {
  int v = j >> 1;
  cinematicbg_arr7[v] = x;
  cinematicbg_arr8[v] = y;
  cinematicbg_arr9[v] = chr;
}

static const uint16 kFunc78_Tab0[5] = { 1, 16, 32, 48, 64 };
static const int16 kFunc78_Tab1[5] = { 16, -16, 16, -16, 0 };
static const int16 kFunc78_Tab2[5] = { -16, 16, 16, -16, 0 };
static const int16 g_word_8BC4EB[16] = { 14, -8, 8, 12, -16, 12, -8, -14, 0, 0, 16, 14, -12, 4, -8, -16 };
static const int16 kFunc81_Tab0[4] = { 1, 4, 8, 16 };
static const int16 kFunc81_Tab1[4] = { 8, 12, -8, -12 };
static const int16 kFunc81_Tab2[4] = { -4, 8, -10, 12 };

static void CallCinematicSpriteObjectSetup(uint16 f, uint16 j) {
  int v = j >> 1;
  uint16 p = cinematic_spawn_param;
  switch (f) {
  case FUNC16(CinematicFunction_nullsub_116): break;
  case FUNC16(CinematicFunction_Intro_Func59): SetSprPos(j, 80, 159, 2048); break;      // 0x8BBF22
  case FUNC16(CinematicFunction_Intro_Func63): SetSprPos(j, 128, 96, 2048); break;     // 0x8BBF76
  case FUNC16(CinematicFunction_Intro_Func65):                                         // 0x8BBFA0
    if (p) {
      cinematicbg_arr7[v] = 224;
    } else {
      cinematicbg_arr7[v] = 112;
      cinematicspr_preinstr_func[v] = FUNC16(CinematicSprPreInstr_nullsub_300);
    }
    cinematicbg_arr8[v] = 87;
    cinematicbg_arr9[v] = 2048;
    break;
  case FUNC16(CinematicFunction_Intro_Func78):                                         // 0x8BC434
    cinematicspr_goto_timer[v] = p;
    cinematicspr_instr_timer[v] = kFunc78_Tab0[p];
    SetSprPos(j, kFunc78_Tab1[p] + reg_M7X - cinematic_var8, kFunc78_Tab2[p] + reg_M7Y - cinematic_var10, 2560);
    break;
  case FUNC16(CinematicFunction_Intro_Func80):                                         // 0x8BC4B9
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, g_word_8BC4EB[2 * p] + reg_M7X - cinematic_var8, g_word_8BC4EB[2 * p + 1] + reg_M7Y - cinematic_var10, 2560);
    break;
  case FUNC16(CinematicFunction_Intro_Func81):                                         // 0x8BC533
    cinematicspr_goto_timer[v] = p;
    cinematicspr_instr_timer[v] = kFunc81_Tab0[p];
    SetSprPos(j, kFunc81_Tab1[p] + reg_M7X - cinematic_var8, kFunc81_Tab2[p] + reg_M7Y - cinematic_var10, 2560);
    break;
  case FUNC16(CinematicFunction_Intro_Func83):                                         // 0x8BC5A9
    SetSprPos(j, reg_M7X - cinematic_var8, reg_M7Y - cinematic_var10, 2560);
    break;
  case FUNC16(CinematicFunction_Intro_Func89): SetSprPos(j, 136, 111, 3584); break;    // 0x8BC83B
  case FUNC16(CinematicFunction_Intro_Func92): SetSprPos(j, 128, 127, 2048); break;    // 0x8BC897
  case FUNC16(CinematicFunction_Intro_Func98): SetSprPos(j, 48, 47, 2048); break;      // 0x8BC942
  case FUNC16(CinematicFunction_Intro_Func99): SetSprPos(j, 208, 47, 2048); break;     // 0x8BC956
  case FUNC16(CinematicFunction_Intro_Func100): SetSprPos(j, 48, 207, 2048); break;    // 0x8BC96A
  case FUNC16(CinematicFunction_Intro_Func101): SetSprPos(j, 208, 207, 2048); break;   // 0x8BC97E
  case FUNC16(CinematicFunction_Intro_Func102): SetSprPos(j, 128, 186, 0); break;      // 0x8BC992
  case FUNC16(CinematicFunction_Intro_Func104): SetSprPos(j, 128, 204, 0); break;      // 0x8BC9E6
  default: if (!IntroSpriteSetup(f, j)) nt_unimpl_v("CinematicSpriteObjectSetup (inicio sin portar)", f); break;
  }
}

static uint8 SpawnCinematicSpriteObjectInner(uint16 d, uint16 j) {  // 0x8B93AC
  int v = j >> 1;
  cinematicspr_preinstr_func[v] = B8BW(d + 2);
  cinematicspr_instr_ptr[v] = B8BW(d + 4);
  cinematicspr_instr_timer[v] = 1;
  cinematicspr_whattodraw[v] = 0;
  cinematicspr_goto_timer[v] = 0;
  cinematicspr_arr6[v] = 0;
  cinematicspr_arr7[v] = 0;
  CallCinematicSpriteObjectSetup(B8BW(d), j);
  return 0;
}

/* pre-instrucciones (cada cuadro, antes de la lista) */
static void MoveUp(uint16 k, uint16 accel) {   /* Func91/94/97: sube acelerando; fuera de pantalla -> se borra */
  int v = k >> 1;
  cinematicspr_goto_timer[v] += accel;
  add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], -INT16_SHL8(cinematicspr_goto_timer[v]));
  if (sign16(cinematicbg_arr8[v] + 128)) {
    cinematicspr_instr_timer[v] = 1;
    cinematicspr_instr_ptr[v] = addr_off_8BCE53;
    if (cinematicspr_preinstr_func[v] == FUNC16(CinematicFunction_Intro_Func94))
      cinematic_function = FUNC16(CinematicFunction_Intro_Func95);
  }
}

static void CallCinematicSprPreInstr(uint16 f, uint16 k) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(CinematicSprPreInstr_nullsub_300): case FUNC16(CinematicFunction_nullsub_116):
  case FUNC16(CinematicFunction_nullsub_298):
    break;
  case FUNC16(CinematicFunction_Intro_Func60):                   // 0x8BBF35
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], 0x4000);
    cinematicbg_arr7[v] &= 0x1FF;
    break;
  case FUNC16(CinematicFunction_Intro_Func62):                   // 0x8BBF5F
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], 4096);
    cinematicbg_arr7[v] &= 0x1FF;
    break;
  case FUNC16(CinematicFunction_Intro_Func64):                   // 0x8BBF89
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], 2048);
    cinematicbg_arr7[v] &= 0x1FF;
    break;
  case FUNC16(CinematicFunction_Intro_Func66):                   // 0x8BBFC6
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], -0x2000);
    break;
  case FUNC16(CinematicFunction_Intro_Func82):                   // 0x8BC582
    add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], -4096);
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], 0x4000);
    break;
  case FUNC16(CinematicSprPreInstr_C489):                        // 0x8BC489: chispas alrededor de la estación
    if (cinematic_function == FUNC16(CinematicFunction_Intro_Func77)) {
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_nullsub_116);
    } else if (!--cinematic_var4 || (cinematic_var4 & 0x8000)) {
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEC1, cinematic_var13);
      cinematic_var4 = 12;
      cinematic_var13 = (cinematic_var13 + 1) & 7;
    }
    break;
  case FUNC16(CinematicFunction_Intro_Func90):                   // 0x8BC84E
    if (cinematic_function == FUNC16(nullsub_124)) cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func91);
    break;
  case FUNC16(CinematicFunction_Intro_Func93):                   // 0x8BC8AA
    if (cinematic_function == FUNC16(nullsub_124)) cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func94);
    break;
  case FUNC16(CinematicFunction_Intro_Func96):                   // 0x8BC8F9
    if (cinematic_function == FUNC16(nullsub_124)) cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func97);
    break;
  case FUNC16(CinematicFunction_Intro_Func91): MoveUp(k, 64); break;   // 0x8BC85D
  case FUNC16(CinematicFunction_Intro_Func94): MoveUp(k, 32); break;   // 0x8BC8B9 (al salir: Func95)
  case FUNC16(CinematicFunction_Intro_Func97): MoveUp(k, 32); break;   // 0x8BC908
  default: if (!IntroSpritePreInstr(f, k)) nt_unimpl_v("CinematicSprPreInstr (pre-instrucción sin portar)", f); break;
  }
}

/* instrucciones: devuelven el puntero siguiente, o 0 si el objeto termina este cuadro */
static uint16 CallCinematicSprInstr(uint16 f, uint16 k, uint16 j) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(CinematicSprInstr_Delete): cinematicspr_whattodraw[v] = 0; cinematicspr_instr_ptr[v] = 0; return 0;
  case FUNC16(CinematicSprInstr_Sleep): cinematicspr_instr_ptr[v] = j - 2; return 0;
  case FUNC16(CinematicSprInstr_SetPreInstr): cinematicspr_preinstr_func[v] = B8BW(j); return j + 2;
  case FUNC16(CinematicSprInstr_ClearPreInstr): cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_nullsub_298); return j;
  case FUNC16(CinematicSprInstr_GotoRel): return j + (int8)B8BB(j);
  case FUNC16(CinematicSprInstr_Goto): return B8BW(j);
  case FUNC16(CinematicSprInstr_DecTimerGoto): return (cinematicspr_goto_timer[v]-- == 1) ? j + 2 : B8BW(j);
  case FUNC16(CinematicSprInstr_DecTimerGotoRel): return (cinematicspr_goto_timer[v]-- == 1) ? j + 1 : j + (int8)B8BB(j);
  case FUNC16(CinematicSprInstr_SetTimer): cinematicspr_goto_timer[v] = B8BW(j); return j + 2;
  case FUNC16(CinematicSprInstr_C9A5): SpawnPalfxObject(addr_kPalfx_E1B0); return j;          // 0x8BC9A5
  case FUNC16(CinematicSprInstr_C9AF):                                                         // 0x8BC9AF
    if (japanese_text_flag)
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEB5, japanese_text_flag);
    return j;
  case FUNC16(CinematicSprInstr_C9BD): SpawnPalfxObject(addr_loc_8BE1B4); return j;            // 0x8BC9BD
  case FUNC16(CinematicSprInstr_C9C7):                                                         // 0x8BC9C7: la nave hacia Zebes
    cinematic_var8 = 62;
    cinematic_var10 = -112;
    cinematic_var5 = 32;
    cinematic_var6 = 16;
    cinematic_function = FUNC16(CinematicFunction_Intro_Func105);
    return j;
  case FUNC16(CinematicSprInstr_SpawnCeresExplosions1):                                        // 0x8BC404
    for (uint16 i = 0; i < 5; i++)
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEBB, i);
    return j;
  case FUNC16(CinematicSprInstr_SpawnCeresExplosions3):                                        // 0x8BC50C
    for (uint16 i = 0; i < 4; i++)
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEC7, i);
    return j;
  default: {
    uint16 r = IntroSpriteInstr(f, k, j);
    if (r != 0xFFFF) return r;
    nt_unimpl_v("CinematicSprInstr (instrucción sin portar)", f);
    cinematicspr_whattodraw[v] = 0; cinematicspr_instr_ptr[v] = 0;
    return 0;
  }
  }
}

static void ProcessCinematicSpriteInstructionList(uint16 k) {  // 0x8B9409
  CallCinematicSprPreInstr(cinematicspr_preinstr_func[k >> 1], k);
  uint16 v1 = cinematic_obj_index;
  int v2 = v1 >> 1;
  if (cinematicspr_instr_timer[v2]-- == 1) {
    uint16 p = cinematicspr_instr_ptr[v2], f;
    while ((f = B8BW(p)) & 0x8000) {
      p = CallCinematicSprInstr(f, v1, p + 2);
      if (!p)
        return;
    }
    cinematicspr_instr_timer[v2] = f;
    cinematicspr_whattodraw[v2] = B8BW(p + 2);
    cinematicspr_instr_ptr[v2] = p + 4;
  }
}

static void HandleCinematicSprites(void) {  // 0x8B93EF
  for (int i = 30; i >= 0; i -= 2) {
    cinematic_obj_index = i;
    if (cinematicspr_instr_ptr[i >> 1]) {
      ProcessCinematicSpriteInstructionList(i);
      i = cinematic_obj_index;
    }
  }
}

static void DrawCinematicSpriteObjects_Intro(void) {  // 0x8B9746
  for (int i = 15; i >= 0; i--) {
    uint16 sm = cinematicspr_whattodraw[i];
    if (!sm)
      continue;
    uint16 x = cinematicbg_arr7[i] - layer1_x_pos;
    uint16 y = cinematicbg_arr8[i] - layer1_y_pos;
    if ((uint16)(y + 128) < 0x1FF)
      DrawCinematicSpritemap(sm, x, y, cinematicbg_arr9[i], (y & 0xFF00) != 0);
  }
}

/* ------------------------------------------------------------------ escena: Ceres explota ($8B:C11B) */
static void CinematicFunctionBlackoutFromCeres(void) {  // 0x8BC11B
  SetupPpu_3_Mode7();
  nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
  door_def_ptr = 0;
  layer1_x_pos = 0;
  layer1_y_pos = 0;
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = kPalettes_Intro2[i];
  nt_pal_dirty(0, 255);
  CinematicGfx(1);                           /* tiles y mapa del modo 7 de la estación, tiles de sprites */
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256;
  reg_M7X = 52; reg_M7Y = 48;
  cinematic_var8 = -44;
  cinematic_var10 = -112;
  cinematic_var5 = 0;
  cinematic_var6 = 256;
  DisableHdmaObjects();
  WaitUntilEndOfVblankAndClearHdma();
  ClearPaletteFXObjects();
  EnablePaletteFx();
  ClearCinematicBgObjects(0x2F);
  cinematicbg_var3 = 0;
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE7F, 0);
  SpawnCinematicSpriteObjectToR18(addr_kCinematicSpriteObjectDef_8BCE8B, 2);
  SpawnCinematicSpriteObjectToR18(addr_kCinematicSpriteObjectDef_8BCE91, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF33, 0);
  SpawnPalfxObject(addr_kPalfx_E1B8);
  screen_fade_delay = 1;
  screen_fade_counter = 1;
  cinematic_function = FUNC16(CinematicFunction_Intro_Func74);
  QueueMusic_Delayed8(0);
  QueueMusic_Delayed8(0xFF2D);
  QueueMusic_DelayedY(game_state == 37 ? 8 : 7, 0xE);
}

static void CinematicFunction_Intro_Func75(void) {  // 0x8BC2F1
  add_hilo(&cinematic_var10, &cinematic_var9, 4096);
  add_hilo(&cinematic_var8, &cinematic_var7, -0x4000);
  ++cinematic_var6;
  HandleFadeIn();
  if (reg_INIDISP == 15) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    cinematic_function = FUNC16(CinematicFunction_Intro_Func76);
    cinematic_var4 = 1;
    cinematic_var13 = 0;
  }
}

static void CinematicFunction_Intro_Func76(void) {  // 0x8BC345
  add_hilo(&cinematic_var10, &cinematic_var9, 4096);
  add_hilo(&cinematic_var8, &cinematic_var7, -0x4000);
  if (sign16(cinematic_var6 - 640)) {
    ++cinematic_var6;
  } else {
    power_bomb_explosion_x_pos = reg_M7X - cinematic_var8;
    power_bomb_explosion_y_pos = reg_M7Y - cinematic_var10;
    EnableHdmaObjects();
    SpawnPowerBombExplosion();
    reg_CGWSEL = 16;
    reg_CGADSUB = 55;
    cinematic_var6 = 768;
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF2D, 0);
    Mode7Transfers(game_state == 37 ? addr_kCinematicFunction_Intro_Func76_M7_2 : addr_kCinematicFunction_Intro_Func76_M7_0);
    Mode7Transfers(addr_kCinematicFunction_Intro_Func76_M7_1);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func77);
  }
}

static void CinematicFunction_Intro_Func77(void) {  // 0x8BC5CA
  reg_CGWSEL = 16;
  reg_CGADSUB = 55;
  cinematic_var8 += 2;
  cinematic_var5 = (uint8)(cinematic_var5 - 1);
  if (sign16(cinematic_var6 - 16)) {
    cinematic_function = FUNC16(CinematicFunction_Intro_Func84);
    cinematic_var4 = 192;
  } else {
    cinematic_var6 -= 16;
  }
}

static void CinematicFunction_Intro_Func85(void) {  // 0x8BC627
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    if (game_state == 37) {
      QueueMusic_Delayed8(0);
      hud_item_index = 0;
      samus_auto_cancel_hud_item_index = 0;
      samus_invincibility_timer = 0;
      samus_knockback_timer = 0;
      WaitUntilEndOfVblankAndClearHdma();
      fx_layer_blending_config_a = 0;
      next_gameplay_CGWSEL = 0;
      next_gameplay_CGADSUB = 0;
      reg_TM = 16; reg_TS = 0; reg_TMW = 0; reg_TSW = 0;
      reg_BGMODE = 9;
      game_options_screen_index = 0;
      menu_index = 0;
      nt_clear_ram(0x1A7D + 16, 0x1A7D + 16 + 256);   /* cinematicbg_arr7[8 + i], i < 128 */
      game_state = 25;
    } else {
      cinematic_function = FUNC16(CinematicFunction_Intro_Func86);
    }
  }
}

/* ------------------------------------------------------------------ escena: la nave hacia Zebes ($8B:C699) */
static void CinematicFunction_Intro_Func86(void) {  // 0x8BC699
  SetupPpu_4_Mode1();
  nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
  CinematicGfx(2);                           /* Zebes: tiles y mapa del BG1 (modo 1) y del modo 7 (la nave) */
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256;
  reg_M7X = 56; reg_M7Y = 24;
  cinematic_var8 = 0;
  cinematic_var10 = 0;
  cinematic_var5 = 0;
  cinematic_var6 = 256;
  DisableHdmaObjects();
  WaitUntilEndOfVblankAndClearHdma();
  ClearPaletteFXObjects();
  EnablePaletteFx();
  reg_MOSAIC = 0x81;
  SpawnPalfxObject(addr_kPalfx_E1A8);
  cinematic_function = FUNC16(CinematicFunction_Intro_Func87);
  screen_fade_delay = 1;
  screen_fade_counter = 1;
}

static void CinematicFunction_Intro_Func87(void) {  // 0x8BC79C
  if ((cinematic_var12 & 3) == 0)
    reg_MOSAIC -= 16;
  HandleFadeIn();
  if (reg_INIDISP == 15) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    cinematic_function = FUNC16(CinematicFunction_Intro_Func88);
  }
}

static void CinematicFunction_Intro_Func88(void) {  // 0x8BC7CA
  if ((cinematic_var12 & 3) == 0) {
    reg_MOSAIC -= 16;
    if ((reg_MOSAIC & 0xF0) == 0) {
      reg_BGMODE = 7;
      reg_M7SEL = 0x80;
      reg_BG1SC = 0;
      reg_BG12NBA = 0;
      reg_TM = 17;
      cinematic_var8 = 128;
      cinematic_var10 = -104;
      cinematic_var5 = 32;
      cinematic_var6 = 256;
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEA3, 0);
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEF7, 0);
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEFD, 0);
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF03, 0);
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF09, 0);
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEAF, 0);
      cinematic_function = FUNC16(nullsub_120);
    }
  }
}

static void CinematicFunction_Intro_Func105(void) {  // 0x8BC9F9
  add_hilo(&cinematic_var10, &cinematic_var9, 0x2000);
  add_hilo(&cinematic_var8, &cinematic_var7, -0x8000);
  if (sign16(cinematic_var6 - 1152))
    cinematic_var6 += 4;
  else
    cinematic_function = FUNC16(CinematicFunction_Intro_Func106);
}

static void CinematicFunction_Intro_Func106(void) {  // 0x8BCA36
  add_hilo(&cinematic_var10, &cinematic_var9, 0x2000);
  add_hilo(&cinematic_var8, &cinematic_var7, -0x8000);
  if (sign16(cinematic_var8 + 128)) {
    cinematic_function = FUNC16(CinematicFunction_Intro_Func107);
  } else {
    cinematic_var6 += 16;
    if (cinematic_var5 != 224)
      cinematic_var5 = (uint8)(cinematic_var5 - 1);
  }
}

static void CinematicFunction_Intro_Func107(void) {  // 0x8BCA85
  add_hilo(&cinematic_var10, &cinematic_var9, 0x2000);
  add_hilo(&cinematic_var8, &cinematic_var7, 0x2000);
  if (sign16(cinematic_var6 - 0x2000)) {
    cinematic_var6 += 32;
  } else {
    reg_TM = 16;
    cinematic_function = FUNC16(CinematicFunction_Intro_Func108);
    cinematic_var4 = 64;
  }
}

static void CinematicFunction_Intro_Func95(void) {  // 0x8BCADF: fin de la escena, carga de Zebes
  reg_INIDISP = 0x80;
  nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
  game_state = 6;                            /* kGameState_6_LoadingGameData */
  samus_health = samus_max_health;
}

static void CallCinematicFunction(uint16 f) {
  switch (f) {
  case FUNC16(CinematicFunctionBlackoutFromCeres): CinematicFunctionBlackoutFromCeres(); break;
  case FUNC16(CinematicFunction_Intro_Func74):                    // 0x8BC2E4
    if (!(HasQueuedMusic() & 1))
      cinematic_function = FUNC16(CinematicFunction_Intro_Func75);
    break;
  case FUNC16(CinematicFunction_Intro_Func75): CinematicFunction_Intro_Func75(); break;
  case FUNC16(CinematicFunction_Intro_Func76): CinematicFunction_Intro_Func76(); break;
  case FUNC16(CinematicFunction_Intro_Func77): CinematicFunction_Intro_Func77(); break;
  case FUNC16(CinematicFunction_Intro_Func84):                    // 0x8BC610
    if (!--cinematic_var4 || (cinematic_var4 & 0x8000)) {
      screen_fade_delay = 1;
      screen_fade_counter = 1;
      cinematic_function = FUNC16(CinematicFunction_Intro_Func85);
    }
    break;
  case FUNC16(CinematicFunction_Intro_Func85): CinematicFunction_Intro_Func85(); break;
  case FUNC16(CinematicFunction_Intro_Func86): CinematicFunction_Intro_Func86(); break;
  case FUNC16(CinematicFunction_Intro_Func87): CinematicFunction_Intro_Func87(); break;
  case FUNC16(CinematicFunction_Intro_Func88): CinematicFunction_Intro_Func88(); break;
  case FUNC16(nullsub_120): case FUNC16(nullsub_124): break;
  case FUNC16(CinematicFunction_Intro_Func105): CinematicFunction_Intro_Func105(); break;
  case FUNC16(CinematicFunction_Intro_Func106): CinematicFunction_Intro_Func106(); break;
  case FUNC16(CinematicFunction_Intro_Func107): CinematicFunction_Intro_Func107(); break;
  case FUNC16(CinematicFunction_Intro_Func108):                   // 0x8BCAD0
    if (!--cinematic_var4 || (cinematic_var4 & 0x8000))
      cinematic_function = FUNC16(nullsub_124);
    break;
  case FUNC16(CinematicFunction_Intro_Func95): CinematicFunction_Intro_Func95(); break;
  default: if (!IntroCinematicFunction(f)) nt_unimpl_v("CallCinematicFunction (función de cinemática sin portar)", f); break;
  }
}

void DrawSamusAndProjectiles(void);
void DrawPlayerExplosions2(void);
static void DrawIntroSprites(void) {  // 0x8B8E2D
  if (cinematic_var15) {
    if (cinematic_var15 & 0x8000) {
      DrawSamusAndProjectiles();
      DrawPlayerExplosions2();
      DrawCinematicSpriteObjects_Intro();
    } else {
      DrawCinematicSpriteObjects_Intro();
      DrawSamusAndProjectiles();
      DrawPlayerExplosions2();
    }
  } else {
    DrawCinematicSpriteObjects_Intro();
  }
}

void ClearCinematicSprites(void) {  // 0x8B93DA
  for (int i = 0; i < 16; i++) {
    cinematicspr_instr_ptr[i] = 0;
    cinematicspr_whattodraw[i] = 0;
  }
}

/* GameState_37_CeresGoesBoomWithSamus_ ($8B:A35B): también es el estado 34 (la nave se va) */
void GameState_CeresGoesBoom(void) {
  CallCinematicFunction(cinematic_function);
  if (!(cinematic_var11 & 0x8000))
    ++cinematic_var11;
  ++cinematic_var12;
  IntroHandleSamus();                       /* HandleSamusDuringIntro */
  HandleCinematicSprites();
  if (mode7_obj_instr_ptr[0] | mode7_obj_instr_ptr[1])
    nt_unimpl("HandleMode7Objects (texto en japonés)");
  IntroHandleBgAndGlow();                   /* HandleCinematicBgObjects, HandleTextGlowObjects */
  PaletteFxHandler();
  DrawIntroSprites();
  HandleMode7TransformationMatrix();
}
