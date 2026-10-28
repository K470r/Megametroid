/* Lo que en la Genesis hace md/ (dibujo, sonido): en el PC no hace nada, o registra (MM_DRAWLOG). */
#include <stdio.h>
#include <stdlib.h>
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
#define room_ptr rid()
void sfx_play(uint16 channel, uint8 id) { LOG("%04X sfx %u %02X\n", room_ptr, channel, id); }
void DrawSamusSpritemap(uint16 idx, uint16 x, uint16 y) { LOG("%04X samus %u\n", room_ptr, idx); }
void DrawSamusFrontExtra(void) { LOG("%04X front\n", room_ptr); }
void samus_tiles_changed(void) { LOG("%04X tiles %04X %04X %04X\n", room_ptr, nmi_copy_samus_halves, nmi_copy_samus_top_half_src, nmi_copy_samus_bottom_half_src); }
void hud_minimap_update(uint16 a, uint16 b, uint16 c) { LOG("%04X minimap %04X %04X %04X\n", room_ptr, a, b, c); }
void music_upload(uint16 i) { LOG("%04X musicup %04X\n", room_ptr, i); }
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
void music_play(uint8 t) { LOG("%04X music %02X\n", room_ptr, t); }
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
void DrawAtmosphericGfx(uint16_t type, uint16_t frame, int16_t x, int16_t y) { (void)type; (void)frame; (void)x; (void)y; }
void DrawArmCannon(int16 x, int16 y, uint16 attr, uint16 src) { LOG("%04X armcannon %04X\n", room_ptr, src); (void)x; (void)y; (void)attr; }
