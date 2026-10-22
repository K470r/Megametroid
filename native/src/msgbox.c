/* MegaMetroid — motor nativo: cajas de mensaje (banco $85), escrito a mano desde snesrev/sm (sm_85.c).
 *
 * Como en el decomp son corrutinas: DisplayMessageBox_Async (coroutine_state_3) espera cuadros con
 * WaitForNMI_NoUpdate_Async dentro de OpenMessageBox_Async / HandleMessageBoxInteraction_Async /
 * CloseMessageBox_Async (coroutine_state_4). La llaman GameState_8_MainGameplay (coroutine_state_1 = 1) y
 * PlmHandler (coroutine_state_2 = 1) y, mientras la caja está abierta, el resto del cuadro no corre.
 *
 * La RAM es la del SNES: el mapa de la caja y la tabla de HDMA del BG3 en 7E:3000 (ram3000 ->
 * cinematic_text_tilemap, palabras nativas), el mapa vacío del BG3 en 7E:3800 (ram3800 -> cinematic_bg_tilemap).
 * Lo que el SNES hace con el PPU (registros, DMA a la VRAM, copia de la VRAM en 7E:4100) no tiene RAM que
 * comparar: la Genesis dibuja la caja con md_msgbox (md_engine.c) a partir de ese mapa y del radio de la animación. */
#include "nt.h"
#include "nmath.h"
#include "samus.h"
#include "game.h"

void QueueSfx1_Max6(uint16 a);
void QueueSfx2_Max6(uint16 a);
void QueueSfx3_Max6(uint16 a);
void HandleSoundEffects(void);
void ReadJoypadInputs_Cur(void);   /* game.c: ReadJoypadInputs con el mando de este cuadro */

#define COROUTINE_BEGIN(state_var, start_pos) uint8 *_state_variable_ = &(state_var); switch (*_state_variable_) { case start_pos:
#define COROUTINE_AWAIT(position, function) case (position): { uint8 _coret_ = (function); if (_coret_) { *_state_variable_ = (position); return _coret_; } }
#define COROUTINE_END(position) } *_state_variable_ = (position); return 0

uint8 nt_msgbox_active;   /* Genesis: la caja se dibuja (de InitializeMessageBox a RestorePpuForMessageBox) */
uint8 nt_msgbox_row0, nt_msgbox_rows;   /* fila del BG3 donde va el mapa (VRAM $5800 + r52) y cuántas filas tiene */
uint16 nt_msgbox_serial;                /* sube cada vez que cambia el mapa de la caja */

/* 7E:3000 + off (byte del SNES) en el arreglo de palabras nativas */
static void wb3000(uint16 off, uint8 v) {
  uint16 *w = &cinematic_text_tilemap[off >> 1];
  *w = (off & 1) ? (uint16)((*w & 0xFF) | v << 8) : (uint16)((*w & 0xFF00) | v);
}
static uint8 rb3000(uint16 off) {
  uint16 w = cinematic_text_tilemap[off >> 1];
  return (off & 1) ? (uint8)(w >> 8) : (uint8)w;
}
#define PMT cinematic_text_tilemap         /* ram3000.pause_menu_map_tilemap */

#define MSGBOX_W(a) kBank85_MsgBox[(uint16)((a) - kBank85_MsgBox_LO)]
#define kMessageBoxDefs 0x869B             /* MsgBoxConfig { modify_box_func, draw_initial_tilemap, message_tilemap } */

static const uint16 kMsgBoxSpecialButtonTilemapOffs[27] = {  // 0x8583D1
  0, 0x12a, 0x12a, 0x12c, 0x12c, 0x12c, 0, 0, 0, 0, 0, 0, 0x120, 0, 0, 0,
  0,     0, 0x12a,     0,     0,     0, 0, 0, 0, 0, 0,
};
static const uint16 kTileNumbersForButtonLetters[8] = { 0x28e0, 0x3ce1, 0x2cf7, 0x38f8, 0x38d0, 0x38eb, 0x38f1, 0x284e };

static uint16 message_box_das0l_value;

static void InitializePpuForMessageBoxes(void) {  // 0x858143
  save_confirmation_selection = 0;
  wb3000(1002, reg_HDMAEN);                         // ram3000.msgbox.backup_of_enabled_hdma_channels
  wb3000(1003, gameplay_BG3SC);                     // ram3000.misc.field_3E8[3]
  gameplay_BG3SC = 88;
  gameplay_TM = 23;
  gameplay_CGWSEL = 0;
  gameplay_CGADSUB = 0;
  for (int i = 128; i >= 0; i -= 2)
    PMT[i >> 1] = 0;
  /* la copia de la VRAM $5880 en 7E:4100 (DMA de lectura): la Genesis no la necesita */
}

static void ClearMessageBoxBg3Tilemap(void) {  // 0x8581F3
  for (int v0 = 1790; v0 >= 0; v0 -= 2)
    cinematic_bg_tilemap[v0 >> 1] = 0xe;
}

static const uint16 kLargeMsgBoxTopBottomBorderTilemap[32] = {  // 0x85825A
     0xe,    0xe,    0xe, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f,
  0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x280f, 0x284e, 0x284e,    0xe,    0xe,    0xe,
};
static const uint16 kSmallMsgBoxTopBottomBorderTilemap[32] = {
     0xe,    0xe,    0xe,    0xe,    0xe,    0xe, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e,
  0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e, 0x284e,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,
};

static uint16 WriteMessageTilemap(void) {  // 0x8582B8
  message_box_animation_y1 = 112;
  message_box_animation_y0 = 124;
  message_box_animation_y_radius = 0;
  for (int i = 0; i != 112; ++i)
    PMT[i] = 0;
  uint16 r0 = MSGBOX_W(kMessageBoxDefs + 6 * (message_box_index - 1) + 4);
  uint16 r9 = MSGBOX_W(kMessageBoxDefs + 6 * message_box_index + 4) - r0;
  int n = r9 >> 1;
  r9 += 128;
  message_box_das0l_value = r9;
  uint16 v1 = 32;
  uint16 v2 = 0;
  do {
    PMT[v1 + 256] = MSGBOX_W(r0 + v2);
    ++v1;
    v2 += 2;
  } while (--n);
  return v1;
}

static void WriteMessageBoxTilemap(const uint16 *border) {  // 0x85825A / 0x858289
  for (int i = 0; i != 32; ++i)
    PMT[i + 256] = border[i];
  uint16 i = WriteMessageTilemap();
  for (int n = 0; n < 32; n++, i++)
    PMT[i + 256] = border[n];
}

static void MsgBoxMakeHdmaTable(void) {  // 0x8583??: tabla del desplazamiento vertical del BG3 (la caja se abre)
  message_box_animation_y2 = (uint16)(31488 - message_box_animation_y_radius) >> 8;
  message_box_animation_y3 = 99;
  message_box_animation_y0 = (uint16)(message_box_animation_y_radius + 31744) >> 8;
  message_box_animation_y1 = 148;
  uint16 v0 = 123;
  uint16 v1 = 124;
  uint16 r20 = 30;
  do {
    PMT[v0] = message_box_animation_y3 - message_box_animation_y2;
    --message_box_animation_y3;
    --message_box_animation_y2;
    PMT[v1] = message_box_animation_y1 - message_box_animation_y0;
    --v0;
    ++message_box_animation_y1;
    ++message_box_animation_y0;
    ++v1;
  } while (--r20);
  for (uint16 v2 = v1 * 2; (int16)(v2 - 480) < 0; v2 += 2)
    PMT[v2 >> 1] = 0;
}

static void SetupMessageBoxBg3YscrollHdma(void) {  // 0x858363
  /* *(uint32 *)&ram3000.menu.palette_backup_in_menu[64] = -516947713 (0xE12FFFFF) */
  wb3000(896, 0xFF); wb3000(897, 0xFF); wb3000(898, 0x2F); wb3000(899, 0xE1);
  PMT[450] = 12542;
  wb3000(896 + 6, 0);                               // ram3000.msgbox.indirect_hdma[6]
  MsgBoxMakeHdmaTable();
}

static void SetupPpuForActiveMessageBox(uint16 r52) {  // 0x85831E
  /* el DMA del mapa (7E:3200, message_box_das0l_value bytes) a la VRAM $5800 + r52: la Genesis lo lee de la RAM */
  SetupMessageBoxBg3YscrollHdma();
  nt_msgbox_row0 = (uint8)(r52 >> 5);
  nt_msgbox_rows = (uint8)(message_box_das0l_value >> 6);
  nt_msgbox_serial++;
  nt_msgbox_active = 1;
}

static void DrawSpecialButtonAndSetupPpuForLargeMessageBox(uint16 a) {
  uint16 v1 = 0;
  if ((a & kButton_A) == 0) {
    v1 = 2;
    if ((a & 0x8000) == 0) {
      v1 = 4;
      if ((a & kButton_X) == 0) {
        v1 = 6;
        if ((a & kButton_Y) == 0) {
          v1 = 8;
          if ((a & kButton_Select) == 0) {
            v1 = 10;
            if ((a & kButton_L) == 0) {
              v1 = 12;
              if ((a & kButton_R) == 0)
                v1 = 14;
            }
          }
        }
      }
    }
  }
  PMT[256 + (kMsgBoxSpecialButtonTilemapOffs[message_box_index - 1] >> 1)] = kTileNumbersForButtonLetters[v1 >> 1];
  SetupPpuForActiveMessageBox(416);
}

static void InitializeMessageBox(void) {  // 0x858241
  uint16 def = kMessageBoxDefs + 6 * (message_box_index - 1);
  switch (MSGBOX_W(def + 2)) {                      // CallMsgBoxDraw
  case 0x825A: WriteMessageBoxTilemap(kLargeMsgBoxTopBottomBorderTilemap); break;   // WriteLargeMessageBoxTilemap
  case 0x8289: WriteMessageBoxTilemap(kSmallMsgBoxTopBottomBorderTilemap); break;   // WriteSmallMessageBoxTilemap
  default: nt_unimpl("InitializeMessageBox (dibujo)"); break;
  }
  switch (MSGBOX_W(def)) {                          // CallMsgBoxModify
  case 0x83C5: DrawSpecialButtonAndSetupPpuForLargeMessageBox(button_config_shoot_x); break;  // DrawShootButton...
  case 0x83CC: DrawSpecialButtonAndSetupPpuForLargeMessageBox(button_config_run_b); break;    // DrawRunButton...
  case 0x8436: SetupPpuForActiveMessageBox(448); break;                                        // SetupPpuForSmallMessageBox
  case 0x8441: SetupPpuForActiveMessageBox(416); break;                                        // SetupPpuForLargeMessageBox
  default: nt_unimpl("InitializeMessageBox (cambio)"); break;
  }
}

static const uint16 kSaveConfirmationSelectionTilemap[96] = {  // 0x858507
     0xe,    0xe,    0xe,    0xe,    0xe,    0xe, 0x3c4e, 0x3c4e, 0x38cc, 0x38cd, 0x3cf8, 0x3ce4, 0x3cf2, 0x3c4e, 0x3c4e, 0x3c4e,
  0x3c4e, 0x3c4e, 0x3c4e, 0x2ced, 0x2cee, 0x3c4e, 0x3c4e, 0x3c4e, 0x3c4e,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,
     0xe,    0xe,    0xe,    0xe,    0xe,    0xe, 0x3c4e, 0x3c4e, 0x38cc, 0x38cd, 0x3cf8, 0x3ce4, 0x3cf2, 0x3c4e, 0x3c4e, 0x3c4e,
  0x3c4e, 0x3c4e, 0x3c4e, 0x2ced, 0x2cee, 0x3c4e, 0x3c4e, 0x3c4e, 0x3c4e,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,
     0xe,    0xe,    0xe,    0xe,    0xe,    0xe, 0x3c4e, 0x3c4e, 0x3c4e, 0x3c4e, 0x2cf8, 0x2ce4, 0x2cf2, 0x3c4e, 0x3c4e, 0x3c4e,
  0x3c4e, 0x38cc, 0x38cd, 0x3ced, 0x3cee, 0x3c4e, 0x3c4e, 0x3c4e, 0x3c4e,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,    0xe,
};

static void ToggleSaveConfirmationSelection(void) {
  save_confirmation_selection ^= 2;
  uint16 v0 = 64;
  if (save_confirmation_selection == 2)
    v0 = 128;
  uint16 v1 = 128;
  int r52 = 32;
  do {
    PMT[v1 + 256] = kSaveConfirmationSelectionTilemap[v0 >> 1];
    ++v1;
    v0 += 2;
  } while (--r52);
  nt_msgbox_serial++;
}

static void RestorePpuForMessageBox(void) {  // 0x85861A
  reg_HDMAEN = rb3000(1002);
  gameplay_BG3SC = rb3000(1003);
  gameplay_TM = reg_TM;
  gameplay_CGWSEL = next_gameplay_CGWSEL;
  gameplay_CGADSUB = next_gameplay_CGADSUB;
  nt_msgbox_active = 0;
}

static uint8 WaitForNMI_NoUpdate_Async(void) {  // (decomp: sin NMI con actualización)
  if (coroutine_completion_flags) {
    coroutine_completion_flags = 0;
    return 0;
  }
  coroutine_completion_flags = 1;
  return 1;
}

static uint8 OpenMessageBox_Async(void) {  // 0x85844C
  COROUTINE_BEGIN(coroutine_state_4, 0);
  for (message_box_animation_y_radius = 0;; message_box_animation_y_radius += 512) {
    COROUTINE_AWAIT(1, WaitForNMI_NoUpdate_Async());
    HandleMusicQueue();
    HandleSoundEffects();
    MsgBoxMakeHdmaTable();
    if (message_box_animation_y_radius == 0x1800)
      break;
  }
  COROUTINE_END(0);
}

static uint8 HandleMessageBoxInteraction_Async(void) {  // 0x85846D
  COROUTINE_BEGIN(coroutine_state_4, 0);
  if (message_box_index == 23 || message_box_index == 28) {
    save_confirmation_selection = 0;
    while (1) {
      do {
        COROUTINE_AWAIT(1, WaitForNMI_NoUpdate_Async());
        HandleMusicQueue();
        HandleSoundEffects();
        ReadJoypadInputs_Cur();
      } while (!joypad1_newkeys);
      if ((joypad1_newkeys & kButton_A) != 0)
        break;
      if ((joypad1_newkeys & kButton_B) != 0) {
        save_confirmation_selection = 2;
        goto GETOUT;
      }
      if ((joypad1_newkeys & (kButton_Select | kButton_Left | kButton_Right)) != 0) {
        ToggleSaveConfirmationSelection();
        QueueSfx1_Max6(0x37);
      }
    }
  } else {
    my_counter = 10;
    if (message_box_index != 20 && message_box_index != 21 && message_box_index != 22 && message_box_index != 24)
      my_counter = 360;
    do {
      COROUTINE_AWAIT(2, WaitForNMI_NoUpdate_Async());
      HandleMusicQueue();
      HandleSoundEffects();
    } while (--my_counter);
    do {
      COROUTINE_AWAIT(3, WaitForNMI_NoUpdate_Async());
      ReadJoypadInputs_Cur();
    } while ((bug_fix_counter < 1 ? joypad1_newkeys : joypad1_lastkeys) == 0);
  }
GETOUT:;
  COROUTINE_END(0);
}

static uint8 CloseMessageBox_Async(void) {  // 0x858589
  COROUTINE_BEGIN(coroutine_state_4, 0);
  do {
    COROUTINE_AWAIT(1, WaitForNMI_NoUpdate_Async());
    HandleMusicQueue();
    HandleSoundEffects();
    MsgBoxMakeHdmaTable();
    message_box_animation_y_radius -= 512;
  } while ((message_box_animation_y_radius & 0x8000) == 0);
  COROUTINE_END(0);
}

static void CancelSoundEffects(void) {  // 0x82BE17
  QueueSfx1_Max6(2);
  QueueSfx2_Max6(0x71);
  QueueSfx3_Max6(1);
}

static void QueueSamusMovementSfx(void) {  // 0x82BE2F
  if ((speed_boost_counter & 0xFF00) == 1024)
    QueueSfx3_Max6(0x2B);
  if (!sign16(flare_counter - 16))
    QueueSfx1_Max6(0x41);
  CallSomeSamusCode(0x14);
}

uint8 DisplayMessageBox_Async(uint16 a) {  // 0x858080
  COROUTINE_BEGIN(coroutine_state_3, 0);
  message_box_index = a;
  CancelSoundEffects();
  InitializePpuForMessageBoxes();
  ClearMessageBoxBg3Tilemap();
  InitializeMessageBox();
  COROUTINE_AWAIT(2, OpenMessageBox_Async());
  COROUTINE_AWAIT(3, HandleMessageBoxInteraction_Async());
  COROUTINE_AWAIT(4, CloseMessageBox_Async());
  if (message_box_index == 28 && save_confirmation_selection != 2) {
    message_box_index = 24;
    ClearMessageBoxBg3Tilemap();
    QueueSfx1_Max6(0x2E);
    my_counter = 160;
    do {
      HandleMusicQueue();
      HandleSoundEffects();
      COROUTINE_AWAIT(8, WaitForNMI_NoUpdate_Async());
    } while (--my_counter);
    InitializeMessageBox();
    COROUTINE_AWAIT(5, OpenMessageBox_Async());
    COROUTINE_AWAIT(6, HandleMessageBoxInteraction_Async());
    COROUTINE_AWAIT(7, CloseMessageBox_Async());
    message_box_index = 28;
  }
  ClearMessageBoxBg3Tilemap();
  RestorePpuForMessageBox();
  QueueSamusMovementSfx();
  if (message_box_index == 20)
    game_state = 12;                                // kGameState_12_Pausing
  COROUTINE_END(0);
}

int DisplayMessageBox_Poll(uint16 a) {
  if (a == message_box_index) {
    message_box_index = 0;
    return save_confirmation_selection;
  }
  queued_message_box_index = a;
  return -1;
}
