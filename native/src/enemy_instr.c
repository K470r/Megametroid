/* MegaMetroid — motor nativo: instrucciones comunes de las listas de los enemigos ($A0:80xx, copiadas en cada banco)
 * que leen su propio puntero (el traductor no las puede pasar: docs/42). jp: dirección de los argumentos en el banco
 * del enemigo; devuelven la dirección siguiente o 0 (la lista se detiene este cuadro). */
#include "samus.h"
#include "game.h"
#include "enemies.h"

uint16 EnemyInstrWord(uint8 bank, uint16 a);

uint16 EnemyInstr_GotoRel(uint16 k, uint16 jp) {  // 0xA080F2: salto relativo (byte con signo)
  return (uint16)(jp + (int8)(EnemyInstrWord(gEnemyData(k)->bank, jp) & 0xFF));
}

uint16 EnemyInstr_DecTimerAndGotoRel(uint16 k, uint16 jp) {  // 0xA08118
  EnemyData *E = gEnemyData(k);
  uint8 t = (uint8)E->timer;
  E->timer = (E->timer & 0xFF00) | (uint8)(t - 1);
  if (t == 1)
    return (uint16)(jp + 1);
  return EnemyInstr_GotoRel(k, jp);
}

uint16 EnemyInstr_Sleep(uint16 k, uint16 jp) {  // 0xA2812F: la lista queda en esta instrucción
  gEnemyData(k)->current_instruction = (uint16)(jp - 2);
  return 0;
}

uint16 EnemyInstr_WaitNframes(uint16 k, uint16 jp) {  // 0xA0813A
  EnemyData *E = gEnemyData(k);
  E->instruction_timer = EnemyInstrWord(E->bank, jp);
  E->current_instruction = (uint16)(jp + 2);
  return 0;
}

uint16 EnemyInstr_CopyToVram(uint16 k, uint16 jp) {  // 0xA0814B: en la Genesis los tiles ya están (sin cola de VRAM)
  (void)k;
  return (uint16)(jp + 7);
}
