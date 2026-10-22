/* MegaMetroid — motor nativo: scroll de la cámara entre las zonas de la sala (banco $80 del decomp, docs/31).
 * Escrito a mano desde snesrev/sm sm_80.c. Lo que en el SNES sube columnas/filas de bloques a la VRAM
 * (UploadLevelDataColumn...) lo hace aquí el dibujo de la Genesis (md/) a partir de las posiciones. */
#include "samus.h"
#include "room.h"

static inline uint16 Mult8x8(uint8 a, uint8 b) { return (uint16)a * b; }
static inline uint16 swap16(uint16 v) { return (uint16)(v << 8 | v >> 8); }

void HandleAutoscrolling_X(void) {  // 0x80A528
  if (time_is_frozen_flag)
    return;
  uint16 var939 = layer1_x_pos, v4;
  if (layer1_x_pos & 0x8000)
    layer1_x_pos = 0;
  uint16 v0 = swap16(room_width_in_scrolls - 1);
  if (v0 < layer1_x_pos)
    layer1_x_pos = v0;
  uint16 row = Mult8x8((uint16)(layer1_y_pos + 128) >> 8, room_width_in_scrolls);
  uint16 v2 = row + HI8(layer1_x_pos);
  if (scrolls[v2]) {
    if (scrolls[(uint16)(v2 + 1)])
      return;
    uint16 var933 = layer1_x_pos & 0xFF00;
    uint16 v5 = var939 - absolute_moved_last_frame_x - 2;
    if ((int16)(v5 - (layer1_x_pos & 0xFF00)) < 0) {
      v4 = var933;
    } else {
      var939 = v5;
      v4 = scrolls[(uint16)(row + HI8(var939))] ? var939 : (var939 & 0xFF00) + 256;
    }
  } else {
    uint16 var933 = (layer1_x_pos & 0xFF00) + 256;
    if ((uint16)(absolute_moved_last_frame_x + var939 + 2) >= var933) {
      v4 = var933;
    } else {
      var939 += absolute_moved_last_frame_x + 2;
      v4 = scrolls[(uint16)(row + (uint8)(HI8(var939) + 1))] ? var939 : var939 & 0xFF00;
    }
  }
  layer1_x_pos = v4;
}

void HandleScrollingWhenTriggeringScrollRight(void) {  // 0x80A641
  uint16 var939 = layer1_x_pos;
  if ((int16)(ideal_layer1_xpos - layer1_x_pos) < 0) {
    layer1_x_pos = ideal_layer1_xpos;
    layer1_x_subpos = 0;
  }
  uint16 v0 = swap16(room_width_in_scrolls - 1);
  if (v0 >= layer1_x_pos) {
    uint16 row = Mult8x8((uint16)(layer1_y_pos + 128) >> 8, room_width_in_scrolls);
    if (!scrolls[(uint16)(row + 1 + HI8(layer1_x_pos))]) {
      uint16 var933 = layer1_x_pos & 0xFF00;
      uint16 v4 = var939 - absolute_moved_last_frame_x - 2;
      if ((int16)(v4 - (layer1_x_pos & 0xFF00)) < 0)
        v4 = var933;
      layer1_x_pos = v4;
    }
  } else {
    layer1_x_pos = v0;
  }
}

void HandleScrollingWhenTriggeringScrollLeft(void) {  // 0x80A6BB
  uint16 var939 = layer1_x_pos;
  if ((int16)(layer1_x_pos - ideal_layer1_xpos) < 0) {
    layer1_x_pos = ideal_layer1_xpos;
    layer1_x_subpos = 0;
  }
  if (!(layer1_x_pos & 0x8000)) {
    uint16 prod = Mult8x8((uint16)(layer1_y_pos + 128) >> 8, room_width_in_scrolls);
    if (!scrolls[(uint16)(prod + HI8(layer1_x_pos))]) {
      uint16 var933 = (layer1_x_pos & 0xFF00) + 256;
      uint16 v1 = absolute_moved_last_frame_x + var939 + 2;
      if (v1 >= var933)
        v1 = var933;
      layer1_x_pos = v1;
    }
  } else {
    layer1_x_pos = 0;
  }
}

void HandleAutoscrolling_Y(void) {  // 0x80A731
  if (time_is_frozen_flag)
    return;
  uint16 col = (uint16)(layer1_x_pos + 128) >> 8;
  uint16 r20 = Mult8x8(HI8(layer1_y_pos), room_width_in_scrolls) + col;
  uint16 var933 = (scrolls[r20] != 1) ? 31 : 0;
  uint16 var939 = layer1_y_pos, v8;
  if (layer1_y_pos & 0x8000)
    layer1_y_pos = 0;
  uint16 v3 = var933 + swap16(room_height_in_scrolls - 1);
  if (v3 < layer1_y_pos)
    layer1_y_pos = v3;
  uint16 v5 = Mult8x8(HI8(layer1_y_pos), room_width_in_scrolls) + col;
  if (!scrolls[v5]) {
    uint16 var935 = (layer1_y_pos & 0xFF00) + 256;
    uint16 v6 = absolute_moved_last_frame_y + var939 + 2;
    if (v6 >= var935) {
      v8 = var935;
    } else {
      var939 += absolute_moved_last_frame_y + 2;
      uint16 prod = Mult8x8(HI8(v6) + 1, room_width_in_scrolls);
      v8 = scrolls[(uint16)(prod + col)] ? var939 : var939 & 0xFF00;
    }
    layer1_y_pos = v8;
    return;
  }
  if (!scrolls[(uint16)(room_width_in_scrolls + v5)]) {
    uint16 var937 = var933 + (layer1_y_pos & 0xFF00);
    if (var937 < layer1_y_pos) {
      uint16 v9 = var939 - absolute_moved_last_frame_y - 2;
      if ((int16)(v9 - var937) < 0) {
        v8 = var937;
      } else {
        var939 = v9;
        uint16 prod = Mult8x8(HI8(v9), room_width_in_scrolls);
        v8 = scrolls[(uint16)(prod + col)] ? var939 : (var939 & 0xFF00) + 256;
      }
      layer1_y_pos = v8;
    }
  }
}

void HandleScrollingWhenTriggeringScrollDown(void) {  // 0x80A893
  uint16 var939 = layer1_y_pos;
  uint16 r20 = Mult8x8(HI8(layer1_y_pos), room_width_in_scrolls) + ((uint16)(layer1_x_pos + 128) >> 8);
  uint16 var933 = (scrolls[r20] != 1) ? 31 : 0;
  if ((int16)(ideal_layer1_ypos - layer1_y_pos) < 0) {
    layer1_y_pos = ideal_layer1_ypos;
    layer1_y_subpos = 0;
  }
  uint16 var937 = var933 + swap16(room_height_in_scrolls - 1);
  bool clamp = var937 < layer1_y_pos;
  if (!clamp && !scrolls[(uint16)(room_width_in_scrolls + r20)]) {
    var937 = var933 + (layer1_y_pos & 0xFF00);
    clamp = var937 < layer1_y_pos;
  }
  if (clamp) {
    uint16 v3 = var939 - absolute_moved_last_frame_y - 2;
    if ((int16)(v3 - var937) < 0)
      v3 = var937;
    layer1_y_pos = v3;
  }
}

void HandleScrollingWhenTriggeringScrollUp(void) {  // 0x80A936
  uint16 var939 = layer1_y_pos;
  if ((int16)(layer1_y_pos - ideal_layer1_ypos) < 0) {
    layer1_y_pos = ideal_layer1_ypos;
    layer1_y_subpos = 0;
  }
  if (!(layer1_y_pos & 0x8000)) {
    uint16 prod = Mult8x8(HI8(layer1_y_pos), room_width_in_scrolls);
    if (!scrolls[(uint16)(prod + ((uint16)(layer1_x_pos + 128) >> 8))]) {
      uint16 var933 = (layer1_y_pos & 0xFF00) + 256;
      uint16 v1 = absolute_moved_last_frame_y + var939 + 2;
      if (v1 >= var933)
        v1 = var933;
      layer1_y_pos = v1;
    }
  } else {
    layer1_y_pos = 0;
  }
}

/* ------------------------------------------------------------------ capa 2 y posición en bloques ($80:A2F9) */
uint8 CalculateLayer2Xpos(void) {  // 0x80A2F9
  if (!layer2_scroll_x) {
    layer2_x_pos = layer1_x_pos;
    return 0;
  }
  if (layer2_scroll_x != 1) {
    int t = layer2_scroll_x & 0xFE;
    uint8 var933 = t * LO8(layer1_x_pos) >> 8;
    layer2_x_pos = t * HI8(layer1_x_pos) + var933;
    return 0;
  }
  return 1;
}

uint8 CalculateLayer2Ypos(void) {  // 0x80A33A
  if (!layer2_scroll_y) {
    layer2_y_pos = layer1_y_pos;
    return 0;
  }
  if (layer2_scroll_y != 1) {
    int t = layer2_scroll_y & 0xFE;
    uint8 var933 = t * LO8(layer1_y_pos) >> 8;
    layer2_y_pos = t * HI8(layer1_y_pos) + var933;
    return 0;
  }
  return 1;
}

static uint16 blk(uint16 p) {
  uint16 v = p >> 4;
  if (v & 0x800)
    v |= 0xF000;
  return v;
}

void CalculateBgScrollAndLayerPositionBlocks(void) {  // 0x80A4BB
  bg1_x_block = reg_BG1HOFS >> 4;
  bg2_x_block = reg_BG2HOFS >> 4;
  layer1_x_block = blk(layer1_x_pos);
  layer2_x_block = blk(layer2_x_pos);
  bg1_y_block = reg_BG1VOFS >> 4;
  bg2_y_block = reg_BG2VOFS >> 4;
  layer1_y_block = blk(layer1_y_pos);
  layer2_y_block = blk(layer2_y_pos);
}

/* UpdateBgGraphicsWhenScrolling ($80:A3DF): en el SNES sube a la VRAM la columna o fila nueva. Aquí solo se
   llevan las variables; la Genesis dibuja las columnas/filas por su cuenta (md/room_draw.c). */
void UpdateBgGraphicsWhenScrolling(void) {
  CalculateBgScrollAndLayerPositionBlocks();
  if (layer1_x_block != previous_layer1_x_block) {
    uint16 v0 = (int16)(layer1_x_block - previous_layer1_x_block) < 0 ? 0 : 16;
    previous_layer1_x_block = layer1_x_block;
    blocks_to_update_x_block = layer1_x_block + v0;
    vram_blocks_to_update_x_block = bg1_x_block + v0;
    blocks_to_update_y_block = layer1_y_block;
    vram_blocks_to_update_y_block = bg1_y_block;
  }
  if (!(layer2_scroll_x & 1) && layer2_x_block != previous_layer2_x_block) {
    uint16 v2 = (int16)(layer2_x_block - previous_layer2_x_block) < 0 ? 0 : 16;
    previous_layer2_x_block = layer2_x_block;
    blocks_to_update_x_block = layer2_x_block + v2;
    vram_blocks_to_update_x_block = bg2_x_block + v2;
    blocks_to_update_y_block = layer2_y_block;
    vram_blocks_to_update_y_block = bg2_y_block;
  }
  if (layer1_y_block != previous_layer1_y_block) {
    uint16 v4 = (int16)(layer1_y_block - previous_layer1_y_block) < 0 ? 1 : 15;
    previous_layer1_y_block = layer1_y_block;
    blocks_to_update_y_block = layer1_y_block + v4;
    vram_blocks_to_update_y_block = bg1_y_block + v4;
    blocks_to_update_x_block = layer1_x_block;
    vram_blocks_to_update_x_block = bg1_x_block;
  }
  if (!(layer2_scroll_y & 1) && layer2_y_block != previous_layer2_y_block) {
    uint16 v6 = (int16)(layer2_y_block - previous_layer2_y_block) < 0 ? 1 : 15;
    previous_layer2_y_block = layer2_y_block;
    blocks_to_update_y_block = layer2_y_block + v6;
    vram_blocks_to_update_y_block = bg2_y_block + v6;
    blocks_to_update_x_block = layer2_x_block;
    vram_blocks_to_update_x_block = bg2_x_block;
  }
}

void CalculateLayer2PosAndScrollsWhenScrolling(void) {  // 0x80A3AB
  if (time_is_frozen_flag)
    return;
  reg_BG1HOFS = bg1_x_offset + layer1_x_pos;
  reg_BG1VOFS = bg1_y_offset + layer1_y_pos;
  if (!CalculateLayer2Xpos())
    reg_BG2HOFS = bg2_x_scroll + layer2_x_pos;
  if (!CalculateLayer2Ypos())
    reg_BG2VOFS = bg2_y_scroll + layer2_y_pos;
  UpdateBgGraphicsWhenScrolling();
}
