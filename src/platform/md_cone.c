/* MegaMetroid — capa de la Genesis: el haz de luz del ojo (MorphBallEye; después, los rayos X).
 *
 * Lo que hace el SNES (decomp, docs/46):
 *  - MorphBallEye_Func_2 ($A8:912E) crea el HDMA $88:E8EC (SpawnMorphBallEyeBeamHdma): una tabla por línea con los
 *    bordes de la ventana 1 (7E:9100: izquierda en el byte bajo, derecha en el alto; CalculateXrayHdmaTableInner,
 *    hdmaobj.c la calcula igual).
 *  - fx_layer_blending_config_c = 16 -> LayerBlendFunc_10 ($88:80B8): dentro de la ventana el BG3 (un tile negro
 *    opaco, 0x184E) se quita de la subpantalla, y ahí se suma el color fijo COLDATA a BG1, BG2, sprites (paletas 4-7)
 *    y fondo (CGADSUB 0x33). COLDATA: R = G = 16 bajando a 8 y volviendo en 16 cuadros (tabla $88:EA8B), B = 0.
 *  - Al encenderse, el ancho del haz crece (HdmaobjPreInstr_E9E6); al apagarse, COLDATA baja hasta 0 ($88:EACB).
 *
 * En la Genesis: no hay ventanas por línea ni sumas de color. Lo más parecido es el modo sombra/luz del VDP: un
 * sprite con el color 14 de la línea 3 no se dibuja y aclara lo que tiene debajo (planos y fondo). La forma del haz
 * es la tabla del SNES, línea por línea: cada banda de 8 líneas se cubre con sprites de 8 de alto; las columnas
 * donde el borde pasa dentro de la banda llevan un tile armado en el cuadro (cada fila, el tramo exacto de esa
 * línea), las del medio tiles llenos fijos.
 *
 * Lo que cambia con el modo sombra/luz y cómo se resuelve (solo en las salas con el ojo: zebes_eye_room):
 *  - el VDP oscurece todo píxel que no tenga prioridad alta en ningún plano: las celdas de esas salas vienen con
 *    prioridad alta (mkzebes.py) y los sprites se pasan a prioridad alta aquí (si no, quedarían detrás de todo);
 *  - con prioridad alta los sprites pasarían por encima del HUD (plano ventana): dos sprites al principio de la lista
 *    en las líneas del HUD los enmascaran (el VDP no dibuja los sprites que siguen a uno con x = 0 en esa línea si
 *    antes hubo uno con x distinta);
 *  - en los sprites, los colores 14 y 15 de la línea 3 son "aclarar" y "oscurecer": en esas salas los enemigos no
 *    los usan (mkzobj.py, CONE_SLOTS).
 * Diferencias que quedan con el SNES: "aclarar" lleva hacia el blanco (el SNES suma amarillo), no aclara a los
 * sprites, y no tiene intensidad: mientras COLDATA está entre 8 y 16 se aclara siempre; por debajo de 8 (al
 * apagarse), un cuadro sí y otro no; por debajo de 4, nada. */
#pragma GCC optimize("O2", "no-unroll-loops")
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "md.h"
#include "md_vdp.h"

typedef struct { int16 y; uint16 size_link; uint16 attr; int16 x; } GenSpr;
extern GenSpr *sp_ptr, *spr;
uint8 zebes_cone_slot(uint16 c);   /* el color c de la línea 3 está libre en la sala (md_zebes.c) */
uint8 zebes_eye_room(void);        /* sala con el ojo: celdas con prioridad alta (md_zebes.c) */

#define CONE_TILE0 1920                 /* VRAM 0xF000-0xFFFF: 128 tiles */
#define CONE_FULL CONE_TILE0            /* 4 tiles llenos (sprites de hasta 4 de ancho) */
#define CONE_EDGE (CONE_TILE0 + 4)      /* bordes rectos fijos: 92 tiles (cone_edge_tiles) */
#define CONE_NEDGE 92
#define CONE_DYN (CONE_EDGE + CONE_NEDGE)   /* tiles armados en el cuadro (bandas que no son dos bordes rectos) */
#define CONE_DYN_MAX (128 - 4 - CONE_NEDGE)
#define CONE_SPR_MAX 44
/* el armado del haz se hace de a pasos cortos (md_cone_idle y md_cone_frame): cuánto hace cada paso. Con tiempo
   libre: líneas de la tabla, líneas de bandas, tiles armados; sin tiempo libre (uno por cuadro): lo mismo, menos */
#ifndef CONE_TAB_STEP
#define CONE_TAB_STEP 32
#endif
#ifndef CONE_BAND_STEP
#define CONE_BAND_STEP 24
#endif
#ifndef CONE_DYN_STEP
#define CONE_DYN_STEP 6
#endif
#ifndef CONE_TAB_SMALL
#define CONE_TAB_SMALL 24
#endif
#ifndef CONE_BAND_SMALL
#define CONE_BAND_SMALL 16
#endif
#ifndef CONE_DYN_SMALL
#define CONE_DYN_SMALL 3
#endif
#ifndef CONE_THROTTLE
#define CONE_THROTTLE 1                 /* sin tiempo libre, el paso corto solo uno de cada dos cuadros */
#endif
#define CONE_TOP 32                     /* debajo del HUD */
#define CONE_COLOR 14                   /* modo sombra/luz: aclarar */
#define MAX_SPR 64

static uint8 cone_on, cone_on_shown, cone_fixed_ok, sh_on;
static uint16 cone_ndyn;                /* tiles armados de este cuadro (los copia el vblank) */
extern uint16 beam_tab_version;
void beam_job_start(void);           /* hdmaobj.c: la tabla del haz, por partes */
uint8 xray_job_run(uint16 budget);
static uint32 cache_a[2 * CONE_SPR_MAX];
static uint32 *cache = cache_a;         /* los sprites del haz que se muestran */
/* el que se arma (el otro): en el texto de las cinemáticas (la intro), que no se usa en Zebes (la RAM es justa) */
#define cone_buf ((uint32 *)cinematic_bg_tilemap)   /* 2 KB: solo los usan la intro y las cinemáticas */
#define CACHE_B ((uint32 *)((uint8 *)cinematic_text_tilemap + 1600))
static uint32 *cache_b;
static uint16 cache_n, cpar[4];         /* sprites que se muestran; con qué valores del haz se armaron */
/* El haz nuevo se arma en el tiempo que le sobra a los cuadros (md_cone_idle, mientras la consola espera): primero
   la tabla del SNES (hdmaobj.c, de a unas líneas), después los sprites (de a unas bandas), en el otro búfer; cuando
   está, md_cone_frame lo cambia por el que se muestra. Si un cuadro no deja tiempo, el haz anterior sigue (corrido
   con la cámara) un poco más: el juego nunca espera al haz. */
static uint8 job;                       /* 0 nada, 1 la tabla, 2 los sprites, 3 listo */
static uint16 jpar[4], jn, jndyn;
typedef struct { const uint8 *tp; uint32 *o, *omax; uint16 *fb; uint16 link, y; } ConeArgs;
static ConeArgs jca;
static uint16 jfby[2];
static uint32 *jtb;
static int16 jcol = -1;                 /* banda armada a medias: desde qué columna sigue */
typedef struct { uint16 serial; int16 dx, dy; uint16 n; } BankSig;
static GenSpr cone_sat[2][CONE_SPR_MAX];  /* los sprites del haz de cada lista */
static BankSig bank_sig[2];             /* qué haz tiene cada una de las dos listas de sprites */
static uint16 cache_serial = 1;
void beam_params(uint16 *p);            /* hdmaobj.c */
static uint8 cache_ok;

/* fila de un tile (8 píxeles de 4 bits): el color del cono en los píxeles a..b; a de 0 a 8, b de -1 a 7 */
uint32 kRow[9][9] __attribute__((used));
int16 cone_lr[16] __attribute__((used));   /* cone_build: izquierda y derecha de las 8 líneas */
uint8 kEdgeTile[8][8] __attribute__((used));           /* índice desde CONE_EDGE de cada borde (a, d) */
static uint8 rows_ok;
static void make_rows(void) {
  for (int16 a = 0; a <= 8; a++)
    for (int16 b = -1; b <= 7; b++) {
      uint32 w = 0;
      for (int16 x = a; x <= b; x++) w |= 0xEu << (4 * (7 - x));
      kRow[a][b + 1] = w;
    }
  uint16 n = 0;
  for (uint16 a = 0; a < 8; a++)
    for (uint16 d = 0; d < 8; d++) { kEdgeTile[a][d] = n; n += (a + d > 7) ? 2 : 1; }
  rows_ok = 1;
}

/* Bordes rectos (los del haz lo son: dos rayos desde el ojo). Un borde izquierdo (lleno a su derecha) que en una
   banda de 8 líneas empieza en el píxel a de su columna y baja d píxeles a la derecha (0 <= d <= 7): fila i en el
   píxel e = a + (d * i + 3) / 7. Ocupa una columna, o dos si a + d > 7 (la segunda, consecutiva en la VRAM). Con el
   volteo vertical sirve para d < 0; con el horizontal, para los bordes derechos. */
static void edge_rows(uint16 a, uint16 d, uint16 col, uint32 *t) {
  for (uint16 i = 0; i < 8; i++) {
    int16 e = a + (d * i + 3) / 7 - 8 * col;
    if (e < 0) e = 0;
    t[i] = kRow[e > 8 ? 8 : e][8];
  }
}

/* las salas con el ojo: los sprites con prioridad alta y, delante de todos, los dos que tapan el HUD */
extern uint8 md_spr_reserved;
static void eye_room_sprites(void) {
  if (!md_spr_reserved) {                  /* el cuadro en que se entra a la sala: correr la lista dos lugares */
    uint16 n = (uint16)(sp_ptr - spr);
    if (n > MAX_SPR - 2) n = MAX_SPR - 2;
    for (int16 i = n - 1; i >= 0; i--) {
      spr[i + 2] = spr[i];
      spr[i + 2].size_link = (spr[i + 2].size_link & 0x0F00) | (uint16)(i + 3);
    }
    sp_ptr = spr + n + 2;
  }
  /* md_logic dejó libres los dos primeros (sp_ptr empieza en spr + 2) */
  for (GenSpr *q = spr + 2; q < sp_ptr; q++) q->attr |= 0x8000;
  /* 1 x 4 celdas (8 x 32) en las líneas del HUD: el primero con x = 1 (fuera de la pantalla), el segundo con x = 0
     enmascara a todos los que siguen en esas líneas */
  spr[0].y = 128; spr[0].size_link = (3 << 8) | 1; spr[0].attr = 0x8000; spr[0].x = 1;
  spr[1].y = 128; spr[1].size_link = (3 << 8) | 2; spr[1].attr = 0x8000; spr[1].x = 0;
  if (sp_ptr < spr + 2) sp_ptr = spr + 2;
}

/* cone_bands(&args): desde la banda args.y (con su tabla en args.tp), las bandas de 8 líneas con dos bordes rectos,
   como sprites (fijos: bordes y llenos), hasta la primera que no lo sea (líneas sin haz, bordes muy inclinados o
   que se tocan): deja su y en args.fb y vuelve (la arma C y sigue). Las bandas vacías se saltan. Así se dibuja de
   arriba (el ojo) hacia abajo hasta acabar los sprites. Escrito en ensamblador: en C costaba un tercio de cuadro. */
uint16 cone_ymax __attribute__((used)) = 352;    /* cone_bands: hasta qué banda (y + 128) */
void cone_bands(ConeArgs *a);
__asm__(
  "    .text\n"
  "    .global cone_bands\n"
  "cone_bands:\n"
  "    movem.l %d2-%d7/%a2-%a6,-(%sp)\n"
  "    move.l 48(%sp),%a6\n"
  "    move.l (%a6),%a0\n"                    /* tabla (bytes: derecha, izquierda por línea) */
  "    move.l 4(%a6),%a1\n"                   /* salida */
  "    move.l 8(%a6),%a2\n"                   /* límite */
  "    move.l 12(%a6),%a4\n"                  /* bandas para C */
  "    move.w 16(%a6),%d6\n"                  /* enlace */
  "    lea kEdgeTile,%a3\n"
  "    move.w 18(%a6),%d7\n"                  /* y de la banda */
  "1:  cmpa.l %a2,%a1\n"
  "    bcc 90f\n"
  "    movea.l %a1,%a5\n"                     /* comienzo de la banda (para deshacer) */
  "    moveq #0,%d0\n"
  "    move.b (%a0),%d0\n"                    /* r0 */
  "    moveq #0,%d1\n"
  "    move.b 1(%a0),%d1\n"                   /* l0 */
  "    moveq #0,%d2\n"
  "    move.b 14(%a0),%d2\n"                  /* r7 */
  "    moveq #0,%d3\n"
  "    move.b 15(%a0),%d3\n"                  /* l7 */
  "    cmp.w %d0,%d1\n"
  "    bhi 80f\n"
  "    cmp.w %d2,%d3\n"
  "    bhi 80f\n"
  /* borde izquierdo */
  "    move.w %d3,%d4\n"
  "    sub.w %d1,%d4\n"                       /* dl */
  "    bpl 2f\n"
  "    neg.w %d4\n"
  "    move.w %d3,%d1\n"
  "    move.w #0x1000,%d5\n"
  "    bra 3f\n"
  "2:  moveq #0,%d5\n"
  "3:  cmp.w #7,%d4\n"
  "    bhi 85f\n"
  "    move.w %d1,%d3\n"
  "    lsr.w #3,%d3\n"                        /* columna */
  "    and.w #7,%d1\n"                        /* a */
  "    movea.w %d1,%a6\n"
  "    adda.w %d4,%a6\n"                      /* a + d */
  "    lsl.w #3,%d1\n"
  "    add.w %d4,%d1\n"
  "    moveq #0,%d4\n"
  "    move.b (%a3,%d1.w),%d4\n"
  "    add.w #0xE784,%d4\n"                   /* 0xE000 | CONE_EDGE (1924) */
  "    or.w %d5,%d4\n"
  "    moveq #0,%d1\n"
  "    cmpa.w #7,%a6\n"
  "    ble 4f\n"
  "    moveq #1,%d1\n"
  "4:  move.w %d7,(%a1)+\n"
  "    move.w %d1,%d5\n"
  "    lsl.w #8,%d5\n"
  "    lsl.w #2,%d5\n"
  "    or.w %d6,%d5\n"
  "    move.w %d5,(%a1)+\n"
  "    addq.w #1,%d6\n"
  "    move.w %d4,(%a1)+\n"
  "    move.w %d3,%d5\n"
  "    lsl.w #3,%d5\n"
  "    add.w #128,%d5\n"
  "    move.w %d5,(%a1)+\n"
  "    movea.w %d3,%a6\n"
  "    adda.w %d1,%a6\n"
  "    addq.w #1,%a6\n"                       /* primera llena */
  /* borde derecho: el izquierdo del mundo reflejado (x -> 255 - x), con volteo horizontal */
  "    move.w #255,%d1\n"
  "    sub.w %d0,%d1\n"                       /* m0 */
  "    move.w #255,%d3\n"
  "    sub.w %d2,%d3\n"                       /* m7 */
  "    move.w %d3,%d4\n"
  "    sub.w %d1,%d4\n"
  "    bpl 5f\n"
  "    neg.w %d4\n"
  "    move.w %d3,%d1\n"
  "    move.w #0x1800,%d5\n"
  "    bra 6f\n"
  "5:  move.w #0x0800,%d5\n"
  "6:  cmp.w #7,%d4\n"
  "    bhi 85f\n"
  "    move.w %d1,%d3\n"
  "    lsr.w #3,%d3\n"
  "    and.w #7,%d1\n"
  "    move.w %d1,%d0\n"
  "    add.w %d4,%d0\n"                       /* a + d */
  "    lsl.w #3,%d1\n"
  "    add.w %d4,%d1\n"
  "    moveq #0,%d4\n"
  "    move.b (%a3,%d1.w),%d4\n"
  "    add.w #0xE784,%d4\n"
  "    or.w %d5,%d4\n"
  "    moveq #0,%d1\n"
  "    cmp.w #7,%d0\n"
  "    ble 7f\n"
  "    moveq #1,%d1\n"
  "7:  moveq #31,%d2\n"
  "    sub.w %d3,%d2\n"
  "    sub.w %d1,%d2\n"                       /* columna del derecho */
  "    cmpa.w %d2,%a6\n"
  "    bgt 85f\n"                             /* los bordes se tocan */
  "    move.w %d7,(%a1)+\n"
  "    move.w %d1,%d5\n"
  "    lsl.w #8,%d5\n"
  "    lsl.w #2,%d5\n"
  "    or.w %d6,%d5\n"
  "    move.w %d5,(%a1)+\n"
  "    addq.w #1,%d6\n"
  "    move.w %d4,(%a1)+\n"
  "    move.w %d2,%d5\n"
  "    lsl.w #3,%d5\n"
  "    add.w #128,%d5\n"
  "    move.w %d5,(%a1)+\n"
  /* llenas: desde la primera hasta la anterior al derecho, de a 4 */
  "    move.w %a6,%d0\n"
  "    move.w %d2,%d3\n"
  "    sub.w %d0,%d3\n"
  "8:  tst.w %d3\n"
  "    ble 95f\n"
  "    cmpa.l %a2,%a1\n"
  "    bcc 95f\n"
  "    moveq #4,%d5\n"
  "    cmp.w %d5,%d3\n"
  "    bge 9f\n"
  "    move.w %d3,%d5\n"
  "9:  move.w %d7,(%a1)+\n"
  "    move.w %d5,%d4\n"
  "    subq.w #1,%d4\n"
  "    lsl.w #8,%d4\n"
  "    lsl.w #2,%d4\n"
  "    or.w %d6,%d4\n"
  "    move.w %d4,(%a1)+\n"
  "    addq.w #1,%d6\n"
  "    move.w #0xE780,(%a1)+\n"               /* 0xE000 | CONE_FULL (1920) */
  "    move.w %d0,%d4\n"
  "    lsl.w #3,%d4\n"
  "    add.w #128,%d4\n"
  "    move.w %d4,(%a1)+\n"
  "    add.w %d5,%d0\n"
  "    sub.w %d5,%d3\n"
  "    bra 8b\n"
  /* banda con líneas sin haz: si están todas vacías, nada; si no, a C */
  "80: moveq #7,%d4\n"
  "    movea.l %a0,%a6\n"
  "81: move.b (%a6)+,%d0\n"
  "    cmp.b (%a6)+,%d0\n"
  "    bcc 82f\n"                             /* izquierda <= derecha: hay haz en esta línea */
  "    dbra %d4,81b\n"
  "    bra 95f\n"
  "82: move.w %d7,(%a4)+\n"
  "    bra 96f\n"
  /* deshacer lo de la banda y mandarla a C */
  "85: move.l %a1,%d0\n"
  "    sub.l %a5,%d0\n"
  "    lsr.l #3,%d0\n"
  "    sub.w %d0,%d6\n"
  "    movea.l %a5,%a1\n"
  "    move.w %d7,(%a4)+\n"
  "96: lea 16(%a0),%a0\n"                   /* banda para C: vuelve con la siguiente */
  "    addq.w #8,%d7\n"
  "    bra 90f\n"
  "95: lea 16(%a0),%a0\n"
  "    addq.w #8,%d7\n"
  "    cmp.w cone_ymax,%d7\n"
  "    blt 1b\n"
  "90: move.l 48(%sp),%a6\n"
  "    move.l %a0,(%a6)\n"
  "    move.w %d7,18(%a6)\n"
  "    move.l %a1,4(%a6)\n"
  "    move.l %a4,12(%a6)\n"
  "    move.w %d6,16(%a6)\n"
  "    movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
  "    rts\n");

/* cone_build(&args): una banda que no son dos bordes rectos, línea por línea: un tile por columna (las filas de
   kRow con el tramo de cada línea) y sprites de hasta 4 de ancho; las columnas llenas en las 8 líneas (el medio de
   un haz ancho) van con los tiles llenos fijos, sin armar. En ensamblador por lo mismo que cone_bands. */
/* cone_build: con 2 * (izquierda - x0) da 36 * a (a = izquierda - x0 recortada a 0..8) y con 2 * (derecha - x0) + 1536
   da 4 * (b + 1) (b = derecha - x0 recortada a -1..7): sumados, el desplazamiento en kRow de la fila del tile */
const int16 kBuildIdx[1279] __attribute__((used)) = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 72, 108, 144, 180, 216, 252, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288,
  288, 288, 288, 288, 288, 288, 288, 288, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32,
};
int16 cone_fc0 __attribute__((used)), cone_fc1 __attribute__((used));   /* columnas llenas de la banda */
typedef struct { const uint8 *tp; uint32 *tb, *o, *omax; uint16 ndyn, ndyn_max, link, y; int16 col0; uint16 cap; } BuildArgs;
void cone_build(BuildArgs *a);
__asm__(
  "    .text\n"
  "    .global cone_build\n"
  "cone_build:\n"
  "    movem.l %d2-%d7/%a2-%a6,-(%sp)\n"
  "    move.l 48(%sp),%a6\n"
  "    move.l (%a6),%a0\n"
  "    lea cone_lr,%a2\n"
  "    move.w #256,%d4\n"                     /* izquierda mínima */
  "    moveq #-1,%d5\n"                       /* derecha máxima */
  "    moveq #-1,%d2\n"                       /* izquierda máxima */
  "    move.w #256,%d3\n"                     /* derecha mínima */
  "    moveq #0,%d7\n"                        /* alguna línea sin haz */
  "    moveq #7,%d6\n"
  "    movea.l %a2,%a3\n"
  "1:  moveq #0,%d0\n"
  "    move.b (%a0)+,%d0\n"                   /* derecha */
  "    moveq #0,%d1\n"
  "    move.b (%a0)+,%d1\n"                   /* izquierda */
  "    cmp.w %d0,%d1\n"
  "    bls 2f\n"
  "    move.w #512,(%a3)\n"                   /* línea sin haz (2 * 256 y 2 * -1 + 1536) */
  "    move.w #1534,16(%a3)\n"
  "    moveq #1,%d7\n"
  "    bra 3f\n"
  "2:  movea.w %d1,%a1\n"                    /* se guardan 2 * izquierda y 2 * derecha + 1536 (para kBuildIdx) */
  "    adda.w %a1,%a1\n"
  "    move.w %a1,(%a3)\n"
  "    movea.w %d0,%a1\n"
  "    adda.w %a1,%a1\n"
  "    lea 1536(%a1),%a1\n"
  "    move.w %a1,16(%a3)\n"
  "    cmp.w %d4,%d1\n"
  "    bge 4f\n"
  "    move.w %d1,%d4\n"
  "4:  cmp.w %d5,%d0\n"
  "    ble 5f\n"
  "    move.w %d0,%d5\n"
  "5:  cmp.w %d2,%d1\n"
  "    ble 6f\n"
  "    move.w %d1,%d2\n"
  "6:  cmp.w %d3,%d0\n"
  "    bge 3f\n"
  "    move.w %d0,%d3\n"
  "3:  addq.l #2,%a3\n"
  "    dbra %d6,1b\n"
  "    tst.w %d5\n"
  "    bmi 96f\n"
  /* columnas llenas en las 8 líneas: de (izquierda máxima + 7) / 8 a (derecha mínima - 7) / 8 (con todas las
     líneas con haz): van con los tiles llenos fijos, sin armar */
  "    tst.w %d7\n"
  "    bne 7f\n"
  "    addq.w #7,%d2\n"
  "    asr.w #3,%d2\n"
  "    subq.w #7,%d3\n"
  "    asr.w #3,%d3\n"
  "    cmp.w %d3,%d2\n"
  "    ble 8f\n"
  "7:  move.w #1000,%d2\n"
  "    moveq #-1,%d3\n"
  "8:  move.w %d2,cone_fc0\n"
  "    move.w %d3,cone_fc1\n"
  "    lsr.w #3,%d4\n"                        /* columna */
  "    lsr.w #3,%d5\n"                        /* última */
  "    move.w 24(%a6),%d0\n"                 /* se sigue una banda ya empezada: desde esa columna */
  "    bmi 90f\n"
  "    move.w %d0,%d4\n"
  "90:\n"
  "    move.l 4(%a6),%a1\n"                   /* tiles */
  "    move.l 8(%a6),%a4\n"                   /* sprites */
  "    lea kBuildIdx+510,%a5\n"
  "    lea kRow,%a3\n"
  "10: cmp.w %d5,%d4\n"
  "    bgt 97f\n"
  "    cmpa.l 12(%a6),%a4\n"
  "    bcc 97f\n"
  "    move.w cone_fc1,%d3\n"
  "    cmp.w cone_fc0,%d4\n"
  "    blt 20f\n"                            /* antes de las llenas: armadas hasta la anterior a la primera llena */
  "    cmp.w %d3,%d4\n"
  "    bgt 21f\n"                            /* después: armadas hasta la última */
  /* llenas: hasta 4 con los tiles fijos */
  "    sub.w %d4,%d3\n"
  "    addq.w #1,%d3\n"
  "    cmp.w #4,%d3\n"
  "    ble 22f\n"
  "    moveq #4,%d3\n"
  "22: move.w 22(%a6),(%a4)+\n"
  "    move.w %d3,%d0\n"
  "    subq.w #1,%d0\n"
  "    lsl.w #8,%d0\n"
  "    lsl.w #2,%d0\n"
  "    or.w 20(%a6),%d0\n"
  "    move.w %d0,(%a4)+\n"
  "    addq.w #1,20(%a6)\n"
  "    move.w #0xE780,(%a4)+\n"               /* 0xE000 | CONE_FULL (1920) */
  "    move.w %d4,%d0\n"
  "    lsl.w #3,%d0\n"
  "    add.w #128,%d0\n"
  "    move.w %d0,(%a4)+\n"
  "    add.w %d3,%d4\n"
  "    bra 10b\n"
  "20: move.w cone_fc0,%d3\n"
  "    subq.w #1,%d3\n"
  "    cmp.w %d5,%d3\n"
  "    ble 23f\n"
  "21: move.w %d5,%d3\n"
  "23: sub.w %d4,%d3\n"
  "    addq.w #1,%d3\n"
  "    cmp.w #4,%d3\n"
  "    ble 11f\n"
  "    moveq #4,%d3\n"
  "11: move.w 16(%a6),%d0\n"
  "    add.w %d3,%d0\n"
  "    cmp.w 18(%a6),%d0\n"
  "    bgt 30f\n"
  "    cmp.w 26(%a6),%d0\n"
  "    bgt 40f\n"                            /* ya armó lo que podía en esta llamada: sigue en la próxima */
  "    move.w 22(%a6),(%a4)+\n"               /* y */
  "    move.w %d3,%d0\n"
  "    subq.w #1,%d0\n"
  "    lsl.w #8,%d0\n"
  "    lsl.w #2,%d0\n"
  "    or.w 20(%a6),%d0\n"
  "    move.w %d0,(%a4)+\n"
  "    addq.w #1,20(%a6)\n"
  "    move.w 16(%a6),%d0\n"
  "    add.w #0xE7E0,%d0\n"                   /* 0xE000 | CONE_DYN (2016) */
  "    move.w %d0,(%a4)+\n"
  "    move.w %d4,%d0\n"
  "    lsl.w #3,%d0\n"
  "    add.w #128,%d0\n"
  "    move.w %d0,(%a4)+\n"
  "    add.w %d3,16(%a6)\n"
  "    move.w %d3,%d7\n"
  "    subq.w #1,%d7\n"
  "    move.w %d4,%d2\n"
  "    lsl.w #4,%d2\n"                        /* 2 * x0 */
  "12: moveq #7,%d6\n"
  "    movea.l %a2,%a0\n"
  "13: move.w (%a0),%d0\n"                   /* 4 * (9a + b + 1) con kBuildIdx (a y b recortados) */
  "    sub.w %d2,%d0\n"
  "    move.w (%a5,%d0.w),%d1\n"
  "    move.w 16(%a0),%d0\n"
  "    sub.w %d2,%d0\n"
  "    add.w (%a5,%d0.w),%d1\n"
  "    move.l (%a3,%d1.w),(%a1)+\n"
  "    addq.l #2,%a0\n"
  "    dbra %d6,13b\n"
  "    add.w #16,%d2\n"
  "    dbra %d7,12b\n"
  "    add.w %d3,%d4\n"
  "    bra 10b\n"
  /* sin tiles para armar: esas columnas llenas si el centro (x + 4) está dentro del haz en la línea del medio de la
     banda (el borde de a 8 píxeles, en vez de cortar el haz) */
  "30: move.w 8(%a2),%d0\n"                  /* izquierda de la línea 4 (2 * x) */
  "    asr.w #1,%d0\n"
  "    addq.w #3,%d0\n"
  "    asr.w #3,%d0\n"                        /* primera columna con el centro dentro */
  "    cmp.w %d4,%d0\n"
  "    bge 31f\n"
  "    move.w %d4,%d0\n"
  "31: move.w 24(%a2),%d1\n"                 /* derecha de la línea 4 (2 * x + 1536) */
  "    sub.w #1536,%d1\n"
  "    asr.w #1,%d1\n"
  "    subq.w #4,%d1\n"
  "    asr.w #3,%d1\n"                        /* última */
  "    move.w %d4,%d2\n"
  "    add.w %d3,%d2\n"
  "    subq.w #1,%d2\n"
  "    cmp.w %d2,%d1\n"
  "    ble 32f\n"
  "    move.w %d2,%d1\n"
  "32: add.w %d3,%d4\n"                      /* la corrida sigue después de estas columnas */
  "33: cmp.w %d1,%d0\n"
  "    bgt 10b\n"
  "    cmpa.l 12(%a6),%a4\n"
  "    bcc 97f\n"
  "    move.w %d1,%d3\n"
  "    sub.w %d0,%d3\n"
  "    addq.w #1,%d3\n"
  "    cmp.w #4,%d3\n"
  "    ble 34f\n"
  "    moveq #4,%d3\n"
  "34: move.w 22(%a6),(%a4)+\n"
  "    move.w %d3,%d2\n"
  "    subq.w #1,%d2\n"
  "    lsl.w #8,%d2\n"
  "    lsl.w #2,%d2\n"
  "    or.w 20(%a6),%d2\n"
  "    move.w %d2,(%a4)+\n"
  "    addq.w #1,20(%a6)\n"
  "    move.w #0xE780,(%a4)+\n"
  "    move.w %d0,%d2\n"
  "    lsl.w #3,%d2\n"
  "    add.w #128,%d2\n"
  "    move.w %d2,(%a4)+\n"
  "    add.w %d3,%d0\n"
  "    bra 33b\n"
  "40: move.w %d4,24(%a6)\n"
  "    bra 98f\n"
  "96: move.w #-1,24(%a6)\n"
  "    bra 99f\n"
  "97: move.w #-1,24(%a6)\n"
  "98: move.l %a1,4(%a6)\n"
  "    move.l %a4,8(%a6)\n"
  "99: movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
  "    rts\n");

/* cone_copy(dst, src, n): n sprites (8 bytes cada uno) de src a dst, de a 6 con movem */
void cone_copy(void *dst, const void *src, uint16 n);
__asm__(
  "    .text\n"
  "    .global cone_copy\n"
  "cone_copy:\n"
  "    movem.l %d2-%d7/%a2-%a6,-(%sp)\n"
  "    movea.l 48(%sp),%a1\n"
  "    movea.l 52(%sp),%a0\n"
  "    move.w 58(%sp),%d0\n"
  "1:  cmp.w #6,%d0\n"
  "    blt 2f\n"
  "    movem.l (%a0)+,%d1-%d7/%a2-%a6\n"      /* 12 largos = 6 sprites */
  "    movem.l %d1-%d7/%a2-%a6,(%a1)\n"
  "    lea 48(%a1),%a1\n"
  "    subq.w #6,%d0\n"
  "    bra 1b\n"
  "2:  subq.w #1,%d0\n"
  "    bmi 4f\n"
  "3:  move.l (%a0)+,(%a1)+\n"
  "    move.l (%a0)+,(%a1)+\n"
  "    dbra %d0,3b\n"
  "4:  movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
  "    rts\n");

/* cone_copy_off(dst, src, n, dx, dy, link0): n sprites corridos (dx, dy) y con los enlaces desde link0 */
void cone_copy_off(void *dst, const void *src, uint16 n, int16 dx, int16 dy, uint16 link0);
__asm__(
  "    .text\n"
  "    .global cone_copy_off\n"
  "cone_copy_off:\n"
  "    movem.l %d2-%d5,-(%sp)\n"
  "    movea.l 20(%sp),%a1\n"
  "    movea.l 24(%sp),%a0\n"
  "    move.w 30(%sp),%d0\n"                  /* n */
  "    move.w 34(%sp),%d2\n"                  /* dx */
  "    move.w 38(%sp),%d3\n"                  /* dy */
  "    move.w 42(%sp),%d4\n"                  /* enlace */
  "    subq.w #1,%d0\n"
  "    bmi 2f\n"
  "1:  move.w (%a0)+,%d5\n"
  "    add.w %d3,%d5\n"
  "    move.w %d5,(%a1)+\n"                   /* y */
  "    move.w (%a0)+,%d5\n"
  "    and.w #0xFF80,%d5\n"
  "    or.w %d4,%d5\n"
  "    move.w %d5,(%a1)+\n"
  "    addq.w #1,%d4\n"
  "    move.w (%a0)+,(%a1)+\n"                /* atributo */
  "    move.w (%a0)+,%d5\n"
  "    add.w %d2,%d5\n"
  "    move.w %d5,(%a1)+\n"                   /* x */
  "    dbra %d0,1b\n"
  "2:  movem.l (%sp)+,%d2-%d5\n"
  "    rts\n");

/* cone_move(dst, n, ddx, ddy): corre n sprites ya armados (sumar a x e y en su lugar; si no cambia, nada) */
void cone_move(void *dst, uint16 n, int16 ddx, int16 ddy);
__asm__(
  "    .text\n"
  "    .global cone_move\n"
  "cone_move:\n"
  "    movea.l 4(%sp),%a0\n"
  "    move.w 10(%sp),%d0\n"
  "    move.w 14(%sp),%d1\n"                  /* ddx */
  "    subq.w #1,%d0\n"
  "    bmi 9f\n"
  "    tst.w %d1\n"
  "    beq 2f\n"
  "    movea.l %a0,%a1\n"
  "    addq.l #6,%a1\n"
  "1:  add.w %d1,(%a1)\n"
  "    addq.l #8,%a1\n"
  "    dbra %d0,1b\n"
  "    move.w 10(%sp),%d0\n"
  "    subq.w #1,%d0\n"
  "2:  move.w 18(%sp),%d1\n"                  /* ddy */
  "    beq 9f\n"
  "3:  add.w %d1,(%a0)\n"
  "    addq.l #8,%a0\n"
  "    dbra %d0,3b\n"
  "9:  rts\n");

/* md_post: los sprites del haz al final de la lista */
uint8 md_spr_bank(const GenSpr *p);     /* md_engine.c: 0 o 1, cuál de las dos listas es */

/* un paso del armado (unas líneas de la tabla o unas bandas de sprites); 1 cuando terminó */
static uint8 job_step(uint8 small) {
  if (job == 1) {
    if (xray_job_run(small ? CONE_TAB_SMALL : CONE_TAB_STEP)) {
#ifdef MD_TABCHK
      { extern void beam_job_check(void); beam_job_check(); }
#endif
      job = 2;
      cache_b = (cache == cache_a) ? CACHE_B : cache_a;
      jca.tp = (const uint8 *)(&ERAM(0x9100) + CONE_TOP - 2);
      jca.o = cache_b;
      jca.omax = cache_b + 2 * (CONE_SPR_MAX - 6);
      jca.link = 1;
      jca.y = 128 + CONE_TOP;
      jtb = cone_buf;
      jndyn = 0;
      jcol = -1;
    }
    return 0;
  }
  if (job != 2) return job == 3;
  if (cone_ndyn) return 0;                     /* el vblank todavía no copió los tiles del haz anterior */
  { uint16 st = small ? CONE_BAND_SMALL : CONE_BAND_STEP;
    cone_ymax = jca.y + st < 352 ? jca.y + st : 352; }
  if (jcol < 0) {
    jca.fb = jfby;
    cone_bands(&jca);
  }
  if (jcol >= 0 || jca.fb != jfby) {           /* una banda que no son dos bordes rectos: línea por línea */
    BuildArgs ba;
    ba.tp = (const uint8 *)(&ERAM(0x9100) + CONE_TOP - 2) + 2 * (jfby[0] - 128 - CONE_TOP);
    ba.tb = jtb; ba.o = jca.o; ba.omax = jca.omax;
    ba.ndyn = jndyn; ba.ndyn_max = CONE_DYN_MAX; ba.link = jca.link; ba.y = jfby[0];
    ba.col0 = jcol;
    ba.cap = jndyn + (small ? CONE_DYN_SMALL : CONE_DYN_STEP);   /* tiles armados por paso (cada uno cuesta) */
    cone_build(&ba);
    jcol = ba.col0;                            /* >= 0: la banda sigue en el próximo paso */
    jtb = ba.tb; jndyn = ba.ndyn;
    jca.o = ba.o;
    jca.link = ba.link;
    if (jcol >= 0) return 0;
  }
  if (jca.y >= 128 + 224 || jca.o >= jca.omax) {
    jn = (uint16)((jca.o - cache_b) >> 1);
    job = 3;
    return 1;
  }
  return 0;
}

/* el tiempo libre del cuadro (md_main.c y md_engine.c, mientras esperan al vblank) */
static uint8 idle_ran;                  /* este cuadro hubo tiempo libre */
void md_cone_idle(void) {
  idle_ran = 1;
  if (job == 1 || job == 2) job_step(0);
}

uint16 md_cone_frame(void) {
  cone_on = 0;
  cone_ndyn = 0;
  uint8 eye = zebes_eye_room();
#ifndef CONE_NOSPRFIX
  if (eye) eye_room_sprites();
#endif
  if (!eye || fx_layer_blending_config_c != 16 || !zebes_cone_slot(CONE_COLOR)) return 0;
  uint16 k = reg_COLDATA[0] & 31;          /* la intensidad del SNES (R = G) */
  if (k < 4 || (k < 8 && (nmi_frame_counter_word & 1))) return 0;
#ifdef CONE_NODRAW
  return 0;
#endif
  if (!rows_ok) make_rows();
  uint16 nfree = (uint16)(spr + MAX_SPR - sp_ptr);
  if (nfree > CONE_SPR_MAX) nfree = CONE_SPR_MAX;
  if (nfree < 8) return 0;
  /* El haz solo cambia de forma si cambia su ángulo o su ancho; si solo se mueve el ojo en la pantalla (la cámara)
     es la misma forma corrida: se muestran los sprites armados corridos. */
  uint16 p[4];
  beam_params(p);                             /* x, y del ojo en la pantalla, ángulo, ancho */
  if (!cache_ok) {                            /* nada que mostrar todavía: todo en este cuadro */
    beam_job_start();
    for (uint16 i = 0; i < 4; i++) jpar[i] = p[i];
    job = 1;
    while (!job_step(0)) {}
  }
  if (job == 3) {                             /* el haz nuevo está armado: se muestra desde este cuadro */
    uint32 *t = cache; cache = cache_b; cache_b = t;
    cache_n = jn;
    cone_ndyn = jndyn;                          /* sus tiles: los copia el vblank */
    for (uint16 i = 0; i < 4; i++) cpar[i] = jpar[i];
    cache_serial++;
    cache_ok = 1;
    job = 0;

  }
  int16 dx = (int16)(p[0] - cpar[0]), dy = (int16)(p[1] - cpar[1]);
  /* el ángulo: se rearma si cambió 2 o más (1/128 de vuelta); un paso de 1/256 no se ve */
  int8 da = (int8)(uint8)(p[2] - cpar[2]);
  uint8 same = da > -2 && da < 2 && p[3] == cpar[3] && dx > -24 && dx < 24 && dy > -24 && dy < 24;
  if (!same && !job) {                        /* empieza a armarse el nuevo con estos valores */
    beam_job_start();
    for (uint16 i = 0; i < 4; i++) jpar[i] = p[i];
    job = 1;
  }
  uint16 n = cache_n < nfree ? cache_n : nfree;
  /* El haz va en los últimos lugares de la tabla de sprites del VDP (64 - n a 63), con sus propios enlaces: el último
     de la lista del juego enlaza con el primero del haz (md_engine.c, vblank). Así sus sprites se escriben en su
     propio búfer (uno por cada lista de sprites: el vblank manda la anterior mientras la lógica arma la nueva) y,
     si no cambió el haz, solo se corren x e y. */
  uint8 b = md_spr_bank(spr);
  BankSig *bs = &bank_sig[b];
  if (bs->serial != cache_serial || bs->n != n) {
    cone_copy_off(cone_sat[b], cache, n, dx, dy, MAX_SPR - n + 1);
    if (n) cone_sat[b][n - 1].size_link &= 0x0F00;
  } else if (bs->dx != dx || bs->dy != dy) {
    cone_move(cone_sat[b], n, dx - bs->dx, dy - bs->dy);
  }
  bs->serial = cache_serial; bs->n = n; bs->dx = dx; bs->dy = dy;
  if (n) cone_on = 1;
#ifndef CONE_NOFORCE
  /* un paso corto por cuadro aunque no sobre tiempo (el haz no se queda atrás); si el cuadro no tuvo nada de tiempo
     libre (md_cone_idle no corrió: va justo), uno cada dos */
  static uint8 tight;
  if ((job == 1 || job == 2) && (idle_ran || !tight || !CONE_THROTTLE)) { job_step(1); tight = !idle_ran; }
  else tight = 0;
  idle_ran = 0;
#endif
  return n;
}

/* vblank: dónde están los sprites del haz de la lista bank (0 o 1) */
const void *md_cone_sat(uint8 bank) { return cone_sat[bank]; }

/* md_post, después de los colores de los objetos (sin nada que hacer: el modo sombra/luz no usa colores) */
void md_cone_color(void) {}

/* vblank: los tiles llenos (una vez por sala), los armados de este cuadro y el modo sombra/luz */
void md_cone_vblank(void) {
  if (cone_on && !cone_fixed_ok) {             /* llenos y bordes rectos: una vez por sala */
    uint32 t[8];
    vdp_set_vram_write(CONE_FULL * 32);
    for (uint16 i = 0; i < 4 * 16; i++) VDP_DATA = 0xEEEE;
    for (uint16 a = 0; a < 8; a++)
      for (uint16 d = 0; d < 8; d++)
        for (uint16 col = 0; col < ((a + d > 7) ? 2 : 1); col++) {
          edge_rows(a, d, col, t);
          for (uint16 i = 0; i < 8; i++) { VDP_DATA = (uint16)(t[i] >> 16); VDP_DATA = (uint16)t[i]; }
        }
    cone_fixed_ok = 1;
  }
  if (cone_on && cone_ndyn && cone_fixed_ok) vdp_dma_vram(cone_buf, CONE_DYN * 32, cone_ndyn * 16);
  uint8 want = zebes_eye_room();          /* en las salas con el ojo, siempre (todo tiene prioridad alta) */
  if (want != sh_on) { vdp_reg(0x0C, want ? 0x08 : 0x00); sh_on = want; }
  cone_on_shown = cone_on;
}

/* la VRAM de 0xF000 se vuelve a llenar al cambiar de sala (la pueden usar las cinemáticas) */
void md_cone_room(void) {
  cone_fixed_ok = 0; cache_ok = 0; job = 0;
  bank_sig[0].serial = bank_sig[1].serial = 0;
}
