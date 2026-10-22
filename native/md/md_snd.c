/* MegaMetroid — sonido de la Genesis (docs/40).
 *
 * Por ahora solo las voces de la intro, con el controlador PCM de SGDK (un canal, en el Z80). El motor pide la música
 * como el SNES (HandleMusicQueue en src/door.c): music_upload(conjunto) y music_play(pista). Las voces son la pista 5
 * de los conjuntos $3F ("The last Metroid is in captivity") y $42 ("The galaxy is at peace").
 *
 * El Z80 lee la muestra de la ROM por el bus del 68000: mientras suena, el vblank (que hace DMA) le pide el bus
 * (md_vblank_run, md_engine.c), como el controlador de SGDK durante sus DMA. */
#include <genesis.h>

extern const u8 kVoiceCaptivity[], kVoicePeace[];   /* gen/md_voice.s (native/tools/mkvoice.py) */
extern const u32 kVoiceCaptivityLen, kVoicePeaceLen;

static u8 snd_set;                                /* conjunto de música cargado (music_upload) */
volatile u16 md_snd_frames;                       /* vblanks que le quedan a la muestra: el vblank pide el bus */

void md_snd_init(void) {
  SND_PCM_loadDriver(TRUE);
}

void music_upload(u16 i) { snd_set = (u8)i; }

static void play(const u8 *s, u32 len) {
  if (!len) return;                               /* sin los WAV (mkvoice.py): sin voz */
  md_snd_frames = (u16)(len * 60 / 16000 + 2);
  SND_PCM_startPlay(s, len, SOUND_PCM_RATE_16000, SOUND_PAN_CENTER, FALSE);
}

void music_play(u8 t) {
  if (t == 5 && snd_set == 0x3F) { play(kVoiceCaptivity, kVoiceCaptivityLen); return; }
  if (t == 5 && snd_set == 0x42) { play(kVoicePeace, kVoicePeaceLen); return; }
  if (t == 0 && md_snd_frames) { SND_PCM_stopPlay(); md_snd_frames = 0; }   /* pista 0: silencio */
}
