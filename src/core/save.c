/* MegaMetroid — motor nativo: las partidas guardadas (docs/65). Escrito a mano desde snesrev/sm: SaveToSram
 * ($81:8000), LoadFromSram ($81:8085), PackMapToSave / UnpackMapFromSave ($81:834B / $81:82E4).
 *
 * La SRAM es la del SNES, byte por byte (8 KB): las sumas de cada ranura en 0x0000 (y al revés en 0x0008), copiadas
 * en 0x1FF0 / 0x1FF8; la última ranura elegida en 0x1FEC (y al revés en 0x1FEE); cada ranura (0x10, 0x66C, 0xCC8) es
 * la RAM del SNES 7E:D7C0-DE1B: el equipo (copia de 7E:09A2-0A01), los eventos, objetos, puertas y estaciones
 * (7E:D820-D91B, con la estación y el área donde se guardó) y el mapa explorado comprimido (7E:D91C: solo los bytes
 * que pueden tener algo, las listas de $81:82D6). En el motor esa RAM son variables nativas: se lee y se escribe por
 * dirección (nt_ram_w, nt_ram_set_w) y el mapa se arma al escribir. La plataforma guarda los bytes (nt_sram_read /
 * nt_sram_write: en la Genesis, la SRAM del cartucho; en el PC, memoria o un archivo). */
#pragma GCC optimize("Os")                     /* (lo común de la ROM está lleno; corre poco) */
#include "samus.h"
#include "game.h"
#include "nmath.h"
#include "tables.h"

void nt_sram_read(uint16 off, uint8 *dst, uint16 n);
void nt_sram_write(uint16 off, const uint8 *src, uint16 n);
void SaveExploredMapTilesToSaved(void);

#define SLOT_WORDS ((0xDE1C - 0xD7C0) / 2)      /* 814 */
static const uint16 kSlotOff[3] = { 0x10, 0x66C, 0xCC8 };   // kOffsetToSaveSlot ($81:812B)

/* el byte b (0-1791) de explored_map_tiles_saved como en el SNES (palabras: el byte par es el bajo) */
static uint8 ExploredB(uint16 b) { return (uint8)(explored_map_tiles_saved[b >> 1] >> ((b & 1) << 3)); }
static void ExploredSetB(uint16 b, uint8 v) {
  uint16 *w = &explored_map_tiles_saved[b >> 1];
  *w = (b & 1) ? (uint16)((*w & 0x00FF) | v << 8) : (uint16)((*w & 0xFF00) | v);
}

/* el byte k del mapa comprimido (7E:D91C + k): el de explored_map_tiles_saved que dice kSaveMapIdxB (las listas de
   $81:82D6, mkdata.py); los demás, 0 */
static uint16 MapIdx(uint16 k) { return nt_paged_w(kSaveMapIdxB, 0x8000 + 2 * k); }
static uint8 PackedMapByte(uint16 k) { return k < SAVE_MAP_BYTES ? ExploredB(MapIdx(k)) : 0; }

/* la palabra de la ranura en la RAM del SNES a (7E:D7C0-DE1A) */
static uint16 SlotWord(uint16 a) {
  if (a < 0xD820) return nt_ram_w(0x09A2 + (a - 0xD7C0));   /* el equipo */
  if (a == 0xD916) return load_station_index;             /* sram_save_station_index */
  if (a == 0xD918) return area_index;                     /* sram_area_index */
  if (a < 0xD91C) return nt_ram_w(a);
  uint16 k = a - 0xD91C;
  return PackedMapByte(k) | PackedMapByte(k + 1) << 8;
}

static void SramW(uint16 off, uint16 v) { uint8 b[2] = { (uint8)v, (uint8)(v >> 8) }; nt_sram_write(off, b, 2); }
static uint16 SramR(uint16 off) { uint8 b[2]; nt_sram_read(off, b, 2); return b[0] | b[1] << 8; }

void SaveToSram(uint16 a) {  // 0x818000
  uint16 slot = a & 3;
  if (slot > 2) return;
  SaveExploredMapTilesToSaved();
  sram_save_station_index = load_station_index;
  sram_area_index = area_index;
  uint16 sum = 0, off = kSlotOff[slot];
  uint8 buf[64];
  uint16 nb = 0;
  for (uint16 i = 0; i < SLOT_WORDS; i++) {
    uint16 v = SlotWord(0xD7C0 + 2 * i);
    sum += v;
    buf[nb++] = (uint8)v; buf[nb++] = (uint8)(v >> 8);
    if (nb == sizeof buf || i == SLOT_WORDS - 1) { nt_sram_write(off, buf, nb); off += nb; nb = 0; }
  }
  SramW(2 * slot, sum);
  SramW(0x1FF0 + 2 * slot, sum);
  SramW(8 + 2 * slot, (uint16)~sum);
  SramW(0x1FF8 + 2 * slot, (uint16)~sum);
}

/* 1 si la ranura tiene una partida: la suma (o la copia de arriba) coincide */
static uint8 SlotValid(uint16 slot) {
  uint16 sum = 0, off = kSlotOff[slot];
  uint8 buf[64];
  for (uint16 i = 0; i < SLOT_WORDS; i += 32) {
    uint16 n = SLOT_WORDS - i < 32 ? SLOT_WORDS - i : 32;
    nt_sram_read(off + 2 * i, buf, 2 * n);
    for (uint16 j = 0; j < n; j++) sum += buf[2 * j] | buf[2 * j + 1] << 8;
  }
  return (sum == SramR(2 * slot) && (uint16)~sum == SramR(8 + 2 * slot)) ||
         (sum == SramR(0x1FF0 + 2 * slot) && (uint16)~sum == SramR(0x1FF8 + 2 * slot));
}

uint8 LoadFromSram(uint16 a) {  // 0x818085: 0 cargada, 1 vacía (o rota)
  uint16 slot = a & 3;
  if (slot > 2 || !SlotValid(slot)) {
    load_station_index = 0;
    area_index = 0;
    return 1;
  }
  for (uint16 i = 0; i < 896; i++) explored_map_tiles_saved[i] = 0;   // UnpackMapFromSave ($81:82E4)
  uint16 off = kSlotOff[slot];
  uint8 buf[64];
  for (uint16 i = 0; i < SLOT_WORDS; i += 32) {
    uint16 n = SLOT_WORDS - i < 32 ? SLOT_WORDS - i : 32;
    nt_sram_read(off + 2 * i, buf, 2 * n);
    for (uint16 j = 0; j < n; j++) {
      uint16 a2 = 0xD7C0 + 2 * (i + j), v = buf[2 * j] | buf[2 * j + 1] << 8;
      if (a2 < 0xD820) nt_ram_set_w(0x09A2 + (a2 - 0xD7C0), v);
      else if (a2 < 0xD91C) nt_ram_set_w(a2, v);
      else {
        uint16 k = a2 - 0xD91C;
        if (k < SAVE_MAP_BYTES) ExploredSetB(MapIdx(k), (uint8)v);
        if (k + 1 < SAVE_MAP_BYTES) ExploredSetB(MapIdx(k + 1), (uint8)(v >> 8));
      }
      if (a2 == 0xD916) load_station_index = v;
      if (a2 == 0xD918) area_index = v;
    }
  }
  return 0;
}

/* para el menú de las partidas (la Genesis): energía, tanques y tiempo de una ranura; 0 si está vacía */
uint8 SramSlotInfo(uint16 slot, uint16 *health, uint16 *max_health, uint16 *hours, uint16 *minutes) {
  if (slot > 2 || !SlotValid(slot)) return 0;
  uint16 off = kSlotOff[slot];
  *health = SramR(off + (0x09C2 - 0x09A2));
  *max_health = SramR(off + (0x09C4 - 0x09A2));
  *hours = SramR(off + (0x09E0 - 0x09A2));
  *minutes = SramR(off + (0x09DE - 0x09A2));
  return 1;
}

/* la última ranura usada (FileSelectMenu: 7E:1FEC/1FEE de la SRAM) */
uint16 SramLastSlot(void) {
  uint16 s = SramR(0x1FEC);
  return ((uint16)~s == SramR(0x1FEE) && s < 3) ? s : 0;
}
void SramSetLastSlot(uint16 s) { SramW(0x1FEC, s); SramW(0x1FEE, (uint16)~s); }

/* Genesis: empezar el juego con la partida de la ranura (después de md_init, con el estado del comienzo en Ceres).
   Lo que sigue en el SNES a elegir la partida (GameOptionsMenu_4_StartGame, $82:EEB4): con loading_game_state 0x1F
   se empieza en Ceres (el estado inicial, tal cual), con 0x22 la huida de Ceres (estado 34) y con 5 en Zebes: el
   mapa de la partida (estado 5, filemap.c) y después el estado 6, cargar en la estación. */
void StartLoadedGame(uint16 slot) {
  selected_save_slot = slot;
  if (LoadFromSram(slot))
    return;
  if (loading_game_state == 0x22) {
    game_state = 34;
    cinematic_function = FUNC16(CinematicFunctionBlackoutFromCeres);
  } else if (loading_game_state != 0x1F) {
    game_state = 5;
    menu_index = 0;
  }
}

/* la firma de 7E:1FE0 de la SRAM: "supermetroid" al terminar el juego (el SNES suma entonces el cuarto juego de
   demos) y "madadameyohn" al encender sin ninguna partida (VerifySRAM, $80:8261; la Genesis la llama al arrancar) */
void CopySuperMetroidString(void) {  // 0x80824F
  nt_sram_write(0x1FE0, (const uint8 *)"supermetroid", 12);
}
void VerifySRAM(void) {  // 0x808261 (num_demo_sets: las demos del título no están)
  if (!SlotValid(0) && !SlotValid(1) && !SlotValid(2))
    nt_sram_write(0x1FE0, (const uint8 *)"madadameyohn", 12);
}
