/* MegaMetroid — motor nativo: objetos de HDMA ($88, hdmaobj.c) y efectos de paleta ($8D, palfx.c). */
#ifndef HDMAOBJ_H
#define HDMAOBJ_H
#include "nt.h"

uint16 SpawnHdmaObject(uint8 bank, uint16 list);   /* $88:8435: devuelve la ranura (0, 2, ... 10) o 0xFFFF */
void HdmaObjectHandler(void);                       /* $88:84B9 */
void DisableHdmaObjects(void);
void EnableHdmaObjects(void);
void WaitUntilEndOfVblankAndClearHdma(void);
void SpawnBG3ScrollHdmaObject(void);                /* $88:D865 */
void FxTypeFunc(uint8 type);                        /* kFxTypeFuncPtrs */
void SpawnPowerBombExplosion(void);                 /* $88:8AA4 */

void EnablePaletteFx(void);
void DisablePaletteFx(void);
void ClearPaletteFXObjects(void);
void SpawnPalfxObject(uint16 j);                    /* $8D:C4E9 */
void PaletteFxHandler(void);                        /* $8D:C527 */
void CalculateXrayHdmaTableInner(uint16 k, uint16 j, uint16 r18, uint16 r20, bool off_screen, uint16 *dest_addr);   /* $91:BE11: tabla de la ventana del cono por línea */
void SpawnMorphBallEyeBeamHdma(void);

#endif
