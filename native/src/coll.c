/* MegaMetroid — motor nativo: colisión con los bloques de la sala (banco $94 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_94.c. level_data y BTS son arreglos nativos de la sala actual
 * (room.h: el nivel ya preparado en el PC, con los cambios de los PLMs). Los bloques especiales (disparables,
 * bombas, púas, aire especial) crean PLMs como en el SNES (plm.c). */
#include "samus.h"
#include "room.h"
#include "plm.h"
#include "game.h"
#include "nmath.h"
#include "../gen/enums.h"

/* tablas de PLMs del banco $94 (palabras nativas) */
static inline uint16 B94W(uint16 a) { return kBank94_React[(uint16)(a - kBank94_React_LO)]; }
#define kPlmHeaderDefPtrs(i) B94W(0x9139 + 2 * (i))                 /* bloque especial */
#define off_94936B(i) B94W(0x936B + 2 * (i))                        /* bombas */
#define off_9492D9(a) B94W(0x92D9 + 2 * (a))                         /* aire especial por área */
#define off_9492E9(a) B94W(0x92E9 + 2 * (a))                         /* bloque especial por área */
#define kBlockShotBombedReactionShootablePlm(i) B94W(0x9EA6 + 2 * (i))
/* la cantidad de la colisión en curso (el decomp: cur_coll_amt32): la usa la arena de Maridia (plm.c) */
int32 *nt_cur_coll_amt;

static inline uint16 Mult8x8(uint8 a, uint8 b) { return (uint16)a * b; }
static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }
static inline uint16 SnesModulus(uint16 a, uint8 b) { return b ? (uint16)(DivMod16(a, b) >> 16) : a; }
static inline uint16 SnesDivide(uint16 a, uint8 b) { return b ? (uint16)DivMod16(a, b) : 0xffff; }

typedef struct {
  int32 ci_r18_r20;
  uint16 ci_r24, ci_r26, ci_r28, ci_r30, ci_r32, ci_r38, ci_r40;
} CollInfo;

/* ------------------------------------------------------------------ pendientes */
static uint8 BlockColl_Horiz_Slope_NonSquare(CollInfo *ci) {  // 0x9484D6
  if ((current_slope_bts & 0x80) || PAIR32(samus_y_speed, samus_y_subspeed))
    return 0;
  uint16 v1 = 4 * (current_slope_bts & 0x1F);
  uint16 v2 = (uint16)(ci->ci_r18_r20 >> 8);
  if (ci->ci_r18_r20 >= 0)
    ci->ci_r18_r20 = (int32)((uint32)v2 * kBlockColl_Horiz_Slope_NonSquare_Tab[(v1 >> 1) + 1]);
  else
    ci->ci_r18_r20 = -(int32)((uint32)(uint16)-v2 * kBlockColl_Horiz_Slope_NonSquare_Tab[(v1 >> 1) + 1]);
  return 0;
}

static uint8 BlockColl_Vert_Slope_NonSquare(CollInfo *ci, uint16 k) {  // 0x9486FE
  if ((samus_x_pos >> 4) != SnesModulus(cur_block_index, room_width_in_blocks))
    return 0;
  uint8 b = bts_get(k);
  if (samus_collision_direction & 1) {
    if (b & 0x80)
      return 0;
    uint16 v10 = (b & 0x40) ? samus_x_pos ^ 0xF : samus_x_pos;
    uint16 v11 = 16 * (b & 0x1F) + (v10 & 0xF);
    int16 v12 = (kAlignYPos_Tab0[v11] & 0x1F) - ((samus_y_radius + ci->ci_r24 - 1) & 0xF) - 1;
    if (v12 > 0)
      return 0;
    int16 v13 = (int16)(ci->ci_r18_r20 >> 16) + v12;
    if (v13 < 0)
      v13 = 0;
    ci->ci_r18_r20 = SHL16(v13);
    return 1;
  }
  if (!(b & 0x80))
    return 0;
  uint16 v4 = (b & 0x40) ? (samus_x_pos ^ 0xF) : samus_x_pos;
  uint16 v5 = 16 * (b & 0x1F) + (v4 & 0xF);
  int16 v6 = (kAlignYPos_Tab0[v5] & 0x1F) - (((ci->ci_r24 - samus_y_radius) & 0xF) ^ 0xF) - 1;
  if (v6 > 0)
    return 0;
  int16 v7 = (int16)(ci->ci_r18_r20 >> 16) + v6;
  if (v7 < 0)
    v7 = 0;
  ci->ci_r18_r20 = SHL16(v7);
  return 1;
}

void Samus_AlignYPosSlope(void) {  // 0x9487F4
  if (!(enable_horiz_slope_coll & 2))
    return;
  CalculateBlockAt(samus_x_pos, samus_y_radius + samus_y_pos - 1, 0, 0);
  if ((level_get(cur_block_index) & 0xF000) == 0x1000 && (bts_get(cur_block_index) & 0x1F) >= 5) {
    uint16 dd4 = (samus_y_radius + samus_y_pos - 1) & 0xF;
    uint16 dd6 = 16 * (bts_get(cur_block_index) & 0x1F);
    uint8 v0 = bts_get(cur_block_index);
    if (!(v0 & 0x80)) {
      uint16 v1 = (v0 & 0x40) ? samus_x_pos ^ 0xF : samus_x_pos;
      uint16 v2 = (kAlignYPos_Tab0[dd6 + (v1 & 0xF)] & 0x1F) - dd4 - 1;
      if ((int16)v2 < 0) {
        samus_y_pos += v2;
        samus_pos_adjusted_by_slope_flag = 1;
      }
    }
  }
  CalculateBlockAt(samus_x_pos, samus_y_pos - samus_y_radius, 0, 0);
  if ((level_get(cur_block_index) & 0xF000) == 0x1000 && (bts_get(cur_block_index) & 0x1F) >= 5) {
    uint16 dd4 = ((samus_y_pos - samus_y_radius) & 0xF) ^ 0xF;
    uint16 dd6 = 16 * (bts_get(cur_block_index) & 0x1F);
    uint8 v3 = bts_get(cur_block_index);
    if (v3 & 0x80) {
      uint16 v4 = (v3 & 0x40) ? samus_x_pos ^ 0xF : samus_x_pos;
      uint16 v6 = (kAlignYPos_Tab0[dd6 + (v4 & 0xF)] & 0x1F) - dd4 - 1;
      if ((int16)v6 <= 0) {
        samus_y_pos -= v6;
        samus_pos_adjusted_by_slope_flag = 1;
      }
    }
  }
}

static uint8 BlockColl_Horiz_Slope_Square(CollInfo *ci, uint16 a, uint16 k) {  // 0x948D2B
  uint16 v2 = 4 * a + ((bts_get(k) >> 6) ^ ((ci->ci_r32 & 8) >> 3));
  bool hit;
  if (!ci->ci_r26) {
    if (!((samus_y_radius + samus_y_pos - 1) & 8))
      hit = kTab948E54[v2] != 0;
    else
      hit = kTab948E54[v2] || kTab948E54[v2 ^ 2];
  } else if (ci->ci_r26 != ci->ci_r28 || !((samus_y_pos - samus_y_radius) & 8)) {
    hit = kTab948E54[v2] || kTab948E54[v2 ^ 2];
  } else {
    hit = kTab948E54[v2 ^ 2] != 0;
  }
  if (!hit)
    return 0;
  if (ci->ci_r18_r20 < 0) {
    int16 v5 = samus_x_radius + (ci->ci_r32 | 7) + 1 - samus_x_pos;
    if (v5 >= 0)
      v5 = 0;
    ci->ci_r18_r20 = SHL16(v5);
    samus_x_subpos = 0;
  } else {
    int16 v4 = (ci->ci_r32 & 0xFFF8) - samus_x_radius - samus_x_pos;
    if (v4 < 0)
      v4 = 0;
    ci->ci_r18_r20 = SHL16(v4);
    samus_x_subpos = 0xFFFF;
  }
  return 1;
}

static uint8 BlockColl_Vert_Slope_Square(CollInfo *ci, uint16 a, uint16 k) {  // 0x948DBD
  uint16 v2 = 4 * a + ((bts_get(k) >> 6) ^ ((ci->ci_r32 & 8) >> 2));
  bool hit;
  if (!ci->ci_r26) {
    if (!((samus_x_radius + samus_x_pos - 1) & 8))
      hit = kTab948E54[v2] != 0;
    else
      hit = kTab948E54[v2] || kTab948E54[v2 ^ 1];
  } else if (ci->ci_r26 != ci->ci_r28 || !((samus_x_pos - samus_x_radius) & 8)) {
    hit = kTab948E54[v2] || kTab948E54[v2 ^ 1];
  } else {
    hit = kTab948E54[v2 ^ 1] != 0;
  }
  if (!hit)
    return 0;
  if (ci->ci_r18_r20 < 0) {
    int16 v5 = samus_y_radius + (ci->ci_r32 | 7) + 1 - samus_y_pos;
    if (v5 >= 0)
      v5 = 0;
    ci->ci_r18_r20 = SHL16(v5);
    samus_y_subpos = 0;
    return 1;
  }
  int16 v4 = (ci->ci_r32 & 0xFFF8) - samus_y_radius - samus_y_pos;
  if (v4 < 0)
    v4 = 0;
  ci->ci_r18_r20 = SHL16(v4);
  samus_y_subpos = 0xFFFF;
  samus_pos_adjusted_by_slope_flag = 1;
  return 1;
}

static uint8 BlockColl_Horiz_Solid(CollInfo *ci) {  // 0x948F49
  if (ci->ci_r18_r20 < 0) {
    int16 v2 = samus_x_radius + (ci->ci_r32 | 0xF) + 1 - samus_x_pos;
    if (v2 >= 0)
      v2 = 0;
    ci->ci_r18_r20 = SHL16(v2);
    samus_x_subpos = 0;
  } else {
    int16 v0 = (ci->ci_r32 & 0xFFF0) - samus_x_radius - samus_x_pos;
    if (v0 < 0)
      v0 = 0;
    ci->ci_r18_r20 = SHL16(v0);
    samus_x_subpos = 0xFFFF;
  }
  return 1;
}

static uint8 BlockColl_Vert_Solid(CollInfo *ci) {  // 0x948F82
  if (ci->ci_r18_r20 < 0) {
    int16 v2 = samus_y_radius + (ci->ci_r32 | 0xF) + 1 - samus_y_pos;
    if (v2 >= 0)
      v2 = 0;
    ci->ci_r18_r20 = SHL16(v2);
    samus_y_subpos = 0;
  } else {
    int16 v0 = (ci->ci_r32 & 0xFFF0) - samus_y_radius - samus_y_pos;
    if (v0 < 0)
      v0 = 0;
    ci->ci_r18_r20 = SHL16(v0);
    samus_y_subpos = 0xFFFF;
  }
  return 1;
}

static uint8 BlockColl_Horiz_Slope(CollInfo *ci) {  // 0x948FBB
  uint16 v0 = bts_get(cur_block_index) & 0x1F;
  if (v0 < 5)
    return BlockColl_Horiz_Slope_Square(ci, v0, cur_block_index);
  current_slope_bts = bts_get(cur_block_index);
  return BlockColl_Horiz_Slope_NonSquare(ci);
}

static uint8 BlockColl_Vert_Slope(CollInfo *ci) {  // 0x948FDA
  uint16 v0 = bts_get(cur_block_index) & 0x1F;
  if (v0 < 5)
    return BlockColl_Vert_Slope_Square(ci, v0, cur_block_index);
  current_slope_bts = bts_get(cur_block_index);
  return BlockColl_Vert_Slope_NonSquare(ci, cur_block_index);
}

/* puertas ($94:938B): la definición de la puerta viene ya preparada (room.c) */
static uint8 BlockColl_Door(CollInfo *ci, bool vert) {
  door_transition_function = FUNC16(DoorTransitionFunction_HandleElevator);
  uint16 v0 = room_door_def_ptr(bts_get(cur_block_index) & 0x7F);
  if (!(door_def_room_ptr(v0) & 0x8000)) {
    if (samus_pose < 9)
      elevator_flags = 1;
    return vert ? BlockColl_Vert_Solid(ci) : BlockColl_Horiz_Solid(ci);
  }
  door_def_ptr = v0;
  game_state = 9;
  return 0;
}

static uint8 BlockReact_HorizExt(void) {  // 0x949411
  if (bts_get(cur_block_index)) {
    cur_block_index += (int8)bts_get(cur_block_index);
    return 0xff;
  }
  return 0;
}

static uint8 BlockReact_VertExt(void) {  // 0x949447
  if (bts_get(cur_block_index)) {
    cur_block_index += Mul16((int8)bts_get(cur_block_index), room_width_in_blocks);
    return 0xff;
  }
  return 0;
}

static uint16 Samus_GetYposSpan(void) {  // 0x949495
  uint16 r26 = (samus_y_pos - samus_y_radius) & 0xFFF0;
  return (uint16)(samus_y_radius + samus_y_pos - 1 - r26) >> 4;
}

static uint16 Samus_GetXposSpan(void) {  // 0x9494B5
  uint16 r26 = (samus_x_pos - samus_x_radius) & 0xFFF0;
  return (uint16)(samus_x_radius + samus_x_pos - 1 - r26) >> 4;
}

/* ---- púas, especiales, bombas ($94:8E83...) */
static void SpikeHurt(uint16 dmg) {
  if (!samus_invincibility_timer) {
    samus_invincibility_timer = 60;
    samus_knockback_timer = 10;
    samus_periodic_damage += dmg;
    knockback_x_dir = ((samus_pose_x_dir ^ 0xC) & 8) != 0;
  }
}
static void BlockColl_SpikeBlock(void) {  // kBlockColl_SpikeBlock ($94:904B)
  switch (bts_get(cur_block_index)) {
  case 0: if (area_index != 3 || (CheckBossBitForCurArea(1) & 1)) SpikeHurt(60); break;   // 0x948E83
  case 1: case 3: SpikeHurt(16); break;                                                     // 0x948ECF / 0x948F0A
  default: break;
  }
}
static uint8 BlockColl_SpecialAir(void) {  // 0x94906F / 0x94909D
  uint8 v0 = bts_get(cur_block_index);
  if (v0 & 0x80)
    return SpawnPLM(B94W(off_9492D9(area_index) + 2 * (v0 & 0x7F))) & 1;
  SpawnPLM(kPlmHeaderDefPtrs(v0));
  return 0;
}
static uint8 BlockColl_SpecialBlock(CollInfo *ci, bool vert) {  // 0x9490CB / 0x949102
  uint8 v0 = bts_get(cur_block_index);
  uint16 h = (v0 & 0x80) ? B94W(off_9492E9(area_index) + 2 * (v0 & 0x7F)) : kPlmHeaderDefPtrs(v0);
  if (SpawnPLM(h) & 1)
    return vert ? BlockColl_Vert_Solid(ci) : BlockColl_Horiz_Solid(ci);
  return 0;
}
static uint8 BlockColl_BombBlock(CollInfo *ci, bool vert) {  // 0x94932D / 0x94934C
  uint8 v0 = bts_get(cur_block_index);
  if ((v0 & 0x80) || (SpawnPLM(off_94936B(v0)) & 1))
    return vert ? BlockColl_Vert_Solid(ci) : BlockColl_Horiz_Solid(ci);
  return 0;
}

static uint8 BlockColl_CheckColl(CollInfo *ci, uint16 k, bool vert) {  // 0x949515 / 0x94952C
  uint8 rv;
  cur_block_index = k >> 1;
  nt_cur_coll_amt = &ci->ci_r18_r20;
  do {
    switch ((level_block(cur_block_index) & 0xF000) >> 12) {
    case 0: case 2: case 4: case 6: rv = 0; break;                /* aire (con púas: nada al chocar) */
    case 1: rv = vert ? BlockColl_Vert_Slope(ci) : BlockColl_Horiz_Slope(ci); break;
    case 3: rv = BlockColl_SpecialAir(); break;
    case 5: rv = BlockReact_HorizExt(); break;
    case 7:                                                       // aire que se rompe con bombas
      if (!(bts_get(cur_block_index) & 0x80)) SpawnPLM(off_94936B(bts_get(cur_block_index)));
      rv = 0;
      break;
    case 8: case 12: case 14: rv = vert ? BlockColl_Vert_Solid(ci) : BlockColl_Horiz_Solid(ci); break;
    case 9: rv = BlockColl_Door(ci, vert); break;
    case 10: BlockColl_SpikeBlock(); rv = vert ? BlockColl_Vert_Solid(ci) : BlockColl_Horiz_Solid(ci); break;
    case 11: rv = BlockColl_SpecialBlock(ci, vert); break;
    case 13: rv = BlockReact_VertExt(); break;
    default: rv = BlockColl_BombBlock(ci, vert); break;           // 15
    }
  } while (rv & 0x80);
  nt_cur_coll_amt = 0;
  return rv;
}

static Pair_Bool_Amt BlockColl_Handle_Horiz(int32 amt) {  // 0x949543
  uint16 r28 = Samus_GetYposSpan();
  uint16 prod = Mult8x8((uint16)(samus_y_pos - samus_y_radius) >> 4, room_width_in_blocks);
  uint16 v0 = (uint16)((amt + (int32)PAIR32(samus_x_pos, samus_x_subpos)) >> 16);
  uint16 v1 = (amt >= 0) ? samus_x_radius + v0 - 1 : v0 - samus_x_radius;
  uint16 v2 = 2 * (prod + (v1 >> 4));
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = v0, .ci_r26 = r28, .ci_r28 = r28, .ci_r32 = v1 };
  while (!(BlockColl_CheckColl(&ci, v2, false) & 1)) {
    v2 += room_width_in_blocks * 2;
    if (--ci.ci_r26 & 0x8000)
      return (Pair_Bool_Amt){ false, ci.ci_r18_r20 };
  }
  return (Pair_Bool_Amt){ true, ci.ci_r18_r20 };
}

static Pair_Bool_Amt BlockColl_Handle_Vert_LeftToRight(int32 amt) {  // 0x94959E
  uint16 r28 = Samus_GetXposSpan();
  uint16 v0 = (uint16)((amt + (int32)PAIR32(samus_y_pos, samus_y_subpos)) >> 16);
  uint16 v1 = (amt >= 0) ? samus_y_radius + v0 - 1 : v0 - samus_y_radius;
  uint16 prod = Mult8x8(v1 >> 4, room_width_in_blocks);
  cur_block_index = prod + ((uint16)(samus_x_pos - samus_x_radius) >> 4);
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = v0, .ci_r26 = r28, .ci_r28 = r28, .ci_r32 = v1 };
  for (uint16 i = 2 * cur_block_index; !(BlockColl_CheckColl(&ci, i, true) & 1); i += 2) {
    if (--ci.ci_r26 & 0x8000)
      return (Pair_Bool_Amt){ false, ci.ci_r18_r20 };
  }
  return (Pair_Bool_Amt){ true, ci.ci_r18_r20 };
}

static Pair_Bool_Amt BlockColl_Handle_Vert_RightToLeft(int32 amt) {  // 0x9495F5
  uint16 r28 = Samus_GetXposSpan();
  uint16 v0 = (uint16)((amt + (int32)PAIR32(samus_y_pos, samus_y_subpos)) >> 16);
  uint16 v1 = (amt >= 0) ? samus_y_radius + v0 - 1 : v0 - samus_y_radius;
  uint16 prod = Mult8x8(v1 >> 4, room_width_in_blocks);
  cur_block_index = prod + ((uint16)(samus_x_radius + samus_x_pos - 1) >> 4);
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = v0, .ci_r26 = 0, .ci_r28 = r28, .ci_r32 = v1 };
  for (uint16 i = 2 * cur_block_index; !(BlockColl_CheckColl(&ci, i, true) & 1); i -= 2) {
    if (ci.ci_r28 < ++ci.ci_r26)
      return (Pair_Bool_Amt){ false, ci.ci_r18_r20 };
  }
  return (Pair_Bool_Amt){ true, ci.ci_r18_r20 };
}

int32 WallJumpBlockCollDetect(int32 amt) {  // 0x94967F
  samus_collision_direction |= 0xF;
  flag_samus_in_quicksand = 0;
  Pair_Bool_Amt pair = BlockColl_Handle_Horiz(amt);
  samus_collision_flag = pair.flag;
  return pair.amt >= 0 ? pair.amt : -pair.amt;
}

static int32 CollDetectDueToPoseChange_SingleBlock(int32 amt) {  // 0x9496E3
  samus_collision_direction |= 0xF;
  flag_samus_in_quicksand = 0;
  Pair_Bool_Amt pair = !(nmi_frame_counter_word & 1) ? BlockColl_Handle_Vert_LeftToRight(amt) : BlockColl_Handle_Vert_RightToLeft(amt);
  samus_collision_flag = pair.flag;
  return pair.amt >= 0 ? pair.amt : -pair.amt;
}

int32 Samus_CollDetectChangedPose(int32 amt) {  // 0x9496AB
  if (!(abs16((uint16)(amt >> 16)) & 0xFFF8))
    return CollDetectDueToPoseChange_SingleBlock(amt);
  int32 amt_backup = amt;
  amt = CollDetectDueToPoseChange_SingleBlock(SHL16(((uint16)(amt >> 16) & 0xFFF0) | 8));
  if (samus_collision_flag)
    return amt;
  return CollDetectDueToPoseChange_SingleBlock(amt_backup);
}

int32 Samus_MoveRight_NoSolidColl(int32 amt) {  // 0x94971E
  if (amt != 0) {
    flag_samus_in_quicksand = 0;
    Pair_Bool_Amt pair = BlockColl_Handle_Horiz(amt);
    amt = pair.amt;
    samus_collision_flag = pair.flag;
  } else {
    samus_collision_flag = 0;
  }
  add_hilo(&samus_x_pos, &samus_x_subpos, amt);
  return amt;
}

int32 Samus_MoveDown_NoSolidColl(int32 amt) {  // 0x949763
  if (amt) {
    samus_pos_adjusted_by_slope_flag = 0;
    flag_samus_in_quicksand = 0;
    Pair_Bool_Amt pair = !(nmi_frame_counter_word & 1) ? BlockColl_Handle_Vert_LeftToRight(amt) : BlockColl_Handle_Vert_RightToLeft(amt);
    amt = pair.amt;
    if (pair.flag) {
      samus_collision_flag = 1;
      add_hilo(&samus_y_pos, &samus_y_subpos, amt);
      return amt;
    }
  }
  samus_collision_flag = (flag_samus_in_quicksand != 0);
  add_hilo(&samus_y_pos, &samus_y_subpos, amt);
  return amt;
}

/* ------------------------------------------------------------------ Samus dentro de un bloque ($94:9B60) */
static void BlockInsideReact_SpecialAir(void) {
  uint8 v0 = bts_get(cur_block_index);
  if (v0 & 0x80) {                                  /* g_off_949B06: tablas por área ($94:9A06 + 32 * área) */
    SpawnPLM(B94W(0x9A06 + 0x20 * area_index + 2 * (v0 & 0x7F)));
    return;
  }
  switch (v0) {                                     /* off_949966 */
  case 8: case 9:                                   // cintas transportadoras (0x9498EA / 0x949910)
    if ((area_index != 3 || (CheckBossBitForCurArea(1) & 1)) && !samus_y_speed) {
      extra_samus_x_subdisplacement = 0;
      extra_samus_x_displacement = v0 == 8 ? 2 : -2;
    }
    break;
  case 10: case 11:                                 // 0x949936 / 0x949946
    extra_samus_x_subdisplacement = 0;
    extra_samus_x_displacement = v0 == 10 ? 2 : -2;
    break;
  case 70:                                          // 0x949956: bloque de scroll al tocarlo
    if (inside_block_reaction_samus_point == 1)
      SpawnPLM(0xB6FF);                             // addr_kPlmHeader_B6FF
    return;
  }
  samus_x_speed_table_pointer = addr_kSamusSpeedTable_Normal_X + 12;
}

static uint8 BlockInsideReact(void) {
  uint8 type = (HI8(level_block(cur_block_index)) & 0xF0) >> 4;
  switch (type) {
  case 1: return 0;                            // pendiente
  case 2:                                      // aire con púas (0x949866: BTS 2)
    if (bts_get(cur_block_index) == 2) {
      if (!samus_contact_damage_index && !samus_invincibility_timer) {
        samus_invincibility_timer = 60;
        samus_knockback_timer = 10;
        samus_periodic_damage += 16;
        knockback_x_dir = ((samus_pose_x_dir ^ 0xC) & 8) != 0;
      }
      samus_x_speed_table_pointer = addr_kSamusSpeedTable_Normal_X + 12;
    }
    return 0;
  case 3: BlockInsideReact_SpecialAir(); return 0;
  case 5: return BlockReact_HorizExt();
  case 13: return BlockReact_VertExt();
  default:                                     // BlockInsideReact_ShootableAir / BlockInsideReact_Special_
    samus_x_speed_table_pointer = addr_kSamusSpeedTable_Normal_X + 12;
    return 0;
  }
}

void BlockInsideDetection(void) {  // 0x949B60
  samus_x_decel_mult = 0;
  samus_x_decel_mult_hi = 0;
  extra_samus_x_subdisplacement = 0;
  extra_samus_x_displacement = 0;
  extra_samus_y_subdisplacement = 0;
  extra_samus_y_displacement = 0;
  inside_block_reaction_samus_point = 0;
  uint16 bottom = samus_y_radius + samus_y_pos - 1;
  CalculateBlockAt(samus_x_pos, bottom, 0, 0);
  while (BlockInsideReact() & 0x80) {}
  inside_block_reaction_samus_point = 1;
  if ((bottom ^ samus_y_pos) & 0xFFF0) {
    CalculateBlockAt(samus_x_pos, samus_y_pos, 0, 0);
    while (BlockInsideReact() & 0x80) {}
  }
  inside_block_reaction_samus_point = 2;
  uint16 top = samus_y_pos - samus_y_radius;
  if (((bottom ^ top) & 0xFFF0) && ((samus_y_pos ^ ((bottom ^ top) & 0xFFF0)) & 0xFFF0)) {
    CalculateBlockAt(samus_x_pos, top, 0, 0);
    while (BlockInsideReact() & 0x80) {}
  }
}

void CalculateBlockAt(uint16 r26, uint16 r28, uint16 r30, uint16 r32) {  // 0x949C1D
  int16 v0 = r30 + r26, v1 = r32 + r28;
  if (v0 >= 0 && sign16(v0 - 4096) && v1 >= 0 && sign16(v1 - 4096))
    cur_block_index = ((uint16)(v0 & 0xFFF0) >> 4) + Mult8x8((uint16)(v1 & 0xFFF0) >> 4, room_width_in_blocks);
  else
    cur_block_index = 0xFFFF;
}

/* ------------------------------------------------------------------ proyectiles contra bloques ($94:A1B5) */
static uint8 BlockShotReact_Slope_NonSquare(CollInfo *ci, uint16 j, uint16 k) {  // 0x94A58F
  uint16 v2 = (projectile_x_pos[j >> 1] & 0xf) ^ ((bts_get(k) & 0x40) ? 0xf : 0);
  uint16 v4 = (projectile_y_pos[j >> 1] & 0xf) ^ ((bts_get(k) & 0x80) ? 0xf : 0);
  uint16 v5 = kAlignYPos_Tab0[16 * (bts_get(k) & 0x1F) + (v2 & 0xF)] & 0x1F;
  if ((int16)(v5 - v4) <= 0) {
    ci->ci_r38 = 0;
    ci->ci_r40 = 0;
    return 1;
  }
  return 0;
}

static uint8 BlockShotReactVert_Slope_Square(CollInfo *ci, uint16 a, uint16 k) {  // 0x94A66A
  uint16 v2 = 4 * a + ((bts_get(k) >> 6) ^ ((ci->ci_r28 & 8) >> 3));
  int v3 = projectile_index >> 1;
  if (!ci->ci_r30) {
    if (!ci->ci_r26) {
      if (((projectile_y_pos[v3] - projectile_y_radius[v3]) & 8) || !kTab948E54[v2]) {
        if (!((LO8(projectile_y_radius[v3]) + LO8(projectile_y_pos[v3]) - 1) & 8) || !kTab948E54[v2 ^ 2])
          return 0;
      }
      return 1;
    }
    if (ci->ci_r38) {
      if (ci->ci_r38 == ci->ci_r26 && ((projectile_y_pos[v3] - projectile_y_radius[v3]) & 8))
        return kTab948E54[v2 ^ 2] != 0;
    } else if (!((LO8(projectile_y_radius[v3]) + LO8(projectile_y_pos[v3]) - 1) & 8)) {
      return kTab948E54[v2] != 0;
    }
    if (kTab948E54[v2])
      return 1;
    return kTab948E54[v2 ^ 2] != 0;
  }
  if (projectile_y_pos[v3] & 8)
    v2 ^= 2;
  return kTab948E54[v2] != 0;
}

static uint8 BlockShotReactHoriz_Slope_Square(CollInfo *ci, uint16 a, uint16 k) {  // 0x94A71A
  uint16 v2 = 4 * a + ((bts_get(k) >> 6) ^ ((ci->ci_r28 & 8) >> 2));
  int v3 = projectile_index >> 1;
  if (!ci->ci_r30) {
    if (!ci->ci_r26) {
      if (((projectile_x_pos[v3] - projectile_x_radius[v3]) & 8) || !kTab948E54[v2]) {
        if (!((LO8(projectile_x_radius[v3]) + LO8(projectile_x_pos[v3]) - 1) & 8) || !kTab948E54[v2 ^ 1])
          return 0;
      }
      return 1;
    }
    if (ci->ci_r38) {
      if (ci->ci_r38 == ci->ci_r26 && ((projectile_x_pos[v3] - projectile_x_radius[v3]) & 8))
        return kTab948E54[v2 ^ 1] != 0;
    } else if (!((LO8(projectile_x_radius[v3]) + LO8(projectile_x_pos[v3]) - 1) & 8)) {
      return kTab948E54[v2] != 0;
    }
    if (kTab948E54[v2])
      return 1;
    return kTab948E54[v2 ^ 1] != 0;
  }
  if (projectile_x_pos[v3] & 8)
    v2 ^= 1;
  return kTab948E54[v2] != 0;
}

/* (el decomp cruza los nombres: la reacción horizontal usa las funciones "Vert" y al revés) */
static uint8 BlockShotReactHoriz_Slope(CollInfo *ci) {  // 0x94A147
  if ((bts_get(cur_block_index) & 0x1F) < 5)
    return BlockShotReactVert_Slope_Square(ci, bts_get(cur_block_index) & 0x1F, cur_block_index);
  uint16 div = SnesDivide(cur_block_index, room_width_in_blocks);   // 0x94A543
  if ((projectile_y_pos[projectile_index >> 1] >> 4) == div)
    return BlockShotReact_Slope_NonSquare(ci, projectile_index, cur_block_index);
  return 0;
}

static uint8 BlockShotReactVert_Slope(CollInfo *ci) {  // 0x94A15E
  if ((bts_get(cur_block_index) & 0x1F) < 5)
    return BlockShotReactHoriz_Slope_Square(ci, bts_get(cur_block_index) & 0x1F, cur_block_index);
  uint16 mod = SnesModulus(cur_block_index, room_width_in_blocks);   // 0x94A569
  if ((projectile_x_pos[projectile_index >> 1] >> 4) == mod)
    return BlockShotReact_Slope_NonSquare(ci, projectile_index, cur_block_index);
  return 0;
}

static uint8 BlockReact_ShootableAir(CollInfo *ci) {  // 0x949E55
  uint8 v0 = bts_get(cur_block_index);
  if (!(v0 & 0x80)) {
    uint16 t = kBlockShotBombedReactionShootablePlm(v0);
    if (t == 0xB974) { ci->ci_r38 = 0; ci->ci_r40 = 0xFFFF; }
    SpawnPLM(t);
  }
  return 0;
}
static uint8 BlockReact_Shootable(CollInfo *ci) {  // 0x949E73
  uint8 v0 = bts_get(cur_block_index);
  if (v0 & 0x80) {
    SpawnPLM(B94W(B94W(0x9FC6 + 2 * area_index) + 2 * (v0 & 0x7F)));
  } else {
    uint16 t = kBlockShotBombedReactionShootablePlm(v0);
    if (t == 0xB974) { ci->ci_r38 = 0; ci->ci_r40 = 0xFFFF; }
    SpawnPLM(t);
  }
  return 1;
}
static uint8 BlockReact_Bombable(uint8 solid) {  // 0x949FD6 (aire) / 0x949FF4 (bloque)
  uint8 v0 = bts_get(cur_block_index);
  if (!(v0 & 0x80))
    SpawnPLM(B94W(0xA012 + 2 * v0));
  return solid;
}

static uint8 BlockShotReact(CollInfo *ci, uint16 k, bool vert) {  // 0x94A1B5 / 0x94A1D6
  uint8 v1;
  if (k >= room_size_in_blocks)
    return 1;
  cur_block_index = k >> 1;
  do {
    switch ((level_block(cur_block_index) & 0xF000) >> 12) {
    case 0: case 2: case 3: case 6: v1 = 0; break;
    case 1: v1 = vert ? BlockShotReactVert_Slope(ci) : BlockShotReactHoriz_Slope(ci); break;
    case 4: v1 = BlockReact_ShootableAir(ci); break;
    case 5: v1 = BlockReact_HorizExt(); break;
    case 7: v1 = BlockReact_Bombable(0); break;
    case 8: case 9: case 10: case 11: case 14: v1 = 1; break;
    case 12: v1 = BlockReact_Shootable(ci); break;
    case 13: v1 = BlockReact_VertExt(); break;
    default: v1 = BlockReact_Bombable(1); break;   // 15
    }
  } while (v1 & 0x80);
  if (v1)
    ci->ci_r40--;
  return v1;
}

static uint16 BlockGetSomePos1(uint16 k) {  // 0x94A1F7
  int v1 = k >> 1;
  uint16 r38 = (projectile_y_pos[v1] - projectile_y_radius[v1]) & 0xFFF0;
  return (uint16)(projectile_y_radius[v1] + projectile_y_pos[v1] - 1 - r38) >> 4;
}

static uint16 BlockGetSomePos2(uint16 k) {  // 0x94A219
  int v1 = k >> 1;
  uint16 r38 = (projectile_x_pos[v1] - projectile_x_radius[v1]) & 0xFFF0;
  return (uint16)(projectile_x_radius[v1] + projectile_x_pos[v1] - 1 - r38) >> 4;
}

uint8 BlockCollNoWaveBeamHoriz(uint16 k) {  // 0x94A23B
  int v1 = k >> 1;
  int32 amt = (int32)(int16)projectile_bomb_x_speed[v1] << 8;
  uint16 some_pos = BlockGetSomePos1(k);
  uint16 prod = Mult8x8((uint16)(projectile_y_pos[v1] - projectile_y_radius[v1]) >> 4, room_width_in_blocks);
  add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], amt);
  uint16 r24 = projectile_x_pos[v1];
  uint16 v7 = (amt < 0) ? projectile_x_pos[v1] - projectile_x_radius[v1] : projectile_x_radius[v1] + projectile_x_pos[v1] - 1;
  uint16 v8 = 2 * (prod + (v7 >> 4));
  if (!sign16(some_pos - 16) || (int16)(HI8(v7) - room_width_in_scrolls) >= 0)
    return 0;
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = r24, .ci_r26 = some_pos, .ci_r28 = v7, .ci_r30 = 0, .ci_r38 = some_pos, .ci_r40 = some_pos };
  do {
    BlockShotReact(&ci, v8, false);
    v8 += room_width_in_blocks * 2;
  } while (!(--ci.ci_r38 & 0x8000));
  if (!(ci.ci_r40 & 0x8000))
    return 0;
  KillProjectile(k);
  return 1;
}

/* misiles: un solo bloque (el del centro), sin radio ($94:A46F, $94:A4D9) */
uint8 BlockCollMissileHoriz(uint16 k) {  // 0x94A46F
  int v1 = k >> 1;
  int32 amt = (int32)(int16)projectile_bomb_x_speed[v1] << 8;
  uint16 prod = Mult8x8(projectile_y_pos[v1] >> 4, room_width_in_blocks);
  add_hilo(&projectile_x_pos[v1], &projectile_bomb_x_subpos[v1], amt);
  uint16 r24 = projectile_x_pos[v1];
  uint16 v7 = 2 * (prod + (projectile_x_pos[v1] >> 4));
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = r24, .ci_r26 = 0, .ci_r28 = r24, .ci_r30 = 1, .ci_r38 = 0 };
  if ((int16)(HI8(r24) - room_width_in_scrolls) >= 0 || !BlockShotReact(&ci, v7, false))
    return 0;
  KillProjectile(k);
  return 1;
}

uint8 BlockCollMissileVert(uint16 k) {  // 0x94A4D9
  int v1 = k >> 1;
  int32 amt = (int32)(int16)projectile_bomb_y_speed[v1] << 8;
  add_hilo(&projectile_y_pos[v1], &projectile_bomb_y_subpos[v1], amt);
  uint16 r24 = projectile_y_pos[v1];
  uint16 prod = Mult8x8(projectile_y_pos[v1] >> 4, room_width_in_blocks);
  uint16 v8 = 2 * (prod + (projectile_x_pos[v1] >> 4));
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = r24, .ci_r26 = 0, .ci_r28 = r24, .ci_r30 = 1, .ci_r38 = 0 };
  if ((int16)(HI8(r24) - room_height_in_scrolls) >= 0 || !BlockShotReact(&ci, v8, true))
    return 0;
  KillProjectile(k);
  return 1;
}

uint8 BlockCollNoWaveBeamVert(uint16 k) {  // 0x94A2CA
  int v1 = k >> 1;
  int32 amt = (int32)(int16)projectile_bomb_y_speed[v1] << 8;
  uint16 some_pos = BlockGetSomePos2(k);
  add_hilo(&projectile_y_pos[v1], &projectile_bomb_y_subpos[v1], amt);
  uint16 r24 = projectile_y_pos[v1];
  uint16 v7 = (amt < 0) ? projectile_y_pos[v1] - projectile_y_radius[v1] : projectile_y_radius[v1] + projectile_y_pos[v1] - 1;
  uint16 v8 = (uint16)(projectile_x_pos[v1] - projectile_x_radius[v1]) >> 4;
  uint16 v9 = 2 * (Mult8x8(v7 >> 4, room_width_in_blocks) + v8);
  if (!sign16(some_pos - 16) || (int16)(HI8(v7) - room_height_in_scrolls) >= 0)
    return 0;
  CollInfo ci = { .ci_r18_r20 = amt, .ci_r24 = r24, .ci_r26 = some_pos, .ci_r28 = v7, .ci_r30 = 0, .ci_r38 = some_pos, .ci_r40 = some_pos };
  do {
    BlockShotReact(&ci, v9, true);
    v9 += 2;
  } while (!(--ci.ci_r38 & 0x8000));
  if (!(ci.ci_r40 & 0x8000))
    return 0;
  KillProjectile(k);
  return 1;
}

/* ------------------------------------------------------------------ bombas contra bloques ($94:9CAC) */
static const uint8 kBlockFunc_9C73_Tab0[36] = {  // 0x949C73
   3,  20, 0,
  10,  40, 1,
   5,  30, 0,
  10,  40, 1,
  20,  80, 0,
  20,  80, 1,
  20,  80, 0,
  20,  80, 1,
  30, 100, 0,
  30, 100, 1,
  30, 100, 0,
  30, 100, 1,
};
static const int8 kBlockFunc_9C73_Tab1[18] = {
   0,  0,
  10,  0,
  50,  0,
  50,  3,
   0, -1,
  10,  2,
   0, -1,
   0, -1,
   0, -1,
};

static uint16 BlockFunc_9C73(uint16 v0) {  // 0x949C73
  int v1 = v0 >> 1;
  if (projectile_type[v1] & 0xF00)
    return (uint16)(int16)kBlockFunc_9C73_Tab1[2 * (HI8(projectile_type[v1]) & 0xF) + 1];
  return kBlockFunc_9C73_Tab0[3 * (projectile_type[v1] & 0xF) + 2];
}

static uint16 BlockColl_GetBlockIndexAbove(uint16 k) { return k - room_width_in_blocks - room_width_in_blocks; }  // 0x949D34
static uint16 BlockColl_GetBlockIndexRight(uint16 k) { return room_width_in_blocks * 2 + k + 1 + 1; }            // 0x949D3E
static uint16 BlockColl_GetBlockIndexLeft(uint16 k) { return k - 4; }                                          // 0x949D49
static uint16 BlockColl_GetBlockIndexBelow(uint16 k) { return room_width_in_blocks * 2 + k + 1 + 1; }            // 0x949D4E

static uint8 BlockBombedReact_Special(CollInfo *ci) {  // 0x949D71
  uint8 v0 = bts_get(cur_block_index);
  if (v0 & 0x80)
    SpawnPLM(B94W(B94W(0x9E44 + 2 * area_index) + 2 * (v0 & 0x7F)));
  else
    SpawnPLM(B94W(0x9DA4 + 2 * v0));
  return 1;
}

static void BlockBombedReact(CollInfo *ci, uint16 v0) {  // kBlockBombedReact ($94:A052)
  uint8 r;
  cur_block_index = v0 >> 1;
  do {
    switch ((level_block(cur_block_index) & 0xF000) >> 12) {
    case 0: case 1: case 2: case 3: case 6: r = 0; break;          // ClearCarry_9
    case 4: r = BlockReact_ShootableAir(ci); break;
    case 5: r = BlockReact_HorizExt(); break;
    case 7: r = BlockReact_Bombable(0); break;                      // BlockReact_BombableAir
    case 8: case 9: case 10: case 14: r = 1; break;                 // SetCarry_0
    case 11: r = BlockBombedReact_Special(ci); break;
    case 12: r = BlockReact_Shootable(ci); break;
    case 13: r = BlockReact_VertExt(); break;
    default: r = BlockReact_Bombable(1); break;                     // 15: BlockReact_BombableBlock
    }
  } while (r & 0x80);
}

static void BlockColl_BombExplosion(CollInfo *ci, uint16 k) {  // 0x949CF4
  int v1 = k >> 1;
  if (!projectile_variables[v1]) {
    uint16 v2 = projectile_type[v1];
    if (!(v2 & 1)) {
      projectile_type[v1] = v2 | 1;
      if (cur_block_index != 0xFFFF) {
        uint16 v3 = 2 * cur_block_index;
        BlockBombedReact(ci, 2 * cur_block_index);
        uint16 idx = BlockColl_GetBlockIndexAbove(v3);
        BlockBombedReact(ci, idx);
        uint16 idx2 = BlockColl_GetBlockIndexRight(idx);
        BlockBombedReact(ci, idx2);
        idx = BlockColl_GetBlockIndexLeft(idx2);
        BlockBombedReact(ci, idx);
        idx = BlockColl_GetBlockIndexBelow(idx);
        BlockBombedReact(ci, idx);
      }
    }
  }
}

void BombOrPowerBomb_Func1(uint16 v0) {  // 0x949CAC
  uint16 temp_collision_DD2 = BlockFunc_9C73(v0);
  int v1 = v0 >> 1;
  int16 v2 = projectile_x_pos[v1];
  if (v2 >= 0) {
    uint16 r26 = projectile_x_pos[v1];
    if ((int16)(HI8(v2) - room_width_in_scrolls) < 0) {
      int16 v3 = projectile_y_pos[v1];
      if (v3 >= 0) {
        uint16 r28 = projectile_y_pos[v1];
        if ((int16)(HI8(v3) - room_height_in_scrolls) < 0) {
          cur_block_index = 0;
          CalculateBlockAt(r26, r28, 0, 0);
          CollInfo ci = { .ci_r24 = 0, .ci_r26 = r26, .ci_r28 = r28 };
          if (temp_collision_DD2 == 2)
            BlockColl_BombExplosion(&ci, v0);
          else
            nt_unimpl("BlockFunc_A06A (bomba de poder contra bloques)");
        }
      }
    }
  }
}
