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

/* hlref: corre un guion completo (formato g2.txt) y desde el primer cuadro >= desde en que
   game_state != 0x1E escribe en el archivo de salida: [RAM 128 KB + VRAM 64 KB] del estado inicial y
   luego, por cuadro: entrada (u16, orden del decomp) + RAM 128 KB (después del cuadro).
   Uso: hlref rom guion salida desde [hasta] */
extern Snes *g_snes;
int main(int argc, char **argv) {
  Snes *sn = SnesInit(argv[1]); if (!sn) return 1;
  PpuBeginDrawing(sn->snes_ppu, pix1, 256 * 4, 0);
  PpuBeginDrawing(sn->my_ppu, pix2, 256 * 4, 0);
  g_runmode = 1;
  { extern int g_hl_nom7; g_hl_nom7 = getenv("HL_NOM7") != 0; }   /* sin la capa del modo 7 (patch_decomp.py) */
  if (getenv("HL_SHOTS")) sscanf(getenv("HL_SHOTS"), "%ld,%ld,%ld,%399s", &shot_from, &shot_step, &shot_to, shot_pfx);
  FILE *g = fopen(argv[2], "r"); char line[256];
  FILE *out = fopen(argv[3], "wb"); long from = atol(argv[4]), to = argc > 5 ? atol(argv[5]) : 1L << 30;
  fin = fopen("/dev/null", "w"); flog = fopen("/dev/null", "w");
  int started = 0; long n = 0;
  static long vram_f[32]; static char vram_fn[32][300]; int vram_n = 0, vram_i = 0;
  if (getenv("HL_VRAM")) {   /* "cuadro:archivo,cuadro:archivo" en orden */
    char *s = strdup(getenv("HL_VRAM")), *t = strtok(s, ",");
    for (; t && vram_n < 32; t = strtok(0, ",")) { vram_f[vram_n] = atol(t); strcpy(vram_fn[vram_n], strchr(t, ':') + 1); vram_n++; }
  }
  /* HL_WARP="n:puerta,n:puerta": antes del cuadro grabado n (0 = el primero), si el juego está en el estado 8,
     entra por esa puerta como el visor de la Genesis (transición de ascensor) */
  static long warp_n[64]; static unsigned warp_door[64]; int nwarp = 0, iwarp = 0;
  extern int g_viewer_noenemy;
  if (getenv("HL_WARP")) {
    g_viewer_noenemy = !getenv("HL_ENEMIES");  /* como el visor: sin enemigos (salvo la nave); HL_ENEMIES: con */
    char *s = strdup(getenv("HL_WARP")), *t = strtok(s, ",");
    for (; t && nwarp < 64; t = strtok(0, ",")) { warp_n[nwarp] = atol(t); warp_door[nwarp] = strtoul(strchr(t, ':') + 1, 0, 16); nwarp++; }
  }
  /* HL_SETXY="n:x:y,...": Samus a (x, y) antes del cuadro grabado n (para pruebas en un lugar de la sala, como el
     ascensor; check.c hace lo mismo) */
  static long sxy_n[32]; static int sxy_x[32], sxy_y[32]; int nsxy = 0, isxy = 0;
  if (getenv("HL_SETXY")) {
    char *s = strdup(getenv("HL_SETXY")), *t = strtok(s, ",");
    for (; t && nsxy < 32; t = strtok(0, ",")) { sscanf(t, "%ld:%d:%d", &sxy_n[nsxy], &sxy_x[nsxy], &sxy_y[nsxy]); nsxy++; }
  }
  while (fgets(line, sizeof line, g)) {
    int cnt = atoi(line); char *b = strchr(line, ' '); int in = b ? parse(b + 1) : 0;
    for (int i = 0; i < cnt && frame < to; i++) {
      if (getenv("HL_LOGSTATE")) { static int last = -1; if (w16(0x998) != last) { last = w16(0x998); fprintf(stderr, "cuadro %ld: estado %d\n", frame, last); } }
      if (!started && frame >= from && (w16(0x998) != 0x1E || getenv("HL_ANY"))) {
        if (getenv("HL_ITEMS")) {   /* HL_ITEMS=hex: objetos tomados y equipados desde el principio (pruebas: la bola sin su caja de mensaje) */
          unsigned it = strtoul(getenv("HL_ITEMS"), 0, 16);
          g_ram[0x9A2] |= it; g_ram[0x9A3] |= it >> 8; g_ram[0x9A4] |= it; g_ram[0x9A5] |= it >> 8;
        }
        if (getenv("HL_POKE")) {    /* HL_POKE="dir:valor,...": palabras de la RAM al empezar (pruebas: misiles, HUD) */
          char *ps = strdup(getenv("HL_POKE"));
          for (char *t = strtok(ps, ","); t; t = strtok(0, ",")) {
            unsigned ad = strtoul(t, 0, 16), v = strtoul(strchr(t, ':') + 1, 0, 16);
            g_ram[ad] = v; g_ram[ad + 1] = v >> 8;
          }
        }
        started = 1; fwrite(g_ram, 1, 0x20000, out); fwrite(g_snes->ppu->vram, 1, 0x10000, out);
        fprintf(stderr, "inicio en el cuadro %ld (estado %d)\n", frame, w16(0x998));
      }
      if (started && iwarp < nwarp && n >= warp_n[iwarp] && w16(0x998) == 8) {
        g_ram[0x78D] = warp_door[iwarp]; g_ram[0x78E] = warp_door[iwarp] >> 8;
        g_ram[0x99C] = 0x7D; g_ram[0x99D] = 0xE1; g_ram[0x998] = 9; g_ram[0x999] = 0;
        g_ram[0x9C2] = g_ram[0x9C4]; g_ram[0x9C3] = g_ram[0x9C5];
        fprintf(stderr, "salto %d en el cuadro grabado %ld: puerta %04X\n", iwarp, n, warp_door[iwarp]);
        iwarp++;
      }
      if (started && isxy < nsxy && n >= sxy_n[isxy] && w16(0x998) == 8) {
        g_ram[0xAF6] = sxy_x[isxy]; g_ram[0xAF7] = sxy_x[isxy] >> 8;
        g_ram[0xAFA] = sxy_y[isxy]; g_ram[0xAFB] = sxy_y[isxy] >> 8;
        if (!getenv("HL_SETXY_SCROLL")) {   /* HL_SETXY_SCROLL: sin tocar la posición anterior: la cámara la sigue */
          g_ram[0xB10] = sxy_x[isxy]; g_ram[0xB11] = sxy_x[isxy] >> 8;
          g_ram[0xB14] = sxy_y[isxy]; g_ram[0xB15] = sxy_y[isxy] >> 8;
        }
        if (getenv("HL_OPENSCROLLS"))   /* pruebas: todas las pantallas de la sala desbloqueadas (scrolls $7E:CD20 = 2, azul) */
          for (int sc = 0; sc < 50; sc++) g_ram[0xCD20 + sc] = 2;
        fprintf(stderr, "Samus a %d,%d en el cuadro grabado %ld\n", sxy_x[isxy], sxy_y[isxy], n);
        isxy++;
      }
      if (started && getenv("HL_GOD") && w16(0x998) == 8) { g_ram[0x9C2] = g_ram[0x9C4]; g_ram[0x9C3] = g_ram[0x9C5]; }   /* HL_GOD: Samus no muere (check.c igual) */
      step(in);
      if (vram_n && frame == vram_f[vram_i]) {   /* HL_VRAM: VRAM (64 KB) + CGRAM (512 B) del SNES tras ese cuadro */
        FILE *vf = fopen(vram_fn[vram_i], "wb"); fwrite(g_snes->ppu->vram, 1, 0x10000, vf); fwrite(g_snes->ppu->cgram, 1, 512, vf); fwrite(g_snes->ppu->oam, 1, 512, vf); fwrite(g_snes->ppu->highOam, 1, 32, vf); fclose(vf);
        if (vram_i + 1 < vram_n) vram_i++;
      }
      if (getenv("HL_VDUMP")) {   /* HL_VDUMP="desde,hasta,archivo": VRAM+CGRAM+OAM (66592 bytes) de cada cuadro, para
                                     ubicar en qué cuadro se hizo una captura (tools/inputs/find_frames.py) */
        static long vd_a = -1, vd_b; static FILE *vd;
        if (vd_a < 0) { char fn[300]; sscanf(getenv("HL_VDUMP"), "%ld,%ld,%299s", &vd_a, &vd_b, fn); vd = fopen(fn, "wb"); }
        if (frame >= vd_a && frame <= vd_b) { fwrite(g_snes->ppu->vram, 1, 0x10000, vd); fwrite(g_snes->ppu->cgram, 1, 512, vd);
                                              fwrite(g_snes->ppu->oam, 1, 512, vd); fwrite(g_snes->ppu->highOam, 1, 32, vd); }
      }
      if (started) { uint16_t v = in; fwrite(&v, 2, 1, out); fwrite(g_ram, 1, 0x20000, out); n++; }
    }
  }
  fclose(out); fprintf(stderr, "cuadros %ld\n", n);
  return 0;
}
