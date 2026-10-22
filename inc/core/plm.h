/* MegaMetroid — motor nativo: PLMs (banco $84, plm.c) */
#ifndef PLM_H
#define PLM_H
#include "nt.h"
void EnablePLMs(void);
void DisablePLMs(void);
void ClearPLMs(void);
void SpawnHardcodedPlm(uint8 x, uint8 y, uint16 id);   /* $84:83D7 */
void SpawnRoomPLM(uint16 k);                           /* $84:846A */
void SpawnRoomPlms(uint16 header_ptr);                 /* la población de PLMs de la sala ($8F) */
uint8 SpawnPLM(uint16 a);
void SpawnDoorClosingPLM(void);                        /* $82:E8EB */                              /* $84:84E7 */
uint8 PlmHandler(void);                                /* $84:85B4 */
void ProcessPlmDrawInstruction(uint16 k);              /* $84:861E */
void CalculatePlmBlockCoords(uint16 k);                /* $84:8290 */
void WriteLevelDataBlockTypeAndBts(uint16 k, uint16 a);/* $84:82B4 */
#endif
