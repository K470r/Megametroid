/* MegaMetroid — ROM de la Genesis con el motor nativo (docs/32): la estación Ceres jugable.
 *
 * Secuencia: título y menú de partidas (front/, docs/36) -> el juego con el motor (native/src) y su capa de la
 * Genesis (md_engine.c). En el juego SGDK solo espera el vblank y lee el mando.
 *
 * Mando: los botones en el lugar de los del SNES (Edwin, 2026-10-21).
 *   6 botones:  X cancelar ítem (Y del SNES)  Y disparar (X)  Z apuntar arriba (R)
 *               A correr (B)                  B saltar (A)    C apuntar abajo (L)
 *               MODE elegir ítem (SELECT)     START pausa/mapa (START)
 *   3 botones:  A correr (B)  B disparar (X)  C saltar (A)
 *               START corto = elegir ítem (SELECT; dando la vuelta se llega a "nada" = cancelar),
 *               START mantenido = pausa/mapa (START); sin apuntar en diagonal quieto (sí caminando o corriendo)
 * Con MD_TEST_INPUT la entrada sale de la grabación del SNES (para comparar cuadro a cuadro). */
#include <genesis.h>
#include "front/title.h"
#include "front/menu.h"
/* md.h usa <stdint.h>, que choca con los tipos de SGDK: aquí van las declaraciones con tipos de SGDK */
void md_init(uint8_t intro);
void md_logic(u16 snes_joy);
void md_post(void);
void md_publish(void);
void md_wait_free(void);
void md_hint(void);
void md_vint(void);
#define SNES_B 0x8000
#define SNES_Y 0x4000
#define SNES_SELECT 0x2000
#define SNES_START 0x1000
#define SNES_UP 0x0800
#define SNES_DOWN 0x0400
#define SNES_LEFT 0x0200
#define SNES_RIGHT 0x0100
#define SNES_A 0x0080
#define SNES_X 0x0040
#define SNES_L 0x0020
#define SNES_R 0x0010

#ifdef MD_TEST_INPUT
extern const u16 kTestInput[];
extern const u16 kTestInputLen;
static u16 test_pos;
#endif

static bool six;
#define START_HOLD 20                           /* mando de 3 botones: cuadros de START mantenido para la pausa */
static u16 start_frames;

static u16 read_joy(void) {
#ifdef MD_TEST_INPUT
  if (test_pos < kTestInputLen) return kTestInput[test_pos++];
#endif
  u16 pad = JOY_readJoypad(JOY_1);
  u16 s = 0;
  six = JOY_getJoypadType(JOY_1) == JOY_TYPE_PAD6;   /* cada cuadro: el mando puede cambiar */
  if (pad & BUTTON_UP) s |= SNES_UP;
  if (pad & BUTTON_DOWN) s |= SNES_DOWN;
  if (pad & BUTTON_LEFT) s |= SNES_LEFT;
  if (pad & BUTTON_RIGHT) s |= SNES_RIGHT;
  if (six) {
    if (pad & BUTTON_B) s |= SNES_A;
    if (pad & BUTTON_A) s |= SNES_B;
    if (pad & BUTTON_Y) s |= SNES_X;
    if (pad & BUTTON_X) s |= SNES_Y;
    if (pad & BUTTON_C) s |= SNES_L;
    if (pad & BUTTON_Z) s |= SNES_R;
    if (pad & BUTTON_MODE) s |= SNES_SELECT;
    if (pad & BUTTON_START) s |= SNES_START;
    start_frames = 0;
  } else {
    if (pad & BUTTON_C) s |= SNES_A;
    if (pad & BUTTON_A) s |= SNES_B;
    if (pad & BUTTON_B) s |= SNES_X;
    /* START: toque corto = SELECT (un cuadro, al soltar); mantenido START_HOLD cuadros = START */
    if (pad & BUTTON_START) {
      if (start_frames < 0xFFFF) start_frames++;
      if (start_frames >= START_HOLD) s |= SNES_START;
    } else {
      if (start_frames && start_frames < START_HOLD) s |= SNES_SELECT;
      start_frames = 0;
    }
  }
  return s;
}

volatile u16 md_busy;
/* líneas desde el encendido (vtimer sube en la línea 224; el contador del VDP repite 0xE5-0xEA: se toma
   la segunda vuelta) */
u32 now_lines(void) {
  u16 sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  u32 t = vtimer;
  u16 st = *(vu16 *)0xC00004;
  u16 vc = *(vu16 *)0xC00008 >> 8;
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
  u16 line = vc < 0xE0 ? vc + 262 : vc < 0xE5 ? vc : vc + 6;   /* 224..261, 262..485 */
  if (line < 262 && (st & 0x80)) t++;            /* la interrupción vertical todavía no se atendió */
  return t * 262 + line - 224;
}

/* Cuándo empieza la lógica del cuadro siguiente (docs/39). Si el cuadro anterior fue liviano, se espera a que el
   vblank lo tome (o a que falten PIPE_TS líneas para el vblank): la entrada se lee justo antes, como en el SNES
   (un cuadro de demora). Si fue pesado (más de PIPE_H líneas), la lógica empieza ya, con lo que le sobra al cuadro
   publicado: el cuadro siguiente tiene más tiempo y no se pierde la imagen (la entrada se lee un poco antes). */
#ifndef PIPE_H
#define PIPE_H 200
#endif
#ifndef PIPE_TS
#define PIPE_TS 40
#endif
static void pipe_wait(u16 cost) {
  extern volatile u8 md_pub;
  if (cost > PIPE_H) return;
  extern void md_cone_idle(void);
  while (md_pub) {
    md_cone_idle();                               /* lo que sobra del cuadro: el armado del haz (md_cone.c) */
    u16 vc = *(vu16 *)0xC00008 >> 8;
    if (vc < 0xE0 && vc >= 224 - PIPE_TS) break;
  }
}

/* Las variables del juego como al encender (md_ssf2.ld): para empezar otra partida sin reiniciar la consola.
   Las de SGDK (mando, vtimer, interrupciones) y las de las pantallas de antes del juego no se tocan. */
extern u8 _game_data_start[], _game_data_end[], _game_bss_start[], _game_bss_end[];
extern const u8 _game_data_rom[];
static void map_normal_page(void) {             /* la ventana 7 del SSF2 con su página de siempre: allí están los datos
                                                   del título y la copia de las variables (la partida deja otra puesta) */
  __asm__ volatile("" ::: "memory");
  *(volatile u8 *)0xA130FF = 7;
  __asm__ volatile("" ::: "memory");
}
static void reset_game_vars(void) {
  map_normal_page();
  memcpy(_game_data_start, _game_data_rom, _game_data_end - _game_data_start);
  memset(_game_bss_start, 0, _game_bss_end - _game_bss_start);
}

/* Una partida: desde la intro (intro = 1) o desde Ceres hasta que el juego termina. Devuelve 1 para seguir en Ceres
   (la intro terminó, o YES en el fin de juego) y 0 para ir al título (NO). */
static u16 play_game(u16 intro) {
  SYS_disableInts();
  reset_game_vars();
  six = JOY_getJoypadType(JOY_1) == JOY_TYPE_PAD6;
  md_init(intro);
  { extern uint8_t nt_viewer, nt_viewer_place, nt_viewer_enemies; nt_viewer = 1; nt_viewer_place = 1; nt_viewer_enemies = 1; }   /* Zebes: visor de salas (src/viewer.c) */
  SYS_setHIntCallback(md_hint);
  SYS_setVIntCallback(md_vint);
  SYS_enableInts();
  u16 cost = 0;
  for (;;) {
    pipe_wait(cost);
    JOY_update();
    md_busy = 1;
#ifdef MD_STATESUM
    { extern void md_state_sum(void); md_state_sum(); }
#endif
    u32 t0 = now_lines();                         /* líneas de la consola que usa el cuadro (sin las esperas) */
    md_logic(read_joy());
    u32 t1 = now_lines();
    { extern uint16_t game_state;                 /* fin de juego: 31 = YES (otra vez Ceres), 0xFFFF = NO (al título) */
      if (game_state == 31 || game_state == 0xFFFF) {
        u16 again = game_state == 31;
        md_wait_free();
        SYS_disableInts();
        SYS_setVIntCallback(NULL);
        SYS_setHIntCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_enableInts();
        return again;
      } }
    md_wait_free();                               /* el vblank tomó el cuadro anterior */
    u32 t2 = now_lines();
    md_post();
    u32 t3 = now_lines();
    cost = (u16)(t1 - t0 + t3 - t2);
#ifdef MD_BUSYLOG                                  /* medición: lógica y segunda parte de cada cuadro */
    { extern volatile u8 *const md_busy_log, *const md_post_log;
      extern volatile u16 md_frame_count;
      u16 k = (u16)(md_frame_count - 1 - MD_BUSYLOG);
      if (k < 304) { u16 a = t1 - t0, b = t3 - t2; md_busy_log[k] = a > 255 ? 255 : a; md_post_log[k] = b > 255 ? 255 : b; } }
#endif
    md_busy = 0;                                  /* para native_compare: fuera del cuadro las variables están completas */
    md_publish();
#ifdef MD_STOP_AT
    { extern volatile u16 md_frame_count; if (md_frame_count == MD_STOP_AT) for (;;) SYS_doVBlankProcess(); }
#endif
  }
}

/* Título -> partidas y opciones (SALIR vuelve al título) -> juego. Con MD_TEST_INPUT (comparación cuadro a cuadro
   con la grabación del SNES) o MD_NO_TITLE se empieza directo en el juego. */
static void front_screens(void) {
  map_normal_page();
  VDP_init();                                     /* el juego deja el VDP a su manera: SGDK lo vuelve a armar */
  for (;;) {
    title_run();
    if (menu_run() == MENU_START) break;
  }
}

int main(bool hard) {
  (void)hard;
  { extern void md_snd_init(void); md_snd_init(); }
  JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);
#if !defined(MD_TEST_INPUT) && !defined(MD_NO_TITLE)
  front_screens();
#endif
#if (!defined(MD_TEST_INPUT) && !defined(MD_NO_TITLE)) || defined(MD_TEST_INTRO)
  u16 intro = 1;                                  /* partida nueva: la intro y después Ceres */
#else
  u16 intro = 0;
#endif
  for (;;) {
    while (play_game(intro)) intro = 0;           /* fin de la intro o YES: otra vez desde Ceres */
    front_screens();                              /* NO: al título */
    intro = 1;
  }
  return 0;
}
