/* MegaMetroid — motor nativo: cinemáticas (cinematic.c) e intro (intro.c) */
#ifndef CINEMATIC_H
#define CINEMATIC_H
#include "nt.h"

/* fases de gráficos de la intro (IntroGfx, capa de la Genesis) */
enum { INTRO_GFX_TEXT = 1, INTRO_GFX_MOTHER_BRAIN, INTRO_GFX_BABY, INTRO_GFX_SCIENTIST, INTRO_GFX_LAB, INTRO_GFX_FLIGHT };
/* escenas que dibuja la Genesis (IntroScene): las capas del SNES que se ven en cada momento de la intro */
enum { INTRO_SC_CAPT, INTRO_SC_TEXT, INTRO_SC_MB, INTRO_SC_BABY, INTRO_SC_SCIENTIST, INTRO_SC_LAB, INTRO_SC_FLY,
       INTRO_SC_SPACE, INTRO_SC_COUNT };
uint16 IntroScene(uint16 shown, uint16 *factor);
extern uint8 cinematic_text_dirty;        /* intro.c: el juego escribió en cinematic_text_tilemap (lo baja la Genesis) */

void HandleFadeIn(void);
void HandleFadeOut(void);
void SetupPpu_3_Mode7(void);
uint8 SpawnCinematicSpriteObject(uint16 d, uint16 a);
uint8 SpawnCinematicSpriteObjectToR18(uint16 d, uint16 r18);
void ClearCinematicSprites(void);
void ClearCinematicBgObjects(uint16 a);   /* intro.c */
void Mode7Transfers(uint16 list);         /* capa de la Genesis: QueueMode7Transfers($8B, list) */

/* intro.c: lo de la intro en los despachos de cinematic.c (1 / el puntero: era de la intro) */
uint8 IntroCinematicFunction(uint16 f);
uint8 IntroSpriteSetup(uint16 f, uint16 j);
uint8 IntroSpritePreInstr(uint16 f, uint16 k);
uint16 IntroSpriteInstr(uint16 f, uint16 k, uint16 j);   /* 0xFFFF: no es de la intro */
void IntroHandleSamus(void);
void IntroHandleBgAndGlow(void);
void DemoObjectInputHandler(void);
#endif
