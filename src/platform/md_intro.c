/*
 * MegaMetroid — la intro en la Genesis (docs/38): páginas del diario, recuerdos y el vuelo a Ceres.
 *
 * La lógica es la del motor nativo (src/intro.c, comprobada contra el SNES). Aquí solo se dibuja lo que el SNES
 * tendría en pantalla, con los datos de tools/mkintro.py (en la página INTRO_PAGE del SSF2):
 *   * plano A = BG3 (texto y franjas negras de los recuerdos) o BG1 de "SPACE COLONY": se sigue lo que el SNES tiene
 *     en ese mapa de la VRAM (HandleCinematicBgObjects manda 7E:3000) y se convierten solo las filas que cambian.
 *   * plano B = BG2 (el casco) o BG1 (el recuerdo): mapa fijo del grupo; los ojos del casco, que el juego cambia,
 *     se convierten al vuelo.
 *   * sprites: los spritemaps que dibuja el motor (DrawCinematicSpritemap, proyectiles), ya convertidos por clave;
 *     Samus y el cañón como en el juego (md_engine.c).
 *   * paleta: cada color de la Genesis sale de un índice de palette_buffer, con el brillo de INIDISP y la suma de
 *     color fijo (el destello del vuelo).
 * El fundido cruzado del SNES (mezcla del texto con la escena) se hace oscureciendo la escena que se va y aclarando
 * la que llega (IntroScene, src/intro.c); el cambio de gráficos queda en el medio, en negro.
 * El vuelo a Ceres: la nave es BG1 en modo 7 en el SNES; aquí cada cuadro ya viene dibujado en tiles y se muestra
 * en un plano, con el otro de búfer para el cuadro siguiente.
 *
 * Se incluye desde md_engine.c (usa su lista de sprites y su CRAM).
 */
#include "md_intro_data.h"
#include "cinematic.h"

static int16 vb_lines_left(void);              /* md_engine.c */
#define INTRO_PLANE_W 64                       /* planos de 64x32 durante la intro */
#define INTRO_DY 2                              /* líneas que se sube la imagen respecto del juego */
#define INTRO_BUDGET_LINES 4                    /* líneas del vblank que se dejan de margen */

static uint8 intro_on;                          /* la intro está en pantalla (game_state 30) */
static int8 it_scene = -1;                      /* escena que se ve (INTRO_SC_*) */
static int8 it_group = -1;                      /* grupo de gráficos en la VRAM */
static int8 it_load = -1;                       /* grupo por cargar en el próximo vblank */
static uint8 it_draw_scene, it_draw_ok;         /* escena de los sprites del cuadro (la del momento del dibujo) */
static uint16 it_factor = 64;
static uint16 it_dest;                          /* mapa del SNES que va al plano A ($4C00 o $5C00) */
static uint32 it_dirty;                         /* filas del plano A por escribir */
static uint8 it_upload;                         /* el cuadro anterior el SNES mandaba el mapa del texto */
static uint8 it_eyes_n;                         /* ojos por escribir */
static uint8 it_bplane_a;                       /* "SPACE COLONY": el plano B muestra el mismo mapa que el A */
static uint16 it_ah, it_av, it_bh, it_bv;      /* scroll */
static uint8 it_hide_a;                         /* fundido cruzado hacia/desde un recuerdo: sin el plano A (texto) */
static uint16 it_eyes_raw[64], it_eyes[64];
static uint16 it_cram_src[64];                  /* color del SNES de cada ranura la última vez (para no rehacer) */
static uint8 it_cram_bright = 0xFF;
static uint16 it_cram_fixed = 0xFFFF;
typedef union {
  uint16 a[1024];                               /* el mapa del plano A tal como está en la VRAM del SNES */
  uint16 ship[28 * 32];                         /* vuelo: el cuadro de la nave que se arma */
} ItBuf;
_Static_assert(sizeof(ItBuf) <= sizeof md_scratch, "md_scratch");
#define it_buf (*(ItBuf *)md_scratch)            /* (RAM compartida) */
#define it_shadow (it_buf.a)

/* canal de 5 bits del SNES -> 3 bits de la Genesis según el brillo (0-15) */
static const uint8 kMdCh[16][32] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2},
  {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2},
  {0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3},
  {0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
  {0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4},
  {0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4},
  {0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4,5,5},
  {0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5},
  {0,0,0,1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,5,6},
  {0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,6,6,6},
  {0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,6,6,7},
  {0,0,0,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,5,6,6,6,6,7,7,7},
};

/* página de la ventana 7 del SSF2 (con barrera: el compilador no mueve lecturas de datos constantes de un lado al
   otro del cambio) */
static inline void it_map(uint8 p) {
  __asm__ volatile("" ::: "memory");
  md_hw_page = p;
  *(volatile uint8 *)0xA130FF = p;
  __asm__ volatile("" ::: "memory");
}
static inline void it_page(void) { it_map(INTRO_PAGE); }
static inline void it_page_back(void) { it_map(md_cur_page); }

/* palabra de un mapa del SNES -> celda de la Genesis por la tabla del grupo (idx: carácter -> fila de 8 en val) */
static inline uint16 it_cell(const uint16 *idx, const uint16 *val, uint16 e, uint8 prio) {
  uint16 o = idx[e & 0x3FF];
  if (o == 0xFFFF) return 0;
  uint16 w = val[o + ((e >> 10) & 7)];
  if (!w) return 0;
  w |= ((e >> 14) & 1) << 11 | ((e >> 15) & 1) << 12;
  if (prio && (e & 0x2000)) w |= 0x8000;
  return w;
}

/* ------------------------------------------------------------------ sprites (ganchos del motor) */
static struct { uint16 a, b; uint8 kind, grp; const uint16 *rec; } it_kc[32];   /* claves ya buscadas */
static void it_draw(uint8 kind, uint16 a, uint16 b, int16 x, int16 y) {
  if (!it_draw_ok) {                            /* la escena del momento del dibujo: la de sus tablas */
    uint16 f;
    it_draw_scene = (uint8)IntroScene(it_scene < 0 ? INTRO_SC_CAPT : (uint16)it_scene, &f);
    it_draw_ok = 1;
  }
  /* la misma clave que hace poco (el juego dibuja lo mismo cuadro tras cuadro): sin buscar */
  uint8 grp = kIntroSceneGroup[it_draw_scene];
  uint16 h = (a ^ (a >> 4) ^ b ^ kind) & 31;
  if (it_kc[h].a == a && it_kc[h].b == b && it_kc[h].kind == kind && it_kc[h].grp == grp) {
    it_page();
    if (it_kc[h].rec) put_recs(it_kc[h].rec, x, y, 0);
    it_page_back();
    return;
  }
  it_page();
  const IntroGroup *G = &kIntroGroups[grp];
  it_kc[h].a = a; it_kc[h].b = b; it_kc[h].kind = kind; it_kc[h].grp = grp; it_kc[h].rec = 0;
  int lo = 0, hi = G->nkeys - 1;
  while (lo <= hi) {                            /* claves ordenadas por (tipo, a, b) */
    int m = (lo + hi) >> 1;
    const IntroKey *k = &G->keys[m];
    int d = k->kind != kind ? (k->kind < kind ? -1 : 1) : k->a != a ? (k->a < a ? -1 : 1) : k->b != b ? (k->b < b ? -1 : 1) : 0;
    if (!d) { it_kc[h].rec = G->recs + k->rec; put_recs(G->recs + k->rec, x, y, 0); break; }
    if (d < 0) lo = m + 1; else hi = m - 1;
  }
  it_page_back();
}

/* X de 9 bits como en el SNES; Y, el byte de la OAM (DrawSpritemap, $81:879F) */
static inline int16 it_x9(uint16 x) { return (int16)(x << 7) >> 7; }

/* ------------------------------------------------------------------ la nave del vuelo (modo 7) */
#define SHIP_BUDGET 5600                        /* bytes por vblank para la nave */
/* Cuándo se muestra el cuadro que se prepara no depende de cuánto vblank sobra (así la imagen de cada cuadro del
   motor es siempre la misma, docs/50): cada cuadro de la nave cuesta sus bytes (ship_bytes) y cada cuadro del motor
   cuenta SHIP_STEP. Se muestra cuando ese presupuesto alcanza y está cargado; la carga de verdad (ship_vblank) usa lo
   que sobra del vblank y va adelante. */
#ifndef SHIP_STEP
#define SHIP_STEP 2400
#endif
static int8 ship_front = -1;                    /* plano que se ve (0 = A, 1 = B), -1 = ninguno */
static int16 ship_shown = -1, ship_load = -1, ship_now = -1;
static uint16 ship_hint, ship_tile_done;
static uint8 ship_lo[2], ship_hi[2];            /* filas con algo de la nave en cada plano */
static uint8 ship_rlo, ship_rhi, ship_map_done, ship_ready;
static uint16 ship_credit;                      /* presupuesto que le falta al cuadro que se prepara */
static uint8 ship_go;                           /* el vblank de este cuadro lo muestra (lo decide ship_update) */
#ifdef MD_M7LOG
volatile uint16 md_ship_wait;                   /* (medición) cuadros en que el presupuesto alcanzaba y la carga no */
#endif

static int16 ship_find(void) {
  uint16 key[8] = { reg_M7A, reg_M7B, reg_M7C, reg_M7D, reg_M7X, reg_M7Y, reg_BG1HOFS, reg_BG1VOFS };
  for (uint16 j = 0, i = ship_hint; j < INTRO_SHIP_N; j++, i = (i + 1 == INTRO_SHIP_N) ? 0 : i + 1) {
    const uint16 *k = kIntroShipKey[i];
    if (k[0] == key[0] && k[1] == key[1] && k[2] == key[2] && k[3] == key[3] &&
        k[4] == key[4] && k[5] == key[5] && k[6] == key[6] && k[7] == key[7]) {
      ship_hint = i;
      return (int16)i;
    }
  }
  return -1;
}

/* registro del cuadro id: deja puesta la página del cuadro (las tablas están en INTRO_PAGE: se leen antes) */
static inline const uint16 *ship_rec(int16 id) {
  uint32 off = kIntroShipOff[id];
  it_map(kIntroShipPage[id]);
  return (const uint16 *)(0x380000 + off);
}

/* celdas y tiles del cuadro id (vuelve a INTRO_PAGE) */
static void ship_hdr(int16 id, uint16 *nc, uint16 *nt) {
  const uint16 *r = ship_rec(id);
  *nc = r[0];
  *nt = r[1];
  it_page();
}

/* bytes que cuesta un cuadro (sus tiles y el mapa) */
static uint16 ship_bytes(uint16 nt) { return (uint16)(nt * 32 + 28 * 64); }

/* vblanks que tarda en llegar un cuadro (se ve en el último) */
static uint16 ship_cost(int16 id) {
  uint16 nc, nt;
  ship_hdr(id, &nc, &nt);
  if (!nc) return 1;
  return (uint16)((ship_bytes(nt) + SHIP_STEP - 1) / SHIP_STEP);
}

/* empieza a preparar el cuadro id para el plano de atrás: el mapa en la RAM (con ceros donde estaba el anterior) */
static void ship_begin(int16 id) {
  const uint16 *r = ship_rec(id);
  uint16 nc = r[0], nt = r[1];
  if (!nc) { it_page(); return; }               /* cuadro demasiado grande: queda el anterior */
  uint8 back = ship_front < 0 ? 0 : ship_front ^ 1;
  uint16 *map = it_buf.ship;
  for (uint16 i = 0; i < 28 * 32; i++) map[i] = 0;
  uint8 lo = ship_lo[back], hi = ship_hi[back];
  const uint16 *pos = r + 2, *tid = pos + nc;
  const uint16 base = (back ? INTRO_SHIP_BUF1 : INTRO_SHIP_BUF0) | (INTRO_SHIP_LINE << 13) | 0x8000;
  uint8 nlo = 28, nhi = 0;
  for (uint16 i = 0; i < nc; i++) {
    uint16 c = pos[i];
    map[c] = base + tid[i];
    uint8 row = (uint8)(c >> 5);
    if (row < nlo) nlo = row;
    if (row > nhi) nhi = row;
  }
  it_page();
  ship_rlo = nlo < lo ? nlo : lo;               /* filas por mandar: las de este y las del que había */
  ship_rhi = nhi > hi ? nhi : hi;
  ship_lo[back] = nlo; ship_hi[back] = nhi;
  ship_load = id;
  ship_credit = ship_bytes(nt);
  ship_go = 0;
  ship_tile_done = 0;
  ship_map_done = 0;
  ship_ready = 0;
}

static void ship_pick(void);
/* en cada cuadro de la lógica (md_post): qué cuadro de la nave preparar (el primero que alcanza a llegar a tiempo) y si
   el vblank de este cuadro lo muestra */
static void ship_update(void) {
  ship_now = ship_find();
  if (ship_load < 0) ship_pick();
  if (ship_load >= 0) {                         /* lo que cuenta el vblank de este cuadro; ¿se muestra en él? */
    uint16 step = it_load >= 0 ? 0 : SHIP_STEP;   /* (ese vblank carga un grupo nuevo: no queda para la nave) */
    ship_credit = ship_credit > step ? ship_credit - step : 0;
    ship_go = !ship_credit && ship_now >= ship_load;
  }
}
static void ship_pick(void) {
  if (ship_now < 0 || ship_now == ship_shown) return;
  int16 last = INTRO_SHIP_N - 1, t = ship_now;
  uint16 nc, nt;
  for (int16 c = ship_now; c <= last && c <= ship_now + 12; c++) {
    ship_hdr(c, &nc, &nt);
    if (!nc) continue;
    t = c;
    if ((int16)ship_cost(c) <= c - ship_now + 1) break;
  }
  ship_hdr(t, &nc, &nt);
  if (!nc || t == ship_shown) return;
  ship_begin(t);
}

/* en el vblank: manda lo que quepa del cuadro que se prepara; al terminar (y si el juego ya llegó), se muestra */
static void ship_vblank(void) {
  if (ship_load < 0) return;
  uint8 back = ship_front < 0 ? 0 : ship_front ^ 1;
  if (!ship_ready) {
    const uint16 *r = ship_rec(ship_load);
    uint16 nc = r[0], nt = r[1];
    const uint8 *tiles = (const uint8 *)(r + 2 + nc * 2);
    if ((uint32)tiles & 2) tiles += 2;            /* alineados a 4 */
    int16 lines = vb_lines_left() - INTRO_BUDGET_LINES;   /* lo que queda del vblank (140 bytes por línea) */
    uint16 budget = lines > 0 ? (uint16)lines * 140 : 0;
    if (budget > SHIP_BUDGET) budget = SHIP_BUDGET;
    if (!ship_map_done) {
      uint16 plane = back ? MD_VRAM_PLANE_B : MD_VRAM_PLANE_A;
      if (ship_rlo <= ship_rhi)
        for (uint16 row = ship_rlo; row <= ship_rhi; row++)
          vdp_dma_vram(it_buf.ship + row * 32, plane + row * INTRO_PLANE_W * 2, 32);
      ship_map_done = 1;
      uint16 used = (ship_rhi - ship_rlo + 1) * 64;
      budget = budget > used ? budget - used : 0;
    }
    uint16 left = nt - ship_tile_done, now = budget / 32;
    if (now > left) now = left;
    if (now) {
      vdp_dma_vram(tiles + ship_tile_done * 32, ((back ? INTRO_SHIP_BUF1 : INTRO_SHIP_BUF0) + ship_tile_done) * 32, now * 16);
      ship_tile_done += now;
    }
    if (ship_tile_done == nt) ship_ready = 1;
    it_page();
  }
#ifdef MD_M7LOG
  if (ship_go && !ship_ready) md_ship_wait++;
#endif
  if (ship_go && ship_ready) {                  /* (si la carga no llegó, espera: no debería pasar) */
    ship_go = 0;
    ship_front = (int8)back;
    ship_shown = ship_load;
    ship_load = -1;
    ship_ready = 0;
  }
}

static void ship_reset(void) {
  ship_front = -1; ship_shown = ship_load = ship_now = -1; ship_hint = 0; ship_go = 0;
  ship_lo[0] = ship_lo[1] = 28; ship_hi[0] = ship_hi[1] = 0;
}

/* ------------------------------------------------------------------ cuadro */
/* suma de color fijo del SNES sobre todo (el destello blanco del vuelo): r | g << 5 | b << 10 (0: nada) */
static uint16 it_fixed(void) {
  if (!(reg_CGADSUB & 0x3F) || (reg_CGADSUB & 0x80) || (reg_CGWSEL & 2)) return 0;
  uint16 f = 0;
  for (uint16 i = 0; i < 3; i++) {
    uint8 c = reg_COLDATA[i];
    uint16 v = c & 31;
    if (c & 0x20) f |= v;
    if (c & 0x40) f |= v << 5;
    if (c & 0x80) f |= v << 10;
  }
  return f;
}

static uint8 it_slots[64], it_nslots;            /* ranuras de la CRAM con color en el grupo */
static const IntroGroup *it_cram_group;

static void it_build_cram(const IntroGroup *G, uint8 bright, uint16 fixed, uint8 all) {
  const uint8 *ch = kMdCh[bright];
  const uint8 *m = G->cram_map;
  if (G != it_cram_group) {                     /* grupo nuevo: la lista de ranuras y la CRAM en negro */
    it_cram_group = G;
    it_nslots = 0;
    for (uint16 s = 0; s < 64; s++) {
      if (m[s] != 0xFF) it_slots[it_nslots++] = (uint8)s;
      else if (cram[s]) { cram[s] = 0; cram_dirty = 1; }
    }
    all = 1;
  }
  all |= bright != it_cram_bright || fixed != it_cram_fixed;
  it_cram_bright = bright;
  it_cram_fixed = fixed;
  const uint16 fr = fixed & 31, fg = (fixed >> 5) & 31, fb = fixed >> 10;
  for (uint16 k = 0; k < it_nslots; k++) {
    uint8 s = it_slots[k];
    uint16 raw = palette_buffer[m[s]] & 0x7FFF;
    if (!all && raw == it_cram_src[s]) continue;
    it_cram_src[s] = raw;
    uint16 r = raw & 31, g = (raw >> 5) & 31, b = raw >> 10;
    if (fixed) {
      r += fr; g += fg; b += fb;
      if (r > 31) r = 31;
      if (g > 31) g = 31;
      if (b > 31) b = 31;
    }
    uint16 c = (uint16)(ch[b] << 9 | ch[g] << 5 | ch[r] << 1);
    if (c != cram[s]) { cram[s] = c; cram_dirty = 1; }
  }
}

/* el cursor del diario (objeto de sprite 15) no va sobre un recuerdo (en el SNES lo tapa la franja negra de BG3;
   al terminar el recuerdo el juego ya lo vuelve a dibujar) */
static uint8 it_skip_cursor(uint16 sm, uint16 x, uint16 y) {
  if (sm != cinematicspr_whattodraw[15]) return 0;
  if (x != (uint16)(cinematicbg_arr7[15] - layer1_x_pos) || y != (uint16)(cinematicbg_arr8[15] - layer1_y_pos)) return 0;
  uint16 f;
  uint16 sc = IntroScene(it_scene < 0 ? INTRO_SC_CAPT : (uint16)it_scene, &f);
  return sc >= INTRO_SC_MB && sc <= INTRO_SC_LAB;
}

/* después del cuadro del motor (md_frame) */
static void intro_frame(void) {
  uint16 f;
  uint16 sc = IntroScene(it_scene < 0 ? INTRO_SC_CAPT : (uint16)it_scene, &f);
  it_draw_ok = 0;
  int8 g = (int8)kIntroSceneGroup[sc];
  uint8 newgroup = g != it_group && g != it_load;
  if (newgroup) it_load = g;
  int8 cg = it_load >= 0 ? it_load : it_group;   /* grupo con el que se arma este cuadro */
  uint8 fly = sc == INTRO_SC_FLY;
  if (fly && it_scene != INTRO_SC_FLY) {         /* empieza el vuelo: los dos planos para la nave */
    ship_reset();
    it_dest = 0;                                 /* el búfer del plano A pasa a la nave */
  }
  it_scene = (int8)sc;
  it_factor = f;
  it_page();
  const IntroGroup *G = &kIntroGroups[cg];
  /* plano A: lo que el SNES manda a la VRAM (HandleCinematicBgObjects) */
  uint8 upload = (cinematic_enable_objs & 0x8000) && (cinematic_enable_bg_tilemap & 0x8000) && (reg_BGMODE & 7) != 7;
  if (upload && (cinematic_text_dirty || !it_upload || cinematicbg_var3 != it_dest)) {   /* solo si algo cambió */
    cinematic_text_dirty = 0;
    if (cinematicbg_var3 != it_dest) {
      it_dest = cinematicbg_var3;
      for (uint16 i = 0; i < 1024; i++) it_shadow[i] = 0;
      it_dirty = 0xFFFFFFFF;
    }
    const uint32 *s = (const uint32 *)cinematic_text_tilemap;
    uint32 *d = (uint32 *)it_shadow;
    for (uint16 r = 0; r < 30; r++) {
      uint8 ch = 0;
      for (uint16 k = 0; k < 16; k++, s++, d++)
        if (*s != *d) { *d = *s; ch = 1; }
      if (ch) it_dirty |= 1UL << r;
    }
  }
  it_upload = upload;
  if (newgroup) it_dirty = 0xFFFFFFFF;            /* tablas nuevas: todo el plano otra vez */
  /* ojos del casco (UpdateSamusEyesTilemap: filas 13-14 de 7E:3800 al mapa de BG2) */
  if (cg == (int8)kIntroSceneGroup[INTRO_SC_TEXT] && (cinematic_enable_objs & 0x8000) && (reg_BGMODE & 7) != 7) {
    uint8 any = newgroup;
    for (uint16 k = 0; k < 64; k++) {
      uint16 w = cinematic_bg_tilemap[416 + k];
      if (w != it_eyes_raw[k] || newgroup) {
        it_eyes_raw[k] = w;
        it_eyes[k] = it_cell(G->b_idx, G->b_val, w, 0);
        any = 1;
      }
    }
    if (any) it_eyes_n = 64;
  }
  /* sprites: Samus solo en los recuerdos con Samus (en el fundido cruzado el motor la dibuja igual) */
  if (!G->samus) {
    GenSpr *dd = spr;
    for (GenSpr *q = spr; q < sp_ptr; q++) {
      uint16 t = q->attr & 0x7FF;
      if (t >= SAMUS_BASE - 1 && t < SAMUS_BASE + 32) continue;
      *dd++ = *q;
    }
    for (GenSpr *q = spr; q < dd; q++) q->size_link = (q->size_link & 0x0F00) | (uint16)(q - spr + 1);
    sp_ptr = dd;
    nspr = (uint16)(dd - spr);
  }
  /* Fundido cruzado entre el diario y un recuerdo (TM = BG2 + BG3: el SNES mezcla la otra escena con el color math).
     La Genesis muestra una sola escena a la vez (una mitad del fundido cada una): en la del diario no van los sprites
     del recuerdo, y en la del recuerdo no va el plano del texto (el juego ya escribe allí la página siguiente). */
  uint8 xfade = reg_TM == 0x06;
  it_hide_a = xfade && sc != INTRO_SC_TEXT;
  if (xfade && sc == INTRO_SC_TEXT) { sp_ptr = spr; nspr = 0; }
  /* todo 2 líneas más arriba que en el juego (medido contra el SNES: en la intro no está la ventana del HUD) */
  for (GenSpr *q = spr; q < sp_ptr; q++) q->y -= INTRO_DY;
  /* colores */
  uint8 bright = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 15);
  it_build_cram(G, (uint8)((bright * it_factor) >> 6), it_fixed(), newgroup);
  /* scroll */
  if (sc >= INTRO_SC_MB && sc <= INTRO_SC_LAB) { it_bh = reg_BG1HOFS; it_bv = reg_BG1VOFS; }
  else { it_bh = reg_BG2HOFS; it_bv = reg_BG2VOFS; }
  if (sc == INTRO_SC_SPACE) { it_ah = reg_BG1HOFS; it_av = reg_BG1VOFS; }
  else { it_ah = reg_BG3HOFS; it_av = reg_BG3VOFS; }
  it_bplane_a = sc == INTRO_SC_SPACE || sc == INTRO_SC_CAPT;   /* sin plano B: el mismo mapa que el A */
  if (fly) ship_update();
  it_page_back();
}

/* en el vblank, después de los sprites y de los tiles de Samus (md_vblank) */
static void intro_vblank(void) {
  it_page();
  if (it_load >= 0) {                           /* grupo nuevo: con la pantalla apagada (la escena está en negro) */
    md_vb_room = 1;                             /* (no cuenta para md_vb_need) */
    const IntroGroup *G = &kIntroGroups[it_load];
    vdp_display(0);
    vdp_reg(0x10, 0x01);                        /* planos de 64x32 */
    vdp_reg(0x12, 0x00);                        /* sin ventana (HUD) */
    vdp_reg(0x0B, 0x00);                        /* scroll de la pantalla entera */
    for (uint16 i = 0; i < G->nseg; i++)
      vdp_dma_vram(G->seg[i].data, G->seg[i].tile * 32, G->seg[i].n * 16);
    vdp_fill_vram(MD_VRAM_PLANE_A, 0, 64 * 32);
    if (G->b_map)
      for (uint16 y = 0; y < 32; y++) vdp_dma_vram(G->b_map + y * 32, MD_VRAM_PLANE_B + y * INTRO_PLANE_W * 2, 32);
    else
      vdp_fill_vram(MD_VRAM_PLANE_B, 0, 64 * 32);
    it_group = it_load;
    it_load = -1;
    VDP_CTRL32 = VDP_CMD_CRAM_W(0);
    for (uint16 i = 0; i < 64; i++) VDP_DATA = cram[i];
    cram_dirty = 0;
    vdp_display(1);
  } else if (cram_dirty) {
    cram_dirty = 0;
    VDP_CTRL32 = VDP_CMD_CRAM_W(0);
    for (uint16 i = 0; i < 64; i++) VDP_DATA = cram[i];
  }
  const IntroGroup *G = &kIntroGroups[it_group];
  uint8 fly = it_scene == INTRO_SC_FLY;
  if (fly) {
    ship_vblank();
    vdp_reg(0x04, MD_VRAM_PLANE_B >> 13);
    vdp_set_vram_write(MD_VRAM_HSCROLL);
    VDP_DATA = ship_front == 0 ? 0 : (uint16)-256;
    VDP_DATA = ship_front == 1 ? 0 : (uint16)-256;
    vdp_set_vsram_write(0);
    VDP_DATA = INTRO_DY;
    VDP_DATA = INTRO_DY;
  } else {
    /* filas del plano A (convertidas aquí: pocas por cuadro; las demás, en los siguientes) */
    uint32 rows = it_dirty;
    for (uint16 r = 0; rows && r < 32; r++) {
      if (!(rows & (1UL << r))) continue;
      if (vb_lines_left() < INTRO_BUDGET_LINES + 4) break;
      vdp_set_vram_write(MD_VRAM_PLANE_A + r * INTRO_PLANE_W * 2);
      const uint16 *s = it_shadow + r * 32;
      for (uint16 x = 0; x < 32; x++) VDP_DATA = it_cell(G->a_idx, G->a_val, s[x], 1);
      rows &= ~(1UL << r);
    }
    it_dirty = rows;
    if (it_eyes_n) {                            /* ojos: filas 13-14 del mapa de BG2 */
      vdp_set_vram_write(MD_VRAM_PLANE_B + 13 * INTRO_PLANE_W * 2);
      for (uint16 k = 0; k < 32; k++) VDP_DATA = it_eyes[k];
      vdp_set_vram_write(MD_VRAM_PLANE_B + 14 * INTRO_PLANE_W * 2);
      for (uint16 k = 32; k < 64; k++) VDP_DATA = it_eyes[k];
      it_eyes_n = 0;
    }
    vdp_reg(0x04, (it_bplane_a ? MD_VRAM_PLANE_A : MD_VRAM_PLANE_B) >> 13);
    vdp_set_vram_write(MD_VRAM_HSCROLL);
    VDP_DATA = (uint16)(-it_ah - (it_hide_a ? 256 : 0));   /* (columnas 32-63 del plano A: vacías) */
    VDP_DATA = (uint16)-(it_bplane_a ? it_ah : it_bh);
    vdp_set_vsram_write(0);
    VDP_DATA = it_av + INTRO_DY;
    VDP_DATA = (it_bplane_a ? it_av : it_bv) + INTRO_DY;
  }
  it_page_back();
}

/* IntroGfx ($8B: los DecompressToMem y DMA de cada parte): la Genesis sigue la escena por los registros; aquí solo
   se marca que empieza la intro */
void IntroGfx(uint16 which) {
  if (which == INTRO_GFX_TEXT && !intro_on) {
    md_sync();                                  /* lo del vblank cambia: que antes tome el cuadro anterior */
    intro_on = 1;
    it_scene = -1; it_group = -1; it_load = -1;
    it_dest = 0x4C00;
    it_page();                                  /* el mapa de BG3 inicial ($97:8D12): el del grupo CAPT */
    for (uint16 i = 0; i < 1024; i++) it_shadow[i] = kIntroBg3Initial[i];
    it_page_back();
    it_dirty = 0xFFFFFFFF;
    it_cram_bright = 0xFF;
    for (uint16 i = 0; i < 64; i++) it_eyes_raw[i] = 0xFFFF;
  }
}
