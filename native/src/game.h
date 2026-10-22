/* MegaMetroid — motor nativo: el cuadro del juego (docs/31). */
#ifndef GAME_H
#define GAME_H
#include "nt.h"

/* Sistemas del cuadro que todavía no están escritos para la Genesis. En el PC (comprobación contra el SNES)
   se rellenan con funciones que copian ese estado desde la grabación del SNES; en la Genesis quedan en NULL
   hasta que se escriben. */
typedef struct {
  void (*palette_fx)(void), (*eproj_run_all)(void),
       (*plm_handler)(void), (*animtiles)(void), (*draw_enemies)(void), (*enemy_bg2_transfer)(void),
       (*hud_tilemap)(void), (*room_main_code)(void), (*hdma)(void);
  /* carga de sala (door.c) */
  void (*room_gfx)(void);                                   /* tiles, tilemaps y fondo de la sala nueva */
  void (*spawn_room_plms)(uint16 plm_header_ptr);
  void (*spawn_door_closing_plm)(void);
  void (*spawn_hardcoded_plm)(uint8 x, uint8 y, uint16 id);
  void (*plm_item_gfx)(uint16 slot, uint16 gfx, uint16 pal_ptr); /* PlmInstr_LoadItemPlmGfx: tiles de un objeto */
  void (*fx_type_func)(uint8 type);                         /* kFxTypeFuncPtrs (HDMA de la sala) */
  void (*fx_spawn_palfx_animtiles)(uint8 palfx_bits, uint8 animtiles_bits);
  void (*spawn_bg3_scroll_hdma)(void);
  uint8 (*plm_handler_async)(void);                         /* != 0: el PLM espera un cuadro */
} GameHooks;
extern GameHooks g_hooks;

uint16 NextRandom(void);
void QueueMusic_Delayed8(uint16 a);
void QueueMusic_DelayedY(uint16 a, uint16 j);
void SpawnEprojWithRoomGfx(uint16 j, uint16 a);   /* $86:8097 */
void DrawTimer(void);                          /* $80:9F6C */
uint8 ProcessTimer(void);                      /* $80:9DE7 */
void DetermineWhichEnemiesToProcess(void);
void DrawSamusEnemiesAndProjectiles(void);
void QueueEnemyBG2TilemapTransfers(void);
void HandleMusicQueue(void);
uint8 HasQueuedMusic(void);
void SetLiquidPhysicsType(void);
uint8 GameState_9_HitDoorBlock(void);
uint8 GameState_10(void);
uint8 GameState_11_LoadingNextRoom_Async(void);
void irq_reset(void);
/* cinemáticas y fin de juego (docs/35) */
void GameState_CeresGoesBoom(void);            /* cinematic.c: estados 34 y 37 ($8B:A35B) */
uint8 GameOverStates(uint16 st);              /* gameover.c: estados 25 y 26 */
void HandleFadeOut(void);                      /* $80:8924 */
void HandleFadeIn(void);
void AddXrayToHudTilemap(void);
void AddGrappleToHudTilemap(void);
void AddMissilesToHudTilemap(void);
void AddSuperMissilesToHudTilemap(void);
void AddPowerBombsToHudTilemap(void);
void PlayRoomMusicTrackAfterAFrames(uint16 a); /* $82:E118 */
void SaveExploredMapTilesToSaved(void);         /* $80:85C6 */
void LoadMirrorOfExploredMapTiles(void);        /* $80:858C */
void SetElevatorsAsUsed(void);                  /* $80:CD07 */                       /* $80:894D */
uint8 AdvancePaletteFadeForAllPalettes(void);  /* $82:DA02 */
uint8 CheckEventHappened(uint16 a);            /* $80:8233 */
void SetEventHappened(uint16 a);               /* $80:81FA */
uint8 CheckBossBitForCurArea(uint16 a);        /* $80:81DC */
void SetBossBitForCurArea(uint16 a);           /* $80:81A6 */
void run_irqs_for_frame(void);
void CalculateLayer2PosAndScrollsWhenScrolling(void);
/* carga de sala (door.c), también para empezar la partida (gamestart.c) */
void LoadColorsForSpritesBeamsAndEnemies(void);
void UpdateBeamTilesAndPalette(void);
void LoadRoomMusic(void);
void LoadNewMusicTrackIfChanged(void);
void LoaadDesinationRoomCreBitset(void);
void LoadDoorHeader(void);
void LoadRoomHeader(void);
void LoadStateHeader(void);
void LoadLevelScrollAndCre(void);
void CreatePlmsExecuteDoorAsmRoomSetup(void);
void LoadFXHeader(void);
void InitializeSpecialEffectsForNewRoom(void);
void CalculateBgScrolls(void);
uint8 WaitForNMI_Async(void);
uint8 InitAndLoadGameData_Async(void);        /* gamestart.c: estado 6 */
void game_frame(uint16 joy);   /* un cuadro: joy en el orden de bits del mando del SNES (JOY1) */
#endif
