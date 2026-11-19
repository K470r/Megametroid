/* MegaMetroid — motor nativo: la muerte de Samus (docs/64). Escrito a mano desde snesrev/sm: los estados 19-25 y 27
 * ($82:DC10-DDF0), la secuencia del banco $9B ($9B:B3A7-B7FF) y el dibujo de Samus muriendo ($90:8976-89FF,
 * $92:EDBE).
 *
 * Samus sin energía (HandleSamusOutOfHealthAndGameTile, game.c): con tanques de reserva en automático se rellena
 * (estado 27); si no, el juego se oscurece con Samus encima (19-20), queda sola en negro, gira (21-22), titila (23)
 * y el traje explota en pedazos con la pantalla que se pone blanca (24); después se apaga (25) y el menú de fin de
 * juego (26, gameover.c). Los tiles de los pedazos ($9B:8000-93FF, que el SNES sube a la VRAM con la cola) los pone
 * la Genesis: SamusDeathTiles. Las paletas y los tiempos salen de la ROM (mkdata.py: kDeathPals, kDeathTimes). */
#pragma GCC optimize("Os")                     /* (lo común de la ROM está lleno; no es un camino caliente) */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "tables.h"
#include "dcompat.h"

void SamusFunc_F433(void);
void Samus_SetAnimationFrameIfPoseChanged(void);
void DrawSamusSpritemap(uint16 idx, uint16 x, uint16 y);
void SetSamusTilesDefsForCurAnim(void);
uint8 SamusBottomDrawn(void);
void SamusDeathGfx(void);                      /* Genesis: Samus sola en negro (el apagón terminó) */
void SamusDeathTiles(uint16 i);                 /* Genesis: los tiles de los pedazos del traje (0-3: filas 2-9; 4: 0-1) */

static const uint8 kDeathAnimationFrames[28] = {  // $9B:B3E1 (por tipo de movimiento)
  5, 5, 5, 5, 1, 5, 5, 0, 1, 0, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 5, 5, 5, 5, 5, 5, 5, 5,
};
static const uint16 kShadesOfWhite[22] = {        // $9B:B7D3 - 22
  0x421, 0xc63, 0x14a5, 0x1ce7, 0x2529, 0x2d6b, 0x35ad, 0x4210, 0x4a52, 0x4e73, 0x5294, 0x56b5, 0x5ad6, 0x5ef7,
  0x6318, 0x6739, 0x6b5a, 0x6f7b, 0x739c, 0x77bd, 0x7bde, 0x7fff,
};

void StartSamusDeathAnimation(void) {  // 0x9BB3A7
  uint16 v0 = samus_movement_type;
  if (samus_movement_type == 3)
    QueueSfx1_Max6(0x32);
  uint16 v1 = kDeathAnimationFrames[v0];
  if (samus_pose_x_dir == 4)
    samus_pose = kPose_D8_FaceL_CrystalFlashEnd;
  else
    samus_pose = kPose_D7_FaceR_CrystalFlashEnd;
  SamusFunc_F433();
  Samus_SetAnimationFrameIfPoseChanged();
  samus_last_different_pose = samus_prev_pose;
  samus_last_different_pose_x_dir = samus_prev_pose_x_dir;          /* (palabras del SNES: dirección y tipo de */
  samus_last_different_pose_movement_type = samus_prev_movement_type2;   /*  movimiento juntos) */
  samus_prev_pose = samus_pose;
  samus_prev_pose_x_dir = samus_pose_x_dir;
  samus_prev_movement_type2 = samus_movement_type;
  samus_anim_frame_skip = 0;
  samus_anim_frame = v1;
  samus_x_pos -= layer1_x_pos;
  samus_y_pos -= layer1_y_pos;
}

/* (Samus_DrawStartingDeathAnim y Samus_DrawDuringDeathAnim, $90:8976/8998: en samus_move.c) */

static void DrawSamusSuitExploding(void) {  // 0x92EDBE
  uint16 r18 = (samus_pose_x_dir == 4) ? g_word_7E0DE4 + 2085 : g_word_7E0DE4 + 2076;
  DrawSamusSpritemap(r18, samus_x_pos, samus_y_pos);
}

static void CopyPalettesForSamusDeath(uint16 v0) {  // 0x9BB5CE
  uint16 s = 0x8000 + ((samus_suit_palette_index >> 1) * 10 + (v0 >> 1)) * 32;   /* (kDeathPalsB: en la página fría) */
  uint16 t = 0x8000 + (30 + (v0 >> 1)) * 32;
  for (int i = 0; i < 16; i++) {
    palette_buffer[192 + i] = nt_paged_w(kDeathPalsB, s + 2 * i);
    palette_buffer[240 + i] = nt_paged_w(kDeathPalsB, t + 2 * i);
  }
  nt_pal_dirty(192, 255);
}

static void QueueTransferOfSamusDeathSequence(uint16 v0) {  // 0x9BB6D8
  SamusDeathTiles(v0 >> 1);
}

void GameState_24_SamusNoHealth_Explosion_1(void) {  // 0x9BB710
  if (!substate && g_word_7E0DE4) {
    uint16 c = kShadesOfWhite[g_word_7E0DE6];
    for (int i = 0; i < 192; i++) palette_buffer[i] = c;
    for (int i = 208; i < 240; i++) palette_buffer[i] = c;
    nt_pal_dirty(0, 239);
    if (sign16(g_word_7E0DE6 - 20))
      ++g_word_7E0DE6;
  }
}

uint16 GameState_24_SamusNoHealth_Explosion_2(void) {  // 0x9BB758
  if ((int16)--game_options_screen_index <= 0) {
    if (!sign16(++g_word_7E0DE4 - 9)) {
      g_word_7E0DE6 = 21;
      GameState_24_SamusNoHealth_Explosion_1();
      substate = 0;
      return 1;
    }
    if (!substate || sign16(g_word_7E0DE4 - 2)) {
      game_options_screen_index = kDeathTimes[2 * g_word_7E0DE4];
      CopyPalettesForSamusDeath(2 * kDeathTimes[2 * g_word_7E0DE4 + 1]);
    } else {
      game_options_screen_index = kDeathTimes[2 * g_word_7E0DE4];
    }
  }
  DrawSamusSuitExploding();
  return 0;
}

static void HandleSamusDeathSequence_Helper2(void) {  // 0x9BB4B6
  CopyPalettesForSamusDeath(0);                 /* ($9B:B5C8 y $9B:A120: las primeras de las listas) */
  QueueTransferOfSamusDeathSequence(8);
  game_options_screen_index = kDeathTimes[0];
  g_word_7E0DE4 = 0;
  g_word_7E0DE6 = 0;
  GameState_24_SamusNoHealth_Explosion_2();
}

uint16 HandleSamusDeathSequence(void) {  // 0x9BB441
  if (sign16(g_word_7E0DE6 - 4))
    QueueTransferOfSamusDeathSequence(2 * g_word_7E0DE6);
  if (sign16(++g_word_7E0DE6 - 60)) {
    if ((int16)--game_options_screen_index <= 0) {
      if (g_word_7E0DE4) {
        g_word_7E0DE4 = 0;
        game_options_screen_index = 3;
      } else {
        g_word_7E0DE4 = 1;
        game_options_screen_index = 1;
      }
      CopyPalettesForSamusDeath(g_word_7E0DE4 * 2);
    }
    return 0;
  }
  HandleSamusDeathSequence_Helper2();
  substate = (joypad2_last & 0xB0) == (kButton_A | kButton_L | kButton_R);
  return 1;
}

/* ------------------------------------------------------------------ tanques de reserva en automático */
void QueueSfx3_Max3(uint16 a);

uint8 RefillHealthFromReserveTanks(void) {  // 0x82DC31
  if (samus_reserve_health) {
    if ((nmi_frame_counter_word & 7) == 0)
      QueueSfx3_Max3(0x2D);
    if ((int16)(++samus_health - samus_max_health) >= 0) {
      samus_health = samus_max_health;
      samus_reserve_health = 0;
      return 1;
    }
    uint16 r = --samus_reserve_health;
    if (!r) return 1;
    if (r & 0x8000) { samus_health += r; samus_reserve_health = 0; return 1; }
  }
  return samus_reserve_health == 0;
}


/* Samus sin energía (HandleSamusOutOfHealthAndGameTile, $82:DB69): 1 si el original sale sin contar el tiempo */
uint8 SamusOutOfHealth(void) {
  if ((reserve_health_mode & 1) && samus_reserve_health) {
    time_is_frozen_flag = 0x8000;
    game_state = 27;                               /* kGameState_27_ReserveTanksAuto */
    CallSomeSamusCode(0x1B);
    return 0;
  }
  if (game_state != 8)
    return 1;
  time_is_frozen_flag = 0x8000;
  CallSomeSamusCode(0x11);
  game_state = 19;                                 /* kGameState_19_SamusNoHealth */
  return 0;
}
