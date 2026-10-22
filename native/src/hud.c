/* MegaMetroid — motor nativo: HUD (banco $80 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_80.c: HandleHudTilemap ($80:9B44), ToggleHudItemHighlight, dígitos.
 * El resultado queda en hud_tilemap (formato de celdas del SNES: tile, paleta, prioridad, giro); la capa de la
 * Genesis lo pasa a su plano con una tabla de celdas preparada en el PC. */
#include "samus.h"
#include "game.h"

static const uint16 kEnergyTankIconTilemapOffsets[14] = { 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 2, 4, 6, 8, 0xa, 0xc, 0xe };
static const uint16 kHudItemTilemapOffsets[5] = { 0x14, 0x1c, 0x22, 0x28, 0x2e };

static void ToggleHudItemHighlight(uint16 a, uint16 k) {  // 0x809CEA
  hud_item_tilemap_palette_bits = k;
  int16 v2 = a - 1;
  if (v2 >= 0) {
    int v3 = kHudItemTilemapOffsets[v2] >> 1;
    static const int8 offs[6] = { 0, 1, 32, 33, 2, 34 };
    for (int i = 0; i < (v2 == 0 ? 6 : 4); i++) {
      uint16 *p = &hud_tilemap[v3 + offs[i]];
      if (*p != 11279)
        *p = hud_item_tilemap_palette_bits | (*p & 0xE3FF);
    }
  }
}

static void DrawTwoHudDigits(const uint16 *t, uint16 a, uint16 k) {  // 0x809D98
  hud_tilemap[k >> 1] = t[a / 10];
  hud_tilemap[(k >> 1) + 1] = t[a % 10];
}

static void DrawThreeHudDigits(const uint16 *t, uint16 a, uint16 k) {  // 0x809D78
  hud_tilemap[k >> 1] = t[a / 100];
  DrawTwoHudDigits(t, a % 100, k + 2);
}

void HandleHudTilemap(void) {  // 0x809B44
  if (reserve_health_mode == 1) {
    const uint16 *v1 = kHudTilemaps_AutoReserve + (samus_reserve_health ? 0 : 6);
    hud_tilemap[8] = v1[0];
    hud_tilemap[9] = v1[1];
    hud_tilemap[40] = v1[2];
    hud_tilemap[41] = v1[3];
    hud_tilemap[72] = v1[4];
    hud_tilemap[73] = v1[5];
  }
  if (samus_health != samus_prev_health) {
    samus_prev_health = samus_health;
    uint16 r20 = samus_health / 100;
    uint16 r18 = samus_health % 100;
    int v2 = 0;
    int n = samus_max_health / 100 + 1;
    do {
      if (!--n)
        break;
      uint16 v3 = 13360;
      if (r20) {
        --r20;
        v3 = 10289;
      }
      hud_tilemap[kEnergyTankIconTilemapOffsets[v2 >> 1] >> 1] = v3;
      v2 += 2;
    } while ((int16)(v2 - 28) < 0);
    DrawTwoHudDigits(kDigitTilesetsHealth, r18, 0x8C);
  }
  if (samus_max_missiles && samus_missiles != samus_prev_missiles) {
    samus_prev_missiles = samus_missiles;
    DrawThreeHudDigits(kDigitTilesetsWeapon, samus_missiles, 0x94);
  }
  if (samus_max_super_missiles && samus_super_missiles != samus_prev_super_missiles) {
    samus_prev_super_missiles = samus_super_missiles;
    if (joypad_dbg_flags & 0x1F40)
      DrawThreeHudDigits(kDigitTilesetsWeapon, samus_prev_super_missiles, 0x9C);
    else
      DrawTwoHudDigits(kDigitTilesetsWeapon, samus_prev_super_missiles, 0x9C);
  }
  if (samus_max_power_bombs && samus_power_bombs != samus_prev_power_bombs) {
    samus_prev_power_bombs = samus_power_bombs;
    DrawTwoHudDigits(kDigitTilesetsWeapon, samus_power_bombs, 0xA2);
  }
  if (hud_item_index != samus_prev_hud_item_index) {
    ToggleHudItemHighlight(hud_item_index, 0x1000);
    ToggleHudItemHighlight(samus_prev_hud_item_index, 0x1400);
    samus_prev_hud_item_index = hud_item_index;
    if (samus_movement_type != 3 && samus_movement_type != 20 && grapple_beam_function == 0xC4F0 && !time_is_frozen_flag)
      QueueSfx1_Max6(0x39);
  }
  ToggleHudItemHighlight(samus_auto_cancel_hud_item_index, (nmi_frame_counter_byte & 0x10) ? 4096 : 5120);
  /* la copia de hud_tilemap a la VRAM: la Genesis (cola de transferencias del cuadro) */
}

/* ------------------------------------------------------------------ HUD al empezar la partida ($80:9A79) */
static void AddToTilemapInner(uint16 k, const uint16 *j) {  // 0x809A4C
  int v2 = k >> 1;
  if ((hud_tilemap[v2] & 0x3FF) == 15) {
    hud_tilemap[v2] = j[0];
    hud_tilemap[v2 + 1] = j[1];
    hud_tilemap[v2 + 32] = j[2];
    hud_tilemap[v2 + 33] = j[3];
  }
}

void AddXrayToHudTilemap(void) { AddToTilemapInner(0x2E, kHudTilemaps_Missiles + 18); }          // 0x809A3E
void AddGrappleToHudTilemap(void) { AddToTilemapInner(0x28, kHudTilemaps_Missiles + 14); }       // 0x809A2E
void AddSuperMissilesToHudTilemap(void) { AddToTilemapInner(0x1C, kHudTilemaps_Missiles + 6); }  // 0x809A0E
void AddPowerBombsToHudTilemap(void) { AddToTilemapInner(0x22, kHudTilemaps_Missiles + 10); }    // 0x809A1E
void AddMissilesToHudTilemap(void) {  // 0x8099CF
  if ((hud_tilemap[10] & 0x3FF) == 15) {
    hud_tilemap[10] = kHudTilemaps_Missiles[0];
    hud_tilemap[11] = kHudTilemaps_Missiles[1];
    hud_tilemap[12] = kHudTilemaps_Missiles[2];
    hud_tilemap[42] = kHudTilemaps_Missiles[3];
    hud_tilemap[43] = kHudTilemaps_Missiles[4];
    hud_tilemap[44] = kHudTilemaps_Missiles[5];
  }
}

void InitializeHud(void) {  // 0x809A79 (las 4 filas de la VRAM: la Genesis)
  for (int i = 0; i < 96; i++)
    hud_tilemap[i] = kHudTilemaps_Row1to3[i];
  if (equipped_items & 0x8000) AddXrayToHudTilemap();
  if (equipped_items & 0x4000) AddGrappleToHudTilemap();
  if (samus_max_missiles) AddMissilesToHudTilemap();
  if (samus_max_super_missiles) AddSuperMissilesToHudTilemap();
  if (samus_max_power_bombs) AddPowerBombsToHudTilemap();
  samus_prev_health = 0;
  samus_prev_missiles = 0;
  samus_prev_super_missiles = 0;
  samus_prev_power_bombs = 0;
  samus_prev_hud_item_index = 0;
  if (samus_max_missiles) DrawThreeHudDigits(kDigitTilesetsWeapon, samus_missiles, 0x94);
  if (samus_max_super_missiles) DrawTwoHudDigits(kDigitTilesetsWeapon, samus_super_missiles, 0x9C);
  if (samus_max_power_bombs) DrawTwoHudDigits(kDigitTilesetsWeapon, samus_power_bombs, 0xA2);
  ToggleHudItemHighlight(hud_item_index, 0x1000);
  ToggleHudItemHighlight(samus_prev_hud_item_index, 0x1400);
  HandleHudTilemap();
}
