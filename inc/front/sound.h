/* MegaMetroid — efectos de las pantallas de antes del juego (el menú de partidas y las opciones, src/front/menu.c).
 *
 * El SNES los pide con QueueSfx1/2/3_MaxN(id) (bibliotecas 1, 2 y 3 del SPC700); aquí llegan con el mismo id y van
 * a sfx_play (src/platform/md_snd.c, docs/55), como los del juego.
 */
#ifndef SOUND_H
#define SOUND_H
#include <genesis.h>

#define SND_LIB(kind)   ((kind) >> 8)          /* 1, 2 o 3 */
#define SND_MAXQ(kind)  ((kind) & 0xFF)        /* largo máximo de la cola en el SNES */

void snd_request(u16 kind, u16 id);          /* kind = biblioteca << 8 | largo máximo de la cola en el SNES */
extern u16 snd_last_kind, snd_last_id;         /* último pedido (depuración) */

#endif
