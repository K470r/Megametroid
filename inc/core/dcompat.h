/* MegaMetroid — motor nativo: lo que usa el código escrito a partir del decomp (tools/port/xlate.py, docs/42).
 * Los mismos nombres y la misma semántica que types.h / variables.h del decomp, para el 68000 (big endian) y el PC. */
#ifndef DCOMPAT_H
#define DCOMPAT_H
#include "nt.h"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define NT_BE 1
#else
#define NT_BE 0
#endif
static inline uint16 swap16(uint16 v) { return (uint16)((v << 8) | (v >> 8)); }   /* types.h del decomp */
/* byte bajo / alto (también para asignar), como LOBYTE/HIBYTE del decomp */
#define LOBYTE(x) (*((uint8 *)&(x) + (NT_BE ? sizeof(x) - 1 : 0)))
#define HIBYTE(x) (*((uint8 *)&(x) + (NT_BE ? 0 : sizeof(x) - 1)))
#define LOWORD(x) (*((uint16 *)&(x) + (NT_BE ? sizeof(x) / 2 - 1 : 0)))
#define HIWORD(x) (*((uint16 *)&(x) + (NT_BE ? 0 : sizeof(x) / 2 - 1)))
#define GET_HIBYTE(x) (((x) & 0xff00) >> 8)
#define PAIR16(high, low) ((uint16)((high) << 8) | (uint8)(low))
#define __PAIR32__(high, low) (((uint32)(high) << 16) | (uint16)(low))
#ifndef IPAIR32
#define IPAIR32(high, low) ((int32)(((uint32)(high) << 16) | (uint16)(low)))
#endif
#define INT16_SHL8(x) ((int16)(x) << 8)
#define INT16_SHL16(x) ((int16)(x) << 16)

static inline void AddToHiLo(uint16 *hi, uint16 *lo, uint32 v) {
  uint32 t = *lo + ((uint32)*hi << 16) + v;
  *lo = (uint16)t;
  *hi = (uint16)(t >> 16);
}
static inline void SetHiLo(uint16 *hi, uint16 *lo, uint32 t) {
  *lo = (uint16)t;
  *hi = (uint16)(t >> 16);
}
static inline bool __CFADD__uint16(uint16 x, uint16 y) { return (uint16)(x) > (uint16)(x + y); }
static inline bool __CFADD__uint8(uint8 x, uint8 y) { return (uint8)(x) > (uint8)(x + y); }

#define sign32(x) ((x) & 0x80000000)
static inline uint16 Mult8x8(uint8 a, uint8 b) {
#ifdef __mc68000__
  uint16 r = a;                                 /* mulu.w del 68000 */
  __asm__("mulu.w %1,%0" : "+d"(r) : "d"((uint16)b));
  return r;
#else
  return (uint16)(a * b);
#endif
}
static inline uint16 SnesDivide(uint16 a, uint8 b) {
  if (!b) return 0xffff;
#ifdef __mc68000__
  uint32 q = a;
  __asm__("divu.w %1,%0" : "+d"(q) : "d"((uint16)b));
  return (uint16)q;
#else
  return a / b;
#endif
}
static inline uint16 SnesModulus(uint16 a, uint8 b) {
  if (!b) return a;
#ifdef __mc68000__
  uint32 q = a;
  __asm__("divu.w %1,%0" : "+d"(q) : "d"((uint16)b));
  return (uint16)(q >> 16);
#else
  return a % b;
#endif
}
typedef struct Point32 { uint32 x, y; } Point32;
typedef struct Point16U { uint16 x, y; } Point16U;
typedef struct Rect16U { uint16 x, y, w, h; } Rect16U;
extern Rect16U eproj_spawn_rect;   /* del decomp: rectángulo para algunos proyectiles (eproj.c) */
extern uint16 eproj_spawn_r22;
#endif
