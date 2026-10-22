/* MegaMetroid — motor nativo: enemigos (banco $A0 del decomp, docs/31). Estructura igual a la del decomp
 * (EnemyData, 64 bytes por enemigo en 7E:0F78), en variables nativas. */
#ifndef ENEMIES_H
#define ENEMIES_H
#include "nt.h"

typedef struct EnemyData {
  uint16 enemy_ptr, x_pos, x_subpos, y_pos, y_subpos, x_width, y_height, properties, extra_properties,
         ai_handler_bits, health, spritemap_pointer, timer, current_instruction, instruction_timer,
         palette_index, vram_tiles_index, layer, flash_timer, frozen_timer, invincibility_timer,
         shake_timer, frame_counter;
  uint8 bank, field_2F;
  uint16 ai_var_A, ai_var_B, ai_var_C, ai_var_D, ai_var_E, ai_preinstr, parameter_1, parameter_2;
} EnemyData;

typedef struct EnemySpawnData {     /* 7E:7000 + k (64 bytes por enemigo) */
  uint16 some_flag, cause_of_death, field_4, vram_tiles_index, palette_index, field_A, field_C, field_E,
         xpos2, ypos2, field_14, field_16, field_18, field_1A, field_1C, id, x_pos, y_pos, init_param,
         properties, extra_properties, param_1, param_2;
  uint16 name[9];                    /* uint8 name[18] en el decomp: aquí en palabras (byte bajo primero) */
} EnemySpawnData;

#define ENEMY_COUNT 32
extern EnemyData enemy_data[ENEMY_COUNT];
/* el decomp indexa los enemigos por desplazamiento en bytes (k = 0, 0x40, 0x80...) */
_Static_assert(sizeof(EnemyData) == 64, "EnemyData: 64 bytes, como en el SNES");
/* (k & ~63 en lugar de k >> 6 * 64: en el 68000 son dos desplazamientos de 6 menos por acceso) */
static inline EnemyData *gEnemyData(uint16 k) { return (EnemyData *)((uint8 *)enemy_data + (k & 0xFFC0)); }
extern EnemySpawnData enemy_spawn_data[ENEMY_COUNT];
/* RAM extra de los enemigos (7E:7800-97FF): variables de jefes como Ridley */
extern uint16 enemy_ram7800[0x1000];
#define ERAM(addr) (enemy_ram7800[(uint16)((addr) - 0x7800) >> 1])
static inline EnemySpawnData *gEnemySpawnData(uint16 k) { return &enemy_spawn_data[k >> 6]; }

#include "../gen/tables.h"
const EnemyDefN *enemy_def_slow(uint16 addr);
extern const EnemyDefN *enemy_def_cache[8];
/* definición del enemigo ($A0:addr): casi siempre la última que se pidió con esos bits (cada definición: 0x40 bytes) */
static inline const EnemyDefN *enemy_def(uint16 a) {
  const EnemyDefN *d = enemy_def_cache[(a >> 6) & 7];
  return (d && d->addr == a) ? d : enemy_def_slow(a);
}
void CallEnemyAi(uint8 bank, uint16 f);
void LoadAndInitializeEnemies(void);
void LoadEnemies(void);                        /* $A0:8A1E */
void InitializeEnemies(void);                  /* $A0:8A9E */
void TransferEnemyTilesToVramAndInit(void);    /* $A0:8CD7 */
void EnemyMain(void);
void SamusProjectileInteractionHandler(void);
void DrawEnemyQueue(uint16 phase);
/* enemigos */
void CeresDoor_Init(void);
void CeresDoor_Main(void);
void CeresRidley_Init(void);
void CeresRidley_Main(void);
void CeresRidley_Hurt(void);
void Ridley_Shot(void);
void Ridley_Powerbomb(void);
void NormalEnemyTouchAiSkipDeathAnim(void);
void GunshipTop_Init(void);
void GunshipBottom_Init(void);
void GunshipTop_Main(void);
uint16 GunshipInstrWord(uint16 a);
void CeresSteam_Init(void);
void CeresSteam_Main(void);
void CeresSteam_Touch(void);
uint16 SpawnEnemyN(uint8 db, uint16 k);
extern uint8 nt_enemy_gfx_drawn_hook;     /* enemy_gfx_drawn_hook: 1 = Ridley_A2F2 ($A6:A2F2), 0 = nullsub */
void CallEnemyGfxDrawnHook(void);
uint16 RidleyInstr(uint16 f, uint16 jp, int *ok);
#endif
