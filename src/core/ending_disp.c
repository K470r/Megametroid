/* Generado por tools/port/ending_disp.py desde sm_8b.c: no editar (docs/63). */
#include "samus.h"

void CinematicFunction_Intro_Func151(uint16 k);
void CinematicFunction_Intro_Func179(uint16 k);
void CinematicFunction_Intro_Func180(uint16 k);
void CinematicFunction_Intro_Func184(uint16 k);
void CinematicSprPreInstr_F35A(uint16 k);
void CinematicFunction_Intro_Func189(uint16 k);
void CinematicFunction_Intro_Func189b(uint16 k);
void CinematicFunction_Intro_Func191(uint16 k);
void CinematicFunction_Intro_Func199(uint16 k);
void CinematicFunction_Intro_Func200(uint16 k);
void CinematicFunction_Intro_Func201(uint16 k);
void CinematicFunction_Intro_Func202(uint16 k);
void CinematicFunction_Intro_Func203(uint16 k);
void CinematicFunction_Intro_Func204(uint16 k);
void CinematicFunction_Intro_Func205(uint16 k);
void CinematicFunction_Intro_Func206(uint16 k);
void CinematicFunction_Intro_Func207(uint16 k);
void CinematicSprPreInstr_F528(uint16 k);
void CinematicSprPreInstr_F57F(uint16 k);
void CinematicFunction_Intro_Func215(uint16 k);
uint16 CinematicSprInstr_Func181(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func182(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func183(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func185(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func186(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func187(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func190(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func192(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func193(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func194(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func195(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func196(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func197(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func198(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func208(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func211(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func213(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func214(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func217(uint16 k, uint16 j);
uint16 CinematicSprInstr_Func218(uint16 k, uint16 j);
void CinematicFunction_Intro_Func150(uint16 j);
void CinematicFunction_Intro_Func152(uint16 j);
void CinematicFunction_Intro_Func153(uint16 j);
void CinematicFunction_Intro_Func154(uint16 j);
void CinematicFunction_Intro_Func155(uint16 j);
void CinematicFunction_Intro_Func156(uint16 j);
void CinematicFunction_Intro_Func157(uint16 j);
void CinematicFunction_Intro_Func159(uint16 j);
void CinematicFunction_Intro_Func161(uint16 j);
void CinematicFunction_Intro_Func162(uint16 j);
void CinematicFunction_Intro_Func163(uint16 j);
void CinematicFunction_Intro_Func164(uint16 j);
void CinematicFunction_Intro_Func165(uint16 j);
void CinematicFunction_Intro_Func166(uint16 j);
void CinematicFunction_Intro_Func167(uint16 j);
void CinematicFunction_Intro_Func168(uint16 j);
void CinematicFunction_Intro_Func169(uint16 j);
void CinematicFunction_Intro_Func170(uint16 j);
void CinematicFunction_Intro_Func171(uint16 j);
void CinematicFunction_Intro_Func172(uint16 j);
void CinematicFunction_Intro_Func173(uint16 j);
void CinematicFunction_Intro_Func174(uint16 j);
void CinematicFunction_Intro_Func175(uint16 j);
void CinematicFunction_Intro_Func176(uint16 j);
void CinematicFunction_Intro_Func177(uint16 j);
void CinematicFunction_Intro_Func178(uint16 j);
uint16 CalcItemPercentageCount(uint16 k, uint16 j);
uint16 CinematicFunction_Intro_Func146(uint16 k, uint16 j);
uint16 CinematicFunction_Intro_Func147(uint16 k, uint16 j);

uint8 EndingSpritePreInstr(uint16 f, uint16 k) {
  switch (f) {
  case 0xEFB2: CinematicFunction_Intro_Func151(k); return 1;
  case 0xF1E7: CinematicFunction_Intro_Func179(k); return 1;
  case 0xF227: CinematicFunction_Intro_Func180(k); return 1;
  case 0xF2A5: CinematicFunction_Intro_Func184(k); return 1;
  case 0xF35A: CinematicSprPreInstr_F35A(k); return 1;
  case 0xF375: CinematicFunction_Intro_Func189(k); return 1;
  case 0xF39B: CinematicFunction_Intro_Func189b(k); return 1;
  case 0xF3B9: CinematicFunction_Intro_Func191(k); return 1;
  case 0xF455: CinematicFunction_Intro_Func199(k); return 1;
  case 0xF464: CinematicFunction_Intro_Func200(k); return 1;
  case 0xF478: CinematicFunction_Intro_Func201(k); return 1;
  case 0xF487: CinematicFunction_Intro_Func202(k); return 1;
  case 0xF49B: CinematicFunction_Intro_Func203(k); return 1;
  case 0xF4AA: CinematicFunction_Intro_Func204(k); return 1;
  case 0xF4D1: CinematicFunction_Intro_Func205(k); return 1;
  case 0xF4E0: CinematicFunction_Intro_Func206(k); return 1;
  case 0xF507: CinematicFunction_Intro_Func207(k); return 1;
  case 0xF528: CinematicSprPreInstr_F528(k); return 1;
  case 0xF57F: CinematicSprPreInstr_F57F(k); return 1;
  case 0xF5DD: CinematicFunction_Intro_Func215(k); return 1;
  case 0xF51C: return 1;                        /* nullsub_128 */
  }
  return 0;
}

uint16 EndingSpriteInstr(uint16 f, uint16 k, uint16 j) {   /* 0xFFFF: no es del final */
  switch (f) {
  case 0xF25E: return CinematicSprInstr_Func181(k, j);
  case 0xF284: return CinematicSprInstr_Func182(k, j);
  case 0xF295: return CinematicSprInstr_Func183(k, j);
  case 0xF2B7: return CinematicSprInstr_Func185(k, j);
  case 0xF2FA: return CinematicSprInstr_Func186(k, j);
  case 0xF32B: return CinematicSprInstr_Func187(k, j);
  case 0xF3B0: return CinematicSprInstr_Func190(k, j);
  case 0xF3CE: return CinematicSprInstr_Func192(k, j);
  case 0xF41B: return CinematicSprInstr_Func193(k, j);
  case 0xF424: return CinematicSprInstr_Func194(k, j);
  case 0xF42D: return CinematicSprInstr_Func195(k, j);
  case 0xF436: return CinematicSprInstr_Func196(k, j);
  case 0xF43F: return CinematicSprInstr_Func197(k, j);
  case 0xF448: return CinematicSprInstr_Func198(k, j);
  case 0xF51D: return CinematicSprInstr_Func208(k, j);
  case 0xF554: return CinematicSprInstr_Func211(k, j);
  case 0xF597: return CinematicSprInstr_Func213(k, j);
  case 0xF5BA: return CinematicSprInstr_Func214(k, j);
  case 0xF604: return CinematicSprInstr_Func217(k, j);
  case 0xF651: return CinematicSprInstr_Func218(k, j);
  }
  return 0xFFFF;
}

uint8 EndingSpriteSetup(uint16 f, uint16 j) {
  switch (f) {
  case 0xEF99: CinematicFunction_Intro_Func150(j); return 1;
  case 0xEFEA: CinematicFunction_Intro_Func152(j); return 1;
  case 0xEFFD: CinematicFunction_Intro_Func153(j); return 1;
  case 0xF005: CinematicFunction_Intro_Func154(j); return 1;
  case 0xF018: CinematicFunction_Intro_Func155(j); return 1;
  case 0xF02B: CinematicFunction_Intro_Func156(j); return 1;
  case 0xF03E: CinematicFunction_Intro_Func157(j); return 1;
  case 0xF05E: CinematicFunction_Intro_Func159(j); return 1;
  case 0xF06D: CinematicFunction_Intro_Func161(j); return 1;
  case 0xF07C: CinematicFunction_Intro_Func162(j); return 1;
  case 0xF085: CinematicFunction_Intro_Func163(j); return 1;
  case 0xF094: CinematicFunction_Intro_Func164(j); return 1;
  case 0xF0B2: CinematicFunction_Intro_Func165(j); return 1;
  case 0xF0E1: CinematicFunction_Intro_Func166(j); return 1;
  case 0xF0E9: CinematicFunction_Intro_Func167(j); return 1;
  case 0xF0F1: CinematicFunction_Intro_Func168(j); return 1;
  case 0xF0F9: CinematicFunction_Intro_Func169(j); return 1;
  case 0xF11E: CinematicFunction_Intro_Func170(j); return 1;
  case 0xF143: CinematicFunction_Intro_Func171(j); return 1;
  case 0xF156: CinematicFunction_Intro_Func172(j); return 1;
  case 0xF169: CinematicFunction_Intro_Func173(j); return 1;
  case 0xF17C: CinematicFunction_Intro_Func174(j); return 1;
  case 0xF18F: CinematicFunction_Intro_Func175(j); return 1;
  case 0xF1A8: CinematicFunction_Intro_Func176(j); return 1;
  case 0xF1C1: CinematicFunction_Intro_Func177(j); return 1;
  case 0xF1D4: CinematicFunction_Intro_Func178(j); return 1;
  }
  return 0;
}

uint16 EndingBgInstr(uint16 f, uint16 k, uint16 j) {   /* 0xFFFF: no es del final */
  switch (f) {
  case 0xE627: return CalcItemPercentageCount(k, j);
  case 0xE769: return CinematicFunction_Intro_Func146(k, j);
  case 0xE780: return CinematicFunction_Intro_Func147(k, j);
  }
  return 0xFFFF;
}
