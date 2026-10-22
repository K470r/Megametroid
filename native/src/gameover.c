/* MegaMetroid — motor nativo: fin de juego (estados 25-26; $82:DDC7 y el menú de $81:8D0F; docs/35).
 *
 * Después de que Ceres explota con Samus adentro: fundido a negro (25) y el menú "GAME OVER / FIND THE METROID
 * LARVA! / TRY AGAIN? / YES / NO" (26) con la bebé metroide y el misil que marca la opción.
 * Lo que va a la VRAM (tiles de los menús, el tilemap del BG1 en 7E:3600) lo dibuja la Genesis: MenuGfx() avisa
 * que se cargan los gráficos del menú, y DrawMenuSpritemap() dibuja los spritemaps de los menús ($82).
 * Las salidas: YES vuelve al juego cargando la partida (estado 31 en Ceres) y NO reinicia la consola; la Genesis las
 * lleva al comienzo de Ceres hasta que estén el título y las partidas guardadas. */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "hdmaobj.h"
#include "enemies.h"

void MenuGfx(void);                                                   /* capa de la Genesis */
void DrawMenuSpritemap(uint16 idx, uint16 x, uint16 y, uint16 chr);   /* $81:891F (Genesis) */

static void HandleFadeInMenu(void) {  // 0x80894D
  if ((int16)(screen_fade_counter - 1) < 0) {
    screen_fade_counter = screen_fade_delay;
    if (((reg_INIDISP + 1) & 0xF) != 0)
      reg_INIDISP = (reg_INIDISP + 1) & 0xF;
  } else {
    --screen_fade_counter;
  }
}

/* ------------------------------------------------------------------ la bebé y el misil ($82:BA6E, $82:BB75) */
static const uint16 kDrawMenuSelectionMissile_Enable[4] = { 8, 8, 8, 8 };
static const uint16 kDrawMenuSelectionMissile_SpriteMap[4] = { 0x37, 0x36, 0x35, 0x34 };

static void DrawMenuSelectionMissile(void) {  // 0x82BA6E
  if (eproj_enable_flag) {
    if (!--eproj_enable_flag) {
      eproj_id[0] = ((uint8)eproj_id[0] + 1) & 3;
      eproj_enable_flag = kDrawMenuSelectionMissile_Enable[eproj_id[0]];
    }
  }
  DrawMenuSpritemap(kDrawMenuSelectionMissile_SpriteMap[eproj_id[0]], eproj_id[5], eproj_id[10], 3584);
}

static void DrawBabyMetroid(uint16 k) {  // 0x82BB9E: paleta del cuadro y dos spritemaps
  uint16 pal = B82W(k + 4);
  for (int i = 0; i < 16; i++)
    palette_buffer[192 + i] = B82W(pal + 2 * i);
  nt_pal_dirty(192, 207);
  DrawMenuSpritemap(B82W(k + 2), 0x7C, 0x50, 2048);
  DrawMenuSpritemap(0x64, 0x7C, 0x50, 2560);
}

static void GameOverBabyRestart(void);

static void FinishProcessingGameOverBabyMetroidAsm(void) {  // 0x82BBF0
  uint16 t = B82W(enemy_data[0].current_instruction + 8);
  enemy_data[0].instruction_timer = t;
  enemy_data[0].current_instruction += 8;
  if (t == 0xFFFF)
    GameOverBabyRestart();
  else
    DrawBabyMetroid(enemy_data[0].current_instruction);
}

static void GameOverBabyStep(uint16 a) {  // 0x82BB7F
  uint16 ci = enemy_data[0].current_instruction;
  enemy_data[0].instruction_timer = a - 1;
  if (a == 1) {
    uint16 nx = B82W(ci + 6);
    if (nx == 0xFFFF) {
      GameOverBabyRestart();
    } else if ((int16)(nx + 1) >= 0) {
      enemy_data[0].instruction_timer = nx;
      enemy_data[0].current_instruction += 6;
      DrawBabyMetroid(ci + 6);
    } else {
      switch (nx) {                               /* BabyMetroidPlaySfx0x23/26/27 */
      case FUNC16(BabyMetroidPlaySfx0x23): QueueSfx3_Max6(0x23); break;
      case FUNC16(BabyMetroidPlaySfx0x26): QueueSfx3_Max6(0x26); break;
      case FUNC16(BabyMetroidPlaySfx0x27): QueueSfx3_Max6(0x27); break;
      default: nt_unimpl("GameOverBabyStep (función sin portar)"); return;
      }
      FinishProcessingGameOverBabyMetroidAsm();
    }
  } else {
    DrawBabyMetroid(ci);
  }
}

static void GameOverBabyRestart(void) {  // 0x82BBDD
  enemy_data[0].current_instruction = addr_kgameOverBabyMetridInstructionList;
  enemy_data[0].instruction_timer = 10;
  GameOverBabyStep(10);
}

static void HandleGameOverBabyMetroid(void) {  // 0x82BB75
  if (enemy_data[0].instruction_timer)
    GameOverBabyStep(enemy_data[0].instruction_timer);
  else
    GameOverBabyRestart();
}

/* ------------------------------------------------------------------ menú */
static void LoadMenuTilemap(uint16 k, uint16 j) {  // 0x81B3E2: k en bytes desde 7E:3600, j en $81
  uint16 kb = k;
  for (;;) {
    uint16 w;
    while ((w = B81W(j)) == 0xFFFE) {
      j += 2;
      k = (kb += 64);
    }
    if (w == 0xFFFF)
      break;
    menu_tilemap[k >> 1] = enemy_data[0].palette_index | w;
    k += 2;
    j += 2;
  }
}

static void GameOverMenu_0_FadeOutConfigGfx(void) {  // 0x818D0F
  HandleFadeOut();
  if (!(reg_INIDISP & 0xF)) {
    reg_INIDISP |= 0x80;                          // ScreenOff
    QueueSfx3_Max6(1);
    DisableHdmaObjects();
    WaitUntilEndOfVblankAndClearHdma();
    /* copias de la paleta y de los registros en 7E:3300 (para volver del mapa): sin portar, no se usan aquí */
    reg_TS = 0; reg_OBSEL = 3;                    // MapVramForMenu
    reg_BG1SC = 81; reg_BG2SC = 88; reg_BG3SC = 92;
    reg_BG12NBA = 0; reg_BG34NBA = 4; reg_TM = 19;
    MenuGfx();                                    // LoadInitialMenuTiles
    reg_BG1HOFS = 0; reg_BG2HOFS = 0; reg_BG3HOFS = 0;
    reg_BG1VOFS = 0; reg_BG2VOFS = 0; reg_BG3VOFS = 0;
    for (int i = 0; i < 256; i++)                 // LoadMenuPalettes
      palette_buffer[i] = kMenuPalettes[i];
    nt_pal_dirty(0, 255);
    ++menu_index;
  }
}

static void GameOverMenu_1_Init(void) {  // 0x8191A4
  reg_TM = 17;
  reg_COLDATA[0] = 32; reg_COLDATA[1] = 64; reg_COLDATA[2] = 0x80;
  QueueMusic_Delayed8(0);
  QueueMusic_Delayed8(0xFF03);
  eproj_enable_flag = 1;
  eproj_id[0] = 0;
  for (int i = 0; i < 1024; i++)
    menu_tilemap[i] = 15;
  enemy_data[0].palette_index = 0;
  LoadMenuTilemap(0x156, addr_kMenuTilemap_GameOver);
  LoadMenuTilemap(0x38A, addr_kMenuTilemap_FindMetroidLarva);
  LoadMenuTilemap(0x414, addr_kMenuTilemap_TryAgain);
  LoadMenuTilemap(0x4CE, addr_kMenuTilemap_YesReturnToGame);
  LoadMenuTilemap(0x5CE, addr_kMenuTilemap_NoGoToTitle);
  enemy_data[0].current_instruction = 0;
  enemy_data[0].instruction_timer = 0;
  HandleGameOverBabyMetroid();
  reg_CGWSEL = 0;                                 // *(uint16 *)&reg_CGWSEL = gameplay_CGWSEL << 8 (7E:6F-70)
  DisableHdmaObjects();
  WaitUntilEndOfVblankAndClearHdma();
  EnableHdmaObjects();
  SpawnHdmaObject(0x81, 0x927D);                  /* degradado de fondo (la Genesis: el color de fondo por línea) */
  SpawnHdmaObject(0x81, 0x928D);
  reg_INIDISP &= ~0x80;                           // ScreenOn
  ++menu_index;
  screen_fade_delay = 0;
  screen_fade_counter = 0;
  file_select_map_area_index = 0;
  eproj_id[5] = 40;
  eproj_id[10] = 160;
}

static void GameOverMenu_4_Main(void) {  // 0x81912B
  HandleGameOverBabyMetroid();
  DrawMenuSelectionMissile();
  if (joypad1_newkeys & (kButton_Select | kButton_Up | kButton_Down)) {
    QueueSfx1_Max6(0x37);
    file_select_map_area_index ^= 1;
  } else if (joypad1_newkeys & kButton_A) {
    enemy_data[0].instruction_timer = 180;
    if (file_select_map_area_index) {
      menu_index = 7;
    } else {
      if (loading_game_state == 31)
        game_state = loading_game_state;          /* Ceres: se empieza de nuevo (estado 31) */
      else
        ++menu_index;
      /* LoadFromSram(selected_save_slot): la Genesis vuelve al comienzo de Ceres (sin partidas guardadas todavía) */
    }
    return;
  }
  eproj_id[5] = 40;
  eproj_id[10] = file_select_map_area_index ? 192 : 160;
}

static void GameOverMenu(void) {
  switch (menu_index) {
  case 0: GameOverMenu_0_FadeOutConfigGfx(); break;
  case 1: GameOverMenu_1_Init(); break;
  case 2:                                         // GameOverMenu_2_PlayMusic
    if (!HasQueuedMusic()) {
      ++menu_index;
      QueueMusic_Delayed8(4);
    }
    break;
  case 3:                                         // GameOverMenu_3_FadeIn
    HandleGameOverBabyMetroid();
    DrawMenuSelectionMissile();
    HandleFadeInMenu();
    if ((reg_INIDISP & 0xF) == 15)
      ++menu_index;
    break;
  case 4: GameOverMenu_4_Main(); break;
  case 5:                                         // GameOverMenu_5_FadeOutToGameMap
    HandleGameOverBabyMetroid();
    DrawMenuSelectionMissile();
    HandleFadeOut();
    if (!(reg_INIDISP & 0xF))
      ++menu_index;
    break;
  case 7:                                         // GameOverMenu_7_FadeOutToSoftReset
    DrawMenuSelectionMissile();
    HandleGameOverBabyMetroid();
    HandleFadeOut();
    if (!(reg_INIDISP & 0xF))
      game_state = 0xFFFF;                        /* SoftReset: la Genesis reinicia (sin título todavía) */
    break;
  default: nt_unimpl("GameOverMenu (paso sin portar: mapa de la partida)"); break;
  }
}

uint8 GameOverStates(uint16 st) {
  if (st == 25) {                                 // GameState_25_SamusNoHealth_BlackOut ($82:DDC7)
    HandleFadeOut();
    if (reg_INIDISP == 0x80) {
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      ++game_state;
      menu_index = 0;
      debug_disable_sounds = 0;
    }
  } else {
    GameOverMenu();
  }
  return 0;
}
