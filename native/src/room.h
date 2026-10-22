/* MegaMetroid — motor nativo: datos de la sala actual (docs/31).
 * Bloque: tipo en los 4 bits altos, como el SNES; BTS aparte. */
#ifndef ROOM_H
#define ROOM_H
#include "nt.h"

/* Nivel de la sala (docs/37). En el SNES se descomprime en la RAM (7F:0002 level_data, 7F:6402 BTS: 0x3200 bloques).
   Aquí el nivel de cada sala ya viene descomprimido en la ROM (mkdata.py, kLevels) y en la RAM solo quedan los
   bloques que el juego cambia (puertas, bloques rotos...): un mapa de bits de bloques cambiados y una tabla con su
   valor. Así una sala grande no ocupa 38 KB de los 64 KB de la Genesis. */
#define ROOM_MAX_BLOCKS 0x3200
#define LVL_OV_N 512                   /* bloques cambiados a la vez, como mucho (potencia de 2) */
typedef struct { uint16 idx, lvl; uint8 bts, used; } LvlMod;
extern uint16 lvl_index;               /* nivel de la sala (kLevels); 0xFFFF = ninguno */
#define LVL_INTRO_MB   0xFFF0          /* niveles de los recuerdos de la intro (kIntroLevel*, mkdata.py) */
#define LVL_INTRO_BABY 0xFFF1
extern uint8 lvl_mod[ROOM_MAX_BLOCKS / 8];
extern LvlMod lvl_ov[LVL_OV_N];
extern const uint16 *lvl_rom;          /* del nivel actual (lvl_attach) */
extern const uint8 *bts_rom;
extern uint16 lvl_blocks;
extern uint16 lvl_fast;                /* lvl_blocks si no hay bloques cambiados (todo sale de la ROM); si no, 0 */

void lvl_attach(void);                 /* punteros del nivel lvl_index (después de cargar la sala o el estado guardado) */
void lvl_load(uint16 index);           /* sala nueva: su nivel, sin bloques cambiados */
LvlMod *lvl_find_slow(uint16 i);
/* bloque cambiado i: casi siempre en su casilla (i & (LVL_OV_N - 1)); si no, la búsqueda lineal */
static inline LvlMod *lvl_find(uint16 i) {
  LvlMod *m = &lvl_ov[i & (LVL_OV_N - 1)];
  return (m->idx == i && m->used) ? m : lvl_find_slow(i);
}
void level_set(uint16 i, uint16 v);
void bts_set(uint16 i, uint8 v);
/* página del mapeador SSF2 de cada nivel (Genesis: ventana 0x380000; en el PC no hace nada) */
void nt_map_level_page(uint8 page);
/* un PLM cambió el bloque i: la Genesis lo redibuja si se ve (en el PC no hace nada) */
void nt_block_changed(uint16 i);

/* lo común: dentro de la sala y sin cambiar (una comparación, un bit y la ROM) */
static inline uint8 lvl_is_mod(uint16 i) { return (lvl_mod[i >> 3] >> (i & 7)) & 1; }
static inline uint16 level_get(uint16 i) {
  if (i < lvl_fast) return lvl_rom[i];          /* lo común: la sala sin cambios */
  if (i < lvl_blocks) return lvl_is_mod(i) ? lvl_find(i)->lvl : lvl_rom[i];
  if (i >= ROOM_MAX_BLOCKS) return 0x8000;
  return lvl_is_mod(i) ? lvl_find(i)->lvl : 0x8000;
}
static inline uint8 bts_get(uint16 i) {
  if (i < lvl_fast) return bts_rom[i];
  if (i < lvl_blocks) return lvl_is_mod(i) ? lvl_find(i)->bts : bts_rom[i];
  if (i >= ROOM_MAX_BLOCKS) return 0;
  return lvl_is_mod(i) ? lvl_find(i)->bts : 0;
}
/* bloque i para las colisiones: fuera de la sala (Samus por encima del techo: la fila se calcula con 8 bits) el
   SNES lee el resto de su búfer, que la carga de sala deja en 0x8000 (sólido) */
static inline uint16 level_block(uint16 i) { return level_get(i); }

/* puertas de la sala actual: la entrada i de la lista de puertas (door_list_pointer) */
uint16 room_door_def_ptr(uint16 i);
/* room_definition_ptr de una definición de puerta (bit 15 = puerta de verdad; si no, ascensor) */
uint16 door_def_room_ptr(uint16 door_def);

#include "../gen/tables.h"
const RoomDefN *room_def(uint16 addr);
const RoomStateN *room_state(uint16 addr);
const DoorDefN *door_def(uint16 addr);
#endif
