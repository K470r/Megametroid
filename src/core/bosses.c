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

/* ---------------------------------------------------------------- Kraid (sala A59F, docs/59) */
/* el cuerpo en el BG2: el SNES descomprime $B9:FE3E en 7E:2000 (tilemap_stuff) y $B9:FA38 en 7E:4000 (las primeras
   1024 palabras son el mapa del que sale la mitad de arriba); aquí ya descomprimidos (mkdata.py: kKraidMap2000/4000) */
NT_BOSS void Kraid_SetupGfxWithTilePrioClear(uint16 r18) {  // 0xA7AAC6
  for (uint16 i = 0; i < 2048; i++) tilemap_stuff[i] = kKraidMap2000[i];
  ERAM(0x782A) = 0;                                // kraid_hurt_frame
  ERAM(0x782C) = 0;                                // kraid_hurt_frame_timer
  for (uint16 i = 0; i < 768; i++) tilemap_stuff[i + 1024] = tilemap_stuff[i] & 0xDFFF;
  for (uint16 i = 0; i < 1024; i++) tilemap_stuff[i] = r18 & kKraidMap4000[i];
}

/* muerte: el fundido; al terminar el SNES borra 512 bytes de 7E:4000 y los sube a la VRAM (tiles del fondo): aquí
   solo la entrada de la cola (la Genesis no la usa) */
NT_BOSS void Kraid_Death_Fadeout(void) {  // 0xA7C3F9
  Kraid_ProcessKraidInstr();
  if (AdvancePaletteFade_BgPalette6() & 1) {
    EnemyData *E = gEnemyData(0);
    E->ai_var_A = 0xC4A4;                          // Kraid_Death_UpdateBG2TilemapTopHalf
    ERAM(0x782C) = 1;
    ERAM(0x782A) = 1;
    Kraid_Palette_Handling();
    uint16 v = vram_write_queue_tail;
    nt_vwq_w16(v, 512);
    nt_vwq_w16((uint16)(v + 2), 0x4000);
    nt_vwq_w8((uint16)(v + 4), 0x7E);
    nt_vwq_w16((uint16)(v + 5), ((reg_BG12NBA & 0xF) << 8) + 16128);
    vram_write_queue_tail = v + 7;
  }
}

/* colores de Kraid (líneas 7 del BG y de los sprites): como el SNES, pero solo se escriben (y se avisa a la Genesis)
   los que cambian. Con la vida en 0 (muerte, sala vencida) corren en cada cuadro y antes rehacían la CRAM entera */
static void Kraid_SetPal(uint16 i, uint16 c) {
  if (palette_buffer[i] != c) { palette_buffer[i] = c; nt_pal_dirty((uint8)i, (uint8)i); }
}
static void Kraid_PalRow(uint16 a, uint16 dst) {   /* 16 colores de la ROM ($A7:a) a palette_buffer[dst] */
  const uint8 *p = kBankA7B + (uint16)(a - 0x8000);   /* (con la vida en 0 corre en cada cuadro: con un puntero) */
  uint16 *pb = palette_buffer + dst;
  for (uint16 i = 0; i < 16; i++, p += 2) {
    uint16 c = (uint16)(p[0] | p[1] << 8);
    if (pb[i] != c) { pb[i] = c; nt_pal_dirty((uint8)(dst + i), (uint8)(dst + i)); }
  }
}
NT_BOSS void Kraid_HurtFlash_Handling(void) {  // 0xA7B371
  Kraid_PalRow(0xB513 + ((ERAM(0x782A) & 1) ? 0 : 32), 240);   // kraid_hurt_frame; kKraid_SprPalette7_KraidDeath
}
NT_BOSS void Kraid_HealthBasedPaletteHandling(void) {  // 0xA7B394
  uint16 v0 = 0;
  if ((ERAM(0x782A) & 1) == 0) {
    uint16 v2 = 14, health = gEnemyData(0)->health;
    do {
      if ((int16)(health - ERAM_ARR(0x780C + v2)[0]) >= 0)   // kraid_healths_8ths
        break;
      v2 -= 2;
    } while (v2);
    v0 = 16 * (v2 + 2);
  }
  Kraid_PalRow(0xB3D3 + v0, 112);                 // kKraid_BgPalette7
  Kraid_PalRow(0xB513 + v0, 240);                 // kKraid_SprPalette7_KraidDeath
}

/* las uñas de Kraid (las dos, en cada cuadro): la tabla del contorno de Kraid ($A7:BF1D, pares x, y) se recorre hasta
   la altura de la uña; como el original, sin volver a calcular las dos piezas en cada vuelta */
NT_BOSS void KraidsFingernail_Fire(uint16 k) {  // 0xA7BE8E
  EnemyData *v1 = gEnemyData(k);
  if (Enemy_MoveRight_IgnoreSlopes(k, __PAIR32__(v1->ai_var_C, v1->ai_var_B))) {
    v1->ai_var_B = -v1->ai_var_B;
    v1->ai_var_C = -v1->ai_var_C;
  } else {
    EnemyData *v3 = gEnemyData(0);
    uint16 dy = (uint16)(v3->y_pos - v1->y_pos), a = 0xBF1D;
    while ((int16)(BW_A7(a + 2) + dy) >= 0)          /* g_word_A7BF1D[(i >> 1) + 1] */
      a += 4;
    uint16 r18 = BW_A7(a) + v3->x_pos;
    if (!sign16(v1->x_width + v1->x_pos - r18) && (v1->ai_var_C & 0x8000) == 0) {
      v1->ai_var_B = -v1->ai_var_B;
      v1->ai_var_C = -v1->ai_var_C;
    }
  }
  if (Enemy_MoveDown(k, __PAIR32__(v1->ai_var_E, v1->ai_var_D)))
    SetHiLo(&v1->ai_var_E, &v1->ai_var_D, -IPAIR32(v1->ai_var_E, v1->ai_var_D));
}

/* Al volver de la pausa la Genesis rehace su VRAM: el plano B sale del mapa inicial (zebes_load_vram). Los ganchos
   le mandan lo que el SNES copia por DMA, como entradas de la cola de VRAM (nt_vram_queue_flush, md_zebes.c, las
   convierte por filas): la mitad de arriba desde 7E:2000 (Kraid_TransferTopHalfToVram) y, como en la Genesis el
   plano B entero se perdió, lo que el SNES conserva en su VRAM: la mitad de abajo (7E:2800) si Kraid ya es grande y
   las filas que el hundimiento ya borró. */
void nt_vram_queue_flush(const uint8 *q, uint16 n);
static void Kraid_Bg2Copy(uint16 src, uint16 size, uint16 dst) {
  uint8 e[7] = { (uint8)size, (uint8)(size >> 8), (uint8)src, (uint8)(src >> 8), 0x7E, (uint8)dst, (uint8)(dst >> 8) };
  nt_vram_queue_flush(e, 7);
}
static uint8 Kraid_IsBig(void) {                  /* en la primera fase Kraid está abajo (y 0x250) */
  return (int16)(gEnemyData(0)->y_pos - 0x250) < 0;
}
NT_BOSS void Kraid_TransferTopHalfToVram(void) {  // 0xA7C278
  Kraid_Bg2Copy(0x2000, 0x800, (uint16)((reg_BG2SC & 0xFC) << 8));
}
/* gancho de la pausa: el SNES guarda en 7E:5000 la VRAM de la pared (la pausa la pisa); la Genesis la vuelve a subir
   sola (wall_on) */
NT_BOSS void PauseHook_Kraid(void) {  // 0xA7C325
}
NT_BOSS CoroutineRet UnpauseHook_Kraid_IsDead(void) {  // 0xA7C1FB
  Kraid_TransferTopHalfToVram();
  return 0;
}
NT_BOSS CoroutineRet UnpauseHook_Kraid_IsAlive(void) {  // 0xA7C24E
  Kraid_TransferTopHalfToVram();
  if (Kraid_IsBig())                               /* (Genesis) */
    Kraid_Bg2Copy(0x2800, 0x800, (uint16)(((reg_BG2SC & 0xFC) << 8) + 0x800));
  return 0;
}
NT_BOSS CoroutineRet Kraid_UnpauseHook_IsSinking(void) {  // 0xA7C2A0
  uint16 base = (uint16)((reg_BG2SC & 0xFC) << 8);
  Kraid_Bg2Copy(0x2000, 0x800, base);              /* (Genesis: las dos mitades, como las subió Kraid_Death_*) */
  Kraid_Bg2Copy(0x2800, 0x800, (uint16)(base + 0x800));
  uint16 y = gEnemyData(0)->y_pos;
  for (uint16 a = 0xC5E7; BW_A7(a) != 0xFFFF && (int16)(y - BW_A7(a)) >= 0; a += 6)   /* kKraidSinkEntry */
    Kraid_Bg2Copy(0x2FC0, 64, (uint16)(BW_A7(a + 2) + base));
  return 0;
}

/* colores del Golden Torizo según su vida (byte alto & 0x78, hasta 56): 16 de cada tabla a las paletas de sprites
   1 y 2 (palette_buffer 144-175) */
NT_BOSS void SetGoldenTorizoPalette(uint16 a) {  // 0x848000
  uint16 v1 = (a >> 8) & 0x78;
  if (v1 & 0x40)
    v1 = 56;
  uint16 v2 = (uint16)((4 * v1) | 0x1E);
  uint8 ch = 0;                                  /* (se llama en cada cuadro: la Genesis convierte solo si cambió) */
  for (int16 i = 15; i >= 0; i--, v2 -= 2) {
    uint16 c2 = B84W((uint16)(0x8132 + v2)), c1 = B84W((uint16)(0x8032 + v2));   // kGoldenTorizoPalette2 / 1
    if (palette_buffer[i + 160] != c2 || palette_buffer[i + 144] != c1) ch = 1;
    palette_buffer[i + 160] = c2;
    palette_buffer[i + 144] = c1;
  }
  if (ch) nt_pal_dirty(144, 175);
}

/* ------------------------------------------------------------------ cadáveres que se pudren ($A9:DB12-DCB9)
   El SNES copia los tiles del cadáver a la RAM (Mother Brain: 7E:9000) y cada cuadro baja algunas columnas de píxeles
   (la tabla de 7E:9700: { retraso, fila } por columna) y sube los tiles por la cola de VRAM. Aquí la lógica entera,
   en la RAM extra (ERAM) como en el SNES; el dibujo en la Genesis todavía no. Variables del cadáver (Enemy_DeadMonsters):
   dms_var_41.. en 7E:8802 + índice del enemigo, dms_var_52.. en 7E:8824 + índice. */
#include "tables.h"
#define DMS(n, k) ERAM(0x8800 + 2 * ((n) - 0x40) + (k))

NT_BOSS void MotherBrain_CorpseRottingInitFunc(void) {  // 0xA9E08B
  static const uint16 kSrc[6] = { 192, 704, 1216, 1728, 2240, 2752 }, kDst[6] = { 0, 0xE0, 0x1C0, 0x2A0, 0x380, 0x460 },
                      kLen[6] = { 0xC0, 0xC0, 0xC0, 0xC0, 0xE0, 0xE0 };
  for (int i = 0; i < 6; i++)
    for (uint16 o = 0; o < kLen[i]; o += 2)
      ERAM(0x9000 + kDst[i] + o) = kMbCorpseTiles[(kSrc[i] + o) >> 1];
}

/* (los dos de Mother Brain: mueven de a palabras en 7E:9000; j = destino, k = origen, desplazamientos pares) */
NT_BOSS static void mbc_mv(uint16 dst, uint16 src, uint8 copy, uint8 clear) {
  if (copy) ERAM(dst) = ERAM(src);
  if (clear) ERAM(src) = 0;
}
NT_BOSS static void mb_corpse_rows(uint16 j, uint16 k, uint8 move) {
  uint16 a = DMS(0x41, 0);
  uint8 live = sign16(a - 46);
  if (a >= 0x10) { mbc_mv(0x9002 + j, 0x9000 + k, live, move); mbc_mv(0x9012 + j, 0x9010 + k, live, move); }
  if (a >= 8)    { mbc_mv(0x9022 + j, 0x9020 + k, live, move); mbc_mv(0x9032 + j, 0x9030 + k, live, move); }
  mbc_mv(0x9042 + j, 0x9040 + k, live, move); mbc_mv(0x9052 + j, 0x9050 + k, live, move);
  mbc_mv(0x9062 + j, 0x9060 + k, live, move); mbc_mv(0x9072 + j, 0x9070 + k, live, move);
  mbc_mv(0x9082 + j, 0x9080 + k, live, move); mbc_mv(0x9092 + j, 0x9090 + k, live, move);
  if (a >= 8)    { mbc_mv(0x90A2 + j, 0x90A0 + k, live, move); mbc_mv(0x90B2 + j, 0x90B0 + k, live, move); }
  if (a >= 0x20) { mbc_mv(0x90C2 + j, 0x90C0 + k, live, move); mbc_mv(0x90D2 + j, 0x90D0 + k, live, move); }
}
NT_BOSS void MotherBrain_CorpseRottingMoveFunc(uint16 j, uint16 k) { mb_corpse_rows(j, k, 1); }  // 0xA9EA40
NT_BOSS void MotherBrain_CorpseRottingCopyFunc(uint16 j, uint16 k) { mb_corpse_rows(j, k, 0); }  // 0xA9EB0B

NT_BOSS static void CallCorpseRottingInit(uint16 f) {
  switch (f) {
  case 0xE08B: MotherBrain_CorpseRottingInitFunc(); return;
  default: nt_unimpl_v("CallCorpseRottingInit", f); return;
  }
}
NT_BOSS static void CallCorpseRottingMove(uint16 f, uint16 j, uint16 k) {
  switch (f) {
  case 0xEA40: MotherBrain_CorpseRottingMoveFunc(j, k); return;
  case 0xEB0B: MotherBrain_CorpseRottingCopyFunc(j, k); return;
  default: nt_unimpl_v("CallCorpseRottingMove", f); return;
  }
}
void MotherBrain_CorpseRottingFinished(void);
void DeadTorizo_CorpseRottingFinished(void);
void CorpseRottingRotEntryFinishedHook(void);
NT_BOSS static void CallCorpseRottingFinish(uint16 f) {
  switch (f) {
  case 0xB223: MotherBrain_CorpseRottingFinished(); return;
  case 0xD5BD: DeadTorizo_CorpseRottingFinished(); return;
  case 0xDC08: CorpseRottingRotEntryFinishedHook(); return;
  default: nt_unimpl_v("CallCorpseRottingFinish", f); return;
  }
}

NT_BOSS void InitializeEnemyCorpseRotting(uint16 k, uint16 j) {  // 0xA9DC5F
  DMS(0x52, k) = BW_A9(j);
  DMS(0x53, k) = BW_A9(j + 2);
  DMS(0x54, k) = BW_A9(j + 4);
  DMS(0x55, k) = BW_A9(j + 6);
  DMS(0x5B, k) = BW_A9(j + 14);
  uint16 v4 = BW_A9(j + 12);
  DMS(0x56, k) = v4;
  DMS(0x5A, k) = BW_A9(v4 + 2) - 12;
  uint16 n = BW_A9(j + 8);
  DMS(0x57, k) = n;
  DMS(0x58, k) = n - 1;
  DMS(0x59, k) = n - 2;
  /* InitializeCorpseRottingDataTable ($A9:DC40): { n - 1 - i, 2 * i } por columna */
  uint16 t = BW_A9(j), r18 = 0;
  for (int16 v2 = n - 1; v2 >= 0; v2--, t += 4, r18 += 2) { ERAM(t) = v2; ERAM(t + 2) = r18; }
  CallCorpseRottingInit(BW_A9(j + 10));
}

NT_BOSS static void CopyMoveCorpseRottingRotEntry(uint16 r20, uint16 a, uint16 r18) {  // 0xA9DBE0
  DMS(0x41, 0) = a;
  uint16 v1 = (uint16)(a & 0xFFF8) >> 2, v2 = a & 7;
  uint16 v6 = BW_A9(r20 + v1) + 2 * v2;
  CallCorpseRottingMove(r18, v2 >= 6 ? DMS(0x48, 0) + v6 : v6, v6);
}

NT_BOSS uint8 ProcessCorpseRotting(uint16 k) {  // 0xA9DB12
  DMS(0x45, 0) = DMS(0x57, k);
  DMS(0x46, 0) = DMS(0x58, k);
  DMS(0x47, 0) = DMS(0x59, k);
  DMS(0x48, 0) = DMS(0x5A, k);
  DMS(0x49, 0) = DMS(0x5B, k);
  DMS(0x44, 0) = DMS(0x56, k);
  DMS(0x42, 0) = DMS(0x54, k);
  DMS(0x43, 0) = DMS(0x55, k);
  uint16 p = DMS(0x52, k);
  for (uint16 v15 = 0;; v15++, p += 4) {
    if ((int16)ERAM(p) >= 0) {
      if (ERAM(p + 2)) {
        uint16 v8 = ERAM(p + 2) - 1;
        ERAM(p + 2) = v8;
        if (v8 < 4)
          CopyMoveCorpseRottingRotEntry(DMS(0x44, 0), ERAM(p), v15 >= DMS(0x47, 0) ? DMS(0x43, 0) : DMS(0x42, 0));
      } else {
        CopyMoveCorpseRottingRotEntry(DMS(0x44, 0), ERAM(p), DMS(0x43, 0));
        uint16 v12 = ERAM(p) + 2;
        if (v12 < DMS(0x46, 0)) {
          ERAM(p) = v12;
        } else {
          CallCorpseRottingFinish(DMS(0x49, 0));
          if (v15 >= DMS(0x46, 0))
            return 0;
          ERAM(p) = 0xFFFF;
        }
      }
    }
    if ((int16)(v15 + 1 - DMS(0x45, 0)) >= 0)
      return (uint16)(v15 + 1) >= DMS(0x45, 0);
  }
}

/* ProcessCorpseRottingVramTransfers ($A9:DCB9): sube los tiles podridos por la cola de VRAM del SNES; en la Genesis
   (todavía) nada: solo el aviso de que ya se subieron */
NT_BOSS void ProcessCorpseRottingVramTransfers(uint16 k) {
  (void)k;
  ERAM(0x8004) = 0;      /* Get_DeadEnemy(0)->dey_var_22 */
}

/* MotherBrain_AddSpritemapToOam ($A9:93EE): un spritemap de $A9 en (x, y) de la sala con r22 (paleta y tiles) */
void DrawEnemySpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles);
NT_BOSS void MotherBrain_AddSpritemapToOam(uint16 j, uint16 r18, uint16 r20, uint16 r22) {
  DrawEnemySpritemap(0xA9, j, r18 - layer1_x_pos, r20 - layer1_y_pos, r22 & 0x0E00, r22 & 0x01FF);
}
