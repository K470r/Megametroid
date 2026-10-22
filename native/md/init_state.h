/* Estado inicial del motor (native/host/mkinit): variable -> bytes en kInitBlob, en orden. */
#ifndef INIT_STATE_H
#define INIT_STATE_H
#include <stdint.h>
typedef struct { void *ptr; uint16_t size; } InitEnt;
extern const InitEnt kInitState[];
#endif
