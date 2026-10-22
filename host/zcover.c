/* MegaMetroid — cobertura de dibujo de los enemigos (docs/42): carga el estado del SNES de una grabación (ya en
 * Zebes), entra a cada sala de la lista por su puerta (como check con HL_WARP) y juega cuadros con un mando al azar,
 * Samus sin morir. Con MM_DRAWLOG se registra todo lo que se dibuja: de ahí salen los sprites de cada sala para la
 * Genesis (mkzobj.py).
 * Uso: zcover ref.bin rom.smc cuadro "sala:puerta,..." cuadros_por_sala semilla */
#define main check_main
#include "check.c"
#undef main

static uint32_t rnd_s;
static uint32_t rnd(void) { rnd_s = rnd_s * 1103515245u + 12345u; return rnd_s >> 16; }

int main(int argc, char **argv) {
  if (argc < 7) { fprintf(stderr, "uso: zcover ref.bin rom.smc cuadro salas cuadros semilla\n"); return 2; }
  ref = fopen(argv[1], "rb");
  FILE *rf = fopen(argv[2], "rb"); fseek(rf, 0, SEEK_END); romsz = ftell(rf); fseek(rf, 0, SEEK_SET);
  rom = malloc(romsz); if (fread(rom, 1, romsz, rf) != romsz) return 1; fclose(rf);
  if (romsz % 0x8000) { memmove(rom, rom + (romsz % 0x8000), romsz - romsz % 0x8000); }
  long from = atol(argv[3]);
  int per = atoi(argv[5]);
  rnd_s = (uint32_t)atol(argv[6]);
  irq_reset();
  if (!read_frame(from)) { fprintf(stderr, "no puedo leer el cuadro %ld\n", from); return 1; }
  load_all(ram_cur);
  extern uint8 nt_viewer;
  nt_viewer = 0;
  cur_frame = from;
  char *ws = strdup(argv[4]);
  for (char *t = strtok(ws, ","); t; t = strtok(0, ",")) {
    unsigned room = strtoul(t, 0, 16), door = strtoul(strchr(t, ':') + 1, 0, 16);
    int guard = 0;
    while (game_state != 8 && guard++ < 2000) { cur_frame++; game_frame(0); }
    door_def_ptr = door;
    door_transition_function = FUNC16(DoorTransitionFunction_HandleElevator);
    game_state = 9;
    guard = 0;
    do { cur_frame++; game_frame(0); } while ((game_state != 8 || room_ptr != room) && guard++ < 2000);
    uint16_t joy = 0;
    int hold = 0;
    for (int f = 0; f < per; f++) {
      if (--hold <= 0) {
        static const uint16_t dirs[] = { 0, kButton_Left, kButton_Right, kButton_Up, kButton_Down,
                                         kButton_Left | kButton_Up, kButton_Right | kButton_Up };
        joy = dirs[rnd() % 7];
        if (rnd() & 1) joy |= kButton_X;          /* disparar */
        if (rnd() % 3 == 0) joy |= kButton_A;     /* saltar */
        if (rnd() % 4 == 0) joy |= kButton_B;     /* correr */
        if (rnd() % 6 == 0) joy |= kButton_L | kButton_R;
        hold = 4 + rnd() % 40;
      }
      if (f & 1) joy ^= rnd() % 3 == 0 ? kButton_X : 0;   /* disparos sueltos */
      samus_health = samus_max_health;
      cur_frame++;
      game_frame(joy);
      if (game_state != 8 && game_state != 9 && game_state != 10 && game_state != 11) break;
      if (game_state == 8 && room_ptr != room) {     /* salió de la sala: de vuelta por la misma puerta */
        door_def_ptr = door;
        door_transition_function = FUNC16(DoorTransitionFunction_HandleElevator);
        game_state = 9;
      }
    }
    fprintf(stderr, "sala %04X: %04X estado %d\n", room, room_ptr, game_state);
    if (getenv("MM_EPDUMP"))
      for (int i = 0; i < 18; i++)
        if (eproj_id[i]) fprintf(stderr, "  eproj %d: id %04X pre %04X x %d y %d\n", i, eproj_id[i], eproj_pre_instr[i], eproj_x_pos[i], eproj_y_pos[i]);
  }
  return 0;
}
