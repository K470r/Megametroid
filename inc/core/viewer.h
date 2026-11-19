/* MegaMetroid — motor nativo: visor de salas de Zebes (viewer.c) */
#ifndef VIEWER_H
#define VIEWER_H
#include "nt.h"
extern uint8 nt_viewer;                    /* 1: visor (sin enemigos salvo la nave; START + izq./der. cambia de sala) */
extern uint8 nt_viewer_enemies;            /* 1: con los enemigos (salvo las salas de jefes sin portar) */
extern uint8 nt_viewer_place;              /* 1: Samus a un lugar libre al llegar (solo la Genesis) */
/* El visor es para probar: está en el PC (host/) y en las ROM de prueba (TEST=1: MD_TEST_INPUT), no en la ROM normal
   (ahí además dejaba a Samus con la energía siempre llena y atrasaba START un cuadro) */
#if defined(NT_SSF2) && !defined(MD_TEST_INPUT)
#define NT_NO_VIEWER 1
static inline uint8 viewer_keep_enemy(uint16 enemy_ptr) { (void)enemy_ptr; return 1; }
static inline uint16 viewer_input(uint16 joy) { return joy; }
#else
uint8 viewer_keep_enemy(uint16 enemy_ptr);
uint16 viewer_input(uint16 joy);           /* filtra el mando antes de cada cuadro del juego */
#endif
#endif
