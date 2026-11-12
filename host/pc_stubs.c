/* Lo que en la Genesis hace md/ (dibujo, sonido): en el PC no hace nada, o registra (MM_DRAWLOG). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samus.h"
#include "cinematic.h"
static FILE *dl(void) { static FILE *f; static int init; if (!init) { init = 1; if (getenv("MM_DRAWLOG")) f = fopen(getenv("MM_DRAWLOG"), "w"); } return f; }
#define LOG(...) do { if (dl()) fprintf(dl(), __VA_ARGS__); } while (0)
/* sala del registro: la sala, o el estado en la huida (en la Genesis son salas aparte, mkmd.py ROOMS) */
static unsigned rid(void) {
  if (game_state == 30) {                      /* intro: la escena que se ve (md_intro.c), C180 + INTRO_SC_* */
    static uint16 shown; uint16 f;
    shown = IntroScene(shown, &f);
    return 0xC180 | shown;
  }
  switch (roomdefroomstate_ptr) { case 0xE097: case 0xE04D: case 0xE003: case 0xDFB9: case 0xDF71: return roomdefroomstate_ptr; }
  return room_ptr;
}
/* MM_SNDLOG: los pedidos al SPC con el cuadro del SNES (nmi_frame_counter_word, sin vueltas), el estado del juego y el
   cuadro de la grabación (tools/snd/snesref.py y sndcmp.py: la línea de tiempo del sonido, docs/55) */
long mm_rec_frame;                         /* host/check: el cuadro de la grabación (la última columna del registro) */
static void sndlog(const char *k, unsigned a, unsigned b) {
  static FILE *f; static int init; static unsigned long hi; static uint16 last;
  if (!init) { init = 1; if (getenv("MM_SNDLOG")) f = fopen(getenv("MM_SNDLOG"), "w"); }
  if (!f) return;
  if (nmi_frame_counter_word < last) hi += 0x10000;
  last = nmi_frame_counter_word;
  fprintf(f, "%lu %u %s %u %u %ld\n", hi + nmi_frame_counter_word, game_state, k, a, b, mm_rec_frame);
  fflush(f);
}
#define room_ptr rid()
void sfx_play(uint16 channel, uint8 id) { LOG("%04X sfx %u %02X\n", room_ptr, channel, id); if (id) sndlog("sfx", channel, id); }
void DrawSamusSpritemap(uint16 idx, uint16 x, uint16 y) { LOG("%04X samus %u\n", room_ptr, idx); }
void DrawSamusFrontExtra(void) { LOG("%04X front\n", room_ptr); }
void samus_tiles_changed(void) { LOG("%04X tiles %04X %04X %04X\n", room_ptr, nmi_copy_samus_halves, nmi_copy_samus_top_half_src, nmi_copy_samus_bottom_half_src); }
void hud_minimap_update(uint16 a, uint16 b, uint16 c) { LOG("%04X minimap %04X %04X %04X\n", room_ptr, a, b, c); }
void music_upload(uint16 i) { LOG("%04X musicup %04X\n", room_ptr, i); sndlog("up", i, 0); }
void DrawEnemySpritemap(uint8 bank, uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) { LOG("%04X enemy %02X %04X %04X %04X\n", room_ptr, bank, sm, pal, tiles); }
void DrawSpriteObjSpritemap(uint16 sm, uint16 x, uint16 y, uint16 pal, uint16 tiles) { LOG("%04X sprobj %04X %04X %04X\n", room_ptr, sm, pal, tiles); }
void CeresDoor_QueueMode7Transfers(uint16 w) { LOG("%04X m7door %u\n", room_ptr, w); }
void DrawA6SpritemapScreen(uint16 sm, uint16 x, uint16 y, uint16 c) { LOG("%04X a6s %04X %04X\n", room_ptr, sm, c); }
uint8 DrawEnemyExtSpritemap(uint8 b, uint16 sm, uint16 x, uint16 y, uint16 p, uint16 t) { LOG("%04X enemyext %02X %04X %04X %04X\n", room_ptr, b, sm, p, t); return 0; }
void DrawA6Spritemap(uint16 sm, uint16 x, uint16 y, uint16 a) { LOG("%04X a6 %04X %04X\n", room_ptr, sm, a); }
void RidleyQueueTiles(uint16 s1, uint16 s2, uint16 d1, uint16 d2, uint16 n) { LOG("%04X rtiles %04X %04X %04X %04X %04X\n", room_ptr, s1, s2, d1, d2, n); }
void RidleyGetawayMode7Queue(uint16 l) { LOG("%04X m7q %04X\n", room_ptr, l); }
void RidleyGetawayFrame(void) { }
void DrawEprojSpritemap(uint16 sm, uint16 x, uint16 y, uint16 g) { LOG("%04X eproj %04X %04X\n", room_ptr, sm, g); }
void DrawProjectileSpritemap(uint16 k, uint16 x, uint16 y) { LOG("%04X proj %04X\n", room_ptr, projectile_spritemap_pointers[k >> 1]); }
void DrawGrappleOrProjectileSpritemap(uint16 sm, uint16 x, uint16 y) { LOG("%04X proj %04X\n", room_ptr, sm); (void)x; (void)y; }
void DrawProjectileTrailSprite(uint16 x, uint16 y, uint16 t) { LOG("%04X trail %04X\n", room_ptr, t); }
void DrawGrappleOam(int16 x, int16 y, uint16 chr) { LOG("%04X grapple %04X\n", room_ptr, chr); (void)x; (void)y; }
void GrappleTiles(uint16 point_src, uint16 beam_src) { (void)point_src; (void)beam_src; }
void BeamTiles(uint16 combo) { (void)combo; }
void music_play(uint8 t) { LOG("%04X music %02X\n", room_ptr, t); sndlog("play", t, 0); }
void EscapeTextCell(uint16 dst, uint16 w) { LOG("%04X text %04X %04X\n", room_ptr, dst, w); }
void DrawTimerSpritemap(uint16 sm, uint16 x, uint16 y) { LOG("%04X spr80 %04X 0A00\n", room_ptr, sm); }
/* cinemáticas y menús: "salas" propias para la Genesis (C001 Ceres explota, C002 Zebes, C0FF fin de juego) */
static uint16 pseudo_room;
void DrawCinematicSpritemap(uint16 sm, uint16 x, uint16 y, uint16 chr, uint8 off) { LOG("%04X spr8c %04X %04X\n", game_state == 30 ? room_ptr : pseudo_room, sm, chr); }
void CinematicGfx(uint16 which) { pseudo_room = 0xC000 | which; }
void IntroGfx(uint16 which) { pseudo_room = 0xC100 | which; }
void Mode7Transfers(uint16 list) { }
void MenuGfx(void) { pseudo_room = 0xC0FF; }
void PauseGfx(uint16 what) { (void)what; }
void DrawMenuSpritemap(uint16 idx, uint16 x, uint16 y, uint16 chr) { LOG("%04X spr82 %04X %04X\n", pseudo_room, idx, chr); }

/* nivel de la sala en otra página del mapeador SSF2: en el PC está todo a la vista (room.h) */
void nt_map_level_page(uint8 page) { (void)page; }

void nt_block_changed(uint16_t i) { (void)i; }
void nt_animtiles_transfer(uint16_t src, uint16_t size, uint16_t vram) { (void)src; (void)size; (void)vram; }
void nt_enemy_vram_copy(uint32_t src, uint16_t dst) { (void)src; (void)dst; }
/* MM_DRAWLOG: las palabras distintas del BG2 de los jefes (tilemap_stuff al subirse) y las entradas de la cola de VRAM
   (tamaño, origen, destino), por sala: mkzebes.py arma con ellas los tiles y el plano B de los cuerpos (docs/59) */
static unsigned char bg2_seen[0x10000 / 8]; static unsigned bg2_room;
static void bg2_reset(void) { if (bg2_room != room_ptr) { bg2_room = room_ptr; memset(bg2_seen, 0, sizeof bg2_seen); } }
void nt_enemy_bg2_transfer(void) {
  if (!dl()) return;
  bg2_reset();
  { static unsigned hash_last; unsigned h = 0;
    for (unsigned i = 0; i < enemy_bg2_tilemap_size / 2u && i < 0x800; i++) h = h * 31 + tilemap_stuff[i];
    LOG("%04X bg2t %u %s\n", room_ptr, enemy_bg2_tilemap_size, h != hash_last ? "nuevo" : "igual"); hash_last = h;
    /* tiles distintos (tile y línea) del mapa en esta subida: el tamaño de la caché de la Genesis (docs/59) */
    { static unsigned char u[8192 / 8]; unsigned nu = 0; memset(u, 0, sizeof u);
      for (unsigned i = 0; i < enemy_bg2_tilemap_size / 2u && i < 0x800; i++) {
        unsigned k = tilemap_stuff[i] & 0x1FFF;
        if (!(u[k >> 3] & (1 << (k & 7)))) { u[k >> 3] |= 1 << (k & 7); nu++; } }
      LOG("%04X bg2u %u\n", room_ptr, nu); } }
  /* colores del BG (0-127): el más brillante visto de cada índice (los jefes llenan sus líneas después de cargar) */
  { static uint16 best[128]; static unsigned broom;
    if (broom != room_ptr) { broom = room_ptr; memset(best, 0, sizeof best); }
    for (unsigned i = 0; i < 128; i++) {
      uint16 c = palette_buffer[i], b = best[i];
      unsigned lc = (c & 31) + (c >> 5 & 31) + (c >> 10 & 31), lb = (b & 31) + (b >> 5 & 31) + (b >> 10 & 31);
      if (lc > lb) { best[i] = c; LOG("%04X bg2pal %u %04X\n", room_ptr, i, c); }
    } }
  for (unsigned i = 0; i < enemy_bg2_tilemap_size / 2u && i < 0x800; i++) {
    uint16_t w = tilemap_stuff[i];
    if (!(bg2_seen[w >> 3] & (1 << (w & 7)))) { bg2_seen[w >> 3] |= 1 << (w & 7); LOG("%04X bg2 %04X\n", room_ptr, w); }
  }
}
void nt_vram_queue_flush(const uint8_t *q, uint16_t n) {
  if (!dl()) return;
  for (unsigned i = 0; i + 7 <= n; i += 7) {
    unsigned size = q[i] | q[i + 1] << 8, src = q[i + 2] | q[i + 3] << 8, bank = q[i + 4];
    LOG("%04X vwq %04X %02X%04X %04X\n", room_ptr, size, bank, src, q[i + 5] | q[i + 6] << 8);
    /* Kraid (A59F): sube su BG2 en dos mitades de 7E:2000/2800 por la cola (Kraid_UpdateBg2TilemapTopHalf $A7:C874
       y BottomHalf): sus palabras como las de los otros jefes (los colores también: nt_enemy_bg2_transfer) */
    if (room_ptr == 0xA59F && bank == 0x7E && (src == 0x2000 || src == 0x2800) && size == 2048) {
      uint16 keep = enemy_bg2_tilemap_size;
      enemy_bg2_tilemap_size = 0x1000;
      nt_enemy_bg2_transfer();
      enemy_bg2_tilemap_size = keep;
    }
  }
}
void DrawAtmosphericGfx(uint16_t type, uint16_t frame, int16_t x, int16_t y) { (void)type; (void)frame; (void)x; (void)y; }
void DrawArmCannon(int16 x, int16 y, uint16 attr, uint16 src) { LOG("%04X armcannon %04X\n", room_ptr, src); (void)x; (void)y; (void)attr; }
