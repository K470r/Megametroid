/* MegaMetroid — motor nativo: nivel de la sala actual (docs/37). Ver room.h. */
#include "samus.h"
#include "room.h"

uint16 lvl_index = 0xFFFF;
uint8 lvl_mod[ROOM_MAX_BLOCKS / 8];
LvlMod lvl_ov[LVL_OV_N];
const uint16 *lvl_rom;
const uint8 *bts_rom;
uint16 lvl_blocks;
uint16 lvl_fast;                                   /* room.h: lvl_blocks mientras no haya bloques cambiados */

static const uint8 kZeroBts[512];

/* lvl_fast: el camino corto de level_get/bts_get vale mientras ningún bloque de la sala esté cambiado */
static void lvl_fast_update(void) {
  lvl_fast = lvl_blocks;
  for (uint16 s = 0; s < LVL_OV_N; s++)
    if (lvl_ov[s].used) { lvl_fast = 0; break; }
}

void lvl_attach(void) {
  lvl_fast = 0;
  if (lvl_index == 0xFFFF) { lvl_rom = 0; bts_rom = 0; lvl_blocks = 0; return; }
  if (lvl_index == LVL_INTRO_MB || lvl_index == LVL_INTRO_BABY) {   /* recuerdos de la intro (ver intro.c) */
    lvl_rom = lvl_index == LVL_INTRO_MB ? kIntroLevelMotherBrain : kIntroLevelBabyMetroid;
    bts_rom = kZeroBts;
    lvl_blocks = lvl_index == LVL_INTRO_MB ? 256 : 512;
    lvl_fast_update();
    return;
  }
  const LevelDataN *L = &kLevels[lvl_index];
  nt_map_level_page(L->page);
  lvl_rom = L->level;
  bts_rom = L->bts;
  lvl_blocks = L->size >> 1;
  if (lvl_blocks > ROOM_MAX_BLOCKS) { nt_unimpl("lvl_attach (sala más grande que el búfer del SNES)"); lvl_blocks = ROOM_MAX_BLOCKS; }
  lvl_fast_update();
}

void lvl_load(uint16 index) {
  lvl_index = index;
  memset(lvl_mod, 0, sizeof lvl_mod);
  memset(lvl_ov, 0, sizeof lvl_ov);
  lvl_attach();
}

static uint16 lvl_slot(uint16 i) { return i & (LVL_OV_N - 1); }   /* como lvl_find (room.h) */

LvlMod *lvl_find_slow(uint16 i) {
  uint16 s = lvl_slot(i);
  for (uint16 n = 0; n < LVL_OV_N; n++, s = (s + 1) & (LVL_OV_N - 1))
    if (lvl_ov[s].used && lvl_ov[s].idx == i) return &lvl_ov[s];
  return &lvl_ov[0];                               /* no pasa: el bit dice que está */
}

/* el bloque i pasa a la tabla de cambiados (con su valor actual) */
static LvlMod *lvl_mod_entry(uint16 i) {
  if (lvl_mod[i >> 3] & (1 << (i & 7))) return lvl_find(i);
  uint16 s = lvl_slot(i);
  for (uint16 n = 0; n < LVL_OV_N; n++, s = (s + 1) & (LVL_OV_N - 1))
    if (!lvl_ov[s].used) {
      LvlMod *m = &lvl_ov[s];
      m->idx = i;
      m->lvl = i < lvl_blocks ? lvl_rom[i] : 0x8000;
      m->bts = i < lvl_blocks ? bts_rom[i] : 0;
      m->used = 1;
      lvl_mod[i >> 3] |= 1 << (i & 7);
      lvl_fast = 0;
      return m;
    }
  nt_unimpl("level_set (más de LVL_OV_N bloques cambiados)");
  return 0;
}

void level_set(uint16 i, uint16 v) {
  if (i >= ROOM_MAX_BLOCKS) return;
  if (!(lvl_mod[i >> 3] & (1 << (i & 7))) && v == (i < lvl_blocks ? lvl_rom[i] : 0x8000)) return;
  LvlMod *m = lvl_mod_entry(i);
  if (m) m->lvl = v;
}

void bts_set(uint16 i, uint8 v) {
  if (i >= ROOM_MAX_BLOCKS) return;
  if (!(lvl_mod[i >> 3] & (1 << (i & 7))) && v == (i < lvl_blocks ? bts_rom[i] : 0)) return;
  LvlMod *m = lvl_mod_entry(i);
  if (m) m->bts = v;
}
