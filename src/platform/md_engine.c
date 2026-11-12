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
NT_GAMEHI uint16 md_scratch[1440];
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
  "    bcc.s 1f\n"                         /* nada (si no, se comería la primera banda: docs/54) */
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
  "    cmpi.b #30,0xC00008\n"             /* antes de la línea 30 (la primera es la 31): la que salta al volver a */
  "    bcs.s 1f\n"                         /* activarlas si el vblank se pasó al HUD; nada (docs/60) */
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
   HUD, que en el SNES es el BG3): niebla (FX 0xC: el color medio de su BG3, kFogW) y las restas de color fijo
   (LayerBlendFunc_28/2A). La neblina del Bomb Torizo (LayerBlendFunc 0x2C) va por franjas: lq_prepare */
static int8 tint_r, tint_g, tint_b;
uint8 tint_on;                                /* (md_zebes.c lo lee: sin tinte, los colores de los objetos directos) */
/* el color con el tinte, ya en la CRAM: kCram555 va canal por canal (comprobado en el PC para los 32768), así que al
   cambiar el tinte se arman tres tablas de 32 (cada canal sumado, recortado y convertido) y cada color son tres
   lecturas: la bomba de poder cambia el tinte casi en cada cuadro de su explosión (docs/58) */
static uint8 tint_lr[32], tint_lg[32], tint_lb[32];   /* (bytes: la RAM es justa) */
static void tint_tables(void) {
  for (int16 v = 0; v < 32; v++) {
    int16 r = v + tint_r, g = v + tint_g, b = v + tint_b;
    r = r < 0 ? 0 : r > 31 ? 31 : r; g = g < 0 ? 0 : g > 31 ? 31 : g; b = b < 0 ? 0 : b > 31 ? 31 : b;
    tint_lr[v] = (uint8)kCram555[r]; tint_lg[v] = (uint8)(kCram555[g << 5] >> 4); tint_lb[v] = (uint8)(kCram555[b << 10] >> 8);
  }
}
static inline uint16 tint_cram(uint16 c) {
  return tint_lr[c & 31] | (uint16)tint_lg[(c >> 5) & 31] << 4 | (uint16)tint_lb[(c >> 10) & 31] << 8;
}
uint16 md_tint_cram(uint16 c) { return tint_on ? tint_cram(c) : kCram555[c & 0x7FFF]; }   /* md_zebes.c */

/* líquido de la capa 3 (lava, ácido: docs/59). En el SNES el BG3 va solo en la pantalla principal y la sala en la
   subpantalla, sumados (LayerBlendFunc_18, CGADSUB 0x24): debajo de la superficie el color del líquido se suma a todo.
   Aquí: desde la línea de la superficie, la interrupción horizontal pone una paleta con el color medio del líquido
   sumado (lq_col: las 4 líneas, la 3 con los colores de los objetos); el vblank repone la normal. El color medio sale
   de palette_buffer con los pesos de los píxeles del tilemap del FX ($8A, kFxTypeTilemapPtrs) sobre los tiles del BG3
   ($9A:B200): lava 27/26/25, ácido 3/2/1 (92/5/3 %), así sigue el brillo de su paleta (palfx). */
typedef struct __attribute__((packed)) { const uint16 *col; uint16 n; uint32 cmd; uint16 reg; } LqStep;
static const LqStep *volatile lq_hp;
extern uint16 cinematic_text_tilemap[];
#define LQ_MAXSTEP 10                             /* pasos de la interrupción: el líquido (hasta 5) y la onda de Crocomire */
#define lq_steps ((LqStep *)((uint8 *)cinematic_text_tilemap + 3968))  /* los que lee la interrupción (el vblank copia
                                                                         lq_next; md_hint_lq: la misma dirección) */
/* (la RAM es justa: en cinematic_text_tilemap + 3200, después de lo que usan en estas salas la caché de los jefes,
   md_zebes.c, y crocomire_bg2_scroll_hdma_data; la pausa y la caja de mensaje lo pisan: se rearma cada cuadro) */
#define lq_col ((uint16 *)((uint8 *)cinematic_text_tilemap + 3200))
#define lq_cols ((uint16 *)((uint8 *)cinematic_text_tilemap + 3328))    /* la franja de la superficie */
#define LQ_SURF 8                                 /* líneas de la superficie (escribir 64 colores toma ~3) */
#define lq_next ((LqStep *)((uint8 *)cinematic_text_tilemap + 3520))   /* los que arma md_post (LQ_MAXSTEP) */
/* onda de Crocomire (HDMA $A4:943C: el BG2VOFS de cada línea desde crocomire_bg2_scroll_hdma_data): los valores de
   la VSRAM del plano B de sus pasos; [15]: el de arriba (md_vint_top), [14]: 1 si hay onda */
#define lq_wv_next ((uint16 *)((uint8 *)cinematic_text_tilemap + 3456))
#define lq_wv ((uint16 *)((uint8 *)cinematic_text_tilemap + 3488))
static uint8 lq_wn;                               /* pasos de la onda en lq_wv_next (md_post les suma scroll_by) */
static uint8 lq_on;                             /* este cuadro lleva líquido (lq_steps listos) */
static uint8 lq_haze;                           /* lq_on es la neblina del Bomb Torizo */
__asm__(
  "    .text\n"
  "    .global md_hint_lq\n"
  "md_hint_lq:\n"
  "    cmpi.b #0xE0,0xC00008\n"           /* aún en el vblank: nada */
  "    bcc.s 9f\n"
  "    cmpi.b #30,0xC00008\n"             /* antes de la línea 30 (la primera es la 31): la que salta al volver a */
  "    bcs.s 9f\n"                         /* activarlas si el vblank se pasó al HUD; nada (docs/60) */
  "    movem.l %d0/%a0-%a1,-(%sp)\n"
  "    movea.l lq_hp,%a0\n"
  "    cmpi.b #40,0xC00008\n"             /* arriba (la primera del cuadro, línea 31): desde el primer paso */
  "    bcc.s 3f\n"
  "    lea cinematic_text_tilemap+3968,%a0\n"   /* (lq_steps) */
  "3:  move.l (%a0)+,%a1\n"
  "    move.w (%a0)+,%d0\n"
  "    cmpa.w #0,%a1\n"
  "    beq.s 1f\n"
  "    move.l (%a0),0xC00004\n"
  "    subq.w #1,%d0\n"
  "2:  move.w (%a1)+,0xC00000\n"
  "    dbra %d0,2b\n"
  "    bra.s 5f\n"
  "1:  cmpi.w #0xFFFF,%d0\n"              /* paso final (sin colores, n 0xFFFF): se queda en él */
  "    bne.s 5f\n"
  "    move.w 4(%a0),0xC00004\n"
  "    bra.s 4f\n"
  "5:  addq.l #4,%a0\n"
  "    move.w (%a0)+,0xC00004\n"
  "    move.l %a0,lq_hp\n"
  "4:  movem.l (%sp)+,%d0/%a0-%a1\n"
  "9:  rte\n");
void md_hint_lq(void);
void zebes_obj_colors_add(int16 ar, int16 ag, int16 ab, uint16 *out);
/* tablas por canal (5 bits -> el canal de la Genesis, como tint_tables): cada color, tres lecturas */
#define lq_tab ((uint8 *)cinematic_text_tilemap + 3648)       /* 2 x 96: el cuerpo y la superficie */
void zebes_obj_colors_tab(const uint8 *t, uint16 *out);
static void lq_tables(int16 ar, int16 ag, int16 ab, uint8 *t) {
  for (int16 v = 0; v < 32; v++) {
    int16 r = v + ar, g = v + ag, b = v + ab;
    r = r < 0 ? 0 : r > 31 ? 31 : r; g = g < 0 ? 0 : g > 31 ? 31 : g; b = b < 0 ? 0 : b > 31 ? 31 : b;
    t[v] = (uint8)kCram555[r]; t[32 + v] = (uint8)(kCram555[g << 5] >> 4); t[64 + v] = (uint8)(kCram555[b << 10] >> 8);
  }
}
static void lq_palette(const uint8 *t, uint16 *out) {   /* las 64: con el color sumado (tabla t) */
  const uint8 *pbb = (const uint8 *)palette_buffer;
  for (uint16 i = 0; i < 48; i++) {
    uint16 o = cram_off[i];
    if (o == 0xFFFE) { out[i] = cram[i]; continue; }
    uint16 c = o == 0xFFFF ? 0 : *(const uint16 *)(pbb + o);
    out[i] = t[c & 31] | (uint16)t[32 + ((c >> 5) & 31)] << 4 | (uint16)t[64 + ((c >> 10) & 31)] << 8;
  }
  out[48] = cram[48];
  zebes_obj_colors_tab(t, out + 49);
}
/* el HDMA de Crocomire derritiéndose (Crocomire_950F $A4:950F: pre-instrucción $A4:943C, tabla indirecta
   {0xFF -> 7E:CAF0, 0xE1 -> 7E:CBEE}: una palabra por línea desde la 0) está activo */
uint8 md_croco_wave(void) {
  if (room_ptr != 0xA98D) return 0;
  for (uint16 i = 0; i < 6; i++)
    if (hdma_object_channels_bitmask[i] && hdma_object_pre_instructions[i] == 0x943C) return 1;
  return 0;
}
static uint8 lq_pal_dirty = 1;                  /* la paleta cambió (md_post) o se pisó el lugar de las tablas */
static uint8 lq_lo = 255, lq_hi;                 /* índices de palette_buffer que cambiaron (pocos) desde la última vez
                                                   (lq_lo > lq_hi: ninguno) */
static int8 lq_last[3] = { 127, 0, 0 };
static void lq_prepare(void) {
  lq_on = 0;
#ifdef MD_NO_LQ
  return;
#endif
  uint8 water = fx_type == 6 || fx_type == 0x26;  /* (0x26: el agua de la estatua de Tourian, el mismo tilemap) */
  uint8 rain = fx_type == 0xA;                  /* lluvia: su bruma (línea 6 del BG3, como la lava) en toda la imagen */
  /* neblina del Bomb Torizo (SpawnBombTorizoHaze: el COLDATA crece hacia abajo desde la línea MD_TORIZO_HAZE_L1 del
     SNES, mkmd.py): nada arriba y dos franjas con su color medio (la primera en lq_cols, la segunda en lq_col) */
  uint8 haze = !water && !rain && fx_type != 2 && fx_type != 4 && (uint8)fx_layer_blending_config_c == 0x2C;
  if (!md_zebes || cine_stage || md_bright != 15 || (fx_type != 2 && fx_type != 4 && !water && !rain && !haze)) return;
  if (haze) {
    { extern uint8 nt_msgbox_active; if (nt_msgbox_active) { lq_pal_dirty = 1; return; } }   /* (su RAM es de la caja) */
    if (lq_pal_dirty || lq_last[0] != 127 - 1) {
      lq_pal_dirty = 0; lq_last[0] = 127 - 1;   /* (marca: las tablas son las de la neblina) */
      lq_tables(MD_TORIZO_HAZE1_R, MD_TORIZO_HAZE1_G, MD_TORIZO_HAZE1_B, lq_tab + 96);
      lq_tables(MD_TORIZO_HAZE2_R, MD_TORIZO_HAZE2_G, MD_TORIZO_HAZE2_B, lq_tab);
      lq_palette(lq_tab, lq_col);
      lq_palette(lq_tab + 96, lq_cols);
    } else if (lq_lo <= lq_hi) {
      const uint8 *pbb = (const uint8 *)palette_buffer;
      const uint8 *t = lq_tab, *u = lq_tab + 96;
      for (uint16 i = lq_lo; i <= (uint16)lq_hi; i++)
        for (uint8 sl = cram_rev[i]; sl != 0xFF; sl = cram_next[sl]) {
          if (sl >= 48) continue;
          uint16 c = *(const uint16 *)(pbb + cram_off[sl]);
          lq_col[sl] = t[c & 31] | (uint16)t[32 + ((c >> 5) & 31)] << 4 | (uint16)t[64 + ((c >> 10) & 31)] << 8;
          lq_cols[sl] = u[c & 31] | (uint16)u[32 + ((c >> 5) & 31)] << 4 | (uint16)u[64 + ((c >> 10) & 31)] << 8;
        }
      if (lq_hi >= 128) {
        zebes_obj_colors_tab(t, lq_col + 49);
        zebes_obj_colors_tab(u, lq_cols + 49);
      }
    }
    lq_lo = 255; lq_hi = 0;
    static const LqStep kEndH = { 0, 0xFFFF, 0, 0x8A1F };
    /* líneas: 31 (los colores de los objetos), 63, y cada franja una línea antes de la suya en la Genesis (la imagen
       va 2 líneas más abajo que en el SNES; los colores tardan unas líneas en escribirse) */
    static const int16 kHzLn[4] = { 31, 63, MD_TORIZO_HAZE_L1 + 2 - 1, MD_TORIZO_HAZE_L2 + 2 - 1 };
    LqStep st[4] = { { zobj_col, 15, 0xC0620000u, 0 }, { 0, 0, 0, 0 }, { lq_cols, 64, 0xC0000000u, 0 },
                     { lq_col, 64, 0xC0000000u, 0 } };
    for (uint16 k = 0; k < 4; k++) {
      st[k].reg = k + 2 < 4 ? (uint16)(0x8A00 | (uint16)(kHzLn[k + 2] - kHzLn[k + 1] - 1)) : 0x8A1F;
      lq_next[k] = st[k];
    }
    lq_next[4] = kEndH;
    lq_wn = 0;
    lq_wv_next[14] = 0;
    lq_on = 1; lq_haze = 1;
    return;
  }
  lq_haze = 0;
  if (lq_last[0] == 127 - 1) lq_pal_dirty = 1;
  uint16 ly = rain ? layer1_y_pos : water ? fx_y_pos : lava_acid_y_pos;   /* superficie: el agua usa fx_y_pos */
  if (ly & 0x8000) return;
  int16 S = (int16)(ly - layer1_y_pos) - 2;     /* línea de la Genesis (medido contra el SNES: la interrupción y la escritura de colores llegan unas líneas tarde) */
  if (S >= 224) return;
  if (S < 66) S = 32;                           /* (con una interrupción menos: desde la primera línea del juego; 64-65: el intervalo daría 0 o menos) */
  int16 ar, ag, ab;
  if (water) {                                  /* agua: 74 % el color 26, 26 % el 25 (su tilemap del FX) */
    const uint16 *pb = palette_buffer;
    ar = (int16)((24 * (pb[26] & 31) + 8 * (pb[25] & 31)) >> 5);
    ag = (int16)((24 * ((pb[26] >> 5) & 31) + 8 * ((pb[25] >> 5) & 31)) >> 5);
    ab = (int16)((24 * ((pb[26] >> 10) & 31) + 8 * ((pb[25] >> 10) & 31)) >> 5);
    uint8 bl = (uint8)fx_layer_blending_config_c;   /* LayerBlendFunc_14/16: se resta (Maridia); 18: se suma */
    if (bl == 0x14 || bl == 0x16) { ar = -ar; ag = -ag; ab = -ab; }
  } else {
    const uint16 *pb = palette_buffer + (fx_type == 4 ? 0 : 24);   /* lava y lluvia: línea 6; ácido: 0 */
    /* pesos sobre 32: 92 % el color 3, 5 % el 2, 3 % el 1 */
    ar = (int16)((29 * (pb[3] & 31) + 2 * (pb[2] & 31) + (pb[1] & 31)) >> 5);
    ag = (int16)((29 * ((pb[3] >> 5) & 31) + 2 * ((pb[2] >> 5) & 31) + ((pb[1] >> 5) & 31)) >> 5);
    ab = (int16)((29 * ((pb[3] >> 10) & 31) + 2 * ((pb[2] >> 10) & 31) + ((pb[1] >> 10) & 31)) >> 5);
  }
  if (tint_on) { ar += tint_r; ag += tint_g; ab += tint_b; }
  { extern uint8 nt_msgbox_active; if (nt_msgbox_active) lq_pal_dirty = 1; }   /* (la caja de mensaje usa la misma RAM) */
  uint8 again = lq_pal_dirty || ar != lq_last[0] || ag != lq_last[1] || ab != lq_last[2];
  if (again) {                                  /* solo si cambió la paleta o el color del líquido */
    lq_pal_dirty = 0; lq_last[0] = ar; lq_last[1] = ag; lq_last[2] = ab;
    lq_tables(ar, ag, ab, lq_tab);
    lq_tables(ar + (ar >> 1), ag + (ag >> 1), ab + (ab >> 1), lq_tab + 96);   /* superficie: ~1,5 veces (SNES) */
    lq_palette(lq_tab, lq_col);
    lq_palette(lq_tab + 96, lq_cols);
  } else if (lq_lo <= lq_hi) {                  /* solo los colores que salen de los índices que cambiaron */
    const uint8 *pbb = (const uint8 *)palette_buffer;
    const uint8 *t = lq_tab, *u = lq_tab + 96;
    for (uint16 i = lq_lo; i <= (uint16)lq_hi; i++)
      for (uint8 sl = cram_rev[i]; sl != 0xFF; sl = cram_next[sl]) {
        if (sl >= 48) continue;
        uint16 c = *(const uint16 *)(pbb + cram_off[sl]);
        lq_col[sl] = t[c & 31] | (uint16)t[32 + ((c >> 5) & 31)] << 4 | (uint16)t[64 + ((c >> 10) & 31)] << 8;
        lq_cols[sl] = u[c & 31] | (uint16)u[32 + ((c >> 5) & 31)] << 4 | (uint16)u[64 + ((c >> 10) & 31)] << 8;
      }
    if (lq_hi >= 128) {                         /* (índices de los sprites: los colores de los objetos) */
      zebes_obj_colors_tab(t, lq_col + 49);
      zebes_obj_colors_tab(u, lq_cols + 49);
    }
  }
  lq_lo = 255; lq_hi = 0;
  /* sin apagar la interrupción: después del último paso salta cada 32 líneas sin hacer nada (paso final, n 0xFFFF)
     y en el cuadro siguiente vuelve a la línea 31, que empieza otra vez (md_hint_lq); así un cuadro atrasado en que
     el vblank no la reprograma (VDP ocupada) también lleva el líquido */
  static const LqStep kEnd = { 0, 0xFFFF, 0, 0x8A1F };
  /* los pasos con su línea: 31 (la primera), 63 (fija: el intervalo de la 31 lo da el vblank), la superficie (S - 1)
     y el cuerpo del líquido (+ LQ_SURF); el registro de cada paso es el intervalo entre los dos siguientes */
  LqStep st[LQ_MAXSTEP];
  int16 ln[LQ_MAXSTEP];
  uint16 n = 0;
  if (S == 32) {                                /* línea 31: todo de una vez (sin franja) */
    st[n] = (LqStep){ lq_col, 64, 0xC0000000u, 0 }; ln[n++] = 31;
    st[n] = (LqStep){ 0, 0, 0, 0 }; ln[n++] = 63;
  } else {
    /* (escribir colores durante la imagen frena al 68000: lo justo. La línea 31 solo los objetos: las líneas 0-2 las
       repone md_vint_top; la franja, solo las líneas 0-2) */
    st[n] = (LqStep){ zobj_col, 15, 0xC0620000u, 0 }; ln[n++] = 31;
    st[n] = (LqStep){ 0, 0, 0, 0 }; ln[n++] = 63;
    st[n] = (LqStep){ lq_cols, 48, 0xC0000000u, 0 }; ln[n++] = S - 1;
    st[n] = (LqStep){ lq_col, 64, 0xC0000000u, 0 }; ln[n++] = S - 1 + LQ_SURF;
  }
  lq_wn = 0;
  lq_wv_next[14] = 0;
  { /* onda de Crocomire: las líneas en que cambia el BG2VOFS de la tabla (línea del SNES + 2 en la Genesis), las que
       quepan (repartidas); cada una escribe la VSRAM del plano B (md_post suma scroll_by) */
    extern uint8 md_croco_wave(void);
    if (md_croco_wave()) {
      const uint16 *t = crocomire_bg2_scroll_hdma_data;
      uint16 room = LQ_MAXSTEP - 1 - n;           /* (uno para el final) */
      uint8 ch[112]; uint16 nc = 0;
      for (uint16 i = 31; i < 222 && nc < sizeof ch; i++) if (t[i] != t[i - 1]) ch[nc++] = (uint8)i;
      lq_wv_next[14] = 1;
      for (uint16 j = 0; j < room && j < nc; j++) {
        uint16 k = nc <= room ? j : (uint16)((2 * j + 1) * nc / (2 * room));
        int16 L = ch[k] + 2;
        /* lugar en la lista (ordenada por línea), con aire: 4 líneas después de un paso de colores, 2 de los demás */
        uint16 p = n;
        while (p > 0 && ln[p - 1] >= L) p--;
        if (p > 0) { int16 g = ln[p - 1] + (st[p - 1].n > 1 ? 4 : 2); if (L < g) L = g; }
        if (p < n && ln[p] < L + (int16)2) continue;   /* (no cabe antes del siguiente: se salta) */
        if (L > 222) continue;
        for (uint16 q = n; q > p; q--) { st[q] = st[q - 1]; ln[q] = ln[q - 1]; }
        lq_wv_next[lq_wn] = (uint16)(t[ch[k]] - t[0]);
        st[p] = (LqStep){ &lq_wv[lq_wn], 1, 0x40020010u, 0 }; ln[p] = L;
        lq_wn++;
        n++;
      }
    }
  }
  for (uint16 k = 0; k < n; k++) {
    st[k].reg = k + 2 < n ? (uint16)(0x8A00 | (uint16)(ln[k + 2] - ln[k + 1] - 1)) : 0x8A1F;
    lq_next[k] = st[k];
  }
  lq_next[n] = kEnd;
  /* (la paleta normal la repone md_vint_top, líneas 0-2; y el primer paso, línea 31) */
  lq_on = 1;
}

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
  "    cmpi.b #30,0xC00008\n"             /* antes de la línea 30 (la primera es la 31): la que salta al volver a */
  "    bcs.s 9f\n"                         /* activarlas si el vblank se pasó al HUD; nada (docs/60) */
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
      { void zebes_boss_tiles_again(void); zebes_boss_tiles_again(); }   /* (o del cuerpo del jefe, docs/59) */
      if (md_zebes) zebes_frz_reset();            /* y los de los congelados (filas del Window) */
      lq_pal_dirty = 1;                           /* (la caja pisó las paletas del líquido o de la neblina) */
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
#ifdef MD_HUDLOG
volatile uint8 md_hud_vl, md_hud_tl, md_hud_pub, md_hud_el;   /* (medición) línea de entrada al vblank, de la reposición; md_pub */
#endif
static void md_vint_top(void) {
  if (zobj_hint == 2) mb_hp = mb_steps;
  if (zobj_hint && !md_vdp_busy) {             /* Zebes: aunque el cuadro se atrase, el HUD con sus colores arriba */
    if (zobj_hint == 3) {                       /* líquido: los pasos desde el principio y arriba la paleta sin él */
      lq_hp = lq_steps;
      VDP_CTRL32 = VDP_CMD_CRAM_W(0);
      for (uint16 i = 0; i < 48; i++) VDP_DATA = cram[i];
      if (lq_wv[14]) { VDP_CTRL32 = 0x40020010u; VDP_DATA = lq_wv[15]; }   /* onda: arriba, el scroll sin ella */
    }
    VDP_CTRL32 = VDP_CMD_CRAM_W(0x62);
    for (uint16 i = 0; i < 15; i++) VDP_DATA = cram[49 + i];
    vdp_reg(0x0A, 31);   /* cambio de colores en la línea 32 (la primera del juego): no en el HUD */
    vdp_reg(0x00, 0x14);
#ifdef MD_HUDLOG
    md_hud_tl = VDP_VCOUNT >> 8;
#endif
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
     cuadro que se ve dos veces salía la segunda vez sin el degradado (docs/54) */
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
extern volatile uint16 md_snd_frames;          /* md_snd.c: el XGM2 está cargado (el Z80 lee la ROM) */
void md_snd_protect(uint8 on);
#ifdef MD_M7LOG
volatile uint16 md_m7_start;                   /* (medición) cómo empezó el último vblank: tarde << 8 | línea */
#endif
/* el trabajo del vblank, con la página del nivel puesta y, con el XGM2 cargado, el Z80 avisado de que no lea la ROM
   (lee la música y las muestras por el bus del 68000) mientras duran las DMA (md_snd_protect, docs/55). Devuelve lo
   que vb_exit repone. */
static uint16 vb_enter(uint8 snd) {
  uint8 z80 = 0;
  if (snd) {
    md_snd_protect(1);
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
  if (sv & 0x8000) md_snd_protect(0);
}
static void md_vblank_lag(void);
static int16 vb_lines_left(void);
/* líneas que necesita el vblank de un cuadro (lo que tardaron los últimos, con margen): un cuadro que llega con el
   vblank empezado se toma solo si quedan tantas; si no, en el vblank siguiente (docs/54). Si el vblank se pasara a
   la imagen, sus DMA irían a la velocidad de la imagen y las interrupciones horizontales empezarían tarde. */
static int16 md_vb_need = 20;
static uint8 md_vb_room;                       /* este vblank cargó una sala (con la pantalla apagada: no cuenta) */
static void md_vblank_run(uint16 late) {
  md_vb_tick = vtimer;
  int16 l0 = vb_lines_left();
#ifdef MD_M7LOG
  md_m7_start = late << 8 | VDP_VCOUNT >> 8;
#endif
  uint16 sv = vb_enter(md_snd_frames != 0);
  md_vb_room = 0;
  md_vblank(late);
#ifdef MD_HUDLOG
  md_hud_el = VDP_VCOUNT >> 8;
#endif
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
#ifdef MD_HUDLOG
  md_hud_vl = VDP_VCOUNT >> 8; md_hud_pub = md_pub; md_hud_tl = 0xFF;
#endif
  if (!md_pub) md_vblank_lag();
  md_vint_top();
  if (md_pub) md_vblank_run(0);
}

#ifndef LATE_MAX
#define LATE_MAX 0xEA
#endif
/* (fuera de las interrupciones) el cuadro está listo: lo toma el próximo vblank. Si el vblank ya empezó sin cuadro
   (el anterior tardó) y todavía queda lo que necesita (md_vb_need), se escribe ya */
static int16 ga_job_lines;      /* huida: líneas de vblank que pide su cambio en este cuadro (ga_post) */
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
      vb_lines_left() >= md_vb_need + ga_job_lines)
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

uint16 md_missing_ring[8][4];                   /* pruebas: las últimas claves que faltaron (distintas; 8: la RAM es justa) */
uint8 md_missing_rpos;
static void missing(uint16 kind, uint16 a, uint16 b, uint16 c) {
  if (!md_missing) { md_missing_key[0] = kind; md_missing_key[1] = a; md_missing_key[2] = b; md_missing_key[3] = c; }
  md_missing++;
  for (uint8 i = 0; i < 8; i++)
    if (md_missing_ring[i][0] == kind && md_missing_ring[i][1] == a && md_missing_ring[i][2] == b && md_missing_ring[i][3] == c) return;
  uint16 *r = md_missing_ring[md_missing_rpos++ & 7];
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

uint16 md_ovl[MD_OVL_BYTES / 2];                  /* (md.h: la intro y Zebes, docs/60) */
NT_GAMEHI uint16 md_boss_zone[MD_BOSS_BYTES / 2];   /* (md.h: la sala del jefe, docs/60) */
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

/* destello de los jefes (docs/59): el SNES los dibuja con la paleta 0 en los cuadros alternos de flash_timer
   ($A0:9929); aquí se dibujan con su paleta y ese cuadro los colores de los objetos van a blanco (boss_flash_post) */
static uint8 boss_flash_req, boss_flash_was;
static uint8 room_is_boss(void) {
  switch (room_ptr) {
  case 0x9DC7: case 0xA59F: case 0xA98D: case 0xB283: case 0xB32E: case 0xCD13: case 0xD95E: case 0xDA60: case 0xDD58:
    return 1;
  }
  return 0;
}
uint8 zebes_boss_line(void);
static void boss_flash_post(void) {              /* (después de zebes_obj_colors) */
  if (boss_flash_req) {
    static const uint8 kSlots[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 13, 14 };   /* colores 1-8, 14, 15 (9-13: el rayo) */
    for (uint16 i = 0; i < 10; i++) zobj_col[kSlots[i]] = 0x0EEE;
    if (zebes_boss_line())
      for (uint16 c = 17; c < 32; c++) if (cram[c] != 0x0EEE) { cram[c] = 0x0EEE; cram_dirty = 1; }
    boss_flash_was = 1;
  } else if (boss_flash_was) {                   /* la línea 1 vuelve a sus colores */
    boss_flash_was = 0;
    nt_palette_changed = 1; nt_palette_lo = 0; nt_palette_hi = 255;
  }
  boss_flash_req = 0;
}

void DrawEnemySpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) {  // 0x818AB8
  (void)bank;
  if (md_zebes && !pal && room_is_boss()) {
    uint16 base = gEnemyData(cur_enemy_index)->palette_index;
    if (base && zebes_obj_draw(sm, base, tiles, (int16)x, (int16)y)) { boss_flash_req = 1; return; }
  }
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
  /* congelado (WriteEnemyOams: paleta 0x0C00) sin lugar en la VRAM (md_zebes.c, docs/58): con su paleta */
  if (md_zebes && pal == 0x0C00 &&
      zebes_obj_draw(sm, gEnemyData(cur_enemy_index)->palette_index, tiles, (int16)x, (int16)y)) return;
  if (!drawkey(1, sm, pal, tiles, (int16)x, (int16)y)) missing(1, sm, pal, tiles);
}

/* WriteEnemyOams ($A0:944A) con un spritemap extendido: si la sala lo tiene preparado entero (Ridley: sus piezas
   ya juntadas en sprites grandes), se dibuja de una vez; si no, el motor dibuja cada spritemap */
uint8 DrawEnemyExtSpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) {
  (void)bank;
  if (md_zebes) return 0;                       /* (las claves de tipo 7 son las de Ceres; en Zebes, pieza por pieza) */
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
static const uint16 kNoRecs[1] = { 0 };          /* (lista vacía: md_putrecs no dibuja nada) */
uint32 ep_lookup(uint16 v2, uint16 sm, uint16 g, int16 x, int16 y) {
  if (sm == 0x8000) return 0;                   /* $8D:8000: spritemap vacío (el proyectil aún sin instrucción) */
  struct EpCache *c = &ep_cache[v2 >> 1];
  /* clave por cuadro (Kraid: las rocas): ep_cache la recuerda con el bit 31 (número de la entrada); se dibuja aquí,
     desde la página del depósito, con el lugar sumado (md_zebes.c, zebes_ep_draw), sin copiar sus registros */
  extern void zebes_ep_draw(uint16 n, int16 x, int16 y);
  if (ep_zebes && ((uint32)c->rec & 0x80000000u) && c->sm == sm && c->g == g) {
    zebes_ep_draw((uint16)(uint32)c->rec, x, y);
    return (uint32)kNoRecs;
  }
  const uint16 *r = ep_zebes ? zebes_obj_find_cached(sm, g, 0x8000) : find_key_q(2, sm, g, 0);
  if (!r) { missing(2, sm, g, 0); return 0; }
  if (ep_zebes) {
    extern int16 zebes_ep_frozen(const uint16 *r);
    int16 n = zebes_ep_frozen(r);
    if (n == -2) return 0;
    if (n >= 0) {
      c->sm = sm; c->g = g; c->rec = (const uint16 *)(0x80000000u | (uint16)n);
      zebes_ep_draw((uint16)n, x, y);
      return (uint32)kNoRecs;
    }
  }
  c->sm = sm; c->g = g; c->rec = r;
  return (uint32)r;
}
/* md_putrecs: los registros de sprites de a0 en x = d0, y = d1 (en pantalla), como el final de md_drawkey.
   Conserva todo menos d0, d1, a0, a1. */
__asm__(
  "    .text\n"
  "    .global md_putrecs\n"
  "md_putrecs:\n"
  "    movem.l %d2/%d4-%d7/%a2,-(%sp)\n"
  "    bsr md_putrecs_ns\n"
  "    movem.l (%sp)+,%d2/%d4-%d7/%a2\n"
  "    rts\n"
  /* md_putrecs_ns: lo mismo sin guardar d2/d4-d7/a2 (los guarda quien llama: zfrz_put, md_zebes.c) */
  "    .global md_putrecs_ns\n"
  "md_putrecs_ns:\n"
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
  "8:  rts\n"
  "20: addq.l #2,%a0\n"
  "21: addq.l #2,%a0\n"
  "22: addq.l #2,%a0\n"
  "23: movem.l %d3/%a3,-(%sp)\n"
  "    move.l spr_end,%a3\n"
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
  "    movem.l (%sp)+,%d3/%a3\n"
  "    rts\n");

/* md_putrecs_t: lo mismo, con d4 sumado al atributo de cada sprite (el tile de un lugar del depósito por cuadro,
   md_zebes.c: sin una segunda pasada por los sprites ya escritos) */
__asm__(
  "    .text\n"
  "    .global md_putrecs_t\n"
  "md_putrecs_t:\n"
  "    movem.l %d2/%d5-%d7/%a2,-(%sp)\n"
  "    move.w %d0,%d6\n"
  "    move.w %d1,%d7\n"
  "    move.w (%a0)+,%d5\n"                   /* n */
  "    beq 8f\n"
  "    movea.l sp_ptr,%a1\n"
    "    move.w (%a0)+,%d0\n"
  "    add.w %d7,%d0\n"
  "    cmp.w #-31,%d0\n"
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
  "    move.w (%a0)+,%d0\n"
  "    add.w %d4,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w (%a0)+,%d0\n"
  "    add.w %d6,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    dbra %d5,6b\n"
  "    move.l %a1,sp_ptr\n"
  "8:  movem.l (%sp)+,%d2/%d5-%d7/%a2\n"
  "    rts\n"
  "20: addq.l #2,%a0\n"
  "21: addq.l #2,%a0\n"
  "22: addq.l #2,%a0\n"
  "23: movem.l %d3/%a3,-(%sp)\n"
  "    move.l spr_end,%a3\n"
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
  "    move.w 2(%a0),%d0\n"
  "    add.w %d4,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "    move.w %a2,%d0\n"
  "    add.w #128,%d0\n"
  "    move.w %d0,(%a1)+\n"
  "11: addq.l #6,%a0\n"
  "    dbra %d5,10b\n"
  "12: move.l %a1,sp_ptr\n"
  "    movem.l (%sp)+,%d3/%a3\n"
  "    movem.l (%sp)+,%d2/%d5-%d7/%a2\n"
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
  "    lea eproj_id,%a2\n"                    /* (bases: un solo índice, d5) */
  "    lea eproj_properties,%a3\n"
  "    lea eproj_x_pos,%a4\n"
  "    lea eproj_y_pos,%a5\n"
  "    lea ep_cache,%a6\n"
  "    moveq #34,%d5\n"
  "1:  tst.w (%a2,%d5.w)\n"
  "    beq 9f\n"
  "    move.w (%a3,%d5.w),%d0\n"
  "    and.w #0x1000,%d0\n"
  "    cmp.w %d2,%d0\n"
  "    bne 9f\n"
  "    move.w (%a4,%d5.w),%d6\n"
  "    add.w %d3,%d6\n"                     /* x */
  "    move.w %d6,%d0\n"
  "    add.w #128,%d0\n"
  "    and.w #0xFE00,%d0\n"
  "    bne 9f\n"
  "    move.w (%a5,%d5.w),%d1\n"
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
  "    move.w %d5,%d1\n"
  "    add.w %d1,%d1\n"
  "    add.w %d1,%d1\n"
  "    lea (%a6,%d1.w),%a0\n"              /* su entrada de ep_cache (8 bytes) */
  "    move.l 4(%a0),%d1\n"
  "    beq 5f\n"
  "    cmp.w (%a0),%d0\n"
  "    bne 5f\n"
  "    cmp.w 2(%a0),%d7\n"
  "    bne 5f\n"
  "    tst.l %d1\n"
  "    bmi 6f\n"                            /* (por cuadro: zebes_ep_draw, sin ep_lookup) */
  "    movea.l %d1,%a0\n"
  "4:  move.w (%a5,%d5.w),%d1\n"
  "    add.w %d4,%d1\n"
  "    move.w %d6,%d0\n"
  "    jsr md_putrecs\n"
  "9:  subq.w #2,%d5\n"
  "    bpl 1b\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
  "    rts\n"
  "6:  move.w %d1,%d7\n"                   /* n (la entrada de la caché) */
  "    move.w %d1,%d0\n"
  "    move.w (%a5,%d5.w),%d1\n"
  "    add.w %d4,%d1\n"
  "    movea.w %d1,%a1\n"                    /* y */
  "    move.w %d6,%d1\n"                     /* x */
  "    jsr zfrz_put\n"                       /* (md_zebes.c: con el lugar ya puesto) */
  "    tst.w %d0\n"
  "    bne 9b\n"
  "    move.w (%a5,%d5.w),%d0\n"                   /* si no: zebes_ep_draw(n, x, y) */
  "    add.w %d4,%d0\n"
  "    ext.l %d0\n"
  "    move.l %d0,-(%sp)\n"
  "    move.w %d6,%d0\n"
  "    ext.l %d0\n"
  "    move.l %d0,-(%sp)\n"
  "    moveq #0,%d1\n"
  "    move.w %d7,%d1\n"
  "    move.l %d1,-(%sp)\n"
  "    jsr zebes_ep_draw\n"
  "    lea 12(%sp),%sp\n"
  "    bra 9b\n"
  "5:  move.w (%a5,%d5.w),%d1\n"                 /* ep_lookup(v2, sm, g, x, y) */
  "    add.w %d4,%d1\n"
  "    ext.l %d1\n"
  "    move.l %d1,-(%sp)\n"
  "    move.w %d6,%d1\n"
  "    ext.l %d1\n"
  "    move.l %d1,-(%sp)\n"
  "    move.l %d7,-(%sp)\n"
  "    move.l %d0,-(%sp)\n"
  "    move.l %d5,-(%sp)\n"
  "    jsr ep_lookup\n"
  "    lea 20(%sp),%sp\n"
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
  /* Zebes (Ridley de Norfair): las claves de los objetos de la sala (mkzkeys.py: enemy A6 sm paleta 0000) */
  if (md_zebes && zebes_obj_draw(sm, a, 0, (int16)(x - layer1_x_pos), (int16)(y - layer1_y_pos))) return;
  if (md_zebes && !a && room_is_boss() &&                       /* destello: con su paleta (la 7: 0x0E00) y en blanco */
      zebes_obj_draw(sm, 0x0E00, 0, (int16)(x - layer1_x_pos), (int16)(y - layer1_y_pos))) { boss_flash_req = 1; return; }
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

/* rayos (docs/57): un juego de claves común a todas las salas de Zebes (mkmd.beam_common, en la parte fija) para
   los spritemaps hechos solo con los 8 tiles del rayo (todas las combinaciones, cargados o no, SBA, ecos). Esos tiles
   van en BEAM_TILE (VRAM 0xEA00) y son los de la combinación equipada: el juego los cambia en
   UpdateBeamTilesAndPalette ($90:AC8D: kBeamTilePtrs -> VRAM $6300), aquí ya convertidos (kBeamTiles); sus colores
   son los 9-13 de la línea 3 (palette_buffer[225-229]). */
static const uint8 *beam_dma, *beam_dmal;
void BeamTiles(uint16 combo) {
  if (combo < BEAM_COMBOS) beam_dmal = kBeamTiles + combo * 256;
}
static const uint16 *beam_find(uint16 sm) {
  int lo = 0, hi = BEAM_KEYS - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    if (kBeamKeySm[m] == sm) return kBeamRecs + kBeamKeyRec[m];
    if (kBeamKeySm[m] < sm) lo = m + 1; else hi = m - 1;
  }
  return 0;
}

/* un spritemap de $93 (proyectiles de Samus y el destello de la carga, DrawBeamGrappleSpritemap $81:8A37) */
void DrawGrappleOrProjectileSpritemap(uint16 sm, uint16 x, uint16 y) {  // 0x818A5F
  if (intro_on) { it_draw(INTRO_KIND_PROJ, sm, 0, (int16)x, (int16)y); return; }
  if (md_zebes) {
    const uint16 *r = beam_find(sm);
    if (r) { md_put_recs(r, (int16)x, (int16)y); return; }
    /* misiles, bombas y explosiones con los colores de la sala (mkzobj.py: clave sm, 0, 0xC000) */
    if (zebes_obj_draw(sm, 0, 0xC000, (int16)x, (int16)y)) return;
  }
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

/* rayo gancho (grapple.c, docs/56): los segmentos y la punta son sprites de 8x8 con tiles que el SNES copia de $9A
   cada cuadro (UpdateGrappleBeamTiles $9B:BFBD: el punto, 4 cuadros, y 4 tiles de segmento según el ángulo) a los
   tiles 0x20-0x24 de los sprites. Esos tiles solo usan el color 15 de la paleta 5 (palette_buffer[223], que el gancho
   pone en 0x7F91): aquí son una máscara (kGrappleSnesTiles, mkdata) que se pinta con el lugar de la CRAM que sale de
   ese color (o, si la sala no lo usa, el color de la CRAM más parecido) y va a GRAPPLE_TILE (5 tiles al final de 0xF000; son del haz
   del ojo: en esas salas no se dibuja). */
#define GRAPPLE_TILE 2043
uint8 zebes_eye_room(void);
static uint16 gr_point = 0xFFFF, gr_beam = 0xFFFF;
static uint8 gr_slot = 0xFF;
static uint8 gr_tiles[5][32] __attribute__((aligned(2)));
static volatile uint8 gr_dirty, gr_dirty_l;

static void gr_conv(uint8 *dst, const uint8 *b, uint8 nib) {   /* tile del SNES (4 bits por planos) -> Genesis */
  for (int y = 0; y < 8; y++) {
    uint8 m = b[2 * y] | b[2 * y + 1] | b[16 + 2 * y] | b[17 + 2 * y];   /* (todos los píxeles usados son el 15) */
    for (int x = 0; x < 8; x += 2) {
      uint8 v = 0;
      if (m & (0x80 >> x)) v |= nib << 4;
      if (m & (0x40 >> x)) v |= nib;
      dst[4 * y + (x >> 1)] = v;
    }
  }
}

static uint8 gr_nearest(uint16 snes) {               /* el lugar de la CRAM (no transparente) más parecido a ese color */
  uint16 want = kCram555[snes & 0x7FFF], best = 0xFFFF;
  uint8 slot = 0xFF;
  for (uint8 i = 0; i < 64; i++) {
    if (!(i & 15)) continue;
    uint16 c = cram[i], d = 0;
    for (uint8 k = 1; k < 12; k += 4) {
      int16 a = (c >> k) & 7, b = (want >> k) & 7;
      d += (a > b) ? a - b : b - a;
    }
    if (d < best) { best = d; slot = i; }
  }
  return slot;
}

void GrappleTiles(uint16 point_src, uint16 beam_src) {
  uint8 slot = cram_rev[223] != 0xFF ? cram_rev[223] : gr_nearest(palette_buffer[223]);
  if (point_src == gr_point && beam_src == gr_beam && slot == gr_slot)
    return;
  gr_point = point_src; gr_beam = beam_src; gr_slot = slot;
  uint8 nib = (slot == 0xFF) ? 15 : (slot & 15);
  gr_conv(gr_tiles[0], kGrappleSnesTiles + (uint16)(point_src - 0x8200), nib);
  for (int t = 0; t < 4; t++)
    gr_conv(gr_tiles[1 + t], kGrappleSnesTiles + (uint16)(beam_src - 0x8200) + 32 * t, nib);
  gr_dirty_l = 1;
}

void DrawGrappleOam(int16 x, int16 y, uint16 chr) {
  if (!md_zebes || zebes_eye_room() || x < -8 || x >= 256 || y < -8 || y >= 224 || sp_ptr == spr + MAX_SPR)
    return;
  uint16 t = (chr & 0x1FF) - 0x20;
  if (t > 4)
    return;
  uint8 line = (gr_slot == 0xFF) ? 2 : (gr_slot >> 4);
  GenSpr *s = sp_ptr++;
  s->y = y + 129;                                 /* (el SNES dibuja los sprites una línea más abajo: DrawArmCannon) */
  s->size_link = (uint16)(s - spr + 1);
  s->attr = (uint16)((((chr >> 12) & 3) == 3 ? 0x8000 : 0) | (line << 13) | (((chr >> 15) & 1) << 12) |
                     (((chr >> 14) & 1) << 11) | (GRAPPLE_TILE + t));
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
 * hasta GA_BUDGET tiles por vblank, con lo que sobra del vblank: no se ven) y después cambia las celdas y la paleta.
 * Cuándo se cambia y qué filas van en cada cuadro lo decide la lógica (ga_post), no lo que sobra del vblank: los
 * tiles de cada cuadro de la huida cuestan 16 palabras cada uno y cada cuadro del motor después del pedido cuenta
 * GA_STEP (los GA_BUDGET tiles que entraban en un vblank); sin tiles, se cambia en el mismo cuadro. Las filas de un
 * cambio grande van de a GA_FULL_VBLANK líneas por cuadro del motor. Así la imagen de cada cuadro del motor es
 * siempre la misma (como el modo 7 y la nave, docs/54; docs/55: con sonido el Z80 le roba ciclos al 68000 y la carga
 * terminaba un cuadro antes o después). */
#define GA_BUDGET 120
#define GA_WORDS_LINE 70                        /* DMA en el vblank (H32): 167 bytes por línea, con margen */
#define GA_ROW_COST 12                          /* lo que cuesta armar la DMA de cada fila, en palabras */
#define GA_FULL_VBLANK 26                       /* líneas para la huida en un vblank sin retraso */
#ifndef GA_STEP
#define GA_STEP 1920                            /* presupuesto de cada cuadro del motor, en palabras (GA_BUDGET tiles) */
#endif
static const uint16 ga_budget = GA_BUDGET;
static int16 ga_want = -1, ga_shown = -1, ga_loading = -1;
static uint8 ga_called;
static int16 ga_want_l;                          /* el cuadro que pidió la lógica (md_post lo pasa a ga_want) */
static uint16 ga_done;
static uint8 ga_reg, ga_shown_reg;
/* lo que decide la lógica (ga_post) para el vblank de este cuadro del motor */
static int16 ga_sched = -1;                      /* el cuadro que se prepara (sus tiles se cargan) */
static uint8 ga_sched_reg;                       /* en qué mitad de la zona de tiles */
static uint16 ga_left;                           /* presupuesto que le falta */
static int16 ga_sw = -1;                         /* el cuadro que se está cambiando (filas) */
static int16 ga_job_y0, ga_job_y1;               /* filas de este cuadro del motor ([y0, y1); y0 == y1: ninguna) */
static uint8 ga_job_last;                        /* con estas filas termina el cambio (paleta) */
#ifdef MD_GAWAIT
volatile uint16 md_ga_wait;                      /* (medición) cambios que tuvieron que terminar de cargar tiles */
#endif

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

/* rectángulo de celdas que cambia y cuántas filas entran en un cuadro del motor */
static uint16 ga_per_row(const GetawayFrame *F) { return ga_uw + GA_ROW_COST + (F->w ? 2 * GA_ROW_COST : 0); }
/* (lógica, md_post) qué hace el vblank de este cuadro con la huida */
static void ga_post(void) {
  ga_job_y0 = ga_job_y1 = 0;
  ga_job_last = 0;
  ga_job_lines = 0;
  uint8 fresh = 0;
  if (ga_sw < 0 && ga_sched < 0 && ga_want >= 0 && ga_want != ga_shown) {   /* el siguiente: el que pide el motor */
    ga_sched = ga_want;
    ga_sched_reg = ga_shown >= 0 ? ga_shown_reg ^ 1 : 0;
    ga_left = kGetaway[ga_sched].ntiles * 16;
    fresh = 1;
  }
  if (ga_sw < 0 && ga_sched >= 0) {               /* preparando: el presupuesto (desde el cuadro siguiente; sin tiles, ya) */
    if (!fresh) ga_left = ga_left > GA_STEP ? ga_left - GA_STEP : 0;
    if (!ga_left) {                               /* empieza el cambio de celdas (en el vblank de este cuadro) */
      const GetawayFrame *P = ga_shown >= 0 && kGetaway[ga_shown].w ? &kGetaway[ga_shown] : 0;
      ga_union(P, &kGetaway[ga_sched]);
      ga_sw = ga_sched;
      ga_sy = ga_uy0;
      ga_sy1 = ga_uy0 + ga_uh;
    }
  }
  if (ga_sw < 0) return;
  const GetawayFrame *F = &kGetaway[ga_sw];       /* filas de este cuadro */
  /* entero si cabe en un vblank (las filas más la paleta, como antes); si no, por partes de arriba abajo */
  uint16 cost = ga_uh * (ga_uw + GA_ROW_COST) + (F->w ? F->h * 2 * GA_ROW_COST : 0);
  uint16 k = cost <= GA_FULL_VBLANK * GA_WORDS_LINE ? ga_sy1 - ga_sy : (GA_FULL_VBLANK * GA_WORDS_LINE) / ga_per_row(F);
  if (!k) k = 1;
  if (k > (uint16)(ga_sy1 - ga_sy)) k = ga_sy1 - ga_sy;
  ga_job_y0 = ga_sy;
  ga_job_y1 = ga_sy + k;
  ga_job_lines = (int16)((k * ga_per_row(F)) / GA_WORDS_LINE) + 1;
  ga_sy += k;
  if (ga_sy >= ga_sy1) {                          /* termina en este cuadro: se ve el nuevo */
    ga_job_last = 1;
    ga_shown = ga_sw;
    ga_shown_reg = ga_sched_reg;
    ga_sw = ga_sched = -1;
    ga_sy = -1;
  }
}

/* (vblank) los tiles del cuadro que se prepara con lo que sobra; las filas que decidió ga_post */
static void ga_vblank(void) {
  int16 t = ga_job_last ? ga_shown : ga_sched;    /* (al terminar, ga_post ya lo pasó a ga_shown) */
  if (t >= 0 && t != ga_loading) {                /* otro cuadro: sus tiles desde el principio */
    ga_loading = t;
    ga_done = 0;
    ga_reg = ga_job_last ? ga_shown_reg : ga_sched_reg;
  }
  if (ga_loading < 0) return;
  const GetawayFrame *F = &kGetaway[ga_loading];
  uint8 sw = ga_job_y1 > ga_job_y0;
  uint16 n = F->ntiles - ga_done;
  if (n) {
    int16 b = ((vb_lines_left() - 12) * GA_WORDS_LINE) >> 4;   /* tiles de 16 palabras: lo que sobra del vblank */
    if (sw) {                                     /* hay que cambiar ya: todos (no debería pasar) */
      b = n;
#ifdef MD_GAWAIT
      md_ga_wait++;
#endif
    }
    if (b > 0) {
      if (n > (uint16)b) n = b;
      if (!sw && n > ga_budget) n = ga_budget;
      uint8 lp = md_cur_page;
      map_page_ext(CINE_PAGE_kGetawayTiles);
      vdp_dma_vram(kGetawayTiles + F->toff + ga_done * 32, (GETAWAY_TILE0 + ga_reg * GETAWAY_REGION + ga_done) * 32, n * 16);
      map_page_ext(lp);
      ga_done += n;
    }
  }
  if (!sw) return;
  ga_union_rows(F, ga_job_y0, ga_job_y1);         /* (ga_union: el rectángulo que dejó ga_post) */
  if (!ga_job_last) return;
  if (F->w) {
    VDP_CTRL32 = VDP_CMD_CRAM_W(34);
    for (uint16 c = 0; c < 15; c++) { cram[17 + c] = F->pal[c]; VDP_DATA = F->pal[c]; }
  }
  if (!F->w) flash_shown = 0xFF;                  /* sin Ridley: la línea 1 vuelve a sus colores (md_frame) */
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
  ga_sched = ga_sw = -1;
  ga_job_y0 = ga_job_y1 = 0;
  ga_job_last = 0;
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
   y casi siempre va adelante (docs/54). */
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
#include "md_pause.c"                         /* la pausa: mapa y equipo (docs/53) */
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
uint8 md_cone_pb_prio(void);
void md_logic(uint16_t joy) {
  sp_ptr = spr;
  md_spr_reserved = zebes_eye_room() || md_cone_pb_prio();   /* (la luz de la bomba de poder: md_cone.c) */
  if (md_spr_reserved) sp_ptr = spr + 2;       /* salas con el ojo: dos sprites que tapan el HUD (md_cone.c) */
  /* en el SNES el HUD (BG3 con prioridad, BGMODE bit 3) tapa todos los sprites de las líneas 0-31; en la Genesis la
     Window no tapa a los sprites: los dos primeros de la lista los enmascaran (como en las salas del ojo, md_cone.c:
     8 x 32 en las líneas del HUD, el primero con x = 1, fuera de la pantalla, y el segundo con x = 0) */
  else if ((uint16)(game_state - 7) <= 4) {
    spr[0].y = 128; spr[0].size_link = (3 << 8) | 1; spr[0].attr = 0x8000; spr[0].x = 1;
    spr[1].y = 128; spr[1].size_link = (3 << 8) | 2; spr[1].attr = 0x8000; spr[1].x = 0;
    sp_ptr = spr + 2;
  }
  door_seen = 0;
  flash_seen = 0;
  game_frame(viewer_input(joy));
  { extern uint8 md_snd_pending; if (md_snd_pending) { extern void md_snd_frame(void); md_snd_frame(); } }
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
  if (gr_dirty_l) { gr_dirty = 1; gr_dirty_l = 0; }
  if (beam_dmal) { beam_dma = beam_dmal; beam_dmal = 0; }
  if (ga_called) ga_want = ga_want_l;
  m7_variant = m7_variant_l;
  if (md_zebes) zebes_take();
}

/* fundido a negro: cram_out = los colores con cada componente (3 bits) por brillo/15. Se arma al preparar el cuadro
   (md_post), no en el vblank: son ~25 líneas que se comían el vblank de la cinemática (docs/54) */
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
uint8 md_cone_pb_tint(int16 *r, int16 *g, int16 *b);
const void *md_cone_sat(uint8 bank);
void md_cone_color(void);
void md_cone_vblank(void);
void md_cone_room(void);
/* lluvia (FX 0x0A, la sala de aterrizaje: docs/59). En el SNES el BG3 lleva una bruma (el tinte, lq_prepare) y motas
   sueltas (colores 1-2 de los tiles 0x50-0x53 del BG3) que caen ~6 píxeles por cuadro y se mueven con la cámara
   (HdmaobjPreInstr_RainBg3Scroll $88:D9A1). Aquí: 16 sprites de 8x32 con esos 4 tiles apilados (las motas en un
   color libre de la línea 3: zebes_cone_slot), en lugares fijos de un dibujo de 256x256 que cae y deriva. */
#define RAIN_TILE 2039                          /* 4 tiles en 0xF000 (sin el ojo; el gancho desde el 2043) */
static const uint8 kRainPos[16][2] = {
  { 12, 20 }, { 71, 140 }, { 133, 60 }, { 197, 200 }, { 40, 95 }, { 101, 230 }, { 166, 10 }, { 228, 120 },
  { 25, 175 }, { 88, 45 }, { 150, 150 }, { 212, 75 }, { 55, 250 }, { 118, 110 }, { 182, 35 }, { 244, 185 } };
static uint8 rain_ox, rain_oy;
static uint8 rain_slot, rain_up, rain_line, rain_fixed, rain_slot_k = 0xFF, rain_pal_dirty = 1;   /* color de las motas (línea, índice) */
#define rain_tiles ((uint16 *)((uint8 *)cinematic_text_tilemap + 3776))   /* los 4 tiles ya convertidos (128 bytes) */
extern volatile uint16 md_frame_count;
uint8 zebes_cone_slot(uint16 c);
static void rain_post(void) {
  rain_slot = 0;
  if (!md_zebes || cine_stage || fx_type != 0xA || nt_msgbox_active) return;
  if (rain_pal_dirty && (rain_slot_k == 0xFF || !(md_frame_count & 15))) {   /* el color de las motas: si cambió la
                                                   paleta, como mucho cada 16 cuadros (en la sala de aterrizaje cambia casi siempre) */
    rain_pal_dirty = 0;
    static const uint8 kTry[5] = { 15, 14, 8, 7, 6 };
    uint8 sl = 0, ln = 3, fx = 0;
    for (uint16 i = 0; i < 5 && !sl; i++) if (zebes_cone_slot(kTry[i])) sl = kTry[i];
    if (!sl) {                                  /* sin lugar libre: el color más parecido de la CRAM (como el gancho) */
      uint16 want = kCram555[palette_buffer[25] & 0x7FFF], best = 0xFFFF;
      for (uint16 i = 1; i < 64; i++) {
        if (!(i & 15)) continue;
        uint16 v = i >= 48 ? zobj_col[i - 49] : cram[i];
        int16 dr = (int16)(v & 0xE) - (int16)(want & 0xE), dg = (int16)((v >> 4) & 0xE) - (int16)((want >> 4) & 0xE);
        int16 db = (int16)((v >> 8) & 0xE) - (int16)((want >> 8) & 0xE);
        uint16 d = (uint16)(dr * dr + dg * dg + db * db);
        if (d < best) { best = d; sl = i & 15; ln = (uint8)(i >> 4); }
      }
      fx = 1;                                   /* (no se pinta: se usa como está) */
    }
    rain_line = ln; rain_fixed = fx;
    if (sl != rain_slot_k) {                    /* los 4 tiles (2 bpp del SNES -> el índice sl), para el vblank */
      rain_slot_k = sl;
      uint16 *o = rain_tiles;
      for (uint16 t = 0; t < 4; t++)
        for (uint16 y = 0; y < 8; y++) {
          uint8 a = kRainSnesTiles[t * 16 + 2 * y], b = kRainSnesTiles[t * 16 + 2 * y + 1];
          uint32 row = 0;
          for (uint16 x = 0; x < 8; x++) {
            uint8 v = ((a >> (7 - x)) & 1) | (((b >> (7 - x)) & 1) << 1);
            row = row << 4 | ((v == 1 || v == 2) ? sl : 0);
          }
          *o++ = (uint16)(row >> 16); *o++ = (uint16)row;
        }
      rain_up = 1;
    }
  }
  rain_slot = rain_slot_k == 0xFF ? 0 : rain_slot_k;
  if (!rain_slot) return;
  rain_oy += 6; rain_ox -= 1;
  for (uint16 i = 0; i < 16 && sp_ptr < spr + MAX_SPR; i++) {
    int16 x = (int16)((kRainPos[i][0] + rain_ox - layer1_x_pos) & 255);
    int16 y = (int16)((kRainPos[i][1] + rain_oy - layer1_y_pos) & 255);
    if (y >= 224 || y < 32 || x >= 256) continue;   /* (no sobre el HUD: líneas 0-31) */
    GenSpr *s = sp_ptr++;
    s->y = y + 129;
    s->size_link = 0x0300 | (uint16)(s - spr + 1);   /* 8x32 */
    s->attr = 0x8000 | ((uint16)rain_line << 13) | RAIN_TILE;
    s->x = x + 128;
  }
  if (!(md_frame_count & 63)) rain_up = 1;      /* (la bomba de poder o la caja de mensaje pueden pisar 0xF000) */
}
static void rain_vblank(void) {                 /* los 4 tiles (rain_post los convirtió) */
  rain_up = 0;
  vdp_dma_vram(rain_tiles, RAIN_TILE * 32, 64);
}

void md_post(void) {
  if (pz_dim) { rain_slot_k = 0xFF; rain_pal_dirty = 1; md_pause_frame(); return; }   /* pausa (estados 12-18): md_pause.c (pisa la RAM de los tiles de la lluvia) */
  pipe_take();
  md_haze_read();
  ncone = (md_zebes && !cine_stage && !nt_msgbox_active && !mb_shown) ? md_cone_frame() : 0;
  if (md_zebes) md_msgbox_post();
  /* conos de luz (md_cone.c): al final de la lista de sprites */
  rain_post();
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
  ga_post();
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
  /* el tinte de la matemática de color (ver tint_cram) */
  {
    int16 r = 0, g = 0, b = 0;
    if (md_zebes && !cine_stage && hz_state == HZ_OFF) {
      uint8 cfg = (uint8)fx_layer_blending_config_c;
      if (cfg == 0x2A) { r = -6; g = -2; }                      /* LayerBlendFunc_2A: COLDATA 38/66/0x80, resta */
      else if (cfg == 0x28) { r = -5; }                         /* LayerBlendFunc_28: COLDATA 37/64/0x80, resta */
      /* (0x2C, la neblina del Bomb Torizo: por franjas, lq_prepare) */
      if (fx_type == 0x0C) {                                    /* niebla: su BG3 sumado */
        uint32 sr = 0, sg = 0, sb = 0;
        for (uint16 i = 0; i < 32; i++) {
          uint16 w = kFogW[i], c = palette_buffer[i];
          if (!w) continue;
          sr += (uint32)w * (c & 31); sg += (uint32)w * ((c >> 5) & 31); sb += (uint32)w * ((c >> 10) & 31);
        }
        r += (int16)((sr + 32768) >> 16); g += (int16)((sg + 32768) >> 16); b += (int16)((sb + 32768) >> 16);
      }
      /* bomba de poder cubriendo toda el área de juego (md_cone.c, docs/58): solo su color fijo, sumado (en el SNES
         su matemática de color reemplaza la de la sala: HandleLayerBlendingPowerBomb, $88:81FE) */
      int16 pr = 0, pg = 0, pbl = 0;
      if (md_cone_pb_tint(&pr, &pg, &pbl)) { r = pr; g = pg; b = pbl; }
    }
    uint8 on = r || g || b;
    if (on != tint_on || r != tint_r || g != tint_g || b != tint_b) {
      tint_on = on; tint_r = (int8)r; tint_g = (int8)g; tint_b = (int8)b;
      if (on) tint_tables();
      nt_palette_changed = 1; nt_palette_lo = 0; nt_palette_hi = 255;
    }
  }
  /* colores: cada uno sale de un índice de palette_buffer (tabla de la sala), solo si la paleta cambió */
  if (nt_palette_changed) {
    nt_palette_changed = 0;
    rain_pal_dirty = 1;
    if (nt_palette_hi - nt_palette_lo < 32) {   /* pocos índices: el líquido rehace solo sus colores (lq_prepare) */
      if (nt_palette_lo < lq_lo) lq_lo = (uint8)nt_palette_lo;
      if (nt_palette_hi > lq_hi) lq_hi = (uint8)nt_palette_hi;
    } else lq_pal_dirty = 1;
    const uint8 *pb = (const uint8 *)palette_buffer;
    const uint8 *lut = (const uint8 *)kCram555;
    if (nt_palette_hi - nt_palette_lo < 32) {
      /* pocos índices (ciclo de color de la puerta, paleta de Samus): solo los colores que salen de ellos */
      for (uint16 i = nt_palette_lo; i <= nt_palette_hi; i++) {
        uint8 slot = cram_rev[i];
        if (slot == 0xFF) continue;
        do {
          uint16 v = *(const uint16 *)(lut + (uint16)(*(const uint16 *)(pb + i * 2) << 1));
          if (tint_on && slot < 48) v = tint_cram(palette_buffer[i]);
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
        if (tint_on && c < cram + 48 && o != 0xFFFF) v = tint_cram(*(const uint16 *)(pb + o));
        if (v != *c) { *c = v; cram_dirty = 1; }
        c++;
      }
    }
    if (palette_buffer[0] != hz_last_c) hz_bd_dirty = 1;   /* el degradado sale solo del color de fondo */
    rl_pal_dirty = 1;
  }
  if (md_zebes) { zebes_obj_colors(); boss_flash_post(); md_cone_color(); }
  if (md_zebes && (fx_type == 2 || fx_type == 4 || fx_type == 6 || fx_type == 0x26 || (uint8)fx_layer_blending_config_c == 0x2C))
    lq_prepare();
  else lq_on = 0;
  /* (la bruma de la lluvia, FX 0x0A, no: suma G+1 B+2 de 5 bits, en la Genesis casi nada, y en la sala de aterrizaje
     la paleta cambia casi cada cuadro: rehacer las paletas atrasaba el juego) */
  if (rain_slot && !rain_fixed) {               /* el color de las motas de la lluvia (rain_post) */
    uint16 c = kCram555[palette_buffer[25] & 0x7FFF];
    zobj_col[rain_slot - 1] = c;
    if (lq_on) lq_col[48 + rain_slot] = c;
  }
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
  if (lq_on && lq_wv_next[14]) {               /* onda de Crocomire: el HDMA reemplaza al BG2VOFS en todas las líneas */
    scroll_by += (uint16)(crocomire_bg2_scroll_hdma_data[0] - reg_BG2VOFS);
    for (uint16 k = 0; k < lq_wn; k++) lq_wv_next[k] += scroll_by;
  }
  lq_wv_next[15] = scroll_by;
  if (cine_stage) cine_frame(); else hud_prepare();
  if (md_zebes && !cine_stage) {
    /* la ventana dibujada sigue a la cámara sin el temblor (unos píxeles: entra en su margen); con él se volvían a
       dibujar columnas y filas en cada cuadro del temblor (Kraid hundiéndose) */
    zebes_frame(scroll_ax - nt_shake_bg1x, scroll_ay - nt_shake_bg1y,
                g_room->b_mode == 1 ? scroll_bx : scroll_bx - nt_shake_bg2x,
                g_room->b_mode == 1 ? scroll_by : scroll_by - nt_shake_bg2y);
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
  if (gr_dirty) { vdp_dma_vram(&gr_tiles[0][0], GRAPPLE_TILE * 32, 80); gr_dirty = 0; }
  if (beam_dma) {                    /* (y los 6 de la explosión del rayo, fijos: mkmd.beam_common) */
    vdp_dma_vram(beam_dma, BEAM_TILE * 32, 128); vdp_dma_vram(kBeamExplTiles, BEAM_EXPL_TILE * 32, 96); beam_dma = 0;
  }
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
  if (rain_up && rain_slot && md_zebes && !room_pending) rain_vblank();
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
    /* (con la caja de mensaje la neblina no: sus paletas comparten la RAM con la caja y el juego está parado) */
    if (want == 1 && lq_on && !(lq_haze && nt_msgbox_active)) {   /* líquido (md_hint_lq) y la onda de Crocomire */
      want = 3; memcpy(lq_steps, lq_next, LQ_MAXSTEP * sizeof(LqStep)); memcpy(lq_wv, lq_wv_next, 32); lq_hp = lq_steps;
    }   /* líquido (md_hint_lq) */
    if (zobj_hint != want) { SYS_setHIntCallback(want == 2 ? md_hint_msg : want == 3 ? md_hint_lq : md_hint_obj); zobj_hint = want; }
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
