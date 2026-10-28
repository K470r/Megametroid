/* MegaMetroid — motor nativo: la pausa, el mapa y la pantalla de equipo (banco $82; docs/49).
 * Escrito a mano desde snesrev/sm (sm_82.c), con los nombres y las direcciones del SNES.
 *
 * Estados 13-17 y lo que sigue al juego en el 12 y el 18 (game.c llama antes a GameState_8_MainGameplay). La estación de mapa
 * (PlmInstr_ActivateMapStation, $84:8C8F) también termina aquí: DisplayMessageBox_Async pone game_state = 12 al
 * cerrar la caja "MAP DATA ACCESS COMPLETED" y el SNES abre la pausa en el mapa.
 *
 * La RAM es la del SNES: el mapa de la pausa y el equipo viven en 7E:3000-3FFF (ram3000.pause_menu_map_tilemap y
 * ram3800: cinematic_text_tilemap y su alias cinematic_bg_tilemap; vars_extra.txt), la copia de la paleta del juego
 * en 7E:3300. Lo que el SNES copia a la VRAM (tiles de la pausa, BG2 desde $B6:E000, el mapa del área armado en
 * 7E:4000, el BG1 del equipo desde 7E:3800) lo dibuja la Genesis (md_pause.c) a partir de esa RAM y de PauseMapCell,
 * que arma una celda del mapa como LoadPauseMenuMapTilemap. 7E:4000 no se guarda: el SNES lo pisa enseguida
 * (QueueClearingOfFxTilemap) y el motor no lo lee.
 * Lo que no hace falta en Crateria llama a nt_unimpl (ganchos de pausa de Kraid y Draygon, hiperrayo). */
#include "samus.h"
#include "game.h"
#include "nmath.h"
/* la pausa no tiene apuro (el SNES tampoco hace mucho por cuadro) y la parte fija de la ROM está casi llena (docs/48):
   chica antes que rápida */
#pragma GCC optimize("Os")
#include "hdmaobj.h"
#include "animtiles.h"

void DrawMenuSpritemap(uint16 idx, uint16 x, uint16 y, uint16 chr);   /* $81:891F (Genesis) */
void PauseGfx(uint16 what);           /* Genesis: lo que el SNES pasa a la VRAM (PAUSE_GFX_*) */
void ResumeGameplay(void);            /* gamestart.c */
void HandleHudTilemap(void);          /* hud.c */

static inline uint8 B82B(uint16 a) { return kBank82B[(uint16)(a - 0x8000)]; }
/* 7E:3000-3FFF (dirección del SNES) en el arreglo de palabras */
static inline uint16 *R3(uint16 a) { return &cinematic_text_tilemap[(uint16)(a - 0x3000) >> 1]; }
#define PMT cinematic_text_tilemap                      /* ram3000.pause_menu_map_tilemap */
#define BG3800 cinematic_bg_tilemap                     /* ram3800.cinematic_bg_tilemap / equipment_screen_bg1_tilemap */

/* tablas del banco $82 (direcciones) */
#define kPauseScreenSpriteAnimationData_3 0xC0E8
#define kPAuseSpritePaletteIndexValues 0xC0FA
#define kPausePtsToAnimationSpritemapBaseIds 0xC1E4
#define kPauseLrHighlightAnimData 0xC10C
#define kPauseReserveTankAnimationData 0xC165
#define kPauseAnimatedPalette 0xA987
#define kPauseAreaLabelTilemap 0x965F
#define kPauseMenuMapData 0x9717
#define kEquipmentPtrsToRamTilemapOffsets 0xC02C
#define kEquipmentPtrsToBitmasks 0xC034
#define kEquipmentPtrsToBitsets 0xC03C
#define kEquipmentPtrsToEquipmentTilemaps 0xC044
#define kEquipmentBitmasks_Weapons 0xC04C
#define kEquipmentBitmasks_Suits 0xC056
#define kEquipmentBitmasks_Boots 0xC062
#define kEquipmentTilemapOffs_Tanks 0xC068
#define kEquipmentTilemapOffs_Weapons 0xC06C
#define kEquipmentTilemapOffs_Suits 0xC076
#define kEquipmentTilemapOffs_Boots 0xC082
#define kEquipmentTilemaps_Tanks 0xC088
#define kEquipmentTilemaps_Weapons 0xC08C
#define kEquipmentTilemaps_Suits 0xC096
#define kEquipmentTilemaps_Boots 0xC0A2
#define kHyperBeamWeaponsTilemaps 0xC0A8
#define kEquipmentScreenWireframeCmp 0xB257
#define kEquipmentScreenWireframePtrs 0xB25F
#define kEquipmentScreenPtrsToItemXYpos 0xC18E
#define addr_kEquipmentScreenTilemap_Blank 0xC01A
#define addr_kEquipmentScreenTilemap_AUTO 0xBF2A
#define addr_kEquipmentScreenTilemap_MANUAL 0xBF22
#define addr_kDummySamusWireframeTilemap 0xC639
#define addr_kMapIconDataPointers 0xC7CB
#define kMapElevatorDests 0xC74D

static void EnableNMI(void) { reg_NMITIMEN |= 0x80; }   // 0x80834B
/* (copias de funciones de otros bancos que en el resto del motor quedan inlinadas: llamarlas desde aquí las sacaría
   como funciones aparte y correría las direcciones de la ROM, docs/49) */
static void DisableAnimtiles_(void) { animtiles_enable_flag &= ~0x8000; }   // 0x87800B
static void EnableAnimtiles_(void) { animtiles_enable_flag |= 0x8000; }     // 0x878000
static void CancelSoundEffects(void) {  // 0x82BE17
  QueueSfx1_Max6(2);
  QueueSfx2_Max6(0x71);
  QueueSfx3_Max6(1);
}
static void QueueSamusMovementSfx(void) {  // 0x82BE2F
  if ((speed_boost_counter & 0xFF00) == 1024)
    QueueSfx3_Max6(0x2B);
  if (!sign16(flare_counter - 16))
    QueueSfx1_Max6(0x41);
  CallSomeSamusCode(0x14);
}

/* memcpy(g_ram + dst, RomPtr_82(src), n) */
static void CopyFrom82(uint16 dst, uint16 src, uint16 n) {
  uint16 *d = R3(dst);
  for (uint16 i = 0; i < n; i += 2) *d++ = B82W(src + i);
}

/* ------------------------------------------------------------------ mando ($80:8146) */
static void ReleaseButtonsFilter(uint16 v0) {  // 0x808146
  timed_held_input_timer_reset = v0;
  bool v1 = ((uint16)~joypad1_newkeys & joypad1_lastkeys) == joypad_released_keys;
  joypad_released_keys = ~joypad1_newkeys & joypad1_lastkeys;
  if (!v1) {
    timed_held_input_timer = timed_held_input_timer_reset;
    timed_held_input = 0;
    goto LABEL_6;
  }
  if ((--timed_held_input_timer & 0x8000) == 0) {
    timed_held_input = 0;
    goto LABEL_6;
  }
  timed_held_input_timer = 0;
  previous_timed_held_input = timed_held_input;
  timed_held_input = ~joypad1_newkeys & joypad1_lastkeys;
LABEL_6:
  newly_held_down_timed_held_input = timed_held_input & (previous_timed_held_input ^ timed_held_input);
}

/* ------------------------------------------------------------------ las animaciones de sprites de la pausa */
/* kPauseScreenSpriteAnimationData_0/1/2 ($82:C0B2/C0C4/C0D6): direcciones de la RAM (temporizador, cuadro y lo que
   elige el spritemap) de cada animación */
static uint16 *const kAnimTimer[9] = {
  &pausemenu_start_lr_pressed_highlight_timer, &pausemenu_lr_animation_timer, &pausemenu_item_selector_animation_timer,
  &pausemenu_reserve_tank_animation_timer, &UNUSED_word_7E0731, &pausemenu_uparrow_animation_timer,
  &pausemenu_downarrow_animation_timer, &pausemenu_rightarrow_animation_timer, &pausemenu_leftarrow_animation_timer,
};
static uint16 *const kAnimFrame[9] = {
  &UNUSED_word_7E073D, &pausemenu_lr_animation_frame, &pausemenu_item_selector_animation_frame,
  &pausemenu_reserve_tank_animation_frame, &UNUSED_word_7E0745, &pausemenu_uparrow_animation_frame,
  &pausemenu_downarrow_animation_frame, &pausemenu_rightarrow_animation_frame, &pausemenu_leftarrow_animation_frame,
};
static uint16 *const kAnimSel[9] = {
  &pausemenu_shoulder_button_highlight, &pausemenu_button_label_mode, &pausemenu_equipment_category_item,
  &pausemenu_reserve_tank_delay_ctr, &UNUSED_word_7E0759, &UNUSED_word_7E075B, &UNUSED_word_7E075D,
  &UNUSED_word_7E075F, &UNUSED_word_7E0761,
};

static void DrawPauseScreenSpriteAnim(uint16 a, uint16 input_k, uint16 input_j) {  // 0x82A881
  int t = a - 1;
  uint16 *v8 = kAnimFrame[t];
  uint16 *v5 = kAnimTimer[t];
  uint16 v3 = B82W(kPauseScreenSpriteAnimationData_3 + 2 * t);
  if ((int16)--(*v5) <= 0) {
    uint16 v10 = B82B(v3 + 3 * ++(*v8));
    if (v10 == 255) {
      *v8 = 0;
      v10 = B82B(v3);
    }
    *v5 = v10;
  }
  uint16 r3 = B82W(kPAuseSpritePaletteIndexValues + 6);
  uint16 r24 = B82B(v3 + 3 * *v8 + 2);
  uint16 r26 = 2 * (uint8)*kAnimSel[t];               /* *RomPtr_RAM(...): el byte bajo */
  uint16 v11 = r26 + B82W(kPausePtsToAnimationSpritemapBaseIds + 2 * t);
  DrawMenuSpritemap(r24 + B82W(v11), input_k, input_j - 1, r3);
}

static void HandlePauseScreenPaletteAnimation(void) {  // 0x82A92B
  uint8 v1;
  if ((uint8)pausemenu_palette_animation_timer) {
    pausemenu_palette_animation_timer = (pausemenu_palette_animation_timer & 0xFF00) | (uint8)(pausemenu_palette_animation_timer - 1);
    if (!(uint8)pausemenu_palette_animation_timer) {
      for (int i = (uint8)(pausemenu_palette_animation_frame + 1); ; i = 0) {
        pausemenu_palette_animation_frame = (pausemenu_palette_animation_frame & 0xFF00) | (uint8)i;
        v1 = B82B(kPauseLrHighlightAnimData + 3 * i);
        if (v1 != 0xFF)
          break;
        QueueSfx3_Max6(0x2A);
      }
      pausemenu_palette_animation_timer = (pausemenu_palette_animation_timer & 0xFF00) | v1;
      uint16 v3 = (uint16)((uint8)pausemenu_palette_animation_frame << 8) >> 3;
      for (int j = 0; j < 16; j++)
        palette_buffer[176 + j] = B82W(kPauseAnimatedPalette + v3 + 2 * j);
      nt_pal_dirty(176, 191);
    }
  }
}

/* ------------------------------------------------------------------ entrar a la pausa (estado 13) */
static void BackupSomeGfxStateForPause(void) {  // 0x828DBD
  pausemenu_bak_BG1SC = reg_BG1SC;
  pausemenu_bak_BG2SC = reg_BG2SC;
  pausemenu_bak_BG3SC = reg_BG3SC;
  pausemenu_bak_BG12NBA = reg_BG12NBA;
  pausemenu_bak_BG34NBA = reg_BG34NBA;
  pausemenu_bak_OBSEL = reg_OBSEL;
  pausemenu_bak_BG1HOFS = reg_BG1HOFS;
  pausemenu_bak_BG2HOFS = reg_BG2HOFS;
  pausemenu_bak_BG3HOFS = reg_BG3HOFS;
  pausemenu_bak_BG1VOFS = reg_BG1VOFS;
  pausemenu_bak_BG2VOFS = reg_BG2VOFS;
  pausemenu_bak_BG3VOFS = reg_BG3VOFS;
  pausemenu_bak_BGMODE = reg_BGMODE;
  pausemenu_bak_layer2_scroll_x = layer2_scroll_x;
  pausemenu_bak_layer2_scroll_y = layer2_scroll_y;
  pausemenu_bak_MOSAIC = reg_MOSAIC;
  pausemenu_bak_CGADSUB = next_gameplay_CGADSUB;
}

static void RestoreSomeGfxStateForPause(void) {  // 0x828E19
  layer2_scroll_x = pausemenu_bak_layer2_scroll_x;
  layer2_scroll_y = pausemenu_bak_layer2_scroll_y;
  reg_BGMODE = pausemenu_bak_BGMODE;
  reg_BG3VOFS = (reg_BG3VOFS & 0xFF00) | pausemenu_bak_BG3VOFS;   /* LOBYTE(...) = */
  reg_BG2VOFS = (reg_BG2VOFS & 0xFF00) | pausemenu_bak_BG2VOFS;
  reg_BG1VOFS = (reg_BG1VOFS & 0xFF00) | pausemenu_bak_BG1VOFS;
  reg_BG3HOFS = (reg_BG3HOFS & 0xFF00) | pausemenu_bak_BG3HOFS;
  reg_BG2HOFS = (reg_BG2HOFS & 0xFF00) | pausemenu_bak_BG2HOFS;
  reg_BG1HOFS = (reg_BG1HOFS & 0xFF00) | pausemenu_bak_BG1HOFS;
  reg_OBSEL = pausemenu_bak_OBSEL;
  reg_BG34NBA = pausemenu_bak_BG34NBA;
  reg_BG12NBA = pausemenu_bak_BG12NBA;
  reg_BG3SC = pausemenu_bak_BG3SC;
  reg_BG2SC = pausemenu_bak_BG2SC;
  reg_BG1SC = pausemenu_bak_BG1SC;
  reg_MOSAIC = pausemenu_bak_MOSAIC;
  next_gameplay_CGADSUB = pausemenu_bak_CGADSUB;
}

static void WriteSamusWireframeTilemap(void) {  // 0x82B20C
  uint16 i;
  for (i = 0; (equipped_items & 0x101) != B82W(kEquipmentScreenWireframeCmp + i); i += 2)
    ;
  uint16 src = B82W(kEquipmentScreenWireframePtrs + i);
  for (int m = 0; m < 17; m++)                          /* ram3800 + 472, filas de 64 bytes */
    for (int n = 0; n < 8; n++, src += 2)
      BG3800[236 + 32 * m + n] = B82W(src);
}

static void LoadEqupmentScreenReserveHealthTilemap(void) {  // 0x828F70
  if (samus_max_reserve_health) {
    int r42 = samus_reserve_health / 100;
    int rest = samus_reserve_health % 100;
    BG3800[394] = rest % 10 + 2052;
    BG3800[393] = rest / 10 + 2052;
    BG3800[392] = r42 + 2052;
  }
}

static void LoadPauseScreenBaseTilemaps(void) {  // 0x828EDA
  /* $B6:E000 (0x800) -> VRAM 0x3800 (BG2): la Genesis (PauseGfx) */
  for (uint16 i = 0; i < 0x200; i++) PMT[0x200 + i] = BB6W(0xE400 + 2 * i);   /* -> 7E:3400 */
  for (uint16 i = 0; i < 0x400; i++) PMT[0x400 + i] = BB6W(0xE800 + 2 * i);   /* -> 7E:3800 */
  uint16 v0 = addr_kDummySamusWireframeTilemap;
  uint16 v1 = 236;
  for (int m = 0; m < 17; m++) {
    for (int n = 0; n < 8; n++, v0 += 2)
      PMT[v1++] = B82W(v0);
    v1 += 8;
  }
  WriteSamusWireframeTilemap();
  LoadEqupmentScreenReserveHealthTilemap();
}

/* una celda del mapa de la pausa (64x32, índice 0-2047), como la arma LoadPauseMenuMapTilemap ($82:943D) en
   7E:4000; la Genesis la usa para dibujar el plano */
uint16 PauseMapCell(uint16 i) {
  uint16 v0 = area_index;
  if (!sign16(area_index - 7))
    v0 = 0;
  uint16 tile = kAreaMapTilemaps[2048 * v0 + i];
  uint8 bit = 0x80 >> (i & 7);
  if (map_station_byte_array[area_index]) {
    /* r38/r40: palabras de a 16 celdas con los bytes cambiados (swap16): el byte i/8 de cada una */
    if (map_tiles_explored[i >> 3] & bit)
      return tile & ~0x400;
    if (!(kAreaMapData[256 * v0 + (i >> 3)] & bit))
      return 31;
    return tile;
  }
  /* sin la estación: solo lo explorado (el SNES rota cada byte de map_tiles_explored 8 veces: queda igual) */
  if (!(map_tiles_explored[i >> 3] & bit))
    return 0x1f;
  return tile & 0xFBFF;
}

/* lo mismo para 8 celdas seguidas (i múltiplo de 8: un byte de los mapas de bits), más rápido para la Genesis */
void PauseMapCells8(uint16 i, uint16 *out) {
  uint16 v0 = area_index;
  if (!sign16(area_index - 7))
    v0 = 0;
  const uint16 *t = &kAreaMapTilemaps[2048 * v0 + i];
  uint8 ex = map_tiles_explored[i >> 3];
  uint8 md = map_station_byte_array[area_index] ? kAreaMapData[256 * v0 + (i >> 3)] : 0;
  for (uint8 bit = 0x80; bit; bit >>= 1, t++)
    *out++ = (ex & bit) ? (*t & ~0x400) : (md & bit) ? *t : 31;
}

static void LoadPauseMenuMapTilemapAndAreaLabel(void) {  // 0x8293C3
  reg_BG1HOFS = reg_BG4HOFS;
  reg_BG1VOFS = reg_BG4VOFS;
  /* LoadPauseMenuMapTilemap (7E:4000 -> VRAM 0x3000) y el nombre del área (kPauseAreaLabelTilemap -> VRAM 0x38AA) */
  PauseGfx(PAUSE_GFX_MAP);
}

static void BackupGameplayPalettesAndLoadForPause(void) {  // 0x828FD4
  for (int i = 0; i < 256; i++)
    PMT[i + 384] = palette_buffer[i];
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = BB6W(0xF000 + 2 * i);          /* kPauseScreenPalettes */
  nt_pal_dirty(0, 255);
}

static void SetupPpuForPauseMenu(void) {  // 0x82A09A
  reg_OBSEL = 1;
  reg_BGMODE = 9;
  reg_BG12NBA = 0;
  reg_BG34NBA = 4;
  reg_BG1SC = 49;
  reg_BG2SC = 56;
  reg_BG3SC = 88;
  reg_BG4SC = 0;
  reg_TM = 23;
  reg_MOSAIC = 0;
  reg_COLDATA[0] &= 0xE0;
  reg_COLDATA[1] &= 0xE0;
  reg_COLDATA[2] &= 0xE0;
  next_gameplay_CGADSUB = 0;
}

static void ResetPauseMenuAnimations(void) {  // 0x82A0F7
  pausemenu_reserve_tank_delay_ctr = 0;
  reg_BG1HOFS = 0;
  reg_BG2HOFS = 0;
  reg_BG3HOFS = 0;
  reg_BG2VOFS = 0;
  reg_BG3VOFS = 0;
  pausemenu_button_label_mode = 0;
  pausemenu_lr_animation_frame = 0;
  UNUSED_word_7E0745 = 0;
  samus_position_indicator_animation_frame = 0;
  samus_position_indicator_animation_timer = 0;
  samus_position_indicator_animation_loop_counter = 0;
  pausemenu_lr_animation_timer = B82W(kPauseLrHighlightAnimData);
  pausemenu_palette_animation_timer = 1;
  pausemenu_palette_animation_frame = 0;
}

static void ChangePaletteValues(uint16 *target, uint16 new_color, uint16 n) {  // 0x82A29D
  for (uint16 i = 0; i != n; i++)
    target[i] = (target[i] & ~0x1C00) | new_color;
}

static void LoadEquipmentScreenEquipmentTilemaps(void) {  // 0x82A12B
  if (samus_max_reserve_health) {
    CopyFrom82(B82W(kEquipmentTilemapOffs_Tanks), B82W(kEquipmentTilemaps_Tanks), 14);
    CopyFrom82(B82W(kEquipmentTilemapOffs_Tanks + 2), B82W(kEquipmentTilemaps_Tanks + 2), 14);
  }
  if (hyper_beam_flag) {
    for (int i = 0; i < 5; i++)
      CopyFrom82(B82W(kEquipmentTilemapOffs_Weapons + 2 * i), B82W(kHyperBeamWeaponsTilemaps + 2 * i), 10);
  } else {
    for (int i = 0; i < 6; i++) {          /* 6: el último lee la tabla siguiente (como el SNES) */
      uint16 dst = B82W(kEquipmentTilemapOffs_Weapons + 2 * i), mask = B82W(kEquipmentBitmasks_Weapons + 2 * i);
      if ((collected_beams & mask) != 0) {
        CopyFrom82(dst, B82W(kEquipmentTilemaps_Weapons + 2 * i), 10);
        if ((equipped_beams & mask) == 0)
          ChangePaletteValues(R3(dst), 3072, 5);
      } else {
        CopyFrom82(dst, addr_kEquipmentScreenTilemap_Blank, 10);
      }
    }
  }
  for (int i = 0; i < 6; i++) {
    uint16 dst = B82W(kEquipmentTilemapOffs_Suits + 2 * i), mask = B82W(kEquipmentBitmasks_Suits + 2 * i);
    if ((collected_items & mask) != 0) {
      CopyFrom82(dst, B82W(kEquipmentTilemaps_Suits + 2 * i), 18);
      if ((equipped_items & mask) == 0)
        ChangePaletteValues(R3(dst), 3072, 9);
    } else {
      CopyFrom82(dst, addr_kEquipmentScreenTilemap_Blank, 18);
    }
  }
  for (int i = 0; i < 3; i++) {
    uint16 dst = B82W(kEquipmentTilemapOffs_Boots + 2 * i), mask = B82W(kEquipmentBitmasks_Boots + 2 * i);
    if ((collected_items & mask) != 0) {
      CopyFrom82(dst, B82W(kEquipmentTilemaps_Boots + 2 * i), 18);
      if ((equipped_items & mask) == 0)
        ChangePaletteValues(R3(dst), 3072, 9);
    } else {
      CopyFrom82(dst, addr_kEquipmentScreenTilemap_Blank, 18);
    }
  }
}

/* SetPauseScreenButtonLabelPalettes_0/1/2 ($82:A796/A6DF/A628): las palabras 805-809/837-841 ("MAP"),
   812-815/844-847 ("EXIT") y 822-826/854-858 ("SAMUS") de 7E:3000, en paleta 2 (apagada) o 5 (encendida) */
static void LabelPal(uint16 first, uint16 n, uint16 pal) {
  for (uint16 i = 0; i < n; i++)
    PMT[first + i] = (PMT[first + i] & 0xE3FF) | pal;
}

static void SetPauseScreenButtonLabelPalettes_0(void) {  // 0x82A796
  LabelPal(822, 5, 0x800);
  LabelPal(854, 5, 0x800);
  LabelPal(812, 4, 0x800);
  LabelPal(844, 4, 0x800);
  LabelPal(805, 5, 0x1400);
  LabelPal(837, 5, 0x1400);
}

static void SetPauseScreenButtonLabelPalettes_1(void) {  // 0x82A6DF
  LabelPal(812, 4, 0x800);
  LabelPal(844, 4, 0x800);
  LabelPal(805, 5, 0x1400);
  LabelPal(837, 5, 0x1400);
  LabelPal(822, 5, 0x1400);
  LabelPal(854, 5, 0x1400);
}

static void SetPauseScreenButtonLabelPalettes_2(void) {  // 0x82A628
  LabelPal(805, 5, 0x800);
  LabelPal(837, 5, 0x800);
  LabelPal(812, 4, 0x800);
  LabelPal(844, 4, 0x800);
  LabelPal(822, 5, 0x1400);
  LabelPal(854, 5, 0x1400);
}

static void SetPauseScreenButtonLabelPalettes(void) {  // 0x82A615
  switch (pausemenu_button_label_mode) {
  case 0: SetPauseScreenButtonLabelPalettes_0(); break;
  case 1: SetPauseScreenButtonLabelPalettes_1(); break;
  case 2: SetPauseScreenButtonLabelPalettes_2(); break;
  }
}

static void UpdatePauseMenuLRStartVramTilemap(void) {  // 0x82A84D
  /* cola de la VRAM: 128 bytes de 7E:3640 a la fila 25 del BG2. La Genesis copia esas dos filas en cada cuadro de
     la pausa (md_pause.c) */
}

/* ------------------------------------------------------------------ límites y scroll del mapa */
static const uint8 k0x80Shr[8] = { 0x80, 0x40, 0x20, 0x10, 8, 4, 2, 1 };

/* el mapa de 64x32 celdas se lee como en el SNES: un puntero a bytes (kAreaMapData o map_tiles_explored) que puede
   salir del arreglo hacia los lados (el SNES lee la RAM o la ROM que siga): MB() lo resuelve igual */
typedef struct { const uint8 *p; int16 o; uint8 ram; } MapBits;
static uint8 MB(MapBits m, int16 i) {
  int16 a = m.o + i;
  if (m.ram) {                                          /* map_tiles_explored (7E:07F7): 256 bytes */
    if ((uint16)a < 256) return map_tiles_explored[a];
    nt_unimpl("DetermineMapScrollLimits (lectura fuera de map_tiles_explored)");
    return 0;
  }
  return B82B((uint16)(m.o + i));                       /* kPauseMenuMapData en $82: m.o es la dirección */
}

static uint16 DetermineLeftmostMapColumn(MapBits r0) {
  uint16 v0;
  uint16 result = 0;
LABEL_2:
  v0 = result & 7;
  uint8 r18 = k0x80Shr[v0];
  uint16 v2 = 0;
  while ((r18 & MB(r0, v2)) == 0) {
    v2 += 4;
    if ((int16)(v2 - 128) >= 0) {
      if ((int16)(++result - 64) >= 0)
        return 26;
      if ((result & 7) == 0)
        r0.o += 1;
      if (result == 32)
        r0.o += 123;
      goto LABEL_2;
    }
  }
  return result;
}

static uint16 DetermineRightmostMapColumn(MapBits r0) {  // 0x829FA9
  uint16 result = 63;
LABEL_2:;
  uint8 r18 = k0x80Shr[result & 7];
  uint16 v1 = 0;
  while ((r18 & MB(r0, v1)) == 0) {
    v1 += 4;
    if ((int16)(v1 - 128) >= 0) {
      if ((--result & 0x8000) != 0)
        return 28;
      if ((result & 7) == 7)
        r0.o--;
      if (result == 31)
        r0.o -= 124;
      goto LABEL_2;
    }
  }
  return result;
}

static uint16 DetermineTopmostMapColumn(MapBits r0) {  // 0x82A009
  MapBits r3 = r0;
  r3.o += 128;
  uint16 result = 0;
  uint16 v1 = 0;
  while (!MB(r0, v1) && !MB(r3, v1)) {
    if ((int16)(++v1 - 4) >= 0) {
      v1 = 0;
      r0.o += 4;
      r3.o += 4;
      if ((int16)(++result - 31) >= 0)
        return 1;
    }
  }
  return result;
}

static uint16 DetermineBottommostMapColumn(MapBits r0) {  // 0x82A053
  MapBits r3 = r0;
  r3.o += 128;
  uint16 result = 31;
  uint16 v1 = 0;
  while (!MB(r0, v1) && !MB(r3, v1)) {
    if ((int16)(++v1 - 4) >= 0) {
      v1 = 0;
      r0.o -= 4;
      r3.o -= 4;
      if (!--result)
        return 11;
    }
  }
  return result;
}

static void DetermineMapScrollLimits(void) {  // 0x829EC4
  MapBits r6;
  if (has_area_map) {
    r6.p = 0; r6.ram = 0; r6.o = (int16)B82W(kPauseMenuMapData + 2 * area_index);
  } else {
    r6.p = map_tiles_explored; r6.ram = 1; r6.o = 0;
  }
  map_min_x_scroll = DetermineLeftmostMapColumn(r6) * 8;
  if (area_index == 4)
    map_min_x_scroll -= 24;
  MapBits t = r6; t.o += 131;
  map_max_x_scroll = DetermineRightmostMapColumn(t) * 8;
  map_min_y_scroll = DetermineTopmostMapColumn(r6) * 8;
  t = r6; t.o += 124;
  map_max_y_scroll = DetermineBottommostMapColumn(t) * 8;
}

static void SetupMapScrollingForPauseMenu(uint16 a) {  // 0x829E27
  int16 v1;
  int16 v2;
  reg_BG1HOFS = map_min_x_scroll + ((uint16)(map_max_x_scroll - map_min_x_scroll) >> 1) - 128;
  uint16 r18 = 8 * (room_x_coordinate_on_map + ((samus_x_pos & 0xFF00) >> 8)) - reg_BG1HOFS;
  v1 = 224 - r18;
  if ((int16)(224 - r18) >= 0) {
    r18 = 32 - r18;
    if ((r18 & 0x8000) == 0)
      reg_BG1HOFS -= r18;
  } else {
    reg_BG1HOFS -= v1;
  }
  r18 = map_min_y_scroll + ((uint16)(map_max_y_scroll - map_min_y_scroll) >> 1) + 16;
  reg_BG1VOFS = -((a - r18) & 0xFFF8);
  r18 = 8 * (room_y_coordinate_on_map + HI8(samus_y_pos) + 1) + ((a - r18) & 0xFFF8);
  v2 = 64 - r18;
  if ((int16)v2 >= 0) {
    reg_BG1VOFS -= v2;
    if (sign16(reg_BG1VOFS + 40))
      reg_BG1VOFS = -40;
  }
}

static void ContinueInitPauseMenu(void) {  // 0x829009
  SetupPpuForPauseMenu();
  ResetPauseMenuAnimations();
  LoadEquipmentScreenEquipmentTilemaps();
  SetPauseScreenButtonLabelPalettes_0();
  UpdatePauseMenuLRStartVramTilemap();
  DetermineMapScrollLimits();
  SetupMapScrollingForPauseMenu(0x80);
}

static void CallPauseHook(void) {
  /* pause_hook: PauseHook_Empty salvo en las salas de Kraid y Draygon (door.c todavía no los pone) */
}

static void QueueClearingOfFxTilemap(void) {  // 0x80A211
  /* 7E:4000 = 0x184E y a la VRAM del BG3 (0x5880): la Genesis no tiene ese búfer (no se usa en la pausa) */
}

static uint8 GameState_13_Pausing_Async(void) {  // 0x828CEF
  DisableHdmaObjects();
  reg_HDMAEN = 0;
  DisableAnimtiles_();
  /* BackupBG2TilemapForPauseMenu ($82:8D51): VRAM -> 7E:DF5C; la Genesis vuelve a dibujar la sala al salir */
  CallPauseHook();
  CancelSoundEffects();
  BackupSomeGfxStateForPause();
  PauseGfx(PAUSE_GFX_ENTER);                            /* LoadPauseMenuTilesAndClearBG2 ($82:8E75) */
  LoadPauseScreenBaseTilemaps();
  LoadPauseMenuMapTilemapAndAreaLabel();
  BackupGameplayPalettesAndLoadForPause();
  ContinueInitPauseMenu();
  screen_fade_delay = 1;
  screen_fade_counter = 1;
  pausemenu_leftarrow_animation_frame = 0;
  pausemenu_palette_animation_timer = 1;
  map_scrolling_direction = 0;
  map_scrolling_speed_index = 0;
  QueueClearingOfFxTilemap();
  ++game_state;
  return 0;
}

/* ------------------------------------------------------------------ íconos del mapa */
static uint16 CheckIfMapPositionIsExplored(uint16 k, uint16 j) {  // 0x82B855
  int r18 = (uint16)(k & 0xFF00) >> 1;
  r18 += (uint8)k >> 6;
  r18 += (uint16)(j & 0xFFF8) >> 1;
  uint16 v3 = (k >> 3) & 7;
  return (k0x80Shr[v3] & map_tiles_explored[(uint8)r18]) != 0;
}

static void DrawMapIconsOfType(uint16 a, uint16 r34, uint16 r36, uint16 r3) {  // 0x82B81C
  while (1) {
    uint16 x = B82W(a), y = B82W(a + 2);
    if ((x & 0x8000) != 0)
      break;
    uint8 v3 = r36 & 1;
    r36 >>= 1;
    if (v3) {
      if (CheckIfMapPositionIsExplored(x, y))
        DrawMenuSpritemap(r34, x - reg_BG1HOFS, y - reg_BG1VOFS, r3);
    }
    a += 4;
  }
}

static void DrawSimpleMapIcons(uint16 a, uint16 k, uint16 r3) {  // 0x82B805
  uint16 v2 = B82W(k + 2 * area_index);
  if (v2)
    DrawMapIconsOfType(v2, a, -1, r3);
}

static void DrawSaveStationMapIcon(uint16 a, uint16 k, uint16 r3) {  // 0x82B798
  static const uint8 kShlBit[8] = { 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80 };
  if (area_index == sram_area_index) {
    uint16 R36 = used_save_stations_and_elevators[2 * area_index] | used_save_stations_and_elevators[2 * area_index + 1] << 8;
    R36 = (R36 & 0xFF00) | (uint8)(R36 & kShlBit[load_station_index & 7]);
    uint16 v2 = B82W(k + 2 * area_index);
    if (v2)
      DrawMapIconsOfType(v2, a, R36, r3);
  }
}

static void DrawBossMapIcons(uint16 a, uint16 k) {  // 0x82B892
  int bits = boss_bits_for_area[area_index];
  uint16 t = B82W(k + 2 * area_index);
  if (t == 0)
    return;
  for (; ; t += 4) {
    uint16 x = B82W(t), y = B82W(t + 2);
    if (x == 0xffff)
      return;
    if (x != 0xfffe) {
      int v5 = bits & 1;
      bits >>= 1;
      if (v5) {
        DrawMenuSpritemap(0x62, x - reg_BG1HOFS, y - reg_BG1VOFS, 3584);
        DrawMenuSpritemap(a, x - reg_BG1HOFS, y - reg_BG1VOFS, 3072);
        continue;
      }
      if (has_area_map) {
        DrawMenuSpritemap(a, x - reg_BG1HOFS, y - reg_BG1VOFS, 3584);
        continue;
      }
    }
    bits >>= 1;
  }
}

static const uint16 kMap_Criteria_SavePoints[2] = { 0xd8, 0x28 };

static void DrawMapIcons(void) {  // 0x82B672
  DrawBossMapIcons(9, addr_kMapIconDataPointers);
  DrawSimpleMapIcons(0xB, addr_kMapIconDataPointers + 0x10, 3584);
  DrawSimpleMapIcons(0xA, addr_kMapIconDataPointers + 0x20, 3584);
  DrawSimpleMapIcons(0x4E, addr_kMapIconDataPointers + 0x30, 3584);
  DrawSaveStationMapIcon(8, 0xC80B, 1024);
  if (enable_debug)
    DrawSimpleMapIcons(8, 0xC82B, 1024);
  if (!area_index)
    DrawMenuSpritemap(0x63, kMap_Criteria_SavePoints[0] - reg_BG1HOFS, kMap_Criteria_SavePoints[1] - reg_BG1VOFS, 3584);
}

static void DisplayMapElevatorDestinations(void) {  // 0x82BB30
  if (map_station_byte_array[area_index]) {
    for (uint16 i = B82W(kMapElevatorDests + 2 * area_index); ; i += 6) {
      if (B82W(i) == 0xFFFF)
        break;
      DrawMenuSpritemap(B82W(i + 4), B82W(i) - reg_BG1HOFS, B82W(i + 2) - reg_BG1VOFS, 0);
    }
  }
}

static const uint16 kMap_SamusPositionIndicator_Delays[4] = { 8, 4, 8, 4 };
static const uint16 kMap_SamusPositionIndicator_SpritemapIds[4] = { 0x5f, 0x60, 0x61, 0x60 };

static uint16 UpdateSamusPositionIndicatorAnimation(void) {  // 0x82B9FC
  uint16 v0 = samus_position_indicator_animation_timer;
  if (!v0) {
    uint16 v1 = samus_position_indicator_animation_frame + 2;
    if ((int16)(v1 - 8) >= 0) {
      ++samus_position_indicator_animation_loop_counter;
      v1 = 0;
    }
    samus_position_indicator_animation_frame = v1;
    v0 = kMap_SamusPositionIndicator_Delays[v1 >> 1];
    samus_position_indicator_animation_timer = v0;
  }
  samus_position_indicator_animation_timer = v0 - 1;
  return kMap_SamusPositionIndicator_SpritemapIds[samus_position_indicator_animation_frame >> 1];
}

static void MapScreenDrawSamusPositionIndicator(void) {  // 0x82B9C8
  uint16 a = UpdateSamusPositionIndicatorAnimation();
  DrawMenuSpritemap(a, 8 * (room_x_coordinate_on_map + HI8(samus_x_pos)) - reg_BG1HOFS,
                    8 * (room_y_coordinate_on_map + HI8(samus_y_pos) + 1) - reg_BG1VOFS, 3584);
}

static void DrawMapScrollArrowAndCheckToScroll(uint16 k) {  // 0x82B90A (MapScrollArrowData en $82)
  DrawPauseScreenSpriteAnim(B82W(k + 4), B82W(k), B82W(k + 2));
  if ((joypad1_lastkeys & B82W(k + 6)) != 0 && !map_scrolling_direction)
    map_scrolling_direction = B82W(k + 8);
}

static void HandleMapScrollArrows(void) {  // 0x82B934
  if (sign16(map_min_x_scroll - 24 - reg_BG1HOFS))
    DrawMapScrollArrowAndCheckToScroll(0xB9A0);
  if (!sign16(map_max_x_scroll - 232 - reg_BG1HOFS))
    DrawMapScrollArrowAndCheckToScroll(0xB9AA);
  if (sign16(map_min_y_scroll - 56 - reg_BG1VOFS))
    DrawMapScrollArrowAndCheckToScroll(0xB9B4);
  if (sign16(map_max_y_scroll - 177 - reg_BG1VOFS)) {
    if (map_scrolling_direction == B82W(0xB9BE + 8)) {
      map_scrolling_gear_switch_timer = 0;
      map_scrolling_direction = 0;
      map_scrolling_speed_index = 0;
    }
  } else {
    DrawMapScrollArrowAndCheckToScroll(0xB9BE);
  }
}

static const uint16 kMapScrollingSpeedTable[32] = {  // 0x82928E
  0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static uint16 MapScrolling_GetSpeedIndex(void) {  // 0x82927F
  uint16 result = map_scrolling_speed_index;
  if (!map_scrolling_gear_switch_timer)
    return map_scrolling_speed_index + 32;
  return result;
}

static void MapScrolling_Common(void) {  // 0x829299
  ++map_scrolling_speed_index;
  if ((++map_scrolling_speed_index & 0xF) == 0) {
    QueueSfx1_Max6(0x36);
    map_scrolling_direction = 0;
    map_scrolling_speed_index = 0;
    if (map_scrolling_gear_switch_timer)
      --map_scrolling_gear_switch_timer;
  }
}

static void MapScrolling(void) {  // 0x829261 (kMapScrollingFuncs)
  /* el índice puede pasar de la tabla de 32 (velocidad + 32): el SNES lee lo que sigue en el banco */
  uint16 i = MapScrolling_GetSpeedIndex() >> 1;
  uint16 sp = i < 32 ? kMapScrollingSpeedTable[i] : B82W(0x928E + 2 * i);
  switch (map_scrolling_direction) {
  case 0: map_scrolling_gear_switch_timer = 4; return;          // MapScrolling_0_None ($82:9278)
  case 1: reg_BG1HOFS -= sp; break;                             // MapScrolling_1_Left
  case 2: reg_BG1HOFS += sp; break;                             // MapScrolling_2_Right ($82:92BD)
  case 3: reg_BG1VOFS -= sp; break;                             // MapScrolling_3_Up ($82:92CA)
  case 4: reg_BG1VOFS += sp; break;                             // MapScrolling_4_Down ($82:92D7)
  }
  MapScrolling_Common();
}

/* ------------------------------------------------------------------ L, R y START */
static const uint16 kPauseLrButtonPressedHighlight_Spritemap[2] = { 0x28, 0x29 };
static const uint16 kPauseLrButtonPressedHighlight_Y[2] = { 0x18, 0xe8 };
static const uint16 kPauseLrButtonPressedHighlight_X[2] = { 0xd0, 0xd0 };

static void HandlePauseMenuLRPressHighlight(void) {  // 0x82A56D
  if (pausemenu_start_lr_pressed_highlight_timer) {
    --pausemenu_start_lr_pressed_highlight_timer;
    if (pausemenu_shoulder_button_highlight) {
      int v0 = pausemenu_shoulder_button_highlight - 1;
      DrawMenuSpritemap(kPauseLrButtonPressedHighlight_Spritemap[v0], kPauseLrButtonPressedHighlight_Y[v0],
                        kPauseLrButtonPressedHighlight_X[v0] - 1, 0);
    }
  }
}

static void HandlePauseScreenLrInput(void) {  // 0x82A50C
  if ((newly_held_down_timed_held_input & kButton_L) != 0) {
    if (!pausemenu_button_label_mode)
      return;
    pausemenu_start_lr_pressed_highlight_timer = 5;
    menu_index = 5;
    pausemenu_shoulder_button_highlight = 1;
    pausemenu_button_label_mode = 0;
    SetPauseScreenButtonLabelPalettes();
    QueueSfx1_Max6(0x38);
    return;
  }
  if ((newly_held_down_timed_held_input & kButton_R) != 0 && pausemenu_button_label_mode != 2) {
    pausemenu_start_lr_pressed_highlight_timer = 5;
    menu_index = 2;
    pausemenu_shoulder_button_highlight = 2;
    pausemenu_button_label_mode = 2;
    SetPauseScreenButtonLabelPalettes();
    QueueSfx1_Max6(0x38);
  }
}

static void DrawLrHighlight(void) {  // 0x82A59A
  DrawPauseScreenSpriteAnim(2, 0x18, 0xD0);
  DrawPauseScreenSpriteAnim(2, 0xE8, 0xD0);
}

static void HandlePauseScreenLR(void) {  // 0x82A505
  HandlePauseScreenLrInput();
  DrawLrHighlight();
}

static void HandlePauseScreenStart(void) {  // 0x82A5B7
  if ((newly_held_down_timed_held_input & kButton_Start) != 0) {
    QueueSfx1_Max6(0x38);
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    uint16 v0 = pausemenu_button_label_mode;
    pausemenu_button_label_mode = 1;
    SetPauseScreenButtonLabelPalettes();
    pausemenu_button_label_mode = v0;
    pausemenu_start_lr_pressed_highlight_timer = 11;
    ++game_state;
  }
  UpdatePauseMenuLRStartVramTilemap();
}

static void HighlightPauseScreenButton(void) {  // 0x82A5F1
  if (pausemenu_start_lr_pressed_highlight_timer) {
    --pausemenu_start_lr_pressed_highlight_timer;
    DrawMenuSpritemap(0x2B, 0x90, 0xD0, 0);
  }
}

/* ------------------------------------------------------------------ pantalla de equipo */
static void EquipmentScreenEnergyArrowGlow_On(void) {  // 0x82AE01
  for (int i = 0; i < 8; i++)
    BG3800[129 + i * 32] = (BG3800[129 + i * 32] & ~0x1C00) | 0x1800;
  for (int i = 0; i < 2; i++)
    BG3800[385 + i] = (BG3800[385 + i] & ~0x1C00) | 0x1800;
}

static void EquipmentScreenEnergyArrowGlow_Off(void) {  // 0x82AE46
  for (int i = 0; i < 8; i++)
    BG3800[129 + i * 32] = (BG3800[129 + i * 32] & ~0x1C00) | 0x1C00;
  for (int i = 0; i < 2; i++)
    BG3800[385 + i] = (BG3800[385 + i] & ~0x1C00) | 0x1C00;
}

static const uint16 kEquipmentScreenGlowingArrowPalettes0[32] = { 0x39e, 0x77d, 0xb5c, 0xf5b, 0x133a, 0x171a, 0x1f19, 0x22f8, 0x26d7, 0x2ad6, 0x2eb6, 0x3695, 0x3a94, 0x3e73, 0x4253, 0x4a52, 0x4a52, 0x4253, 0x3e73, 0x3a94, 0x3695, 0x2eb6, 0x2ad6, 0x26d7, 0x22f8, 0x1f19, 0x171a, 0x133a, 0xf5b, 0xb5c, 0x77d, 0x39e };
static const uint16 kEquipmentScreenGlowingArrowPalettes1[32] = { 0x156, 0x155, 0x554, 0x954, 0xd53, 0xd52, 0x1152, 0x1551, 0x1970, 0x1d70, 0x1d6f, 0x216e, 0x256e, 0x296d, 0x296c, 0x318c, 0x318c, 0x296c, 0x296d, 0x256e, 0x216e, 0x1d6f, 0x1d70, 0x1970, 0x1551, 0x1152, 0xd52, 0xd53, 0x954, 0x554, 0x155, 0x156 };

static void EquipmentScreenGlowingArrowSolidOn(void) {  // 0x82ADDD
  palette_buffer[107] = 926;
  palette_buffer[102] = 342;
  nt_pal_dirty(102, 107);
  EquipmentScreenEnergyArrowGlow_On();
}

static void EquipmentScreenGlowingArrowSolidOff(void) {  // 0x82ADEF
  palette_buffer[107] = 926;
  palette_buffer[102] = 342;
  nt_pal_dirty(102, 107);
  EquipmentScreenEnergyArrowGlow_Off();
}

static void EquipmentScreenGlowingArrowAnimated(void) {  // 0x82AD29
  if (reserve_health_mode == 1) {
    int v0 = nmi_frame_counter_byte & 0x1F;
    palette_buffer[102] = kEquipmentScreenGlowingArrowPalettes0[v0];
    palette_buffer[107] = kEquipmentScreenGlowingArrowPalettes1[v0];
    nt_pal_dirty(102, 107);
    EquipmentScreenEnergyArrowGlow_On();
  } else {
    palette_buffer[107] = 926;
    palette_buffer[102] = 342;
    nt_pal_dirty(102, 107);
    EquipmentScreenEnergyArrowGlow_Off();
  }
}

static void EquipmentScreenGlowingArrow(void) {  // 0x82AD0A
  if ((uint8)pausemenu_equipment_category_item)
    EquipmentScreenGlowingArrowSolidOff();
  else if (HI8(pausemenu_equipment_category_item))
    EquipmentScreenGlowingArrowSolidOn();
  else
    EquipmentScreenGlowingArrowAnimated();
}

static void EquipmentScreenHudReserveAutoTilemap_On_BUGGY(void) {  // 0x82AEFD (lee basura, como el SNES)
  uint16 v0 = samus_reserve_health ? 0x998B : 0x9997;
  hud_tilemap[8] = B82W(v0);
  hud_tilemap[9] = B82W(v0 + 2);
  hud_tilemap[40] = B82W(v0 + 4);
  hud_tilemap[41] = B82W(v0 + 6);
  hud_tilemap[72] = B82W(v0 + 8);
  hud_tilemap[73] = B82W(v0 + 10);
}

static void EquipmentScreenHudReserveAutoTilemap_Off(void) {  // 0x82AF33
  hud_tilemap[8] = 11279;
  hud_tilemap[9] = 11279;
  hud_tilemap[40] = 11279;
  hud_tilemap[41] = 11279;
  hud_tilemap[72] = 11279;
  hud_tilemap[73] = 11279;
}

static const uint16 kEquipmentScreenTilemap_MANUAL[4] = { 0x3d46, 0x3d47, 0x3d48, 0x3d49 };
static const uint16 kEquipmentScreenTilemap_AUTO[4] = { 0x3d56, 0x3d57, 0x3d58, 0x3d59 };

static void EquipmentScreenCategory_Tanks_0(void) {
  if ((joypad1_newkeys & kButton_A) != 0 && samus_max_reserve_health) {
    QueueSfx1_Max6(0x37);
    if (reserve_health_mode == 1) {
      reserve_health_mode = 2;
      EquipmentScreenHudReserveAutoTilemap_Off();
      for (int i = 0; i < 4; i++)
        BG3800[i + 327] = kEquipmentScreenTilemap_MANUAL[i] | (BG3800[i + 327] & 0xFC00);
    } else {
      reserve_health_mode = 1;
      EquipmentScreenHudReserveAutoTilemap_On_BUGGY();
      for (int i = 0; i < 4; i++)
        BG3800[i + 327] = kEquipmentScreenTilemap_AUTO[i] | (BG3800[i + 327] & 0xFC00);
    }
  }
}

static const uint16 kReserveTankEnergyTransferPerFrame = 1;

static void EquipmentScreenCategory_Tanks_1(void) {  // 0x82AF4F
  if (!pausemenu_reserve_tank_delay_ctr) {
    if ((joypad1_newkeys & kButton_A) == 0)
      return;
    pausemenu_reserve_tank_delay_ctr = (samus_reserve_health + 7) & 0xFFF8;
  }
  if ((--pausemenu_reserve_tank_delay_ctr & 7) == 7)
    QueueSfx3_Max6(0x2D);
  samus_health += kReserveTankEnergyTransferPerFrame;
  if ((int16)(samus_health - samus_max_health) < 0) {
    samus_reserve_health -= kReserveTankEnergyTransferPerFrame;
    if ((int16)samus_reserve_health > 0)
      return;
    samus_health += samus_reserve_health;
  } else {
    samus_health = samus_max_health;
  }
  samus_reserve_health = 0;
  pausemenu_reserve_tank_delay_ctr = 0;
  EquipmentScreenEnergyArrowGlow_Off();
  pausemenu_equipment_category_item = 0;
}

static uint16 EquipmentScreenMoveToReserveTanks(void) {  // 0x82B43F
  uint16 result = samus_max_reserve_health;
  if (samus_max_reserve_health) {
    pausemenu_equipment_category_item = 0;
    QueueSfx1_Max6(0x37);
    return 1;
  }
  return result;
}

static void EquipmentScreenMoveToBeams(uint16 v0, uint16 r18) {  // 0x82B456
  if (hyper_beam_flag) {
LABEL_4:
    pausemenu_equipment_category_item = r18;
  } else {
    while ((B82W(kEquipmentBitmasks_Weapons + v0) & collected_beams) == 0) {
      v0 += 2;
      if ((int16)(v0 - 10) >= 0)
        goto LABEL_4;
    }
    pausemenu_equipment_category_item = (uint16)((v0 >> 1) << 8) | 1;
    QueueSfx1_Max6(0x37);
  }
}

static uint16 EquipmentScreenMoveToBottomOfBeams(uint16 k) {  // 0x82B489
  if (hyper_beam_flag)
    return -1;
  while ((B82W(kEquipmentBitmasks_Weapons + k) & collected_beams) == 0) {
    k -= 2;
    if ((k & 0x8000) != 0)
      return -1;
  }
  QueueSfx1_Max6(0x37);
  uint16 result = (k >> 1) << 8 | 1;
  pausemenu_equipment_category_item = result;
  return result;
}

static uint16 EquipmentScreenMoveLowerOnSuitsMisc(uint16 v0) {  // 0x82B4B7
  while ((B82W(kEquipmentBitmasks_Suits + v0) & collected_items) == 0) {
    v0 += 2;
    if ((int16)(v0 - 10) >= 0)
      return -1;
  }
  QueueSfx1_Max6(0x37);
  pausemenu_equipment_category_item = (uint16)((v0 >> 1) << 8) | 2;
  return 0;
}

static void EquipmentScreenMoveToScrewOrHigherOnSuits(uint16 v0, uint16 r18) {  // 0x82B4E6
  while ((B82W(kEquipmentBitmasks_Suits + v0) & collected_items) == 0) {
    v0 -= 2;
    if ((v0 & 0x8000) != 0) {
      pausemenu_equipment_category_item = r18;
      return;
    }
  }
  QueueSfx1_Max6(0x37);
  pausemenu_equipment_category_item = (v0 >> 1) << 8 | 2;
}

static void EquipmentScreenMoveToHighJumpOrLowerInBoots(uint16 v0, uint16 r18) {  // 0x82B511
  while ((B82W(kEquipmentBitmasks_Boots + v0) & collected_items) == 0) {
    v0 += 2;
    if ((int16)(v0 - 6) >= 0) {
      pausemenu_equipment_category_item = r18;
      return;
    }
  }
  QueueSfx1_Max6(0x37);
  pausemenu_equipment_category_item = (v0 >> 1) << 8 | 3;
}

static uint16 EquipmentScreenCategory_Boots_MoveUpInBoots(uint16 k) {  // 0x82B53F
  while ((B82W(kEquipmentBitmasks_Boots + k) & collected_items) == 0) {
    k -= 2;
    if ((k & 0x8000) != 0)
      return -1;
  }
  QueueSfx1_Max6(0x37);
  uint16 result = (k >> 1) << 8 | 3;
  pausemenu_equipment_category_item = result;
  return result;
}

static void EquipmentScreenHandleDpad(void) {  // 0x82AC8B
  uint16 r18 = pausemenu_equipment_category_item;
  if ((joypad1_newkeys & kButton_Right) != 0) {
    if ((joypad1_newkeys & kButton_Down) != 0 || EquipmentScreenMoveLowerOnSuitsMisc(0) == 0xFFFF)
      EquipmentScreenMoveToHighJumpOrLowerInBoots(0, r18);
  } else if ((joypad1_newkeys & kButton_Up) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) != 0) {
      QueueSfx1_Max6(0x37);
      pausemenu_equipment_category_item -= 256;
    }
  } else if ((joypad1_newkeys & kButton_Down) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) == 256
        || reserve_health_mode == 1
        || (pausemenu_equipment_category_item += 256, !samus_reserve_health)) {
      EquipmentScreenMoveToBeams(0, r18);
    } else {
      QueueSfx1_Max6(0x37);
    }
  }
}

static void EquipmentScreenCategory_Tanks(void) {  // 0x82AC70
  if (HI8(pausemenu_equipment_category_item))
    EquipmentScreenCategory_Tanks_1();
  else
    EquipmentScreenCategory_Tanks_0();
  EquipmentScreenHandleDpad();
  EquipmentScreenGlowingArrow();
}

/* kEquipmentPtrsToBitsets: direcciones de la RAM de las variables de lo equipado */
static uint16 *EquipBitset(uint16 a) {
  switch (a) {
  case 0x9A2: return &equipped_items;
  case 0x9A6: return &equipped_beams;
  }
  nt_unimpl("EquipmentScreenCategory_ButtonResponse (variable sin preparar)");
  return &equipped_items;
}

static void EquipmentScreenCategory_ButtonResponse(uint16 r24) {  // 0x82B568
  if ((joypad1_newkeys & kButton_A) != 0) {
    QueueSfx1_Max6(0x38);
    int item = HI8(pausemenu_equipment_category_item);
    int category = (uint8)pausemenu_equipment_category_item;
    uint16 target = B82W(B82W(kEquipmentPtrsToRamTilemapOffsets + 2 * category) + item * 2);
    uint16 *var = EquipBitset(B82W(kEquipmentPtrsToBitsets + 2 * category));
    uint16 mask = B82W(B82W(kEquipmentPtrsToBitmasks + 2 * category) + item * 2);
    if ((*var & mask) != 0) {
      *var &= ~mask;
      ChangePaletteValues(R3(target), 0xc00, r24 >> 1);
    } else {
      *var |= mask;
      uint16 src = B82W(B82W(kEquipmentPtrsToEquipmentTilemaps + 2 * category) + item * 2);
      CopyFrom82(target, src, r24);
    }
  }
}

static void EquipmentScreenCategory_Weapons_MoveButtons(void) {  // 0x82AFDB
  uint16 r18 = pausemenu_equipment_category_item;
  if ((joypad1_newkeys & 0x100) != 0) {
    if ((joypad1_newkeys & 0x800) != 0) {
      EquipmentScreenMoveLowerOnSuitsMisc(0);
    } else if (EquipmentScreenMoveLowerOnSuitsMisc(4)) {
      EquipmentScreenMoveToHighJumpOrLowerInBoots(0, r18);
    }
  } else if ((joypad1_newkeys & 0x400) != 0) {
    if (!hyper_beam_flag && pausemenu_equipment_category_item != 1025) {
      pausemenu_equipment_category_item += 256;
      EquipmentScreenMoveToBeams(2 * HI8(pausemenu_equipment_category_item), r18);
    }
  } else if ((joypad1_newkeys & 0x800) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) == 0
        || (pausemenu_equipment_category_item -= 256,
            EquipmentScreenMoveToBottomOfBeams(2 * HI8(pausemenu_equipment_category_item)) == 0xFFFF)) {
      if (!EquipmentScreenMoveToReserveTanks())
        pausemenu_equipment_category_item = r18;
    }
  }
}

static void EquipmentScreenCategory_Weapons_PlazmaSpazerCheck(uint16 R36) {  // 0x82B068
  int t = equipped_beams & ~R36;
  if ((t & 4) != 0) {
    if ((R36 & 4) == 0 && (equipped_beams & 8) != 0) {
      equipped_beams &= ~8;
      ChangePaletteValues(R3(B82W(kEquipmentTilemapOffs_Weapons + 8)), 3072, 5);
    }
  } else if ((t & 8) != 0 && (R36 & 8) == 0 && (equipped_beams & 4) != 0) {
    equipped_beams &= ~4;
    ChangePaletteValues(R3(B82W(kEquipmentTilemapOffs_Weapons + 6)), 3072, 5);
  }
}

static void EquipmentScreenCategory_Weapons(void) {  // 0x82AFBE
  EquipmentScreenCategory_Weapons_MoveButtons();
  uint16 R36 = equipped_beams;
  if (collected_beams) {
    if ((uint8)pausemenu_equipment_category_item == 1) {   /* (el decomp corrige aquí una variable) */
      EquipmentScreenCategory_ButtonResponse(10);
      EquipmentScreenCategory_Weapons_PlazmaSpazerCheck(R36);
    }
  }
}

static void EquipmentScreenCategory_Suit_MoveResponse(void) {  // 0x82B0D2
  uint16 r18 = pausemenu_equipment_category_item;
  if ((joypad1_newkeys & kButton_Left) != 0) {
    if ((joypad1_newkeys & kButton_Down) == 0) {
      if (EquipmentScreenMoveToReserveTanks())
        return;
      pausemenu_equipment_category_item = r18;
    }
    EquipmentScreenMoveToBeams(0, r18);
  } else if ((joypad1_newkeys & kButton_Up) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) != 0) {
      pausemenu_equipment_category_item -= 256;
      EquipmentScreenMoveToScrewOrHigherOnSuits(2 * HI8(pausemenu_equipment_category_item), r18);
    }
  } else if ((joypad1_newkeys & kButton_Down) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) == 1280
        || (pausemenu_equipment_category_item += 256,
            EquipmentScreenMoveLowerOnSuitsMisc(2 * HI8(pausemenu_equipment_category_item)) == 0xFFFF)) {
      EquipmentScreenMoveToHighJumpOrLowerInBoots(0, r18);
    }
  }
}

static void EquipmentScreenCategory_Suit(void) {  // 0x82B0C2
  EquipmentScreenCategory_Suit_MoveResponse();
  if ((uint8)pausemenu_equipment_category_item == 2)
    EquipmentScreenCategory_ButtonResponse(18);
}

static void EquipmentScreenCategory_Boots_MoveResponse(void) {  // 0x82B160
  uint16 r18 = pausemenu_equipment_category_item;
  if ((joypad1_newkeys & kButton_Left) != 0) {
    if (((joypad1_newkeys & kButton_Up) != 0 || EquipmentScreenMoveToBottomOfBeams(8) == 0xFFFF)
        && !EquipmentScreenMoveToReserveTanks()) {
      pausemenu_equipment_category_item = r18;
    }
  } else if ((joypad1_newkeys & kButton_Down) != 0) {
    if (pausemenu_equipment_category_item != 515) {
      pausemenu_equipment_category_item += 256;
      EquipmentScreenMoveToHighJumpOrLowerInBoots(2 * HI8(pausemenu_equipment_category_item), r18);
    }
  } else if ((joypad1_newkeys & kButton_Up) != 0) {
    if ((pausemenu_equipment_category_item & 0xFF00) == 0
        || (pausemenu_equipment_category_item -= 256,
            EquipmentScreenCategory_Boots_MoveUpInBoots(2 * HI8(pausemenu_equipment_category_item)) == 0xFFFF)) {
      EquipmentScreenMoveToScrewOrHigherOnSuits(0xA, r18);
    }
  }
}

static void EquipmentScreenCategory_Boots(void) {  // 0x82B150
  EquipmentScreenCategory_Boots_MoveResponse();
  if ((uint8)pausemenu_equipment_category_item == 3)
    EquipmentScreenCategory_ButtonResponse(18);
}

static void WriteSamusWireframeTilemapAndQueue(void) {  // 0x82B1E0
  WriteSamusWireframeTilemap();
  /* cola: 1280 bytes de 7E:3900 a la VRAM 0x3080 (el BG1 del equipo): la Genesis copia lo que cambió (md_pause.c) */
}

static void EquipmentScreenDrawItemSelector(void) {  // 0x82B267
  if (samus_max_reserve_health | (uint16)(collected_items | collected_beams)) {
    uint16 t = B82W(kEquipmentScreenPtrsToItemXYpos + 2 * (uint8)pausemenu_equipment_category_item)
               + 4 * HI8(pausemenu_equipment_category_item);
    DrawPauseScreenSpriteAnim(3, B82W(t) - 1, B82W(t + 2));
  }
}

static const uint16 kEquipmentScreenReserveTank_X[6] = { 0x18, 0x20, 0x28, 0x30, 0x38, 0x40 };
static const uint16 kEquipmentScreenReserveTank_Y = 0x60;
static const uint16 kPartialReserveTankSpritemapIds[16] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27 };

static uint16 EquipmentScreenDisplayReserves_PaletteSetup(void) {  // 0x82B3F9
  if (samus_reserve_health) {
    bool v0 = (--pausemenu_reserve_tank_animation_timer & 0x8000) != 0;
    if (!pausemenu_reserve_tank_animation_timer || v0) {
      ++pausemenu_reserve_tank_animation_frame;
      uint16 v1 = B82B(kPauseReserveTankAnimationData + 2 * pausemenu_reserve_tank_animation_frame);
      if (v1 == 255) {
        pausemenu_reserve_tank_animation_frame = 0;
        v1 = B82B(kPauseReserveTankAnimationData);
      }
      pausemenu_reserve_tank_animation_timer = v1;
    }
    return B82W(kPAuseSpritePaletteIndexValues + 6);
  } else {
    return 1536;
  }
}

static void EquipmentScreenDisplayReserveTankAmount(void) {  // 0x82B2A2
  uint16 r3 = EquipmentScreenDisplayReserves_PaletteSetup();
  int r52 = 0;
  if (!samus_max_reserve_health)
    return;
  uint16 R44 = samus_max_reserve_health / 100;
  int mod_value = samus_reserve_health % 100;
  uint16 R50 = mod_value;
  int r42 = samus_reserve_health / 100;
  uint16 R48 = r42;
  int r46 = r42;
  if (r42) {
    uint16 v0 = 0;
    do {
      DrawMenuSpritemap(0x1B, kEquipmentScreenReserveTank_X[v0 >> 1], kEquipmentScreenReserveTank_Y - 1, r3);
      v0 += 2;
    } while (--r46);
    r52 = v0;
  }
  uint16 RegWord = mod_value;
  if (RegWord) {
    uint16 v2 = 2 * (RegWord / 14);
    if (sign16(v2 - 7) && RegWord % 14 && (nmi_frame_counter_byte & 4) == 0)
      v2 += 2;
    if (!sign16(samus_reserve_health - 100))
      v2 += 16;
    DrawMenuSpritemap(kPartialReserveTankSpritemapIds[v2 >> 1], kEquipmentScreenReserveTank_X[r52 >> 1],
                      kEquipmentScreenReserveTank_Y - 1, r3);
    ++R48;
    r52 += 2;
  }
  while (sign16(R48 - R44)) {
    DrawMenuSpritemap(0x20, kEquipmentScreenReserveTank_X[r52 >> 1], kEquipmentScreenReserveTank_Y - 1, r3);
    r52 += 2;
    ++R48;
  }
  DrawMenuSpritemap(0x1F, kEquipmentScreenReserveTank_X[r52 >> 1], kEquipmentScreenReserveTank_Y - 1, r3);
  BG3800[394] = R50 % 10 + 2052;
  BG3800[393] = R50 / 10 + 2052;
  BG3800[392] = r42 + 2052;
}

static void EquipmentScreenMain(void) {  // 0x82AC4F
  switch ((uint8)pausemenu_equipment_category_item) {
  case 0: EquipmentScreenCategory_Tanks(); break;
  case 1: EquipmentScreenCategory_Weapons(); break;
  case 2: EquipmentScreenCategory_Suit(); break;
  case 3: EquipmentScreenCategory_Boots(); break;
  }
  EquipmentScreenDrawItemSelector();
  EquipmentScreenDisplayReserveTankAmount();
  WriteSamusWireframeTilemapAndQueue();
}

static void EquipmentScreenSetupReserveMode(void) {  // 0x82AB47
  reg_BG4HOFS = reg_BG1HOFS;
  reg_BG4VOFS = reg_BG1VOFS;
  reg_BG1HOFS = 0;
  reg_BG1VOFS = 0;
  if (samus_max_reserve_health && reserve_health_mode) {
    uint16 v0 = addr_kEquipmentScreenTilemap_AUTO;
    if (reserve_health_mode != 1)
      v0 = addr_kEquipmentScreenTilemap_MANUAL;
    for (int i = 0; i < 4; i++)
      BG3800[i + 327] = (BG3800[i + 327] & 0xFC00) | B82W(v0 + 2 * i);
  }
  pausemenu_item_selector_animation_frame = 0;
  pausemenu_item_selector_animation_timer = B82B(kPauseLrHighlightAnimData);
  pausemenu_reserve_tank_animation_frame = 0;
  pausemenu_reserve_tank_animation_timer = B82B(kPauseReserveTankAnimationData);
  if (samus_max_reserve_health) {
    pausemenu_equipment_category_item = 0;
  } else if (hyper_beam_flag) {
LABEL_15:;
    uint16 v6 = 0;
    do {
      if ((B82W(kEquipmentBitmasks_Suits + v6) & collected_items) != 0) {
        pausemenu_equipment_category_item = (uint16)((v6 >> 1) << 8) | 2;
        goto LABEL_21;
      }
      v6 += 2;
    } while ((int16)(v6 - 12) < 0);
  } else {
    uint16 v4 = 0;
    while ((B82W(kEquipmentBitmasks_Weapons + v4) & collected_beams) == 0) {
      v4 += 2;
      if ((int16)(v4 - 10) >= 0)
        goto LABEL_15;
    }
    pausemenu_equipment_category_item = (uint16)((v4 >> 1) << 8) | 1;
  }
LABEL_21:
  if (samus_reserve_health) {
    EquipmentScreenGlowingArrowSolidOn();
    WriteSamusWireframeTilemapAndQueue();
  }
}

static void EquipmentScreenTransferBG1Tilemap(void) {  // 0x82AC22
  PauseGfx(PAUSE_GFX_EQUIPMENT);                       /* 7E:3800 -> VRAM 0x3000 */
  reg_BG1VOFS = 0;
}

/* ------------------------------------------------------------------ el menú (estado 15) */
static void DrawPauseMenuDuringFadeIn(void) {  // 0x82B62B
  if (menu_index) {
    EquipmentScreenDrawItemSelector();
    EquipmentScreenDisplayReserveTankAmount();
  } else {
    MapScreenDrawSamusPositionIndicator();
    DrawMapIcons();
    DisplayMapElevatorDestinations();
  }
}

static void PauseMenu_0_MapScreen(void) {  // 0x829120
  HandlePauseScreenLR();
  HandlePauseScreenStart();
  HandleMapScrollArrows();
  MapScrolling();
  MapScreenDrawSamusPositionIndicator();
  DrawMapIcons();
  DisplayMapElevatorDestinations();
  pause_screen_mode = 0;
}

static void PauseMenu_1_EquipmentScreen(void) {  // 0x829142
  reg_BG1HOFS = 0;
  reg_BG1VOFS = 0;
  EquipmentScreenMain();
  HandlePauseScreenLR();
  HandlePauseScreenStart();
  pause_screen_mode = 1;
}

static void PauseMenu_2(void) {  // 0x829156
  DisplayMapElevatorDestinations();
  MapScreenDrawSamusPositionIndicator();
  DrawMapIcons();
  HandlePauseMenuLRPressHighlight();
  pause_screen_mode = 0;
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    EnableNMI();
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    ++menu_index;
  }
}

static void PauseMenu_5(void) {  // 0x829186
  EquipmentScreenDrawItemSelector();
  EquipmentScreenDisplayReserveTankAmount();
  HandlePauseMenuLRPressHighlight();
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    EnableNMI();
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    ++menu_index;
  }
}

static void PauseMenu_3_MapToEquipment_Load(void) {  // 0x8291AB
  DisplayMapElevatorDestinations();
  EquipmentScreenSetupReserveMode();
  EquipmentScreenTransferBG1Tilemap();
  pause_screen_mode = 1;
  SetPauseScreenButtonLabelPalettes();
  pausemenu_lr_animation_frame = 0;
  pausemenu_lr_animation_timer = B82W(kPauseLrHighlightAnimData);
  screen_fade_delay = 1;
  screen_fade_counter = 1;
  ++menu_index;
}

static void PauseMenu_6_EquipmentToMap_Load(void) {  // 0x8291D7
  DisplayMapElevatorDestinations();
  LoadPauseMenuMapTilemapAndAreaLabel();
  SetPauseScreenButtonLabelPalettes();
  pausemenu_lr_animation_frame = 0;
  pausemenu_lr_animation_timer = B82W(kPauseLrHighlightAnimData);
  screen_fade_delay = 1;
  screen_fade_counter = 1;
  pause_screen_mode = 0;
  ++menu_index;
}

static void PauseMenu_7_EquipmentToMap_FadeIn(void) {  // 0x829200
  MapScreenDrawSamusPositionIndicator();
  DrawMapIcons();
  DisplayMapElevatorDestinations();
  pause_screen_mode = 0;
  HandleFadeIn();
  if (reg_INIDISP == 15) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    menu_index = pausemenu_button_label_mode ? 1 : 0;
  }
}

static void PauseMenu_4_MapToEquipment_FadeIn(void) {  // 0x829231
  EquipmentScreenDrawItemSelector();
  EquipmentScreenDisplayReserveTankAmount();
  pause_screen_mode = 1;
  HandleFadeIn();
  if (reg_INIDISP == 15) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    menu_index = pausemenu_button_label_mode ? 1 : 0;
  }
}

static void MainPauseRoutine(void) {  // 0x8290FF (kPauseMenuFuncs)
  switch (menu_index) {
  case 0: PauseMenu_0_MapScreen(); break;
  case 1: PauseMenu_1_EquipmentScreen(); break;
  case 2: PauseMenu_2(); break;
  case 3: PauseMenu_3_MapToEquipment_Load(); break;
  case 4: PauseMenu_4_MapToEquipment_FadeIn(); break;
  case 5: PauseMenu_5(); break;
  case 6: PauseMenu_6_EquipmentToMap_Load(); break;
  case 7: PauseMenu_7_EquipmentToMap_FadeIn(); break;
  default: nt_unimpl("MainPauseRoutine (menu_index fuera de la tabla)"); break;
  }
}

static uint8 GameState_14_Paused_Async(void) {  // 0x8290C8
  DrawPauseMenuDuringFadeIn();
  HandleFadeIn();
  if (reg_INIDISP == 15) {
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    ++game_state;
  }
  return 0;
}

static uint8 GameState_15_Paused_Async(void) {  // 0x8290E8
  ReleaseButtonsFilter(3);
  MainPauseRoutine();
  HandleHudTilemap();
  HandlePauseScreenPaletteAnimation();
  return 0;
}

/* ------------------------------------------------------------------ salir de la pausa (estados 16 y 17) */
static void DrawPauseMenuDuringFadeout(void) {  // 0x82934B
  if (pause_screen_mode == 1) {
    EquipmentScreenDrawItemSelector();
    EquipmentScreenDisplayReserveTankAmount();
    HandlePauseMenuLRPressHighlight();
  } else {
    DisplayMapElevatorDestinations();
    DrawMapIcons();
    MapScreenDrawSamusPositionIndicator();
  }
}

static uint8 GameState_16_Unpausing_Async(void) {  // 0x829324
  HighlightPauseScreenButton();
  DrawPauseMenuDuringFadeout();
  HandleFadeOut();
  if (reg_INIDISP == 0x80) {
    EnableNMI();
    screen_fade_delay = 0;
    screen_fade_counter = 0;
    ++game_state;
  }
  return 0;
}

static void SetupPpuForGameplayResume(void) {  // 0x82A313
  reg_OBSEL = 3;
  reg_BGMODE = 9;
  reg_BG12NBA = 0;
  reg_BG34NBA = 4;
  reg_BG1SC = 81;
  reg_BG2SC = 73;
  reg_BG3SC = 90;
  reg_BG4SC = 0;
}

static void CalculateBgScrolls_Unpause(void) {  // 0x82A34E
  reg_BG1HOFS = bg1_x_offset + layer1_x_pos;
  reg_BG1VOFS = bg1_y_offset + layer1_y_pos;
  reg_BG2HOFS = bg2_x_scroll + layer2_x_pos;
  reg_BG2VOFS = bg2_y_scroll + layer2_y_pos;
}

static void ClearPauseMenuData(void) {  // 0x82A380
  menu_index = 0;
  pausemenu_start_lr_pressed_highlight_timer = 0;
  pausemenu_lr_animation_timer = 0;
  pausemenu_item_selector_animation_timer = 0;
  pausemenu_reserve_tank_animation_timer = 0;
  UNUSED_word_7E0731 = 0;
  pausemenu_uparrow_animation_timer = 0;
  pausemenu_downarrow_animation_timer = 0;
  pausemenu_rightarrow_animation_timer = 0;
  pausemenu_leftarrow_animation_timer = 0;
  UNUSED_word_7E073D = 0;
  pausemenu_lr_animation_frame = 0;
  pausemenu_item_selector_animation_frame = 0;
  pausemenu_reserve_tank_animation_frame = 0;
  UNUSED_word_7E0745 = 0;
  pausemenu_uparrow_animation_frame = 0;
  pausemenu_downarrow_animation_frame = 0;
  pausemenu_rightarrow_animation_frame = 0;
  pausemenu_leftarrow_animation_frame = 0;
  pausemenu_shoulder_button_highlight = 0;
  pausemenu_button_label_mode = 0;
  pausemenu_equipment_category_item = 0;
  pausemenu_reserve_tank_delay_ctr = 0;
  UNUSED_word_7E0759 = 0;
  UNUSED_word_7E075B = 0;
  UNUSED_word_7E075D = 0;
  UNUSED_word_7E075F = 0;
  UNUSED_word_7E0761 = 0;
}

static void ContinueInitGameplayResume(void) {  // 0x82A2E3
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = PMT[i + 384];
  nt_pal_dirty(0, 255);
  SetupPpuForGameplayResume();
  CalculateBgScrolls_Unpause();
  UpdateBeamTilesAndPalette();                          /* UpdateBeamTilesAndPalette_Unpause ($82:A377) */
  ClearPauseMenuData();
  CallSomeSamusCode(0xC);
}

static uint8 GameState_17_Unpausing_Async(void) {  // 0x829367
  /* ClearSamusBeamTiles ($82:A2BE): tiles de la VRAM; la Genesis rehace la sala entera (ResumeGameplay) */
  ContinueInitGameplayResume();
  ResumeGameplay();
  RestoreSomeGfxStateForPause();
  /* RestoreBG2TilemapFromPauseScreen ($82:8D96): la VRAM, la Genesis */
  screen_fade_delay = 1;
  screen_fade_counter = 1;
  /* CallUnpauseHook_Async(unpause_hook): UnpauseHook vacío fuera de Kraid, Draygon y Mother Brain (sin portar) */
  EnableHdmaObjects();
  EnableAnimtiles_();
  QueueSamusMovementSfx();
  ++game_state;
  return 0;
}

/* lo que sigue a GameState_8_MainGameplay en los estados 12 y 18 (game.c llama antes a GameState_8_MainGameplay) */
uint8 PauseFadeStates(uint16 st) {
  PauseGfx(PAUSE_GFX_FADE);                             /* (Genesis: el brillo de INIDISP en estos cuadros) */
  if (st == 12) {                                       // GameState_12_Pausing_Darkening_Async ($82:8CCF)
    HandleFadeOut();
    if ((reg_INIDISP & 0xF) == 0) {
      EnableNMI();
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      ++game_state;
    }
  } else {                                              // GameState_18_Unpausing ($82:93A1)
    HandleFadeIn();
    if (reg_INIDISP == 15) {
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      game_state = 8;
    }
  }
  return 0;
}

uint8 PauseStates(uint16 st) {
  switch (st) {
  case 13: return GameState_13_Pausing_Async();
  case 14: return GameState_14_Paused_Async();
  case 15: return GameState_15_Paused_Async();
  case 16: return GameState_16_Unpausing_Async();
  case 17: return GameState_17_Unpausing_Async();
  }
  return 0;
}
