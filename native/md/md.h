/* MegaMetroid — capa de la Genesis del motor nativo (docs/32).
 *
 * El motor (native/src) corre un cuadro del juego y deja su estado en variables; lo que dibuja lo pide por las
 * funciones de dibujo del decomp (DrawSamusSpritemap, DrawEnemySpritemap...). Esta capa las implementa con
 * datos ya preparados en el PC (native/tools/mkmd.py -> md/gen): no hay nada del SNES que convertir aquí.
 *
 *   md_init(intro)   VDP, tiles fijos (HUD), estado inicial del motor (Ceres o la intro), primera sala
 *   md_logic(joy)    la lógica de un cuadro del motor (puede correr mientras el vblank toma el cuadro anterior)
 *   md_post()        (con el vblank libre) preparación de lo que va al VDP (sprites, CRAM, HUD, tiles de Samus)
 *   md_publish()     el cuadro queda listo: lo escribe en el VDP la interrupción vertical (md_vint -> md_vblank)
 * Cuadros en cadena (docs/39): la lógica del cuadro siguiente empieza en cuanto se publica el anterior, sin esperar
 * el vblank; un cuadro largo usa el tiempo que le sobró al anterior, como en el SNES no se pierde la imagen.
 *
 * Mapa de la VRAM (bytes): 0x0000-0x9FFF tiles | 0xA000 plano A (8 KB) | 0xC000 plano B (8 KB) |
 * 0xE000 Window (HUD) | 0xE800 sprites | 0xEC00 scroll horizontal. Modo H32 (256x224) como el SNES.
 */
#ifndef MD_H
#define MD_H
#include <stdint.h>

#define MD_VRAM_PLANE_A 0xA000
#define MD_VRAM_PLANE_B 0xC000
#define MD_VRAM_WINDOW  0xE000
#define MD_VRAM_SPRITES 0xE800
#define MD_VRAM_HSCROLL 0xEC00

void md_init(uint8_t intro);   /* intro: estado inicial de la intro (si no, el de Ceres) */
void md_logic(uint16_t snes_joy);
void md_post(void);
void md_publish(void);
void md_wait_free(void);         /* espera a que el vblank tome el cuadro publicado */
void md_vblank(uint16_t late);   /* late: el vblank empezó hace rato (cuadro largo): menos DMA */

/* para el arnés del emulador (se leen de la RAM) */
extern volatile uint16_t md_frame_count;       /* cuadros del motor */
extern volatile uint16_t md_missing;           /* dibujos pedidos sin datos preparados */
extern volatile uint16_t md_missing_key[4];
extern volatile uint16_t md_unimpl_count;
extern const char *volatile md_unimpl_msg;

/* botones del SNES (orden de JOY1) */
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
#endif
