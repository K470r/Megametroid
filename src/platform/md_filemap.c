/* MegaMetroid — el mapa de la partida en la Genesis (estado 5, docs/67). Lo incluye md_engine.c al final (usa su lista
 * de sprites, la CRAM y lo de la pausa), como la muerte y el final.
 *
 * filemap.c deja todo como el SNES; esto lo dibuja:
 *   - el planeta (menu_index 3-8 y 17-22): la imagen del área ya sumada (BG1 + BG3, mkfilemap.py) en el plano A, con
 *     las ventanas del SNES: el cuadrado (la tabla del HDMA de 7E:9E00 y expand_sq_left/right_pos) y W12SEL/TM/TMW
 *     deciden, por celda de 8x8, si el BG1 se ve. Lo que el SNES muestra adentro del cuadrado en los pasos 7-8 y 20-21
 *     (el marco del mapa del área, BG2) aquí queda negro: sus colores son los de la pausa y no caben con los del planeta;
 *   - "PLANET ZEBES" y los nombres de las áreas: sprites de kFmSprRecs (fm_spr), tapados por la misma ventana;
 *   - el mapa del área (9-14): la pausa (md_pause.c, pz_fsel: sin HUD y con el pie del marco).
 * El código y los datos van en la página FM_PAGE (lo común de la ROM está lleno): md_filemap_frame_ y FileMapGfx la
 * ponen en la ventana 7 y la sacan. */
#include "md_fmap_data.h"
#pragma GCC push_options
#pragma GCC optimize("Os")
#ifdef NT_SSF2
#define FM_CODE __attribute__((section(".bank23"), noinline))
_Static_assert(FM_PAGE == 23, "FM_CODE: la sección de la página");
#else
#define FM_CODE NT_LATE
#endif

uint8 fm_on;                                    /* estado 5: md_post pasa por md_filemap_frame */
static uint8 fm_room;                           /* el mapa del área: la pausa (pz_on) */
static uint8 fm_load;                           /* falta preparar la VRAM (tiles del planeta y de los sprites) */
static int8 fm_img = -1;                        /* la imagen del planeta en la VRAM */
static int8 fm_want;                            /* la que se quiere */
static uint16 fm_key[6] = { 0xFFFF };           /* lo que se dibujó en el plano A (para no repetir) */
static const uint16 kFmAreas[6] = { 0, 3, 5, 1, 4, 2 };   /* kFileSelectMap_AreaIndexes */
void FileSelectMapFrame(void);                  /* filemap.c (página FM_PAGE) */

NT_LATE static void fm_page_on(uint8 *lp) { *lp = md_cur_page; pz_map_page(FM_PAGE); }

/* --------------------------------------------------------------- la ventana 1 del SNES */
typedef struct { int16 top, bot, left, right; } FmRect;
FM_CODE static uint8 fm_wb(uint16 o) {          /* byte de 7E:9E00 + o */
  uint16 v = hdma_table_2[128 + (o >> 1)];
  return (o & 1) ? (uint8)(v >> 8) : (uint8)v;
}
FM_CODE static FmRect fm_rect(void) {           /* las líneas de la tabla izquierda que apuntan a 7E:9E32 (el borde) */
  FmRect r = { 0, 0, (uint8)expand_sq_left_pos, (uint8)expand_sq_right_pos };
  uint16 line = 0, o = 0;
  uint8 in = 0;
  r.top = r.bot = -1;
  for (uint16 k = 0; k < 8; k++, o += 3) {
    uint8 n = fm_wb(o);
    if (!n) break;
    uint16 cnt = (n & 0x80) ? (n & 0x7F) : n;
    uint16 p = fm_wb(o + 1) | fm_wb(o + 2) << 8;
    if (p == 0x9E32) { if (!in) { r.top = line; in = 1; } r.bot = line + cnt; }
    line += cnt;
  }
  return r;
}
/* ¿la capa de la máscara m (W12SEL: bits 1 = activa, 0 = invertida; TMW: bit t) se ve en (x, y)? */
FM_CODE static uint8 fm_shown(const FmRect *r, uint8 wsel, uint8 tbit, int16 x, int16 y) {
  if (!(reg_TM & tbit)) return 0;
  if (!(reg_TMW & tbit) || !(wsel & 2)) return 1;
  uint8 inside = y >= r->top && y < r->bot && x >= r->left && x <= r->right;
  return !(inside ^ (wsel & 1));
}

/* --------------------------------------------------------------- sprites */
FM_CODE void fm_spr(uint16 idx, uint16 x, uint16 y, uint16 chr) {   /* DrawMenuSpritemap en el planeta */
  uint16 i = idx - FM_SM0;
  if (i >= 7 || fm_room) return;
  FmRect r = fm_rect();
  if (!fm_shown(&r, reg_WOBJSEL, 0x10, (int16)x, (int16)y)) return;   /* (por el punto del nombre) */
  put_recs(kFmSprRecs + kFmSprIdx[i + ((chr & 0x200) ? 7 : 0)], (int16)x, (int16)y, 0);
}

/* --------------------------------------------------------------- lo que pide filemap.c */
FM_CODE static void FileMapGfx_(uint16 what) {
  md_sync();                                   /* el vblank toma antes el cuadro anterior */
  if (!fm_on) { fm_on = 1; fm_load = 1; fm_img = -1; }
  switch (what) {
  case 0:                                      /* FMG_BLACK: el BG1 vacío (al entrar y al volver del mapa del área) */
    if (fm_room) { fm_room = 0; pz_on = 0; pz_fsel = 0; fm_load = 1; fm_img = -1; }
    break;
  case 1:                                      /* FMG_PLANET: el BG3 del área (LoadAreaSelectBackgroundTilemap) */
    fm_want = (int8)kFmAreas[file_select_map_area_index < 6 ? file_select_map_area_index : 0];
    break;
  case 2:                                      /* FMG_ROOMMAP: el mapa del área (DrawRoomSelectMap): la pausa */
    fm_room = 1;
    pz_fsel = 1;
    PauseGfx(PAUSE_GFX_ENTER);
    PauseGfx(PAUSE_GFX_MAP);
    break;
  }
  fm_key[0] = 0xFFFF;
}
NT_LATE void FileMapGfx(uint16 what) {         /* (filemap.c ya corre con la página puesta; por las dudas) */
  uint8 lp;
  fm_page_on(&lp);
  FileMapGfx_(what);
  pz_map_page(lp);
}

/* --------------------------------------------------------------- el cuadro */
FM_CODE static void fm_load_vram(void) {       /* (vblank libre, sin interrupciones) */
  room_pending = 0;                            /* (la sala del estado inicial que md_init dejó para el vblank: no) */
  vdp_reg(0x12, 0x00);                         /* sin la ventana del HUD */
  vdp_reg(0x0B, 0x00);
  vdp_reg(0x10, 0x01);                         /* planos de 64x32 */
  rot_shown = 0;
  vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
  vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
  pz_dma(kFmSprTiles, FM_SPR_BASE * 32, FM_SPR_TILES * 2 * 16);
}

FM_CODE static void fm_plane_a(void) {         /* el planeta por celdas, con la ventana */
  uint16 mi = menu_index & 0xFF;
  uint8 planet = fm_img >= 0 && ((mi >= 3 && mi <= 8) || mi >= 17);
  FmRect r = fm_rect();
  uint16 key[6] = { planet ? (uint16)(fm_img + 1) : 0, (uint16)r.top << 8 | (uint8)r.bot,
                    (uint16)r.left << 8 | (uint8)r.right, reg_TM | reg_TMW << 8, reg_W12SEL, mi };
  if (!planet) key[1] = key[2] = key[3] = key[4] = key[5] = 0;
  uint8 same = 1;
  for (uint16 i = 0; i < 6; i++) if (key[i] != fm_key[i]) same = 0;
  if (same) return;
  for (uint16 i = 0; i < 6; i++) fm_key[i] = key[i];
  const uint16 *mp = planet ? kFmPlanet[fm_img].map : 0;
  for (uint16 cy = 0; cy < 28; cy++) {
    vdp_set_vram_write(MD_VRAM_PLANE_A + cy * 128);
    for (uint16 cx = 0; cx < 32; cx++) {
      uint16 c = 0;
      if (planet && fm_shown(&r, reg_W12SEL, 1, cx * 8 + 4, cy * 8 + 4)) c = mp[cy * 32 + cx];
      VDP_DATA = c;
    }
  }
}

FM_CODE static uint8 md_filemap_frame(void) {  /* md_post en el estado 5; 0: ya no (md_post sigue como siempre) */
  if (game_state != 5) {                       /* (al estado 6 o 2: lo de la sala o las opciones lo vuelven a armar) */
    fm_on = 0; fm_room = 0; pz_fsel = 0; pz_on = 0; pz_dim = 0;
    return 0;
  }
  if (fm_room) { md_pause_frame(); return 1; } /* el mapa del área: la pausa */
  pipe_take();
  nspr = (uint16)(sp_ptr - spr);
  md_frame_count++;
  md_zebes = 0; cine_stage = 0; rot_on = 0; cv_line = 0; hz_state = HZ_OFF;
  tint_on = 0; md_tint = 0;
  nt_palette_changed = 0;                      /* (los colores son los de la imagen: palette_buffer no se mira) */
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  md_vdp_busy = 1;
  if (fm_load) { fm_load = 0; fm_load_vram(); for (uint16 i = 0; i < 16; i++) cram[48 + i] = kFmSprPal[i]; cram_dirty = 1; }
  if (fm_want != fm_img && fm_want >= 0) {
    const FmImage *im = &kFmPlanet[fm_want];
    pz_dma(im->tiles, FM_TILE_BASE * 32, im->ntiles * 16);
    for (uint16 i = 0; i < 48; i++) cram[i] = im->cram[i];
    cram_dirty = 1;
    fm_img = fm_want;
    fm_key[0] = 0xFFFF;
  }
  fm_plane_a();
  md_vdp_busy = 0;
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
  uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  if (cram_dirty) cram_out_ok = 0;
  scroll_ax = scroll_ay = scroll_bx = scroll_by = 0;
  return 1;
}
NT_LATE uint8 md_filemap_frame_(void) {
  uint8 lp;
  fm_page_on(&lp);
  uint8 r = md_filemap_frame();
  pz_map_page(lp);
  return r;
}

/* el estado 5 desde game.c: filemap.c está en la página FM_PAGE */
NT_LATE void FileSelectMapPaged(void) {
  uint8 lp;
  fm_page_on(&lp);
  FileSelectMapFrame();
  pz_map_page(lp);
}
#pragma GCC pop_options
