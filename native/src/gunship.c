/* MegaMetroid — motor nativo: la nave de Samus (enemigos D07F arriba y D0BF abajo, banco $A2 del decomp,
 * sm_a2.c GunshipTop_* / GunshipBottom_*).
 *
 * La nave son tres enemigos seguidos: arriba (k), la escotilla (k + 64, también D0BF con parameter_2 != 0) y
 * abajo (k + 128). ai_preinstr (gtp_var_F en el decomp) es la función de cada cuadro. Portado: aterrizaje al
 * llegar de Ceres (3-7), espera (8), entrar a la nave para recargar y guardar (9-16). La huida final de Zebes
 * (17-21) queda para más adelante. */
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "nmath.h"
#include "hdmaobj.h"

#define GS_W(a) kBankA2_Gunship[(uint16)((a) - kBankA2_Gunship_LO)]
enum {                                     /* listas de instrucciones ($A2) */
  kIlist_A5BE = 0xA5BE, kIlist_A5EE = 0xA5EE, kIlist_A60E = 0xA60E, kIlist_A616 = 0xA616, kIlist_A61C = 0xA61C,
};
/* funciones de cada cuadro (direcciones del decomp: así se comparan con el SNES) */
enum {
  fGunship_nullsub_187 = 0xA7D7, fGunship_3 = 0xA80C, fGunship_4 = 0xA8D0, fGunship_5 = 0xA942,
  fGunship_6 = 0xA950, fGunship_7 = 0xA987, fGunship_8 = 0xA9BD, fGunship_9 = 0xAA4F, fGunship_10 = 0xAA5D,
  fGunship_11 = 0xAA94, fGunship_12 = 0xAAA2, fGunship_13 = 0xAB1F, fGunship_14 = 0xAB60, fGunship_15 = 0xAB6E,
  fGunship_16 = 0xABA5, fGunship_17 = 0xABC7,
};

static void AddHiLo(uint16 *hi, uint16 *lo, uint32 v) {   /* AddToHiLo: (hi:lo) += v */
  uint32 t = ((uint32)*hi << 16 | *lo) + v;
  *hi = (uint16)(t >> 16);
  *lo = (uint16)t;
}

/* temporizador de la nave (gtp_var_A del enemigo 0): 1 cuando se acaba */
static int TimerDone(void) {
  EnemyData *E = gEnemyData(0);
  int a = E->ai_var_A == 1;
  int b = (--E->ai_var_A & 0x8000) != 0;
  return a || b;
}

static void HatchAnim(uint16 k, uint16 list, uint8 sfx) {   /* la escotilla se abre o se cierra */
  EnemyData *E2 = gEnemyData(k + 128);
  E2->instruction_timer = 1;
  E2->current_instruction = list;
  gEnemyData(0)->ai_var_A = 144;
  QueueSfx3_Max6(sfx);
}

void GunshipTop_Init(void) {  // 0xA2A644
  EnemyData *E = gEnemyData(cur_enemy_index);
  E->properties |= 0x2000 | 0x400;                // kEnemyProps_DisableSamusColl | kEnemyProps_Tangible
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = kIlist_A616;
  E->palette_index = 3584;
  if (game_state != 40 || demo_set) {
    if (loading_game_state == 0x22) {             // kLoadingGameState_22_EscapingCeres: llega de Ceres
      E->y_pos = samus_y_pos - 17;
      E->ai_preinstr = fGunship_3;
    } else {
      E->y_pos -= 25;
      E->ai_var_E = E->y_pos;
      E->ai_preinstr = fGunship_8;
    }
  } else {                                        /* demo */
    samus_y_pos = 1138;
    E->y_pos -= 25;
    E->ai_var_E = E->y_pos;
    E->ai_preinstr = 0xAB60;                      // GunshipTop_14
    gEnemyData(0)->ai_var_A = 144;
  }
  SpawnPalfxObject(0xE1C0);                       // addr_stru_8DE1C0: las luces de la nave
  gEnemyData(cur_enemy_index)->ai_var_D = 1;
  gEnemyData(0)->ai_var_C = 0;
}

void GunshipBottom_Init(void) {  // 0xA2A6D2
  uint16 k = cur_enemy_index;
  EnemyData *E = gEnemyData(k);
  E->properties |= 0x2000 | 0x400;
  E->instruction_timer = 1;
  E->timer = 0;
  E->current_instruction = E->parameter_2 ? kIlist_A60E : kIlist_A61C;
  E->vram_tiles_index = gEnemyData(k - 64)->vram_tiles_index;   /* enemy_drawing_queue[k/2 + 106]: el anterior */
  E->palette_index = 3584;
  if (E->parameter_2) {                           /* la escotilla */
    E->y_pos = gEnemyData(k - 128)->y_pos - 1;    /* enemy_drawing_queue[k/2 + 61]: y de la parte de arriba */
    if (game_state == 40 && !demo_set) {
      E->instruction_timer = 1;
      E->current_instruction = kIlist_A5BE;
    }
  } else if (loading_game_state == 34) {          // kGameState_34_CeresGoesBoom (sic: 0x22)
    E->y_pos = samus_y_pos + 23;
  } else {
    E->y_pos += 15;
    E->ai_var_D = 71;
  }
  E->ai_preinstr = fGunship_nullsub_187;
}

static void GunshipTop_1(uint16 k) {  // 0xA2A784: la nave sube y baja un poco (flota)
  EnemyData *E0 = gEnemyData(k);
  int a = E0->ai_var_D == 1;
  int b = (--E0->ai_var_D & 0x8000) != 0;
  if (a || b) {
    uint16 v5 = 2 * E0->ai_var_C;
    uint16 t = GS_W(0xA7CF + v5);
    E0->ai_var_D = (uint8)t;
    uint16 dy = (int8)(t >> 8);
    E0->y_pos += dy;
    gEnemyData(k + 64)->y_pos += dy;
    gEnemyData(k + 128)->y_pos += dy;
    E0->ai_var_C = (E0->ai_var_C + 1) & 3;
  }
}

static void GunshipTop_3(uint16 k) {  // 0xA2A80C: baja hasta el suelo
  EnemyData *E = gEnemyData(k), *E2 = gEnemyData(k + 128), *E1 = gEnemyData(k + 64);
  uint32 v = sign16(E->y_pos - 768) ? 0x48000 : 0x28000;
  AddHiLo(&samus_y_pos, &samus_y_subpos, v);
  AddHiLo(&E2->y_pos, &E2->y_subpos, v);
  AddHiLo(&E1->y_pos, &E1->y_subpos, v);
  AddHiLo(&E->y_pos, &E->y_subpos, v);
  if (v == 0x28000 && !sign16(E->y_pos - 1119)) {
    E->y_pos = 1119;
    E1->y_pos = 1159;
    E2->y_pos = E->y_pos - 1;
    E->ai_preinstr = fGunship_4;
    E->ai_var_E = 0;
  }
}

static void GunshipTop_4(uint16 k) {  // 0xA2A8D0: el golpe del aterrizaje
  EnemyData *E = gEnemyData(k);
  uint16 d = GS_W(0xA622 + 2 * E->ai_var_E);
  samus_y_pos += d;
  gEnemyData(k + 128)->y_pos += d;
  gEnemyData(k + 64)->y_pos += d;
  E->y_pos += d;
  if (!sign16(++E->ai_var_E - 17)) {
    E->ai_preinstr = fGunship_5;
    E->ai_var_E = E->y_pos;
    E->ai_var_D = 1;
    gEnemyData(0)->ai_var_C = 0;
    samus_x_pos = E->x_pos + 1;
    samus_prev_x_pos = samus_x_pos;
    HatchAnim(k, kIlist_A5BE, 0x14);
  }
}

static void GunshipTop_6(uint16 k) {  // 0xA2A950: Samus sale por la escotilla
  EnemyData *E = gEnemyData(k);
  uint16 r18 = E->ai_var_E - 30;
  if (sign16(--samus_y_pos - r18)) {
    E->ai_preinstr = fGunship_7;
    HatchAnim(k, kIlist_A5EE, 0x15);
  }
}

static void GunshipTop_7(uint16 k) {  // 0xA2A987: Samus libre
  if (TimerDone()) {
    gEnemyData(k)->ai_preinstr = fGunship_8;
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
    frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    loading_game_state = 5;                       // kLoadingGameState_5_Main
    used_save_stations_and_elevators[0] |= 1;     /* palabra |= 1: el bit 0 del byte del área 0 */
    load_station_index = 0;
    SaveExploredMapTilesToSaved();                /* SaveToSram: la Genesis todavía no guarda partidas */
  }
}

static void GunshipTop_8(uint16 k) {  // 0xA2A9BD: esperar a que Samus entre (abajo sobre la nave)
  if (game_state == 8 && frame_handler_alfa == FUNC16(Samus_FrameHandlerAlfa_Func11)) {
    EnemyData *E = gEnemyData(k);
    if ((int16)(E->x_pos - 8 - samus_x_pos) < 0 && (int16)(E->x_pos + 8 - samus_x_pos) >= 0 &&
        (int16)(E->y_pos - 64 - samus_y_pos) < 0 && (int16)(E->y_pos - samus_y_pos) >= 0 &&
        !samus_movement_type && (joypad1_newkeys & kButton_Down)) {
      E->ai_preinstr = fGunship_9;
      if (samus_x_pos != 1152) {
        samus_x_pos = E->x_pos;
        samus_prev_x_pos = samus_x_pos;
      }
      MakeSamusFaceForward();
      CallSomeSamusCode(0x1A);
      elevator_status = 0;
      gEnemyData(k + 128)->y_pos = E->y_pos - 1;
      HatchAnim(k, kIlist_A5BE, 0x14);
    }
  }
}

static void GunshipTop_10(uint16 k) {  // 0xA2AA5D: Samus baja dentro de la nave
  EnemyData *E = gEnemyData(k);
  uint16 r18 = E->ai_var_E + 18;
  samus_y_pos += 2;
  if (!sign16(samus_y_pos - r18)) {
    E->ai_preinstr = fGunship_11;
    HatchAnim(k, kIlist_A5EE, 0x15);
  }
}

static void GunshipTop_12(uint16 k) {  // 0xA2AAA2: recargar todo (o, con la bomba de Zebes, despegar)
  if (CheckEventHappened(0xE)) {
    nt_unimpl("GunshipTop_12 (huida final de Zebes)");
    return;
  }
  Samus_RestoreHealth(2);
  Samus_RestoreMissiles(2);
  Samus_RestoreSuperMissiles(2);
  Samus_RestorePowerBombs(2);
  if ((int16)(samus_reserve_health - samus_max_reserve_health) >= 0 && (int16)(samus_health - samus_max_health) >= 0 &&
      (int16)(samus_missiles - samus_max_missiles) >= 0 &&
      (int16)(samus_super_missiles - samus_max_super_missiles) >= 0 &&
      (int16)(samus_power_bombs - samus_max_power_bombs) >= 0)
    gEnemyData(k)->ai_preinstr = fGunship_13;
}

static void GunshipTop_13(uint16 k) {  // 0xA2AB1F: "¿guardar?" (caja de mensaje aún sin portar: se guarda)
  used_save_stations_and_elevators[0] |= 1;
  load_station_index = 0;
  SaveExploredMapTilesToSaved();
  gEnemyData(k)->ai_preinstr = fGunship_14;
  HatchAnim(k, kIlist_A5BE, 0x14);
}

static void GunshipTop_15(uint16 k) {  // 0xA2AB6E: Samus sale otra vez
  EnemyData *E = gEnemyData(k);
  uint16 r18 = E->ai_var_E - 30;
  samus_y_pos -= 2;
  if (sign16(samus_y_pos - r18)) {
    E->ai_preinstr = fGunship_16;
    HatchAnim(k, kIlist_A5EE, 0x15);
  }
}

static void GunshipTop_16(uint16 k) {  // 0xA2ABA5
  if (TimerDone()) {
    gEnemyData(k)->ai_preinstr = fGunship_8;
    if (sign16(game_state - 40)) {
      frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
      frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    }
  }
}

void GunshipTop_Main(void) {  // 0xA2A759
  uint16 k = cur_enemy_index;
  EnemyData *E1 = gEnemyData(k + 64);
  int a = E1->ai_var_D == 1;
  int b = (--E1->ai_var_D & 0x8000) != 0;
  if (a || b) {                                   /* el zumbido de la nave */
    QueueSfx2_Max6(0x4D);
    E1->ai_var_D = 70;
  }
  EnemyData *E0 = gEnemyData(k);
  uint16 f = E0->ai_preinstr;
  if (!sign16(f + 0x56BE) && sign16(f + 0x53E5))  /* de GunshipTop_5 a GunshipTop_16: flota */
    GunshipTop_1(k);
  switch (f) {
  case fGunship_nullsub_187: break;
  case fGunship_3: GunshipTop_3(k); break;
  case fGunship_4: GunshipTop_4(k); break;
  case fGunship_5: if (TimerDone()) E0->ai_preinstr = fGunship_6; break;
  case fGunship_6: GunshipTop_6(k); break;
  case fGunship_7: GunshipTop_7(k); break;
  case fGunship_8: GunshipTop_8(k); break;
  case fGunship_9: if (TimerDone()) E0->ai_preinstr = fGunship_10; break;
  case fGunship_10: GunshipTop_10(k); break;
  case fGunship_11: if (TimerDone()) E0->ai_preinstr = fGunship_12; break;
  case fGunship_12: GunshipTop_12(k); break;
  case fGunship_13: GunshipTop_13(k); break;
  case fGunship_14: if (TimerDone()) E0->ai_preinstr = fGunship_15; break;
  case fGunship_15: GunshipTop_15(k); break;
  case fGunship_16: GunshipTop_16(k); break;
  default: nt_unimpl("GunshipTop_Main (función sin portar)"); break;
  }
}

/* instrucciones de las listas de la nave (solo las comunes: Goto $80ED, Sleep $812F) */
uint16 GunshipInstrWord(uint16 a) { return GS_W(a); }
