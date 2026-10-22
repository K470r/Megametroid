/* Funciones pequeñas del decomp (bancos sin traducir) que usa el código traducido (port_gen.c, docs/42). */
#include "nt.h"

uint32 Multiply16x16(uint16 a, uint16 j) {  // 0x8082D6
  return (uint32)a * (uint32)j;
}
