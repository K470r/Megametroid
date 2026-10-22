/* Tabla variable nativa -> dirección en la RAM del SNES (solo PC, para la comprobación). */
#ifndef VARS_MAP_H
#define VARS_MAP_H
typedef struct { const char *name; void *ptr; unsigned addr, size, count; } VarMap;
extern const VarMap g_vars_map[];
#endif
