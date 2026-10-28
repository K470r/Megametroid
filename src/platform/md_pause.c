/* MegaMetroid — la pausa en la Genesis: el mapa y la pantalla de equipo (docs/53). Lo incluye md_engine.c (usa su
 * lista de sprites, la CRAM y el HUD).
 *
 * El motor (pause.c) deja todo como el SNES: los mapas de la pausa en la RAM 7E:3000-3FFF y, para el mapa del área,
 * PauseMapCell (lo que el SNES arma en 7E:4000). Lo que el SNES pasa a la VRAM lo hace esto, con datos ya preparados
 * en el PC (tools/conv/mkpause.py: tiles, celdas, colores y sprites de la pausa desde la ROM):
 *   plano A = BG1 (mapa 64x32 con su scroll, o el equipo desde 7E:3800), plano B = BG2 (el marco, $B6:E000, con el
 *   nombre del área y las filas de L/R/START de 7E:3640), Window = el HUD (como en el juego), sprites = DrawMenuSpritemap.
 * Las celdas salen de kPzLutT/P/V (palabra del SNES -> celda); los colores, de palette_buffer con kPzSrc (los animados de la
 * pausa cambian solos). Los tiles de la pausa ocupan los de la sala (256-794): al salir, ResumeGameplay vuelve a
 * cargar la sala (g_hooks.room_gfx). Lo que cambia de golpe (entrar, pasar del equipo al mapa) se dibuja con la
 * pantalla oscura, como en el SNES: tiles y marco con la pantalla apagada, el mapa de a PZ_MAPROWS filas por cuadro.
 * md_post y md_vblank (md_engine.c) no cambian salvo un desvío al principio de md_post (md_pause_frame, al final): los
 * tiempos del resto del juego no se mueven (docs/53, regresión). */
#include "md_pause_data.h"

static void pipe_take(void);                   /* (md_engine.c, más abajo) */
static void hud_prepare(void);
extern uint8 md_spr_reserved;

uint8 pz_on;                                   /* la pausa se está dibujando (de PAUSE_GFX_ENTER a la sala otra vez) */
uint8 pz_dim;                                  /* estados 12-18 (md_post y md_vblank pasan por aquí) */
static uint8 pz_line3;                         /* 12 y 18: los colores de la línea 3 están oscurecidos (pz_line3_save) */
static uint16 pz_line3_save[15];
static uint8 pz_load;                          /* vblank: tiles y plano B (el A, vacío) */
static uint8 pz_amode;                         /* plano A: 0 el mapa, 1 el equipo */
static uint8 pz_label_dirty;                   /* vblank: el nombre del área en el plano B */
static uint8 pz_map_row = 32;                  /* mapa: próxima fila a armar (32: listo) */
#define PZ_MAPROWS 3                           /* mapa: filas por cuadro (con la pantalla oscura: entrar, volver del equipo) */
static uint8 pz_mn, pz_mrow[PZ_MAPROWS];       /* mapa: filas listas para el vblank (celdas en pz_mrowc) */
static uint8 pz_brows;                         /* filas 25-26 del plano B que cambiaron (en pz_bcells) */
#define PZ_AROWS 8                             /* equipo: filas del plano A que se pasan por cuadro (como mucho) */
static uint8 pz_an, pz_arow[PZ_AROWS];         /* equipo: filas listas para el vblank (celdas en pz_arowc) */
#define pz_araw md_scratch                     /* equipo: las 32x32 palabras del SNES ya mostradas (RAM compartida) */
#define pz_bcells (md_scratch + 1024)          /* plano B, filas 25 y 26 */
#define pz_arowc (md_scratch + 1088)           /* celdas de las filas pz_arow */
#define pz_mrowc md_scratch                    /* mapa: celdas de las filas pz_mrow (el mapa no usa pz_araw) */
#define pz_braw (md_scratch + 1088 + PZ_AROWS * 32)   /* plano B, filas 25 y 26: las palabras del SNES mostradas */
_Static_assert(1088 + PZ_AROWS * 32 + 64 <= sizeof md_scratch / 2, "md_scratch");
_Static_assert(PZ_MAPROWS * 64 <= 1024, "md_scratch");

/* vdp_dma_vram y vdp_dma_raw, copiadas: otra llamada a la de md_vdp.h haría que gcc la parta en md_engine.c y correría las direcciones
   de la ROM (docs/53) */
NT_LATE static void pz_dma_raw(uint32_t from, uint16_t to, uint16_t words) {   /* vdp_dma_raw */
  volatile uint16_t *pw = &VDP_CTRL;
  *pw = 0x9300 | (words & 0xFF);
  *pw = 0x9400 | ((words >> 8) & 0xFF);
  from >>= 1;
  *pw = 0x9500 | (from & 0xFF);
  *pw = 0x9600 | ((from >> 8) & 0xFF);
  *pw = 0x9700 | ((from >> 16) & 0x7F);
  static volatile uint16_t cmd[2];
  uint32_t c = VDP_CMD_VRAM_W(to) | 0x80;
  cmd[0] = c >> 16;
  cmd[1] = (uint16_t)c;
  *pw = cmd[0];
  __asm__ volatile("move.w (%0),(%1)" : : "a"(&cmd[1]), "a"(pw) : "memory");
}
NT_LATE static void pz_dma(const void *src, uint16_t to, uint16_t words) {
  if (!words) return;
  uint32_t from = (uint32_t)src;
  uint32_t limit = (0x20000 - (from & 0x1FFFF)) >> 1;        /* palabras hasta el borde de 128 KB */
  if (words > limit) {
    pz_dma_raw(from + limit * 2, to + limit * 2, words - limit);
    words = limit;
  }
  pz_dma_raw(from, to, words);
}

NT_LATE static uint16 pz_lookup(uint16 w) {            /* palabra del SNES -> celda de la Genesis (volteo y prioridad del SNES) */
  uint16 k = kPzLutT[w & 0x3FF];
  if (k != 0xFFFF) {
    uint8 p = (w >> 10) & 7;
    for (; kPzLutP[k] != 0xFF; k++)
      if (kPzLutP[k] == p) return (kPzLutV[k] ^ ((w >> 3) & 0x1800)) | ((w << 2) & 0x8000);
  }
  return 0;
}
NT_LATE static uint16 pz_cell(uint16 w) {              /* lo mismo recordando la última (el mapa la repite mucho; solo md_post) */
  static uint16 last_w = 0xFFFF, last_c;
  if (w != last_w) { last_w = w; last_c = pz_lookup(w); }
  return last_c;
}

extern uint8 md_cur_page;                      /* md_zebes.c */
extern volatile uint8 md_hw_page;
NT_LATE static void pz_map_page(uint8 p) {             /* (como map_page de md_zebes.c) */
  __asm__ volatile("" ::: "memory");
  md_cur_page = p;
  md_hw_page = p;
  *(volatile uint8 *)0xA130FF = p;
  __asm__ volatile("" ::: "memory");
}

NT_LATE static void pz_set_cram(void) {                /* como md_use_room, con los colores de la pausa */
  for (uint16 i = 0; i < 256; i++) cram_rev[i] = 0xFF;
  for (int16 i = 63; i >= 0; i--) {
    uint8 s = kPzSrc[i];
    cram_off[i] = s == 0xFF ? 0xFFFF : s * 2;
    cram_next[i] = 0xFF;
    if (s != 0xFF) { cram_next[i] = cram_rev[s]; cram_rev[s] = (uint8)i; }
  }
  nt_palette_lo = 0;
  nt_palette_hi = 255;
  nt_palette_changed = 1;
}

NT_LATE void PauseGfx(uint16 what) {
  if (what != PAUSE_GFX_FADE)
    md_sync();                                 /* el vblank toma antes el cuadro anterior (usa md_scratch) */
  switch (what) {
  case PAUSE_GFX_FADE:                         /* estados 12 y 18: el juego se oscurece o se aclara con INIDISP */
    pz_dim = 1;
    return;
  case PAUSE_GFX_ENTER:                        /* LoadPauseMenuTilesAndClearBG2 ($82:8E75) */
    pz_on = 1;
    pz_dim = 1;
    pz_load = 1;
    md_zebes = 0;                               /* la sala no se dibuja (md_vblank); al salir, zebes_use la vuelve a poner */
    for (uint16 i = 0; i < 64; i++) pz_braw[i] = 0xFFFF;   /* las filas 25-26 del BG2: todas por convertir */
    pz_set_cram();
    break;
  case PAUSE_GFX_MAP:                          /* LoadPauseMenuMapTilemapAndAreaLabel ($82:93C3) */
    pz_amode = 0;
    pz_map_row = 0;                             /* md_pause_post lo arma de a PZ_MAPROWS filas */
    pz_mn = 0;
    pz_label_dirty = 1;
    break;
  case PAUSE_GFX_EQUIPMENT:                    /* EquipmentScreenTransferBG1Tilemap ($82:AC22) */
    pz_amode = 1;
    pz_an = 0;
    for (uint16 i = 0; i < 1024; i++) pz_araw[i] = 0xFFFF;   /* todo distinto: md_pause_post lo pasa en unos cuadros */
    break;
  }
}

NT_LATE static void pz_spr(uint16 idx, uint16 x, uint16 y, uint16 chr) {   /* DrawMenuSpritemap en la pausa */
  uint16 o = idx < PZ_SPR_COUNT ? kPzSprIdx[idx * 8 + ((chr >> 9) & 7)] : 0xFFFF;
  if (o == 0xFFFF) { missing(10, idx, chr, 0xC0FE); return; }
  put_recs(kPzSprRecs + o, (int16)x, (int16)y, 0);
}

/* md_post: lo que cambió en los mapas de la RAM pasa a celdas (el vblank solo copia) */
NT_LATE static void md_pause_post(void) {
  scroll_ax = reg_BG1HOFS;
  scroll_ay = reg_BG1VOFS;
  scroll_bx = reg_BG2HOFS;
  scroll_by = reg_BG2VOFS;
  const uint16 *b = &cinematic_text_tilemap[0x320];   /* 7E:3640: filas 25-26 del BG2 (UpdatePauseMenuLRStartVramTilemap) */
  for (uint16 i = 0; i < 64; i++)
    if (b[i] != pz_braw[i]) { pz_braw[i] = b[i]; pz_bcells[i] = pz_cell(b[i]); pz_brows |= 1 << (i >> 5); }
  pz_mn = 0;
  if (pz_amode == 0) {                         /* mapa (LoadPauseMenuMapTilemap): el BG1 de 64x32 del SNES son dos
                                                  pantallas de 32x32 una detrás de la otra (celda i: pantalla i >> 10) */
    for (; pz_map_row < 32 && pz_mn < PZ_MAPROWS; pz_map_row++) {
      uint16 *o = pz_mrowc + pz_mn * 64;
      uint16 r = pz_map_row;
      for (uint16 c = 0; c < 64; c += 8) {
        PauseMapCells8((c >> 5) * 1024 + r * 32 + (c & 31), o + c);
        for (uint16 k = 0; k < 8; k++) o[c + k] = pz_cell(o[c + k]);
      }
      pz_mrow[pz_mn++] = (uint8)r;
    }
  }
  if (pz_amode == 1) {                         /* equipo: 7E:3800 (lo que el SNES pone en la cola cada cuadro) */
    /* las palabras del SNES contra las mostradas, de a 32 bits; solo las filas distintas se convierten (hasta
       PZ_AROWS por cuadro: el resto en los siguientes, como al entrar, con la pantalla oscura) */
    const uint32 *e = (const uint32 *)cinematic_bg_tilemap;
    const uint32 *s = (const uint32 *)pz_araw;
    const uint32 *end = e + 512;
    pz_an = 0;
    while (pz_an < PZ_AROWS) {
      /* la primera palabra larga distinta (cmpm: ~20 ciclos cada una; casi siempre son todas iguales) */
      uint16 n = (uint16)(end - e) - 1, eq;
      if ((int16)n < 0) break;
      __asm__ volatile("1: cmpm.l (%1)+,(%2)+\n\tdbne %3,1b\n\tseq %0"
                       : "=d"(eq), "+a"(e), "+a"(s), "+d"(n) : : "cc", "memory");
      if (eq & 1) break;
      uint16 r = (uint16)((e - 1) - (const uint32 *)cinematic_bg_tilemap) >> 4;   /* la fila de la distinta */
      e = (const uint32 *)cinematic_bg_tilemap + r * 16;
      uint32 *sw = (uint32 *)pz_araw + r * 16;
      for (uint16 c = 0; c < 16; c++) sw[c] = e[c];
      const uint16 *w = (const uint16 *)e;
      uint16 *o = pz_arowc + pz_an * 32;
      for (uint16 c = 0; c < 32; c++) o[c] = pz_cell(w[c]);
      pz_arow[pz_an++] = (uint8)r;
      e += 16;
      s = sw + 16;
    }
  }
}

NT_LATE static void pz_plane_b(void) {                 /* (pantalla apagada) el marco: $B6:E000, 32x32, y el nombre del área */
  vdp_set_vram_write(MD_VRAM_PLANE_B);
  for (uint16 r = 0; r < 32; r++) {
    for (uint16 c = 0; c < 32; c++) VDP_DATA = pz_lookup(kBankB6_Pause[r * 32 + c]);   /* BB6W(0xE000 + ...) */
    for (uint16 c = 0; c < 32; c++) VDP_DATA = 0;
  }
}

NT_LATE static void pz_label(void) {                   /* kPauseAreaLabelTilemap ($82:965F) -> BG2, fila 5, columna 10 */
  uint16 a = area_index < 7 ? area_index : 0;
  uint16 p = B82W(0x965F + 2 * a);
  vdp_set_vram_write(MD_VRAM_PLANE_B + (5 * 64 + 10) * 2);
  for (uint16 i = 0; i < 12; i++) VDP_DATA = pz_lookup(B82W(p + 2 * i));
}

/* lo propio de la pausa en la VRAM (desde md_pause_frame; la CRAM, los sprites, el HUD y el scroll los escribe md_vblank
   como siempre) */
NT_LATE static void md_pause_vblank(void) {
  if (pz_load) {
    vdp_display(0);
    {
      pz_load = 0;
      uint8 lp = md_cur_page;                  /* los tiles están en la página PZ_PAGE_N (ventana 7) */
      pz_map_page(PZ_PAGE_N);
      pz_dma(kPzTiles, PZ_TILE_BASE * 32, PZ_TILE_COUNT * 16);
      pz_map_page(lp);
      vdp_reg(0x10, 0x01);                      /* planos de 64x32 (BG1SC = 0x31, BG2SC = 0x38) */
      vdp_reg(0x12, 0x04);                      /* el HUD en la ventana */
      vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
      pz_plane_b();
      pz_brows = 3;
    }
    vdp_display(1);
  }
  if (pz_label_dirty) { pz_label_dirty = 0; pz_label(); }
  for (uint16 i = 0; i < pz_mn; i++)
    pz_dma(pz_mrowc + i * 64, MD_VRAM_PLANE_A + pz_mrow[i] * 128, 64);
  pz_mn = 0;
  for (uint16 r = 0; r < 2; r++)
    if (pz_brows & (1 << r)) pz_dma(&pz_bcells[r * 32], MD_VRAM_PLANE_B + (25 + r) * 128, 32);
  pz_brows = 0;
  for (uint16 i = 0; i < pz_an; i++)
    pz_dma(pz_arowc + i * 32, MD_VRAM_PLANE_A + pz_arow[i] * 128, 32);
  pz_an = 0;
}

/* ------------------------------------------------------------------ el cuadro en los estados 12-18
   md_post y md_vblank (md_engine.c) son los de siempre con un desvío al principio: así no cambia cómo se compilan ni
   cuánto tardan fuera de la pausa (la regresión de imagen de Ceres y de la intro depende del ciclo exacto, docs/53) */
static uint8 md_bright_of_inidisp(void) { return (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF); }

NT_LATE static void pz_scale(uint16 *c, uint16 n) {   /* colores de la CRAM con el brillo md_bright (como md_vblank) */
  static const uint8 kPzBright[16][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 1, 1, 1, 1 }, { 0, 0, 0, 1, 1, 1, 1, 1 },
    { 0, 0, 1, 1, 1, 1, 2, 2 }, { 0, 0, 1, 1, 1, 2, 2, 2 }, { 0, 0, 1, 1, 2, 2, 2, 3 }, { 0, 0, 1, 1, 2, 2, 3, 3 },
    { 0, 1, 1, 2, 2, 3, 3, 4 }, { 0, 1, 1, 2, 2, 3, 4, 4 }, { 0, 1, 1, 2, 3, 3, 4, 5 }, { 0, 1, 1, 2, 3, 4, 4, 5 },
    { 0, 1, 2, 2, 3, 4, 5, 6 }, { 0, 1, 2, 3, 3, 4, 5, 6 }, { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 1, 2, 3, 4, 5, 6, 7 } };
  const uint8 *bt = kPzBright[md_bright & 15];
  for (uint16 i = 0; i < n; i++)
    c[i] = (uint16)(bt[(c[i] >> 1) & 7] << 1 | bt[(c[i] >> 5) & 7] << 5 | bt[(c[i] >> 9) & 7] << 9);
}

NT_LATE void md_pause_frame(void) {            /* md_post en los estados 12-18 */
  if (!pz_on) {
    /* 12 y 18: el juego, oscurecido con INIDISP (en el juego md_post usa siempre 15). md_vblank pone los colores de la
       línea 3 (HUD arriba, enemigos desde la línea 32 con la interrupción) sin el brillo: aquí se oscurecen antes */
    if (pz_line3) { for (uint16 i = 0; i < 15; i++) cram[49 + i] = pz_line3_save[i]; pz_line3 = 0; }
    pz_dim = 0;
    md_post();
    uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
    if (b != md_bright) { md_bright = b; cram_dirty = 1; }
    if (md_bright != 15 && md_zebes) {
      for (uint16 i = 0; i < 15; i++) pz_line3_save[i] = cram[49 + i];
      pz_scale(&cram[49], 15);
      /* lo que la interrupción de la línea 31 pone en la línea 3 (zobj_col: colores 1-15): los mismos del HUD, ya
         oscurecidos (los enemigos los usan unos cuadros, con la pantalla casi negra) */
      for (uint16 i = 0; i < 15; i++) zobj_col[i] = cram[49 + i];
      pz_line3 = 1;
    }
    pz_dim = (uint16)(game_state - 12) <= 6;   /* (18 -> 8: se terminó) */
    return;
  }
  pipe_take();
  if (md_spr_reserved) {                       /* salas con el ojo: los dos sprites del haz (md_logic) fuera de la vista */
    for (uint16 i = 0; i < 2; i++) { spr[i].y = 0; spr[i].size_link = i + 1; spr[i].attr = 0; spr[i].x = 0; }
  }
  ncone = 0;
  nspr = (uint16)(sp_ptr - spr);
  md_frame_count++;
  if (tint_on) { tint_on = 0; nt_palette_changed = 1; nt_palette_lo = 0; nt_palette_hi = 255; }   /* sin tinte */
  if (nt_palette_changed) {                    /* colores: como en md_post */
    nt_palette_changed = 0;
    if (nt_palette_hi - nt_palette_lo < 32) {  /* pocos índices (la flecha, la paleta animada): solo sus colores */
      for (uint16 i = nt_palette_lo; i <= nt_palette_hi; i++)
        for (uint8 slot = cram_rev[i]; slot != 0xFF; slot = cram_next[slot]) {
          uint16 v = kCram555[palette_buffer[i] & 0x7FFF];
          if (v != cram[slot]) { cram[slot] = v; cram_dirty = 1; }
        }
    } else {
      const uint16 *off = cram_off;
      uint16 *c = cram;
      for (uint16 i = 64; i; i--, c++) {
        uint16 o = *off++;
        uint16 v = o >= 0xFFFE ? 0 : kCram555[palette_buffer[o >> 1] & 0x7FFF];
        if (v != *c) { *c = v; cram_dirty = 1; }
      }
    }
  }
  uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  if (md_tint) { md_tint = 0; cram_dirty = 1; cram_t_ok = 0; }
  md_pause_post();
  hud_prepare();
  /* lo propio de la pausa en la VRAM, ya: md_vblank (el de siempre: sprites, colores, HUD, scroll) no lo hace. El
     cuadro anterior ya está tomado (md_post corre con el vblank libre); sin interrupciones mientras se escribe */
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  md_vdp_busy = 1;
  md_pause_vblank();
  md_vdp_busy = 0;
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
}
