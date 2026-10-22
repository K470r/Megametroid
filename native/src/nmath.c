/* MegaMetroid — motor nativo: cuentas comunes de los enemigos (docs/33). Escrito a mano desde snesrev/sm
 * (sm_a0.c CalculateAngleFromXY / SuitDamageDivision, sm_86.c Math_MultBySinCos, sm_a9.c sub_A9C46C,
 * sm_91.c Samus_DealDamage). */
#include "nmath.h"
#include "samus.h"

uint16 CalculateAngleFromXY(uint16 x, uint16 y) {  // 0xA0C0B1
  int idx = 0;
  if (x & 0x8000) { idx += 2; x = -x; }
  if (y & 0x8000) { idx += 1; y = -y; }
  uint8 r;
  if (y < x) {
    uint16 d = SnesDivideN(y << 8, x) >> 3;              /* el divisor es de 8 bits, como en el SNES */
    switch (idx) {
    case 0: r = d + 64; break;                            // _0
    case 1: r = 64 - d; break;                            // _3
    case 2: r = -64 - d; break;                           // _5
    default: r = d - 64; break;                           // _6
    }
  } else {
    uint16 d = SnesDivideN(x << 8, y) >> 3;
    switch (idx) {
    case 0: r = 0x80 - d; break;                          // _1
    case 1: r = d; break;                                 // _2
    case 2: r = d + 0x80; break;                          // _4
    default: r = 0 - d; break;                            // _7
    }
  }
  return r;
}

static uint16 Math_MultBySinCos(uint16 m, uint16 ang) {  // 0x86C27A
  uint16 s = kSinCosTable8bit_Sext[(ang & 0xff) + 64];
  uint16 prod = (uint32)m * abs16n(s) >> 8;
  return sign16(s) ? -prod : prod;
}
uint16 Math_MultBySin(uint16 a, uint16 ang) { return Math_MultBySinCos(a, ang); }        // 0x86C26C
uint16 Math_MultByCos(uint16 a, uint16 ang) { return Math_MultBySinCos(a, ang + 64); }   // 0x86C272

/* sub_A9C46C: M7A = seno (16 bits con signo), M7B = m (8 bits con signo); MPYM-MPYH = bits 8-23 del producto */
static uint16 sub_A9C46C(uint16 ang, uint16 m) {
  int32 p = (int32)(int16)kSinCosTable8bit_Sext[(ang & 0xff) + 64] * (int8)m;
  return (uint16)(p >> 8);
}
uint16 ComputeSinMult(uint16 m, uint16 ang) { return sub_A9C46C(ang, m); }        // 0xA9C460
uint16 ComputeCosMult(uint16 m, uint16 ang) { return sub_A9C46C(ang + 64, m); }   // 0xA9C465

uint16 SuitDamageDivision(uint16 a) {  // 0xA0A45E
  if (equipped_items & 0x20)
    return a >> 2;
  if (equipped_items & 1)
    return a >> 1;
  return a;
}

void Samus_DealDamage(uint16 a) {  // 0x91DF51
  if (a & 0x8000) {
    nt_unimpl("Samus_DealDamage (daño negativo: InvalidInterrupt_Crash)");
    return;
  }
  if (a != 300 && !time_is_frozen_flag) {
    samus_health -= a;
    if (samus_health & 0x8000)
      samus_health = 0;
  }
}

void Mode7Rotate(uint16 *x, uint16 *y) {
  uint16 r34 = *x - reg_M7X;
  uint16 r36 = reg_M7Y - *y;
  uint16 r26 = (uint16)(((int32)(int16)r34 * (int16)reg_M7A) >> 8);
  r26 += (uint16)(((int32)(int16)reg_M7B * (int16)r36) >> 8);
  uint16 nx = r26 + reg_M7X;
  r26 = (uint16)(((int32)(int16)reg_M7C * (int16)r34) >> 8);
  r26 += (uint16)(((int32)(int16)reg_M7A * (int16)r36) >> 8);
  *y = reg_M7Y - r26;
  *x = nx;
}
