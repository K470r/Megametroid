/* MegaMetroid — motor nativo: enemigos (banco $A0 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_a0.c. */
#include "samus.h"
#include "enemies.h"

EnemyData enemy_data[ENEMY_COUNT];

static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }

CheckEnemyColl_Result Samus_CheckSolidEnemyColl(int32 amt) {  // 0xA0A8F0
  uint16 r18 = (uint16)((uint32)amt >> 16), r20 = (uint16)amt;
  uint16 px = 0, py = 0;
  if (!interactive_enemy_indexes_write_ptr)
    return (CheckEnemyColl_Result){ 0, amt };
  switch (samus_collision_direction & 3) {
  case 0: {   /* restar con préstamo y luego un decremento más (como el 65816) */
    px = samus_x_pos - r18;
    bool z = samus_x_subpos == r20;
    if (samus_x_subpos < r20)
      z = px-- == 1;
    if (!z)
      --px;
    py = samus_y_pos;
    break;
  }
  case 1: {
    px = samus_x_pos + r18;
    bool z = (uint16)(samus_x_subpos + r20) == 0;
    if ((uint16)(samus_x_subpos + r20) < samus_x_subpos)
      z = px++ == 0xFFFF;
    if (!z)
      ++px;
    py = samus_y_pos;
    break;
  }
  case 2: {
    py = samus_y_pos - r18;
    bool z = samus_y_subpos == r20;
    if (samus_y_subpos < r20)
      z = py-- == 1;
    if (!z)
      --py;
    px = samus_x_pos;
    break;
  }
  case 3: {
    py = samus_y_pos + r18;
    bool z = (uint16)(samus_y_subpos + r20) == 0;
    if ((uint16)(samus_y_subpos + r20) < samus_y_subpos)
      z = py++ == 0xFFFF;
    if (!z)
      ++py;
    px = samus_x_pos;
    break;
  }
  }
  uint16 pw = samus_x_radius, ph = samus_y_radius;
  collision_detection_index = 0;
  for (int i = 0;; i++) {
    uint16 v7 = interactive_enemy_indexes[i];
    if (v7 == 0xFFFF)
      break;
    collision_detection_index = v7;
    EnemyData *ED = gEnemyData(v7);
    if (!ED->frozen_timer && !(ED->properties & 0x8000))
      continue;
    uint16 v11 = abs16(ED->x_pos - px);
    if (!(v11 < ED->x_width || (uint16)(v11 - ED->x_width) < pw))
      continue;
    uint16 v14 = abs16(ED->y_pos - py);
    if (!(v14 < ED->y_height || (uint16)(v14 - ED->y_height) < ph))
      continue;
    int16 v18;
    switch (samus_collision_direction & 3) {
    case 0: {
      uint16 e = ED->x_width + ED->x_pos;
      v18 = samus_x_pos - samus_x_radius - e;
      break;
    }
    case 1:
      v18 = ED->x_pos - ED->x_width - (samus_x_radius + samus_x_pos);
      break;
    case 2: {
      uint16 e = ED->y_height + ED->y_pos;
      v18 = samus_y_pos - samus_y_radius - e;
      break;
    }
    default:
      v18 = ED->y_pos - ED->y_height - (samus_y_radius + samus_y_pos);
      break;
    }
    if (!v18) {
      samus_y_subpos = 0;
      enemy_index_colliding_dirs[samus_collision_direction & 3] = collision_detection_index;
      return (CheckEnemyColl_Result){ 0xFFFF, 0 };
    }
    if (v18 > 0) {
      enemy_index_colliding_dirs[samus_collision_direction & 3] = collision_detection_index;
      return (CheckEnemyColl_Result){ 0xFFFF, SHL16(v18) };
    }
  }
  return (CheckEnemyColl_Result){ 0, amt };
}
