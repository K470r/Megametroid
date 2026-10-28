/* MegaMetroid — el motor en el PC sin mirar al SNES, con el estado inicial de la Genesis (mkinit) y la entrada de la
 * prueba: suma del estado en cada cuadro, igual que md_state_sum de la Genesis (MD_STATESUM). Si las dos coinciden,
 * el código propio de la Genesis (ensamblador, caminos con NT_SSF2) hace lo mismo que el C del PC.
 * Uso: simsum ref.bin rom.smc cuadro_inicial(97) desde n   -> una suma por línea (cuadros desde..desde+n-1) */
#define main check_main
#include "check.c"
#undef main
#include "viewer.h"

static uint16_t sum;
static void sb(uint8_t b) { sum = (uint16_t)((sum << 1) | (sum >> 15)) ^ b; }
static void sw(uint16_t w) { sb(w >> 8); sb(w & 0xFF); }
static uint16_t state_sum(void) {
  sum = 0;
  for (const VarMap *v = g_vars_map; v->name; v++)
    for (unsigned i = 0; i < v->count && v->ptr != (void *)palette_buffer; i++) {
      if (v->size == 2) sw(((uint16_t *)v->ptr)[i]); else sb(((uint8_t *)v->ptr)[i]);
    }
  for (int k = 0; k < ENEMY_COUNT; k++) {
    EnemyData *E = &enemy_data[k];
    uint16_t *w = &E->enemy_ptr, *w2 = &E->ai_var_A;
    for (int i = 0; i < 23; i++) sw(w[i]);
    sb(E->bank); sb(E->field_2F);
    for (int i = 0; i < 8; i++) sw(w2[i]);
  }
  for (int k = 0; k < ENEMY_COUNT; k++) {
    uint16_t *w = &enemy_spawn_data[k].some_flag;
    for (int i = 0; i < 32; i++) sw(w[i]);
  }
  for (int i = 0; i < 0x1000; i++) sw(enemy_ram7800[i]);
  sw(lvl_index);
  for (unsigned i = 0; i < sizeof lvl_mod; i++) sb(lvl_mod[i]);
  for (unsigned i = 0; i < LVL_OV_N; i++) { sw(lvl_ov[i].idx); sw(lvl_ov[i].lvl); sb(lvl_ov[i].bts); sb(lvl_ov[i].used); }
  return sum;
}

int main(int argc, char **argv) {
  ref = fopen(argv[1], "rb");
  FILE *rf = fopen(argv[2], "rb"); fseek(rf, 0, SEEK_END); romsz = ftell(rf); fseek(rf, 0, SEEK_SET);
  rom = malloc(romsz); if (fread(rom, 1, romsz, rf) != romsz) return 1; fclose(rf);
  if (romsz % 0x8000) { memmove(rom, rom + (romsz % 0x8000), romsz - romsz % 0x8000); }
  long f0 = atol(argv[3]), from = atol(argv[4]), n = atol(argv[5]);
  irq_reset();
  if (!read_frame(f0)) return 1;
  load_all(ram_cur);
  extern uint8 nt_viewer, nt_viewer_place;
  nt_viewer = 1; nt_viewer_place = 1;               /* como la Genesis (md_main.c) */
  for (long k = 0; k < from + n; k++) {
    if (k >= from) printf("%ld %04X\n", k, state_sum());
    if (!read_frame(f0 + 1 + k)) break;             /* la entrada del cuadro: la del registro siguiente */
    game_frame(viewer_input(joy_from_bot(in_cur)));
  }
  return 0;
}
