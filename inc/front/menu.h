/* MegaMetroid — menú de partidas, opciones e intro con texto. */
#ifndef MENU_H
#define MENU_H
#include <genesis.h>

enum { LANG_EN = 0, LANG_ES = 1 };
extern u8 g_lang;                  /* idioma de los textos */
extern u8 g_opt_icon_cancel;       /* 0 automático, 1 manual (como el SNES) */
extern u8 g_opt_moonwalk;          /* 1 = caminar hacia atrás */

#define MENU_START 1               /* empezar partida */
#define MENU_EXIT  0               /* volver al título */

u16 menu_run(void);                /* pantalla de partidas + opciones */
u16 menu_run_at(u16 options);      /* lo mismo; options: empezar en las opciones de g_menu_slot */
void menu_load_options(void);      /* idioma y opciones guardados en la SRAM */
extern u16 g_menu_slot;            /* la ranura elegida (0-2) */
extern u8 g_menu_slot_used;        /* 1 si tiene partida (se carga); 0: partida nueva (la intro) */
void intro_run(void);              /* texto de la intro (páginas del diario de Samus) */

#endif
