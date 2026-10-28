/* MegaMetroid — motor nativo: marco de enemigos (banco $A0 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_a0.c: carga de la población de la sala, EnemyMain (IA, instrucciones,
 * colisiones con Samus y con sus disparos), cola de dibujo por capas. Las cabeceras de enemigo, poblaciones y
 * sets de tiles vienen preparados en el PC (gen/tables.c: kEnemyDefs, kEnemyPopulations, kEnemyTilesets).
 * La IA de cada enemigo está en su archivo (ceres_door.c...). Los tiles de los enemigos los carga la Genesis. */
#include <string.h>
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "viewer.h"
#include "nmath.h"

EnemySpawnData enemy_spawn_data[ENEMY_COUNT];
uint16 enemy_ram7800[0x1000];

static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }

/* palabra de un banco de datos de enemigos (punteros de 16 bits del SNES, palabras ya nativas) */
static inline uint16 BankW(uint8 bank, uint16 a) {
  if (bank == 0xA6) return A6W(a);
  { extern uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w); uint16 w; if (port_bank_word(bank, a, &w)) return w; }
  nt_unimpl_v("BankW (banco sin preparar)", bank);
  return 0;
}

/* Definición de enemigo por su dirección: búsqueda binaria (mkdata.py los deja ordenados), con una caché de las
   últimas: el bucle de enemigos la pide muchas veces por cuadro y con todos los enemigos de Zebes la búsqueda son
   8 pasos (en la Genesis se notaba en la pelea con Ridley). */
const EnemyDefN *enemy_def_cache[8];
const EnemyDefN *enemy_def_slow(uint16 a) {
  const EnemyDefN **c = &enemy_def_cache[(a >> 6) & 7];
  int lo = 0, hi = (int)(sizeof kEnemyDefs / sizeof kEnemyDefs[0]) - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    if (kEnemyDefs[m].addr == a) return *c = &kEnemyDefs[m];
    if (kEnemyDefs[m].addr < a) lo = m + 1; else hi = m - 1;
  }
  nt_unimpl_v("enemy_def (enemigo sin preparar)", a);
  return &kEnemyDefs[0];
}
static const EnemyPopulationN *enemy_population(uint16 a) {   /* ordenadas por dirección (mkdata.py) */
  int lo = 0, hi = (int)(sizeof kEnemyPopulations / sizeof kEnemyPopulations[0]) - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    if (kEnemyPopulations[m].addr == a) return &kEnemyPopulations[m];
    if (kEnemyPopulations[m].addr < a) lo = m + 1; else hi = m - 1;
  }
  nt_unimpl("enemy_population (población sin preparar)");
  return &kEnemyPopulations[0];
}
static const EnemyTilesetN *enemy_tileset(uint16 a) {
  for (unsigned i = 0; i < sizeof kEnemyTilesets / sizeof kEnemyTilesets[0]; i++)
    if (kEnemyTilesets[i].addr == a) return &kEnemyTilesets[i];
  nt_unimpl("enemy_tileset (set sin preparar)");
  return &kEnemyTilesets[0];
}

/* ------------------------------------------------------------------ IA (CallEnemyAi) */
typedef void (*PortFnV)(void);
PortFnV port_FindCallEnemyAi(uint32 ea);
void port_nop_v(void);
void CallEnemyAi(uint8 bank, uint16 f) {
  uint32 ea = (uint32)bank << 16 | f;
  static uint32 cea[16];                         /* traducidas ya buscadas (casi siempre las mismas cada cuadro) */
  static PortFnV cfn[16];
  uint16 h = (f ^ (f >> 4) ^ bank) & 15;
  if (cea[h] == ea) { if (cfn[h] != port_nop_v) cfn[h](); return; }
  switch (ea) {
  case 0xA0804C: case 0xA0804B: case 0xA6804C: case 0xA6F920: return;     // nullsub_170 / 169 / 170_A6 / 41
  case 0xA6F6C5: CeresDoor_Init(); return;
  case 0xA6F765: CeresDoor_Main(); return;
  case 0xA6EFB1: CeresSteam_Init(); return;
  case 0xA6F00D: CeresSteam_Main(); return;
  case 0xA6F03F: CeresSteam_Touch(); return;
  case 0xA6A0F5: CeresRidley_Init(); return;
  case 0xA6A288: CeresRidley_Main(); return;
  case 0xA6A2D3: CeresRidley_Hurt(); return;
  case 0xA6DF8A: Ridley_Shot(); return;
  case 0xA6DFB2: Ridley_Powerbomb(); return;
  case 0xA6DF59: NormalEnemyTouchAiSkipDeathAnim(); return;      // sub_A6DF59 (cajas de Ridley)
  case 0xA2804C: case 0xA2800F: case 0xA28041: return;           // nullsubs del banco $A2
  case 0xA2A644: GunshipTop_Init(); return;
  case 0xA2A6D2: GunshipBottom_Init(); return;
  case 0xA2A759: GunshipTop_Main(); return;
  default:
    { PortFnV fn = port_FindCallEnemyAi(ea);      /* traducidas del decomp (docs/42) */
      if (fn) { cea[h] = ea; cfn[h] = fn; if (fn != port_nop_v) fn(); return; } }
#ifndef __mc68000__
    { extern int printf(const char *, ...); printf("CallEnemyAi %06X enemy %u ptr %04X stage %u\n", (unsigned)ea, cur_enemy_index >> 6, gEnemyData(cur_enemy_index)->enemy_ptr, enemy_processing_stage); }
#endif
    nt_unimpl_v("CallEnemyAi (IA de enemigo sin portar)", ea); return;
  }
}

/* ------------------------------------------------------------------ carga ($A0:8A1E) */
static void ClearSpriteObjects(void) {  // 0xB4BD97 (el original borra 511 palabras: los 8 arreglos menos la primera)
  for (int i = 1; i < 64; i++) sprite_instr_list_ptrs[i] = 0;
  for (int i = 0; i < 64; i++) {
    sprite_instr_timer[i] = 0; sprite_palettes[i] = 0; sprite_x_pos[i] = 0; sprite_x_subpos[i] = 0;
    sprite_y_pos[i] = 0; sprite_y_subpos[i] = 0; sprite_disable_flag[i] = 0;
  }
}

static void ProcessEnemyTilesets(void) {  // 0xA08D64
  enemy_tile_load_data_write_pos = 0;
  for (int i = 0; i < 4; i++) {
    enemy_def_ptr[i] = 0;
    enemy_gfxdata_tiles_index[i] = 0;
    enemy_gfxdata_vram_ptr[i] = 0;
  }
  uint16 w = 0, next_enemy_tiles_index = 0;
  const EnemyTilesetN *ET = enemy_tileset(room_enemy_tilesets_ptr);
  for (int i = 0; i < ET->n; i++) {
    const EnemyDefN *ED = enemy_def(ET->enemy_def[i]);
    uint16 p = (LO8(ET->vram_dst[i]) + 8) * 16;
    for (int c = 0; c < 16; c++)
      target_palettes[p + c] = ED->palette[c];
    /* enemy_tile_load_data: qué tiles copiar; la Genesis tiene los tiles ya convertidos */
    enemy_tile_load_data_write_pos += 7;
    enemy_gfxdata_tiles_index[w] = next_enemy_tiles_index;
    enemy_def_ptr[w] = ET->enemy_def[i];
    enemy_gfxdata_vram_ptr[w] = ET->vram_dst[i];
    w++;
    next_enemy_tiles_index += ED->tile_data_size >> 5;
  }
}

void LoadEnemies(void) {  // 0xA08A1E
  debug_time_frozen_for_enemies = 0;
  nt_enemy_gfx_drawn_hook = 0;                               // enemy_gfx_drawn_hook = nullsub_170
  { extern uint8 nt_gfx_hook_bank; nt_gfx_hook_bank = 0; }
  enemy_bg2_tilemap_size = 2048;
  boss_id = 0;
  memset(enemy_data, 0, sizeof enemy_data);                  // ClearEnemyDataAndProcessEnemySet
  if (enemy_population(room_enemy_population_ptr)->n)
    ProcessEnemyTilesets();
  enemy_tile_load_data_write_pos = 0;                        // LoadEnemyTileData (los tiles: la Genesis)
  enemy_tile_vram_src = 0;
  flag_disable_projectile_interaction = 0;
  ClearSpriteObjects();
}

static void LoadEnemyGfxIndexes(const EnemyPopEntryN *P, uint16 j) {  // 0xA08BF3
  EnemyData *E = gEnemyData(j);
  EnemySpawnData *ES = gEnemySpawnData(j);
  const EnemyTilesetN *ET = enemy_tileset(room_enemy_tilesets_ptr);
  uint16 f = 0;
  for (int i = 0; i < ET->n; i++) {
    if (P->enemy_ptr == ET->enemy_def[i]) {
      E->palette_index = ES->palette_index = (ET->vram_dst[i] & 0xF) << 9;
      E->vram_tiles_index = ES->vram_tiles_index = f;
      return;
    }
    f += enemy_def(ET->enemy_def[i])->tile_data_size >> 5;
  }
  E->vram_tiles_index = ES->vram_tiles_index = 0;
  E->palette_index = ES->palette_index = 2560;
}

static void RecordEnemySpawnData(uint16 j) {  // 0xA088D0
  EnemyData *v1 = gEnemyData(j);
  EnemySpawnData *v2 = gEnemySpawnData(j);
  v2->id = v1->enemy_ptr;
  v2->x_pos = v1->x_pos;
  v2->y_pos = v1->y_pos;
  v2->init_param = v1->current_instruction;
  v2->properties = v1->properties;
  v2->extra_properties = v1->extra_properties;
  v2->param_1 = v1->parameter_1;
  v2->param_2 = v1->parameter_2;
  uint16 name_ptr = enemy_def(v1->enemy_ptr)->name_ptr;
  for (int i = 0; i < 6; i++) {                 /* nombre (de depuración) en $B4: palabras 0-4 y 6 (así el original) */
    v2->name[i] = name_ptr ? BANKB_W(kBankB4B, name_ptr + 2 * (i == 5 ? 6 : i)) : 0;
  }
}

void InitializeEnemies(void) {  // 0xA08A9E
  memset(enemy_spawn_data, 0, sizeof enemy_spawn_data);     // memset 7E:7000, 0x2800
  memset(enemy_ram7800, 0, sizeof enemy_ram7800);
  num_enemies_in_room = 0;
  num_enemies_killed_in_room = 0;
  flag_process_all_enemies = 0;
  /* el original borra 144 palabras desde eproj_flags (7E:F380-F49F): eproj_flags, eproj_enemy_header_ptr,
     eproj_killed_enemy_index y el origen de la caída de objetos especiales */
  for (int i = 0; i < 36; i++) {
    eproj_flags[i] = 0;
    eproj_enemy_header_ptr[i] = 0;
  }
  special_death_item_drop_x_origin_pos = 0;
  special_death_item_drop_y_origin_pos = 0;
  for (int j = 0; j < 18; j++)
    eproj_killed_enemy_index[j] = -1;
  const EnemyPopulationN *P = enemy_population(room_enemy_population_ptr);
  if (!P->n)
    return;
  uint16 v5 = 0;
  for (int n = 0; n < P->n; n++) {
    const EnemyPopEntryN *EP = &P->e[n];
    if (!viewer_keep_enemy(EP->enemy_ptr))       /* visor de salas: sin enemigos */
      continue;
    LoadEnemyGfxIndexes(EP, v5);
    const EnemyDefN *ED = enemy_def(EP->enemy_ptr);
    EnemyData *E = gEnemyData(v5);
    E->x_width = ED->x_radius;
    E->y_height = ED->y_radius;
    E->health = ED->health;
    E->layer = ED->layer;
    E->bank = ED->bank;
    E->field_2F = ED->hurt_ai_time;
    if (ED->boss_fight_value)
      boss_id = ED->boss_fight_value;
    E->enemy_ptr = EP->enemy_ptr;
    E->x_pos = EP->x_pos;
    E->y_pos = EP->y_pos;
    E->current_instruction = EP->init_param;
    E->properties = EP->properties;
    E->extra_properties = EP->extra_properties;
    E->parameter_1 = EP->parameter1;
    E->parameter_2 = EP->parameter2;
    E->frame_counter = 0;
    E->timer = 0;
    E->instruction_timer = 1;
    RecordEnemySpawnData(v5);
    cur_enemy_index = v5;
    CallEnemyAi(ED->bank, ED->ai_init);
    E->spritemap_pointer = 0;
    if (E->properties & 0x2000)
      E->spritemap_pointer = (E->extra_properties & 4) ? 0x804F : 0x804D;   // kSpritemap_Nothing_A4 / Extended
    v5 += 64;
  }
  first_free_enemy_index = v5;
  num_enemies_in_room = v5 >> 6;
  num_enemy_deaths_left_to_clear = P->kill_count;
}

uint16 SpawnEnemyN(uint8 db, uint16 k) {  // 0xA09275: población de un solo enemigo en el banco db (16 bytes)
  uint16 varE20 = k;
  uint16 backup = cur_enemy_index;
  int16 v3 = enemy_def(BankW(db, k))->num_parts - 1;
  if (v3 < 0) v3 = 0;
  uint16 varE22 = v3, varE26 = v3;
  uint16 ni = 0;
  for (;;) {
    if (ni >= 2048)
      return 0xffff;
    uint16 v4 = ni;
    int found = 0;
    while (!gEnemyData(v4)->enemy_ptr) {
      if (!varE22) { found = 1; break; }
      --varE22;
      v4 += 64;
      if (v4 >= 2048)
        return 0xffff;
    }
    if (found) break;
    ni += 64;
  }
  for (;;) {
    EnemyData *E = gEnemyData(ni);
    uint16 ep = BankW(db, varE20);
    int v10;
    for (v10 = 0; v10 < 4 && ep != enemy_def_ptr[v10]; v10++) {}
    if (v10 < 4) {
      E->vram_tiles_index = enemy_gfxdata_tiles_index[v10];
      uint16 vp = enemy_gfxdata_vram_ptr[v10];
      E->palette_index = 2 * (uint16)(vp << 8 | vp >> 8);
    } else {
      E->vram_tiles_index = 0;
      E->palette_index = 0;
    }
    const EnemyDefN *ED = enemy_def(ep);
    E->x_width = ED->x_radius;
    E->y_height = ED->y_radius;
    E->health = ED->health;
    E->layer = ED->layer;
    E->bank = ED->bank;
    E->field_2F = ED->hurt_ai_time;
    E->enemy_ptr = ep;
    E->x_pos = BankW(db, varE20 + 2);
    E->y_pos = BankW(db, varE20 + 4);
    E->current_instruction = BankW(db, varE20 + 6);
    E->properties = BankW(db, varE20 + 8);
    E->extra_properties = BankW(db, varE20 + 10);
    E->parameter_1 = BankW(db, varE20 + 12);
    E->parameter_2 = BankW(db, varE20 + 14);
    E->frame_counter = 0;
    E->timer = 0;
    E->ai_var_A = E->ai_var_B = E->ai_var_C = E->ai_var_D = E->ai_var_E = 0;
    E->ai_preinstr = 0;
    E->instruction_timer = 1;
    RecordEnemySpawnData(ni);
    cur_enemy_index = ni;
    if (sign16(ED->ai_init))
      CallEnemyAi(ED->bank, ED->ai_init);
    if (E->properties & 0x2000)
      E->spritemap_pointer = 0x804D;              // kSpritemap_Nothing_A0
    if (!varE26 || !--varE26) {
      cur_enemy_index = backup;
      return ni;
    }
    ni += 64;
    varE20 += 16;
  }
}

/* tiles de los enemigos a la VRAM, de a 2 KB por cuadro (los sube la Genesis con la sala) y al final InitializeEnemies */
void TransferEnemyTilesToVramAndInit(void) {  // 0xA08CD7
  if (!enemy_tile_vram_src) {
    enemy_tile_vram_src = 0x7000;                             /* ADDR16_OF_RAM(*enemy_spawn_data) */
    enemy_tile_vram_dst = 0x6C00;
  }
  uint16 v0 = enemy_tile_vram_src;
  if (v0 == 0xFFFF)
    return;
  if (v0 == 0xFFFE) {
    InitializeEnemies();
    enemy_tile_vram_src = 0xFFFF;
  } else if (v0 == 0x9800) {
    enemy_tile_vram_src = 0xFFFE;
  } else {                                                    /* la cola de VRAM: la vacía el NMI (la Genesis) */
    enemy_tile_vram_src += 2048;
    enemy_tile_vram_dst += 1024;
  }
}

void LoadAndInitializeEnemies(void) {
  LoadEnemies();
  InitializeEnemies();
}

/* ------------------------------------------------------------------ instrucciones ($A0:C26A) */
uint16 EnemyInstrWord(uint8 bank, uint16 a);   /* palabra de una lista de instrucciones (ceres_door.c) */
const uint16 *CallEnemyInstr(uint8 bank, uint16 f, uint16 k, uint16 *pc);

static void ProcessEnemyInstructions(void) {  // 0xA0C26A
  EnemyData *E = gEnemyData(cur_enemy_index);
  if (E->ai_handler_bits & 4)
    return;
  if (E->instruction_timer-- == 1) {
    uint16 pc = E->current_instruction;
    uint16 w;
    while ((w = EnemyInstrWord(E->bank, pc)) & 0x8000) {
      if (!CallEnemyInstr(E->bank, w, cur_enemy_index, &pc))
        return;
    }
    E->instruction_timer = w;
    E->spritemap_pointer = EnemyInstrWord(E->bank, pc + 2);
    E->current_instruction = pc + 4;
    E->extra_properties |= 0x8000;
  } else {
    E->extra_properties &= ~0x8000;
  }
}

/* ------------------------------------------------------------------ colisiones ($A0:9758) */
static void EprojCollHandler(void) {  // 0xA0A143 (disparos de Samus contra el enemigo)
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 7;
  if (!projectile_counter)
    return;
  if (!E->spritemap_pointer || E->spritemap_pointer == 0x804D || (E->properties & 0x400)
      || E->enemy_ptr == 0xDAFF || E->invincibility_timer)
    return;
  for (int pidx = 0; pidx < 5; pidx++) {
    uint16 j = projectile_type[pidx];
    if (j && (j & 0xF00) != 768 && (j & 0xF00) != 1280 && sign16((j & 0xF00) - 1792)) {
      uint16 x = abs16(projectile_x_pos[pidx] - E->x_pos);
      uint16 y = abs16(projectile_y_pos[pidx] - E->y_pos);
      if (x - projectile_x_radius[pidx] < E->x_width && y - projectile_y_radius[pidx] < E->y_height) {   /* como el decomp: resta en int */
        if ((projectile_type[pidx] & 0xF00) == 512) {
          earthquake_timer = 30;
          earthquake_type = 18;
        }
        if ((E->properties & 0x1000) || !(projectile_type[pidx] & 8))
          projectile_dir[pidx] |= 0x10;
        collision_detection_index = pidx;
        CallEnemyAi(E->bank, enemy_def(E->enemy_ptr)->shot_ai);
        return;
      }
    }
  }
}

static void EnemyBombCollHandler(void) {  // 0xA0A236
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 8;
  if (!bomb_counter || !E->spritemap_pointer || E->invincibility_timer || E->enemy_ptr == 0xDAFF)
    return;
  for (int pidx = 5; pidx < 10; pidx++) {
    if (!projectile_type[pidx] || projectile_variables[pidx] ||
        ((projectile_type[pidx] & 0xF00) != 1280 && !(projectile_type[pidx] & 0x8000)))
      continue;
    if (abs16(projectile_x_pos[pidx] - E->x_pos) - projectile_x_radius[pidx] < E->x_width &&
        abs16(projectile_y_pos[pidx] - E->y_pos) - projectile_y_radius[pidx] < E->y_height) {
      collision_detection_index = pidx;
      projectile_dir[pidx] |= 0x10;
      CallEnemyAi(E->bank, enemy_def(E->enemy_ptr)->shot_ai);
      return;
    }
  }
}

static void EnemySamusCollHandler(void) {  // 0xA0A07A
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 9;
  if (!E->spritemap_pointer)
    return;
  if (samus_contact_damage_index) {
    samus_invincibility_timer = 0;
  } else if (samus_invincibility_timer) {
    if (E->enemy_ptr != 0xDAFF)
      return;
    uint16 some_flag = gEnemySpawnData(cur_enemy_index)->some_flag;
    if (some_flag == 0 || some_flag == 8)
      return;
  }
  const EnemyDefN *ED = enemy_def(E->enemy_ptr);
  if (ED->touch_ai == 0x804C || ED->touch_ai == 0x804B)     // nullsub_170 / nullsub_169
    return;
  if (abs16(samus_x_pos - E->x_pos) - samus_x_radius < E->x_width &&
      abs16(samus_y_pos - E->y_pos) - samus_y_radius < E->y_height) {
    if (E->enemy_ptr == 0xDAFF || !E->frozen_timer)
      CallEnemyAi(E->bank, ED->touch_ai);
  }
}

/* --- enemigos de varias cajas (spritemap extendido: cada pieza con su lista de cajas, $A0:9A5A-9E99).
   Spritemap extendido en el banco del enemigo: n, y n entradas {x, y, spritemap, cajas}; cajas: m, y m entradas
   {izq, arriba, der, abajo, función al tocar, función al recibir un disparo}. */

void NormalEnemyTouchAiSkipDeathAnim(void) {  // 0xA0A4A1
  EnemyData *E = gEnemyData(cur_enemy_index);
  const EnemyDefN *ED = enemy_def(E->enemy_ptr);
  if (samus_contact_damage_index == 0) {
    Samus_DealDamage(SuitDamageDivision(ED->damage));
    samus_invincibility_timer = 96;
    samus_knockback_timer = 5;
    knockback_x_dir = (int16)(samus_x_pos - E->x_pos) >= 0;
    return;
  }
  nt_unimpl("NormalEnemyTouchAiSkipDeathAnim (daño de contacto de Samus)");
}

static void EnemySamusCollHandler_Multibox(void) {  // 0xA09A5A
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 6;
  if (!E->spritemap_pointer)
    return;
  uint16 touch_ai = enemy_def(E->enemy_ptr)->touch_ai;
  if (touch_ai == 0x804C || touch_ai == 0x804B)
    return;
  if (samus_contact_damage_index)
    samus_invincibility_timer = 0;
  else if (samus_invincibility_timer)
    return;
  if (!sign16(E->spritemap_pointer))
    return;
  uint16 sr = samus_x_radius + samus_x_pos, sl = samus_x_pos - samus_x_radius;
  uint16 sb = samus_y_radius + samus_y_pos, st = samus_y_pos - samus_y_radius;
  uint16 n = (uint8)BankW(E->bank, E->spritemap_pointer);       /* el original lee un byte */
  uint16 ep = E->spritemap_pointer + 2;
  do {
    uint16 cx = BankW(E->bank, ep) + E->x_pos, cy = BankW(E->bank, ep + 2) + E->y_pos;
    uint16 hp = BankW(E->bank, ep + 6);
    uint16 m = BankW(E->bank, hp);
    for (uint16 hb = hp + 2; m; m--, hb += 12) {
      if ((int16)(BankW(E->bank, hb) + cx - sr) < 0 && (int16)(BankW(E->bank, hb + 4) + cx - sl) >= 0 &&
          (int16)(BankW(E->bank, hb + 2) + cy - sb) < 0 && (int16)(BankW(E->bank, hb + 6) + cy - st) >= 0) {
        CallEnemyAi(E->bank, BankW(E->bank, hb + 8));             // CallHitboxTouch
        return;
      }
    }
    ep += 8;
  } while (--n);
}

static void EprojCollHandler_Multibox(void) {  // 0xA09B7F
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 3;
  if (!projectile_counter || !E->spritemap_pointer || E->spritemap_pointer == 0x804D)
    return;
  uint16 shot_ai = enemy_def(E->enemy_ptr)->shot_ai;
  if (shot_ai == 0x804C || shot_ai == 0x804B)
    return;
  if ((E->properties & 0x400) || E->invincibility_timer || E->enemy_ptr == 0xDAFF)
    return;
  for (int pidx = 0; pidx < 5; pidx++) {
    uint16 v4 = projectile_type[pidx];
    if (!(v4 && (v4 & 0xF00) != 768 && (v4 & 0xF00) != 1280 && sign16((v4 & 0xF00) - 1792)))
      continue;
    uint16 n = (uint8)BankW(E->bank, E->spritemap_pointer);    /* aquí el original lee un byte */
    for (uint16 ep = E->spritemap_pointer + 2; n; n--, ep += 8) {
      uint16 cx = BankW(E->bank, ep) + E->x_pos, cy = BankW(E->bank, ep + 2) + E->y_pos;
      uint16 hp = BankW(E->bank, ep + 6);
      uint16 m = BankW(E->bank, hp);
      for (uint16 hb = hp + 2; m; m--, hb += 12) {
        if ((int16)(projectile_x_radius[pidx] + projectile_x_pos[pidx] - (cx + BankW(E->bank, hb))) >= 0 &&
            (int16)(projectile_x_pos[pidx] - projectile_x_radius[pidx] - (cx + BankW(E->bank, hb + 4))) < 0 &&
            (int16)(projectile_y_radius[pidx] + projectile_y_pos[pidx] - (cy + BankW(E->bank, hb + 2))) >= 0 &&
            (int16)(projectile_y_pos[pidx] - projectile_y_radius[pidx] - (cy + BankW(E->bank, hb + 6))) < 0) {
          if ((projectile_type[pidx] & 0xF00) == 512) {
            earthquake_timer = 30;
            earthquake_type = 18;
          }
          if ((E->properties & 0x1000) || !(projectile_type[pidx] & 8))
            projectile_dir[pidx] |= 0x10;
          collision_detection_index = pidx;
          CallEnemyAi(E->bank, BankW(E->bank, hb + 10));          // CallHitboxShot
          return;
        }
      }
    }
  }
}

static void EnemyBombCollHandler_Multibox(void) {  // 0xA09D23
  EnemyData *E = gEnemyData(cur_enemy_index);
  enemy_processing_stage = 4;
  if (!E->spritemap_pointer || (E->properties & 0x400) || E->invincibility_timer)
    return;
  uint16 shot_ai = enemy_def(E->enemy_ptr)->shot_ai;
  if (shot_ai == 0x804C || shot_ai == 0x804B || !bomb_counter)
    return;
  for (int pidx = 5; pidx != 10; pidx++) {
    if (!projectile_x_pos[pidx])
      continue;
    uint16 v4 = projectile_type[pidx];
    if (!(v4 && (v4 & 0xF00) == 1280 && !projectile_variables[pidx]))
      continue;
    if (!sign16(E->spritemap_pointer))
      nt_unimpl("EnemyBombCollHandler_Multibox (Unreachable)");
    uint16 n = BankW(E->bank, E->spritemap_pointer);          /* aquí el original lee una palabra (GET_WORD) */
    for (uint16 ep = E->spritemap_pointer + 2; n; n--, ep += 8) {
      uint16 cx = BankW(E->bank, ep) + E->x_pos, cy = BankW(E->bank, ep + 2) + E->y_pos;
      uint16 hp = BankW(E->bank, ep + 6);
      uint16 m = BankW(E->bank, hp);
      for (uint16 hb = hp + 2; m; m--, hb += 12) {
        if ((int16)(projectile_x_radius[pidx] + projectile_x_pos[pidx] - (cx + BankW(E->bank, hb))) >= 0 &&
            (int16)(projectile_x_pos[pidx] - projectile_x_radius[pidx] - (cx + BankW(E->bank, hb + 4))) < 0 &&
            (int16)(projectile_y_radius[pidx] + projectile_y_pos[pidx] - (cy + BankW(E->bank, hb + 2))) >= 0 &&
            (int16)(projectile_y_pos[pidx] - projectile_y_radius[pidx] - (cy + BankW(E->bank, hb + 6))) < 0) {
          projectile_dir[pidx] |= 0x10;
          collision_detection_index = pidx;
          CallEnemyAi(E->bank, BankW(E->bank, hb + 10));         // CallHitboxShot
          return;
        }
      }
    }
  }
}

static void EnemyCollisionHandler(void) {  // 0xA09758
  if (gEnemyData(cur_enemy_index)->extra_properties & 4) {
    EprojCollHandler_Multibox();
    EnemyBombCollHandler_Multibox();
    EnemySamusCollHandler_Multibox();
    return;
  }
  EprojCollHandler();
  EnemyBombCollHandler();
  EnemySamusCollHandler();
}

void SamusProjectileInteractionHandler(void) {  // 0xA09785 (los disparos contra Samus: bombas que la empujan)
  enemy_processing_stage = 10;
  if (flag_disable_projectile_interaction)
    return;
  int num_colls_to_check = 5;
  if (bomb_counter)
    num_colls_to_check = 10;
  else if (!projectile_counter)
    return;
  if (projectile_invincibility_timer || samus_contact_damage_index)
    return;
  for (int pidx = 0; pidx != num_colls_to_check; pidx++) {
    collision_detection_index = pidx;
    if (!projectile_damage[pidx] || sign16(projectile_type[pidx]) || !sign16((projectile_type[pidx] & 0xF00) - 1792))
      continue;
    if (projectile_dir[pidx] & 0x10)
      continue;
    if (abs16(projectile_x_pos[pidx] - samus_x_pos) - projectile_x_radius[pidx] < samus_x_radius &&
        abs16(projectile_y_pos[pidx] - samus_y_pos) - projectile_y_radius[pidx] < samus_y_radius) {
      if ((projectile_type[pidx] & 0xFF00) != 768 && (projectile_type[pidx] & 0xFF00) != 1280) {
        nt_unimpl("SamusProjectileInteractionHandler (otro proyectil contra Samus: assert(0) en el decomp)");
        return;
      }
      if (projectile_variables[pidx] == 8) {
        bomb_jump_dir = (samus_x_pos == projectile_x_pos[pidx]) ? 2 :
            (int16)(samus_x_pos - projectile_x_pos[pidx]) < 0 ? 1 : 3;
      }
    }
  }
}

/* ------------------------------------------------------------------ cuadro de los enemigos ($A0:8FD4) */
static uint16 EnemyWithNormalSpritesIsOffScreen(void) {  // 0xA0ADE7
  EnemyData *E = gEnemyData(cur_enemy_index);
  return (int16)(E->x_width + E->x_pos - layer1_x_pos) < 0 || (int16)(E->x_width + layer1_x_pos + 256 - E->x_pos) < 0 ||
         (int16)(E->y_pos + 8 - layer1_y_pos) < 0 || (int16)(layer1_y_pos + 248 - E->y_pos) < 0;
}

static void DrawOneEnemy(void) {  // 0xA09423
  uint16 l = gEnemyData(cur_enemy_index)->layer & 7;
  enemy_drawing_queue[(uint16)(enemy_drawing_queue_sizes[l] + kEnemyLayerToQueuePtr[l] - kEnemyLayerToQueue_0) >> 1] = cur_enemy_index;
  enemy_drawing_queue_sizes[l] += 2;
}

/* ------------------------------------------------------------------ objetos de sprite ($B4:BC26) */
static inline uint16 B4W(uint16 a) { return kBankB4_Sprite[(uint16)(a - kBankB4_Sprite_LO)]; }
/* explosiones y polvo sueltos: una lista de instrucciones (temporizador, spritemap) por objeto.
   En la Genesis los recorridos de los 32 objetos se saltan mientras no haya ninguno (casi siempre): sprite_objs_live
   se pone al crear uno y se baja cuando el recorrido no encuentra ninguno. En el PC no (la comprobación copia la RAM
   del SNES en las cargas de sala). */
#ifdef NT_SSF2
static uint8 sprite_objs_live = 1;
#define SPRITE_OBJS_LIVE(v) (sprite_objs_live = (v))
#define SPRITE_OBJS_NONE() (!sprite_objs_live)
#else
#define SPRITE_OBJS_LIVE(v) ((void)0)
#define SPRITE_OBJS_NONE() 0
#endif
uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal) {  // 0xB4BC26
  SPRITE_OBJS_LIVE(1);
  int i = 31;
  while (sprite_instr_list_ptrs[i])
    if (--i < 0)
      return 0xFFFF;
  sprite_x_subpos[i] = 0;
  sprite_y_subpos[i] = 0;
  sprite_disable_flag[i] = 0;
  sprite_x_pos[i] = x;
  sprite_y_pos[i] = y;
  sprite_palettes[i] = pal;
  uint16 l = B4W(0xBDA8 + 2 * ilist);
  sprite_instr_list_ptrs[i] = l;
  sprite_instr_timer[i] = B4W(l);
  return 2 * i;
}

static void SpriteObjectInstr(int i, uint16 f) {
  switch (f) {
  case 0xBCF0:                                    // SpriteObject_Instr_RepeatLast
    sprite_instr_list_ptrs[i] -= 4;
    sprite_instr_timer[i] = 0x7FFF;
    break;
  case 0xBD07: sprite_instr_list_ptrs[i] = 0; break;   // SpriteObject_Instr_Terminate
  case 0xBD12: {                                  // SpriteObject_Instr_Goto
    uint16 l = B4W(sprite_instr_list_ptrs[i] + 2);
    sprite_instr_list_ptrs[i] = l;
    sprite_instr_timer[i] = B4W(l);
    break;
  }
  default: nt_unimpl_v("SpriteObjectInstr (instrucción desconocida)", f); break;
  }
}

static void HandleSpriteObjects(void) {  // 0xB4BC82
  if (debug_time_frozen_for_enemies | time_is_frozen_flag)
    return;
  if (SPRITE_OBJS_NONE()) {
    sprite_object_index = 0xFFFE;
    return;
  }
  uint8 live = 0;
  for (int i = 31; i >= 0; i--) {
    sprite_object_index = 2 * i;
    if (!sprite_instr_list_ptrs[i])
      continue;
    live = 1;
    if (sprite_disable_flag[i] & 1)
      continue;
    uint16 t = sprite_instr_timer[i];
    if (!sign16(t)) {
      sprite_instr_timer[i] = t - 1;
      if (t != 1)
        continue;
      uint16 l = sprite_instr_list_ptrs[i] + 4;
      sprite_instr_list_ptrs[i] = l;
      t = B4W(l);
      if (!sign16(t)) {
        sprite_instr_timer[i] = t;
        continue;
      }
    }
    SpriteObjectInstr(i, t);
  }
  sprite_object_index = 0xFFFE;
  if (!live) SPRITE_OBJS_LIVE(0);
}

void DrawSpriteObjSpritemap(uint16 spritemap, uint16 x, uint16 y, uint16 pal, uint16 tiles);   /* capa Genesis */

void DrawSpriteObjects(void) {  // 0xB4BD32
  if (SPRITE_OBJS_NONE())
    return;
  for (int i = 31; i >= 0; i--) {
    if (!sprite_instr_list_ptrs[i])
      continue;
    uint16 x = sprite_x_pos[i] - layer1_x_pos;
    int16 y = sprite_y_pos[i] - layer1_y_pos;
    if ((int16)(x + 16) >= 0 && sign16(x - 272) && y >= 0 && sign16(y - 272))
      DrawSpriteObjSpritemap(B4W(sprite_instr_list_ptrs[i] + 2), x, y, sprite_palettes[i] & 0xE00,
                             sprite_palettes[i] & 0x1FF);
  }
}

void EnemyMain(void) {  // 0xA08FD4
  if (first_free_enemy_index) {
    if (enemy_index_to_shake != 0xFFFF) {
      gEnemyData(enemy_index_to_shake)->shake_timer = 64;
      enemy_index_to_shake = -1;
    }
    for (int ai = 0; ; ai++) {
      uint16 v1 = active_enemy_indexes[ai];
      if (v1 == 0xFFFF)
        break;
      cur_enemy_index = v1;
      EnemyData *E = gEnemyData(cur_enemy_index);
      bool skip_ai = false;
      if (!(E->properties & 0x400)) {
        if (E->invincibility_timer) {
          --E->invincibility_timer;
        } else if (!debug_disable_sprite_interact) {
          if (!(debug_time_frozen_for_enemies | time_is_frozen_flag)) {
            EnemyCollisionHandler();
            if (!E->enemy_ptr)
              goto LABEL_32;
          }
          if (E->extra_properties & 1)
            skip_ai = true;
        }
      }
      if (!skip_ai) {
        if (!(debug_time_frozen_for_enemies | time_is_frozen_flag)) {
          int v6 = 0;
          uint16 bits = E->ai_handler_bits;
          if (bits) {
            uint8 c;
            do { ++v6; c = bits & 1; bits >>= 1; } while (!c);
          }
          if (v6 > 4) nt_unimpl("EnemyMain (bit de IA fuera de tabla)");
          CallEnemyAi(E->bank, enemy_def(E->enemy_ptr)->ai_bits[v6 > 4 ? 0 : v6]);
          ++E->frame_counter;
          if (E->properties & 0x2000) {
            enemy_processing_stage = 2;
            ProcessEnemyInstructions();
          }
        } else if (enemy_def(E->enemy_ptr)->ai_bits[4]) {
          nt_unimpl("EnemyMain (IA con el tiempo congelado)");
        }
      }
      if ((E->extra_properties & 1) && (E->flash_timer == 1 || E->frozen_timer == 1))
        nt_unimpl("EnemyDeathAnimation");
      if (((E->extra_properties & 4) || !EnemyWithNormalSpritesIsOffScreen()) && !(E->properties & 0x300))
        DrawOneEnemy();
LABEL_32:
      if (E->flash_timer && !(debug_time_frozen_for_enemies | time_is_frozen_flag)) {
        if (sign16(--E->flash_timer - 8))
          E->ai_handler_bits &= ~2;
      }
    }
  }
  HandleSpriteObjects();
  random_enemy_counter++;
  for (int i = 0; i < 4; i++)
    enemy_index_colliding_dirs[i] = -1;
}

/* ------------------------------------------------------------------ dibujo ($A0:944A) */
void DrawEnemySpritemap(uint8 bank, uint16 spritemap, uint16 x, uint16 y, uint16 palette, uint16 tiles); /* capa Genesis */
uint8 DrawEnemyExtSpritemap(uint8 bank, uint16 spritemap, uint16 x, uint16 y, uint16 palette, uint16 tiles);

static void WriteEnemyOams(void) {  // 0xA0944A
  EnemyData *E = gEnemyData(cur_enemy_index);
  EnemySpawnData *ES = gEnemySpawnData(cur_enemy_index);
  uint16 x2 = ES->xpos2 + E->x_pos - layer1_x_pos;
  uint16 y2 = ES->ypos2 + E->y_pos - layer1_y_pos;
  if (E->shake_timer) {
    x2 += (E->frame_counter & 2) ? -1 : 1;
    E->shake_timer--;
  }
  uint16 palette_index;
  if (E->flash_timer && (random_enemy_counter & 2))
    palette_index = 0;
  else if (E->frozen_timer && (E->frozen_timer >= 0x5A || (E->frozen_timer & 2)))
    palette_index = 3072;
  else
    palette_index = E->palette_index;
  if (E->extra_properties & 4) {
    /* capa de la Genesis: si tiene el spritemap extendido entero ya preparado (Ridley), lo dibuja de una vez */
    if (DrawEnemyExtSpritemap(E->bank, E->spritemap_pointer, x2, y2, palette_index, E->vram_tiles_index))
      return;
    uint16 n = (uint8)BankW(E->bank, E->spritemap_pointer);       /* el original lee un byte */
    uint16 ep = E->spritemap_pointer + 2;
    do {
      uint16 sm = BankW(E->bank, ep + 4);
      uint16 x = x2 + BankW(E->bank, ep), y = y2 + BankW(E->bank, ep + 2);
      if (BankW(E->bank, sm) == 0xFFFE) {
        if (E->extra_properties & 0x8000)
          nt_unimpl("WriteEnemyOams (tilemap extendido)");   /* ProcessExtendedTilemap */
      } else if (!((x + 128) & 0xFE00) && !((y + 128) & 0xFE00)) {
        DrawEnemySpritemap(E->bank, sm, x, y, palette_index, E->vram_tiles_index);
      }
      ep += 8;
    } while (--n);
  } else {
    enemy_processing_stage = 1;
    DrawEnemySpritemap(E->bank, E->spritemap_pointer, x2, y2, palette_index, E->vram_tiles_index);
  }
}

void DrawEnemyQueue(uint16 phase) {   /* la parte de DrawSamusEnemiesAndProjectiles ($A0:884D) de cada capa */
  uint16 n = enemy_drawing_queue_sizes[phase];
  if (!n)
    return;
  uint16 base = (uint16)(kEnemyLayerToQueuePtr[phase] - kEnemyLayerToQueue_0) >> 1;
  enemy_drawing_queue_sizes[phase] = 0;
  for (uint16 v1 = 0; v1 != n; v1 += 2) {
    cur_enemy_index = enemy_drawing_queue[base + (v1 >> 1)];
    enemy_drawing_queue[base + (v1 >> 1)] = 0;
    WriteEnemyOams();
  }
}

/* ------------------------------------------------------------------ proyectiles enemigos contra Samus y sus disparos */
static void HandleEprojCollWithSamus(uint16 k) {  // 0xA09923
  samus_invincibility_timer = 96;
  samus_knockback_timer = 5;
  int v3 = k >> 1;
  uint16 v1 = B86W(eproj_id[v3] + 10);            /* EprojDef.hit_instruction_list */
  if (v1) {
    eproj_instr_list_ptr[v3] = v1;
    eproj_instr_timers[v3] = 1;
  }
  if (!(eproj_properties[v3] & 0x4000))
    eproj_id[v3] = 0;
  Samus_DealDamage(SuitDamageDivision(eproj_properties[v3] & 0xFFF));
  knockback_x_dir = (int16)(samus_x_pos - eproj_x_pos[v3]) >= 0;
}

void EprojSamusCollDetect(void) {  // 0xA09894
  enemy_processing_stage = 11;
  if (samus_invincibility_timer || samus_contact_damage_index)
    return;
  for (int i = 17; i >= 0; i--) {
    if (eproj_id[i] && !(eproj_properties[i] & 0x2000) && eproj_radius[i]) {
      if (abs16(samus_x_pos - eproj_x_pos[i]) - samus_x_radius < LO8(eproj_radius[i]) &&
          abs16(samus_y_pos - eproj_y_pos[i]) - samus_y_radius < HI8(eproj_radius[i])) {
        collision_detection_index = i * 2;
        HandleEprojCollWithSamus(i * 2);
      }
    }
  }
}

uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal);
void QueueSfx1_Max6(uint16 a);
static void HandleEprojCollWithProj(uint16 k, uint16 j) {  // 0xA099F9
  int i = j >> 1;
  if ((projectile_type[i] & 8) == 0)
    projectile_dir[i] |= 0x10;
  if (eproj_flags[k >> 1] == 1) {
    int v4 = k >> 1;                                /* (el decomp lee la posición del disparo con el índice del proyectil enemigo) */
    CreateSpriteAtPos(projectile_x_pos[v4], projectile_y_pos[v4], 6, 0);
    QueueSfx1_Max6(0x3D);
  } else {
    int j2 = k >> 1;
    eproj_G[j2] = projectile_type[i];
    eproj_instr_list_ptr[j2] = B86W(eproj_id[j2] + 12);   /* EprojDef.shot_instruction_list */
    eproj_instr_timers[j2] = 1;
    eproj_pre_instr[j2] = FUNC16(EprojPreInstr_nullsub_83);
    eproj_properties[j2] &= 0xFFF;
  }
}

void EprojProjCollDet(void) {  // 0xA0996C
  enemy_processing_stage = 12;
  if (!projectile_counter)
    return;
  for (int i = 17; i >= 0; i--) {
    if (!eproj_id[i] || !(eproj_properties[i] & 0x8000))
      continue;
    for (int j = 0; j < 5; j++) {
      if (eproj_flags[i] == 2)
        break;
      uint16 v4 = projectile_type[j];
      if (v4 && (v4 & 0xF00) != 768 && (v4 & 0xF00) != 1280 && sign16((v4 & 0xF00) - 1792)) {
        if ((eproj_x_pos[i] & 0xFFE0) == (projectile_x_pos[j] & 0xFFE0) &&
            (eproj_y_pos[i] & 0xFFE0) == (projectile_y_pos[j] & 0xFFE0))
          HandleEprojCollWithProj(i * 2, j * 2);
      }
    }
  }
}

void ProcessEnemyPowerBombInteraction(void) {  // 0xA0A306
  enemy_processing_stage = 5;
  if (HI8(power_bomb_explosion_radius))
    nt_unimpl("ProcessEnemyPowerBombInteraction (superbomba)");
}
