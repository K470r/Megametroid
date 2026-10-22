/* MegaMetroid — cola de sonido (ver sound.h). */
#include "sound.h"


#define QLEN 16
static u16 q_kind[QLEN], q_id[QLEN];
static u16 q_head, q_tail;
u16 snd_last_kind, snd_last_id;

void snd_request(u16 kind, u16 id)
{
    u16 n = (q_tail + 1) & (QLEN - 1);
    if (n == q_head) return;                         /* cola llena: se descarta, como en el SNES */
    q_kind[q_tail] = kind;
    q_id[q_tail] = id;
    q_tail = n;
}

/* Aquí irá el controlador: efecto `id` de la biblioteca SND_LIB(kind) del SNES. */
static void play_sfx(u16 kind, u16 id)
{
    (void) kind;
    (void) id;
}

void snd_init(void)
{
    q_head = q_tail = 0;
}

void snd_frame(void)
{
    while (q_head != q_tail)
    {
        snd_last_kind = q_kind[q_head];
        snd_last_id = q_id[q_head];
        play_sfx(snd_last_kind, snd_last_id);
        q_head = (q_head + 1) & (QLEN - 1);
    }
}
