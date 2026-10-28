/* MegaMetroid — motor nativo: efectos de paleta (banco $8D del decomp, sm_8d.c; docs/35).
 * Objetos con una lista de instrucciones que escriben colores en palette_buffer cada tantos cuadros. Las listas y
 * los colores se leen del banco $8D ya en palabras del 68000 (B8DW). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "enemies.h"

void EnablePaletteFx(void) { flag_for_palette_fx_objects |= 0x8000; }    // 0x8DC4C2
void DisablePaletteFx(void) { flag_for_palette_fx_objects &= ~0x8000; }  // 0x8DC4CD
void ClearPaletteFXObjects(void) {                                        // 0x8DC4D8
  for (int i = 7; i >= 0; i--)
    palettefx_ids[i] = 0;
}

static void CallPalfxInit(uint16 f, uint16 j) {
  switch (f) {
  case FUNC16(nullsub_131): break;
  case FUNC16(PalInit_E1BC): palettefx_pre_instr[j >> 1] = FUNC16(PalPreInstr_E1BC); break;   // 0x8DE204
  case FUNC16(PalInit_F761_Norfair1):                                                          // 0x8DE440
    palettefx_instr_list_ptrs[j >> 1] = (equipped_items & 0x20) ? 0xE8B6 : (equipped_items & 1) ? 0xE68A : 0xE45E;
    break;
  case FUNC16(PalInit_F779_Brinstar8):                                                         // 0x8DF730
    if (boss_bits_for_area[area_index] & 2)
      palettefx_ids[j >> 1] = 0;
    break;
  default: nt_unimpl("PalfxInit (inicio de efecto de paleta sin portar)"); break;
  }
}

void SpawnPalfxObject(uint16 j) {  // 0x8DC4E9
  uint16 v = 14;
  while (palettefx_ids[v >> 1]) {
    v -= 2;
    if (v & 0x8000)
      return;
  }
  int i = v >> 1;
  palettefx_ids[i] = j;
  palettefx_color_indexes[i] = 0;
  palettefx_pre_instr[i] = FUNC16(PalPreInstr_nullsub_129);
  palettefx_instr_list_ptrs[i] = B8DW(j + 2);
  palettefx_instr_timers[i] = 1;
  palettefx_timers[i] = 0;
  CallPalfxInit(B8DW(j), v);
}

static void CallPalFxPreInstr(uint16 f, uint16 k) {
  switch (f) {
  case FUNC16(PalPreInstr_nullsub_129): case addr_locret_8DC5E3: break;
  case FUNC16(PalPreInstr_E1BC):                                  // 0x8DE20B: la página 2 de la intro
    if (cinematic_function == FUNC16(CinematicFunction_Intro_Page2)) {
      palettefx_instr_list_ptrs[k >> 1] = 0xE192;
      palettefx_instr_timers[k >> 1] = 1;
    }
    break;
  case FUNC16(PalPreInstr_CheckEnemy0Health):                     // 0x8DE2E0
    if (!enemy_data[0].health)
      palettefx_ids[k >> 1] = 0;
    break;
  case FUNC16(PalPreInstr_SamusInHeat): {                         // 0x8DE379
    if (!(equipped_items & 0x21)) {
      uint32 t = ((uint32)samus_periodic_damage << 16 | samus_periodic_subdamage) + 0x4000;
      samus_periodic_damage = (uint16)(t >> 16);
      samus_periodic_subdamage = (uint16)t;
      if (!(nmi_frame_counter_word & 7) && samus_health > 0x46)
        QueueSfx3_Max6(0x2D);
    }
    if (samus_in_heat_palfx_index != samus_in_heat_palettefx_prev_index) {
      samus_in_heat_palettefx_prev_index = samus_in_heat_palfx_index;
      int v3 = k >> 1;
      palettefx_instr_timers[v3] = 1;
      uint16 tab = (equipped_items & 0x20) ? 0xE3E0 : (equipped_items & 1) ? 0xE400 : 0xE420;
      palettefx_instr_list_ptrs[v3] = B8DW(tab + 2 * samus_in_heat_palfx_index);
    }
    break;
  }
  case FUNC16(PalPreInstr_SwitchIfYpos): case FUNC16(PalPreInstr_SwitchIfYpos2):   // 0x8DEC59 / 0x8DED84
    if (samus_y_pos < 0x380) {
      palettefx_instr_timers[k >> 1] = 1;
      palettefx_instr_list_ptrs[k >> 1] = f == FUNC16(PalPreInstr_SwitchIfYpos) ? 0xEB43 : 0xEC76;
    }
    break;
  case FUNC16(PalPreInstr_DeletePalfxIfMinibossDead):             // 0x8DEEC5
    if (boss_bits_for_area[area_index] & 2)
      palettefx_ids[k >> 1] = 0;
    break;
  case FUNC16(PalPreInstr_F621): {                                // 0x8DF621: la palabra en 7E:1E79 + k
    uint16 w = k == 0 ? flag_for_palette_fx_objects : k == 2 ? palettefx_index : palettefx_ids[(k - 4) >> 1];
    if (w)
      palettefx_ids[k >> 1] = 0;
    break;
  }
  default: nt_unimpl("PalPreInstr (pre-instrucción de paleta sin portar)"); break;
  }
}

/* una instrucción ($8D:C595...): devuelve 0 en j si el objeto termina este cuadro; k: objeto o índice de color */
typedef struct { uint16 k, j; } PalRet;
static PalRet CallPalInstr(uint16 f, uint16 k, uint16 j) {
  PalRet r = { k, j };
  switch (f) {
  case FUNC16(PalInstr_Wait): palettefx_instr_list_ptrs[palettefx_index >> 1] = j + 4; r.k = 0; r.j = 0; break;
  case FUNC16(PalInstr_ColorPlus2): r.k = k + 4; r.j = j + 2; break;
  case FUNC16(PalInstr_ColorPlus3): r.k = k + 6; r.j = j + 2; break;
  case FUNC16(PalInstr_ColorPlus4): r.k = k + 8; r.j = j + 2; break;
  case FUNC16(PalInstr_ColorPlus8): r.k = k + 16; r.j = j + 2; break;
  case FUNC16(PalInstr_ColorPlus9): r.k = k + 18; r.j = j + 2; break;
  case FUNC16(PalInstr_ColorPlus15): r.k = k + 30; r.j = j + 2; break;
  case FUNC16(PalInstr_Delete): palettefx_ids[k >> 1] = 0; r.j = 0; break;
  case FUNC16(PalInstr_SetPreInstr): palettefx_pre_instr[k >> 1] = B8DW(j); r.j = j + 2; break;
  case FUNC16(PalInstr_ClearPreInstr): palettefx_pre_instr[k >> 1] = addr_locret_8DC5E3; break;
  case FUNC16(PalInstr_Goto): r.j = B8DW(j); break;
  case FUNC16(PalInstr_DecTimerGoto):
    r.j = (palettefx_timers[k >> 1]-- == 1) ? j + 2 : B8DW(j);
    break;
  case FUNC16(PalInstr_SetTimer):              /* solo el byte bajo */
    palettefx_timers[k >> 1] = (palettefx_timers[k >> 1] & 0xFF00) | (uint8)B8DW(j);
    r.j = j + 1;
    break;
  case FUNC16(PalInstr_SetColorIndex): palettefx_color_indexes[k >> 1] = B8DW(j); r.j = j + 2; break;
  case FUNC16(PalInstr_QueueMusic): QueueMusic_Delayed8((uint8)B8DW(j)); r.j = j + 1; break;
  case FUNC16(PalInstr_QueueSfx1): QueueSfx1_Max6(B8DW(j)); r.j = j + 1; break;
  case FUNC16(PalInstr_QueueSfx2): QueueSfx2_Max6(B8DW(j)); r.j = j + 1; break;
  case FUNC16(PalInstr_QueueSfx3): QueueSfx3_Max6(B8DW(j)); r.j = j + 1; break;
  case FUNC16(PalInstr_SetPalfxIndex): samus_in_heat_palfx_index = (uint8)B8DW(j); r.j = j + 1; break;
  default: nt_unimpl("PalInstr (instrucción de paleta sin portar)"); palettefx_ids[palettefx_index >> 1] = 0; r.j = 0; break;
  }
  return r;
}

static void PalFx_ProcessOne(uint16 k) {  // 0x8DC54A
  CallPalFxPreInstr(palettefx_pre_instr[k >> 1], k);
  uint16 v1 = palettefx_index;
  if (palettefx_instr_timers[v1 >> 1]-- != 1)
    return;
  uint16 j = palettefx_instr_list_ptrs[v1 >> 1], f;
  while ((f = B8DW(j)) & 0x8000) {
    PalRet r = CallPalInstr(f, v1, j + 2);
    v1 = r.k;
    j = r.j;
    if (!j)
      return;
  }
  int v8 = v1 >> 1;
  palettefx_instr_timers[v8] = f;
  uint16 ci = palettefx_color_indexes[v8];
  uint16 lo = 0xFFFF, hi = 0;
  do {
    uint16 c;
    while (!((c = B8DW(j + 2)) & 0x8000)) {
      palette_buffer[ci >> 1] = c;
      if ((ci >> 1) < lo) lo = ci >> 1;
      if ((ci >> 1) > hi) hi = ci >> 1;
      ci += 2;
      j += 2;
    }
    PalRet r = CallPalInstr(c, ci, j);
    ci = r.k;
    j = r.j;
  } while (j);
  if (lo <= hi)
    nt_pal_dirty((uint8)lo, (uint8)hi);
}

void PaletteFxHandler(void) {  // 0x8DC527
  if (flag_for_palette_fx_objects & 0x8000) {
    for (int i = 14; i >= 0; i -= 2) {
      palettefx_index = i;
      if (palettefx_ids[i >> 1]) {
        PalFx_ProcessOne(i);
        i = palettefx_index;
      }
    }
  }
}
