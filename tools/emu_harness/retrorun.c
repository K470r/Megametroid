/*
 * retrorun — ejecuta un core libretro sin pantalla con un guion.
 * Sirve para SNES (snes9x) y Genesis (Genesis Plus GX).
 *
 * Uso: retrorun core.so rom.bin guion.txt
 * Comandos del guion (uno por línea, '#' = comentario):
 *   sram <archivo>                 carga SRAM (antes de ejecutar frames)
 *   hold <frames> <botones>        botones libretro: B Y s(select) S(start) U D L R A X l r  o '-'
 *   shot <archivo.ppm>
 *   save <archivo>  / load <archivo>   savestates
 *   poke <addr_hex> <valor_hex> <bytes>  escribe RAM del sistema (little/big según core: byte a byte)
 *   dumpram <archivo>              vuelca RAM del sistema completa
 *   dumpvram <archivo>             vuelca la VRAM (si el núcleo la expone)
 *   trace <archivo> <addr_hex:len,...>  a partir de aquí, cada frame agrega esos bytes (binario)
 *   untrace
 *   (variable RR_PAD=6: mando de 6 botones en Genesis Plus GX)
 *   (variable RR_VARS="clave=valor;...": opciones del núcleo, p. ej. snes9x_layer_5=disabled)
 *   input <archivo>                reproduce un archivo de entrada: una línea por frame con botones
 *   hashseq <archivo> <addr_hex>   en cada cuadro: "cuadro valor hash" (valor: palabra de 16 bits de la RAM del
 *                                  68000 en addr, p. ej. el cuadro del motor que se ve; hash de la imagen, FNV-1a)
 *   vhashseq <archivo> <addr_hex>  lo mismo con el hash del estado del VDP (VRAM, CRAM, VSRAM, registros)
 *   unhashseq
 *   rowhashseq <archivo> <addr_hex> / unrowhashseq: binario, por cuadro: (cuadro, valor, alto) y un hash
 *                                  de 16 bits por línea (la prueba de regresión separa los corrimientos de una línea)
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "libretro.h"

static void *core_h;
static FILE *ctrace_f;
static unsigned fb_w, fb_h;
static uint16_t *fb;
static uint32_t *fb32;
static unsigned pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned cur_buttons;
static FILE *trace_f;
static FILE *vtrace_f;                   /* vtrace: VRAM completa (64 KB) en cada cuadro */
static char shot_pfx[512]; static long shot_n;   /* shots: una captura por cuadro (prefijo + número) */
static int ntr, tr_addr[64], tr_len[64];
static FILE *hash_f; static unsigned hash_addr; static long hash_frame; static int hash_vdp; static FILE *rows_f;

#define SYM(n) __typeof__(n) *p_##n
SYM(retro_set_environment); SYM(retro_set_video_refresh); SYM(retro_set_audio_sample);
SYM(retro_set_audio_sample_batch); SYM(retro_set_input_poll); SYM(retro_set_input_state);
SYM(retro_init); SYM(retro_load_game); SYM(retro_run); SYM(retro_set_controller_port_device);
SYM(retro_get_memory_data); SYM(retro_get_memory_size); SYM(retro_serialize_size);
SYM(retro_serialize); SYM(retro_unserialize); SYM(retro_get_system_av_info);

static bool env_cb(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixfmt = *(const enum retro_pixel_format *) data;
        return pixfmt == RETRO_PIXEL_FORMAT_RGB565 || pixfmt == RETRO_PIXEL_FORMAT_XRGB8888;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **) data = "/tmp";
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        /* RR_VARS="clave=valor;clave=valor": opciones del núcleo (p. ej. snes9x_layer_5=disabled) */
        struct retro_variable *v = (struct retro_variable *) data;
        const char *env = getenv("RR_VARS");
        static char val[128];
        if (!env || !v->key) return false;
        size_t kl = strlen(v->key);
        for (const char *p = env; *p; ) {
            const char *e = strchr(p, ';');
            size_t len = e ? (size_t)(e - p) : strlen(p);
            if (len > kl && !strncmp(p, v->key, kl) && p[kl] == '=') {
                size_t vl = len - kl - 1 < sizeof val - 1 ? len - kl - 1 : sizeof val - 1;
                memcpy(val, p + kl + 1, vl); val[vl] = 0; v->value = val; return true;
            }
            p += len + (e ? 1 : 0);
        }
        return false;
    }
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *) data = true;
        return true;
    default:
        return false;
    }
}
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) return;
    fb_w = w; fb_h = h;
    if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) {
        fb32 = realloc(fb32, w * h * 4);
        for (unsigned y = 0; y < h; y++) memcpy(fb32 + y * w, (const uint8_t *) data + y * pitch, w * 4);
    } else {
        fb = realloc(fb, w * h * 2);
        for (unsigned y = 0; y < h; y++) memcpy(fb + y * w, (const uint8_t *) data + y * pitch, w * 2);
    }
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
    if (!strcmp(s, "-")) return 0;
    for (; *s; s++) switch (*s) {
        case 'B': b |= 1 << RETRO_DEVICE_ID_JOYPAD_B; break;
        case 'Y': b |= 1 << RETRO_DEVICE_ID_JOYPAD_Y; break;
        case 's': b |= 1 << RETRO_DEVICE_ID_JOYPAD_SELECT; break;
        case 'S': b |= 1 << RETRO_DEVICE_ID_JOYPAD_START; break;
        case 'U': b |= 1 << RETRO_DEVICE_ID_JOYPAD_UP; break;
        case 'D': b |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN; break;
        case 'L': b |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT; break;
        case 'R': b |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
        case 'A': b |= 1 << RETRO_DEVICE_ID_JOYPAD_A; break;
        case 'X': b |= 1 << RETRO_DEVICE_ID_JOYPAD_X; break;
        case 'l': b |= 1 << RETRO_DEVICE_ID_JOYPAD_L; break;
        case 'r': b |= 1 << RETRO_DEVICE_ID_JOYPAD_R; break;
    }
    return b;
}
static void save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%u %u\n255\n", fb_w, fb_h);
    for (unsigned i = 0; i < fb_w * fb_h; i++) {
        unsigned char rgb[3];
        if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) {
            uint32_t p = fb32[i]; rgb[0] = p >> 16; rgb[1] = p >> 8; rgb[2] = p;
        } else {
            uint16_t p = fb[i];
            rgb[0] = ((p >> 11) & 31) * 255 / 31; rgb[1] = ((p >> 5) & 63) * 255 / 63; rgb[2] = (p & 31) * 255 / 31;
        }
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
static void run_frame(void) {
    p_retro_run();
    if (hash_f) {
        uint64_t h = 1469598103934665603ULL;
        if (hash_vdp) {   /* estado del VDP (VRAM, CRAM, VSRAM, registros): no depende de en qué línea cae cada escritura */
            static void *(*fc)(void), *(*fv)(void), *(*fr)(void);
            if (!fc) { fc = (void *(*)(void)) dlsym(core_h, "mm_cram"); fv = (void *(*)(void)) dlsym(core_h, "mm_vsram"); fr = (void *(*)(void)) dlsym(core_h, "mm_vdpreg"); }
            const uint8_t *parts[4] = { p_retro_get_memory_data(RETRO_MEMORY_VIDEO_RAM), fc(), fv(), fr() };
            size_t lens[4] = { 0x10000, 128, 0x50, 0x18 };
            for (int k = 0; k < 4; k++) for (size_t i = 0; i < lens[k]; i++) { h ^= parts[k][i]; h *= 1099511628211ULL; }
        } else {
        const uint8_t *px = pixfmt == RETRO_PIXEL_FORMAT_XRGB8888 ? (const uint8_t *) fb32 : (const uint8_t *) fb;
        size_t n = (size_t) fb_w * fb_h * (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2);
        for (size_t i = 0; i < n; i++) { h ^= px[i]; h *= 1099511628211ULL; }
        }
        uint8_t *ram = p_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        unsigned v = ram[hash_addr ^ 1] << 8 | ram[(hash_addr + 1) ^ 1];   /* GPGX: la RAM en palabras del 68000 */
        fprintf(hash_f, "%ld %u %016llx\n", hash_frame++, v, (unsigned long long) h);
    }
    if (rows_f) {   /* rowhashseq: por cuadro, valor de 16 bits de la RAM y un hash de 16 bits por línea de la imagen */
        uint8_t *ram = p_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        uint16_t v = ram[hash_addr ^ 1] << 8 | ram[(hash_addr + 1) ^ 1];
        int bpp = pixfmt == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2;
        const uint8_t *px = bpp == 4 ? (const uint8_t *) fb32 : (const uint8_t *) fb;
        uint16_t hdr[3] = { (uint16_t) hash_frame, v, (uint16_t) fb_h };
        fwrite(hdr, 2, 3, rows_f);
        for (int y = 0; y < fb_h; y++) {
            uint32_t h = 2166136261u;
            const uint8_t *r = px + (size_t) y * fb_w * bpp;
            for (int i = 0; i < fb_w * bpp; i++) { h ^= r[i]; h *= 16777619u; }
            uint16_t hh = (uint16_t)(h ^ (h >> 16));
            fwrite(&hh, 2, 1, rows_f);
        }
        hash_frame++;
    }
    if (trace_f) {
        uint8_t *ram = p_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        for (int i = 0; i < ntr; i++) fwrite(ram + tr_addr[i], 1, tr_len[i], trace_f);
    }
    if (vtrace_f) {
        void *m = p_retro_get_memory_data(RETRO_MEMORY_VIDEO_RAM);
        fwrite(m, 1, p_retro_get_memory_size(RETRO_MEMORY_VIDEO_RAM), vtrace_f);
    }
    if (ctrace_f) {   /* ctrace: CRAM del VDP por cuadro (núcleo GPGX con mm_cram, 64 x u16 BBBGGGRRR) */
        void *(*cr)(void) = (void *(*)(void)) dlsym(core_h, "mm_cram");
        if (cr) fwrite(cr(), 1, 128, ctrace_f);
    }
    if (shot_pfx[0]) { char n[600]; snprintf(n, sizeof n, "%s%05ld.ppm", shot_pfx, shot_n++); save_ppm(n); }
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "uso: retrorun core.so rom.bin guion.txt\n"); return 1; }
    void *h = dlopen(argv[1], RTLD_NOW);
    core_h = h;
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
#define LOAD(n) p_##n = (__typeof__(n) *) dlsym(h, #n)
    LOAD(retro_set_environment); LOAD(retro_set_video_refresh); LOAD(retro_set_audio_sample);
    LOAD(retro_set_audio_sample_batch); LOAD(retro_set_input_poll); LOAD(retro_set_input_state);
    LOAD(retro_init); LOAD(retro_load_game); LOAD(retro_run); LOAD(retro_set_controller_port_device);
    LOAD(retro_get_memory_data); LOAD(retro_get_memory_size); LOAD(retro_serialize_size);
    LOAD(retro_serialize); LOAD(retro_unserialize); LOAD(retro_get_system_av_info);
    p_retro_set_environment(env_cb); p_retro_set_video_refresh(video_cb);
    p_retro_set_audio_sample(audio_cb); p_retro_set_audio_sample_batch(audio_batch_cb);
    p_retro_set_input_poll(poll_cb); p_retro_set_input_state(input_cb);
    p_retro_init();
    FILE *rf = fopen(argv[2], "rb");
    fseek(rf, 0, SEEK_END); long n = ftell(rf); fseek(rf, 0, SEEK_SET);
    void *rom = malloc(n);
    if (fread(rom, 1, n, rf) != (size_t) n) return 1;
    fclose(rf);
    struct retro_game_info gi = { argv[2], rom, (size_t) n, NULL };
    if (!p_retro_load_game(&gi)) { fprintf(stderr, "load_game falló\n"); return 1; }
    if (p_retro_set_controller_port_device) {
        /* RR_PAD=6: mando de 6 botones en Genesis Plus GX (subclase 1 del joypad) */
        const char *pad = getenv("RR_PAD");
        unsigned dev = RETRO_DEVICE_JOYPAD;
        if (pad && atoi(pad) == 6) dev = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1);
        p_retro_set_controller_port_device(0, dev);
    }
    FILE *sf = fopen(argv[3], "r");
    char line[1024], a[512], b[512], c[64];
    long frame = 0;
    while (fgets(line, sizeof line, sf)) {
        if (line[0] == '#') continue;
        if (sscanf(line, "sram %511s", a) == 1) {
            uint8_t *s = p_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
            size_t sz = p_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
            FILE *f = fopen(a, "rb"); size_t got = fread(s, 1, sz, f); fclose(f);
            printf("sram %zu/%zu bytes\n", got, sz);
        } else if (sscanf(line, "hold %511s %511s", a, b) == 2) {
            cur_buttons = parse_buttons(b);
            for (int i = atoi(a); i > 0; i--) { run_frame(); frame++; }
        } else if (sscanf(line, "vtrace %511s", a) == 1) {
            vtrace_f = fopen(a, "wb");
        } else if (!strncmp(line, "unvtrace", 8)) {
            if (vtrace_f) fclose(vtrace_f); vtrace_f = 0;
        } else if (sscanf(line, "rowhashseq %511s %511s", a, b) == 2) {
            rows_f = fopen(a, "wb"); hash_addr = strtoul(b, 0, 16) & 0xFFFF; hash_frame = frame;
        } else if (!strncmp(line, "unrowhashseq", 12)) {
            if (rows_f) fclose(rows_f); rows_f = 0;
        } else if (sscanf(line, "vhashseq %511s %511s", a, b) == 2) {
            hash_f = fopen(a, "w"); hash_addr = strtoul(b, 0, 16) & 0xFFFF; hash_frame = frame; hash_vdp = 1;
        } else if (sscanf(line, "hashseq %511s %511s", a, b) == 2) {
            hash_f = fopen(a, "w"); hash_addr = strtoul(b, 0, 16) & 0xFFFF; hash_frame = frame; hash_vdp = 0;
        } else if (!strncmp(line, "unhashseq", 9)) {
            if (hash_f) fclose(hash_f); hash_f = 0;
        } else if (sscanf(line, "shotseq %511s", a) == 1) {
            snprintf(shot_pfx, sizeof shot_pfx, "%s", a); shot_n = 0;
        } else if (!strncmp(line, "unshotseq", 9)) {
            shot_pfx[0] = 0;
        } else if (sscanf(line, "shot %511s", a) == 1) {
            save_ppm(a);
        } else if (sscanf(line, "save %511s", a) == 1) {
            size_t sz = p_retro_serialize_size(); void *buf = malloc(sz);
            p_retro_serialize(buf, sz); FILE *f = fopen(a, "wb"); fwrite(buf, 1, sz, f); fclose(f); free(buf);
        } else if (sscanf(line, "load %511s", a) == 1) {
            size_t sz = p_retro_serialize_size(); void *buf = malloc(sz);
            FILE *f = fopen(a, "rb"); size_t got = fread(buf, 1, sz, f); fclose(f);
            if (!p_retro_unserialize(buf, got)) fprintf(stderr, "unserialize falló\n");
            free(buf);
        } else if (sscanf(line, "poke %511s %511s %63s", a, b, c) == 3) {
            uint8_t *ram = p_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
            unsigned addr = strtoul(a, 0, 16), val = strtoul(b, 0, 16); int nb = atoi(c);
            for (int i = 0; i < nb; i++) ram[addr + i] = (val >> (8 * i)) & 0xFF;   /* little-endian */
        } else if (sscanf(line, "dumpvram %511s", a) == 1) {
            /* RETRO_MEMORY_VIDEO_RAM: en GPGX es la VRAM del VDP (64 KB) */
            FILE *f = fopen(a, "wb");
            void *m = p_retro_get_memory_data(RETRO_MEMORY_VIDEO_RAM);
            if (m) fwrite(m, 1, p_retro_get_memory_size(RETRO_MEMORY_VIDEO_RAM), f);
            fclose(f);
        } else if (sscanf(line, "ctrace %511s", a) == 1) {
            ctrace_f = fopen(a, "wb");
        } else if (!strncmp(line, "regs", 4)) {
            /* registros del 68000 (núcleo GPGX con mm_m68k_reg: D0-D7, A0-A7, PC, SR) */
            unsigned (*reg)(int) = (unsigned (*)(int)) dlsym(core_h, "mm_m68k_reg");
            if (reg) {
                for (int r = 0; r < 18; r++) printf("%s%d=%08X%s", r < 8 ? "D" : r < 16 ? "A" : r == 16 ? "PC" : "SR",
                                                    r < 16 ? r % 8 : -1, reg(r), r % 8 == 7 || r >= 16 ? "\n" : " ");
                fflush(stdout);
            }
        } else if (sscanf(line, "dumpram %511s", a) == 1) {
            FILE *f = fopen(a, "wb");
            fwrite(p_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM), 1, p_retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), f);
            fclose(f);
        } else if (sscanf(line, "trace %511s %511s", a, b) == 2) {
            trace_f = fopen(a, "wb"); ntr = 0;
            for (char *t = strtok(b, ","); t; t = strtok(0, ",")) {
                tr_addr[ntr] = strtoul(t, 0, 16); char *col = strchr(t, ':'); tr_len[ntr++] = col ? atoi(col + 1) : 2;
            }
        } else if (!strncmp(line, "untrace", 7)) {
            if (trace_f) fclose(trace_f); trace_f = 0;
        } else if (sscanf(line, "input %511s", a) == 1) {
            FILE *f = fopen(a, "r"); char l2[128];
            while (fgets(l2, sizeof l2, f)) { l2[strcspn(l2, "\r\n")] = 0; cur_buttons = parse_buttons(l2[0] ? l2 : "-"); run_frame(); frame++; }
            fclose(f);
        }
    }
    printf("frames=%ld\n", frame);
    return 0;
}
