/* MegaMetroid — motor nativo: puertas y carga de salas (banco $82, con partes de $80, $88, $89 y $8F del decomp,
 * docs/31). Escrito a mano desde snesrev/sm sm_82.c (GameState_9/10/11 y DoorTransitionFunction_*), sm_80.c
 * (desplazamiento de la puerta) y sm_8f.c / sm_89.c (cabeceras de sala, código de puerta y de sala, FX).
 *
 * Las salas vienen preparadas en el PC (gen/tables.c: kRoomDefs, kRoomStates, kDoorDefs, kLevels, kFxDefs): la
 * Genesis no descomprime nada ni lee la ROM del SNES. Las direcciones del SNES (room_ptr, door_def_ptr,
 * roomdefroomstate_ptr...) se conservan como identificadores, para poder comparar con el SNES.
 *
 * Lo que en el SNES iba a la VRAM (tiles del tileset, de los enemigos, fondo de la capa 2, tilemap de la capa 3)
 * lo hace la capa de la Genesis con la sala ya convertida (g_hooks.room_gfx). Los sistemas aún sin escribir
 * (enemigos, PLMs, proyectiles enemigos, HDMA, animaciones de tiles) se llaman por g_hooks. */
#include "samus.h"
#include "game.h"
#include "hdmaobj.h"
void HandleCeresRidleyGetawayCutscene(void);   /* ceres_ridley.c */
#include "room.h"
#include "enemies.h"
#include "animtiles.h"
#include "plm.h"
#include "nmath.h"
void EprojRunAll(void);

/* ------------------------------------------------------------------ datos de la sala preparados en el PC */
/* búsqueda binaria: mkdata.py deja las tablas ordenadas por dirección del SNES */
#define NT_BSEARCH(T, tab, a, what) do { \
    int lo = 0, hi = (int)(sizeof tab / sizeof tab[0]) - 1; \
    while (lo <= hi) { int m = (lo + hi) >> 1; \
      if (tab[m].addr == (a)) return &tab[m]; \
      if (tab[m].addr < (a)) lo = m + 1; else hi = m - 1; } \
    nt_unimpl(what); return &tab[0]; } while (0)
const RoomDefN *room_def(uint16 a) { NT_BSEARCH(RoomDefN, kRoomDefs, a, "room_def (sala sin preparar)"); }
const RoomStateN *room_state(uint16 a) { NT_BSEARCH(RoomStateN, kRoomStates, a, "room_state (estado sin preparar)"); }
const DoorDefN *door_def(uint16 a) { NT_BSEARCH(DoorDefN, kDoorDefs, a, "door_def (puerta sin preparar)"); }
uint16 room_door_def_ptr(uint16 i) {   /* la entrada i de la lista de puertas de la sala */
  const RoomDefN *R = room_def(room_ptr);
  return i < R->ndoors ? R->doors[i] : 0;
}
uint16 door_def_room_ptr(uint16 d) { return door_def(d)->room_definition_ptr; }


/* ------------------------------------------------------------------ paletas: fundido ($82:DA02) */
/* kFadeQuot[den][d] = (d << 8) / den: el cociente de CalculateNthTransitionColorComponentFromXtoY ya calculado */
static const uint16 kFadeQuot[32][32] = {
  {65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535},
  {0, 256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840, 4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888, 6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936},
  {0, 128, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456, 3584, 3712, 3840, 3968},
  {0, 85, 170, 256, 341, 426, 512, 597, 682, 768, 853, 938, 1024, 1109, 1194, 1280, 1365, 1450, 1536, 1621, 1706, 1792, 1877, 1962, 2048, 2133, 2218, 2304, 2389, 2474, 2560, 2645},
  {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856, 1920, 1984},
  {0, 51, 102, 153, 204, 256, 307, 358, 409, 460, 512, 563, 614, 665, 716, 768, 819, 870, 921, 972, 1024, 1075, 1126, 1177, 1228, 1280, 1331, 1382, 1433, 1484, 1536, 1587},
  {0, 42, 85, 128, 170, 213, 256, 298, 341, 384, 426, 469, 512, 554, 597, 640, 682, 725, 768, 810, 853, 896, 938, 981, 1024, 1066, 1109, 1152, 1194, 1237, 1280, 1322},
  {0, 36, 73, 109, 146, 182, 219, 256, 292, 329, 365, 402, 438, 475, 512, 548, 585, 621, 658, 694, 731, 768, 804, 841, 877, 914, 950, 987, 1024, 1060, 1097, 1133},
  {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992},
  {0, 28, 56, 85, 113, 142, 170, 199, 227, 256, 284, 312, 341, 369, 398, 426, 455, 483, 512, 540, 568, 597, 625, 654, 682, 711, 739, 768, 796, 824, 853, 881},
  {0, 25, 51, 76, 102, 128, 153, 179, 204, 230, 256, 281, 307, 332, 358, 384, 409, 435, 460, 486, 512, 537, 563, 588, 614, 640, 665, 691, 716, 742, 768, 793},
  {0, 23, 46, 69, 93, 116, 139, 162, 186, 209, 232, 256, 279, 302, 325, 349, 372, 395, 418, 442, 465, 488, 512, 535, 558, 581, 605, 628, 651, 674, 698, 721},
  {0, 21, 42, 64, 85, 106, 128, 149, 170, 192, 213, 234, 256, 277, 298, 320, 341, 362, 384, 405, 426, 448, 469, 490, 512, 533, 554, 576, 597, 618, 640, 661},
  {0, 19, 39, 59, 78, 98, 118, 137, 157, 177, 196, 216, 236, 256, 275, 295, 315, 334, 354, 374, 393, 413, 433, 452, 472, 492, 512, 531, 551, 571, 590, 610},
  {0, 18, 36, 54, 73, 91, 109, 128, 146, 164, 182, 201, 219, 237, 256, 274, 292, 310, 329, 347, 365, 384, 402, 420, 438, 457, 475, 493, 512, 530, 548, 566},
  {0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 256, 273, 290, 307, 324, 341, 358, 375, 392, 409, 426, 443, 460, 477, 494, 512, 529},
  {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 400, 416, 432, 448, 464, 480, 496},
  {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240, 256, 271, 286, 301, 316, 331, 346, 361, 376, 391, 406, 421, 436, 451, 466},
  {0, 14, 28, 42, 56, 71, 85, 99, 113, 128, 142, 156, 170, 184, 199, 213, 227, 241, 256, 270, 284, 298, 312, 327, 341, 355, 369, 384, 398, 412, 426, 440},
  {0, 13, 26, 40, 53, 67, 80, 94, 107, 121, 134, 148, 161, 175, 188, 202, 215, 229, 242, 256, 269, 282, 296, 309, 323, 336, 350, 363, 377, 390, 404, 417},
  {0, 12, 25, 38, 51, 64, 76, 89, 102, 115, 128, 140, 153, 166, 179, 192, 204, 217, 230, 243, 256, 268, 281, 294, 307, 320, 332, 345, 358, 371, 384, 396},
  {0, 12, 24, 36, 48, 60, 73, 85, 97, 109, 121, 134, 146, 158, 170, 182, 195, 207, 219, 231, 243, 256, 268, 280, 292, 304, 316, 329, 341, 353, 365, 377},
  {0, 11, 23, 34, 46, 58, 69, 81, 93, 104, 116, 128, 139, 151, 162, 174, 186, 197, 209, 221, 232, 244, 256, 267, 279, 290, 302, 314, 325, 337, 349, 360},
  {0, 11, 22, 33, 44, 55, 66, 77, 89, 100, 111, 122, 133, 144, 155, 166, 178, 189, 200, 211, 222, 233, 244, 256, 267, 278, 289, 300, 311, 322, 333, 345},
  {0, 10, 21, 32, 42, 53, 64, 74, 85, 96, 106, 117, 128, 138, 149, 160, 170, 181, 192, 202, 213, 224, 234, 245, 256, 266, 277, 288, 298, 309, 320, 330},
  {0, 10, 20, 30, 40, 51, 61, 71, 81, 92, 102, 112, 122, 133, 143, 153, 163, 174, 184, 194, 204, 215, 225, 235, 245, 256, 266, 276, 286, 296, 307, 317},
  {0, 9, 19, 29, 39, 49, 59, 68, 78, 88, 98, 108, 118, 128, 137, 147, 157, 167, 177, 187, 196, 206, 216, 226, 236, 246, 256, 265, 275, 285, 295, 305},
  {0, 9, 18, 28, 37, 47, 56, 66, 75, 85, 94, 104, 113, 123, 132, 142, 151, 161, 170, 180, 189, 199, 208, 218, 227, 237, 246, 256, 265, 274, 284, 293},
  {0, 9, 18, 27, 36, 45, 54, 64, 73, 82, 91, 100, 109, 118, 128, 137, 146, 155, 164, 173, 182, 192, 201, 210, 219, 228, 237, 246, 256, 265, 274, 283},
  {0, 8, 17, 26, 35, 44, 52, 61, 70, 79, 88, 97, 105, 114, 123, 132, 141, 150, 158, 167, 176, 185, 194, 203, 211, 220, 229, 238, 247, 256, 264, 273},
  {0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 128, 136, 145, 153, 162, 170, 179, 187, 196, 204, 213, 221, 230, 238, 247, 256, 264},
  {0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123, 132, 140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239, 247, 256},
};

static uint16 CalculateNthTransitionColorComponentFromXtoY(uint16 a, uint16 k, uint16 j) {  // 0x82DAA6
  if (!a)
    return k;
  uint16 v4 = a - 1;
  if (v4 == palette_change_denom)
    return j;
  int16 d = (int16)(j - k);
  uint16 ad = (uint16)(d < 0 ? -d : d);
  uint16 den = palette_change_denom - (v4 + 1) + 1;
  /* SnesDivide((|d| << 8) / den): en la Genesis la división de 16 bits es cara; la tabla da el mismo cociente */
  uint16 RegWord = !den ? 0xFFFF : (den < 32 && ad < 32) ? kFadeQuot[den][ad] : (uint16)((uint16)(ad << 8) / den);
  uint16 r18 = d < 0 ? -RegWord : RegWord;
  return (uint16)(r18 + (k << 8)) >> 8;
}

static uint16 CalculateNthTransitionColorFromXtoY(uint16 a, uint16 k, uint16 j) {  // 0x82DA4A
  return CalculateNthTransitionColorComponentFromXtoY(a, k & 0x1F, j & 0x1F) |
         CalculateNthTransitionColorComponentFromXtoY(a, (k >> 5) & 0x1F, (j >> 5) & 0x1F) << 5 |
         CalculateNthTransitionColorComponentFromXtoY(a, (k >> 10) & 0x1F, (j >> 10) & 0x1F) << 10;
}

#include "fade_tab.h"

static inline uint16 fade_comp(const uint16 *q, uint16 den, uint16 k, uint16 j) {
  /* CalculateNthTransitionColorComponentFromXtoY con a != 0 y a - 1 != palette_change_denom */
  if (j >= k) {
    uint16 ad = j - k;
    uint16 qq = q ? q[ad] : (uint16)((uint16)(ad << 8) / den);
    return (uint16)(qq + (k << 8)) >> 8;
  } else {
    uint16 ad = k - j;
    uint16 qq = q ? q[ad] : (uint16)((uint16)(ad << 8) / den);
    return (uint16)(-qq + (k << 8)) >> 8;
  }
}

uint8 AdvancePaletteFadeForAllPalettes(void) {  // 0x82DA02
  if ((uint16)(palette_change_denom + 1) >= palette_change_num) {
    /* lo mismo que llamar a CalculateNthTransitionColorFromXtoY con cada color distinto del objetivo, sin las
       llamadas por componente: con a == 0 el color no cambia; con a - 1 == denom pasa a ser el objetivo */
    uint16 a = palette_change_num;
    if (a) {
      uint16 v4 = a - 1;
      if (v4 == palette_change_denom) {
        for (int i = 0; i < 256; i++)
          if (target_palettes[i] != palette_buffer[i])
            palette_buffer[i] = target_palettes[i] & 0x7FFF;
      } else {
        uint16 den = palette_change_denom - v4;
        if (den < 32) {
          const uint8 *T = kFadeComp[den];
#ifdef NT_FADE_LIST
          /* Genesis: solo los colores que se ven (lista de la sala, ver md/); al terminar el fundido (rama de
             arriba) todos quedan igual que en el SNES */
          for (uint16 n = 0; n < nt_fade_count; n++) {
            uint16 i = nt_fade_list[n];
#else
          for (int i = 0; i < 256; i++) {
#endif
            uint16 t = target_palettes[i], p = palette_buffer[i];
            if (t == p)
              continue;
            palette_buffer[i] = T[((p & 31) << 5) | (t & 31)] | T[(p & 0x3E0) | ((t >> 5) & 31)] << 5 |
                                T[((p >> 5) & 0x3E0) | ((t >> 10) & 31)] << 10;
          }
        } else {
          for (int i = 0; i < 256; i++) {
            uint16 t = target_palettes[i], p = palette_buffer[i];
            if (t == p)
              continue;
            palette_buffer[i] = fade_comp(0, den, p & 0x1F, t & 0x1F) |
                                fade_comp(0, den, (p >> 5) & 0x1F, (t >> 5) & 0x1F) << 5 |
                                fade_comp(0, den, (p >> 10) & 0x1F, (t >> 10) & 0x1F) << 10;
          }
        }
      }
    }
    nt_pal_dirty(0, 255);
    g_word_7EC404 = 0x1FE;                         /* el índice del último color (el bucle del decomp lo deja así) */
    ++palette_change_num;
    return 0;
  }
  palette_change_num = 0;
  return 1;
}

static uint8 AdvancePaletteFadeForAllPalettes_0xc(void) {  // 0x82D961
  palette_change_denom = 12;
  return AdvancePaletteFadeForAllPalettes();
}

void LoadColorsForSpritesBeamsAndEnemies(void) {  // 0x82E139
  for (int i = 15; i >= 0; i--)
    target_palettes[i + 208] = kInitialPalette_Sprites[i];
  for (int i = 15; i >= 0; i--)
    target_palettes[i + 224] = palette_buffer[i + 224];
  for (int i = 15; i >= 0; i--)
    target_palettes[i + 128] = kCommonSpritesPalette1[i];
}

static void WriteBeamPalette_Y(uint16 j) {  // 0x90ACCD
  if ((j >> 1) >= 12) { nt_unimpl("WriteBeamPalette_Y (combinación de rayos sin tabla)"); return; }
  for (int i = 0; i < 16; i++)
    palette_buffer[224 + i] = kBeamPalettes[(j >> 1) * 16 + i];
  nt_pal_dirty(224, 239);
}

void WriteBeamPalette_A(uint16 a) {  // 0x90ACC2
  WriteBeamPalette_Y(2 * (a & 0xFFF));
}

void BeamTiles(uint16 combo);   /* capa de la Genesis: los 8 tiles de la combinación (kBeamTilePtrs -> VRAM $6300) */
void UpdateBeamTilesAndPalette(void) {  // 0x90AC8D
  BeamTiles(equipped_beams & 0xFFF);
  WriteBeamPalette_Y(2 * (equipped_beams & 0xFFF));
}

/* ------------------------------------------------------------------ música ($80:8EF4) */
uint8 HasQueuedMusic(void) {  // 0x808EF4
  for (int v0 = 7; v0 >= 0; v0--)
    if (music_queue_delay[v0])
      return 1;
  return 0;
}

void QueueMusic_Delayed8(uint16 a) {  // 0x808FC1
  if (game_state < 0x28 && ((music_queue_write_pos + 2) & 0xE) != music_queue_read_pos) {
    int v2 = music_queue_write_pos >> 1;
    music_queue_track[v2] = a;
    music_queue_delay[v2] = 8;
    music_queue_write_pos = (music_queue_write_pos + 2) & 0xE;
  }
}

void PlayRoomMusicTrackAfterAFrames(uint16 a) {  // 0x82E118
  if (game_state < 40) {                          // kGameState_40_TransitionToDemo
    QueueMusic_DelayedY(0, a);
    QueueMusic_Delayed8(music_track_index);
  }
}

void QueueMusic_DelayedY(uint16 a, uint16 j) {  // 0x808FF7
  if (game_state < 0x28) {
    int v2 = music_queue_write_pos >> 1;
    music_queue_track[v2] = a;
    music_queue_delay[v2] = j < 8 ? 8 : j;
    music_queue_write_pos = (music_queue_write_pos + 2) & 0xE;
  }
}

void music_upload(uint16 data_index);   /* capa de la Genesis: cargar el banco de música */
void music_play(uint8 track);           /* capa de la Genesis: tocar la pista */

void HandleMusicQueue(void) {  // 0x808F0C
  bool v0 = music_timer-- == 1;
  if (!(music_timer & 0x8000)) {
    if (!v0)
      return;
    if (music_entry & 0x8000) {
      music_data_index = (uint8)music_entry;
      cur_music_track = -1;
      music_upload(music_data_index);
      cur_music_track = 0;
      int v5 = music_queue_read_pos;
      music_queue_track[v5 >> 1] = 0;
      music_queue_delay[v5 >> 1] = 0;
      music_queue_read_pos = (v5 + 2) & 0xE;
      sound_handler_downtime = 8;
      return;
    }
    uint8 v1 = music_entry & 0x7F;
    music_track_index = v1;
    music_play(v1);
    cur_music_track = v1;
    sound_handler_downtime = 8;
    int v2 = music_queue_read_pos;
    music_queue_track[v2 >> 1] = 0;
    music_queue_delay[v2 >> 1] = 0;
    music_queue_read_pos = (v2 + 2) & 0xE;
  }
  if (music_queue_read_pos == music_queue_write_pos) {
    music_timer = 0;
  } else {
    int v4 = music_queue_read_pos >> 1;
    music_entry = music_queue_track[v4];
    music_timer = music_queue_delay[v4];
  }
}

void LoadRoomMusic(void) {  // 0x82E071
  if (game_state < 0x28 && room_music_data_index && room_music_data_index != music_data_index) {
    QueueMusic_Delayed8(0);
    QueueMusic_Delayed8(room_music_data_index | 0xFF00);
  }
}

void LoadNewMusicTrackIfChanged(void) {  // 0x82E0D5
  if (game_state < 0x28 && room_music_track_index) {
    uint16 r18 = room_music_data_index << 8 | room_music_track_index;
    uint16 r20 = music_data_index << 8 | music_track_index;
    if (r18 != r20)
      QueueMusic_DelayedY(room_music_track_index, 6);
  }
}

/* ------------------------------------------------------------------ cabeceras ($82:DDF1) */
void LoaadDesinationRoomCreBitset(void) {  // 0x82DDF1
  uint16 room_definition_ptr = door_def(door_def_ptr)->room_definition_ptr;
  previous_cre_bitset = cre_bitset;
  cre_bitset = room_def(room_definition_ptr)->cre_bitset;
}

void LoadDoorHeader(void) {  // 0x82DE12
  const DoorDefN *D = door_def(door_def_ptr);
  room_ptr = D->room_definition_ptr;
  elevator_door_properties_orientation = D->door_bitflags | D->door_orientation << 8;
  elevator_flags = elevator_door_properties_orientation & 0x80;
  door_direction = D->door_orientation;
  door_destination_x_pos = D->x_pos_in_room << 8;
  door_destination_y_pos = D->y_pos_in_room << 8;
  int16 samus_distance_from_door = D->samus_distance_from_door;
  if (samus_distance_from_door < 0)
    samus_distance_from_door = (door_direction & 2) ? 384 : 200;
  uint32 t = (uint32)samus_distance_from_door << 8;
  samus_door_transition_subspeed = t & 0xFFFF;
  samus_door_transition_speed = t >> 16;
}

static void HandleRoomDefStateSelect(const RoomDefN *R) {  // 0x8FE5D2
  for (int i = 0; i < R->nsel; i++) {
    const StateSelectN *S = &R->sel[i];
    switch (S->kind) {
    case 0: roomdefroomstate_ptr = S->state; return;                          // RoomDefStateSelect_Finish
    case 2: if (CheckEventHappened(S->arg)) { roomdefroomstate_ptr = S->state; return; } break;
    case 3: if (CheckBossBitForCurArea(S->arg) & 1) { roomdefroomstate_ptr = S->state; return; } break;
    case 1: if (CheckBossBitForCurArea(1) & 1) { roomdefroomstate_ptr = S->state; return; } break;     // TourianBoss01
    case 4: if ((collected_items & 4) && samus_max_missiles) { roomdefroomstate_ptr = S->state; return; } break;  // MorphBallMissiles
    case 5: if (samus_max_power_bombs) { roomdefroomstate_ptr = S->state; return; } break;              // PowerBombs
    }
  }
}

void LoadRoomHeader(void) {  // 0x82DE6F
  const RoomDefN *R = room_def(room_ptr);
  room_index = R->semiunique_room_number;
  area_index = R->area_index;
  room_x_coordinate_on_map = R->x_coordinate_on_map;
  room_y_coordinate_on_map = R->y_coordinate_on_map;
  room_width_in_scrolls = R->width;
  room_width_in_blocks = 16 * room_width_in_scrolls;
  room_height_in_scrolls = R->height;
  room_height_in_blocks = 16 * room_height_in_scrolls;
  up_scroller = R->up_scroller;
  down_scroller = R->down_scroller;
  door_list_pointer = R->ptr_to_doorout;
  HandleRoomDefStateSelect(R);
  room_size_in_blocks = 2 * (uint16)((uint8)room_width_in_blocks * (uint8)room_height_in_blocks);
}

void LoadStateHeader(void) {  // 0x82DEF2
  const RoomStateN *S = room_state(roomdefroomstate_ptr);
  /* tileset (tiles, tabla de tiles y paleta): los pone la Genesis con la sala ya convertida */
  room_music_data_index = S->music_track;
  room_music_track_index = S->music_control;
  room_layer3_asm_ptr = S->room_layer3_fx_ptr;
  room_enemy_population_ptr = S->enemy_population_ptr;
  room_enemy_tilesets_ptr = S->enemy_tilesets_ptr;
  layer2_scroll_x = LO8(S->vertical_screen_nudge_limit);
  layer2_scroll_y = HI8(S->vertical_screen_nudge_limit);
  room_main_code_ptr = S->main_code_ptr;
}

void LoadLevelScrollAndCre(void) {  // 0x82EA73
  const RoomStateN *S = room_state(roomdefroomstate_ptr);
  lvl_load(S->level);                             /* el nivel ya descomprimido (room.h) */
  /* el fondo propio de la capa 2 (bloques) y la tabla de tiles: los dibuja la Genesis */
  int16 rdf_scroll_ptr = S->rdf_scroll_ptr;
  if (rdf_scroll_ptr >= 0) {
    uint8 r20 = room_height_in_scrolls - 1;
    uint8 v8 = 2, v9 = 0;
    int8 v10 = 0;
    do {
      if (v10 == (int8)r20)
        v8 = (uint8)rdf_scroll_ptr + 1;
      int8 v11 = 0;
      do {
        scrolls[v9++] = v8;
        ++v11;
      } while (v11 != (int8)(uint8)room_width_in_scrolls);
      v10++;
    } while (v10 != (int8)(uint8)room_height_in_scrolls);
  } else {
    for (int m = 0; m < 50; m++)                   /* la tabla de scrolls de la sala ($8F) */
      scrolls[m] = (uint8)B8FW((uint16)(rdf_scroll_ptr + m));
  }
}

/* ------------------------------------------------------------------ código de puerta y de sala ($8F) */
/* room_main_asm_variables: bytes en la RAM del SNES, leídos como palabras por el código de sala */
static inline uint16 rmv_get(int i) { return room_main_asm_variables[i] | room_main_asm_variables[i + 1] << 8; }
static inline void rmv_set(int i, uint16 v) { room_main_asm_variables[i] = (uint8)v; room_main_asm_variables[i + 1] = v >> 8; }

static void RunDoorSetupCode(void) {  // 0x8FE8A3
  switch (door_def(door_def_ptr)->door_setup_code) {
  case 0: break;
  case 0xE4E0:                                    // DoorCode_CeresElevatorShaft
    reg_BGMODE_fake = 7;
    reg_M7A = 256;
    reg_M7D = 256;
    reg_M7B = 0;
    reg_M7C = 0;
    reg_M7X = 128;
    reg_M7Y = 1008;
    irq_enable_mode7 = 1;
    room_main_asm_variables[0] = '"'; room_main_asm_variables[1] = 0;   /* strcpy del original: 0x22, 0 */
    room_main_asm_variables[2] = '<'; room_main_asm_variables[3] = 0;
    break;
  case 0xE513:                                    // DoorCode_CeresElevatorShaft_2
    reg_BGMODE_fake = 9;
    irq_enable_mode7 = 0;
    break;
  case 0xB981: scrolls[6] = 2; break;   // DoorCode_Scroll6_Green
  case 0xB98C: scrolls[0] = 1; break;   // DoorCode_Scroll0_Blue
  case 0xB997: scrolls[19] = 1; break;   // DoorCode_Scroll13_Blue
  case 0xB9A2: scrolls[4] = 0; scrolls[8] = 2; break;   // DoorCode_Scroll_4Blue_8Green
  case 0xB9B3: scrolls[8] = 0; scrolls[9] = 0; scrolls[10] = 0; scrolls[11] = 0; break;   // DoorCode_Scroll_8toB_Red
  case 0xB9CA: scrolls[2] = 0; scrolls[3] = 0; scrolls[4] = 0; scrolls[5] = 0; scrolls[11] = 0; scrolls[12] = 0; scrolls[13] = 0; scrolls[17] = 0; break;   // DoorCode_Scroll_LotsRed
  case 0xB9F1: scrolls[1] = 2; scrolls[4] = 2; break;   // DoorCode_Scroll_1_4_Green
  case 0xBA00: scrolls[2] = 1; break;   // DoorCode_Scroll_2_Blue
  case 0xBA0B: scrolls[23] = 1; break;   // DoorCode_Scroll_17_Blue
  case 0xBA16: scrolls[4] = 1; break;   // DoorCode_Scroll_4_Blue
  case 0xBA21: scrolls[6] = 2; break;   // DoorCode_Scroll_6_Green
  case 0xBA2C: scrolls[3] = 2; break;   // DoorCode_Scroll_3_Green
  case 0xBD07: scrolls[24] = 2; scrolls[28] = 2; break;   // DoorCode_SetScroll_0
  case 0xBD16: scrolls[5] = 1; scrolls[6] = 1; break;   // DoorCode_SetScroll_1
  case 0xBD25: scrolls[29] = 1; break;   // DoorCode_SetScroll_2
  case 0xBD30: scrolls[2] = 2; scrolls[3] = 2; break;   // DoorCode_SetScroll_3
  case 0xBD3F: scrolls[0] = 0; scrolls[1] = 2; break;   // DoorCode_SetScroll_4
  case 0xBD50: scrolls[11] = 2; break;   // DoorCode_SetScroll_5
  case 0xBD5B: scrolls[28] = 0; scrolls[29] = 1; break;   // DoorCode_SetScroll_6
  case 0xBD6C: scrolls[4] = 0; break;   // DoorCode_SetScroll_7
  case 0xBD77: scrolls[32] = 2; scrolls[36] = 2; scrolls[37] = 2; break;   // DoorCode_SetScroll_8
  case 0xBD8A: scrolls[2] = 1; break;   // DoorCode_SetScroll_9
  case 0xBD95: scrolls[0] = 2; break;   // DoorCode_SetScroll_10
  case 0xBDA0: scrolls[6] = 2; scrolls[7] = 2; break;   // DoorCode_SetScroll_11
  case 0xBDAF: scrolls[1] = 1; scrolls[2] = 0; break;   // DoorCode_SetScroll_12
  case 0xBDC0: scrolls[1] = 1; scrolls[3] = 0; break;   // DoorCode_SetScroll_13
  case 0xBDD1: scrolls[0] = 0; scrolls[4] = 1; break;   // DoorCode_SetScroll_14
  case 0xBDE2: scrolls[2] = 1; scrolls[3] = 1; break;   // DoorCode_SetScroll_15
  case 0xBDF1: scrolls[0] = 2; scrolls[1] = 2; break;   // DoorCode_SetScroll_16
  case 0xBE00: scrolls[1] = 2; break;   // DoorCode_SetScroll_17
  case 0xBE0B: scrolls[15] = 2; scrolls[18] = 2; break;   // DoorCode_SetScroll_18
  case 0xBE1A: scrolls[6] = 2; break;   // DoorCode_SetScroll_19
  case 0xBE25: scrolls[0] = 2; scrolls[1] = 1; break;   // DoorCode_SetScroll_20
  case 0xBE36: scrolls[2] = 2; break;   // DoorCode_SetScroll_21
  case 0xBF9E: scrolls[3] = 0; scrolls[4] = 0; scrolls[6] = 1; scrolls[7] = 1; scrolls[8] = 1; break;   // DoorCode_SetScroll_22
  case 0xBFBB: scrolls[1] = 1; scrolls[2] = 1; scrolls[3] = 1; scrolls[4] = 2; scrolls[6] = 0; break;   // DoorCode_SetScroll_23
  case 0xBFDA: scrolls[0] = 1; scrolls[1] = 1; break;   // DoorCode_SetScroll_24
  case 0xBFE9: scrolls[1] = 0; scrolls[0] = 1; break;   // DoorCode_SetScroll_25
  case 0xBFFA: scrolls[10] = 2; break;   // DoorCode_SetScroll_26
  case 0xC005: scrolls[0] = 1; scrolls[2] = 0; break;   // DoorCode_SetScroll_27
  case 0xC016: scrolls[0] = 2; scrolls[2] = 2; break;   // DoorCode_SetScroll_28
  case 0xC025: scrolls[6] = 1; scrolls[7] = 1; scrolls[8] = 0; break;   // DoorCode_SetScroll_29
  case 0xC03A: scrolls[2] = 0; scrolls[3] = 1; break;   // DoorCode_SetScroll_30
  case 0xC04B: scrolls[7] = 2; break;   // DoorCode_SetScroll_31
  case 0xC056: scrolls[1] = 0; scrolls[2] = 1; break;   // DoorCode_SetScroll_32
  case 0xC067: scrolls[3] = 0; scrolls[0] = 1; break;   // DoorCode_SetScroll_33
  case 0xC078: scrolls[1] = 1; scrolls[4] = 0; break;   // DoorCode_SetScroll_34
  case 0xC089: scrolls[0] = 1; scrolls[1] = 0; scrolls[2] = 0; scrolls[3] = 0; break;   // DoorCode_SetScroll_35
  case 0xC0A2: scrolls[0] = 2; break;   // DoorCode_SetScroll_36
  case 0xC0AD: scrolls[0] = 1; scrolls[1] = 1; scrolls[4] = 0; break;   // DoorCode_SetScroll_37
  case 0xC0C2: scrolls[0] = 1; scrolls[3] = 0; break;   // DoorCode_SetScroll_38
  case 0xC0D3: scrolls[0] = 1; break;   // DoorCode_SetScroll_39
  case 0xC0DE: scrolls[0] = 1; scrolls[1] = 0; break;   // DoorCode_SetScroll_40
  case 0xC0EF: scrolls[24] = 1; break;   // DoorCode_SetScroll_41
  case 0xC0FA: scrolls[3] = 0; scrolls[2] = 1; break;   // DoorCode_SetScroll_42
  case 0xC10B: scrolls[14] = 0; break;   // DoorCode_SetScroll_43
  case 0xE1E8: scrolls[1] = 1; break;   // DoorCode_SetScroll_44
  case 0xE1F3: scrolls[0] = 2; break;   // DoorCode_SetScroll_45
  case 0xE1FE: scrolls[3] = 0; scrolls[4] = 1; break;   // DoorCode_SetScroll_46
  case 0xE20F: scrolls[41] = 1; break;   // DoorCode_SetScroll_47
  case 0xE21A: scrolls[40] = 2; scrolls[46] = 2; break;   // DoorCode_SetScroll_48
  case 0xE229: scrolls[6] = 0; scrolls[7] = 0; scrolls[8] = 0; scrolls[9] = 0; scrolls[10] = 0; scrolls[11] = 0; break;   // DoorCode_SetScroll_49
  case 0xE318: scrolls[10] = 0; scrolls[11] = 1; break;   // DoorCode_SetScroll_50
  case 0xE345: scrolls[0] = 0; scrolls[4] = 1; break;   // DoorCode_SetScroll_53
  case 0xE356: scrolls[0] = 0; scrolls[1] = 1; break;   // DoorCode_SetScroll_54
  case 0xE367: scrolls[9] = 0; scrolls[10] = 1; break;   // DoorCode_SetScroll_55
  case 0xE378: scrolls[0] = 0; scrolls[2] = 0; scrolls[1] = 1; break;   // DoorCode_SetScroll_56
  case 0xE38D: scrolls[1] = 1; break;   // DoorCode_SetScroll_57
  case 0xE398: scrolls[6] = 1; break;   // DoorCode_SetScroll_58
  case 0xE3A3: scrolls[4] = 0; break;   // DoorCode_SetScroll_59
  case 0xE3AE: scrolls[1] = 0; break;   // DoorCode_SetScroll_60
  case 0xE3B9: scrolls[4] = 0; scrolls[7] = 0; break;   // DoorCode_SetScroll_61
  case 0xE3C8: scrolls[2] = 0; scrolls[1] = 1; break;   // DoorCode_SetScroll_62
  case 0xE3D9: scrolls[0] = 2; scrolls[2] = 2; break;   // DoorCode_SetScroll_63
  case 0xE4C0: scrolls[0] = 2; scrolls[1] = 2; break;   // DoorCode_SetScroll_64
  case 0xE4CF: scrolls[24] = 1; scrolls[25] = 0; break;   // DoorCode_SetScroll_65
  case 0xB971:                                    // DoorCode_StartWreckedShipTreadmillWest
    SpawnAnimtiles(0x8275);                       /* addr_kAnimtiles_WreckedShipTradmillRight */
    SpawnHardcodedPlm(0x04, 0x09, 0xB64B);
    break;
  case 0xE1D8:                                    // DoorCode_StartWreckedSkipTreadmill_East
    SpawnAnimtiles(0x827B);                       /* addr_kAnimtiles_WreckedShipTradmillLeft */
    SpawnHardcodedPlm(0x04, 0x09, 0xB64F);
    break;
  case 0xE26C:                                    // DoorCode_SetupElevatubeFromSouth
  case 0xE291: {                                  // DoorCode_SetupElevatubeFromNorth
    bool south = door_def(door_def_ptr)->door_setup_code == 0xE26C;
    rmv_set(4, south ? 0xFF00 : 256);
    rmv_set(2, south ? 2496 : '@');
    rmv_set(6, south ? 0xFFE0 : ' ');
    CallSomeSamusCode(0);
    SpawnHardcodedPlm(0x01, 0x00, 0xB8F9);
    break;
  }
  case 0xE309: scrolls[0] = 2; scrolls[1] = 2;   // DoorCode_ResetElevatubeSouthExit
    /* sigue */
  case 0xE301: CallSomeSamusCode(1); break;       // DoorCode_ResetElevatubeNorthExit
  default: nt_unimpl_v("RunDoorSetupCode (código de puerta sin portar)", door_def(door_def_ptr)->door_setup_code); break;
  }
}

static void RunRoomSetupCode(void) {  // 0x8FE88F
  switch (room_state(roomdefroomstate_ptr)->room_setup_code) {
  case 0: break;
  case 0xC96E:                                    // RoomCode_SetCeresDoorSolid
    SpawnHardcodedPlm(0x0F, 0x26, 0xBA48);
    FxTypeFunc(0x2C);                             /* FxTypeFunc_2C_Haze */
    break;
  case 0xC976:                                    // RoomCode_CeresColorMathHdma
    FxTypeFunc(0x2C);
    break;
  case 0xC97B:                                    // RoomCode_CeresColorMathHdma_BgBase
    reg_BG12NBA = 102;
    FxTypeFunc(0x2C);
    hdma_data_table_in_ceres = 9;
    break;
  case 0x91BD:                                    // RoomSetup_ShakeDuringEscape
    earthquake_type = 6;
    earthquake_timer = -1;
    FxTypeFunc(0x20);
    break;
  case 0x91C9: FxTypeFunc(0x20); break;           // RoomSetup_ScrollingSkyLand
  case 0x91CE: FxTypeFunc(0x22); break;           // RoomSetup_ScrollingSkyOcean (RoomSetupAsm_ScrollingSkyOcean)
  case 0x91BB: case 0x91BC: case 0x91D3: case 0x91D4: case 0x91D5: case 0x91D6: case 0x91F4: case 0x91F5:
  case 0x91F6: case 0x91F7: case 0xC8C7: case 0xC8D0: case 0xC8D1: case 0xC8D2: case 0xC8DC: case 0xC91E:
    break;                                        // nullsub_132..147
  case 0x9194:                                    // RoomSetup_AfterSavingAnimals
    SpawnHardcodedPlm(0x3D, 0x0B, 0xBB30);
    earthquake_type = 24;
    earthquake_timer = -1;
    break;
  case 0x91A9: SpawnHardcodedPlm(0x10, 0x87, 0xB964); break;   // RoomSetup_AutoDestroyWallAfterEscape
  case 0x91B2: SpawnHardcodedPlm(0x0F, 0x0A, 0xB9ED); break;   // RoomSetup_TurnWallIntoShotblocks
  case 0x91D7:                                    // RoomSetup_RunStatueUnlockAnims
    SpawnAnimtiles(0x8558);                       /* estatuas de Tourian: Kraid, Phantoon, Draygon, Ridley */
    SpawnAnimtiles(0x854C);
    SpawnAnimtiles(0x855E);
    SpawnAnimtiles(0x8552);
    break;
  case 0xC8C8: SpawnEprojWithRoomGfx(addr_kEproj_PrePhantomRoom, 0); break;   // RoomCode_8FC8C8
  case 0xC8D3: SpawnHardcodedPlm(0x00, 0x00, 0xB8EB); break;   // RoomCode_SetupShaktoolRoomPlm
  case 0xC8DD:                                    // RoomCode_SetPauseCodeForDraygon
    /* PauseHook_DraygonRoom / UnpauseHook_DraygonRoom: ganchos de la pausa (el menú de pausa no está portado) */
    break;
  case 0xC90A: {                                  // RoomCode_SetCollectedMap
    uint16 v = (map_station_byte_array[area_index] | map_station_byte_array[area_index + 1] << 8) | 1;
    map_station_byte_array[area_index] = (uint8)v;
    map_station_byte_array[area_index + 1] = v >> 8;
    has_area_map = v;
    break;
  }
  case 0xC91F:                                    // RoomCode_SetZebesTimebombEvent
    SetEventHappened(0xE);
    earthquake_type = 18;
    earthquake_timer = -1;
    break;
  case 0xC953:                                    // RoomCode_Escape4_SetMediumHorizRoomShake
    SpawnHardcodedPlm(0x10, 0x10, 0xB968);
    /* sigue */
  case 0xC933:                                    // RoomCode_SetLightHorizRoomShake
    earthquake_type = room_state(roomdefroomstate_ptr)->room_setup_code == 0xC933 ? 18 : 21;
    rmv_set(2, earthquake_type);
    rmv_set(0, 0);
    earthquake_timer = -1;
    break;
  case 0xC946:                                    // RoomCode_SetMediumHorizRoomShake
    earthquake_type = 21;
    earthquake_timer = -1;
    break;
  default: nt_unimpl_v("RunRoomSetupCode (código de sala sin portar)", room_state(roomdefroomstate_ptr)->room_setup_code); break;
  }
}

void CreatePlmsExecuteDoorAsmRoomSetup(void) {  // 0x82EB6C
  const RoomStateN *S = room_state(roomdefroomstate_ptr);
  if (S->room_plm_header_ptr)
    SpawnRoomPlms(S->room_plm_header_ptr);
  RunDoorSetupCode();
  RunRoomSetupCode();
  if (elevator_flags)
    elevator_status = 2;
}

void LoadFXHeader(void) {  // 0x89AB82
  uint16 entry_ptr = room_layer3_asm_ptr;
  if (!entry_ptr)
    return;
  const FxDefN *FD = 0;
  for (unsigned i = 0; i < sizeof kFxDefs / sizeof kFxDefs[0]; i++)
    if (kFxDefs[i].addr == entry_ptr) { FD = &kFxDefs[i]; break; }
  if (!FD) { nt_unimpl("LoadFXHeader (FX sin preparar)"); return; }
  for (;;) {
    uint16 door_ptr = FD->door_ptr;
    if (!door_ptr)
      break;
    if (door_ptr == 0xFFFF)
      return;
    if (door_ptr == door_def_ptr)
      break;
    FD++;
  }
  fx_base_y_pos = FD->base_y_pos;
  fx_target_y_pos = FD->target_y_pos;
  fx_y_vel = FD->y_vel;
  fx_timer = FD->timer;
  fx_layer_blending_config_a = FD->default_layer_blend;
  fx_layer_blending_config_b = FD->layer3_layer_blend;
  fx_liquid_options = FD->fx_liquid_options_;
  if (FD->palette_blend) {
    int v4 = FD->palette_blend >> 1;
    target_palettes[25] = g_word_89AA02[v4];
    target_palettes[26] = g_word_89AA02[v4 + 1];
    target_palettes[27] = g_word_89AA02[v4 + 2];
  } else {
    target_palettes[27] = 0;
  }
  fx_type = FD->type;
  if (FD->type) {
    fx_tilemap_ptr = kFxTypeTilemapPtrs[FD->type >> 1];
    FxTypeFunc(FD->type);
  }
  for (int j = 0, bits = FD->palette_fx_bitset; j < 8; j++, bits >>= 1)
    if (bits & 1)
      SpawnPalfxObject(kAreaPalFxLists[8 * area_index + j]);
  for (int j = 0, bits = FD->animtiles_bitset; j < 8; j++, bits >>= 1)
    if (bits & 1)
      SpawnAnimtiles(kAreaAnimtilesLists[8 * area_index + j]);
}

void InitializeSpecialEffectsForNewRoom(void) {  // 0x8882C1
  earthquake_sfx_index = 0;
  earthquake_sfx_timer = 0;
  /* salas de jefes sin ruido de terremoto (0x9804, 0x96BA, 0xB32E, 0xB457, 0xDD58, 0xDEDE) */
  if (room_ptr == 0x9804 || room_ptr == 0x96BA || room_ptr == 0xB32E || room_ptr == 0xB457
      || room_ptr == 0xDD58 || room_ptr == 0xDEDE)
    earthquake_sfx_timer = -1;
  debug_disable_minimap = 0;
  /* (registros de HDMA del SNES: nada en la Genesis) */
  fx_y_subpos = 0;
  fx_y_pos = -1;
  lava_acid_y_subpos = 0;
  lava_acid_y_pos = -1;
  hud_bg3_xpos = 0;
  hud_bg3_ypos = 0;
  bg3_xpos = 0;
  bg3_ypos = 0;
  irq_enable_mode7 = 0;
  camera_distance_index = 0;
  tourian_entrance_statue_animstate = 0;
  tourian_entrance_statue_finished = 0;
  earthquake_timer = 0;
  phantom_related_layer_flag = 0;
  power_bomb_explosion_status = 0;
  power_bomb_flag = 0;
  power_bomb_explosion_radius = 0;
  fx_tilemap_ptr = 0;
  fx_type = 0;
  fx_base_y_subpos = 0x8000;
  fx_base_y_pos = 0;
  fx_target_y_pos = 0;
  fx_y_vel = 0;
  fx_liquid_options = 0;
  fx_timer = 0;
  tide_phase = 0;
  fx_y_suboffset = 0;
  fx_y_offset = 0;
  fx_layer_blending_config_a = 2;
  layer2_x_pos = 0;
  layer2_y_pos = 0;
  room_loading_irq_handler = 0;
  /* pause_hook / unpause_hook = PauseHook_Empty: la pausa la hace la Genesis a su manera.
     Registros de vídeo del SNES: los que la capa de la Genesis lee para imitar el color math y los fondos */
  reg_BG3HOFS = 0;
  reg_BG3VOFS = 0;
  reg_HDMAEN = 0;
  reg_COLDATA[0] = 32;
  reg_COLDATA[1] = 64;
  reg_COLDATA[2] = 0x80;
  reg_MOSAIC = 0;
  reg_TM = 19;
  reg_CGWSEL = 0;
  reg_CGADSUB = 0;
  reg_BG12NBA = 0;
  reg_BG34NBA = 4;
  reg_BG2SC = 73;
  reg_BG3SC = 90;
  gameplay_BG3SC = 90;
}

void SetLiquidPhysicsType(void) {  // 0x90EC??  (sm_90.c SetLiquidPhysicsType)
  uint16 r18 = Samus_GetBottom_R18();
  switch ((fx_type & 0xF) >> 1) {
  case 1: case 2:
    liquid_physics_type = (!(lava_acid_y_pos & 0x8000) && sign16(lava_acid_y_pos - r18)) ? 2 : 0;
    break;
  case 3:
    liquid_physics_type = (!(fx_y_pos & 0x8000) && sign16(fx_y_pos - r18) && !(fx_liquid_options & 4)) ? 1 : 0;
    break;
  default:
    liquid_physics_type = 0;
  }
}

/* ------------------------------------------------------------------ desplazamiento de la puerta ($80:AD1D) */
void CalculateBgScrollAndLayerPositionBlocks(void);
void UpdateBgGraphicsWhenScrolling(void);
uint8 CalculateLayer2Xpos(void);
uint8 CalculateLayer2Ypos(void);

void CalculateBgScrolls(void) {  // 0x80A37B
  reg_BG1HOFS = bg1_x_offset + layer1_x_pos;
  reg_BG1VOFS = bg1_y_offset + layer1_y_pos;
  reg_BG2HOFS = bg2_x_scroll + layer2_x_pos;
  reg_BG2VOFS = bg2_y_scroll + layer2_y_pos;
}

static void UpdateScrollVarsUpdateMap(void) {  // 0x80A3A0
  CalculateBgScrolls();
  UpdateBgGraphicsWhenScrolling();
}

static void UpdatePreviousLayerBlocks(void) {  // 0x80AE10
  previous_layer1_x_block = layer1_x_block;
  previous_layer2_x_block = layer2_x_block;
  previous_layer1_y_block = layer1_y_block;
  previous_layer2_y_block = layer2_y_block;
}

static void UpdateBgScrollOffsets(void) {  // 0x80AE29
  bg1_x_offset = reg_BG1HOFS - layer1_x_pos;
  bg1_y_offset = reg_BG1VOFS - layer1_y_pos;
  bg2_x_scroll = reg_BG2HOFS - layer1_x_pos;
  bg2_y_scroll = reg_BG2VOFS - layer1_y_pos;
}

static uint8 DoorTransition_Right(void) {  // 0x80AE7E
  uint16 v2 = door_transition_frame_counter;
  add_hilo(&samus_x_pos, &samus_x_subpos, PAIR32(samus_door_transition_speed, samus_door_transition_subspeed));
  samus_prev_x_pos = samus_x_pos;
  layer1_x_pos += 4;
  layer2_x_pos += 4;
  UpdateScrollVarsUpdateMap();
  door_transition_frame_counter = v2 + 1;
  if (v2 != 63)
    return 0;
  UpdateScrollVarsUpdateMap();
  return 1;
}

static uint8 DoorTransition_Left(void) {  // 0x80AEC2
  uint16 v2 = door_transition_frame_counter;
  add_hilo(&samus_x_pos, &samus_x_subpos, -IPAIR32(samus_door_transition_speed, samus_door_transition_subspeed));
  samus_prev_x_pos = samus_x_pos;
  layer1_x_pos -= 4;
  layer2_x_pos -= 4;
  UpdateScrollVarsUpdateMap();
  door_transition_frame_counter = v2 + 1;
  return v2 == 63;
}

static uint8 DoorTransition_Down(void) {  // 0x80AF02
  uint16 v6 = door_transition_frame_counter;
  if (door_transition_frame_counter) {
    if (door_transition_frame_counter < 0x39) {
      add_hilo(&samus_y_pos, &samus_y_subpos, PAIR32(samus_door_transition_speed, samus_door_transition_subspeed));
      samus_prev_y_pos = samus_y_pos;
      layer1_y_pos += 4;
      layer2_y_pos += 4;
      UpdateScrollVarsUpdateMap();
    }
  } else {
    uint16 v5 = reg_BG1VOFS, v4 = reg_BG2VOFS, v3 = layer1_y_pos, v2 = layer2_y_pos;
    layer1_y_pos -= 15;
    layer2_y_pos -= 15;
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    --previous_layer1_y_block;
    --previous_layer2_y_block;
    UpdateScrollVarsUpdateMap();
    layer2_y_pos = v2;
    layer1_y_pos = v3;
    reg_BG2VOFS = v4;
    reg_BG1VOFS = v5;
  }
  door_transition_frame_counter = v6 + 1;
  if ((uint16)(v6 + 1) < 0x39)
    return 0;
  UpdateScrollVarsUpdateMap();
  return 1;
}

static uint8 DoorTransition_Up(void) {  // 0x80AF89
  uint16 v6 = door_transition_frame_counter;
  if (door_transition_frame_counter) {
    add_hilo(&samus_y_pos, &samus_y_subpos, -IPAIR32(samus_door_transition_speed, samus_door_transition_subspeed));
    samus_prev_y_pos = samus_y_pos;
    layer1_y_pos -= 4;
    layer2_y_pos -= 4;
    if (door_transition_frame_counter >= 5) {
      UpdateScrollVarsUpdateMap();
    } else {
      reg_BG1HOFS = bg1_x_offset + layer1_x_pos;
      reg_BG1VOFS = bg1_y_offset + layer1_y_pos;
      reg_BG2HOFS = bg2_x_scroll + layer2_x_pos;
      reg_BG2VOFS = bg2_y_scroll + layer2_y_pos;
    }
  } else {
    uint16 v5 = reg_BG1VOFS, v4 = reg_BG2VOFS, v3 = layer1_y_pos, v2 = layer2_y_pos;
    layer1_y_pos -= 16;
    layer2_y_pos -= 16;
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    ++previous_layer1_y_block;
    ++previous_layer2_y_block;
    UpdateScrollVarsUpdateMap();
    layer2_y_pos = v2;
    layer1_y_pos = v3;
    reg_BG2VOFS = v4;
    reg_BG1VOFS = v5;
  }
  door_transition_frame_counter = v6 + 1;
  return v6 == 56;
}

void Irq_FollowDoorTransition(void) {  // 0x80AE4E
  uint8 done;
  switch (door_direction & 3) {
  case 0: done = DoorTransition_Right(); break;
  case 1: done = DoorTransition_Left(); break;
  case 2: done = DoorTransition_Down(); break;
  default: done = DoorTransition_Up(); break;
  }
  if (done) {
    layer1_x_pos = door_destination_x_pos;
    layer1_y_pos = door_destination_y_pos;
    door_transition_flag |= 0x8000;
  }
}

static void FixDoorsMovingUp(void) {  // 0x80AD1D
  door_transition_frame_counter = 0;
  CalculateBgScrollAndLayerPositionBlocks();
  UpdatePreviousLayerBlocks();
  ++previous_layer1_y_block;
  ++previous_layer2_y_block;
  DoorTransition_Up();
}

static void DoorTransitionScrollingSetup(void) {  // 0x80AD30
  layer1_x_pos = door_destination_x_pos;
  layer1_y_pos = door_destination_y_pos;
  switch (door_direction & 3) {
  case 0:                                         // 0x80AD4A
    CalculateLayer2Xpos();
    layer2_x_pos -= 256;
    CalculateLayer2Ypos();
    layer1_x_pos -= 256;
    UpdateBgScrollOffsets();
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    --previous_layer1_x_block;
    --previous_layer2_x_block;
    DoorTransition_Right();
    break;
  case 1:                                         // 0x80AD74
    CalculateLayer2Xpos();
    layer2_x_pos += 256;
    CalculateLayer2Ypos();
    layer1_x_pos += 256;
    UpdateBgScrollOffsets();
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    ++previous_layer1_x_block;
    ++previous_layer2_x_block;
    DoorTransition_Left();
    break;
  case 2:                                         // 0x80AD9E
    CalculateLayer2Xpos();
    CalculateLayer2Ypos();
    layer2_y_pos -= 224;
    layer1_y_pos -= 224;
    UpdateBgScrollOffsets();
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    --previous_layer1_y_block;
    --previous_layer2_y_block;
    DoorTransition_Down();
    break;
  default: {                                      // 0x80ADC8
    CalculateLayer2Xpos();
    uint16 v1 = layer1_y_pos;
    layer1_y_pos += 31;
    CalculateLayer2Ypos();
    layer2_y_pos += 224;
    layer1_y_pos = v1 + 256;
    UpdateBgScrollOffsets();
    door_destination_y_pos += 32;
    CalculateBgScrollAndLayerPositionBlocks();
    UpdatePreviousLayerBlocks();
    ++previous_layer1_y_block;
    ++previous_layer2_y_block;
    --layer1_y_pos;
    DoorTransition_Up();
    break;
  }
  }
}

/* ------------------------------------------------------------------ estados 9, 10 y 11 ($82:E169) */
static void DrawSamusEnemiesAndProjectiles_(void) { DrawSamusEnemiesAndProjectiles(); }

static void EnsureSamusDrawnEachFrame(void) {  // 0x82DFC7
  if (!elevator_flags)
    Samus_DrawWhenNotAnimatingOrDying();
}

static void EnemiesAndDraw(void) {
  DetermineWhichEnemiesToProcess();
  EnemyMain();
  DrawSamusEnemiesAndProjectiles_();
  EnsureSamusDrawnEachFrame();
}

static uint8 CallDoorTransitionFunction_Async(void);

static uint8 GameState_10_LoadingNextRoom_Async(void) {  // 0x82E1B7
  door_transition_flag_enemies = 1;
  door_transition_flag_elevator_zebetites = 1;
  debug_disable_minimap = 0;
  save_station_lockout_flag = 0;
  EnemiesAndDraw();
  LoaadDesinationRoomCreBitset();
  for (int i = 127; i >= 0; i--) {
    target_palettes[i] = 0;
    target_palettes[i + 128] = 0;
  }
  target_palettes[9] = palette_buffer[9];
  target_palettes[10] = palette_buffer[10];
  target_palettes[13] = palette_buffer[13];
  target_palettes[14] = palette_buffer[14];
  target_palettes[17] = palette_buffer[17];
  target_palettes[18] = palette_buffer[18];
  target_palettes[19] = palette_buffer[19];
  target_palettes[29] = palette_buffer[29];
  if (!((previous_cre_bitset | cre_bitset) & 1)) {
    target_palettes[20] = palette_buffer[20];
    target_palettes[21] = palette_buffer[21];
    target_palettes[22] = palette_buffer[22];
    target_palettes[23] = palette_buffer[23];
    target_palettes[28] = palette_buffer[28];
    if (timer_status) {
      target_palettes[209] = palette_buffer[209];
      target_palettes[210] = palette_buffer[210];
      target_palettes[212] = palette_buffer[212];
      target_palettes[221] = palette_buffer[221];
      DrawTimer();
    }
  }
  CallSomeSamusCode(0x1D);                      // ClearSoundsWhenGoingThroughDoor
  QueueSfx2_Max15(0x71);
  debug_disable_sounds = -1;
  door_transition_function = FUNC16(DoorTransitionFunction_WaitForSoundsToFinish);
  ++game_state;
  return 0;
}

static uint8 DoorTransitionFunction_Wait48frames(void) {  // 0x82E19F: el ascensor baja un poco antes de cambiar de sala
  if ((--downwards_elevator_delay_timer & 0x8000) != 0) {
    ++game_state;
    return GameState_10_LoadingNextRoom_Async();
  }
  DetermineWhichEnemiesToProcess();
  EnemyMain();
  DrawSamusEnemiesAndProjectiles();
  EnsureSamusDrawnEachFrame();
  return 0;
}

static uint8 DoorTransitionFunction_HandleElevator(void) {  // 0x82E17D
  if (!elevator_flags) {
    ++game_state;
    return GameState_10_LoadingNextRoom_Async();
  }
  CallSomeSamusCode(0);
  if ((elevator_direction & 0x8000) != 0) {
    ++game_state;
    return GameState_10_LoadingNextRoom_Async();
  }
  downwards_elevator_delay_timer = 48;
  door_transition_function = FUNC16(DoorTransitionFunction_Wait48frames);
  return DoorTransitionFunction_Wait48frames();
}

static uint8 DoorTransitionFunction_WaitForSoundsToFinish(void) {  // 0x82E29E
  EnemiesAndDraw();
  if (!((sfx_writepos[0] - sfx_readpos[0]) & 0xF) && !((sfx_writepos[1] - sfx_readpos[1]) & 0xF)
      && !((sfx_writepos[2] - sfx_readpos[2]) & 0xF))
    door_transition_function = FUNC16(DoorTransitionFunction_FadeOutScreen);
  return 0;
}

static uint8 DoorTransitionFunction_FadeOutScreen(void) {  // 0x82E2DB
  if (AdvancePaletteFadeForAllPalettes_0xc())
    door_transition_function = FUNC16(DoorTransitionFunction_LoadDoorHeaderEtc);
  else
    EnemiesAndDraw();
  return 0;
}

static uint8 DoorTransitionFunction_LoadDoorHeaderEtc(void) {  // 0x82E2F7
  LoadDoorHeader();
  WaitUntilEndOfVblankAndClearHdma();             // sub_8882AC: se apagan los objetos de HDMA
  DisableHdmaObjects();
  irqhandler_next_handler = 8;
  door_transition_function = FUNC16(DoorTransitionFunction_ScrollScreenToAlignment);
  return 0;
}

static uint8 DoorTransitionFunction_ScrollScreenToAlignment(void) {  // 0x82E310
  if (door_direction & 2) {
    if ((uint8)layer1_x_pos) {
      if (layer1_x_pos & 0x80) ++layer1_x_pos; else --layer1_x_pos;
      CalculateLayer2PosAndScrollsWhenScrolling();
      return 0;
    }
  } else if ((uint8)layer1_y_pos) {
    if (layer1_y_pos & 0x80) ++layer1_y_pos; else --layer1_y_pos;
    CalculateLayer2PosAndScrollsWhenScrolling();
    return 0;
  }
  CalculateLayer2PosAndScrollsWhenScrolling();
  door_transition_function = FUNC16(DoorTransitionFunction_FixDoorsMovingUp);
  return 0;
}

static uint8 DoorTransitionFunction_FixDoorsMovingUp(void) {  // 0x82E353
  if ((door_direction & 3) == 3)
    FixDoorsMovingUp();
  door_transition_function = FUNC16(DoorTransitionFunction_SetupNewRoom);
  return 0;
}

static uint8 DoorTransitionFunction_SetupNewRoom(void) {  // 0x82E36E
  const DoorDefN *D = door_def(door_def_ptr);
  if (elevator_door_properties_orientation & 0xF)            // SaveMapExploredifElevator ($82:DF99)
    SetElevatorsAsUsed();
  if (D->door_bitflags & 0x40)                                // cambio de área: guardar el mapa explorado
    SaveExploredMapTilesToSaved();
  LoadRoomHeader();
  LoadStateHeader();
  if (D->door_bitflags & 0x40)                                // LoadMapExploredIfElevator ($82:DFB6)
    LoadMirrorOfExploredMapTiles();
  InitializeSpecialEffectsForNewRoom();
  LoadLevelScrollAndCre();
  door_transition_function = FUNC16(DoorTransitionFunction_SetupScrolling);
  return 0;
}

static uint8 DoorTransitionFunction_SetupScrolling(void) {  // 0x82E38E
  reg_BG2HOFS = 0;
  reg_BG2VOFS = 0;
  scrolling_finished_hook = 0;
  if ((door_direction & 3) == 2)
    ++reg_BG1VOFS;
  if ((door_direction & 3) != 3)
    door_transition_frame_counter = 0;
  DoorTransitionScrollingSetup();
  door_transition_function = FUNC16(DoorTransitionFunction_PlaceSamusLoadTiles);
  return 0;
}

static uint8 DoorTransitionFunction_PlaceSamusLoadTiles(void) {  // 0x82E3C0
  samus_x_pos = layer1_x_pos + (uint8)samus_x_pos;
  samus_prev_x_pos = samus_x_pos;
  samus_y_pos = layer1_y_pos + (uint8)samus_y_pos;
  samus_prev_y_pos = samus_y_pos;
  door_transition_flag = 0;
  irqhandler_next_handler = (door_direction & 3) == 2 ? 16 : 22;
  /* tiles del tileset y de la CRE: la Genesis los carga con la sala ya convertida */
  if (g_hooks.room_gfx) g_hooks.room_gfx();
  /* paleta del tileset (DecompressToMem a 7E:C200 = target_palettes[0..127]), preparada en el PC */
  {
    uint8 gs = room_state(roomdefroomstate_ptr)->graphics_set;
    unsigned i = 0;
    while (i < sizeof kTilesetPaletteSets && kTilesetPaletteSets[i] != gs) i++;
    if (i == sizeof kTilesetPaletteSets) nt_unimpl("PlaceSamusLoadTiles (paleta de tileset sin preparar)");
    else for (int c = 0; c < 128; c++) target_palettes[c] = kTilesetPalettes[i * 128 + c];
  }
  if ((door_direction & 3) == 3)
    irqhandler_next_handler = 16;
  door_transition_function = FUNC16(DoorTransitionFunction_LoadMoreThings_Async);
  return 0;
}

uint8 WaitForNMI_Async(void) {  // 0x808338
  if (coroutine_completion_flags) {
    coroutine_completion_flags = 0;
    return 0;
  }
  waiting_for_nmi = 1;
  coroutine_completion_flags = 1;
  return 1;
}

static void ResetProjectileData_(void) { ResetProjectileData(); }

static uint8 DoorTransitionFunction_LoadMoreThings_Async(void) {  // 0x82E413
  switch (coroutine_state_1) {
  case 0:
    /* LoadEnemyGfxToVram: los tiles de los enemigos los sube la Genesis (g_hooks.room_gfx) */
    LoadRoomMusic();
    for (int i = 17; i >= 0; i--) eproj_id[i] = 0;              // ClearEprojs
    for (int i = 5; i >= 0; i--) animtiles_ids[i] = 0;          // ClearAnimtiles
    ClearPaletteFXObjects();
    for (int i = 39; i >= 0; i--) plm_header_ptr[i] = 0;        // ClearPLMs
    plm_item_gfx_index = 0;
    CreatePlmsExecuteDoorAsmRoomSetup();
    LoadFXHeader();
    SpawnDoorClosingPLM();
    UpdateBeamTilesAndPalette();
    LoadColorsForSpritesBeamsAndEnemies();
    LoadAndInitializeEnemies();                                 // LoadEnemies + InitializeEnemies
    ResetProjectileData_();
    Samus_LoadSuitTargetPalette();
    /* ClearFxTilemap, tilemap de FX y bg_data de la sala: la Genesis (g_hooks.room_gfx) */
    /* fallthrough */
    /* esperar a que termine el desplazamiento de la puerta (lo lleva la interrupción de cada cuadro).
       Como en el decomp, al volver se continúa dentro del bucle, en la espera (case 1). */
    while (!(door_transition_flag & 0x8000)) {
  case 1:
      if (WaitForNMI_Async()) { coroutine_state_1 = 1; return 1; }
    }
    palette_buffer[196] = 15328;
    nt_pal_dirty(196, 196);
    SpawnBG3ScrollHdmaObject();
    hdma_objects_enable_flag |= 0x8000;
    /* fallthrough */
  case 2:
    PlmHandler();                                              // PlmHandler_Async (en Ceres nunca espera)
    if (!(door_direction & 2)) {
      if (door_direction & 3)
        samus_x_pos &= 0xFFF8;
      else
        samus_x_pos |= 7;
    }
    door_transition_function = FUNC16(DoorTransition_C_HandleAnimTiles);
  }
  coroutine_state_1 = 0;
  return 0;
}

static uint8 DoorTransition_C_HandleAnimTiles(void) {  // 0x82E659
  AnimtilesHandler();
  door_transition_function = FUNC16(DoorTransition_WaitForMusicToClear);
  return 0;
}

static uint8 DoorTransition_WaitForMusicToClear(void) {  // 0x82E664
  if (!HasQueuedMusic()) {
    door_transition_function = FUNC16(DoorTransition_HandleTransition);
    LoadNewMusicTrackIfChanged();
  }
  return 0;
}

static uint8 DoorTransition_HandleTransition(void) {  // 0x82E6A2
  if ((samus_x_pos & 0xF0) == 16)
    samus_x_pos = (samus_x_pos | 0xF) + 8;
  else if ((samus_x_pos & 0xF0) == 224)
    samus_x_pos = (samus_x_pos & 0xFFF0) - 8;
  if ((samus_y_pos & 0xF0) == 16)
    samus_y_pos = (samus_y_pos | 0xF) + 8;
  /* tablas de HDMA de la ventana: no hay en la Genesis */
  irqhandler_next_handler = room_loading_irq_handler ? room_loading_irq_handler : 4;
  if (elevator_flags)
    CallSomeSamusCode((elevator_direction & 0x8000) ? 0 : 7);
  SetLiquidPhysicsType();
  door_transition_function = FUNC16(DoorTransition_FadeInScreenAndFinish);
  reg_INIDISP |= 0x1F;                          /* brillo al máximo */
  return 0;
}

static uint8 DoorTransition_FadeInScreenAndFinish(void) {  // 0x82E737
  AnimtilesHandler();
  DetermineWhichEnemiesToProcess();
  EnemyMain();
  EprojRunAll();
  DrawSamusEnemiesAndProjectiles_();
  EnsureSamusDrawnEachFrame();
  QueueEnemyBG2TilemapTransfers();
  if (AdvancePaletteFadeForAllPalettes_0xc()) {
    debug_disable_sounds = 0;
    CallSomeSamusCode(0x1C);                    // PlaySpinJumpSoundIfSpinJumping
    door_transition_flag_elevator_zebetites = 0;
    door_transition_flag_enemies = 0;
    game_state = 8;
  }
  return 0;
}

static uint8 CallDoorTransitionFunction_Async(void) {
  switch (door_transition_function) {
  case FUNC16(DoorTransitionFunction_HandleElevator): return DoorTransitionFunction_HandleElevator();
  case FUNC16(DoorTransitionFunction_Wait48frames): return DoorTransitionFunction_Wait48frames();
  case FUNC16(DoorTransitionFunction_WaitForSoundsToFinish): return DoorTransitionFunction_WaitForSoundsToFinish();
  case FUNC16(DoorTransitionFunction_FadeOutScreen): return DoorTransitionFunction_FadeOutScreen();
  case FUNC16(DoorTransitionFunction_LoadDoorHeaderEtc): return DoorTransitionFunction_LoadDoorHeaderEtc();
  case FUNC16(DoorTransitionFunction_ScrollScreenToAlignment): return DoorTransitionFunction_ScrollScreenToAlignment();
  case FUNC16(DoorTransitionFunction_FixDoorsMovingUp): return DoorTransitionFunction_FixDoorsMovingUp();
  case FUNC16(DoorTransitionFunction_SetupNewRoom): return DoorTransitionFunction_SetupNewRoom();
  case FUNC16(DoorTransitionFunction_SetupScrolling): return DoorTransitionFunction_SetupScrolling();
  case FUNC16(DoorTransitionFunction_PlaceSamusLoadTiles): return DoorTransitionFunction_PlaceSamusLoadTiles();
  case FUNC16(DoorTransitionFunction_LoadMoreThings_Async): return DoorTransitionFunction_LoadMoreThings_Async();
  case FUNC16(DoorTransition_C_HandleAnimTiles): return DoorTransition_C_HandleAnimTiles();
  case FUNC16(DoorTransition_WaitForMusicToClear): return DoorTransition_WaitForMusicToClear();
  case FUNC16(DoorTransition_HandleTransition): return DoorTransition_HandleTransition();
  case FUNC16(DoorTransition_FadeInScreenAndFinish): return DoorTransition_FadeInScreenAndFinish();
  default: nt_unimpl("CallDoorTransitionFunction_Async (función de puerta sin portar)"); return 0;
  }
}

uint8 GameState_9_HitDoorBlock(void) {  // 0x82E169
  return CallDoorTransitionFunction_Async();
}

uint8 GameState_11_LoadingNextRoom_Async(void) {  // 0x82E288
  uint8 r = CallDoorTransitionFunction_Async();
  if (r)
    return r;
  if (timer_status)
    DrawTimer();
  return 0;
}

uint8 GameState_10(void) { return GameState_10_LoadingNextRoom_Async(); }

/* ------------------------------------------------------------------ interrupciones de línea ($80:966E)
 * En el SNES dividen la pantalla (HUD arriba, sala abajo) y, durante una puerta, mueven la cámara una vez por
 * cuadro. Aquí solo queda lo que cambia el estado del juego: qué manejador sigue y el paso de la puerta. La
 * Genesis no necesita la división (el HUD va en la ventana). */
static uint16 irq_vtimer;
static uint8 irq_disabled;

static void IrqHandler_SetResult(uint16 a, uint16 y) { cur_irq_handler = a; irq_vtimer = y; }

static void Vector_IRQ(void) {
  uint16 a;
  switch (cur_irq_handler >> 1) {
  case 0: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a, 0); break;
  case 1: irq_disabled = 1; IrqHandler_SetResult(0, 0); break;
  case 2: IrqHandler_SetResult(6, 31); break;
  case 3: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a ? a : 4, 0); break;
  case 4: IrqHandler_SetResult(10, 31); break;
  case 5: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a ? a : 8, 0); break;
  case 6: IrqHandler_SetResult(14, 31); break;
  case 7: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a ? a : 12, 0); break;
  case 8: IrqHandler_SetResult(18, 31); break;
  case 9:
    if (!(door_transition_flag & 0x8000)) Irq_FollowDoorTransition();
    IrqHandler_SetResult(20, 216);
    break;
  case 10: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a ? a : 16, 0); break;
  case 11: IrqHandler_SetResult(24, 31); break;
  case 12:
    if (!(door_transition_flag & 0x8000)) Irq_FollowDoorTransition();
    IrqHandler_SetResult(26, 160);
    break;
  case 13: a = irqhandler_next_handler; irqhandler_next_handler = 0; IrqHandler_SetResult(a ? a : 22, 0); break;
  default: nt_unimpl("Vector_IRQ (manejador desconocido)"); break;
  }
}

void irq_reset(void) { irq_vtimer = 0; irq_disabled = 0; }

void run_irqs_for_frame(void) {   /* las líneas 1..225 del cuadro, como DrawFrameToPpu */
  /* igual que recorrer las 225 líneas y disparar en la línea irq_vtimer, pero saltando de una a la siguiente */
  uint16 next = 0;
  while (!irq_disabled) {
    uint16 t = irq_vtimer;
    if (t < next || t > 224)
      break;
    next = t + 1;
    Vector_IRQ();
  }
}

/* ------------------------------------------------------------------ código principal de la sala ($8F:E8BD) */
/* cielo de la superficie ($88:AFA3): las filas del cielo nuevas que entran a la pantalla las sube la Genesis */
static void RoomMainAsm_ScrollingSky(void) {
  if (time_is_frozen_flag)
    scrolling_sky_bg2_indirect_hdma[0] = scrolling_sky_bg2_indirect_hdma[1] = 0;
  else
    reg_BG2VOFS = layer1_y_pos;
}

uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal);

static void RoomCode_GenRandomExplodes(uint16 x, uint16 y) {  // 0x8FC1A9
  static const uint8 kSprite[8] = { 3, 3, 9, 12, 12, 18, 18, 21 };
  static const uint8 kSfx[8] = { 0x24, 0, 0, 0x25, 0, 0, 0, 0 };
  uint16 v1 = NextRandom() & 0xF;                 /* el original indexa con X basura: se toma lo del port del PC */
  if (v1 < 8 && kSfx[v1])
    QueueSfx2_Max6(kSfx[v1]);
  CreateSpriteAtPos(x, y, kSprite[v1 & 7], 0);
}

static void RoomCode_GenRandomExplodes_4th(void) {  // 0x8FC183
  if (!time_is_frozen_flag && (nmi_frame_counter_word & 3) == 0) {
    uint16 rnd = NextRandom();
    RoomCode_GenRandomExplodes(layer1_x_pos + (uint8)rnd, layer1_y_pos + HI8(rnd));
  }
}

static void RoomCode_GenRandomExplodes_Nonblank(void) {  // 0x8FC131
  if (!time_is_frozen_flag && (nmi_frame_counter_word & 1) == 0) {
    uint16 rnd = NextRandom();
    uint16 x = layer1_x_pos + (uint8)rnd;
    uint16 y = layer1_y_pos + HI8(rnd);
    uint16 prod = (uint8)(y >> 4) * (uint8)room_width_in_blocks;
    if ((level_get(prod + (x >> 4)) & 0x3FF) != 255)
      RoomCode_GenRandomExplodes(x, y);
  }
}

void RunRoomMainCode(void) {  // 0x8FE8BD
  switch (room_main_code_ptr) {
  case 0: case 0xE524: break;                     // nullsub_148
  case 0xE51F:                                    // RoomCode_CeresElevatorShaft ($89:ACC3): solo en la huida
    if (ceres_status & 0x8000) {
      if ((int16)(112 - samus_x_pos) < 0 && (int16)(144 - samus_x_pos) >= 0 && sign16(samus_y_pos - 128) &&
          !sign16(samus_y_pos - 75) && !samus_y_speed && !samus_y_subspeed && game_state == 8) {
        CallSomeSamusCode(2);                       /* Samus en el ascensor: se acaba la huida de Ceres */
        screen_fade_delay = 0;
        screen_fade_counter = 0;
        game_state = 32;
      }
      /* el pozo se balancea: la matriz de modo 7 sigue la tabla $89:AD5F */
      uint16 t = (room_main_asm_variables[2] | room_main_asm_variables[3] << 8) - 1;
      if ((int16)t < 0) {
        uint16 i = room_main_asm_variables[0] | room_main_asm_variables[1] << 8;
        int v0 = (uint16)(6 * i) >> 1;
        t = g_word_89AD5F[v0];
        reg_M7B = g_word_89AD5F[v0 + 1];
        reg_M7C = -reg_M7B;
        reg_M7A = g_word_89AD5F[v0 + 2];
        reg_M7D = reg_M7A;
        int16 v1;
        if ((int16)i < 0)
          v1 = (i == 0x8001) ? 0 : (int16)(i - 1);
        else
          v1 = (i == 67) ? -32700 : (int16)(i + 1);
        room_main_asm_variables[0] = (uint8)v1;
        room_main_asm_variables[1] = (uint16)v1 >> 8;
      }
      room_main_asm_variables[2] = (uint8)t;
      room_main_asm_variables[3] = t >> 8;
    }
    break;
  case 0xE525:                                    // RoomCode_SpawnCeresFallingDebris: solo en la huida
    if (ceres_status) {
      uint16 t = (room_main_asm_variables[0] | room_main_asm_variables[1] << 8) - 1;
      room_main_asm_variables[0] = (uint8)t;
      room_main_asm_variables[1] = t >> 8;
      if ((int16)t < 0) {
        static const uint16 kTab[16] = { 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0,
                                         0xd0, 0xe0, 0xf0, 0x110, 0x130, 0x150, 0x170, 0x190 };
        room_main_asm_variables[0] = 8;
        room_main_asm_variables[1] = 0;
        SpawnEprojWithRoomGfx((random_number & 0x8000) ? 0x9742 : 0x9734, kTab[random_number & 0xF]);
      }
    }
    break;
  case 0xE571:                                    // RoomCode_HandleCeresRidleyGetaway
    if (ceres_status & 1) HandleCeresRidleyGetawayCutscene();
    break;
  case 0xC116:                                    // RoomCode_ScrollingSkyLand_ ($88:AF8D)
  case 0xC11B:                                    // RoomCode_ScrollingSkyOcean_ ($88:AF99)
    RoomMainAsm_ScrollingSky();
    break;
  case 0xC120:                                    // RoomCode_ScrollingSkyLand_Shakes
    RoomMainAsm_ScrollingSky();
    RoomCode_GenRandomExplodes_Nonblank();
    earthquake_timer |= 0x8000;
    break;
  case 0xC124:                                    // RoomCode_ExplodeShakes
    RoomCode_GenRandomExplodes_Nonblank();
    earthquake_timer |= 0x8000;
    break;
  case 0xC183: case 0xE5A0: RoomCode_GenRandomExplodes_4th(); break;   // RoomCode_GenRandomExplodes_4th(_)
  case 0xC1E6:                                    // RoomCode_ScrollRightDachora
    if (scrolls[11] == 2 && layer1_y_pos < 0x500 && layer1_x_pos < 0x380)
      layer1_x_pos += 3;
    break;
  case 0xE2B6: {                                  // RoomCode_Elevatube
    samus_x_pos = 128;
    samus_x_subpos = 0;
    int32 amt = (int32)(int16)rmv_get(4) << 8;
    uint32 pos = rmv_get(0) | (uint32)rmv_get(2) << 16;
    rmv_set(2, (uint32)(amt + pos) >> 16);
    rmv_set(0, rmv_get(0) + (uint16)amt);
    Samus_MoveDown_NoSolidColl(amt);
    if ((uint16)(rmv_get(6) + rmv_get(4) + 3616) < 0x1C41)
      rmv_set(4, rmv_get(4) + rmv_get(6));
    break;
  }
  case 0xE57C:                                    // RoomCode_ShakeScreenHorizDiag
    if (rmv_get(0)) {
      rmv_set(0, rmv_get(0) - 1);
      if (!rmv_get(0)) earthquake_type = 18;
    } else if (NextRandom() < 0x200) {
      rmv_set(0, '*');
      earthquake_type = 23;
    }
    RoomCode_GenRandomExplodes_4th();
    break;
  case 0xE5A4:                                    // RoomCode_ShakeScreenHorizDiagStrong
    if (rmv_get(0)) {
      rmv_set(0, rmv_get(0) - 1);
      if (!rmv_get(0)) rmv_set(2, 21);
    } else if (NextRandom() < 0x180) {
      rmv_set(0, '*');
      rmv_set(2, 26);
    }
    earthquake_type = rmv_get(2);
    RoomCode_GenRandomExplodes_4th();
    break;
  case 0xE8CD: {                                  // RoomCode_CrocomireShaking
    EnemyData *E0 = gEnemyData(0), *E1 = gEnemyData(0x40);
    if (!(E0->properties & 0x200)) {
      if (E0->ai_var_A == 64) {
        reg_BG1VOFS = E1->ai_var_D + bg1_y_offset + layer1_y_pos;
      } else if (E0->ai_var_B & 0x400) {
        uint16 d = --E1->ai_var_D;
        if (sign16(d + 7)) d = d - 2 * (d + 7);
        reg_BG1VOFS += d;
        reg_BG2VOFS = d - 48;
      } else if (E0->ai_var_C == 34 && E0->ai_var_D) {
        --E0->ai_var_D;
        if (E0->ai_var_D & 1) layer1_x_pos -= 4; else layer1_x_pos += 4;
      }
    }
    break;
  }
  case 0xE950: {                                  // RoomCode_RidleyRoomShaking
    static const int16 kX[8] = { 0, 2, 2, 2, 0, -2, -2, -2 };
    static const int16 kY[8] = { -2, -2, 0, 2, 2, 2, 0, -2 };
    EnemyData *E4 = gEnemyData(0x100);
    if (E4->ai_var_A) {
      int v = --E4->ai_var_A;
      reg_BG1HOFS += kX[v];
      reg_BG2HOFS += kX[v];
      reg_BG1VOFS += kY[v];
      reg_BG2VOFS += kY[v];
    }
    break;
  }
  default: nt_unimpl_v("RunRoomMainCode (código de sala sin portar)", room_main_code_ptr); break;
  }
}

/* fundido de algunas líneas de paleta (Torizo, $82:DAF7): a = máscara de las líneas (bit n: línea n de 16 colores) */
static void sub_82DB41(void) {  // 0x82DB41
  uint16 v0 = g_word_7EC404;
  do {
    int v1 = v0 >> 1;
    if (target_palettes[v1] != palette_buffer[v1]) {
      uint16 v2 = CalculateNthTransitionColorFromXtoY(palette_change_num, palette_buffer[v1], target_palettes[v1]);
      v0 = g_word_7EC404;
      palette_buffer[g_word_7EC404 >> 1] = v2;
    }
    v0 += 2;
    g_word_7EC404 = v0;
  } while ((v0 & 0x1F) != 0);
}

uint8 sub_82DB0C(uint16 a) {  // 0x82DB0C
  if ((uint16)(palette_change_denom + 1) >= palette_change_num) {
    g_word_7EC404 = 0;
    while (a) {
      if (!(a & 1))
        g_word_7EC404 += 32;
      else
        sub_82DB41();
      a >>= 1;
    }
    ++palette_change_num;
    return 0;
  } else {
    palette_change_num = 0;
    return 1;
  }
}

uint8 sub_82DAF7(uint16 a) {  // 0x82DAF7
  palette_change_denom = 12;
  return sub_82DB0C(a);
}
