/* MegaMetroid — la muerte de Samus en la Genesis (docs/64). Lo incluye md_engine.c al final (usa su lista de sprites,
 * la CRAM y la VRAM), como la pausa y el final.
 *
 * Hasta el apagón (estados 19-20) el juego se dibuja como siempre (la paleta se oscurece). Al terminar
 * (SamusDeathGfx, death.c: el SNES deja solo los sprites: TM = 0x10) se borran los planos, se esconde el HUD y se
 * cargan los tiles de los pedazos del traje (mkdeath.py: $9B:8000-93FF) donde estaban los de la sala; la CRAM queda:
 * el color 0 (el fondo: se pone blanco cuando el traje explota), la línea de Samus (palette_buffer 192-207) y la de
 * Samus sin traje (240-255). Samus se sigue dibujando con sus tiles de siempre (la DMA de Samus en md_vblank); los
 * spritemaps 2076-2093 (DrawSamusSuitExploding) salen de kDeathRecs. Termina con el menú de fin de juego (MenuGfx) o
 * con una sala nueva. */
#include "md_death_data.h"
#pragma GCC push_options
#pragma GCC optimize("Os")

uint8 dh_on;                                   /* la muerte: md_post pasa por md_death_frame */
static uint8 dh_load;                          /* falta preparar la VRAM (en md_death_frame, con el vblank libre) */

NT_LATE void SamusDeathGfx(void) { dh_on = 1; dh_load = 1; }
NT_LATE void SamusDeathTiles(uint16 i) { (void)i; }   /* (todos los tiles se cargan juntos: SamusDeathGfx) */

NT_LATE void dh_samus_spr_(uint16 idx, uint16 x, uint16 y) {   /* DrawSamusSpritemap de los pedazos */
  uint8 lp = md_cur_page;
  pz_map_page(DEATH_PAGE);
  put_recs(kDeathRecs + kDeathIdx[idx - DEATH_SM_FIRST], (int16)x, (int16)y, 0);
  pz_map_page(lp);
}

NT_LATE static void dh_set_cram(void) {
  for (uint16 i = 0; i < 256; i++) cram_rev[i] = 0xFF;
  for (int16 i = 63; i >= 0; i--) {
    uint8 l = i >> 4, k = i & 15;
    uint16 s = !i ? 0 : !k ? 0xFFFF : l == SAMUS_PAL ? 192 + k : l == DEATH_LINE ? 240 + k : 0xFFFF;   /* (255 es un índice) */
    cram_off[i] = s == 0xFFFF ? 0xFFFF : s * 2;
    cram_next[i] = 0xFF;
    if (s != 0xFFFF) { cram_next[i] = cram_rev[s]; cram_rev[s] = (uint8)i; }
  }
  nt_palette_lo = 0;
  nt_palette_hi = 255;
  nt_palette_changed = 1;
}

NT_LATE void md_death_frame(void) {            /* md_post desde el apagón de la muerte */
  pipe_take();
  nspr = (uint16)(sp_ptr - spr);
  md_frame_count++;
  md_zebes = 0; cine_stage = 0; rot_on = 0; cv_line = 0; hz_state = HZ_OFF;
  if (tint_on) { tint_on = 0; nt_palette_changed = 1; nt_palette_lo = 0; nt_palette_hi = 255; }
  md_tint = 0;
  if (dh_load) {
    dh_load = 0;
    dh_set_cram();
    uint16 sr;
    __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
    md_vdp_busy = 1;
    vdp_reg(0x12, 0x00);                       /* sin la ventana del HUD */
    vdp_reg(0x0B, 0x00);
    rot_shown = 0;
    vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
    vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
    uint8 lp = md_cur_page;
    pz_map_page(DEATH_PAGE);
    pz_dma(kDeathTiles, DEATH_BASE * 32, DEATH_TILES * 16);
    pz_map_page(lp);
    md_vdp_busy = 0;
    __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
  }
  if (nt_palette_changed) {                    /* colores: como en md_pause_frame */
    nt_palette_changed = 0;
    const uint16 *off = cram_off;
    uint16 *c = cram;
    for (uint16 i = 64; i; i--, c++) {
      uint16 o = *off++;
      uint16 v = o >= 0xFFFE ? 0 : kCram555[palette_buffer[o >> 1] & 0x7FFF];
      if (v != *c) { *c = v; cram_dirty = 1; }
    }
  }
  uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  if (cram_dirty) cram_out_ok = 0;
  scroll_ax = scroll_ay = scroll_bx = scroll_by = 0;
}
#pragma GCC pop_options
