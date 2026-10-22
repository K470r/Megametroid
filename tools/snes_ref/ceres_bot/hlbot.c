/* hlbot: juega la parte de Ceres de Super Metroid con el decomp (snesrev) y graba la entrada.
   Uso: hlbot rom.smc prefijo.txt salida_entrada.txt log.txt [max_cuadros] [ref.bin desde] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "snes/snes.h"
#include "snes/ppu.h"
Snes *SnesInit(const char *filename);
static uint8_t pix1[256*4*240], pix2[256*4*240];
bool RtlRunFrame(int inputs);
extern uint8_t g_ram[];
extern uint8_t g_runmode;
bool g_debug_flag; int g_got_mismatch_count; bool g_new_ppu = true; bool g_other_image; void *g_spc_player;
void Die(const char *e) { fprintf(stderr, "DIE %s\n", e); exit(1); }
void Warning(const char *e) { fprintf(stderr, "WARN %s\n", e); }
void RtlApuLock(void) {} void RtlApuUnlock(void) {}
void SpcPlayer_CopyVariablesFromRam(void *p) {} void SpcPlayer_CopyVariablesToRam(void *p) {}
void SpcPlayer_GenerateSamples(void *p) {} void SpcPlayer_Initialize(void *p) {} void SpcPlayer_Upload(void *p, const uint8_t *d) {}
#define MAXF 16384
static void *seen[MAXF]; static int nseen; static int seen_on;
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *fn, void *cs) {
  if (!seen_on) return;
  for (int i = nseen - 1; i >= 0; i--) if (seen[i] == fn) return;
  if (nseen < MAXF) seen[nseen++] = fn;
}
static uint8_t ramprev[0x20000], ramw[0x20000];
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *fn, void *cs) {}
enum { B_ = 1, Y_ = 2, SEL = 4, ST = 8, UP = 16, DN = 32, LE = 64, RI = 128, A_ = 256, X_ = 512, L_ = 1024, R_ = 2048 };
static int parse(const char *b) {
  int r = 0;
  for (; *b && *b != '\n'; b++) switch (*b) {
    case 'B': r |= 1; break; case 'Y': r |= 2; break; case 's': r |= 4; break; case 'S': r |= 8; break;
    case 'U': r |= 16; break; case 'D': r |= 32; break; case '<': r |= 64; break; case '>': r |= 128; break;
    case 'A': r |= 256; break; case 'X': r |= 512; break; case 'L': r |= 1024; break; case 'R': r |= 2048; break;
  }
  return r;
}
static uint16_t w16(int a) { return g_ram[a] | g_ram[a + 1] << 8; }
static FILE *fin, *flog; static long frame; static int last_in = -1, run = 0;
static void emit(int in) {
  if (in == last_in) { run++; return; }
  if (run) { fprintf(fin, "%d ", run); for (int b = 0; b < 12; b++) if (last_in & (1 << b)) fputc("BYsSUD<>AXLR"[b], fin); if (!last_in) fputc('-', fin); fputc('\n', fin); }
  last_in = in; run = 1;
}
static long shot_from = -1, shot_step = 1, shot_to = 0; static char shot_pfx[400];
static long seen_from = -1;
static void step(int in) {
  seen_on = seen_from >= 0 && frame >= seen_from;
  emit(in); RtlRunFrame(in); frame++;
  if (seen_on) { for (int i = 0; i < 0x20000; i++) if (g_ram[i] != ramprev[i]) ramw[i] = 1; }
  memcpy(ramprev, g_ram, 0x20000);
  if (shot_from >= 0 && frame >= shot_from && (frame - shot_from) % shot_step == 0 && frame < shot_to) {
    char fn[512]; snprintf(fn, sizeof fn, "%s%05ld.ppm", shot_pfx, frame);
    FILE *pf = fopen(fn, "wb"); fprintf(pf, "P6\n256 224\n255\n");
    for (int y = 0; y < 224; y++) for (int x = 0; x < 256; x++) { uint8_t *q = pix1 + (y + 1) * 1024 + x * 4; fputc(q[2], pf); fputc(q[1], pf); fputc(q[0], pf); }
    fclose(pf);
  }
}
#include "bot_logic.h"
int main(int argc, char **argv) {
  Snes *sn = SnesInit(argv[1]); if (!sn) return 1;
  PpuBeginDrawing(sn->snes_ppu, pix1, 256 * 4, 0);
  PpuBeginDrawing(sn->my_ppu, pix2, 256 * 4, 0);
  g_runmode = 1;
  if (getenv("HL_SHOTS")) sscanf(getenv("HL_SHOTS"), "%ld,%ld,%ld,%399s", &shot_from, &shot_step, &shot_to, shot_pfx);
  FILE *g = fopen(argv[2], "r"); char line[256];
  fin = fopen(argv[3], "w"); flog = fopen(argv[4], "w");
  long maxf = argc > 5 ? atol(argv[5]) : 30000;
  if (getenv("HL_SEEN")) seen_from = atol(getenv("HL_SEEN"));
  /* prefijo: el guion hasta que empieza el juego en Ceres (estado 8) */
  while (fgets(line, sizeof line, g)) {
    int n = atoi(line); char *b = strchr(line, ' '); int in = b ? parse(b + 1) : 0;
    for (int i = 0; i < n; i++) { if (w16(0x998) == 8) goto play; step(in); }
  }
play:
  fprintf(flog, "juego en el cuadro %ld\n", frame);
  bot_init();
  while (frame < maxf) {
    int in = bot(frame);
    if (in < 0) break;
    {   /* HL_NOISE=semilla: ráfagas de entrada al azar sobre el bot (para cubrir más caminos del motor) */
      static long noise_left; static unsigned seed; static int noise_in, init;
      if (!init) { init = 1; seed = getenv("HL_NOISE") ? atoi(getenv("HL_NOISE")) : 0; }
      if (seed) {
        static const int combos[] = { 0, RI, LE, UP, DN, B_, A_, X_, RI|B_, LE|B_, RI|A_, LE|A_, UP|X_, DN|X_, RI|X_, LE|X_, RI|B_|A_, LE|B_|A_, UP|A_, DN|A_, L_, R_, RI|L_, LE|R_, RI|R_, LE|L_, A_|X_, RI|A_|X_, LE|A_|X_ };
        if (noise_left > 0) { noise_left--; in = noise_in; }
        else { seed = seed * 1103515245u + 12345u; if (((seed >> 16) % 100) < 3) { noise_left = 5 + (seed >> 8) % 40; seed = seed * 1103515245u + 12345u; noise_in = combos[(seed >> 16) % (sizeof combos / sizeof *combos)]; } }
      }
    }
    step(in);
  }
  emit(-2);
  if (seen_from >= 0) {
    FILE *q = fopen("seen.txt", "w"); for (int i = 0; i < nseen; i++) fprintf(q, "%p\n", seen[i]); fclose(q);
    q = fopen("ramw.bin", "wb"); fwrite(ramw, 1, 0x20000, q); fclose(q);
  }
  fclose(fin); fclose(flog);
  printf("cuadros %ld\n", frame);
  return 0;
}
