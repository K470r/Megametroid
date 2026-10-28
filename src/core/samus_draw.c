/* MegaMetroid — motor nativo: lo que el dibujo de Samus deja en la RAM (bancos $90/$92 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm. El dibujo en sí (sprites y tiles en la VRAM) lo hace md/ con datos ya
 * convertidos; aquí solo se elige qué dibujar, como el original. */
#include "samus.h"

void SetSamusTilesDefsForCurAnim(void) {  // 0x928000
  const uint8 *AD = bank92_animdef(4 * samus_anim_frame + kSamus_AnimationDefinitionPtrs[samus_pose]);
  nmi_copy_samus_top_half_src = 7 * AD[1] + kSamus_TileDefs_TopHalf[AD[0]];
  nmi_copy_samus_halves = (nmi_copy_samus_halves & 0xFF00) | 1;
  if (AD[2] != 255) {
    nmi_copy_samus_bottom_half_src = 7 * AD[3] + kSamus_TileDefs_BottomHalf[AD[2]];
    nmi_copy_samus_halves = (nmi_copy_samus_halves & 0x00FF) | 0x100;
  }
  samus_tiles_changed();
}

/* efectos de ambiente ($90:8B0A): salpicaduras, polvo, burbujas. La lógica (cuadros, temporizadores, posición) es
   la del SNES; el dibujo lo hace la Genesis (DrawAtmosphericGfx: tipo, cuadro, posición en la pantalla). */
#define ATM_W(a) kBank90_Atmos[(uint16)((a) - kBank90_Atmos_LO)]
void DrawAtmosphericGfx(uint16 type, uint16 frame, int16 x, int16 y);   /* capa de la Genesis */
static void AtmosDraw(uint16 type, int j) {
  int v = j >> 1;
  DrawAtmosphericGfx(type, (uint8)atmospheric_gfx_frame_and_type[v], atmospheric_gfx_x_pos[v] - layer1_x_pos,
                     atmospheric_gfx_y_pos[v] - layer1_y_pos);
}
void HandleAtmosphericEffects(void) {  // 0x908B0A
  for (int i = 6; i >= 0; i -= 2) {
    int v1 = i >> 1;
    uint16 v2 = atmospheric_gfx_frame_and_type[v1];
    if (!v2)
      continue;
    uint16 r18 = 2 * (uint8)v2, type = HI8(v2);
    int16 v4 = atmospheric_gfx_anim_timer[v1] - 1;
    atmospheric_gfx_anim_timer[v1] = v4;
    if (v4) {
      if (v4 < 0) {
        if (v4 != (int16)0x8000)
          continue;
        atmospheric_gfx_anim_timer[v1] = ATM_W(r18 + ATM_W(0x8B93 + 2 * type));
      }
    } else {
      atmospheric_gfx_anim_timer[v1] = ATM_W(r18 + ATM_W(0x8B93 + 2 * type));
      uint16 v5 = atmospheric_gfx_frame_and_type[v1] + 1;
      atmospheric_gfx_frame_and_type[v1] = v5;
      if ((int16)((uint8)v5 - ATM_W(0x8BEF + 2 * type)) >= 0) {
        atmospheric_gfx_frame_and_type[v1] = 0;
        continue;
      }
    }
    switch (type) {
    case 3: atmospheric_gfx_y_pos[v1] = fx_y_pos; break;                       // DivingSplash
    case 4: atmospheric_gfx_x_pos[v1] += (i & 4) ? -1 : 1; --atmospheric_gfx_y_pos[v1]; break;   // LavaSurfaceDmg
    case 6: case 7: --atmospheric_gfx_y_pos[v1]; break;                        // Dust
    }
    AtmosDraw(type, i);
  }
}

void Samus_DrawEcho(uint16 j) {  // 0x908855: un eco con el spritemap actual de Samus
  int v1 = j >> 1;
  int16 v2 = speed_echo_ypos[v1] - kPoseParams[samus_pose].y_offset_to_gfx - layer1_y_pos;
  if (v2 >= 0 && sign16(v2 - 248)) {
    DrawSamusSpritemap(samus_top_half_spritemap_index, speed_echo_xpos[v1] - layer1_x_pos, v2);
    if (samus_bottom_half_spritemap_index)
      DrawSamusSpritemap(samus_bottom_half_spritemap_index, speed_echo_xpos[v1] - layer1_x_pos, v2);
  }
}

void Samus_DrawEchoes(void) {  // 0x9087BD
  if (!(speed_echoes_index & 0x8000)) {
    if ((speed_boost_counter & 0xFF00) == 1024) {     /* a toda velocidad: los dos ecos que siguen a Samus */
      if (speed_echo_xpos[1])
        Samus_DrawEcho(2);
      if (speed_echo_xpos[0])
        Samus_DrawEcho(0);
    }
    return;
  }
  for (int i = 2; i >= 0; i -= 2) {                   /* al frenar: los ecos alcanzan a Samus */
    int v1 = i >> 1;
    if (!speed_echo_xpos[v1])
      continue;
    uint16 v2 = speed_echo_ypos[v1];
    if (v2 != samus_y_pos)
      speed_echo_ypos[v1] = ((int16)(v2 - samus_y_pos) < 0) ? v2 + 2 : v2 - 2;
    if (speed_echo_xspeed[v1] & 0x8000) {
      uint16 v5 = speed_echo_xspeed[v1] + speed_echo_xpos[v1];
      speed_echo_xpos[v1] = v5;
      if ((int16)(v5 - samus_x_pos) < 0) {
        speed_echo_xpos[v1] = 0;
        continue;
      }
    } else {
      uint16 v4 = speed_echo_xspeed[v1] + speed_echo_xpos[v1];
      speed_echo_xpos[v1] = v4;
      if ((int16)(v4 - samus_x_pos) >= 0) {
        speed_echo_xpos[v1] = 0;
        continue;
      }
    }
    Samus_DrawEcho(i);
  }
  if (!speed_echo_xpos[1] && !speed_echo_xpos[0])
    speed_echoes_index = 0;
}

void Samus_DrawShinesparkCrashEchoProjectiles(void) {  // 0x908953
  if (nmi_frame_counter_word & 1) {
    if (speed_echo_xspeed[3])
      Samus_DrawEcho(6);
    if (speed_echo_xspeed[2])
      Samus_DrawEcho(4);
  }
}

uint8 CheckBossBitForCurArea(uint16 a) {  // 0x8081DC
  return (a & boss_bits_for_area[area_index]) != 0;
}
void SetBossBitForCurArea(uint16 a) { boss_bits_for_area[area_index] |= a; }   // 0x8081A6
void SetEventHappened(uint16 a) {  // 0x8081FA
  bitmask = 1 << (a & 7);
  events_that_happened[a >> 3] |= bitmask;
}

uint8 CheckEventHappened(uint16 a) {  // 0x808233
  bitmask = 1 << (a & 7);                    // PrepareBitAccess ($80:818E)
  return (bitmask & events_that_happened[a >> 3]) != 0;
}
