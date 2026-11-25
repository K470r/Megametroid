/* MegaMetroid — cadáveres que se pudren (docs/59): las funciones del decomp (sm_a9.c) que preparan y mueven los píxeles
 * de los tiles de los cadáveres en la RAM del SNES (tilemap_stuff, 7E:9500). En la ROM común (con -Os): desenrolladas
 * como en el decomp no entran en la página de los jefes. Las llama bosses.c (CallCorpseRotting*). */
#pragma GCC optimize("Os")
#include "port_gen.h"
#include "tables.h"
#define DMS(n, k) ERAM(0x8800 + 2 * ((n) - 0x40) + (k))
/* el Torizo muerto (sala DC65): sus tiles del SNES ($B7:A800, kDeadTorizoTiles) en tilemap_stuff (7E:2000) y el montón
   de arena en 7E:9500; las palabras como las guarda el SNES (plano 0 en el byte bajo, el 1 en el alto). La Genesis
   rehace con ellas los tiles de sus sprites (md_corpse.c) */
void md_corpse_dirty(uint16 snes_addr, uint16 bytes);   /* (md_corpse.c: un tramo de la RAM del SNES cambió) */
void Torizo_CorpseRottingInitFunc(void) {  // 0xA9DE18
  static const uint16 kDst[12] = { 0x060, 0x1A0, 0x2C0, 0x400, 0x540, 0x680, 0x7C0, 0x900, 0xA40, 0xB60, 0xC80, 0xDC0 },
                      kSrc[12] = { 288, 800, 1280, 1792, 2304, 2816, 3328, 3840, 4352, 4832, 5312, 5824 },
                      kLen[12] = { 0xC0, 0xC0, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x120, 0x140, 0x140 };
  for (int i = 0; i < 12; i++)
    for (uint16 o = 0; o < kLen[i]; o += 2)
      tilemap_stuff[(kDst[i] + o) >> 1] = kDeadTorizoTiles[(kSrc[i] + o) >> 1];
  md_corpse_dirty(0x2000, 0x1000);
}

/* los cadáveres de la sala de la bebé (DCB1): sus tiles del SNES ($B7:C000, kDeadMonsterTiles) en tilemap_stuff */
static void cp_tiles(uint16 dst, uint16 src, uint16 len) {
  for (uint16 o = 0; o < len; o += 2) tilemap_stuff[(dst + o) >> 1] = kDeadMonsterTiles[(src + o) >> 1];
  md_corpse_dirty(0x2000 + dst, len);
}

/* (generadas desde el decomp, sm_a9.c: mueven o copian filas de píxeles de una columna en tilemap_stuff; j = destino,
   k = origen; antes, el tramo que tocan se marca para la Genesis) */
void Torizo_CorpseRottingMoveFunc(uint16 j, uint16 k) {  // 0xA9E272
  md_corpse_dirty(0x2000 + j + 2, 306); md_corpse_dirty(0x2000 + k + 0, 306);
  if (DMS(0x41, 0) >= 0x50) {
    if (sign16(DMS(0x41, 0) - 94)) {
      int v3 = k >> 1;
      int v4 = j >> 1;
      tilemap_stuff[v4 + 1] = tilemap_stuff[v3];
      tilemap_stuff[v4 + 9] = tilemap_stuff[v3 + 8];
    }
    int v5 = k >> 1;
    tilemap_stuff[v5] = 0;
    tilemap_stuff[v5 + 8] = 0;
  }
  if (DMS(0x41, 0) >= 0x48) {
    if (sign16(DMS(0x41, 0) - 94)) {
      int v6 = k >> 1;
      int v7 = j >> 1;
      tilemap_stuff[v7 + 17] = tilemap_stuff[v6 + 16];
      tilemap_stuff[v7 + 25] = tilemap_stuff[v6 + 24];
    }
    int v8 = k >> 1;
    tilemap_stuff[v8 + 16] = 0;
    tilemap_stuff[v8 + 24] = 0;
  }
  if (DMS(0x41, 0) >= 0x10) {
    if (sign16(DMS(0x41, 0) - 94)) {
      int v9 = k >> 1;
      int v10 = j >> 1;
      tilemap_stuff[v10 + 33] = tilemap_stuff[v9 + 32];
      tilemap_stuff[v10 + 41] = tilemap_stuff[v9 + 40];
    }
    int v11 = k >> 1;
    tilemap_stuff[v11 + 32] = 0;
    tilemap_stuff[v11 + 40] = 0;
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v12 = k >> 1;
    int v13 = j >> 1;
    tilemap_stuff[v13 + 49] = tilemap_stuff[v12 + 48];
    tilemap_stuff[v13 + 57] = tilemap_stuff[v12 + 56];
  }
  int v14 = k >> 1;
  tilemap_stuff[v14 + 48] = 0;
  tilemap_stuff[v14 + 56] = 0;
  if (sign16(DMS(0x41, 0) - 94)) {
    int v15 = j >> 1;
    tilemap_stuff[v15 + 65] = tilemap_stuff[v14 + 64];
    tilemap_stuff[v15 + 73] = tilemap_stuff[v14 + 72];
  }
  tilemap_stuff[v14 + 64] = 0;
  tilemap_stuff[v14 + 72] = 0;
  if (sign16(DMS(0x41, 0) - 94)) {
    int v16 = j >> 1;
    tilemap_stuff[v16 + 81] = tilemap_stuff[v14 + 80];
    tilemap_stuff[v16 + 89] = tilemap_stuff[v14 + 88];
  }
  tilemap_stuff[v14 + 80] = 0;
  tilemap_stuff[v14 + 88] = 0;
  if (sign16(DMS(0x41, 0) - 94)) {
    int v17 = j >> 1;
    tilemap_stuff[v17 + 97] = tilemap_stuff[v14 + 96];
    tilemap_stuff[v17 + 105] = tilemap_stuff[v14 + 104];
  }
  tilemap_stuff[v14 + 96] = 0;
  tilemap_stuff[v14 + 104] = 0;
  if (sign16(DMS(0x41, 0) - 94)) {
    int v18 = j >> 1;
    tilemap_stuff[v18 + 113] = tilemap_stuff[v14 + 112];
    tilemap_stuff[v18 + 121] = tilemap_stuff[v14 + 120];
  }
  tilemap_stuff[v14 + 112] = 0;
  tilemap_stuff[v14 + 120] = 0;
  if (sign16(DMS(0x41, 0) - 94)) {
    int v19 = j >> 1;
    tilemap_stuff[v19 + 129] = tilemap_stuff[v14 + 128];
    tilemap_stuff[v19 + 137] = tilemap_stuff[v14 + 136];
  }
  tilemap_stuff[v14 + 128] = 0;
  tilemap_stuff[v14 + 136] = 0;
  if (DMS(0x41, 0) >= 0x10) {
    if (sign16(DMS(0x41, 0) - 94)) {
      int v20 = j >> 1;
      tilemap_stuff[v20 + 145] = tilemap_stuff[v14 + 144];
      tilemap_stuff[v20 + 153] = tilemap_stuff[v14 + 152];
    }
    tilemap_stuff[v14 + 144] = 0;
    tilemap_stuff[v14 + 152] = 0;
  }
}

void Torizo_CorpseRottingCopyFunc(uint16 j, uint16 k) {  // 0xA9E38B
  md_corpse_dirty(0x2000 + j + 2, 306);
  if (DMS(0x41, 0) >= 0x50 && sign16(DMS(0x41, 0) - 94)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1] = tilemap_stuff[v3];
    tilemap_stuff[v4 + 9] = tilemap_stuff[v3 + 8];
  }
  if (DMS(0x41, 0) >= 0x48 && sign16(DMS(0x41, 0) - 94)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 17] = tilemap_stuff[v5 + 16];
    tilemap_stuff[v6 + 25] = tilemap_stuff[v5 + 24];
  }
  if (DMS(0x41, 0) >= 0x10 && sign16(DMS(0x41, 0) - 94)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 33] = tilemap_stuff[v7 + 32];
    tilemap_stuff[v8 + 41] = tilemap_stuff[v7 + 40];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v9 = k >> 1;
    int v10 = j >> 1;
    tilemap_stuff[v10 + 49] = tilemap_stuff[v9 + 48];
    tilemap_stuff[v10 + 57] = tilemap_stuff[v9 + 56];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v11 = k >> 1;
    int v12 = j >> 1;
    tilemap_stuff[v12 + 65] = tilemap_stuff[v11 + 64];
    tilemap_stuff[v12 + 73] = tilemap_stuff[v11 + 72];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v13 = k >> 1;
    int v14 = j >> 1;
    tilemap_stuff[v14 + 81] = tilemap_stuff[v13 + 80];
    tilemap_stuff[v14 + 89] = tilemap_stuff[v13 + 88];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v15 = k >> 1;
    int v16 = j >> 1;
    tilemap_stuff[v16 + 97] = tilemap_stuff[v15 + 96];
    tilemap_stuff[v16 + 105] = tilemap_stuff[v15 + 104];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v17 = k >> 1;
    int v18 = j >> 1;
    tilemap_stuff[v18 + 113] = tilemap_stuff[v17 + 112];
    tilemap_stuff[v18 + 121] = tilemap_stuff[v17 + 120];
  }
  if (sign16(DMS(0x41, 0) - 94)) {
    int v19 = k >> 1;
    int v20 = j >> 1;
    tilemap_stuff[v20 + 129] = tilemap_stuff[v19 + 128];
    tilemap_stuff[v20 + 137] = tilemap_stuff[v19 + 136];
  }
  if (DMS(0x41, 0) >= 0x10) {
    if (sign16(DMS(0x41, 0) - 94)) {
      int v21 = k >> 1;
      int v22 = j >> 1;
      tilemap_stuff[v22 + 145] = tilemap_stuff[v21 + 144];
      tilemap_stuff[v22 + 153] = tilemap_stuff[v21 + 152];
    }
  }
}

void Sidehopper_CorpseRottingMoveFunc_0(uint16 j, uint16 k) {  // 0xA9E468
  md_corpse_dirty(0x2000 + j + 2, 146); md_corpse_dirty(0x2000 + k + 0, 146);
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v3 = k >> 1;
      int v4 = j >> 1;
      tilemap_stuff[v4 + 1] = tilemap_stuff[v3];
      tilemap_stuff[v4 + 9] = tilemap_stuff[v3 + 8];
    }
    int v5 = k >> 1;
    tilemap_stuff[v5] = 0;
    tilemap_stuff[v5 + 8] = 0;
  }
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v6 = k >> 1;
      int v7 = j >> 1;
      tilemap_stuff[v7 + 17] = tilemap_stuff[v6 + 16];
      tilemap_stuff[v7 + 25] = tilemap_stuff[v6 + 24];
    }
    int v8 = k >> 1;
    tilemap_stuff[v8 + 16] = 0;
    tilemap_stuff[v8 + 24] = 0;
  }
  if (sign16(DMS(0x41, 0) - 38)) {
    int v9 = k >> 1;
    int v10 = j >> 1;
    tilemap_stuff[v10 + 33] = tilemap_stuff[v9 + 32];
    tilemap_stuff[v10 + 41] = tilemap_stuff[v9 + 40];
  }
  int v11 = k >> 1;
  tilemap_stuff[v11 + 32] = 0;
  tilemap_stuff[v11 + 40] = 0;
  if (sign16(DMS(0x41, 0) - 38)) {
    int v12 = j >> 1;
    tilemap_stuff[v12 + 49] = tilemap_stuff[v11 + 48];
    tilemap_stuff[v12 + 57] = tilemap_stuff[v11 + 56];
  }
  tilemap_stuff[v11 + 48] = 0;
  tilemap_stuff[v11 + 56] = 0;
  if (sign16(DMS(0x41, 0) - 38)) {
    int v13 = j >> 1;
    tilemap_stuff[v13 + 65] = tilemap_stuff[v11 + 64];
    tilemap_stuff[v13 + 73] = tilemap_stuff[v11 + 72];
  }
  tilemap_stuff[v11 + 64] = 0;
  tilemap_stuff[v11 + 72] = 0;
}

void Sidehopper_CorpseRottingCopyFunc_0(uint16 j, uint16 k) {  // 0xA9E4F5
  md_corpse_dirty(0x2000 + j + 2, 146);
  if (DMS(0x41, 0) >= 8 && sign16(DMS(0x41, 0) - 38)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1] = tilemap_stuff[v3];
    tilemap_stuff[v4 + 9] = tilemap_stuff[v3 + 8];
  }
  if (DMS(0x41, 0) >= 8 && sign16(DMS(0x41, 0) - 38)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 17] = tilemap_stuff[v5 + 16];
    tilemap_stuff[v6 + 25] = tilemap_stuff[v5 + 24];
  }
  if (sign16(DMS(0x41, 0) - 38)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 33] = tilemap_stuff[v7 + 32];
    tilemap_stuff[v8 + 41] = tilemap_stuff[v7 + 40];
  }
  if (sign16(DMS(0x41, 0) - 38)) {
    int v9 = k >> 1;
    int v10 = j >> 1;
    tilemap_stuff[v10 + 49] = tilemap_stuff[v9 + 48];
    tilemap_stuff[v10 + 57] = tilemap_stuff[v9 + 56];
  }
  if (sign16(DMS(0x41, 0) - 38)) {
    int v11 = k >> 1;
    int v12 = j >> 1;
    tilemap_stuff[v12 + 65] = tilemap_stuff[v11 + 64];
    tilemap_stuff[v12 + 73] = tilemap_stuff[v11 + 72];
  }
}

void Sidehopper_CorpseRottingMoveFunc_2(uint16 j, uint16 k) {  // 0xA9E564
  md_corpse_dirty(0x2000 + j + 802, 146); md_corpse_dirty(0x2000 + k + 800, 146);
  if (sign16(DMS(0x41, 0) - 38)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 401] = tilemap_stuff[v3 + 400];
    tilemap_stuff[v4 + 409] = tilemap_stuff[v3 + 408];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 400] = 0;
  tilemap_stuff[v5 + 408] = 0;
  if (sign16(DMS(0x41, 0) - 38)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 417] = tilemap_stuff[v5 + 416];
    tilemap_stuff[v6 + 425] = tilemap_stuff[v5 + 424];
  }
  tilemap_stuff[v5 + 416] = 0;
  tilemap_stuff[v5 + 424] = 0;
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v7 = j >> 1;
      tilemap_stuff[v7 + 433] = tilemap_stuff[v5 + 432];
      tilemap_stuff[v7 + 441] = tilemap_stuff[v5 + 440];
    }
    tilemap_stuff[v5 + 432] = 0;
    tilemap_stuff[v5 + 440] = 0;
  }
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v8 = j >> 1;
      tilemap_stuff[v8 + 449] = tilemap_stuff[v5 + 448];
      tilemap_stuff[v8 + 457] = tilemap_stuff[v5 + 456];
    }
    tilemap_stuff[v5 + 448] = 0;
    tilemap_stuff[v5 + 456] = 0;
  }
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v9 = j >> 1;
      tilemap_stuff[v9 + 465] = tilemap_stuff[v5 + 464];
      tilemap_stuff[v9 + 473] = tilemap_stuff[v5 + 472];
    }
    tilemap_stuff[v5 + 464] = 0;
    tilemap_stuff[v5 + 472] = 0;
  }
}

void Sidehopper_CorpseRottingCopyFunc_2(uint16 j, uint16 k) {  // 0xA9E5F6
  md_corpse_dirty(0x2000 + j + 802, 146);
  if (sign16(DMS(0x41, 0) - 38)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 401] = tilemap_stuff[v3 + 400];
    tilemap_stuff[v4 + 409] = tilemap_stuff[v3 + 408];
  }
  if (sign16(DMS(0x41, 0) - 38)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 417] = tilemap_stuff[v5 + 416];
    tilemap_stuff[v6 + 425] = tilemap_stuff[v5 + 424];
  }
  if (DMS(0x41, 0) >= 8 && sign16(DMS(0x41, 0) - 38)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 433] = tilemap_stuff[v7 + 432];
    tilemap_stuff[v8 + 441] = tilemap_stuff[v7 + 440];
  }
  if (DMS(0x41, 0) >= 8 && sign16(DMS(0x41, 0) - 38)) {
    int v9 = k >> 1;
    int v10 = j >> 1;
    tilemap_stuff[v10 + 449] = tilemap_stuff[v9 + 448];
    tilemap_stuff[v10 + 457] = tilemap_stuff[v9 + 456];
  }
  if (DMS(0x41, 0) >= 8) {
    if (sign16(DMS(0x41, 0) - 38)) {
      int v11 = k >> 1;
      int v12 = j >> 1;
      tilemap_stuff[v12 + 465] = tilemap_stuff[v11 + 464];
      tilemap_stuff[v12 + 473] = tilemap_stuff[v11 + 472];
    }
  }
}

void Zoomer_CorpseRottingMoveFunc_0(uint16 j, uint16 k) {  // 0xA9E66A
  md_corpse_dirty(0x2000 + j + 2370, 82); md_corpse_dirty(0x2000 + k + 2368, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1185] = tilemap_stuff[v3 + 1184];
    tilemap_stuff[v4 + 1193] = tilemap_stuff[v3 + 1192];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1184] = 0;
  tilemap_stuff[v5 + 1192] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1201] = tilemap_stuff[v5 + 1200];
    tilemap_stuff[v6 + 1209] = tilemap_stuff[v5 + 1208];
  }
  tilemap_stuff[v5 + 1200] = 0;
  tilemap_stuff[v5 + 1208] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = j >> 1;
    tilemap_stuff[v7 + 1217] = tilemap_stuff[v5 + 1216];
    tilemap_stuff[v7 + 1225] = tilemap_stuff[v5 + 1224];
  }
  tilemap_stuff[v5 + 1216] = 0;
  tilemap_stuff[v5 + 1224] = 0;
}

void Zoomer_CorpseRottingCopyFunc_0(uint16 j, uint16 k) {  // 0xA9E6B9
  md_corpse_dirty(0x2000 + j + 2370, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1185] = tilemap_stuff[v3 + 1184];
    tilemap_stuff[v4 + 1193] = tilemap_stuff[v3 + 1192];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1201] = tilemap_stuff[v5 + 1200];
    tilemap_stuff[v6 + 1209] = tilemap_stuff[v5 + 1208];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 1217] = tilemap_stuff[v7 + 1216];
    tilemap_stuff[v8 + 1225] = tilemap_stuff[v7 + 1224];
  }
}

void Zoomer_CorpseRottingMoveFunc_2(uint16 j, uint16 k) {  // 0xA9E6F6
  md_corpse_dirty(0x2000 + j + 2562, 82); md_corpse_dirty(0x2000 + k + 2560, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1281] = tilemap_stuff[v3 + 1280];
    tilemap_stuff[v4 + 1289] = tilemap_stuff[v3 + 1288];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1280] = 0;
  tilemap_stuff[v5 + 1288] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1297] = tilemap_stuff[v5 + 1296];
    tilemap_stuff[v6 + 1305] = tilemap_stuff[v5 + 1304];
  }
  tilemap_stuff[v5 + 1296] = 0;
  tilemap_stuff[v5 + 1304] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = j >> 1;
    tilemap_stuff[v7 + 1313] = tilemap_stuff[v5 + 1312];
    tilemap_stuff[v7 + 1321] = tilemap_stuff[v5 + 1320];
  }
  tilemap_stuff[v5 + 1312] = 0;
  tilemap_stuff[v5 + 1320] = 0;
}

void Zoomer_CorpseRottingCopyFunc_2(uint16 j, uint16 k) {  // 0xA9E745
  md_corpse_dirty(0x2000 + j + 2562, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1281] = tilemap_stuff[v3 + 1280];
    tilemap_stuff[v4 + 1289] = tilemap_stuff[v3 + 1288];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1297] = tilemap_stuff[v5 + 1296];
    tilemap_stuff[v6 + 1305] = tilemap_stuff[v5 + 1304];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 1313] = tilemap_stuff[v7 + 1312];
    tilemap_stuff[v8 + 1321] = tilemap_stuff[v7 + 1320];
  }
}

void Zoomer_CorpseRottingMoveFunc_4(uint16 j, uint16 k) {  // 0xA9E782
  md_corpse_dirty(0x2000 + j + 2754, 82); md_corpse_dirty(0x2000 + k + 2752, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1377] = tilemap_stuff[v3 + 1376];
    tilemap_stuff[v4 + 1385] = tilemap_stuff[v3 + 1384];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1376] = 0;
  tilemap_stuff[v5 + 1384] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1393] = tilemap_stuff[v5 + 1392];
    tilemap_stuff[v6 + 1401] = tilemap_stuff[v5 + 1400];
  }
  tilemap_stuff[v5 + 1392] = 0;
  tilemap_stuff[v5 + 1400] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = j >> 1;
    tilemap_stuff[v7 + 1409] = tilemap_stuff[v5 + 1408];
    tilemap_stuff[v7 + 1417] = tilemap_stuff[v5 + 1416];
  }
  tilemap_stuff[v5 + 1408] = 0;
  tilemap_stuff[v5 + 1416] = 0;
}

void Zoomer_CorpseRottingCopyFunc_4(uint16 j, uint16 k) {  // 0xA9E7D1
  md_corpse_dirty(0x2000 + j + 2754, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1377] = tilemap_stuff[v3 + 1376];
    tilemap_stuff[v4 + 1385] = tilemap_stuff[v3 + 1384];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1393] = tilemap_stuff[v5 + 1392];
    tilemap_stuff[v6 + 1401] = tilemap_stuff[v5 + 1400];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 1409] = tilemap_stuff[v7 + 1408];
    tilemap_stuff[v8 + 1417] = tilemap_stuff[v7 + 1416];
  }
}

void Ripper_CorpseRottingMoveFunc_0(uint16 j, uint16 k) {  // 0xA9E80E
  md_corpse_dirty(0x2000 + j + 2946, 82); md_corpse_dirty(0x2000 + k + 2944, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1473] = tilemap_stuff[v3 + 1472];
    tilemap_stuff[v4 + 1481] = tilemap_stuff[v3 + 1480];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1472] = 0;
  tilemap_stuff[v5 + 1480] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1489] = tilemap_stuff[v5 + 1488];
    tilemap_stuff[v6 + 1497] = tilemap_stuff[v5 + 1496];
  }
  tilemap_stuff[v5 + 1488] = 0;
  tilemap_stuff[v5 + 1496] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = j >> 1;
    tilemap_stuff[v7 + 1505] = tilemap_stuff[v5 + 1504];
    tilemap_stuff[v7 + 1513] = tilemap_stuff[v5 + 1512];
  }
  tilemap_stuff[v5 + 1504] = 0;
  tilemap_stuff[v5 + 1512] = 0;
}

void Ripper_CorpseRottingCopyFunc_0(uint16 j, uint16 k) {  // 0xA9E85D
  md_corpse_dirty(0x2000 + j + 2946, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1473] = tilemap_stuff[v3 + 1472];
    tilemap_stuff[v4 + 1481] = tilemap_stuff[v3 + 1480];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1489] = tilemap_stuff[v5 + 1488];
    tilemap_stuff[v6 + 1497] = tilemap_stuff[v5 + 1496];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 1505] = tilemap_stuff[v7 + 1504];
    tilemap_stuff[v8 + 1513] = tilemap_stuff[v7 + 1512];
  }
}

void Ripper_CorpseRottingMoveFunc_2(uint16 j, uint16 k) {  // 0xA9E89A
  md_corpse_dirty(0x2000 + j + 3138, 82); md_corpse_dirty(0x2000 + k + 3136, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1569] = tilemap_stuff[v3 + 1568];
    tilemap_stuff[v4 + 1577] = tilemap_stuff[v3 + 1576];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1568] = 0;
  tilemap_stuff[v5 + 1576] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1585] = tilemap_stuff[v5 + 1584];
    tilemap_stuff[v6 + 1593] = tilemap_stuff[v5 + 1592];
  }
  tilemap_stuff[v5 + 1584] = 0;
  tilemap_stuff[v5 + 1592] = 0;
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = j >> 1;
    tilemap_stuff[v7 + 1601] = tilemap_stuff[v5 + 1600];
    tilemap_stuff[v7 + 1609] = tilemap_stuff[v5 + 1608];
  }
  tilemap_stuff[v5 + 1600] = 0;
  tilemap_stuff[v5 + 1608] = 0;
}

void Ripper_CorpseRottingCopyFunc_2(uint16 j, uint16 k) {  // 0xA9E8E9
  md_corpse_dirty(0x2000 + j + 3138, 82);
  if (sign16(DMS(0x41, 0) - 14)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1569] = tilemap_stuff[v3 + 1568];
    tilemap_stuff[v4 + 1577] = tilemap_stuff[v3 + 1576];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1585] = tilemap_stuff[v5 + 1584];
    tilemap_stuff[v6 + 1593] = tilemap_stuff[v5 + 1592];
  }
  if (sign16(DMS(0x41, 0) - 14)) {
    int v7 = k >> 1;
    int v8 = j >> 1;
    tilemap_stuff[v8 + 1601] = tilemap_stuff[v7 + 1600];
    tilemap_stuff[v8 + 1609] = tilemap_stuff[v7 + 1608];
  }
}

void Skree_CorpseRottingMoveFunc_0(uint16 j, uint16 k) {  // 0xA9E926
  md_corpse_dirty(0x2000 + j + 1602, 50); md_corpse_dirty(0x2000 + k + 1600, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 801] = tilemap_stuff[v3 + 800];
    tilemap_stuff[v4 + 809] = tilemap_stuff[v3 + 808];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 800] = 0;
  tilemap_stuff[v5 + 808] = 0;
  if (sign16(DMS(0x41, 0) - 30)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 817] = tilemap_stuff[v5 + 816];
    tilemap_stuff[v6 + 825] = tilemap_stuff[v5 + 824];
  }
  tilemap_stuff[v5 + 816] = 0;
  tilemap_stuff[v5 + 824] = 0;
}

void Skree_CorpseRottingCopyFunc_0(uint16 j, uint16 k) {  // 0xA9E95B
  md_corpse_dirty(0x2000 + j + 1602, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 801] = tilemap_stuff[v3 + 800];
    tilemap_stuff[v4 + 809] = tilemap_stuff[v3 + 808];
  }
  if (sign16(DMS(0x41, 0) - 30)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 817] = tilemap_stuff[v5 + 816];
    tilemap_stuff[v6 + 825] = tilemap_stuff[v5 + 824];
  }
}

void Skree_CorpseRottingMoveFunc_2(uint16 j, uint16 k) {  // 0xA9E984
  md_corpse_dirty(0x2000 + j + 1858, 50); md_corpse_dirty(0x2000 + k + 1856, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 929] = tilemap_stuff[v3 + 928];
    tilemap_stuff[v4 + 937] = tilemap_stuff[v3 + 936];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 928] = 0;
  tilemap_stuff[v5 + 936] = 0;
  if (sign16(DMS(0x41, 0) - 30)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 945] = tilemap_stuff[v5 + 944];
    tilemap_stuff[v6 + 953] = tilemap_stuff[v5 + 952];
  }
  tilemap_stuff[v5 + 944] = 0;
  tilemap_stuff[v5 + 952] = 0;
}

void Skree_CorpseRottingCopyFunc_2(uint16 j, uint16 k) {  // 0xA9E9B9
  md_corpse_dirty(0x2000 + j + 1858, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 929] = tilemap_stuff[v3 + 928];
    tilemap_stuff[v4 + 937] = tilemap_stuff[v3 + 936];
  }
  if (sign16(DMS(0x41, 0) - 30)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 945] = tilemap_stuff[v5 + 944];
    tilemap_stuff[v6 + 953] = tilemap_stuff[v5 + 952];
  }
}

void Skree_CorpseRottingMoveFunc_4(uint16 j, uint16 k) {  // 0xA9E9E2
  md_corpse_dirty(0x2000 + j + 2114, 50); md_corpse_dirty(0x2000 + k + 2112, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1057] = tilemap_stuff[v3 + 1056];
    tilemap_stuff[v4 + 1065] = tilemap_stuff[v3 + 1064];
  }
  int v5 = k >> 1;
  tilemap_stuff[v5 + 1056] = 0;
  tilemap_stuff[v5 + 1064] = 0;
  if (sign16(DMS(0x41, 0) - 30)) {
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1073] = tilemap_stuff[v5 + 1072];
    tilemap_stuff[v6 + 1081] = tilemap_stuff[v5 + 1080];
  }
  tilemap_stuff[v5 + 1072] = 0;
  tilemap_stuff[v5 + 1080] = 0;
}

void Skree_CorpseRottingCopyFunc_4(uint16 j, uint16 k) {  // 0xA9EA17
  md_corpse_dirty(0x2000 + j + 2114, 50);
  if (sign16(DMS(0x41, 0) - 30)) {
    int v3 = k >> 1;
    int v4 = j >> 1;
    tilemap_stuff[v4 + 1057] = tilemap_stuff[v3 + 1056];
    tilemap_stuff[v4 + 1065] = tilemap_stuff[v3 + 1064];
  }
  if (sign16(DMS(0x41, 0) - 30)) {
    int v5 = k >> 1;
    int v6 = j >> 1;
    tilemap_stuff[v6 + 1073] = tilemap_stuff[v5 + 1072];
    tilemap_stuff[v6 + 1081] = tilemap_stuff[v5 + 1080];
  }
}

void Sidehopper_CorpseRottingInitFunc_0(void) {  // 0xA9DEC1
  cp_tiles(0x040, 64, 0x60);
  cp_tiles(0x0A0, 512, 0xA0);
  cp_tiles(0x140, 1024, 0xA0);
  cp_tiles(0x1E0, 1536, 0xA0);
  cp_tiles(0x280, 2048, 0xA0);
}

void Sidehopper_CorpseRottingInitFunc_2(void) {  // 0xA9DF08
  cp_tiles(0x320, 288, 0x40);
  cp_tiles(0x3C0, 800, 0xA0);
  cp_tiles(0x460, 1312, 0xA0);
  cp_tiles(0x500, 1824, 0xA0);
  cp_tiles(0x5A0, 2336, 0xA0);
}

void Zoomer_CorpseRottingInitFunc_0(void) {  // 0xA9DF4F
  cp_tiles(0x940, 2656, 0x60);
  cp_tiles(0x9A0, 3168, 0x60);
}

void Zoomer_CorpseRottingInitFunc_2(void) {  // 0xA9DF6C
  cp_tiles(0xA00, 2752, 0x60);
  cp_tiles(0xA60, 3264, 0x60);
}

void Zoomer_CorpseRottingInitFunc_4(void) {  // 0xA9DF89
  cp_tiles(0xAC0, 2848, 0x60);
  cp_tiles(0xB20, 3360, 0x60);
}

void Ripper_CorpseRottingInitFunc_0(void) {  // 0xA9DFA6
  cp_tiles(0xB80, 2560, 0x60);
  cp_tiles(0xBE0, 3072, 0x60);
}

void Ripper_CorpseRottingInitFunc_2(void) {  // 0xA9DFC3
  cp_tiles(0xC40, 2944, 0x60);
  cp_tiles(0xCA0, 3456, 0x60);
}

void Skree_CorpseRottingInitFunc_0(void) {  // 0xA9DFE0
  cp_tiles(0x640, 672, 0x40);
  cp_tiles(0x680, 1184, 0x40);
  cp_tiles(0x6C0, 1696, 0x40);
  cp_tiles(0x700, 2208, 0x40);
}

void Skree_CorpseRottingInitFunc_2(void) {  // 0xA9E019
  cp_tiles(0x740, 224, 0x40);
  cp_tiles(0x780, 736, 0x40);
  cp_tiles(0x7C0, 1248, 0x40);
  cp_tiles(0x800, 1760, 0x40);
}

void Skree_CorpseRottingInitFunc_4(void) {  // 0xA9E052
  cp_tiles(0x840, 448, 0x40);
  cp_tiles(0x880, 960, 0x40);
  cp_tiles(0x8C0, 1472, 0x40);
  cp_tiles(0x900, 1984, 0x40);
}

/* el montón de arena: una línea de tiles de kDeadTorizo_TileData en 7E:9500 ($A9:D5EA) */
void DeadTorizo_CopyLineOfSandHeapTileData(uint16 a) {  // 0xA9D5EA
  uint16 v1 = BW_A9(0xD67C + 2 * a);
  uint16 v2 = BW_A9(0xD69C + 2 * a) >> 1;
  for (uint16 r = 0; r < 18; r++)
    ERAM(0x9500 + 0x10 * r + v1) = kDeadTorizoTiles[v2 + 8 * r];
  md_corpse_dirty(0x9500 + v1, 0x120);
}

