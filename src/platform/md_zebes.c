/* MegaMetroid — capa de la Genesis: salas de Zebes (docs/37).
 *
 * En Ceres cada sala es una imagen completa preparada en el PC (mkmd.py). En Zebes las salas son grandes y los
 * PLMs cambian bloques (puertas, bloques rotos): aquí se dibuja como el SNES, desde el nivel (level_get) y la
 * tabla de bloques de la sala (mkzebes.py, gen/zebes.c): cada bloque de 16x16 son 4 celdas de la Genesis.
 *
 * Los planos son de 64x64 celdas. De cada capa se mantiene dibujada una ventana de ZW x ZH celdas alrededor de la
 * cámara; cuando la cámara se mueve entran columnas o filas nuevas (se arman en md_frame, se copian en el vblank).
 * Los datos de la sala (tiles, tabla de bloques) están en una página del SSF2, igual que los niveles: para leer
 * ambos se juntan primero los bloques (página del nivel) y después se buscan sus celdas (página de gráficos).
 *
 * Paletas: líneas 0 y 1 de la CRAM para la sala (cada color sale de un índice de palette_buffer: kZebesGfx.src),
 * 2 Samus, 3 HUD y objetos (como en Ceres: de la sala base de Ceres). */
#include <string.h>
#include "samus.h"
#include "game.h"
#include "room.h"
#include "md.h"
#include "md_vdp.h"
#include "md_data.h"
#include "zebes.h"
#include "md_zebes.h"
#include "nmath.h"

uint8 md_zebes;
uint8 md_cur_page = 7;                    /* página del SSF2 en la ventana 0x380000 (sombra del registro) */
volatile uint8 md_hw_page = 7;            /* lo que tiene el registro (también con las páginas de la intro) */
uint8 md_level_page = 7;                  /* la del nivel de la sala (nt_map_level_page) */
static const ZebesGfx *zg;
static const ZebesObj *zo;                /* enemigos de la sala (mkzobj.py) o NULL */
uint16 zobj_col[16];                      /* colores 1-15 de la línea 3 debajo del HUD (los escribe md_hint_obj):
                                             los de los enemigos de la sala y, en 9-13, el rayo (mkzobj.py) */
static MdRoom zroom;                      /* la sala base de Ceres con la paleta y los planos de la de Zebes */
static uint8 zsrc[64];
extern uint16 md_scratch[1440];          /* (RAM compartida, md_engine.c) */
#define meta_idx md_scratch               /* bloque (10 bits) -> 1 + su número en la tabla de la sala (0: no está) */
#define META_IDX_BYTES 2048             /* bloque (10 bits) -> 1 + su número en la tabla de la sala (0: no está) */
#define ZT_BASE 448                       /* primer tile de la sala (antes: los objetos de la sala base de Ceres) */
#define ZW 36                             /* ventana dibujada: columnas */
#define ZH 32                             /*                   filas */
static int16 wx[2], wy[2];                /* esquina de la ventana dibujada de cada capa (celdas) */
static uint8 drawn[2];

static inline void map_page(uint8 p) {        /* con barreras: las lecturas de la página no cruzan el cambio */
  __asm__ volatile("" ::: "memory");
  md_cur_page = p;
  md_hw_page = p;
  *(volatile uint8 *)0xA130FF = p;
  __asm__ volatile("" ::: "memory");
}

/* ---------------------------------------------------------------- colas para el vblank */
#define QCOL 6
#define QROW 6
static uint16 qcol_buf[QCOL][ZH];
static int16 qcol_x[QCOL], qcol_y[QCOL];
static uint8 qcol_l[QCOL], qrow_l[QROW];
static uint16 qrow_buf[QROW][ZW], qrow_addr[QROW], qrow_n[QROW];
static uint8 ncol, nrow;
#define QBLK 24
static uint16 qblk_addr[QBLK], qblk_c[QBLK][4];
static uint8 nblk, full_redraw;
#define QANIM 6
static uint16 qanim[QANIM][2];            /* tiles animados: (origen en $87, VRAM del SNES) */
static uint8 nanim;
/* lo que piden los ganchos durante la lógica del cuadro (puede correr mientras el vblank todavía toma el cuadro
   anterior): bloques cambiados y tiles animados; md_post (zebes_take) los pasa a las colas del vblank */
static uint16 lblk[QBLK];
static uint8 nlblk, lfull;
static uint16 lanim[QANIM][2];
static uint8 nlanim;

/* bloque de la capa 1 (nivel, con los cambios de los PLMs: página del nivel) o de la capa 2 propia de la sala
   (zg->l2: página de gráficos) */
static inline uint8 in_room(int16 bx, int16 by) {
  return bx >= 0 && by >= 0 && bx < (int16)room_width_in_blocks && by < (int16)room_height_in_blocks;
}
static inline uint16 block_at(int16 bx, int16 by) {
  return in_room(bx, by) ? level_get(Mul16(by, room_width_in_blocks) + bx) : 0xFFFF;
}
static inline uint16 block2_at(int16 bx, int16 by) {   /* con la página de gráficos puesta */
  return in_room(bx, by) ? zg->l2[Mul16(by, room_width_in_blocks) + bx] : 0xFFFF;
}
static inline uint16 plane_of(uint8 L) { return L ? MD_VRAM_PLANE_B : MD_VRAM_PLANE_A; }
/* las 4 celdas de un bloque (arriba-izq, abajo-izq, arriba-der, abajo-der), con la página de gráficos puesta */
static inline const uint16 *cells_of(uint16 w) {
  static const uint16 kEmpty[4] = { 0, 0, 0, 0 };
  if (w == 0xFFFF) return kEmpty;
  uint16 m = meta_idx[w & 0x3FF];
  if (!m) return kEmpty;
  return zg->cells + (uint16)((m - 1) * 16 + ((w >> 8) & 0xC));
}
static inline uint16 tile_fix(uint16 c) { return (c & 0x7FF) ? c + ZT_BASE : c; }

/* una columna de celdas (cx) desde la fila cy0, ZH celdas, en buf */
static void build_column(uint8 L, int16 cx, int16 cy0, uint16 *buf) {
  uint16 blk[ZH / 2 + 1];
  int16 by0 = cy0 >> 1, bx = cx >> 1;
  uint16 nb = ((cy0 + ZH - 1) >> 1) - by0 + 1;
  if (!L) for (uint16 i = 0; i < nb; i++) blk[i] = block_at(bx, by0 + i);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  if (L) for (uint16 i = 0; i < nb; i++) blk[i] = block2_at(bx, by0 + i);
  uint16 q = (cx & 1) << 1;
  for (int16 cy = cy0; cy < cy0 + ZH; cy++) {
    const uint16 *c = cells_of(blk[(cy >> 1) - by0]);
    buf[cy - cy0] = tile_fix(c[q + (cy & 1)]);
  }
  map_page(lp);
}
static void build_row(uint8 L, int16 cy, int16 cx0, uint16 *buf) {
  uint16 blk[ZW / 2 + 1];
  int16 bx0 = cx0 >> 1, by = cy >> 1;
  uint16 nb = ((cx0 + ZW - 1) >> 1) - bx0 + 1;
  if (!L) for (uint16 i = 0; i < nb; i++) blk[i] = block_at(bx0 + i, by);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  if (L) for (uint16 i = 0; i < nb; i++) blk[i] = block2_at(bx0 + i, by);
  uint16 q = cy & 1;
  for (int16 cx = cx0; cx < cx0 + ZW; cx++) {
    const uint16 *c = cells_of(blk[(cx >> 1) - bx0]);
    buf[cx - cx0] = tile_fix(c[((cx & 1) << 1) + q]);
  }
  map_page(lp);
}

/* escribir una columna (con el VDP en pasos de 128: una fila del plano): la fila del plano es cy & 63, y la columna
   puede dar la vuelta al borde de abajo */
static void put_column_cpu(uint8 L, int16 cx, int16 cy0, const uint16 *buf) {
  uint16 r0 = cy0 & 63, n1 = 64 - r0 < ZH ? 64 - r0 : ZH;
  vdp_set_vram_write(plane_of(L) + r0 * 128 + ((cx & 63) << 1));
  for (uint16 i = 0; i < n1; i++) VDP_DATA = buf[i];
  if (n1 < ZH) {
    vdp_set_vram_write(plane_of(L) + ((cx & 63) << 1));
    for (uint16 i = n1; i < ZH; i++) VDP_DATA = buf[i];
  }
}
static void put_column_dma(uint8 L, int16 cx, int16 cy0, const uint16 *buf) {
  uint16 r0 = cy0 & 63, n1 = 64 - r0 < ZH ? 64 - r0 : ZH;
  vdp_dma_vram(buf, plane_of(L) + r0 * 128 + ((cx & 63) << 1), n1);
  if (n1 < ZH) vdp_dma_vram(buf + n1, plane_of(L) + ((cx & 63) << 1), ZH - n1);
}
/* cielo de la superficie (zg->sky): fila R de la sala = 32 celdas, repetidas a lo ancho del plano (64) */
static void sky_row(int16 R, uint16 *buf) {      /* con la página de gráficos puesta */
  if (R < 0 || R >= 160) { for (uint16 i = 0; i < 64; i++) buf[i] = 0; return; }
  const uint16 *c = zg->bg2 + (uint16)R * 32;
  for (uint16 i = 0; i < 32; i++) buf[i] = buf[i + 32] = tile_fix(c[i]);
}
#define QSKY 4
static uint16 qsky_buf[QSKY][64];
static int16 qsky_r[QSKY];
static uint8 nsky;
static void draw_sky(void) {                   /* (VDP libre) las ZH filas de la ventana */
  uint16 buf[64];
  uint8 lp = md_cur_page;
  map_page(zg->page);
  for (int16 R = wy[1]; R < wy[1] + ZH; R++) {
    sky_row(R, buf);
    vdp_set_vram_write(MD_VRAM_PLANE_B + (uint16)(R & 63) * 128);
    for (uint16 i = 0; i < 64; i++) VDP_DATA = buf[i];
  }
  map_page(lp);
}

static void draw_window(uint8 L) {            /* (VDP libre) toda la ventana de la capa */
  if (L && zg->sky) { draw_sky(); return; }
  uint16 buf[ZH];
  vdp_reg(0x0F, 128);
  for (int16 cx = wx[L]; cx < wx[L] + ZW; cx++) {
    build_column(L, cx, wy[L], buf);
    put_column_cpu(L, cx, wy[L], buf);
  }
  vdp_reg(0x0F, 2);
}

/* ---------------------------------------------------------------- sala nueva */
const MdRoom *zebes_use(void) {
  int lo = 0, hi = (int)(sizeof kZebesStates / sizeof kZebesStates[0]) - 1, f = -1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    if (kZebesStates[m].state == roomdefroomstate_ptr) { f = m; break; }
    if (kZebesStates[m].state < roomdefroomstate_ptr) lo = m + 1; else hi = m - 1;
  }
  if (f < 0) { md_zebes = 0; return 0; }
  zg = &kZebesGfx[kZebesStates[f].gfx];
  zo = kZebesStates[f].obj != 0xFFFF ? &kZebesObj[kZebesStates[f].obj] : 0;
  const MdRoom *base = &kMdRooms[ZEBES_BASE_ROOM];
  zroom = *base;
  zroom.room = room_ptr;
  zroom.state = roomdefroomstate_ptr;
  zroom.plane_w = 64; zroom.plane_h = 64;
  zroom.planeA = 0; zroom.planeB = 0;
  /* fondo de biblioteca (64x32 celdas en el plano B, como el BG2 del SNES): b_mode 1, el plano B se mueve con los
     registros del SNES (BG2HOFS = bg2_x_scroll + layer2_x_pos, door.c); p. ej. 9804, la sala del Bomb Torizo, con
     bg2_x_scroll = 256 */
  zroom.b_mode = !zg->l2 && !zg->sky && zg->bg2 ? 1 : 0;
  zroom.patch = 0; zroom.npatch = 0; zroom.wall = 0; zroom.door_sm = 0; zroom.flash_line = 0;
  memcpy(zsrc, base->src, 64);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  for (uint16 i = 0; i < 32; i++)
    zsrc[i] = i < (uint16)zg->npal * 16 ? (uint8)zg->src[i] : 0xFF;
  memset(meta_idx, 0, META_IDX_BYTES);
  for (uint16 i = 0; i < zg->nmeta; i++) meta_idx[zg->ids[i] & 0x3FF] = i + 1;
  if (zo && zg->npal == 1) {                 /* jefe de colores propios (mkzobj.BOSS_LINE): la línea 1 es de los objetos */
    map_page(zo->page);
    for (uint16 i = 1; i < 16; i++) {
      uint16 s = zo->src[14 + i];
      zsrc[16 + i] = s == 0xFFFF ? 0xFF : (uint8)s;
    }
  }
  map_page(lp);
  zsrc[0] = 0;                               /* color de fondo: palette_buffer[0] */
  zroom.src = zsrc;
  /* lista del fundido (NT_FADE_LIST, AdvancePaletteFadeForAllPalettes): la de la sala base (Samus, HUD, rayo) más
     los colores de esta sala y de sus objetos (el despertar del Bomb Torizo es un fundido de sus paletas) */
  {
    static uint8 zfade[256];
    uint8 seen[32];
    uint16 n = 0;
    memset(seen, 0, sizeof seen);
    #define ZFADE_ADD(c) do { uint8 c_ = (c); if (!(seen[c_ >> 3] & (1 << (c_ & 7)))) { seen[c_ >> 3] |= 1 << (c_ & 7); zfade[n++] = c_; } } while (0)
    for (uint16 i = 0; i < base->nfade; i++) ZFADE_ADD(base->fade[i]);
    for (uint16 i = 1; i < 64; i++) if (zsrc[i] != 0xFF) ZFADE_ADD(zsrc[i]);
    if (zo) {
      map_page(zo->page);
      for (uint16 i = 0; i < 30; i++) if (zo->src[i] != 0xFFFF) ZFADE_ADD((uint8)zo->src[i]);
      map_page(lp);
    }
    #undef ZFADE_ADD
    zroom.fade = zfade; zroom.nfade = n;
  }
  md_zebes = 1;
  drawn[0] = drawn[1] = 0;
  ncol = nrow = nblk = nsky = 0;
  nanim = nlanim = nlblk = lfull = 0;
  full_redraw = 0;
  return &zroom;
}

/* tiles de la sala o de sus objetos: los tramos (en la página rp) dicen qué tiles del depósito común (kZebesPool,
   mkzebes.py) van, en orden, desde el tile vt de la VRAM */
static void zebes_dma_runs(uint8 rp, const uint16 *r, uint16 nr, uint16 vt) {
  for (; nr; nr--, r += 2) {
    map_page(rp);
    uint16 j = r[0], n = r[1];
    while (n) {
      uint16 o = j & (ZEBES_POOL_CHUNK - 1), k = ZEBES_POOL_CHUNK - o;
      const ZebesPool *pc = &kZebesPool[j / ZEBES_POOL_CHUNK];
      if (k > n) k = n;
      map_page(pc->page);
      vdp_dma_vram(pc->tiles + (uint32)o * 32, vt * 32, k * 16);
      vt += k; j += k; n -= k;
    }
  }
}

/* (pantalla apagada) tiles de la sala, los de los objetos de la sala base y los planos */
void zebes_load_vram(uint16 ax, uint16 ay, uint16 bx, uint16 by) {
  const MdRoom *base = &kMdRooms[ZEBES_BASE_ROOM];
  vdp_reg(0x10, 0x11);                                      /* 64x64 */
  if (base->tiles_total > base->bg_tiles)                   /* rayo, bombas...: donde los espera la sala base */
    vdp_dma_vram(base->tiles + base->bg_tiles * 32, (BG_BASE + base->bg_tiles) * 32,
                 (base->tiles_total - base->bg_tiles) * 16);
  uint8 lp = md_cur_page;
  zebes_dma_runs(zg->page, zg->runs, zg->nruns, ZT_BASE);
  if (zo) zebes_dma_runs(zo->page, zo->runs, zo->nruns, zo->base);
  map_page(lp);
  vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
  vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
  /* capa 1: la ventana alrededor de la cámara */
  wx[0] = ((int16)ax >> 3) - 1; wy[0] = ((int16)ay >> 3) - 1; drawn[0] = 1;
  draw_window(0);
  /* capa 2: propia de la sala (como la 1) o fondo de biblioteca (64x32 fijo, repetido abajo: da la vuelta a los
     256 píxeles como en el SNES) */
  if (zg->l2) {
    wx[1] = ((int16)bx >> 3) - 1; wy[1] = ((int16)by >> 3) - 1; drawn[1] = 1;
    draw_window(1);
  } else if (zg->sky) {
    wx[1] = 0; wy[1] = ((int16)ay >> 3) - 1; drawn[1] = 1;
    draw_sky();
  } else if (zg->bg2) {
    map_page(zg->page);
    for (uint16 h = 0; h < 2; h++) {
      vdp_set_vram_write(MD_VRAM_PLANE_B + h * 32 * 128);
      const uint16 *c = zg->bg2;
      for (uint16 i = 0; i < 64 * 32; i++) VDP_DATA = tile_fix(*c++);
    }
    map_page(lp);
  }
}

/* ---------------------------------------------------------------- cada cuadro: columnas y filas que entran */
static void queue_col(uint8 L, int16 cx, int16 cy0) {
  if (ncol >= QCOL) { full_redraw |= 1 << L; return; }
  build_column(L, cx, cy0, qcol_buf[ncol]);
  qcol_x[ncol] = cx; qcol_y[ncol] = cy0; qcol_l[ncol] = L;
  ncol++;
}
static void queue_row(uint8 L, int16 cy, int16 cx0) {
  if (nrow >= QROW) { full_redraw |= 1 << L; return; }
  build_row(L, cy, cx0, qrow_buf[nrow]);
  qrow_l[nrow] = L;
  qrow_addr[nrow] = (uint16)((cy & 63) * 128);
  qrow_n[nrow] = (uint16)(cx0 & 63);
  nrow++;
}

static void follow_sky(uint16 y) {
  int16 cy = ((int16)y >> 3) - 1;
  if (cy - wy[1] > 4 || wy[1] - cy > 4) { full_redraw |= 2; wy[1] = cy; return; }
  while (wy[1] != cy) {
    int16 R;
    if (wy[1] < cy) { R = wy[1] + ZH; wy[1]++; } else { wy[1]--; R = wy[1]; }
    if (nsky >= QSKY) { full_redraw |= 2; continue; }
    uint8 lp = md_cur_page;
    map_page(zg->page);
    sky_row(R, qsky_buf[nsky]);
    map_page(lp);
    qsky_r[nsky++] = R;
  }
}

static void follow(uint8 L, uint16 x, uint16 y) {
  if (L && zg->sky) { follow_sky(y); return; }
  /* con signo: el temblor (HandleRoomShaking) puede dejar la cámara en -1..-3 en el borde de la sala; sin signo eso
     era un salto de 8000 celdas y se volvía a dibujar toda la ventana en cada cuadro del temblor */
  int16 cx = ((int16)x >> 3) - 1, cy = ((int16)y >> 3) - 1;
  if (cx - wx[L] > 4 || wx[L] - cx > 4 || cy - wy[L] > 4 || wy[L] - cy > 4) {
    full_redraw |= 1 << L;
    wx[L] = cx; wy[L] = cy;
    return;
  }
  while (wx[L] < cx) { queue_col(L, wx[L] + ZW, wy[L]); wx[L]++; }
  while (wx[L] > cx) { wx[L]--; queue_col(L, wx[L], wy[L]); }
  while (wy[L] < cy) { queue_row(L, wy[L] + ZH, wx[L]); wy[L]++; }
  while (wy[L] > cy) { wy[L]--; queue_row(L, wy[L], wx[L]); }
}

void zebes_frame(uint16 ax, uint16 ay, uint16 bx, uint16 by) {
  if (drawn[0]) follow(0, ax, ay);
  if (drawn[1]) follow(1, bx, zg->sky ? ay : by);   /* el cielo baja con la capa 1 (BG2VOFS = layer1_y_pos) */
}

/* un PLM cambió el bloque i: se anota (zebes_take lo mira con la ventana dibujada) */
void nt_block_changed(uint16 i) {
  if (!md_zebes) return;
  if (nlblk >= QBLK) { lfull = 1; return; }
  lblk[nlblk++] = i;
}
/* si está en la ventana dibujada, sus 4 celdas otra vez */
static void block_changed(uint16 i) {
  if (!drawn[0] || !room_width_in_blocks) return;
  int16 by = (int16)(i / room_width_in_blocks), bx = (int16)(i - (uint16)by * room_width_in_blocks);
  int16 cx = bx << 1, cy = by << 1;
  if (cx + 1 < wx[0] || cx >= wx[0] + ZW || cy + 1 < wy[0] || cy >= wy[0] + ZH) return;
  if (nblk >= QBLK) { full_redraw = 1; return; }
  uint16 w = block_at(bx, by);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  const uint16 *c = cells_of(w);
  for (uint16 k = 0; k < 4; k++) qblk_c[nblk][k] = tile_fix(c[k]);
  map_page(lp);
  qblk_addr[nblk] = MD_VRAM_PLANE_A + (uint16)(((cy & 63) * 64 + (cx & 63)) * 2);
  nblk++;
}

/* ---------------------------------------------------------------- vblank */
/* tiles animados ($87): el motor dice qué trozo va a qué VRAM del SNES; aquí se suben los tiles de la sala que
   salen de ese trozo (ya convertidos en el PC: mkzebes.py) */
void nt_animtiles_transfer(uint16 src, uint16 size, uint16 vram) {
  (void)size;
  if (md_zebes && zg && zg->nanim && nlanim < QANIM) { lanim[nlanim][0] = src; lanim[nlanim][1] = vram; nlanim++; }
}

/* (md_post, con el vblank libre) lo de la lógica pasa a las colas del vblank */
void zebes_take(void) {
  if (lfull) { full_redraw = 1; lfull = 0; }
  for (uint16 k = 0; k < nlblk; k++) block_changed(lblk[k]);
  nlblk = 0;
  for (uint16 k = 0; k < nlanim && nanim < QANIM; k++) { qanim[nanim][0] = lanim[k][0]; qanim[nanim][1] = lanim[k][1]; nanim++; }
  nlanim = 0;
}
static void anim_vblank(void) {
  uint8 lp = md_cur_page;
  map_page(zg->page);
  for (uint16 q = 0; q < nanim; q++) {
    int lo = 0, hi = zg->nanim - 1;
    while (lo <= hi) {
      int m = (lo + hi) >> 1;
      const uint16 *e = zg->anim + 4 * m;
      int32 d = (int32)e[0] - qanim[q][0];
      if (!d) d = (int32)e[1] - qanim[q][1];
      if (!d) {
        const uint16 *t = zg->anim_data + e[2];
        for (uint16 n = e[3]; n; n--, t += 17) {
          vdp_set_vram_write((ZT_BASE + t[0]) * 32);
          for (uint16 w = 1; w <= 16; w++) VDP_DATA = t[w];
        }
        break;
      }
      if (d < 0) lo = m + 1; else hi = m - 1;
    }
  }
  nanim = 0;
  map_page(lp);
}

void zebes_vblank(void) {
  if (nanim) anim_vblank();
  if (full_redraw) {                          /* salto grande: toda la ventana (rara vez: puertas, temblores) */
    uint8 f = full_redraw;
    full_redraw = 0;
    ncol = nrow = nblk = nsky = 0;
    if (f & 1) draw_window(0);
    if (f & 2) draw_window(1);
    return;
  }
  if (ncol) {
    vdp_reg(0x0F, 128);                       /* columna: de fila en fila del plano (64 celdas) */
    for (uint16 k = 0; k < ncol; k++) put_column_dma(qcol_l[k], qcol_x[k], qcol_y[k], qcol_buf[k]);
    vdp_reg(0x0F, 2);
    ncol = 0;
  }
  for (uint16 k = 0; k < nrow; k++) {         /* fila: puede dar la vuelta al borde del plano (64 celdas) */
    uint16 x0 = qrow_n[k], n1 = 64 - x0 < ZW ? 64 - x0 : ZW;
    uint16 pl = plane_of(qrow_l[k]);
    vdp_dma_vram(qrow_buf[k], pl + qrow_addr[k] + x0 * 2, n1);
    if (n1 < ZW) vdp_dma_vram(qrow_buf[k] + n1, pl + qrow_addr[k], ZW - n1);
  }
  nrow = 0;
  for (uint16 k = 0; k < nsky; k++) vdp_dma_vram(qsky_buf[k], MD_VRAM_PLANE_B + (uint16)(qsky_r[k] & 63) * 128, 64);
  nsky = 0;
  for (uint16 k = 0; k < nblk; k++) {
    uint16 a = qblk_addr[k];
    vdp_set_vram_write(a); VDP_DATA = qblk_c[k][0]; VDP_DATA = qblk_c[k][2];
    vdp_set_vram_write(a + 128); VDP_DATA = qblk_c[k][1]; VDP_DATA = qblk_c[k][3];
  }
  nblk = 0;
}

/* cielo: el BG2 del SNES se mueve por franjas (HDMA, hdmaobj.c: scrolling_sky_bg2_hdma_data); aquí scroll por
   línea. Línea l de la Genesis = línea l - 2 del SNES (la imagen va 2 líneas más abajo, ver md_frame). La franja
   de cada línea se busca por su altura en la sala (kHdmaScrollEntrys, $88:AEC1: arriba de cada franja). */
uint8 zebes_sky_on(void) { return md_zebes && zg && zg->sky; }
uint8 zebes_eye_room(void) { return md_zebes && zg && zg->eye; }   /* md_cone.c */
void zebes_sky_lines(uint16 *h, uint16 ax, uint16 ay) {
  /* por franja del cielo: un tramo de líneas con el mismo par (plano A, plano B), escrito de a 32 bits */
  #define SKYE(i, f) B88W(0xAEC1 + 8 * (i) + 2 * (f))
  uint32 *p = (uint32 *)h;
  uint32 a16 = (uint32)(uint16)-ax << 16;
  int16 y = (int16)ay - 2, yend = y + 224;
  uint16 e = 0;
  while (e < 22 && (int16)SKYE(e + 1, 0) <= y) e++;
  while (y < yend) {
    int16 next = e < 22 ? (int16)SKYE(e + 1, 0) : yend;
    if (next > yend) next = yend;
    uint16 d = (uint16)(SKYE(e, 3) - 0x9F80) >> 1;
    uint32 v = a16 | (uint16)-scrolling_sky_bg2_hdma_data[d + 1];
    for (int16 n = next - y; n > 0; n--)
      *p++ = v;
    y = next;
    e++;
  }
}

/* ---------------------------------------------------------------- objetos (enemigos) */
uint8 zebes_obj_on(void) { return md_zebes; }   /* (el rayo va siempre en la línea 3 debajo del HUD) */

/* colores de los objetos: de palette_buffer, como los demás (cada cuadro: son 12) */
static const uint16 *zobj_srcp[15];            /* de dónde sale cada color (zobj_src_prepare) */
static const uint16 kZero;
static const void *zobj_srcp_of;
static void zobj_src_prepare(void) {           /* una vez por sala: los índices de la ROM (página de los objetos) a la RAM */
  zobj_srcp_of = zo;
  uint8 lp = md_cur_page;
  map_page(zo->page);
  for (uint16 i = 0; i < 15; i++) {
    uint16 s = zo->src[i];
    zobj_srcp[i] = s == 0xFFFF ? &kZero : &palette_buffer[s];
  }
  map_page(lp);
}
uint8 zebes_cone_slot(uint16 c) {             /* md_cone.c: el color c (1-15) de la línea 3 no es de ningún enemigo */
  return zo && zobj_srcp_of == zo && zobj_srcp[c - 1] == &kZero;
}
uint16 md_tint_cram(uint16 c);   /* md_engine.c: con el tinte de la matemática de color */
void zebes_obj_colors(void) {
  if (!zo) {                                 /* sala sin enemigos: solo el rayo */
    for (uint16 i = 0; i < 5; i++) zobj_col[8 + i] = md_tint_cram(palette_buffer[0xE1 + i]);
    return;
  }
  if (zobj_srcp_of != zo) zobj_src_prepare();
  const uint16 **p = zobj_srcp;
  uint16 *o = zobj_col;
  for (uint16 i = 0; i < 15; i++) *o++ = md_tint_cram(**p++);
}

void md_put_recs(const uint16 *r, int16 x, int16 y);
/* spritemap de un enemigo: búsqueda binaria en las claves de la sala (spritemap, paleta, tiles -> registros) */
/* los registros de una clave (con la página de los objetos ya puesta) o 0 */
const uint16 *zebes_obj_find(uint16 sm, uint16 pal, uint16 tiles) {
  int lo = 0, hi = zo->nkeys - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    const uint16 *k = zo->keys + 4 * m;
    int32 d = (int32)k[0] - sm;
    if (!d) d = (int32)k[1] - pal;
    if (!d) d = (int32)k[2] - tiles;
    if (!d) return zo->recs + k[3];
    if (d < 0) lo = m + 1; else hi = m - 1;
  }
  return 0;
}

/* proyectiles enemigos de Zebes (md_engine.c): la página de los objetos puesta mientras se dibujan */
uint8 zebes_obj_page(void) { return zo ? zo->page : 0xFF; }
uint16 zebes_obj_end(void) { return zo ? zo->base + zo->ntiles : ZOBJ_BASE; }   /* md_cone.c: tiles libres hasta 1279 */

/* claves ya buscadas (por dispersión del spritemap): casi siempre el mismo enemigo con el mismo cuadro */
static struct { uint16 sm, pal, tiles; const uint16 *rec; const void *zo; } zcache[64];
/* con la página de los objetos puesta: los registros de la clave (de zcache o de la búsqueda) o 0 */
const uint16 *zebes_obj_find_cached(uint16 sm, uint16 pal, uint16 tiles) {
  uint16 h = (sm ^ (sm >> 6) ^ pal ^ tiles) & 63;
  if (zcache[h].zo == zo && zcache[h].sm == sm && zcache[h].pal == pal && zcache[h].tiles == tiles)
    return zcache[h].rec;
  const uint16 *r = zebes_obj_find(sm, pal, tiles);
  zcache[h].sm = sm; zcache[h].pal = pal; zcache[h].tiles = tiles; zcache[h].rec = r; zcache[h].zo = zo;
  return r;
}
uint8 zebes_obj_draw(uint16 sm, uint16 pal, uint16 tiles, int16 x, int16 y) {
  if (!zo) return 0;
  uint8 lp = md_cur_page;
  map_page(zo->page);
  const uint16 *r = zebes_obj_find_cached(sm, pal, tiles);
  if (r) md_put_recs(r, x, y);
  map_page(lp);
  return r != 0;
}

void map_page_ext(uint8 p) { map_page(p); }
