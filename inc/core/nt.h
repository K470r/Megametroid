/* MegaMetroid — motor nativo para la Genesis (docs/31).
 * Tipos y utilidades comunes. El código del motor se escribe a mano a partir del decomp de
 * Super Metroid (snesrev/sm, la "biblia"), con variables nativas del 68000 y tablas ya preparadas
 * en el PC (tools/conv). No hay registros, RAM ni ROM del SNES en tiempo de ejecución.
 * Compila igual en el PC (comprobación contra el SNES, host) y en la Genesis (src/platform). */
#ifndef NT_H
#define NT_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;

#define sign16(x) (((uint16)(x) & 0x8000) != 0)
#define sign8(x) (((uint8)(x) & 0x80) != 0)
#ifndef NT_POINT16U
#define NT_POINT16U
typedef struct Point16U { uint16 x, y; } Point16U;
#endif
#define LO8(x) ((uint8)(x))
#define HI8(x) ((uint8)((uint16)(x) >> 8))
#define PAIR32(hi, lo) ((uint32)(((uint32)(uint16)(hi) << 16) | (uint16)(lo)))
#define IPAIR32(hi, lo) ((int32)PAIR32(hi, lo))
#define SHL16(x) ((int32)((uint32)(int32)(int16)(x) << 16))

/* hi:lo += v (aritmética de 32 bits partida en dos palabras, como el 65816) */
static inline void add_hilo(uint16 *hi, uint16 *lo, int32 v) {
  uint32 t = PAIR32(*hi, *lo) + (uint32)v;
  *hi = (uint16)(t >> 16); *lo = (uint16)t;
}
static inline void set_hilo(uint16 *hi, uint16 *lo, int32 v) {
  *hi = (uint16)((uint32)v >> 16); *lo = (uint16)v;
}

/* botones del SNES (mismo orden de bits que JOY1: la Genesis los traduce al leer el mando) */
enum {
  kButton_B = 0x8000, kButton_Y = 0x4000, kButton_Select = 0x2000, kButton_Start = 0x1000,
  kButton_Up = 0x800, kButton_Down = 0x400, kButton_Left = 0x200, kButton_Right = 0x100,
  kButton_A = 0x80, kButton_X = 0x40, kButton_L = 0x20, kButton_R = 0x10,
};

/* aviso para la capa de la Genesis: palette_buffer cambió (subir colores en el próximo vblank). En el SNES
   el NMI copiaba la paleta entera en cada cuadro; aquí solo cuando hace falta. */
extern uint8 nt_palette_changed;
extern uint8 nt_palette_lo, nt_palette_hi;      /* índices de palette_buffer que cambiaron desde el último aviso */
static inline void nt_pal_dirty(uint8 lo, uint8 hi) {
  if (!nt_palette_changed) { nt_palette_lo = lo; nt_palette_hi = hi; }
  else { if (lo < nt_palette_lo) nt_palette_lo = lo; if (hi > nt_palette_hi) nt_palette_hi = hi; }
  nt_palette_changed = 1;
}
/* Genesis (NT_FADE_LIST): índices de palette_buffer que el fundido calcula cuadro a cuadro (los que se ven) */
extern const uint8 *nt_fade_list;
/* temblor de la sala en este cuadro (HandleRoomShaking): lo que el SNES suma a BG1/BG2 HOFS/VOFS (la Genesis lo
   suma a sus planos) */
extern int16 nt_shake_bg1x, nt_shake_bg1y, nt_shake_bg2x, nt_shake_bg2y;
extern uint16 nt_fade_count;

/* código y datos agregados después de aprobar la regresión de imagen: al final de la parte fija de la ROM (md_ssf2.ld),
   así no corren las direcciones de lo que ya estaba (docs/53) */
#ifdef NT_SSF2
#define NT_LATE __attribute__((section(".late_text"), noinline))
#define NT_LATE_DATA __attribute__((section(".late_rodata")))
#else
#define NT_LATE
#define NT_LATE_DATA
#endif
#include "vars.h"
/* memcpy(g_ram + dirección, RomPtr_XX(src), n) del decomp (p. ej. Dachora_Func_1: su paleta): las paletas de
   7E:C000-C3FF (palette_buffer y target_palettes) desde la ROM, palabra por palabra */
void nt_unimpl(const char *what);
static inline void nt_ram_copy_rom(uint16 dst, uint16 (*bw)(uint16), uint16 src, uint16 n) {
  uint16 *d;
  if (dst >= 0xC000 && dst < 0xC200) d = &palette_buffer[(dst - 0xC000) >> 1];
  else if (dst >= 0xC200 && dst < 0xC400) d = &target_palettes[(dst - 0xC200) >> 1];
  else { nt_unimpl("nt_ram_copy_rom (dirección de la RAM)"); return; }
  for (uint16 i = 0; i < n; i += 2) *d++ = bw((uint16)(src + i));
  if (dst < 0xC200) nt_pal_dirty((uint8)((dst - 0xC000) >> 1), (uint8)(((dst - 0xC000) + n - 2) >> 1));
}
/* producto de 16 bits (los 16 bits bajos, iguales con o sin signo): un mulu.w del 68000; en C, uint16 * uint16 se
   promueve a int y gcc llama a __mulsi3 (una multiplicación de 32 bits por software) */
static inline uint16 Mul16(uint16 a, uint16 b) {
#ifdef __mc68000__
  __asm__("mulu.w %1,%0" : "+d"(a) : "d"(b));
  return a;
#else
  return (uint16)(a * b);
#endif
}
/* división sin signo de 16 bits (b != 0): un divu.w; cociente en los 16 bits bajos y resto en los altos (en C,
   uint16 / uint16 se promueve a int y gcc llama a __divsi3 / __modsi3) */
static inline uint32 DivMod16(uint16 a, uint16 b) {
#ifdef __mc68000__
  uint32 q = a;
  __asm__("divu.w %1,%0" : "+d"(q) : "d"(b));
  return q;
#else
  return (uint32)(a / b) | (uint32)(a % b) << 16;
#endif
}
#endif
