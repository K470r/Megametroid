/* MegaMetroid — motor nativo: proyectiles enemigos (banco $86) y PLMs (banco $84) de Ceres (docs/31).
 * Escrito a mano desde snesrev/sm sm_86.c y sm_84.c. En la estación (sin la huida) solo existen:
 *   - la plataforma y el piso del ascensor que bajan a Samus al empezar (EprojPreInstr_CeresElevatorPad /
 *     _Platform), con sus listas de instrucciones ($86:A28B-A2A0, preparadas en el PC);
 *   - el PLM B7EB, que vuelve a habilitar los sonidos 32 cuadros después de empezar.
 * Lo demás llama a nt_unimpl. */
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "nmath.h"
#include "room.h"
#include "plm.h"
uint8 port_CallEprojInit(uint32 ea, uint16 j);
uint8 port_CallEprojPreInstr(uint32 ea, uint16 k);
typedef void (*PortFnK)(uint16);
PortFnK port_FindCallEprojPreInstr(uint32 ea);
void port_nop_k(uint16 k);
uint16 port_CallEprojInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);

/* ------------------------------------------------------------------ proyectiles enemigos */
static uint16 b86(uint16 a) { return B86W(a); }

/* ------------------------------------------------------------------ crear ($86:8027-80A0) */
uint16 eproj_spawn_x, eproj_spawn_y, eproj_spawn_varE24;
uint16 eproj_spawn_r22;                                                   /* R22 del decomp */
#include "dcompat.h"
Rect16U eproj_spawn_rect;               /* eproj_spawn_pt del decomp (R18/R20) */

static void CallEprojInit(uint16 f, uint16 j);

static void SpawnEprojInner(uint16 j, uint16 gfx_idx) {  // 0x868027
  uint16 v3 = 34;
  while (eproj_id[v3 >> 1]) {
    v3 -= 2;
    if (v3 & 0x8000)
      return;
  }
  int v4 = v3 >> 1;
  eproj_gfx_idx[v4] = gfx_idx;
  eproj_id[v4] = j;
  /* EprojDef ($86): init, pre_instr, instr_list, radius, properties, hit_list, shot_list */
  eproj_pre_instr[v4] = B86W(j + 2);
  eproj_instr_list_ptr[v4] = B86W(j + 4);
  eproj_radius[v4] = B86W(j + 6);
  eproj_properties[v4] = B86W(j + 8);
  eproj_instr_timers[v4] = 1;
  eproj_spritemap_ptr[v4] = 0x8000;
  eproj_E[v4] = 0;
  eproj_F[v4] = 0;
  eproj_timers[v4] = 0;
  eproj_x_subpos[v4] = 0;
  eproj_y_subpos[v4] = 0;
  eproj_G[v4] = 0;
  CallEprojInit(B86W(j), v3);
}

void SpawnEprojWithGfx(uint16 a, uint16 k, uint16 j) {  // 0x868027
  eproj_init_param_1 = a;
  EnemyData *E = gEnemyData(k);
  SpawnEprojInner(j, E->vram_tiles_index | E->palette_index);
}

void SpawnEprojWithRoomGfx(uint16 j, uint16 a) {  // 0x868097
  eproj_init_param_1 = a;
  SpawnEprojInner(j, 0);
}

/* ------------------------------------------------------------------ movimiento y bloques ($86:8506-89FF) */
uint16 MoveEprojWithVelocityX(uint16 k) {  // 0x869311
  int v1 = k >> 1;
  uint16 v2 = eproj_x_vel[v1];
  uint16 c = HI8(eproj_x_subpos[v1]) + (v2 & 0xff);
  eproj_x_subpos[v1] = (eproj_x_subpos[v1] & 0xFF) | (c & 0xFF) << 8;
  return eproj_x_pos[v1] += (int8)(v2 >> 8) + (c >> 8);
}

uint16 MoveEprojWithVelocityY(uint16 k) {  // 0x8692F3
  int v1 = k >> 1;
  uint16 v2 = eproj_y_vel[v1];
  uint16 c = HI8(eproj_y_subpos[v1]) + (v2 & 0xff);
  eproj_y_subpos[v1] = (eproj_y_subpos[v1] & 0xFF) | (c & 0xFF) << 8;
  return eproj_y_pos[v1] += (int8)(v2 >> 8) + (c >> 8);
}

uint16 MoveEprojWithVelocity(uint16 k) {  // 0x8692D6
  MoveEprojWithVelocityX(k);
  return MoveEprojWithVelocityY(k);
}

/* choque de un proyectil enemigo con los bloques ($86:8506-89FF), como el decomp: reacción por tipo de bloque
   (kEprojBlockCollisition_FuncA para el movimiento horizontal, FuncB para el vertical) */
typedef struct { uint16 r20, r24, r26, r28, r30, r32, r34; } EprojCollInfo;
static const uint8 unk_868729[20] = { 0, 1, 0x82, 0x83, 0, 0x81, 2, 0x83, 0, 1, 2, 0x83, 0, 0x81, 0x82, 0x83, 0x80, 0x81, 0x82, 0x83 };
#define CHECK_locret_868728(i) (unk_868729[i] & 0x80 ? -1 : 0)
static inline uint16 SnesModulusE(uint16 a, uint8 b) { return b ? (uint16)(DivMod16(a, b) >> 16) : a; }

static __attribute__((noinline)) uint8 EprojColl_85C2(EprojCollInfo *eci, uint16 a, uint16 k) {  // 0x8685C2
  uint16 v2 = 4 * a + ((bts_get(k) >> 6) ^ ((eci->r34 & 8) >> 3));
  int v3 = eproj_index >> 1;
  if (!eci->r32) {
    if (((eproj_y_pos[v3] - eci->r30) & 8) || CHECK_locret_868728(v2) >= 0) {
      if (!((eci->r30 + eproj_y_pos[v3] - 1) & 8) || CHECK_locret_868728(v2 ^ 2) >= 0)
        return 0;
    }
    goto LABEL_17;
  }
  if (!eci->r26) {
    if (!((eci->r30 + eproj_y_pos[v3] - 1) & 8)) {
      if (CHECK_locret_868728(v2) >= 0)
        return 0;
      goto LABEL_17;
    }
    goto LABEL_14;
  }
  if (eci->r26 != eci->r32 || !((eproj_y_pos[v3] - eci->r30) & 8)) {
LABEL_14:
    if (CHECK_locret_868728(v2) < 0)
      goto LABEL_17;
  }
  if (CHECK_locret_868728(v2 ^ 2) >= 0)
    return 0;
LABEL_17:
  eproj_x_subpos[v3] = 0;
  eproj_x_pos[v3] = (eci->r20 & 0x8000) ? eci->r28 + (eci->r34 | 7) + 1 : (eci->r34 & 0xFFF8) - eci->r28;
  return 1;
}

static __attribute__((noinline)) uint8 EprojColl_8676(EprojCollInfo *eci, uint16 a, uint16 k) {  // 0x868676
  uint16 v3 = 4 * a + ((bts_get(k) >> 6) ^ ((eci->r34 & 8) >> 2));
  int v4 = eproj_index >> 1;
  if (!eci->r32) {
    if (((eproj_x_pos[v4] - eci->r28) & 8) || CHECK_locret_868728(v3) >= 0) {
      if (!((eci->r28 + eproj_x_pos[v4] - 1) & 8) || CHECK_locret_868728(v3 ^ 1) >= 0)
        return 0;
    }
    goto LABEL_17;
  }
  if (!eci->r26) {
    if (!((eci->r28 + eproj_x_pos[v4] - 1) & 8)) {
      if (CHECK_locret_868728(v3) >= 0)
        return 0;
      goto LABEL_17;
    }
    goto LABEL_14;
  }
  if (eci->r26 != eci->r32 || !((eproj_x_pos[v4] - eci->r28) & 8)) {
LABEL_14:
    if (CHECK_locret_868728(v3) < 0)
      goto LABEL_17;
  }
  if (CHECK_locret_868728(v3 ^ 1) >= 0)
    return 0;
LABEL_17:
  eproj_y_subpos[v4] = 0;
  eproj_y_pos[v4] = (eci->r20 & 0x8000) ? eci->r30 + (eci->r34 | 7) + 1 : (eci->r34 & 0xFFF8) - eci->r30;
  return 1;
}

static __attribute__((noinline)) uint8 EprojColl_874E(EprojCollInfo *eci) {  // 0x86874E (pendiente, movimiento vertical)
  uint16 v9 = cur_block_index;
  int v10 = eproj_index >> 1;
  if ((int16)(eproj_x_pos[v10] >> 4) != (int16)SnesModulusE(cur_block_index, room_width_in_blocks))
    return 0;
  uint16 dd6 = 16 * (bts_get(v9) & 0x1F);
  int16 v12 = (int16)(bts_get(v9) << 8);
  uint16 v13 = (v12 & 0x4000) ? eproj_x_pos[v10] ^ 0xF : eproj_x_pos[v10];
  uint16 v14 = dd6 + (v13 & 0xF);
  if (eci->r20 & 0x8000) {
    uint16 dd4 = ((eci->r24 - eci->r30) & 0xF) ^ 0xF;
    if (v12 >= 0) return 0;
    int16 v15 = (kAlignYPos_Tab0[v14] & 0x1F) - dd4 - 1;
    if ((kAlignYPos_Tab0[v14] & 0x1F) - dd4 == 1 || v15 < 0) {
      eproj_y_pos[v10] = eci->r24 - v15;
      eproj_y_subpos[v10] = 0;
      return 1;
    }
    return 0;
  } else {
    uint16 dd4 = (eci->r30 + eci->r24 - 1) & 0xF;
    if (v12 < 0) return 0;
    int16 v8 = (kAlignYPos_Tab0[v14] & 0x1F) - dd4 - 1;
    if ((kAlignYPos_Tab0[v14] & 0x1F) - dd4 == 1 || v8 < 0) {
      eproj_y_pos[v10] = eci->r24 + v8;
      eproj_y_subpos[v10] = 0xFFFF;
      return 1;
    }
    return 0;
  }
}

static uint8 EprojBlockReact(EprojCollInfo *eci, uint8 vertical) {
  for (;;) {
    uint16 blk = level_block(cur_block_index);
    switch (blk >> 12) {
    case 1: {
      uint16 v0 = bts_get(cur_block_index) & 0x1F;
      if (!vertical) {                                       // EprojColl_858E
        if (v0 < 5)
          return EprojColl_85C2(eci, v0, cur_block_index);
        current_slope_bts = bts_get(cur_block_index);
        return 0;                                            // EprojColl_873D
      }
      return v0 >= 5 ? EprojColl_874E(eci) : EprojColl_8676(eci, v0, cur_block_index);   // EprojColl_85AD
    }
    case 5:                                                  // EprojColl_8506
      if (bts_get(cur_block_index)) { cur_block_index += (int8)bts_get(cur_block_index); continue; }
      return 0;
    case 0xD:                                                // EprojColl_Unknown8536
      if (bts_get(cur_block_index)) { cur_block_index += Mul16((int8)bts_get(cur_block_index), room_width_in_blocks); continue; }
      return 0;
    case 8: case 9: case 0xA: case 0xB: case 0xC: case 0xF: return 1;
    default: return 0;
    }
  }
}

/* tipos de bloque con los que un proyectil enemigo reacciona (EprojBlockReact): pendiente (1), salto horizontal y
   vertical por BTS (5, 0xD) y sólidos (8-0xC, 0xF); con los demás sigue de largo */
#define EPROJ_REACTS ((1 << 1) | (1 << 5) | (1 << 8) | (1 << 9) | (1 << 0xA) | (1 << 0xB) | (1 << 0xC) | (1 << 0xD) | (1 << 0xF))

static __attribute__((noinline, noclone, used)) uint8 EprojBlockCollisition_Horiz_full(uint16 k) {  // 0x8688B6
  int v1 = k >> 1;
  int32 amt = (int32)(int16)eproj_x_vel[v1] << 8;
  uint16 R30 = HI8(eproj_radius[v1]);
  uint16 R28 = LO8(eproj_radius[v1]);
  uint16 r26 = (eproj_y_pos[v1] - R30) & 0xFFF0;
  r26 = (uint16)(R30 + eproj_y_pos[v1] - 1 - r26) >> 4;
  uint16 prod = Mul16((uint8)((uint16)(eproj_y_pos[v1] - R30) >> 4), room_width_in_blocks);
  uint32 np = (uint32)amt + PAIR32(eproj_x_pos[v1], eproj_x_subpos[v1]);
  uint16 r24 = np >> 16, r22 = (uint16)np;
  uint16 v4 = (amt < 0) ? (uint16)(r24 - R28) : (uint16)(R28 + r24 - 1);
  uint16 v5 = 2 * (prod + (v4 >> 4));
  EprojCollInfo eci = { (uint16)(amt >> 16), r24, r26, R28, R30, r26, v4 };
  for (;;) {
    cur_block_index = v5 >> 1;
    if (EprojBlockReact(&eci, 0) & 1)
      break;
    v5 += room_width_in_blocks * 2;
    if (--eci.r26 & 0x8000) {
      eproj_x_subpos[v1] = r22;
      eproj_x_pos[v1] = r24;
      return 0;
    }
  }
  eproj_x_subpos[v1] = 0;
  if (amt < 0) {
    uint16 v10 = eci.r28 + (eci.r34 | 0xF) + 1;
    if (v10 <= eproj_x_pos[v1])
      eproj_x_pos[v1] = v10;
  } else {
    uint16 v9 = (eci.r34 & 0xFFF0) - eci.r28;
    if (v9 >= eproj_x_pos[v1])
      eproj_x_pos[v1] = v9;
  }
  return 1;
}

static __attribute__((noinline, noclone, used)) uint8 EprojBlockCollisition_Vertical_full(uint16 k) {  // 0x86897B
  /* lo mismo que el SNES con cuentas de 16 bits (la posición de 32 bits = y:subpos más la velocidad << 8) */
  int v1 = k >> 1;
  uint16 vel = eproj_y_vel[v1], rad = eproj_radius[v1];
  uint16 R30 = rad >> 8, R28 = rad & 0xFF;
  uint16 x = eproj_x_pos[v1], xl = x - R28;
  uint16 n = (uint16)(R28 + x - 1 - (xl & 0xFFF0)) >> 4;
  uint16 sub = eproj_y_subpos[v1], r22 = sub + (uint16)(vel << 8);
  uint16 hi = (uint16)((int16)vel >> 8);
  uint16 r24 = eproj_y_pos[v1] + hi + (r22 < sub);
  uint16 v4 = (vel & 0x8000) ? (uint16)(r24 - R30) : (uint16)(R30 + r24 - 1);
  uint16 i = (uint16)(Mul16((uint8)(v4 >> 4), room_width_in_blocks) + (xl >> 4));
  EprojCollInfo eci = { hi, r24, n, R28, R30, n, v4 };
  for (;; i++) {
    cur_block_index = i & 0x7FFF;
    if (EprojBlockReact(&eci, 1) & 1)
      break;
    if (--eci.r26 & 0x8000) {
      eproj_y_subpos[v1] = r22;
      eproj_y_pos[v1] = r24;
      return 0;
    }
  }
  eproj_y_subpos[v1] = 0;
  if (vel & 0x8000) {
    uint16 v11 = eci.r30 + (eci.r34 | 0xF) + 1;
    if (v11 <= eproj_y_pos[v1])
      eproj_y_pos[v1] = v11;
  } else {
    uint16 v10 = (eci.r34 & 0xFFF0) - eci.r30;
    if (v10 >= eproj_y_pos[v1])
      eproj_y_pos[v1] = v10;
  }
  return 1;
}

/* Lo común (rocas en el aire): ningún bloque del borde reacciona; entonces solo se mueve. Cuentas de 16 bits (en el
   68000 las de 32 bits y los índices int eran la mitad del costo) y los arreglos por su desplazamiento en bytes (k).
   Si algún bloque reacciona, la función completa (*_full) desde el principio: nada se cambió antes. */
#define EPW(arr, k) (*(uint16 *)((uint8 *)(arr) + (k)))
static inline uint8 EprojBlocksQuiet(uint16 bi, uint16 step, uint16 m) {   /* m + 1 bloques desde bi (de a step) */
  for (;;) {
    if ((EPROJ_REACTS >> (level_block(bi & 0x7FFF) >> 12)) & 1)
      return 0;
    if (!m--)
      break;
    bi += step;
  }
  cur_block_index = bi & 0x7FFF;
  return 1;
}
/* lo que ya se vio (docs/60): por proyectil y dirección, el último tramo de bloques sin ninguno que reaccione (bloque
   inicial, cuántos, la versión del nivel y el último índice, que EprojBlocksQuiet deja en cur_block_index). El
   resultado depende solo de los bloques: con el mismo tramo y el nivel sin cambios es el mismo (la roca que cae dentro
   de la misma fila de bloques, el polvo...). En la Genesis, el mismo en ensamblador (ep_qcache). */
typedef struct { uint16 bi, ver, last; uint8 cnt1, pad; } EpQc;   /* cnt1: cuántos + 1 (0: vacía) */
__attribute__((used)) EpQc ep_qc[2][18];
_Static_assert(sizeof(EpQc) == 8, "ep_qc: el ensamblador (ep_qcache)");
#ifdef __mc68000__
/* Los caminos rápidos de abajo en ensamblador (el C de la rama #else es la referencia): ningún bloque tocado
   reacciona (y se lee de la ROM de la sala o es uno no cambiado) → mueve el proyectil y devuelve 0; si no, el
   original completo (_full) con el mismo argumento. eproj_quiet: d4 bloque, d1 paso, d3 m (m + 1 bloques);
   usa d0/d2/d5/a0/a2; d2 = 0 si ninguno reacciona (y cur_block_index = el último). */
__asm__(
  "    .text\n"
  "eproj_quiet:\n"
  "    movea.l lvl_rom,%a0\n"
  "    lea lvl_mod,%a2\n"
  "    move.l #0xBF22,%d0\n"                  /* EPROJ_REACTS: 1, 5, 8-0xD, 0xF */
  "1:  move.w %d4,%d5\n"
  "    and.w #0x7FFF,%d5\n"
  "    cmp.w lvl_fast,%d5\n"
  "    bcc 3f\n"
  "2:  add.w %d5,%d5\n"
  "    move.b (%a0,%d5.w),%d2\n"              /* el byte alto del bloque: el tipo en los 4 bits altos */
  "    lsr.b #4,%d2\n"
  "    btst %d2,%d0\n"
  "    bne 8f\n"
  "    add.w %d1,%d4\n"
  "    dbra %d3,1b\n"
  "    sub.w %d1,%d4\n"
  "    and.w #0x7FFF,%d4\n"
  "    move.w %d4,cur_block_index\n"
  "    moveq #0,%d2\n"
  "    rts\n"
  "3:  cmp.w lvl_blocks,%d5\n"
  "    bcc 8f\n"                              /* fuera del nivel */
  "    move.w %d5,%d2\n"
  "    lsr.w #3,%d2\n"
  "    btst %d5,(%a2,%d2.w)\n"                /* lvl_mod (en memoria: el bit módulo 8) */
  "    beq 2b\n"
  "8:  moveq #1,%d2\n"                        /* reacciona o cambiado por un PLM: el original */
  "    rts\n"
  /* ep_qcache: eproj_quiet con lo ya visto (EprojBlocksQuietC, la referencia en C): a0 = su entrada de ep_qc
     (bi, versión, último, cuántos + 1); mismo tramo y nivel sin cambios -> ninguno reacciona, sin mirar */
  "ep_qcache:\n"
  "    cmp.w (%a0),%d4\n"
  "    bne 1f\n"
  "    move.w lvl_ver,%d2\n"
  "    cmp.w 2(%a0),%d2\n"
  "    bne 1f\n"
  "    move.b %d3,%d2\n"
  "    addq.b #1,%d2\n"
  "    cmp.b 6(%a0),%d2\n"
  "    bne 1f\n"
  "    move.w 4(%a0),cur_block_index\n"
  "    moveq #0,%d2\n"
  "    rts\n"
  "1:  movem.l %d3-%d4/%a0,-(%sp)\n"
  "    bsr eproj_quiet\n"
  "    movem.l (%sp)+,%d3-%d4/%a0\n"
  "    tst.w %d2\n"
  "    bne 2f\n"
  "    move.w %d4,(%a0)\n"
  "    move.w lvl_ver,2(%a0)\n"
  "    move.w cur_block_index,4(%a0)\n"
  "    move.b %d3,6(%a0)\n"
  "    addq.b #1,6(%a0)\n"
  "2:  rts\n"
  /* uint8 EprojBlockCollisition_Horiz(uint16 k)   // 0x8688B6 */
  "    .global EprojBlockCollisition_Horiz\n"
  "EprojBlockCollisition_Horiz:\n"
  "    movem.l %d2-%d7/%a2,-(%sp)\n"
  "    movea.w 34(%sp),%a1\n"                 /* k */
  "    bsr ep_h_body\n"
  "    movem.l (%sp)+,%d2-%d7/%a2\n"
  "    rts\n"
  /* el cuerpo: a1 = k; usa d0-d7/a0/a2 (EprojRunAll lo llama así, con los registros ya guardados) */
  "ep_h_body:\n"
  "    lea eproj_radius,%a0\n"
  "    moveq #0,%d1\n"
  "    move.b (%a0,%a1.w),%d1\n"              /* R30 (el byte alto) */
  "    moveq #0,%d2\n"
  "    move.b 1(%a0,%a1.w),%d2\n"             /* R28 */
  "    lea eproj_y_pos,%a0\n"
  "    move.w (%a0,%a1.w),%d3\n"
  "    move.w %d3,%d4\n"
  "    sub.w %d1,%d4\n"                       /* t = y - R30 */
  "    add.w %d1,%d3\n"
  "    subq.w #1,%d3\n"
  "    move.w %d4,%d1\n"
  "    and.w #0xFFF0,%d1\n"
  "    sub.w %d1,%d3\n"
  "    lsr.w #4,%d3\n"                        /* r26 */
  "    cmp.w #16,%d3\n"
  "    bhi 90f\n"
  "    lea eproj_x_vel,%a0\n"
  "    move.w (%a0,%a1.w),%d1\n"              /* vel */
  "    move.b (%a0,%a1.w),%d5\n"
  "    ext.w %d5\n"                          /* vel >> 8 con signo */
  "    move.w %d1,%d6\n"
  "    lsl.w #8,%d6\n"
  "    lea eproj_x_subpos,%a0\n"
  "    add.w (%a0,%a1.w),%d6\n"               /* r22; X = acarreo */
  "    lea eproj_x_pos,%a0\n"
  "    move.w (%a0,%a1.w),%d7\n"
  "    addx.w %d5,%d7\n"                      /* r24 */
  "    move.w %d7,%d5\n"
  "    tst.w %d1\n"
  "    bmi 1f\n"
  "    add.w %d2,%d5\n"
  "    subq.w #1,%d5\n"
  "    bra 2f\n"
  "1:  sub.w %d2,%d5\n"
  "2:  lsr.w #4,%d5\n"
  "    lsr.w #4,%d4\n"
  "    and.w #0xFF,%d4\n"
  "    move.w room_width_in_blocks,%d1\n"
  "    mulu.w %d1,%d4\n"
  "    add.w %d5,%d4\n"                       /* bi */
  "    lea ep_qc,%a0\n"                       /* ep_qc[0][k / 2] */
  "    move.w %a1,%d0\n"
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    adda.w %d0,%a0\n"
  "    bsr ep_qcache\n"
  "    tst.w %d2\n"
  "    bne 90f\n"
  "    lea eproj_x_subpos,%a0\n"
  "    move.w %d6,(%a0,%a1.w)\n"
  "    lea eproj_x_pos,%a0\n"
  "    move.w %d7,(%a0,%a1.w)\n"
  "    moveq #0,%d0\n"
  "    rts\n"
  "90: moveq #0,%d0\n"                        /* el original completo: _full(k) */
  "    move.w %a1,%d0\n"
  "    move.l %d0,-(%sp)\n"
  "    jsr EprojBlockCollisition_Horiz_full\n"
  "    addq.l #4,%sp\n"
  "    rts\n"
  /* uint8 EprojBlockCollisition_Vertical(uint16 k)   // 0x86897B */
  "    .global EprojBlockCollisition_Vertical\n"
  "EprojBlockCollisition_Vertical:\n"
  "    movem.l %d2-%d7/%a2,-(%sp)\n"
  "    movea.w 34(%sp),%a1\n"                 /* k */
  "    bsr ep_v_body\n"
  "    movem.l (%sp)+,%d2-%d7/%a2\n"
  "    rts\n"
  /* el cuerpo: a1 = k; usa d0-d7/a0/a2 (EprojRunAll lo llama así, con los registros ya guardados) */
  "ep_v_body:\n"
  "    lea eproj_radius,%a0\n"
  "    moveq #0,%d2\n"
  "    move.b (%a0,%a1.w),%d2\n"              /* R30 */
  "    moveq #0,%d1\n"
  "    move.b 1(%a0,%a1.w),%d1\n"             /* R28 */
  "    lea eproj_x_pos,%a0\n"
  "    move.w (%a0,%a1.w),%d3\n"
  "    move.w %d3,%d4\n"
  "    sub.w %d1,%d4\n"                       /* xl = x - R28 */
  "    add.w %d1,%d3\n"
  "    subq.w #1,%d3\n"
  "    move.w %d4,%d1\n"
  "    and.w #0xFFF0,%d1\n"
  "    sub.w %d1,%d3\n"
  "    lsr.w #4,%d3\n"                        /* n */
  "    cmp.w #16,%d3\n"
  "    bhi 90f\n"
  "    lea eproj_y_vel,%a0\n"
  "    move.w (%a0,%a1.w),%d1\n"
  "    move.b (%a0,%a1.w),%d5\n"
  "    ext.w %d5\n"                          /* vel >> 8 con signo */
  "    move.w %d1,%d6\n"
  "    lsl.w #8,%d6\n"
  "    lea eproj_y_subpos,%a0\n"
  "    add.w (%a0,%a1.w),%d6\n"               /* r22 */
  "    lea eproj_y_pos,%a0\n"
  "    move.w (%a0,%a1.w),%d7\n"
  "    addx.w %d5,%d7\n"                      /* r24 */
  "    move.w %d7,%d5\n"
  "    tst.w %d1\n"
  "    bmi 1f\n"
  "    add.w %d2,%d5\n"
  "    subq.w #1,%d5\n"
  "    bra 2f\n"
  "1:  sub.w %d2,%d5\n"
  "2:  lsr.w #4,%d5\n"
  "    and.w #0xFF,%d5\n"
  "    mulu.w room_width_in_blocks,%d5\n"
  "    lsr.w #4,%d4\n"
  "    add.w %d5,%d4\n"                       /* bi */
  "    moveq #1,%d1\n"
  "    lea ep_qc+144,%a0\n"                   /* ep_qc[1][k / 2] */
  "    move.w %a1,%d0\n"
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    adda.w %d0,%a0\n"
  "    bsr ep_qcache\n"
  "    tst.w %d2\n"
  "    bne 90f\n"
  "    lea eproj_y_subpos,%a0\n"
  "    move.w %d6,(%a0,%a1.w)\n"
  "    lea eproj_y_pos,%a0\n"
  "    move.w %d7,(%a0,%a1.w)\n"
  "    moveq #0,%d0\n"
  "    rts\n"
  "90: moveq #0,%d0\n"                        /* el original completo: _full(k) */
  "    move.w %a1,%d0\n"
  "    move.l %d0,-(%sp)\n"
  "    jsr EprojBlockCollisition_Vertical_full\n"
  "    addq.l #4,%sp\n"
  "    rts\n");
#else
static inline uint8 EprojBlocksQuietC(uint16 k, uint8 dir, uint16 bi, uint16 step, uint16 m) {
  EpQc *q = &ep_qc[dir][k >> 1];
  if (q->bi == bi && q->cnt1 == (uint8)(m + 1) && q->ver == lvl_ver) { cur_block_index = q->last; return 1; }
  if (!EprojBlocksQuiet(bi, step, m)) return 0;
  q->bi = bi; q->cnt1 = (uint8)(m + 1); q->ver = lvl_ver; q->last = cur_block_index;
  return 1;
}
uint8 EprojBlockCollisition_Horiz(uint16 k) {  // 0x8688B6
  uint16 vel = EPW(eproj_x_vel, k), rad = EPW(eproj_radius, k);
  uint16 R30 = rad >> 8, R28 = rad & 0xFF;
  uint16 y = EPW(eproj_y_pos, k), t = y - R30;
  uint16 r26 = (uint16)(R30 + y - 1 - (t & 0xFFF0)) >> 4;
  if (r26 <= 16) {
    uint16 sub = EPW(eproj_x_subpos, k), r22 = sub + (uint16)(vel << 8);
    uint16 r24 = EPW(eproj_x_pos, k) + (uint16)((int16)vel >> 8) + (r22 < sub);
    uint16 v4 = (vel & 0x8000) ? (uint16)(r24 - R28) : (uint16)(R28 + r24 - 1);
    uint16 bi = Mul16((uint8)(t >> 4), room_width_in_blocks) + (v4 >> 4);
    if (EprojBlocksQuietC(k, 0, bi, room_width_in_blocks, r26)) {
      EPW(eproj_x_subpos, k) = r22;
      EPW(eproj_x_pos, k) = r24;
      return 0;
    }
  }
  return EprojBlockCollisition_Horiz_full(k);
}
uint8 EprojBlockCollisition_Vertical(uint16 k) {  // 0x86897B
  uint16 vel = EPW(eproj_y_vel, k), rad = EPW(eproj_radius, k);
  uint16 R30 = rad >> 8, R28 = rad & 0xFF;
  uint16 x = EPW(eproj_x_pos, k), xl = x - R28;
  uint16 n = (uint16)(R28 + x - 1 - (xl & 0xFFF0)) >> 4;
  if (n <= 16) {
    uint16 sub = EPW(eproj_y_subpos, k), r22 = sub + (uint16)(vel << 8);
    uint16 r24 = EPW(eproj_y_pos, k) + (uint16)((int16)vel >> 8) + (r22 < sub);
    uint16 v4 = (vel & 0x8000) ? (uint16)(r24 - R30) : (uint16)(R30 + r24 - 1);
    uint16 bi = Mul16((uint8)(v4 >> 4), room_width_in_blocks) + (xl >> 4);
    if (EprojBlocksQuietC(k, 1, bi, 1, n)) {
      EPW(eproj_y_subpos, k) = r22;
      EPW(eproj_y_pos, k) = r24;
      return 0;
    }
  }
  return EprojBlockCollisition_Vertical_full(k);
}
#endif

/* ------------------------------------------------------------------ bolas de fuego de Ridley ($86:93CA-9620) */
static void SetAreaDependentEprojProperties(uint16 j) {  // 0x869402 (kRidleysFireball_Tab0 = $86:9408)
  uint16 v2;
  if (area_index == 2)
    v2 = B86W(0x9408 + 2);
  else if (area_index == 5)
    v2 = B86W(0x9408 + 4);
  else
    v2 = B86W(0x9408);
  eproj_properties[j >> 1] = v2;
}

static void EprojInit_9642_RidleysFireball(uint16 j) {  // 0x8693CA
  int v1 = j >> 1;
  eproj_F[v1] = eproj_unk1995;
  int16 v2 = eproj_init_param_1 ? 25 : -25;
  eproj_x_pos[v1] = enemy_data[0].x_pos + v2;
  eproj_y_pos[v1] = enemy_data[0].y_pos - 43;
  eproj_gfx_idx[v1] = 2560;
  eproj_x_vel[v1] = ERAM(0x7832);                 /* ridley_var_19 */
  eproj_y_vel[v1] = ERAM(0x7834);                 /* ridley_var_1A */
  SetAreaDependentEprojProperties(j);
}

static void EprojPreInstr_9642_RidleysFireball(uint16 k) {  // 0x86940E
  uint16 v1;
  if (EprojBlockCollisition_Horiz(k)) {
    v1 = 0x965E;                                   /* -27042 */
  } else {
    if (!EprojBlockCollisition_Vertical(k))
      return;
    v1 = 0x9650;                                   /* -27056 */
  }
  int v2 = k >> 1;
  eproj_id[v2] = 0;
  if (!eproj_F[v2]) {
    eproj_spawn_x = eproj_x_pos[v2];
    eproj_spawn_y = eproj_y_pos[v2];
    SpawnEprojWithRoomGfx(v1, 3);
    QueueSfx2_Max6(0x2B);
  }
}

static void EprojInit_9660_FireballExplosion(uint16 j) {  // 0x86947F
  int v1 = j >> 1;
  eproj_gfx_idx[v1] = 2560;
  eproj_x_pos[v1] = eproj_spawn_x;
  eproj_y_pos[v1] = eproj_spawn_y;
  eproj_E[v1] = eproj_init_param_1;
  eproj_F[v1] = 0;
  eproj_x_vel[v1] = 0;
  eproj_y_vel[v1] = 0;
}

static void EprojInit_Common(uint16 j, uint16 k, uint16 a) {  // 0x8694EE
  int v3 = j >> 1;
  eproj_F[v3] = a;
  int v4 = k >> 1;
  eproj_x_pos[v3] = eproj_x_pos[v4];
  eproj_y_pos[v3] = eproj_y_pos[v4];
  eproj_E[v3] = eproj_E[v4];
  eproj_gfx_idx[v3] = 2560;
  SetAreaDependentEprojProperties(j);
}

static void EprojInit_Spread(uint16 j, int16 xv, int16 yv, uint16 self) {  // 0x8694A0-94DC
  int v2 = j >> 1;
  eproj_x_vel[v2] = xv;
  eproj_y_vel[v2] = yv;
  EprojInit_Common(j, eproj_init_param_1, self);
}

static void CallEprojInit(uint16 f, uint16 j) {
  switch (f) {
  case 0x93CA: EprojInit_9642_RidleysFireball(j); return;
  case 0x947F: EprojInit_9660_FireballExplosion(j); return;
  case 0x94A0: EprojInit_Spread(j, 0, -3584, 0x9688); return;     // EprojInit_9688
  case 0x94B4: EprojInit_Spread(j, 0, 3584, 0x9696); return;      // EprojInit_9696
  case 0x94C8: EprojInit_Spread(j, 3584, 0, 0x966C); return;      // EprojInit_966C
  case 0x94DC: EprojInit_Spread(j, -3584, 0, 0x967A); return;     // EprojInit_967A
  case 0x96DC: {                                                    // EprojInit_9734_CeresFallingDebris
    int v1 = j >> 1;
    eproj_E[v1] = 0;
    eproj_F[v1] = 0;
    eproj_x_vel[v1] = 0;
    eproj_gfx_idx[v1] = 3584;
    eproj_x_pos[v1] = eproj_init_param_1;
    eproj_y_pos[v1] = 42;
    eproj_y_vel[v1] = 16;
    return;
  }
  case 0xE468: {                                                    // EprojInit_DustCloudOrExplosion
    int v1 = j >> 1;
    eproj_instr_list_ptr[v1] = B86W(0xE42C + 2 * eproj_init_param_1);   // off_86E42C
    eproj_x_pos[v1] = eproj_spawn_x;
    eproj_y_pos[v1] = eproj_spawn_y;
    return;
  }
  case 0xE5D0: case 0xE5D5: case 0xE5DA: {         // EprojInit_SpawnedShotGate / ClosedDownwards / ClosedUpwards
    int v3 = j >> 1;
    CalculatePlmBlockCoords(plm_id);
    eproj_E[v3] = plm_block_indices[plm_id >> 1];
    eproj_x_pos[v3] = 16 * plm_x_block;
    eproj_y_pos[v3] = (f == 0xE5D5 ? 0x40 : f == 0xE5DA ? -0x40 : 0) + 16 * plm_y_block;
    return;
  }
  case 0xA3A3: bg2_y_scroll = 0; return;            // EprojPreInstr_PrePhantomRoom (también es su init)
  default:
    if (!port_CallEprojInit(0x860000 | f, j))
      nt_unimpl_v("CallEprojInit (proyectil enemigo sin portar)", f);
    return;
  }
}

uint16 CheckIfEprojIsOffScreen(uint16 k) {  // 0x86E6E0
  int v1 = k >> 1;
  if ((int16)(eproj_x_pos[v1] - layer1_x_pos) >= 0 && (int16)(eproj_x_pos[v1] - (layer1_x_pos + 256)) < 0 &&
      (int16)(eproj_y_pos[v1] - layer1_y_pos) >= 0 && (int16)(eproj_y_pos[v1] - (layer1_y_pos + 256)) < 0)
    return 0;
  return 1;
}

static void CallEprojPreInstr(uint16 f, uint16 k);
void ep_preinstr_c(uint16 f, uint16 k) { CallEprojPreInstr(f, k); }   /* (EprojRunAll en ensamblador) */
static void CallEprojPreInstr(uint16 f, uint16 k) {
  int v1 = k >> 1;
  switch (f) {
  case 0xA328:                                      // EprojPreInstr_CeresElevatorPad
    if (!eproj_E[v1] || eproj_E[v1]-- == 1 || (int16)eproj_E[v1] < 0) {
      /* el original: si E es 0, o al decrementar llega a 0 o pasa a negativo */
      eproj_y_pos[v1] = samus_y_pos + 28;
      if (!sign16(++samus_y_pos - 73)) {
        samus_y_pos = 72;
        eproj_instr_timers[v1] = 1;
        eproj_instr_list_ptr[v1] = 0xA28B;          // off_86A28B
        CallSomeSamusCode(0xE);
      }
    }
    break;
  case 0xA364:                                      // EprojPreInstr_CeresElevatorPlatform
    if (samus_y_pos == 72) {
      eproj_instr_timers[v1] = 1;
      eproj_instr_list_ptr[v1] = 0xA28B;
    }
    break;
  case 0x8170: break;                               // vacío
  case 0x940E: EprojPreInstr_9642_RidleysFireball(k); break;
  case 0x950C: break;                               // nullsub (explosión)
  case 0x950D:                                      // EprojPreInstr_966C
    MoveEprojWithVelocityX(k);
    if (EprojBlockCollisition_Vertical(k)) {
      eproj_instr_list_ptr[v1] = 0x9574;
      eproj_instr_timers[v1] = 1;
    }
    break;
  case 0x9522:                                      // EprojPreInstr_9688
    MoveEprojWithVelocityY(k);
    if (EprojBlockCollisition_Horiz(k)) {
      eproj_instr_list_ptr[v1] = 0x9574;
      eproj_instr_timers[v1] = 1;
    }
    break;
  case 0x9701:                                      // EprojPreInstr_9734_CeresFallingDebris
    eproj_y_vel[v1] += 16;
    if (EprojBlockCollisition_Vertical(k) & 1) {
      eproj_id[v1] = 0;
      eproj_spawn_x = eproj_x_pos[v1];
      eproj_spawn_y = eproj_y_pos[v1];
      SpawnEprojWithRoomGfx(0xE509, 9);             // kEproj_DustCloudExplosion
      QueueSfx2_Max6(0x6D);
    }
    break;
  case 0xE4FE:                                      // EprojPreInstr_DustCloudOrExplosion
    if (CheckIfEprojIsOffScreen(k))
      eproj_id[v1] = 0;
    break;
  case 0xE604: break;                               // nullsub_99
  case 0xE605: {                                    // EprojPreInstr_E605: la compuerta se mueve
    uint16 v2 = eproj_timers[v1] + abs16n(eproj_y_vel[v1]);
    if (v2 >= 0x1000) {
      eproj_instr_timers[v1] = 1;
      eproj_instr_list_ptr[v1] += 2;
      v2 = 0;
    }
    eproj_timers[v1] = v2;
    eproj_y_subpos[v1] += (uint8)eproj_y_vel[v1] << 8;
    eproj_y_pos[v1] += (int8)HI8(eproj_y_vel[v1]);
    break;
  }
  case 0xA3A3: bg2_y_scroll = 0; break;             // EprojPreInstr_PrePhantomRoom
  default: {                                        /* traducidas (port_gen.c): la función recordada por proyectil */
    static uint16 pf[18];
    static PortFnK pfn[18];
    PortFnK fn;
    if (pf[v1] == f) fn = pfn[v1];
    else { fn = port_FindCallEprojPreInstr(0x860000 | f); pf[v1] = f; pfn[v1] = fn; }
    if (!fn) nt_unimpl_v("CallEprojPreInstr (proyectil enemigo sin portar)", f);
    else if (fn != port_nop_k) fn(k);
    break;
  }
  }
}

/* la lista de instrucciones del proyectil v1 cuando su temporizador llega a 0 (EprojRunOne) */
void ep_instr_list(uint16 v1) {
  {
    uint16 p = eproj_instr_list_ptr[v1 >> 1];
    uint16 f;
    while ((f = b86(p)) & 0x8000) {
      switch (f) {
      case 0x8154: eproj_id[v1 >> 1] = 0; return;                         // EprojInstr_Delete
      case 0x8159: eproj_instr_list_ptr[v1 >> 1] = p; return;             // EprojInstr_Sleep
      case 0x81AB: p = b86(p + 2); break;                                 // EprojInstr_Goto
      case 0x8161: eproj_pre_instr[v1 >> 1] = b86(p + 2); p += 4; break;  // EprojInstr_SetPreInstr_
      case 0x816A: eproj_pre_instr[v1 >> 1] = 0x8170; p += 2; break;      // EprojInstr_ClearPreInstr
      case 0x95BA:                                                        // EprojInstr_95BA
        eproj_unk1995 = 0; SpawnEprojWithRoomGfx(0x966C, v1);
        eproj_unk1995 = 0; SpawnEprojWithRoomGfx(0x967A, v1);
        p += 2; break;
      case 0x95ED:                                                        // EprojInstr_95ED
        eproj_unk1995 = 0; SpawnEprojWithRoomGfx(0x9688, v1);
        eproj_unk1995 = 0; SpawnEprojWithRoomGfx(0x9696, v1);
        p += 2; break;
      case 0x9620: {                                                      // EprojInstr_9620
        uint8 e = (uint8)eproj_E[v1 >> 1] - 1;
        eproj_E[v1 >> 1] = (eproj_E[v1 >> 1] & 0xFF00) | e;
        if ((int8)e >= 0)
          SpawnEprojWithRoomGfx(eproj_F[v1 >> 1], v1);
        p += 2; break;
      }
      case 0xE533: eproj_y_vel[v1 >> 1] = b86(p + 2); p += 4; break;     // EprojInstr_SetYVel
      case 0xA3BE:                                                        // EprojInstr_A3BE
        eproj_x_pos[v1 >> 1] = eproj_E[v1 >> 1];
        eproj_y_pos[v1 >> 1] = eproj_F[v1 >> 1];
        p += 2; break;
      case 0xB3B8:                                                        // EprojInstr_GotoIfFunc1 (rayo del ojo
        /* del Golden Torizo en el suelo): espera mientras el bit 15 de 7E:7800 + F + 0x0C (kraid_healths_8ths[0] en
           el decomp: del Torizo que lo disparó) está en 0 */
        p = (enemy_ram7800[eram_idx((uint16)(0x7800 + eproj_F[v1 >> 1] + 0x0C))] & 0x8000) ? p + 4 : b86(p + 2);
        break;
      case 0xB13E:                                                        // sub_86B13E (huevo del Golden Torizo)
        p = (eproj_E[v1 >> 1] & 0x8000) ? 0xB166 : 0xB14B;
        break;
      case 0x81DE: p += 2; break;                                         // nullsub_82
      case 0xA07A: case 0xA05C: {   // EprojPreInstr_PirateMotherBrainLaser_MoveRight / MoveLeft como instrucción
        if (!port_CallEprojPreInstr(0x860000 | f, v1)) { nt_unimpl_v("EprojRunOne (instrucción sin portar)", f); return; }
        p += 2; break;
      }
      case 0xB436:                                                        // EprojInstr_ResetXYpos2
        eproj_x_pos[v1 >> 1] = eproj_E[v1 >> 1];
        eproj_y_pos[v1 >> 1] = eproj_F[v1 >> 1];
        p += 2; break;
      case 0xA456:                                                        // EprojInstr_GotoWithProbability25
        p = ((NextRandom() & 0xC000) == 0xC000) ? b86(p + 2) : p + 4;
        break;
      default: {                                                          // traducidas (port_gen.c)
        uint8 ok;
        uint16 r = port_CallEprojInstr(0x860000 | f, v1, p + 2, &ok);
        if (!ok) { nt_unimpl_v("EprojRunOne (instrucción sin portar)", f); return; }
        if (!r) return;
        p = r;
        break;
      }
      }
    }
    eproj_instr_timers[v1 >> 1] = f;
    eproj_spritemap_ptr[v1 >> 1] = b86(p + 2);
    eproj_instr_list_ptr[v1 >> 1] = p + 4;
  }
}
static void EprojRunOne(uint16 k) {  // 0x868125
  CallEprojPreInstr(eproj_pre_instr[k >> 1], k);
  uint16 v1 = eproj_index;
  if (eproj_instr_timers[v1 >> 1]-- == 1)
    ep_instr_list(v1);
}

#ifdef __mc68000__
/* EprojRunAll en ensamblador (el C de la rama #else es la referencia): el recorrido de los 18 proyectiles sin las
   capas de C; las rocas de Kraid (EprojPreInstr_KraidRocks, $86:9D56) directas; las demás, ep_preinstr_c; la lista
   de instrucciones (ep_instr_list) solo cuando el temporizador llega a 0 */
__asm__(
  "    .text\n"
  "    .global EprojRunAll\n"
  "EprojRunAll:\n"
  "    tst.w eproj_enable_flag\n"
  "    bpl 99f\n"
  "    movem.l %d2-%d7/%a2-%a3,-(%sp)\n"     /* (una vez: los cuerpos de H y V los usan) */
  "    movea.w #34,%a3\n"                     /* i */
  "1:  move.w %a3,eproj_index\n"
  "    lea eproj_id,%a0\n"
  "    tst.w (%a0,%a3.w)\n"
  "    beq 9f\n"
  "    lea eproj_pre_instr,%a0\n"
  "    move.w (%a0,%a3.w),%d0\n"
  "    cmp.w #0x9D56,%d0\n"
  "    bne 5f\n"
  /* EprojPreInstr_KraidRocks */
  "    movea.l %a3,%a1\n"
  "    bsr ep_h_body\n"
  "    btst #0,%d0\n"
  "    bne 3f\n"
  "    movea.l %a3,%a1\n"
  "    bsr ep_v_body\n"
  "    btst #0,%d0\n"
  "    bne 3f\n"
  "    lea eproj_x_vel,%a0\n"
  "    move.w (%a0,%a3.w),%d0\n"
  "    addq.w #8,%d0\n"
  "    move.w %d0,%d1\n"
  "    sub.w #256,%d1\n"
  "    bmi 2f\n"
  "    move.w #0xFF00,%d0\n"                  /* -256 */
  "2:  move.w %d0,(%a0,%a3.w)\n"
  "    lea eproj_y_vel,%a0\n"
  "    add.w #64,(%a0,%a3.w)\n"
  "    bra 6f\n"
  "3:  lea eproj_id,%a0\n"
  "    clr.w (%a0,%a3.w)\n"
  "    bra 6f\n"
  "5:  move.l %a3,-(%sp)\n"                  /* ep_preinstr_c(f, k) */
  "    moveq #0,%d1\n"
  "    move.w %d0,%d1\n"
  "    move.l %d1,-(%sp)\n"
  "    jsr ep_preinstr_c\n"
  "    addq.l #8,%sp\n"
  "6:  moveq #0,%d2\n"
  "    move.w eproj_index,%d2\n"             /* (el de ahora: EprojRunOne lo vuelve a leer) */
  "    lea eproj_instr_timers,%a0\n"
  "    subq.w #1,(%a0,%d2.w)\n"
  "    bne 8f\n"
  "    move.l %d2,-(%sp)\n"
  "    jsr ep_instr_list\n"
  "    addq.l #4,%sp\n"
  "8:  movea.w eproj_index,%a3\n"            /* i = eproj_index */
  "9:  subq.w #2,%a3\n"
  "    move.w %a3,%d0\n"
  "    bpl 1b\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a3\n"
  "99: rts\n");
#else
void EprojRunAll(void) {  // 0x868104
  if (eproj_enable_flag & 0x8000) {
    for (int i = 34; i >= 0; i -= 2) {
      eproj_index = i;
      if (eproj_id[i >> 1]) {
        EprojRunOne(i);
        i = eproj_index;
      }
    }
  }
}
#endif

void DrawEprojSpritemap(uint16 spritemap, uint16 x, uint16 y, uint16 gfx_idx);   /* capa de la Genesis (banco $8D) */
void md_draw_eprojs(uint16 prio, uint16 bx, uint16 by);                          /* Genesis: el recorrido entero */

/* DrawEprojs ($86:83D6) para los proyectiles de una prioridad (bit 12 de las propiedades = prio): el temblor de
   pantalla (GetValuesForScreenShaking, $86:8427) es el mismo para todos, se calcula una vez */
static void DrawEprojsWithPriority(uint16 prio) {
  uint16 sx = 0, sy = 0;
  if (earthquake_timer && !time_is_frozen_flag && sign16(earthquake_type - 36)) {
    int v0 = (uint16)(4 * earthquake_type) >> 1;
    sx = kScreenShakeOffsets[v0];
    sy = kScreenShakeOffsets[v0 + 1];
    if (earthquake_timer & 2) { sx = -sx; sy = -sy; }
  }
  uint16 bx = sx - layer1_x_pos, by = sy - layer1_y_pos;
#if defined(NT_SSF2) && !defined(MD_OLD_EPDRAW)
  md_draw_eprojs(prio, bx, by);                   /* la Genesis lo hace entero (md_engine.c) */
  return;
#endif
  for (int v1 = 17; v1 >= 0; v1--) {
    if (!eproj_id[v1] || (eproj_properties[v1] & 0x1000) != prio)
      continue;
    uint16 r20 = bx + eproj_x_pos[v1];
    if (!((r20 + 128) & 0xFE00)) {
      uint16 r18 = by + eproj_y_pos[v1];
      if (!(r18 & 0xFF00) || !((r18 + 128) & 0xFE00))
        DrawEprojSpritemap(eproj_spritemap_ptr[v1], r20, r18, eproj_gfx_idx[v1]);
    }
  }
}

void DrawLowPriorityEprojs(void) {  // 0x868390
  DrawEprojsWithPriority(0x1000);
}

void DrawHighPriorityEprojs(void) {  // 0x8683B2
  DrawEprojsWithPriority(0);
}
