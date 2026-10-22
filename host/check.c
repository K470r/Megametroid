/* MegaMetroid — comprobación del motor nativo contra el SNES (docs/31).
 *
 * Carga el estado del SNES de una grabación (tools/snes_ref/ceres_bot/hlref: por cuadro, la entrada y los 128 KB
 * de RAM), corre el motor nativo con la misma entrada y compara sus variables con la RAM del SNES cuadro a cuadro.
 *
 * Uso: check ref.bin rom.smc desde hasta [modo]
 *   modo 0 (libre): el estado del SNES solo al empezar; después el motor corre solo.
 *   modo 1 (resincronizado): antes de cada cuadro se carga el estado del SNES del cuadro anterior.
 * Los sistemas que todavía no están escritos (enemigos, proyectiles enemigos, PLMs, paletas, HUD...) se copian
 * del SNES después de cada cuadro (sus variables no se comparan). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "room.h"
#include "vars_map.h"

static uint8_t *rom; static long romsz;
static FILE *ref;
static uint8_t ram_prev[0x20000], ram_cur[0x20000];
static uint16_t in_cur;
static long cur_frame;

extern uint32_t nt_unimpl_val;
void nt_unimpl(const char *what) {
  static char seen[400][96]; static int n;
  char key[96];
  snprintf(key, sizeof key, nt_unimpl_val ? "%.80s %X" : "%.80s", what, nt_unimpl_val);
  for (int i = 0; i < n; i++) if (!strcmp(seen[i], key)) return;
  if (n < 400) strcpy(seen[n++], key);
  printf("cuadro %ld: SIN PORTAR: %s\n", cur_frame, key);
}

static uint16_t rw(const uint8_t *r, unsigned a) { return r[a] | r[a + 1] << 8; }
static const uint8_t *romp(uint32_t a) { return rom + ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF); }

static int read_frame(long f) {
  if (fseek(ref, 0x30000 + f * 0x20002L, SEEK_SET)) return 0;
  uint8_t b[2];
  if (fread(b, 1, 2, ref) != 2) return 0;
  in_cur = b[0] | b[1] << 8;
  return fread(ram_cur, 1, 0x20000, ref) == 0x20000;
}

/* variables que hoy pertenecen a sistemas sin portar o a lo que la Genesis hace distinto: no se comparan y
   se copian del SNES tras cada cuadro */
static const char *kNotCompared[] = {
  "oam_next_ptr", "cur_enemy_index", "sfx1_queue", "sfx2_queue", "sfx3_queue", "sfx_cur", "sfx_readpos", "sfx_writepos", "sfx_state", "sfx_clear_delay", "sfx_max_queued", "hdma_data_table_in_ceres", "hdma_object_channels_bitmask", "hdma_objects_enable_flag", "reg_HDMAEN", "reg_BG1HOFS", "reg_BG1VOFS", "reg_BG2HOFS", "reg_BG2VOFS",
  /* registros de la PPU que arma LayerBlendingHandler ($88:8000, sin portar: solo imagen) */
  "next_gameplay_CGWSEL", "next_gameplay_CGADSUB", "gameplay_CGWSEL", "gameplay_CGADSUB", "gameplay_TM", "reg_TM", "reg_TS", "reg_TMW", "reg_TSW", "reg_W12SEL", "reg_W34SEL",
  "reg_WOBJSEL",
  /* las interrupciones del motor no son las del SNES (IRQ de la PPU) */
  "reg_NMITIMEN",
  /* 7E:9800: tabla HDMA de ventana (rayos X, etc.; el SNES también la usa en Ceres); el motor solo la limpia */
  "hdma_table_1", 0
};
static int not_compared(const char *n) {
  for (int i = 0; kNotCompared[i]; i++) if (!strcmp(kNotCompared[i], n)) return 1;
  return 0;
}
/* partes de arreglos que todavía escribe otro sistema (se copian del SNES): 1 = no comparar ese elemento */
static int elem_skip(const char *n, unsigned i) {
  if (!strcmp(n, "palette_buffer") || !strcmp(n, "target_palettes"))
    return 0;
  if (!strcmp(n, "hud_tilemap"))
    return 0;
  /* la tabla del haze: al cargar una sala el SNES la deja en cero un cuadro (con la pantalla negra; no se encontró
     quién) */
  if (!strcmp(n, "hdma_table_2"))
    return (game_state >= 9 && game_state <= 11) ? 1 : 0;
  /* tilemap_stuff (7E:2000) es un búfer que el SNES usa para muchas cosas (carga de salas...); el motor solo lo
     usa para la cola de Ridley (primeras 128 palabras en su sala) */
  /* 7E:9F00-9FFF: tablas del cielo de la superficie; fuera de esas salas el SNES usa esa RAM para otras cosas (y el
     cielo empieza con lo que haya quedado ahí): se copia del SNES mientras no se usa */
  if (!strncmp(n, "scrolling_sky_bg2", 17))
    return (room_main_code_ptr == 0xC116 || room_main_code_ptr == 0xC11B || room_main_code_ptr == 0xC120) ? 0 : 1;
  if (!strcmp(n, "tilemap_stuff"))
    return room_ptr != 0xE0B5 || i >= 128 ? 1 : game_state != 8 ? 2 : 0;
  return 0;
}

/* 7E:198D-1C1E es de los proyectiles enemigos en el juego y de las cinemáticas en una cinemática (variables_extra.h):
   en el motor son variables distintas; se compara la del dueño de ese cuadro y la otra se copia del SNES */
static int is_cine_var(const char *n) {
  return !strncmp(n, "cinematic", 9) || !strncmp(n, "mode7_obj", 9) || !strcmp(n, "mode7_cur_index")
      || !strcmp(n, "mode7_spawn_param") || !strncmp(n, "g_word_7E199", 12);
}
static int max_nd = 12;
static uint16_t gs_frame_start;   /* game_state al empezar el cuadro (el del cuadro anterior en el SNES) */
static int ram_owner_skip(const VarMap *v) {
  int cine = gs_frame_start == 30 || gs_frame_start == 34 || gs_frame_start == 37;
  if (!strcmp(v->name, "menu_tilemap"))           /* 7E:3600 es de los menús solo en ellos */
    return !(gs_frame_start == 25 || gs_frame_start == 26);
  if (is_cine_var(v->name)) return !cine;
  return cine && v->addr < 0x1C1F && v->addr + v->size * v->count > 0x198D;
}

static void load_var(const VarMap *v, const uint8_t *r) {
  for (unsigned i = 0; i < v->count; i++) {
    if (v->size == 2) ((uint16_t *) v->ptr)[i] = rw(r, v->addr + 2 * i);
    else ((uint8_t *) v->ptr)[i] = r[v->addr + i];
  }
}
static void load_enemies(const uint8_t *r) {
  for (int k = 0; k < ENEMY_COUNT; k++) {
    const uint8_t *e = r + 0xF78 + 64 * k;
    EnemyData *E = &enemy_data[k];
    uint16_t *w = &E->enemy_ptr;
    for (int i = 0; i < 23; i++) w[i] = rw(e, 2 * i);
    E->bank = e[46]; E->field_2F = e[47];
    uint16_t *w2 = &E->ai_var_A;
    for (int i = 0; i < 8; i++) w2[i] = rw(e, 48 + 2 * i);
  }
}
static void load_spawn(const uint8_t *r) {
  for (int k = 0; k < ENEMY_COUNT; k++) {
    uint16_t *w = &enemy_spawn_data[k].some_flag;
    for (int i = 0; i < 32; i++) w[i] = rw(r, 0x7000 + 64 * k + 2 * i);
  }
  for (int i = 0; i < 0x1000; i++) enemy_ram7800[i] = rw(r, 0x7800 + 2 * i);
}
/* compara enemy_data y enemy_spawn_data con la RAM del SNES */
static int cmp_enemies(long f, const uint8_t *r, int nd) {
  for (int k = 0; k < ENEMY_COUNT; k++) {
    const uint8_t *e = r + 0xF78 + 64 * k;
    EnemyData *E = &enemy_data[k];
    uint16_t nat[32], *w = &E->enemy_ptr, *w2 = &E->ai_var_A;
    for (int i = 0; i < 23; i++) nat[i] = w[i];
    nat[23] = E->bank | E->field_2F << 8;
    for (int i = 0; i < 8; i++) nat[24 + i] = w2[i];
    for (int i = 0; i < 32; i++) {
      if (nat[i] != rw(e, 2 * i)) {
        if (nd < max_nd) printf("cuadro %ld: enemigo %d palabra +%02X nativo %04X snes %04X\n", f, k, 2 * i, nat[i], rw(e, 2 * i));
        nd++;
      }
    }
    uint16_t *s = &enemy_spawn_data[k].some_flag;
    /* durante la carga de una sala el SNES usa 7E:7000 de búfer (fondo de la sala) antes de cargar los
       enemigos; el motor nativo no necesita ese búfer: no se compara mientras game_state == 11 */
    if (game_state == 11 || game_state == 6) continue;   /* 6: comienzo de la partida, igual */
    for (int i = 0; i < 32; i++) {
      if (s[i] != rw(r, 0x7000 + 64 * k + 2 * i)) {
        if (nd < max_nd) printf("cuadro %ld: datos de aparición %d palabra +%02X nativo %04X snes %04X\n", f, k, 2 * i, s[i], rw(r, 0x7000 + 64 * k + 2 * i));
        nd++;
      }
    }
  }
  return nd;
}
#include "lvl_host.h"
static void load_room(const uint8_t *r) { lvl_from_snes(r); }
static void load_all(const uint8_t *r) {
  for (const VarMap *v = g_vars_map; v->name; v++) load_var(v, r);
  load_enemies(r);
  load_spawn(r);
  load_room(r);
}

static uint16_t joy_from_bot(uint16_t in) {     /* orden de hlbot -> JOY1 del SNES */
  static const uint16_t m[12] = { 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10 };
  uint16_t j = 0;
  for (int b = 0; b < 12; b++) if (in & (1 << b)) j |= m[b];
  return j;
}

int main(int argc, char **argv) {
  if (argc < 5) { fprintf(stderr, "uso: check ref.bin rom.smc desde hasta [modo]\n"); return 2; }
  ref = fopen(argv[1], "rb");
  FILE *rf = fopen(argv[2], "rb"); fseek(rf, 0, SEEK_END); romsz = ftell(rf); fseek(rf, 0, SEEK_SET);
  rom = malloc(romsz); if (fread(rom, 1, romsz, rf) != romsz) return 1; fclose(rf);
  if (romsz % 0x8000) { memmove(rom, rom + (romsz % 0x8000), romsz - romsz % 0x8000); }
  if (getenv("MM_NDIFF")) max_nd = atoi(getenv("MM_NDIFF"));
  long from = atol(argv[3]), to = atol(argv[4]); int mode = argc > 5 ? atoi(argv[5]) : 0;
  irq_reset();
  if (!read_frame(from)) { fprintf(stderr, "no puedo leer el cuadro %ld\n", from); return 1; }
  memcpy(ram_prev, ram_cur, sizeof ram_cur);
  load_all(ram_prev);
  long bad_frames = 0, first_bad = -1;
  /* HL_WARP="n:puerta,...": los mismos saltos de sala que hizo hlref al grabar (antes del cuadro grabado n, si el
     juego está en el estado 8: transición de ascensor por esa puerta, como el visor) */
  static long warp_n[64]; static unsigned warp_door[64]; int nwarp = 0, iwarp = 0;
  if (getenv("HL_WARP")) {
    extern uint8 nt_viewer;
    nt_viewer = !getenv("HL_ENEMIES");           /* sin enemigos, como el visor (hlref hace lo mismo en el SNES); HL_ENEMIES: con */
    char *ws = strdup(getenv("HL_WARP")), *t = strtok(ws, ",");
    for (; t && nwarp < 64; t = strtok(0, ",")) { warp_n[nwarp] = atol(t); warp_door[nwarp] = strtoul(strchr(t, ':') + 1, 0, 16); nwarp++; }
  }
  /* HL_SETXY="n:x:y,...": como hlref, Samus a (x, y) antes del cuadro grabado n */
  static long sxy_n[32]; static int sxy_x[32], sxy_y[32]; int nsxy = 0, isxy = 0;
  if (getenv("HL_SETXY")) {
    char *ss = strdup(getenv("HL_SETXY")), *t = strtok(ss, ",");
    for (; t && nsxy < 32; t = strtok(0, ",")) { sscanf(t, "%ld:%d:%d", &sxy_n[nsxy], &sxy_x[nsxy], &sxy_y[nsxy]); nsxy++; }
  }
  for (long f = from + 1; f <= to; f++) {
    cur_frame = f;
    if (!read_frame(f)) break;
    while (isxy < nsxy && sxy_n[isxy] <= from) isxy++;
    if (isxy < nsxy && f >= sxy_n[isxy] && game_state == 8) {
      samus_x_pos = samus_prev_x_pos = sxy_x[isxy];
      samus_y_pos = samus_prev_y_pos = sxy_y[isxy];
      isxy++;
    }
    if (mode == 1) load_all(ram_prev);
    if (mode == 2) { if (bad_frames > 2) bad_frames = 2; }
    while (iwarp < nwarp && warp_n[iwarp] <= from) iwarp++;   /* ya hechos en el SNES */
    if (iwarp < nwarp && f >= warp_n[iwarp] && game_state == 8) {   /* read_frame(f) = cuadro grabado f */
      door_def_ptr = warp_door[iwarp];
      door_transition_function = FUNC16(DoorTransitionFunction_HandleElevator);
      game_state = 9;
      samus_health = samus_max_health;
      iwarp++;
    }
    if (getenv("HL_GOD") && game_state == 8) samus_health = samus_max_health;   /* HL_GOD: como hlref */
    game_frame(joy_from_bot(in_cur));
    if (getenv("MM_HUDLOG")) {   /* palabras del HUD que aparecen (para la tabla de celdas de la Genesis) */
      static uint8_t seen[65536];
      static FILE *hf;
      if (!hf) hf = fopen(getenv("MM_HUDLOG"), "w");
      for (unsigned i = 0; i < sizeof(hud_tilemap) / 2; i++)
        if (!seen[hud_tilemap[i]]) { seen[hud_tilemap[i]] = 1; fprintf(hf, "%04X\n", hud_tilemap[i]); fflush(hf); }
    }
    int nd = 0;
    /* Samus en el borde de la sala o fuera (por ejemplo al saltar por arriba en el visor): el
       SNES lee la RAM que haya después del nivel y el motor no; se toma el estado del SNES y no se compara */
    int sx = (int16_t)rw(ram_cur, 0xAF6), sy = (int16_t)rw(ram_cur, 0xAFA);
    int sxr = rw(ram_cur, 0xAFE) + 16, syr = rw(ram_cur, 0xB00) + 16;   /* radios + un bloque de margen */
    if (nwarp && (sx - sxr < 0 || sy - syr < 0 || sx + sxr >= 16 * rw(ram_cur, 0x7A5) || sy + syr >= 16 * rw(ram_cur, 0x7A7))) {
      load_all(ram_cur);
      memcpy(ram_prev, ram_cur, sizeof ram_cur);
      continue;
    }
    for (const VarMap *v = g_vars_map; v->name; v++) {
      if (v == g_vars_map) { static uint16_t gs_prev = 0xFFFF; gs_frame_start = gs_prev == 0xFFFF ? game_state : gs_prev;
                             gs_prev = ram_cur[0x998] | ram_cur[0x999] << 8; }
      if (not_compared(v->name) || ram_owner_skip(v)) { load_var(v, ram_cur); continue; }
      for (unsigned i = 0; i < v->count; i++) {
        unsigned a = v->addr + v->size * i;
        int sk = elem_skip(v->name, i);
        /* al cargar una sala el SNES descomprime los gráficos en 7E:2000-BFFF (tablas de HDMA, búferes): ahí manda
           el SNES hasta que termina la carga */
        if (!sk && game_state >= 9 && game_state <= 11 && a >= 0x2000 && a < 0xC000) sk = 1;
        if (sk == 2) continue;                   /* no se compara este cuadro, pero queda el valor del motor */
        if (sk) {
          if (v->size == 2) ((uint16_t *) v->ptr)[i] = rw(ram_cur, a); else ((uint8_t *) v->ptr)[i] = ram_cur[a];
          continue;
        }
        unsigned sn = v->size == 2 ? rw(ram_cur, a) : ram_cur[a];
        unsigned na = v->size == 2 ? ((uint16_t *) v->ptr)[i] : ((uint8_t *) v->ptr)[i];
        if (sn != na) {
          if (nd < max_nd) printf("cuadro %ld: %s[%u] (7E:%04X) nativo %04X snes %04X\n", f, v->name, i, a, na, sn);
          nd++;
        }
      }
    }
    /* nivel de la sala (level_data y BTS); en las cinemáticas y el fin de juego el SNES usa esa RAM (7F:0000) para
       los gráficos del modo 7 */
    int level_live = !(game_state == 30 || game_state == 34 || game_state == 37 || game_state == 6 || game_state == 25 || game_state == 26);
    for (unsigned i = 0; level_live && i < (unsigned)(room_size_in_blocks >> 1) && i < ROOM_MAX_BLOCKS; i++) {
      if (level_get(i) != rw(ram_cur, 0x10002 + 2 * i) || bts_get(i) != ram_cur[0x16402 + i]) {
        if (nd < max_nd) printf("cuadro %ld: bloque %u nativo %04X/%02X snes %04X/%02X\n", f, i, level_get(i), bts_get(i),
                            rw(ram_cur, 0x10002 + 2 * i), ram_cur[0x16402 + i]);
        nd++;
      }
    }
    nd = cmp_enemies(f, ram_cur, nd);
    /* RAM extra de enemigos (7E:7800-97FF: variables de jefes como Ridley); como 7E:7000, no durante la carga */
    if (game_state != 11 && game_state != 6)
      for (int i = 0; i < 0x1000; i++)
        if (enemy_ram7800[i] != rw(ram_cur, 0x7800 + 2 * i)) {
          if (nd < max_nd) printf("cuadro %ld: ERAM 7E:%04X nativo %04X snes %04X\n", f, 0x7800 + 2 * i, enemy_ram7800[i], rw(ram_cur, 0x7800 + 2 * i));
          nd++;
        }
    if (nd) { bad_frames++; if (first_bad < 0) first_bad = f; if (bad_frames >= 3 && mode == 0) break;  /* modo 2: libre sin parar (la sala de Ridley es distinta a propósito) */ }
    memcpy(ram_prev, ram_cur, sizeof ram_cur);
  }
  printf("cuadros %ld-%ld: %ld con diferencias (primero %ld)\n", from + 1, to, bad_frames, first_bad);
  return bad_frames != 0;
}
