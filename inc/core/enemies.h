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
/* RAM extra de los enemigos (7E:7800-97FF): variables de jefes como Ridley. Sin el tramo 7E:8AC0-8FFF (docs/60): es
   la parte alta de los bloques de 64 bytes por enemigo de 7E:8800 (0x8800 + índice), y el más alto que puede usar un
   enemigo es 7E:8ABF (Yapping Maw con índice 10 en la sala A293; las demás especies que usan 7E:88xx van con índice
   0). Las direcciones desde 7E:9000 bajan 0x540 bytes; en el PC, tocar el tramo quitado avisa (check). */
#define ERAM_GAP_LO 0x8AC0
#define ERAM_GAP_HI 0x9000
#define ERAM_WORDS ((0x9800 - 0x7800 - (ERAM_GAP_HI - ERAM_GAP_LO)) >> 1)
extern uint16 enemy_ram7800[ERAM_WORDS];
#ifndef NT_SSF2
void nt_eram_gap(uint16 a);                    /* (nt_ram.c) */
#endif
static inline uint16 eram_idx(uint16 a) {      /* palabra de enemy_ram7800 de la dirección a (par) del SNES */
#ifndef NT_SSF2
  if (a >= ERAM_GAP_LO && a < ERAM_GAP_HI) nt_eram_gap(a);
#endif
  uint16 i = (uint16)(a - 0x7800) >> 1;
  return a >= ERAM_GAP_HI ? (uint16)(i - ((ERAM_GAP_HI - ERAM_GAP_LO) >> 1)) : i;
}
#define ERAM(addr) (enemy_ram7800[eram_idx(addr)])
#define ERAM_ARR(addr) (&enemy_ram7800[eram_idx(addr)])   /* arreglo de palabras en la RAM extra: ERAM_ARR(a)[i] */
/* 7E:9000: el decomp la nombra aparte, pero es la misma RAM que ERAM(0x9000) (Draygon_Instr_22 $A5:E872 la escribe y
   EprojPreInstr_SporeSpawners $86:DD46 la lee; check la compara como parte de enemy_ram7800) */
#define kraid_unk9000 ERAM(0x9000)
static inline EnemySpawnData *gEnemySpawnData(uint16 k) { return &enemy_spawn_data[k >> 6]; }
/* palabra de 7E:7000-97FF por su dirección del SNES: el decomp lee gEnemySpawnData(k)[31].campo (otro enemigo, o
   pasado el final de los datos de aparición, la RAM extra 7E:7800: NorfairRio, NorfairLavaMan, MiniDraygon) */
static inline uint16 *nt_spawn_p(uint16 a) {
  if (a < 0x7800)
    return (uint16 *)((uint8 *)enemy_spawn_data + (uint16)(a - 0x7000));
  return &enemy_ram7800[eram_idx(a)];
}

#include "tables.h"
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
extern uint16 enemy_hw;
extern uint8 sprobj_lw;                        /* (enemy_main.c) el lugar de objeto de sprite más bajo usado */
void sprobj_note(uint16 i);                        /* (game.c) los lugares hasta el más alto usado, en bytes */
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
uint16 SpawnEnemy(uint8 db, uint16 k);
extern uint8 nt_enemy_gfx_drawn_hook;     /* enemy_gfx_drawn_hook: 1 = Ridley_A2F2 ($A6:A2F2), 0 = nullsub */
void CallEnemyGfxDrawnHook(void);
uint16 RidleyInstr(uint16 f, uint16 jp, int *ok);
#endif
