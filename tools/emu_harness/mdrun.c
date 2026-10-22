/*
 * mdrun — ejecuta una ROM de Mega Drive sin pantalla con un core libretro
 * (Genesis Plus GX), simula el mando y guarda capturas PPM.
 *
 * Uso: mdrun core.so rom.bin script.txt outprefix
 * script.txt: una acción por línea
 *   hold <frames> <botones>   botones: U D L R A B C S o '-' (ninguno)
 *   shot <nombre>
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "libretro.h"

static unsigned fb_w, fb_h, fb_pitch;
static uint16_t *fb;
static unsigned pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned cur_buttons;   /* bits en orden RETRO_DEVICE_ID_JOYPAD_* */

static bool env_cb(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixfmt = *(const enum retro_pixel_format *) data;
        return pixfmt == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **) data = "/tmp";
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *) data = true;
        return true;
    default:
        return false;
    }
}
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) return;
    if (!fb || w != fb_w || h != fb_h) { free(fb); fb = malloc(w * h * 2); fb_w = w; fb_h = h; }
    for (unsigned y = 0; y < h; y++)
        memcpy(fb + y * w, (const uint8_t *) data + y * pitch, w * 2);
    fb_pitch = pitch;
}
static void audio_cb(int16_t l, int16_t r) { (void) l; (void) r; }
static size_t audio_batch_cb(const int16_t *d, size_t f) { (void) d; return f; }
static void poll_cb(void) {}
static int16_t input_cb(unsigned port, unsigned dev, unsigned idx, unsigned id) {
    (void) idx;
    if (port != 0 || dev != RETRO_DEVICE_JOYPAD) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t) cur_buttons;
    return (cur_buttons >> id) & 1;
}
static unsigned parse_buttons(const char *s) {
    unsigned b = 0;
    for (; *s; s++) switch (*s) {
        case 'U': b |= 1 << RETRO_DEVICE_ID_JOYPAD_UP; break;
        case 'D': b |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN; break;
        case 'L': b |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT; break;
        case 'R': b |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
        case 'A': b |= 1 << RETRO_DEVICE_ID_JOYPAD_Y; break;   /* Genesis A */
        case 'B': b |= 1 << RETRO_DEVICE_ID_JOYPAD_B; break;   /* Genesis B */
        case 'C': b |= 1 << RETRO_DEVICE_ID_JOYPAD_A; break;   /* Genesis C */
        case 'S': b |= 1 << RETRO_DEVICE_ID_JOYPAD_START; break;
    }
    return b;
}
static void save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%u %u\n255\n", fb_w, fb_h);
    for (unsigned i = 0; i < fb_w * fb_h; i++) {
        uint16_t p = fb[i];
        unsigned char rgb[3] = { (unsigned char)(((p >> 11) & 31) * 255 / 31),
                                 (unsigned char)(((p >> 5) & 63) * 255 / 63),
                                 (unsigned char)((p & 31) * 255 / 31) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "uso: mdrun core.so rom.bin script.txt outprefix\n"); return 1; }
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
#define SYM(n) __typeof__(n) *p_##n = (__typeof__(n) *) dlsym(h, #n)
    SYM(retro_set_environment);
    SYM(retro_set_video_refresh);
    SYM(retro_set_audio_sample);
    SYM(retro_set_audio_sample_batch);
    SYM(retro_set_input_poll);
    SYM(retro_set_input_state);
    SYM(retro_init);
    SYM(retro_load_game);
    SYM(retro_run);
    SYM(retro_set_controller_port_device);
    p_retro_set_environment(env_cb);
    p_retro_set_video_refresh(video_cb);
    p_retro_set_audio_sample(audio_cb);
    p_retro_set_audio_sample_batch(audio_batch_cb);
    p_retro_set_input_poll(poll_cb);
    p_retro_set_input_state(input_cb);
    p_retro_init();
    FILE *rf = fopen(argv[2], "rb");
    fseek(rf, 0, SEEK_END); long n = ftell(rf); fseek(rf, 0, SEEK_SET);
    void *rom = malloc(n); fread(rom, 1, n, rf); fclose(rf);
    struct retro_game_info gi = { argv[2], rom, (size_t) n, NULL };
    if (!p_retro_load_game(&gi)) { fprintf(stderr, "load_game falló\n"); return 1; }
    if (p_retro_set_controller_port_device) p_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    FILE *sf = fopen(argv[3], "r");
    char line[256], a[64], b[64];
    while (fgets(line, sizeof line, sf)) {
        if (sscanf(line, "hold %63s %63s", a, b) == 2) {
            cur_buttons = strcmp(b, "-") ? parse_buttons(b) : 0;
            for (int i = atoi(a); i > 0; i--) p_retro_run();
        } else if (sscanf(line, "shot %63s", a) == 1) {
            char p[512]; snprintf(p, sizeof p, "%s_%s.ppm", argv[4], a);
            save_ppm(p); printf("captura %s (%ux%u)\n", p, fb_w, fb_h);
        }
    }
    return 0;
}
