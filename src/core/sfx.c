/* MegaMetroid — motor nativo: cola de efectos de sonido (banco $80/$82 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm: QueueSfx*_Internal ($80:9051) y HandleSoundEffects ($82:8A2C). La cola es
 * estado del juego; lo que en el SNES se escribía al APU va a sfx_play() (la Genesis lo manda a su driver). */
#include "samus.h"

void sfx_play(uint16 channel, uint8 id);

static uint8 *sfx_queue(int j) { return j == 0 ? sfx1_queue : j == 1 ? sfx2_queue : sfx3_queue; }

static void QueueSfx_Internal(int j, uint16 a) {  // 0x809051 / 0x8090D3 / 0x809155
  sfx_max_queued[j] = (uint8)a;
  if (((sfx_writepos[j] - sfx_readpos[j]) & 0xF) < (uint8)a) {
    uint8 v2 = HI8(a);
    if (!debug_disable_sounds && game_state < 0x28 && !(power_bomb_explosion_status & 0x8000)) {
      uint8 *q = sfx_queue(j);
      uint8 v1 = sfx_writepos[j];
      uint8 v3 = sfx_writepos[j] + 1;
      if (v3 >= 0x10)
        v3 = 0;
      if (v3 == sfx_readpos[j]) {
        if (v2 < q[v1])
          q[v1] = v2;
      } else {
        q[sfx_writepos[j]] = v2;
        sfx_writepos[j] = v3;
        q[v3] = 0;
      }
    }
  }
}

void QueueSfx1_Max3(uint16 v) { QueueSfx_Internal(0, v << 8 | 3); }
void QueueSfx2_Max3(uint16 v) { QueueSfx_Internal(1, v << 8 | 3); }
void QueueSfx3_Max3(uint16 v) { QueueSfx_Internal(2, v << 8 | 3); }
void QueueSfx1_Max6(uint16 v) { QueueSfx_Internal(0, v << 8 | 6); }
void QueueSfx1_Max9(uint16 v) { QueueSfx_Internal(0, v << 8 | 9); }
void QueueSfx1_Max15(uint16 v) { QueueSfx_Internal(0, v << 8 | 15); }
void QueueSfx2_Max6(uint16 v) { QueueSfx_Internal(1, v << 8 | 6); }
void QueueSfx2_Max15(uint16 v) { QueueSfx_Internal(1, v << 8 | 15); }
void QueueSfx2_Max9(uint16 v) { QueueSfx_Internal(1, v << 8 | 9); }
void QueueSfx1_Max1(uint16 v) { QueueSfx_Internal(0, v << 8 | 1); }
void QueueSfx2_Max1(uint16 v) { QueueSfx_Internal(1, v << 8 | 1); }
void QueueSfx3_Max1(uint16 v) { QueueSfx_Internal(2, v << 8 | 1); }
void QueueSfx3_Max6(uint16 v) { QueueSfx_Internal(2, v << 8 | 6); }
void QueueSfx3_Max9(uint16 v) { QueueSfx_Internal(2, v << 8 | 9); }
void QueueSfx3_Max15(uint16 v) { QueueSfx_Internal(2, v << 8 | 15); }

void HandleSoundEffects(void) {  // 0x8289EF
  if ((int8)(sound_handler_downtime - 1) >= 0) {
    sound_handler_downtime = (sound_handler_downtime & 0xFF00) | (uint8)(sound_handler_downtime - 1);
    for (int j = 0; j < 3; j++) {
      sfx_play(j, 0);
      sfx_cur[j] = 0;
    }
    return;
  }
  for (int j = 0; j < 3; ++j) {
    switch (sfx_state[j]) {
    case 0:                                       // 0x828A2C
      if (sfx_readpos[j] != sfx_writepos[j]) {
        uint8 v3 = sfx_queue(j)[sfx_readpos[j]];
        sfx_play(j, v3);
        sfx_cur[j] = v3;
        sfx_readpos[j] = (sfx_readpos[j] + 1) & 0xF;
        ++sfx_state[j];
      }
      break;
    case 1:                                       // 0x828A55 (modificada en el decomp)
      ++sfx_state[j];
      sfx_clear_delay[j] = 6;
      break;
    case 2:                                       // 0x828A6C
      if (sfx_clear_delay[j]-- == 1) {
        sfx_cur[j] = 0;
        ++sfx_state[j];
      }
      break;
    case 3:                                       // 0x828A7C (modificada en el decomp)
      sfx_state[j] = 0;
      if (sfx_readpos[j] != sfx_writepos[j]) {
        uint8 v3 = sfx_queue(j)[sfx_readpos[j]];
        sfx_play(j, v3);
        sfx_cur[j] = v3;
        sfx_readpos[j] = (sfx_readpos[j] + 1) & 0xF;
        ++sfx_state[j];
      }
      break;
    case 4:                                       // 0x828A90
      if (sfx_clear_delay[j]-- == 1)
        sfx_state[j] = 0;
      break;
    }
  }
}
