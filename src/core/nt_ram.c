/* MegaMetroid — motor nativo: borrar un rango de la RAM del SNES (docs/35).
 * Algunas rutinas del SNES ponen a cero un bloque de RAM entero (p. ej. las cinemáticas: 7E:198D-1C1E, que pisa
 * también los proyectiles enemigos). Aquí cada variable es nativa: gen_vars.py deja la tabla g_ram_vars (dirección
 * del SNES de cada variable usada) y esto borra los elementos que caen en el rango. */
#include "nt.h"
#include "vars.h"

void nt_clear_ram(uint16 from, uint16 to) {   /* [from, to), direcciones del SNES (7E:xxxx) */
  for (int i = 0; i < NT_RAM_VARS; i++) {
    const NtRamVar *v = &g_ram_vars[i];
    uint16 end = v->addr + v->esize * v->n;
    if (end <= from || v->addr >= to)
      continue;
    for (uint16 e = 0; e < v->n; e++) {
      uint16 a = v->addr + e * v->esize;
      if (v->esize == 1) {
        if (a >= from && a < to) ((uint8 *)v->p)[e] = 0;
      } else {
        uint16 *w = (uint16 *)v->p + e;
        if (a >= from && a < to) *w &= 0xFF00;          /* byte bajo (dirección par) */
        if (a + 1 >= from && a + 1 < to) *w &= 0x00FF;  /* byte alto */
      }
    }
  }
}

/* un byte de la RAM del SNES por su dirección (las variables que el motor no tiene valen 0) */
static uint8 nt_ram_b(uint16 a) {
  int lo = 0, hi = NT_RAM_VARS - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    const NtRamVar *v = &g_ram_vars[m];
    if (a < v->addr) { hi = m - 1; continue; }
    uint16 o = a - v->addr;
    if (o >= v->esize * v->n) { lo = m + 1; continue; }
    if (v->esize == 1)
      return ((const uint8 *)v->p)[o];
    uint16 w = ((const uint16 *)v->p)[o >> 1];
    return (o & 1) ? w >> 8 : (uint8)w;
  }
  return 0;
}

/* una palabra de la RAM del SNES por su dirección: el decomp a veces lee una variable con un desplazamiento (el
   fantasma de Wrecked Ship, $A8:9D36: samus_y_pos + índice del enemigo) */
uint16 nt_ram_w(uint16 a) {
  return nt_ram_b(a) | nt_ram_b((uint16)(a + 1)) << 8;
}
