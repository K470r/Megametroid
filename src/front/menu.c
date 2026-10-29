/*
 * MegaMetroid — pantalla de partidas ("SAMUS DATA") y de opciones, con textos por idioma.
 *
 * Gráficos del SNES (make_menu_data.py): fuente grande 8x16 del menú y la pequeña, marcos,
 * cursor y cascos, y el planeta Zebes de fondo. Los textos salen de tools/lang/menu.txt (inglés y
 * español); las tildes y la Ñ se dibujan en la fila de tiles de arriba de la letra.
 *
 * Planos: A = textos y marcos, B = fondo. Paletas: PAL0 texto, PAL1-2 fondo, PAL3 marcos y sprites.
 * Mando: arriba/abajo elige, A/C/START acepta, B cancela.
 */
#include "menu.h"
#include "menu_data.h"
#include "menu_strings.h"
#include "sound.h"

u8 g_lang = LANG_EN;
u8 g_opt_icon_cancel = 0;
u8 g_opt_moonwalk = 0;

#define TB          TILE_USER_INDEX
#define FONT_N      g_mnu_font_tiles
#define OBJ_BASE    (TB + 2 * FONT_N)
#define IMG_BASE    (OBJ_BASE + 64)
#define ATTR_TEXT   TILE_ATTR(PAL0, 1, 0, 0)
#define ATTR_BOX    TILE_ATTR(PAL3, 1, 0, 0)

static u16 cram[64];

/* ------------------------------------------------------------------ utilidades */
static const char *mstr(u16 id)
{
    return (const char *) mnu_strings + g_mnu_str_off[g_lang * g_mnu_num_strings + id];
}

static u16 slen(const char *s)
{
    u16 n = 0;
    while (s[n]) n++;
    return n;
}

static void fade_to(u16 from, u16 to)
{
    /* brillo lineal de 'from' a 'to' (0-15) en 8 cuadros sobre la CRAM guardada */
    for (u16 k = 0; k <= 8; k++)
    {
        u16 b = from + ((s16) (to - from) * (s16) k) / 8;
        u16 tmp[64];
        for (u16 i = 0; i < 64; i++)
        {
            u16 c = cram[i];
            u16 r = ((c >> 1) & 7) * b / 15, g = ((c >> 5) & 7) * b / 15, bl = ((c >> 9) & 7) * b / 15;
            tmp[i] = (bl << 9) | (g << 5) | (r << 1);
        }
        SYS_doVBlankProcess();
        PAL_setColors(0, tmp, 64, CPU);
    }
}

static u16 read_pad(u16 *prev)
{
    u16 pad = JOY_readJoypad(JOY_1);
    u16 pressed = pad & ~*prev;
    *prev = pad;
    return pressed;
}

/* ------------------------------------------------------------------ texto */
/* Fuente grande 8x16: dos tiles por letra; la tilde va en la fila de arriba. */
static void draw_big(const char *s, u16 x, u16 y, bool dim)
{
    u16 base = TB + (dim ? FONT_N : 0);
    for (; *s; s++, x++)
    {
        u8 ch = (u8) *s;
        u16 w = g_mnu_big[ch];
        VDP_setTileMapXY(BG_A, ATTR_TEXT + base + (w & 0xFF), x, y);
        VDP_setTileMapXY(BG_A, ATTR_TEXT + base + (w >> 8), x, y + 1);
        if (g_mnu_big_acc[ch])
            VDP_setTileMapXY(BG_A, ATTR_TEXT + base + g_mnu_font_accent + g_mnu_big_acc[ch] - 1, x, y - 1);
    }
}

static void draw_small(const char *s, u16 x, u16 y)
{
    for (; *s; s++, x++)
        VDP_setTileMapXY(BG_A, ATTR_TEXT + TB + g_mnu_small[(u8) *s], x, y);
}

static void clear_rows(u16 y0, u16 y1)
{
    for (u16 y = y0; y < y1; y++) VDP_clearTileMapRect(BG_A, 0, y, 32, 1);
}

/* Marco del título (piezas de los sprites del SNES, aquí en el plano A): ancho según el texto */
static void draw_box_title(const char *s)
{
    u16 n = slen(s);
    u16 w = n + 4;
    u16 x0 = (32 - w) / 2;
    u16 ob = OBJ_BASE;
    for (u16 x = 0; x < w; x++)
    {
        u16 t = x == 0 ? 0xF9 : (x == w - 1 ? 0xFB : 0xFA);
        VDP_setTileMapXY(BG_A, ATTR_BOX + ob + t - 0xC0, x0 + x, 0);
        t = x == 0 ? 0xFC : (x == w - 1 ? 0xFD : 0xFA);
        VDP_setTileMapXY(BG_A, ATTR_BOX + ob + t - 0xC0, x0 + x, 3);
    }
    for (u16 y = 1; y < 3; y++)
    {
        VDP_setTileMapXY(BG_A, ATTR_BOX + ob + 0xED - 0xC0, x0, y);
        VDP_setTileMapXY(BG_A, ATTR_BOX + ob + 0xED - 0xC0, x0 + w - 1, y);
    }
    draw_big(s, x0 + 2, 1, FALSE);
}

/* Pie: cruz + ELEGIR, A + OK!, B + CANCELAR (íconos del SNES) */
static void draw_footer(void)
{
    const u16 *ic = g_mnu_icons;
    u16 xs[3] = { 1, 11, 19 };
    for (u16 k = 0; k < 3; k++)
        for (u16 j = 0; j < 4; j++)
        {
            u16 w = ic[k * 4 + j];
            VDP_setTileMapXY(BG_A, ATTR_TEXT + TB + (w & 0x3FF) + (w & 0x1800), xs[k] + (j & 1), 25 + (j >> 1));
        }
    draw_small(mstr(S_FOOT_SELECT), 3, 26);
    draw_small(mstr(S_FOOT_OK), 13, 26);
    draw_small(mstr(S_FOOT_CANCEL), 21, 26);
}

/* ------------------------------------------------------------------ sprites */
static u16 nspr;
static void spr_add(s16 x, s16 y, u16 tile)
{
    if (nspr >= 64) return;
    VDP_setSpriteFull(nspr, x, y, SPRITE_SIZE(1, 1), TILE_ATTR_FULL(PAL3, 1, 0, 0, OBJ_BASE + tile - 0xC0), nspr + 1);
    nspr++;
}

static void spr_end(void)
{
    if (!nspr) { VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, 0); nspr = 1; }
    else VDP_setSpriteLink(nspr - 1, 0);
    VDP_updateSprites(nspr, DMA);
    nspr = 0;
}

static void cursor(s16 x, s16 y)
{
    spr_add(x, y, 0xDF);
    spr_add(x + 8, y, 0xEF);
}

/* DrawMenuSpritemap ($81:891F) con los spritemaps de los cascos ($82:C569, 0x2C-0x33): cada
   entrada de 16x16 va como 4 sprites de 8x8 (tiles t, t+1, t+16, t+17 del SNES). */
static void spr_add_f(s16 x, s16 y, u16 tile, u16 hf, u16 vf)
{
    if (nspr >= 64) return;
    VDP_setSpriteFull(nspr, x, y, SPRITE_SIZE(1, 1), TILE_ATTR_FULL(PAL3, 1, vf, hf, OBJ_BASE + tile - 0xC0), nspr + 1);
    nspr++;
}

static void draw_helmet_smap(u16 idx, s16 x, s16 y)
{
    const u16 *e = g_mnu_helmet_smap + g_mnu_helmet_smap_off[idx];
    u16 n = *e++;
    for (; n; n--, e += 3)
    {
        s16 ex = x + (s16) e[0], ey = y + (s16) e[1];
        u16 t = 0xC0 + (e[2] & 0xFF), big = (e[2] >> 8) & 1, hf = (e[2] >> 9) & 1, vf = (e[2] >> 10) & 1;
        if (!big) { spr_add_f(ex, ey, t, hf, vf); continue; }
        for (u16 c = 0; c < 4; c++)
        {
            u16 cx = c & 1, cy = c >> 1;
            spr_add_f(ex + ((cx ^ hf) << 3), ey + ((cy ^ vf) << 3), t + cx + (cy << 4), hf, vf);
        }
    }
}

/* Cascos de las ranuras (DrawFileSelectSlotSamusHelmet, $81:9DE4): al elegir una ranura su
   temporizador arranca en 1 y el casco gira de lado a frente, un cuadro de animación cada 8. */
static u8 helm_frame[3], helm_timer[3];
static const u16 kHelmetX = 100, kHelmetY[3] = { 47, 87, 127 };

static void reset_helmets(void)
{
    for (u16 k = 0; k < 3; k++) helm_frame[k] = helm_timer[k] = 0;
}

static void helmet_slot(u16 k)
{
    if (helm_timer[k] && !--helm_timer[k])
    {
        helm_timer[k] = 8;
        u16 f = helm_frame[k] + 1;
        if (helm_frame[k] >= 7) { helm_timer[k] = 0; f = 7; }
        helm_frame[k] = f;
    }
    draw_helmet_smap(helm_frame[k], kHelmetX, kHelmetY[k]);
}

/* ------------------------------------------------------------------ carga */
static void load_screen(void)
{
    SYS_disableInts();
    VDP_setEnable(FALSE);
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VDP_getVerticalScrollingMode());
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0); VDP_setVerticalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0); VDP_setVerticalScroll(BG_B, 0);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_loadTileData((const u32 *) mnu_font, TB, 2 * FONT_N, CPU);
    VDP_loadTileData((const u32 *) mnu_obj, OBJ_BASE, 64, CPU);
    const MnuImage *im = &g_img_planet;
    VDP_loadTileData(im->tiles, IMG_BASE, im->num_tiles, CPU);
    for (u16 y = 0; y < 28; y++)
        for (u16 x = 0; x < 32; x++)
        {
            u16 w = im->map[y * 32 + x];
            VDP_setTileMapXY(BG_B, w ? w + IMG_BASE + (1 << 13) : 0, x, y);
        }
    for (u16 i = 0; i < 16; i++)
    {
        cram[i] = g_mnu_text_pal[i];
        cram[16 + i] = im->cram[i];
        cram[32 + i] = im->cram[16 + i];
        cram[48 + i] = g_mnu_obj_pal[i];
    }
    cram[0] = 0;
    u16 black[64] = { 0 };
    PAL_setColors(0, black, 64, CPU);
    VDP_setEnable(TRUE);
    SYS_enableInts();
}

/* ------------------------------------------------------------------ pantallas */
static void draw_fsel(void)
{
    clear_rows(0, 28);
    draw_box_title(mstr(S_FSEL_TITLE));
    for (u16 i = 0; i < 3; i++)
    {
        char name[12];
        const char *s = mstr(S_SLOT);
        u16 n = 0;
        while (s[n] && n < 9) { name[n] = s[n]; n++; }
        name[n++] = ' ';
        name[n++] = 'A' + i;
        name[n] = 0;
        draw_big(name, 3, 5 + i * 5, FALSE);
        draw_small(mstr(S_NO_DATA), 15, 6 + i * 5);
        if (g_lang == LANG_EN)
            for (u16 k = 0; k < 3; k++)
                VDP_setTileMapXY(BG_A, ATTR_TEXT + TB + (g_mnu_icons[12 + k] & 0x3FF), 27 + k, 5 + i * 5);
        else
            draw_small(mstr(S_TIME), 25, 5 + i * 5);
    }
    draw_small(mstr(S_EXIT), 4, 26);
}

static void draw_opt(u16 sel_lang)
{
    clear_rows(0, 28);
    draw_box_title(mstr(S_OPT_TITLE));
    draw_big(mstr(S_START_GAME), 4, 6, FALSE);
    draw_big(mstr(S_LANG_EN), 4, 10, sel_lang != LANG_EN);
    draw_big(mstr(S_LANG_ES), 4, 13, sel_lang != LANG_ES);
    draw_big(mstr(S_CONTROLLER), 4, 17, FALSE);
    draw_big(mstr(S_SPECIAL), 4, 21, FALSE);
    draw_footer();
}

static void draw_special(u16 sel)
{
    clear_rows(0, 28);
    draw_box_title(mstr(S_SP_TITLE));
    draw_big(mstr(S_SP_ICON), 4, 6, FALSE);
    draw_big(mstr(S_SP_AUTO), 6, 9, g_opt_icon_cancel != 0);
    draw_big(mstr(S_SP_MANUAL), 16, 9, g_opt_icon_cancel != 1);
    draw_big(mstr(S_SP_MOON), 4, 14, FALSE);
    draw_big(mstr(S_SP_ON), 6, 17, !g_opt_moonwalk);
    draw_big(mstr(S_SP_OFF), 16, 17, g_opt_moonwalk);
    draw_footer();
    (void) sel;
}

static void draw_controller(void)
{
    clear_rows(0, 28);
    draw_box_title(mstr(S_CTL_TITLE));
    draw_big(mstr(S_CTL_JUMP), 3, 5, FALSE);
    draw_big(mstr(S_CTL_RUN), 3, 8, FALSE);
    draw_big(mstr(S_CTL_SHOOT), 3, 11, FALSE);
    draw_big(mstr(S_CTL_AIM6), 3, 14, FALSE);
    draw_big(mstr(S_CTL_SEL6), 3, 17, FALSE);
    draw_small(mstr(S_CTL_PAD6), 3, 21);
    draw_footer();
}

/* Opciones: devuelve TRUE para empezar, FALSE para volver a las partidas */
static bool run_options(void)
{
    static const u8 rows[5] = { 6, 10, 13, 17, 21 };
    u16 sel = 0, prev = 0xFFFF;
    draw_opt(g_lang);
    for (;;)
    {
        u16 p = read_pad(&prev);
        /* GameOptionsMenu_3_OptionsScreen ($82): arriba/abajo 1/$37, A o START 1/$38 */
        if (p & BUTTON_UP) { sel = (sel + 4) % 5; snd_request(0x106, 0x37); }
        else if (p & BUTTON_DOWN) { sel = (sel + 1) % 5; snd_request(0x106, 0x37); }
        if (p & BUTTON_B) return FALSE;
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START))
        {
            snd_request(0x106, 0x38);
            if (sel == 0) return TRUE;
            if (sel == 1 || sel == 2)
            {
                g_lang = sel == 1 ? LANG_EN : LANG_ES;
                draw_opt(g_lang);
            }
            if (sel == 3)
            {
                draw_controller();
                u16 pv = 0xFFFF;
                for (;;) { cursor(-32, -32); spr_end(); SYS_doVBlankProcess(); if (read_pad(&pv) & (BUTTON_B | BUTTON_A | BUTTON_C | BUTTON_START)) break; }
                draw_opt(g_lang);
            }
            if (sel == 4)
            {
                u16 s2 = 0, pv = 0xFFFF;
                draw_special(0);
                for (;;)
                {
                    u16 q = read_pad(&pv);
                    /* GameOptionsMenu_8_SpecialSettings ($82): arriba/abajo 1/$37; cambiar o salir 1/$38 */
                    if (q & (BUTTON_UP | BUTTON_DOWN)) { s2 ^= 1; snd_request(0x106, 0x37); }
                    if (q & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_A | BUTTON_C | BUTTON_B | BUTTON_START))
                        snd_request(0x106, 0x38);
                    if (q & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_A | BUTTON_C))
                    {
                        if (s2 == 0) g_opt_icon_cancel ^= 1; else g_opt_moonwalk ^= 1;
                        draw_special(s2);
                    }
                    if (q & (BUTTON_B | BUTTON_START)) break;
                    cursor(16, s2 ? 14 * 8 + 4 : 6 * 8 + 4);
                    spr_end();
                    SYS_doVBlankProcess();
                }
                draw_opt(g_lang);
            }
        }
        cursor(16, rows[sel] * 8 + 4);
        spr_end();
        SYS_doVBlankProcess();
    }
}

u16 menu_run(void)
{
    load_screen();
    draw_fsel();
    fade_to(0, 15);
    u16 sel = 0, prev = 0xFFFF;
    reset_helmets();
    for (;;)
    {
        u16 p = read_pad(&prev);
        /* FileSelectMenu_4_Main ($81:A1C2) sin partidas guardadas: ranuras A-C y EXIT */
        if (p & BUTTON_UP) { sel = (sel + 3) % 4; snd_request(0x106, 0x37); }
        else if (p & BUTTON_DOWN) { sel = (sel + 1) % 4; snd_request(0x106, 0x37); }
        else if ((p & BUTTON_B) && !(p & (BUTTON_A | BUTTON_C | BUTTON_START)))
        {
            snd_request(0x106, 0x37);                   /* B: al título */
            fade_to(15, 0);
            return MENU_EXIT;
        }
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START))
        {
            if (sel == 3)
            {
                snd_request(0x106, 0x37);
                fade_to(15, 0);
                return MENU_EXIT;
            }
            /* ranura elegida: el casco gira (FileSelectMenu_31_TurnSamusHelmet, $81:9D77) hasta quedar
               de frente o hasta otro botón, y se pasa a las opciones */
            snd_request(0x106, 0x2A);
            helm_timer[sel] = 1;
            for (;;)
            {
                SYS_doVBlankProcess();
                u16 q = read_pad(&prev);
                for (u16 i = 0; i < 3; i++) helmet_slot(i);
                cursor(6, 44 + sel * 40);
                spr_end();
                if ((q & (BUTTON_A | BUTTON_C | BUTTON_START)) || (helm_frame[sel] == 7 && !helm_timer[sel])) break;
            }
            fade_to(15, 0);
            spr_end();                                  /* sin cascos en las opciones */
            draw_opt(g_lang);
            fade_to(0, 15);
            if (run_options())
            {
                fade_to(15, 0);
                return MENU_START;
            }
            fade_to(15, 0);
            draw_fsel();
            reset_helmets();
            for (u16 i = 0; i < 3; i++) helmet_slot(i);
            cursor(6, 44 + sel * 40);
            spr_end();
            fade_to(0, 15);
            prev = 0xFFFF;
        }
        for (u16 i = 0; i < 3; i++) helmet_slot(i);
        cursor(6, sel < 3 ? 44 + sel * 40 : 26 * 8);
        spr_end();
        SYS_doVBlankProcess();
    }
}
