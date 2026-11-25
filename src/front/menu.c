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
u16 g_menu_slot;                   /* la ranura elegida (0-2) */
u8 g_menu_slot_used;               /* la ranura tiene partida */

/* las partidas (src/core/save.c) y la SRAM (md_sram.c) */
extern u8 SramSlotInfo(u16 slot, u16 *health, u16 *max_health, u16 *hours, u16 *minutes);
extern u16 SramLastSlot(void);
extern void SramSetLastSlot(u16 s);
extern void nt_sram_read(u16 off, u8 *dst, u16 n);
extern void nt_sram_write(u16 off, const u8 *src, u16 n);

/* las opciones de la Genesis, después de la SRAM del SNES (0x2000): "MM", idioma, cancelar ícono, caminar atrás */
#define OPT_SRAM 0x2000
/* (el código del menú va en la página 23, que front_screens pone antes de llamarlo; esto se llama antes: en lo común) */
__attribute__((section(".text.keepboot"), noinline)) void menu_load_options(void)
{
    u8 b[5];
    nt_sram_read(OPT_SRAM, b, 5);
    if (b[0] != 'M' || b[1] != 'M') return;
    g_lang = b[2] == LANG_ES ? LANG_ES : LANG_EN;
    g_opt_icon_cancel = b[3] & 1;
    g_opt_moonwalk = b[4] & 1;
}
static void save_options(void)
{
    u8 b[5] = { 'M', 'M', g_lang, g_opt_icon_cancel, g_opt_moonwalk };
    nt_sram_write(OPT_SRAM, b, 5);
}

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

/* ------------------------------------------------------------------ ranuras en la SRAM */
static const u16 kSlotOffs[3] = { 0x10, 0x66C, 0xCC8 };   /* kOffsetToSaveSlot ($81:812B) */
static u8 slot_used(u16 i) { u16 a, b, c, d; return SramSlotInfo(i, &a, &b, &c, &d); }
/* las ranuras con partida (bit por ranura): se miran al dibujar la pantalla (draw_fsel, también después de copiar o
   borrar), no en cada cuadro. Validar una ranura suma sus 0x65C bytes de la SRAM y cada lectura le quita el bus al Z80
   (md_sram.c): las tres en cada cuadro dejaban al XGM2 casi sin correr y la música del menú sonaba lenta (MiSTer) */
static u8 g_used = 0xFF;
static u8 used_mask(void)
{
    if (g_used == 0xFF)
    {
        g_used = 0;
        for (u16 i = 0; i < 3; i++) if (slot_used(i)) g_used |= 1 << i;
    }
    return g_used;
}
static u8 any_used(void) { return used_mask() != 0; }

/* FileSelectMenu_13_FileCopyDoIt ($81:9A2C): los 0x65C bytes de la ranura y sus cuatro sumas */
static void sram_copy_slot(u16 src, u16 dst)
{
    u8 buf[64];
    for (u16 o = 0; o < 0x65C; o += 64)
    {
        u16 n = 0x65C - o < 64 ? 0x65C - o : 64;
        nt_sram_read(kSlotOffs[src] + o, buf, n);
        nt_sram_write(kSlotOffs[dst] + o, buf, n);
    }
    static const u16 kSums[4] = { 0, 8, 0x1FF0, 0x1FF8 };
    for (u16 k = 0; k < 4; k++)
    {
        nt_sram_read(kSums[k] + 2 * src, buf, 2);
        nt_sram_write(kSums[k] + 2 * dst, buf, 2);
    }
}
/* FileSelectMenu_25_FileClearDoClear ($81:9C9E): la ranura y sus sumas a cero */
static void sram_clear_slot(u16 s)
{
    u8 z[64] = { 0 };
    for (u16 o = 0; o < 0x65C; o += 64)
        nt_sram_write(kSlotOffs[s] + o, z, 0x65C - o < 64 ? 0x65C - o : 64);
    static const u16 kSums[4] = { 0, 8, 0x1FF0, 0x1FF8 };
    for (u16 k = 0; k < 4; k++) nt_sram_write(kSums[k] + 2 * s, z, 2);
}

/* ------------------------------------------------------------------ pantallas */
/* DrawFileSelectionHealth / DrawFileSelectionTime ($81:A087 / $81:A14E): "ENERGY", los tanques (de a 7 por fila,
   la segunda fila arriba), la energía que sobra de los tanques y la hora; sin partida, NO DATA */
static void cell(u16 tile, u16 x, u16 y) { VDP_setTileMapXY(BG_A, ATTR_TEXT + TB + tile, x, y); }
static void draw_slot_data(u16 i, u16 x, u16 y)
{
    u16 hp, mhp, hh, mm;
    if (!SramSlotInfo(i, &hp, &mhp, &hh, &mm))
    {
        draw_small(mstr(S_NO_DATA), x + 1, y + 1);
        return;
    }
    if (g_lang == LANG_EN)
    {
        static const u8 kEnergy[4] = { 0x9D, 0x9E, 0x9F, 0xCC };
        for (u16 k = 0; k < 4; k++) cell(kEnergy[k], x + k, y);
    }
    else
        draw_small(mstr(S_ENERGY), x, y);
    u16 full = hp / 100, tanks = mhp / 100, tx = x + 4, ty = y + 1, m = 7;
    for (u16 k = 0; k < tanks; k++)
    {
        cell(full ? (full--, 0x98) : 0x99, tx++, ty);
        if (!--m) { tx = x + 4; ty--; m = 8; }
    }
    u16 r = hp % 100;
    cell(0x60 + r / 10, x + 1, y + 1);
    cell(0x60 + r % 10, x + 2, y + 1);
    /* la hora (7E:3000 + 0x1B4: fila de abajo, columnas 26-30) */
    cell(0x60 + hh / 10, 26, y + 1);
    cell(0x60 + hh % 10, 27, y + 1);
    cell(0x8C, 28, y + 1);
    cell(0x60 + mm / 10, 29, y + 1);
    cell(0x60 + mm % 10, 30, y + 1);
}

static void draw_fsel(void)
{
    g_used = 0xFF;                               /* (la SRAM pudo cambiar: copiar, borrar, guardar) */
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
        draw_slot_data(i, 14, 5 + i * 5);
        if (g_lang == LANG_EN)
            for (u16 k = 0; k < 3; k++)
                VDP_setTileMapXY(BG_A, ATTR_TEXT + TB + (g_mnu_icons[12 + k] & 0x3FF), 27 + k, 5 + i * 5);
        else
            draw_small(mstr(S_TIME), 25, 5 + i * 5);
    }
    if (any_used())
    {
        draw_big(mstr(S_DATA_COPY), 4, 19, FALSE);
        draw_big(mstr(S_DATA_CLEAR), 4, 22, FALSE);
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
    /* lo que hace el mando conectado (md_main.c, read_joy) */
    static const u16 k3[7] = { S_C3_TITLE, S_C3_JUMP, S_C3_RUN, S_C3_SHOOT, S_C3_ITEM, S_C3_PAUSE, S_C3_CANCEL };
    static const u16 k6[7] = { S_C6_TITLE, S_C6_JUMP, S_C6_RUN, S_C6_SHOOT, S_C6_AIM, S_C6_ITEM, S_C6_PAUSE };
    const u16 *k = JOY_getJoypadType(JOY_1) == JOY_TYPE_PAD6 ? k6 : k3;
    draw_small(mstr(k[0]), 3, 5);
    for (u16 i = 1; i < 7; i++) draw_big(mstr(k[i]), 3, 4 + i * 3, FALSE);
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
        if (p & BUTTON_B) { save_options(); return FALSE; }
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START))
        {
            snd_request(0x106, 0x38);
            if (sel == 0) { save_options(); return TRUE; }
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

/* ------------------------------------------------------------------ copiar y borrar (FileSelectMenu 6-26) */
static void slot_name(char *o, u16 i)          /* "SAMUS A" (con el texto de la ranura del idioma) */
{
    const char *s = mstr(S_SLOT);
    u16 n = 0;
    while (s[n] && n < 9) { o[n] = s[n]; n++; }
    o[n++] = ' ';
    o[n++] = 'A' + i;
    o[n] = 0;
}
/* (de a un byte y sin juntar: gcc escribía " " como una palabra, y en una dirección impar el 68000 se detiene) */
static __attribute__((noinline)) void cat(char *o, const char *s)
{
    while (*o) o++;
    for (; *s; s++) { volatile char *v = o++; *v = *s; }
    *(volatile char *)o = 0;
}

static const char kSp[2] = " ";
static void prompt(const char *l1, const char *l2)   /* las filas de abajo (19-27): dos líneas grandes */
{
    clear_rows(18, 28);
    if (l1) draw_big(l1, 3, 19, FALSE);
    if (l2) draw_big(l2, 3, 22, FALSE);
}

/* una ranura con el cursor (allowed: bits de las que se pueden elegir); -1 con B */
static s16 choose_slot(u16 allowed, u16 *prev)
{
    u16 s = 0;
    while (!(allowed & (1 << s))) s++;
    for (;;)
    {
        u16 p = read_pad(prev);
        if (p & BUTTON_B) { snd_request(0x106, 0x37); return -1; }
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START)) { snd_request(0x106, 0x38); return s; }
        if (p & (BUTTON_UP | BUTTON_DOWN))
        {
            u16 d = (p & BUTTON_UP) ? 2 : 1;
            do s = (s + d) % 3; while (!(allowed & (1 << s)));
            snd_request(0x106, 0x37);
        }
        for (u16 i = 0; i < 3; i++) helmet_slot(i);
        cursor(6, 44 + s * 40);
        spr_end();
        SYS_doVBlankProcess();
    }
}

static u8 confirm(u16 *prev)                   /* YES / NO (fila 25); 1 = YES */
{
    u16 s = 1;
    draw_big(mstr(S_YES), 6, 25, FALSE);
    draw_big(mstr(S_NO), 18, 25, FALSE);
    for (;;)
    {
        u16 p = read_pad(prev);
        if (p & BUTTON_B) { snd_request(0x106, 0x37); return 0; }
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START)) { snd_request(0x106, 0x38); return s == 0; }
        if (p & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)) { s ^= 1; snd_request(0x106, 0x37); }
        for (u16 i = 0; i < 3; i++) helmet_slot(i);
        cursor(s ? 16 * 8 - 4 : 4 * 8 - 4, 25 * 8 + 4);
        spr_end();
        SYS_doVBlankProcess();
    }
}

static void wait_any(u16 *prev)
{
    for (;;)
    {
        if (read_pad(prev)) { snd_request(0x106, 0x37); return; }
        for (u16 i = 0; i < 3; i++) helmet_slot(i);
        cursor(-32, -32);
        spr_end();
        SYS_doVBlankProcess();
    }
}

static void copy_or_clear(u8 copy)
{
    u16 prev = 0xFFFF, used = 0;
    used = used_mask();
    clear_rows(0, 4);
    draw_box_title(mstr(copy ? S_COPY_TITLE : S_CLEAR_TITLE));
    prompt(mstr(copy ? S_COPY_WHICH : S_CLEAR_WHICH), 0);
    s16 a = choose_slot(used, &prev);
    if (a < 0) return;
    s16 b = -1;
    char l1[32], na[12], nb[12];
    slot_name(na, a);
    l1[0] = 0;
    if (copy)
    {
        prompt(mstr(S_COPY_WHERE), 0);
        b = choose_slot(7 & ~(1 << a), &prev);
        if (b < 0) return;
        slot_name(nb, b);
        cat(l1, mstr(S_COPY_VERB)); cat(l1, kSp); cat(l1, na); cat(l1, kSp); cat(l1, mstr(S_COPY_TO)); cat(l1, kSp); cat(l1, nb);
    }
    else
    {
        cat(l1, mstr(S_CLEAR_VERB)); cat(l1, kSp); cat(l1, na);
    }
    prompt(l1, mstr(S_IS_OK));
    if (!confirm(&prev)) return;
    if (copy) sram_copy_slot(a, b); else sram_clear_slot(a);
    draw_fsel();                               /* las ranuras como quedaron */
    clear_rows(0, 4);
    draw_box_title(mstr(copy ? S_COPY_TITLE : S_CLEAR_TITLE));
    prompt(mstr(copy ? S_COPIED : S_CLEARED), 0);
    wait_any(&prev);
}

u16 menu_run(void) { return menu_run_at(0); }

/* options: empezar en las opciones de la ranura g_menu_slot (B en el mapa de la partida: GameOptionsMenu) */
u16 menu_run_at(u16 options)
{
    load_screen();
    u16 sel = SramLastSlot(), prev = 0xFFFF;     /* (FileSelectMenu: la última ranura usada, SRAM 0x1FEC) */
    if (options)
    {
        sel = g_menu_slot;
        draw_opt(g_lang);
        fade_to(0, 15);
        if (run_options())
        {
            fade_to(15, 0);
            return MENU_START;
        }
        fade_to(15, 0);
    }
    draw_fsel();
    fade_to(0, 15);
    reset_helmets();
    for (;;)
    {
        u16 p = read_pad(&prev);
        /* FileSelectMenu_4_Main ($81:A1C2): ranuras A-C, DATA COPY y DATA CLEAR (si hay partidas) y EXIT */
        u8 used = any_used();
        if (p & BUTTON_UP) { do sel = (sel + 5) % 6; while (!used && (sel == 3 || sel == 4)); snd_request(0x106, 0x37); }
        else if (p & BUTTON_DOWN) { do sel = (sel + 1) % 6; while (!used && (sel == 3 || sel == 4)); snd_request(0x106, 0x37); }
        else if ((p & BUTTON_B) && !(p & (BUTTON_A | BUTTON_C | BUTTON_START)))
        {
            snd_request(0x106, 0x37);                   /* B: al título */
            fade_to(15, 0);
            return MENU_EXIT;
        }
        if (p & (BUTTON_A | BUTTON_C | BUTTON_START))
        {
            if (sel == 5)
            {
                snd_request(0x106, 0x37);
                fade_to(15, 0);
                return MENU_EXIT;
            }
            if (sel == 3 || sel == 4)
            {
                snd_request(0x106, 0x37);
                copy_or_clear(sel == 3);
                draw_fsel();
                sel = any_used() ? sel : 0;
                prev = 0xFFFF;
                continue;
            }
            /* ranura elegida: el casco gira (FileSelectMenu_31_TurnSamusHelmet, $81:9D77) hasta quedar
               de frente o hasta otro botón, y se pasa a las opciones */
            snd_request(0x106, 0x2A);
            helm_timer[sel] = 1;
            SramSetLastSlot(sel);
            g_menu_slot = sel;
            { u16 a, b, c, d; g_menu_slot_used = SramSlotInfo(sel, &a, &b, &c, &d); }
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
        cursor(6, sel < 3 ? 44 + sel * 40 : sel == 3 ? 19 * 8 + 4 : sel == 4 ? 22 * 8 + 4 : 26 * 8);
        spr_end();
        SYS_doVBlankProcess();
    }
}
