/* MegaMetroid — el final (estado 39) en la Genesis: las escenas en modo 1 (docs/63). Lo incluye md_engine.c (usa su
 * lista de sprites, la CRAM y el scroll), como la pausa (md_pause.c).
 *
 * El motor (ending.c, lo traducido del banco $8B) deja todo como el SNES: el texto en 7E:3000
 * (cinematic_text_tilemap), que el SNES sube a la VRAM con EndingTextUpload (los créditos) o con
 * UpdateCinematicBgTilemap en cada cuadro (la pantalla final), los registros de la PPU y los sprites
 * (DrawCinematicSpritemap, las estrellas: EndingStar). Lo que el SNES tiene en la VRAM en cada grupo de escenas está
 * preparado en el PC (tools/conv/mkending.py, md_ending_data.h):
 *   plano A = BG1: 64x64 celdas; las columnas 0-31 son la pantalla del SNES (filas 0-31 la de arriba, 32-63 la de
 *             abajo o la misma otra vez si el BG1 es de 32x32: así el scroll vertical da la vuelta igual). Las
 *             columnas 32-63 quedan vacías: una capa apagada (TM) se esconde corriendo el scroll 256.
 *   plano B = BG2: la pantalla fija del grupo (Samus, el símbolo), igual.
 * Las escenas en modo 7 (las nubes, Zebes, el casco, el símbolo que gira) y las del espacio todavía no se dibujan:
 * se ve negro. */
#include "md_ending_data.h"

uint8 ed_on;                                   /* el final: md_post pasa por md_ending_frame */
static uint8 ed_scene;                         /* el último EndingGfx (1-3: escenas todavía sin dibujo; 4-5: créditos) */
static const EdGroup *ed_g;                    /* grupo cargado en la VRAM */
static uint8 ed_loaded;                        /* (el primer cuadro: los planos del juego se borran) */
static uint16 ed_a_scr[2] = { 0xFFFF, 0xFFFF };  /* pantalla del SNES en cada mitad del plano A */
#define ed_sum (md_scratch + ED_ROWS * 32)     /* suma de las palabras del SNES de cada fila del plano A ya escrita */
static uint32 ed_valid[2];                     /* fila del plano A ya escrita (bit por fila) */
static uint16 ed_up_addr = 0xFFFF;             /* EndingTextUpload: la dirección del SNES (se toma en el cuadro) */
static uint8 ed_up;
#define ED_ROWS 16                            /* filas convertidas por cuadro (como mucho; las demás, después) */
static uint8 ed_rn;
#define ed_cells md_scratch                    /* celdas de las filas a escribir: ED_ROWS * 32 */
#define ed_rrow ((uint8 *)(md_scratch + ED_ROWS * 32 + 64))   /* filas del plano de cada una (dos: 0xFF ninguna) */
_Static_assert(ED_ROWS * 32 + 64 + ED_ROWS <= sizeof md_scratch / 2, "md_scratch");

NT_LATE static uint16 ed_lookup(const uint16 *lt, const uint8 *lp, const uint16 *lv, uint16 w) {
  uint16 k = lt[w & 0x3FF];
  if (k != 0xFFFF) {
    uint8 p = (w >> 10) & 7;
    for (; lp[k] != 0xFF; k++)
      if (lp[k] == p) return (lv[k] ^ ((w >> 3) & 0x1800)) | ((w << 2) & 0x8000);
  }
  return 0;
}

NT_LATE void EndingGfx(uint16 scene) {
  md_sync();
  ed_on = 1;
  ed_scene = (uint8)scene;
}
NT_LATE void EndingTextUpload(uint16 vram) { ed_up_addr = vram; ed_up = 1; }
NT_LATE void EndingM7Queue(uint16 a, uint16 b, uint16 c) { (void)a; (void)b; (void)c; }

NT_LATE static void ed_spr(uint16 sm, uint16 x, uint16 y, uint16 chr) {   /* DrawCinematicSpritemap en el final */
  if (!ed_g) return;
  uint8 lp = md_cur_page;                      /* las tablas están en la página ED_PAGE_N (ventana 7) */
  pz_map_page(ED_PAGE_N);
  const uint16 *k = ed_g->keys;
  int lo = 0, hi = ed_g->nkeys - 1, found = 0;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    const uint16 *e = k + m * 3;
    int c = e[0] != sm ? (e[0] < sm ? -1 : 1) : e[1] != chr ? (e[1] < chr ? -1 : 1) : 0;
    if (!c) { put_recs(ed_g->recs + e[2], (int16)x, (int16)y, 0); found = 1; break; }
    if (c < 0) lo = m + 1; else hi = m - 1;
  }
  pz_map_page(lp);
  if (!found) missing(9, sm, chr, 0xC2FF);
}

NT_LATE void EndingStar(uint16 x, uint16 y, uint16 chr) {
  if (!ed_g || (uint16)(chr - 0x09F0) > 3 || sp_ptr >= spr + MAX_SPR) return;
  GenSpr *s = sp_ptr++;
  s->y = y + 129;
  s->size_link = (uint16)(s - spr + 1);
  s->attr = ed_g->star[chr - 0x09F0];
  s->x = x + 128;
}

/* el grupo que corresponde a lo que muestra el motor: en modo 7 o antes de los créditos, ninguno (negro) */
NT_LATE static const EdGroup *ed_want(void) {
  if (ed_scene < 4) return 0;
  if (reg_BGMODE == 7) return reg_OBSEL == 0 ? &kEd_d2 : &kEd_d2b;   /* el símbolo: por ahora solo sus sprites */
  return reg_BG1SC == 0x4E ? &kEd_d3 : &kEd_d1;
}

NT_LATE static void ed_load(const EdGroup *g) {   /* (sin interrupciones) tiles, planos vacíos, el BG2 fijo */
  vdp_display(0);
  vdp_reg(0x10, 0x11);                         /* planos de 64x64 */
  vdp_reg(0x12, 0x00);                         /* sin la ventana del HUD */
  vdp_reg(0x0B, 0x00);                         /* scroll de la pantalla entera */
  rot_shown = 0;
  vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
  vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
  if (g) {
    pz_dma(g->tiles, ED_TILE_BASE * 32, g->ntiles * 16);   /* (la página ED_PAGE_N ya está: md_ending_frame) */
    for (uint16 r = 0; r < 32; r++) {          /* el BG2 fijo, dos veces (filas r y r + 32) */
      uint16 *o = ed_cells;
      for (uint16 c = 0; c < 32; c++) {
        uint16 v = ed_lookup(g->lutTb, g->lutPb, g->lutVb, g->bg2[r * 32 + c]);
        o[c] = (v & 0x7FF) != ED_TILE_BASE ? v | 0x8000 : v;   /* (prioridad alta: tapa los sprites de la secundaria) */
      }
      pz_dma(o, MD_VRAM_PLANE_B + r * 128, 32);
      pz_dma(o, MD_VRAM_PLANE_B + (r + 32) * 128, 32);
    }
  }
  vdp_display(1);
  ed_valid[0] = ed_valid[1] = 0;
  ed_a_scr[0] = ed_a_scr[1] = 0xFFFF;
}

/* filas de 7E:3000 subidas a la pantalla addr del SNES: las que cambiaron pasan a celdas (hasta ED_ROWS) */
NT_LATE static uint8 ed_rows(uint16 addr, uint16 nrows) {
  uint8 top = ed_a_scr[0] == addr, bot = ed_a_scr[1] == addr;
  if (!top && !bot) return 0;
  /* el slogan (docs/63): "GENESIS DOES..." en las filas 16-17 de la pantalla final desde que se ve el porcentaje
     (el "%" de CalcItemPercentageCount, 7E:3000 + 2 * 465) */
  uint8 slog = ed_g->slogan && addr == 0x4C00 && cinematic_text_tilemap[14 * 32 + 17] == 0x386A;
  for (uint16 r = 0; r < nrows; r++) {
    const uint16 *w = &cinematic_text_tilemap[r * 32];
    uint16 sum = 0;
    for (uint16 c = 0; c < 32; c++) sum = (uint16)((sum << 1 | sum >> 15) + w[c]);
    if (slog && (r == 16 || r == 17)) { w = ed_g->slogan + (r - 16) * 32; sum ^= 0x5A5A; }
    uint16 h = top ? r : r + 32;
    if ((ed_valid[h >> 5] >> (h & 31) & 1) && ed_sum[h] == sum) continue;
    if (ed_rn >= ED_ROWS) return 1;            /* (quedan: el próximo cuadro) */
    uint16 *o = ed_cells + ed_rn * 32;
    for (uint16 c = 0; c < 32; c++) o[c] = ed_lookup(ed_g->lutTa, ed_g->lutPa, ed_g->lutVa, w[c]);
    uint8 n = 0;
    if (top) { ed_rrow[ed_rn * 2 + n++] = (uint8)r; ed_sum[r] = sum; ed_valid[0] |= 1UL << r; }
    if (bot) { ed_rrow[ed_rn * 2 + n++] = (uint8)(r + 32); ed_sum[r + 32] = sum; ed_valid[1] |= 1UL << r; }
    if (n < 2) ed_rrow[ed_rn * 2 + 1] = 0xFF;
    ed_rn++;
  }
  return 0;
}

NT_LATE void md_ending_frame(void) {          /* md_post en el estado 39 */
  pipe_take();
  uint8 lp = md_cur_page;                      /* las tablas del grupo: página ED_PAGE_N en la ventana 7 */
  pz_map_page(ED_PAGE_N);
  nspr = (reg_TM | reg_TS) & 0x10 ? (uint16)(sp_ptr - spr) : 0;   /* sprites apagados (TM/TS): el SNES no los muestra */
  /* sprites en la pantalla principal (TM): encima de todo. Solo en la secundaria (TS, sumados por el color math: Samus
     sin traje que aparece sobre la del traje, las estrellas de la pantalla final): detrás del BG2 (sus celdas van con
     prioridad alta) */
  if (reg_TM & 0x10)
    for (GenSpr *q = spr; q < sp_ptr; q++) q->attr |= 0x8000;
  md_frame_count++;
  rot_on = 0; cv_line = 0; hz_state = HZ_OFF; md_zebes = 0; cine_stage = 0;
  if (tint_on) tint_on = 0;
  md_tint = 0;
  /* colores: los del grupo, de palette_buffer en cada cuadro */
  const uint8 *src = ed_g ? ed_g->src : 0;
  for (uint16 i = 0; i < 64; i++) {
    uint16 v = 0;
    if (src && src[i] != 0xFF) v = kCram555[palette_buffer[src[i]] & 0x7FFF];
    else if (!src && !i) v = kCram555[palette_buffer[0] & 0x7FFF];
    if (v != cram[i]) { cram[i] = v; cram_dirty = 1; }
  }
  uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  if (cram_dirty) cram_out_ok = 0;
  const EdGroup *want = ed_want();
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  md_vdp_busy = 1;
  if (want != ed_g || !ed_loaded) {
    ed_load(want);
    ed_g = want;
    ed_loaded = 1;
  }
  ed_rn = 0;
  if (ed_g) {
    /* pantallas del BG1: la de arriba y la de abajo (32x64) o la misma dos veces (32x32) */
    uint16 base = (uint16)(reg_BG1SC & 0xFC) << 8;
    uint16 s0 = base, s1 = (reg_BG1SC & 2) ? base + 0x400 : base;
    if (s0 != ed_a_scr[0] || s1 != ed_a_scr[1]) {
      vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
      ed_a_scr[0] = s0; ed_a_scr[1] = s1;
      ed_valid[0] = ed_valid[1] = 0;
    }
    uint8 more = 0;
    if (ed_up) more |= ed_rows(ed_up_addr, 32);
    if ((cinematic_enable_objs & 0x8000) && (cinematic_enable_bg_tilemap & 0x8000))
      more |= ed_rows(cinematicbg_var3, 30);
    ed_up = more;                               /* (lo que no entró sigue pendiente) */
    for (uint16 i = 0; i < ed_rn; i++)
      for (uint16 j = 0; j < 2; j++)
        if (ed_rrow[i * 2 + j] != 0xFF) pz_dma(ed_cells + i * 32, MD_VRAM_PLANE_A + ed_rrow[i * 2 + j] * 128, 32);
  }
  md_vdp_busy = 0;
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
  /* scroll: una capa apagada (TM) mira las columnas vacías */
  uint8 showa = ed_g && (reg_TM & 1), showb = ed_g && (reg_TM & 2) && (reg_BG2SC & 0xFC) == ed_g->bg2_scr >> 8;
  scroll_ax = (reg_BG1HOFS & 255) + (showa ? 0 : 256);
  scroll_ay = reg_BG1VOFS & 511;
  scroll_bx = (reg_BG2HOFS & 255) + (showb ? 0 : 256);
  scroll_by = reg_BG2VOFS & 511;
  pz_map_page(lp);
}
