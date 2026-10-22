/* MegaMetroid — motor nativo: visor de salas de Zebes (viewer.c) */
#ifndef VIEWER_H
#define VIEWER_H
#include "nt.h"
extern uint8 nt_viewer;                    /* 1: visor (sin enemigos salvo la nave; START + izq./der. cambia de sala) */
extern uint8 nt_viewer_enemies;            /* 1: con los enemigos de Crateria */
extern uint8 nt_viewer_place;              /* 1: Samus a un lugar libre al llegar (solo la Genesis) */
uint8 viewer_keep_enemy(uint16 enemy_ptr);
uint16 viewer_input(uint16 joy);           /* filtra el mando antes de cada cuadro del juego */
#endif
