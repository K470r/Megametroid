/* MegaMetroid — capa de la Genesis del motor nativo (docs/32): lo que el SNES hacía en el NMI y en la PPU.
 *
 * Funciones de dibujo del motor -> lista de sprites de la Genesis (registros ya armados en el PC).
 * Fin de cuadro -> CRAM (64 colores tomados de palette_buffer con kCram555), celdas del HUD (kHudLut), tiles
 * de Samus (bloques ya intercalados), scroll de los planos. md_vblank() lo escribe en el VDP.
 * Sin SGDK aquí: acceso directo al VDP (md_vdp.h). */
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "room.h"
#include "viewer.h"
#include "md.h"
#include "md_vdp.h"
#ifdef MD_DKCOUNT
volatile uint16 md_dk_calls[16];
#endif
#ifdef MD_STATESUM
#include "init_state.h"
/* prueba: suma del estado del motor (las variables del estado inicial, en orden) en cada cuadro, para comparar con
   el mismo motor en el PC (host/simsum): así se comprueba el código propio de la Genesis */
volatile uint16_t md_state_log[256];
void md_state_sum(void) {
  extern volatile uint16 md_frame_count;
  uint16 k = (uint16)(md_frame_count - MD_STATESUM);
  if (k >= 256) return;
  uint16 s = 0;
  for (const InitEnt *e = kInitState; e->ptr; e++) {
    if (e->ptr == (void *)palette_buffer) continue;   /* fundidos: la Genesis solo funde los colores que se ven */
    const uint8 *p = (const uint8 *)e->ptr;
    for (uint16 i = 0; i < e->size; i++) s = (uint16)((s << 1) | (s >> 15)) ^ p[i];
  }
  md_state_log[k] = s;
}
#endif
#ifdef MD_BUSYLOG
/* medición: líneas de cada cuadro del motor (md_main.c). La RAM es justa: van en el texto de las cinemáticas, que
   no se usa en Zebes (desde el byte 1600 está el búfer del haz, md_cone.c) */
extern uint16 cinematic_text_tilemap[];
volatile uint8_t *const md_busy_log = (volatile uint8_t *)cinematic_text_tilemap;
volatile uint8_t *const md_post_log = (volatile uint8_t *)cinematic_text_tilemap + 304;
#endif
#include "md_data.h"
#include "md_zebes.h"
#include "md_getaway.h"
#include "md_m7.h"
#include "md_cine.h"                     /* página del SSF2 de cada bloque de las cinemáticas (mkzebes.py) */
#include "hdmaobj.h"
#include "init_state.h"

volatile uint16_t md_frame_count;
#ifndef MD_LAG_N
#define MD_LAG_N 4096                            /* cuadros del registro de retrasos (la RAM es justa) */
#endif
volatile uint8_t md_lag_log[MD_LAG_N];

volatile uint16_t md_vb_over, md_vb_late, md_vb_overlog[128];   /* MD_LAGLOG: línea del VDP al terminar el cuadro / el vblank */          /* MD_LAGLOG: cuadros de la consola que tomó cada cuadro del motor */
volatile uint16_t md_missing;
volatile uint16_t md_missing_key[4];
volatile uint16_t md_unimpl_count;
const char *volatile md_unimpl_msg;

/* ------------------------------------------------------------------ estado de la capa */
static const MdRoom *g_room;
static uint8 room_pending;                    /* la sala cambió: cargar tiles y planos en el próximo vblank */

#define MAX_SPR 64                            /* H32: 64 sprites */
typedef struct { int16 y; uint16 size_link; uint16 attr; int16 x; } GenSpr;
/* Cuadros en cadena (docs/39): la lógica del cuadro siguiente corre mientras el vblank todavía no mostró el
   anterior. Lo que el motor escribe durante su cuadro (sprites, colas de los ganchos) va a un lado "de la lógica";
   la segunda parte del cuadro (md_post) lo pasa al lado "del vblank" cuando el vblank ya tomó el cuadro anterior. */
/* RAM compartida: la usa uno solo de los modos a la vez (la intro, la capa del modo 7 de las cinemáticas, las salas
   de Zebes); cada uno la vuelve a armar al empezar */
uint16 md_scratch[1440];
static GenSpr spr_bank[2][MAX_SPR];
GenSpr *spr = spr_bank[0], *spr_end = spr_bank[0] + MAX_SPR;   /* la lista que arma la lógica */
static GenSpr *spr_out = spr_bank[1];          /* la que toma el vblank */
static uint16 nspr, nspr_out, nspr_shown;
static uint16 ncone, ncone_out;                /* sprites del haz del ojo (md_cone.c): en los últimos lugares de la tabla */
uint8 md_spr_bank(const GenSpr *p) { return p != spr_bank[0]; }
volatile uint8 md_pub;                         /* 1: hay un cuadro listo que el vblank todavía no tomó */
static uint16 md_pub_frame;
volatile uint16 md_shown_frame;                /* el cuadro del motor que se ve (pruebas: tools/regress) */
static void md_sync(void) { while (md_pub); }  /* (ganchos que cambian lo del vblank: salas, cinemáticas) */

static uint16 cram[64];
static uint8 cine_stage;                        /* 0 juego, 1 Ceres explota, 2 Zebes, 3 menú de fin de juego (docs/35) */
static uint8 cram_dirty;
static uint16 cram_off[64];                   /* desplazamiento en bytes en palette_buffer de cada color (0xFFFF: negro) */
static uint8 cram_rev[256], cram_next[64];    /* índice de palette_buffer -> primer color de la CRAM que sale de él; siguiente */

static uint16 hud_shown[96];
static uint8 hud_dirty_all;                    /* todo el HUD otra vez (sala nueva) */
static uint16 hud_vram[96];                    /* filas ya convertidas a celdas de la Genesis (en md_frame) */
static uint8 hud_rows, hud_black;              /* filas por escribir en el vblank; fila 0 negra por escribir */

static const uint8 *samus_dma_src[2];
static const uint8 *samus_vram_src[2];          /* lo que hay ahora en la VRAM de Samus */
static uint16 samus_vram_n[2];
static uint16 samus_dma_tiles[2];

static uint16 scroll_ax, scroll_ay, scroll_bx, scroll_by;
extern uint8 pz_on;                           /* la pausa (md_pause.c) */
extern uint8 pz_dim;                          /* estados 12-18: md_post de la pausa (md_pause.c) */
void md_pause_frame(void);
/* pozo girado en la huida (modo 7 con B != 0): la Genesis no gira; se aproxima con scroll por línea (corrimiento
   horizontal que crece con la altura) y por columnas de 16 píxeles (corrimiento vertical que crece hacia los lados).
   Para giros chicos (en la huida, hasta ~8 grados) es casi lo mismo que girar. */
static uint8 rot_on, rot_shown;
/* pozo de modo 7 (g_room->wall): plano de 64x64; columnas 32-63 la pared, 0-31 el pozo en anillo (fila R del mapa
   del SNES en la fila R & 63). Quedan cargadas las filas ring_top..ring_top+63 alrededor de la cámara. */
#define RING_ABOVE 16
static int16 ring_top;
static int16 ring_q[64];
static uint16 ring_n;
static void ring_row_write(int16 R) {          /* con el VDP libre (vblank o pantalla apagada) */
  vdp_dma_vram(g_room->planeA + (uint16)(R & 127) * 32, MD_VRAM_PLANE_A + (uint16)(R & 63) * 128, 32);
}
static void ring_follow(void) {                 /* md_frame: filas que entran en la ventana -> cola */
  int16 want = ((int16)scroll_ay >> 3) - RING_ABOVE;
  if (want - ring_top >= 64 || ring_top - want >= 64) {
    ring_top = want; ring_n = 0;
    for (int16 i = 0; i < 64; i++) ring_q[ring_n++] = want + i;
    return;
  }
  while (ring_top < want && ring_n < 64) { ring_q[ring_n++] = ring_top + 64; ring_top++; }
  while (ring_top > want && ring_n < 64) { ring_top--; ring_q[ring_n++] = ring_top; }
}
static uint16 rot_h[224 * 2];                  /* tabla de scroll horizontal: plano A y B por línea */
static uint16 rot_v[16 * 2];                   /* scroll vertical por columna de 16 píxeles: A y B */
/* brillo (INIDISP del SNES) al final de la huida: la Genesis escala los colores */
static uint8 md_bright = 15, md_bright_shown = 15;
static uint16 cram_out[64];
static uint16 cram_t[64];                      /* la CRAM con el tinte rojo de la huida (md_tint) */
static uint8 md_tint, cram_t_ok;
static uint8 m7_variant, m7_variant_shown;
static uint8 door_seen;                       /* este cuadro el motor dibujó la puerta que en la Genesis es fondo */
uint8 flash_seen; static uint8 flash_shown;
static uint8 rl_pal_dirty = 1;                  /* la paleta cambió: rehacer la línea de Ridley */         /* este cuadro Ridley se dibujó con la paleta de parpadeo */
static const DynTiles *dyn_q[4], *dyn_ql[4];  /* tiles dinámicos a copiar en el próximo vblank (y los de la lógica) */
static uint8 dyn_n, dyn_nl;

/* ------------------------------------------------------------------ haze de Ceres ($88:DDC7) */
/* En el SNES, un objeto de HDMA suma azul (COLDATA, color math) a toda la imagen por bandas de 8 líneas desde la
 * línea 56: nivel de la banda i = hdma_table_2[i] (0 a 15). La Genesis no tiene color math, y cambiar todos los
 * colores a mitad de pantalla deja puntos en la imagen (escrituras a la CRAM fuera del borrado horizontal). Lo que
 * se reemplaza: el degradado en el color de fondo (todo lo negro de la estación), un color por banda, escrito por
 * la interrupción horizontal cada 8 líneas (líneas 55, 63, ...: la escritura cae en el borrado horizontal). */
enum { HZ_OFF, HZ_FADED_IN };
static uint8 hz_state, hz_red;
static uint8 hz_tab[16];                       /* nivel (0-31) por banda (hdma_table_2) */
/* por búfer: pares (color de fondo, orden "registro 10 = intervalo siguiente") para cada interrupción horizontal;
   solo hay interrupción donde el color cambia (más una al principio, ver hz_build_backdrop) */
static uint16 hz_bd[2][2 * 32];
static uint8 hz_first[2], hz_cnt[2];
static uint8 hz_back;
static const uint16 *volatile hz_bdp;

static uint8 hz_bd_dirty, hz_bd_ready;
/* el motor lleva el objeto de HDMA del haze (src/hdmaobj.c): aquí se lee su tabla (hdma_table_2: un byte por banda,
   nivel en los 5 bits bajos y el color en los altos: 0x80 azul, 0x20 rojo cuando Ridley ya se fue) */
static void md_haze_read(void) {
  uint8 on = 0;
#ifndef MD_NO_HAZE
  for (uint16 k = 0; k < 6; k++) {
    uint16 f = hdma_object_pre_instructions[k];
    if ((hdma_object_channels_bitmask[k] & 0xFF) && (hdma_objects_enable_flag & 0x8000) &&
        (f == FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_CeresRidleyAlive) ||
         f == FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_CeresRidleyDead) ||
         f == FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingIn) ||
         f == FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadedIn) ||
         f == FUNC16(HdmaobjPreInstr_HazeColorMathSubscreen_FadingOut)))
      on = 1;
  }
#endif
  if (cine_stage == 3) { on = 1; hz_red = 0; }  /* el degradado del menú de fin de juego usa la misma interrupción */
  uint8 st = on ? HZ_FADED_IN : HZ_OFF;
  if (st != hz_state) { hz_state = st; hz_bd_dirty = 1; }
  if (!on || cine_stage == 3) return;
  for (uint16 i = 0; i < 16; i++) {
    uint8 b = (uint8)(hdma_table_2[i >> 1] >> ((i & 1) * 8));
    if ((b & 31) != hz_tab[i]) { hz_tab[i] = b & 31; hz_bd_dirty = 1; }
    if (i == 0) {
      uint8 red = (b & 0xE0) == 0x20;
      if (red != hz_red) { hz_red = red; hz_bd_dirty = 1; }
    }
  }
}

/* El color de la banda k (desde la línea 8k+8) se escribe en una interrupción al final de la línea 8k+7, pero solo
 * en las bandas donde cambia (con 3 bits por canal, el degradado tiene pocos escalones). La interrupción i recarga
 * el contador con el registro 10 que hay en ese momento; el manejador i deja en el registro el intervalo de la
 * siguiente. La primera recarga usa el valor puesto en el vblank, que también fija la primera línea: por eso hay
 * una interrupción inicial en la línea D = (F0 - 1) / 2, con el mismo color de arriba. */
static uint16 hz_last_c = 0xFFFF;               /* color de fondo con el que se armó la tabla */
static void hz_build_backdrop(void) {
  uint16 c = palette_buffer[0];
  hz_last_c = c;
  uint16 *d = hz_bd[hz_back];
  uint16 fire[30], col[30], n = 0;
  uint16 prev = kCram555[c & 0x7FFF];
  for (uint16 k = 0; k < 29; k++) {
    int16 band = (int16)k - 6;
    uint16 L = band < 0 ? 0 : hz_tab[band > 15 ? 15 : band] & 31;
    uint16 v;
    if (cine_stage == 3) {                     /* menú de fin de juego: verde y azul sumados abajo (HDMA de $81:92A7) */
      static const uint8 kMenuTeal[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 9, 10, 11, 13 };
      uint16 t = kMenuTeal[k], g = ((c >> 5) & 31) + t, b = ((c >> 10) & 31) + t;
      v = (c & 31) | (g > 31 ? 31 : g) << 5 | (b > 31 ? 31 : b) << 10;
    } else if (hz_red) { uint16 r = (c & 31) + L; v = (c & ~31) | (r > 31 ? 31 : r); }
    else { uint16 b = ((c >> 10) & 31) + L; v = (c & 0x03FF) | ((b > 31 ? 31 : b) << 10); }
    v = kCram555[v & 0x7FFF];
    if (v != prev) { fire[n] = 8 * k + 7; col[n] = v; n++; prev = v; }
  }
  if (!n) { hz_cnt[hz_back] = 0; hz_bd_ready = 1; return; }
  uint16 D = (fire[0] - 1) >> 1;
  uint16 i = 0;
  /* interrupción inicial: color de arriba; intervalos: D, F0-D-1 (= D), F1-F0-1, ... */
  d[i++] = kCram555[c & 0x7FFF];
  d[i++] = 0x8A00 | (n > 1 ? fire[1] - fire[0] - 1 : 0xFF);
  for (uint16 j = 0; j < n; j++) {
    d[i++] = col[j];
    d[i++] = 0x8A00 | (j + 2 < n ? fire[j + 2] - fire[j + 1] - 1 : 0xFF);
  }
  hz_first[hz_back] = (uint8)D;
  hz_cnt[hz_back] = (uint8)(n + 1);
  hz_bd_ready = 1;
}

/* interrupción horizontal: el color de fondo de la banda siguiente y el intervalo hasta la próxima */
__asm__(
  "    .text\n"
  "    .global md_hint\n"
  "md_hint:\n"
  "    cmpi.b #0xE0,0xC00008\n"           /* aún en el vblank (interrupción pendiente de antes, al encenderlas): */
  "    bcc.s 1f\n"                         /* nada (si no, se comería la primera banda: docs/50) */
  "    move.l %a0,-(%sp)\n"
  "    movea.l hz_bdp,%a0\n"
  "    move.l #0xC0000000,0xC00004\n"
  "    move.w (%a0)+,0xC00000\n"
  "    move.w (%a0)+,0xC00004\n"
  "    move.l %a0,hz_bdp\n"
  "    movea.l (%sp)+,%a0\n"
  "1:  rte\n");

/* Zebes: debajo del HUD, los colores 1-15 de la línea 3 pasan a ser los de los enemigos de la sala y el rayo (en el
   vblank vuelven los del HUD). Una sola interrupción por cuadro: se apaga a sí misma. */
__asm__(
  "    .text\n"
  "    .global md_hint_obj\n"
  "md_hint_obj:\n"
  "    cmpi.b #0xE0,0xC00008\n"           /* aún en el vblank (interrupción pendiente de antes): nada */
  "    bcc.s 1f\n"
  "    move.l %a0,-(%sp)\n"
  "    lea zobj_col,%a0\n"
  "    move.l #0xC0620000,0xC00004\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.l (%a0)+,0xC00000\n"
  "    move.w (%a0)+,0xC00000\n"
  "    move.w #0x8004,0xC00004\n"
  "    movea.l (%sp)+,%a0\n"
  "1:  rte\n");
void md_hint_obj(void);
static uint8 zobj_hint;

/* matemática de color del SNES que la Genesis no tiene: un color que se suma (o resta) a todos los de la sala (no al
   HUD, que en el SNES es el BG3): niebla (FX 0xC: el color medio de su BG3, kFogW), neblina del Bomb Torizo
   (LayerBlendFunc 0x2C: COLDATA medio de su degradado) y las restas de color fijo (LayerBlendFunc_28/2A) */
static int8 tint_r, tint_g, tint_b;
static uint8 tint_on;
static inline uint16 tint15(uint16 c) {
  int16 r = (int16)(c & 31) + tint_r, g = (int16)((c >> 5) & 31) + tint_g, b = (int16)((c >> 10) & 31) + tint_b;
  r = r < 0 ? 0 : r > 31 ? 31 : r; g = g < 0 ? 0 : g > 31 ? 31 : g; b = b < 0 ? 0 : b > 31 ? 31 : b;
  return (uint16)(r | g << 5 | b << 10);
}
uint16 md_tint_cram(uint16 c) { return kCram555[tint_on ? tint15(c & 0x7FFF) : (c & 0x7FFF)]; }   /* md_zebes.c */

/* caja de mensaje (src/msgbox.c): sprites con los tiles de sus celdas (kMsgTiles, mkmd.py) en la línea 3; en las
   líneas de la caja la línea 3 vuelve a tener los colores del HUD (interrupciones: 31 los de los enemigos, 63 nada,
   arriba de la caja los del HUD, abajo de la caja otra vez los de los enemigos) */
typedef struct __attribute__((packed)) { const uint16 *col; uint16 reg; } MbStep;
static MbStep mb_steps[4];
static const MbStep *volatile mb_hp;
__asm__(
  "    .text\n"
  "    .global md_hint_msg\n"
  "md_hint_msg:\n"
  "    cmpi.b #0xE0,0xC00008\n"           /* aún en el vblank: nada */
  "    bcc.s 9f\n"
  "    movem.l %a0-%a1,-(%sp)\n"
  "    movea.l mb_hp,%a0\n"
  "    move.l (%a0)+,%a1\n"
  "    cmpa.w #0,%a1\n"
  "    beq.s 1f\n"
  "    move.l #0xC0620000,0xC00004\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.l (%a1)+,0xC00000\n"
  "    move.w (%a1)+,0xC00000\n"
  "1:  move.w (%a0)+,0xC00004\n"
  "    move.l %a0,mb_hp\n"
  "    movem.l (%sp)+,%a0-%a1\n"
  "9:  rte\n");
void md_hint_msg(void);
extern uint8 nt_msgbox_active, nt_msgbox_row0, nt_msgbox_rows;
extern uint16 zobj_col[];
extern GenSpr *sp_ptr;
void md_cone_room(void);
extern uint16 nt_msgbox_serial;
extern uint16 cinematic_text_tilemap[];
static uint8 mb_shown;                          /* la caja estaba en pantalla el cuadro anterior */
static uint16 mb_serial = 0xFFFF;               /* mapa con el que se armaron los tiles */
static uint8 mb_nspr, mb_prev_n;                /* sprites de la caja (al principio de la lista) */
static uint8 mb_c0, mb_w;                       /* columnas de la caja */
typedef struct { uint16 tile; uint8 col, row, w, h; } MbSpr;
static MbSpr mb_spr[24];
static uint8 mb_cells[32 * 6];                  /* tile de kMsgTiles de cada celda de la caja */
static uint16 mb_up_spr, mb_up_n;               /* subida de tiles: sprite y tiles que faltan */
static uint8 mb_buf[8 * 32] __attribute__((aligned(2)));   /* un sprite (hasta 4x2 tiles) por cuadro */
static uint16 mb_dma_dst, mb_dma_n;             /* para el vblank */
static uint16 mb_top, mb_bot;                   /* líneas de la caja a la vista (para las interrupciones) */
static uint16 msg_tile_of(uint16 w) {           /* kMsgCells: búsqueda binaria */
  w &= 0x7FFF;
  uint16 lo = 0, hi = MSG_CELL_COUNT;
  while (lo < hi) {
    uint16 m = (lo + hi) >> 1;
    if (kMsgCells[2 * m] < w) lo = m + 1; else hi = m;
  }
  return (lo < MSG_CELL_COUNT && kMsgCells[2 * lo] == w) ? kMsgCells[2 * lo + 1] : 0;
}
static void mb_layout(void) {
  uint16 rows = nt_msgbox_rows > 6 ? 6 : nt_msgbox_rows;
  /* columnas: las que no están vacías en alguna fila */
  uint8 c0 = 32, c1 = 0;
  for (uint16 r = 0; r < rows; r++)
    for (uint16 c = 0; c < 32; c++) {
      uint16 w = cinematic_text_tilemap[256 + r * 32 + c];
      uint16 t = msg_tile_of(w);
      mb_cells[r * 32 + c] = (uint8)t;
      if (t) { if (c < c0) c0 = (uint8)c; if (c > c1) c1 = (uint8)c; }
    }
  mb_nspr = 0;
  if (c0 > c1) return;
  mb_c0 = c0; mb_w = (uint8)(c1 - c0 + 1);
  uint8 ch = (rows & 1) ? 1 : 2;                /* filas de 2 (6 filas) o de 1 (3 filas): se abren desde el medio */
  /* tiles: 0xF000-0xFFFF (128) y las filas de la ventana que no usa el HUD (5-31: 54) */
  uint16 r0 = 1920, r0n = 128, r1 = 1802, r1n = 54;
  for (uint16 r = 0; r < rows; r += ch)
    for (uint16 c = 0; c < mb_w && mb_nspr < 24; c += 4) {
      uint8 w = (uint8)(mb_w - c > 4 ? 4 : mb_w - c);
      uint16 n = w * ch, t;
      if (r0n >= n) { t = r0; r0 += n; r0n -= n; }
      else if (r1n >= n) { t = r1; r1 += n; r1n -= n; }
      else continue;
      MbSpr *m = &mb_spr[mb_nspr++];
      m->tile = t; m->col = (uint8)(c0 + c); m->row = (uint8)r; m->w = w; m->h = ch;
    }
  mb_up_spr = 0;
  mb_up_n = 0;
}
/* tiles de un sprite al búfer (columna por columna, como los lee el VDP) */
static void mb_upload_step(void) {
  if (mb_up_spr >= mb_nspr) return;
  MbSpr *m = &mb_spr[mb_up_spr];
  uint16 n = 0;
  for (uint16 x = 0; x < m->w; x++)
    for (uint16 y = 0; y < m->h; y++) {
      const uint16 *src = (const uint16 *)(kMsgTiles + mb_cells[(m->row + y) * 32 + m->col + x] * 32);
      uint16 *d = (uint16 *)(mb_buf + n * 32);
      for (uint16 i = 0; i < 16; i++) d[i] = src[i];
      n++;
    }
  mb_dma_dst = m->tile * 32;
  mb_dma_n = n * 16;
  mb_up_spr++;
}
uint8 mb_ready(uint16 i) { return i < mb_up_spr && !(i + 1 == mb_up_spr && mb_dma_n); }
/* InitializePpuForMessageBoxes: la CGRAM 25 = 0x0BB1 y 26 = 0x001F mientras está la caja: en las líneas de la caja,
   los colores del HUD con esos dos cambiados (MB_SLOT25/26, mkmd.py) */
static uint16 mb_col[16];
static void mb_cram_override(void) {
  if (!nt_msgbox_active) return;
  for (uint16 i = 0; i < 15; i++) mb_col[i] = cram[49 + i];
  mb_col[MB_SLOT25 - 1] = kCram555[0x0BB1];
  mb_col[MB_SLOT26 - 1] = kCram555[0x001F];
}
/* md_post: los sprites de la caja delante de los de la lista */
static void md_msgbox_post(void) {
  extern uint16 message_box_animation_y_radius;
  if (!nt_msgbox_active) {
    if (mb_shown) {
      mb_shown = 0;
      md_cone_room();                             /* los tiles de 0xF000 eran del haz: se vuelven a cargar */
    }
    mb_prev_n = 0;
    mb_top = mb_bot = 0;
    return;
  }
  mb_shown = 1;
  if (nt_msgbox_serial != mb_serial) { mb_serial = nt_msgbox_serial; mb_layout(); }
  if (!mb_dma_n) mb_upload_step();                /* un sprite por cuadro (el vblank lo copia) */
  uint16 rows = nt_msgbox_rows > 6 ? 6 : nt_msgbox_rows;
  int16 R = (int16)(message_box_animation_y_radius >> 8);
  int16 y0 = nt_msgbox_row0 * 8, mid = y0 + rows * 4;
  GenSpr box[24];
  uint16 nb = 0;
  mb_top = 0xFFFF; mb_bot = 0;
  for (uint16 i = 0; i < mb_nspr; i++) {
    MbSpr *m = &mb_spr[i];
    if (!mb_ready(i)) continue;
    int16 ya = y0 + m->row * 8, yb = ya + m->h * 8;
    if (!(ya < mid + R && yb > mid - R)) continue;   /* todavía cerrada en esas filas */
    GenSpr *g = &box[nb++];
    g->y = ya + 128;
    g->size_link = (uint16)(((m->w - 1) << 10) | ((m->h - 1) << 8));
    g->attr = (uint16)(0x8000 | (3 << 13) | m->tile);
    g->x = m->col * 8 + 128;
    if ((uint16)ya < mb_top) mb_top = ya;
    if ((uint16)yb > mb_bot) mb_bot = yb;
  }
  /* la lista: la caja, luego la del juego (en los cuadros sin actualización, la anterior sin su caja) */
  uint16 ng = (uint16)(sp_ptr - spr);
  if (ng + nb > MAX_SPR) ng = MAX_SPR - nb;
  for (int16 i = ng - 1; i >= 0; i--) spr[i + nb] = spr[i];
  for (uint16 i = 0; i < nb; i++) spr[i] = box[i];
  sp_ptr = spr + ng + nb;
  for (uint16 i = 0; i < ng + nb; i++) spr[i].size_link = (spr[i].size_link & 0x0F00) | (uint16)(i + 1);
  mb_prev_n = (uint8)nb;
  if (mb_bot <= mb_top) mb_top = mb_bot = 0;
}

/* círculo de la explosión de Ceres (ver "cinemáticas y menús") */
#define CIRC_T0 1               /* 2 tiles: lleno y triángulo (los del HUD: no se ve en las cinemáticas) */
static uint8 cv_line;           /* el círculo se ve este cuadro (scroll por línea e interrupción horizontal) */
static uint8 cv_build, cv_cur;  /* tabla que se arma / la que se muestra */
static uint16 cv_tab[2][232];   /* scroll vertical del plano B por línea (el de g + 1 lo escribe la interrupción de g) */
static uint16 cv_ha = 0xFFFF;
static uint16 cv_key[3];        /* radio, x y centro con que se armó la tabla */
static uint8 cv_new;            /* tabla nueva en este cuadro */   /* scroll horizontal del plano A ya puesto en la tabla por línea */
static uint16 cv_code[9];       /* la interrupción, en RAM: copia una palabra de la tabla y avanza la dirección */
static uint8 cv_active;         /* interrupción del círculo encendida */

void md_hint(void);
void SYS_setHIntCallback(void (*cb)(void));
static void cv_point(uint16 line) {   /* la interrupción de la línea "line" escribe cv_tab[cv_cur][line] */
  uint32 a = (uint32)&cv_tab[cv_cur][line < 224 ? line + 1 : 1];
  cv_code[1] = (uint16)(a >> 16);
  cv_code[2] = (uint16)a;
}

/* inicio del vblank (interrupción vertical de SGDK): el color de fondo vuelve al de arriba */
static volatile uint8 md_vdp_busy;
static void md_vint_top(void) {
  if (zobj_hint == 2) mb_hp = mb_steps;
  if (zobj_hint && !md_vdp_busy) {             /* Zebes: aunque el cuadro se atrase, el HUD con sus colores arriba */
    VDP_CTRL32 = VDP_CMD_CRAM_W(0x62);
    for (uint16 i = 0; i < 15; i++) VDP_DATA = cram[49 + i];
    vdp_reg(0x0A, 31);   /* cambio de colores en la línea 32 (la primera del juego): no en el HUD */
    vdp_reg(0x00, 0x14);
  }
  if (cv_active && !md_vdp_busy) {           /* círculo: si el cuadro se atrasa, la misma tabla desde arriba */
    cv_point(0);
    *(volatile uint32_t *)0xC00004 = 0x40020010u;
    *(volatile uint16_t *)0xC00000 = cv_tab[cv_cur][0];
    return;
  }
  if (md_vdp_busy || hz_state == HZ_OFF || md_bright != 15) return;
  *(volatile uint32_t *)0xC00004 = 0xC0000000u;
  *(volatile uint16_t *)0xC00000 = cram[0];
  hz_bdp = hz_bd[hz_back ^ 1];
  /* y la primera interrupción: la última del cuadro dejó el registro 10 en 0xFF (sin más interrupciones), así que un
     cuadro que se ve dos veces salía la segunda vez sin el degradado (docs/50) */
  if (hz_cnt[hz_back ^ 1]) vdp_reg(0x0A, hz_first[hz_back ^ 1]);
}

/* el vblank del cuadro publicado: con la página del nivel puesta (la lógica puede estar en medio de otra) */
void md_vblank(uint16_t late);
#ifdef MD_LAGLOG
#ifndef LAG_BASE
#define LAG_BASE 0
#endif
static uint32 lag_last;
#endif
extern volatile uint32 vtimer;                 /* SGDK: sube al empezar cada vblank */
static uint32 md_vb_tick = 0xFFFFFFFF;          /* vblank en el que se tomó el último cuadro */
extern volatile uint16 md_snd_frames;          /* md_snd.c: el Z80 está tocando una muestra de la ROM */
#ifdef MD_M7LOG
volatile uint16 md_m7_start;                   /* (medición) cómo empezó el último vblank: tarde << 8 | línea */
#endif
/* el trabajo del vblank, con la página del nivel puesta y, con una muestra sonando, el bus del Z80 (que lee la ROM
   por el bus del 68000) pedido durante las DMA. Devuelve lo que vb_exit repone. */
static uint16 vb_enter(uint8 snd) {
  uint8 z80 = 0;
  if (snd && (*(volatile uint16 *)0xA11100 & 0x0100)) {  /* (si nadie lo tiene ya) */
    *(volatile uint16 *)0xA11100 = 0x0100;
    while (*(volatile uint16 *)0xA11100 & 0x0100);
    z80 = 1;
  }
  uint16 sv = (uint16)md_cur_page << 8 | md_hw_page | (z80 ? 0x8000 : 0);
  md_cur_page = md_level_page;
  md_hw_page = md_level_page;
  *(volatile uint8 *)0xA130FF = md_level_page;
  __asm__ volatile("" ::: "memory");
  return sv;
}
static void vb_exit(uint16 sv) {
  __asm__ volatile("" ::: "memory");
  md_cur_page = (sv >> 8) & 0x7F;
  md_hw_page = (uint8)sv;
  *(volatile uint8 *)0xA130FF = md_hw_page;
  if (sv & 0x8000) *(volatile uint16 *)0xA11100 = 0x0000;
}
static void md_vblank_lag(void);
static int16 vb_lines_left(void);
/* líneas que necesita el vblank de un cuadro (lo que tardaron los últimos, con margen): un cuadro que llega con el
   vblank empezado se toma solo si quedan tantas; si no, en el vblank siguiente (docs/50). Si el vblank se pasara a
   la imagen, sus DMA irían a la velocidad de la imagen y las interrupciones horizontales empezarían tarde. */
static int16 md_vb_need = 20;
static uint8 md_vb_room;                       /* este vblank cargó una sala (con la pantalla apagada: no cuenta) */
static void md_vblank_run(uint16 late) {
  md_vb_tick = vtimer;
  int16 l0 = vb_lines_left();
#ifdef MD_M7LOG
  md_m7_start = late << 8 | VDP_VCOUNT >> 8;
#endif
  uint8 snd = md_snd_frames != 0;
  if (snd) md_snd_frames--;
  uint16 sv = vb_enter(snd);
  md_vb_room = 0;
  md_vblank(late);
  vb_exit(sv);
  if (!md_vb_room) {
    int16 l1 = vb_lines_left();
    int16 used = l1 > 0 ? l0 - l1 + 2 : l0 + 4;     /* (si se pasó a la imagen, más de lo que había) */
    md_vb_need = used > md_vb_need - 1 ? used : md_vb_need - 1;
    if (md_vb_need < 8) md_vb_need = 8;
    if (md_vb_need > 36) md_vb_need = 36;
  }
#ifdef MD_LAGLOG
  { uint16 k = (uint16)(md_pub_frame - 1 - LAG_BASE); md_lag_log[k < MD_LAG_N ? k : MD_LAG_N - 1] = (uint8)(vtimer - lag_last); lag_last = vtimer; }
#endif
  md_shown_frame = md_pub_frame;
  md_pub = 0;
}

/* interrupción vertical: si hay un cuadro listo, su vblank; si no, lo que se puede adelantar (md_vblank_lag) */
void md_vint(void) {
  if (!md_pub) md_vblank_lag();
  md_vint_top();
  if (md_pub) md_vblank_run(0);
}

#ifndef LATE_MAX
#define LATE_MAX 0xEA
#endif
/* (fuera de las interrupciones) el cuadro está listo: lo toma el próximo vblank. Si el vblank ya empezó sin cuadro
   (el anterior tardó) y todavía queda lo que necesita (md_vb_need), se escribe ya */
void md_publish(void) {
  spr_out = spr; nspr_out = nspr; ncone_out = ncone;
  spr = spr == spr_bank[0] ? spr_bank[1] : spr_bank[0];
  spr_end = spr + MAX_SPR;
  md_pub_frame = md_frame_count;
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  md_pub = 1;
  uint16 vc = VDP_VCOUNT >> 8;
  if ((*(volatile uint16 *)0xC00004 & 8) && vc >= 0xE0 && vc <= LATE_MAX && vtimer != md_vb_tick &&
      vb_lines_left() >= md_vb_need)
    md_vblank_run(1);                           /* (y si este vblank todavía no tomó un cuadro y alcanza) */
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
}
uint8 md_published(void) { return md_pub; }
void md_cone_idle(void);
void md_wait_free(void) { while (md_pub) md_cone_idle(); }   /* lo que sobra del cuadro: el armado del haz (md_cone.c) */

/* ------------------------------------------------------------------ motor: avisos */
volatile uint32 md_unimpl_val;                 /* el valor del último aviso (dirección de la función que falta) */
void nt_unimpl(const char *what) {
  extern uint32 nt_unimpl_val;
  md_unimpl_count++;
  md_unimpl_msg = what;
  md_unimpl_val = nt_unimpl_val;
}

/* sonido: todavía no (la cola del motor ya está; falta el driver de la Genesis) */
void sfx_play(uint16 channel, uint8 id) { (void)channel; (void)id; }
void hud_minimap_update(uint16 a, uint16 b, uint16 c) { (void)a; (void)b; (void)c; }
void samus_tiles_changed(void) {}
/* pendientes en la Genesis (Zebes): tiles animados y efectos de ambiente (pisadas, polvo) */
void DrawAtmosphericGfx(uint16 type, uint16 frame, int16 x, int16 y) { (void)type; (void)frame; (void)x; (void)y; }

/* ------------------------------------------------------------------ sprites */
GenSpr *sp_ptr = spr_bank[0];                       /* próximo sprite libre */

#ifdef MD_SPRSUM                                /* prueba: suma de la lista de sprites de 256 cuadros desde MD_SPRSUM */
volatile uint16 md_spr_sum[256];
#endif
static void put_recs(const uint16 *r, int16 x, int16 y, uint8 clip_y) {
  uint16 n = *r++;
  GenSpr *s = sp_ptr;
  int16 ylo = clip_y ? 0 : -31;
  /* todo en pantalla y con lugar en la lista: se copia sin mirar cada sprite */
  if (y + (int16)r[0] >= ylo && y + (int16)r[1] < 224 && x + (int16)r[2] >= -31 && x + (int16)r[3] < 256 &&
      s + n <= spr + MAX_SPR) {
    r += 4;
    uint16 yb = y + 129, xb = x + 128;        /* 2 líneas más abajo que el SNES, como los planos */
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
  uint16 yrange = clip_y ? 224 : 224 + 31, yoff = clip_y ? 0 : 31;   /* y válida: [0,224) o (-32,224) */
  for (; n; n--, r += 4) {
    int16 sy = y + (int16)r[0];
    if ((uint16)(sy + yoff) >= yrange) continue;
    int16 sx = x + (int16)r[3];
    if ((uint16)(sx + 31) >= 256 + 31) continue;
    if (s == spr + MAX_SPR) break;
    s->y = sy + 129;
    s->size_link = r[1] | (uint16)(s - spr + 1);   /* enlace al siguiente (el último se corrige en el vblank) */
    s->attr = r[2];
    s->x = sx + 128;
    s++;
  }
  sp_ptr = s;
}


/* Dibujo de un spritemap por su clave, a mano en el 68000: la búsqueda en la tabla de dispersión y la copia a la
   lista de sprites en una sola rutina con todo en registros (en C cada dibujo costaba ~1800 ciclos, 30 líneas
   por cuadro en la pelea con Ridley). Entrada: d0 = a<<16|b, d1 = c<<16|tipo, a0 = x, a1 = y (en pantalla).
   Salida: d0 = 1 si la clave existe. Solo cambia d0, d1, a0, a1. El tipo 3 recorta arriba en la línea 0. */
const SprKey *md_keys; const uint16 *md_khash; uint16 md_kmask;
uint32_t md_kcache[64][4];                     /* clave (2 largos), registros, parpadeo */
static const uint16 kKindMul[16] __attribute__((used)) = { 0, 0x9E, 0x13C, 0x1DA, 0x278, 0x316, 0x3B4, 0x452, 0x4F0,
  0x58E, 0x62C, 0x6CA, 0x768, 0x806, 0x8A4, 0x942 };
__asm__(
  "    .text\n"
  "    .global md_drawkey\n"
  "md_drawkey:\n"
  "    movem.l %d2-%d7/%a2-%a4,-(%sp)\n"
  "    move.w %a0,%d6\n"                      /* x */
  "    move.w %a1,%d7\n"                      /* y */
  "    move.w %d1,%d3\n"                      /* tipo (también para el recorte) */
  /* memoria de 64 claves recientes (la cola de Ridley repite las mismas): sin buscar en la tabla */
  "    move.l %d0,%d5\n"
  "    swap %d5\n"                          /* a */
  "    eor.w %d0,%d5\n"                     /* ^ b */
  "    eor.w %d1,%d5\n"                     /* ^ tipo */
  "    move.w %d5,%d4\n"
  "    lsr.w #6,%d4\n"
  "    eor.w %d4,%d5\n"
  "    and.w #63,%d5\n"
  "    lsl.w #4,%d5\n"
  "    lea md_kcache,%a1\n"
  "    add.w %d5,%a1\n"
#ifdef MD_NOKCACHE
  "    bra 20f\n"
#endif
  "    cmp.l (%a1),%d0\n"
  "    bne 20f\n"
  "    cmp.l 4(%a1),%d1\n"
  "    bne 20f\n"
  "    movea.l 8(%a1),%a0\n"
  "    cmpa.w #0,%a0\n"
  "    beq 9f\n"
  "    tst.b 12(%a1)\n"                      /* el tipo de la clave: bit 7 = parpadeo */
  "    bpl 5f\n"
  "    move.b #1,flash_seen\n"
  "    bra 5f\n"
  "20: move.l %d0,(%a1)\n"                   /* se anota la clave; el resto si se encuentra */
  "    move.l %d1,4(%a1)\n"
  "    clr.l 8(%a1)\n"
  "    move.w %d0,%d2\n"                      /* b */
  "    swap %d0\n"                            /* a */
  "    swap %d1\n"                            /* c */
  "    move.w %d0,%d4\n"
  "    lsr.w #5,%d4\n"
  "    eor.w %d0,%d4\n"
  "    move.w %d2,%d5\n"
  "    lsr.w #8,%d5\n"
  "    eor.w %d5,%d4\n"
  "    eor.w %d2,%d4\n"
  "    eor.w %d1,%d4\n"
  "    move.w %d3,%d5\n"
  "    add.w %d5,%d5\n"
  "    lea kKindMul,%a2\n"
  "    move.w (%a2,%d5.w),%d5\n"
  "    eor.w %d5,%d4\n"
  "    movea.l md_keys,%a3\n"
  "    movea.l md_khash,%a4\n"
  "1:  and.w md_kmask,%d4\n"
  "    moveq #0,%d5\n"
  "    move.w %d4,%d5\n"
  "    add.l %d5,%d5\n"
  "    move.w (%a4,%d5.l),%d5\n"             /* índice de la clave (0xFFFF: no está) */
  "    bmi 9f\n"
  "    move.l %d5,%a2\n"
  "    add.l %d5,%d5\n"
  "    add.l %d5,%d5\n"
  "    add.l %a2,%d5\n"
  "    add.l %d5,%d5\n"                       /* *10 */
  "    lea (%a3,%d5.l),%a2\n"
  "    cmp.w 2(%a2),%d0\n"
  "    bne 2f\n"
  "    cmp.w 4(%a2),%d2\n"
  "    bne 2f\n"
  "    cmp.w 6(%a2),%d1\n"
  "    bne 2f\n"
  "    move.b (%a2),%d5\n"
  "    bclr #7,%d5\n"
  "    cmp.b %d3,%d5\n"
  "    beq 3f\n"
  "2:  addq.w #1,%d4\n"
  "    bra 1b\n"
  "9:  moveq #0,%d0\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a4\n"
  "    rts\n"
  /* encontrada */
  "3:  move.b (%a2),12(%a1)\n"                /* bit 7: parpadeo */
  "    tst.b (%a2)\n"
  "    bpl 4f\n"
  "    move.b #1,flash_seen\n"
  "4:  moveq #0,%d5\n"
  "    move.w 8(%a2),%d5\n"
  "    add.l %d5,%d5\n"
  "    lea kObjRecs,%a0\n"
  "    add.l %d5,%a0\n"
  "    move.l %a0,8(%a1)\n"
  "5:  move.w (%a0)+,%d5\n"                   /* n */
  "    beq 8f\n"
  "    movea.l sp_ptr,%a1\n"
  "    moveq #-31,%d4\n"                      /* y mínima */
  "    cmp.b #3,%d3\n"
  "    bne 5f\n"
  "    moveq #0,%d4\n"
  "5:  move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    cmp.w %d4,%d0\n"
  "    blt 20f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    cmp.w #224,%d0\n"
  "    bge 21f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    cmp.w #-31,%d0\n"
  "    blt 22f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    cmp.w #256,%d0\n"
  "    bge 23f\n"
  "    move.w %d5,%d0\n"
  "    lsl.w #3,%d0\n"
  "    lea (%a1,%d0.w),%a2\n"
  "    cmpa.l spr_end,%a2\n"                 /* MAX_SPR * 8 */
  "    bhi 23f\n"
  /* todo en pantalla: copia directa */
  "    add.w #129,%d7\n"
  "    add.w #128,%d6\n"
  "    move.l %a1,%d2\n"
  "    sub.l spr,%d2\n"
  "    lsr.w #3,%d2\n"
  "    addq.w #1,%d2\n"                       /* enlace al siguiente */
  "    subq.w #1,%d5\n"
  "6:  move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w (%a0)+,%d0\n"
  "    or.w %d2,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    addq.w #1,%d2\n"
  "    move.w (%a0)+,(%a1)+\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    dbra %d5,6b\n"
  "    move.l %a1,sp_ptr\n"
  "8:  moveq #1,%d0\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a4\n"
  "    rts\n"
  /* recorte sprite a sprite (a0 en el primer registro) */
  "20: addq.l #2,%a0\n"
  "21: addq.l #2,%a0\n"
  "22: addq.l #2,%a0\n"
  "23: move.l spr_end,%a3\n"
  "    neg.w %d4\n"                           /* 0 o 31 */
  "    move.w #224,%d3\n"
  "    add.w %d4,%d3\n"                       /* rango de y válida */
  "    move.l %a1,%d2\n"
  "    sub.l spr,%d2\n"
  "    lsr.w #3,%d2\n"
  "    addq.w #1,%d2\n"
  "    subq.w #1,%d5\n"
  "10: move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    move.w %d0,%d1\n"
  "    add.w %d4,%d1\n"
  "    cmp.w %d3,%d1\n"
  "    bcc 11f\n"
  "    move.w 4(%a0),%d1\n"
  "    add.w %d6,%d1\n"
  "    move.w %d1,%a2\n"
  "    add.w #31,%d1\n"
  "    cmp.w #287,%d1\n"
  "    bcc 11f\n"
  "    cmpa.l %a3,%a1\n"
  "    beq 12f\n"
  "    add.w #129,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w (%a0),%d0\n"
  "    or.w %d2,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    addq.w #1,%d2\n"
  "    move.w 2(%a0),(%a1)+\n"
  "    move.w %a2,%d0\n"
  "    add.w #128,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "11: addq.l #6,%a0\n"
  "    dbra %d5,10b\n"
  "12: move.l %a1,sp_ptr\n"
  "    moveq #1,%d0\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a4\n"
  "    rts\n");

static inline __attribute__((always_inline)) const uint16 *find_key_q(uint8 kind, uint16 a, uint16 b, uint16 c);
static void put_recs(const uint16 *r, int16 x, int16 y, uint8 clip_y);
static inline __attribute__((always_inline)) uint16 drawkey(uint16 kind, uint16 a, uint16 b, uint16 c, int16 x, int16 y) {
#ifdef MD_DKCOUNT
  { extern volatile uint16 md_dk_calls[16]; md_dk_calls[kind & 15]++; }
#endif
#ifdef MD_C_SPR
  const uint16 *r = find_key_q(kind, a, b, c);
  if (!r) return 0;
  put_recs(r, x, y, kind == 3);
  return 1;
#endif
  register uint32_t r0 __asm__("d0") = ((uint32_t)a << 16) | b;
  register uint32_t r1 __asm__("d1") = ((uint32_t)c << 16) | kind;
  register int32_t r8 __asm__("a0") = x;
  register int32_t r9 __asm__("a1") = y;
  __asm__ volatile("jsr md_drawkey" : "+d"(r0), "+d"(r1), "+a"(r8), "+a"(r9) : : "cc", "memory");
  return (uint16)r0;
}

uint16 md_missing_ring[32][4];                  /* pruebas: las últimas claves que faltaron (distintas) */
uint8 md_missing_rpos;
static void missing(uint16 kind, uint16 a, uint16 b, uint16 c) {
  if (!md_missing) { md_missing_key[0] = kind; md_missing_key[1] = a; md_missing_key[2] = b; md_missing_key[3] = c; }
  md_missing++;
  for (uint8 i = 0; i < 32; i++)
    if (md_missing_ring[i][0] == kind && md_missing_ring[i][1] == a && md_missing_ring[i][2] == b && md_missing_ring[i][3] == c) return;
  uint16 *r = md_missing_ring[md_missing_rpos++ & 31];
  r[0] = kind; r[1] = a; r[2] = b; r[3] = c;
}

/* claves en una tabla de dispersión armada en el PC (key_hash de mkmd.py) */
static inline __attribute__((always_inline)) const uint16 *find_key_q(uint8 kind, uint16 a, uint16 b, uint16 c) {
  const SprKey *keys = g_room->keys;
  const uint16 *ht = g_room->khash;
  uint16 mask = g_room->khash_mask;
  uint16 h = (a ^ (a >> 5) ^ (b >> 8) ^ b ^ c ^ (uint16)(kind * 0x9E)) & mask;
  for (;;) {
    uint16 i = ht[h];
    if (i == 0xFFFF) break;
    const SprKey *k = &keys[i];
    if (k->a == a && k->b == b && k->c == c && (k->kind & 0x7F) == kind) {
      if (k->kind & 0x80) flash_seen = 1;      /* Ridley con la paleta 0: misma imagen, la línea cambia de colores */
      return kObjRecs + k->rec;
    }
    h = (h + 1) & mask;
  }
  return 0;
}

static inline __attribute__((always_inline)) const uint16 *find_key(uint8 kind, uint16 a, uint16 b, uint16 c) {
  const uint16 *r = find_key_q(kind, a, b, c);
  if (!r) missing(kind, a, b, c);
  return r;
}

#include "md_intro.c"                         /* la intro (docs/38): usa la lista de sprites y la CRAM */

void DrawSamusSpritemap(uint16 idx, uint16 x, uint16 y) {  // 0x8189AE
  /* Al aterrizar ($90:8CC3) el SNES lee una palabra de una tabla de bytes y la y queda con basura en el byte alto;
     allí no importa porque la y del sprite es de 8 bits. Aquí se toma igual solo el byte bajo cuando la y es absurda
     (si no, Samus desaparecía un cuadro al tocar el suelo). */
  if ((int16)y < -128 || (int16)y > 384) y &= 0xFF;
  if (idx >= SAMUS_SPRITEMAP_COUNT) { missing(9, idx, 0, 0); return; }
  uint16 o = kSamusSprIndex[idx];
  if (o == 0xFFFF) { missing(6, idx, 0, 0); return; }
  put_recs(kSamusSprRecs + o, (int16)x, (int16)y, 0);
}

void DrawSamusFrontExtra(void) {  // 0x90868D: un sprite más con Samus de frente
  if (!drawkey(5, 0, 0, 0, (int16)(samus_x_pos - 7 - layer1_x_pos), (int16)(samus_y_pos - 17 - layer1_y_pos))) missing(5, 0, 0, 0);
}

void md_put_recs(const uint16 *r, int16 x, int16 y) {   /* md_putrecs (asm, más abajo) */
  register int32_t d0 __asm__("d0") = x;
  register int32_t d1 __asm__("d1") = y;
  register const uint16 *a0 __asm__("a0") = r;
  __asm__ volatile("jsr md_putrecs" : "+d"(d0), "+d"(d1), "+a"(a0) : : "a1", "cc", "memory");
}

/* destello de un enemigo herido (WriteEnemyOams: paleta 0 del SNES, casi blanca): en Zebes la misma imagen con la
   línea de colores más clara de la Genesis (la línea 3 es la de todos los objetos) */
static uint8 fl_line = 0xFF;                   /* la de la sala (se calcula al primer destello) */
static uint8 flash_line(void) {
  if (fl_line != 0xFF) return fl_line;
  uint16 best = 0, bl = 0;
  for (uint16 l = 0; l < 3; l++) {
    uint16 sum = 0;
    for (uint16 c = 1; c < 16; c++) { uint16 v = cram[l * 16 + c]; sum += (v & 0xE) + ((v >> 4) & 0xE) + ((v >> 8) & 0xE); }
    if (sum > best) { best = sum; bl = l; }
  }
  return fl_line = (uint8)bl;
}

void DrawEnemySpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) {  // 0x818AB8
  (void)bank;
  if (md_zebes && !pal) {
    uint16 base = gEnemyData(cur_enemy_index)->palette_index;
    GenSpr *s0 = sp_ptr;
    if (base && zebes_obj_draw(sm, base, tiles, (int16)x, (int16)y)) {
      uint16 a = (uint16)flash_line() << 13;
      for (GenSpr *q = s0; q < sp_ptr; q++) q->attr = (q->attr & 0x9FFF) | a;
      return;
    }
  }
  if (md_zebes && zebes_obj_draw(sm, pal, tiles, (int16)x, (int16)y)) return;   /* enemigos de Zebes */
  if (!drawkey(1, sm, pal, tiles, (int16)x, (int16)y)) missing(1, sm, pal, tiles);
}

/* WriteEnemyOams ($A0:944A) con un spritemap extendido: si la sala lo tiene preparado entero (Ridley: sus piezas
   ya juntadas en sprites grandes), se dibuja de una vez; si no, el motor dibuja cada spritemap */
uint8 DrawEnemyExtSpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) {
  (void)bank;
  return drawkey(7, sm, pal, tiles, (int16)x, (int16)y);
}

/* objetos de sprite ($B4:BD32): explosiones sueltas; tipo 10 de las claves de sprites */
void DrawSpriteObjSpritemap(uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) {
  if (md_zebes && zebes_obj_draw(sm, pal, tiles | 0x4000, (int16)x, (int16)y)) return;   /* Zebes (mkzobj.py) */
  if (!drawkey(10, sm, pal, tiles, (int16)x, (int16)y)) missing(10, sm, pal, tiles);
}

void DrawEprojSpritemap(uint16 sm, uint16 x, uint16 y, uint16 g) {  // 0x818C0A
  if (md_zebes && zebes_obj_draw(sm, g, 0x8000, (int16)x, (int16)y)) return;
  if (!drawkey(2, sm, g, 0, (int16)x, (int16)y)) missing(2, sm, g, 0);
}

/* DrawLowPriorityEprojs / DrawHighPriorityEprojs ($86:8390/83B2) enteros en la Genesis: el recorrido de los 18
   proyectiles, la prueba de pantalla del SNES y el dibujo con los registros de cada uno recordados por lugar (el
   spritemap de un proyectil cambia poco): sin búsqueda en la tabla ni llamadas por proyectil. bx/by = temblor -
   cámara. La lista de sprites que sale es la misma que con DrawEprojSpritemap. */
struct EpCache { uint16 sm, g; const uint16 *rec; };
struct EpCache ep_cache[18] __attribute__((used));
/* proyectil v (índice*2) sin registros recordados: se buscan en la tabla de la sala (0: no está) */
const uint16 *zebes_obj_find(uint16 sm, uint16 pal, uint16 tiles);
const uint16 *zebes_obj_find_cached(uint16 sm, uint16 pal, uint16 tiles);
uint8 zebes_obj_page(void);
static uint8 ep_zebes;                          /* md_draw_eprojs en Zebes: claves de los objetos de la sala */
uint32 ep_lookup(uint16 v2, uint16 sm, uint16 g) {
  if (sm == 0x8000) return 0;                   /* $8D:8000: spritemap vacío (el proyectil aún sin instrucción) */
  const uint16 *r = ep_zebes ? zebes_obj_find_cached(sm, g, 0x8000) : find_key_q(2, sm, g, 0);
  if (!r) { missing(2, sm, g, 0); return 0; }
  struct EpCache *c = &ep_cache[v2 >> 1];
  c->sm = sm; c->g = g; c->rec = r;
  return (uint32)r;
}
/* md_putrecs: los registros de sprites de a0 en x = d0, y = d1 (en pantalla), como el final de md_drawkey.
   Conserva todo menos d0, d1, a0, a1. */
__asm__(
  "    .text\n"
  "    .global md_putrecs\n"
  "md_putrecs:\n"
  "    movem.l %d2-%d7/%a2-%a3,-(%sp)\n"
  "    move.w %d0,%d6\n"
  "    move.w %d1,%d7\n"
  "    move.w (%a0)+,%d5\n"                   /* n */
  "    beq 8f\n"
  "    movea.l sp_ptr,%a1\n"
  "    moveq #-31,%d4\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    cmp.w %d4,%d0\n"
  "    blt 20f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    cmp.w #224,%d0\n"
  "    bge 21f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    cmp.w #-31,%d0\n"
  "    blt 22f\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    cmp.w #256,%d0\n"
  "    bge 23f\n"
  "    move.w %d5,%d0\n"
  "    lsl.w #3,%d0\n"
  "    lea (%a1,%d0.w),%a2\n"
  "    cmpa.l spr_end,%a2\n"
  "    bhi 23f\n"
  "    add.w #129,%d7\n"
  "    add.w #128,%d6\n"
  "    move.l %a1,%d2\n"
  "    sub.l spr,%d2\n"
  "    lsr.w #3,%d2\n"
  "    addq.w #1,%d2\n"
  "    subq.w #1,%d5\n"
  "6:  move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w (%a0)+,%d0\n"
  "    or.w %d2,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    addq.w #1,%d2\n"
  "    move.w (%a0)+,(%a1)+\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    dbra %d5,6b\n"
  "    move.l %a1,sp_ptr\n"
  "8:  movem.l (%sp)+,%d2-%d7/%a2-%a3\n"
  "    rts\n"
  "20: addq.l #2,%a0\n"
  "21: addq.l #2,%a0\n"
  "22: addq.l #2,%a0\n"
  "23: move.l spr_end,%a3\n"
  "    move.w #224+31,%d3\n"
  "    move.l %a1,%d2\n"
  "    sub.l spr,%d2\n"
  "    lsr.w #3,%d2\n"
  "    addq.w #1,%d2\n"
  "    subq.w #1,%d5\n"
  "10: move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    move.w %d0,%d1\n"
  "    add.w #31,%d1\n"
  "    cmp.w %d3,%d1\n"
  "    bcc 11f\n"
  "    move.w 4(%a0),%d1\n"
  "    add.w %d6,%d1\n"
  "    move.w %d1,%a2\n"
  "    add.w #31,%d1\n"
  "    cmp.w #287,%d1\n"
  "    bcc 11f\n"
  "    cmpa.l %a3,%a1\n"
  "    beq 12f\n"
  "    add.w #129,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w (%a0),%d0\n"
  "    or.w %d2,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    addq.w #1,%d2\n"
  "    move.w 2(%a0),(%a1)+\n"
  "    move.w %a2,%d0\n"
  "    add.w #128,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "11: addq.l #6,%a0\n"
  "    dbra %d5,10b\n"
  "12: move.l %a1,sp_ptr\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a3\n"
  "    rts\n");

/* DrawLowPriorityEprojs / DrawHighPriorityEprojs ($86:8390/83B2) enteros en la Genesis, a mano: el recorrido de los
   18 proyectiles, la prueba de pantalla del SNES y el dibujo con los registros de cada uno recordados por lugar (el
   spritemap de un proyectil cambia poco). bx/by = temblor - cámara. La lista de sprites que sale es la misma que con
   DrawEprojSpritemap (comprobado con MD_SPRSUM). C: md_draw_eprojs(prio, bx, by). */
__asm__(
  "    .text\n"
  "    .global md_draw_eprojs_asm\n"
  "md_draw_eprojs_asm:\n"
  "    movem.l %d2-%d7/%a2-%a6,-(%sp)\n"   /* 44 bytes: argumentos en 48, 52, 56 (palabra baja +2) */
  "    move.w 50(%sp),%d2\n"                /* prio */
  "    move.w 54(%sp),%d3\n"                /* bx */
  "    move.w 58(%sp),%d4\n"                /* by */
  "    lea eproj_id+34,%a2\n"
  "    lea eproj_properties+34,%a3\n"
  "    lea eproj_x_pos+34,%a4\n"
  "    lea eproj_y_pos+34,%a5\n"
  "    lea ep_cache+136,%a6\n"
  "    moveq #34,%d5\n"
  "1:  tst.w (%a2)\n"
  "    beq 9f\n"
  "    move.w (%a3),%d0\n"
  "    and.w #0x1000,%d0\n"
  "    cmp.w %d2,%d0\n"
  "    bne 9f\n"
  "    move.w (%a4),%d6\n"
  "    add.w %d3,%d6\n"                     /* x */
  "    move.w %d6,%d0\n"
  "    add.w #128,%d0\n"
  "    and.w #0xFE00,%d0\n"
  "    bne 9f\n"
  "    move.w (%a5),%d1\n"
  "    add.w %d4,%d1\n"                     /* y */
  "    move.w %d1,%d0\n"
  "    and.w #0xFF00,%d0\n"
  "    beq 2f\n"
  "    move.w %d1,%d0\n"
  "    add.w #128,%d0\n"
  "    and.w #0xFE00,%d0\n"
  "    bne 9f\n"
  "2:  lea eproj_spritemap_ptr,%a0\n"
  "    move.w (%a0,%d5.w),%d0\n"            /* spritemap */
  "    lea eproj_gfx_idx,%a0\n"
  "    move.w (%a0,%d5.w),%d7\n"            /* tiles y paleta */
  "    move.l 4(%a6),%d1\n"
  "    beq 5f\n"
  "    cmp.w (%a6),%d0\n"
  "    bne 5f\n"
  "    cmp.w 2(%a6),%d7\n"
  "    bne 5f\n"
  "    movea.l %d1,%a0\n"
  "4:  move.w (%a5),%d1\n"
  "    add.w %d4,%d1\n"
  "    move.w %d6,%d0\n"
  "    jsr md_putrecs\n"
  "9:  subq.l #2,%a2\n"
  "    subq.l #2,%a3\n"
  "    subq.l #2,%a4\n"
  "    subq.l #2,%a5\n"
  "    subq.l #8,%a6\n"
  "    subq.w #2,%d5\n"
  "    bpl 1b\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
  "    rts\n"
  "5:  move.l %d7,-(%sp)\n"                /* ep_lookup(v2, sm, g) */
  "    move.l %d0,-(%sp)\n"
  "    move.l %d5,-(%sp)\n"
  "    jsr ep_lookup\n"
  "    lea 12(%sp),%sp\n"
  "    tst.l %d0\n"
  "    beq 9b\n"
  "    movea.l %d0,%a0\n"
  "    bra 4b\n");
void md_draw_eprojs_asm(uint16 prio, uint16 bx, uint16 by);
void map_page_ext(uint8 p);
void md_draw_eprojs(uint16 prio, uint16 bx, uint16 by) {
  uint8 zp = md_zebes ? zebes_obj_page() : 0xFF;
  if (zp == 0xFF) { ep_zebes = 0; md_draw_eprojs_asm(prio, bx, by); return; }
  extern uint8 md_cur_page;
  uint8 lp = md_cur_page;
  ep_zebes = 1;
  map_page_ext(zp);
  md_draw_eprojs_asm(prio, bx, by);
  map_page_ext(lp);
}

void DrawA6Spritemap(uint16 sm, uint16 x, uint16 y, uint16 a) {  // 0xA6DC13: posición en la sala
  if (!drawkey(3, sm, a, 0, (int16)(x - layer1_x_pos), (int16)(y - layer1_y_pos))) missing(3, sm, a, 0);
}

void DrawA6SpritemapScreen(uint16 sm, uint16 x, uint16 y, uint16 chr) {  // DrawSpritemap ($81:879F): ya en pantalla
  if (sm == g_room->door_sm) { door_seen = 1; return; }   /* la puerta cerrada de la sala de Ridley: fondo */
  if (!drawkey(6, sm, chr, 0, (int16)x, (int16)y)) missing(6, sm, chr, 0);
}

/* contador de la huida: DrawSpritemap($80, sm, x, y, 0x0A00) */
void DrawTimerSpritemap(uint16 sm, uint16 x, uint16 y) {  // 0x809FB3
  if (!drawkey(8, sm, 0x0A00, 0, (int16)x, (int16)y)) missing(8, sm, 0x0A00, 0);
}

/* texto de la huida (EMERGENCY...): celdas del BG3 del SNES (tilemap en 0x5000, 32 de ancho) que la Genesis pone en
   el plano A de la sala de Ridley, donde no hay nada, con las letras del HUD */
#define TEXT_MAX 128
static uint16 text_q[TEXT_MAX][2], text_ql[TEXT_MAX][2];   /* del vblank, de la lógica */
static uint8 text_n, text_nl;
void EscapeTextCell(uint16 dst, uint16 word) {
  if (text_nl < TEXT_MAX) {
    uint16 row = (dst - 0x5000) >> 5, col = dst & 31;
    text_ql[text_nl][0] = MD_VRAM_PLANE_A + (row * g_room->plane_w + col) * 2;
    int lo = 0, hi = ESC_TEXT_N - 1;               /* kEscText: pares ordenados por palabra */
    while (lo <= hi) {
      int mid = (lo + hi) >> 1;
      uint16 w = kEscText[mid * 2];
      if (w == word) {
        text_ql[text_nl][1] = kEscText[mid * 2 + 1];
        text_nl++;
        return;
      }
      if (w < word) lo = mid + 1; else hi = mid - 1;
    }
  }
}

uint8 zebes_obj_draw(uint16 sm, uint16 pal, uint16 tiles, int16 x, int16 y);
void DrawGrappleOrProjectileSpritemap(uint16 sm, uint16 x, uint16 y);
void DrawProjectileSpritemap(uint16 k, uint16 x, uint16 y) {  // 0x818A4B
  DrawGrappleOrProjectileSpritemap(projectile_spritemap_pointers[k >> 1], x, y);
}

/* un spritemap de $93 (proyectiles de Samus y el destello de la carga, DrawBeamGrappleSpritemap $81:8A37) */
void DrawGrappleOrProjectileSpritemap(uint16 sm, uint16 x, uint16 y) {  // 0x818A5F
  if (intro_on) { it_draw(INTRO_KIND_PROJ, sm, 0, (int16)x, (int16)y); return; }
  /* Zebes: misiles, bombas y explosiones con los colores de la sala (mkzobj.py: clave sm, 0, 0xC000) */
  if (md_zebes && zebes_obj_draw(sm, 0, 0xC000, (int16)x, (int16)y)) return;
  GenSpr *s0 = sp_ptr;
  if (!drawkey(4, sm, 0, 0, (int16)x, (int16)y)) missing(4, sm, 0, 0);
  if (md_zebes)                                   /* Zebes: el rayo usa la línea 3 (debajo del HUD: sus colores) */
    for (GenSpr *s = s0; s < sp_ptr; s++)
      if ((s->attr & 0x6000) == 0x2000) s->attr |= 0x6000;
}

/* cañón del brazo abierto ($90:C663): un sprite de 8x8 con un tile que el juego copia de $9A según la dirección y el
   cuadro; aquí ese tile va en ARM_TILE (justo antes de los de Samus) y se copia en el vblank si cambió */
#define ARM_TILE (SAMUS_BASE - 1)
static uint16 arm_src_shown = 0xFFFF;
static const uint8 *arm_dma, *arm_dmal;
void DrawArmCannon(int16 x, int16 y, uint16 attr, uint16 src) {
  if (src != arm_src_shown) {
    int lo = 0, hi = ARM_CANNON_TILES - 1;
    while (lo <= hi) {
      int m = (lo + hi) >> 1;
      if (kArmCannonSrc[m] == src) { arm_dmal = kArmCannonTiles + m * 32; arm_src_shown = src; break; }
      if (kArmCannonSrc[m] < src) lo = m + 1; else hi = m - 1;
    }
  }
  if (x < 0 || y >= 224 || sp_ptr == spr + MAX_SPR) return;
  GenSpr *s = sp_ptr++;
  s->y = y + 129;
  s->size_link = (uint16)(s - spr + 1);
  s->attr = (uint16)((((attr >> 12) & 3) == 3 ? 0x8000 : 0) | (2 << 13) | (((attr >> 15) & 1) << 12) |
                     (((attr >> 14) & 1) << 11) | ARM_TILE);
  s->x = x + 128;
}

void DrawProjectileTrailSprite(uint16 x, uint16 y, uint16 t) {
  if (intro_on) { it_draw(INTRO_KIND_TRAIL, t, 0, (int16)x, (int16)y); return; }
  /* Zebes: con los colores de la sala (mkzobj.py: clave tile, 0, 0xC001); en Ceres no hay misiles */
  if (md_zebes && zebes_obj_draw(t, 0, 0xC001, (int16)x, (int16)y)) return;
  missing(7, t, 0, 0);
}

static uint8 m7_variant_l;
void CeresDoor_QueueMode7Transfers(uint16 w) { m7_variant_l = (uint8)w; }  // 0xA6F8F1

/* Ridley_Func_118/119: el juego copia tiles nuevos de Ridley a la VRAM; la Genesis tiene esas versiones listas */
void RidleyQueueTiles(uint16 src1, uint16 src2, uint16 dst1, uint16 dst2, uint16 size) {
  (void)src2; (void)dst2; (void)size;
  for (uint16 i = 0; i < DYN_COUNT; i++)
    if (kDyn[i].dst == dst1 && kDyn[i].src == src1 && dyn_nl < 4)
      dyn_ql[dyn_nl++] = &kDyn[i];
}

/* ------------------------------------------------------------------ huida de Ridley (modo 7 en el SNES)
 * Cada cuadro ya está armado en el PC (mkgetaway.py): paleta de la línea 1, tiles y el rectángulo de celdas del
 * plano B. La Genesis carga los tiles del cuadro pedido en la mitad libre de la zona de tiles de Ridley (doble búfer,
 * hasta GA_BUDGET tiles por vblank) y, cuando están todos, cambia las celdas y la paleta de una vez. */
#define GA_BUDGET 120
#define GA_WORDS_LINE 70                        /* DMA en el vblank (H32): 167 bytes por línea, con margen */
#define GA_ROW_COST 12                          /* lo que cuesta armar la DMA de cada fila, en palabras */
#define GA_FULL_VBLANK 26                       /* líneas para la huida en un vblank sin retraso */
static const uint16 ga_budget = GA_BUDGET;
static int16 ga_want = -1, ga_shown = -1, ga_loading = -1;
static uint8 ga_called;
static int16 ga_want_l;                          /* el cuadro que pidió la lógica (md_post lo pasa a ga_want) */                          /* el motor pidió un cuadro de la huida en este cuadro */
static uint16 ga_done;
static uint8 ga_reg, ga_shown_reg;

void RidleyGetawayMode7Queue(uint16 list) { (void)list; }   /* el aleteo ya está en los cuadros preparados */
void RidleyGetawayFrame(void) {
  if (g_room->room != 0xE0B5) return;
  int16 f = (int16)(ERAM(0x8026) >> 1) - 1;                 /* ridley_var_33: (cuadro + 1) * 2 */
  if (f < 0) f = 0;
  if (f >= GETAWAY_FRAMES) f = GETAWAY_FRAMES - 1;
  ga_want_l = f;
  ga_called = 1;
}



/* DMA de un rectángulo de celdas al plano B: h filas de w palabras (fuente: una fila cada stride bytes; destino:
   cada 128 bytes).
   A mano: ~60 ciclos por fila más la transferencia (en C cada DMA costaba ~500). Si una fila cruzara un límite de
   128 KB de la fuente, esa fila va por el 68000. */
void md_dma_rows(const uint16 *src, uint16 dst, uint16 w, uint16 h, uint16 stride);
static volatile uint16 md_dma_trig;
__asm__(
  "    .text\n"
  "    .global md_dma_rows\n"
  "md_dma_rows:\n"
  "    movem.l %d2-%d7/%a2,-(%sp)\n"
  "    move.l 32(%sp),%a0\n"                  /* src */
  "    move.w 38(%sp),%d0\n"                  /* dst */
  "    move.w 42(%sp),%d1\n"                  /* w */
  "    move.w 46(%sp),%d2\n"                  /* h */
  "    beq 9f\n"
  "    tst.w %d1\n"
  "    beq 9f\n"
  "    lea 0xC00004,%a1\n"
  "    lea md_dma_trig,%a2\n"
  "    move.w %d1,%d4\n"                      /* registros 0x93/0x94: el largo */
  "    lsr.w #8,%d4\n"
  "    or.w #0x9400,%d4\n"
  "    swap %d4\n"
  "    move.b %d1,%d4\n"
  "    and.w #0xFF,%d4\n"
  "    or.w #0x9300,%d4\n"
  "    swap %d4\n"                            /* d4 = 9300ll 9400hh */
  "    move.w 50(%sp),%d7\n"                  /* stride */
  "    subq.w #1,%d2\n"
  "1:  move.l %a0,%d3\n"
  "    lsr.l #1,%d3\n"
  "    move.w %d3,%d5\n"
  "    add.w %d1,%d5\n"
  "    bcs 5f\n"                              /* cruza 128 KB: por el 68000 */
  "    move.l %d4,(%a1)\n"
  "    moveq #0,%d5\n"
  "    move.b %d3,%d5\n"
  "    or.w #0x9500,%d5\n"
  "    swap %d5\n"
  "    move.w %d3,%d6\n"
  "    lsr.w #8,%d6\n"
  "    or.w #0x9600,%d6\n"
  "    move.w %d6,%d5\n"
  "    move.l %d5,(%a1)\n"
  "    swap %d3\n"
  "    and.w #0x7F,%d3\n"
  "    or.w #0x9700,%d3\n"
  "    move.w %d3,(%a1)\n"
  "    move.w %d0,%d6\n"
  "    and.w #0x3FFF,%d6\n"
  "    or.w #0x4000,%d6\n"
  "    move.w %d6,(%a1)\n"
  "    move.w %d0,%d6\n"
  "    rol.w #2,%d6\n"
  "    and.w #3,%d6\n"
  "    or.w #0x80,%d6\n"
  "    move.w %d6,(%a2)\n"
  "    move.w (%a2),(%a1)\n"                  /* la palabra que arranca la DMA, leída de la RAM */
  "    add.w %d7,%a0\n"
  "4:  add.w #128,%d0\n"
  "    dbra %d2,1b\n"
  "9:  movem.l (%sp)+,%d2-%d7/%a2\n"
  "    rts\n"
  "5:  move.l %a0,-(%sp)\n"                   /* fila por el 68000 */
  "    move.w %d0,%d6\n"
  "    and.w #0x3FFF,%d6\n"
  "    or.w #0x4000,%d6\n"
  "    move.w %d6,(%a1)\n"
  "    move.w %d0,%d6\n"
  "    rol.w #2,%d6\n"
  "    and.w #3,%d6\n"
  "    move.w %d6,(%a1)\n"
  "    move.w %d1,%d6\n"
  "    subq.w #1,%d6\n"
  "6:  move.w (%a0)+,0xC00000\n"
  "    dbra %d6,6b\n"
  "    move.l (%sp)+,%a0\n"
  "    add.w %d7,%a0\n"
  "    bra 4b\n");

/* Cambio de celdas de la huida, fila por fila del plano B: lo que el rectángulo anterior P tapaba fuera del nuevo F
   vuelve al plano B de la sala y se escribe la fila de F. Costo en palabras (más ~40 por cada DMA). */
static int16 ga_sy = -1, ga_sy1;               /* cambio a medias (cuadros grandes): próxima fila y fin */
static int16 ga_ux0, ga_uy0, ga_uw, ga_uh;        /* rectángulo del cambio en curso */
static void ga_union(const GetawayFrame *P, const GetawayFrame *F) {
  int16 x0 = 64, y0 = 64, x1 = 0, y1 = 0;
  if (F->w) { x0 = F->x0; y0 = F->y0; x1 = F->x0 + F->w; y1 = F->y0 + F->h; }
  if (P) {
    if (P->x0 < x0) x0 = P->x0;
    if (P->y0 < y0) y0 = P->y0;
    if (P->x0 + P->w > x1) x1 = P->x0 + P->w;
    if (P->y0 + P->h > y1) y1 = P->y0 + P->h;
  }
  if (x1 <= x0) { ga_ux0 = ga_uy0 = 0; ga_uw = ga_uh = 0; return; }
  ga_ux0 = x0; ga_uy0 = y0; ga_uw = x1 - x0; ga_uh = y1 - y0;
}

/* filas [y0, y1) del plano B dentro de U: arriba y abajo de F, el plano B de la sala; en las filas de F, la sala a los
   costados y las celdas de F en el medio */
static void ga_union_rows(const GetawayFrame *F, int16 y0, int16 y1) {
  const uint16 *pb = g_room->planeB;
  int16 fy0 = F->w ? F->y0 : y1, fy1 = F->w ? F->y0 + F->h : y1;
  if (fy0 < y0) fy0 = y0;
  if (fy0 > y1) fy0 = y1;
  if (fy1 > y1) fy1 = y1;
  if (fy1 < fy0) fy1 = fy0;
  if (fy0 > y0)                                   /* arriba de F */
    md_dma_rows(pb + y0 * 64 + ga_ux0, MD_VRAM_PLANE_B + (y0 * 64 + ga_ux0) * 2, ga_uw, fy0 - y0, 128);
  if (fy1 > fy0) {                                /* filas de F */
    uint16 n = fy1 - fy0;
    if (F->x0 > ga_ux0)
      md_dma_rows(pb + fy0 * 64 + ga_ux0, MD_VRAM_PLANE_B + (fy0 * 64 + ga_ux0) * 2, F->x0 - ga_ux0, n, 128);
    const uint16 *fm = kGetawayMaps + F->moff + (ga_reg ? F->w * F->h : 0) + (fy0 - F->y0) * F->w;
    uint8 lp = md_cur_page;
    map_page_ext(CINE_PAGE_kGetawayMaps);
    md_dma_rows(fm, MD_VRAM_PLANE_B + (fy0 * 64 + F->x0) * 2, F->w, n, F->w * 2);
    map_page_ext(lp);
    int16 fx1 = F->x0 + F->w, ux1 = ga_ux0 + ga_uw;
    if (ux1 > fx1)
      md_dma_rows(pb + fy0 * 64 + fx1, MD_VRAM_PLANE_B + (fy0 * 64 + fx1) * 2, ux1 - fx1, n, 128);
  }
  int16 b0 = fy1;
  if (y1 > b0)                                    /* abajo de F */
    md_dma_rows(pb + b0 * 64 + ga_ux0, MD_VRAM_PLANE_B + (b0 * 64 + ga_ux0) * 2, ga_uw, y1 - b0, 128);
}

/* líneas que quedan hasta el final del vblank (NTSC: el contador va 0-0xEA y luego 0xE5-0xFF; 0xE5-0xEA se toma
   como la segunda vuelta, lo más tarde posible) */
static int16 vb_lines_left(void) {
  uint16 vc = VDP_VCOUNT >> 8;
  uint16 line = vc < 0xE0 ? 262 : vc < 0xE5 ? vc : vc + 6;
  return (int16)(262 - line);
}
int16 md_vb_lines_left(void) { return vb_lines_left(); }   /* md_cone.c */

#ifdef MD_LAGLOG
volatile uint16 md_ga_log[4][128]; volatile uint16 md_ga_n, md_ga_cost, md_ga_words, md_ga_ndma;
static void ga_vblank_(void);
static void ga_vblank(void) {
  uint16 a = VDP_VCOUNT >> 8, l = vb_lines_left(); uint16 st = ga_loading, d = ga_done; md_ga_cost = 0; md_ga_words = 0;
  ga_vblank_();
  uint16 b = VDP_VCOUNT >> 8;
  if (md_frame_count >= 5000 && md_frame_count < 5128) { uint16 i = md_ga_n++ & 127; md_ga_log[0][i] = a << 8 | b; md_ga_log[1][i] = md_frame_count; md_ga_log[2][i] = ga_loading == -1 ? 0x8000 | md_ga_cost : (ga_done - d); md_ga_log[3][i] = md_ga_words; }
}
static void ga_vblank_(void) {
#else
static void ga_vblank(void) {
#endif
  if (ga_want >= 0 && ga_loading < 0 && ga_want != ga_shown) {
    ga_loading = ga_want;
    ga_done = 0;
    ga_reg = ga_shown >= 0 ? ga_shown_reg ^ 1 : 0;
  }
  if (ga_loading < 0) return;
  const GetawayFrame *F = &kGetaway[ga_loading];
  int16 left = vb_lines_left() - 12;              /* líneas de vblank que quedan (12 para el resto del vblank) */
  uint16 n = F->ntiles - ga_done;
  if (n) {
    int16 b = (left * GA_WORDS_LINE) >> 4;        /* tiles de 16 palabras */
    if (b <= 0) return;
    if (n > (uint16)b) n = b;
    if (n > ga_budget) n = ga_budget;
    uint8 lp = md_cur_page;
    map_page_ext(CINE_PAGE_kGetawayTiles);
    vdp_dma_vram(kGetawayTiles + F->toff + ga_done * 32, (GETAWAY_TILE0 + ga_reg * GETAWAY_REGION + ga_done) * 32, n * 16);
    map_page_ext(lp);
    ga_done += n;
    return;                                       /* el cambio de celdas, en un vblank propio */
  }
  /* cambio de celdas: el rectángulo U = el anterior P más el nuevo F se reescribe entero: las celdas de F (con el
     plano B original donde no está Ridley) y el plano B de la sala en el resto de U. Entero en este vblank si cabe; si
     no cabe ni en un vblank entero (los últimos cuadros, Ridley de cerca), por partes de arriba abajo, y la paleta
     al final. */
  const GetawayFrame *P = ga_shown >= 0 && kGetaway[ga_shown].w ? &kGetaway[ga_shown] : 0;
  int16 budget = left * GA_WORDS_LINE;
  if (ga_sy < 0) {
    ga_union(P, F);
    uint16 cost = ga_uh * (ga_uw + GA_ROW_COST) + (F->w ? F->h * 2 * GA_ROW_COST : 0);
#ifdef MD_LAGLOG
    md_ga_cost = cost;
#endif
    if ((int16)cost > budget && cost <= GA_FULL_VBLANK * GA_WORDS_LINE) return;   /* en el vblank siguiente */
    ga_sy = ga_uy0;
    ga_sy1 = ga_uy0 + ga_uh;
  }
  {
    uint16 per = ga_uw + GA_ROW_COST + (F->w ? 2 * GA_ROW_COST : 0);
    uint16 k = ga_sy1 - ga_sy;
    if (budget < (int16)(k * per)) { k = budget > 0 ? (uint16)budget / per : 0; if (!k) return; }
    ga_union_rows(F, ga_sy, ga_sy + k);
#ifdef MD_LAGLOG
    md_ga_words = ga_uw | (k << 8) | (F->w ? 0x8000 : 0);
#endif
    ga_sy += k;
    if (ga_sy < ga_sy1) return;                   /* sigue en el vblank siguiente */
  }
  ga_sy = -1;
  if (F->w) {
    VDP_CTRL32 = VDP_CMD_CRAM_W(34);
    for (uint16 c = 0; c < 15; c++) { cram[17 + c] = F->pal[c]; VDP_DATA = F->pal[c]; }
  }
  if (!F->w) flash_shown = 0xFF;                  /* sin Ridley: la línea 1 vuelve a sus colores (md_frame) */
  ga_shown = ga_loading;
  ga_shown_reg = ga_reg;
  ga_loading = -1;
}

/* ------------------------------------------------------------------ salas */
static const MdRoom *find_room(void) {
  for (uint16 i = 0; i < MD_ROOM_COUNT; i++)
    if (kMdRooms[i].room == room_ptr && kMdRooms[i].state == roomdefroomstate_ptr)
      return &kMdRooms[i];
  for (uint16 i = 0; i < MD_ROOM_COUNT; i++)
    if (kMdRooms[i].room == room_ptr)
      return &kMdRooms[i];
  return &kMdRooms[0];
}

static void md_use_room(const MdRoom *R);
static void md_room_gfx(void) {   /* g_hooks.room_gfx: la sala nueva ya está en las variables del motor */
  md_sync();                                   /* lo del vblank cambia: que antes tome el cuadro anterior */
  memset(ep_cache, 0, sizeof ep_cache);        /* las claves de sprites son de la sala */
  cine_stage = 0;
  pz_on = 0;                                   /* (al salir de la pausa: ResumeGameplay) */
  if (area_index != 6) {                       /* Zebes: se dibuja desde el nivel (md_zebes.c) */
    const MdRoom *z = zebes_use();
    if (z) { md_use_room(z); return; }
  }
  md_zebes = 0;
  md_use_room(find_room());
}

static void md_use_room(const MdRoom *R) {
  g_room = R;
  fl_line = 0xFF;
  md_keys = g_room->keys; md_khash = g_room->khash; md_kmask = g_room->khash_mask;
  memset(md_kcache, 0, sizeof md_kcache);
  for (uint16 i = 0; i < 256; i++) cram_rev[i] = 0xFF;
  for (int16 i = 63; i >= 0; i--) {
    uint8 s = g_room->src[i];
    uint8 own = R->free_line < 4 && (i >> 4) == R->free_line && (i & 15);   /* color que pone la Genesis */
    cram_off[i] = own ? 0xFFFE : s == 0xFF ? 0xFFFF : s * 2;
    cram_next[i] = 0xFF;
    if (!own && s != 0xFF) { cram_next[i] = cram_rev[s]; cram_rev[s] = (uint8)i; }
  }
  nt_palette_lo = 0;
  nt_palette_hi = 255;
  nt_fade_list = g_room->fade;
  nt_fade_count = g_room->nfade;
  nt_palette_changed = 1;
  room_pending = 1;
  m7_variant = m7_variant_shown = m7_variant_l = 0;
  door_seen = 0;
  flash_seen = flash_shown = 0;
  dyn_n = dyn_nl = 0;
  text_n = text_nl = 0;
  arm_dma = arm_dmal = 0;
  arm_src_shown = 0xFFFF;                      /* los tiles de la sala pueden pisar el del cañón */
  ga_want = ga_want_l = ga_shown = ga_loading = -1;
  ga_sy = -1;
}


/* ------------------------------------------------------------------ cinemáticas y menús (docs/35) */
/* Las escenas de Ceres explotando y del viaje a Zebes, y el menú de fin de juego, usan "salas" propias preparadas en
 * el PC (C001, C002, C0FF: sus sprites). Lo que en el SNES es la capa del modo 7 (la estación, la nave, Zebes con
 * mosaico) son imágenes ya dibujadas (mkm7.py): se elige la de la matriz del motor y va al plano A.
 *
 * Capa del modo 7, con doble búfer: el plano A (64x64 celdas) tiene dos mitades de 32 filas. La imagen que se ve
 * está en una y la siguiente se arma en la otra en varios vblanks (sus tiles en la otra mitad de la zona de tiles,
 * después sus celdas). Cuando está lista se cambia de mitad con el scroll vertical (+256) y los colores, en el
 * mismo vblank. La fila lógica Y (V/8 del SNES) de la mitad b es la fila física (Y + 32 b) & 63.
 *
 * Círculo de la explosión (plano B): en cada línea, un tramo [izquierda, derecha] de las tablas del motor. El plano
 * B tiene un dibujo fijo: el píxel (X, Y) es del color 15 si X <= Y (un triángulo de 512x512). Con el scroll
 * horizontal por línea, la línea muestra la fila Y desde la columna "izquierda"; la fila elige el ancho
 * (Y = derecha - izquierda). La fila de cada línea se elige con el scroll vertical, que cambia una interrupción
 * horizontal en cada línea (cv_*). Así los bordes son exactos, sin tiles por cuadro. */
static uint8 cine_var;          /* etapa 1: 1 estación, 2 nave al explotar, 3 Ceres explota con Samus */

/* --- capa del modo 7 */
static int16 m7_want = -1;      /* imagen que pide el motor (-1: ninguna) */
static int16 m7_shown = -1;     /* la que se ve (en la mitad m7_buf del plano, sus tiles en la mitad m7_half) */
static int16 m7_bx0, m7_by0;    /* lo que quedó dibujado en la otra mitad (hay que borrarlo): rectángulo de celdas */
static uint16 m7_bw, m7_bh;     /* (la anterior, o también una que se dejó a medio armar) */
static uint8 m7_buf, m7_half;
static uint8 m7_busy;           /* armando m7_load en la otra mitad */
static volatile uint8 m7_lock;  /* cine_frame o cine_start cambian lo de arriba */
static int16 m7_load;
static uint8 m7_load_half, m7_step;   /* pasos: 0 tiles, 1 borrar la anterior, 2 celdas, 3 lista */
static uint16 m7_loaded, m7_row;
/* Cuándo se cambia de imagen no depende de cuánto vblank sobra (así la imagen de cada cuadro del motor es siempre la
   misma): cada imagen cuesta palabras de DMA (m7_cost_of) y cada vblank cuenta M7_BUDGET. Se cambia cuando ese
   presupuesto alcanza y la imagen está armada; el armado de verdad (cine_vblank) usa todo lo que sobra del vblank
   y casi siempre va adelante (docs/50). */
#ifndef M7_BUDGET
#define M7_BUDGET 650
#endif
#ifndef M7_BUDGET_CIRC
#define M7_BUDGET_CIRC 400      /* con el círculo de la explosión (su tabla de scroll va en el mismo vblank) */
#endif
static uint16 m7_cost;          /* presupuesto que le falta a la imagen que se arma */
#ifdef MD_M7LOG
volatile uint16 md_m7_wait;     /* cuadros en que el presupuesto alcanzaba y el armado no (el cambio se atrasa) */
#endif
/* scroll de la imagen que se ve: el del motor, salvo cuando el motor ya cambió de modo (llegada a Zebes: del mosaico
   al modo 7) y todavía se ve la imagen del modo anterior: esa sigue con el scroll de su modo hasta el cambio */
static uint16 m7_show_h, m7_show_v, m7_load_h, m7_load_v;
static uint8 m7_show_m7, m7_load_m7;            /* la imagen es del modo 7 (si no, la del mosaico de la llegada a Zebes) */

#define M7_MAXW 40
#define M7_MAXH 36
_Static_assert(M7_MAXW * M7_MAXH <= sizeof md_scratch / 2, "md_scratch");
#define m7_cells md_scratch   /* celdas de la imagen nueva (con la base de su mitad de tiles) */
static const uint16 kZeros[64];

/* la imagen i es del modo 7 (sus claves no son las del mosaico, var 0x1x) */
static uint8 m7_is_mode7(int16 i) {
  if (i < 0) return 0;
  for (uint16 k = 0; k < M7_KEYS; k++)
    if (kM7Keys[k].img == (uint16)i) return (kM7Keys[k].var & 0xF0) != 0x10;
  return 1;
}
static uint16 m7_tile0(void) { return cine_stage == 2 ? M7_TILE0_2 : M7_TILE0_1; }
static uint16 m7_region(void) { return cine_stage == 2 ? M7_REGION_2 : M7_REGION_1; }

static int16 m7_lookup(uint8 var, uint16 a, uint16 b, uint16 c, uint16 d) {
  int lo = 0, hi = M7_KEYS - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    const M7Key *k = &kM7Keys[m];
    int cmp = k->var != var ? (k->var < var ? -1 : 1) : k->a != a ? (k->a < a ? -1 : 1) : k->b != b ? (k->b < b ? -1 : 1)
            : k->c != c ? (k->c < c ? -1 : 1) : k->d != d ? (k->d < d ? -1 : 1) : 0;
    if (!cmp) return (int16)k->img;
    if (cmp < 0) lo = m + 1; else hi = m - 1;
  }
  return -1;
}

/* ---------------- círculo */
static void circ_vram(void) {   /* (pantalla apagada) tiles y el dibujo del plano B: celda (cx, R) llena si cx < R */
  static const uint32 kTri[8] = { 0xF0000000, 0xFF000000, 0xFFF00000, 0xFFFF0000, 0xFFFFF000, 0xFFFFFF00, 0xFFFFFFF0, 0xFFFFFFFF };
  vdp_set_vram_write(CIRC_T0 * 32);
  for (uint16 i = 0; i < 16; i++) VDP_DATA = 0xFFFF;                       /* lleno */
  for (uint16 i = 0; i < 8; i++) { VDP_DATA = kTri[i] >> 16; VDP_DATA = (uint16)kTri[i]; }   /* X <= Y */
  vdp_set_vram_write(MD_VRAM_PLANE_B);
  for (uint16 R = 0; R < 64; R++)
    for (uint16 cx = 0; cx < 64; cx++) VDP_DATA = cx < R ? CIRC_T0 : cx == R ? CIRC_T0 + 1 : 0;
}

/* líneas g0..g1-1: vacías (columnas 256 en adelante y una fila < 256: la g, sin scroll) o con el tramo de la tabla del SNES desde idx
   (paso d): columna "izquierda" y fila Y = derecha - izquierda (si llega al borde derecho, 255 - izquierda alcanza
   igual). La fila de la línea g sale con el scroll vertical Y - g. */
static void cv_empty(int16 g0, int16 g1, uint16 *hb, uint16 *vt) {
  for (int16 g = g0; g < g1; g++) { hb[2 * g] = 256; vt[g] = 0; }
}
static void cv_spans(int16 g0, int16 g1, int16 idx, int16 d, uint16 *hb, uint16 *vt) {
  const uint8 *pa = power_bomb_explosion_left_hdma + idx, *pb = power_bomb_explosion_right_hdma + idx;
  uint16 *h = hb + 2 * g0, *v = vt + g0;
  for (int16 g = g0; g < g1; g++) {
    uint16 a = *pa, b = *pb;
    pa += d; pb += d;
    if (a <= b) { *h = a; *v = (uint16)(b - a - g) & 511; }
    else { *h = 256; *v = 0; }
    h += 2; v++;
  }
}

static void circ_frame(void) {
  uint8 on = cine_stage == 1 && (power_bomb_explosion_status & 0x8000) && power_bomb_explosion_y_pos_rsub_0x1ff;
  cv_line = on;
  if (!on) {                    /* el dibujo fuera de la pantalla: columnas 256-511 de las filas 0-223 (vacías) */
    scroll_bx = 256;
    scroll_by = 0;
    cv_key[2] = 0x8000;                         /* al volver, se arma de nuevo */
    cv_new = 0;
    return;
  }
  uint16 c = kCram555[(reg_COLDATA[2] & 31) << 10 | (reg_COLDATA[1] & 31) << 5 | (reg_COLDATA[0] & 31)];
  if (cram[15] != c) { cram[15] = c; cram_dirty = 1; }    /* color 15 de la línea 0: COLDATA (5 bits por canal) */
  int16 cy = (int16)(power_bomb_explosion_y_pos - layer1_y_pos);
  uint16 *vt = cv_tab[cv_build], *hb = rot_h + 1;
  uint16 ha = (uint16)-scroll_ax;
  /* las tablas del motor salen del radio, la posición y el scroll: si son los mismos, el círculo es el mismo */
  if (cv_key[0] == power_bomb_explosion_radius && cv_key[1] == power_bomb_explosion_x_pos_plus_0x100 &&
      cv_key[2] == (uint16)cy && ha == cv_ha) {
    cv_new = 0;
    return;
  }
  cv_key[0] = power_bomb_explosion_radius; cv_key[1] = power_bomb_explosion_x_pos_plus_0x100; cv_key[2] = (uint16)cy;
  cv_new = 1;
  if (ha != cv_ha) { cv_ha = ha; for (uint16 g = 0; g < 224; g++) rot_h[2 * g] = ha; }
  /* línea g de la Genesis = línea g + 1 del SNES (el scroll de 2). La tabla del SNES va por distancia al centro:
     arriba (g < cy) idx = cy - 1 - g, abajo idx = g - cy; solo hay 192 */
  int16 ext = 191;                              /* idx más lejano con tramo (más allá, la tabla está vacía) */
  while (ext >= 0 && power_bomb_explosion_left_hdma[ext] > power_bomb_explosion_right_hdma[ext]) ext--;
  int16 gu = cy < 0 ? 0 : cy > 224 ? 224 : cy;
  int16 e = cy - 1 - ext < 0 ? 0 : cy - 1 - ext > gu ? gu : cy - 1 - ext;   /* arriba, más lejos que el círculo */
  cv_empty(0, e, hb, vt);
  cv_spans(e, gu, cy - 1 - e, -1, hb, vt);
  int16 f = cy + ext + 1 > 224 ? 224 : cy + ext + 1 < gu ? gu : cy + ext + 1;   /* abajo, hasta el borde del círculo */
  cv_spans(gu, f, gu - cy, 1, hb, vt);
  cv_empty(f, 224, hb, vt);
  /* las 2 líneas vacías de arriba y de abajo del círculo toman la fila del borde (sin pasarse de la 255): si una
     línea del círculo llegara a tomar el valor de su vecina (un cuadro o una línea de diferencia entre el scroll
     horizontal y el vertical), no se vería una raya hasta el borde de la pantalla */
  int16 gt = 0, gb = 223;                       /* primera y última línea con círculo */
  while (gt < 224 && hb[2 * gt] == 256) gt++;
  while (gb > gt && hb[2 * gb] == 256) gb--;
  if (gt < 224) {
    for (int16 d = gt - 1; d >= 0 && d >= gt - 2; d--)
      if (((vt[gt] + d) & 511) <= 255) vt[d] = vt[gt];
    for (int16 d = gb + 1; d < 224 && d <= gb + 2; d++)
      if (((vt[gb] + d) & 511) <= 255) vt[d] = vt[gb];
  }
  for (uint16 i = 224; i < 232; i++) vt[i] = vt[223];
}

static void cv_setup_code(void) {   /* move.w (tabla).l,($C00000).l ; addq.w #2,(dirección de la tabla) ; rte */
  uint32 data = 0xC00000, src_lo = (uint32)&cv_code[2];
  cv_code[0] = 0x33F9;
  cv_code[3] = (uint16)(data >> 16); cv_code[4] = (uint16)data;
  cv_code[5] = 0x5479;
  cv_code[6] = (uint16)(src_lo >> 16); cv_code[7] = (uint16)src_lo;
  cv_code[8] = 0x4E73;
}
/* ---------------- capa del modo 7 */
#define M7_ROW_COST 48          /* lo que cuesta cada fila aparte de sus celdas (CPU de las DMA, medido), en palabras */
#ifndef M7_ROW_UNITS
#define M7_ROW_UNITS 128        /* (presupuesto) cada fila, aparte de sus celdas: medido, una fila tarda como ~150 palabras de tiles */
#endif
/* presupuesto para armar la imagen i (-1: ninguna) en la otra mitad: sus tiles, borrar lo que había y sus celdas */
static uint16 m7_cost_of(int16 i) {
  uint16 c = m7_bh * (m7_bw + M7_ROW_UNITS);
  if (i >= 0) { const M7Image *I = &kM7Images[i]; c += I->ntiles * 16 + I->h * (I->w + M7_ROW_UNITS); }
  return c;
}
static void m7_back_set(int16 i) {
  if (i < 0) { m7_bh = 0; return; }
  const M7Image *I = &kM7Images[i];
  m7_bx0 = I->x0; m7_by0 = I->y0; m7_bw = I->w; m7_bh = I->h;
}
/* lo que hay que borrar pasa a incluir también la imagen i (el rectángulo que cubre los dos; como mucho la mitad) */
static void m7_back_add(int16 i) {
  if (i < 0) return;
  if (!m7_bh) { m7_back_set(i); return; }
  const M7Image *I = &kM7Images[i];
  int16 x0 = I->x0 < m7_bx0 ? I->x0 : m7_bx0, y0 = I->y0 < m7_by0 ? I->y0 : m7_by0;
  int16 x1 = I->x0 + I->w > m7_bx0 + m7_bw ? I->x0 + I->w : m7_bx0 + m7_bw;
  int16 y1 = I->y0 + I->h > m7_by0 + m7_bh ? I->y0 + I->h : m7_by0 + m7_bh;
  m7_bx0 = x0; m7_by0 = y0;
  m7_bw = x1 - x0 > 64 ? 64 : x1 - x0;
  m7_bh = y1 - y0 > 32 ? 32 : y1 - y0;
}
static void cine_frame_(void);
static void cine_frame(void) {   /* md_frame: qué imagen del modo 7 se quiere, el cambio de mitad y el círculo */
  m7_lock = 1;                   /* (que un vblank sin cuadro no arme mientras tanto) */
  cine_frame_();
  m7_lock = 0;
}
static void cine_frame_(void) {
  int16 want = m7_want;
  if (cine_stage == 3 || !(reg_TM & 1)) {
    want = -1;
  } else {
    uint8 var;
    uint16 a = reg_M7A, b = reg_M7B, c = reg_M7C, d = reg_M7D;
    if (cine_stage == 1) var = cine_var;
    else if (reg_BGMODE == 7) var = 4;
    else { var = 0x10 | (reg_MOSAIC >> 4); a = b = c = d = 0; }
    int16 i = m7_lookup(var, a, b, c, d);
    if (i >= 0) want = i;
  }
  m7_want = want;
  /* el scroll del motor vale para la imagen que se ve salvo si el motor ya cambió de modo (llegada a Zebes: del
     mosaico al modo 7, el scroll significa otra cosa): entonces la que se ve sigue con el suyo hasta el cambio */
  uint8 now_m7 = (reg_BGMODE & 7) == 7;
  if (m7_busy) {                                 /* lo que contó el vblank anterior (cv_line: el del cuadro anterior) */
    uint16 b = cv_line ? M7_BUDGET_CIRC : M7_BUDGET;
    m7_cost = m7_cost > b ? m7_cost - b : 0;
  }
  if (m7_busy && !m7_cost) {                     /* lista (por el presupuesto) */
    /* se cambia de mitad en este vblank (scroll y colores) si el motor ya no pide la que se ve: la armada es esa o
       una más nueva en la cinemática. Si todavía pide la que se ve, la adelantada espera. */
#ifdef MD_M7LOG
    if (m7_want != m7_shown && m7_step != 3) md_m7_wait++;
#endif
    if (m7_want != m7_shown && m7_step == 3) {   /* (si el armado de verdad no llegó, espera: no debería pasar) */
      m7_buf ^= 1;
      m7_back_set(m7_shown);
      m7_shown = m7_load;
      m7_show_h = m7_load_h; m7_show_v = m7_load_v; m7_show_m7 = m7_load_m7;
      m7_half = m7_load_half;
      m7_busy = 0;
    }
  }
  if (m7_shown >= 0) {                           /* sus colores: los de palette_buffer de este cuadro */
    const M7Image *I = &kM7Images[m7_shown];
    for (uint16 i = 0; i < 14; i++) {
      uint16 c = kCram555[palette_buffer[I->col[i]] & 0x7FFF];
      if (cram[1 + i] != c) { cram[1 + i] = c; cram_dirty = 1; }
    }
  }
  /* la siguiente: la que pide el motor o, si ya se ve esa, la que sigue en la cinemática (se adelanta). La que
     esté a medio armar se termina primero. */
  int16 target = m7_want != m7_shown ? m7_want : m7_shown >= 0 && !cv_line ? kM7Images[m7_shown].next : m7_shown;
  /* (con el círculo de la explosión no se adelanta: el vblank libre le hace falta al cuadro siguiente) */
  /* el motor ya pide otra y armarla cuesta menos de lo que le falta a la que se arma (la nave al explotar crece en
     cada cuadro y después desaparece): se deja la que se armaba y se empieza la pedida. Lo que la otra ya escribió
     en la mitad oculta se borra con lo de antes. */
  if (m7_busy && m7_load != m7_want && m7_want != m7_shown) {
    int16 sx0 = m7_bx0, sy0 = m7_by0; uint16 sw = m7_bw, sh = m7_bh;
    if (m7_step >= 2) m7_back_add(m7_load);
    if (m7_cost_of(m7_want) < m7_cost) m7_busy = 0;
    else { m7_bx0 = sx0; m7_by0 = sy0; m7_bw = sw; m7_bh = sh; }
  }
  if (!m7_busy && target != m7_shown && (target >= 0 || m7_want != m7_shown)) {
    m7_busy = 1;
    m7_load = target;
    m7_load_h = reg_BG1HOFS;
    m7_load_v = reg_BG1VOFS;
    m7_load_m7 = m7_is_mode7(target);
    m7_load_half = m7_shown >= 0 ? m7_half ^ 1 : 0;
    m7_step = 0; m7_loaded = 0; m7_row = 0;
    m7_cost = m7_cost_of(target);
    if (m7_load >= 0) {                          /* sus celdas, con la base de la mitad de tiles */
      const M7Image *I = &kM7Images[m7_load];
      uint16 base = m7_tile0() + m7_load_half * m7_region() - 1;
      const uint16 *src = kM7Maps + I->moff;
      uint16 n = I->w * I->h;
      uint8 lp = md_cur_page;
      map_page_ext(CINE_PAGE_kM7Maps);
      for (uint16 i = 0; i < n; i++) {
        uint16 w = src[i];
        m7_cells[i] = w ? (uint16)(0x8000 | ((w & 0xF800) + base + (w & 0x7FF))) : 0;   /* prioridad: encima de los sprites de prioridad 0 */
      }
      map_page_ext(lp);
    }
  }
  /* sin la ventana del HUD no hacen falta las 2 líneas de más del juego (medido contra el SNES) */
  if (m7_shown < 0 || cine_stage != 2 || now_m7 == m7_show_m7) { m7_show_h = reg_BG1HOFS; m7_show_v = reg_BG1VOFS; }
  scroll_ax = m7_shown >= 0 ? m7_show_h : (uint16)reg_BG1HOFS;
  scroll_ay = (m7_shown >= 0 ? m7_show_v : (uint16)reg_BG1VOFS) + 2 + (m7_buf ? 256 : 0);
  circ_frame();
}

/* fila lógica y de la mitad b del plano A, columnas x0..x0+w-1 (con la vuelta en 64): celdas o ceros */
static void m7_row_dma(uint8 b, int16 y, int16 x0, uint16 w, const uint16 *cells) {
  uint16 py = (uint16)(y + 32 * b) & 63, px = (uint16)x0 & 63;
  uint16 n1 = w, n2 = 0;
  if (px + w > 64) { n1 = 64 - px; n2 = w - n1; }
  vdp_dma_lean(cells ? cells : kZeros, MD_VRAM_PLANE_A + (py * 64 + px) * 2, n1);
  if (n2) vdp_dma_lean(cells ? cells + n1 : kZeros, MD_VRAM_PLANE_A + py * 128, n2);
}

/* filas lógicas que se pueden tocar en la otra mitad sin pisar la imagen que se ve: la fila Y de una mitad es la
   Y + 32 de la otra, así que se evitan las filas de la imagen mostrada + 32 */
static uint8 m7_row_ok(int16 y) {
  if (m7_shown < 0) return 1;
  const M7Image *F = &kM7Images[m7_shown];
  int16 d = (int16)((y - (F->y0 + 32)) & 63);
  if (d < (int16)F->h) return 0;
  return 1;
}

/* ¿cabe todavía una DMA de n palabras en lo que queda del vblank? Se mira el contador de líneas en cada paso (el
   68000 queda parado durante cada DMA, así que el contador ya incluye lo hecho), con 4 líneas de margen. */
static int16 m7_keep;          /* líneas que se dejan además (vblank sin cuadro: las de un cuadro que llegue tarde) */
static uint8 m7_fits(uint16 n) { return vb_lines_left() - 4 - m7_keep > (int16)(n / GA_WORDS_LINE) + 1; }

static void m7_build(void) {   /* armar la imagen siguiente en la otra mitad, con lo que queda del vblank */
  if (!m7_busy || m7_step == 3) return;
  uint8 bb = m7_buf ^ 1;
  const M7Image *I = m7_load >= 0 ? &kM7Images[m7_load] : 0;
  if (m7_step == 0) {                                  /* tiles, de a 16 */
    uint16 nt = I ? I->ntiles : 0;
    while (m7_loaded < nt) {
      uint16 n = nt - m7_loaded;
      if (n > 16) n = 16;
      if (!m7_fits(n * 16)) return;
      uint8 lp = md_cur_page;
      map_page_ext(CINE_PAGE_kM7Tiles);
      vdp_dma_vram(kM7Tiles + I->toff + (uint32)m7_loaded * 32,
                   (m7_tile0() + m7_load_half * m7_region() + m7_loaded) * 32, n * 16);
      map_page_ext(lp);
      m7_loaded += n;
    }
    m7_step = 1; m7_row = 0;
  }
  if (m7_step == 1) {                                  /* borrar lo de la anterior que la nueva no tapa */
    if (m7_bh) {
      int16 nx0 = I ? I->x0 : 0, nx1 = I ? I->x0 + I->w : 0, ny0 = I ? I->y0 : 0, ny1 = I ? I->y0 + I->h : 0;
      while (m7_row < m7_bh) {
        int16 y = m7_by0 + m7_row;
        if (!m7_fits(m7_bw + M7_ROW_COST)) return;
        if (!m7_row_ok(y)) {                           /* es una fila de la que se ve: fuera de sus columnas va vacía */
          const M7Image *F = &kM7Images[m7_shown];
          if (m7_bx0 < F->x0) m7_row_dma(bb, y, m7_bx0, (uint16)((F->x0 < m7_bx0 + m7_bw ? F->x0 : m7_bx0 + m7_bw) - m7_bx0), 0);
          if (m7_bx0 + m7_bw > F->x0 + F->w) {
            int16 x = F->x0 + F->w > m7_bx0 ? F->x0 + F->w : m7_bx0;
            m7_row_dma(bb, y, x, (uint16)(m7_bx0 + m7_bw - x), 0);
          }
        } else {
          if (y >= ny0 && y < ny1) {
            if (m7_bx0 < nx0) m7_row_dma(bb, y, m7_bx0, (uint16)((nx0 < m7_bx0 + m7_bw ? nx0 : m7_bx0 + m7_bw) - m7_bx0), 0);
            if (m7_bx0 + m7_bw > nx1) {
              int16 x = nx1 > m7_bx0 ? nx1 : m7_bx0;
              m7_row_dma(bb, y, x, (uint16)(m7_bx0 + m7_bw - x), 0);
            }
          } else {
            m7_row_dma(bb, y, m7_bx0, m7_bw, 0);
          }
        }
        m7_row++;
      }
    }
    m7_step = 2; m7_row = 0;
  }
  if (m7_step == 2) {                                  /* celdas de la nueva */
    if (I) {
      while (m7_row < I->h) {
        int16 y = I->y0 + m7_row;
        if (!m7_fits(I->w + M7_ROW_COST)) return;
        if (m7_row_ok(y)) m7_row_dma(bb, y, I->x0, I->w, m7_cells + m7_row * I->w);
        m7_row++;
      }
    }
    m7_step = 3;
  }
}

#ifdef MD_M7LOG
volatile int16 md_m7_in, md_m7_out;   /* líneas del vblank al entrar y al salir de cine_vblank (medición) */
static void cine_vblank(void) { md_m7_in = vb_lines_left(); m7_build(); md_m7_out = vb_lines_left(); }
#else
static void cine_vblank(void) { m7_build(); }
#endif
/* vblank sin cuadro nuevo (el motor se atrasó): en las cinemáticas se sigue armando la imagen del modo 7 con lo que
   sobra. Cuándo se muestra lo decide el presupuesto (m7_cost), así que esto solo hace que el armado llegue antes. */
static void md_vblank_lag(void) {
  if ((cine_stage != 1 && cine_stage != 2) || m7_lock || room_pending || md_vdp_busy || !m7_busy || m7_step == 3)
    return;
  uint16 sv = vb_enter(md_snd_frames != 0);
  md_vdp_busy = 1;
  vdp_reg(0x0F, 0x02);                         /* (el círculo la deja en 0) */
  m7_keep = md_vb_need;
  m7_build();
  m7_keep = 0;
  if (cv_active) vdp_reg(0x0F, 0x00);          /* la dirección del VDP para el círculo la repone md_vint_top */
  md_vdp_busy = 0;
  vb_exit(sv);
}

static const MdRoom *find_room_id(uint16 id) {
  for (uint16 i = 0; i < MD_ROOM_COUNT; i++)
    if (kMdRooms[i].room == id) return &kMdRooms[i];
  return &kMdRooms[0];
}
static void cine_start(uint8 stage, uint16 id) {
  md_sync();
  m7_lock = 1;
  md_use_room(find_room_id(id));
  cine_stage = stage;
  cine_var = 1;
  m7_want = m7_shown = -1;
  m7_bh = 0;
  m7_cost = 0;
  m7_buf = m7_half = 0;
  m7_busy = 0;
  cv_line = 0;
  m7_lock = 0;
}
void CinematicGfx(uint16 which) { cine_start((uint8)which, 0xC000 | which); }
void MenuGfx(void) { cine_start(3, 0xC0FF); }
void Mode7Transfers(uint16 list) {
  if (list == addr_kCinematicFunction_Intro_Func76_M7_0) cine_var = 2;
  else if (list == addr_kCinematicFunction_Intro_Func76_M7_2) cine_var = 3;
}
void DrawCinematicSpritemap(uint16 sm, uint16 x, uint16 y, uint16 chr, uint8 off) {   /* $81:879F con banco $8C */
  if (intro_on) { if (!it_skip_cursor(sm, x, y)) it_draw(INTRO_KIND_SPR8C, sm, chr, it_x9(x), (uint8)y); return; }
  if (!drawkey(9, sm, chr, 0, (int16)x, (int16)y - 2)) missing(9, sm, chr, 0);   /* sin HUD: sin las 2 líneas del juego */
}
#include "nmath.h"
#include "md_pause.c"                         /* la pausa: mapa y equipo (docs/49) */
void DrawMenuSpritemap(uint16 idx, uint16 x, uint16 y, uint16 chr) {   /* $81:891F */
  if (pz_on) { pz_spr(idx, x, y, chr); return; }
  if (!drawkey(10, idx, chr, 0, (int16)x, (int16)y - 2)) missing(10, idx, chr, 0);
}

/* nivel de la sala en otra página del SSF2 (room.h): la ventana 7 (0x380000) muestra esa página; 0 = la normal */
void nt_map_level_page(uint8 page) {
  __asm__ volatile("" ::: "memory");
  md_cur_page = md_level_page = md_hw_page = page ? page : 7;
  *(volatile uint8 *)0xA130FF = md_cur_page;
  __asm__ volatile("" ::: "memory");
}

/* ------------------------------------------------------------------ inicio */
extern const uint8 kInitBlob[], kInitBlobIntro[];

void md_init(uint8 intro) {
  const uint8 *p = intro ? kInitBlobIntro : kInitBlob;   /* la intro: la RAM del SNES al elegir partida nueva */
  for (const InitEnt *e = kInitState; e->ptr; e++) {
    memcpy(e->ptr, p, e->size);
    p += (e->size + 1) & ~1;
  }
  irq_reset();
  lvl_attach();                              /* el nivel de la sala del estado inicial (room.h) */
  cv_setup_code();
  memset(&g_hooks, 0, sizeof(g_hooks));
  g_hooks.room_gfx = md_room_gfx;
  hz_state = HZ_OFF;                         /* el estado inicial trae el objeto del haze: md_haze_read lo toma */
  md_haze_read();

  vdp_setup();
  vdp_dma_vram(kHudTiles, HUD_BASE * 32, HUD_TILE_COUNT * 16);
  vdp_fill_vram(0, 0, 16);                                   /* tile 0 vacío */
  hud_dirty_all = 1;
  md_room_gfx();
}

/* ------------------------------------------------------------------ cuadro */
static const SamusTileDef *find_tiledef_bs(uint16 addr);
static const SamusTileDef *find_tiledef(uint16 addr) {
  static uint16 last_addr[2]; static const SamusTileDef *last_def[2];   /* Samus repite las mismas mitades */
  if (last_def[0] && last_addr[0] == addr) return last_def[0];
  if (last_def[1] && last_addr[1] == addr) return last_def[1];
  const SamusTileDef *d = find_tiledef_bs(addr);
  last_addr[1] = last_addr[0]; last_def[1] = last_def[0];
  last_addr[0] = addr; last_def[0] = d;
  return d;
}
static const SamusTileDef *find_tiledef_bs(uint16 addr) {
  int lo = 0, hi = SAMUS_TILEDEF_COUNT - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    uint16 a = kSamusTileDefs[m].addr;
    if (a == addr) return &kSamusTileDefs[m];
    if (a < addr) lo = m + 1; else hi = m - 1;
  }
  return 0;
}

/* HUD: las filas de hud_tilemap que cambiaron pasan a celdas de la Genesis aquí, fuera del vblank */
static void hud_prepare(void) {
  uint8 all = hud_dirty_all;
  if (!all) {                                    /* lo normal: nada cambió (las 3 filas, 48 comparaciones largas) */
    const uint16 *a = hud_tilemap, *b = hud_shown;
    uint16 n = 47, r;
    __asm__ volatile("1: cmpm.l (%1)+,(%2)+\n\tdbne %3,1b\n\tseq %0" : "=d"(r), "+a"(a), "+a"(b), "+d"(n) : : "cc", "memory");
    if (r & 1) return;
  }
  if (all) { hud_dirty_all = 0; hud_black = 1; }
  uint8 rows = 0;
  for (uint16 r = 0; r < 3; r++) {               /* solo las celdas que cambiaron (casi siempre una o dos) */
    const uint16 *w = &hud_tilemap[r * 32];
    uint16 *b = &hud_shown[r * 32];
    uint16 *o = &hud_vram[r * 32];
    uint8 ch = 0;
    for (uint16 i = 0; i < 32; i++) {
      uint16 v = w[i];
      if (v == b[i] && !all) continue;
      b[i] = v;
      o[i] = kHudLut[v & 0x1FFF] | ((v >> 3) & 0x1800);
      ch = 1;
    }
    if (ch) rows |= 1 << r;
  }
  hud_rows |= rows;
}

/* primera parte del cuadro: la lógica del motor (puede correr antes de que el vblank tome el cuadro anterior) */
uint8 zebes_eye_room(void);
uint8 md_spr_reserved;
void md_logic(uint16_t joy) {
  sp_ptr = spr;
  md_spr_reserved = zebes_eye_room();
  if (md_spr_reserved) sp_ptr = spr + 2;       /* salas con el ojo: dos sprites que tapan el HUD (md_cone.c) */
  door_seen = 0;
  flash_seen = 0;
  game_frame(viewer_input(joy));
  { extern uint8 nt_frame_noupdate;              /* cuadro sin NMI con actualización (caja de mensaje): el SNES deja
                                                    la OAM como estaba: la lista anterior, sin su caja */
    if (nt_frame_noupdate) {
      sp_ptr = spr;
      for (uint16 i = mb_prev_n; i < nspr_out && i < MAX_SPR; i++) *sp_ptr++ = spr_out[i];
    } }
}

/* lo que la lógica dejó en sus colas pasa a las del vblank (el vblank ya tomó el cuadro anterior: las suyas están
   vacías) */
static void pipe_take(void) {
  for (uint16 i = 0; i < text_nl; i++) { text_q[i][0] = text_ql[i][0]; text_q[i][1] = text_ql[i][1]; }
  text_n = text_nl; text_nl = 0;
  for (uint16 i = 0; i < dyn_nl; i++) dyn_q[i] = dyn_ql[i];
  dyn_n = dyn_nl; dyn_nl = 0;
  if (arm_dmal) { arm_dma = arm_dmal; arm_dmal = 0; }
  if (ga_called) ga_want = ga_want_l;
  m7_variant = m7_variant_l;
  if (md_zebes) zebes_take();
}

/* fundido a negro: cram_out = los colores con cada componente (3 bits) por brillo/15. Se arma al preparar el cuadro
   (md_post), no en el vblank: son ~25 líneas que se comían el vblank de la cinemática (docs/50) */
static const uint8 kBright[16][8] = {   /* (v * brillo + 7) / 15 (sin divisiones: el 68000 no tiene de 32 bits) */
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1 },
  { 0, 0, 0, 1, 1, 1, 1, 1 },
  { 0, 0, 1, 1, 1, 1, 2, 2 },
  { 0, 0, 1, 1, 1, 2, 2, 2 },
  { 0, 0, 1, 1, 2, 2, 2, 3 },
  { 0, 0, 1, 1, 2, 2, 3, 3 },
  { 0, 1, 1, 2, 2, 3, 3, 4 },
  { 0, 1, 1, 2, 2, 3, 4, 4 },
  { 0, 1, 1, 2, 3, 3, 4, 5 },
  { 0, 1, 1, 2, 3, 4, 4, 5 },
  { 0, 1, 2, 2, 3, 4, 5, 6 },
  { 0, 1, 2, 3, 3, 4, 5, 6 },
  { 0, 1, 2, 3, 4, 5, 6, 7 },
  { 0, 1, 2, 3, 4, 5, 6, 7 } };
static uint8 cram_out_ok;                       /* cram_out ya tiene los colores de ahora con el brillo de ahora */
static void cram_bright(void) {
  const uint16 *src = (md_tint && cram_t_ok) ? cram_t : cram;
  const uint8 *bt = kBright[md_bright & 15];
  for (uint16 i = 0; i < 64; i++) {
    uint16 c = src[i];
    cram_out[i] = (uint16)(bt[(c >> 1) & 7] << 1 | bt[(c >> 5) & 7] << 5 | bt[(c >> 9) & 7] << 9);
  }
  cram_out_ok = 1;
}

/* segunda parte: con el vblank libre (md_sync), se arma lo que va al VDP */
uint16 md_cone_frame(void);
const void *md_cone_sat(uint8 bank);
void md_cone_color(void);
void md_cone_vblank(void);
void md_cone_room(void);
void md_post(void) {
  if (pz_dim) { md_pause_frame(); return; }   /* pausa (estados 12-18): md_pause.c */
  pipe_take();
  md_haze_read();
  ncone = (md_zebes && !cine_stage && !nt_msgbox_active && !mb_shown) ? md_cone_frame() : 0;
  if (md_zebes) md_msgbox_post();
  /* conos de luz (md_cone.c): al final de la lista de sprites */
  nspr = (uint16)(sp_ptr - spr);
#ifdef MD_SPRSUM
  { uint16 sum = nspr; const uint16 *w = (const uint16 *)spr; for (uint16 i = 0; i < nspr * 4; i++) sum = (sum << 1 | sum >> 15) ^ w[i];
    if ((uint16)(md_frame_count - MD_SPRSUM) < 256) md_spr_sum[(uint16)(md_frame_count - MD_SPRSUM)] = sum; }
#endif
  if (g_room->door_sm) m7_variant = door_seen;    /* celdas de la puerta en el plano A mientras Ridley la dibuja */
  /* fin de la huida: si el motor deja de pedir cuadros con Ridley todavía en el plano B, se pasa al último cuadro
     (vacío): su cambio repone el plano B de la sala */
  if (!ga_called && ga_shown >= 0 && kGetaway[ga_shown].w) ga_want = GETAWAY_FRAMES - 1;
  ga_called = 0;
  md_frame_count++;
  /* tiles de Samus (NMI: NmiTransferSamusToVram) */
  if (nmi_copy_samus_halves & 0xFF) {
    const SamusTileDef *d = find_tiledef(nmi_copy_samus_top_half_src);
    if (d) { samus_dma_src[0] = kSamusTileBlob + d->first * 32; samus_dma_tiles[0] = d->count; }
    else missing(8, nmi_copy_samus_top_half_src, 0, 0);
  }
  if (nmi_copy_samus_halves & 0xFF00) {
    const SamusTileDef *d = find_tiledef(nmi_copy_samus_bottom_half_src);
    if (d) { samus_dma_src[1] = kSamusTileBlob + d->first * 32; samus_dma_tiles[1] = d->count; }
    else missing(8, nmi_copy_samus_bottom_half_src, 1, 0);
  }
  if (intro_on) {                               /* la intro: su propio dibujo (md_intro.c) */
    if (game_state == 30) { intro_frame(); return; }
    intro_on = 0;
  }
  /* el tinte de la matemática de color (ver tint15) */
  {
    int16 r = 0, g = 0, b = 0;
    if (md_zebes && !cine_stage && hz_state == HZ_OFF) {
      uint8 cfg = (uint8)fx_layer_blending_config_c;
      if (cfg == 0x2A) { r = -6; g = -2; }                      /* LayerBlendFunc_2A: COLDATA 38/66/0x80, resta */
      else if (cfg == 0x28) { r = -5; }                         /* LayerBlendFunc_28: COLDATA 37/64/0x80, resta */
      else if (cfg == 0x2C) { r = MD_TORIZO_HAZE_R; g = MD_TORIZO_HAZE_G; b = MD_TORIZO_HAZE_B; }
      if (fx_type == 0x0C) {                                    /* niebla: su BG3 sumado */
        uint32 sr = 0, sg = 0, sb = 0;
        for (uint16 i = 0; i < 32; i++) {
          uint16 w = kFogW[i], c = palette_buffer[i];
          if (!w) continue;
          sr += (uint32)w * (c & 31); sg += (uint32)w * ((c >> 5) & 31); sb += (uint32)w * ((c >> 10) & 31);
        }
        r += (int16)((sr + 32768) >> 16); g += (int16)((sg + 32768) >> 16); b += (int16)((sb + 32768) >> 16);
      }
    }
    uint8 on = r || g || b;
    if (on != tint_on || r != tint_r || g != tint_g || b != tint_b) {
      tint_on = on; tint_r = (int8)r; tint_g = (int8)g; tint_b = (int8)b;
      nt_palette_changed = 1; nt_palette_lo = 0; nt_palette_hi = 255;
    }
  }
  /* colores: cada uno sale de un índice de palette_buffer (tabla de la sala), solo si la paleta cambió */
  if (nt_palette_changed) {
    nt_palette_changed = 0;
    const uint8 *pb = (const uint8 *)palette_buffer;
    const uint8 *lut = (const uint8 *)kCram555;
    if (nt_palette_hi - nt_palette_lo < 32) {
      /* pocos índices (ciclo de color de la puerta, paleta de Samus): solo los colores que salen de ellos */
      for (uint16 i = nt_palette_lo; i <= nt_palette_hi; i++) {
        uint8 slot = cram_rev[i];
        if (slot == 0xFF) continue;
        do {
          uint16 v = *(const uint16 *)(lut + (uint16)(*(const uint16 *)(pb + i * 2) << 1));
          if (tint_on && slot < 48) v = kCram555[tint15(palette_buffer[i] & 0x7FFF)];
          if (v != cram[slot]) { cram[slot] = v; cram_dirty = 1; }
          slot = cram_next[slot];
        } while (slot != 0xFF);
      }
    } else {
      const uint16 *off = cram_off;
      uint16 *c = cram;
      for (uint16 i = 64; i; i--) {
        uint16 o = *off++;
        if (o == 0xFFFE) { c++; continue; }
        uint16 v = o == 0xFFFF ? 0 : *(const uint16 *)(lut + (uint16)(*(const uint16 *)(pb + o) << 1));
        if (tint_on && c < cram + 48 && o != 0xFFFF) v = kCram555[tint15(*(const uint16 *)(pb + o) & 0x7FFF)];
        if (v != *c) { *c = v; cram_dirty = 1; }
        c++;
      }
    }
    if (palette_buffer[0] != hz_last_c) hz_bd_dirty = 1;   /* el degradado sale solo del color de fondo */
    rl_pal_dirty = 1;
  }
  if (md_zebes) { zebes_obj_colors(); md_cone_color(); }
  mb_cram_override();   /* colores de los enemigos (debajo del HUD) y del cono */
  /* La línea de Ridley (paleta 7 de los sprites del SNES), cada cuadro:
     - parpadeo: toma los colores de la paleta 0 del SNES (allí: los mismos tiles con otra paleta);
     - haze: en el SNES el color math suma el degradado también a los sprites de las paletas 4-7, así que Ridley
       oscuro antes de la pelea no se ve contra el fondo. Aquí se suma a sus colores el nivel de la banda donde
       está (un solo nivel para todo Ridley; en el SNES cambia cada 8 líneas). */
  if (g_room->flash_line) {
    const uint8 *pb = (const uint8 *)palette_buffer;
    const uint16 *lut = kCram555;
    uint16 L = g_room->flash_line * 16, add = 0;
    if (hz_state != HZ_OFF) {
      int16 band = (int16)((int16)(enemy_data[0].y_pos - layer1_y_pos) >> 3) - 6;
      add = band < 0 ? 0 : hz_tab[band > 15 ? 15 : band] & 31;
    }
    static uint16 rl_add = 0xFFFF, rl_lit;
    if (rl_pal_dirty || add != rl_add || flash_shown != flash_seen || flash_seen) {   /* solo si algo cambió */
    rl_pal_dirty = 0;
    rl_add = add;
    uint16 lit = 0;
    for (uint16 c = 1; c < 16; c++) {
      uint16 o = flash_seen ? (uint16)((g_room->flash_base + c) * 2) : cram_off[L + c];
      uint16 v = 0;
      if (o != 0xFFFF) {
        uint16 sc = *(const uint16 *)(pb + o) & 0x7FFF;
        lit |= sc;
        if (add) {
          if (hz_red) { uint16 r = (sc & 31) + add; sc = (sc & ~31) | (r > 31 ? 31 : r); }
          else { uint16 bl = ((sc >> 10) & 31) + add; sc = (sc & 0x03FF) | ((bl > 31 ? 31 : bl) << 10); }
        }
        v = lut[sc];
      }
      if (v != cram[L + c]) { cram[L + c] = v; cram_dirty = 1; }
    }
    flash_shown = flash_seen;
    rl_lit = lit;
    }
    uint16 lit = rl_lit;
    /* toda negra (antes de la pelea): en el SNES, negro + haze = el fondo de cada banda, no se ve. Con un solo
       nivel para todo Ridley quedaría una silueta: sus sprites se sacan de la lista. */
    if (!lit && add) {
      GenSpr *d = spr;
      for (GenSpr *q = spr; q < sp_ptr; q++)
        if (((q->attr >> 13) & 3) != g_room->flash_line) *d++ = *q;
      for (GenSpr *q = spr; q < d; q++) q->size_link = (q->size_link & 0x0F00) | (uint16)(q - spr + 1);
      sp_ptr = d;
      nspr = (uint16)(d - spr);
    }
  }
  if (ga_shown >= 0 && kGetaway[ga_shown].w) {     /* huida: la línea 1 tiene los colores del cuadro mostrado */
    for (uint16 c = 0; c < 15; c++)
      if (cram[17 + c] != kGetaway[ga_shown].pal[c]) { cram[17 + c] = kGetaway[ga_shown].pal[c]; cram_dirty = 1; }
  }
  if (hz_state != HZ_OFF && hz_bd_dirty) { hz_bd_dirty = 0; hz_build_backdrop(); }
  /* scroll: el plano A es la sala entera (o el plano de modo 7); el B, la capa 2 o el fondo de biblioteca */
  scroll_ax = layer1_x_pos + nt_shake_bg1x;      /* temblor (HandleRoomShaking): lo que el SNES suma a BG1HOFS */
  /* Medido contra el SNES: allí el HUD ocupa las líneas 0-29 y el juego empieza en la 30; aquí la ventana del HUD
     ocupa 32 líneas (4 filas). Toda la imagen va 2 líneas más abajo que en el SNES: el HUD queda igual respecto
     del juego, y el juego empieza justo debajo de la ventana. */
  scroll_ay = layer1_y_pos + nt_shake_bg1y;
  if (g_room->b_mode == 1) { scroll_bx = reg_BG2HOFS; scroll_by = reg_BG2VOFS; }
  else { scroll_bx = layer2_x_pos + nt_shake_bg2x; scroll_by = layer2_y_pos + nt_shake_bg2y; }
  if (cine_stage) cine_frame(); else hud_prepare();
  if (md_zebes && !cine_stage) {
    zebes_frame(scroll_ax, scroll_ay, scroll_bx, scroll_by);
    if (zebes_sky_on()) zebes_sky_lines(rot_h, scroll_ax, scroll_ay);
  }
  /* modo 7 girado (pozo en la huida) */
  if (g_room->wall) ring_follow();
  rot_on = g_room->wall && reg_M7B != 0;
  if (rot_on) {
    int16 hofs = scroll_ax, vofs = scroll_ay;
    int16 B = (int16)reg_M7B, C = (int16)reg_M7C;
    int32 h = -((int32)hofs << 8) - (int32)B * ((int16)(vofs - 2) - (int16)reg_M7Y);   /* línea 0 de la Genesis = línea -2 del SNES */
    /* por línea: la parte entera de h/256 para los dos planos, h -= B. En 16.16 (h << 8) la parte entera es la
       palabra alta (swap): ~37 ciclos por línea en lugar de ~80 con el desplazamiento de 32 bits */
    {
      register uint32 H __asm__("d0") = (uint32)h << 8;
      register int32 S __asm__("d1") = -((int32)B << 8);
      register uint16 *d __asm__("a0") = rot_h;
      register uint16 n __asm__("d4") = 224 / 2 - 1;
      __asm__ volatile(
        "1: move.l %0,%%d2\n"
        "   swap %%d2\n"
        "   move.w %%d2,(%2)+\n"
        "   move.w %%d2,(%2)+\n"
        "   add.l %1,%0\n"
        "   move.l %0,%%d2\n"
        "   swap %%d2\n"
        "   move.w %%d2,(%2)+\n"
        "   move.w %%d2,(%2)+\n"
        "   add.l %1,%0\n"
        "   dbra %3,1b\n"
        : "+d"(H), "+d"(S), "+a"(d), "+d"(n) : : "d2", "memory", "cc");
    }
    int32 vv = ((int32)vofs << 8) + (int32)C * ((int16)(8 + hofs) - (int16)reg_M7X);
    for (uint16 c = 0; c < 16; c++) {
      uint16 v = (uint16)(vv >> 8);
      rot_v[2 * c] = v;
      rot_v[2 * c + 1] = v;
      vv += (int32)C * 16;
    }
  }
  /* brillo: al final de Ceres, en las cinemáticas y en el fin de juego; en el resto el juego no usa INIDISP para fundir */
  uint8 b = 15;
  if ((game_state >= 32 && game_state <= 37) || game_state == 25 || game_state == 26 || game_state == 6)
    b = (reg_INIDISP & 0x80) ? 0 : (reg_INIDISP & 0xF);
  if (b != md_bright) { md_bright = b; cram_dirty = 1; }
  /* huida: en el SNES el haze rojo (Ridley ya se fue) suma un degradado de rojo a todo con el color math (de 0 arriba
     a 15/31 abajo). La Genesis no puede cambiar todos los colores línea a línea: suma un rojo fijo (el del medio de
     la pantalla) a los colores de la sala y de los sprites, menos el HUD; el degradado queda en el color de fondo. */
  uint8 t = (!cine_stage && hz_state != HZ_OFF && hz_red) ? 2 : 0;
  if (t != md_tint) { md_tint = t; cram_dirty = 1; cram_t_ok = 0; }
  if (md_tint && (cram_dirty || !cram_t_ok)) {
    for (uint16 i = 0; i < 64; i++) {
      uint16 c = cram[i];
      if ((i & 15) == 0 || (i >= 48 && i <= 48 + HUD_NSLOTS)) { cram_t[i] = c; continue; }
      uint16 r = ((c >> 1) & 7) + md_tint;
      cram_t[i] = (c & ~0xE) | ((r > 7 ? 7 : r) << 1);
    }
    cram_t_ok = 1;
    cram_dirty = 1;
  }
  if (cram_dirty && md_bright != 15) cram_bright();
}

/* ------------------------------------------------------------------ vblank */
static uint8 hud_tiles_lost;                   /* las cinemáticas usaron los tiles del HUD (CINE_BG_BASE, círculo) */
static void load_room_vram(void) {
  const MdRoom *R = g_room;
  samus_vram_src[0] = samus_vram_src[1] = 0;   /* VRAM nueva: los tiles de Samus se copian otra vez */
  vdp_display(0);
  if (cine_stage) hud_tiles_lost = 1;
  else if (hud_tiles_lost) {                    /* de vuelta al juego: el HUD otra vez con sus tiles */
    hud_tiles_lost = 0;
    vdp_dma_vram(kHudTiles, HUD_BASE * 32, HUD_TILE_COUNT * 16);
  }
  if (md_zebes && !cine_stage) {
    zebes_load_vram(scroll_ax, scroll_ay, scroll_bx, scroll_by);
    vdp_reg(0x12, 0x04);
    vdp_display(1);
    return;
  }
  vdp_reg(0x10, R->plane_w == 32 && !R->wall ? 0x30 : 0x11);             /* 32x128 o 64x64 */
  vdp_dma_vram(R->tiles, (cine_stage ? CINE_BG_BASE : BG_BASE) * 32, R->tiles_total * 16);
  if (R->wall) {                                /* pozo de modo 7: pared a la derecha, el pozo alrededor de la cámara */
    for (uint16 r = 0; r < 64; r++) {
      vdp_set_vram_write(MD_VRAM_PLANE_A + r * 128 + 64);
      for (uint16 c = 0; c < 32; c++) VDP_DATA = R->wall[(r & 1) * 2 + (c & 1)];
    }
    ring_top = ((int16)scroll_ay >> 3) - RING_ABOVE;
    for (int16 i = 0; i < 64; i++) ring_row_write(ring_top + i);
    ring_n = 0;
  } else
    vdp_dma_vram(R->planeA, MD_VRAM_PLANE_A, R->plane_w * R->plane_h);
  if (R->planeB) vdp_dma_vram(R->planeB, MD_VRAM_PLANE_B, R->plane_w * R->plane_h);
  else vdp_fill_vram(MD_VRAM_PLANE_B, 0, 0x1000);
  vdp_reg(0x12, cine_stage ? 0x00 : 0x04);     /* ventana del HUD: no en las cinemáticas ni en los menús */
  if (cine_stage == 1) {                        /* tiles del círculo de la explosión (donde va el HUD) */
    circ_vram();
  }
  vdp_display(1);
}

#if defined(MD_LAGLOG) || defined(MD_M7LOG)
volatile uint8 md_vbs[8];
#define VBS(i) md_vbs[i] = VDP_VCOUNT >> 8
#else
#define VBS(i)
#endif
void md_vblank(uint16_t late) {
  VBS(0);
  uint16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");   /* sin interrupciones mientras se escribe el VDP */
  md_vdp_busy = 1;
  vdp_reg(0x00, 0x04);                        /* sin interrupción horizontal mientras se escribe el VDP */
  vdp_reg(0x0F, 0x02);                        /* (la del círculo la deja en 0) */
  if (room_pending) {
    room_pending = 0;
    md_vb_room = 1;
    load_room_vram();
    md_cone_room();
    hud_dirty_all = 1;
    cram_dirty = 1;
    cram_t_ok = 0;
    cram_out_ok = 0;
  }
  VBS(1);
  /* huida: primero, con todo el vblank (sus cambios de celdas grandes no caben después del resto); el DMA que
     haga se mide con lo que queda del vblank */
  ga_vblank();
  /* sprites */
  GenSpr *so = spr_out;
  uint16 n = nspr_out;
  if (n == 0) { so[0].y = 0; so[0].size_link = 0; so[0].attr = 0; so[0].x = 0; n = 1; }
  so[n - 1].size_link &= 0x0F00;             /* los enlaces se escriben al armar la lista; el último termina */
  uint16 c = ncone_out;                      /* el haz: el último de la lista enlaza con su primero */
  if (c) so[n - 1].size_link |= MAX_SPR - c;
  vdp_dma_vram(so, MD_VRAM_SPRITES, n * 4);
  if (nspr_shown > n) vdp_fill_vram(MD_VRAM_SPRITES + n * 8, 0, (nspr_shown - n) * 4);
  nspr_shown = n;
  if (c) vdp_dma_vram(md_cone_sat(md_spr_bank(so)), MD_VRAM_SPRITES + (MAX_SPR - c) * 8, c * 4);
  VBS(2);
  if (mb_dma_n) { vdp_dma_vram(mb_buf, mb_dma_dst, mb_dma_n); mb_dma_n = 0; }
  if (!mb_shown) md_cone_vblank();
  /* tiles dinámicos de Ridley */
  for (uint16 i = 0; i < dyn_n; i++)
    vdp_dma_vram(kDynTiles + dyn_q[i]->off, dyn_q[i]->tile * 32, 64);
  dyn_n = 0;
  /* tiles de Samus */
  if (vb_lines_left() >= 14) {                 /* si la huida se llevó el vblank, los de Samus esperan al siguiente */
    /* los que ya están en la VRAM no se vuelven a copiar (el SNES los copia cada cuadro; aquí la DMA para al 68000) */
    if (samus_dma_src[0] && (samus_dma_src[0] != samus_vram_src[0] || samus_dma_tiles[0] != samus_vram_n[0])) {
      vdp_dma_vram(samus_dma_src[0], SAMUS_BASE * 32, samus_dma_tiles[0] * 16);
      samus_vram_src[0] = samus_dma_src[0]; samus_vram_n[0] = samus_dma_tiles[0];
    }
    if (samus_dma_src[1] && (samus_dma_src[1] != samus_vram_src[1] || samus_dma_tiles[1] != samus_vram_n[1])) {
      vdp_dma_vram(samus_dma_src[1], (SAMUS_BASE + 16) * 32, samus_dma_tiles[1] * 16);
      samus_vram_src[1] = samus_dma_src[1]; samus_vram_n[1] = samus_dma_tiles[1];
    }
    samus_dma_src[0] = samus_dma_src[1] = 0;
    if (intro_on || cine_stage) samus_vram_src[0] = samus_vram_src[1] = 0;   /* ahí se usa la VRAM para otras cosas */
  }
  if (arm_dma) { vdp_dma_vram(arm_dma, ARM_TILE * 32, 16); arm_dma = 0; }
  if (intro_on) {
    intro_vblank();
    md_vdp_busy = 0;
    __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
    return;
  }
  VBS(3);
  /* colores (con haze, la CRAM se rehace en cada vblank: las interrupciones la cambian a mitad de pantalla) */
  if (cram_dirty) {
    cram_dirty = 0;
    const uint16 *src = (md_tint && cram_t_ok) ? cram_t : cram;
    if (md_bright == 15) {
      vdp_write_cram(src, 64);
    } else {                                  /* fundido a negro: cada componente (3 bits) por brillo/15 */
      if (!cram_out_ok) cram_bright();
      vdp_write_cram(cram_out, 64);
    }
    md_bright_shown = md_bright;
  }
  if (hz_bd_ready) {                          /* lo armado en este cuadro pasa a la pantalla */
    hz_bd_ready = 0;
    hz_back ^= 1;
  }
  hz_bdp = hz_bd[hz_back ^ 1];
  VBS(4);
  /* HUD: filas 1-3 del plano Window (convertidas en md_frame); la fila 0 es negra */
  if (hud_black) {
    hud_black = 0;
    vdp_set_vram_write(MD_VRAM_WINDOW);
    for (uint16 i = 0; i < 32; i++) VDP_DATA = HUD_BLACK_CELL;
  }
  for (uint16 r = 0; hud_rows; r++, hud_rows >>= 1)
    if (hud_rows & 1) vdp_dma_vram(&hud_vram[r * 32], MD_VRAM_WINDOW + 64 + r * 64, 32);
  for (uint16 i = 0; i < text_n; i++) {        /* texto de la huida */
    vdp_set_vram_write(text_q[i][0]);
    VDP_DATA = text_q[i][1];
  }
  text_n = 0;
  if (md_zebes && !cine_stage && !room_pending) zebes_vblank();
  /* puerta del pozo (modo 7): dos versiones de unas celdas */
  uint8 repatch = 0;
  if (ring_n) {                                 /* pozo de modo 7: filas nuevas del anillo (desde la ROM: versión 0) */
    if (g_room->wall)
      for (uint16 i = 0; i < ring_n; i++) ring_row_write(ring_q[i]);
    ring_n = 0;
    repatch = m7_variant_shown != 0;
  }
  if (g_room->npatch && (m7_variant != m7_variant_shown || repatch)) {
    m7_variant_shown = m7_variant;
    const uint16 *p = g_room->patch;
    for (uint16 i = 0; i < g_room->npatch; i++, p += 3) {
      uint16 a = p[0];
      if (g_room->wall) {                       /* plano de 64 columnas en anillo: solo si la fila está cargada */
        int16 y = (int16)(a >> 5);
        if (y < ring_top || y >= ring_top + 64) continue;
        a = (uint16)(y & 63) * 64 + (a & 31);
      }
      vdp_set_vram_write(MD_VRAM_PLANE_A + a * 2);
      VDP_DATA = p[1 + m7_variant];
    }
  }
  VBS(5);
  /* scroll */
  uint8 zsky = !cine_stage && zebes_sky_on();
  uint8 smode = rot_on ? 0x07 : (cv_line || zsky) ? 0x03 : 0x00;   /* por línea y por columna, por línea, o la pantalla entera */
  uint8 smode_changed = smode != rot_shown;
  if (smode_changed) {
    rot_shown = smode;
    vdp_reg(0x0B, smode);
  }
  if (cv_line) {                              /* círculo: el scroll horizontal por línea y la tabla nueva */
    if (cv_new || smode_changed) {
      cv_new = 0;
      cv_cur = cv_build;
      cv_build ^= 1;
      vdp_dma_vram(rot_h, MD_VRAM_HSCROLL, 224 * 2);
    }
    vdp_set_vsram_write(0);
    VDP_DATA = scroll_ay;
    VDP_DATA = cv_tab[cv_cur][0];
  } else if (zsky) {                          /* cielo de Zebes: el plano B por franjas */
    vdp_dma_vram(rot_h, MD_VRAM_HSCROLL, 224 * 2);
    vdp_set_vsram_write(0);
    VDP_DATA = scroll_ay;
    VDP_DATA = scroll_ay;
  } else if (rot_on) {
    vdp_dma_vram(rot_h, MD_VRAM_HSCROLL, 224 * 2);
    vdp_set_vsram_write(0);
    for (uint16 i = 0; i < 32; i++) VDP_DATA = rot_v[i];
  } else {
    vdp_set_vram_write(MD_VRAM_HSCROLL);
    VDP_DATA = (uint16)-scroll_ax;
    VDP_DATA = (uint16)-scroll_bx;
    vdp_set_vsram_write(0);
    VDP_DATA = scroll_ay;
    VDP_DATA = scroll_by;
  }
  /* La interrupción horizontal del haze escribe en la CRAM y cambia la dirección del VDP: se vuelve a activar
     recién aquí, cuando ya no queda nada por escribir. Si se activara antes y este trabajo se alargara hasta la
     imagen (cuadro con retraso), los datos siguientes (HUD, scroll) caerían en la CRAM y cambiarían los colores. */
  /* cinemáticas: la capa del modo 7 y el círculo, con lo que queda del vblank (colores y scroll ya están) */
  if (cine_stage == 1 || cine_stage == 2) cine_vblank();
  if (cv_line) {                              /* círculo: una interrupción por línea (la dirección del VDP, la última) */
    if (!cv_active) { SYS_setHIntCallback((void (*)(void))(void *)cv_code); cv_active = 1; }
    uint16 vc = VDP_VCOUNT >> 8;
    cv_point(vc < 224 ? vc : 0);
    vdp_reg(0x0F, 0x00);
    vdp_reg(0x0A, 0x00);
    vdp_reg(0x00, 0x14);
    vdp_set_vsram_write(2);
  } else if (md_zebes && !cine_stage && zebes_obj_on()) {
    /* enemigos de Zebes: la línea 3 con los colores del HUD hasta la línea 31 */
    if (cv_active) cv_active = 0;
    uint8 want = mb_bot > mb_top && mb_top >= 66 ? 2 : 1;   /* caja de mensaje: más interrupciones */
    if (want == 2) {
      mb_steps[0].col = zobj_col; mb_steps[0].reg = 0x8A00 | (uint16)(mb_top - 1 - 64);
      mb_steps[1].col = 0;        mb_steps[1].reg = 0x8A00 | (uint16)(mb_bot - mb_top - 1);
      mb_steps[2].col = mb_col;    mb_steps[2].reg = 0x8AFF;
      mb_steps[3].col = zobj_col; mb_steps[3].reg = 0x8004;
      mb_hp = mb_steps;
    }
    if (zobj_hint != want) { SYS_setHIntCallback(want == 2 ? md_hint_msg : md_hint_obj); zobj_hint = want; }
    VDP_CTRL32 = VDP_CMD_CRAM_W(0x62);
    for (uint16 i = 0; i < 15; i++) VDP_DATA = cram[49 + i];
    vdp_reg(0x0A, 31);   /* cambio de colores en la línea 32 (la primera del juego): no en el HUD */
    vdp_reg(0x00, 0x14);
  } else {
    if (cv_active) { SYS_setHIntCallback(md_hint); cv_active = 0; }
    if (zobj_hint) { SYS_setHIntCallback(md_hint); zobj_hint = 0; vdp_reg(0x00, 0x04); }
    if (hz_state != HZ_OFF && hz_cnt[hz_back ^ 1] && md_bright == 15) {
      vdp_reg(0x0A, hz_first[hz_back ^ 1]);     /* primera interrupción; las demás las programa el manejador */
      vdp_reg(0x00, 0x14);
    }
  }
  md_vdp_busy = 0;
  VBS(6);
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
}
