/* MegaMetroid — el motor en el PC sin mirar al SNES (como simsum) hasta el cuadro K: imprime cada variable
 * ("nombre índice valor") para compararla con la RAM de la Genesis detenida en el mismo cuadro (STOP_AT=K).
 * Uso: vardump ref.bin rom.smc 97 K */
#define main check_main
#include "check.c"
#undef main
#include "../src/viewer.h"
int main(int argc, char **argv) {
  ref = fopen(argv[1], "rb");
  FILE *rf = fopen(argv[2], "rb"); fseek(rf, 0, SEEK_END); romsz = ftell(rf); fseek(rf, 0, SEEK_SET);
  rom = malloc(romsz); if (fread(rom, 1, romsz, rf) != romsz) return 1; fclose(rf);
  if (romsz % 0x8000) { memmove(rom, rom + (romsz % 0x8000), romsz - romsz % 0x8000); }
  long f0 = atol(argv[3]), K = atol(argv[4]);
  irq_reset();
  if (!read_frame(f0)) return 1;
  load_all(ram_cur);
  extern uint8 nt_viewer, nt_viewer_place;
  nt_viewer = 1; nt_viewer_place = 1;
  for (long k = 0; k < K; k++) {
    if (!read_frame(f0 + 1 + k)) break;
    game_frame(viewer_input(joy_from_bot(in_cur)));
  }
  for (const VarMap *v = g_vars_map; v->name; v++)
    for (unsigned i = 0; i < v->count; i++)
      printf("%s %u %u %X\n", v->name, i, v->size, v->size == 2 ? ((uint16_t *)v->ptr)[i] : ((uint8_t *)v->ptr)[i]);
  for (int k = 0; k < ENEMY_COUNT; k++) {
    uint16_t *w = &enemy_data[k].enemy_ptr;
    for (int i = 0; i < 23; i++) printf("enemy_data %u 2 %X\n", k * 32 + i, w[i]);
  }
  for (int k = 0; k < ENEMY_COUNT; k++) {
    uint16_t *w = &enemy_spawn_data[k].some_flag;
    for (int i = 0; i < 32; i++) printf("enemy_spawn_data %u 2 %X\n", k * 32 + i, w[i]);
  }
  for (int i = 0; i < 0x1000; i++) printf("enemy_ram7800 %u 2 %X\n", i, enemy_ram7800[i]);
  printf("lvl_index 0 2 %X\n", lvl_index);
  for (unsigned i = 0; i < sizeof lvl_mod; i++) printf("lvl_mod %u 1 %X\n", i, lvl_mod[i]);
  return 0;
}
