/* MegaMetroid — capa de la Genesis: el rayo arco iris de Mother Brain (docs/59).
 *
 * Lo que hace el SNES (decomp):
 *  - SpawnMotherBrainRainbowBeamHdma ($88:E748) crea el HDMA $88:E74C: DMAP 0x41 (indirecto, dos registros), BBAD 0x26
 *    (WH0 y WH1: los bordes izquierdo y derecho de la ventana 1), tabla indirecta en 7E:9C00
 *    (mother_brain_indirect_hdma) que apunta a hdma_table_2 (7E:9D00). Su código ($A9:9466, MotherBrain_CalcHdma y las
 *    funciones por dirección) llena las dos tablas cada cuadro: dentro de la ventana, de la cabeza hacia el borde.
 *  - LayerBlendFunc_24 (fx_layer_blending_config_c = 36): dentro de la ventana se suma el color fijo COLDATA a BG1,
 *    BG2, sprites y fondo. COLDATA cambia cada cuadro (la tabla $88:E833: los colores del arco iris).
 *
 * En la Genesis no hay ventanas ni sumas de color. Con sprites no alcanza (Mother Brain y la pelea usan ~48 de los 64
 * del modo H32 y el rayo pide ~40): el rayo va en el plano B (el BG2 del SNES: el cuerpo y el fondo de tubos), con
 * prioridad alta (delante del BG1, como el tinte), en celdas del color 14 de la línea 3 (en su sala no lo usa ningún
 * enemigo: mkzobj.RAINBOW_ROOMS; cada cuadro lleva el COLDATA del SNES). Las celdas que la ventana cubre enteras
 * llevan un tile lleno; las del borde, tiles armados en el cuadro. El vblank repone lo que había debajo (lo lee de
 * la VRAM antes de pisarlo) y escribe las celdas nuevas, por tramos de fila; las filas que el cuerpo reescribió en
 * ese vblank (bb_vblank, md_zebes.c) ya están al día.
 */
#pragma GCC optimize("O2")
#include <string.h>
#include "game.h"
#include "md.h"
#include "md_vdp.h"
#include "md_data.h"

typedef struct { int16 y; uint16 size_link; uint16 attr; int16 x; } GenSpr;
extern GenSpr *sp_ptr, *spr, *spr_end;
extern uint16 zobj_col[16];
uint8 zebes_cone_slot(uint16 c);                /* md_zebes.c: el color c de la línea 3 no es de ningún enemigo */
int16 md_vb_lines_left(void);

#define RB_SLOT 14
#define RB_FULL 2007                            /* el tile lleno (después de los 87 del cuerpo, desde 1920) */
#define RB_DYN_A 2008                           /* armados: 35 aquí (hasta el 2042; después, el gancho) */
#define RB_DYN_AN 35
#define RB_DYN_B 1802                           /* y 13 en las filas del plano Window que no se ven */
#define RB_DYN_BN 13
#define RB_DYN_MAX (RB_DYN_AN + RB_DYN_BN)
#define RB_TOP 32                               /* primera línea de juego de la Genesis (debajo del HUD) */
#define RB_ROWS 26
#define RB_SAVE 176

NT_GAMEHI static uint32 rb_tiles[RB_DYN_MAX][8];   /* tiles armados de este cuadro (1,5 KB) */
static uint8 rb_ntiles, rb_full_ok, rb_on, rb_dirty;
static uint16 rb_need;                          /* líneas de vblank que necesita la vuelta (si no hay, el siguiente) */
static uint16 rb_sum;                           /* la ventana y el scroll del último armado (si no cambian, nada) */
NT_GAMEHI static uint8 rb_l[224], rb_r[224];    /* ventana por línea de la Genesis (l > r: vacía) */
uint32 md_bb_rows_done;                         /* md_zebes.c (bb_vblank): filas del BG2 que el cuerpo reescribió */
/* filas del plano B con rayo: fila, primera columna, celdas, de borde a la izquierda y a la derecha, primer tile
   armado (0xFF: todas llenas). En la RAM libre de esa sala: cinematic_text_tilemap desde el byte 2528 hasta el 3199
   (el mapa del cuerpo 0-2047, las tablas de los objetos por cuadro desde 2048, las sumas de las filas desde 2400; el
   ácido usa 3200: md_zebes.c) */
typedef struct { uint8 k, c0, n, pl, pr, d0; } RbRow;
typedef struct { RbRow nw[RB_ROWS], pv[RB_ROWS]; uint8 nnw, npv; uint16 save[RB_SAVE]; } RbRam;
extern uint16 cinematic_text_tilemap[];
#define RB_RAM ((RbRam *)((uint8 *)cinematic_text_tilemap + 2528))
_Static_assert(sizeof(RbRam) <= 3200 - 2528, "md_rbeam: RbRam");

/* el HDMA indirecto del SNES: la tabla de 7E:9C00 (cuántas líneas, dirección de los datos) y dos bytes por línea */
static uint8 rb_byte(uint16 a) {
  uint16 o = (uint16)(a - 0x9D00);
  if (o < 480) { uint16 w = hdma_table_2[o >> 1]; return (o & 1) ? (uint8)(w >> 8) : (uint8)w; }
  return 0xFF;                                  /* (fuera de la tabla: vacía) */
}
static void rb_window(void) {
  memset(rb_l, 0xFF, sizeof rb_l);
  memset(rb_r, 0, sizeof rb_r);
  uint16 line = 0, p = 0;                       /* línea del SNES, lugar en la tabla indirecta */
  while (line < 222 && p + 3 <= sizeof mother_brain_indirect_hdma) {
    uint8 c = mother_brain_indirect_hdma[p];
    if (!c) break;
    uint16 a = mother_brain_indirect_hdma[p + 1] | mother_brain_indirect_hdma[p + 2] << 8;
    p += 3;
    uint16 n = c & 0x7F, rep = c & 0x80;
    if (!n) n = 128;
    for (uint16 i = 0; i < n && line < 222; i++, line++) {
      uint16 g = line + 2;                      /* (la imagen de la Genesis va 2 líneas más abajo) */
      rb_l[g] = rb_byte(a); rb_r[g] = rb_byte(a + 1);
      if (rb_l[g] == 255) rb_r[g] = 0;          /* (255-255: un píxel en el borde, la tabla de las líneas sin rayo) */
      if (rep) a += 2;
    }
  }
}

/* el tile de la celda que empieza en x0 (Genesis) con sus líneas desde g0: color 14 donde está la ventana */
static void rb_tile(uint32 *t, int16 x0, int16 g0) {
  for (int16 j = 0; j < 8; j++) {
    int16 g = g0 + j;
    uint32 row = 0;
    if (g >= RB_TOP && g < 224 && rb_l[g] <= rb_r[g]) {
      int16 a = rb_l[g] - x0, b = rb_r[g] - x0;
      if (a < 0) a = 0;
      if (b > 7) b = 7;
      if (a <= b) row = (0xFFFFFFFFu >> (4 * a)) & (0xFFFFFFFFu << (4 * (7 - b))) & 0xEEEEEEEEu;
    }
    t[j] = row;
  }
}
static uint8 rb_full(int16 x0, int16 g0) {      /* la ventana cubre la celda entera */
  if (x0 < 0 || x0 > 248) return 0;
  for (int16 j = 0; j < 8; j++) {
    int16 g = g0 + j;
    if (g < RB_TOP || g >= 224 || rb_l[g] > x0 || rb_r[g] < x0 + 7 || rb_l[g] > rb_r[g]) return 0;
  }
  return 1;
}

/* md_post: las filas del plano B con el rayo (y sus tiles armados) para el vblank */
uint16 md_rbeam_post(void) {
  if (room_ptr != 0xDD58) { rb_on = 0; return 0; }
  RbRam *R = RB_RAM;
  uint8 live = fx_layer_blending_config_c == 36 && zebes_cone_slot(RB_SLOT);
  if (live) {
    live = 0;
    for (uint16 i = 0; i < 6; i++)
      if (hdma_object_channels_bitmask[i] && hdma_object_pre_instructions[i] == 0xE7BC) live = 1;
  }
  uint16 bx = reg_BG2HOFS, by = reg_BG2VOFS;
  if (!live) {
    rb_on = 0;
    if (R->npv && !rb_dirty) { R->nnw = 0; rb_dirty = 1; rb_sum = 0xFFFF; rb_need = 6; }   /* (quitarlo: reponer lo de debajo) */
    return 0;
  }
  rb_on = 1;
  if (rb_dirty) return 0;                       /* (el vblank todavía no tomó el anterior) */
  rb_window();
  uint16 sum = bx * 7 + by * 13;
  for (uint16 g = RB_TOP; g < 224; g++) sum = (uint16)(sum * 3 + rb_l[g] + (rb_r[g] << 5));
  if (sum == rb_sum) return 0;                  /* (lo mismo que se ve) */
  rb_sum = sum;
  uint16 nt = 0, ncell = 0, nr = 0;
  int16 kb = (int16)((RB_TOP + by) >> 3), ke = (int16)((223 + by) >> 3);
  for (int16 k = kb; k <= ke && nr < RB_ROWS; k++) {
    int16 g0 = (int16)(k * 8 - by);             /* primera línea de la Genesis de la fila (puede ser < RB_TOP) */
    int16 lo = 256, hi = -1;
    for (int16 j = 0; j < 8; j++) {
      int16 g = g0 + j;
      if (g < RB_TOP || g >= 224 || rb_l[g] > rb_r[g]) continue;
      if (rb_l[g] < lo) lo = rb_l[g];
      if (rb_r[g] > hi) hi = rb_r[g];
    }
    if (hi < 0) continue;
    int16 c0 = (int16)((lo + bx) >> 3), c1 = (int16)((hi + bx) >> 3);
    int16 n = c1 - c0 + 1;
    if (ncell + n > RB_SAVE) break;
    int16 pl = 0, pr = 0;
    while (pl < n && !rb_full((int16)((c0 + pl) * 8 - bx), g0)) pl++;
    while (pr < n - pl && !rb_full((int16)((c1 - pr) * 8 - bx), g0)) pr++;
    RbRow *w = &R->nw[nr++];
    w->k = (uint8)(k & 63); w->c0 = (uint8)(c0 & 63); w->n = (uint8)n; w->pl = (uint8)pl; w->pr = (uint8)pr;
    if (nt + pl + pr <= RB_DYN_MAX) {
      w->d0 = (uint8)nt;
      for (int16 i = 0; i < pl; i++) rb_tile(rb_tiles[nt++], (int16)((c0 + i) * 8 - bx), g0);
      for (int16 i = 0; i < pr; i++) rb_tile(rb_tiles[nt++], (int16)((c1 - pr + 1 + i) * 8 - bx), g0);
    } else w->d0 = 0xFF;                        /* (sin lugar: llenas) */
    ncell += n;
  }
  R->nnw = (uint8)nr;
  rb_ntiles = (uint8)nt;
  /* (~12 ciclos por celda escrita o leída, 3 por celda; ~200 bytes de DMA por línea; 488 ciclos por línea) */
  uint16 cprev = 0;
  for (uint16 r = 0; r < R->npv; r++) cprev += R->pv[r].n;
  rb_need = (uint16)(3 + ((cprev + 2 * ncell) * 12 + 2 * nr * 40) / 488 + (nt * 32) / 200);
  rb_dirty = 1;
  return 0;
}

/* md_post, después de los colores de los objetos: el color del rayo (COLDATA, 5 bits por canal) */
void md_rbeam_color(void) {
  if (!rb_on) return;
  uint16 col = (uint16)((reg_COLDATA[2] & 31) << 10 | (reg_COLDATA[1] & 31) << 5 | (reg_COLDATA[0] & 31));
  zobj_col[RB_SLOT - 1] = kCram555[col];
}

static inline uint16 rb_tile_of(uint16 d) { return d < RB_DYN_AN ? RB_DYN_A + d : RB_DYN_B + (d - RB_DYN_AN); }
static inline void rb_addr_w(uint16 k, uint16 c) { VDP_CTRL32 = VDP_CMD_VRAM_W(MD_VRAM_PLANE_B + k * 128 + c * 2); }
static inline void rb_addr_r(uint16 k, uint16 c) {
  uint16 a = MD_VRAM_PLANE_B + k * 128 + c * 2;
  VDP_CTRL32 = ((uint32)(a & 0x3FFF) << 16) | (a >> 14);
}

/* vblank (después del de Zebes: el cuerpo ya escribió sus filas): el tile lleno (una vez por sala), reponer las
   celdas de antes, guardar las de ahora, los tiles armados y las celdas del rayo */
void md_rbeam_vblank(void) {
  if (room_ptr != 0xDD58) { md_bb_rows_done = 0; return; }
  if (rb_on && !rb_full_ok) {
    vdp_set_vram_write(RB_FULL * 32);
    for (uint16 i = 0; i < 16; i++) VDP_DATA = 0xEEEE;
    rb_full_ok = 1;
  }
  RbRam *R = RB_RAM;
  uint32 bbd = md_bb_rows_done;
  if (!rb_dirty) {
    /* el cuerpo reescribió una fila con rayo: se rearma (lo guardado de esa fila ya no vale: se marca hasta la
       vuelta siguiente) */
    for (uint16 r = 0; r < R->npv && bbd; r++)
      if ((bbd >> (R->pv[r].k & 31)) & 1) { rb_sum = 0xFFFF; break; }
    return;
  }
  if (md_vb_lines_left() < (int16)rb_need) return;
  rb_dirty = 0;
  md_bb_rows_done = 0;
  uint16 *sv = R->save;
  for (uint16 r = 0; r < R->npv; r++) {         /* reponer */
    const RbRow *p = &R->pv[r];
    uint8 fresh = (bbd >> (p->k & 31)) & 1;     /* (el cuerpo reescribió las columnas 0-31 de esa fila) */
    uint16 c = p->c0;
    rb_addr_w(p->k, c);
    for (uint16 j = 0; j < p->n; j++, c = (c + 1) & 63, sv++) {
      if (c == 0 && j) rb_addr_w(p->k, 0);
      if (fresh && c < 32) { rb_addr_w(p->k, (c + 1) & 63); continue; }
      VDP_DATA = *sv;
    }
  }
  sv = R->save;
  for (uint16 r = 0; r < R->nnw; r++) {         /* guardar */
    const RbRow *p = &R->nw[r];
    uint16 c = p->c0;
    rb_addr_r(p->k, c);
    for (uint16 j = 0; j < p->n; j++, c = (c + 1) & 63) {
      if (c == 0 && j) rb_addr_r(p->k, 0);
      *sv++ = VDP_DATA;
    }
  }
  uint16 n = rb_ntiles, na = n < RB_DYN_AN ? n : RB_DYN_AN;
  if (na) vdp_dma_vram(rb_tiles, RB_DYN_A * 32, na * 16);
  if (n > na) vdp_dma_vram(rb_tiles[RB_DYN_AN], RB_DYN_B * 32, (n - na) * 16);
  for (uint16 r = 0; r < R->nnw; r++) {         /* el rayo */
    const RbRow *p = &R->nw[r];
    uint16 c = p->c0;
    rb_addr_w(p->k, c);
    for (uint16 j = 0; j < p->n; j++, c = (c + 1) & 63) {
      if (c == 0 && j) rb_addr_w(p->k, 0);
      uint16 t = RB_FULL;
      if (p->d0 != 0xFF) {
        if (j < p->pl) t = rb_tile_of(p->d0 + j);
        else if (j >= p->n - p->pr) t = rb_tile_of(p->d0 + p->pl + (j - (p->n - p->pr)));
      }
      VDP_DATA = 0x8000 | (3 << 13) | t;
    }
  }
  memcpy(R->pv, R->nw, sizeof R->nw);
  R->npv = R->nnw;
}

/* la VRAM se rehace al cambiar de sala (y al volver de la pausa): no hay nada que reponer */
void md_rbeam_room(void) {
  rb_full_ok = 0; rb_on = 0; rb_dirty = 0; rb_sum = 0xFFFF;
  if (room_ptr == 0xDD58) { RB_RAM->npv = 0; RB_RAM->nnw = 0; }
}
