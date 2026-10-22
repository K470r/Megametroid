/* MegaMetroid — sonido: cola de pedidos del juego hacia el futuro controlador de sonido.
 *
 * El núcleo del SNES pide los efectos con QueueSfx1/2/3_MaxN(id) (bibliotecas de efectos 1, 2 y 3
 * del SPC700; los de Samus están casi todos en la 1). Esas llamadas llegan aquí con el mismo id
 * del SNES, así el controlador de la Genesis (YM2612 + PSG) solo tiene que traducir id -> sonido.
 * Todavía no suena nada: con -DMM_SOUND_DEBUG la línea de información muestra el último pedido
 * (sirve para identificar cada efecto mientras se rehacen para la Genesis).
 */
#ifndef SOUND_H
#define SOUND_H
#include <genesis.h>

#define SND_LIB(kind)   ((kind) >> 8)          /* 1, 2 o 3 */
#define SND_MAXQ(kind)  ((kind) & 0xFF)        /* largo máximo de la cola en el SNES */

void snd_init(void);
void snd_request(u16 kind, u16 id);          /* pedido directo (menús, intro): kind = biblioteca << 8 | cola */
void snd_frame(void);                          /* una vez por cuadro: pasa la cola al controlador */
extern u16 snd_last_kind, snd_last_id;         /* último pedido (depuración) */

#endif
