/*
 * MegaMetroid — secuencia de título para la Genesis (docs/36).
 *
 * No se emula el modo 7: todo viene pre-renderizado (make_title_data.py) y una línea de tiempo
 * grabada del SNES real dice, cuadro a cuadro, qué escena se ve, su scroll, el brillo, los sprites,
 * la paleta y el degradado. Aquí solo se copian datos:
 *   - Tomas 1-3 (acercamientos fijos con paneo): imagen ancha/alta; se dibujan por columnas/filas
 *     a medida que avanza el scroll (plano B).
 *   - Alejamiento: 16 tamaños en dos bancos de VRAM (plano B = banco 0, plano A = banco 1); el
 *     siguiente se carga en el banco libre mientras se ve el actual, y se cambia de plano.
 *   - Degradado del título (HDMA de COLDATA en el SNES): interrupción horizontal cada 4 líneas
 *     que cambia el color de fondo (cada línea costaría ~35 % del cuadro).
 *   - Paleta: copia de la paleta del SNES en RAM, actualizada con los cambios grabados (pantallas
 *     del laboratorio, brillo del logo); cada cuadro se convierte a CRAM con el brillo (fundidos).
 * Todas las transferencias al VDP se hacen justo después del vblank.
 */
#include "title.h"
#include "front.h"              /* mm_color5to3 */

#define TB           TILE_USER_INDEX
/* los mapas de los tamaños vienen con la base de tiles ya sumada (make_title_data.py, TILE_BASE) */
_Static_assert(TILE_USER_INDEX == 16, "make_title_data.py (TILE_BASE) supone TILE_USER_INDEX = 16");
#define BANK_TILES   576
#define SPR_BASE     (TB + 2 * BANK_TILES)
#define PW           64          /* plano de 64x32 celdas (mismo esquema de VRAM que el juego) */
#define PH           32
/* Bytes que se cargan por cuadro (tiles + mapa de los tamaños). Medido en Genesis Plus GX: las
   transferencias empiezan en la línea 225 (espera propia del vblank) y 3200 bytes terminan antes de
   la línea 0 junto con la paleta, los sprites y el scroll. make_title_data.py usa el mismo valor. */
#define FRAME_BYTES  3200
#define CHUNK_TILES  (FRAME_BYTES / 32)
/* Los tiles de las escenas y los mapas de los tamaños vienen comprimidos con LZ4W (make_title_data.py: bloques de
   FRAME_BYTES y mitades de mapa): cada paso descomprime lo que se va a transferir en este búfer y la lista de DMA
   lo copia desde ahí en el vblank. En un cuadro va a lo sumo un bloque de tiles y una mitad de mapa. El búfer es
   RAM del juego (enemy_ram7800, 8 KB): mientras se ve el título el juego no corre y al empezar una partida sus
   variables se ponen como al encender (md_main.c, reset_game_vars). */
extern u8 enemy_ram7800[];
#define ZBUF_TILES   (enemy_ram7800)
#define ZBUF_MAP     (enemy_ram7800 + FRAME_BYTES)
_Static_assert(FRAME_BYTES + 14 * 64 * 2 <= 8192, "el búfer del título no entra en enemy_ram7800");
/* Música (src/platform/md_snd.c, docs/55): como el SNES, el conjunto $03 con la pista 5 (CinematicFunctionOpening,
   $8B:9B68) desde el cuadro en que el SNES la escribe en el puerto (g_title_music_frame), y la 6 (el menú de
   partidas) al terminar de fundir a negro con START (HandleCinematicsTransitions_1, $8B:9A83). music_data_index:
   HandleMusicQueue lo pone al subir el conjunto. */
extern u16 music_data_index;
void music_upload(u16 i);
void music_play(u8 t);
/* Chillidos del bebé metroide: el objeto de modo 7 del bebé (addr_kMode7ObjBabyMetroidInTitle $8B:A355, lo crea
   CinematicFunctionOpening) ejecuta PlayBabyMetroidCry1/2/3 ($8B:A25B/A263/A26B): QueueSfx3_Max6($23/$26/$27), que
   HandleSoundEffects escribe en el puerto del SPC en el mismo cuadro. g_title_cry_frame: esos cuadros de la línea de
   tiempo (make_title_data.py, cry_frames); se tocan con sfx_play (src/platform/md_snd.c), como los del juego.
   Ni saltar la intro ni salir con START tocan el objeto (MaybeSkipCinematics $8B:9A48, CinematicFunc_Func10
   $8B:9F52): los chillidos siguen la línea de tiempo y el que suena no se corta. */
void sfx_play(u16 j, u8 id);
static u16 cry_i;
static void title_cries(u16 cur)
{
    while (cry_i < g_title_cry_n && g_title_cry_frame[cry_i] <= cur)
    {
        if (g_title_cry_frame[cry_i] == cur) sfx_play(2, g_title_cry_sfx[cry_i]);   /* biblioteca 3 */
        cry_i++;
    }
}
static void title_music_reset(void)
{
    music_data_index = 0x03;
    music_upload(0x03);
    cry_i = 0;
}
static const u8 *unpack_tiles(const TitleScene *sc, u16 done)    /* el bloque que empieza en el tile done */
{
    lz4w_unpack(sc->tiles + sc->tile_blk[done / CHUNK_TILES], ZBUF_TILES);
    return ZBUF_TILES;
}
#define MAP_HALF     (14 * PW)   /* medio mapa de un tamaño: 14 filas de 64 celdas (palabras) */

/* Memoria de trabajo del título: se pide al montón y se libera al salir, así el juego (que
   necesita casi toda la RAM para el level data del SNES) no la pierde. */
#define POOL_WORDS 1024
#define MAX_SEG 64
typedef struct {
    u16 snes_pal[256];
    u16 cram[64];
    u16 bright_lut[16][8];
    u16 seg_first[MAX_SEG];
    u8 seg_scene[MAX_SEG];
    s16 slot_col[PW], slot_row[PH];
    u16 pool[POOL_WORDS];
    struct { const void *src; u16 dst, len, step; } dl[48];
} TitleMem;
static TitleMem *M;
#define snes_pal (M->snes_pal)
#define cram (M->cram)
#define bright_lut (M->bright_lut)
#define seg_first (M->seg_first)
#define seg_scene (M->seg_scene)
#define slot_col (M->slot_col)
#define slot_row (M->slot_row)
#define pool (M->pool)
#define dl (M->dl)
static u8 cur_grad = 0xFE, want_grad = 0xFF;
static const u16 *hint_cols;             /* degradado en uso (lo lee la interrupción) */
static const TitleFrame *tl;
volatile u16 mm_title_cur;
volatile u16 mm_title_vc;                /* arnés: línea del VDP al empezar y al terminar las transferencias */
volatile u16 mm_title_vc_end;



/* bancos: escena residente y carga en curso */
static s8 resident[2];
static struct { s8 scene; u8 bank; u16 done; u8 phase; } job;

/* tramos de la línea de tiempo con la misma escena (para planificar las cargas sin recorrerla) */
static u16 nseg;

static inline const TitleScene *S(u16 s) { return &g_title_scenes[s]; }
/* banco 0 = plano B (tomas y tamaños pares), banco 1 = plano A (tamaños impares). */
static inline u16 bank_of(u16 s) { return s >= 4 ? ((s - 4) & 1) : 0; }
static inline VDPPlane plane_of(u16 bank) { return bank ? BG_A : BG_B; }
static inline u16 plane_addr(u16 bank) { return bank ? VDP_getBGAAddress() : VDP_getBGBAddress(); }
static inline u16 tile_base(u16 s) { return (s <= 3) ? TB : TB + bank_of(s) * BANK_TILES; }

static inline u16 conv(u16 c, u16 b)
{
    const u16 *l = bright_lut[b];
    return (l[mm_color5to3[(c >> 10) & 31]] << 9) | (l[mm_color5to3[(c >> 5) & 31]] << 5) | (l[mm_color5to3[c & 31]] << 1);
}

static inline u16 sub5(u16 c, u16 s)
{
    s16 r = (c & 31) - (s & 31), g = ((c >> 5) & 31) - ((s >> 5) & 31), b = ((c >> 10) & 31) - ((s >> 10) & 31);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return r | (g << 5) | (b << 10);
}

/* ------------------------------------------------------------------ degradado */
HINTERRUPT_CALLBACK title_hint(void)
{
    u16 line = (*(vu16 *) 0xC00008) >> 8;
    *(vu32 *) VDP_CTRL_PORT = VDP_WRITE_CRAM_ADDR(0);
    *(vu16 *) VDP_DATA_PORT = hint_cols[line < 221 ? line + 2 : 223];   /* centro del bloque siguiente */
}

/* Antes del vblank: elige el degradado (tablas ya en colores de CRAM, en la ROM). */
static inline void prep_grad(u8 g)
{
    want_grad = g;
}

/* En el vblank: activa el degradado preparado (o apaga la interrupción). */
static void apply_grad(void)
{
    u8 g = want_grad;
    if (g == cur_grad) return;
    if (g == 0xFF)
        VDP_setHInterrupt(FALSE);
    else
    {
        hint_cols = (const u16 *) (ttl_grad + g * 448);
        if (cur_grad == 0xFF || cur_grad == 0xFE)
        {
            SYS_setHIntCallback(title_hint);
            VDP_setHIntCounter(3);                  /* cada 4 líneas */
            VDP_setHInterrupt(TRUE);
        }
    }
    cur_grad = g;
}

/* ------------------------------------------------------------------ dibujo */
/* Todo lo que va al VDP se prepara antes del vblank (lista de DMA + búfer) y en el vblank solo se
   transfiere: así el trabajo del vblank es corto y no se pierden cuadros. */
static u16 pool_n;
static u16 ndl;
static bool clear_b;
static u16 baby_cur = 0xFF;

static inline void dma_add(const void *src, u16 dst, u16 len, u16 step)
{
    if (ndl < 48) { dl[ndl].src = src; dl[ndl].dst = dst; dl[ndl].len = len; dl[ndl].step = step; ndl++; }
}

static void run_dl(void)
{
    if (clear_b) { VDP_clearPlane(BG_B, TRUE); clear_b = FALSE; }
    for (u16 i = 0; i < ndl; i++) DMA_doDma(DMA_VRAM, (void *) dl[i].src, dl[i].dst, dl[i].len, dl[i].step);
    ndl = 0;
    pool_n = 0;
}

static bool prep_col(u16 s, s16 c)          /* toma con paneo: columna c de la imagen */
{
    if (pool_n + PH > POOL_WORDS) return FALSE;
    const TitleScene *sc = S(s);
    u16 *buf = pool + pool_n;
    pool_n += PH;
    u16 base = tile_base(s);
    if (c >= 0 && c < (s16) sc->w)
    {
        const u16 *src = sc->map + mulu(c, sc->h);
        for (u16 y = 0; y < PH; y++)
        {
            u16 w = y < sc->h ? src[y] : 0;
            buf[y] = w ? w + base : 0;
        }
    }
    else
        for (u16 y = 0; y < PH; y++) buf[y] = 0;
    dma_add(buf, plane_addr(0) + ((c & (PW - 1)) << 1), PH, PW * 2);
    slot_col[c & (PW - 1)] = c;
    return TRUE;
}

static bool prep_rows(u16 s, u16 bank, s16 r, u16 n)   /* n filas desde r (ancho completo del plano) */
{
    if (pool_n + n * PW > POOL_WORDS) return FALSE;
    const TitleScene *sc = S(s);
    u16 *buf = pool + pool_n;
    pool_n += n * PW;
    u16 base = tile_base(s);
    for (u16 k = 0; k < n; k++, r++)
    {
        u16 *d = buf + k * PW;
        const u16 *src = sc->map + mulu(r, sc->w);
        bool in = r >= 0 && r < (s16) sc->h;
        for (u16 x = 0; x < PW; x++)
        {
            u16 w = (in && x < sc->w) ? src[x] : 0;
            d[x] = w ? w + base : 0;
        }
        slot_row[r & (PH - 1)] = r;
        /* filas consecutivas del plano: una sola DMA mientras no den la vuelta */
        if (k == 0 || ((r & (PH - 1)) == 0))
            dma_add(d, plane_addr(bank) + ((r & (PH - 1)) * PW * 2), 0, 2);
        dl[ndl - 1].len += PW;
    }
    return TRUE;
}

/* Tomas: mantener dibujado lo visible según el scroll (como mucho 4 columnas/filas por cuadro) */
static void stream_shot(u16 s, s16 hs, s16 vs)
{
    const TitleScene *sc = S(s);
    u16 budget = 4;
    if (sc->col_major)
    {
        s16 c0 = hs >> 3;
        for (s16 c = c0; c <= c0 + 32 && budget; c++)
            if (slot_col[c & (PW - 1)] != c && prep_col(s, c)) budget--;
    }
    else
    {
        s16 r0 = vs >> 3;
        for (s16 r = r0; r <= r0 + 28 && budget; r++)
            if (slot_row[r & (PH - 1)] != r && prep_rows(s, 0, r, 1)) budget--;
    }
}

/* ------------------------------------------------------------------ carga de escenas */
static void start_job(u16 s)
{
    job.scene = s;
    job.bank = bank_of(s);
    job.done = 0;
    job.phase = 0;
    resident[job.bank] = -1;
    if (s <= 3)
    {
        resident[1] = -1;                    /* las tomas ocupan los dos bancos */
        for (u16 i = 0; i < PW; i++) slot_col[i] = -32768;
        for (u16 i = 0; i < PH; i++) slot_row[i] = -32768;
    }
}

/* Primer cuadro en que se verá la escena (para dibujar de antemano la parte visible). */
static const TitleFrame *first_frame_of(u16 s, u16 cur)
{
    for (u16 i = 0; i < nseg; i++)
        if (seg_scene[i] == s && seg_first[i] >= cur) return &tl[seg_first[i]];
    return &tl[cur];
}

/* Un paso de la carga (se prepara aquí, se transfiere en el vblank). */
static void prep_job(u16 cur)
{
    const TitleScene *sc = S(job.scene);
    if (job.scene >= 4)
    {
        /* tamaños: tiles y después el mapa (28 filas x 64 celdas, con la base de tiles ya sumada y
           las columnas 32-63 vacías), en dos mitades de 14 filas copiadas de una vez desde la ROM.
           Todo lo que quepa en FRAME_BYTES por cuadro: una transferencia larga cuesta mucho menos
           que muchas cortas. */
        u16 budget = FRAME_BYTES;
        for (;;)
        {
            if (job.phase == 0)
            {
                u16 n = sc->num_tiles - job.done;
                if (n > budget / 32) n = budget / 32;
                if (!n) return;
                if (n > CHUNK_TILES) n = CHUNK_TILES;   /* (los tiles van primero en el cuadro: es un bloque entero) */
                dma_add(unpack_tiles(sc, job.done), (tile_base(job.scene) + job.done) * 32, n * 16, 2);
                budget -= n * 32;
                job.done += n;
                if (job.done >= sc->num_tiles) { job.phase = 1; job.done = 0; }
                continue;
            }
            if (budget < MAP_HALF * 2) return;
            lz4w_unpack((const u8 *) sc->map + sc->map_blk[job.done / 14], ZBUF_MAP);
            dma_add(ZBUF_MAP, plane_addr(job.bank) + job.done * PW * 2, MAP_HALF, 2);
            budget -= MAP_HALF * 2;
            job.done += 14;
            if (job.done >= 28) break;
        }
        if (job.scene == g_title_baby_scene) baby_cur = 0;     /* los tiles del bebé: cuadro 0 */
        resident[job.bank] = job.scene;
        job.scene = -1;
        return;
    }
    if (job.phase == 0)
    {
        u16 n = sc->num_tiles - job.done;
        if (n > CHUNK_TILES) n = CHUNK_TILES;
        dma_add(unpack_tiles(sc, job.done), (tile_base(job.scene) + job.done) * 32, n * 16, 2);
        job.done += n;
        if (job.done >= sc->num_tiles) { job.phase = 1; job.done = 0; }
        return;
    }
    /* tomas: dibujar la ventana visible del primer cuadro (el plano A no se limpia: sus
       columnas 32-63 siempre están vacías y se oculta con el scroll) */
    const TitleFrame *f0 = first_frame_of(job.scene, cur);
    if (sc->col_major)
    {
        s16 c0 = (f0->hs >> 3) + job.done;
        for (u16 k = 0; k < 12 && job.done <= 32; k++, c0++, job.done++) prep_col(job.scene, c0);
        if (job.done <= 32) return;
    }
    else
    {
        s16 r0 = (f0->vs >> 3) + job.done;
        u16 n = 29 - job.done;
        if (n > 8) n = 8;
        prep_rows(job.scene, 0, r0, n);
        job.done += n;
        if (job.done < 29) return;
    }
    resident[job.bank] = job.scene;
    job.scene = -1;
}

/* Elige qué cargar: la próxima escena de la línea de tiempo que no está en su banco, si ese banco
   no se está viendo (las tomas solo mientras la pantalla está sin fondo). */
static inline u16 bank_mask(u16 s) { return s <= 3 ? 3 : 1 << bank_of(s); }

static void schedule(u16 cur, u16 shown)
{
    if (job.scene >= 0) return;
    u16 i = 0;
    while (i + 1 < nseg && seg_first[i + 1] <= cur) i++;
    u16 reserved = shown ? bank_mask(shown) : 0;       /* bancos que todavía se van a ver */
    for (; i < nseg && seg_first[i] < cur + 600; i++)
    {
        u16 s = seg_scene[i];
        if (!s) continue;
        u16 m = bank_mask(s);
        if (resident[bank_of(s)] == (s8) s) { reserved |= m; continue; }
        if (m & reserved) return;                      /* esperar a que ese banco quede libre */
        start_job(s);
        return;
    }
}

/* ------------------------------------------------------------------ paleta y sprites */
static void build_cram(const TitleFrame *f, u16 shown, u8 src3, u8 src2)
{
    u16 b = f->bright;
    for (u16 i = 0; i < 64; i++) cram[i] = 0;
    if (shown)
    {
        const u8 *pt = S(shown)->pal;
        for (u16 p = 0; p < 4; p++)
            for (u16 k = 1; k < 16; k++)
            {
                u8 idx = pt[p * 16 + k];
                if (idx == 0xFF) continue;
                u16 c = snes_pal[idx];
                if (f->sub) c = sub5(c, f->sub);
                cram[p * 16 + k] = conv(c, b);
            }
    }
    if (src3 != 0xFF)                        /* los sprites (si hay) usan PAL3 */
        for (u16 k = 1; k < 16; k++) cram[48 + k] = conv(snes_pal[src3 + k], b);
    if (src2 != 0xFF)
        for (u16 k = 1; k < 16; k++) cram[32 + k] = conv(snes_pal[src2 + k], b);
    cram[0] = (f->grad != 0xFF) ? ((const u16 *) (ttl_grad + f->grad * 448))[0] : 0;
}

static u16 build_sprites(u16 set, u8 *src3, u8 *src2)
{
    const u8 *p = ttl_spr_sets + g_title_spr_set_off[set];
    u16 n = *(const u16 *) p;
    *src3 = p[2];
    *src2 = p[3];
    const u16 *r = (const u16 *) (p + 4);
    if (n > 80) n = 80;
    for (u16 i = 0; i < n; i++, r += 4)
    {
        VDPSprite *sp = &vdpSpriteCache[i];
        sp->y = r[0];
        sp->size = r[1] >> 8;
        sp->link = i + 1;
        sp->attribut = r[2] + SPR_BASE;
        sp->x = r[3];
    }
    if (!n)
    {
        VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, 0);
        n = 1;
    }
    vdpSpriteCache[n - 1].link = 0;
    return n;
}

static void apply_pdelta(const TitleFrame *f)
{
    const u16 *d = (const u16 *) (ttl_pdelta + f->pd_start * 4);
    for (u16 i = 0; i < f->pd_n; i++, d += 2) snes_pal[d[0]] = d[1];
}

/* ------------------------------------------------------------------ bucle */
static void reset_state(void)
{
    resident[0] = resident[1] = -1;
    job.scene = -1;
    baby_cur = 0xFF;
    for (u16 i = 0; i < 256; i++) snes_pal[i] = 0;
}

void title_run(void)
{
    tl = (const TitleFrame *) ttl_timeline;
    M = MEM_alloc(sizeof(TitleMem));
    if (!M) return;
    for (u16 b = 0; b < 16; b++)
        for (u16 c = 0; c < 8; c++) bright_lut[b][c] = (c * b + 7) / 15;
    SYS_disableInts();
    VDP_setEnable(FALSE);
    VDP_setScreenWidth256();
    VDP_setPlaneSize(PW, PH, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setBackgroundColor(0);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setColors(0, cram, 64, CPU);
    VDP_loadTileData((const u32 *) ttl_spr_tiles, SPR_BASE, g_title_spr_tiles, CPU);
    VDP_setEnable(TRUE);
    SYS_enableInts();
    reset_state();
    title_music_reset();
    nseg = 0;
    for (u16 f = 0; f < g_title_frames && nseg < MAX_SEG; f++)
        if (!nseg || tl[f].scene != seg_scene[nseg - 1])
        {
            seg_first[nseg] = f;
            seg_scene[nseg++] = tl[f].scene;
        }
    cur_grad = 0xFE;
    hint_cols = (const u16 *) ttl_grad;
    want_grad = 0xFF;
    apply_grad();

    u16 cur = 0, prev_pad = 0xFFFF;
    s16 dim = 15;                  /* tope de brillo para los fundidos propios (saltar / salir) */
    u16 mode = 0;                  /* 0 normal, 1 saltando (bajando), 2 subiendo tras el salto, 3 saliendo */
    u16 fade_t = 0;
    for (;;)
    {
        u16 pad = JOY_readJoypad(JOY_1);
        u16 pressed = pad & ~prev_pad;
        prev_pad = pad;
        const TitleFrame *f = &tl[cur];
        mm_title_cur = cur;
        apply_pdelta(f);
        if (cur >= g_title_music_frame) music_play(5);      /* (la misma pista no vuelve a empezar) */
        title_cries(cur);                                   /* (una vez por cuadro de la línea de tiempo) */

        if (pressed & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C))
        {
            if (cur >= g_title_skip_frame && mode == 0) mode = 3;          /* START en el título */
            else if (mode == 0) mode = 1;                                   /* saltar la intro */
        }

        u16 shown = (f->scene && resident[bank_of(f->scene)] == (s8) f->scene) ? f->scene : 0;
        if (f->scene > 3 && !shown && mode == 0)
        {
            /* el tamaño siguiente no terminó de cargar: seguir con el que está */
            for (u16 b = 0; b < 2; b++)
                if (resident[b] >= 4) shown = resident[b];
        }
        u8 src3, src2;
        u16 ns = build_sprites(f->sprset, &src3, &src2);
        TitleFrame ff = *f;
        if (ff.bright > dim) ff.bright = dim;
        prep_grad(shown ? f->grad : 0xFF);
        build_cram(&ff, shown, src3, src2);
        schedule(cur, shown);
        if (shown && shown <= 3) stream_shot(shown, f->hs, f->vs);
        if (job.scene >= 0) prep_job(cur);

        /* Espera propia del vblank: SYS_doVBlankProcess() lee los mandos y vacía su cola dentro del
           vblank y se come ~10 líneas; así las transferencias empiezan en la línea 224. */
        {
            vu16 *st = (vu16 *) VDP_CTRL_PORT;
            while (*st & 8);
            while (!(*st & 8));
        }
        /* ---- vblank (sin interrupciones: la del degradado escribe en el VDP) */
        SYS_disableInts();
        mm_title_vc = (*(vu16 *) 0xC00008) >> 8;
        apply_grad();
        PAL_setColors(0, cram, 64, CPU);
        VDP_updateSprites(ns, CPU);
        if (shown)
        {
            u16 vb = bank_of(shown);
            VDP_setHorizontalScroll(plane_of(vb), -f->hs);
            VDP_setVerticalScroll(plane_of(vb), f->vs);
            VDP_setHorizontalScroll(plane_of(vb ^ 1), -256);
            VDP_setVerticalScroll(plane_of(vb ^ 1), 0);
        }
        else
        {
            VDP_setHorizontalScroll(BG_A, -256);
            VDP_setHorizontalScroll(BG_B, -256);
        }
        if (shown == g_title_baby_scene && f->baby != 0xFF && f->baby != baby_cur && baby_cur != 0xFF)
        {
            /* bebé metroide: 4 tiles del cuadro de animación del SNES */
            u16 n = g_title_baby_ntiles;
            dma_add(ttl_baby + f->baby * n * 32, (tile_base(shown) + g_title_baby_tile) * 32, n * 16, 2);
            baby_cur = f->baby;
        }
        run_dl();
        mm_title_vc_end = (*(vu16 *) 0xC00008) >> 8;

        SYS_enableInts();
        JOY_update();

        /* ---- avance */
        if (mode == 3)
        {
            if ((++fade_t & 1) && --dim <= 0) break;   /* fundido lento, como el SNES */
        }
        else if (mode == 1)
        {
            if (--dim <= 0)
            {
                /* a negro: saltar al título con su paleta y volver a subir el brillo */
                for (u16 i = cur + 1; i <= g_title_skip_frame; i++) apply_pdelta(&tl[i]);
                cur = g_title_skip_frame;
                resident[0] = resident[1] = -1;
                job.scene = -1;
                baby_cur = 0xFF;
                clear_b = TRUE;              /* el plano B puede tener una toma (ocupa las 64 columnas) */
                mode = 2;
                continue;
            }
            continue;
        }
        else if (mode == 2 && ++dim >= 15)
        {
            dim = 15;
            mode = 0;
        }
        if (cur + 1 < g_title_frames) cur++;
        else
        {
            /* la grabación termina cuando el SNES, sin tocar nada, se va a las demos (y ya fundió
               a negro): aquí vuelve a empezar la secuencia */
            cur = 0;
            reset_state();
            title_music_reset();
        }
    }
    music_play(6);                  /* el menú de partidas */
    /* salida: dejar el VDP como lo espera el juego */
    want_grad = 0xFF;
    apply_grad();
    SYS_setHIntCallback(NULL);
    SYS_disableInts();
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, 0);   /* sin los sprites del logo */
    VDP_updateSprites(1, CPU);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_setEnable(TRUE);
    SYS_enableInts();
    MEM_free(M);
    M = NULL;
}
