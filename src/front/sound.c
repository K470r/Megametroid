/* MegaMetroid — pedidos de efectos de las pantallas de antes del juego (ver sound.h). */
#include "sound.h"

void sfx_play(u16 j, u8 id);                     /* md_snd.c: Sfx1/2/3_HandleCmdFromSnes, con sus prioridades */

u16 snd_last_kind, snd_last_id;

/* El SNES encola el pedido y HandleSoundEffects ($82:89EF) lo escribe al SPC en el mismo cuadro (la cola de cada
   biblioteca está vacía en los menús): aquí va directo a sfx_play. */
void snd_request(u16 kind, u16 id)
{
    snd_last_kind = kind;
    snd_last_id = id;
    sfx_play(SND_LIB(kind) - 1, (u8)id);
}
