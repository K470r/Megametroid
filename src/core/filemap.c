/* MegaMetroid — motor nativo: el mapa de la partida (estado 5, docs/67). Escrito a mano desde snesrev/sm:
 * FileSelectMap ($81:9E3E, menu_index 0-22) y lo que usa de los bancos $81 y $82.
 *
 * Después de elegir una partida guardada en Zebes: el planeta con el área de la partida encendida (0-6; el cuadrado
 * se abre desde el centro), A o START: un cuadrado crece desde el nombre del área y aparece el mapa del área (7-10)
 * con la estación de guardado titilando; A o START: fundido y el juego (estado 6, 11-14). B en el mapa del área vuelve
 * al planeta (15-21, el cuadrado se cierra); B en el planeta vuelve a las opciones (22: estado 2).
 * Lo que el SNES hace con el PPU (ventanas por HDMA, matemática de color, colas a la VRAM) lo dibuja la Genesis desde
 * estas variables (FileMapGfx y md_filemap.c): la lógica (menu_index, el cuadrado, el scroll del mapa, los colores de
 * palette_buffer) es la del SNES.
 * El código va en la página FM_PAGE del SSF2 (lo común de la ROM está lleno): lo llama nt_call_paged. */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "room.h"
#include "tables.h"
#include "enemies.h"
#pragma GCC optimize("Os")

#ifdef NT_SSF2
#define FM_CODE __attribute__((section(".bank23"), noinline))
#else
#define FM_CODE
#endif

void DrawMenuSpritemap(uint16 idx, uint16 x, uint16 y, uint16 chr);   /* Genesis */
void FileMapGfx(uint16 what);                                         /* Genesis: md_filemap.c */
uint16 NtTransitionColor(uint16 a, uint16 k, uint16 j);                /* door.c */
void LoadFromLoadStation_(void);                                       /* gamestart.c */
/* la pausa (pause.c): el mapa del área */
void PauseFsel_ResetAnimations(void);
void PauseFsel_DetermineMapScrollLimits(void);
void PauseFsel_PaletteAnimation(void);
void PauseFsel_DrawBossMapIcons(uint16 a, uint16 k);
void PauseFsel_DrawSimpleMapIcons(uint16 a, uint16 k, uint16 r3);
uint16 PauseFsel_SamusIndicator(void);
void PauseFsel_ElevatorDestinations(void);
void PauseFsel_MapScrolling(void);
void PauseFsel_SpriteAnim(uint16 a, uint16 k, uint16 j);
extern uint16 pause_map_blank;

enum { FMG_BLACK = 0, FMG_PLANET, FMG_ROOMMAP, FMG_OPTIONS };

static const uint16 kFileSelectMap_AreaIndexes[6] = { 0, 3, 5, 1, 4, 2 };
static const uint16 kActiveAreaMapForegroundColors[6] = { 0, 0xa, 0x10, 0x16, 0x24, 0x2a };
static const uint16 kInactiveAreaMapForegroundColors[6] = { 0x30, 0x3a, 0x40, 0x46, 0x54, 0x5a };
static const uint16 kAreaSelectMapLabelPositions[12] = {  // 0x81A97E
  0x5b, 0x32, 0x2a, 0x7f, 0x5e, 0xb5, 0xce, 0x50, 0xce, 0x9f, 0x87, 0x8b,
};
static const uint16 kRoomSelectMapExpandingSquareTimers[6] = { 0x33, 0x35, 0x2d, 0x33, 0x33, 0x22 };
#define kExpandingSquareVels 0xAA34                                   /* $81: 8 palabras por área */
#define kAreaMapForegroundSetDefs 0xA4E6
#define kAreaMapForegroundColors 0xA40E
#define kMapIconDataPointers 0xC7CB                                   /* $82 */
#define kExpandingSquareTransitionSpeed 4

/* las tablas de las ventanas (7E:9E00 izquierda, 7E:9E10 derecha: entradas de 3 bytes del HDMA indirecto) como bytes */
FM_CODE static uint8 *win(uint16 o) { return (uint8 *)hdma_table_2 + 0x100 + o; }   /* 7E:9E00 + o */
FM_CODE static uint8 rwb(uint16 o) {   /* el byte del SNES (palabras: el par es el bajo) */
  uint16 v = hdma_table_2[128 + (o >> 1)];
  return (o & 1) ? (uint8)(v >> 8) : (uint8)v;
}
FM_CODE static void wwb(uint16 o, uint8 v) {
  uint16 *p = &hdma_table_2[128 + (o >> 1)];
  *p = (o & 1) ? (uint16)((*p & 0xFF) | v << 8) : (uint16)((*p & 0xFF00) | v);
}
FM_CODE static void wwe(uint16 o, uint8 n, uint16 ptr) { wwb(o, n); wwb(o + 1, (uint8)ptr); wwb(o + 2, ptr >> 8); }
#define LEFT(i) ((uint16)(3 * (i)))          /* hdma_window_1_left_pos[i] */
#define RIGHT(i) ((uint16)(0x10 + 3 * (i)))  /* hdma_window_1_right_pos[i] */

/* ------------------------------------------------------------------ colores de las áreas */
FM_CODE static void LoadAreaMapForegroundColors(uint16 j) {  // 0x81A3E3
  while (B81W(kAreaMapForegroundSetDefs + j) != 0xFFFF) {
    uint16 v3 = B81W(kAreaMapForegroundSetDefs + j), v2 = B81W(kAreaMapForegroundSetDefs + j + 2);
    for (int n = 0; n < 5; n++, v2 += 2, v3 += 2)
      palette_buffer[v2 >> 1] = B81W(kAreaMapForegroundColors + v3);
    j += 4;
  }
  nt_pal_dirty(0, 255);
}
FM_CODE static void LoadActiveAreaMapForegroundColors(uint16 v0) { LoadAreaMapForegroundColors(kActiveAreaMapForegroundColors[v0]); }
FM_CODE static void LoadInactiveAreaMapForegroundColors(uint16 v0) { LoadAreaMapForegroundColors(kInactiveAreaMapForegroundColors[v0]); }

FM_CODE static void LoadMenuPalettes(void) {  // 0x818E60 (y LoadFileSelectPalettes, $81:9486)
  for (int i = 0; i < 256; i++) palette_buffer[i] = kMenuPalettes[i];
  nt_pal_dirty(0, 255);
}

FM_CODE static uint8 AdvanceGradualColorChangeOfPalette(uint16 k, uint16 j) {  // 0x82D9B8
  palette_change_denom = 15;
  if ((int16)(15 - palette_change_num) >= 0) {
    do {
      palette_buffer[k >> 1] = NtTransitionColor(palette_change_num + 1, palette_buffer[k >> 1], target_palettes[k >> 1]);
      k += 2;
    } while (k < j);
    ++palette_change_num;
    nt_pal_dirty(0, 31);
    return 0;
  }
  palette_change_num = 0;
  return 1;
}

/* ------------------------------------------------------------------ el planeta */
FM_CODE static uint16 AreaSaveBits(uint16 a) {      /* used_save_stations_and_elevators, palabra del área */
  return used_save_stations_and_elevators[2 * a] | used_save_stations_and_elevators[2 * a + 1] << 8;
}
FM_CODE static uint16 SavePtrs(uint16 a) { return B82W(kMapIconDataPointers + 64 + 2 * a); }   /* kMapIconDataPointers[4] */

FM_CODE static void DrawAreaSelectMapLabels(void) {  // 0x81A9AA
  uint16 sm0 = B82W(0xC749);                                          /* g_word_82C749[0] */
  DrawMenuSpritemap(sm0, 0x80, 0x10, 0);
  for (int i = 0; i < 6; i++) {
    uint16 r3 = (i == file_select_map_area_index) ? 0 : 512;
    uint16 a = kFileSelectMap_AreaIndexes[i];
    uint16 r36 = AreaSaveBits(a);
    uint16 v2 = SavePtrs(a);
    int R30 = 16;
    while (B82W(v2) != 0xffff) {
      int v4 = r36 & 1;
      r36 >>= 1;
      if (v4 && B82W(v2) != 0xfffe)
        goto draw;
      v2 += 4;
      if (!--R30) {
        if (enable_debug && B82W(v2) != 0xFFFF)
          goto draw;
        break;
      }
    }
    continue;
draw:
    DrawMenuSpritemap(sm0 + a + 1, kAreaSelectMapLabelPositions[2 * a], kAreaSelectMapLabelPositions[2 * a + 1], r3);
  }
}

FM_CODE static uint16 CheckIfFileSelectMapAreaCanBeSelected(uint16 a) {  // 0x81A931
  uint16 ar = kFileSelectMap_AreaIndexes[a];
  if (AreaSaveBits(ar))
    return 1;
  return (uint16)(B82W(kMapIconDataPointers + 64 + 2 * ar) + 64) != 0xffff;   /* (lo que lee el SNES) */
}

FM_CODE static void SwitchActiveFileSelectMapArea(uint16 R28) {  // 0x81A958
  LoadInactiveAreaMapForegroundColors(kFileSelectMap_AreaIndexes[file_select_map_area_index]);
  file_select_map_area_index = R28;
  LoadActiveAreaMapForegroundColors(kFileSelectMap_AreaIndexes[R28]);
  FileMapGfx(FMG_PLANET);                    /* LoadAreaSelectBackgroundTilemap: el planeta de esa área */
}

FM_CODE static uint16 DecAndWraparoundTo5(uint16 a) { uint16 r = a - 1; return (r & 0x8000) ? 5 : r; }   // 0x81A898
FM_CODE static uint16 WraparoundFrom6to0(uint16 a) { uint16 r = a + 1; return !sign16(r - 6) ? 0 : r; }   // 0x81A89F

FM_CODE static void SelectFileSelectMapArea(void) {  // 0x81A8A9
  ++menu_index;
  if (!enable_debug) {
    area_index = sram_area_index;
    load_station_index = sram_save_station_index;
    DrawAreaSelectMapLabels();
    return;
  }
  area_index = kFileSelectMap_AreaIndexes[file_select_map_area_index];
  uint16 q = AreaSaveBits(area_index), v0 = 0, r0 = SavePtrs(area_index);
  for (int r20 = 16; r20; r20--, v0++) {
    uint8 c = q & 1;
    q >>= 1;
    uint16 v3 = B82W(r0 + 4 * v0);
    if (c && v3 != 0xFFFE) {
      if (v3 != 0xFFFF) break;
      v0 = 0xFFFF;
    }
  }
  load_station_index = v0;
  DrawAreaSelectMapLabels();
}

FM_CODE static void FileSelectMap_6_AreaSelectMap(void) {  // 0x81A800
  int16 v0 = joypad1_newkeys;
  if (joypad1_newkeys & (kButton_Up | kButton_Left)) {
    v0 = enable_debug;
    if (enable_debug) {
      uint16 R28 = DecAndWraparoundTo5(file_select_map_area_index);
      if (CheckIfFileSelectMapAreaCanBeSelected(R28)
          || (R28 = DecAndWraparoundTo5(R28), CheckIfFileSelectMapAreaCanBeSelected(R28))
          || (R28 = DecAndWraparoundTo5(R28), CheckIfFileSelectMapAreaCanBeSelected(R28))) {
        QueueSfx1_Max6(0x37);
        SwitchActiveFileSelectMapArea(R28);
      }
      DrawAreaSelectMapLabels();
      return;
    }
  } else if ((joypad1_newkeys & (kButton_Select | kButton_Down | kButton_Right)) && enable_debug) {
    uint16 R28 = file_select_map_area_index;
    for (int n = 6; n; n--) {
      R28 = WraparoundFrom6to0(R28);
      if (CheckIfFileSelectMapAreaCanBeSelected(R28)) {
        SwitchActiveFileSelectMapArea(R28);
        QueueSfx1_Max6(0x37);
        break;
      }
    }
    DrawAreaSelectMapLabels();
    return;
  }
  if (v0 < 0) {                              /* B: a las opciones */
    menu_index = 22;
    DrawAreaSelectMapLabels();
    return;
  }
  if (v0 & (kButton_Start | kButton_A)) {
    QueueSfx1_Max6(0x38);
    SelectFileSelectMapArea();
    return;
  }
  DrawAreaSelectMapLabels();
}

/* ------------------------------------------------------------------ las ventanas (el cuadrado) */
FM_CODE static void SetupInitialExpandingSquareHDMA(void) {  // 0x81A61C
  expand_sq_topbottom_margin_right_pos = 0;
  expand_sq_topbottom_margin_left_pos = 0xFFFF;
  expand_sq_left_pos = (expand_sq_left_pos & 0xFF00) | 127;
  expand_sq_right_pos = (expand_sq_right_pos & 0xFF00) | 0x81;
  wwe(LEFT(0), 111, 0x9E22); wwe(LEFT(1), 1, 0x9E32); wwe(LEFT(2), 1, 0x9E32); wwe(LEFT(3), 111, 0x9E20);
  wwe(RIGHT(0), 111, 0x9E20); wwe(RIGHT(1), 1, 0x9E36); wwe(RIGHT(2), 1, 0x9E36); wwe(RIGHT(3), 111, 0x9E20);
}

FM_CODE static void AddExpandingSqTransLeftIndirHDMA(uint16 a, uint16 k, uint16 j) {  // 0x81ABF7
  if (a & 0x80) { wwe(LEFT(0) + k, a - 127, j); wwe(LEFT(1) + k, 127, j); }
  else wwe(LEFT(0) + k, a, j);
}
FM_CODE static uint16 AddExpandingSqTransRightIndirHDMA(uint16 a, uint16 k, uint16 j) {  // 0x81AC2D
  if (a & 0x80) { wwe(RIGHT(0) + k, a - 127, j); wwe(RIGHT(1) + k, 127, j); return k + 6; }
  wwe(RIGHT(0) + k, a, j);
  return k + 3;
}
FM_CODE static void SetupRoomSelectMapExpandingSquareTransHDMA(void) {  // 0x81ABA7
  uint16 k = 0;
  uint16 v0 = expand_sq_top_pos;
  AddExpandingSqTransLeftIndirHDMA(v0, k, 0x9E22);
  k = AddExpandingSqTransRightIndirHDMA(v0, k, 0x9E20);
  uint16 v2 = expand_sq_bottom_pos - expand_sq_top_pos;
  if ((uint8)expand_sq_bottom_pos == (uint8)expand_sq_top_pos) v2 = 1;
  AddExpandingSqTransLeftIndirHDMA(v2, k, 0x9E32);
  k = AddExpandingSqTransRightIndirHDMA(v2, k, 0x9E36);
  uint16 v4 = -32 - expand_sq_bottom_pos;
  if ((uint8)expand_sq_bottom_pos == 0xE0) v4 = 1;
  AddExpandingSqTransLeftIndirHDMA(v4, k, 0x9E22);
  k = AddExpandingSqTransRightIndirHDMA(v4, k, 0x9E20);
  wwb(LEFT(0) + k, 0);
  wwb(RIGHT(0) + k, 0);
}

FM_CODE static void LoadSquareVels(uint16 a) {
  uint16 v = kExpandingSquareVels + 16 * a;
  expand_sq_left_subvel = B81W(v); expand_sq_left_vel = B81W(v + 2);
  expand_sq_right_subvel = B81W(v + 4); expand_sq_right_vel = B81W(v + 6);
  expand_sq_top_subvel = B81W(v + 8); expand_sq_top_vel = B81W(v + 10);
  expand_sq_bottom_subvel = B81W(v + 12); expand_sq_bottom_vel = B81W(v + 14);
}
FM_CODE static void AddHiLo(uint16 *hi, uint16 *lo, uint32 d) {   /* AddToHiLo */
  uint32 v = ((uint32)*hi << 16 | *lo) + d;
  *hi = v >> 16; *lo = (uint16)v;
}
FM_CODE static uint16 HandleRoomSelectMapExpandingSquareTrans(void) {  // 0x81AC84
  AddHiLo(&expand_sq_left_pos, &expand_sq_left_subpos, (uint32)expand_sq_left_vel << 16 | expand_sq_left_subvel);
  if (sign16(expand_sq_left_pos - 1)) expand_sq_left_pos = 1;
  AddHiLo(&expand_sq_right_pos, &expand_sq_right_subpos, (uint32)expand_sq_right_vel << 16 | expand_sq_right_subvel);
  if (!sign16(expand_sq_right_pos - 256)) expand_sq_right_pos = 255;
  AddHiLo(&expand_sq_top_pos, &expand_sq_top_subpos, (uint32)expand_sq_top_vel << 16 | expand_sq_top_subvel);
  if (sign16(expand_sq_top_pos - 1)) expand_sq_top_pos = 1;
  AddHiLo(&expand_sq_bottom_pos, &expand_sq_bottom_subpos, (uint32)expand_sq_bottom_vel << 16 | expand_sq_bottom_subvel);
  if (!sign16(expand_sq_bottom_pos - 224)) expand_sq_bottom_pos = 224;
  SetupRoomSelectMapExpandingSquareTransHDMA();
  return --expand_sq_timer;
}

/* ------------------------------------------------------------------ el mapa del área */
FM_CODE static void DrawMapScrollArrowAndCheckToScroll81(uint16 k) {  // 0x82B90A con el banco $81
  PauseFsel_SpriteAnim(B81W(k + 4), B81W(k), B81W(k + 2));
  if ((joypad1_lastkeys & B81W(k + 6)) && !map_scrolling_direction)
    map_scrolling_direction = B81W(k + 8);
}
FM_CODE static void HandleFileSelectMapScrollArrows(void) {  // 0x81AECA
  if (sign16(map_min_x_scroll - 24 - reg_BG1HOFS)) DrawMapScrollArrowAndCheckToScroll81(0xAF32);
  if (!sign16(map_max_x_scroll - 232 - reg_BG1HOFS)) DrawMapScrollArrowAndCheckToScroll81(0xAF3C);
  if (sign16(map_min_y_scroll - 64 - reg_BG1VOFS)) DrawMapScrollArrowAndCheckToScroll81(0xAF46);
  if (sign16(map_max_y_scroll - 145 - reg_BG1VOFS)) {
    if (map_scrolling_direction == B81W(0xAF50 + 8)) {
      map_scrolling_gear_switch_timer = 0;
      map_scrolling_direction = 0;
      map_scrolling_speed_index = 0;
    }
  } else {
    DrawMapScrollArrowAndCheckToScroll81(0xAF50);
  }
}

FM_CODE static void DrawFileSelectMapIcons(void) {  // 0x82B6DD
  PauseFsel_PaletteAnimation();
  PauseFsel_DrawBossMapIcons(9, 0xC7CB);
  PauseFsel_DrawSimpleMapIcons(0xB, 0xC7DB, 3584);
  PauseFsel_DrawSimpleMapIcons(0xA, 0xC7EB, 3584);
  PauseFsel_DrawSimpleMapIcons(0x4E, 0xC7FB, 3584);
  uint16 a = PauseFsel_SamusIndicator();
  uint16 d = SavePtrs(area_index) + 4 * load_station_index;
  uint16 v1 = B82W(d) - reg_BG1HOFS, v2 = B82W(d + 2) - reg_BG1VOFS;
  if ((samus_position_indicator_animation_loop_counter & 1) == 0)
    DrawMenuSpritemap(0x12, v1, v2, 3584);
  DrawMenuSpritemap(a, v1, v2, 3584);
  if (enable_debug)
    PauseFsel_DrawSimpleMapIcons(0xC, 0xC82B, 1536);    /* (las de depuración de estaciones y ascensores: no están) */
  if (!area_index)
    DrawMenuSpritemap(0x63, 0xd8 - reg_BG1HOFS, 0x28 - reg_BG1VOFS, 3584);
}

FM_CODE static void SetupMapScrollingForFileSelectMap(void) {  // 0x829028
  PauseFsel_ResetAnimations();
  PauseFsel_DetermineMapScrollLimits();
  reg_BG1HOFS = map_min_x_scroll + ((uint16)(map_max_x_scroll - map_min_x_scroll) >> 1) - 128;
  uint16 r18 = 8 * (room_x_coordinate_on_map + ((samus_x_pos & 0xFF00) >> 8)) - reg_BG1HOFS;
  int16 v0 = 224 - r18;
  if ((int16)(224 - r18) >= 0) {
    r18 = 32 - r18;
    if ((r18 & 0x8000) == 0) reg_BG1HOFS -= r18;
  } else {
    reg_BG1HOFS -= v0;
  }
  r18 = map_min_y_scroll + ((uint16)(map_max_y_scroll - map_min_y_scroll) >> 1) + 16;
  reg_BG1VOFS = -((112 - r18) & 0xFFF8);
  r18 = 8 * (room_y_coordinate_on_map + HI8(samus_y_pos) + 1) + ((112 - r18) & 0xFFF8);
  int16 v1 = 64 - r18;
  if ((int16)(64 - r18) >= 0) {
    reg_BG1VOFS -= v1;
    if (sign16(reg_BG1VOFS + 40)) reg_BG1VOFS = -40;
  }
}

FM_CODE static void FileSelectMap_10_RoomSelectMap(void) {  // 0x81AD7F (sin el SELECT del segundo mando: depuración)
  DrawFileSelectMapIcons();
  HandleFileSelectMapScrollArrows();
  PauseFsel_MapScrolling();
  PauseFsel_ElevatorDestinations();
  if (joypad1_newkeys & 0x8000) {            /* B: al planeta */
    menu_index += 5;
    uint16 v0 = 0;
    while (area_index != kFileSelectMap_AreaIndexes[v0]) {
      if (++v0 >= 6) { file_select_map_area_index = 0; return; }
    }
    file_select_map_area_index = v0;
    QueueSfx1_Max6(0x3C);
  } else if (joypad1_newkeys & (kButton_Start | kButton_A)) {
    ++menu_index;
    QueueSfx1_Max6(0x38);
  }
}

/* ------------------------------------------------------------------ los pasos */
FM_CODE static void FileSelectMap_4_SetupExpandingSquareTransition(void) {  // 0x81A5B3
  reg_TS = 4; reg_TSW = 4; reg_CGWSEL = 2; reg_CGADSUB = 37;
  reg_COLDATA[2] = 0x80; reg_COLDATA[1] = 64; reg_COLDATA[0] = 32;
  SetupInitialExpandingSquareHDMA();
  reg_HDMAEN = 12;
  reg_TM = 19; reg_TMW = 19; reg_W12SEL = 35; reg_W34SEL = 3; reg_WOBJSEL = 35;   /* ConfigureWindow1ForExpandingSquare */
  hdma_objects_enable_flag = 0;
  menu_index = (menu_index & 0xFF00) | (uint8)(menu_index + 1);
}

FM_CODE static void FileSelectMap_5_ExpandingSquare(void) {  // 0x81A725
  DrawAreaSelectMapLabels();
  uint8 v0 = rwb(LEFT(0)) - kExpandingSquareTransitionSpeed;
  if ((int8)v0 < 0) {
    menu_index = (menu_index & 0xFF00) | (uint8)(menu_index + 1);
    reg_TM &= ~2;
    reg_TMW = 0;
    reg_TSW = 0;
    reg_BG2VOFS = 24;
    pausemenu_palette_animation_timer = 1;
    /* ram4000: el marco del mapa del área ($B6:E000) con el nombre del área y las filas de abajo
       (kFileSelectExpandingSquareTilemap): la Genesis (FileMapGfx) */
  } else {
    wwb(LEFT(0), v0); wwb(LEFT(3), v0); wwb(RIGHT(0), v0); wwb(RIGHT(3), v0);
    uint8 c = rwb(LEFT(1)) + kExpandingSquareTransitionSpeed;
    wwb(LEFT(1), c); wwb(LEFT(2), c); wwb(RIGHT(1), c); wwb(RIGHT(2), c);
    expand_sq_left_pos = (expand_sq_left_pos & 0xFF00) | (uint8)(expand_sq_left_pos - kExpandingSquareTransitionSpeed);
    expand_sq_right_pos = (expand_sq_right_pos & 0xFF00) | (uint8)(expand_sq_right_pos + kExpandingSquareTransitionSpeed);
  }
}

FM_CODE static void FileSelectMap_7_PrepExpandSquareTransToRoomMap(void) {  // 0x81AAAC
  DrawAreaSelectMapLabels();
  SetupInitialExpandingSquareHDMA();
  reg_TM = 19; reg_TMW = 19; reg_W12SEL = 50; reg_W34SEL = 2; reg_CGADSUB = 5; reg_WOBJSEL = 34;
  reg_BG12NBA = 48;
  wwb(LEFT(3), 0);
  wwb(RIGHT(3), 0);
  expand_sq_timer = kRoomSelectMapExpandingSquareTimers[area_index];
  expand_sq_left_pos = expand_sq_right_pos = kAreaSelectMapLabelPositions[2 * area_index];
  expand_sq_top_pos = expand_sq_bottom_pos = kAreaSelectMapLabelPositions[2 * area_index + 1];
  expand_sq_left_subpos = expand_sq_right_subpos = expand_sq_top_subpos = expand_sq_bottom_subpos = 0;
  LoadSquareVels(area_index);
  SetupRoomSelectMapExpandingSquareTransHDMA();
  reg_HDMAEN = 12;
  ++menu_index;
  QueueSfx1_Max6(0x3B);
}

FM_CODE static void FileSelectMap_9_InitRoomSelectMap(void) {  // 0x81AD17
  LoadMirrorOfExploredMapTiles();
  reg_BG12NBA = 51; reg_TM = 19; reg_BG1VOFS = -40;                  /* DrawRoomSelectMap ($82:9517): el mapa, la Genesis */
  pause_map_blank = 0xF;                                             /* (sin la estación del mapa: 0xF, no 0x1F) */
  LoadFromLoadStation_();
  DisableHdmaObjects();
  const RoomDefN *rd = room_def(room_ptr);
  area_index = (area_index & 0xFF00) | rd->area_index;
  room_x_coordinate_on_map = (room_x_coordinate_on_map & 0xFF00) | rd->x_coordinate_on_map;
  room_y_coordinate_on_map = (room_y_coordinate_on_map & 0xFF00) | rd->y_coordinate_on_map;
  SetupMapScrollingForFileSelectMap();
  map_min_y_scroll += 24;
  reg_BG2VOFS = 24;
  reg_TM &= ~4;
  ++menu_index;
  map_scrolling_direction = 0;
  map_scrolling_speed_index = 0;
  samus_position_indicator_animation_frame = 0;
  samus_position_indicator_animation_timer = 0;
  samus_position_indicator_animation_loop_counter = 0;
  FileMapGfx(FMG_ROOMMAP);
}

FM_CODE static void FileSelectMap_20_SetupExpandingSquare(void) {  // 0x81AFF6
  reg_HDMAEN = 0;
  QueueSfx1_Max6(0x3C);
  expand_sq_timer = kRoomSelectMapExpandingSquareTimers[area_index] - 12;
  LoadSquareVels(area_index);
  expand_sq_left_pos = 8; expand_sq_right_pos = 248; expand_sq_top_pos = 8; expand_sq_bottom_pos = 216;
  expand_sq_left_subpos = expand_sq_right_subpos = expand_sq_top_subpos = expand_sq_bottom_subpos = 0;
  SetupRoomSelectMapExpandingSquareTransHDMA();
  ++menu_index;
  reg_HDMAEN = 12; reg_W12SEL = 50; reg_W34SEL = 2; reg_BG12NBA = 48; reg_WOBJSEL = 34;
  wwb(LEFT(3), 0);
  wwb(RIGHT(3), 0);
  reg_BG1VOFS = 0;
  reg_BG1HOFS = 0;
}

FM_CODE static void FileSelectMap_21_MoveExpandingSquare(void) {  // 0x81B0BB
  AddHiLo(&expand_sq_left_pos, &expand_sq_left_subpos, -((uint32)expand_sq_left_vel << 16 | expand_sq_left_subvel));
  AddHiLo(&expand_sq_right_pos, &expand_sq_right_subpos, -((uint32)expand_sq_right_vel << 16 | expand_sq_right_subvel));
  AddHiLo(&expand_sq_top_pos, &expand_sq_top_subpos, -((uint32)expand_sq_top_vel << 16 | expand_sq_top_subvel));
  AddHiLo(&expand_sq_bottom_pos, &expand_sq_bottom_subpos, -((uint32)expand_sq_bottom_vel << 16 | expand_sq_bottom_subvel));
  SetupRoomSelectMapExpandingSquareTransHDMA();
  DrawAreaSelectMapLabels();
  if (--expand_sq_timer & 0x8000) {
    menu_index -= 15;
    reg_TM = 17;
    reg_TMW = 0;
    reg_TSW = 0;
  }
}

FM_CODE static void FileSelectMap_Body(void) {  // 0x819E3E (kFileSelectMapFuncs)
  switch (menu_index & 0xFF) {
  case 0:                                     // 0x81A32A
    /* ClearMenuTilemap y el BG2 vacío: la Genesis (FMG_BLACK) */
    palette_change_num = 0;
    LoadMenuPalettes();
    for (int i = 0; i < 32; i++) target_palettes[i] = palette_buffer[i];
    target_palettes[14] = 0;
    target_palettes[30] = 0;
    FileMapGfx(FMG_BLACK);
    ++menu_index;
    break;
  case 1:                                     // 0x81A37C
    if (AdvanceGradualColorChangeOfPalette(0, 0x40)) {
      reg_BG1VOFS = reg_BG1HOFS = reg_BG2VOFS = reg_BG2HOFS = 0;
      uint16 v0 = 0;
      while (area_index != kFileSelectMap_AreaIndexes[v0])
        if (++v0 >= 6) { v0 = 0; break; }
      file_select_map_area_index = v0;
      for (uint16 a = 0; a < 6; a++) {
        if (a == area_index) LoadActiveAreaMapForegroundColors(a);
        else LoadInactiveAreaMapForegroundColors(a);
      }
      ++menu_index;
      g_word_7E0787 = 0;
      reg_TM = 2;
    }
    break;
  case 2: case 17:                            // 0x81A546: el BG1 del planeta
    reg_INIDISP = 0xF;
    ++menu_index;
    break;
  case 3: case 18:                            // 0x81A582 / 0x81A578: el BG3 del área
    ++menu_index;
    FileMapGfx(FMG_PLANET);
    break;
  case 4: case 19: FileSelectMap_4_SetupExpandingSquareTransition(); break;
  case 5: FileSelectMap_5_ExpandingSquare(); break;
  case 6: FileSelectMap_6_AreaSelectMap(); break;
  case 7: FileSelectMap_7_PrepExpandSquareTransToRoomMap(); break;
  case 8:                                     // 0x81AC66
    if (HandleRoomSelectMapExpandingSquareTrans() & 0x8000) {
      ++menu_index;
      reg_TM = 2; reg_TMW = 0; reg_TSW = 0; reg_TS = 0;
    }
    DrawAreaSelectMapLabels();
    break;
  case 9: FileSelectMap_9_InitRoomSelectMap(); break;
  case 10: FileSelectMap_10_RoomSelectMap(); break;
  case 11: case 12:                           // 0x81AF5A
    DrawFileSelectMapIcons();
    PauseFsel_ElevatorDestinations();
    ++menu_index;
    break;
  case 13:                                    // 0x81AF66
    DrawFileSelectMapIcons();
    PauseFsel_ElevatorDestinations();
    HandleFadeOut();
    if (!(reg_INIDISP & 0xF)) {
      ++menu_index;
      enemy_data[0].x_pos = 32;
    }
    break;
  case 14:                                    // 0x81AF83
    if (!--enemy_data[0].x_pos) {
      reg_INIDISP = 0x80;                     /* ScreenOff */
      pause_map_blank = 0x1F;
      ++game_state;
      menu_index = 0;
    }
    break;
  case 15:                                    // 0x81AF97: el BG1 vacío
    reg_TM = 18;
    pause_map_blank = 0x1F;
    FileMapGfx(FMG_BLACK);
    ++menu_index;
    break;
  case 16:                                    // 0x81AFD3
    LoadMenuPalettes();
    for (uint16 a = 0; a < 6; a++) LoadInactiveAreaMapForegroundColors(a);
    LoadActiveAreaMapForegroundColors(area_index);
    ++menu_index;
    break;
  case 20: FileSelectMap_20_SetupExpandingSquare(); break;
  case 21: FileSelectMap_21_MoveExpandingSquare(); break;
  case 22:                                    // 0x819E7B
    DrawAreaSelectMapLabels();
    HandleFadeOut();
    if (!(reg_INIDISP & 0xF)) {
      game_state = 2;                         /* kGameState_2_GameOptionsMenu: la Genesis (md_main.c) */
      menu_index = 0;
    }
    break;
  }
}

/* el estado 5 (lo llama game.c con la página FM_PAGE puesta) */
FM_CODE void FileSelectMapFrame(void) {
  FileSelectMap_Body();
}
