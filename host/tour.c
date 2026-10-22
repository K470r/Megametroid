/* MegaMetroid — recorrido del visor de salas en el PC (sin la Genesis): carga el estado del SNES de una grabación
 * (ya en Zebes) y pasa por todas las salas con START + derecha, mostrando lo que falta portar en cada una.
 * Uso: tour ref.bin rom.smc cuadro [salas] [cuadros_por_sala] */
#define main check_main
#define nt_unimpl check_nt_unimpl
#include "check.c"
#undef main
#undef nt_unimpl
/* lo que falta portar, por sala (cada mensaje una vez por sala) */
static char room_seen[64][96]; static int room_nseen;
extern uint32_t nt_unimpl_val;
void nt_unimpl(const char *what) {
  char key[96];
  snprintf(key, sizeof key, nt_unimpl_val ? "%.80s %X" : "%.80s", what, nt_unimpl_val);
  for (int i = 0; i < room_nseen; i++) if (!strcmp(room_seen[i], key)) return;
  if (room_nseen < 64) strcpy(room_seen[room_nseen++], key);
}
#include "viewer.h"

int main(int argc, char **argv) {
  ref = fopen(argv[1], "rb");
  FILE *rf = fopen(argv[2], "rb"); fseek(rf, 0, SEEK_END); romsz = ftell(rf); fseek(rf, 0, SEEK_SET);
  rom = malloc(romsz); if (fread(rom, 1, romsz, rf) != romsz) return 1; fclose(rf);
  long from = atol(argv[3]);
  int nrooms = argc > 4 ? atoi(argv[4]) : 300, per = argc > 5 ? atoi(argv[5]) : 240;
  irq_reset();
  if (!read_frame(from)) { fprintf(stderr, "no puedo leer el cuadro %ld\n", from); return 1; }
  load_all(ram_cur);
  nt_viewer = 1;
  { extern uint8 nt_viewer_enemies, nt_viewer_place;   /* TOUR_ENEMIES: con los enemigos (como la Genesis) */
    if (getenv("TOUR_ENEMIES")) { nt_viewer_enemies = 1; nt_viewer_place = 1; } }
  if (getenv("TOUR_ITEMS")) { unsigned it = strtoul(getenv("TOUR_ITEMS"), 0, 16); collected_items |= it; equipped_items |= it; }
  if (getenv("TOUR_MIS")) samus_missiles = samus_max_missiles = atoi(getenv("TOUR_MIS"));
  int first = getenv("TOUR_FIRST") ? atoi(getenv("TOUR_FIRST")) : -1;
  cur_frame = from;
  for (int i = 0; i < 400; i++, cur_frame++) game_frame(viewer_input(0));   /* que Samus salga de la nave */
  for (int n = 0; n < nrooms; n++) {
    if (first >= 0 && n == 0) { extern uint16 viewer_debug_next; viewer_debug_next = first; }
    uint16_t seq[3] = { kButton_Start, kButton_Start | kButton_Right, 0 };
    for (int k = 0; k < 3; k++)
      for (int f = 0; f < 3; f++, cur_frame++) game_frame(viewer_input(seq[k]));
    for (int f = 0; f < per; f++, cur_frame++) { samus_health = samus_max_health; game_frame(viewer_input(0)); }
    printf("sala %d: %04X estado %04X juego %d samus %d,%d", n, room_ptr, roomdefroomstate_ptr, game_state,
           samus_x_pos, samus_y_pos);
    for (int i = 0; i < room_nseen; i++) printf(" | %s", room_seen[i]);
    printf("\n");
    room_nseen = 0;
  }
  return 0;
}
