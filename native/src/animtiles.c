/* MegaMetroid — motor nativo: tiles animados (banco $87 del decomp, sm_87.c).
 * Cada objeto tiene una lista de instrucciones (banco $87, en palabras del 68000: B87W) que cada tantos cuadros
 * elige un trozo de tiles (animtiles_src_ptr, de animtiles_sizes bytes) para la VRAM (animtiles_vram_ptr). La capa
 * de la Genesis sube los tiles ya convertidos que corresponden a ese trozo. */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "animtiles.h"
#include "hdmaobj.h"

void EnableAnimtiles(void) { animtiles_enable_flag |= 0x8000; }    // 0x878000
void DisableAnimtiles(void) { animtiles_enable_flag &= ~0x8000; }  // 0x87800B
void ClearAnimtiles(void) {                                         // 0x878016
  for (int i = 5; i >= 0; i--)
    animtiles_ids[i] = 0;
}

void SpawnAnimtiles(uint16 j) {  // 0x878027
  uint16 v = 10;
  while (animtiles_ids[v >> 1]) {
    v -= 2;
    if (v & 0x8000)
      return;
  }
  int i = v >> 1;
  animtiles_ids[i] = j;
  animtiles_timers[i] = 0;
  animtiles_instr_list_ptrs[i] = B87W(j);
  animtiles_src_ptr[i] = 0;
  animtiles_sizes[i] = B87W(j + 2);
  animtiles_vram_ptr[i] = B87W(j + 4);
  animtiles_instr_timers[i] = 1;
}

static uint16 Goto(uint16 j) { return B87W(j); }

/* una instrucción: devuelve el puntero siguiente o 0 (el objeto espera o se borró) */
static uint16 CallAnimtilesInstr(uint16 f, uint16 k, uint16 j) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(AnimtilesInstr_Delete): animtiles_ids[v] = 0; return 0;
  case FUNC16(AnimtilesInstr_Goto): return Goto(j);
  case FUNC16(AnimtilesInstr_GotoRel): animtiles_instruction = j; return j + (int8)B87W(j);
  case FUNC16(AnimtilesInstr_DecrementTimerAndGoto): return animtiles_timers[v]-- == 1 ? j + 2 : Goto(j);
  case FUNC16(AnimtilesInstr_DecrementTimerAndGotoRel):
    if (animtiles_timers[v]-- == 1) return j + 1;
    animtiles_instruction = j;
    return j + (int8)B87W(j);
  case FUNC16(AnimtilesInstr_SetTimer):                /* solo el byte bajo */
    animtiles_timers[v] = (animtiles_timers[v] & 0xFF00) | (uint8)B87W(j);
    return j + 1;
  case FUNC16(AnimtilesInstr_QueueMusic): QueueMusic_Delayed8((uint8)B87W(j)); return j + 1;
  case FUNC16(AnimtilesInstr_QueueSfx1): QueueSfx1_Max6((uint8)B87W(j)); return j + 1;
  case FUNC16(AnimtilesInstr_QueueSfx2): QueueSfx2_Max6((uint8)B87W(j)); return j + 1;
  case FUNC16(AnimtilesInstr_QueueSfx3): QueueSfx3_Max6((uint8)B87W(j)); return j + 1;
  case FUNC16(AnimtilesInstr_GotoIfBossBitSet):
    return (CheckBossBitForCurArea((uint8)B87W(j)) & 1) ? Goto(j + 1) : j + 3;
  case FUNC16(AnimtilesInstr_SetBossBit): SetBossBitForCurArea((uint8)B87W(j)); return j + 1;
  case FUNC16(AnimtilesInstr_GotoIfEventHappened): return (CheckEventHappened(B87W(j)) & 1) ? Goto(j + 2) : j + 4;
  case FUNC16(AnimtilesInstr_SetEventHappened): SetEventHappened(B87W(j)); return j + 2;
  case FUNC16(AnimtilesInstr_WaitUntilAreaBossDead_DoubleRet):
    if (!(CheckBossBitForCurArea(1) & 1)) { animtiles_instr_timers[v] = 1; return 0; }
    return j;
  case FUNC16(AnimtilesInstr_GotoIfBossBitSetInArea):
    return ((uint8)B87W(j) & boss_bits_for_area[(uint8)B87W(j + 1)]) ? Goto(j + 2) : j + 4;
  case FUNC16(AnimtilesInstr_GotoIfTourianStatueBusy): return (tourian_entrance_statue_animstate & 0x8000) ? Goto(j) : j + 2;
  case FUNC16(AnimtilesInstr_TourianStatueSetState): tourian_entrance_statue_animstate |= B87W(j); return j + 2;
  case FUNC16(AnimtilesInstr_TourianStatueClearState): tourian_entrance_statue_animstate &= ~B87W(j); return j + 2;
  case FUNC16(AnimtilesInstr_Clear3PaletteColors): {
    int c = B87W(j) >> 1;
    palette_buffer[c] = palette_buffer[c + 1] = palette_buffer[c + 2] = 0;
    nt_pal_dirty((uint8)c, (uint8)(c + 2));
    return j + 2;
  }
  case FUNC16(AnimtilesInstr_SpawnPalfxObj): SpawnPalfxObject(B87W(j)); return j + 2;
  case FUNC16(AnimtilesInstr_Write8PaletteColors): {
    static const uint16 kColors[8] = { 0x3800, 0x7f58, 0x6ed5, 0x5a71, 0x49ee, 0x356a, 0x24e7, 0x1083 };
    uint16 c = B87W(j) >> 1;
    for (int i = 0; i < 8; i++) target_palettes[c + i] = kColors[i];
    return j + 2;
  }
  default:   /* estatua de Tourian: proyectiles enemigos con gráficos de la sala, aún sin portar */
    nt_unimpl("AnimtilesInstr (instrucción sin portar)");
    animtiles_ids[v] = 0;
    return 0;
  }
}

static void ProcessAnimtilesObject(void) {  // 0x878085
  uint16 k = animtiles_object_index;
  int v = k >> 1;
  if (animtiles_instr_timers[v]-- != 1)
    return;
  uint16 p = animtiles_instr_list_ptrs[v], f;
  while ((f = B87W(p)) & 0x8000) {
    animtiles_instruction = f;
    p = CallAnimtilesInstr(f, k, p + 2);
    if (!p)
      return;
  }
  animtiles_instr_timers[v] = f;
  animtiles_src_ptr[v] = B87W(p + 2);
  animtiles_instr_list_ptrs[v] = p + 4;
}

void AnimtilesHandler(void) {  // 0x878064
  if (animtiles_enable_flag & 0x8000) {
    for (int i = 10; i >= 0; i -= 2) {
      animtiles_object_index = i;
      if (animtiles_ids[i >> 1]) {
        ProcessAnimtilesObject();
        i = animtiles_object_index;
      }
    }
  }
}
