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
#include <stddef.h>
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
#include "enemies.h"

uint8 md_zebes;
uint8 md_cur_page = 7;                    /* página del SSF2 en la ventana 0x380000 (sombra del registro) */
volatile uint8 md_hw_page = 7;            /* lo que tiene el registro (también con las páginas de la intro) */
uint8 md_level_page = 7;                  /* la del nivel de la sala (nt_map_level_page) */
static const ZebesGfx *zg;
static const ZebesObj *zo;                /* enemigos de la sala (mkzobj.py) o NULL */
/* cuerpo del jefe en el BG2 (más abajo: nt_enemy_bg2_transfer) */
extern uint16 cinematic_text_tilemap[];
#define bb_buf cinematic_text_tilemap
#define BB_MAX 1024                               /* palabras (32 filas): Mother Brain sube 2048 bytes; los demás, 1024 */
static const ZebesBossBg2 *zbb;
static uint16 bb_n;                               /* palabras convertidas que esperan al vblank */
static uint8 bb_ever;                             /* el jefe ya subió algo en esta sala */
static uint16 bb_sum;                             /* largo del último mapa convertido (las sumas por fila: bb_rsum) */
static void bb_convert(uint16 n, uint32 rows);
/* suma de cada fila del último mapa: 16 en + 3904; Mother Brain (32 filas, su mapa ocupa 0-2047 y el ácido usa 3200 y
   3968) en + 2400 */
#define bb_rsum ((uint32 *)((uint8 *)cinematic_text_tilemap + (room_ptr == 0xDD58 ? 2400 : 3904)))
static uint32 bb_rows;                            /* filas convertidas que esperan al vblank (bits) */
static uint8 bb_hide;                             /* el BG2 no está en ninguna pantalla del SNES: el plano B vacío */
static void bb_vblank(void);
static uint8 bt_pending;                          /* volver a subir los tiles del cuerpo (0xF000) en el vblank */
static void bt_dma(void);
uint16 zobj_col[16];                      /* colores 1-15 de la línea 3 debajo del HUD (los escribe md_hint_obj):
                                             los de los enemigos de la sala y, en 9-13, el rayo (mkzobj.py) */
static MdRoom zroom;                      /* la sala base de Ceres con la paleta y los planos de la de Zebes */
static uint8 zsrc[64];
extern uint16 md_scratch[1440];          /* (RAM compartida, md_engine.c) */
#define meta_idx md_scratch               /* bloque (10 bits) -> 1 + su número en la tabla de la sala (0: no está) */
#define META_IDX_BYTES 2048             /* bloque (10 bits) -> 1 + su número en la tabla de la sala (0: no está) */
static void meta_build(void);
NT_GAMEHI static uint8 esc_n, esc_on, esc_font_pend;   /* texto de la huida de Mother Brain (más abajo: esc_scan) */
extern volatile uint16 md_snd_frames;     /* md_snd.c: el XGM2 está cargado */
void md_snd_protect(uint8 on);
#define ZT_BASE 448                       /* primer tile de la sala (antes: los objetos de la sala base de Ceres) */
#define ZW 36                             /* ventana dibujada: columnas */
#define ZH 32                             /*                   filas */
static int16 wx[2], wy[2];                /* esquina de la ventana dibujada de cada capa (celdas) */
static uint8 drawn[2];

static inline void map_page(uint8 p) {        /* con barreras: las lecturas de la página no cruzan el cambio */
  __asm__ volatile("" ::: "memory");
  md_cur_page = p;
  if (md_hw_page != p) {                      /* (ya puesta: casi siempre, en el dibujo de los objetos; la interrupción
                                                 guarda y repone la suya) */
    md_hw_page = p;
    *(volatile uint8 *)0xA130FF = p;
  }
  __asm__ volatile("" ::: "memory");
}

/* ---------------------------------------------------------------- colas para el vblank */
#define QCOL 6
#define QROW 6
#define QSKY 4
/* los búferes grandes de Zebes van en la RAM compartida con la intro (md.h: md_ovl, docs/60): la caché de claves de
   los objetos (zcache, se borra en zebes_use) y las colas del vblank (se escriben antes de leerse) */
typedef struct { uint16 sm, pal, tiles; const uint16 *rec; const void *zo; } ZcEnt;
typedef struct {
  ZcEnt zc[64];                                   /* zcache (primero: el ensamblador de zebes_obj_draw lo lee en md_ovl) */
  uint16 qcol_buf[QCOL][ZH], qrow_buf[QROW][ZW], qsky_buf[QSKY][64];
} ZebesOvl;
_Static_assert(sizeof(ZebesOvl) <= MD_OVL_BYTES, "md_ovl: no entran los búferes de Zebes");
#define ZOV ((ZebesOvl *)md_ovl)
#define zcache (ZOV->zc)
#define qcol_buf (ZOV->qcol_buf)
#define qrow_buf (ZOV->qrow_buf)
#define qsky_buf (ZOV->qsky_buf)
static int16 qcol_x[QCOL], qcol_y[QCOL];
static uint8 qcol_l[QCOL], qrow_l[QROW];
static uint16 qrow_addr[QROW], qrow_n[QROW];
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
static uint8 xr_shown[16], xr_any;             /* rayos X: los revelados que se dibujaron (xray_take) */
static uint8 nlblk, lfull;
static uint16 lanim[QANIM][2];
static uint8 nlanim;
/* copias de tiles de los enemigos (EnemyInstr_CopyToVram: el ojo del Bomb Torizo): (origen bajo, banco, destino);
   el vblank sube los tiles de la Genesis que mkzobj.py reconvirtió para esa copia (zo->vcp) */
#define QVCP 6
static uint16 lvcp[QVCP][3], qvcp[QVCP][3];
static uint8 nlvcp, nvcp;

/* bloque de la capa 1 (nivel, con los cambios de los PLMs: página del nivel) o de la capa 2 propia de la sala
   (zg->l2: página de gráficos) */
static void bt_dma(void) {                        /* los tiles que solo usa el cuerpo (mkzebes.py: BOSS_HI_TILE) */
  uint8 lp = md_cur_page;
  map_page(ZEBES_BOSS_PAGE);
  uint16 n = zbb->ntiles, n1 = n > 123 ? 123 : n;   /* 0xF000: hasta el tile 2042; lo que sigue (Kraid), en las filas
                                                      libres de la Window (1802: mkzebes.py BOSS_WIN_TILE) */
  vdp_dma_vram(zbb->tiles, ZEBES_BOSS_HI_TILE * 32, n1 * 16);
  if (n > n1) vdp_dma_vram(zbb->tiles + n1 * 16, 1802 * 32, (n - n1) * 16);
  map_page(lp);
}
/* caché de los tiles del cuerpo (zbb->cache: Crocomire, 312 tiles, ~155 a la vez): BC_N lugares de la VRAM (los 123
   de 0xF000 y las 54 filas de la ventana que no usa el HUD, como la caja de mensaje: md_engine.c); en la RAM, después
   del buffer de las filas (cinematic_text_tilemap + 1024..1823: antes de crocomire_bg2_scroll_hdma_data, bytes 1840-2399) */
#define BC_N_MAX 281                              /* 177 + las filas 36-61 del plano A (104 tiles) en salas de una pantalla */
#define BC_K 400                                  /* tiles del cuerpo como mucho (mkzebes.py lo comprueba; Crocomire con el
                                                     derretido: 387) */
#define BC_B ((BC_N_MAX + 7) / 8)
#define bc_k2s ((uint16 *)((uint8 *)cinematic_text_tilemap + 1024))  /* tile del cuerpo -> 1 + lugar (0: no está; 16 bits:
                                                                        hay más de 255 lugares) */
/* (lo que sigue, después de crocomire_bg2_scroll_hdma_data, bytes 1840-2399: en estas salas el resto de
   cinematic_text_tilemap está libre) */
#define bc_used ((uint8 *)cinematic_text_tilemap + 2400)              /* lugares que usa la subida en curso (bits) */
/* lugares que usa el mapa que se ve y los del que espera al vblank: no se reemplazan mientras haya otros (si no, las
   filas que se ven mostraban tiles nuevos: la boca de Crocomire rota al cambiar) */
#define bc_shown ((uint8 *)cinematic_text_tilemap + 2440)
#define bc_next ((uint8 *)cinematic_text_tilemap + 2480)
#define bc_pend ((uint8 *)cinematic_text_tilemap + 2520)              /* lugares por subir en el vblank (bits) */
#define bc_s2k ((uint16 *)((uint8 *)cinematic_text_tilemap + 2560))  /* lugar -> 1 + tile del cuerpo (0: libre) */
static uint16 BC_N;                               /* lugares de la sala (177 o 281) */
static uint16 bc_rr, bc_npend;
static uint8 bc_atomic;                           /* se reemplazaron lugares que se ven: tiles y filas en un vblank */
static uint8 bb_redo;                             /* volver a convertir (la caché se vació) */
static inline uint16 bc_tile(uint16 s) {
  return s < 123 ? ZEBES_BOSS_HI_TILE + s : s < 177 ? 1802 + (s - 123) : (MD_VRAM_PLANE_A + 36 * 128) / 32 + (s - 177);
}
static void bc_reset(void) {
  BC_N = zbb && zbb->cache == 3 ? BC_N_MAX : 177;
  memset(bc_k2s, 0, BC_K * 2);
  memset(bc_s2k, 0, BC_N_MAX * 2);
  memset(bc_pend, 0, BC_B);
  memset(bc_shown, 0, BC_B);
  memset(bc_next, 0, BC_B);
  bc_npend = 0; bc_rr = 0; bc_atomic = 0;
}
static void bc_vblank(uint16 max) {               /* tiles pendientes de la caché (desde la ROM, página de los jefes) */
  uint8 lp = md_cur_page;
  map_page(ZEBES_BOSS_PAGE);
  for (uint16 s = 0; s < BC_N && bc_npend && max; s++)
    if (bc_pend[s >> 3] & (1 << (s & 7))) {
      bc_pend[s >> 3] &= ~(1 << (s & 7));
      bc_npend--;
      uint16 k = bc_s2k[s];
      if (k) { vdp_dma_vram(zbb->tiles + (k - 1) * 16, bc_tile(s) * 32, 16); max--; }
    }
  map_page(lp);
}
/* Crocomire derritiéndose (Crocomire_Func_67 $A4:96C8): en el SNES borra, columna por columna (v = 0..127, en orden
   aleatorio), los píxeles de arriba hasta croco_vline_height[v] en las copias de los tiles del BG2 (7E:4000: 6 filas de
   16 tiles; se suben 11 por fila, los tiles r * 16 + c del BG2) y los sube por la cola de VRAM, una fila por cuadro.
   Aquí, mientras dura (el HDMA $A4:943C activo: md_croco_wave), cada cuadro se arman MELT_N de esos 66 tiles desde la
   ROM con las columnas borradas (con el volteo de su clave de la caché) y el vblank los sube al lugar de la caché.
   RAM: en cinematic_text_tilemap, huecos de esta sala (3128, 3840: 64 bytes cada uno). */
#define MELT_N 4
static uint8 melt_cur, melt_n;
static uint16 melt_dst[MELT_N];
static inline uint32 *melt_buf(uint16 j) {
  static const uint16 kOff[2] = { 3128, 3840 };
  return (uint32 *)((uint8 *)cinematic_text_tilemap + kOff[j >> 1] + 32 * (j & 1));
}
uint8 md_croco_wave(void);
static uint8 melt_phase(void);
static void melt_frame(void) {
  melt_n = 0;
  uint8 mph = melt_phase();
  static uint8 melt_last = 0xFF;
  if (mph != melt_last) {                         /* otra fase: los mismos números de tile son otros tiles (el mapa no
                                                     cambia): todo el cuerpo otra vez */
    melt_last = mph;
    if (zbb && zbb->cache) bb_redo = 1;
  }
  if (!zbb || !zbb->cache || mph == 0xFF || !md_croco_wave()) { melt_cur = 0; return; }
  uint16 n = enemy_bg2_tilemap_size >> 1;
  if (n > BB_MAX) n = BB_MAX;
  uint16 pal = 0xFFFF;                            /* la paleta del cuerpo en el mapa (la de la primera palabra del bloque) */
  for (uint16 i = 0; i < n; i++) {
    uint16 w = tilemap_stuff[i], t = w & 0x3FF;
    if (t < CROCO_MELT_ROWS * 16 && (t & 15) < 11) { pal = (w >> 10) & 7; break; }
  }
  if (pal == 0xFFFF) return;
  uint8 lp = md_cur_page;
  map_page(ZEBES_BOSS_PAGE);
  uint16 sl = (uint16)zbb->slot[pal] << 10;
  for (uint16 tries = 0; tries < CROCO_MELT_ROWS * 11 && melt_n < MELT_N; tries++) {
    uint16 i = melt_cur;
    if (++melt_cur >= CROCO_MELT_ROWS * 11) melt_cur = 0;
    uint16 r = i / 11, c = i - r * 11;
    uint16 f = kCrocoMelt[mph][i];
    if (f == 0xFFFF) continue;
    uint16 key = zbb->tab[sl | f];
    if (!(key & 0x8000)) continue;
    uint16 s = bc_k2s[key & 0x7FF];
    if (!s) continue;                             /* (no está en la VRAM: al subirlo la caché lo trae de la ROM) */
    uint32 *b = melt_buf(melt_n);
    memcpy(b, zbb->tiles + (key & 0x7FF) * 16, 32);
    uint8 hf = (key & 0x0800) != 0, vf = (key & 0x1000) != 0;
    for (uint16 x = 0; x < 8; x++) {
      int16 h = (int16)croco_vline_height[c * 8 + x] - (int16)(r * 8);
      if (h <= 0) continue;
      if (h > 8) h = 8;
      uint16 xs = hf ? 7 - x : x;
      uint32 m = ~(0xF0000000u >> (4 * xs));
      for (uint16 y = 0; y < (uint16)h; y++) b[vf ? 7 - y : y] &= m;
    }
    melt_dst[melt_n++] = bc_tile(s - 1);
  }
  map_page(lp);
}
static void melt_vblank(void) {
  for (uint16 j = 0; j < melt_n; j++) vdp_dma_vram(melt_buf(j), melt_dst[j] * 32, 16);
  melt_n = 0;
}

/* md_cone.c, md_engine.c: la bomba de poder o la caja de mensaje usaron 0xF000 */
void zebes_boss_tiles_again(void) {
  if (!md_zebes || !zbb) return;
  if (zbb->cache) { bc_reset(); bb_redo = 1; }
  else if (zbb->ntiles) bt_pending = 1;
}

static inline uint8 in_room(int16 bx, int16 by) {
  return bx >= 0 && by >= 0 && bx < (int16)room_width_in_blocks && by < (int16)room_height_in_blocks;
}
static inline uint16 block_at(int16 bx, int16 by) {
  return in_room(bx, by) ? level_get(Mul16(by, room_width_in_blocks) + bx) : 0xFFFF;
}
static inline uint16 block2_at(int16 bx, int16 by) {   /* con la página de gráficos puesta */
  if (!in_room(bx, by)) return 0xFFFF;
  uint16 i = Mul16(by, room_width_in_blocks) + bx;
  return zg->l2i ? zg->l2[((uint16)zg->l2i[i >> 4] << 4) + (i & 15)] : zg->l2[i];   /* (filas sin repetir: mkzebes) */
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
/* zprio: prioridad alta en todas las celdas del plano A mientras la luz de la bomba de poder usa el modo sombra/luz
   (md_cone.c, docs/58: si no, el VDP oscurece lo que no tiene prioridad alta). El plano B no hace falta: con el A en
   prioridad alta ningún punto queda en sombra. */
static uint16 zprio;
#define PSWEEP 2                         /* columnas por cuadro al cambiar la prioridad (zebes_force_prio) */
static int16 psweep = ZW, psweep_x0;
static uint16 zpmask = 0xFFFF;                  /* rayos X: 0x7FFF, todas con prioridad baja (el modo sombra/luz las
                                                   oscurece; el cono las aclara: md_cone.c) */
static uint8 psweep_n = PSWEEP;
static inline uint16 tile_fix(uint16 c) { return (((c & 0x7FF) ? c + ZT_BASE : c) | zprio) & zpmask; }
static uint16 zpmask_b = 0xFFFF;                /* rayos X: el plano B también con prioridad baja (casi todas sus
                                                   celdas la tienen alta: no se oscurecía el fondo) */
static int8 bgr_row = -1;                        /* fondo de biblioteca: la fila del plano B que sigue al cambiarla */
static inline uint16 tile_fix_b(uint16 c) { return ((c & 0x7FF) ? c + ZT_BASE : c) & zpmask_b; }

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
    buf[cy - cy0] = L ? tile_fix_b(c[q + (cy & 1)]) : tile_fix(c[q + (cy & 1)]);
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
    buf[cx - cx0] = L ? tile_fix_b(c[((cx & 1) << 1) + q]) : tile_fix(c[((cx & 1) << 1) + q]);
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
  for (uint16 i = 0; i < 32; i++) buf[i] = buf[i + 32] = tile_fix_b(c[i]);
}
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

/* congelados (ver "enemigos congelados", más abajo) */
#define FROZEN_REF 0x8000              /* (mkzobj.FROZEN_REF) */
/* el depósito: 3 lugares de 18 tiles en las filas 5-31 del Window (VRAM 0xE140, 54 tiles); en las salas con los
   objetos por cuadro (zo->stream, mkzobj.STREAM_ROOMS: Kraid, Draygon) hasta FRZ_NMAX lugares de 8 tiles desde
   zo->base, con sus tablas en cinematic_text_tilemap + 3200 (Kraid: sin líquido) o + 1024 (Draygon) */
#define FRZ_TILE0 1802
#define FRZ_SLOT 18
#define FRZ_NSLOT 3
#define FRZ_NMAX 48
static uint16 frz_t0 = FRZ_TILE0, frz_slot = FRZ_SLOT;
static uint8 frz_ns = FRZ_NSLOT, frz_stream;
#define FRZ_KEEP 8                      /* cuadros sin dibujarse antes de soltar el lugar (el parpadeo del final) */
static uint16 frz_id_s[FRZ_NSLOT], frz_last_s[FRZ_NSLOT];
static uint8 frz_q_l_s[FRZ_NSLOT], frz_q_s[FRZ_NSLOT];
static uint16 *frz_id = frz_id_s;       /* congelado del lugar (0xFFFF libre, 0xFFFE parte del anterior) */
static uint16 *frz_last = frz_last_s;   /* último cuadro del motor en que se dibujó */
static uint8 *frz_q_l = frz_q_l_s, frz_nq_l, *frz_q = frz_q_s, frz_nq;   /* lugares por subir (lógica / vblank) */
extern uint16 cinematic_text_tilemap[];
/* Kraid: su cola de VRAM y su pared (más abajo, "Kraid") */
static const ZebesWall *zw;
static uint8 wall_on, wall_pend, kq_r, kq_w, kq_conv;
static uint8 mouth_cur, mouth_pend;     /* cuadro de la boca de Kraid que está en la VRAM / por subir (1 + cuadro; 0: ninguno) */
static void kq_frame(void);
static void kq_vblank(void);
static void frz_setup(void) {             /* al entrar a la sala: el depósito de los congelados o de los objetos */
  if (zo && zo->stream) {
    /* (Kraid: + 3200, su sala no tiene líquido; las demás, como Draygon con su agua: + 1024, libre sin la cola de Kraid
       ni la caché del cuerpo) */
    uint8 *m = (uint8 *)cinematic_text_tilemap + (zw ? 3200 : room_ptr == 0xDD58 ? 2048 : 1024);   /* (Mother Brain: su mapa del BG2 es de 2 KB) */
    frz_stream = 1; frz_t0 = zo->base + ((zo->ntiles + 7) & ~7); frz_slot = 8;   /* (después de los fijos) */
    uint16 n = (1280 - frz_t0) / 8;
    frz_ns = (uint8)(n > FRZ_NMAX ? FRZ_NMAX : n);
    frz_id = (uint16 *)m; frz_last = (uint16 *)(m + 2 * FRZ_NMAX);
    frz_q_l = m + 4 * FRZ_NMAX; frz_q = m + 5 * FRZ_NMAX;
  } else {
    frz_stream = 0; frz_t0 = FRZ_TILE0; frz_slot = FRZ_SLOT; frz_ns = FRZ_NSLOT;
    frz_id = frz_id_s; frz_last = frz_last_s; frz_q_l = frz_q_l_s; frz_q = frz_q_s;
  }
}
static void frz_reset(void);
static void frz_vblank(void);

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
  { void cp_room_(void); cp_room_(); }
  zbb = 0; bb_n = 0; bb_ever = 0; bt_pending = 0; bb_redo = 0; bb_rows = 0; bb_hide = 0;
  for (uint16 i = 0; i < NT_ZEBES_NBOSSBG2; i++)
    if (kZebesBossBg2[i].gfx == kZebesStates[f].gfx) zbb = &kZebesBossBg2[i];
  for (uint16 i = 0; i < 64; i++) zcache[i].zo = 0;   /* (md_ovl: la intro pudo dejar cualquier cosa, docs/60) */
  zw = 0;
  for (uint16 i = 0; i < NT_ZEBES_NWALL; i++)
    if (kZebesWall[i].gfx == kZebesStates[f].gfx) zw = &kZebesWall[i];
  frz_setup();                                   /* (después de zw: dónde van sus tablas) */
  frz_reset();
  if (game_state != 17) {                        /* (17: al volver de la pausa, ResumeGameplay: Kraid sigue igual) */
    wall_on = 0; kq_r = kq_w = 0; mouth_cur = 0;
  }
  wall_pend = 0; kq_conv = 0; mouth_pend = 0;
  esc_on = 0; esc_n = 0; esc_font_pend = 0;
  frz_nq = 0;
  zprio = 0; zpmask = 0xFFFF; psweep = ZW; zpmask_b = 0xFFFF; bgr_row = -1;
  const MdRoom *base = &kMdRooms[ZEBES_BASE_ROOM];
  zroom = *base;
  zroom.room = room_ptr;
  zroom.state = roomdefroomstate_ptr;
  zroom.plane_w = 64; zroom.plane_h = 64;
  zroom.planeA = 0; zroom.planeB = 0;
  /* fondo de biblioteca (64x32 celdas en el plano B, como el BG2 del SNES): b_mode 1, el plano B se mueve con los
     registros del SNES (BG2HOFS = bg2_x_scroll + layer2_x_pos, door.c); p. ej. 9804, la sala del Bomb Torizo, con
     bg2_x_scroll = 256 */
  zroom.b_mode = !zg->l2 && !zg->sky && (zg->bg2 || zg->bg2z) ? 1 : 0;
  zroom.patch = 0; zroom.npatch = 0; zroom.wall = 0; zroom.door_sm = 0; zroom.flash_line = 0;
  memcpy(zsrc, base->src, 64);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  for (uint16 i = 0; i < 32; i++)
    zsrc[i] = i < (uint16)zg->npal * 16 ? (uint8)zg->src[i] : 0xFF;
  meta_build();
  if (zo && zg->npal == 1) {                 /* jefe de colores propios (mkzobj.BOSS_LINE): la línea 1 es de los objetos */
    map_page(zo->page);
    for (uint16 i = 1; i < 16; i++) {
      uint16 s = zo->src[14 + i];
      zsrc[16 + i] = s == 0xFFFF ? 0xFF : (uint8)s;
    }
  }
  map_page(lp);
  zsrc[0] = 0;                               /* color de fondo: palette_buffer[0] */
  { extern uint32 md_fine_mask; md_fine_mask = zg->skym; }   /* el cielo de la superficie: kCramFine (md_engine.c) */
  zroom.src = zsrc;
  /* lista del fundido (NT_FADE_LIST, AdvancePaletteFadeForAllPalettes): la de la sala base (Samus, HUD, rayo) más
     los colores de esta sala y de sus objetos (el despertar del Bomb Torizo es un fundido de sus paletas) */
  {
    static uint8 zfade[256];
    uint8 seen[32];
    uint16 n = 0;
    memset(seen, 0, sizeof seen);
    #define ZFADE_ADD(c) do { uint8 c_ = (c); if (!(seen[c_ >> 3] & (1 << (c_ & 7)))) { seen[c_ >> 3] |= 1 << (c_ & 7); zfade[n++] = c_; } } while (0)
    ZFADE_ADD(0);                                  /* el fondo (se ve siempre: el tiempo agotado lo pone blanco) */
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
  for (uint16 k = 0; k < 16; k++) xr_shown[k] = 0;   /* (rayos X) */
  xr_any = 0;
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

/* meta_idx desde la tabla de la sala (también después de cargar los tiles: md_scratch es el búfer de la
   descompresión) */
static void meta_build(void) {
  uint8 lp = md_cur_page;
  map_page(zg->page);
  memset(meta_idx, 0, META_IDX_BYTES);
  for (uint16 i = 0; i < zg->nmeta; i++) meta_idx[zg->ids[i] & 0x3FF] = i + 1;
  map_page(lp);
}

/* descompresor del formato MZ (tools/conv/mz.py, mzpack.c): n bytes de src a dst. Bits de mayor a menor mezclados
   con los bytes; empieza con literales; después de literales: 0 repetir el último desplazamiento, 1 nuevo; después de
   una copia: 0 literales, 1 nuevo. Nuevo: h = gamma, byte b: desplazamiento (h - 1) * 128 + (b >> 1) + 1, largo
   gamma + 1 con su primer bit en b & 1 */
/* (en ensamblador: en C tardaba ~110 ciclos por byte y la sala del aterrizaje sumaba 26 cuadros a la carga).
   a0 entrada, a1 salida, a2 fin, d7 bits (el byte a la izquierda y un 1 de centinela detrás: add.b saca el bit al
   acarreo y a X; si queda 0, el centinela salió y se toma el byte siguiente), d1 valor, d2 último desplazamiento */
void mz_unpack(const uint8 *ip, uint8 *op, uint32 n);
__asm__(
"    .text\n"
"    .even\n"
"    .globl mz_unpack\n"
"    .macro MZBIT\n"
"    add.b %d7,%d7\n"
"    bne.s 1f\n"
"    move.b (%a0)+,%d7\n"
"    addx.b %d7,%d7\n"
"1:\n"
"    .endm\n"
"    .macro MZGAMMA\n"
"    moveq #1,%d1\n"
"2:  MZBIT\n"
"    bcs.s 3f\n"
"    MZBIT\n"
"    addx.w %d1,%d1\n"
"    bra.s 2b\n"
"3:\n"
"    .endm\n"
"mz_unpack:\n"
"    movem.l %d2-%d4/%d7/%a2-%a3,-(%sp)\n"
"    move.l 28(%sp),%a0\n"
"    move.l 32(%sp),%a1\n"
"    move.l 36(%sp),%d0\n"
"    lea (%a1,%d0.l),%a2\n"
"    moveq #1,%d2\n"
"    move.b #0x80,%d7\n"
".Lmz_lit:\n"                                   /* literales */
"    MZGAMMA\n"
"    subq.w #1,%d1\n"
"4:  move.b (%a0)+,(%a1)+\n"
"    dbra %d1,4b\n"
"    cmpa.l %a2,%a1\n"
"    bcc.w .Lmz_done\n"
"    MZBIT\n"
"    bcs.s .Lmz_new\n"
"    MZGAMMA\n"                                 /* repetir el último desplazamiento */
"    subq.w #1,%d1\n"
"    move.l %a1,%a3\n"
"    sub.l %d2,%a3\n"
"5:  move.b (%a3)+,(%a1)+\n"
"    dbra %d1,5b\n"
"    cmpa.l %a2,%a1\n"
"    bcc.w .Lmz_done\n"
"    MZBIT\n"
"    bcc.s .Lmz_lit\n"
".Lmz_new:\n"                                   /* desplazamiento nuevo */
"    MZGAMMA\n"
"    subq.w #1,%d1\n"
"    lsl.w #7,%d1\n"
"    moveq #0,%d3\n"
"    move.b (%a0)+,%d3\n"
"    lsr.b #1,%d3\n"                            /* (el bit 0: primer bit del largo) */
"    scs %d4\n"
"    add.w %d3,%d1\n"
"    addq.w #1,%d1\n"
"    moveq #0,%d2\n"
"    move.w %d1,%d2\n"
"    moveq #1,%d1\n"
"    tst.b %d4\n"
"    bne.s 7f\n"
"    MZBIT\n"
"    addx.w %d1,%d1\n"
"6:  MZBIT\n"
"    bcs.s 7f\n"
"    MZBIT\n"
"    addx.w %d1,%d1\n"
"    bra.s 6b\n"
"7:  move.l %a1,%a3\n"                          /* largo: d1 + 1 */
"    sub.l %d2,%a3\n"
"8:  move.b (%a3)+,(%a1)+\n"
"    dbra %d1,8b\n"
"    cmpa.l %a2,%a1\n"
"    bcc.s .Lmz_done\n"
"    MZBIT\n"
"    bcs.w .Lmz_new\n"
"    bra.w .Lmz_lit\n"
".Lmz_done:\n"
"    movem.l (%sp)+,%d2-%d4/%d7/%a2-%a3\n"
"    rts\n"
);

/* tiles de la sala o de sus objetos (mkzebes.py: ops, en la página rp) desde el tile vt0 de la VRAM: cada trozo
   comprimido se descomprime una vez en md_scratch (2 KB; meta_idx se rehace después: meta_build) y sus tiles suben
   por DMA; los sin comprimir, del depósito */
static void zebes_load_ops(uint8 rp, const uint16 *o, uint16 nw, uint16 vt0) {
  uint8 *buf = (uint8 *)md_scratch;
  while (nw) {
    map_page(rp);
    uint16 sg = o[0], n = o[1];
    o += 2; nw -= 2;
    if (sg == 0x8000) {
      for (; n; n--, o += 3, nw -= 3) {
        map_page(rp);
        uint16 r[2] = { o[2], o[1] };
        zebes_dma_runs(rp, r, 1, vt0 + o[0]);      /* (r está en la RAM: zebes_dma_runs pone rp y lo lee igual) */
      }
    } else {
      map_page(ZEBES_FRZ_PAGE);
      const ZebesSeg *zs = &kZebesSeg[sg];
      const uint8 *d = zs->data;
      uint8 pg = zs->page, nt = zs->ntiles;
      map_page(pg);
      /* (esto corre en el vblank, con el XGM2 avisado de que hay DMA: no lee la ROM y la música se para. Mientras
         se descomprime el 68000 solo lee: el Z80 puede seguir) */
      if (md_snd_frames) md_snd_protect(0);
      mz_unpack(d, buf, (uint32)nt * 32);
      if (md_snd_frames) md_snd_protect(1);
      map_page(rp);
      for (; n; n--, o += 2, nw -= 2) {
        uint16 w = o[1];
        vdp_dma_vram(buf + (w >> 8) * 32, (vt0 + o[0]) * 32, (w & 0xFF) * 16);
      }
    }
  }
}

/* (pantalla apagada) tiles de la sala, los de los objetos de la sala base y los planos */
void zebes_load_vram(uint16 ax, uint16 ay, uint16 bx, uint16 by) {
  const MdRoom *base = &kMdRooms[ZEBES_BASE_ROOM];
  frz_reset();                                              /* (la VRAM se rehace: el depósito, de cero; y la pausa pisa
                                                               sus tablas en cinematic_text_tilemap) */
  vdp_reg(0x10, 0x11);                                      /* 64x64 */
  if (base->tiles_total > base->bg_tiles)                   /* rayo, bombas...: donde los espera la sala base */
    vdp_dma_vram(base->tiles + base->bg_tiles * 32, (BG_BASE + base->bg_tiles) * 32,
                 (base->tiles_total - base->bg_tiles) * 16);
  uint8 lp = md_cur_page;
  zebes_load_ops(zg->page, zg->ops, zg->nops, ZT_BASE);
  if (zo) zebes_load_ops(zo->page, zo->ops, zo->nops, zo->base);
  meta_build();                                             /* (md_scratch fue el búfer de la descompresión) */
  map_page(lp);
  if (zbb && zbb->cache) bc_reset();
  else if (zbb && zbb->ntiles) bt_dma();
  if (zw) { wall_pend = wall_on ? 1 : 2; kq_conv = 0; mouth_pend = mouth_cur; }   /* la pared de Kraid como está (vacía al entrar) */
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
  } else if (zg->bg2z) {
    /* (comprimido: trozos de 16 filas en md_scratch, que después vuelve a ser meta_idx) */
    const uint8 *d = zg->bg2z;
    const uint16 *c = (const uint16 *)md_scratch;
    for (uint16 q = 0; q < (zg->bg2h ? 4 : 2); q++) {
      map_page(zg->page);
      uint16 len = (uint16)(d[0] << 8 | d[1]);
      if (md_snd_frames) md_snd_protect(0);
      mz_unpack(d + 2, (uint8 *)md_scratch, 2048);
      if (md_snd_frames) md_snd_protect(1);
      d += 2 + len;
      for (uint16 h = 0; h < (zg->bg2h ? 1 : 2); h++) {   /* (Kraid: 64x64; las demás, 64x32 dos veces) */
        vdp_set_vram_write(MD_VRAM_PLANE_B + (h * 32 + q * 16) * 128);
        for (uint16 i = 0; i < 64 * 16; i++) VDP_DATA = tile_fix_b(c[i]);
      }
    }
    meta_build();
    map_page(lp);
    if (zbb && bb_ever) {                     /* (vuelta de la pausa) el cuerpo del jefe, desde tilemap_stuff */
      uint16 n = enemy_bg2_tilemap_size >> 1;
      if (n > BB_MAX) n = BB_MAX;
      bb_convert(n, 0xFFFFFFFF);
      if (zbb->cache) bc_vblank(BC_N);
      bb_n = n & ~31; bb_rows = 0xFFFFFFFF;
      bb_vblank();
    }
  } else if (zg->bg2) {
    map_page(zg->page);
    for (uint16 h = 0; h < 2; h++) {
      vdp_set_vram_write(MD_VRAM_PLANE_B + h * 32 * 128);
      const uint16 *c = zg->bg2 + (zg->bg2h ? h * 64 * 32 : 0);   /* (Kraid: 64x64; las demás, 64x32 dos veces) */
      for (uint16 i = 0; i < 64 * 32; i++) VDP_DATA = tile_fix_b(*c++);
    }
    map_page(lp);
    if (zbb && bb_ever) {                     /* (vuelta de la pausa) el cuerpo del jefe, desde tilemap_stuff */
      uint16 n = enemy_bg2_tilemap_size >> 1;
      if (n > BB_MAX) n = BB_MAX;
      bb_convert(n, 0xFFFFFFFF);
      if (zbb->cache) bc_vblank(BC_N);
      bb_n = n & ~31; bb_rows = 0xFFFFFFFF;
      bb_vblank();
    }
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

static void prio_sweep(void) {
  if (full_redraw & 1) { psweep = ZW; return; }   /* (se dibuja toda la ventana igual) */
  for (uint16 k = 0; k < psweep_n && psweep < ZW && ncol < QCOL; ) {
    int16 cx = psweep_x0 + psweep++;
    if (cx >= wx[0] && cx < wx[0] + ZW) { queue_col(0, cx, wy[0]); k++; }   /* (las demás ya salieron o entran nuevas) */
  }
}
void zebes_frame(uint16 ax, uint16 ay, uint16 bx, uint16 by) {
  melt_frame();
  if (zw) kq_frame();
  /* el BG2 fuera de las dos pantallas del SNES (TM/TS: Phantoon invisible, LayerBlendFunc_4): el cuerpo se borra del
     plano B; al volver, se sube otra vez */
  if (zbb && bb_ever) {
    uint8 h = !((reg_TM | reg_TS) & 2);
    if (h != bb_hide) {
      bb_hide = h;
      if (h) { memset(bb_buf, 0, (bb_sum & ~31) * 2); bb_n = bb_sum & ~31; bb_rows = 0xFFFFFFFF; }
      else bb_redo = 1;
    }
  }
  if (bb_redo && zbb && bb_ever) { nt_enemy_bg2_transfer(); bb_redo = 0; }   /* la caché se vació: el cuerpo otra vez */
  if (drawn[0]) { follow(0, ax, ay); if (psweep < ZW) prio_sweep(); }
  if (drawn[1]) follow(1, bx, zg->sky ? ay : by);   /* el cielo baja con la capa 1 (BG2VOFS = layer1_y_pos) */
}

/* un PLM cambió el bloque i: se anota (zebes_take lo mira con la ventana dibujada) */
void nt_block_changed(uint16 i) {
  if (!md_zebes) return;
  if (nlblk >= QBLK) { lfull = 1; return; }
  lblk[nlblk++] = i;
}
/* si está en la ventana dibujada, sus 4 celdas otra vez: las de la palabra w (0xFFFF: la del nivel). 0 si la cola
   está llena (los rayos X esperan al cuadro siguiente; un PLM pide redibujar todo) */
static uint8 block_word(uint16 i, uint16 w) {
  if (!drawn[0] || !room_width_in_blocks) return 1;
  int16 by = (int16)(i / room_width_in_blocks), bx = (int16)(i - (uint16)by * room_width_in_blocks);
  int16 cx = bx << 1, cy = by << 1;
  if (cx + 1 < wx[0] || cx >= wx[0] + ZW || cy + 1 < wy[0] || cy >= wy[0] + ZH) return 1;
  if (nblk >= QBLK) return 0;
  if (w == 0xFFFF) w = block_at(bx, by);
  uint8 lp = md_cur_page;
  map_page(zg->page);
  const uint16 *c = cells_of(w);
  for (uint16 k = 0; k < 4; k++) qblk_c[nblk][k] = tile_fix(c[k]);
  map_page(lp);
  qblk_addr[nblk] = MD_VRAM_PLANE_A + (uint16)(((cy & 63) * 64 + (cx & 63)) * 2);
  nblk++;
  return 1;
}
static void block_changed(uint16 i) {
  if (!block_word(i, 0xFFFF)) full_redraw = 1;
}

/* ---------------------------------------------------------------- rayos X (hdmaobj.c, docs/62) */
/* Los bloques revelados (xray_rev: lista que arma hdmaobj.c al empezar, como Xray_SetupStage4) que caen dentro del
   cono se dibujan con su palabra; al salir del cono o apagarse los rayos X, con la del nivel. El cono: la tabla del
   SNES en 7E:9100 (md_cone.c la arma; por línea, izquierda en el byte bajo y derecha en el alto), en el centro del
   bloque. */
static uint8 xr_in_cone(uint16 i) {
  int16 by = (int16)(i / room_width_in_blocks), bx = (int16)(i - (uint16)by * room_width_in_blocks);
  int16 sx = (bx << 4) + 8 - (int16)layer1_x_pos, sy = (by << 4) + 8 - (int16)layer1_y_pos;
  if (sx < 0 || sx > 255 || sy < 32 || sy > 223) return 0;
  uint16 e = ERAM_ARR(0x9100)[sy - 2];
  return (uint8)e <= sx && sx <= (e >> 8);
}
static void xray_take(void) {
  if (!xray_on && !xr_any) return;
  uint8 any = 0;
  uint16 n = xray_rev_n < 128 ? xray_rev_n : 128;
  for (uint16 k = 0; k < n; k++) {
    uint8 m = 1 << (k & 7), have = (xr_shown[k >> 3] & m) != 0;
    uint8 want = xray_on && xray_cone_ok && xr_in_cone(hdma_table_1[k]);
    if (want != have) {
      if (!block_word(hdma_table_1[k], want ? hdma_table_1[128 + k] : 0xFFFF)) { any = 1; break; }
      xr_shown[k >> 3] ^= m;
      have = want;
    }
    any |= have;
  }
  xr_any = any;
}

/* ---------------------------------------------------------------- vblank */
/* tiles animados ($87): el motor dice qué trozo va a qué VRAM del SNES; aquí se suben los tiles de la sala que
   salen de ese trozo (ya convertidos en el PC: mkzebes.py) */
/* la cola de escrituras a la VRAM del SNES (game.c, NMI): tiles de los jefes que suben por la cola (Crocomire,
   Phantoon, Mother Brain); falta convertirlos para la Genesis (docs/46) */
/* ---------------------------------------------------------------- Kraid (docs/59): lo que sube por la cola de VRAM */
/* La cola de VRAM del SNES (game.c, NMI): en la sala de Kraid (zw: su pared) se interpretan sus entradas:
   - al mapa del BG2 (las dos mitades desde 7E:2000/2800, la boca y el ojo desde $A7:97C8-A0C8, las filas del
     hundimiento y el borrado del final): tareas por fila (origen, celda del plano B, palabras) que zebes_frame
     convierte (como bb_convert, sin caché) de a KQ_ROWS y el vblank sube;
   - a los tiles 1008-1023 del BG1 (la pared de la cueva): kKraidsRoomBg ($A7:A716) la enciende, 7E:4000 (ceros) la
     apaga.
   RAM: cinematic_text_tilemap + 1024 (tareas), + 1792 (filas convertidas), + 2816 (destinos). */
typedef struct { uint32 src; uint16 cell, n; } KqJob;   /* (cell 0xFFFF: cuadro src de la boca) */
static const uint16 kKraidMouthSrc[4] = { 0x97C8, 0x9AC8, 0x9DC8, 0xA0C8 };   /* $A7: Kraid_Shot_* */
#define KQ_N 96
#define KQ_ROWS 16
#define kq_job ((KqJob *)((uint8 *)cinematic_text_tilemap + 1024))
#define kq_buf ((uint16 *)((uint8 *)cinematic_text_tilemap + 1792))
#define kq_dst ((uint16 *)((uint8 *)cinematic_text_tilemap + 2816))
/* zw: la pared de la sala (kZebesWall) o 0; wall_pend: 1 subir sus tiles, 2 borrarlos (declarados arriba) */
static void kq_push(uint32 src, uint16 cell, uint16 n) {
  uint8 w = (uint8)((kq_w + 1) % KQ_N);
  if (w == kq_r) return;                          /* (llena: se pierde; no debería pasar) */
  kq_job[kq_w] = (KqJob){ src, cell, n };
  kq_w = w;
}
/* el texto de la huida en la sala de Mother Brain (TIME BOMB SET! ESCAPE IMMEDIATELY): la máquina de escribir
   (HandleTypewriterText_Ext, $A6:C2A7) pone en la cola de VRAM una palabra por letra (7E:8034, 0x2610 + letra - 'A')
   hacia el tilemap del BG2 ($4800), que el cuerpo ya no usa. Aquí va al plano B (no tapa la sala, como en el SNES),
   con las letras convertidas (kZebesEscFont, mkzebes.py) en los tiles libres de esa sala desde ESC_TILE y los colores
   1 y 3 de la paleta 1 del SNES en la línea 1 (en la pelea la sala va en una línea y la 1 era del cerebro: md_engine.c,
   zebes_esc_colors) */
#define ESC_TILE 2008
#define ESC_QN 8
NT_GAMEHI static uint16 esc_q[ESC_QN][2];       /* (fila << 5 | columna del BG2, tile del SNES) para el vblank */
uint8 zebes_esc_text_on(void) { return esc_on; }
uint16 nt_ram_w(uint16 a);                        /* nt_ram.c: una palabra de la RAM del SNES */
static void esc_scan(const uint8 *q, uint16 n) {
  uint16 base = (uint16)((reg_BG2SC & 0xFC) << 8);
  for (uint16 i = 0; i + 7 <= n; i += 7) {
    uint16 size = q[i] | q[i + 1] << 8, src = q[i + 2] | q[i + 3] << 8, dst = q[i + 5] | q[i + 6] << 8;
    uint16 off = (uint16)(dst - base), t = nt_ram_w(0x8034) & 0x3FF;
    if (size != 2 || q[i + 4] != 0x7E || src != 0x8034 || off >= 0x800) continue;
    if (t < 0x210 || t >= 0x210 + ZEBES_ESC_FONT_N || esc_n >= ESC_QN) continue;
    if (!esc_on) { esc_on = 1; esc_font_pend = 1; }
    esc_q[esc_n][0] = off; esc_q[esc_n][1] = t; esc_n++;
  }
}
static void esc_vblank(void) {
  if (esc_font_pend) { esc_font_pend = 0; vdp_dma_vram(kZebesEscFont, ESC_TILE * 32, ZEBES_ESC_FONT_N * 16); }
  for (uint16 i = 0; i < esc_n; i++) {
    uint16 off = esc_q[i][0], row = (off >> 5) & 63, col = off & 31;
    uint16 w = 0x8000 | (1 << 13) | (ESC_TILE + esc_q[i][1] - 0x210);
    vdp_set_vram_write(MD_VRAM_PLANE_B + row * 128 + col * 2); VDP_DATA = w;
    vdp_set_vram_write(MD_VRAM_PLANE_B + row * 128 + (col + 32) * 2); VDP_DATA = w;
  }
  esc_n = 0;
}

void nt_vram_queue_flush(const uint8 *q, uint16 n) {
  if (md_zebes && room_ptr == 0xDD58) esc_scan(q, n);
  if (!md_zebes || !zw) return;
  for (uint16 i = 0; i + 7 <= n; i += 7) {
    uint16 size = q[i] | q[i + 1] << 8, src = q[i + 2] | q[i + 3] << 8, dst = q[i + 5] | q[i + 6] << 8;
    uint8 bank = q[i + 4];
    if (dst == ((reg_BG12NBA & 0xF) << 8) + 0x3F00 && size == 512) {   /* la pared */
      wall_on = bank != 0x7E;
      wall_pend = wall_on ? 1 : 2;
      continue;
    }
    if (bank == 0xA7 && zw->mouth)                /* un cuadro de la boca: sus tiles propios con sus filas */
      for (uint8 f = 0; f < 4; f++)
        if (src == kKraidMouthSrc[f]) kq_push(f, 0xFFFF, 0);
    uint16 base = (uint16)((reg_BG2SC & 0xFC) << 8), sz = reg_BG2SC & 3;
    uint16 off = (uint16)(dst - base);
    if (off >= 0x1000) continue;
    uint32 a = (uint32)bank << 16 | src;
    for (uint16 left = size >> 1; left; ) {     /* por filas de 32 palabras */
      uint16 scr = off >> 10, y = (off >> 5) & 31, x = off & 31;
      uint16 sx = scr & 1, sy = sz == 3 ? scr >> 1 : sz == 2 ? scr : 0;
      if (sz == 2) sx = 0;
      uint16 k = 32 - x;
      if (k > left) k = left;
      /* (BG2 de 32x32, la sala vencida: el SNES lo repite en toda la pantalla; la fila va a las cuatro pantallas del
         plano B de 64x64: bit 15) */
      kq_push(a, (uint16)((sy * 32 + y) * 64 + sx * 32 + x) | (sz == 0 ? 0x8000 : 0), k);
      a += 2 * k; off += k; left -= k;
    }
  }
}
uint16 nt_bank_w(uint8 bank, uint16 a);           /* enemy_main.c */
static inline uint16 kq_word(uint32 a) {          /* palabra del origen: la RAM del SNES (7E:2000-2FFF) o la ROM */
  uint8 bank = a >> 16;
  uint16 ad = (uint16)a;
  if (bank == 0x7E) return (uint16)(ad - 0x2000) < 0x1000 ? tilemap_stuff[(uint16)(ad - 0x2000) >> 1] : 0;
  return nt_bank_w(bank, ad);
}
static void kq_frame(void) {                      /* (zebes_frame) convierte hasta KQ_ROWS tareas */
  if (kq_conv || kq_r == kq_w) return;
  uint8 lp = md_cur_page;
  uint16 c = 0, wgt = 0;                          /* (wgt: filas a subir; una de 32x32 va a 4 lugares) */
  while (kq_r != kq_w && wgt < KQ_ROWS) {
    KqJob j = kq_job[kq_r];
    if (j.cell == 0xFFFF) {                       /* la boca: empieza una tanda (sus filas, 11, entran en ella) */
      if (c) break;
      mouth_pend = (uint8)j.src + 1;
      kq_r = (uint8)((kq_r + 1) % KQ_N);
      continue;
    }
    kq_r = (uint8)((kq_r + 1) % KQ_N);
    wgt += (j.cell & 0x8000) ? 4 : 1;
    uint16 *o = kq_buf + c * 32;
    uint16 mo = (uint16)j.src - kKraidMouthSrc[0];
    if ((uint8)(j.src >> 16) == 0xA7 && zw->mouth_cells && mo < 4 * 0x300 && mo % 0x300 < 704) {
      /* un cuadro de la boca: sus celdas ya convertidas (mkzebes, ZebesWall.mouth_cells) */
      map_page(ZEBES_BOSS_PAGE);
      const uint16 *src = zw->mouth_cells + (mo / 0x300) * 352 + (mo % 0x300) / 2;
      for (uint16 i = 0; i < j.n; i++) o[i] = src[i];
      kq_dst[2 * c] = j.cell; kq_dst[2 * c + 1] = j.n;
      c++;
      continue;
    }
    for (uint16 i = 0; i < j.n; i++) {
      uint16 w = kq_word(j.src + 2 * i), cc = 0;  /* (kq_word puede cambiar de página: la ROM del origen) */
      if (zbb) {
        if (md_hw_page != ZEBES_BOSS_PAGE) map_page(ZEBES_BOSS_PAGE);
        cc = zbb->tab[(uint16)zbb->slot[(w >> 10) & 7] << 10 | (w & 0x3FF)];
        if (cc & 0x7FF) cc += ZT_BASE;
        cc ^= (w >> 3) & 0x1800;                  /* flips: SNES 15, 14 -> Genesis 12, 11 */
        cc |= (w & 0x2000) << 2;                  /* prioridad */
      }
      o[i] = cc;
    }
    kq_dst[2 * c] = j.cell; kq_dst[2 * c + 1] = j.n;
    c++;
  }
  map_page(lp);
  kq_conv = (uint8)c;
}
static void kq_vblank(void) {
  for (uint16 c = 0; c < kq_conv; c++) {
    uint16 cell = kq_dst[2 * c];
    if (cell & 0x8000)
      for (uint16 q = 0; q < 4; q++)
        vdp_dma_vram(kq_buf + c * 32, MD_VRAM_PLANE_B + ((cell & 0x7FFF) + (q & 1) * 32 + (q >> 1) * 2048) * 2,
                     kq_dst[2 * c + 1]);
    else vdp_dma_vram(kq_buf + c * 32, MD_VRAM_PLANE_B + cell * 2, kq_dst[2 * c + 1]);
  }
  kq_conv = 0;
  if (mouth_pend && zw) {
    uint8 f = mouth_pend - 1, lp = md_cur_page;
    if (zw->mouth_cnt[f]) {
      map_page(ZEBES_BOSS_PAGE);
      vdp_dma_vram(zw->mouth + 32 * zw->mouth_off[f], zw->mouth_t * 32, zw->mouth_cnt[f] * 16);
      map_page(lp);
    }
    mouth_cur = mouth_pend;
    mouth_pend = 0;
  }
  if (wall_pend && zw) {
    uint8 lp = md_cur_page;
    map_page(ZEBES_BOSS_PAGE);
    for (uint16 i = 0; i < zw->n; i++) {
      if (wall_pend == 1) vdp_dma_vram(zw->tiles + 32 * i, zw->idx[i] * 32, 16);
      else vdp_fill_vram(zw->idx[i] * 32, 0, 16);
    }
    map_page(lp);
    wall_pend = 0;
  }
}
/* ---------------------------------------------------------------- cuerpos de los jefes en el BG2 (docs/59)
   El SNES sube tilemap_stuff (enemy_bg2_tilemap_size bytes) al BG2 en $4800 ($A0:9726): la pantalla izquierda del
   fondo, filas de 32 celdas. Aquí cada palabra pasa a una celda de la Genesis por la tabla de la sala (mkzebes.py,
   kZebesBossBg2: tiles ya convertidos con la sala) en el bucle principal, a un buffer de RAM que el vblank sube al
   plano B (las filas y las mismas 32 filas más abajo: el fondo de 256 píxeles da la vuelta como en el SNES).
   Buffer: los primeros 1 KB de cinematic_text_tilemap (en estas salas solo los usan la pausa y los mensajes; al
   volver de la pausa el plano B se rehace desde tilemap_stuff, zebes_load_vram). */

/* fase del derretido de Crocomire (0xFF: no): Crocomire_Func_60 dejó en croco_word_7E069A la lista de la cola de VRAM
   de esa fase ($A4:9BC5 + 0x22 o + 0x7A; 0x54 entre las dos, con los tiles de la primera todavía en la VRAM) */
static uint8 melt_phase(void) {
  if (room_ptr != 0xA98D || !croco_word_7E069A) return 0xFF;
  return croco_word_7E069A >= 0x7A ? 1 : 0;
}
static inline uint16 bb_tile(uint16 w, uint8 mph) {   /* el tile de la palabra (los del derretido: kCrocoMelt) */
  uint16 t = w & 0x3FF;
  if (mph != 0xFF && t < CROCO_MELT_ROWS * 16 && (t & 15) < 11) {
    uint16 f = kCrocoMelt[mph][(t >> 4) * 11 + (t & 15)];
    if (f != 0xFFFF) t = f;
  }
  return t;
}
static void bb_convert(uint16 n, uint32 rows) {   /* rows: filas (de 32) que cambiaron */
  uint8 mph = melt_phase();
  uint8 lp = md_cur_page;
  map_page(ZEBES_BOSS_PAGE);
  const uint16 *tab = zbb->tab;
  const uint8 *slot = zbb->slot;
  if (zbb->cache) {                               /* primero: los lugares que ya tienen tiles de esta subida */
    memset(bc_used, 0, BC_B);
    for (uint16 i = 0; i < n; i++) {
      uint16 w = tilemap_stuff[i];
      uint16 c = tab[(uint16)slot[(w >> 10) & 7] << 10 | bb_tile(w, mph)];
      if (c & 0x8000) {
        uint16 s = bc_k2s[c & 0x7FF];
        if (s) { s--; bc_used[s >> 3] |= 1 << (s & 7); }
      }
    }
  }
  for (uint16 i = 0; i < n; i++) {
    if (!(rows & (1UL << (i >> 5)))) { i |= 31; continue; }   /* fila sin cambios: ya convertida */
    uint16 w = tilemap_stuff[i];
    uint16 c = tab[(uint16)slot[(w >> 10) & 7] << 10 | bb_tile(w, mph)];
    if (c & 0x8000) {                             /* tile de la caché */
      uint16 k = c & 0x7FF, s = bc_k2s[k];
      if (!s) {                                   /* lugar libre o de un tile que esta subida no usa */
        uint16 t = BC_N;                          /* primero: uno que tampoco use el mapa que se ve */
        while (t-- && ((bc_used[bc_rr >> 3] | bc_shown[bc_rr >> 3]) & (1 << (bc_rr & 7))))
          if (++bc_rr >= BC_N) bc_rr = 0;
        if (t == 0xFFFF) {                        /* no hay: uno que esta subida no use (y todo en un vblank) */
          bc_atomic = 1;
          t = BC_N;
          while (t-- && (bc_used[bc_rr >> 3] & (1 << (bc_rr & 7))))
            if (++bc_rr >= BC_N) bc_rr = 0;
        }
        if (t == 0xFFFF) { bb_buf[i] = 0; continue; }   /* (no debería pasar: ~155 a la vez) */
        uint16 r = bc_rr;
        if (++bc_rr >= BC_N) bc_rr = 0;
        if (bc_s2k[r]) bc_k2s[bc_s2k[r] - 1] = 0;
        bc_s2k[r] = k + 1;
        bc_k2s[k] = r + 1;
        bc_used[r >> 3] |= 1 << (r & 7);
        if (!(bc_pend[r >> 3] & (1 << (r & 7)))) { bc_pend[r >> 3] |= 1 << (r & 7); bc_npend++; }
        s = r + 1;
      }
      c = (c & 0x7800) | bc_tile(s - 1);
    } else if (c & 0x7FF) c += ZT_BASE;
    c ^= (w >> 3) & 0x1800;                       /* flips: SNES 15 (vertical), 14 (horizontal) -> Genesis 12, 11 */
    c |= (w & 0x2000) << 2;                       /* prioridad: SNES 13 -> Genesis 15 */
    bb_buf[i] = c;
  }
  if (zbb->cache) {
    memcpy(bc_next, bc_used, BC_B);
    /* los pendientes que este mapa ya no usa (de un mapa que no llegó a verse): no se suben y el lugar queda libre
       (si no, con el mapa cambiando en cada cuadro la caché se llenaba de tiles que nunca se veían) */
    for (uint16 j = 0; j < BC_B; j++) {
      uint8 x = bc_pend[j] & ~bc_used[j];
      if (!x) continue;
      for (uint16 q = 0; q < 8; q++)
        if (x & (1 << q)) {
          uint16 r = j * 8 + q;
          if (bc_s2k[r]) { bc_k2s[bc_s2k[r] - 1] = 0; bc_s2k[r] = 0; }
          bc_npend--;
        }
      bc_pend[j] &= ~x;
    }
  }
  map_page(lp);
}

void nt_enemy_bg2_transfer(void) {
  if (!md_zebes || !zbb || bb_hide) return;
  uint16 n = enemy_bg2_tilemap_size >> 1;
  if (n > BB_MAX) n = BB_MAX;
  /* suma por fila (32 palabras): solo se convierten y se suben las filas que cambiaron (la boca, los brazos) */
  uint32 rows = 0;
  {
    const uint32 *t = (const uint32 *)tilemap_stuff;
    uint8 all = !bb_ever || bb_redo || bb_sum != n;
    for (uint16 r = 0; r < (n >> 5); r++) {
      /* (Fletcher sobre palabras largas: también ve los cambios de lugar; la mitad que rotar y sumar cada palabra) */
      uint32 s1 = r, s2 = 0;
      for (uint16 i = 0; i < 16; i++) { s1 += *t++; s2 += s1; }
      uint32 sum = s2 ^ (s1 << 16 | s1 >> 16);
      if (all || sum != bb_rsum[r]) { rows |= 1UL << r; bb_rsum[r] = sum; }
    }
    if (!rows) return;
    bb_sum = n;
  }
  bb_n = 0;                                       /* (el vblank no sube un buffer a medias) */
  bb_convert(n, rows);
  bb_rows |= rows;
  bb_n = n & ~31;
  bb_ever = 1;
}

extern uint32 md_bb_rows_done;                    /* md_rbeam.c: filas que el cuerpo reescribió en este vblank */
static void bb_vblank(void) {
  for (uint16 r = 0; r < bb_n >> 5; r++) {
    if (!(bb_rows & (1UL << r))) continue;
    md_bb_rows_done |= 1UL << r;
    vdp_dma_vram(bb_buf + r * 32, MD_VRAM_PLANE_B + r * 128, 32);
    vdp_dma_vram(bb_buf + r * 32, MD_VRAM_PLANE_B + (r + 32) * 128, 32);
  }
  if (zbb && zbb->cache) memcpy(bc_shown, bc_next, BC_B);   /* ahora se ve este mapa */
  bb_n = 0; bb_rows = 0;
}

void nt_animtiles_transfer(uint16 src, uint16 size, uint16 vram) {
  (void)size;
  if (md_zebes && zg && zg->nanim && nlanim < QANIM) { lanim[nlanim][0] = src; lanim[nlanim][1] = vram; nlanim++; }
}

void nt_enemy_vram_copy(uint32 src, uint16 dst) {
  if (md_zebes && zo && zo->vcp && nlvcp < QVCP) {
    lvcp[nlvcp][0] = (uint16)src; lvcp[nlvcp][1] = (uint16)(src >> 16); lvcp[nlvcp][2] = dst; nlvcp++;
  }
}
static void vcp_vblank(void) {
  uint8 lp = md_cur_page;
  map_page(zo->page);
  const uint16 *v0 = zo->vcp;
  for (uint16 q = 0; q < nvcp; q++) {
    const uint16 *v = v0 + 1;
    for (uint16 n = v0[0]; n; n--) {
      uint16 nt = v[3];
      if (v[0] == qvcp[q][0] && v[1] == qvcp[q][1] && v[2] == qvcp[q][2]) {
        const uint16 *t = v + 4;
        for (; nt; nt--, t += 17) {
          vdp_set_vram_write(t[0] * 32);
          for (uint16 w = 1; w <= 16; w++) VDP_DATA = t[w];
        }
        break;
      }
      v += 4 + 17 * nt;
    }
  }
  nvcp = 0;
  map_page(lp);
}

/* (md_post, con el vblank libre) lo de la lógica pasa a las colas del vblank */
uint16 frz_lf;                           /* cuadros de lógica (zebes_take): el reloj del depósito por cuadro */
void zebes_take(void) {
  if (lfull) { full_redraw = 1; lfull = 0; }
  for (uint16 k = 0; k < nlblk; k++) block_changed(lblk[k]);
  nlblk = 0;
  xray_take();
  for (uint16 k = 0; k < nlanim && nanim < QANIM; k++) { qanim[nanim][0] = lanim[k][0]; qanim[nanim][1] = lanim[k][1]; nanim++; }
  nlanim = 0;
  for (uint16 k = 0; k < nlvcp && nvcp < QVCP; k++) { qvcp[nvcp][0] = lvcp[k][0]; qvcp[nvcp][1] = lvcp[k][1]; qvcp[nvcp][2] = lvcp[k][2]; nvcp++; }
  nlvcp = 0;
  for (uint16 k = 0; k < frz_nq_l && frz_nq < frz_ns; k++) frz_q[frz_nq++] = frz_q_l[k];   /* congelados */
  frz_nq_l = 0;
  frz_lf++;
  { void cp_take_(void); cp_take_(); }
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
  if (esc_on) esc_vblank();
  if (bgr_row >= 0) {                          /* (rayos X) el fondo de biblioteca con la prioridad nueva */
    uint8 lp = md_cur_page;
    map_page(zg->page);
    for (uint16 k = 0; k < 8 && bgr_row < 64; k++, bgr_row++) {
      if (zg->bg2z) {                            /* (comprimido) la fila del plano B, con la prioridad del mapa */
        uint16 row[64], p = zg->bg2p == 1 ? 0x8000 : 0;
        uint32 a = MD_VRAM_PLANE_B + (uint16)bgr_row * 128;
        VDP_CTRL32 = (a & 0x3FFF) << 16 | a >> 14;   /* (lectura de la VRAM) */
        for (uint16 i = 0; i < 64; i++) row[i] = (uint16)(((VDP_DATA & 0x7FFF) | p) & zpmask_b);
        vdp_set_vram_write((uint16)a);
        for (uint16 i = 0; i < 64; i++) VDP_DATA = row[i];
        continue;
      }
      const uint16 *c = zg->bg2 + (zg->bg2h ? (uint16)bgr_row * 64 : (uint16)(bgr_row & 31) * 64);
      vdp_set_vram_write(MD_VRAM_PLANE_B + (uint16)bgr_row * 128);
      for (uint16 i = 0; i < 64; i++) VDP_DATA = tile_fix_b(c[i]);
    }
    map_page(lp);
    if (bgr_row >= 64) bgr_row = -1;
  }
  if (bt_pending) { bt_pending = 0; bt_dma(); }
  if (bc_npend && zbb) {                         /* (las filas esperan a que estén sus tiles) */
    if (bc_atomic && bb_n) { bc_atomic = 0; bc_vblank(BC_N); }   /* el mapa que se ve ya perdió tiles: todo junto */
    else bc_vblank(96);
  }
  if (bb_n && !bc_npend) bb_vblank();
  if (melt_n) melt_vblank();
  if (zw && (kq_conv || wall_pend || mouth_pend)) kq_vblank();                    /* (después de la caché: los tiles de Crocomire derritiéndose) */
  if (nanim) anim_vblank();
  if (nvcp && zo && zo->vcp) vcp_vblank();
  if (full_redraw) {                          /* salto grande: toda la ventana (rara vez: puertas, temblores) */
    uint8 f = full_redraw;
    full_redraw = 0;
    ncol = nrow = nblk = nsky = 0;
    for (uint16 k = 0; k < 16; k++) xr_shown[k] = 0;   /* (los revelados de los rayos X: otra vez el cuadro siguiente) */
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
const uint16 *sky_entries(void);
void zebes_sky_lines(uint16 *h, uint16 ax, uint16 ay) {
  /* por franja del cielo: un tramo de líneas con el mismo par (plano A, plano B), escrito de a 32 bits */
  const uint16 *st = sky_entries();               /* (hdmaobj.c: la tabla en palabras alineadas) */
  #define SKYE(i, f) st[4 * (i) + (f)]
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
/* jefe de colores propios (mkzobj.BOSS_LINE): sus objetos usan la línea 1 de la CRAM */
uint8 zebes_boss_line(void) { return md_zebes && zo && zg && zg->npal == 1; }

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
/* colores 1-15 de los objetos (debajo del HUD) con un color sumado (canales de 5 bits, recortados): el líquido de la
   capa 3 (md_engine.c, lq_prepare) */
void zebes_obj_colors_tab(const uint8 *t, uint16 *out) {   /* t: 3 tablas de 32 (md_engine.c, lq_tables) */
  if (!zo || zobj_srcp_of != zo) { for (uint16 i = 0; i < 15; i++) out[i] = zobj_col[i]; return; }
  for (uint16 i = 0; i < 15; i++) {
    uint16 c = *zobj_srcp[i];
    out[i] = t[c & 31] | (uint16)t[32 + ((c >> 5) & 31)] << 4 | (uint16)t[64 + ((c >> 10) & 31)] << 8;
  }
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
  extern uint8 tint_on;
  if (!tint_on) {                            /* (lo normal: sin la llamada por color) */
    for (uint16 i = 0; i < 15; i++) *o++ = kCram555[**p++ & 0x7FFF];
    return;
  }
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
/* dos vías: 32 conjuntos de 2 (h, h + 1; las claves de un cuadro chocaban en la tabla directa y cada choque era una
   búsqueda binaria); la nueva entra en h y la que estaba pasa a h + 1 */
static inline int16 zc_find(uint16 sm, uint16 pal, uint16 tiles) {
  uint16 h = (sm ^ (sm >> 6) ^ pal ^ tiles) & 62;      /* (el par h, h + 1 es un conjunto) */
  if (zcache[h].zo == zo && zcache[h].sm == sm && zcache[h].pal == pal && zcache[h].tiles == tiles) return h;
  h ^= 1;
  if (zcache[h].zo == zo && zcache[h].sm == sm && zcache[h].pal == pal && zcache[h].tiles == tiles) return h;
  return -1;
}
static inline void zc_put(uint16 sm, uint16 pal, uint16 tiles, const uint16 *r) {
  uint16 h = (sm ^ (sm >> 6) ^ pal ^ tiles) & 62;
  zcache[h ^ 1] = zcache[h];
  zcache[h].sm = sm; zcache[h].pal = pal; zcache[h].tiles = tiles; zcache[h].rec = r; zcache[h].zo = zo;
}
/* con la página de los objetos puesta: los registros de la clave (de zcache o de la búsqueda) o 0 */
const uint16 *zebes_obj_find_cached(uint16 sm, uint16 pal, uint16 tiles) {
  int16 h = zc_find(sm, pal, tiles);
  if (h >= 0) return zcache[h].rec;
  const uint16 *r = zebes_obj_find(sm, pal, tiles);
  zc_put(sm, pal, tiles, r);
  return r;
}
/* luz de la bomba de poder (md_cone.c): el plano A con prioridad alta (on) o la de cada celda. Las celdas que entran
   ya salen así; las de la ventana dibujada se vuelven a escribir de a PSWEEP columnas por cuadro (toda la ventana de
   una vez costaba ~10 cuadros perdidos; md_cone.c lo pide al poner la bomba, cuando sobra tiempo); zebes_prio_busy
   dice si todavía falta */
void zebes_force_prio(uint8 on) {      /* (on 2: todas con prioridad baja, los rayos X: el tiempo está detenido, así
                                          que se reescriben más columnas por cuadro) */
  uint16 p = on == 1 ? 0x8000 : 0, m = on == 2 ? 0x7FFF : 0xFFFF;
  if (p == zprio && m == zpmask) return;
  psweep_n = (on == 2 || zpmask != 0xFFFF || xray_on || time_is_frozen_flag) ? 8 : PSWEEP;   /* (los rayos X: el tiempo
                                                                                    está detenido, hay lugar) */
  zprio = p; zpmask = m;
  psweep = 0; psweep_x0 = wx[0];
}
uint8 zebes_prio_busy(void) { return psweep < ZW; }
/* rayos X: el plano B con prioridad baja (low) o la suya: se vuelve a escribir (la ventana de la capa 2 o el cielo
   de una vez; el fondo de biblioteca, de a 8 filas por vblank) */
void zebes_plane_b_low(uint8 low) {
  uint16 m = low ? 0x7FFF : 0xFFFF;
  if (m == zpmask_b) return;
  zpmask_b = m;
  if (!drawn[1] && !zg->bg2 && !zg->bg2z) return;
  if (zg->l2 || zg->sky) full_redraw |= 2;
  else if ((zg->bg2 || zg->bg2z) && !zbb) bgr_row = 0;
}

/* ---------------------------------------------------------------- enemigos congelados (docs/58) */
/* WriteEnemyOams ($A0:944A) dibuja al congelado con la paleta 0x0C00. Sus claves (mkzobj.frozen_entry) apuntan a un
   juego común (kZebesFrz, página ZEBES_FRZ_PAGE): los tiles con los colores del hielo van en el depósito y se suben a
   la VRAM cuando se dibuja, a un lugar de las filas 5-31 del plano Window (no se ven: la ventana solo muestra el HUD).
   Un congelado no se anima: el mismo spritemap mientras dura. 3 lugares de 18 tiles (el 90 % de los congelados
   tiene 16 o menos; uno más grande toma lugares seguidos). Sin lugar (o con la caja de mensaje, que usa esas filas,
   o en las salas del ojo, donde las usa el haz) se dibuja con su paleta (md_engine.c). */
/* por cuadro, en la zona del jefe después de los registros: frz_pend (por lugar: 1 = sus tiles todavía no se subieron;
   no se dibuja) y frz_fb (por enemigo: la última clave grande que se dibujó entera, para dibujarla mientras se sube la
   nueva) */
#define FRZ_PEND_OFF 3840
#define FRZ_FB_OFF (FRZ_PEND_OFF + FRZ_NMAX)
#define FRZ_FB_N 64                              /* respaldos: 32 enemigos x 2 piezas */
#define FRZ_EPFB_N 18                            /* y uno por proyectil enemigo (después de los de los enemigos) */
_Static_assert(FRZ_NMAX * 40 * 2 <= FRZ_PEND_OFF && FRZ_FB_OFF + 2 * (FRZ_FB_N + FRZ_EPFB_N) <= MD_BOSS_BYTES, "frz_rec: no entra en la zona del jefe");
#define frz_rec md_boss_zone
#define frz_pend ((uint8 *)md_boss_zone + FRZ_PEND_OFF)   /* 1: el lugar está en una cola (sus tiles por subir) */
#define frz_fb ((uint16 *)((uint8 *)md_boss_zone + FRZ_FB_OFF))
#define FRZ_VB_KEEP 12                           /* líneas del vblank que frz_vblank deja para lo que sigue */
#ifndef FRZ_FB_MIN
#define FRZ_FB_MIN 1                             /* tiles desde los que una clave tiene respaldo (frz_fb): todas (con 48,
                                                    solo las grandes, en la pelea con Draygon faltaban las crías) */
#endif
#define FRZ_FB_KEEP 48                           /* tiles desde los que un respaldo no se pisa (las claves grandes) */
static uint8 frz_is_fb(uint16 id) {              /* (la última pasada no pisa los respaldos grandes: la bebé no
                                                    desaparece; los chicos se usan solo si siguen puestos) */
  if (id >= 0xFFFE || kZebesFrz[4 * id + 2] < FRZ_FB_KEEP) return 0;   /* (con la página ZEBES_FRZ_PAGE puesta) */
  const uint16 *f = frz_fb;
  for (uint16 i = 0; i < FRZ_FB_N + FRZ_EPFB_N; i++) if (f[i] == id) return 1;
  return 0;
}
static void frz_reset(void) {
  frz_nq = 0;
  for (uint16 i = 0; i < frz_ns; i++) frz_id[i] = 0xFFFF;
  frz_nq_l = 0;
  if (frz_stream) {
    memset(frz_pend, 0, FRZ_NMAX);
    memset(frz_fb, 0xFF, 2 * (FRZ_FB_N + FRZ_EPFB_N));
  }
}
void zebes_frz_reset(void) { frz_reset(); }   /* md_engine.c: la caja de mensaje usó las filas del Window */
/* (con la página ZEBES_FRZ_PAGE puesta) primer tile de la VRAM del congelado n, o 0 si no hay lugar */
/* por cuadro (frz_stream): lugar de cada entrada por número (n & 63), para no recorrer el depósito en cada dibujo
   (cinematic_text_tilemap + 2880..3007: libre en esas salas, entre kq_dst y las tablas del depósito) */
#define frz_hc ((uint8 *)cinematic_text_tilemap + 2880)
/* por cuadro (frz_stream): los registros de cada congelado puesto, copiados a la zona del jefe (md.h) con el tile de
   su lugar ya sumado, en FRZ_REC palabras por lugar desde su cabeza: zfrz_put los dibuja sin cambiar a la página del
   depósito. 0xFFFF: no entra (más de 8 sprites por lugar de 8 tiles: 7 congelados de 1321), por la página. */
#define FRZ_REC 40
static void frz_rec_fill(uint16 i, uint16 n, uint16 k) {   /* (con la página ZEBES_FRZ_PAGE puesta) */
  uint16 *d = frz_rec + i * FRZ_REC;
  const uint16 *s = kZebesFrzRecs + kZebesFrz[4 * n + 3];
  uint16 c = s[0];
  if (5 + 4 * c > FRZ_REC * k) { d[0] = 0xFFFF; return; }
  uint16 t = frz_t0 + i * frz_slot;
  for (uint16 j = 0; j < 5; j++) d[j] = s[j];      /* cuántos y la caja */
  d += 5; s += 5;
  for (uint16 j = 0; j < c; j++, d += 4, s += 4) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2] + t; d[3] = s[3]; }
}
static uint16 frz_get(uint16 n) {
  uint16 now = frz_stream ? frz_lf : md_frame_count;
  if (frz_stream) {
    uint8 c = frz_hc[n & 127];
    if (c < frz_ns && frz_id[c] == n) { frz_last[c] = now; if (frz_pend[c]) return 0; return frz_t0 + (c << 3); }   /* (por cuadro: lugares de 8) */
  }
  for (uint16 i = 0; i < frz_ns; i++)
    if (frz_id[i] == n) {
      frz_last[i] = now;
      if (frz_stream) { frz_hc[n & 127] = (uint8)i; if (frz_pend[i]) return 0; }
      return frz_t0 + i * frz_slot;
    }
  uint16 k = (kZebesFrz[4 * n + 2] + frz_slot - 1) / frz_slot;   /* lugares que necesita */
  if (frz_stream) {                             /* (y por sus registros: FRZ_REC palabras por lugar) */
    uint16 kr = (5 + 4 * kZebesFrzRecs[kZebesFrz[4 * n + 3]] + FRZ_REC - 1) / FRZ_REC;
    if (kr > k) k = kr;
  }
  if (k > frz_ns) return 0;
  /* libres: sin usar o sin dibujarse hace FRZ_KEEP cuadros (los del cuadro anterior no: puede no haberse visto; por
     cuadro: 1, los tiles y la lista de sprites cambian juntos en el vblank, frz_q). Una pasada por los bloques (cabeza
     y su resto, 0xFFFE): el primer tramo de bloques libres seguidos con k lugares, desde su comienzo */
  uint16 i = 0, run = 0, st = 0;
  /* por cuadro, tres pasadas (frz_lf: cuadros de lógica): guardando lo de los últimos FRZ_KEEP cuadros (lo que
     parpadea o se dibuja cada tanto no se vuelve a subir), lo del cuadro anterior, y nada (salvo los respaldos de las
     claves grandes, frz_fb). La última solo si el vblank ya tomó el cuadro anterior: si no (la lógica de este empieza
     antes cuando el anterior fue pesado, pipe_wait), sus lugares siguen siendo suyos. Lo nuevo no se dibuja hasta que
     el vblank sube sus tiles (frz_pend); lo pisado lo usa solo la lista de sprites de antes, que cambia en ese vblank */
  uint8 md_published(void);
  static const uint8 kKeepPass[3] = { FRZ_KEEP, 1, 0 };
  uint8 npass = !frz_stream ? 1 : md_published() ? 2 : 3;
  for (uint8 pass = 0; pass < npass; pass++) {
  uint16 keep = frz_stream ? kKeepPass[pass] : FRZ_KEEP;
  run = 0; st = 0;
  if (frz_stream && k >= 4) {
    /* por cuadro, los grandes (la bebé de Mother Brain: 13) desde el final y los chicos desde el principio: si no, los
       chicos quedaban repartidos y no había k lugares seguidos (la bebé no se dibujaba). Libre: el lugar sin usar, o
       el de un objeto (su cabeza: hacia atrás por los 0xFFFE) que no se dibujó en el cuadro anterior */
    for (int16 j = (int16)frz_ns - 1; j >= 0; j--) {
      int16 h = j;
      while (h > 0 && frz_id[h] == 0xFFFE) h--;
      uint16 id = frz_id[h];
      uint8 fr = id == 0xFFFF || (id == 0xFFFE && h == j) || ((uint16)(now - frz_last[h]) > keep && !(keep == 0 && frz_is_fb(id)));
      if (!fr) { run = 0; continue; }
      if (++run >= k) { st = (uint16)j; break; }
    }
  } else
  for (uint16 h = 0; h < frz_ns; ) {
    uint16 j = h + 1;
    while (j < frz_ns && frz_id[j] == 0xFFFE) j++;
    uint16 id = frz_id[h];
    if (!(id >= 0xFFFE || ((uint16)(now - frz_last[h]) > keep && !(frz_stream && keep == 0 && frz_is_fb(id))))) { run = 0; h = j; continue; }
    if (!run) st = h;
    run += j - h;
    h = j;
    if (run >= k) break;
  }
  if (run >= k) break;
  }
  if (run < k) return 0;
  i = st;
  {
    uint16 m;
    if (frz_id[i] == 0xFFFE) {                    /* empieza a mitad de uno que se suelta: su cabeza ya no vale */
      uint16 h = i;
      while (h && frz_id[h] == 0xFFFE) h--;
      for (; h < i; h++) frz_id[h] = 0xFFFF;
    }
    if (i + k < frz_ns && frz_id[i + k] == 0xFFFE)   /* el resto de uno que se suelta: queda libre */
      for (uint16 j = i + k; j < frz_ns && frz_id[j] == 0xFFFE; j++) frz_id[j] = 0xFFFF;
    frz_id[i] = n; frz_last[i] = now;
    if (frz_stream) { frz_hc[n & 127] = (uint8)i; frz_rec_fill(i, n, k); }
    for (m = 1; m < k; m++) frz_id[i + m] = 0xFFFE;
    if (frz_stream) {
      /* (por cuadro: no se dibuja hasta que el vblank suba sus tiles, frz_vblank; si el lugar ya estaba en una cola,
         se sube con la clave que tenga entonces) */
      if (!frz_pend[i]) { frz_pend[i] = 1; if (frz_nq_l < frz_ns) frz_q_l[frz_nq_l++] = (uint8)i; }
      { void cp_all_dirty_(uint16 n); cp_all_dirty_(n); }
      return 0;
    }
    if (frz_nq_l < frz_ns) frz_q_l[frz_nq_l++] = (uint8)i;
    return frz_t0 + i * frz_slot;
  }
}
/* (md_vblank, justo después de la lista de sprites: la lista de este cuadro ya usa estos tiles. La bebé de Mother Brain
   sube 104 de una vez; al final del vblank, detrás del cuerpo y las filas, se terminaban de subir con la imagen ya
   empezada y se veía un cuadro con tiles a medio subir) */
void zebes_frz_vblank(void) { if (frz_nq) frz_vblank(); }
static void frz_vblank(void) {
  /* por cuadro: lo que entra en el vblank (FRZ_VB_KEEP líneas para lo que viene después: los tiles de Samus...; al menos
     un lugar por vblank); lo demás queda en la cola para el siguiente, sin dibujarse (frz_pend) */
  int16 md_vb_lines_left(void);
  uint8 lp = md_cur_page;
  uint16 q = 0;
  for (; q < frz_nq; q++) {
    uint16 i = frz_q[q];
    map_page(ZEBES_FRZ_PAGE);
    uint16 n = frz_id[i];
    if (n < 0xFFFE) {
      const uint16 *e = kZebesFrz + 4 * n;
      if (frz_stream && q && md_vb_lines_left() < FRZ_VB_KEEP + 2 + (int16)(e[2] / 3)) break;
      zebes_dma_runs(ZEBES_FRZ_PAGE, kZebesFrzRuns + 2 * e[0], e[1], frz_t0 + i * frz_slot);
    }
    if (frz_stream) frz_pend[i] = 0;
  }
  uint16 r = 0;
  for (; q < frz_nq; q++) frz_q[r++] = frz_q[q];
  frz_nq = r;
  map_page(lp);
}
typedef struct { int16 y; uint16 size_link; uint16 attr; int16 x; } GenSpr;   /* (md_engine.c) */
extern GenSpr *sp_ptr;
extern uint8 nt_msgbox_active;
/* congelado n en (x, y) */
/* registros de a0 en (x, y) con t sumado al atributo de cada sprite (md_engine.c, md_putrecs_t), sin llamada en C */
static inline __attribute__((always_inline)) void put_recs_t(const uint16 *r, int16 x, int16 y, uint16 t) {
  register int32_t d0 __asm__("d0") = x;
  register int32_t d1 __asm__("d1") = y;
  register const uint16 *a0 __asm__("a0") = r;
  register uint32_t d4 __asm__("d4") = t;
  __asm__ volatile("jsr md_putrecs_t" : "+d"(d0), "+d"(d1), "+a"(a0) : "d"(d4) : "a1", "cc", "memory");
}
static inline __attribute__((always_inline)) uint8 frz_draw(uint16 n, int16 x, int16 y) {
  if (!frz_stream && (nt_msgbox_active || zg->eye)) return 0;   /* (la caja y el haz usan las filas del Window) */
  uint8 lp = md_cur_page;
  map_page(ZEBES_FRZ_PAGE);
  uint16 t = frz_get(n);
  if (t) put_recs_t(kZebesFrzRecs + kZebesFrz[4 * n + 3], x, y, t);
  map_page(lp);
  return t != 0;
}

/* proyectiles enemigos con una clave por cuadro (md_engine.c, ep_lookup): el número de la entrada (o -1 si es una
   clave fija), y su dibujo en (x, y) con el lugar del depósito sumado, desde la página del depósito */
int16 zebes_ep_frozen(const uint16 *r) {   /* (-2: congelado en una sala sin depósito por cuadro: no se dibuja) */
  if (!zo || (uint16)(r - zo->recs) < FROZEN_REF) return -1;
  if (!frz_stream) return -2;
  return (int16)((uint16)(r - zo->recs) - FROZEN_REF);
}
uint16 ep_cur_idx;                                /* el proyectil que se dibuja (md_engine.c: índice x 2) */
static __attribute__((noinline)) void zebes_ep_draw_slow(uint16 n, int16 x, int16 y) {
  uint8 lp = md_cur_page;
  map_page(ZEBES_FRZ_PAGE);
  uint16 t = frz_get(n);
#ifdef MD_NO_EPFB
  uint16 *fb = 0;
#else
  uint16 *fb = frz_stream && ep_cur_idx < 2 * FRZ_EPFB_N ? &frz_fb[FRZ_FB_N + (ep_cur_idx >> 1)] : 0;
#endif
  if (t) {
    put_recs_t(kZebesFrzRecs + kZebesFrz[4 * n + 3], x, y, t);
    if (fb) *fb = n;
  } else {
    /* (sus tiles se están subiendo: la última clave que este proyectil dibujó, si sigue puesta) */
    uint16 m = fb ? *fb : 0xFFFF;
    uint8 c = m < 0xFFFE ? frz_hc[m & 127] : 0xFF;
    if (m < 0xFFFE && m != n && c < frz_ns && frz_id[c] == m && !frz_pend[c]) {
      frz_last[c] = frz_lf;
      put_recs_t(kZebesFrzRecs + kZebesFrz[4 * m + 3], x, y, frz_t0 + (c << 3));
    }
#ifdef MD_FRZ_MISS
    else (*(volatile uint16 *)0xFFF562)++;        /* (prueba: dibujos de proyectiles que faltan) */
#endif
  }
  map_page(lp);
}
void zebes_ep_draw(uint16 n, int16 x, int16 y) {   /* (lo común, corto: el lugar ya puesto) */
  uint8 c = frz_hc[n & 127];
  if (c < frz_ns && frz_id[c] == n && !frz_pend[c]) {
    frz_last[c] = frz_lf;
    if (ep_cur_idx < 2 * FRZ_EPFB_N) frz_fb[FRZ_FB_N + (ep_cur_idx >> 1)] = n;
    uint8 lp = md_cur_page;
    map_page(ZEBES_FRZ_PAGE);
    put_recs_t(kZebesFrzRecs + kZebesFrz[4 * n + 3], x, y, frz_t0 + (c << 3));   /* (por cuadro: lugares de 8) */
    map_page(lp);
    return;
  }
  zebes_ep_draw_slow(n, x, y);
}

static __attribute__((noinline, noclone, used)) uint8 zebes_obj_draw_slow(uint16 sm, uint16 pal, uint16 tiles, int16 x, int16 y) {
  if (!zo) return 0;
  uint8 lp = md_cur_page;
  /* (un acierto de zcache no lee la página de los objetos: solo se pone para buscar o para sus registros) */
  int16 h = zc_find(sm, pal, tiles);
  const uint16 *r;
  if (h >= 0) {
    r = zcache[h].rec;
  } else {
    map_page(zo->page);
    r = zebes_obj_find(sm, pal, tiles);
    zc_put(sm, pal, tiles, r);
  }
  uint8 ok = 0;
  if (!r) {
  } else if ((uint16)(r - zo->recs) >= FROZEN_REF) {   /* congelado o por cuadro: (0x8000 | número) */
    uint16 n = (uint16)(r - zo->recs) - FROZEN_REF;
    if (frz_stream || !(nt_msgbox_active || zg->eye)) {   /* (la caja y el haz usan las filas del Window) */
      map_page(ZEBES_FRZ_PAGE);
      uint16 t = frz_get(n);
      uint16 *fb = 0;
      if (frz_stream && kZebesFrz[4 * n + 2] >= FRZ_FB_MIN) {
        /* (por enemigo y por pieza grande del cuadro: un enemigo dibuja varias, Draygon el cuerpo y los brazos) */
        static uint16 fb_en = 0xFFFF, fb_lf;
        static uint8 fb_part;
        if (cur_enemy_index == fb_en && frz_lf == fb_lf) fb_part++;
        else { fb_en = cur_enemy_index; fb_lf = frz_lf; fb_part = 0; }
        if (fb_part < 2) fb = &frz_fb[((cur_enemy_index >> 6) & 31) * 2 + fb_part];
      }
      if (t) {
        put_recs_t(kZebesFrzRecs + kZebesFrz[4 * n + 3], x, y, t); ok = 1;
        if (fb) *fb = n;
      } else if (fb && *fb != n && *fb < 0xFFFE) {
        /* (por cuadro: la clave grande se está subiendo o no tiene lugar: la última que este enemigo dibujó entera) */
        uint16 m = *fb;
        uint8 c = frz_hc[m & 127];
        if (c < frz_ns && frz_id[c] == m && !frz_pend[c]) {
          frz_last[c] = frz_lf;
          put_recs_t(kZebesFrzRecs + kZebesFrz[4 * m + 3], x, y, frz_t0 + (c << 3));
          ok = 1;
        }
      }
#ifdef MD_FRZ_MISS
      if (frz_stream && !ok) (*(volatile uint16 *)0xFFF560)++;   /* (prueba: dibujos de enemigos que faltan) */
#endif
    }
  } else {
    map_page(zo->page);
    put_recs_t(r, x, y, 0);
    ok = 1;
  }
  map_page(lp);
  return ok;
}

#ifdef __mc68000__
/* zebes_obj_draw en ensamblador (el C de la rama #else es la referencia): sin el guardado de once registros del C;
   lo que no sea lo común salta a zebes_obj_draw_slow con los mismos argumentos */
_Static_assert(offsetof(ZebesOvl, zc) == 0 && sizeof(zcache[0]) == 14 && offsetof(__typeof__(zcache[0]), rec) == 6 &&
               offsetof(__typeof__(zcache[0]), zo) == 10, "zcache: el ensamblador de zebes_obj_draw");
_Static_assert(offsetof(ZebesObj, recs) == 20 && offsetof(ZebesObj, page) == 0, "ZebesObj.recs y .page: el ensamblador de zebes_obj_draw");
_Static_assert(FROZEN_REF == 0x8000, "zebes_obj_draw: constantes del ensamblador");
#define ZFP_S1(x) #x
#define ZFP_S(x) ZFP_S1(x)   /* ZEBES_FRZ_PAGE en el ensamblador */
__asm__(
  "    .text\n"
  /* zfrz_put: el congelado n (d0.w) en (x d1.w, y a1.w) si ya tiene su lugar del cuadro (frz_hc): d0 = 1; si no, 0
     (el camino lento lo busca o lo sube). Preserva d2-d7/a2-a6. (También md_draw_eprojs_asm, md_engine.c.) */
  "    .global zfrz_put\n"
  "zfrz_put:\n"
  "    movem.l %d2-%d7/%a2,-(%sp)\n"
  "    movea.w %d1,%a2\n"                     /* x */
  "    moveq #127,%d2\n"
  "    and.w %d0,%d2\n"
  "    lea cinematic_text_tilemap+2880,%a0\n" /* frz_hc */
  "    moveq #0,%d3\n"
  "    move.b (%a0,%d2.w),%d3\n"              /* c */
  "    cmp.b frz_ns,%d3\n"
  "    bcc 8f\n"
  "    add.w %d3,%d3\n"
  "    movea.l frz_id,%a0\n"
  "    cmp.w (%a0,%d3.w),%d0\n"
  "    bne 8f\n"
  "    lea md_boss_zone+" ZFP_S(FRZ_PEND_OFF) ",%a0\n"   /* frz_pend: sin subir todavía (el camino lento) */
  "    move.w %d3,%d4\n"
  "    lsr.w #1,%d4\n"
  "    tst.b (%a0,%d4.w)\n"
  "    bne 8f\n"
  "    movea.l frz_last,%a0\n"
  "    move.w frz_lf,(%a0,%d3.w)\n"
  "    move.w %d3,%d4\n"                     /* sus registros en la zona del jefe (frz_rec: 80 bytes por lugar) */
  "    lsl.w #3,%d4\n"
  "    move.w %d4,%d2\n"
  "    lsl.w #2,%d2\n"
  "    add.w %d4,%d2\n"                      /* 80 c */
  "    lea md_boss_zone,%a0\n"
  "    adda.w %d2,%a0\n"
  "    cmp.w #0xFFFF,(%a0)\n"
  "    beq 5f\n"                              /* (no entró: por la página) */
  "    move.l %a2,%d0\n"                     /* x */
  "    move.l %a1,%d1\n"                     /* y */
  "    jsr md_putrecs_ns\n"                  /* (d2/d4-d7/a2 ya guardados) */
  "    movem.l (%sp)+,%d2-%d7/%a2\n"
  "    moveq #1,%d0\n"
  "    rts\n"
  "5:  moveq #0,%d4\n"
  "    move.w %d3,%d4\n"
  "    lsl.w #2,%d4\n"
  "    add.w frz_t0,%d4\n"                    /* frz_t0 + (c << 3) */
  "    move.b md_cur_page,%d3\n"              /* la página de antes */
  "    move.b #" ZFP_S(ZEBES_FRZ_PAGE) ",md_cur_page\n"
  "    cmp.b #" ZFP_S(ZEBES_FRZ_PAGE) ",md_hw_page\n"
  "    beq 3f\n"
  "    move.b #" ZFP_S(ZEBES_FRZ_PAGE) ",md_hw_page\n"
  "    move.b #" ZFP_S(ZEBES_FRZ_PAGE) ",0xA130FF\n"
  "3:  and.l #0xFFFF,%d0\n"
  "    lsl.l #3,%d0\n"
  "    lea kZebesFrz,%a0\n"
  "    moveq #0,%d1\n"
  "    move.w 6(%a0,%d0.l),%d1\n"             /* kZebesFrz[4 n + 3] */
  "    add.l %d1,%d1\n"
  "    move.l %a1,%d2\n"                      /* y */
  "    lea kZebesFrzRecs,%a0\n"
  "    adda.l %d1,%a0\n"
  "    move.l %a2,%d0\n"
  "    move.l %d2,%d1\n"
  "    jsr md_putrecs_t\n"
  "    move.b %d3,md_cur_page\n"
  "    cmp.b md_hw_page,%d3\n"
  "    beq 4f\n"
  "    move.b %d3,md_hw_page\n"
  "    move.b %d3,0xA130FF\n"
  "4:  movem.l (%sp)+,%d2-%d7/%a2\n"
  "    moveq #1,%d0\n"
  "    rts\n"
  "8:  movem.l (%sp)+,%d2-%d7/%a2\n"
  "    moveq #0,%d0\n"
  "    rts\n"
  "    .global zebes_obj_draw\n"
  "zebes_obj_draw:\n"                        /* sm 6(sp), pal 10, tiles 14, x 18, y 22 (palabras bajas) */
  "    move.l zo,%d0\n"
  "    beq 80f\n"
  "    movea.l %d0,%a0\n"                     /* zo */
  "    movem.l %d2-%d4,-(%sp)\n"              /* (los argumentos: + 12) */
  "    move.w 18(%sp),%d1\n"                  /* sm */
  "    move.w %d1,%d2\n"
  "    lsr.w #6,%d2\n"
  "    eor.w %d1,%d2\n"
  "    move.w 22(%sp),%d3\n"                  /* pal */
  "    eor.w %d3,%d2\n"
  "    move.w 26(%sp),%d4\n"                  /* tiles */
  "    eor.w %d4,%d2\n"
  "    and.w #62,%d2\n"                       /* h (el par h, h + 1) */
  "    add.w %d2,%d2\n"
  "    move.w %d2,%d0\n"
  "    lsl.w #3,%d0\n"
  "    sub.w %d2,%d0\n"                       /* 14 h */
  "    lea md_ovl,%a1\n"                     /* zcache (ZebesOvl, al principio de md_ovl) */
  "    adda.w %d0,%a1\n"
  "    cmpa.l 10(%a1),%a0\n"
  "    bne 1f\n"
  "    cmp.w (%a1),%d1\n"
  "    bne 1f\n"
  "    cmp.w 2(%a1),%d3\n"
  "    bne 1f\n"
  "    cmp.w 4(%a1),%d4\n"
  "    beq 2f\n"
  "1:  lea 14(%a1),%a1\n"                     /* h + 1 */
  "    cmpa.l 10(%a1),%a0\n"
  "    bne 89f\n"
  "    cmp.w (%a1),%d1\n"
  "    bne 89f\n"
  "    cmp.w 2(%a1),%d3\n"
  "    bne 89f\n"
  "    cmp.w 4(%a1),%d4\n"
  "    bne 89f\n"
  "2:  move.l 6(%a1),%d0\n"                   /* rec */
  "    beq 89f\n"
  "    movea.l %d0,%a1\n"
  "    sub.l 20(%a0),%d0\n"                   /* - zo->recs */
  "    asr.l #1,%d0\n"
  "    sub.w #0x8000,%d0\n"                   /* n = ... - FROZEN_REF */
  "    bmi 50f\n"                             /* registros de la ROM (sin FROZEN_REF) */
  "    tst.b frz_stream\n"                    /* congelado: lo rápido solo con los objetos por cuadro */
  "    beq 89f\n"
  "    movem.l (%sp)+,%d2-%d4\n"
  "    move.w 18(%sp),%d1\n"                  /* x */
  "    movea.w 22(%sp),%a1\n"                 /* y */
  "    bsr zfrz_put\n"
  "    tst.w %d0\n"
  "    beq 90f\n"
  "    rts\n"
  /* los registros de la clave en la página de los objetos (zo->page): esa página un momento y md_putrecs_t */
  "50: move.b md_cur_page,%d3\n"
  "    move.b (%a0),%d2\n"                    /* zo->page */
  "    move.b %d2,md_cur_page\n"
  "    cmp.b md_hw_page,%d2\n"
  "    beq 51f\n"
  "    move.b %d2,md_hw_page\n"
  "    move.b %d2,0xA130FF\n"
  "51: movea.l %a1,%a0\n"
  "    move.w 30(%sp),%d0\n"                  /* x */
  "    ext.l %d0\n"
  "    move.w 34(%sp),%d1\n"                  /* y */
  "    ext.l %d1\n"
  "    moveq #0,%d4\n"
  "    jsr md_putrecs_t\n"
  "    move.b %d3,md_cur_page\n"
  "    cmp.b md_hw_page,%d3\n"
  "    beq 52f\n"
  "    move.b %d3,md_hw_page\n"
  "    move.b %d3,0xA130FF\n"
  "52: movem.l (%sp)+,%d2-%d4\n"
  "    moveq #1,%d0\n"
  "    rts\n"
  "80: moveq #0,%d0\n"
  "    rts\n"
  "89: movem.l (%sp)+,%d2-%d4\n"
  "90: jmp zebes_obj_draw_slow\n");
#else
/* lo común, corto (sin el prólogo del camino lento): clave ya en zcache y, si es por cuadro, con su lugar puesto */
uint8 zebes_obj_draw(uint16 sm, uint16 pal, uint16 tiles, int16 x, int16 y) {
  if (!zo) return 0;
  int16 h = frz_stream ? zc_find(sm, pal, tiles) : -1;
  if (h >= 0) {
    const uint16 *r = zcache[h].rec;
    uint16 n = (uint16)(r - zo->recs) - FROZEN_REF;
    if (r && n < 0x8000) {
      uint8 c = frz_hc[n & 127];
      if (c < frz_ns && frz_id[c] == n && !frz_pend[c]) {
        frz_last[c] = frz_lf;
        uint8 lp = md_cur_page;
        map_page(ZEBES_FRZ_PAGE);
        put_recs_t(kZebesFrzRecs + kZebesFrz[4 * n + 3], x, y, frz_t0 + (c << 3));   /* (por cuadro: lugares de 8) */
        map_page(lp);
        return 1;
      }
    }
  }
  return zebes_obj_draw_slow(sm, pal, tiles, x, y);
}
#endif


/* ---------------------------------------------------------------- cadáveres que se pudren (mkzobj.CORPSE, docs/59)
   El SNES cambia los tiles del cadáver en la RAM (bosses.c, como el decomp) y en cada cuadro sube una de sus tablas
   (DeadTorizo_Func_1). Aquí: md_corpse_dirty anota los tiles de la RAM que cambiaron; md_corpse_upload (la tabla que
   sube el SNES en este cuadro) convierte los suyos (4bpp planar del SNES -> 4bpp de la Genesis, con los lugares de
   la línea: zo->corpse) a un búfer de la zona del jefe (libre en estas salas: no son por cuadro), y el vblank los
   sube. Lo que no entra en un cuadro queda marcado para la próxima vez que se sube su tabla. */
#define CP_MAXT 36                               /* tiles por cuadro (por mitad del búfer) */
#define CP_HALF (CP_MAXT * 32 + CP_MAXT * 2)     /* bytes de una mitad: tiles y sus lugares en la VRAM */
#define CP_HASH_OFF (2 * CP_HALF)                /* suma de cada tile del SNES (192 x 4 bytes) al convertirlo */
#define CP_LUT_OFF (CP_HASH_OFF + 192 * 4)       /* tablas de pares de colores: 256 bytes por lut */
#define CP_NLUT ((MD_BOSS_BYTES - CP_LUT_OFF) / 256)
__attribute__((used)) static const uint32 kCpSpread[256] = {
  0x00000000,0x00000001,0x00000010,0x00000011,0x00000100,0x00000101,0x00000110,0x00000111,
  0x00001000,0x00001001,0x00001010,0x00001011,0x00001100,0x00001101,0x00001110,0x00001111,
  0x00010000,0x00010001,0x00010010,0x00010011,0x00010100,0x00010101,0x00010110,0x00010111,
  0x00011000,0x00011001,0x00011010,0x00011011,0x00011100,0x00011101,0x00011110,0x00011111,
  0x00100000,0x00100001,0x00100010,0x00100011,0x00100100,0x00100101,0x00100110,0x00100111,
  0x00101000,0x00101001,0x00101010,0x00101011,0x00101100,0x00101101,0x00101110,0x00101111,
  0x00110000,0x00110001,0x00110010,0x00110011,0x00110100,0x00110101,0x00110110,0x00110111,
  0x00111000,0x00111001,0x00111010,0x00111011,0x00111100,0x00111101,0x00111110,0x00111111,
  0x01000000,0x01000001,0x01000010,0x01000011,0x01000100,0x01000101,0x01000110,0x01000111,
  0x01001000,0x01001001,0x01001010,0x01001011,0x01001100,0x01001101,0x01001110,0x01001111,
  0x01010000,0x01010001,0x01010010,0x01010011,0x01010100,0x01010101,0x01010110,0x01010111,
  0x01011000,0x01011001,0x01011010,0x01011011,0x01011100,0x01011101,0x01011110,0x01011111,
  0x01100000,0x01100001,0x01100010,0x01100011,0x01100100,0x01100101,0x01100110,0x01100111,
  0x01101000,0x01101001,0x01101010,0x01101011,0x01101100,0x01101101,0x01101110,0x01101111,
  0x01110000,0x01110001,0x01110010,0x01110011,0x01110100,0x01110101,0x01110110,0x01110111,
  0x01111000,0x01111001,0x01111010,0x01111011,0x01111100,0x01111101,0x01111110,0x01111111,
  0x10000000,0x10000001,0x10000010,0x10000011,0x10000100,0x10000101,0x10000110,0x10000111,
  0x10001000,0x10001001,0x10001010,0x10001011,0x10001100,0x10001101,0x10001110,0x10001111,
  0x10010000,0x10010001,0x10010010,0x10010011,0x10010100,0x10010101,0x10010110,0x10010111,
  0x10011000,0x10011001,0x10011010,0x10011011,0x10011100,0x10011101,0x10011110,0x10011111,
  0x10100000,0x10100001,0x10100010,0x10100011,0x10100100,0x10100101,0x10100110,0x10100111,
  0x10101000,0x10101001,0x10101010,0x10101011,0x10101100,0x10101101,0x10101110,0x10101111,
  0x10110000,0x10110001,0x10110010,0x10110011,0x10110100,0x10110101,0x10110110,0x10110111,
  0x10111000,0x10111001,0x10111010,0x10111011,0x10111100,0x10111101,0x10111110,0x10111111,
  0x11000000,0x11000001,0x11000010,0x11000011,0x11000100,0x11000101,0x11000110,0x11000111,
  0x11001000,0x11001001,0x11001010,0x11001011,0x11001100,0x11001101,0x11001110,0x11001111,
  0x11010000,0x11010001,0x11010010,0x11010011,0x11010100,0x11010101,0x11010110,0x11010111,
  0x11011000,0x11011001,0x11011010,0x11011011,0x11011100,0x11011101,0x11011110,0x11011111,
  0x11100000,0x11100001,0x11100010,0x11100011,0x11100100,0x11100101,0x11100110,0x11100111,
  0x11101000,0x11101001,0x11101010,0x11101011,0x11101100,0x11101101,0x11101110,0x11101111,
  0x11110000,0x11110001,0x11110010,0x11110011,0x11110100,0x11110101,0x11110110,0x11110111,
  0x11111000,0x11111001,0x11111010,0x11111011,0x11111100,0x11111101,0x11111110,0x11111111,
};
static uint8 cp_dirty[24];                       /* 7E:2000-2FFF: tiles 0-127; 7E:9000-97FF: 128-191 */
static uint8 cp_hval[24];                        /* (sin por cuadro) la suma del tile vale: se convirtió con ella */
static uint8 cp_half, cp_nl, cp_nv, cp_vhalf, cp_on;
/* salas por cuadro (Mother Brain): la zona del jefe es del depósito; el búfer va en la RAM libre de la sala
   (cinematic_text_tilemap + 3648..3919: después de lq_next, antes de lq_steps), 4 tiles por cuadro, y los colores
   se cambian de a uno (sin tablas de pares). Las entradas dicen (entrada del juego común, tile en ella) */
static uint8 cp_stream, cp_maxt;
static uint16 cp_key;                            /* (por cuadro) la entrada del cadáver */
#define CP_SBUF ((uint8 *)cinematic_text_tilemap + 3648)
#define CP_SMAXT 9                               /* (un solo búfer: 9 x 34 bytes; no se convierte mientras el vblank
                                                    no tomó el cuadro anterior) */
static uint8 *cp_buf(uint8 h) {
  return cp_stream ? CP_SBUF : (uint8 *)md_boss_zone + h * CP_HALF;
}
static int16 cp_idx(uint16 a) {
  if (a >= 0x2000 && a < 0x3000) return (int16)((a - 0x2000) >> 5);
  if (a >= 0x9000 && a < 0x9800) return (int16)(128 + ((a - 0x9000) >> 5));
  return -1;
}
void cp_room_(void);
static void cp_room(void) {                      /* (al cambiar de sala: los colores en pares) */
  cp_on = 0; cp_nl = cp_nv = 0; cp_stream = 0; cp_maxt = CP_MAXT;
  memset(cp_dirty, 0, sizeof cp_dirty);
  memset(cp_hval, 0, sizeof cp_hval);
  if (!zo || !zo->corpse) return;
  const uint16 *c = zo->corpse;
  cp_stream = (c[0] & 0x8000) != 0;
  cp_maxt = cp_stream ? CP_SMAXT : CP_MAXT;
  if (cp_stream) { cp_key = c[2 + 16 * c[1]]; cp_on = 1; return; }
  uint16 nl = c[1] < CP_NLUT ? c[1] : CP_NLUT;
  for (uint16 l = 0; l < nl; l++) {
    const uint16 *lut = c + 2 + 16 * l;
    uint8 *d = (uint8 *)md_boss_zone + CP_LUT_OFF + 256 * l;
    for (uint16 b = 0; b < 256; b++) d[b] = (uint8)(lut[b >> 4] << 4 | lut[b & 15]);
  }
  cp_on = 1;
}
void md_corpse_dirty(uint16 a, uint16 bytes) {
  if (!cp_on || !bytes) return;
  int16 i0 = cp_idx(a), i1 = cp_idx((uint16)(a + bytes - 1));
  if (i0 < 0 || i1 < i0) return;
  /* (por bytes: el pudrirse marca columnas de ~10 tiles muchas veces por cuadro) */
  uint16 b0 = (uint16)i0 >> 3, b1 = (uint16)i1 >> 3;
  uint8 m0 = (uint8)(0xFF << (i0 & 7)), m1 = (uint8)(0xFF >> (7 - (i1 & 7)));
  if (b0 == b1) { cp_dirty[b0] |= m0 & m1; return; }
  cp_dirty[b0] |= m0;
  for (uint16 b = b0 + 1; b < b1; b++) cp_dirty[b] = 0xFF;
  cp_dirty[b1] |= m1;
}
#ifdef __mc68000__
/* cp_conv en ensamblador (el C de abajo es la referencia): por fila, las 4 tablas de kCpSpread juntas en un largo
   (8 píxeles de 4 bits), se escribe y cada byte (dos píxeles) pasa por la tabla de pares pl en su lugar */
void cp_conv_asm(uint8 *d, const uint16 *w, const uint8 *pl);
__asm__(
  "    .text\n"
  "    .global cp_conv_asm\n"
  "cp_conv_asm:\n"
  "    movem.l %d2-%d3/%d7/%a2-%a3,-(%sp)\n"
  "    movea.l 24(%sp),%a1\n"                 /* d */
  "    movea.l 28(%sp),%a0\n"                 /* w (palabras nativas: el byte bajo en +1) */
  "    movea.l 32(%sp),%a2\n"                 /* pl */
  "    lea kCpSpread,%a3\n"
  "    moveq #7,%d7\n"
  "1:  moveq #0,%d0\n"
  "    move.b 1(%a0),%d0\n"                   /* plano 0 */
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    move.l (%a3,%d0.w),%d1\n"
  "    moveq #0,%d0\n"
  "    move.b (%a0),%d0\n"                    /* plano 1 */
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    move.l (%a3,%d0.w),%d2\n"
  "    add.l %d2,%d2\n"
  "    or.l %d2,%d1\n"
  "    moveq #0,%d0\n"
  "    move.b 17(%a0),%d0\n"                  /* plano 2 */
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    move.l (%a3,%d0.w),%d2\n"
  "    lsl.l #2,%d2\n"
  "    or.l %d2,%d1\n"
  "    moveq #0,%d0\n"
  "    move.b 16(%a0),%d0\n"                  /* plano 3 */
  "    add.w %d0,%d0\n"
  "    add.w %d0,%d0\n"
  "    move.l (%a3,%d0.w),%d2\n"
  "    lsl.l #3,%d2\n"
  "    or.l %d2,%d1\n"
  "    addq.l #2,%a0\n"
  "    move.l %d1,(%a1)\n"                    /* los 4 bytes y cada uno por la tabla de pares */
  "    moveq #0,%d0\n"
  "    move.b (%a1),%d0\n"
  "    move.b (%a2,%d0.w),(%a1)+\n"
  "    move.b (%a1),%d0\n"
  "    move.b (%a2,%d0.w),(%a1)+\n"
  "    move.b (%a1),%d0\n"
  "    move.b (%a2,%d0.w),(%a1)+\n"
  "    move.b (%a1),%d0\n"
  "    move.b (%a2,%d0.w),(%a1)+\n"
  "    dbra %d7,1b\n"
  "    movem.l (%sp)+,%d2-%d3/%d7/%a2-%a3\n"
  "    rts\n");
#ifndef MD_CP_CHECK
#define cp_conv cp_conv_asm
#endif
#endif
#if !defined(__mc68000__) || defined(MD_CP_CHECK)
static void cp_conv_c(uint8 *d, const uint16 *w, const uint8 *pl) {   /* un tile: w = sus 16 palabras del SNES */
  for (uint16 r = 0; r < 8; r++) {
    uint16 a = w[r], b = w[8 + r];
    uint32 v = kCpSpread[a & 0xFF] | kCpSpread[a >> 8] << 1 | kCpSpread[b & 0xFF] << 2 | kCpSpread[b >> 8] << 3;
    d[0] = pl[v >> 24]; d[1] = pl[(uint8)(v >> 16)]; d[2] = pl[(uint8)(v >> 8)]; d[3] = pl[(uint8)v];
    d += 4;
  }
}
#ifndef cp_conv
#define cp_conv cp_conv_c
#endif
#endif
static void cp_conv_n(uint8 *d, const uint16 *w, const uint16 *lut) {   /* (por cuadro) colores de a uno */
  for (uint16 r = 0; r < 8; r++) {
    uint16 a = w[r], b = w[8 + r];
    uint32 v = kCpSpread[a & 0xFF] | kCpSpread[a >> 8] << 1 | kCpSpread[b & 0xFF] << 2 | kCpSpread[b >> 8] << 3;
    for (uint16 k = 0; k < 4; k++, v <<= 8)
      *d++ = (uint8)(lut[(v >> 28) & 15] << 4 | lut[(v >> 24) & 15]);
  }
}
void md_corpse_upload(uint16 table) {
  if (!cp_on) return;
  const uint16 *c = zo->corpse;
  uint16 n = c[0] & 0x7FFF, es = cp_stream ? 5 : 4;
  const uint16 *e = c + 2 + 16 * c[1];
  uint8 *buf = cp_buf(cp_half);
  uint16 *tl = (uint16 *)(buf + cp_maxt * 32);
  uint16 slot_t = 0;
  if (cp_stream) {                               /* (por cuadro: el lugar de la entrada, si está y ya se subió) */
    uint8 md_published(void);
    if (md_published()) return;                  /* (un solo búfer: el vblank todavía lee el del cuadro anterior) */
    uint8 sc = frz_hc[cp_key & 127];
    if (!(sc < frz_ns && frz_id[sc] == cp_key && !frz_pend[sc])) return;   /* (al ponerla se marca todo: frz_get) */
    slot_t = frz_t0 + (sc << 3);
  }
  int16 cur = -1;
  uint16 k = 0;
  if (!cp_stream) {
    /* (las entradas van ordenadas por tabla, mkzobj: las de esta, por búsqueda binaria; el resto no se mira) */
    uint16 lo = 0, hi = n;
    while (lo < hi) { uint16 m = (lo + hi) >> 1; if (e[4 * m + 2] < table) lo = m + 1; else hi = m; }
    k = lo; e += 4 * lo;
  }
  for (; k < n; k++, e += es) {
    uint16 ram = e[es - 3], tab = e[es - 2], l = e[es - 1];
    if (tab != table) { if (!cp_stream) break; continue; }
    int16 i = cp_idx(ram);
    if (i < 0) continue;
    if (i != cur) {                              /* (las entradas del mismo tile del SNES van seguidas: mkzobj) */
      if (!(cp_dirty[i >> 3] & (1 << (i & 7)))) { cur = -2; continue; }
      if (cp_nl + (cp_stream ? 1 : 4) > cp_maxt) break;   /* (el resto, la próxima vez que se suba esta tabla) */
      cp_dirty[i >> 3] &= (uint8)~(1 << (i & 7));
      if (!cp_stream) {
        /* (marcado no es cambiado: el pudrirse marca columnas enteras y 3 de cada 4 tiles siguen iguales. Si la suma
           del tile es la de su última conversión, sus tiles de la Genesis ya están) */
        const uint32 *wl = (const uint32 *)(ram < 0x3000 ? &tilemap_stuff[(ram - 0x2000) >> 1] : ERAM_ARR(ram));
        uint32 s1 = 1, s2 = 0;
        for (uint16 q = 0; q < 8; q++) { s1 += wl[q]; s2 += s1; }
        s2 ^= s1 << 16 | s1 >> 16;
        uint32 *hs = (uint32 *)((uint8 *)md_boss_zone + CP_HASH_OFF) + i;
        uint8 hb = (uint8)(1 << (i & 7));
        if ((cp_hval[i >> 3] & hb) && *hs == s2) { cur = -2; continue; }
        *hs = s2; cp_hval[i >> 3] |= hb;
      }
      cur = i;
    } else if (cur == -2) continue;
    if (cp_nl >= cp_maxt) break;
    const uint16 *w = ram < 0x3000 ? &tilemap_stuff[(ram - 0x2000) >> 1] : ERAM_ARR(ram);
    if (cp_stream) {
      cp_conv_n(buf + 32 * cp_nl, w, c + 2 + 16 * (l < c[1] ? l : 0));
      tl[cp_nl++] = slot_t + e[1];
    } else {
      if (l >= CP_NLUT) l = 0;
      cp_conv(buf + 32 * cp_nl, w, (const uint8 *)md_boss_zone + CP_LUT_OFF + 256 * l);
#ifdef MD_CP_CHECK
      { uint8 t2[32]; cp_conv_asm(t2, w, (const uint8 *)md_boss_zone + CP_LUT_OFF + 256 * l);
        if (memcmp(t2, buf + 32 * cp_nl, 32)) (*(volatile uint16 *)0xFFF566)++; }
#endif
      tl[cp_nl++] = e[0];
#ifdef MD_CP_COUNT
      (*(volatile uint16 *)0xFFF564)++;
      { uint16 h = 0; for (uint16 q = 0; q < 16; q++) h = (uint16)((h << 1 | h >> 15) + w[q]);
        volatile uint16 *hs = (volatile uint16 *)0xFFF600;
        if (hs[i] == h) (*(volatile uint16 *)0xFFF568)++; hs[i] = h; }
#endif
    }
  }
}
void cp_all_dirty_(uint16 n) {                    /* (frz_get: la entrada del cadáver tomó un lugar: todo de nuevo) */
  if (cp_on && cp_stream && n == cp_key) memset(cp_dirty, 0xFF, sizeof cp_dirty);
}
void cp_room_(void) { cp_room(); }
void cp_take_(void);
static void cp_take(void) {                      /* (zebes_take) lo del cuadro pasa al vblank */
  cp_nv = cp_nl; cp_vhalf = cp_half; cp_half ^= 1; cp_nl = 0;
}
void cp_take_(void) { cp_take(); }
void zebes_corpse_vblank(void) {                 /* (md_vblank) */
  if (!cp_nv) return;
  const uint8 *buf = cp_buf(cp_vhalf);
  const uint16 *tl = (const uint16 *)(buf + cp_maxt * 32);
  for (uint16 i = 0; i < cp_nv; ) {
    uint16 j = i + 1;
    while (j < cp_nv && tl[j] == tl[i] + (j - i)) j++;
    vdp_dma_vram(buf + 32 * i, tl[i] * 32, (j - i) * 16);
    i = j;
  }
  cp_nv = 0;
}

void map_page_ext(uint8 p) { map_page(p); }
/* bancos del SNES que se leen poco (nmath.h, mkdata.py COLD_BANKS): en la página NT_COLD_PAGE; se pone un momento y
   vuelve la que tenía el registro (md_hw_page: la intro cambia de página sin md_cur_page) */
static inline uint8 cold_on(void) {
  uint8 hw = md_hw_page;
  __asm__ volatile("" ::: "memory");
  md_hw_page = NT_COLD_PAGE;
  *(volatile uint8 *)0xA130FF = NT_COLD_PAGE;
  __asm__ volatile("" ::: "memory");
  return hw;
}
static inline void cold_off(uint8 hw) {
  __asm__ volatile("" ::: "memory");
  md_hw_page = hw;
  *(volatile uint8 *)0xA130FF = hw;
  __asm__ volatile("" ::: "memory");
}
uint16 nt_paged_w(const uint8 *t, uint16 a) {
  uint8 hw = cold_on();
  const uint8 *q = t + (uint16)(a - 0x8000);
  uint8 lo = q[0], hi = q[1];
  cold_off(hw);
  return (uint16)(lo | hi << 8);
}
uint8 nt_paged_b(const uint8 *t, uint16 a) {
  uint8 hw = cold_on();
  uint8 v = t[(uint16)(a - 0x8000)];
  cold_off(hw);
  return v;
}
