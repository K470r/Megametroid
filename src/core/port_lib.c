/* Funciones pequeñas del decomp (bancos sin traducir) que usa el código traducido (port_gen.c, docs/42). */
#include "nt.h"
#include "samus.h"

uint32 Multiply16x16(uint16 a, uint16 j) {  // 0x8082D6
  return (uint32)a * (uint32)j;
}

PairU16 MakePairU16(uint16 k, uint16 j) {  // (sm_rtl.c)
  return (PairU16){ k, j };
}
