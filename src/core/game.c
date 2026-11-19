/* MegaMetroid — motor nativo: el cuadro del juego (bancos $80/$82/$A0 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm: RunOneFrameOfGame ($82:8948), GameState_8_MainGameplay ($82:8B44) y lo
 * que el NMI deja en la RAM (mando, contadores). Lo que en el SNES era hardware (OAM, VRAM, HDMA, APU) lo
 * hace la capa de la Genesis (md/) leyendo el estado que deja este cuadro. */
#include "samus.h"
#include "game.h"
#include "plm.h"
#include "animtiles.h"
#include "hdmaobj.h"
#include "enemies.h"

uint32 nt_unimpl_val;

/* sistemas todavía no portados: en el PC (host) se cargan del SNES antes de cada cuadro */
GameHooks g_hooks;
uint8 nt_palette_changed = 1;
uint8 nt_palette_lo = 0, nt_palette_hi = 255;
const uint8 *nt_fade_list;
uint16 nt_fade_count;

static inline uint16 swap16(uint16 v) { return (uint16)(v << 8 | v >> 8); }

uint16 NextRandom(void) {  // 0x808111
  uint16 RegWord = LO8(random_number) * 5;
  uint8 Reg = HI8(random_number) * 5;
  int carry = HI8(RegWord) + Reg + 1;
  RegWord = (uint16)((RegWord & 0xFF) | (uint8)carry << 8);
  uint16 result = (carry >> 8) + RegWord + 17;
  random_number = result;
  return result;
}

static void ReadJoypadInputs(uint16 joy) {  // 0x809459
  joypad1_lastkeys = joy;
  joypad1_newkeys = joy & (joypad1_prev ^ joy);
  joypad1_newkeys2_UNUSED = joy & (joypad1_prev ^ joy);
  if (joy && joy == joypad1_prev) {
    if (!--joypad1_keyrepeat_ctr) {
      joypad1_newkeys2_UNUSED = joypad1_lastkeys;
      joypad1_keyrepeat_ctr = joypad_ctr_repeat_next;
    }
  } else {
    joypad1_keyrepeat_ctr = joypad_ctr_repeat_first;
  }
  joypad1_prev = joypad1_lastkeys;
}

/* los lugares de enemigo hasta el más alto que se llenó desde que se cargó la sala (docs/60): los de más arriba
   están vacíos (enemy_ptr 0) y el recorrido los salta. Lo suben LoadEnemies, SpawnEnemy y RespawnEnemy; lo baja a 0 el
   borrado de enemy_data al cargar la sala. Empieza en todos (un estado cargado de afuera: el inicial, check). */
uint16 enemy_hw = 2048;
void DetermineWhichEnemiesToProcess(void) {  // 0xA08EB6
  ++UNUSED_word_7E0E46;
  uint16 aw = 0, iw = 0;                          /* active/interactive_enemy_indexes_write_ptr */
  EnemyData *v1 = enemy_data;
  uint16 hw = enemy_hw > 2048 ? 2048 : enemy_hw;
  for (uint16 v0 = 0; v0 != hw; v0 += 64, v1++) {
    if (v1->enemy_ptr && v1->enemy_ptr != 0xDAFF) {
      uint16 properties = v1->properties;
      if (properties & 0x200) {
        v1->enemy_ptr = 0;
      } else if (flag_process_all_enemies || (properties & 0x800) || (v1->ai_handler_bits & 4)
                 || ((int16)(v1->x_width + v1->x_pos - layer1_x_pos) >= 0
                     && (int16)(v1->x_width + layer1_x_pos + 256 - v1->x_pos) >= 0
                     && (int16)(v1->y_pos + 8 - layer1_y_pos) >= 0
                     && (int16)(layer1_y_pos + 248 - v1->y_pos) >= 0)) {
        if (flag_process_all_enemies) {
          /* el original escribe además el índice en interactive_enemy_indexes con el puntero de los activos */
          interactive_enemy_indexes[aw >> 1] = v0;
        }
        active_enemy_indexes[aw >> 1] = v0;
        aw += 2;
        if (!(v1->properties & 0x400)) {
          interactive_enemy_indexes[iw >> 1] = v0;
          iw += 2;
        }
      }
    }
  }
  cur_enemy_index = 2048;
  active_enemy_indexes_write_ptr = aw;
  interactive_enemy_indexes_write_ptr = iw;
  active_enemy_indexes[aw >> 1] = 0xFFFF;
  interactive_enemy_indexes[iw >> 1] = 0xFFFF;
}

uint8 SamusOutOfHealth(void);
static void HandleSamusOutOfHealthAndGameTile(void) {  // 0x82DB69
  if ((int16)samus_health <= 0 && SamusOutOfHealth()) return;   /* (death.c: tanques de reserva o la muerte) */
  if (!sign16(++game_time_frames - 60)) {
    game_time_frames = 0;
    if (!sign16(++game_time_seconds - 60)) {
      game_time_seconds = 0;
      if (!sign16(++game_time_minutes - 60)) {
        game_time_minutes = 0;
        ++game_time_hours;
      }
    }
  }
  if (!sign16(game_time_hours - 100)) {
    game_time_frames = 59;
    game_time_seconds = 59;
    game_time_minutes = 59;
    game_time_hours = 99;
  }
}

int16 nt_shake_bg1x, nt_shake_bg1y, nt_shake_bg2x, nt_shake_bg2y;

static void HandleRoomShaking(void) {  // 0xA086DF
  nt_shake_bg1x = nt_shake_bg1y = nt_shake_bg2x = nt_shake_bg2y = 0;
  if (earthquake_timer && !time_is_frozen_flag && sign16(earthquake_type - 36)) {
    int v0 = (8 * earthquake_type) >> 1;
    int16 sg = (earthquake_timer & 2) ? -1 : 1;
    nt_shake_bg1x = sg * (int16)kRoomShakes[v0];
    nt_shake_bg1y = sg * (int16)kRoomShakes[v0 + 1];
    nt_shake_bg2x = sg * (int16)kRoomShakes[v0 + 2];
    nt_shake_bg2y = sg * (int16)kRoomShakes[v0 + 3];
    reg_BG1HOFS += nt_shake_bg1x;
    reg_BG1VOFS += nt_shake_bg1y;
    reg_BG2HOFS += nt_shake_bg2x;
    reg_BG2VOFS += nt_shake_bg2y;
    --earthquake_timer;
    if (!sign16(earthquake_type - 18)) {               // SetAllEnemiesToShakeFor2Frames
      for (int i = 0; active_enemy_indexes[i] != 0xFFFF; i++)
        gEnemyData(active_enemy_indexes[i])->shake_timer = 2;
    }
  }
  ++frame_counter_every_frame;
}

static void DecrementSamusTimers(void) {  // 0xA09169
  if (samus_invincibility_timer)
    --samus_invincibility_timer;
  if (samus_knockback_timer)
    --samus_knockback_timer;
  if (projectile_invincibility_timer)
    --projectile_invincibility_timer;
  interactive_enemy_indexes[0] = 0xFFFF;
  active_enemy_indexes[0] = 0xFFFF;
}

void HandleSoundEffects(void);

void DrawSpriteObjects(void);
void DrawLowPriorityEprojs(void);
void DrawHighPriorityEprojs(void);
void DrawBombAndProjectileExplosions(void);
void EprojRunAll(void);
void HandleHudTilemap(void);
void RunRoomMainCode(void);

void DrawSamusEnemiesAndProjectiles(void) {  // 0xA0884D
  DrawSpriteObjects();
  DrawBombAndProjectileExplosions();
  DrawLowPriorityEprojs();
  for (uint16 phase = 0; phase != 8; ++phase) {
    if (phase == 3)
      DrawSamusAndProjectiles();
    else if (phase == 6)
      DrawHighPriorityEprojs();
    if (enemy_drawing_queue_sizes[phase]) DrawEnemyQueue(phase);   /* casi siempre vacía: sin la llamada */
  }
  CallEnemyGfxDrawnHook();                               // enemy_gfx_drawn_hook
}

void QueueEnemyBG2TilemapTransfers(void) {  // 0xA09726 (la transferencia a la VRAM la hace la Genesis)
  if (nmi_flag_bg2_enemy_vram_transfer && !(debug_time_frozen_for_enemies | time_is_frozen_flag))
    nt_enemy_bg2_transfer();
  nmi_flag_bg2_enemy_vram_transfer = 0;
}

void EprojSamusCollDetect(void);
void EprojProjCollDet(void);
void ProcessEnemyPowerBombInteraction(void);

uint8 DisplayMessageBox_Async(uint16 a);   /* msgbox.c */

/* corrutina como en el decomp (coroutine_state_1): 1 = caja de mensaje pedida por un enemigo, 2 = PlmHandler_Async
   (caja de mensaje de un PLM); mientras esperan, el resto del cuadro no corre */
static uint8 GameState_8_MainGameplay(void) {  // 0x828B44
  switch (coroutine_state_1) {
  case 1: goto RESUME_1;
  case 2: goto RESUME_2;
  }
  DetermineWhichEnemiesToProcess();
  PaletteFxHandler();
  HandleControllerInputForGamePhysics();
  if (!debug_disable_sprite_interact)
    SamusProjectileInteractionHandler();
  EnemyMain();
  if (queued_message_box_index) {
RESUME_1:
    if (DisplayMessageBox_Async(queued_message_box_index)) { coroutine_state_1 = 1; return 1; }
    queued_message_box_index = 0;
  }
  HandleSamusMovementAndPause();
  EprojRunAll();
RESUME_2:
  if (PlmHandler()) { coroutine_state_1 = 2; return 1; }  // PlmHandler_Async
  AnimtilesHandler();
  if (!debug_disable_sprite_interact) {
    EprojSamusCollDetect();
    EprojProjCollDet();
    ProcessEnemyPowerBombInteraction();
  }
  MainScrollingRoutine();
  DrawSamusEnemiesAndProjectiles();
  QueueEnemyBG2TilemapTransfers();
  HandleHudTilemap();
  CalculateLayer2PosAndScrollsWhenScrolling();
  RunRoomMainCode();
  HandleSamusOutOfHealthAndGameTile();
  HandleRoomShaking();
  DecrementSamusTimers();
  coroutine_state_1 = 0;
  return 0;
}

/* la pausa (estados 12-18, pause.c; docs/53): fuera del switch de RunGameState para no cambiar cómo se compila (los
   tiempos de las escenas de Ceres dependen del ciclo exacto) */
static uint8 RunGameState(uint16 st);
static uint8 DeathGameState(uint16 st);
uint8 RefillHealthFromReserveTanks(void);
NT_LATE static uint8 PauseGameState(uint16 st) {
  /* (también la muerte: 19-24 y 27, death.c; los que corren el juego usan esta misma llamada a RunGameState(8)) */
  if (st == 12 || st == 18 || st == 19 || st == 20 || st == 27) {   // GameState_12_Pausing_Darkening_Async / _18_Unpausing
    if (st == 27 && RefillHealthFromReserveTanks()) {   // GameState_27_ReserveTanksAuto ($82:DC10): antes del juego
      time_is_frozen_flag = 0;
      game_state = 8;
      CallSomeSamusCode(0x10);
    }
    if (RunGameState(8)) return 1;                 /* GameState_8_MainGameplay (así no se agrega otra llamada) */
    return st >= 19 ? DeathGameState(st) : PauseFadeStates(st);
  }
  if (st >= 19) return DeathGameState(st);
  return PauseStates(st);                          /* 13-17 */
}

/* la muerte de Samus (estados 19-25 y 27; death.c, docs/64): fuera del switch de RunGameState, como la pausa */
void StartSamusDeathAnimation(void);
void Samus_DrawStartingDeathAnim(void);
void Samus_DrawDuringDeathAnim(void);
uint16 HandleSamusDeathSequence(void);
void GameState_24_SamusNoHealth_Explosion_1(void);
uint16 GameState_24_SamusNoHealth_Explosion_2(void);
uint8 RefillHealthFromReserveTanks(void);
void SamusDeathGfx(void);
void Samus_DrawWhenNotAnimatingOrDying(void);
void Samus_LowHealthCheck_(void);
uint8 AdvancePaletteFadeForAllPalettes(void);
void WaitUntilEndOfVblankAndClearHdma(void);
void HandleFadeOut(void);
void EnableNMI(void);
void nt_clear_ram(uint16 from, uint16 to);
/* (19, 20 y 27 corren el juego antes: PauseGameState tiene la llamada a RunGameState(8); otra hacía que gcc copiara
   toda la máquina de estados: 27 KB de la ROM fija) */
NT_LATE __attribute__((optimize("Os"))) static uint8 DeathGameState(uint16 st) {
  switch (st) {
  case 27:                                       // GameState_27_ReserveTanksAuto ($82:DC10), después del juego
    Samus_LowHealthCheck_();
    return 0;
  case 19:                                       // GameState_19_SamusNoHealth ($82:DC80)
    for (int i = 255; i >= 0; --i)
      cinematic_text_tilemap[i + 384] = palette_buffer[i];   /* (7E:3300: la paleta del juego, guardada) */
    for (int j = 0; j < 192; j++) target_palettes[j] = 0;
    for (int k = 0; k < 48; k++) target_palettes[k + 208] = 0;
    for (int m = 0; m < 16; m++) target_palettes[m + 192] = palette_buffer[m + 192];
    game_options_screen_index = 3;
    g_word_7E0DE4 = 0;
    g_word_7E0DE6 = 0;
    g_word_7E0DE8 = 0;
    hud_item_index = 0;
    samus_auto_cancel_hud_item_index = 0;
    samus_invincibility_timer = 0;
    samus_knockback_timer = 0;
    ++game_state;
    return 0;
  case 20:                                       // GameState_20_SamusNoHealth_BlackOut ($82:DCE0)
    palette_change_denom = 6;
    if (AdvancePaletteFadeForAllPalettes()) {
      WaitUntilEndOfVblankAndClearHdma();
      reg_NMITIMEN &= ~0x30;                     /* DisableIrqInterrupts ($80:985F) */
      fx_layer_blending_config_a = 0;
      cur_irq_handler = 0;
      next_gameplay_CGWSEL = 0;
      next_gameplay_CGADSUB = 0;
      reg_TM = 16; reg_TS = 0; reg_TMW = 0; reg_TSW = 0;
      reg_BGMODE = 9;
      game_options_screen_index = 0;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      nt_clear_ram(0x1A8D, 0x1B8D);                /* (eproj_y_subpos[15] y las 127 palabras que siguen: 7E:1A8D-1B8C) */
      g_word_7E0DE8 = 16;
      game_options_screen_index = 3;
      g_word_7E0DE4 = 0;
      g_word_7E0DE6 = 0;
      ++game_state;
      power_bomb_explosion_status = 0;
      QueueSfx1_Max15(2);
      QueueSfx2_Max15(0x71);
      QueueSfx3_Max15(1);
      QueueMusic_Delayed8(0);
      QueueMusic_Delayed8(0xFF39);
      QueueMusic_DelayedY(5, 0xE);
      SamusDeathGfx();                           /* Genesis: Samus sola en negro */
    }
    return 0;
  case 21:                                       // GameState_21_SamusNoHealth ($82:DD71)
    Samus_DrawWhenNotAnimatingOrDying();
    if (!HasQueuedMusic()) {
      StartSamusDeathAnimation();
      ++game_state;
    }
    return 0;
  case 22:                                       // GameState_22_SamusNoHealth_Dying ($82:DD87)
    Samus_DrawStartingDeathAnim();
    if ((int16)--g_word_7E0DE8 <= 0)
      ++game_state;
    return 0;
  case 23:                                       // GameState_23_SamusNoHealth_Flashing ($82:DD9A)
    if (HandleSamusDeathSequence())
      ++game_state;
    else
      Samus_DrawDuringDeathAnim();
    return 0;
  case 24:                                       // GameState_24_SamusNoHealth_Explosion ($82:DDAF)
    GameState_24_SamusNoHealth_Explosion_1();
    if (GameState_24_SamusNoHealth_Explosion_2()) {
      screen_fade_delay = 1;
      screen_fade_counter = 1;
      ++game_state;
    }
    return 0;
  case 25:                                       // GameState_25_SamusNoHealth_BlackOut ($82:DDC7)
    HandleFadeOut();
    if (reg_INIDISP == 0x80) {
      EnableNMI();
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      ++game_state;
      menu_index = 0;
      debug_disable_sounds = 0;
    }
    return 0;
  }
  return 0;
}

/* RunOneFrameOfGame ($82:8948) + NMI ($80:9583) + interrupciones de línea del cuadro.
 * Los estados que esperan cuadros (puertas) son corrutinas como en el decomp: coroutine_state_0 = estado + 10
 * mientras esperan, y al volver se salta el comienzo del cuadro (mando, HDMA, aleatorio). */
static uint8 RunGameState(uint16 st) {
  switch (st) {
  case 8: return GameState_8_MainGameplay();
  case 9: return GameState_9_HitDoorBlock();
  case 10: return GameState_10();
  case 11: return GameState_11_LoadingNextRoom_Async();
  case 32:                                          // GameState_32_MadeItToCeresElevator ($82:8367)
    if (timer_status)
      DrawTimer();
    if (GameState_8_MainGameplay()) return 1;
    if (!--reached_ceres_elevator_fade_timer || (reached_ceres_elevator_fade_timer & 0x8000)) {
      ++game_state;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
    }
    return 0;
  case 33:                                          // GameState_33_BlackoutFromCeres ($82:8388)
    if (timer_status)
      DrawTimer();
    if (GameState_8_MainGameplay()) return 1;
    HandleFadeOut();
    if (reg_INIDISP == 0x80) {
      WaitUntilEndOfVblankAndClearHdma();
      fx_layer_blending_config_a = 0;
      cur_irq_handler = 0;
      next_gameplay_CGWSEL = 0;
      next_gameplay_CGADSUB = 0;
      reg_TM = 16; reg_TS = 0; reg_TMW = 0; reg_TSW = 0;
      reg_BGMODE = 9;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      loading_game_state = 0x22;                    // kLoadingGameState_22_EscapingCeres
      game_state = 34;
      SaveToSram(selected_save_slot);
      cinematic_function = FUNC16(CinematicFunctionBlackoutFromCeres);
      ceres_status = 0;
      timer_status = 0;
      QueueMusic_Delayed8(0);
      QueueSfx1_Max15(2);
      QueueSfx2_Max15(0x71);
      QueueSfx3_Max15(1);
    }
    return 0;
  case 30: case 34: case 37:                        // GameState_37_CeresGoesBoomWithSamus ($8B:A35B): 30 = intro
    GameState_CeresGoesBoom();
    return 0;
  case 38:                                          // GameState_38_SamusEscapesFromZebes ($82:84BD): la nave se fue
    if (GameState_8_MainGameplay()) return 1;
    HandleFadeOut();
    if (reg_INIDISP == 0x80) {
      WaitUntilEndOfVblankAndClearHdma();
      fx_layer_blending_config_a = 0;
      next_gameplay_CGWSEL = 0;
      next_gameplay_CGADSUB = 0;
      reg_TM = 16; reg_TS = 0; reg_TMW = 0; reg_TSW = 0;
      reg_BGMODE = 9;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      game_state = 39;                              // kGameState_39_EndingAndCredits
      cinematic_function = FUNC16(CinematicFunctionEscapeFromCebes);   // $8B:D480
      timer_status = 0;
      QueueMusic_Delayed8(0);
      QueueSfx1_Max15(2);
      QueueSfx2_Max15(0x71);
      QueueSfx3_Max15(1);
    }
    return 0;
  case 39: {                                        // GameState_39_EndingAndCredits ($8B:D443): ending.c
    extern void GameState_39_EndingAndCredits(void);
    GameState_39_EndingAndCredits();
    return 0;
  }
  case 35:                                          // GameState_35_TimeUp ($82:8411)
    if (GameState_8_MainGameplay()) return 1;
    palette_change_denom = 8;
    if (AdvancePaletteFadeForAllPalettes()) {
      game_state = 36;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
    }
    return 0;
  case 36:                                          // GameState_36_WhitingOutFromTimeUp ($82:8431)
    HandleFadeOut();
    if (reg_INIDISP == 0x80) {
      WaitUntilEndOfVblankAndClearHdma();
      fx_layer_blending_config_a = 0;
      cur_irq_handler = 0;
      next_gameplay_CGWSEL = 0;
      next_gameplay_CGADSUB = 0;
      reg_TM = 16; reg_TS = 0; reg_TMW = 0; reg_TSW = 0;
      reg_BGMODE = 9;
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      ceres_status = 0;
      timer_status = 0;
      QueueSfx1_Max15(2);
      QueueSfx2_Max15(0x71);
      QueueSfx3_Max15(1);
      if (CheckEventHappened(0xE)) {             /* la huida de Zebes: el menú de fin de juego (docs/64) */
        game_options_screen_index = 0;
        menu_index = 0;
        nt_clear_ram(0x1A8D, 0x1B8D);            /* (eproj_y_subpos[15] y las 127 palabras que siguen) */
        game_state = 25;
      } else {
        game_state = 37;
        cinematic_function = FUNC16(CinematicFunctionBlackoutFromCeres);
      }
    }
    return 0;
  case 6: return InitAndLoadGameData_Async();       /* gamestart.c: empezar en la estación de carga */
  case 7:                                           // GameState_7_MainGameplayFadeIn ($82:8B20)
    if (GameState_8_MainGameplay()) return 1;
    HandleFadeIn();
    if (reg_INIDISP == 15) {
      screen_fade_delay = 0;
      screen_fade_counter = 0;
      ++game_state;
    }
    return 0;
  case 5: case 31: case 0xFFFF: return 0;           /* fin de juego -> la partida guardada / Ceres / reinicio: la Genesis */
  case 25: case 26: return GameOverStates(st);      /* gameover.c */
  default:
    if ((uint16)(st - 12) <= 6) return PauseGameState(st);   /* 12-18: la pausa */
    if ((uint16)(st - 19) <= 6 && st != 25 || st == 27) return PauseGameState(st);   /* 19-24, 27: la muerte (death.c; 25: gameover.c) */
    nt_unimpl("game_frame (estado de juego sin portar)"); return 0;
  }
}

static uint16 cur_joy;
uint8 nt_frame_noupdate;   /* el cuadro terminó esperando sin actualizar la pantalla (caja de mensaje: el SNES no toca la OAM) */
void ReadJoypadInputs_Cur(void) { ReadJoypadInputs(cur_joy); }   /* caja de mensaje: el mando de este cuadro */

void game_frame(uint16 joy) {
  cur_joy = joy;
  uint8 st = coroutine_state_0;
  if (st < 10) {
    coroutine_state_1 = coroutine_state_2 = coroutine_state_3 = coroutine_state_4 = 0;
    ReadJoypadInputs(joy);
    HandleMusicQueue();                    // HdmaObjectHandler: primero la cola de música
    HdmaObjectHandler();
    NextRandom();
    oam_next_ptr = 0;
    nmi_copy_samus_halves = 0;
    nmi_copy_samus_top_half_src = 0;
    nmi_copy_samus_bottom_half_src = 0;
    coroutine_state_0 = st = game_state + 10;
  }
  nt_frame_noupdate = 0;
  if (!RunGameState(st - 10)) {
    HandleSoundEffects();
    oam_next_ptr = 0;                      // ClearUnusedOam
    joypad1_input_samusfilter = joypad1_lastkeys;   // ShowSpareCpu ($82:8AB0)
    coroutine_state_0 = 0;
    waiting_for_nmi = 1;
  }
  else if (!waiting_for_nmi && coroutine_state_3)
    nt_frame_noupdate = 1;                 /* (esperando dentro de DisplayMessageBox_Async) */
  /* NMI ($80:9583): lo que vuelve a la RAM */
  if (waiting_for_nmi) {
    waiting_for_nmi = 0;
    if (animtiles_enable_flag & 0x8000)    // NmiProcessAnimtilesVramTransfers ($80:9416): la Genesis sube los tiles
      for (int i = 5; i >= 0; i--)
        if (animtiles_ids[i] && animtiles_src_ptr[i]) {
          if (animtiles_vram_ptr[i] >= 0x6000)     /* a la VRAM de los sprites (las gemas de las estatuas de Tourian):
                                                     como las copias de tiles de los jefes (mkzobj.ROOM_VRAM_OPS) */
            nt_enemy_vram_copy(0x870000 | animtiles_src_ptr[i], animtiles_vram_ptr[i]);
          else
            nt_animtiles_transfer(animtiles_src_ptr[i], animtiles_sizes[i], animtiles_vram_ptr[i]);
          animtiles_src_ptr[i] = 0;
        }
    if (vram_write_queue_tail) {           // NMI: la cola de VRAM ($80:9667) la sube la plataforma
      nt_vram_queue_flush(nt_vwq, vram_write_queue_tail);
      vram_write_queue_tail = 0;
    }
    nmi_frames_missed = 0;
    ++nmi_frame_counter_byte;
    ++nmi_frame_counter_word;
  } else if (++nmi_frames_missed >= nmi_frames_missed_max) {
    nmi_frames_missed_max = nmi_frames_missed;
  }
  ++nmi_frame_counter_including_lag;
  run_irqs_for_frame();
}
