/* MegaMetroid — motor nativo: empezar la partida en una estación de carga (estados 6 y 7; docs/37).
 *
 * Después de la huida de Ceres el juego pasa al estado 6 (InitAndLoadGameData, $82:8000): Zebes, estación 18 del
 * área 0 (Landing Site). Lo mismo sirve para cargar cualquier partida. Lo que en el SNES va a la VRAM (tiles del
 * BG3 y de los sprites, tilemaps, la sala visible) lo hace la Genesis con la sala ya convertida (g_hooks.room_gfx).
 * Las esperas de cuadros son corrutinas como en el decomp: coroutine_state_1 (esta), _2 (StartGameplay) y _3
 * (Play20FramesOfMusic). */
#include "samus.h"
#include "game.h"
#include "plm.h"
#include "nmath.h"
#include "room.h"
#include "enemies.h"
#include "hdmaobj.h"

void ClearPaletteFXObjects(void);
void EnablePaletteFx(void);
void InitializeHud(void);

static void EnableNMI(void) { reg_NMITIMEN |= 0x80; }                  // 0x80834B
static void DisableNMI(void) { reg_NMITIMEN &= 0x7F; }                 // 0x80835D
static void EnableIrqInterrupts(void) { reg_NMITIMEN |= 0x30; }        // 0x80982A
static void DisableIrqInterrupts(void) { reg_NMITIMEN &= ~0x30; }      // 0x80985F

static void ResetSoundQueues(void) {  // 0x828A9A
  sfx_readpos[0] = sfx_readpos[1] = sfx_readpos[2] = 0;
  sfx_writepos[1] = sfx_writepos[2] = 0;
  sfx_state[0] = sfx_state[1] = sfx_state[2] = 0;
}

static void ClearTimerRam(void) {  // 0x809E93
  timer_x_pos = 0x8000;
  timer_y_pos = 0x8000;
  timer_centiseconds = timer_minutes = 0;
  timer_seconds = 0;
  timer_status = 0;
}

void SaveExploredMapTilesToSaved(void) {  // 0x8085C6
  uint16 base = area_index << 7;
  for (int i = 0; i < 128; i++)
    explored_map_tiles_saved[base + i] = map_tiles_explored[2 * i] | map_tiles_explored[2 * i + 1] << 8;
}

void SetElevatorsAsUsed(void) {  // 0x80CD07: kElevatorUsedBits = $80:CD46
  uint16 p = kElevatorUsedBits[2 * area_index] | kElevatorUsedBits[2 * area_index + 1] << 8;
  if (p < 0xCD46) return;   /* Ceres y Debug no tienen tabla (el SNES lee basura de la RAM baja) */
  const uint8 *v = &kElevatorUsedBits[p - 0xCD46 + 4 * ((elevator_door_properties_orientation & 0xF) - 1)];
  used_save_stations_and_elevators[v[0]] |= v[1];
  used_save_stations_and_elevators[v[2]] |= v[3];
}

void LoadMirrorOfExploredMapTiles(void) {  // 0x80858C: el mapa explorado del área (copia de la partida)
  uint16 base = area_index << 7;
  for (int i = 0; i < 128; i++) {
    uint16 w = explored_map_tiles_saved[base + i];
    map_tiles_explored[2 * i] = (uint8)w;
    map_tiles_explored[2 * i + 1] = (uint8)(w >> 8);
  }
  has_area_map = map_station_byte_array[area_index];
}

static void InitPpuForGameplay(void) {  // 0x8281DD (los registros del PPU que el juego guarda en la RAM)
  reg_INIDISP = 0x80;
  reg_OBSEL = 3;
  reg_BGMODE = 9;
  reg_MOSAIC = 0;
  reg_BG12NBA = 0;
  reg_BG34NBA = 4;
  reg_BG1SC = 81;
  reg_BG2SC = 73;
  reg_BG3SC = 90;
  reg_BG4SC = 0;
  reg_W12SEL = 0;
  reg_W34SEL = 0;
  reg_WOBJSEL = 0;
  reg_WH0 = reg_WH1 = reg_WH2 = reg_WH3 = 0;
  reg_WBGLOG = 0;
  reg_WOBJLOG = 0;
  reg_TM = 23;
  reg_TMW = 0;
  reg_TS = 0;
  reg_TSW = 0;
  next_gameplay_CGWSEL = 0;
  next_gameplay_CGADSUB = 0;
  reg_SETINI = 0;
  oam_next_ptr = 0;
  /* 7E:3000 y 7E:4000 (tilemaps en la RAM): los arma la Genesis */
}

static void InitIoForGameplay(void) {  // 0x82819B
  reg_NMITIMEN = 1;                       // InitCpuForGameplay
  reg_HDMAEN = 0;
  reg_MEMSEL = 1;
  InitPpuForGameplay();
}

static void LoadInitialPalette(void) {  // 0x8282C5
  for (int i = 0; i < 256; i++)
    palette_buffer[i] = kInitialPalette[i];
  nt_pal_dirty(0, 255);
}

static const LoadStationN *load_station(uint16 area, uint16 index) {
  for (unsigned i = 0; i < sizeof kLoadStations / sizeof kLoadStations[0]; i++)
    if (kLoadStations[i].area == area && kLoadStations[i].index == index) return &kLoadStations[i];
  nt_unimpl("load_station (estación de carga sin preparar)");
  return &kLoadStations[0];
}

static void LoadFromLoadStation(void) {  // 0x80C437
  save_station_lockout_flag = 1;
  const LoadStationN *L = load_station(area_index, load_station_index);
  room_ptr = L->room_ptr;
  door_def_ptr = L->door_ptr;
  bg1_x_offset = layer1_x_pos = L->screen_x_pos;
  bg1_y_offset = layer1_y_pos = L->screen_y_pos;
  samus_y_pos = layer1_y_pos + L->samus_y_offset;
  samus_prev_y_pos = samus_y_pos;
  samus_x_pos = layer1_x_pos + 128 + L->samus_x_offset;
  samus_prev_x_pos = samus_x_pos;
  reg_BG1HOFS = 0;
  reg_BG1VOFS = 0;
  area_index = (area_index & 0xFF00) | room_def(room_ptr)->area_index;
  debug_disable_minimap &= 0xFF00;
}

static void LoadCRETilesTilesetTilesAndPalette(void) {  // 0x82E783
  elevator_flags = 0;
  /* tiles de la CRE y del tileset: la Genesis (g_hooks.room_gfx); la paleta del tileset a 7E:C200 */
  uint8 gs = room_state(roomdefroomstate_ptr)->graphics_set;
  unsigned i = 0;
  while (i < sizeof kTilesetPaletteSets && kTilesetPaletteSets[i] != gs) i++;
  if (i == sizeof kTilesetPaletteSets) { nt_unimpl("LoadCRETilesTilesetTilesAndPalette (paleta de tileset sin preparar)"); return; }
  for (int c = 0; c < 128; c++) target_palettes[c] = kTilesetPalettes[i * 128 + c];
}

static void LoadDestinationRoomThings(void) {  // 0x82E76B
  LoaadDesinationRoomCreBitset();
  LoadDoorHeader();
  LoadRoomHeader();
  LoadStateHeader();
  LoadCRETilesTilesetTilesAndPalette();
}

static uint8 Play20FramesOfMusic_Async(void) {  // 0x80A12B
  switch (coroutine_state_3) {
  case 0:
    EnableNMI();
    for (my_counter = 0; my_counter != 20; my_counter++) {
      HandleMusicQueue();
  case 1:
      if (WaitForNMI_Async()) { coroutine_state_3 = 1; return 1; }
    }
    DisableNMI();
  }
  coroutine_state_3 = 0;
  return 0;
}

static uint8 CalculateLayer2Xpos(void) {  // 0x80A2F9
  if (!layer2_scroll_x) { layer2_x_pos = layer1_x_pos; return 0; }
  if (layer2_scroll_x != 1) {
    uint16 t = layer2_scroll_x & 0xFE;
    uint8 frac = (uint8)((t * (uint8)layer1_x_pos) >> 8);
    layer2_x_pos = t * HI8(layer1_x_pos) + frac;
    return 0;
  }
  return 1;
}

static uint8 CalculateLayer2Ypos(void) {  // 0x80A33A
  if (!layer2_scroll_y) { layer2_y_pos = layer1_y_pos; return 0; }
  if (layer2_scroll_y != 1) {
    uint16 t = layer2_scroll_y & 0xFE;
    uint8 frac = (uint8)((t * (uint8)layer1_y_pos) >> 8);
    layer2_y_pos = t * HI8(layer1_y_pos) + frac;
    return 0;
  }
  return 1;
}

static uint16 block_of(uint16 pos) {   /* posición >> 4 con el signo de 12 bits */
  uint16 v = pos >> 4;
  if (v & 0x800) v |= 0xF000;
  return v;
}

static void CalculateBgScrollAndLayerPositionBlocks(void) {  // 0x80A4BB
  bg1_x_block = reg_BG1HOFS >> 4;
  bg2_x_block = reg_BG2HOFS >> 4;
  layer1_x_block = block_of(layer1_x_pos);
  layer2_x_block = block_of(layer2_x_pos);
  bg1_y_block = reg_BG1VOFS >> 4;
  bg2_y_block = reg_BG2VOFS >> 4;
  layer1_y_block = block_of(layer1_y_pos);
  layer2_y_block = block_of(layer2_y_pos);
}

static void DisplayViewablePartOfRoom(void) {  // 0x80A176 (las columnas visibles: la Genesis dibuja la sala entera)
  size_of_bg2 = ((reg_BG1SC - reg_BG2SC) << 8) & 0xF800;
  CalculateBgScrollAndLayerPositionBlocks();
  for (int i = 0; i <= 16; i++) {
    blocks_to_update_x_block = layer1_x_block;
    blocks_to_update_y_block = layer1_y_block;
    vram_blocks_to_update_x_block = bg1_x_block;
    vram_blocks_to_update_y_block = bg1_y_block;
    if (!(layer2_scroll_x & 1)) {
      blocks_to_update_x_block = layer2_x_block;
      blocks_to_update_y_block = layer2_y_block;
      vram_blocks_to_update_x_block = bg2_x_block;
      vram_blocks_to_update_y_block = bg2_y_block;
    }
    ++layer1_x_block;
    ++bg1_x_block;
    ++layer2_x_block;
    ++bg2_x_block;
  }
}

void LoadRoomPlmGfx(void);   /* plm.c */
NT_LATE void ResumeGameplay(void) {  // 0x80A149 (al salir de la pausa)
  DisableNMI();
  DisableIrqInterrupts();
  LoadCRETilesTilesetTilesAndPalette();
  /* LoadLibraryBackground y los tiles y mapas de la sala en la VRAM: la Genesis (g_hooks.room_gfx) */
  if (g_hooks.room_gfx) g_hooks.room_gfx();
  DisplayViewablePartOfRoom();
  LoadRoomPlmGfx();
  EnableNMI();
  EnableIrqInterrupts();
}

static void UpdateMusicTrackIndex(void) {  // 0x82E09B
  if (game_state < 40 && room_music_track_index) {
    uint16 r18 = room_music_data_index << 8 | room_music_track_index;
    uint16 r20 = music_data_index << 8 | music_track_index;
    if (r18 != r20)
      music_track_index = room_music_track_index;
  }
}

static uint8 StartGameplay_Async(void) {  // 0x80A07B
  switch (coroutine_state_2) {
  case 0:
    scrolling_finished_hook = 0;
    music_data_index = 0;
    music_track_index = 0;
    timer_status = 0;
    ResetSoundQueues();
    debug_disable_sounds = -1;
    DisableNMI();
    DisableIrqInterrupts();
    LoadDestinationRoomThings();
  case 1:
    if (Play20FramesOfMusic_Async()) { coroutine_state_2 = 1; return 1; }
    for (int i = 5; i >= 0; i--) animtiles_ids[i] = 0;        // ClearAnimtiles
    WaitUntilEndOfVblankAndClearHdma();
    InitializeSpecialEffectsForNewRoom();
    for (int i = 39; i >= 0; i--) plm_header_ptr[i] = 0;      // ClearPLMs
    plm_item_gfx_index = 0;
    for (int i = 17; i >= 0; i--) eproj_id[i] = 0;            // ClearEprojs
    ClearPaletteFXObjects();
    UpdateBeamTilesAndPalette();
    LoadColorsForSpritesBeamsAndEnemies();
    LoadEnemies();
    LoadRoomMusic();
  case 2:
    if (Play20FramesOfMusic_Async()) { coroutine_state_2 = 2; return 1; }
    UpdateMusicTrackIndex();
    /* ClearBG2Tilemap: la Genesis */
    LoadLevelScrollAndCre();                                   // LoadLevelDataAndOtherThings
    CreatePlmsExecuteDoorAsmRoomSetup();
    LoadFXHeader();
    /* LoadLibraryBackground: la Genesis (g_hooks.room_gfx) */
    if (g_hooks.room_gfx) g_hooks.room_gfx();
    CalculateLayer2Xpos();
    CalculateLayer2Ypos();
    bg2_x_scroll = layer2_x_pos;
    bg2_y_scroll = layer2_y_pos;
    CalculateBgScrolls();
    DisplayViewablePartOfRoom();
    EnableNMI();
    irqhandler_next_handler = room_loading_irq_handler ? room_loading_irq_handler : 4;
    EnableIrqInterrupts();
  case 3:
    if (Play20FramesOfMusic_Async()) { coroutine_state_2 = 3; return 1; }
    SpawnHardcodedPlm(8, 8, 0xB7EB);
    door_transition_function = 0xE737;                         // DoorTransition_FadeInScreenAndFinish
  }
  coroutine_state_2 = 0;
  return 0;
}

uint8 InitAndLoadGameData_Async(void) {  // 0x828000
  switch (coroutine_state_1) {
  case 0:
    if (loading_game_state != 5) {                              // kLoadingGameState_5_Main
      if (loading_game_state == 0x1F) {                         // kLoadingGameState_1F_StartingAtCeres
        area_index = 6;
        load_station_index = 0;
        ClearTimerRam();
      } else if (loading_game_state == 0x22) {                  // kLoadingGameState_22_EscapingCeres
        area_index = 0;
        load_station_index = 18;
        LoadMirrorOfExploredMapTiles();
      }
    }
    InitIoForGameplay();
    /* LoadStdBG3andSpriteTilesClearTilemaps: la Genesis */
    LoadInitialPalette();
    Samus_Initialize();
    LoadFromLoadStation();
  case 1:
    if (StartGameplay_Async()) { coroutine_state_1 = 1; return 1; }
    InitializeHud();
    for (int i = 192; i < 208; i++)
      target_palettes[i] = palette_buffer[i];
    screen_fade_delay = 1;
    screen_fade_counter = 1;
    EnableNMI();
    eproj_enable_flag |= 0x8000;                                // EnableEprojs
    plm_flag |= 0x8000;                                         // EnablePLMs
    EnablePaletteFx();
    hdma_objects_enable_flag |= 0x8000;                         // EnableHdmaObjects
    animtiles_enable_flag |= 0x8000;                            // EnableAnimtiles
    SetLiquidPhysicsType();
    if (loading_game_state == 0x22) {
      QueueMusic_Delayed8(5);
      loop_counter_transfer_enemies_to_vram = 15;
    } else {
      loop_counter_transfer_enemies_to_vram = 6;
    }
    do {
      TransferEnemyTilesToVramAndInit();
  case 3: case 4:   /* 3: huida de Ceres, 4: lo demás (como los COROUTINE_AWAIT del decomp; 2 es la demo) */
      if (WaitForNMI_Async()) { coroutine_state_1 = loading_game_state == 0x22 ? 3 : 4; return 1; }
      --loop_counter_transfer_enemies_to_vram;
    } while (!(loop_counter_transfer_enemies_to_vram & 0x8000));
    if (loading_game_state == 0x22) {
      ++game_state;
    } else {
      game_state = 7;
    }
    for (int i = 0; i < 256; i++)
      palette_buffer[i] = target_palettes[i];
    nt_pal_dirty(0, 255);
    if (loading_game_state != 0x22) {
      if (loading_game_state == 0x1F) {
        palette_buffer[223] = 0;
        CallSomeSamusCode(8);
      } else {
        CallSomeSamusCode(9);
      }
    }
  }
  coroutine_state_1 = 0;
  return 0;
}
