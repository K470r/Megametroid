/* MegaMetroid — motor nativo: el final y los créditos (estado 39, banco $8B del decomp; docs/63).
 *
 * Casi todo el final sale traducido del decomp (tools/port: las funciones de $8B:D480 en adelante, en port_gen.c) y
 * sus despachos de objetos (ending_disp.c). Aquí lo que no se traduce: el cuadro del estado 39, lo que carga gráficos
 * o toca el hardware del SNES (descomprimir a su RAM, DMA, registros, la cola del modo 7, la OAM), que en la Genesis
 * son avisos a la capa de dibujo (Ending*), y lo que usa la RAM del SNES por dirección (las estrellas, 7E:0E0C).
 *
 * La RAM: el mapa del texto de los créditos (7E:3000) es cinematic_text_tilemap (como la caja de mensaje); las 40
 * estrellas (7E:0E0C, 16 bytes cada una) van en enemy_data, que en el final no se usa (el SNES también pisa ahí). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "cinematic.h"
#include "enemies.h"
#include "tables.h"
#include "port_gen.h"
void CinematicFunction_Intro_Func127(void);
void CinematicFunction_Intro_Func149(void);

/* capa de dibujo (md_ending.c en la Genesis; vacío en el PC) */
void EndingGfx(uint16 scene);                 /* los gráficos de una escena (ENDING_GFX_*) */
void EndingTextUpload(uint16 vram);           /* subir el mapa del texto (cinematic_text_tilemap) a vram (palabra del SNES) */
void EndingM7Queue(uint16 a, uint16 b, uint16 c);
void EndingStar(uint16 x, uint16 y, uint16 chr);   /* una estrella (CinematicFunction_Intro_Func149): sprite de 8x8 */   /* una entrada de la cola del modo 7 (CinematicFunction_Intro_Func118) */
enum { ENDING_GFX_ZEBES = 1, ENDING_GFX_CLOUDS = 2, ENDING_GFX_SPACE = 3, ENDING_GFX_CREDITS = 4, ENDING_GFX_CREDITS_3H = 5 };

/* funciones traducidas (port_gen.c) que se llaman desde aquí */
void SetupPpu_5_Mode7(void);
void ClearPaletteFXObjects(void);
void CreditsObject_Process(void);
void DrawCinematicSpriteObjects_Ending(void);
void ClearCinematicBgObjects(uint16 a);
void CinematicFunction_Intro_Func128(uint16 a);
void EnableTextGlowObjects_(void);
void CreditsObject_Init(uint16 j);
void CopyPalettesToFadingPalettes(void);
void DecomposePaletteDataForFading(void);
void ClearYColorsFromIndexX(uint16 k, uint16 j);
void ComposeFadingPalettes(void);
void QueueMusic_DelayedY(uint16 a, uint16 j);
uint8 EndingCinematicFunction(uint16 f);       /* ending_disp.c */
void HandleCinematicSprites_(void);           /* cinematic.c */
void HandleCinematicBgObjects_(void);         /* intro.c */
void HandleMode7TransformationMatrix_(void);  /* cinematic.c */
void nt_clear_ram(uint16 from, uint16 to);

#define CINE_RAM_FROM 0x198D                  /* cinematic_var5 y los 656 bytes que siguen */
#define CINE_RAM_TO   (0x198D + 658)

/* ------------------------------------------------------------------ el cuadro del estado 39 ($8B:D443) */
#ifdef MD_EDPROF                                /* medición (prueba): líneas de cada parte del cuadro */
void md_edprof(uint8 i);
#define EDP(i) md_edprof(i)
#else
#define EDP(i)
#endif
void GameState_39_EndingAndCredits(void) {
  EDP(0);
  EndingCinematicFunction(cinematic_function);   /* (ending_disp.c; las nullsub no están: no hacen nada) */
  EDP(1);
  if (!(cinematic_var11 & 0x8000))
    ++cinematic_var11;
  ++cinematic_var12;
  HandleCinematicSprites_();
  EDP(2);
  if (!sign16(cinematic_function + 0x1AFC))   // MaybeHandleCinematicBgObjects
    HandleCinematicBgObjects_();
  CreditsObject_Process();
  EDP(3);
  PaletteFxHandler();
  EDP(4);
  DrawCinematicSpriteObjects_Ending();
  EDP(5);
  CinematicFunction_Intro_Func149();
  HandleMode7TransformationMatrix_();
  EDP(6);
}

/* ------------------------------------------------------------------ PPU */
void SetupPpu_7_Mode1(void) {  // 0x8B83D3
  reg_INIDISP = 0x80;
  reg_OBSEL = 0;
  reg_BGMODE = 1;
  reg_M7SEL = 0;
  reg_BG1SC = 72; reg_BG2SC = 76; reg_BG3SC = 36; reg_BG4SC = 0;
  reg_BG12NBA = 84; reg_BG34NBA = 2;
  reg_TM = 1; reg_TS = 0;
  reg_W12SEL = 0; reg_W34SEL = 0; reg_WOBJSEL = 0; reg_TMW = 0; reg_TSW = 0;
  reg_CGWSEL = 0; reg_CGADSUB = 0;
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  oam_next_ptr = 0;
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256; reg_M7X = 128; reg_M7Y = 128;
  cinematic_var5 = 0;
  cinematic_var6 = 1;
  reg_BG1HOFS = 0; reg_BG1VOFS = 0; reg_BG2HOFS = 0; reg_BG2VOFS = 0;
  cinematic_var7 = 0; cinematic_var8 = 0; cinematic_var9 = 0; cinematic_var10 = 0;
}

void CinematicUpdateSomeBg(void) {  // 0x8B8806: el mapa del texto a la VRAM (cola de VRAM del SNES)
  if (reg_BGMODE != 7)
    EndingTextUpload(cinematicbg_var3);
}

void MoveUnusedSpritesOffScreen(void) {}     /* la OAM del SNES: la Genesis arma su lista de sprites aparte */

/* ------------------------------------------------------------------ créditos */
/* CreditsObject_Func1 ($8B:99C1): una fila de los créditos (32 celdas) al mapa del texto, desde las filas
   descomprimidas de $97:EEFF (en el SNES, 7F:0000; aquí kCreditsRows, mkdata.py) */
void CreditsObject_Func1(uint16 j) {
  uint16 dst = Mult8x8(cinematic_var26, 0x40) >> 1;
  uint16 src = BW_8C((uint16)(j + 2)) >> 1;
  for (uint16 i = 0; i < 32; i++)
    cinematic_text_tilemap[(uint16)(dst + i) & 2047] = kCreditsRows[(uint16)(src + i) & 4095];
}

/* ------------------------------------------------------------------ escenas */
void CinematicFunctionEscapeFromCebes(void) {  // 0x8BD480: Zebes y la nave (modo 7)
  cur_irq_handler = 0;
  irqhandler_next_handler = 0;
  SetupPpu_5_Mode7();
  nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
  for (uint16 i = 0; i < 256; i++)
    palette_buffer[i] = BW_8C((uint16)(0xEDE9 + 2 * i));   // kPalettes_Intro3
  nt_pal_dirty(0, 255);
  EndingGfx(ENDING_GFX_ZEBES);
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256; reg_M7X = 128; reg_M7Y = 128;
  cinematic_var5 = 32;
  cinematic_var6 = 64;
  cinematic_var8 = 0;
  cinematic_var10 = 0;
  door_def_ptr = 0;
  layer1_x_pos = 0;
  layer1_y_pos = 0;
  ClearPaletteFXObjects();
  EnablePaletteFx();
  cinematic_glow_ptr[7] = 0;                     // eproj_pre_instr[1] del decomp (7E:1A05)
  reg_TM = 17; reg_TS = 1; reg_CGWSEL = 2; reg_CGADSUB = 17;
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEEB, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEF1, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEEB, 1);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEF1, 1);
  screen_fade_delay = 0;
  screen_fade_counter = 2;
  cinematic_function = FUNC16(CinematicFunction_Intro_Func109);
  QueueMusic_Delayed8(0);
  QueueMusic_Delayed8(0xFF33);
  QueueMusic_DelayedY(5, 0xE);
}

void CinematicFunction_Intro_Func112(void) {  // 0x8BD731: las nubes
  EndingGfx(ENDING_GFX_CLOUDS);
  ClearPaletteFXObjects();
  ClearCinematicSprites();
  SpawnPalfxObject(addr_kPalfx_E1E0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEED3, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEED9, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEDF, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEE5, 0);
  cinematic_var5 = 32;
  cinematic_var6 = 64;
  cinematic_var8 = 0;
  cinematic_var10 = 0;
  cinematic_function = FUNC16(CinematicFunction_Intro_Func113);
}

void CinematicFunction_Intro_Func115(void) {  // 0x8BD837: el espacio, Zebes que explota
  EndingGfx(ENDING_GFX_SPACE);
  reg_OBSEL = 2;
  ClearPaletteFXObjects();
  ClearCinematicSprites();
  for (uint16 i = 128; i < 256; i++)
    palette_buffer[i] = BW_8C((uint16)(0xEBE9 + 2 * i));   // kPalettes_Intro6
  nt_pal_dirty(128, 255);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEE9D, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEAF, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEA3, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BEEA9, 0);
  cinematic_var5 = 0;
  cinematic_var6 = 64;
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256; reg_M7X = 128; reg_M7Y = 128;
  cinematic_var8 = 0;
  cinematic_var10 = 0;
  reg_TM = 1; reg_TS = 16; reg_CGWSEL = 2; reg_CGADSUB = 33;
  CopyPalettesToFadingPalettes();
  DecomposePaletteDataForFading();
  ClearYColorsFromIndexX(0x1A0, 0x10);
  ClearYColorsFromIndexX(0x1E0, 0x10);
  ComposeFadingPalettes();
  cinematic_var4 = 63;
  cinematic_function = FUNC16(CinematicFunction_Intro_Func116);
}

void CinematicFunction_Intro_Func118(void) {  // 0x8BDB9E (sin dirección en el decomp): el mapa del modo 7 por trozos
  static const uint16 kTab0[16] = { 0, 0x800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800,
                                    0x4000, 0x4800, 0x5000, 0x5800, 0x6000, 0x6800, 0x7000, 0x7800 };
  static const uint16 kTab1[16] = { 0, 0x800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800,
                                    0, 0x800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800 };
  uint16 v = cinematic_var4;
  EndingM7Queue(sign16(v - 8) ? 192 : 128, kTab0[v & 15], kTab1[v & 15]);
  if (!sign16(++cinematic_var4 - 16)) {
    reg_BGMODE = 1;
    cinematic_var4 = 0;
    cinematic_function = FUNC16(nullsub_125);
  }
}

void CinematicFunction_Intro_Func126(void) {  // 0x8BDE80: el tiempo y los créditos
  SetupPpu_7_Mode1();
  ClearCinematicSprites();
  ClearPaletteFXObjects();
  for (uint16 i = 0; i < 128; i++)
    palette_buffer[i] = BW_8C((uint16)(0xE9E9 + 2 * i));   // kPalettes_Intro5
  nt_pal_dirty(0, 127);
  EndingGfx(sign16(game_time_hours - 3) ? ENDING_GFX_CREDITS : ENDING_GFX_CREDITS_3H);
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256; reg_M7X = 104; reg_M7Y = 112;
  cinematic_var8 = -8;
  cinematic_var10 = -24;
  cinematic_var5 = 0;
  cinematic_var6 = 2304;
  reg_INIDISP = 15;
  layer1_x_pos = 0;
  layer1_y_pos = 0;
  EnableHdmaObjects();
  for (int i = 0; i < 256; i++)
    hdma_table_1[i] = 0;
  CinematicFunction_Intro_Func127();
  ClearCinematicBgObjects(0x7F);
  CinematicFunction_Intro_Func128(0x7F);
  EnableTextGlowObjects_();
  screen_fade_delay = 3;
  screen_fade_counter = 3;
  CreditsObject_Init(addr_stru_8BF6F8);
  cinematic_function = FUNC16(nullsub_125);
}

/* el efecto del texto "THE OPERATION WAS COMPLETED SUCCESSFULLY" ($88:EC3B): la tabla de HDMA del SNES va en la RAM
   de los botones guardados (7E:0D82-0D8F); se escribe igual, por dirección */
void CinematicFunction_Intro_Func133(void) {
  g_word_7E0D9C = 1;
  nt_ram_set_w(0x0D9E, 0x4000);
  loop_counter_transfer_enemies_to_vram = 8;
  button_config_shoot_x_saved = 192;
  button_config_itemcancel_y_saved = 192;
  nt_ram_set_w(0x0D85, 0x00C0);
  nt_ram_set_w(0x0D83, 0x9800);
  button_config_run_b_saved = 0x9880;
  nt_ram_set_w(0x0D8B, 0x00C0);
  nt_ram_set_w(0x0D89, 0x9800);
  button_config_aim_down_L_saved = 0x9880;
  button_config_aim_up_R_saved = 0;
  SpawnHdmaObject(0x88, 0xEC8A);
}

void EnableNMI(void) { reg_NMITIMEN |= 0x80; }   /* 0x80834B */


/* ------------------------------------------------------------------ las estrellas (7E:0E0C, en enemy_data) */
#define STAR(i) ((uint16 *)enemy_data + 8 * (i))
uint16 *ending_star(uint16 i) { return STAR(i); }   /* (host/check las compara con 7E:0E0C) */

void CinematicFunction_Intro_Func127(void) {  // 0x8BE7BB
  for (uint16 i = 0; i < 40; i++) {
    uint16 *s = STAR(i);
    s[0] = i;
    s[6] = 0;
    s[7] = 0;
    uint16 v3 = BW_8B((uint16)(0xE9CF + 2 * (4 * i + 3)));
    if (v3) { s[5] = v3; s[0] |= 0x8000; }
    else s[5] = 32;
    s[1] = 128; s[3] = 128; s[2] = 0; s[4] = 0;
  }
}

/* las mueve y las dibuja ($8B:E812): en el SNES escribe en la OAM; aquí la Genesis las dibuja desde STAR (las que
   tienen el byte alto de s[0] distinto de 0, en (s[1] - 4, s[3] - 4) con el tile kStarChr[s[0] >> 9]) */
void CinematicFunction_Intro_Func149(void) {
  if (!cinematic_glow_ptr[7])                    /* eproj_pre_instr[1] del decomp (7E:1A05) */
    return;
  for (uint16 i = 0; i < 40; i++) {
    uint16 *s = STAR(i);
    if ((int16)s[0] < 0) {
      if ((int16)--s[5] < 0) { s[5] = 32; s[0] = (uint8)s[0]; }
    } else {
      uint16 v4 = 4 * (uint8)s[0];
      uint16 t0 = BW_8B((uint16)(0xE9CF + 2 * v4)), t1 = BW_8B((uint16)(0xE9CF + 2 * (v4 + 1)));
      uint16 v5;
      if (sign16((s[0] & 0xFF00) - 1024)) {
        s[6] += t0;
        v5 = s[7];
      } else {
        s[6] += t0 + t0;
        v5 = t1 + s[7];
      }
      s[7] = t1 + v5;
      uint16 r18 = (uint16)(int8)(s[6] >> 8), r20 = (uint16)(s[6] << 8);
      uint32 sum = (uint32)r20 + s[2];
      s[2] = (uint16)sum;
      s[1] += r18 + (sum >> 16);
      r18 = (uint16)(int8)(s[7] >> 8); r20 = (uint16)(s[7] << 8);
      sum = (uint32)r20 + s[4];
      s[4] = (uint16)sum;
      s[3] += r18 + (sum >> 16);
    }
  }
  for (uint16 i = 0; i < 40; i++) {
    uint16 *s = STAR(i);
    if ((int16)s[0] < 0)
      continue;
    if ((s[1] >> 8) || ((s[1] - 4) & 0xFF00) || (s[3] >> 8) || ((s[3] - 4) & 0xFF00)) {
      s[5] = 32; s[1] = 128; s[3] = 128; s[2] = 0; s[4] = 0; s[6] = 0; s[7] = 0;
      s[0] = (uint8)s[0];
    } else {
      if ((int16)--s[5] <= 0) {
        uint16 v22 = s[0];
        s[5] = BW_8B((uint16)(0xE9CF + 2 * (4 * (uint8)s[0] + 2)));
        s[0] = v22 + 512;
      }
      if (s[0] >> 8)                             /* la estrella a la OAM (8x8, g_word_8BE9A7) */
        EndingStar(s[1] - 4, s[3] - 4, BW_8B((uint16)(0xE9A7 + 2 * ((s[0] >> 8) >> 1))));
    }
  }
}

/* ------------------------------------------------------------------ el porcentaje de objetos ($8B:E627) */
/* los máximos de energía, de reserva, de misiles, de supermisiles y de bombas de poder (las direcciones de
   g_off_8BE70D: 7E:09C4, 09D4, 09C8, 09CC, 09D0) entre lo que da cada tanque, más los objetos y los rayos */
uint16 CalcItemPercentageCount(uint16 k, uint16 instrp) {
  (void)k;
  uint16 r18 = 0;
  for (int16 v0 = 4; v0 >= 0; v0--)
    r18 += SnesDivide(nt_ram_w(BW_8B((uint16)(0xE70D + 2 * v0))), BW_8B((uint16)(0xE717 + 2 * v0)));
  for (uint16 i = 0; i < 11; i++)
    if (BW_8B((uint16)(0xE721 + 2 * i)) & collected_items) ++r18;
  for (uint16 i = 0; i < 5; i++)
    if (BW_8B((uint16)(0xE737 + 2 * i)) & collected_beams) ++r18;
  uint16 tens_all = r18 / 10, r22 = r18 % 10;
  uint16 hund = tens_all / 10, r20 = tens_all % 10;
  if (hund) {
    cinematic_text_tilemap[462] = BW_8B((uint16)(0xE741 + 4 * hund));
    cinematic_text_tilemap[494] = BW_8B((uint16)(0xE741 + 4 * hund + 2));
  }
  if (r20 || hund) {
    cinematic_text_tilemap[463] = BW_8B((uint16)(0xE741 + 4 * r20));
    cinematic_text_tilemap[495] = BW_8B((uint16)(0xE741 + 4 * r20 + 2));
  }
  cinematic_text_tilemap[464] = BW_8B((uint16)(0xE741 + 4 * r22));
  cinematic_text_tilemap[496] = BW_8B((uint16)(0xE741 + 4 * r22 + 2));
  cinematic_text_tilemap[465] = 14442;          /* "%" */
  cinematic_text_tilemap[497] = 14458;
  return instrp;
}

/* ------------------------------------------------------------------ dibujo de los spritemaps ($81:879F, $81:8A5F) */
/* en el SNES escriben en la OAM; en la Genesis, el mismo dibujo de los spritemaps de $8C que usan las cinemáticas */
void DrawCinematicSpritemap(uint16 sm, uint16 x, uint16 y, uint16 chr, uint8 offscreen);
void DrawSpritemap(uint8 db, uint16 j, uint16 x_r20, uint16 y_r18, uint16 chr_r22) {
  if (db == 0x8C)
    DrawCinematicSpritemap(j, x_r20, y_r18, chr_r22, 0);
}
void DrawSpritemapOffScreen(uint16 j, uint16 x_r20, uint16 y_r18, uint16 chr_r22) {
  DrawCinematicSpritemap(j, x_r20, y_r18, chr_r22, 1);
}
