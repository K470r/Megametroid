/* MegaMetroid — motor nativo: tablas de bancos y cuentas comunes de los enemigos (docs/33).
 * Los bancos se guardan como bytes (gen/tables.c, kBankXXB): A6W(a) es la palabra que en el SNES empieza en $A6:a
 * (dos bytes). Los punteros siguen siendo los del SNES; aquí solo son índices. */
#ifndef NMATH_H
#define NMATH_H
#include "nt.h"
#include "../gen/tables.h"

/* bancos guardados como bytes (gen/tables.c, kBankXXB): la palabra de $XX:a, de dos bytes */
/* (el asm vacío impide que gcc junte los dos bytes en una lectura de palabra, que en el 68000 falla si es impar) */
static inline uint16 bankb_w(const uint8 *t, uint16 a) {
  uint8 lo = t[(uint16)(a - 0x8000)], hi = t[(uint16)(a - 0x7FFF)];
  __asm__("" : "+r"(hi));
  return (uint16)(lo | hi << 8);
}
#define BANKB_W(t, a) bankb_w((t), (a))
static inline uint16 A6W(uint16 a) { return BANKB_W(kBankA6B, a); }
static inline uint8 A6B(uint16 a) { return kBankA6B[(uint16)(a - 0x8000)]; }
static inline uint16 B86W(uint16 a) { return BANKB_W(kBank86B, a); }
static inline uint16 B81W(uint16 a) { return BANKB_W(kBank81B, a); }
static inline uint16 B82W(uint16 a) { return BANKB_W(kBank82B, a); }
static inline uint16 B88W(uint16 a) { return BANKB_W(kBank88B, a); }
static inline uint8 B88B(uint16 a) { return kBank88B[(uint16)(a - 0x8000)]; }
static inline uint16 B8BW(uint16 a) { return BANKB_W(kBank8BB, a); }
static inline uint8 B8BB(uint16 a) { return kBank8BB[(uint16)(a - 0x8000)]; }
static inline uint16 B8CW(uint16 a) { return BANKB_W(kBank8CB, a); }
static inline uint8 B8CB(uint16 a) { return kBank8CB[(uint16)(a - 0x8000)]; }
static inline uint16 B8DW(uint16 a) { return BANKB_W(kBank8DB, a); }
static inline uint16 B87W(uint16 a) { return BANKB_W(kBank87B, a); }
static inline uint16 B84W(uint16 a) { return BANKB_W(kBank84B, a); }
static inline uint16 B8FW(uint16 a) { return BANKB_W(kBank8FB, a); }

static inline uint16 abs16n(uint16 t) { return sign16(t) ? (uint16)-t : t; }
static inline uint16 SnesDivideN(uint16 a, uint8 b) {   /* división 16/8 del SNES */
  if (!b) return 0xffff;
#ifdef __mc68000__
  uint32 q = a;                                 /* divu.w del 68000 (sin la división de 32 bits de libgcc) */
  __asm__("divu.w %1,%0" : "+d"(q) : "d"((uint16)b));
  return (uint16)q;
#else
  return a / b;
#endif
}

uint16 CalculateAngleFromXY(uint16 x, uint16 y);          /* $A0:C0B1 */
uint16 Math_MultBySin(uint16 a, uint16 ang);              /* $86:C26C */
uint16 Math_MultByCos(uint16 a, uint16 ang);              /* $86:C272 */
uint16 ComputeSinMult(uint16 m, uint16 ang);              /* $A9:C460: seno(ang) * (int8)m (el multiplicador del modo 7) */
uint16 ComputeCosMult(uint16 m, uint16 ang);              /* $A9:C465 */
uint16 SuitDamageDivision(uint16 a);
/* punto de la sala girado como el plano de modo 7 (Samus_CalcPos_Mode7 $8B:8A52, CalcExplosion_Mode7 $8B:8AD9) */
void Mode7Rotate(uint16 *x, uint16 *y);                       /* $A0:A45E */
void Samus_DealDamage(uint16 a);                           /* $91:DF51 */
#endif
