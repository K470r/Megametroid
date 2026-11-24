/* MegaMetroid — la SRAM del cartucho (partidas guardadas y opciones; docs/65).
 *
 * 16 KB de direcciones con un byte en cada dirección impar (0x200001-0x203FFF: 8 KB de datos), como la RAM de
 * respaldo de los cartuchos de Sega (cabecera: "RA", 0xF820). Con el mapeador SSF2 la SRAM se pone en lugar de la
 * ROM desde 0x200000 al escribir 1 en $A130F1 (0: la ROM otra vez). Mientras está puesta no se ve nada de la ROM
 * desde los 2 MB (ni el código ni las ventanas): estas rutinas van al principio de la ROM (.text.keepboot, el
 * arranque) y sin interrupciones (el vblank corre código de más arriba). Los bytes 0-0x1FFF son la SRAM del SNES tal
 * cual (save.c); 0x2000 en adelante, lo de la Genesis (el idioma y las opciones del menú).
 *
 * El Z80 (el driver de sonido XGM2) lee la música y los efectos de la ROM por su cuenta: de la ventana 6 (0x300000) y
 * de la parte fija. En el MiSTer, con la SRAM puesta toda la zona desde 0x200000 es SRAM (en Genesis Plus GX solo
 * 0x200000-0x203FFF): si el Z80 leía en ese momento recibía basura y el sonido quedaba roto (al entrar al menú de
 * partidas, al guardar). Mientras la SRAM está puesta el Z80 no tiene el bus: se queda quieto un instante. */
#include <stdint.h>

#define MD_SRAM_CTRL (*(volatile uint8_t *)0xA130F1)
#define MD_SRAM ((volatile uint8_t *)0x200001)

__attribute__((section(".text.keepboot"), noinline))
void md_sram_copy(uint16_t off, uint8_t *buf, uint16_t n, uint8_t write) {
  uint16_t sr;
  __asm__ volatile("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d"(sr) : : "memory");
  volatile uint16_t *bus = (volatile uint16_t *)0xA11100;
  uint8_t had = !(*bus & 0x0100);                /* el 68000 ya tenía el bus del Z80 (quien lo pidió lo suelta) */
  if (!had) { *bus = 0x0100; while (*bus & 0x0100); }
  MD_SRAM_CTRL = 1;                              /* la SRAM (con escritura) en lugar de la ROM desde 0x200000 */
  volatile uint8_t *s = MD_SRAM + 2 * off;
  if (write) for (; n; n--, s += 2) *s = *buf++;
  else for (; n; n--, s += 2) *buf++ = *s;
  MD_SRAM_CTRL = 0;                              /* la ROM otra vez */
  if (!had) *bus = 0x0000;                       /* el Z80 sigue */
  __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "memory");
}

void nt_sram_read(uint16_t off, uint8_t *dst, uint16_t n) { md_sram_copy(off, dst, n, 0); }
void nt_sram_write(uint16_t off, const uint8_t *src, uint16_t n) { md_sram_copy(off, (uint8_t *)src, n, 1); }
