/* MegaMetroid — motor nativo: puerta de Ceres (enemigo E23F, banco $A6 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_a6.c (CeresDoor_*, y las instrucciones de CeresSteam que usan sus listas).
 * Las listas de instrucciones vienen preparadas en el PC (kBankA6_CeresDoorInstr, palabras ya nativas). */
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "nmath.h"

static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }

/* datos de las listas de instrucciones de los enemigos (por banco) */
uint16 EnemyInstrWord(uint8 bank, uint16 a) {
  if (bank == 0xA6)
    return A6W(a);
  if (bank == 0xA2 && a >= kBankA2_Gunship_LO && a < kBankA2_Gunship_HI)
    return GunshipInstrWord(a);
  { extern uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w); uint16 w; if (port_bank_word(bank, a, &w)) return w; }
  nt_unimpl("EnemyInstrWord (lista de instrucciones sin preparar)");
  return 0x7FFF;
}

static void WriteColorsToTargetPalette_(uint16 k, const uint16 *src, uint16 n) {  // 0xA9D2F6
  do { target_palettes[k >> 1] = *src++; k += 2; } while (--n);
}

/* instrucciones: devuelven NULL si la lista se detiene este cuadro; si no, *pc queda en la instrucción siguiente */
const uint16 *CallEnemyInstr(uint8 bank, uint16 f, uint16 k, uint16 *pc) {
  static const uint16 kOk = 1;
  EnemyData *E = gEnemyData(k);
  uint16 jp = *pc + 2;                          /* los argumentos van detrás de la instrucción */
  if (bank != 0xA6) {                           /* otros bancos: las traducidas del decomp (docs/42) */
    extern uint16 port_CallEnemyInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);
    uint8 ok;
    uint16 r = port_CallEnemyInstr((uint32)bank << 16 | f, k, jp, &ok);
    if (ok) { if (!r) return 0; *pc = r; return &kOk; }
    if (f == 0x80ED) { *pc = EnemyInstrWord(bank, jp); return &kOk; }                     // EnemyInstr_Goto
    if (f == 0x812F) { E->current_instruction = *pc; return 0; }                           // EnemyInstr_Sleep
    {   /* instrucciones comunes ($XX:80xx-81xx, iguales en cada banco de enemigos; enemy_instr.c) */
      extern uint16 EnemyInstr_GotoRel(uint16 k, uint16 jp), EnemyInstr_DecTimerAndGotoRel(uint16 k, uint16 jp),
                    EnemyInstr_WaitNframes(uint16 k, uint16 jp), EnemyInstr_CopyToVram(uint16 k, uint16 jp);
      uint16 r2 = 1;
      switch (f) {
      case 0x80F2: r2 = EnemyInstr_GotoRel(k, jp); break;
      case 0x8118: r2 = EnemyInstr_DecTimerAndGotoRel(k, jp); break;
      case 0x813A: r2 = EnemyInstr_WaitNframes(k, jp); break;
      case 0x814B: r2 = EnemyInstr_CopyToVram(k, jp); break;
      }
      if (r2 != 1) { if (!r2) return 0; *pc = r2; return &kOk; }
    }
    nt_unimpl_v("CallEnemyInstr (banco)", (uint16)(bank << 8 | (f >> 8))); nt_unimpl_v("CallEnemyInstr (instr)", f); nt_unimpl_v("CallEnemyInstr pc", *pc); nt_unimpl_v("CallEnemyInstr id", E->enemy_ptr); return 0;
  }
  switch (f) {
  case 0x80ED: *pc = EnemyInstrWord(bank, jp); return &kOk;                               // EnemyInstr_Goto
  case 0x812F: E->current_instruction = *pc; return 0;                                     // EnemyInstr_Sleep
  case 0xF63E:                                                                              // CeresDoor_Instr_6
    if (abs16(E->x_pos - samus_x_pos) >= 0x30 || abs16(E->y_pos - samus_y_pos) >= 0x30)
      *pc = EnemyInstrWord(bank, jp);
    else
      *pc = jp + 2;
    return &kOk;
  case 0xF66A:                                                                              // CeresDoor_Instr_4
    /* el original lee una palabra en boss_bits_for_area[area_index] (el byte del área y el siguiente) */
    *pc = (boss_bits_for_area[area_index] & 1) ? jp + 2 : EnemyInstrWord(bank, jp);
    return &kOk;
  case 0xF678:                                                                              // CeresDoor_Instr_8
    *pc = ceres_status ? jp + 2 : EnemyInstrWord(bank, jp);
    return &kOk;
  case 0xF11D: E->properties |= 0x400 | 0x100; *pc = jp; return &kOk;                     // CeresSteam_Instr_1
  case 0xF127:                                                                              // CeresSteam_Instr_2
    if (E->ai_var_D-- != 1)
      *pc = EnemyInstrWord(bank, jp);
    else {
      *pc = EnemyInstrWord(bank, jp + 2);
      E->properties &= 0xFAFF;                                                              // CeresSteam_Instr_3
    }
    return &kOk;
  case 0xF135: E->properties &= 0xFAFF; *pc = jp; return &kOk;                            // CeresSteam_Instr_3
  case 0xF68B: E->properties |= 0x400; *pc = jp; return &kOk;                             // CeresSteam_Instr_4
  case 0xF695: E->properties &= ~0x400; *pc = jp; return &kOk;                            // CeresDoor_Instr_1
  case 0xF69F: E->ai_var_B = 1; *pc = jp; return &kOk;                                    // CeresDoor_Instr_3
  case 0xF6A6: E->properties |= 0x100; *pc = jp; return &kOk;                             // CeresSteam_Instr_5
  case 0xF6B0: E->ai_var_B = 0; E->properties &= ~0x100; *pc = jp; return &kOk;           // CeresDoor_Instr_5 (+_2)
  case 0xF6B3: E->properties &= ~0x100; *pc = jp; return &kOk;                            // CeresDoor_Instr_2
  case 0xF6BD: QueueSfx3_Max6(0x2C); *pc = jp; return &kOk;                               // CeresDoor_Instr_7
  default:
    if (f >= 0xE4BE && f <= 0xE976) {                                                        // Ridley_Instr_*
      int ok;
      *pc = RidleyInstr(f, jp, &ok);
      return ok ? &kOk : 0;
    }
    {                                                                                        // las demás del banco $A6
      extern uint16 port_CallEnemyInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);           // (mini Kraid...): traducidas
      uint8 ok;
      uint16 r = port_CallEnemyInstr(0xA60000 | f, k, jp, &ok);
      if (ok) { if (!r) return 0; *pc = r; return &kOk; }
    }
    nt_unimpl_v("CallEnemyInstr (instrucción sin portar)", f); return 0;
  }
}

void CeresDoor_Init(void) {  // 0xA6F6C5
  EnemyData *E = gEnemyData(cur_enemy_index);
  E->spritemap_pointer = 0xFAC7;                  // kCeresDoor_Sprmap_FAC7
  E->instruction_timer = 1;
  E->timer = 0;
  E->vram_tiles_index = 0;
  E->palette_index = 1024;
  int v2 = E->parameter_1;
  E->ai_var_A = A6W(0xF72B + 2 * v2);            // g_off_A6F72B
  E->current_instruction = A6W(0xF52C + 2 * v2);  // g_off_A6F52C
  E->ai_var_B = 0;
  /* CeresDoor_Func_1: la puerta 2 sube tiles a la VRAM (la Genesis los tiene) */
  const uint16 *v3;
  if (ceres_status) {
    v3 = word_A6F50C + 1;
  } else {
    if (E->parameter_1 == 3) {
      WriteColorsToTargetPalette_(0x142, kCeresDoor_Palette + 1, 0xF);
      return;
    }
    v3 = kCeresDoor_Palette + 1;
  }
  gEnemyData(cur_enemy_index)->palette_index = 3584;
  WriteColorsToTargetPalette_(0x1E2, v3, 0xF);
}

static void CeresDoor_F773(uint16 j) {  // 0xA6F773
  if (ceres_status >= 2 && !earthquake_timer) {
    if ((random_number & 0xFFF) < 0x80) {
      earthquake_timer = 4;
      earthquake_type = j + 6;
    } else {
      earthquake_timer = 2;
      earthquake_type = j;
    }
  }
}

void CeresDoor_QueueMode7Transfers(uint16 which);   /* capa de la Genesis: CeresDoor_Func_8 (tiles del ascensor) */

static void CeresDoor_Func_7(void) {  // 0xA6F850
  CeresDoor_QueueMode7Transfers((nmi_frame_counter_word & 2) >> 1);   // CeresDoor_Func_8
  if (!palette_change_num) {
    /* WriteColorsToPalette(0x52, $A6, 2*(n & 0x38) - 0x78F, 6): 6 colores desde $A6:F871 + 2*(n & 0x38) */
    const uint16 *src = kCeresDoor_PaletteCycle + (nmi_frame_counter_word & 0x38);
    for (int i = 0; i < 6; i++)
      palette_buffer[41 + i] = src[i];
    nt_pal_dirty(41, 46);
  }
}

void CeresDoor_Main(void) {  // 0xA6F765
  EnemyData *E = gEnemyData(cur_enemy_index);
  switch (E->ai_var_A) {
  case 0xF76B: CeresDoor_F773(0x14); break;                         // CeresDoor_Func_2
  case 0xF770: CeresDoor_F773(0x1D); break;                         // CeresDoor_Func_3
  case 0xF7A5:                                                      // CeresDoor_Func_4
    E->properties |= 0x100;
    if (ceres_status & 1) {
      E->palette_index = 3584;
      E->properties &= ~0x100;
    }
    break;
  case 0xF7BD:                                                      // CeresDoor_Func_5
    CeresDoor_Func_7();
    if (ceres_status >= 2) {
      E->ai_var_A = 0xF7DC;                                         // CeresDoor_Func_6
      E->ai_var_D = 48;
      EnemyData *E0 = gEnemyData(0);
      E0->ai_var_E = 0;
      E0->ai_preinstr = 0;                                          // cdr_var_F
    }
    break;
  case 0xF7DC: {                                                    // CeresDoor_Func_6: la puerta explota
    if (--E->ai_var_D & 0x8000) {
      E->properties |= 0x100;
      E->ai_var_A = 0xF850;                                         // CeresDoor_Func_7
      if (ceres_status)                                             // CeresDoor_Func_6b
        ceres_status = 0x8000;
    } else {
      EnemyData *E0 = gEnemyData(0);
      if (--E0->ai_var_E & 0x8000) {
        E0->ai_var_E = 4;
        if (--E0->ai_preinstr & 0x8000)
          E0->ai_preinstr = 3;
        uint16 v5 = 2 * E0->ai_preinstr;                            // g_word_A6F840: pares x, y
        extern uint16 eproj_spawn_x, eproj_spawn_y;
        eproj_spawn_x = E->x_pos + A6W(0xF840 + 2 * v5);
        eproj_spawn_y = E->y_pos + A6W(0xF840 + 2 * v5 + 2);
        uint16 v6 = 3;
        if (NextRandom() < 0x4000)
          v6 = 12;
        SpawnEprojWithRoomGfx(0xE509, v6);
        QueueSfx2_Max6(0x25);
      }
    }
    break;
  }
  case 0xF850: CeresDoor_Func_7(); break;                           // CeresDoor_Func_7
  default: nt_unimpl("CeresDoor_Main (función sin portar)"); break;
  }
}

/* ------------------------------------------------------------------ vapor de Ceres (enemigo E1FF, $A6:EFB1) */
void CeresSteam_Init(void) {  // 0xA6EFB1
  EnemyData *E = gEnemyData(cur_enemy_index);
  E->vram_tiles_index = 0;
  E->properties |= 0x2000;                        // kEnemyProps_DisableSamusColl
  E->extra_properties |= 4;
  E->instruction_timer = 1;
  E->timer = 0;
  E->palette_index = 2560;
  E->ai_var_D = (NextRandom() & 0x1F) + 1;
  int v1 = E->parameter_1;
  E->current_instruction = A6W(0xEFF5 + 2 * v1);  // g_off_A6EFF5
  E->ai_var_A = A6W(0xF001 + 2 * v1);             // g_off_A6F001
}

void CeresSteam_Main(void) {  // 0xA6F00D
  EnemyData *E = gEnemyData(cur_enemy_index);
  E->health = 0x7FFF;
  switch (E->ai_var_A) {
  case 0xEFF4: return;                            // nullsub_350
  case 0xF019: {                                  // CeresSteam_Func_1
    uint16 x = E->x_pos, y = E->y_pos;
    Mode7Rotate(&x, &y);                          // CalcCeresSteamPos_Mode7
    EnemySpawnData *v3 = gEnemySpawnData(cur_enemy_index);
    v3->xpos2 = x - E->x_pos;
    v3->ypos2 = y - E->y_pos;
    return;
  }
  default: nt_unimpl("CeresSteam_Main (función)"); return;
  }
}

void CeresSteam_Touch(void) {  // 0xA6F03F
  gEnemyData(cur_enemy_index)->health = 0x7FFF;
  NormalEnemyTouchAiSkipDeathAnim();              /* NormalEnemyTouchAi: con 0x7FFF de energía no muere */
}
