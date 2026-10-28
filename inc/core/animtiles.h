/* MegaMetroid — motor nativo: tiles animados (banco $87, animtiles.c) */
#ifndef ANIMTILES_H
#define ANIMTILES_H
#include "nt.h"
void EnableAnimtiles(void);
void DisableAnimtiles(void);
void ClearAnimtiles(void);
void SpawnAnimtiles(uint16 j);        /* $87:8027 */
void AnimtilesHandler(void);          /* $87:8064 */
/* tiles animados de $87:src (size bytes) para la VRAM del SNES vram: la Genesis sube los suyos (PC: nada) */
void nt_animtiles_transfer(uint16 src, uint16 size, uint16 vram);
#endif
