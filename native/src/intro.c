/* MegaMetroid — motor nativo: la intro (banco $8B del decomp, sm_8b.c; estado de juego 30; docs/38).
 *
 * Desde "The last Metroid is in captivity" hasta que la nave sale hacia Ceres y empieza la partida nueva:
 *   - páginas del diario: objetos de fondo ($8B:95F0) que escriben el texto en cinematic_text_tilemap (BG3 del
 *     SNES), con brillo letra por letra (objetos de brillo, $8B:97F7);
 *   - recuerdos: Mother Brain y la larva (Samus con su motor y la entrada de demo, $91:8395), el científico y el
 *     laboratorio, con fundidos cruzados (paletas de 5 bits por canal en tilemap_stuff, $8B:8C09);
 *   - el vuelo a Ceres: la nave en modo 7 (solo la matriz: el dibujo lo hace la Genesis) y "SPACE COLONY".
 * Los gráficos (tiles, mapas fijos) no pasan por el motor: IntroGfx() avisa a la Genesis qué fase se carga.
 * El texto en japonés (japanese_text_flag) no está portado: el juego va en inglés o en la traducción. */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "room.h"
#include "cinematic.h"

void IntroGfx(uint16 which);              /* capa de la Genesis: fase de la intro que se carga (INTRO_GFX_*) */
uint8 cinematic_text_dirty;                /* capa de la Genesis: el juego cambió 7E:3000 (cinematic_text_tilemap) */
void QueueMusic_DelayedY(uint16 a, uint16 j);

#define INT16_SHL8(x) ((int32)(int16)(x) << 8)
#define CINE_RAM_FROM 0x198D
#define CINE_RAM_TO   (0x198D + 658)

/* ------------------------------------------------------------------ PPU ($8B:80DA) */
static void SetupPpu_Intro(void) {  // 0x8B80DA
  reg_INIDISP = 0x80;
  reg_OBSEL = 3;
  reg_BGMODE = 9;
  reg_M7SEL = 0;
  reg_BG1SC = 80; reg_BG2SC = 72; reg_BG3SC = 76; reg_BG4SC = 0;
  reg_BG12NBA = 0; reg_BG34NBA = 4;
  reg_TM = 4; reg_TS = 0; reg_TMW = 0;
  next_gameplay_CGWSEL = 0; next_gameplay_CGADSUB = 0;
  reg_CGWSEL = 0; reg_CGADSUB = 0;
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  reg_MOSAIC = 0;
  cinematic_text_dirty = 1;
  for (int i = 0; i < 1023; i++)            /* memset7E(7E:3000, 0x2F, 0x7FE): 1023 palabras del texto */
    cinematic_text_tilemap[i] = 0x2F;
  oam_next_ptr = 0;
  cinematic_var5 = 0; cinematic_var6 = 256;
  cinematic_var7 = 0; cinematic_var8 = 0; cinematic_var9 = 0; cinematic_var10 = 0;
  g_word_7E1999 = 0; g_word_7E199B = 0; g_word_7E199D = 0; g_word_7E199F = 0;
  fx_layer_blending_config_a = 0;
  reg_BG2HOFS = 0; reg_BG2VOFS = 0; reg_BG3HOFS = 0; reg_BG3VOFS = 0;
}

/* ------------------------------------------------------------------ fundidos ($8B:90B8...) */
static uint8 AdvanceSlowScreenFadeOut(void) {  // 0x8B90D5
  bool v0 = (int8)(screen_fade_delay - 1) < 0;
  screen_fade_delay = (screen_fade_delay & 0xFF00) | (uint8)(screen_fade_delay - 1);
  if ((uint8)screen_fade_delay && !v0)
    return 0;
  if (reg_INIDISP & 0xF) {
    if ((reg_INIDISP & 0xF) != 1) {
      reg_INIDISP = (reg_INIDISP & 0xF) - 1;
      screen_fade_delay = (screen_fade_delay & 0xFF00) | (uint8)screen_fade_counter;
      return 0;
    }
    reg_INIDISP = 0x80;
    screen_fade_delay &= 0xFF00;
    screen_fade_counter &= 0xFF00;
  }
  return 1;
}

static uint8 AdvanceSlowScreenFadeIn(void) {  // 0x8B911B
  bool v0 = (int8)(screen_fade_delay - 1) < 0;
  screen_fade_delay = (screen_fade_delay & 0xFF00) | (uint8)(screen_fade_delay - 1);
  if ((uint8)screen_fade_delay && !v0)
    return 0;
  if (sign8(((reg_INIDISP + 1) & 0x1F) - 15)) {
    reg_INIDISP = (reg_INIDISP + 1) & 0x1F;
    screen_fade_delay = (screen_fade_delay & 0xFF00) | (uint8)screen_fade_counter;
    return 0;
  }
  reg_INIDISP = 15;
  screen_fade_delay &= 0xFF00;
  screen_fade_counter &= 0xFF00;
  return 1;
}

/* ------------------------------------------------------------------ paletas para los fundidos cruzados ($8B:8C09)
   tilemap_stuff: 0-255 la paleta compuesta, 256-511 la paleta, 512-1279 los canales (8.8), 1280-2047 lo que suman
   por paso. ComposeFadingPalettes rehace las 256; aquí solo se recalculan los colores cuyos canales cambiaron desde la
   última vez (los demás ya están en tilemap_stuff[0-255]): en la Genesis componer los 256 se comía el cuadro. */
#define FADE_RANGES 16
static uint8 fade_full = 1;                     /* recomponer todo (canales escritos de otra forma) */
static uint8 fade_nr;
static uint8 fade_k[FADE_RANGES], fade_n[FADE_RANGES];
static void FadeMark(uint16 i, uint16 n) {
  if (fade_nr == FADE_RANGES) { fade_full = 1; return; }
  fade_k[fade_nr] = (uint8)i;
  fade_n[fade_nr++] = (uint8)n;
}

static void DecomposePaletteDataForFading(void) {  // 0x8B8C09
  for (int i = 0; i < 256; i++) {
    uint16 c = tilemap_stuff[i + 256];
    uint16 r = c & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    tilemap_stuff[i + 512] = r << 8;  tilemap_stuff[i + 1280] = r << 3;
    tilemap_stuff[i + 768] = g << 8;  tilemap_stuff[i + 1536] = g << 3;
    tilemap_stuff[i + 1024] = b << 8; tilemap_stuff[i + 1792] = b << 3;
  }
  fade_full = 1;
}
static void ClearYColorsFromIndexX(uint16 k, uint16 n) {  // 0x8B8C5E
  FadeMark(k >> 1, n);
  for (int i = k >> 1; n; n--, i++)
    tilemap_stuff[i + 256] = tilemap_stuff[i + 512] = tilemap_stuff[i + 768] = tilemap_stuff[i + 1024] = 0;
}
static void FadeOutYColorsFromIndexX(uint16 k, uint16 n) {  // 0x8B8C83
  FadeMark(k >> 1, n);
  uint16 *c = tilemap_stuff + 512 + (k >> 1);
  for (; n; n--, c++) {
    c[0] -= c[768];
    c[256] -= c[1024];
    c[512] -= c[1280];
  }
}
static void FadeInYColorsFromIndexX(uint16 k, uint16 n) {  // 0x8B8CB2
  FadeMark(k >> 1, n);
  uint16 *c = tilemap_stuff + 512 + (k >> 1);
  for (; n; n--, c++) {
    c[0] = (c[768] + c[0]) & 0x1FFF;
    c[256] = (c[1024] + c[256]) & 0x1FFF;
    c[512] = (c[1280] + c[512]) & 0x1FFF;
  }
}
/* colores k..k+n-1 compuestos de sus canales: r | g << 5 | b << 10 (cada canal 0-31 en el byte alto) */
static void FadeCompose(int k, int n) {
  const uint16 *r = tilemap_stuff + 512 + k, *g = r + 256, *b = r + 512;
  uint16 *o = tilemap_stuff + k;
  for (; n > 0; n--)
    *o++ = (uint16)((*r++ >> 8) & 0x1F) | (uint16)((*g++ & 0x1F00) >> 3) | (uint16)((*b++ & 0x1F00) << 2);
}
static void ComposeFadingPalettes(void) {  // 0x8B8CEA
  if (fade_full) {
    FadeCompose(0, 256);
    fade_full = 0;
  } else {
    for (int r = 0; r < fade_nr; r++)
      FadeCompose(fade_k[r], fade_k[r] + fade_n[r] > 256 ? 256 - fade_k[r] : fade_n[r]);
  }
  fade_nr = 0;
  const uint32 *s = (const uint32 *)tilemap_stuff;      /* palette_buffer = tilemap_stuff[0-255] */
  uint32 *d = (uint32 *)palette_buffer;
  for (int i = 0; i < 128; i++) *d++ = *s++;
  nt_pal_dirty(0, 255);
}
static void LoadIntroPaletteForFading(void) {   /* tilemap_stuff[256..511] = kPalettes_Intro ($8C:E3E9) */
  for (int i = 0; i < 256; i++)
    tilemap_stuff[i + 256] = B8CW(0xE3E9 + 2 * i);
}

/* ------------------------------------------------------------------ objetos de fondo ($8B:95A2) */
static void EnableCinematicBgObjects(void) { cinematic_enable_objs |= 0x8000; }
static void EnableCinematicBgTilemapUpdates(void) { cinematic_enable_bg_tilemap |= 0x8000; }
static void DisableCinematicBgTilemapUpdates(void) { cinematic_enable_bg_tilemap &= ~0x8000; }

void ClearCinematicBgObjects(uint16 a) {  // 0x8B95CE
  cinematic_text_dirty = 1;
  for (int i = 0; i < 1024; i++)
    cinematic_text_tilemap[i] = a;
  for (int j = 0; j < 4; j++) {
    cinematicbg_arr1[j] = 0;
    cinematicbg_instr_ptr[j] = 0;
  }
  cinematicbg_var1 = 0;
}

uint8 SpawnCinematicBgObject(uint16 j, uint16 a) {  // 0x8B95F0
  cinematicbg_var3 = a;
  int v = 3;
  while (cinematicbg_instr_ptr[v])
    if (--v < 0)
      return 1;
  cinematicbg_preinstr[v] = B8BW(j + 2);
  cinematicbg_instr_ptr[v] = B8BW(j + 4);
  cinematicbg_instr_timer[v] = 1;
  cinematicbg_arr1[v] = 0;
  cinematicbg_arr6[v] = 0;
  if (B8BW(j) != FUNC16(CinematicFunction_nullsub_116))
    nt_unimpl_v("SpawnCinematicBgObject (inicio sin portar)", B8BW(j));
  return 0;
}

/* objetos de brillo del texto ($8B:97F7): la letra recién escrita pasa por 3 paletas */
static void SpawnTextGlowObject(uint16 j, uint16 r18) {  // 0x8B97F7
  int v = 7;
  while (cinematic_glow_ptr[v])
    if (--v < 0)
      return;
  cinematic_glow_ptr[v] = j;
  cinematic_glow_timer[v] = 1;
  cinematic_glow_x[v] = (uint8)r18;
  cinematic_glow_y[v] = HI8(r18);
  cinematic_glow_pal[v] = 0;
}

static void ProcessTextGlowObject(int v) {  // 0x8B9849
  if (cinematic_glow_timer[v]-- != 1)
    return;
  uint16 pal = cinematic_glow_pal[v];
  uint16 j = cinematic_glow_ptr[v];
  uint16 r22 = 2 * (uint8)cinematic_glow_x[v] + (uint8)cinematic_glow_y[v] * 0x40;
  uint16 o = r22;
  uint8 n0 = B8CB(j + 2), m = B8CB(j + 3), n = n0;
  cinematic_text_dirty = 1;
  for (;;) {
    do {
      cinematic_text_tilemap[o >> 1] = pal | (cinematic_text_tilemap[o >> 1] & 0xE3FF);
      o += 2;
    } while (--n);
    n = n0;
    if (!--m)
      break;
    r22 += 64;
    o = r22;
  }
  if (cinematic_glow_pal[v] == 3072) {
    cinematic_glow_ptr[v] = 0;
  } else {
    cinematic_glow_pal[v] += 1024;
    cinematic_glow_timer[v] = 5;
  }
}

static void HandleTextGlowObjects(void) {  // 0x8B9828
  for (int i = 14; i >= 0; i -= 2) {
    cinematic_var19 = i;
    if (cinematic_glow_ptr[i >> 1]) {
      ProcessTextGlowObject(i >> 1);
      i = cinematic_var19;
    }
  }
}

/* dibujo de un objeto de fondo: la entrada de $8C dice qué rutina y la forma (ancho, alto, palabras del mapa) */
static void DrawToTextTilemap(uint16 j, uint16 r18) {  // 0x8B88B7
  cinematic_text_dirty = 1;
  uint16 offs = 2 * (uint8)r18 + HI8(r18) * 0x40, r22 = offs;
  uint8 m0 = B8CB(j + 2), n = B8CB(j + 3), m = m0;
  for (;;) {
    do {
      cinematic_text_tilemap[offs >> 1] = B8CW(j + 4);
      j += 2;
      offs += 2;
    } while (--m);
    m = m0;
    if (!--n)
      break;
    r22 += 64;
    offs = r22;
  }
}

static void DrawToBgTilemap(uint16 j, uint16 r18) {  // 0x8B88FD
  uint16 offs = 2 * (uint8)r18 + HI8(r18) * 0x40, r22 = offs;
  uint8 m0 = B8CB(j + 2), n = B8CB(j + 3), m = m0;
  for (;;) {
    do {
      cinematic_bg_tilemap[offs >> 1] = B8CW(j + 4);
      j += 2;
      offs += 2;
    } while (--m);
    m = m0;
    if (!--n)
      break;
    r22 += 64;
    offs = r22;
  }
}

static void ProcessCinematicBgObject(uint16 k, uint16 r18) {  // 0x8B8839
  uint16 j = cinematicbg_arr1[k >> 1];
  switch (B8CW(j)) {
  case FUNC16(ProcessCinematicBgObject_Nothing): break;
  case FUNC16(ProcessCinematicBgObject_DrawChar): {   // 0x8B884D: una letra del diario (con brillo y sonido)
    cinematicbg_var1 = cinematicbg_var1 == 0;
    SpawnTextGlowObject(j, r18);
    uint16 ip = cinematicbg_instr_ptr[k >> 1];
    int16 v3;
    if ((int16)B8CW(ip + 6) < 0) {                  /* el cursor ("_") va detrás de la letra */
      cinematicbg_arr7[15] = 8;
      v3 = 8 * (B8CB(ip + 3) + 2);
    } else {
      cinematicbg_arr7[15] = 8 * B8CB(ip + 8);
      v3 = 8 * B8CB(ip + 9);
    }
    cinematicbg_arr8[15] = v3 - 8;
    if (B8CW(ip + 4) != 0xD67D && cinematicbg_var1)
      QueueSfx3_Max6(0xD);
    DrawToTextTilemap(j, r18);
    break;
  }
  case FUNC16(ProcessCinematicBgObject_DrawToTextTilemap): DrawToTextTilemap(j, r18); break;
  case FUNC16(ProcessCinematicBgObject_DrawToBgTilemap): DrawToBgTilemap(j, r18); break;
  default: nt_unimpl_v("ProcessCinematicBgObject (rutina sin portar: modo 7)", B8CW(j)); break;
  }
}

static uint16 CallCinematicBgInstr(uint16 f, uint16 k, uint16 j);

static void ProcessCinematicBgObjectsInstrs(uint16 k) {  // 0x8B9659
  int v = k >> 1;
  switch (cinematicbg_preinstr[v]) {
  case FUNC16(CinematicFunction_nullsub_116): break;
  case FUNC16(CinematicBgPreInstr_SamusBlink):          // 0x8BB4BC
    if (cinematic_function == FUNC16(CinematicFunction_Intro_Func15b) || cinematic_function == FUNC16(nullsub_121)) {
      cinematicbg_instr_ptr[v] = 0xD613;                /* -10733 */
      cinematicbg_instr_timer[v] = 1;
      cinematicbg_preinstr[v] = FUNC16(CinematicFunction_nullsub_116);
    }
    break;
  default: nt_unimpl_v("CinematicBgPreInstr (sin portar)", cinematicbg_preinstr[v]); break;
  }
  uint16 v1 = cinematicbg_var2;
  int v2 = v1 >> 1;
  if (cinematicbg_instr_timer[v2]-- == 1) {
    uint16 p = cinematicbg_instr_ptr[v2], f;
    while ((f = B8CW(p)) & 0x8000) {
      p = CallCinematicBgInstr(f, v1, p + 2);
      if (!p)
        return;
    }
    cinematicbg_instr_timer[v2] = f;
    cinematicbg_arr1[v2] = B8CW(p + 4);
    ProcessCinematicBgObject(v1, B8CW(p + 2));
    cinematicbg_instr_ptr[v2] = p + 6;
  }
}

static void HandleCinematicBgObjects(void) {  // 0x8B962F
  if (cinematic_enable_objs & 0x8000) {
    for (int i = 6; i >= 0; i -= 2) {
      cinematicbg_var2 = i;
      if (cinematicbg_instr_ptr[i >> 1]) {
        ProcessCinematicBgObjectsInstrs(i);
        i = cinematicbg_var2;
      }
    }
    /* UpdateCinematicBgTilemap / UpdateSamusEyesTilemap: colas de VRAM del SNES (la Genesis lee los mapas) */
  }
}

/* ------------------------------------------------------------------ objetos sprite de la intro */
static void SetSprPos(uint16 j, uint16 x, uint16 y, uint16 chr) {
  int v = j >> 1;
  cinematicbg_arr7[v] = x;
  cinematicbg_arr8[v] = y;
  cinematicbg_arr9[v] = chr;
}

static void SetSomeStuffForSpriteObject_15(void) {  // 0x8BADE1
  cinematicbg_arr7[15] = 8;
  cinematicbg_arr8[15] = 248;
}

static void SetSomeStuffForSpriteObject_16(void) {  // 0x8BADEE: el cursor del diario
  cinematicbg_arr7[15] = 8;
  cinematicbg_arr8[15] = 24;
  cinematicspr_instr_ptr[15] = addr_word_8BCBFB;
  cinematicspr_instr_timer[15] = 1;
}

static void EndSprite(int v) {                  /* lista $8B:CE53: borrar el objeto */
  cinematicspr_instr_timer[v] = 1;
  cinematicspr_instr_ptr[v] = addr_off_8BCE53;
}

static const int16 kSSFSO6_Tab[12] = { 0x5c, 0x58, 0x63, 0x58, 0x59, 0x5d, 0x60, 0x5b, 0x66, 0x5e, 0x63, 0x60 };
static const uint16 kF42_Tab0[4] = { 0x70, 0xc0, 0x80, 0xe8 };
static const uint16 kF42_Tab1[4] = { 0x50, 0x40, 0x38, 0x58 };
static const uint16 kF46_Tab0[5] = { 0, 0x10, 0xfff0, 0xfff8, 8 };
static const uint16 kF46_Tab1[5] = { 0, 0xfff0, 8, 0xfff0, 8 };
static const uint16 kF46_Tab2[5] = { 1, 0x10, 0x20, 0x30, 0x40 };
static const uint16 kF47_Tab0[3] = { 0x10, 0xfff0, 0xfff0 };
static const uint16 kF47_Tab1[3] = { 0, 4, 0xfff8 };
static const uint16 kF47_Tab2[3] = { 1, 8, 0x10 };

/* inicio de un objeto sprite (CallCinematicSpriteObjectSetup); 1 si es de la intro */
uint8 IntroSpriteSetup(uint16 f, uint16 j) {
  int v = j >> 1;
  uint16 p = cinematic_spawn_param;
  switch (f) {
  case FUNC16(SetSomeStuffForSpriteObject_2): SetSprPos(j, 8, 24, 3072); return 1;          // 0x8BA8C1
  case FUNC16(SetSomeStuffForSpriteObject_3): SetSprPos(j, 112, 155, 3584); return 1;       // 0x8BA8D5
  case FUNC16(SetSomeStuffForSpriteObject_6):                                                // 0x8BA958
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, kSSFSO6_Tab[2 * p] + 16, kSSFSO6_Tab[2 * p + 1] + 59, 3584);
    return 1;
  case FUNC16(SetSomeStuffForSpriteObject_8):                                                // 0x8BAA9A
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, cinematicbg_arr7[13], cinematicbg_arr8[13], 3584);
    return 1;
  case FUNC16(SetSomeStuffForSpriteObject_10): SetSprPos(j, 84, 139, 3072); return 1;       // 0x8BAD55
  case FUNC16(SetSomeStuffForSpriteObject_12): SetSprPos(j, 112, 111, 3072); return 1;      // 0x8BAD93
  case FUNC16(SetSomeStuffForSpriteObject_17): SetSprPos(j, 128, 216, 3072); return 1;      // 0x8BAE07
  case FUNC16(CinematicFunction_Intro_Func35): SetSprPos(j, 56, 111, 3584); return 1;       // 0x8BB773
  case FUNC16(CinematicFunction_Intro_Func42):                                               // 0x8BB896
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, kF42_Tab0[p], kF42_Tab1[p] - 8, 3584);
    return 1;
  case FUNC16(CinematicFunction_Intro_Func46):                                               // 0x8BB98D
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, kF46_Tab0[p] + 56, kF46_Tab1[p] + 111, 2560);
    cinematicspr_instr_timer[v] = kF46_Tab2[p];
    return 1;
  case FUNC16(CinematicFunction_Intro_Func47):                                               // 0x8BB9D4
    cinematicspr_goto_timer[v] = p;
    SetSprPos(j, kF47_Tab0[p] + 56, kF47_Tab1[p] + 111, 2560);
    cinematicspr_instr_timer[v] = kF47_Tab2[p];
    return 1;
  case FUNC16(CinematicFunction_Intro_Func49): SetSprPos(j, 112, 155, 3584); return 1;      // 0x8BBA4B
  case FUNC16(CinematicFunction_Intro_Func57):                                               // 0x8BBE7E
    if (p) {
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func66);
      cinematicbg_arr7[v] = -32;
    } else {
      cinematicspr_goto_timer[v] = -1024;
      cinematicbg_arr7[v] = 112;
    }
    cinematicbg_arr8[v] = 87;
    cinematicbg_arr9[v] = 2048;
    return 1;
  case FUNC16(CinematicFunction_Intro_Func61): SetSprPos(j, 116, 160, 3072); return 1;      // 0x8BBF4C
  case FUNC16(CinematicFunction_Intro_Func68): SetSprPos(j, 124, 186, 0); return 1;         // 0x8BC083
  case FUNC16(CinematicFunction_Intro_Func71): SetSprPos(j, 124, 204, 0); return 1;         // 0x8BC0B2
  default: return 0;
  }
}

static const int16 kSSFSO7_Tab0[12] = { -1, 0x4000, 0, 0x4000, -1, -0x8000, -1, 0x2000, 0, -0x8000, 0, 0x2000 };
static const uint16 kSSFSO7_Tab1[82] = {
  0xfffe,      0, 0xfffe, 0xc000, 0xfffe, 0xa000, 0xfffe, 0x8000, 0xfffe, 0x6000, 0xfffe, 0x4000,
  0xfffe, 0x2000, 0xffff,      0, 0xffff, 0xc000, 0xffff, 0xa000, 0xffff, 0x8000, 0xffff, 0x6000,
  0xffff, 0x4000, 0xffff, 0x2000,      0,      0,      0, 0x2000,      0, 0x4000,      0, 0x6000,
       0, 0x8000,      0, 0xa000,      0, 0xc000,      0, 0xe000,      1,      0,      1, 0x2000,
       1, 0x4000,      1, 0x6000,      1, 0x8000,      1, 0xa000,      1, 0xc000,      1, 0xe000,
       2,      0,      2, 0x2000,      2, 0x4000,      2, 0x6000,      2, 0x8000,      2, 0xa000,
       2, 0xc000,      2, 0xe000,  0x9dad, 0x991b, 0x1b7d, 0x97ad, 0x991a, 0x1a7d,
};
static const int16 kSSFSO9_Tab0[10] = { -1, 0, -1, -0x8000, 1, 0, 0, -0x8000, -1, -0x8000 };
static const int16 kSSFSO9_Tab2[124] = {
  -2,       0, -2, -0x4000, -2, -0x6000, -2, -0x8000, -2,  0x6000, -2,  0x4000,
  -2,  0x2000, -1,       0, -1, -0x4000, -1, -0x6000, -1, -0x8000, -1,  0x6000,
  -1,  0x4000, -1,  0x2000,  0,       0,  0,  0x2000,  0,  0x4000,  0,  0x6000,
   0, -0x8000,  0, -0x6000,  0, -0x4000,  0, -0x2000,  1,       0,  1,  0x2000,
   1,  0x4000,  1,  0x6000,  1, -0x8000,  1, -0x6000,  1, -0x4000,  1, -0x2000,
   2,       0,  2,  0x2000,  2,  0x4000,  2,  0x6000,  2, -0x8000,  2, -0x6000,
   2, -0x4000,  2, -0x2000,  3,       0,  3,  0x2000,  3,  0x4000,  3,  0x6000,
   3, -0x8000,  3, -0x6000,  3, -0x4000,  3, -0x2000,  4,       0,  4,  0x2000,
   4,  0x4000,  4,  0x6000,  4, -0x8000,  4, -0x6000,  4, -0x4000,  4, -0x2000,
   5,       0,  5,  0x2000,  5,  0x4000,  5,  0x6000,  5, -0x8000,  5, -0x6000,
   5, -0x4000,  5, -0x2000,
};
static const int16 kSSFSO9_Tab1[138] = {
  -3,       0, -3, -0x4000, -3, -0x6000, -3, -0x8000, -3,  0x6000, -3,  0x4000,
  -3,  0x2000, -2,       0, -2, -0x4000, -2, -0x6000, -2, -0x8000, -2,  0x6000,
  -2,  0x4000, -2,  0x2000, -1,       0, -1, -0x4000, -1, -0x6000, -1, -0x8000,
  -1,  0x6000, -1,  0x4000, -1,  0x2000,  0,       0,  0,  0x2000,  0,  0x4000,
   0,  0x6000,  0, -0x8000,  0, -0x6000,  0, -0x4000,  0, -0x2000,  1,       0,
   1,  0x2000,  1,  0x4000,  1,  0x6000,  1, -0x8000,  1, -0x6000,  1, -0x4000,
   1, -0x2000,  2,       0,  2,  0x2000,  2,  0x4000,  2,  0x6000,  2, -0x8000,
   2, -0x6000,  2, -0x4000,  2, -0x2000,  3,       0,  3,  0x2000,  3,  0x4000,
   3,  0x6000,  3, -0x8000,  3, -0x6000,  3, -0x4000,  3, -0x2000,  4,       0,
   4,  0x2000,  4,  0x4000,  4,  0x6000,  4, -0x8000,  4, -0x6000,  4, -0x4000,
   4, -0x2000,  5,       0,  5,  0x2000,  5,  0x4000,  5,  0x6000,  5, -0x8000,
   5, -0x6000,  5, -0x4000,  5, -0x2000,
};
static const uint16 kF45_Tab0[4] = { 0, 0xffff, 0, 0xffff };

static inline void addw(uint16 *hi, uint16 *lo, uint16 h, uint16 l) {   /* AddToHiLo(hi, lo, h:l) */
  add_hilo(hi, lo, (int32)((uint32)h << 16 | l));
}

static void IntroFunc37(uint16 k) {  // 0x8BB846: parpadeo de Mother Brain al recibir un disparo
  int v = k >> 1;
  uint16 n = cinematicspr_arr6[v];
  if (n) {
    for (int i = 0; i < 16; i++)
      palette_buffer[240 + i] = (n & 1) ? B8CW(0xE3E9 + 2 * (240 + i)) : 0x7FFF;
    nt_pal_dirty(240, 255);
    --cinematicspr_arr6[v];
  }
}
static void IntroFunc40(void) {  // 0x8BB877: la pantalla tiembla
  if (cinematic_var12 & 1) cinematic_var10 -= 4; else cinematic_var10 += 4;
}

/* pre-instrucciones de los objetos sprite; 1 si es de la intro */
uint8 IntroSpritePreInstr(uint16 f, uint16 k) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(nullsub_122): return 1;
  case FUNC16(SetSomeStuffForSpriteObject_4_MetroidEgg):                       // 0x8BA8E8
    if (sign16(samus_x_pos - 169)) {
      cinematicspr_instr_timer[v] = 1;
      cinematicspr_instr_ptr[v] = addr_word_8BCB3B;
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_nullsub_116);
    }
    return 1;
  case FUNC16(CinematicSprPreInstr_A903):                                      // 0x8BA903
    if (!cinematic_var13) {
      EndSprite(v);
      cinematic_var15 = 0;
    }
    return 1;
  case FUNC16(SetSomeStuffForSpriteObject_7): {                                // 0x8BA994
    int i = 2 * (uint8)cinematicspr_goto_timer[v];
    addw(&cinematicbg_arr7[v], &cinematicspr_arr6[v], kSSFSO7_Tab0[i], kSSFSO7_Tab0[i + 1]);
    int jj = 2 * HI8(cinematicspr_goto_timer[v]);
    addw(&cinematicbg_arr8[v], &cinematicspr_arr7[v], kSSFSO7_Tab1[jj], kSSFSO7_Tab1[jj + 1]);
    if (sign16(cinematicbg_arr8[v] - 168))
      cinematicspr_goto_timer[v] += 256;
    else
      EndSprite(v);
    return 1;
  }
  case FUNC16(SetSomeStuffForSpriteObject_9): {                                // 0x8BAAB3
    int i = 2 * (uint8)cinematicspr_goto_timer[v];
    addw(&cinematicbg_arr7[v], &cinematicspr_arr6[v], kSSFSO9_Tab0[i], kSSFSO9_Tab0[i + 1]);
    int jj = 2 * HI8(cinematicspr_goto_timer[v]);
    if (cinematicspr_goto_timer[v] & 1)
      addw(&cinematicbg_arr8[v], &cinematicspr_arr7[v], kSSFSO9_Tab2[jj], kSSFSO9_Tab2[jj + 1]);
    else
      addw(&cinematicbg_arr8[v], &cinematicspr_arr7[v], kSSFSO9_Tab1[jj], kSSFSO9_Tab1[jj + 1]);
    if (!sign16(cinematicbg_arr8[v] - 168)) {
      cinematicspr_instr_timer[v] = 1;
      cinematicspr_instr_ptr[v] = addr_word_8BCD71;
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_nullsub_116);
    } else {
      cinematicspr_goto_timer[v] += 256;
    }
    return 1;
  }
  case FUNC16(SetSomeStuffForSpriteObject_11):                                 // 0x8BAD68
    if (cinematic_var13) {
      if (!(cinematic_var4 & 3) && cinematic_var8) {
        --cinematic_var8;
        ++cinematicbg_arr7[v];
      }
    } else {
      EndSprite(v);
    }
    return 1;
  case FUNC16(SetSomeStuffForSpriteObject_13):                                 // 0x8BADA6
    if (cinematic_var13) {
      if (!(cinematic_var4 & 3) && sign16(cinematic_var10 - 8)) {
        ++cinematic_var10;
        --cinematicbg_arr8[v];
      }
    } else {
      EndSprite(v);
    }
    return 1;
  case FUNC16(SetSomeStuffForSpriteObject_18):                                 // 0x8BAE1A
    if (cinematic_var16 == 59) {
      cinematicspr_instr_timer[v] = 1;
      cinematicspr_instr_ptr[v] = addr_word_8BCC23;
    }
    if (cinematic_var13 == 127)
      EndSprite(v);
    return 1;
  case FUNC16(CinematicFunction_Intro_Func36): {                               // 0x8BB786: Mother Brain recibe disparos
    IntroFunc37(k);
    int p = 4;
    while ((projectile_type[p] & 0xFFF) != 256)
      if (--p < 0)
        return 1;
    if (sign16(projectile_x_pos[p] - 84)) {
      KillProjectile(2 * p);
      cinematicspr_arr6[v] = 8;
      if (++cinematicspr_goto_timer[v] == 4) {
        cinematicspr_goto_timer[v] = 0;
        cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func39);
        for (int i = 0; i < 3; i++)
          SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF1B, i);
        for (int i = 0; i < 5; i++)
          SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF15, i);
      }
    }
    return 1;
  }
  case FUNC16(CinematicFunction_Intro_Func39):                                 // 0x8BB80F
    IntroFunc37(k);
    IntroFunc40();
    if (!sign16(++cinematicspr_goto_timer[v] - 128)) {
      cinematicspr_instr_timer[v] = 1;
      cinematicspr_instr_ptr[v] = addr_off_8BCB19;
    }
    return 1;
  case FUNC16(CinematicSprPreInstr_B82E):                                      // 0x8BB82E
    IntroFunc40();
    if (!cinematic_var13) {
      EndSprite(v);
      cinematic_var15 = 0;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func44):                                 // 0x8BB8D8: anillo (Rinka) hacia Samus
    add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], 0x8000);
    add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], 0x8000);
    if (sign16(cinematicbg_arr7[v] + 8 - (samus_x_pos - 5))) {
      if (cinematicspr_preinstr_func[0] != FUNC16(CinematicFunction_Intro_Func39))
        return 1;
    } else {
      samus_invincibility_timer = 11;
      samus_knockback_timer = 11;
      knockback_x_dir = 1;
    }
    EndSprite(v);
    return 1;
  case FUNC16(CinematicFunction_Intro_Func45):                                 // 0x8BB93B
    addw(&cinematicbg_arr7[v], &cinematicspr_arr6[v], kF45_Tab0[cinematicspr_goto_timer[v]], 0x8000);
    add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], 0x8000);
    if (sign16(cinematicbg_arr8[v] - 16) || !sign16(cinematicbg_arr8[v] - 208) ||
        cinematicspr_preinstr_func[0] == FUNC16(CinematicFunction_Intro_Func39))
      EndSprite(v);
    return 1;
  case FUNC16(CinematicFunction_Intro_Func48):                                 // 0x8BBA0F
    if (!cinematic_var13) EndSprite(v);
    return 1;
  case FUNC16(CinematicFunction_Intro_Func50):                                 // 0x8BBA5E: la larva espera
    if (!sign16(cinematicspr_instr_ptr[v + 1] + 0x3487)) {
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func51);
      cinematic_var14 = 0;
      cinematic_var17 = 0;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func51):                                 // 0x8BBA73: sale del huevo
    if (cinematicbg_arr8[v] == 145) {
      for (int i = 0; i < 4; i++)
        SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEF1, i);
      QueueSfx3_Max6(0x23);
    }
    if ((int16)(samus_y_pos - 32 - cinematicbg_arr8[v]) < 0) {
      if (!sign16(cinematic_var17 + 543)) cinematic_var17 -= 32;
    } else if (sign16(cinematic_var17 - 544)) {
      cinematic_var17 += 32;
    }
    add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], INT16_SHL8(cinematic_var17));
    if (!(cinematic_var17 & 0x8000)) {
      cinematic_var14 = 128;
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func52);
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func52):                                 // 0x8BBB0D
    if (!--cinematic_var14 || (cinematic_var14 & 0x8000)) {
      cinematicspr_preinstr_func[v] = FUNC16(CinematicFunction_Intro_Func53);
      cinematic_var14 = 0;
      cinematic_var17 = 0;
      cinematicspr_goto_timer[v] = 0;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func53):                                 // 0x8BBB24: persigue a Samus
    if (cinematic_var13) {
      if (sign16(cinematicspr_goto_timer[v] - 128)) {
        uint16 t = ++cinematicspr_goto_timer[v];
        if (!(t & 0x3F))
          QueueSfx3_Max6(0x23);
      }
      if ((int16)(samus_x_pos - cinematicbg_arr7[v]) < 0) {
        if (!sign16(cinematic_var14 + 639)) cinematic_var14 -= 32;
      } else if (sign16(cinematic_var14 - 640)) {
        cinematic_var14 += 32;
      }
      cinematic_var15 = sign16(cinematic_var14) ? 1 : -1;
      add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], INT16_SHL8(cinematic_var14));
      if ((int16)(samus_y_pos - 8 - cinematicbg_arr8[v]) < 0) {
        if (!sign16(cinematic_var17 + 543)) cinematic_var17 -= 32;
      } else if (sign16(cinematic_var17 - 544)) {
        cinematic_var17 += 32;
      }
      add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], INT16_SHL8(cinematic_var17));
    } else {
      EndSprite(v);
      cinematic_var15 = 0;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func58):                                 // 0x8BBEB5: la nave se acerca
    if (cinematic_function == FUNC16(CinematicFunction_Intro_Func56)) {
      cinematicspr_goto_timer[v] += 128;
      int32 d = INT16_SHL8(cinematicspr_goto_timer[v]);
      add_hilo(&cinematicbg_arr8[v], &cinematicspr_arr7[v], d);
      add_hilo(&cinematicbg_arr7[v], &cinematicspr_arr6[v], d);
      add_hilo(&cinematic_var8, &cinematic_var7, d);
      add_hilo(&cinematic_var10, &cinematic_var9, d);
    }
    return 1;
  default: return 0;
  }
}

/* instrucciones de los objetos sprite; devuelve el puntero siguiente (0: termina), o 0xFFFF si no es de la intro */
uint16 IntroSpriteInstr(uint16 f, uint16 k, uint16 j) {
  int v = k >> 1;
  switch (f) {
  case FUNC16(CinematicSprInstr_SpawnMetroidEggParticles):                    // 0x8BA918
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCECD, 0);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCED3, 1);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCED9, 2);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEDF, 3);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEE5, 4);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCEEB, 5);
    QueueSfx2_Max6(0xB);
    return j;
  case FUNC16(CinematicSprInstr_StartIntroPage2): cinematic_function = FUNC16(CinematicFunction_Intro_Page2); DisableCinematicBgTilemapUpdates(); return j;
  case FUNC16(CinematicSprInstr_StartIntroPage3): cinematic_function = FUNC16(CinematicFunction_Intro_Page3); DisableCinematicBgTilemapUpdates(); return j;
  case FUNC16(CinematicSprInstr_StartIntroPage4): cinematic_function = FUNC16(CinematicFunction_Intro_Page4); DisableCinematicBgTilemapUpdates(); return j;
  case FUNC16(CinematicSprInstr_StartIntroPage5): cinematic_function = FUNC16(CinematicFunction_Intro_Page5); DisableCinematicBgTilemapUpdates(); return j;
  case FUNC16(CinematicSprInstr_Func43):                                       // 0x8BB8C5
    cinematicspr_preinstr_func[v] = cinematicspr_goto_timer[v] ? FUNC16(CinematicFunction_Intro_Func45)
                                                              : FUNC16(CinematicFunction_Intro_Func44);
    return j;
  case FUNC16(CinematicSprInstr_SpawnIntroRinkas01):                           // 0x8BBA21
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF21, 0);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF21, 1);
    return j;
  case FUNC16(CinematicSprInstr_SpawnIntroRinkas23):                           // 0x8BBA36
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF21, 2);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF21, 3);
    return j;
  case FUNC16(CinematicCommonInstr_Func69): return japanese_text_flag ? j : j + 6;   // 0x8BC096
  case FUNC16(CinematicCommonInstr_Func70):                                    // 0x8BC0A2
    cinematic_function = FUNC16(CinematicFunction_Intro_Func72);
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    return j;
  default: return 0xFFFF;
  }
}

/* ------------------------------------------------------------------ instrucciones de los objetos de fondo */
static uint16 CinematicBgInstr_SetSomeStuffForSpriteObject_14(uint16 j) {  // 0x8BADD4: el cursor parpadea
  cinematicspr_instr_ptr[15] = addr_unk_8BCC03;
  cinematicspr_instr_timer[15] = 1;
  return j;
}

static uint16 CallCinematicBgInstr(uint16 f, uint16 k, uint16 j) {
  int v = k >> 1;
  if (japanese_text_flag && f != FUNC16(CinematicBgInstr_Delete) && f != FUNC16(CinematicBgInstr_Goto))
    nt_unimpl("CinematicBgInstr (texto en japonés)");
  switch (f) {
  case FUNC16(CinematicBgInstr_Delete): cinematicbg_arr1[v] = 0; cinematicbg_instr_ptr[v] = 0; return 0;
  case FUNC16(CinematicBgInstr_Goto): return B8CW(j);
  case FUNC16(CinematicBgInstr_SetSomeStuffForSpriteObject_14): return CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
  case FUNC16(CinematicBgInstr_HandleCreateJpnText_Page1): case FUNC16(CinematicBgInstr_HandleCreateJpnText_Page2):
  case FUNC16(CinematicBgInstr_HandleCreateJpnText_Page3): case FUNC16(CinematicBgInstr_HandleCreateJpnText_Page4):
  case FUNC16(CinematicBgInstr_HandleCreateJpnText_Page5): case FUNC16(CinematicBgInstr_Func16):
    cinematicbg_var1 = 0;
    return j;
  case FUNC16(CinematicBgInstr_SpawnMarkerWaitInput_Page1):
    j = CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
    cinematic_function = FUNC16(CinematicFunction_Intro_WaitInputSetupMotherBrainFight);
    return j;
  case FUNC16(CinematicBgInstr_SpawnMarkerWaitInput_Page2):
    j = CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
    cinematic_function = FUNC16(CinematicFunction_Intro_WaitInputSetupBabyMetroid);
    return j;
  case FUNC16(CinematicBgInstr_SpawnMarkerWaitInput_Page3):
    j = CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func11);
    return j;
  case FUNC16(CinematicBgInstr_SpawnMarkerWaitInput_Page4):
    j = CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func12);
    return j;
  case FUNC16(CinematicBgInstr_SpawnMarkerWaitInput_Page5):
    j = CinematicBgInstr_SetSomeStuffForSpriteObject_14(j);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func15);
    return j;
  case FUNC16(CinematicBgInstr_Func17):                                        // 0x8BB240
    cinematic_function = FUNC16(CinematicFunction_Intro_Func34);
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    return j;
  case FUNC16(CinematicCommonInstr_Func69): return japanese_text_flag ? j : j + 6;
  case FUNC16(CinematicCommonInstr_Func70):
    cinematic_function = FUNC16(CinematicFunction_Intro_Func72);
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    return j;
  default:
    nt_unimpl_v("CinematicBgInstr (instrucción sin portar)", f);
    cinematicbg_arr1[v] = 0; cinematicbg_instr_ptr[v] = 0;
    return 0;
  }
}

/* ------------------------------------------------------------------ entrada de demo de Samus ($91:834E) */
static inline uint16 B91W(uint16 a) { return kBank91_Demo[(uint16)(a - kBank91_Demo_LO)]; }

static void EnableDemoInput(void) {  // 0x91834E
  samus_input_handler = FUNC16(Samus_InputHandler_E91D);
  demo_enable |= 0x8000;
}
static void DisableDemoInput(void) {  // 0x91835F
  samus_input_handler = FUNC16(Samus_InputHandler_E913);
  demo_enable &= ~0x8000;
}
static void ResetDemoData(void) {  // 0x918370
  demo_input_pre_instr = 0;
  demo_input_instr_timer = 0;
  demo_input_instr_ptr = 0;
  demo_timer_counter = 0;
  xray_angle = 0;
  demo_input = 0;
  demo_input_new = 0;
  demo_input_prev = 0;
  demo_input_prev_new = 0;
  demo_enable = 0;
}
static void LoadDemoInputObject(uint16 a, uint16 j) {  // 0x918395
  xray_angle = a;
  demo_input_pre_instr = B91W(j + 2);
  demo_input_instr_ptr = B91W(j + 4);
  demo_input_instr_timer = 1;
  demo_timer_counter = 0;
}

static void DemoSamusFacingLeftStill(void) {   /* DemoInstr_Func3 / UNUSED_DemoInstr_Func4 */
  samus_pose = kPose_02_FaceL_Normal;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;
  samus_last_different_pose_movement_type = samus_prev_movement_type;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type = samus_movement_type;
  DisableDemoInput();
}

static uint16 CallDemoInstr(uint16 f, uint16 j) {
  switch (f) {
  case 0x8427: demo_input_instr_ptr = 0; demo_input = 0; demo_input_new = 0; return 0;   // DemoInstr_Finish
  case 0x8434: demo_input_pre_instr = B91W(j); return j + 2;                             // DemoInstr_SetPreInstr
  case 0x843F: demo_input_pre_instr = 0x8447; return j;                                  // DemoInstr_ClearPreInstr
  case 0x8448: return B91W(j);                                                           // DemoInstr_Goto
  case 0x844F: return --demo_timer_counter ? B91W(j) : j + 2;                            // DemoInstr_DecTimerAndGoto
  case 0x8459: demo_timer_counter = B91W(j); return j + 2;                               // DemoInstr_SetTimer
  case 0x85FC:                                                                           // DemoInstr_Func2
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func11);
    frame_handler_beta = FUNC16(Samus_FrameHandlerBeta_Func17);
    DisableDemoInput();
    return j;
  case 0x8682:                                                                           // DemoInstr_Disable
    frame_handler_alfa = FUNC16(EmptyFunction);
    frame_handler_beta = FUNC16(EmptyFunction);
    DisableDemoInput();
    return j;
  case 0x8739:                                                                           // DemoInstr_Func3
    frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func13);
    DemoSamusFacingLeftStill();
    samus_input_handler = FUNC16(nullsub_152);
    return j;
  default:
    nt_unimpl_v("DemoInstr (instrucción sin portar)", f);
    demo_input_instr_ptr = 0;
    return 0;
  }
}

static void ProcessDemoInputObject(void) {  // 0x9183F2
  switch (demo_input_pre_instr) {
  case 0x83BF: case 0x8447: case 0: break;                                               // DemoPreInstr_nullsub_162 / RTS
  case 0x864F:                                                                           // DemoPreInstr_864F
    if (sign16(samus_x_pos - 178)) {
      demo_input_pre_instr = 0x866A;
      demo_input_instr_ptr = 0x8623;
      demo_input_instr_timer = 1;
    }
    break;
  case 0x866A:                                                                           // DemoPreInstr_866A
    if (!cinematic_var13) {                     /* eproj_x_pos[0] del decomp: 7E:1A4B, cinematic_var13 en la intro */
      demo_input_pre_instr = 0x83BF;            /* DemoPreInstr_nullsub_162 */
      demo_input_instr_ptr = 0x864B;            /* addr_off_91864B */
      demo_input_instr_timer = 1;
    }
    break;
  default: nt_unimpl_v("DemoPreInstr (sin portar)", demo_input_pre_instr); break;
  }
  if (!--demo_input_instr_timer) {
    uint16 p = demo_input_instr_ptr, f;
    while ((f = B91W(p)) & 0x8000) {
      p = CallDemoInstr(f, p + 2);
      if (!p)
        return;
    }
    demo_input_instr_timer = f;
    demo_input = B91W(p + 2);
    demo_input_new = B91W(p + 4);
    demo_input_instr_ptr = p + 6;
  }
}

void DemoObjectInputHandler(void) {  // 0x9183C0
  if ((demo_enable & 0x8000) && demo_input_instr_ptr) {
    ProcessDemoInputObject();
    joypad1_input_samusfilter = demo_input_prev;
    joypad1_newinput_samusfilter = demo_input_prev_new;
    joypad1_lastkeys = demo_input;
    demo_input_prev = demo_input;
    joypad1_newkeys = demo_input_new;
    demo_input_prev_new = demo_input_new;
  }
}

/* ------------------------------------------------------------------ recuerdos */
static void SetSamusStill(uint16 x, uint16 y) {   /* Samus de pie mirando a la izquierda en (x, y) */
  samus_pose = kPose_02_FaceL_Normal;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;
  samus_last_different_pose_movement_type = samus_prev_movement_type;
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type = samus_movement_type;
  samus_x_pos = samus_prev_x_pos = x;
  samus_y_pos = samus_prev_y_pos = y;
}

static void SpawnIntroCutsceneCrossfadeHdma(void) {  // 0x88EBF0
  SpawnHdmaObject(0x88, 0xEC03);
}

static void CinematicFunction_Intro_SetupTransitionToGameplay(void) {  // 0x8BB018
  reg_TM = 6; reg_TS = 17; reg_CGWSEL = 2; reg_CGADSUB = 0;
  SpawnIntroCutsceneCrossfadeHdma();
  SetSomeStuffForSpriteObject_15();
  cinematic_function = FUNC16(CinematicFunction_Intro_XfadeGameplayFade);
  LoadIntroPaletteForFading();
  DecomposePaletteDataForFading();
  ClearYColorsFromIndexX(0x28, 3);
  ClearYColorsFromIndexX(0xE0, 0x10);
  ClearYColorsFromIndexX(0x180, 0x20);
  ClearYColorsFromIndexX(0x1E0, 0x10);
  ComposeFadingPalettes();
}

static void CinematicFunction_Intro_Func13(void) {  // 0x8BB151
  reg_TM = 6; reg_TS = 17; reg_CGWSEL = 2; reg_CGADSUB = 0;
  SpawnIntroCutsceneCrossfadeHdma();
  SetSomeStuffForSpriteObject_15();
  cinematic_function = FUNC16(CinematicFunction_Intro_XfadeScientistFade);
  LoadIntroPaletteForFading();
  DecomposePaletteDataForFading();
  ClearYColorsFromIndexX(0x40, 0x10);
  ClearYColorsFromIndexX(0x1C0, 9);
  ComposeFadingPalettes();
}

static void CinematicFunction_Intro_SetupStuff(void) {  // 0x8BB3A1
  reg_TM = 6; reg_TS = 17; reg_CGWSEL = 2; reg_CGADSUB = 0;
  LoadIntroPaletteForFading();
  DecomposePaletteDataForFading();
  ClearYColorsFromIndexX(0, 0x10);
  ClearYColorsFromIndexX(0x60, 0x10);
  ClearYColorsFromIndexX(0x1D2, 6);
  ComposeFadingPalettes();
  cinematic_var4 = 127;
  EnableCinematicBgTilemapUpdates();
  SetSomeStuffForSpriteObject_16();
}

static void ClearEnglishTextTilemap(void) {  // 0x8BA856
  cinematic_text_dirty = 1;
  for (int i = 128; i < 768; i++)
    cinematic_text_tilemap[i] = 47;
}

static void ResetButtonAssignmentsToDefault(void) {  // 0x8BBC08
  button_config_shoot_x_saved = button_config_shoot_x;
  button_config_jump_a_saved = button_config_jump_a;
  button_config_run_b_saved = button_config_run_b;
  button_config_itemcancel_y_saved = button_config_itemcancel_y;
  button_config_itemswitch_saved = button_config_itemswitch;
  button_config_aim_down_L_saved = button_config_aim_down_L;
  button_config_aim_up_R_saved = button_config_aim_up_R;
  button_config_up = kButton_Up;
  button_config_down = kButton_Down;
  button_config_left = kButton_Left;
  button_config_right = kButton_Right;
  button_config_shoot_x = kButton_X;
  button_config_jump_a = kButton_A;
  button_config_run_b = kButton_B;
  button_config_itemcancel_y = kButton_Y;
  button_config_itemswitch = kButton_Select;
  button_config_aim_down_L = kButton_L;
  button_config_aim_up_R = kButton_R;
}

static void RevertButtonConfig(void) {  // 0x8BBC75
  button_config_shoot_x = button_config_shoot_x_saved;
  button_config_jump_a = button_config_jump_a_saved;
  button_config_run_b = button_config_run_b_saved;
  button_config_itemcancel_y = button_config_itemcancel_y_saved;
  button_config_itemswitch = button_config_itemswitch_saved;
  button_config_aim_down_L = button_config_aim_down_L_saved;
  button_config_aim_up_R = button_config_aim_up_R_saved;
}

/* ------------------------------------------------------------------ funciones de la intro (cinematic_function) */
static void CinematicFunction_Intro_Initial(void) {  // 0x8BA395
  SetupPpu_Intro();
  cur_irq_handler = 0;
  irqhandler_next_handler = 0;
  area_index = 0;
  room_width_in_blocks = 16;
  room_height_in_blocks = 16;
  Samus_Initialize();
  UpdateBeamTilesAndPalette();
  samus_max_missiles = 900;
  samus_missiles = 900;
  cinematic_var15 = 0;
  samus_draw_handler = FUNC16(SamusDrawHandler_Default);
  ResetButtonAssignmentsToDefault();
  samus_invincibility_timer = 0;
  samus_knockback_timer = 0;
  nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = B8CW(0xE3E9 + 2 * i);
  nt_pal_dirty(0, 255);
  IntroGfx(INTRO_GFX_TEXT);                 /* DecompressToMem y DMA de los tiles: la Genesis */
  EnableHdmaObjects();
  ClearPaletteFXObjects();
  EnablePaletteFx();
  door_def_ptr = 0;
  layer1_x_pos = 0;
  layer1_y_pos = 0;
  cinematic_var16 = 0;
  for (int j = 0; j < 1024; j++)
    cinematic_bg_tilemap[j] = kIntroBgTilemap[j];
  cinematic_text_dirty = 1;
  for (int k = 0; k < 128; k++)
    cinematic_text_tilemap[k] = B8CW(0xD81B + 2 * k);
  cinematic_var11 = -1;
  cinematic_function = FUNC16(CinematicFunction_Intro_FadeIn);
  QueueMusic_Delayed8(0);
  QueueMusic_Delayed8(0xFF3F);
}

static void CinematicFunc_Intro_SetupTextPage2(void) {  // 0x8BA66F
  reg_TM = 22;
  ClearCinematicBgObjects(0x2F);
  EnableCinematicBgObjects();
  EnableCinematicBgTilemapUpdates();
  cinematicbg_var3 = 0x4C00;                /* addr_unk_604C00 */
  cinematic_text_dirty = 1;
  for (int i = 0; i < 128; i++) {
    cinematic_text_tilemap[i] = 15401;
    cinematic_text_tilemap[i + 896] = 15401;
  }
  for (int i = 768, v2 = 0; i < 896; i++, v2 += 2)
    cinematic_text_tilemap[i] = B8BW(0xA72B + v2);
  cinematic_text_tilemap[911] = 0x1C29;     /* *(uint32 *)&ram3000.menu.menu_tilemap[286] = 0x1C291C29 (7E:371E) */
  cinematic_text_tilemap[912] = 0x1C29;
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE6D, 0x1C29);
  cinematic_var4 = 127;
  SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF63, 0x4C00);
  cinematic_var10 = 8;
  reg_BG2VOFS = 8;
  reg_BG3VOFS = 8;
  ClearEnglishTextTilemap();
  SetSomeStuffForSpriteObject_16();
  QueueMusic_Delayed8(0);
  QueueMusic_Delayed8(0xFF36);
  QueueMusic_DelayedY(5, 0xE);
  cinematic_function = FUNC16(CinematicFunc_Intro_WaitFadeinShowText);
}

static uint8 WaitPageInput(void) {   /* las páginas: después de un rato, cualquier botón sigue */
  if (cinematic_var16) {
    --cinematic_var16;
    return 0;
  }
  return joypad1_newkeys != 0;
}

static void CinematicFunction_Intro_WaitInputSetupMotherBrainFight(void) {  // 0x8BAEB8
  if (!WaitPageInput())
    return;
  reg_BG1SC = 80;
  IntroGfx(INTRO_GFX_MOTHER_BRAIN);
  SetSamusStill(155, 115);
  cinematic_var8 = 0;
  cinematic_var13 = 127;
  SpawnCinematicSpriteObjectToR18(addr_kCinematicSpriteObjectDef_8BCE55, 0);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF27, 0);
  lvl_load(LVL_INTRO_MB);                   /* level_data = kLevelData_MotherBrainRoomFromCutscene; BTS = 0 */
  hud_item_index = 1;
  frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func12);
  frame_handler_beta = FUNC16(Samus_Func15);
  ResetDemoData();
  EnableDemoInput();
  LoadDemoInputObject(0, 0x8784);           /* addr_stru_918784 */
  cinematic_var15 = -1;
  CinematicFunction_Intro_SetupTransitionToGameplay();
}

static void CinematicFunction_Intro_WaitInputSetupBabyMetroid(void) {  // 0x8BAF6C
  if (!WaitPageInput())
    return;
  reg_BG1SC = 84;
  IntroGfx(INTRO_GFX_BABY);
  room_width_in_blocks = 32;
  room_height_in_blocks = 16;
  SetSamusStill(376, 147);
  cinematic_var8 = 0;
  cinematic_var13 = 127;
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE5B, 0x7F);
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE79, 0);
  cinematic_var11 = 0;
  lvl_load(LVL_INTRO_BABY);
  hud_item_index = 0;
  frame_handler_alfa = FUNC16(Samus_FrameHandlerAlfa_Func12);
  frame_handler_beta = FUNC16(Samus_Func15);
  ResetDemoData();
  EnableDemoInput();
  LoadDemoInputObject(0, 0x877E);           /* addr_stru_91877E */
  cinematic_var15 = 1;
  CinematicFunction_Intro_SetupTransitionToGameplay();
}

static void CinematicFunction_Intro_Func15b(void) {  // 0x8BB207
  EnableCinematicBgTilemapUpdates();
  ClearEnglishTextTilemap();
  SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF5D, 0x4C00);
  cinematic_function = FUNC16(nullsub_121);
  SetSomeStuffForSpriteObject_16();
  QueueMusic_Delayed8(0);
}

static void XfadeEnd(void) {   /* fin de un fundido cruzado: solo el recuerdo; el texto se borra */
  reg_TM = 21; reg_TS = 0; reg_CGWSEL = 0;
  cinematic_text_dirty = 1;
  for (int i = 128; i < 768; i++)
    cinematic_text_tilemap[i] = 47;
  cinematic_function = FUNC16(CinematicFunc_Nothing);
}

static void CinematicFunction_Intro_Func34(void) {  // 0x8BB72F: fin de las páginas
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    reg_BG2HOFS = 0; reg_BG3HOFS = 0; reg_BG2VOFS = 0; reg_BG3VOFS = 0;
    cinematic_var5 = 0;
    nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func54);
    RevertButtonConfig();
    samus_max_missiles = 0;
    samus_missiles = 0;
  }
}

static void CinematicFunction_Intro_Func54(void) {  // 0x8BBCA0: el vuelo a Ceres
  SetupPpu_3_Mode7();
  ClearCinematicSprites();
  cur_irq_handler = 0;
  irqhandler_next_handler = 0;
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = kPalettes_Intro2[i];
  nt_pal_dirty(0, 255);
  IntroGfx(INTRO_GFX_FLIGHT);
  reg_M7A = 256; reg_M7B = 0; reg_M7C = 0; reg_M7D = 256;
  reg_M7X = 56; reg_M7Y = 24;
  cinematic_var8 = -72;
  cinematic_var10 = -104;
  cinematic_var5 = 224;
  cinematic_var6 = 512;
  ClearPaletteFXObjects();
  EnablePaletteFx();
  ClearCinematicBgObjects(0);
  cinematicbg_var3 = 23552;
  SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF0F, 0);
  cinematic_function = FUNC16(CinematicFunction_Intro_Func55);
  QueueMusic_Delayed8(0xFF2D);
  QueueMusic_DelayedY(5, 0xE);
}

static void CinematicFunction_Intro_Func56(void) {  // 0x8BBDF9
  if (sign16(cinematic_var6 - 32)) {
    reg_CGWSEL = 0;
    reg_CGADSUB = 49;
    reg_COLDATA[0] = 63;
    reg_COLDATA[1] = 95;
    reg_COLDATA[2] = 0x9F;
    ClearCinematicSprites();
    Mode7Transfers(addr_kCinematicFunction_Intro_Func56_M7);   /* la nave vista de atrás (capa de la Genesis) */
    cinematic_var8 = -32;
    cinematic_var10 = -128;
    cinematic_var5 = 32;
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF39, 0);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE85, 0);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE8B, 0);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE91, 1);
    SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCF0F, 1);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func67);
    SpawnPalfxObject(addr_kPalfx_E1A8);
    SpawnPalfxObject(addr_kPalfx_E1AC);
  } else {
    cinematic_var6 -= 16;
  }
}

static void CinematicFunction_Intro_Func67(void) {  // 0x8BBFDA: destello blanco y "SPACE COLONY"
  if (sign8(--reg_COLDATA[0] - 32)) reg_COLDATA[0] = 32;
  if (sign8(--reg_COLDATA[1] - 64)) reg_COLDATA[1] = 64;
  if (sign8(--reg_COLDATA[2] + 0x80)) reg_COLDATA[2] = 0x80;
  add_hilo(&cinematic_var8, &cinematic_var7, -0x2000);
  if (sign16(cinematic_var6 - 3072)) {
    cinematic_var6 += 16;
  } else if (sign16(cinematic_var6 - 0x2000)) {
    cinematic_var6 += 32;
  } else {
    reg_TM = 17;
    reg_BGMODE = 1;
    reg_M7SEL = 0;
    reg_BG1SC = 92;
    reg_BG12NBA = 6;
    cinematic_var6 = 256;
    cinematic_var5 = 0; cinematic_var7 = 0; cinematic_var8 = 0; cinematic_var9 = 0; cinematic_var10 = 0;
    EnableCinematicBgObjects();
    EnableCinematicBgTilemapUpdates();
    SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF69, 0x5C00);
    cinematic_function = FUNC16(nullsub_120);
  }
}

static void CinematicFunction_Intro_Func72(void) {  // 0x8BC0C5
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    reg_BG2HOFS = 0; reg_BG3HOFS = 0; reg_BG2VOFS = 0; reg_BG3VOFS = 0;
    cinematic_var5 = 0;
    nt_clear_ram(CINE_RAM_FROM, CINE_RAM_TO);
    cinematic_function = FUNC16(CinematicFunction_Intro_Func73);
  }
}

/* una función de la intro; 1 si es de la intro */
uint8 IntroCinematicFunction(uint16 f) {
  switch (f) {
  case FUNC16(CinematicFunc_Nothing): case FUNC16(nullsub_121): return 1;
  case FUNC16(CinematicFunction_Intro_Initial): CinematicFunction_Intro_Initial(); return 1;
  case FUNC16(CinematicFunction_Intro_FadeIn):                                // 0x8BA5A7
    if (!(HasQueuedMusic() & 1)) {
      cinematic_function = FUNC16(CinematicFunction_Intro_DrawInitJpn);
      screen_fade_delay = 2;
      screen_fade_counter = 2;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_DrawInitJpn):                           // 0x8BA5BD
    if (AdvanceSlowScreenFadeIn() & 1) {
      cinematic_function = FUNC16(CinematicFunction_Intro_LastMetroidCaptivity);
      cinematic_var4 = 60;
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_LastMetroidCaptivity):                  // 0x8BA5F8
    if (!--cinematic_var4 || (cinematic_var4 & 0x8000)) {
      cinematic_function = FUNC16(CinematicFunc_Intro_QueueGalaxyIsAtPeace);
      cinematic_var4 = 200;
      QueueMusic_Delayed8(5);
    }
    return 1;
  case FUNC16(CinematicFunc_Intro_QueueGalaxyIsAtPeace):                     // 0x8BA613
    if (!--cinematic_var4 || (cinematic_var4 & 0x8000)) {
      QueueMusic_Delayed8(0);
      QueueMusic_Delayed8(0xFF42);
      QueueMusic_DelayedY(5, 0xE);
      cinematic_function = FUNC16(CinematicFunc_Intro_WaitForQueueWait4secs);
    }
    return 1;
  case FUNC16(CinematicFunc_Intro_WaitForQueueWait4secs):                    // 0x8BA639
    if (!(HasQueuedMusic() & 1)) {
      cinematic_function = FUNC16(CinematicFunc_Intro_FadeOut);
      cinematic_var4 = 240;
    }
    return 1;
  case FUNC16(CinematicFunc_Intro_FadeOut):                                  // 0x8BA64C
    if (!--cinematic_var4 || (cinematic_var4 & 0x8000)) {
      cinematic_function = FUNC16(CinematicFunc_Intro_WaitForFadeOut);
      screen_fade_delay = 2;
      screen_fade_counter = 2;
    }
    return 1;
  case FUNC16(CinematicFunc_Intro_WaitForFadeOut):                           // 0x8BA663
    if (AdvanceSlowScreenFadeOut() & 1)
      cinematic_function = FUNC16(CinematicFunc_Intro_SetupTextPage2);
    return 1;
  case FUNC16(CinematicFunc_Intro_SetupTextPage2): CinematicFunc_Intro_SetupTextPage2(); return 1;
  case FUNC16(CinematicFunc_Intro_WaitFadeinShowText):                       // 0x8BA82B
    if (!(HasQueuedMusic() & 1)) {
      cinematic_function = FUNC16(CinematicFunc_Intro_WaitForFadeinSleep);
      screen_fade_delay = 2;
      screen_fade_counter = 2;
      SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF3F, 0x4C00);
    }
    return 1;
  case FUNC16(CinematicFunc_Intro_WaitForFadeinSleep):                       // 0x8BA84A
    if (AdvanceSlowScreenFadeIn() & 1)
      cinematic_function = FUNC16(CinematicFunc_Nothing);
    return 1;
  case FUNC16(CinematicFunction_Intro_WaitInputSetupMotherBrainFight): CinematicFunction_Intro_WaitInputSetupMotherBrainFight(); return 1;
  case FUNC16(CinematicFunction_Intro_WaitInputSetupBabyMetroid): CinematicFunction_Intro_WaitInputSetupBabyMetroid(); return 1;
  case FUNC16(CinematicFunction_Intro_Func11):                               // 0x8BB0F2: científico
    if (WaitPageInput()) {
      reg_BG1SC = 88;
      IntroGfx(INTRO_GFX_SCIENTIST);
      cinematic_var8 = 32;
      cinematic_var10 = 8;
      cinematic_var13 = 127;
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE61, 0x7F);
      CinematicFunction_Intro_Func13();
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func12):                               // 0x8BB123: laboratorio
    if (WaitPageInput()) {
      reg_BG1SC = 92;
      IntroGfx(INTRO_GFX_LAB);
      cinematic_var8 = 0;
      cinematic_var10 = -24;
      cinematic_var13 = 127;
      SpawnCinematicSpriteObject(addr_kCinematicSpriteObjectDef_8BCE67, 0x7F);
      CinematicFunction_Intro_Func13();
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func15):                               // 0x8BB1DA
    if (WaitPageInput()) {
      cinematic_var13 = 127;
      CinematicFunction_Intro_Func15b();
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func15b): CinematicFunction_Intro_Func15b(); return 1;
  case FUNC16(CinematicFunction_Intro_XfadeGameplayFade):                    // 0x8BB250
    if (!(cinematic_var4 & 3)) {
      FadeOutYColorsFromIndexX(0, 0x14);
      FadeOutYColorsFromIndexX(0x60, 0x10);
      FadeOutYColorsFromIndexX(0x1D2, 6);
      FadeInYColorsFromIndexX(0x28, 3);
      FadeInYColorsFromIndexX(0xE0, 0x10);
      FadeInYColorsFromIndexX(0x180, 0x20);
      FadeInYColorsFromIndexX(0x1E0, 0x10);
      ComposeFadingPalettes();
    }
    if (--cinematic_var4 & 0x8000) {
      XfadeEnd();
      if (cinematic_var15 & 0x8000)
        SpawnPalfxObject(addr_kPalfx_E1BC);
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_XfadeScientistFade):                   // 0x8BB2D2
    if (!(cinematic_var4 & 3)) {
      FadeOutYColorsFromIndexX(0, 0x14);
      FadeOutYColorsFromIndexX(0x60, 0x10);
      FadeOutYColorsFromIndexX(0x1D2, 6);
      FadeInYColorsFromIndexX(0x40, 0x10);
      FadeInYColorsFromIndexX(0x1C0, 9);
      ComposeFadingPalettes();
    }
    if (--cinematic_var4 & 0x8000)
      XfadeEnd();
    return 1;
  case FUNC16(CinematicFunction_Intro_Page2):                                // 0x8BB35F
    SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF45, 0x4C00);
    cinematic_function = FUNC16(CinematicFunction_Intro_Fadestuff);
    CinematicFunction_Intro_SetupStuff();
    return 1;
  case FUNC16(CinematicFunction_Intro_Page3):
    SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF4B, 0x4C00);
    cinematic_function = FUNC16(CinematicFunction_Intro_Fadestuff);
    CinematicFunction_Intro_SetupStuff();
    return 1;
  case FUNC16(CinematicFunction_Intro_Page4):
    SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF51, 0x4C00);
    cinematic_function = FUNC16(CinematicFunction_Intro_Fadestuff2);
    CinematicFunction_Intro_SetupStuff();
    return 1;
  case FUNC16(CinematicFunction_Intro_Page5):
    SpawnCinematicBgObject(addr_kCinematicBgObjectDef_8BCF57, 0x4C00);
    cinematic_function = FUNC16(CinematicFunction_Intro_Fadestuff2);
    CinematicFunction_Intro_SetupStuff();
    return 1;
  case FUNC16(CinematicFunction_Intro_Fadestuff):                            // 0x8BB3F4: del recuerdo al texto
    if (!(cinematic_var13 & 3)) {
      FadeInYColorsFromIndexX(0, 0x10);
      FadeInYColorsFromIndexX(0x60, 0x10);
      FadeInYColorsFromIndexX(0x1D2, 6);
      FadeOutYColorsFromIndexX(0x28, 3);
      FadeOutYColorsFromIndexX(0xE0, 0x10);
      FadeOutYColorsFromIndexX(0x180, 0x20);
      FadeOutYColorsFromIndexX(0x1E0, 0x10);
      ComposeFadingPalettes();
    }
    if (--cinematic_var13 & 0x8000) {
      reg_TM = 22; reg_TS = 0; reg_CGWSEL = 0; reg_CGADSUB = 0;
      cinematic_function = FUNC16(CinematicFunc_Nothing);
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Fadestuff2):                           // 0x8BB458
    if (!(cinematic_var13 & 3)) {
      FadeInYColorsFromIndexX(0, 0x10);
      FadeInYColorsFromIndexX(0x60, 0x10);
      FadeInYColorsFromIndexX(0x1D2, 6);
      FadeOutYColorsFromIndexX(0x40, 0x10);
      FadeOutYColorsFromIndexX(0x1C0, 9);
      ComposeFadingPalettes();
    }
    if (--cinematic_var13 & 0x8000) {
      reg_TM = 22; reg_TS = 0; reg_CGWSEL = 0; reg_CGADSUB = 0;
      for (int i = 0; i < 7; i++)
        palette_buffer[224 + i] = B8CW(0xE3E9 + 2 * (224 + i));
      nt_pal_dirty(224, 230);
      cinematic_function = FUNC16(CinematicFunc_Nothing);
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func34): CinematicFunction_Intro_Func34(); return 1;
  case FUNC16(CinematicFunction_Intro_Func54): CinematicFunction_Intro_Func54(); return 1;
  case FUNC16(CinematicFunction_Intro_Func55):                               // 0x8BBDE4
    if (!(HasQueuedMusic() & 1)) {
      reg_INIDISP = 15;
      cinematic_function = FUNC16(CinematicFunction_Intro_Func56);
    }
    return 1;
  case FUNC16(CinematicFunction_Intro_Func56): CinematicFunction_Intro_Func56(); return 1;
  case FUNC16(CinematicFunction_Intro_Func67): CinematicFunction_Intro_Func67(); return 1;
  case FUNC16(nullsub_120): return 1;
  case FUNC16(CinematicFunction_Intro_Func72): CinematicFunction_Intro_Func72(); return 1;
  case FUNC16(CinematicFunction_Intro_Func73):                               // 0x8BC100: partida nueva en Ceres
    loading_game_state = kGameState_31_SetUpNewGame;
    game_state = kGameState_31_SetUpNewGame;
    area_index = 6;
    load_station_index = 0;
    /* SaveToSram(selected_save_slot): las partidas guardadas aún no están */
    return 1;
  default: return 0;
  }
}

/* el cuadro de la intro, además de lo común de GameState_37 ($8B:A35B) */
void IntroHandleSamus(void) {  // 0x8B8E0D HandleSamusDuringIntro
  if (cinematic_var15) {
    HandleControllerInputForGamePhysics();
    HandleSamusMovementAndPause();
    if (samus_invincibility_timer)
      --samus_invincibility_timer;
    if (samus_knockback_timer)
      --samus_knockback_timer;
  }
}

void IntroHandleBgAndGlow(void) {
  HandleCinematicBgObjects();
  HandleTextGlowObjects();
}

/* ------------------------------------------------------------------ capa de dibujo */
/* Escena que se ve según los registros del SNES (TM, BG1SC, BGMODE) y brillo del fundido cruzado (0-64). En el SNES
   el fundido cruzado entre el texto y un recuerdo (TM = $06) mezcla las dos escenas con el color math; la Genesis
   oscurece la que se va en la primera mitad y aclara la que llega en la segunda. La usan la Genesis (md_intro.c) y
   el registro de dibujo del PC (qué sprites se ven en cada escena, para mkintro.py). shown: la escena anterior. */
static uint16 IntroSceneOfFlashback(uint16 sc) {
  return sc == 0x54 ? INTRO_SC_BABY : sc == 0x58 ? INTRO_SC_SCIENTIST : sc == 0x5C ? INTRO_SC_LAB : INTRO_SC_MB;
}

uint16 IntroScene(uint16 shown, uint16 *factor) {
  uint16 tm = reg_TM, sc = reg_BG1SC & 0xFC;
  *factor = 64;
  if (tm != 0x06) {
    if (tm == 0x04) return INTRO_SC_CAPT;
    if (tm == 0x16) return INTRO_SC_TEXT;
    if (tm == 0x15) return IntroSceneOfFlashback(sc);
    if (tm == 0x11) return (reg_BGMODE & 7) == 7 ? INTRO_SC_FLY : INTRO_SC_SPACE;
    return shown;
  }
  uint8 to_fb = cinematic_function == FUNC16(CinematicFunction_Intro_XfadeGameplayFade) ||
                cinematic_function == FUNC16(CinematicFunction_Intro_XfadeScientistFade);
  int16 p = 127 - (int16)(to_fb ? cinematic_var4 : cinematic_var13);
  if (p < 0) p = 0;
  if (p > 128) p = 128;
  uint16 fb = IntroSceneOfFlashback(sc);
  if (p < 64) { *factor = 64 - p; return to_fb ? INTRO_SC_TEXT : fb; }
  *factor = p - 64;
  return to_fb ? fb : INTRO_SC_TEXT;
}
