/* MegaMetroid — corre el N-SPC del decomp (sm/src/spc_player.c, el motor de sonido del SPC700 en C) con los datos de
 * tu ROM, en el PC (docs/55). No se distribuye nada: lee sm/sm.smc y escribe en work/.
 *
 *   spcrun rom.smc guion.txt salida.wav estados.bin [ram_]
 *
 * Guion (una orden por línea, en orden de cuadro):
 *   up CONJ        sube el conjunto de música CONJ (como APU_UploadBank($8F:E7E1 + CONJ), $80:8F0C)
 *   p PUERTO VAL   escribe en el puerto de entrada (0 música, 1-3 bibliotecas de efectos), como RtlApuWrite
 *   f N            avanza hasta el cuadro N (cada cuadro: 534 muestras a 32 kHz, como RtlRenderAudio)
 *   ram ARCHIVO    vuelca los 64 KB de la RAM del SPC (muestras BRR, tabla DIR)
 *   noecho         sin eco (los efectos se graban secos)
 *   sfxst ARCHIVO  en cada tick de efectos ($20 de cada $100), la muestra en que pasa y el estado de los 8 canales de
 *                  efectos (ptr, ptr_idx, instr_timer, note, subnote, release_timer, release_flag, legato,
 *                  enable_pitch_slide, disable): con eso se encuentra el lazo de los efectos que no terminan
 *
 * estados.bin: por cuadro, por voz (8): srcn, pitch (u16), volL, volR (s8), env al final (u16), env máximo (u16),
 * kon (subpaso del KON dentro del cuadro + 1, 0 si no hubo), kof (idem), noise; 12 bytes por voz; después 8 bytes:
 * music_ptr_toplevel (la lista de patrones), la pista que suena (port_to_snes[0]), el efecto que suena en cada
 * biblioteca (sfxN.cur_sound) y sus prioridades (bit 0-2: sfx1-3.priority, bit 3: sfx3.some_value == 2). */
#include "spc_player.c"
#include <stdio.h>

static uint8 rom[4 << 20];
static long romlen;
static uint32 lorom(uint32 a) { return ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF); }

typedef struct { uint8 srcn, volL, volR, kon, kof, noise; uint16 pitch, env, envmax; } VState;

int main(int argc, char **argv) {
  if (argc < 5) { fprintf(stderr, "uso: spcrun rom guion wav estados\n"); return 1; }
  FILE *f = fopen(argv[1], "rb");
  romlen = fread(rom, 1, sizeof(rom), f); fclose(f);
  int hdr = romlen & 0x7FFF ? 512 : 0;
  memmove(rom, rom + hdr, romlen - hdr);
  FILE *sc = fopen(argv[2], "r");
  FILE *wav = strcmp(argv[3], "-") ? fopen(argv[3], "wb") : NULL;
  FILE *st = strcmp(argv[4], "-") ? fopen(argv[4], "wb") : NULL;
  if (wav) { uint8 h[44] = {0}; fwrite(h, 1, 44, wav); }
  SpcPlayer *p = SpcPlayer_Create();
  SpcPlayer_Initialize(p);
  SpcPlayer_Upload(p, rom + lorom(0xCF8000));          /* el motor y los efectos: $80:8028 al encender */
  DspRegWriteHistory hist;
  p->reg_write_history = &hist;
  int noecho = 0;
  FILE *sfxst = NULL;
  long frame = 0, nsamp = 0;
  char line[256];
  int16 buf[534 * 2];
  while (fgets(line, sizeof line, sc)) {
    char cmd[16], arg[200]; int a = 0, b = 0;
    if (sscanf(line, "%15s", cmd) != 1 || cmd[0] == '#') continue;
    if (!strcmp(cmd, "up")) {
      sscanf(line, "%*s %i", &a);
      uint32 ptr = lorom(0x8FE7E1) + a;
      uint32 addr = rom[ptr] | rom[ptr + 1] << 8 | rom[ptr + 2] << 16;
      SpcPlayer_Upload(p, rom + lorom(addr));
    } else if (!strcmp(cmd, "p")) {
      sscanf(line, "%*s %i %i", &a, &b);
      p->input_ports[a & 3] = b;
    } else if (!strcmp(cmd, "ram")) {
      sscanf(line, "%*s %199s", arg);
      FILE *r = fopen(arg, "wb"); fwrite(p->ram, 1, 65536, r); fclose(r);
    } else if (!strcmp(cmd, "sfxst")) {
      sscanf(line, "%*s %199s", arg);
      sfxst = fopen(arg, "wb");
    } else if (!strcmp(cmd, "noecho")) {
      noecho = 1;
    } else if (!strcmp(cmd, "f")) {
      sscanf(line, "%*s %i", &a);
      while (frame < a) {
        VState vs[8]; memset(vs, 0, sizeof vs);
        int step = 0;
        for (;;) {                                   /* SpcPlayer_GenerateSamples, por pasos de 64 ciclos */
          if (p->timer_cycles >= 64) {
            hist.count = 0;
            int sfx_tick = p->sfx_timer_accum + (uint8)((p->timer_cycles >> 6) * 0x20) >= 0x100;
            Spc_Loop_Part2(p, p->timer_cycles >> 6);
            if (sfxst && sfx_tick) {
              uint32 at = nsamp + p->dsp->sampleOffset;
              fwrite(&at, 4, 1, sfxst);
              SfxChan *all[8] = {&p->sfx_chans_1[0], &p->sfx_chans_1[1], &p->sfx_chans_1[2], &p->sfx_chans_1[3],
                                 &p->sfx_chans_2[0], &p->sfx_chans_2[1], &p->sfx_chans_3[0], &p->sfx_chans_3[1]};
              for (int i = 0; i < 8; i++) {
                SfxChan *x = all[i];
                uint8 o[11] = {x->ptr, x->ptr >> 8, x->ptr_idx, x->instr_timer, x->note, x->subnote, x->release_timer,
                               x->release_flag, x->legato, x->enable_pitch_slide, x->disable};
                fwrite(o, 1, 11, sfxst);
              }
            }
            Spc_Loop_Part1(p);
            p->timer_cycles &= 63;
            for (uint32 i = 0; i < hist.count; i++) {
              uint8 r = hist.addr[i], v = hist.val[i];
              for (int c = 0; c < 8; c++) {
                if (r == KON && (v >> c & 1)) vs[c].kon = step + 1;
                if (r == KOF && (v >> c & 1)) vs[c].kof = step + 1;
              }
            }
            step++;
            if (noecho) { dsp_write(p->dsp, EVOLL, 0); dsp_write(p->dsp, EVOLR, 0); }
          }
          int n = 534 - p->dsp->sampleOffset;
          if (n > 64 - p->timer_cycles) n = 64 - p->timer_cycles;
          p->timer_cycles += n;
          for (int i = 0; i < n; i++) dsp_cycle(p->dsp);
          for (int c = 0; c < 8; c++)
            if (p->dsp->channel[c].gain > vs[c].envmax) vs[c].envmax = p->dsp->channel[c].gain;
          if (p->dsp->sampleOffset == 534) break;
        }
        for (int c = 0; c < 8; c++) {
          DspChannel *d = &p->dsp->channel[c];
          vs[c].srcn = d->srcn; vs[c].pitch = d->pitch; vs[c].volL = d->volumeL; vs[c].volR = d->volumeR;
          vs[c].env = d->adsrState == 4 && d->gain == 0 ? 0 : d->gain; vs[c].noise = d->useNoise;
        }
        if (st) for (int c = 0; c < 8; c++) {
          uint8 o[12] = {vs[c].srcn, vs[c].pitch, vs[c].pitch >> 8, vs[c].volL, vs[c].volR, vs[c].env, vs[c].env >> 8,
                         vs[c].envmax, vs[c].envmax >> 8, vs[c].kon, vs[c].kof, vs[c].noise};
          fwrite(o, 1, 12, st);
        }
        if (st) {
          uint8 o[8] = {p->music_ptr_toplevel, p->music_ptr_toplevel >> 8, p->port_to_snes[0], p->sfx1.cur_sound,
                        p->sfx2.cur_sound, p->sfx3.cur_sound,
                        (p->sfx1.priority & 1) | (p->sfx2.priority & 1) << 1 | (p->sfx3.priority & 1) << 2 |
                        (p->sfx3.some_value == 2) << 3, 0};
          fwrite(o, 1, 8, st);
        }
        dsp_getSamples(p->dsp, buf, 534);
        if (wav) fwrite(buf, 2, 534 * 2, wav);
        nsamp += 534;
        frame++;
      }
    }
  }
  if (wav) {                                           /* cabecera WAV: 32040 Hz (534 x 60), estéreo, 16 bits */
    uint32 rate = 32040, bytes = nsamp * 4, x;
    fseek(wav, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, wav); x = 36 + bytes; fwrite(&x, 4, 1, wav); fwrite("WAVEfmt ", 1, 8, wav);
    x = 16; fwrite(&x, 4, 1, wav); uint16 s = 1; fwrite(&s, 2, 1, wav); s = 2; fwrite(&s, 2, 1, wav);
    fwrite(&rate, 4, 1, wav); x = rate * 4; fwrite(&x, 4, 1, wav); s = 4; fwrite(&s, 2, 1, wav);
    s = 16; fwrite(&s, 2, 1, wav); fwrite("data", 1, 4, wav); fwrite(&bytes, 4, 1, wav);
    fclose(wav);
  }
  if (st) fclose(st);
  if (sfxst) fclose(sfxst);
  return 0;
}
