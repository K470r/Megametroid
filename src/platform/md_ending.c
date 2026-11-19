/* MegaMetroid — el final (estado 39) en la Genesis (docs/63). Lo incluye md_engine.c (usa su lista de sprites, la
 * CRAM y el scroll), como la pausa (md_pause.c).
 *
 * El motor (ending.c, lo traducido del banco $8B) deja todo como el SNES: el texto en 7E:3000
 * (cinematic_text_tilemap), que el SNES sube a la VRAM con EndingTextUpload (los créditos) o con
 * UpdateCinematicBgTilemap en cada cuadro (la pantalla final), los registros de la PPU y los sprites
 * (DrawCinematicSpritemap, las estrellas: EndingStar). Lo que el SNES tiene en la VRAM en cada grupo de escenas está
 * preparado en el PC (tools/conv/mkending.py, md_ending_data.h).
 *
 * Modo 1:
 *   plano A = BG1: 64x64 celdas; las columnas 0-31 son la pantalla del SNES (filas 0-31 la de arriba, 32-63 la de
 *             abajo o la misma otra vez si el BG1 es de 32x32: así el scroll vertical da la vuelta igual). Las
 *             columnas 32-63 quedan vacías: una capa apagada (TM) se esconde corriendo el scroll 256. Lo que muestra:
 *             una pantalla fija del grupo (la explosión) o lo que sube el juego (el texto).
 *   plano B = BG2: una pantalla fija del grupo (Samus, el símbolo, la explosión), igual.
 * Modo 7 (Zebes, las nubes, la nave, el símbolo): la Genesis no gira ni escala un plano. Se dibuja en el 68000, a
 * bloques de 8x4 píxeles en el plano B: cada celda son dos bloques (arriba y abajo); un tile por par de colores
 * (16x16, con el 0 transparente), fijos desde ED_M7_PAT. La textura (256x256 texels, hasta 15 colores de la línea 0
 * que siguen a palette_buffer) la arma mkending.py. (Con bloques de 4x4 en los dos planos cada imagen costaba ~600
 * líneas de la consola: con la lógica del final, ~8 imágenes por segundo; así, unas 20.)
 * La imagen se calcula de a filas de celdas con el tiempo que le sobra a cada cuadro y el vblank copia las 28 filas de
 * una vez (planos de 32x32 mientras tanto: una sola DMA). */
#include "md_ending_data.h"

/* el código del final que corre con la página ED_PAGE_N puesta (md_ending_frame) va en esa página: lo común de la ROM
   (debajo de 0x300000) no tiene lugar (docs/48). Solo lo llama md_ed_body (y lo que este llama). */
_Static_assert(ED_PAGE_N == 24, "ED_CODE: la sección de la página");
#ifdef NT_SSF2
#define ED_CODE __attribute__((section(".bank24"), noinline))
#else
#define ED_CODE NT_LATE
#endif

uint8 ed_on;                                   /* el final: md_post pasa por md_ending_frame */
static uint8 ed_scene;                         /* el último EndingGfx (1-3: la huida y el espacio; 4-5: créditos) */
static const EdGroup *ed_g;                    /* grupo cargado en la VRAM */
static uint8 ed_loaded;                        /* (el primer cuadro: los planos del juego se borran) */
static uint16 ed_a_scr[2] = { 0xFFFF, 0xFFFF };  /* pantalla del SNES en cada mitad del plano A */
static uint16 ed_b_scr = 0xFFFF;               /* pantalla del SNES en el plano B (0xFFFF: ninguna; 0xFFFE: modo 7) */
#define ed_sum (md_scratch + ED_ROWS * 32)     /* suma de las palabras del SNES de cada fila del plano A ya escrita */
static uint32 ed_valid[2];                     /* fila del plano A ya escrita (bit por fila) */
static uint16 ed_up_addr = 0xFFFF;             /* EndingTextUpload: la dirección del SNES (se toma en el cuadro) */
static uint8 ed_up;
#define ED_ROWS 16                            /* filas convertidas por cuadro (como mucho; las demás, después) */
static uint8 ed_rn;
#define ed_cells md_scratch                    /* celdas de las filas a escribir: ED_ROWS * 32 */
#define ed_rrow ((uint8 *)(md_scratch + ED_ROWS * 32 + 64))   /* filas del plano de cada una (dos: 0xFF ninguna) */
_Static_assert(ED_ROWS * 32 + 64 + ED_ROWS <= sizeof md_scratch / 2, "md_scratch");

/* modo 7: estado en la RAM que el final no usa (la tabla del círculo de Ceres); las celdas, en md_scratch */
typedef struct {
  int32 u, v, du, dv, dur, dvr;                /* el texel de la primera muestra de la fila y los pasos (16.16) */
  uint16 dv8;                                  /* dv en 8.8 (calculado aparte: leer el medio de dv sería una lectura impar) */
  uint16 key[8];                               /* los registros de la imagen que se arma */
  uint16 shown[8];                             /* los de la que se ve */
  uint8 row;                                   /* próxima fila de celdas (28: armada) */
  uint8 ready;                                 /* armada: el vblank la copia */
  uint8 on;                                    /* los planos tienen bloques (la próxima vez que se usen para texto, se borran) */
  uint8 inside;                                /* textura acotada: toda la imagen cae adentro (sin mirar los bordes) */
  uint8 s[2][32];                              /* las muestras de dos filas de bloques */
} EdM7W;
#define ed_w ((EdM7W *)cv_tab)
_Static_assert(sizeof(EdM7W) <= sizeof cv_tab, "cv_tab");
#define ED_M7_STOP 206                         /* modo 7: se arman filas (~10 líneas cada una) hasta esta línea de la pantalla */
#define ed_m7_cellrow(r) (md_scratch + (r) * 32)   /* celdas del plano B: 28 filas de 32 */
_Static_assert(28 * 32 <= sizeof md_scratch / 2, "md_scratch");

ED_CODE static uint16 ed_lookup(const uint16 *lt, const uint8 *lp, const uint16 *lv, uint16 w) {
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

/* put_recs (md_engine.c) para el final: si la lista se llena, lo que entra (sin recorrer el resto); los registros
   que caen enteros en la pantalla se copian sin mirar uno por uno aunque no entren todos */
NT_LATE static void ed_put(const uint16 *r, int16 x, int16 y) {
  uint16 n = *r++;
  GenSpr *s = sp_ptr, *end = spr + MAX_SPR;
  if (s >= end) return;
  uint16 room = (uint16)(end - s);
  if (y + (int16)r[0] >= -31 && y + (int16)r[1] < 224 && x + (int16)r[2] >= -31 && x + (int16)r[3] < 256) {
    if (n > room) n = room;
    r += 4;
    uint16 yb = y + 129, xb = x + 128;
    uint16 link = (uint16)(s - spr + 1);
    for (; n; n--, r += 4, s++) {
      s->y = yb + r[0];
      s->size_link = r[1] | link++;
      s->attr = r[2];
      s->x = xb + r[3];
    }
    sp_ptr = s;
    return;
  }
  r += 4;
  for (; n && s < end; n--, r += 4) {
    int16 sy = y + (int16)r[0];
    if ((uint16)(sy + 31) >= 224 + 31) continue;
    int16 sx = x + (int16)r[3];
    if ((uint16)(sx + 31) >= 256 + 31) continue;
    s->y = sy + 129;
    s->size_link = r[1] | (uint16)(s - spr + 1);
    s->attr = r[2];
    s->x = sx + 128;
    s++;
  }
  sp_ptr = s;
}

NT_LATE static void ed_spr(uint16 sm, uint16 x, uint16 y, uint16 chr) {   /* DrawCinematicSpritemap en el final */
#ifdef ED_NOSPR
  return;
#endif
  if (!ed_g || sp_ptr >= spr + MAX_SPR) return;
  uint8 lp = md_cur_page;                      /* las tablas están en la página ED_PAGE_N (ventana 7) */
  pz_map_page(ED_PAGE_N);
  const uint16 *k = ed_g->keys;
  int lo = 0, hi = ed_g->nkeys - 1, found = 0;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    const uint16 *e = k + m * 3;
    int c = e[0] != sm ? (e[0] < sm ? -1 : 1) : e[1] != chr ? (e[1] < chr ? -1 : 1) : 0;
    if (!c) { ed_put(ed_g->recs + e[2], (int16)x, (int16)y); found = 1; break; }
    if (c < 0) lo = m + 1; else hi = m - 1;
  }
  pz_map_page(lp);
  if (!found) missing(9, sm, chr, 0xC2FF);
}

#define ED_STAR0 (((uint8 *)cv_tab)[700])       /* la primera estrella en la lista de sprites de este cuadro (0xFF: ninguna) */
NT_LATE void EndingStar(uint16 x, uint16 y, uint16 chr) {
  if (!ed_g || (uint16)(chr - 0x09F0) > 3 || sp_ptr >= spr + MAX_SPR) return;
  if (ED_STAR0 == 0xFF || ED_STAR0 > (uint16)(sp_ptr - spr)) ED_STAR0 = (uint8)(sp_ptr - spr);
  GenSpr *s = sp_ptr++;
  s->y = y + 129;
  s->size_link = (uint16)(s - spr + 1);
  s->attr = ed_g->star[chr - 0x09F0];
  s->x = x + 128;
}

/* el grupo que corresponde a lo que muestra el motor */
static uint8 ed_space;                         /* (la escena 3 ya pasó por el modo 1: la nave viene después) */
ED_CODE static const EdGroup *ed_want(void) {
  switch (ed_scene) {
  case 0: return 0;
  case 1: return &kEd_d0a;                     /* Zebes de cerca y las nubes */
  case 2: return &kEd_d0b;                     /* las nubes grises */
  case 3:
    if (reg_BGMODE != 7) { ed_space = 1; return &kEd_d0s; }   /* el espacio y la explosión */
    return ed_space ? &kEd_d0t : &kEd_d0c;     /* la nave y el tiempo / Zebes se aleja */
  }
  /* los finales según el tiempo de juego (CinematicFunction_Intro_Func134...): Samus cambia */
  uint8 v = game_time_hours < 3 ? 0 : game_time_hours < 10 ? 1 : 2;
  static const EdGroup *const kD1[3] = { &kEd_d1, &kEd_d1_5, &kEd_d1_12 };
  static const EdGroup *const kD2[3] = { &kEd_d2, &kEd_d2_5, &kEd_d2_12 };
  static const EdGroup *const kD2B[3] = { &kEd_d2b, &kEd_d2b_5, &kEd_d2b_12 };
  if (reg_BGMODE == 7) return reg_OBSEL == 0 ? kD2[v] : kD2B[v];   /* Samus chica / el símbolo */
  return reg_BG1SC == 0x4E ? &kEd_d3 : kD1[v];
}

/* una pantalla fija del grupo (dirección del SNES) para el BG1 (b = 0) o el BG2 (b = 1); NULL si no está */
ED_CODE static const uint16 *ed_static(uint8 b, uint16 addr) {
  const uint16 *s = ed_g->scr;
  uint16 na = s[0], nb = s[1];
  const uint16 *ad = s + 2, *words = s + 2 + na + nb;
  for (uint16 i = 0; i < na + nb; i++)
    if ((i >= na) == b && ad[i] == addr) return words + i * 1024;
  return 0;
}

/* (sin interrupciones) una pantalla fija en las filas 0-31 (h = 0) o 32-63 (h = 1) del plano; dup: en las dos */
ED_CODE static void ed_put_static(uint8 b, const uint16 *w, uint8 h, uint8 dup) {
  uint16 base = b ? MD_VRAM_PLANE_B : MD_VRAM_PLANE_A;
  for (uint16 r = 0; r < 32; r++) {
    uint16 *o = ed_cells;
    for (uint16 c = 0; c < 32; c++) {
      uint16 v = b ? ed_lookup(ed_g->lutTb, ed_g->lutPb, ed_g->lutVb, w[r * 32 + c])
                   : ed_lookup(ed_g->lutTa, ed_g->lutPa, ed_g->lutVa, w[r * 32 + c]);
      /* el BG2 con prioridad alta: tapa los sprites que el SNES solo suma con el color math */
      o[c] = b && (v & 0x7FF) != ED_TILE_BASE ? v | 0x8000 : v;
    }
    pz_dma(o, base + (r + 32 * h) * 128, 32);
    if (dup) pz_dma(o, base + (r + 32) * 128, 32);
  }
}

ED_CODE static void ed_m7_patterns(void) {     /* (pantalla apagada) los 256 tiles de los bloques */
  vdp_set_vram_write(ED_M7_PAT * 32);
  for (uint16 t = 0; t < 16; t++)
    for (uint16 d = 0; d < 16; d++)
      for (uint16 y = 0; y < 8; y++) {
        uint16 c = y < 4 ? t : d;
        uint16 l = (uint16)(c << 4 | c);
        l |= l << 8;
        VDP_DATA = l;
        VDP_DATA = l;
      }
}

ED_CODE static void ed_load(const EdGroup *g) {   /* (sin interrupciones) tiles y planos vacíos */
  vdp_display(0);
  vdp_reg(0x10, 0x11);                         /* planos de 64x64 */
  vdp_reg(0x12, 0x00);                         /* sin la ventana del HUD */
  vdp_reg(0x0B, 0x00);                         /* scroll de la pantalla entera */
  rot_shown = 0;
  vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
  vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
  if (g) {
    pz_dma(g->tiles, ED_TILE_BASE * 32, g->ntiles * 16);   /* (la página ED_PAGE_N ya está: md_ending_frame) */
    if (g->m7) ed_m7_patterns();
  }
  vdp_display(1);
  ed_valid[0] = ed_valid[1] = 0;
  ed_a_scr[0] = ed_a_scr[1] = 0xFFFF;
  ed_b_scr = 0xFFFF;
  ed_w->on = 0; ed_w->ready = 0; ed_w->row = 28;
  for (uint16 i = 0; i < 8; i++) ed_w->shown[i] = 0xFFFF;
}

/* filas de 7E:3000 subidas a la pantalla addr del SNES: las que cambiaron pasan a celdas (hasta ED_ROWS) */
ED_CODE static uint8 ed_rows(uint16 addr, uint16 nrows) {
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

/* ------------------------------------------------------------------ modo 7 */
/* Muestras sin mirar los bordes (la textura se repite cada 256, o toda la imagen cae dentro de la acotada): el texel
   (u >> 16, v >> 8 en 8.8) es t[v_int << 8 | u_int] con el índice de 16 bits con signo (t = la textura + 32768 y las
   filas 128-255 primero; acotada: t = la textura y v ya sin r0, el índice no llega a 32768). */
/* la fila de arriba de los bloques: 32 muestras */
ED_CODE static void ed_m7_top(uint8 *out, int32 u, uint16 v, int32 du, uint16 dv, const uint8 *t) {
  uint16 n = 7;
  __asm__ volatile(
    "1:\n\t"
    ".rept 4\n\t"
    "move.l %[u],%%d4\n\t"
    "swap %%d4\n\t"
    "move.w %[v],%%d5\n\t"
    "move.b %%d4,%%d5\n\t"
    "move.b 0(%[t],%%d5.w),(%[o])+\n\t"
    "add.l %[du],%[u]\n\t"
    "add.w %[dv],%[v]\n\t"
    ".endr\n\t"
    "dbra %[n],1b"
    : [u] "+d"(u), [v] "+d"(v), [o] "+a"(out), [n] "+d"(n)
    : [du] "d"(du), [dv] "d"(dv), [t] "a"(t)
    : "d4", "d5", "cc", "memory");
}
/* la fila de abajo y las celdas: tile ED_M7_PAT + arriba * 16 + abajo */
ED_CODE static void ed_m7_bot(uint16 *cb, const uint8 *top, int32 u, uint16 v, int32 du, uint16 dv, const uint8 *t) {
  uint16 n = 31;
  __asm__ volatile(
    "1:\n\t"
    "move.l %[u],%%d4\n\t"
    "swap %%d4\n\t"
    "move.w %[v],%%d5\n\t"
    "move.b %%d4,%%d5\n\t"
    "moveq #0,%%d4\n\t"
    "move.b 0(%[t],%%d5.w),%%d4\n\t"
    "move.b (%[s])+,%%d5\n\t"
    "lsl.b #4,%%d5\n\t"
    "or.b %%d5,%%d4\n\t"
    "add.w %[pb],%%d4\n\t"
    "move.w %%d4,(%[cb])+\n\t"
    "add.l %[du],%[u]\n\t"
    "add.w %[dv],%[v]\n\t"
    "dbra %[n],1b"
    : [u] "+d"(u), [v] "+d"(v), [cb] "+a"(cb), [s] "+a"(top), [n] "+d"(n)
    : [du] "d"(du), [dv] "d"(dv), [t] "a"(t), [pb] "i"(ED_M7_PAT)
    : "d4", "d5", "cc", "memory");
}

/* textura acotada, mirando los bordes (la imagen sale de la textura): 32 muestras; una fila que no la toca, de una */
ED_CODE static void ed_m7_box(uint8 *out, int32 u, int32 v, int32 du, int32 dv, const EdM7 *m) {
  int32 ue = u + 31 * du, ve = v + 31 * dv;
  int32 vlo = (int32)m->r0 << 16, vhi = (int32)m->r1 << 16;
  if ((u < 0 && ue < 0) || (u >= (256L << 16) && ue >= (256L << 16)) || (v < vlo && ve < vlo) || (v >= vhi && ve >= vhi)) {
    for (uint16 i = 0; i < 32; i++) out[i] = 0;
    return;
  }
  uint16 r0 = m->r0, nr = m->r1 - m->r0;
  for (uint16 i = 0; i < 32; i++, u += du, v += dv) {
    uint16 x = (uint16)(u >> 16), y = (uint16)((uint16)(v >> 16) - r0);
    out[i] = (x < 256 && y < nr) ? m->tex[(uint16)(y << 8 | x)] : 0;
  }
}

/* los registros de la imagen: se arma de nuevo solo si cambiaron */
ED_CODE static void ed_m7_keyget(uint16 *k) {
  k[0] = reg_M7A; k[1] = reg_M7B; k[2] = reg_M7C; k[3] = reg_M7D;
  k[4] = reg_M7X; k[5] = reg_M7Y; k[6] = reg_BG1HOFS; k[7] = reg_BG1VOFS;
}

ED_CODE static int16 ed_s13(uint16 v) { v &= 0x1FFF; return (int16)(v & 0x1000 ? v | 0xE000 : v); }

/* empieza una imagen: el texel de la primera muestra (bloque 0, 0: píxel 4, 2 de la pantalla, línea 3 de la PPU) */
ED_CODE static void ed_m7_start(void) {
  uint16 *k = ed_w->key;
  int16 a = (int16)k[0], b = (int16)k[1], c = (int16)k[2], d = (int16)k[3];
  int16 x0 = ed_s13(k[4]), y0 = ed_s13(k[5]), h = ed_s13(k[6]), vo = ed_s13(k[7]);
  int32 sx = 4 + h - x0, sy = 3 + vo - y0;
  ed_w->u = ((int32)a * sx + (int32)b * sy + ((int32)x0 << 8)) << 8;
  ed_w->v = ((int32)c * sx + (int32)d * sy + ((int32)y0 << 8)) << 8;
  ed_w->du = (int32)a << 11;  ed_w->dv = (int32)c << 11;    /* 8 píxeles a la derecha */
  ed_w->dur = (int32)b << 10; ed_w->dvr = (int32)d << 10;   /* 4 líneas abajo */
  ed_w->dv8 = (uint16)((int16)c << 3);
  ed_w->row = 0;
  /* acotada: si las cuatro esquinas caen adentro, toda la imagen (la imagen es un paralelogramo en la textura) */
  const EdM7 *m = ed_g->m7;
  ed_w->inside = 0;
  if (m->box) {
    uint8 ok = 1;
    for (uint16 q = 0; q < 4; q++) {
      int32 uu = ed_w->u + ((q & 1) ? 31 * ed_w->du : 0) + ((q & 2) ? 55 * ed_w->dur : 0);
      int32 vv = ed_w->v + ((q & 1) ? 31 * ed_w->dv : 0) + ((q & 2) ? 55 * ed_w->dvr : 0);
      if (uu < 0 || uu >= (256L << 16) || vv < ((int32)m->r0 << 16) || vv >= ((int32)m->r1 << 16)) ok = 0;
    }
    ed_w->inside = ok;
  }
}

ED_CODE static void ed_m7_rows(uint8 n) {      /* n filas de celdas de la imagen que se arma */
  const EdM7 *m = ed_g->m7;
  for (; n && ed_w->row < 28; n--, ed_w->row++) {
    uint16 *cb = ed_m7_cellrow(ed_w->row);
    int32 u = ed_w->u, v = ed_w->v;
    if (!m->box || ed_w->inside) {             /* sin mirar los bordes */
      const uint8 *t = m->box ? m->tex : m->tex + 32768;
      if (m->box) v -= (int32)m->r0 << 16;
      ed_m7_top(ed_w->s[0], u, (uint16)(v >> 8), ed_w->du, ed_w->dv8, t);
      u += ed_w->dur; v += ed_w->dvr;
      ed_m7_bot(cb, ed_w->s[0], u, (uint16)(v >> 8), ed_w->du, ed_w->dv8, t);
    } else {
      ed_m7_box(ed_w->s[0], u, v, ed_w->du, ed_w->dv, m);
      ed_m7_box(ed_w->s[1], u + ed_w->dur, v + ed_w->dvr, ed_w->du, ed_w->dv, m);
      const uint8 *s0 = ed_w->s[0], *s1 = ed_w->s[1];
      for (uint16 c = 0; c < 32; c++) cb[c] = ED_M7_PAT + ((uint16)(*s0++ << 4) | *s1++);
    }
    ed_w->u += 2 * ed_w->dur; ed_w->v += 2 * ed_w->dvr;
  }
  if (ed_w->row == 28) {
    ed_w->ready = 1;
    for (uint16 i = 0; i < 8; i++) ed_w->shown[i] = ed_w->key[i];
  }
}

NT_LATE void ed_vblank(void) {                 /* md_vblank: las celdas del modo 7 armadas */
  if (!ed_on || !ed_w->ready) return;
  ed_w->ready = 0;
  vdp_dma_lean(ed_m7_cellrow(0), MD_VRAM_PLANE_B, 28 * 32);   /* (plano de 32x32) */
}

ED_CODE static void md_ed_body(void);
NT_LATE void md_ending_frame(void) {          /* md_post en el estado 39 */
  pipe_take();
  uint8 lp = md_cur_page;                      /* las tablas y el código del final: página ED_PAGE_N en la ventana 7 */
  pz_map_page(ED_PAGE_N);
  md_ed_body();
  pz_map_page(lp);
}

ED_CODE static void md_ed_body(void) {
  nspr = (reg_TM | reg_TS) & 0x10 ? (uint16)(sp_ptr - spr) : 0;   /* sprites apagados (TM/TS): el SNES no los muestra */
  /* Samus (créditos): los sprites que el SNES solo suma sobre el BG2 (Samus sin traje que aparece sobre la del traje,
     con su paleta subiendo desde negro) taparían la del traje con una figura oscura: no se muestran */
  if (!(reg_TM & 0x10) && (reg_CGADSUB & 0x02) && (ed_g == &kEd_d1 || ed_g == &kEd_d1_5 || ed_g == &kEd_d1_12)) {
    /* el SNES suma las dos (una se apaga y la otra se enciende: CinematicFunction_Intro_Func136/138): se ve la más
       brillante; las estrellas, siempre */
    uint16 lb = 0, lo = 0;
    for (uint16 i = 0; i < 16; i++) {
      uint16 c = palette_buffer[32 + i], d = palette_buffer[208 + i];
      lb += (c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31);
      lo += (d & 31) + ((d >> 5) & 31) + ((d >> 10) & 31);
    }
    if (lo <= lb) {
      uint16 s0 = ED_STAR0 < nspr ? ED_STAR0 : nspr, n = nspr - s0;
      for (uint16 i = 0; i < n; i++) {
        spr[i] = spr[s0 + i];
        spr[i].size_link = (spr[i].size_link & 0x0F00) | (i + 1);
      }
      nspr = n;
    }
  }
  ED_STAR0 = 0xFF;
  /* sprites en la pantalla principal (TM): encima de todo. Solo en la secundaria (TS, sumados por el color math: Samus
     sin traje que aparece sobre la del traje, las estrellas de la pantalla final): detrás del BG2 (sus celdas van con
     prioridad alta) */
  if (reg_TM & 0x10)
    for (GenSpr *q = spr; q < sp_ptr; q++) q->attr |= 0x8000;
  md_frame_count++;
  rot_on = 0; cv_line = 0; hz_state = HZ_OFF; md_zebes = 0; cine_stage = 0;
  if (tint_on) tint_on = 0;
  md_tint = 0;
  const EdGroup *want = ed_want();
  uint8 m7 = want && want->m7 && reg_BGMODE == 7 && (reg_TM & 1);
  /* colores: los del grupo, de palette_buffer en cada cuadro; con modo 7, la línea 0 es la de la textura */
  const uint8 *src = ed_g ? ed_g->src : 0;
  /* el SNES suma el BG1 a sí mismo (color math: BG1 en la pantalla secundaria, suma sin la mitad): el doble de brillo */
  uint8 dbl = (reg_CGWSEL & 2) && (reg_TS & 1) && (reg_CGADSUB & 0xC1) == 0x01;
  /* el color fijo (COLDATA) sumado a las capas del color math (CGWSEL sin la pantalla secundaria): los destellos
     blancos que se apagan (la nave, la pantalla final). Por línea de la CRAM: el 0 es el fondo; con modo 7 la línea
     0 es el BG1 y las demás los sprites; sin modo 7, las capas van mezcladas en las líneas: todas */
  uint8 fx = !(reg_CGWSEL & 2) && !(reg_CGADSUB & 0x80) ? reg_CGADSUB & 0x33 : 0;
  uint16 fr = reg_COLDATA[0] & 31, fg = reg_COLDATA[1] & 31, fb = reg_COLDATA[2] & 31;
  if (!(fr | fg | fb)) fx = 0;
  uint8 m7g = ed_g && ed_g->m7;
  /* Samus con el traje (créditos): el SNES suma el BG3 (un campo azul con rayas, palette_buffer[7]) al BG2: aquí se
     suma ese azul a los colores del BG (índices < 128) */
  uint8 bg3 = (reg_CGWSEL & 2) && (reg_TS & 4) && (reg_CGADSUB & 0x82) == 0x02 && !m7g;
  uint16 b3 = palette_buffer[7];
  for (uint16 i = 0; i < 64; i++) {
    uint16 c = 0;
    uint8 use = 1;
    if (m7g && i && i < 16) c = palette_buffer[ed_g->m7->col[i]] & 0x7FFF;
    else if (src && src[i] != 0xFF) c = palette_buffer[src[i]] & 0x7FFF;
    else if (!i) c = palette_buffer[0] & 0x7FFF;
    else use = 0;
    if (use) {
      uint16 r = c & 31, g = (c >> 5) & 31, bl = (c >> 10) & 31;
      if (dbl && m7g && i && i < 16) { r <<= 1; g <<= 1; bl <<= 1; }
      uint8 lay = !i ? 0x20 : !m7g ? 0x13 : i < 16 ? 0x01 : 0x10;
      if (bg3 && i && src && src[i] != 0xFF && src[i] < 128) {
        r += b3 & 31; g += (b3 >> 5) & 31; bl += (b3 >> 10) & 31;
      }
      if (fx & lay) {
        r += fr; g += fg; bl += fb;
        if (reg_CGADSUB & 0x40) { r >>= 1; g >>= 1; bl >>= 1; }
      }
      c = (r > 31 ? 31 : r) | (g > 31 ? 31 : g) << 5 | (bl > 31 ? 31 : bl) << 10;
    }
    uint16 v = kCram555[c];
    if (v != cram[i]) { cram[i] = v; cram_dirty = 1; }
  }
  uint8 b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  if (cram_dirty) cram_out_ok = 0;
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  md_vdp_busy = 1;
  if (want != ed_g || !ed_loaded) {
    ed_load(want);
    ed_g = want;
    ed_loaded = 1;
  }
  ed_rn = 0;
  uint8 showa = 0, showb = 0;
  if (ed_g && m7) {
    /* modo 7: los dos planos con los bloques (sin scroll); la imagen nueva solo si cambiaron los registros */
    if (!ed_w->on) {
      vdp_reg(0x10, 0x00);                     /* planos de 32x32: las celdas seguidas (una DMA) */
      vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
      vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
      ed_w->on = 1;
      ed_a_scr[0] = ed_a_scr[1] = ed_b_scr = 0xFFFF;
    }
    showa = showb = 1;
  } else if (ed_g) {
    if (ed_w->on) {                            /* (de los bloques al texto: planos vacíos) */
      vdp_reg(0x10, 0x11);                     /* planos de 64x64 */
      vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
      vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
      ed_w->on = 0; ed_w->row = 28; ed_w->ready = 0;
      for (uint16 i = 0; i < 8; i++) ed_w->shown[i] = 0xFFFF;
    }
    /* pantallas del BG1: la de arriba y la de abajo (32x64) o la misma dos veces (32x32) */
    uint16 base = (uint16)(reg_BG1SC & 0xFC) << 8;
    uint16 s0 = base, s1 = (reg_BG1SC & 2) ? base + 0x400 : base;
    if (s0 != ed_a_scr[0] || s1 != ed_a_scr[1]) {
      vdp_fill_vram(MD_VRAM_PLANE_A, 0, 0x1000);
      ed_a_scr[0] = s0; ed_a_scr[1] = s1;
      ed_valid[0] = ed_valid[1] = 0;
      const uint16 *w0 = ed_static(0, s0), *w1 = s1 != s0 ? ed_static(0, s1) : 0;
      if (w0) ed_put_static(0, w0, 0, s1 == s0);
      if (w1) ed_put_static(0, w1, 1, 0);
    }
    uint16 bb = (uint16)(reg_BG2SC & 0xFC) << 8;
    if (bb != ed_b_scr) {
      const uint16 *w = ed_static(1, bb);
      if (w) ed_put_static(1, w, 0, 1);
      else vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
      ed_b_scr = bb;
    }
    uint8 more = 0;
    if (ed_up) more |= ed_rows(ed_up_addr, 32);
    if ((cinematic_enable_objs & 0x8000) && (cinematic_enable_bg_tilemap & 0x8000))
      more |= ed_rows(cinematicbg_var3, 30);
    ed_up = more;                               /* (lo que no entró sigue pendiente) */
    for (uint16 i = 0; i < ed_rn; i++)
      for (uint16 j = 0; j < 2; j++)
        if (ed_rrow[i * 2 + j] != 0xFF) pz_dma(ed_cells + i * 32, MD_VRAM_PLANE_A + ed_rrow[i * 2 + j] * 128, 32);
    showa = (reg_TM & 1) != 0;
    showb = (reg_TM & 2) && ed_static(1, bb);
  }
  md_vdp_busy = 0;
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
  if (ed_g && m7 && !ed_w->ready) {            /* (con el vblank libre: lo armado no se copió todavía si ready) */
    if (ed_w->row >= 28) {
      uint16 k[8];
      ed_m7_keyget(k);
      uint8 same = 1;
      for (uint16 i = 0; i < 8; i++) if (k[i] != ed_w->shown[i]) same = 0;
      if (!same) { for (uint16 i = 0; i < 8; i++) ed_w->key[i] = k[i]; ed_m7_start(); }
    }
#ifndef ED_NOM7
    /* filas mientras quede cuadro: hasta la línea ED_M7_STOP (el vblank empieza en la 224) y al menos una; así el
       final no se atrasa (en las escenas pesadas la imagen cambia menos veces por segundo) */
    uint16 v0 = VDP_VCOUNT >> 8, n = 0;
    while (ed_w->row < 28 && n < 28) {
      ed_m7_rows(1);
      n++;
      uint16 v = VDP_VCOUNT >> 8;
      if (v < v0 || v >= ED_M7_STOP) break;
    }
#endif
  }
  /* scroll: una capa apagada (TM) mira las columnas vacías; en modo 7, los bloques sin scroll */
  if (m7) {
    scroll_ax = scroll_bx = 0;
    scroll_ay = scroll_by = 0;
  } else {
    scroll_ax = (reg_BG1HOFS & 255) + (showa ? 0 : 256);
    scroll_ay = reg_BG1VOFS & 511;
    scroll_bx = (reg_BG2HOFS & 255) + (showb ? 0 : 256);
    scroll_by = reg_BG2VOFS & 511;
  }
}

#ifdef MD_EDPROF
/* medición: líneas de cada parte del cuadro del estado 39 (ending.c), el máximo de cada una en md_scratch[1400..] */
NT_LATE void md_edprof(uint8 i) {
  static uint16 last;
  uint16 v = VDP_VCOUNT >> 8;
  if (i == 1 && md_frame_count == MD_EDPROF) for (uint16 k = 0; k < 8; k++) ((uint16 *)cv_tab)[400 + k] = 0;
  if (i && md_frame_count >= MD_EDPROF && md_frame_count < MD_EDPROF + 40) {
    uint16 d = (uint16)(v - last) & 0xFF; uint16 *mx = (uint16 *)cv_tab + 400 + i; *mx += d; }
  last = v;
}
#endif
