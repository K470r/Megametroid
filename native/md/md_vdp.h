/* MegaMetroid — acceso directo al VDP de la Genesis para la capa del motor nativo (docs/32). */
#ifndef MD_VDP_H
#define MD_VDP_H
#include <stdint.h>
#include "md.h"

#define VDP_DATA (*(volatile uint16_t *)0xC00000)
#define VDP_CTRL (*(volatile uint16_t *)0xC00004)
#define VDP_CTRL32 (*(volatile uint32_t *)0xC00004)
#define VDP_VCOUNT (*(volatile uint16_t *)0xC00008)   /* contador H/V: byte alto = línea */

#define VDP_CMD_VRAM_W(a) (0x40000000u | (((uint32_t)(a) & 0x3FFF) << 16) | ((uint32_t)(a) >> 14))
#define VDP_CMD_CRAM_W(a) (0xC0000000u | (((uint32_t)(a) & 0x3FFF) << 16) | ((uint32_t)(a) >> 14))
#define VDP_CMD_VSRAM_W(a) (0x40000010u | (((uint32_t)(a) & 0x3FFF) << 16) | ((uint32_t)(a) >> 14))

static uint8_t vdp_reg1 = 0x74;

static inline void vdp_reg(uint8_t r, uint8_t v) { VDP_CTRL = 0x8000 | ((uint16_t)r << 8) | v; }
static inline void vdp_set_vram_write(uint16_t a) { VDP_CTRL32 = VDP_CMD_VRAM_W(a); }
static inline void vdp_set_vsram_write(uint16_t a) { VDP_CTRL32 = VDP_CMD_VSRAM_W(a); }
static inline void vdp_display(int on) {
  vdp_reg1 = on ? (vdp_reg1 | 0x40) : (vdp_reg1 & ~0x40);
  vdp_reg(1, vdp_reg1);
}

/* DMA de la memoria del 68000 (ROM o RAM) a la VRAM: words palabras, en tramos que no crucen 128 KB */
static void vdp_dma_raw(uint32_t from, uint16_t to, uint16_t words) {
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
  /* la segunda palabra (la que arranca la DMA) se escribe desde la memoria, como hace SGDK */
  __asm__ volatile("move.w (%0),(%1)" : : "a"(&cmd[1]), "a"(pw) : "memory");
}

static void vdp_dma_vram(const void *src, uint16_t to, uint16_t words) {
  if (!words) return;
  uint32_t from = (uint32_t)src;
  uint32_t limit = (0x20000 - (from & 0x1FFFF)) >> 1;        /* palabras hasta el borde de 128 KB */
  if (words > limit) {
    vdp_dma_raw(from + limit * 2, to + limit * 2, words - limit);
    words = limit;
  }
  vdp_dma_raw(from, to, words);
}

/* lo mismo, para los tramos cortos: los registros de a dos (escrituras largas) y sin llamadas */
static inline __attribute__((always_inline)) void vdp_dma_lean(const void *src, uint16_t to, uint16_t words) {
  uint32_t f = (uint32_t)src >> 1;
  if ((f & 0xFFFF) + words > 0x10000) { vdp_dma_vram(src, to, words); return; }
  volatile uint32_t *c32 = (volatile uint32_t *)0xC00004;
  *c32 = 0x93009400u | ((uint32_t)(words & 0xFF) << 16) | (words >> 8);
  *c32 = 0x95009600u | ((f & 0xFF) << 16) | ((f >> 8) & 0xFF);
  VDP_CTRL = 0x9700 | ((f >> 16) & 0x7F);
  static volatile uint16_t lcmd;
  uint32_t c = VDP_CMD_VRAM_W(to) | 0x80;
  lcmd = (uint16_t)c;
  VDP_CTRL = c >> 16;
  __asm__ volatile("move.w (%0),(%1)" : : "a"(&lcmd), "a"(&VDP_CTRL) : "memory");
}

static void vdp_fill_vram(uint16_t to, uint16_t v, uint16_t words) {
  vdp_set_vram_write(to);
  for (; words; words--) VDP_DATA = v;
}

static void vdp_write_cram(const uint16_t *c, uint16_t n) {
  VDP_CTRL32 = VDP_CMD_CRAM_W(0);
  for (uint16_t i = 0; i < n; i++) VDP_DATA = c[i];
}

static void vdp_setup(void) {
  vdp_reg(0x00, 0x04);                       /* sin interrupción horizontal (por ahora) */
  vdp_reg1 = 0x74;                           /* pantalla encendida (SGDK espera el vblank leyendo el estado), VINT, DMA */
  vdp_reg(0x01, vdp_reg1);
  vdp_reg(0x02, MD_VRAM_PLANE_A >> 10);
  vdp_reg(0x03, MD_VRAM_WINDOW >> 10);
  vdp_reg(0x04, MD_VRAM_PLANE_B >> 13);
  vdp_reg(0x05, MD_VRAM_SPRITES >> 9);
  vdp_reg(0x07, 0x00);                       /* color de fondo: paleta 0, color 0 */
  vdp_reg(0x0B, 0x00);                       /* scroll de pantalla completa */
  vdp_reg(0x0C, 0x00);                       /* H32 */
  vdp_reg(0x0D, MD_VRAM_HSCROLL >> 10);
  vdp_reg(0x0F, 0x02);
  vdp_reg(0x10, 0x11);                       /* planos de 64x64 */
  vdp_reg(0x11, 0x00);
  vdp_reg(0x12, 0x04);                       /* Window: filas 0-3 (HUD) */
  vdp_fill_vram(0, 0, 0x8000);               /* toda la VRAM a cero */
  VDP_CTRL32 = VDP_CMD_CRAM_W(0);
  for (int i = 0; i < 64; i++) VDP_DATA = 0;
  VDP_CTRL32 = VDP_CMD_VSRAM_W(0);
  for (int i = 0; i < 40; i++) VDP_DATA = 0;
}
#endif
