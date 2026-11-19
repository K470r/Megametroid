/* MegaMetroid — prueba de las partidas (docs/65): llena el equipo, los eventos y el mapa explorado, guarda en una
 * ranura, borra todo, carga y compara; además, una ranura rota no carga. Uso: savetest (sale 0 si todo coincide). */
#include <stdio.h>
#include <stdlib.h>
#include "samus.h"
#include "game.h"
#include "tables.h"
uint16 nt_ram_w(uint16 a);
void nt_ram_set_w(uint16 a, uint16 x);
void nt_sram_write(uint16 off, const uint8 *src, uint16 n);

void nt_unimpl(const char *w) { printf("sin portar: %s\n", w); exit(2); }
static uint16 eq[48], ev[126], map[896];
static void snap(void) {
  for (int i = 0; i < 48; i++) eq[i] = nt_ram_w(0x09A2 + 2 * i);
  for (int i = 0; i < 126; i++) ev[i] = nt_ram_w(0xD820 + 2 * i);
  for (int i = 0; i < 896; i++) map[i] = explored_map_tiles_saved[i];
}
int main(void) {
  srand(7);
  for (int i = 0; i < 48; i++) nt_ram_set_w(0x09A2 + 2 * i, rand());
  for (int i = 0; i < 126; i++) nt_ram_set_w(0xD820 + 2 * i, rand());
  area_index = 2; load_station_index = 3;
  for (int i = 0; i < 128; i++) map_tiles_explored[2 * i] = rand(), map_tiles_explored[2 * i + 1] = rand();
  for (int i = 0; i < 896; i++) explored_map_tiles_saved[i] = rand();
  SaveToSram(1);
  /* lo que el SNES no guarda del mapa (bytes fuera de las listas) se pierde: se compara contra lo empaquetado */
  uint16 keep[896] = { 0 };
  for (int k = 0; k < SAVE_MAP_BYTES; k++) {
    uint16 b = kSaveMapIdxB[2 * k] | kSaveMapIdxB[2 * k + 1] << 8;
    keep[b >> 1] |= (b & 1) ? 0xFF00 : 0x00FF;
  }
  for (int i = 0; i < 896; i++) explored_map_tiles_saved[i] &= keep[i];
  snap();
  uint16 hp, mhp, hh, mm;
  if (!SramSlotInfo(1, &hp, &mhp, &hh, &mm)) { printf("info: ranura vacía\n"); return 1; }
  printf("ranura B: energía %u/%u, %02u:%02u\n", hp, mhp, hh, mm);
  if (SramSlotInfo(0, &hp, &mhp, &hh, &mm)) { printf("ranura A debería estar vacía\n"); return 1; }
  for (int i = 0; i < 48; i++) nt_ram_set_w(0x09A2 + 2 * i, 0);
  for (int i = 0; i < 126; i++) nt_ram_set_w(0xD820 + 2 * i, 0);
  for (int i = 0; i < 896; i++) explored_map_tiles_saved[i] = 0x5555;
  area_index = 0; load_station_index = 0;
  if (LoadFromSram(1)) { printf("no cargó\n"); return 1; }
  int bad = 0;
  for (int i = 0; i < 48; i++) if (nt_ram_w(0x09A2 + 2 * i) != eq[i]) { printf("equipo %04X\n", 0x09A2 + 2 * i); bad++; }
  for (int i = 0; i < 126; i++) {
    uint16 a = 0xD820 + 2 * i;
    if (a == 0xD916 || a == 0xD918) continue;
    if (nt_ram_w(a) != ev[i]) { printf("7E:%04X %04X != %04X\n", a, nt_ram_w(a), ev[i]); bad++; }
  }
  for (int i = 0; i < 896; i++) if (explored_map_tiles_saved[i] != map[i]) { if (bad < 40) printf("mapa %d\n", i); bad++; }
  if (area_index != 2 || load_station_index != 3) { printf("área %u estación %u\n", area_index, load_station_index); bad++; }
  uint8 x = 0x77; nt_sram_write(0x66C + 100, &x, 1); nt_sram_write(0x66C + 101, &x, 1);
  if (!LoadFromSram(1)) { printf("una ranura rota cargó\n"); bad++; }
  printf("%s (%d diferencias)\n", bad ? "MAL" : "BIEN", bad);
  return bad != 0;
}
