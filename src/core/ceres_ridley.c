/* MegaMetroid — motor nativo: Ridley de Ceres (enemigo E13F, banco $A6, docs/31 y docs/33).
 *
 * La pelea completa de la estación Ceres: Ridley aparece, vuela, escupe bolas de fuego, embiste con la cola, y
 * cuando Samus baja de 30 de energía (o le pegan 100 disparos) toma la cápsula de la bebé y se va (la huida en
 * modo 7 la dibuja la capa de la Genesis con cuadros ya preparados en el PC).
 *
 * Escrito a mano desde snesrev/sm sm_a6.c: CeresRidley_*, las funciones de Ridley que usa la rama de Ceres
 * (Ridley_Func_*: vuelo, cola, colisiones, dibujo), sus instrucciones (Ridley_Instr_*), la bebé (BabyMetroid_*).
 * Lo que solo usa el Ridley de Norfair queda fuera (llama a nt_unimpl si se llegara a usar).
 *
 * Variables: las de Ridley viven en la RAM extra de enemigos (7E:7800-97FF, ERAM); la cola usa tilemap_stuff
 * (7E:2000), como en el SNES. Las tablas del banco $A6 se leen de kBankA6B (A6W: la palabra, de dos bytes). */
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "nmath.h"

/* variables de Ridley del enemigo 0: ridley_var_00-1F (7E:7800), _20-3F (7E:8000), _40- (7E:8800) */
#define RV(n) ERAM((n) < 0x20 ? 0x7800 + 2 * (n) : (n) < 0x40 ? 0x8000 + 2 * ((n) - 0x20) : 0x8800 + 2 * ((n) - 0x40))
#define TS tilemap_stuff
/* ridley_var_F (cry_var_F) es el campo +3A del enemigo: ai_preinstr en EnemyData */

/* capa de la Genesis */
void DrawA6Spritemap(uint16 spritemap, uint16 x, uint16 y, uint16 attr);         /* sub_A6DC13 (coordenadas de la sala) */
void DrawA6SpritemapScreen(uint16 spritemap, uint16 x, uint16 y, uint16 chr);    /* DrawSpritemap $81:879F */
void RidleyQueueTiles(uint16 src1, uint16 src2, uint16 dst1, uint16 dst2, uint16 size); /* cola de VRAM de Ridley */
void RidleyGetawayFrame(void);                                                     /* huida en modo 7 (cuadro) */
void SpawnEprojWithRoomGfx(uint16 j, uint16 a);
uint16 EnemyInstrWord(uint8 bank, uint16 a);

static void WriteColorsToPalette(uint16 dst, uint16 src, uint16 n) {  // 0x82D... (dst en bytes de palette_buffer)
  uint16 d = dst >> 1;
  for (uint16 i = 0; i < n; i++)
    palette_buffer[d + i] = A6W(src + 2 * i);
  nt_pal_dirty(d, d + n - 1);
}

/* ------------------------------------------------------------------ bebé metroide (cápsula) */
static uint16 CallBabyMetroidInstr(uint16 f, uint16 k) {
  switch (f) {
  case 0xBFC9:                                  // BabyMetroid_Instr_2
    if (!RV(0x46) && (random_number & 1))
      return A6W(k);
    QueueSfx3_Max6(0x24);
    return k + 2;
  case 0xBFE1:                                  // BabyMetroid_Instr_3
    WriteColorsToPalette(0x162, A6W(k), 15);
    return k + 2;
  case 0xBFF2: return RV(0x46) ? A6W(k) : k + 2;   // BabyMetroid_Instr_1
  case 0xBFF8: return A6W(k);                      // BabyMetroid_Goto
  default: nt_unimpl("CallBabyMetroidInstr"); return k + 2;
  }
}

static int BabyMetroid_DBCB_DoubleRetEx(void) {   // estado en ridley_var_03 (ip) / ridley_var_04 (timer)
  if (!(RV(0x03) & 0x8000))
    return -1;
  uint16 v2 = RV(0x03);
  uint16 w0 = A6W(v2);
  if (!sign16(w0)) {
    if (RV(0x04) != w0) {
      RV(0x04)++;
      return A6W(v2 + 2);
    }
    v2 += 4;
  }
  for (;;) {
    w0 = A6W(v2);
    if (!sign16(w0))
      break;
    v2 = CallBabyMetroidInstr(w0, v2 + 2);
  }
  RV(0x04) = 1;
  RV(0x03) = v2;
  return A6W(v2 + 2);
}

static void DrawBabyMetroid_0(void) {  // 0xA6BF1A
  int v0 = BabyMetroid_DBCB_DoubleRetEx();
  if (v0 >= 0)
    DrawA6Spritemap(v0, RV(0x42), RV(0x44), 0);
}

/* ------------------------------------------------------------------ utilidades de Ridley */
static void DisableMinimapAndMarkBossRoomAsExplored(void) {  // 0x90A7E2
  debug_disable_minimap = 1;
  for (int i = 0; i < 5; i++) {
    hud_tilemap[i + 26] = 11295;
    hud_tilemap[i + 58] = 11295;
    hud_tilemap[i + 90] = 11295;
  }
  /* g_stru_90A83A no tiene la entrada del jefe 1 (Ridley de Ceres): no se marca nada en el mapa */
  if (boss_id == 3 || boss_id == 5 || boss_id == 6 || boss_id == 7 || boss_id == 8 || boss_id == 10)
    nt_unimpl("DisableMinimapAndMarkBossRoomAsExplored (mapa del jefe)");
}

typedef struct { uint16 x, y, w, h; } RectN;

static uint8 Shitroid_Func_2(RectN r) {  // 0xA9EF06 (con el enemigo 0): 1 si el punto está lejos
  EnemyData *E = gEnemyData(0);
  uint16 R26 = E->x_width + r.w + 1;
  uint16 v2 = abs16n(r.x - E->x_pos);
  if (v2 < R26) {
    R26 = E->y_height + r.h + 1;
    return abs16n(r.y - E->y_pos) >= R26;
  }
  return 1;
}

static void Ridley_Func_99(uint16 a) {  // 0xA6D467
  EnemyData *E = gEnemyData(0);
  E->current_instruction = a;
  E->instruction_timer = 1;
  E->timer = 0;
}

static void Ridley_Func_93(uint16 j, uint16 k) {  // 0xA6D3B4: 7 palabras a tilemap_stuff, cada 10 palabras
  uint16 d = (uint16)(j - 0x2000) >> 1;
  for (int i = 0; i < 7; i++, d += 10, k += 2)   /* do {...} while (--v3) con v3 = 7: siete vueltas */
    TS[d] = A6W(k);
}

static void Ridley_Func_D3DC(uint16 a) {  // 0xA6D3DC
  TS[16] = a; TS[26] = a; TS[36] = a; TS[46] = a; TS[56] = a; TS[66] = a; TS[76] = a;
}

static void Ridley_Func_92(void) {  // 0xA6D2D6
  TS[10] = 1;
  TS[11] = 16368;
  TS[12] = 16448;
  TS[13] = -1;
  TS[14] = -1;
  TS[9] = 240;
  TS[15] = 16;
  TS[17] = 17; TS[27] = 17; TS[37] = 17; TS[47] = 17; TS[57] = 17; TS[67] = 17; TS[77] = 17;
  Ridley_Func_93(0x2024, 0xD36E);
  Ridley_Func_93(0x2026, 0xD37C);
  Ridley_Func_93(0x202A, 0xD38A);
  Ridley_Func_93(0x202C, 0xD398);
  Ridley_Func_93(0x202E, 0xD3A6);
  Ridley_Func_D3DC(0);
}

static void Ridley_Func_96(void) {  // 0xA6D3F9
  Ridley_Func_93(0x2026, 0xD37C);
  for (int v = 0; v < 70; v += 10) {             /* v0 = 7 vueltas */
    TS[v + 21] = 0x8000 - TS[v + 21];
    TS[v + 18] |= 0x8000;
  }
}

static void Ridley_Func_117(void) {  // 0xA6D9FD
  RV(0x05) = 0xDA71;
  RV(0x06) = 1;
}

/* ------------------------------------------------------------------ vuelo */
static const uint8 g_byte_A6D61F[16] = { 0x10, 0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
static const uint8 g_byte_A6D712[16] = { 0x10, 0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

static void Ridley_Func_105(uint16 r18, uint16 r24, uint16 r26) {  // 0xA6D5A9 (enemigo 0)
  EnemyData *E = gEnemyData(0);
  int16 v2 = E->x_pos - r18;
  if (!v2)
    return;
  if (v2 >= 0) {
    uint16 r22 = SnesDivideN(v2, r24);
    if (!r22) r22 = 1;
    int16 b = E->ai_var_B;
    if (b >= 0)
      b = (uint16)(b - r26 - 8 - r22);
    int16 v15 = b - r22;
    if (sign16(v15 + 1280)) v15 = -1280;
    E->ai_var_B = v15;
  } else {
    uint16 r22 = SnesDivideN(r18 - E->x_pos, r24);
    if (!r22) r22 = 1;
    int16 b = E->ai_var_B;
    if (b < 0)
      b = (uint16)(r26 + b + 8 + r22);
    int16 v9 = r22 + b;
    if (!sign16(v9 - 1280)) v9 = 1280;
    E->ai_var_B = v9;
  }
}

static void Ridley_Func_104(uint16 j, uint16 a, uint16 r18, uint16 r20) {  // 0xA6D526 (enemigo 0)
  uint16 R26 = a;
  uint16 r24 = g_byte_A6D61F[j];
  Ridley_Func_105(r18, r24, a);
  EnemyData *E = gEnemyData(0);
  int16 v4 = E->y_pos - r20;
  if (!v4)
    return;
  if (v4 >= 0) {
    uint16 r22 = SnesDivideN(v4, r24);
    if (!r22) r22 = 1;
    int16 c = E->ai_var_C;
    if (c >= 0)
      c = (uint16)(c - R26 - 8 - r22);
    int16 v17 = c - r22;
    if (sign16(v17 + 1280)) v17 = -1280;
    E->ai_var_C = v17;
  } else {
    uint16 r22 = SnesDivideN(r20 - E->y_pos, r24);
    if (!r22) r22 = 1;
    int16 c = E->ai_var_C;
    if (c < 0)
      c = (uint16)(R26 + c + 8 + r22);
    int16 v11 = r22 + c;
    if (!sign16(v11 - 1280)) v11 = 1280;
    E->ai_var_C = v11;
  }
}
static void Ridley_Func_104_0(uint16 j, uint16 r18, uint16 r20) { Ridley_Func_104(j, 0, r18, r20); }  // 0xA6D523

static void Ridley_Func_107(uint16 r18, uint16 r24) {  // 0xA6D6A6 (enemigo 0)
  EnemyData *E = gEnemyData(0);
  int16 v2 = E->x_pos - r18;
  if (!v2)
    return;
  if (v2 >= 0) {
    uint16 q = SnesDivideN(v2, r24);
    if (!q) q = 1;
    int16 s = E->ai_var_B;
    if (s >= 0)
      s -= 2 * q;
    s -= q;
    E->ai_var_B = sign16(s + 1280) ? -1280 : s;
  } else {
    uint16 q = SnesDivideN(r18 - E->x_pos, r24);
    if (!q) q = 1;
    int16 s = E->ai_var_B;
    if (s < 0)
      s += 2 * q;
    s += q;
    E->ai_var_B = !sign16(s - 1280) ? 1280 : s;
  }
}

static void Ridley_Func_106(uint16 j, uint16 r18, uint16 r20) {  // 0xA6D62F (enemigo 0)
  uint16 r24 = g_byte_A6D712[j];
  Ridley_Func_107(r18, r24);
  EnemyData *E = gEnemyData(0);
  int16 v3 = E->y_pos - r20;
  if (!v3)
    return;
  if (v3 >= 0) {
    uint16 r22 = SnesDivideN(v3, r24);
    if (!r22) r22 = 1;
    int16 c = E->ai_var_C;
    if (c >= 0)
      c = c - r22 - r22;
    int16 v9 = c - r22;
    if (sign16(v9 + 1280)) v9 = -1280;
    E->ai_var_C = v9;
  } else {
    uint16 r22 = SnesDivideN(r20 - E->y_pos, r24);
    if (!r22) r22 = 1;
    int16 c = E->ai_var_C;
    if (c < 0)
      c += r22 + r22;
    int16 v6 = r22 + c;
    if (!sign16(v6 - 1280)) v6 = 1280;
    E->ai_var_C = v6;
  }
}

static void Ridley_Func_111(uint16 r18, uint16 r20, uint16 r22) {  // 0xA6D800
  EnemyData *E = gEnemyData(0);
  if (r22 != RV(0x0B)) {
    if ((int16)(r22 - RV(0x0B)) >= 0) {
      uint16 v2 = RV(0x0B) + 32;
      if (!sign16(v2 - r22)) v2 = r22;
      RV(0x0B) = v2;
    } else {
      uint16 v1 = RV(0x0B) - 32;
      if (sign16(v1 - r22)) v1 = r22;
      RV(0x0B) = v1;
    }
  }
  uint16 v3 = RV(0x0A) + r18;
  if (r18 & 0x8000) {
    if (sign16(v3 - r20)) v3 = r20;
  } else {
    if (!sign16(v3 - r20)) v3 = r20;
  }
  RV(0x0A) = v3;
  uint16 ang = HI8(v3);
  E->ai_var_B = Math_MultBySin(RV(0x0B), ang);
  E->ai_var_C = Math_MultByCos(RV(0x0B), ang);
}

static void Ridley_Func_113(uint16 k) {  // 0xA6D914
  if (area_index != 2) {
    EnemyData *E = gEnemyData(k);
    uint16 r18 = abs16n(E->ai_var_B);
    uint16 v2 = abs16n(E->ai_var_C);
    if (v2 < r18) v2 = r18;
    if (v2 >= 0x280) {
      earthquake_type = (area_index == 2) ? 24 : 33;
      earthquake_timer = 12;
    }
  }
}

static void Ridley_Func_112(void) {  // 0xA6D86B: mueve al enemigo actual dentro de los límites de Ridley
  RV(0x1F) = 0;
  uint16 v1 = cur_enemy_index;
  EnemyData *E = gEnemyData(cur_enemy_index);
  uint16 b = E->ai_var_B;
  uint16 c = LO8(b) + HI8(E->x_subpos);
  E->x_subpos = (E->x_subpos & 0xFF) | (c & 0xFF) << 8;
  uint16 nx = E->x_pos + (int8)(b >> 8) + (c >> 8);
  if ((int16)(nx - RV(0x22)) >= 0) {
    if ((int16)(nx - RV(0x23)) >= 0) {
      E->ai_var_B = 0;
      RV(0x1F) = 2;
      nx = RV(0x23);
    }
    E->x_pos = nx;
  } else {
    Ridley_Func_113(v1);
    E->x_pos = RV(0x22);
    E->ai_var_B = 0;
    RV(0x1F) = 1;
  }
  uint16 cv = E->ai_var_C;
  c = LO8(cv) + HI8(E->y_subpos);
  E->y_subpos = (E->y_subpos & 0xFF) | (c & 0xFF) << 8;
  uint16 ny = E->y_pos + (int8)(cv >> 8) + (c >> 8);
  if ((int16)(ny - RV(0x20)) >= 0) {
    if ((int16)(ny - RV(0x21)) >= 0) {
      E->ai_var_C = 0;
      RV(0x1F) = 8;
      ny = RV(0x21);
    }
    E->y_pos = ny;
  } else {
    E->y_pos = RV(0x20);
    E->ai_var_C = 0;
    RV(0x1F) = 4;
  }
}

static void Ridley_Func_114(void) {  // 0xA6D955
  EnemyData *E = gEnemyData(0);
  uint16 v10 = RV(0x10), v2;
  /* *(int16*)((uint8*)&enemy_ptr + 1): la palabra formada por el byte alto de enemy_ptr y el byte bajo de x_pos */
  int16 w = (int16)(HI8(E->enemy_ptr) | LO8(E->x_pos) << 8);
  if (!v10) {
    if (w < 0)
      return;
    v2 = 0xE6F0;
  } else if (v10 != 1 && w < 0) {
    v2 = 0xE706;
  } else {
    return;
  }
  E->current_instruction = v2;
  E->instruction_timer = 2;
  E->timer = 0;
}

static const uint16 g_word_A6D9ED[8] = { 0xc, 0xe, 0x10, 0x12, 0x1c, 0x20, 0x28, 0x30 };

static void Ridley_Func_116(void) {  // 0xA6D9A8
  EnemyData *E = gEnemyData(0);
  uint16 r18 = abs16n(E->ai_var_B);
  uint16 v1 = r18 + abs16n(E->ai_var_C);
  if (v1) {
    uint16 v2 = v1 - r18;
    if (sign16(v2 - r18)) v2 = r18;
    int v5 = ((4 * v2) & 0xF00) >> 8;
    if (v5 >= 7) v5 = 7;
    uint16 v6 = g_word_A6D9ED[v5];
    if (!(E->ai_var_C & 0x8000)) v6 >>= 1;
    RV(0x08) = v6;
  } else {
    RV(0x08) = 0;
  }
}

static void Ridley_Func_115(void) {  // 0xA6D97D: aleteo
  Ridley_Func_116();
  int16 v1 = RV(0x09) - RV(0x08);
  RV(0x09) = v1;
  if (v1 < 0) {
    RV(0x09) = 32;
    uint16 v2 = RV(0x07) + 1;
    if (v2 >= 0xA) v2 = 0;
    RV(0x07) = v2;
  }
}

/* ------------------------------------------------------------------ paleta */
static void Ridley_D495(uint16 r18) {  // 0xA6D495
  if (!palette_change_num)
    WriteColorsToPalette(0x1E2, (uint16)(28 * r18 - 7062), 0xE);
}

static void Ridley_Func_101(void) {  // 0xA6D4B5
  if (RV(0x01) && RV(0x0D) >= 0x32)
    Ridley_D495(RV(0x0D) >= 0x46 ? 2 : 0);
}

static void Ridley_Func_102(void) {  // 0xA6D4DA
  uint16 v0 = 3584;
  EnemyData *E = gEnemyData(0);
  if (E->flash_timer >= 2 && ((random_enemy_counter + 1) & 2))
    v0 = 0;
  RV(0x0C) = v0;
}

/* ------------------------------------------------------------------ tiles animados (cola de VRAM) */
static void Ridley_Func_118(void) {  // 0xA6DA0C
  uint16 v1 = RV(0x06) - 1;
  RV(0x06) = v1;
  if (v1)
    return;
  uint16 i = RV(0x05), v3;
  for (;;) {
    v3 = i;
    i = A6W(v3);
    if (!(i & 0x8000))
      break;
    RV(0x05) = i;
  }
  RV(0x06) = i;
  /* dos entradas de 64 bytes desde $AD:xxxx a VRAM 0x7220 y 0x7320 (palabras) */
  RidleyQueueTiles(A6W(v3 + 2), A6W(v3 + 4), 29216, 29472, 64);
  /* vram_write_queue_tail += 14: la cola la vacía el NMI en el mismo cuadro (aquí, la Genesis) */
  RV(0x05) = v3 + 6;
}

static void Ridley_Func_119(uint8 carry) {  // 0xA6DA8B
  uint16 v1 = carry ? 0xDAD4 : 0xDAD0;
  RidleyQueueTiles(A6W(v1), A6W(v1 + 2), 31424, 31680, 128);
  /* vram_write_queue_tail += 14: la cola la vacía el NMI en el mismo cuadro (aquí, la Genesis) */
}

/* ------------------------------------------------------------------ cola */
static void Ridley_Func_86(void) {  // 0xA6CEBA
  EnemyData *E = gEnemyData(0);
  TS[23] = TS[25] + E->y_pos + 16;
  TS[33] = TS[35] + TS[23];
  TS[43] = TS[45] + TS[35] + TS[23];
  TS[53] = TS[55] + TS[43];
  TS[63] = TS[65] + TS[55] + TS[43];
  TS[73] = TS[75] + TS[63];
  TS[83] = TS[85] + TS[75] + TS[63];
  static const int16 g_word_A6CF54[3] = { 0x20, 0, -0x20 };
  uint16 v1 = RV(0x10) - 1;
  if (RV(0x10) != 1)
    v1 = TS[24] + g_word_A6CF54[RV(0x10)];
  TS[22] = E->x_pos + v1;
  if (RV(0x10) == 1) {
    TS[32] = E->x_pos;
    TS[42] = TS[32]; TS[52] = TS[32]; TS[62] = TS[32]; TS[72] = TS[32]; TS[82] = TS[32];
  } else {
    TS[32] = TS[34] + TS[22];
    TS[42] = TS[44] + TS[34] + TS[22];
    TS[52] = TS[54] + TS[42];
    TS[62] = TS[64] + TS[54] + TS[42];
    TS[72] = TS[74] + TS[62];
    TS[82] = TS[84] + TS[74] + TS[62];
  }
}

static void Ridley_Func_87_one(int a, int b, uint16 lim, uint16 rest) {
  if (TS[a]) {
    if ((int16)(TS[a] - TS[b]) < 0)
      TS[a] = 0;
    uint16 v = TS[9] + TS[b];
    if (!sign16(TS[9] + TS[b] - lim))
      v = lim;
    TS[b] = v;
  } else if ((int16)(rest - TS[b]) < 0) {
    TS[b] -= 128;
  }
}

static void Ridley_Func_87(void) {  // 0xA6CF5A
  Ridley_Func_87_one(30, 29, 6144, 2048);
  Ridley_Func_87_one(40, 39, 6144, 2048);
  Ridley_Func_87_one(50, 49, 5632, 2048);
  Ridley_Func_87_one(60, 59, 5632, 2048);
  Ridley_Func_87_one(70, 69, 4608, 2048);
  Ridley_Func_87_one(80, 79, 1280, 1280);
}

static void Ridley_Func_88(uint16 k) {  // 0xA6D09F (k: desplazamiento en bytes, 0, 20, ... 120)
  int v4 = k >> 1;
  uint16 v2;
  if (TS[v4 + 16] & 0x8000) {
    uint16 v5 = TS[v4 + 17];
    if (v5 < TS[15]) {
      TS[v4 + 17] += TS[10];
      return;
    }
    if (v5 != 0xFFFF) {
      TS[v4 + 17] = -1;
      TS[v4 + 26] = 0x8000;
      TS[v4 + 28] = TS[v4 + 18];
    }
    if (!(TS[v4 + 18] & 0x8000)) {
      if (TS[14] & 0x8000) {
        v2 = TS[10] + TS[v4 + 21];
        if ((int16)(v2 - TS[12]) >= 0) {
          TS[v4 + 18] = 0x8000;
          v2 = TS[12];
        }
      } else {
        TS[v4 + 20] = 3072;
        v2 = TS[10] + TS[v4 + 21];
        if ((int16)(v2 - TS[14]) >= 0) {
          if (!k || !TS[v4 + 6]) {
            TS[v4 + 21] = TS[14];
            goto stop;
          }
          TS[v4 + 18] = 0x8000;
          v2 = TS[12];
        }
      }
    } else {
      int16 v6;
      if (TS[13] & 0x8000) {
        v6 = TS[v4 + 21] - TS[10] - 1;
        if ((int16)(v6 - TS[11]) < 0) {
          TS[v4 + 18] = 0;
          v2 = TS[11];
          goto set;
        }
      } else {
        TS[v4 + 20] = 3072;
        v6 = TS[v4 + 21] - TS[10] - 1;
        if ((int16)(v6 - TS[13]) < 0) {
          if (!k || !TS[v4 + 6]) {
            TS[v4 + 21] = TS[13];
            goto stop;
          }
          TS[v4 + 18] = 0;
          v2 = TS[11];
          goto set;
        }
      }
      v2 = v6 + 1;
    }
set:
    TS[v4 + 21] = v2;
  } else {
    v2 = TS[v4 + 21];
  }
  {
    uint16 r18 = (uint8)v2;
    if (k)
      r18 = (uint8)(LO8(TS[v4 + 11]) + r18);
    TS[v4 + 24] = ComputeSinMult(HI8(TS[v4 + 19]), r18);
    TS[v4 + 25] = ComputeCosMult(HI8(TS[v4 + 19]), r18);
  }
  return;
stop:
  TS[v4 + 16] = 0;
  TS[v4 + 17] = 0;
  TS[v4 + 18] ^= 0x8000;
}

static void Ridley_CBDC(void) {  // 0xA6CBDC
  for (uint16 k = 0; k < 140; k += 20)
    Ridley_Func_88(k);
}

static uint8 Ridley_Func_90(uint16 *ox, uint16 *oy) {  // 0xA6D242
  uint16 rx = TS[82], ry = TS[83];
  if (!projectile_counter)
    return 0;
  for (int v2 = 0; v2 < 5; v2++) {
    uint8 t = HI8(projectile_type[v2]) & 0xF;
    if (t == 1 || t == 2) {
      uint16 v3 = abs16n(projectile_x_pos[v2] - rx);
      if (v3 < projectile_x_radius[v2] || (uint16)(v3 - projectile_x_radius[v2]) < 64) {
        uint16 v6 = abs16n(projectile_y_pos[v2] - ry);
        if (v6 < projectile_y_radius[v2] || (uint16)(v6 - projectile_y_radius[v2]) < 64) {
          *ox = projectile_x_pos[v2];
          *oy = projectile_y_pos[v2];
          return 1;
        }
      }
    }
  }
  return 0;
}

static void Ridley_Func_89(uint16 a) {  // 0xA6D19D
  uint16 r18, r20;
  TS[5] = (uint16)(LO8(a) << 8);
  if (RV(0x10) != 1 && RV(0x01)) {
    uint16 px, py;
    if (Ridley_Func_90(&px, &py)) {
      r18 = px - TS[22];
      r20 = py - TS[23];
    } else {
      r18 = samus_x_pos - TS[22];
      r20 = samus_y_pos + 24 - TS[23];
    }
    r18 = (uint8)-(CalculateAngleFromXY(r18, r20) + 0x80);
    if (RV(0x10)) {
      uint16 v7 = r18;
      if (r18 >= 0x18 && r18 < 0xE8)
        v7 = 24;
      uint16 v8 = TS[5] + v7 + 0x4000;
      if (v8 >= TS[21]) {
        TS[14] = v8;
        TS[10] = 8;
      }
    } else {
      uint16 v5 = r18;
      if (r18 >= 0x18 && r18 < 0xE8)
        v5 = 232;
      uint16 v6 = v5 + 16128 - TS[5];
      if (v6 < TS[21]) {
        TS[13] = v6;
        TS[10] = 8;
      }
    }
  }
}

static void Ridley_Func_77(void) {  // 0xA6CBFE
  int v0 = RV(0x10);
  TS[11] = A6W(0xCC12 + 2 * v0);                  // g_word_A6CC12
  TS[12] = A6W(0xCC18 + 2 * v0);                  // g_word_A6CC18
}

static uint8 Ridley_Func_78(void) {  // 0xA6CC1E
  return (TS[76] & TS[66] & TS[56] & TS[46] & TS[36] & TS[26] & TS[16]) != 0;
}
static uint16 tail_any(void) { return TS[76] | TS[66] | TS[56] | TS[46] | TS[36] | TS[26] | TS[16]; }

static uint8 Ridley_Func_80_DoubleRet(void) {  // 0xA6CC7D
  EnemyData *E = gEnemyData(0);
  uint8 retval = 0;
  uint16 v1;
  if (TS[2] && (TS[13] & TS[14] & 0x8000))
    goto L7;
  if (!TS[1])
    goto L9;
  if ((uint8)random_number >= 0xF0) {
L7:
    v1 = TS[2] - 1;
    goto L8;
  }
  if (abs16n(samus_x_pos - E->x_pos) < 0x80) {
    v1 = 0;
L8:
    Ridley_Func_89(v1);
    retval = 1;
  }
L9:
  TS[2] = 0;
  return retval;
}

static void Ridley_Func_79(void) {  // 0xA6CC39
  Ridley_Func_77();
  if (Ridley_Func_78() && Ridley_Func_80_DoubleRet())
    return;
  if (!tail_any()) {
    TS[16] = 0x8000;
    TS[13] = -1;
    TS[14] = -1;
    TS[10] = samus_x_pos < 0x70 ? 1 : 2;
  }
}

static uint8 Ridley_Func_82_DoubleRet(void) {
  if (TS[2] && (TS[13] & TS[14] & 0x8000)) {
    Ridley_Func_89(1);
    TS[2] = 0;
    return 1;
  }
  return 0;
}

static void Ridley_Func_81(void) {  // 0xA6CCBD
  Ridley_Func_77();
  if (Ridley_Func_78() && Ridley_Func_82_DoubleRet())
    return;
  if (!tail_any()) {
    TS[16] = 0x8000;
    TS[13] = -1;
    TS[14] = -1;
    TS[10] = samus_x_pos < 0x70 ? 1 : 2;
  }
}

static void Ridley_Func_85(void) {  // 0xA6CE65 (el original arma _61 desde _21: se conserva)
  TS[21] = LO8(TS[21]) | 0x4000;
  TS[31] = LO8(TS[31]) | 0x4000;
  TS[41] = LO8(TS[41]) | 0x4000;
  TS[51] = LO8(TS[51]) | 0x4000;
  TS[61] = (TS[21] & 0x4FF) | 0x4000;
  TS[71] = LO8(TS[71]) | 0x4000;
  TS[81] = LO8(TS[81]) | 0x4000;
}

static void Ridley_Func_83(void) {  // 0xA6CD24
  EnemyData *E = gEnemyData(0);
  Ridley_Func_77();
  if (Ridley_Func_78()
      && ((uint8)random_number >= 0xF0 || abs16n(samus_x_pos - E->x_pos) < 0x80)
      && (TS[13] & TS[14] & 0x8000)) {
    TS[13] = 16128;
    TS[10] = 8;
  } else if (!tail_any()) {
    if (!(E->ai_var_C & 0x8000))
      TS[0] = 5;
    TS[13] = -1;
    TS[14] = -1;
    uint16 v1 = TS[8];
    if (!TS[8] || (v1 = TS[8] - 1, (TS[8] = v1) != 0)) {
      TS[10] = v1;
    } else {
      TS[16] = 0x8000;
      Ridley_Func_85();
      TS[10] = 8;
    }
  }
}

static void Ridley_Func_84(void) {  // 0xA6CDAA
  EnemyData *E = gEnemyData(0);
  Ridley_Func_77();
  if (Ridley_Func_78()
      && ((uint8)random_number >= 0xF0 || abs16n(samus_x_pos - E->x_pos) < 0x80)
      && (TS[13] & TS[14] & 0x8000)) {
    TS[13] = 16128;
    TS[10] = 8;
  } else if (!tail_any()) {
    if (!(E->ai_var_C & 0x8000)) {
      TS[0] = 6;
      for (int v = 20; v <= 80; v += 10) {
        TS[v] = 2560;
        TS[v + 1] = 0x4000;
      }
      TS[16] = 0x8000;
    }
    TS[13] = -1;
    TS[14] = -1;
    uint16 v1 = TS[8];
    if (TS[8]) {
      v1 = TS[8] - 1;
      TS[8] = v1;
      if (!v1) {
        TS[16] = 0x8000;
        Ridley_Func_85();
        v1 = 8;
      }
    }
    TS[10] = v1;
  }
}

static void Ridley_CBC7(void);
static void Ridley_Func_75(void) {  // 0xA6CB72
  if (RV(0x10)) {
    TS[13] = 0x4000;
    TS[14] = -1;
  } else {
    TS[14] = 0x4000;
    TS[13] = -1;
  }
  Ridley_CBC7();
  if (!tail_any())
    TS[0] = 4;
}

static void Ridley_CBC0(void) { Ridley_Func_79(); Ridley_CBDC(); }   // 0xA6CBC0
static void Ridley_CBC7(void) { Ridley_Func_83(); Ridley_CBDC(); }   // 0xA6CBC7
static void Ridley_CBCE(void) { Ridley_Func_84(); Ridley_CBDC(); }   // 0xA6CBCE
static void Ridley_CBD5(void) { Ridley_Func_81(); Ridley_CBDC(); }   // 0xA6CBD5
static void Ridley_Func_71(void) { TS[10] = 8; Ridley_Func_75(); TS[0] = 3; }   // 0xA6CB33
static void Ridley_Func_72(void) { TS[10] = 8; Ridley_Func_75(); }              // 0xA6CB45
static void Ridley_Func_73(void) { TS[10] = 3; Ridley_Func_75(); TS[0] = 6; }   // 0xA6CB4E
static void Ridley_Func_74(void) { TS[10] = 2; Ridley_Func_75(); TS[0] = 7; }   // 0xA6CB60

static void Ridley_Func_91(void) {  // 0xA6D2AA: sonido de la cola
  if (!sign16(TS[10] - 8)) {
    uint16 v1 = RV(0x0E) + 1;
    RV(0x0E) = v1;
    if (sign16(v1 - 16))
      return;
    if (!RV(0x0F))
      QueueSfx3_Max6(0x21);
  }
  RV(0x0E) = 0;
}

static uint8 Ridley_Func_124(RectN r) {  // 0xA6DF29
  uint16 v0 = abs16n(samus_x_pos - r.x);
  if (v0 < samus_x_radius || (uint16)(v0 - samus_x_radius) < r.w) {
    uint16 v3 = abs16n(samus_y_pos - r.y);
    if (v3 < samus_y_radius || (uint16)(v3 - samus_y_radius) < r.h)
      return 1;
  }
  return 0;
}

static void Ridley_Func_127(void) {  // 0xA6DFD9: la punta de la cola contra Samus
  RectN r = { TS[82], TS[83], 14, 14 };
  if (Ridley_Func_124(r)) {
    Samus_DealDamage(SuitDamageDivision(RV(0x1C)));
    samus_invincibility_timer = 96;
    samus_knockback_timer = 5;
    knockback_x_dir = (int16)(samus_x_pos - TS[82]) >= 0;
  }
}

static void Ridley_Func_70(void) {  // 0xA6CAF5
  Ridley_Func_91();
  switch (TS[0]) {                                  // off_A6CB21
  case 0: break;
  case 1: Ridley_CBC0(); break;
  case 2: Ridley_Func_71(); break;
  case 3: Ridley_Func_72(); break;
  case 4: Ridley_CBC7(); break;
  case 5: Ridley_CBCE(); break;
  case 6: Ridley_Func_73(); break;
  case 7: Ridley_Func_74(); break;
  case 8: Ridley_CBD5(); break;
  default: nt_unimpl("Ridley_Func_70 (estado de la cola)"); break;
  }
  Ridley_Func_86();
  Ridley_Func_87();
  EnemyData *E = gEnemyData(0);
  if (!(RV(0x1B) | samus_invincibility_timer | (E->properties & 0x400)))
    Ridley_Func_127();
}

/* ------------------------------------------------------------------ dibujo */
static void sub_A6DB2A(void) {  // 0xA6DB2A: la cola (7 piezas)
  if (gEnemyData(0)->properties & 0x100)
    return;
  uint16 a = RV(0x0C);
  uint8 i = (uint8)((LO8(TS[71]) + LO8(TS[81]) + 8) & 0xF0) >> 4;
  DrawA6Spritemap(A6W(0xDCBA + 2 * i), TS[82], TS[83], a);       // kRidley_Ilist_DCBA
  DrawA6Spritemap(0xDC9E, TS[72], TS[73], a);
  DrawA6Spritemap(0xDC9E, TS[62], TS[63], a);
  DrawA6Spritemap(0xDC97, TS[52], TS[53], a);
  DrawA6Spritemap(0xDC97, TS[42], TS[43], a);
  DrawA6Spritemap(0xDC90, TS[32], TS[33], a);
  DrawA6Spritemap(0xDC90, TS[22], TS[23], a);
}

static void Ridley_Func_120(void) {  // 0xA6DAD8: las alas
  uint16 v10 = RV(0x10);
  if (v10) {
    if (v10 == 1)
      return;
    v10 = 10;
  }
  EnemyData *E = gEnemyData(0);
  uint16 v2 = A6W(0xDB02 + 2 * (RV(0x07) + v10));                 // g_off_A6DB02
  DrawA6Spritemap(v2, E->x_pos, E->y_pos, RV(0x0C));
}

/* ------------------------------------------------------------------ estados de Ceres (CallRidleyFunc) */
static void CeresRidley_Func_1(void) {  // 0xA6A2BD
  if (random_number >= 0xFF00)
    TS[15] = (random_number & 0xF) + 8;
}

static void CeresRidley_Func_2(void) {  // 0xA6A354
  EnemyData *E = gEnemyData(0);
  E->ai_var_B = 0;
  E->ai_var_C = 0;
}

static void CeresRidley_A377(void) {  // 0xA6A377
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    E->ai_var_A = 0xA389;                         // CeresRidley_Func_4
    E->ai_var_E = 0;
    E->ai_preinstr = 0;
  }
}

static void CeresRidley_Func_3(void) {  // 0xA6A35B
  if (door_transition_flag_enemies)
    return;
  EnemyData *E = gEnemyData(0);
  E->ai_var_A = 0xA377;
  E->ai_preinstr = (area_index == 2) ? 170 : 512;
  CeresRidley_A377();
}

static void CeresRidley_Func_4(void) {  // 0xA6A389: Ridley aparece en la oscuridad (ojos)
  EnemyData *E = gEnemyData(0);
  if (E->ai_var_E & 0x8000)
    return;
  uint16 e = E->ai_var_E;
  if (++E->ai_preinstr) {
    E->ai_preinstr = 0;
    uint8 v2 = A6B(0xE269 + e);                    // g_byte_A6E269
    if (v2 == 255) {
      E->ai_var_E = 0;
      E->ai_var_A = 0xA3DF;                       // CeresRidley_Func_5
      RV(0x02) = 1;
    } else {
      E->ai_var_E = e + 1;
      uint16 s = 0xE2AA + 6 * v2;                 // g_word_A6E2AA[3 * v2]
      palette_buffer[252] = A6W(s);
      palette_buffer[253] = A6W(s + 2);
      palette_buffer[254] = A6W(s + 4);
      nt_pal_dirty(252, 254);
    }
  }
}

static void CeresRidley_Func_5(void) {  // 0xA6A3DF: Ridley se ilumina
  EnemyData *E = gEnemyData(0);
  if (++E->ai_preinstr < 2)
    return;
  E->ai_preinstr = 0;
  uint16 e = E->ai_var_E;
  for (int i = 0; i < 11; i++, e += 2) {
    uint16 c = A6W(0xE30A + e);                   // g_word_A6E30A[e >> 1]
    palette_buffer[145 + i] = c;
    palette_buffer[241 + i] = c;
  }
  nt_pal_dirty(145, 251);
  if (e >= 0x160) {
    if (area_index == 2)
      E->layer = 2;
    E->properties &= ~0x400;
    E->ai_var_E = 0;
    E->ai_var_A = 0xA455;                         // CeresRidley_Func_6
    E->ai_preinstr = 4;
    QueueMusic_Delayed8(5);
  } else {
    E->ai_var_E = e;
  }
}

static void CeresRidley_Func_6(void) {  // 0xA6A455
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    Ridley_Func_99(0xE690);
    E->ai_preinstr = 0;
    E->ai_var_A = 0xA478;                         // CeresRidley_Func_7
    if (area_index != 2)
      E->ai_preinstr = 252;
  }
}

static void CeresRidley_Func_7(void) {  // 0xA6A478 (rama de Ceres)
  EnemyData *E = gEnemyData(0);
  if (area_index == 2) { nt_unimpl("CeresRidley_Func_7 (Norfair)"); return; }
  if (!(--E->ai_preinstr & 0x8000))
    return;
  E->ai_var_E = 0;
  Ridley_Func_99(0xE91D);
  RV(0x08) = 8;
  RV(0x09) = 8;
  Ridley_Func_D3DC(0x8000);                       // Ridley_Func_95
  TS[0] = 1;
  E->ai_var_A = 0xA354;                           // CeresRidley_Func_2
}

static void CeresRidley_Func_10(void) {  // 0xA6A6C8
  EnemyData *E = gEnemyData(0);
  E->ai_var_C += 20;
  if (sign16(E->y_pos - 80)) {
    E->ai_var_A = 0xA6E8;                         // CeresRidley_Func_11
    RV(0x01) = 1;
  }
}

static void CeresRidley_Func_9(void) {  // 0xA6A6AF
  EnemyData *E = gEnemyData(0);
  E->ai_var_C -= 16;
  if (sign16(E->y_pos - 112)) {
    E->ai_var_A = 0xA6C8;
    CeresRidley_Func_10();
  }
}

static uint8 CeresRidley_Func_12(void) {  // 0xA6A763
  RectN r = { 192, 100, 8, 8 };
  Ridley_Func_106(0, r.x, r.y);
  return Shitroid_Func_2(r);
}

static void CeresRidley_Func_22(void);
static void Ridley_Func_44(void);

static void CeresRidley_Func_11(void) {  // 0xA6A6E8: elige el próximo ataque
  EnemyData *E = gEnemyData(0);
  if (RV(0x0D) >= 0x64) {
    RV(0x01) = 0;
    E->ai_var_A = 0xBD9A;                         // Ridley_Func_44
    Ridley_Func_44();
  } else if (sign16(samus_health - 30)) {
    RV(0x01) = 0;
    E->ai_var_A = 0xA971;                         // CeresRidley_Func_22
    CeresRidley_Func_22();
  } else {
    if (CeresRidley_Func_12()) {
      uint16 v1 = RV(0x00) + 1;
      RV(0x00) = v1;
      if (v1 < 0x7C)
        return;
    }
    E->ai_var_A = A6W(0xA743 + 2 * (random_number & 0xF));   // g_off_A6A743
    RV(0x00) = 0;
  }
}

static void CeresRidley_A7F9(void) {  // 0xA6A7F9
  uint16 r22 = sign16(random_number) ? -(random_number & 7) : (random_number & 7);
  EnemyData *E = gEnemyData(0);
  Ridley_Func_106(0, RV(0x17) + r22, RV(0x18) + r22);
  int16 v4 = RV(0x00) - 1;
  RV(0x00) = v4;
  if (v4 < 0) {
    RV(0x00) = 0;
    E->ai_var_A = 0xA6E8;
  }
}

static void CeresRidley_Func_13(void) {  // 0xA6A782: bolas de fuego desde arriba
  EnemyData *E = gEnemyData(0);
  uint16 v1 = abs16n(E->ai_var_C);
  if (v1 < 0x80) v1 = 128;
  E->ai_var_C = sign16(E->ai_var_C) ? -v1 : v1;
  Ridley_Func_106(0, E->x_pos, 88);
  if (sign16(E->y_pos - 80)) {
    uint16 v3 = RV(0x00) + 1;
    RV(0x00) = v3;
    if (v3 >= 0x30)
      E->ai_var_A = 0xA88D;                       // CeresRidley_Func_16
  } else if (sign16(E->y_pos - 128)) {
    RV(0x17) = E->x_pos;
    RV(0x18) = E->y_pos;
    Ridley_Func_99(0xE73A);
    E->ai_var_A = 0xA7F9;
    RV(0x00) = 224;
    CeresRidley_A7F9();
  }
}

static void CeresRidley_Func_15(void) {  // 0xA6A84E: persigue a Samus
  EnemyData *E = gEnemyData(0);
  uint16 r18 = samus_x_pos;
  uint16 r20 = sign16(samus_y_pos - 132) ? 64 : (uint16)(samus_y_pos - 68);
  Ridley_Func_106(0xD, r18, r20);
  RectN r = { r18, r20, 2, 2 };
  int doit = 1;
  if (Shitroid_Func_2(r)) {                       /* lejos: sigue mientras F no pase a negativo */
    doit = (int16)(E->ai_preinstr - 1) < 0;
    E->ai_preinstr--;
  }
  if (doit) {
    RV(0x00) = 0;
    E->ai_var_A = 0xA6E8;
  }
}

static void CeresRidley_Func_14(void) {  // 0xA6A83C
  Ridley_Func_99(0xE548);
  EnemyData *E = gEnemyData(0);
  E->ai_var_A = 0xA84E;
  E->ai_preinstr = 64;
  CeresRidley_Func_15();
}

static void CeresRidley_Func_17(void) {  // 0xA6A8A4
  Ridley_Func_106(1, 192, 80);
  EnemyData *E = gEnemyData(0);
  if (sign16(E->y_pos - 96)) {
    E->ai_var_A = 0xA8D4;                         // CeresRidley_Func_18
    E->ai_preinstr = 10;
    RV(0x0A) = 0;
  }
}

static void CeresRidley_Func_16(void) {  // 0xA6A88D: vuelta en picada
  EnemyData *E = gEnemyData(0);
  E->ai_var_A = 0xA8A4;
  E->ai_preinstr = 10;
  RV(0x0A) = 0;
  TS[1] = 0;
  CeresRidley_Func_17();
}

static void CeresRidley_Func_18(void) {  // 0xA6A8D4
  Ridley_Func_111(-32, -1024, 768);
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    E->ai_var_A = 0xA8F8;
    E->ai_preinstr = 36;
  }
}

static void CeresRidley_Func_19(void) {  // 0xA6A8F8
  Ridley_Func_111(-512, -16384, 768);
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    E->ai_var_A = 0xA923;
    E->ai_preinstr = 28;
    TS[2] = 1;
  }
}

static void CeresRidley_Func_20(void) {  // 0xA6A923
  Ridley_Func_111(-512, -30720, 768);
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    E->ai_var_A = 0xA947;
    E->ai_preinstr = 1;
  }
}

static void CeresRidley_Func_21(void) {  // 0xA6A947
  Ridley_Func_111(-768, -30720, 768);
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    E->ai_var_A = 0xA6E8;
    RV(0x00) = 0;
    TS[1] = 1;
  }
}

static void CeresRidley_Func_23(void);
static void CeresRidley_A994(void) {  // 0xA6A994
  EnemyData *E = gEnemyData(0);
  E->ai_var_A = 0xA9A0;                           // CeresRidley_Func_23
  E->ai_preinstr = 64;
  CeresRidley_Func_23();
}

static void CeresRidley_Func_22(void) {  // 0xA6A971: se va con la bebé
  EnemyData *E = gEnemyData(0);
  RV(0x20) = -192;
  Ridley_Func_106(1, 192, -128);
  if (sign16(E->y_pos + 128))
    CeresRidley_A994();
}


static void CeresRidley_Func_23(void) {  // 0xA6A9A0
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    SpawnEnemyN(0xA6, 0xAA2F);                    // CeresRidley_Func_25
    SpawnEnemyN(0xA6, 0xAA3F);
    E->ai_var_B = 0;
    E->ai_var_C = 0;
    TS[0] = 0;
    E->ai_var_A = 0xAA11;                         // CeresRidley_Func_24
    WriteColorsToPalette(0xA2, 0xA9E3, 0xF);
    WriteColorsToPalette(0x42, 0xAA01, 8);
    WriteColorsToPalette(0x1E2, 0xAA01, 8);
  }
}

/* --- huida en modo 7 ($A6:AA54-B0EF): la lógica y los registros del modo 7 son los del SNES; la imagen la elige
   la capa de la Genesis entre cuadros preparados en el PC (RidleyGetawayFrame) */
static void CeresRidley_Func_27(void) {  // 0xA6AA54
  reg_BGMODE_fake = 7;
  irq_enable_mode7 = 1;
  reg_M7SEL = 0x80;
  reg_M7A = 256;
  reg_M7B = 256;
  reg_M7C = 256;
  reg_M7D = 256;
  reg_M7X = 64;
  reg_M7Y = 64;
  RV(0x32) = 0;
  RV(0x33) = 0;
  RV(0x0A) = 1;
  RV(0x34) = -128;
  reg_BG1HOFS = -128;
  RV(0x35) = 32;
  reg_BG1VOFS = 32;
  RV(0x30) = 2048;
  RV(0x31) = 1024;
}

void RidleyGetawayMode7Queue(uint16 list);       /* capa de la Genesis: QueueMode7Transfers($A6, list) */
void Samus_SetPushedOutOfCeresRidley(void);       /* samus_move.c ($90:E119) */

static void CeresRidley_Func_35(uint16 a) {  // 0xA6B0EF: la paleta de Ridley según la escala
  WriteColorsToPalette(0xA2, (uint16)(32 * HI8(a) - 0x4EF9), 0xF);
}

static void CeresRidley_Func_29(void) {  // 0xA6AB5F: matriz del modo 7 (giro y escala)
  uint16 r18 = HI8(RV(0x0A));
  uint16 s = RV(0x30);
  /* CeresMult(CeresRidley_AC30(ang, 0x100), s): seno * 0x100 >> 8 = seno; luego |seno| * s >> 8 con su signo */
  int16 c = kSinCosTable8bit_Sext[((r18 + 64) & 0xff) + 64];
  int16 sn = kSinCosTable8bit_Sext[(r18 & 0xff) + 64];
#define CERES_MULT(a) ((uint16)(sign16(a) ? -(uint16)(((uint32)abs16n(a) * s) >> 8) : (uint16)(((uint32)abs16n(a) * s) >> 8)))
  reg_M7A = CERES_MULT((uint16)c);
  reg_M7B = CERES_MULT((uint16)sn);
  reg_M7C = CERES_MULT((uint16)-sn);
  reg_M7D = CERES_MULT((uint16)c);
#undef CERES_MULT
}

static void CeresRidley_Func_33(void) {  // 0xA6ACBC: aleteo (tilemap del modo 7)
  if ((nmi_frame_counter_word & 3) == 0) {
    uint16 v1 = (RV(0x36) + 1) & 3;
    RV(0x36) = v1;
    RidleyGetawayMode7Queue(A6W(0xACDA + 2 * v1));
  }
}

static void CeresRidley_Func_34(void) {  // 0xA6AD27
  if ((nmi_frame_counter_word & 7) == 0) {
    uint16 v1 = (RV(0x37) + 1) & 1;
    RV(0x37) = v1;
    RidleyGetawayMode7Queue(A6W(0xAD45 + 2 * v1));
  }
}

static void CeresRidley_Func_28(void) {  // 0xA6AABD: un cuadro de la huida
  EnemyData *E = gEnemyData(0);
  uint16 t = RV(0x33);
  RV(0x33) = t + 2;
  if (!t)
    QueueSfx2_Max6(0x4E);
  if (t == 208) {
    Samus_SetPushedOutOfCeresRidley();
    earthquake_type = 35;
    earthquake_timer = 64;
  }
  uint16 v2 = t >> 1;
  uint16 sc = A6W(0xAE4D + 2 * v2);               // g_word_A6AE4D
  if (sc == 0xFFFF) {
    ++RV(0x32);
    reg_BGMODE_fake = 9;
    reg_M7SEL = 0;
    reg_M7A = 0; reg_M7B = 0; reg_M7C = 0; reg_M7D = 0;
    reg_M7X = 0; reg_M7Y = 0;
    reg_BG1HOFS = 0;
    reg_BG1VOFS = 0;
    E->ai_var_A = 0xC04E;                         // Ridley_Func_54
    E->ai_preinstr = 0;
  } else {
    RV(0x30) = sc;
    uint16 v3 = A6W(0xAF2F + 2 * v2) + RV(0x35);  // g_word_A6AF2F
    RV(0x35) = v3;
    reg_BG1VOFS = v3;
    uint16 v4 = RV(0x34) - A6W(0xB00F + 2 * v2);  // g_word_A6B00F
    RV(0x34) = v4;
    reg_BG1HOFS = v4;
    CeresRidley_Func_35(RV(0x30));
    RV(0x0A) += 48;
    CeresRidley_Func_29();
    CeresRidley_Func_33();
    CeresRidley_Func_34();
  }
  RidleyGetawayFrame();
}

void HandleCeresRidleyGetawayCutscene(void) {  // 0xA6AAAF (código de la sala, cada cuadro)
  if (!RV(0x32))
    CeresRidley_Func_28();
}

static void CeresRidley_Func_24(void) {  // 0xA6AA11
  gEnemyData(0)->ai_var_A = 0xAA4F;               // nullsub_233
  ceres_status = 1;
  CeresRidley_Func_27();
}

static void Ridley_Func_61(void);
static void CeresRidley_Func_26(void) { Ridley_Func_61(); }   // 0xA6AA50

static void Ridley_Func_61(void) {  // 0xA6C19C: ciclo de color de la cápsula vacía / la sala
  if (!(door_transition_flag_enemies | palette_change_num) && (nmi_frame_counter_word & 3) == 0) {
    uint16 v1 = RV(0x38) + 1;
    if (v1 >= 0x10)
      v1 = 0;
    RV(0x38) = v1;
    uint16 v2 = 0xC1DF + 2 * (v1 + 2 * v1);                // g_word_A6C1DF
    palette_buffer[97] = A6W(v2);
    palette_buffer[98] = A6W(v2 + 2);
    palette_buffer[99] = A6W(v2 + 4);
    nt_pal_dirty(97, 99);
  }
}

/* ------------------------------------------------------------------ huida: texto y contador ($A6:C04E) */
/* La Genesis: los tiles de la letra y del contador ya están preparados; el texto llega celda por celda. */
void EscapeTextCell(uint16 vram_dst, uint16 word);   /* celda del BG3 del SNES (dirección de VRAM en palabras) */

static uint8 ProcessEscapeTimerTileTransfers(void) {  // 0xA6C26E (las transferencias de tiles: la Genesis no las necesita)
  EnemyData *E = gEnemyData(0);
  uint16 e = E->ai_var_E;
  if (A6W(e)) {
    E->ai_var_E = e + 7;
    if (A6W(E->ai_var_E))
      return 0;
  }
  return 1;
}

static void EscapeCopyEntry(uint16 e) {           /* una entrada de la cola de VRAM: un trozo de tilemap del banco $A6 */
  uint16 n = A6W(e) >> 1, src = A6W(e + 2), dst = A6W(e + 5);
  for (uint16 i = 0; i < n; i++)
    EscapeTextCell(dst + i, A6W(src + 2 * i));
}

static uint8 HandleTypewriterText_Ext(uint16 a) {  // 0xA6C2A7
  if (RV(0x3D)) {
    RV(0x3D)--;
    return 0;
  }
  RV(0x3D) = RV(0x3E);
  uint16 i = RV(0x3B);
  int16 v4;
  for (;;) {
    v4 = A6W(i);
    if (!v4)
      return 1;
    if (v4 == 1) { RV(0x3E) = A6W(i + 2); i += 4; continue; }
    if (v4 == 13) { RV(0x3C) = A6W(i + 2); i += 4; continue; }
    break;
  }
  v4 = (uint8)v4;
  RV(0x3B) = i + 1;
  if (v4 == 32) {
    ++RV(0x3C);
    return 0;
  }
  if (v4 == 33)
    v4 = 91;
  RV(0x3A) = a + v4 - 65;
  EscapeTextCell(RV(0x3C), RV(0x3A));
  RV(0x3C)++;
  if (!sign16(++RV(0x3F) - 2)) {
    RV(0x3F) = 0;
    if (area_index == 6)
      QueueSfx2_Max3(0x45);
    else
      QueueSfx3_Max3(0xD);
  }
  return 0;
}

static void Ridley_Func_59(void) { EscapeCopyEntry(0xC15D); }   // 0xA6C136: "EMERGENCY" (una entrada)
static void Ridley_Func_62(void) {                              // 0xA6C383 (texto japonés): lista hasta un 0
  for (uint16 e = 0xC3B8; A6W(e); e += 7)
    EscapeCopyEntry(e);
}

static void Ridley_C09F(void) {  // 0xA6C09F
  if (ProcessEscapeTimerTileTransfers() & 1) {
    EnemyData *E = gEnemyData(0);
    E->ai_preinstr += 2;
    Ridley_Func_59();
    E->ai_var_E = 128;
    QueueMusic_Delayed8(7);
  }
}

static void Ridley_C08E(void) {  // 0xA6C08E
  if (ProcessEscapeTimerTileTransfers() & 1) {
    EnemyData *E = gEnemyData(0);
    E->ai_var_E = 0xC4FE;
    E->ai_preinstr += 2;
    Ridley_C09F();
  }
}

static void Ridley_Func_55(void) {  // 0xA6C062
  palette_buffer[97] = palette_buffer[1];
  palette_buffer[99] = palette_buffer[3];
  palette_buffer[81] = palette_buffer[17];
  palette_buffer[83] = palette_buffer[19];
  nt_pal_dirty(81, 99);
  EnemyData *E = gEnemyData(0);
  E->ai_var_E = 0xC4CB;
  E->ai_preinstr += 2;
  Ridley_C08E();
}

static void Ridley_Func_56(void) {  // 0xA6C0BB
  Ridley_Func_61();
  EnemyData *E = gEnemyData(0);
  if (E->ai_var_E-- == 1) {
    RV(0x3B) = 0xC450;
    RV(0x3C) = 0;
    RV(0x3D) = 0;
    RV(0x3E) = 0;
    RV(0x3F) = 0;
    E->ai_var_E = 32;
    if (!japanese_text_flag)
      E->ai_preinstr += 2;
    E->ai_preinstr += 2;
  }
}

static void Ridley_C104(void) {  // 0xA6C104
  Ridley_Func_61();
  if (HandleTypewriterText_Ext(0x3582) & 1)
    gEnemyData(0)->ai_preinstr += 2;
}

static void Ridley_Func_57(void) {  // 0xA6C0F5
  EnemyData *E = gEnemyData(0);
  if (E->ai_var_E-- == 1) {
    E->ai_preinstr += 2;
    Ridley_Func_62();
  }
  Ridley_C104();
}

static void Ridley_Func_58(void) {  // 0xA6C117
  Ridley_Func_61();
  CeresRidley_Func_2();
  EnemyData *E = gEnemyData(0);
  E->ai_preinstr = 0;
  E->ai_var_A = 0xAA50;                         // CeresRidley_Func_26
  timer_status = 1;
  ceres_status = 2;
  boss_bits_for_area[area_index] |= 1;          // SetBossBitForCurArea
}

static void Ridley_Func_54(void) {  // 0xA6C04E
  switch (gEnemyData(0)->ai_preinstr) {
  case 0: Ridley_Func_55(); break;
  case 2: Ridley_C08E(); break;
  case 4: Ridley_C09F(); break;
  case 6: Ridley_Func_56(); break;
  case 8: Ridley_Func_57(); break;
  case 0xA: Ridley_C104(); break;
  case 0xC: Ridley_Func_58(); break;
  default: nt_unimpl("Ridley_Func_54 (paso)"); break;
  }
}

/* --- 100 disparos: Ridley toma la cápsula sin esperar */
static void Ridley_Func_46(void);
static void Ridley_Func_47(void);

static void Ridley_Func_44(void) {  // 0xA6BD9A
  Ridley_Func_106(1, 192, 128);
  EnemyData *E = gEnemyData(0);
  if (!sign16(E->x_pos - 192))
    E->ai_var_A = 0xBDBC;                         // Ridley_Func_45
}

static void Ridley_Func_45(void) {  // 0xA6BDBC
  EnemyData *E = gEnemyData(0);
  RV(0x20) = -192;
  Ridley_Func_106(1, 192, -128);
  if (sign16(E->y_pos - 32)) {
    RV(0x40) = 0xBECA;                            // Ridley_Func_52
    E->ai_var_A = 0xBDF2;                         // Ridley_Func_46
    E->ai_preinstr = 21;
  }
}

static void Ridley_Func_46(void) {  // 0xA6BDF2
  EnemyData *E = gEnemyData(0);
  if (--E->ai_preinstr & 0x8000) {
    Ridley_Func_99(0xE658);
    E->ai_var_A = 0xBE03;                         // Ridley_Func_47
    Ridley_Func_47();
  }
}

static uint8 Ridley_Func_48(RectN r) {  // 0xA6BE61
  uint16 v1 = abs16n(RV(0x42) - r.x);
  if (v1 < 4 || (uint16)(v1 - 4) < r.w) {
    uint16 v4 = abs16n(RV(0x44) - r.y);
    if (v4 < 4 || (uint16)(v4 - 4) < r.h)
      return 1;
  }
  return 0;
}

static void Ridley_Func_47(void) {  // 0xA6BE03
  EnemyData *E = gEnemyData(0);
  Ridley_Func_104_0(12, RV(0x42) - 10, RV(0x44) - 56);
  RectN r = { E->x_pos + 14, E->y_pos + 66, 4, 4 };
  if (Ridley_Func_48(r)) {
    RV(0x1B) = 1;
    Ridley_Func_119(1);
    E->ai_var_C = -512;
    RV(0x40) = 0xBEB3;                            // Ridley_Func_51
    E->ai_var_A = 0xA971;                         // CeresRidley_Func_22
  }
}

static void CallRidleyFunc(uint16 f) {
  switch (f) {
  case 0xA354: CeresRidley_Func_2(); return;
  case 0xA35B: CeresRidley_Func_3(); return;
  case 0xA377: CeresRidley_A377(); return;
  case 0xA389: CeresRidley_Func_4(); return;
  case 0xA3DF: CeresRidley_Func_5(); return;
  case 0xA455: CeresRidley_Func_6(); return;
  case 0xA478: CeresRidley_Func_7(); return;
  case 0xA6AF: CeresRidley_Func_9(); return;
  case 0xA6C8: CeresRidley_Func_10(); return;
  case 0xA6E8: CeresRidley_Func_11(); return;
  case 0xA782: CeresRidley_Func_13(); return;
  case 0xA7F9: CeresRidley_A7F9(); return;
  case 0xA83C: CeresRidley_Func_14(); return;
  case 0xA84E: CeresRidley_Func_15(); return;
  case 0xA88D: CeresRidley_Func_16(); return;
  case 0xA8A4: CeresRidley_Func_17(); return;
  case 0xA8D4: CeresRidley_Func_18(); return;
  case 0xA8F8: CeresRidley_Func_19(); return;
  case 0xA923: CeresRidley_Func_20(); return;
  case 0xA947: CeresRidley_Func_21(); return;
  case 0xA971: CeresRidley_Func_22(); return;
  case 0xA9A0: CeresRidley_Func_23(); return;
  case 0xAA11: CeresRidley_Func_24(); return;
  case 0xAA4F: return;                            // nullsub_233
  case 0xAA50: CeresRidley_Func_26(); return;
  case 0xBD9A: Ridley_Func_44(); return;
  case 0xBDBC: Ridley_Func_45(); return;
  case 0xBDF2: Ridley_Func_46(); return;
  case 0xBE03: Ridley_Func_47(); return;
  case 0xC04E: Ridley_Func_54(); return;
  default: nt_unimpl("CallRidleyFunc (estado de Ridley sin portar)"); return;
  }
}

/* --- la bebé junto a Ridley (ridley_var_40) */
static void Ridley_Func_53(void) {  // 0xA6BEDC
  uint16 v1 = RV(0x46) + 8;
  RV(0x46) = v1;
  uint16 t = HI8(RV(0x43)) + (v1 & 0xff);
  RV(0x43) = (RV(0x43) & 0xFF) | (t & 0xFF) << 8;
  RV(0x44) += (int8)(v1 >> 8) + (t >> 8);
  if (!sign16(RV(0x44) - 192)) {
    RV(0x44) = 192;
    RV(0x40) = 0xBF19;                            // locret_A6BF19
  }
}

static void Ridley_Func_49(void) {  // 0xA6BE93
  EnemyData *E = gEnemyData(0);
  switch (RV(0x40)) {
  case 0xBE9C: RV(0x42) = E->x_pos - 16; RV(0x44) = E->y_pos + 22; return;   // Ridley_Func_50
  case 0xBEB3: RV(0x42) = E->x_pos + 14; RV(0x44) = E->y_pos + 66; return;   // Ridley_Func_51
  case 0xBECA: RV(0x45) = 0; RV(0x46) = 0; RV(0x40) = 0xBEDC; Ridley_Func_53(); return;  // Ridley_Func_52
  case 0xBEDC: Ridley_Func_53(); return;
  case 0xBF19: return;
  default: nt_unimpl("Ridley_Func_49"); return;
  }
}

/* ------------------------------------------------------------------ IA */
void CeresRidley_Init(void) {  // 0xA6A0F5 (rama de Ceres)
  EnemyData *E = gEnemyData(0);
  if (boss_bits_for_area[area_index] & 1) {
    E->properties |= 0x400 | 0x200 | 0x100;
    return;
  }
  for (int i = 0; i < 2048; i++)
    TS[i] = 0;
  DisableMinimapAndMarkBossRoomAsExplored();
  E->parameter_1 = 0;
  E->parameter_2 = 0;
  Ridley_Func_99(0xE538);
  E->palette_index = 3584;
  RV(0x0C) = 3584;
  E->extra_properties |= 4;
  RV(0x01) = 0;
  RV(0x0D) = 0;
  earthquake_type = 0;
  earthquake_timer = 0;
  Ridley_Func_92();
  Ridley_Func_117();
  if (area_index == 2) { nt_unimpl("CeresRidley_Init (Norfair)"); return; }
  E->properties |= 0x1000 | 0x400;
  E->x_pos = 186;
  E->y_pos = 169;
  ceres_status = 0;
  RV(0x10) = 0;
  RV(0x02) = 0;
  TS[1] = 1;
  RV(0x1C) = 15;
  E->ai_var_A = 0xA35B;                           // CeresRidley_Func_3
  E->ai_var_B = 0;
  E->ai_var_C = 0;
  RV(0x20) = -32;
  RV(0x21) = 176;
  RV(0x22) = 40;
  RV(0x23) = 224;
  for (int i = 0; i < 0x20; i++)                  // WriteColorsToTargetPalette($A6, 0x140, word_A6E16F, 0x20)
    target_palettes[160 + i] = A6W(0xE16F + 2 * i);
  for (int i = 0; i < 15; i++)
    target_palettes[241 + i] = 0;
  RV(0x03) = 0xBF31;                              // kBabyMetroid_Ilist_BF31
  RV(0x04) = 1;
  RV(0x40) = 0xBE9C;                              // Ridley_Func_50
  RV(0x07) = 5;
  QueueMusic_Delayed8(0);
}

static void Ridley_A2F2(void) {  // 0xA6A2F2
  if (!ceres_status)
    DrawBabyMetroid_0();
  EnemyData *D = gEnemyData(0x40);
  if (D->ai_var_B) {
    /* el original suma -1024 con temblor (defecto del juego, se conserva) */
    static const int16 g_word_A6A321[4] = { 0, -1024, -4, -1 };
    DrawA6SpritemapScreen(0xA329, D->x_pos + g_word_A6A321[earthquake_timer & 3], D->y_pos, 1024);
  }
}

/* enemy_gfx_drawn_hook ($A6:A2F2): se llama después de dibujar los enemigos (DrawSamusEnemiesAndProjectiles) */
uint8 nt_enemy_gfx_drawn_hook;
uint16 nt_gfx_hook_addr; uint8 nt_gfx_hook_bank;   /* el gancho de los enemigos traducidos (docs/42): $bank:addr */
void CallEnemyGfxDrawnHook(void) {
  if (nt_enemy_gfx_drawn_hook)
    Ridley_A2F2();
  else if (nt_gfx_hook_bank) {
    extern uint8 port_call_void(uint32 ea);
    port_call_void((uint32)nt_gfx_hook_bank << 16 | nt_gfx_hook_addr);
  }
}

static void Ridley_A2DC(void) {  // 0xA6A2DC
  Ridley_Func_49();
  if (RV(0x02))
    nt_enemy_gfx_drawn_hook = 1;                  // mov24(&enemy_gfx_drawn_hook, 0xA6A2F2)
  else
    Ridley_A2F2();
}

void CeresRidley_Main(void) {  // 0xA6A288
  EnemyData *E = gEnemyData(0);
  E->health = 0x7FFF;
  CallRidleyFunc(E->ai_var_A);
  if (!ceres_status) {
    if (RV(0x02)) {
      Ridley_Func_102();
      Ridley_Func_112();
      Ridley_Func_115();
      CeresRidley_Func_1();
      Ridley_Func_70();
      Ridley_Func_101();
      Ridley_Func_118();
      sub_A6DB2A();
      Ridley_Func_120();
    }
    Ridley_A2DC();
  }
}

void CeresRidley_Hurt(void) {  // 0xA6A2D3
  Ridley_Func_102();
  sub_A6DB2A();
  Ridley_Func_120();
  Ridley_A2DC();
}

/* ------------------------------------------------------------------ disparos, contacto */
void Ridley_Shot(void) {  // 0xA6DF8A (rama de Ceres)
  if (area_index == 2) { nt_unimpl("Ridley_Shot (Norfair)"); return; }
  EnemyData *E = gEnemyData(0);
  uint16 v0 = 13;
  if (E->flash_timer && (E->flash_timer & 1))
    v0 = 14;
  E->flash_timer = v0;
  ++RV(0x0D);
}

void Ridley_Powerbomb(void) {  // 0xA6DFB2
  nt_unimpl("Ridley_Powerbomb");
}

/* ------------------------------------------------------------------ instrucciones ($A6:E4BE-E976) */
/* devuelve la nueva posición de la lista; *stop = 1 si la lista se detiene */
uint16 RidleyInstr(uint16 f, uint16 jp, int *ok) {
  EnemyData *E = gEnemyData(0);
  *ok = 1;
  switch (f) {
  case 0xE4BE: RV(0x0F) = 89; QueueSfx2_Max6(0x59); return jp;            // Ridley_Instr_5
  case 0xE4CA: RV(0x0F) = 0; return jp;                                    // Ridley_Instr_6
  case 0xE4D2:                                                             // Ridley_Instr_10
    if (area_index == 2 || !sign16(samus_health - 30))
      return jp + 2;
    RV(0x00) = 8;
    return A6W(jp);
  case 0xE4EE: return RV(0x1B) ? A6W(jp) : A6W(jp + 2);                   // Ridley_Instr_4
  case 0xE4F8: return RV(0x1B) ? jp + 2 : A6W(jp);                        // Ridley_Instr_3
  case 0xE501:                                                             // Ridley_Instr_2
    samus_prev_x_pos = samus_x_pos;
    samus_prev_y_pos = samus_y_pos;
    RV(0x1D) = A6W(jp);
    return jp + 2;
  case 0xE517: return RV(0x10) ? A6W(jp) : jp + 2;                        // Ridley_Instr_1
  case 0xE51F:                                                             // Ridley_Instr_14
    E->x_pos += A6W(jp);
    E->y_pos += A6W(jp + 2);
    return jp + 4;
  case 0xE71C: RV(0x10) = 0; Ridley_Func_96(); return jp;                 // Ridley_Instr_9
  case 0xE727: RV(0x10) = 1; return jp;                                    // Ridley_Instr_7
  case 0xE72F: RV(0x10) = 2; Ridley_Func_96(); return jp;                 // Ridley_Instr_8
  case 0xE84D: {                                                           // Ridley_Instr_11: apunta la bola de fuego
    uint16 r18;
    if (RV(0x10)) {
      uint16 v4 = (uint8)-(CalculateAngleFromXY(samus_x_pos - (E->x_pos + 25), samus_y_pos - (E->y_pos - 43)) + 0x80);
      if (v4 < 0x50) {
        if (v4 < 0x15) v4 = 21;
      } else if (v4 < 0xC0) {
        v4 = 80;
      } else {
        v4 = 21;
      }
      r18 = v4;
    } else {
      uint16 v3 = (uint8)-(CalculateAngleFromXY(samus_x_pos - (E->x_pos - 25), samus_y_pos - (E->y_pos - 43)) + 0x80);
      if (v3 >= 0xB0) {
        if (v3 >= 0xEB) v3 = 235;
      } else if (v3 < 0x40) {
        v3 = 235;
      } else {
        v3 = 176;
      }
      r18 = v3;
    }
    RV(0x19) = Math_MultBySin(0x500, r18);
    RV(0x1A) = Math_MultByCos(0x500, r18);
    return jp;
  }
  case 0xE904: eproj_unk1995 = 0; SpawnEprojWithRoomGfx(0x9642, RV(0x10)); return jp;     // Ridley_Instr_12
  case 0xE909: eproj_unk1995 = 0xe; SpawnEprojWithRoomGfx(0x9642, RV(0x10)); return jp;   // Ridley_Instr_13
  case 0xE969: E->ai_var_A = 0xA6AF; E->ai_var_C = -352; return jp;       // Ridley_Instr_15 (CeresRidley_Func_9)
  case 0xE976: E->ai_var_A = 0xB2F3; E->ai_var_C = -352; return jp;       // Ridley_Instr_16 (Ridley_Func_3)
  default: *ok = 0; nt_unimpl("RidleyInstr (instrucción sin portar)"); return jp;
  }
}
