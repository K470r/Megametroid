/* MegaMetroid — motor nativo: lo de los jefes que el traductor no hace (docs/42, docs/46).
 * Escrito a mano desde snesrev/sm (sm_a4.c, sm_a7.c, sm_a9.c). El resto de los jefes sale traducido en port_gen.c.
 * Todo con NT_BOSS: va en la página del SSF2 de los jefes, con los niveles de sus salas (port.py, BOSS_PAGE). */
#include "port_gen.h"

/* estado de una lista de instrucciones de Mother Brain en la RAM del SNES (7E:781C: { ip, timer }) */
NT_BOSS uint16 HandleMotherBrainInstructionList(uint16 a) {  // 0xA9D192
  if (!(ERAM(a) & 0x8000))
    return 0;
  uint16 v2 = ERAM(a);
  if (!sign16(BW_A9(v2))) {
    if (ERAM(a + 2) != BW_A9(v2)) {
      ERAM(a + 2)++;
      return BW_A9(v2 + 2);
    }
    v2 += 4;
  }
  while (sign16(BW_A9(v2)))
    v2 = CallMotherBrainInstr(BW_A9(v2) | 0xA90000, v2 + 2);
  if (!BW_A9(v2)) {
    ERAM(a) = 0;
    ERAM(a + 2) = 0;
    return 0;
  }
  ERAM(a + 2) = 1;
  ERAM(a) = v2;
  return BW_A9(v2 + 2);
}

/* Crocomire derritiéndose: borra una línea vertical por paso. En el SNES también borra los píxeles de esa línea en
   las copias de los tiles del cuerpo (7E:4000, ram4000.backups) que después sube por la cola de VRAM; esos tiles son
   del SNES y la Genesis dibuja el cuerpo a su manera: aquí solo la lógica (alturas de las líneas y la cola). */
NT_BOSS uint16 Crocomire_Func_67(void) {  // 0xA496C8
  int n = croco_word_068C;
  while (1) {
    if (croco_cur_vline_idx > 48)                  // (como el decomp: bugfix)
      return 1;
    int rr = BB_A4(0x9697 + croco_cur_vline_idx);  // kCrocoVlineRandomPos
    if ((int8)(croco_vline_height[rr] - croco_target_0688) < 0)
      break;
    if (++croco_cur_vline_idx >= 128) {
      croco_cur_vline_idx = 0;
      return 0;
    }
  }
  int vline_idx = BB_A4(0x9697 + croco_cur_vline_idx);
  do {
    if (croco_vline_height[vline_idx] == 48)
      break;
    croco_vline_height[vline_idx]++;
  } while (--n);
  uint16 v7, v8;
  while (1) {
    v7 = g_word_7E068A + croco_word_7E069A;
    v8 = vram_write_queue_tail;
    if (BW_A4(0x9BC5 + v7) != 0xFFFF)
      break;
    g_word_7E068A = 0;
  }
  nt_vwq_w16(vram_write_queue_tail, BW_A4(0x9BC5 + v7));          // size
  nt_vwq_w16(vram_write_queue_tail + 2, BW_A4(0x9BCB + v7));      // src.addr
  nt_vwq_w16(vram_write_queue_tail + 4, BW_A4(0x9BC9 + v7));      // *(uint16 *)&src.bank
  nt_vwq_w16(vram_write_queue_tail + 5, BW_A4(0x9BC7 + v7));      // vram_dst
  vram_write_queue_tail = v8 + 7;
  g_word_7E068A += 8;
  return 1;
}

/* con el mando 1 (newkeys & 0x4000) el decomp escribe en una tabla de la ROM ($A7:902D): en el SNES no hace nada */
NT_BOSS void Phantoon_Func_2(uint16 k) {  // 0xA7CF0C
  (void)k;
}

/* 7 palabras de una tabla de $A6 a la RAM del SNES (el tilemap de la puerta en 7E:2000), una cada 10 palabras */
NT_BOSS void Ridley_Func_93(uint16 j, uint16 k) {  // 0xA6D3B4
  for (int i = 0; i < 7; i++)
    nt_ram_set_w((uint16)(j + 20 * i), BW_A6((uint16)(k + 2 * i)));
}

/* lista de instrucciones de la bebé metroide en la RAM del SNES (7E:7806: { ip, timer }); -1: inactiva ("double ret") */
NT_BOSS int BabyMetroid_DBCB_DoubleRetEx(uint16 a) {  // 0xA6DBCB
  if (!(ERAM(a) & 0x8000))
    return -1;
  uint16 v2 = ERAM(a);
  if (!sign16(BW_A6(v2))) {
    if (ERAM(a + 2) != BW_A6(v2)) {
      ERAM(a + 2)++;
      return BW_A6(v2 + 2);
    }
    v2 += 4;
  }
  while (sign16(BW_A6(v2)))
    v2 = CallBabyMetroidInstr(BW_A6(v2) | 0xA60000, v2 + 2);
  ERAM(a + 2) = 1;
  ERAM(a) = v2;
  return BW_A6(v2 + 2);
}

/* spritemap de $A6 en una posición de la sala: el de la plataforma (md_engine.c, DrawA6Spritemap) */
void DrawA6Spritemap(uint16 sm, uint16 x, uint16 y, uint16 a);
NT_BOSS void sub_A6DC13(uint16 j, uint16 r18, uint16 r20, uint16 r22) {  // 0xA6DC13
  DrawA6Spritemap(j, r18, r20, r22);
}

/* Mother Brain sube tiles por la cola de VRAM desde una lista (entradas de 7 bytes) en el banco db; mbn_var_22
   (7E:8004 del enemigo 0) es la entrada actual */
NT_BOSS uint8 ProcessSpriteTilesTransfers(uint8 db, uint16 k) {  // 0xA9C5BE
  uint16 src = ERAM(0x8004);
  if (!src)
    src = k;
  uint16 v4 = vram_write_queue_tail;
  uint16 v6 = nt_bank_w(db, src);
  if (v6) {
    nt_vwq_w16(v4, v6);                                         // size
    nt_vwq_w16((uint16)(v4 + 3), nt_bank_w(db, src + 3));       // *(VoidP *)((uint8 *)&src.addr + 1)
    nt_vwq_w16((uint16)(v4 + 2), nt_bank_w(db, src + 2));       // src.addr
    nt_vwq_w16((uint16)(v4 + 5), nt_bank_w(db, src + 5));       // vram_dst
    vram_write_queue_tail = v4 + 7;
    ERAM(0x8004) = src + 7;
    v6 = nt_bank_w(db, ERAM(0x8004));
    if (v6)
      return 0;
  }
  ERAM(0x8004) = v6;
  return 1;
}

/* la larva se pega a la cara de Mother Brain */
uint8 Shitroid_Func_2(uint16 k, Rect16U rect);
void SomeMotherBrainScripts(uint16 a);
void ShitroidInCutscene_UpdateSpeedAndAngle(uint16 k, uint16 r18, uint16 r20, uint16 r22);
NT_BOSS void ShitroidInCutscene_GetIntoFace(uint16 k) {  // 0xA9C811
  ShitroidInCutscene_UpdateSpeedAndAngle(k, -1536, -32256, 3584);
  EnemyData *E = gEnemyData(0x40);
  Rect16U rect = { E->x_pos, E->y_pos, 4, 4 };
  int fire = !Shitroid_Func_2(k, rect);
  if (!fire) {
    E = gEnemyData(k);
    fire = (int16)(E->ai_preinstr - 1) < 0;        // sice_var_F
    --E->ai_preinstr;
  }
  if (fire) {
    gEnemyData(k)->ai_var_A = 0xC851;               // sice_var_A = ShitroidInCutscene_LatchOntoBrain
    SomeMotherBrainScripts(1);
  }
}
