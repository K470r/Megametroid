/* MegaMetroid — motor nativo: visor de salas de Zebes.
 *
 * Después de aterrizar en Zebes se puede recorrer todo el planeta: START + izquierda/derecha lleva a la sala
 * anterior/siguiente (kViewerRooms, ordenadas por área). El cambio es una transición de puerta normal del juego
 * (como si Samus tocara una puerta que entra a esa sala), así la sala se carga con todo lo del motor. En el visor
 * no hay enemigos (salvo la nave de Samus) ni pausa. */
#include "samus.h"
#include "game.h"
#include "room.h"
#include "viewer.h"

uint8 nt_viewer;                          /* la Genesis lo enciende (md_main); en el PC (comparación con el SNES), no */
uint8 nt_viewer_place;                    /* la Genesis: al llegar a la sala, Samus a un lugar libre en la pantalla */
uint8 nt_viewer_enemies;                  /* la Genesis: con los enemigos de Crateria (ya portados y dibujados) */
static int16 viewer_idx = -1;
static uint8 place_pending;
uint16 viewer_debug_next = 0xFFFF;       /* pruebas: la próxima sala del visor (índice), puesta desde el emulador */

/* La transición de ascensor deja a Samus donde estaba en la pantalla anterior, que en la sala nueva puede caer fuera
   o dentro de una pared. Se busca, desde el centro de la pantalla hacia afuera, una columna con tres bloques de aire
   sobre un piso sólido. */
static uint8 blk_type(int16 bx, int16 by) {
  if (bx < 0 || by < 0 || bx >= room_width_in_blocks || by >= room_height_in_blocks)
    return 8;
  return level_get(by * room_width_in_blocks + bx) >> 12;
}
static void viewer_place_samus(void) {
  int16 cx = (layer1_x_pos + 128) >> 4, cy = (layer1_y_pos + 112) >> 4;
  for (int16 d = 0; d < 16; d++) {
    for (int16 s = -1; s <= 1; s += 2) {
      int16 bx = cx + s * d;
      for (int16 dy = 0; dy < 14; dy++) {
        int16 by = cy + ((dy & 1) ? -(dy >> 1) - 1 : dy >> 1);   /* 0, -1, 1, -2, 2... */
        if (blk_type(bx, by) == 0 && blk_type(bx, by - 1) == 0 && blk_type(bx, by - 2) == 0 &&
            blk_type(bx, by + 1) == 8) {
          samus_x_pos = samus_prev_x_pos = bx * 16 + 8;
          samus_y_pos = samus_prev_y_pos = (by + 1) * 16 - samus_y_radius;
          samus_x_subpos = samus_y_subpos = 0;
          return;
        }
      }
      if (!d) break;
    }
  }
}

uint8 viewer_keep_enemy(uint16 enemy_ptr) {
  /* la nave y los ascensores (Elevator, $A3:D73F) siempre: sin el ascensor Samus se queda arriba del cuarto al bajar
     (Brinstar verde, 9AD9). Con los enemigos: Crateria y Brinstar (salvo la sala de Kraid, A59F, sin portar) */
  return !nt_viewer || area_index == 6 || enemy_ptr == 0xD07F || enemy_ptr == 0xD0BF || enemy_ptr == 0xD73F
         || (nt_viewer_enemies && (area_index == 0 || (area_index == 1 && room_ptr != 0xA59F)));
}

static int16 viewer_find(uint16 room) {
  for (uint16 i = 0; i < sizeof kViewerRooms / sizeof kViewerRooms[0]; i++)
    if (kViewerRooms[i].room == room) return i;
  return -1;
}

uint16 viewer_input(uint16 joy) {
  static uint16 prev;
  static uint8 start_held, start_used;
  uint16 newk = joy & ~prev;
  prev = joy;
  if (!nt_viewer || area_index > 5)
    return joy;
  samus_health = samus_max_health;               /* en el visor Samus no muere (púas, lava, ácido) */
  if (place_pending && game_state == 8) {
    place_pending = 0;
    viewer_place_samus();
  }
  if (game_state == 8 && (newk & (kButton_Left | kButton_Right)) && (joy & kButton_Start)) {
    int16 n = sizeof kViewerRooms / sizeof kViewerRooms[0];
    if (viewer_idx < 0 || kViewerRooms[viewer_idx].room != room_ptr) {
      int16 i = viewer_find(room_ptr);
      if (i >= 0 || viewer_idx < 0) viewer_idx = i < 0 ? 0 : i;   /* sala fuera de la lista (tras un ascensor): la última */
    }
    viewer_idx += (newk & kButton_Right) ? 1 : -1;
    if (viewer_debug_next != 0xFFFF) { viewer_idx = viewer_debug_next; viewer_debug_next = 0xFFFF; }
    if (viewer_idx < 0) viewer_idx = n - 1;
    if (viewer_idx >= n) viewer_idx = 0;
    if (elevator_flags | elevator_status) {      /* saltar de sala en el ascensor: soltarlo (como al llegar, $A3:95BC) */
      elevator_flags = elevator_status = 0;
      CallSomeSamusCode(0xB);
    }
    door_transition_function = FUNC16(DoorTransitionFunction_HandleElevator);
    door_def_ptr = kViewerRooms[viewer_idx].door;
    game_state = 9;
    place_pending = nt_viewer_place;
  }
  /* START + izquierda/derecha cambia de sala; START solo es la pausa (docs/53): en el juego START llega al soltarlo
     si no se usó para cambiar de sala (un cuadro); fuera del juego (pausa), tal cual */
  if (game_state != 8)
    return joy;
  if (joy & kButton_Start) {
    if (newk & (kButton_Left | kButton_Right)) start_used = 1;
    start_held = 1;
    return joy & ~kButton_Start;
  }
  if (start_held) {
    uint8 u = start_used;
    start_held = start_used = 0;
    if (!u) return joy | kButton_Start;
  }
  return joy;
}
