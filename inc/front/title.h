/* MegaMetroid — secuencia de título (logo de arranque, letras rojas, tomas del laboratorio,
   alejamiento y pantalla de título). Datos: tools/genesis_convert/make_title_data.py. */
#ifndef TITLE_H
#define TITLE_H
#include <genesis.h>

typedef struct {
    u16 num_tiles;
    u16 w, h;              /* celdas */
    s16 x0, y0;            /* origen de la imagen en coordenadas pantalla + scroll del SNES */
    u16 col_major;         /* 1 = mapa por columnas (tomas con paneo horizontal) */
    const u8 *tiles;       /* LZ4W en bloques independientes de 100 tiles (tile_blk: desplazamiento de cada uno) */
    const u16 *tile_blk;
    const u16 *map;        /* palabras de tilemap sin la base de tiles (0 = vacío); en los tamaños, LZ4W por mitad */
    const u16 *map_blk;    /* tamaños: desplazamiento (bytes) de cada mitad comprimida; tomas: 0 (mapa sin comprimir) */
    const u8 *pal;         /* [4][16]: índice de la paleta SNES para cada color (0xFF = sin usar) */
} TitleScene;

/* Un cuadro de la línea de tiempo (grabada del SNES real). */
typedef struct {
    u8 bright;             /* 0-15 (0 = negro) */
    u8 scene;              /* 0 = sin fondo, 1-3 tomas, 4.. tamaños del alejamiento (el último = título) */
    s16 hs, vs;            /* scroll dentro de la imagen de la escena */
    u8 sprset;             /* conjunto de sprites */
    u8 grad;               /* tabla de degradado (0-15) o 0xFF */
    u16 sub;               /* color restado al fondo (BGR555) o 0 */
    u16 pd_start, pd_n;    /* cambios de paleta de este cuadro */
    u16 baby;              /* cuadro de animación del bebé metroide (0-2) o 0xFF */
} TitleFrame;

extern const TitleScene g_title_scenes[];
extern const u16 g_title_num_scenes, g_title_tile_base;
extern const u16 g_title_baby_scene, g_title_baby_tile, g_title_baby_ntiles;
extern const u16 g_title_frames, g_title_skip_frame;
extern const u16 g_title_spr_tiles;
extern const u16 g_title_spr_set_off[];
extern const u8 ttl_timeline[], ttl_pdelta[], ttl_spr_tiles[], ttl_spr_sets[], ttl_grad[], ttl_baby[];

/* Corre la secuencia de título; vuelve cuando el jugador pulsa START en la pantalla de título. */
void title_run(void);

#endif
