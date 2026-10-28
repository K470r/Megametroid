/* MegaMetroid — sonido de la Genesis (docs/55).
 *
 * El motor pide el sonido como el SNES: la cola de música (HandleMusicQueue, src/core/door.c) llama a
 * music_upload(conjunto) y music_play(pista), y la de efectos (HandleSoundEffects, src/core/sfx.c) a
 * sfx_play(biblioteca, efecto). Aquí se tocan con el controlador XGM2 de SGDK (en el Z80):
 *   - música: las pistas convertidas en el PC (tools/snd/mksnd.py: N-SPC del decomp -> FM y PSG -> XGM2); las voces
 *     de la intro son pistas PCM en el canal 1;
 *   - efectos: PCM grabado del N-SPC, un canal por biblioteca (1, 2 y 3 del XGM2), con las reglas de prioridad del
 *     SPC (Sfx1/2/3_HandleCmdFromSnes, sm/src/spc_player.c) y su lazo si no terminan;
 *   - donde no hay música convertida (desde la llegada a Zebes) suenan solo los efectos de Samus y de las puertas.
 * Todo está en la parte fija de la ROM (debajo de 0x380000): el Z80 no ve qué página del SSF2 está puesta.
 *
 * Mega CD (docs/48): si hay música en CD, music_play se la pasa a snd_cd_play (todavía no: devuelve 0). */
#include <genesis.h>

typedef struct {                                   /* kSndMusic (md_snd_data.s) */
  u8 song, track, kind, pad;                       /* kind: 1 XGM2, 2 PCM (las voces) */
  const u8 *data;
  u32 len;
} SndMusic;
typedef struct {                                   /* kSndSfx (md_snd_data.s) */
  const u8 *pcm;                                   /* la parte que suena una vez */
  u32 len;
  const u8 *loop;                                  /* el lazo (los efectos que no terminan solos) */
  u32 loop_len;
  u16 frames;                                      /* cuadros de la parte sin lazo */
  u8 lib, id, song, flags, pad0, pad1;             /* song: grabado con ese conjunto (0: los instrumentos de base) */
} SndSfx;
enum { SF_PRIO = 1, SF_SV2 = 2, SF_SV0 = 4, SF_ZEBES = 8, SF_HALF = 16 };

extern const SndMusic kSndMusic[];
extern const SndSfx kSndSfx[];

extern u16 music_data_index;                       /* el conjunto cargado en el SPC (variable del juego, 7E:07F3): una
                                                      partida que empieza de un estado guardado ya lo trae puesto */
static u8 snd_set = 0xFF;                          /* el conjunto que se vio (music_upload o music_data_index) */
static u8 snd_track;                               /* la pista que suena (port_to_snes[0]) */
static u8 snd_full;                                /* el conjunto tiene música convertida: suenan todos los efectos */

volatile u16 md_snd_frames;                        /* 1: el XGM2 está cargado (el vblank protege sus DMA) */
u8 md_snd_pending;                                 /* hay un lazo de efecto por empezar (md_snd_frame) */

typedef struct {
  const SndSfx *cur;                               /* el que suena (cur_sound) */
  u32 end;                                         /* vtimer en que termina la parte sin lazo */
  u8 prio, looping, loop_wait;
} SfxLib;
static SfxLib libs[3];
static u8 sv2;                                     /* biblioteca 3: some_value == 2 (solo el efecto 1 lo corta) */
extern volatile u32 vtimer;

/* el Mega CD (docs/48): devuelve 1 si la pista sale del CD */
static u8 snd_cd_play(u8 song, u8 track) { (void)song; (void)track; return 0; }

void md_snd_init(void) {
  XGM2_loadDriver(TRUE);
  md_snd_frames = 1;
}

/* el vblank, antes y después de sus DMA: el Z80 no lee la ROM mientras tanto (como Z80_setBusProtection de SGDK,
   sin tocar las interrupciones: se llama desde la interrupción) */
void md_snd_protect(u8 on) {
  volatile u16 *bus = (volatile u16 *)0xA11100;
  if (!(*bus & 0x0100)) {                          /* (el bus ya lo tiene el 68000) */
    *(volatile u8 *)0xA00161 = on;                 /* XGM2_IN_DMA: Z80_DRV_VARS + 0x51 */
    return;
  }
  *bus = 0x0100;
  while (*bus & 0x0100);
  *(volatile u8 *)0xA00161 = on;
  *bus = 0x0000;
}

static const SndMusic *find_music(u8 song, u8 track) {
  for (const SndMusic *m = kSndMusic; m->kind; m++)
    if (m->song == song && (m->track == track || !track)) return m;
  return 0;
}

static void sync_set(void) {                       /* el conjunto del juego, si no pasó por music_upload */
  if ((u8)music_data_index == snd_set) return;
  snd_set = (u8)music_data_index;
  snd_full = find_music(snd_set, 0) != 0;
}

void music_upload(u16 i) {                         /* $80:8F0C: APU_UploadBank (el SPC calla) */
  snd_set = (u8)i;
  snd_track = 0;
  snd_full = find_music(snd_set, 0) != 0;
  XGM2_stop();
  XGM2_stopPCM(SOUND_PCM_CH1);
}

void music_play(u8 t) {                            /* Music_HandleCmdFromSnes: la misma pista sigue sonando */
#ifdef MD_SND_MUTE
  return;
#endif
  sync_set();
  if (t == snd_track) return;
  snd_track = t;
  if (snd_cd_play(snd_set, t)) return;
  const SndMusic *m = t ? find_music(snd_set, t) : 0;
  XGM2_stop();
  if (!m || m->kind == 2) XGM2_stopPCM(SOUND_PCM_CH1);
  if (!m) return;
  if (m->kind == 1) XGM2_play(m->data);
  else XGM2_playPCMEx(m->data, m->len, SOUND_PCM_CH1, 15, FALSE, FALSE);
}

static const SndSfx *find_sfx(u8 lib, u8 id) {
  const SndSfx *any = 0;
  for (const SndSfx *s = kSndSfx; s->lib; s++)
    if (s->lib == lib && s->id == id) {
      if (s->song == snd_set) return s;
      if (!any || !s->song) any = s;
    }
  return any;
}

static void lib_play(u16 j, const u8 *p, u32 len, u8 half, u8 loop) {
  XGM2_playPCMEx(p, len, (SoundPCMChannel)j, 15, half, loop);
}

void sfx_play(u16 j, u8 id) {                      /* Sfx1/2/3_HandleCmdFromSnes */
#ifdef MD_SND_MUTE
  return;
#endif
  if (!id || id == 0xFF || j > 2) return;
  sync_set();
  SfxLib *L = &libs[j];
  u8 lib = j + 1;
  u8 active = L->cur && (L->looping || (s32)(vtimer - L->end) < 0);
  if (lib == 1 && id != 1 && id != 2 && active && L->prio) return;
  if (lib == 2 && id != 0x71 && id != 0x7E && active && L->prio) return;
  if (lib == 3 && id != 1 && (sv2 || (id != 2 && active && L->prio))) return;
  const SndSfx *s = find_sfx(lib, id);
  if (s && !snd_full && !(s->flags & SF_ZEBES)) return;   /* en Zebes: solo Samus y puertas */
  if (lib == 3 && s) {
    if (s->flags & SF_SV2) sv2 = 1;
    if (s->flags & SF_SV0) sv2 = 0;
  }
  L->cur = s;
  L->looping = 0;
  L->loop_wait = 0;
  if (!s || (!s->len && !s->loop_len)) {           /* los de silencio (1/2, 0x71...): calla la biblioteca */
    L->cur = 0;
    L->prio = 0;
    XGM2_stopPCM((SoundPCMChannel)j);
    return;
  }
  L->prio = s->flags & SF_PRIO;
  u8 half = (s->flags & SF_HALF) != 0;
  L->end = vtimer + s->frames;
  if (s->len) lib_play(j, s->pcm, s->len, half, FALSE);
  if (s->loop_len) {
    if (!s->len) { lib_play(j, s->loop, s->loop_len, half, TRUE); L->looping = 1; }
    else { L->loop_wait = 1; md_snd_pending = 1; }
  }
}

/* una vez por cuadro de la lógica, solo si hay un lazo por empezar: la parte sin lazo terminó, sigue el lazo */
void md_snd_frame(void) {
  md_snd_pending = 0;
  for (u16 j = 0; j < 3; j++) {
    SfxLib *L = &libs[j];
    if (!L->loop_wait) continue;
    if ((s32)(vtimer - L->end) < 0) { md_snd_pending = 1; continue; }
    L->loop_wait = 0;
    L->looping = 1;
    lib_play(j, L->cur->loop, L->cur->loop_len, (L->cur->flags & SF_HALF) != 0, TRUE);
  }
}
