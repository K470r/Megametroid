/* MegaMetroid — datos del menú (los genera make_menu_data.py). */
#ifndef MENU_DATA_H
#define MENU_DATA_H
#include <genesis.h>

typedef struct {
    u16 num_tiles;
    const u32 *tiles;
    const u16 *map;        /* 32x28 palabras sin la base de tiles */
    u16 cram[48];          /* hasta 3 líneas de paleta */
} MnuImage;

extern const MnuImage g_img_planet;
extern const u8 mnu_font[], mnu_obj[], mnu_strings[];
extern const u16 g_mnu_font_tiles, g_mnu_font_accent;
extern const u16 g_mnu_big[256];
extern const u8 g_mnu_big_acc[256], g_mnu_small[256];
extern const u16 g_mnu_icons[15];
extern const u16 g_mnu_text_pal[16], g_mnu_obj_pal[16];
extern const u16 g_mnu_helmet_smap[], g_mnu_helmet_smap_off[];
extern const u8 g_font8[256], g_font8_acc[256];
extern const u16 g_font16[256];
extern const u16 g_font_green_pal[16], g_font_white_pal[16];
extern const u16 g_mnu_num_strings;
extern const u16 g_mnu_str_off[];

#endif
