/* Generado por tools/port/port.py: lectura de los bancos de la ROM (kBankXXB, gen/tables.c) */
#ifndef PORT_GEN_H
#define PORT_GEN_H
#include "nt.h"
#include "dcompat.h"
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "plm.h"
#include "hdmaobj.h"
#include "room.h"
#include "nmath.h"
#include "../gen/tables.h"
static inline uint16 BW_86(uint16 a) { const uint8 *p = kBank86B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_86(uint16 a) { return kBank86B[(uint16)(a - 0x8000)]; }
#define IW_86(a) BW_86(a)
static inline uint16 BW_94(uint16 a) { const uint8 *p = kBank94B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_94(uint16 a) { return kBank94B[(uint16)(a - 0x8000)]; }
#define IW_94(a) BW_94(a)
static inline uint16 BW_A0(uint16 a) { const uint8 *p = kBankA0B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A0(uint16 a) { return kBankA0B[(uint16)(a - 0x8000)]; }
#define IW_A0(a) BW_A0(a)
static inline uint16 BW_A1(uint16 a) { const uint8 *p = kBankA1B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A1(uint16 a) { return kBankA1B[(uint16)(a - 0x8000)]; }
#define IW_A1(a) BW_A1(a)
static inline uint16 BW_A2(uint16 a) { const uint8 *p = kBankA2B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A2(uint16 a) { return kBankA2B[(uint16)(a - 0x8000)]; }
#define IW_A2(a) BW_A2(a)
static inline uint16 BW_A3(uint16 a) { const uint8 *p = kBankA3B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A3(uint16 a) { return kBankA3B[(uint16)(a - 0x8000)]; }
#define IW_A3(a) BW_A3(a)
static inline uint16 BW_A5(uint16 a) { const uint8 *p = kBankA5B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A5(uint16 a) { return kBankA5B[(uint16)(a - 0x8000)]; }
#define IW_A5(a) BW_A5(a)
static inline uint16 BW_A6(uint16 a) { const uint8 *p = kBankA6B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A6(uint16 a) { return kBankA6B[(uint16)(a - 0x8000)]; }
#define IW_A6(a) BW_A6(a)
static inline uint16 BW_A7(uint16 a) { const uint8 *p = kBankA7B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A7(uint16 a) { return kBankA7B[(uint16)(a - 0x8000)]; }
#define IW_A7(a) BW_A7(a)
static inline uint16 BW_A8(uint16 a) { const uint8 *p = kBankA8B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_A8(uint16 a) { return kBankA8B[(uint16)(a - 0x8000)]; }
#define IW_A8(a) BW_A8(a)
static inline uint16 BW_AA(uint16 a) { const uint8 *p = kBankAAB + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_AA(uint16 a) { return kBankAAB[(uint16)(a - 0x8000)]; }
#define IW_AA(a) BW_AA(a)
static inline uint16 BW_B2(uint16 a) { const uint8 *p = kBankB2B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_B2(uint16 a) { return kBankB2B[(uint16)(a - 0x8000)]; }
#define IW_B2(a) BW_B2(a)
static inline uint16 BW_B3(uint16 a) { const uint8 *p = kBankB3B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_B3(uint16 a) { return kBankB3B[(uint16)(a - 0x8000)]; }
#define IW_B3(a) BW_B3(a)
static inline uint16 BW_B4(uint16 a) { const uint8 *p = kBankB4B + (uint16)(a - 0x8000); uint8 hi = p[1]; __asm__("" : "+r"(hi)); return (uint16)(p[0] | hi << 8); }
static inline uint8 BB_B4(uint16 a) { return kBankB4B[(uint16)(a - 0x8000)]; }
#define IW_B4(a) BW_B4(a)
typedef struct EnemyDef {
  uint16 tile_data_size;
  uint16 palette_ptr;
  uint16 health;
  uint16 damage;
  uint16 x_radius;
  uint16 y_radius;
  uint8 bank;
  uint8 hurt_ai_time;
  uint16 hurt_sfx;
  uint16 boss_fight_value;
  uint16 ai_init;
  uint16 num_parts;
  uint16 field_16;
  uint16 main_ai;
  uint16 grapple_ai;
  uint16 hurt_ai;
  uint16 frozen_ai;
  uint16 time_is_frozen_ai;
  uint16 death_anim;
  uint16 field_24;
  uint16 field_26;
  uint16 powerbomb_reaction;
  uint16 field_2A;
  uint16 field_2C;
  uint16 field_2E;
  uint16 touch_ai;
  uint16 shot_ai;
  uint16 field_34;
  uint32 tile_data;
  uint8 layer;
  uint16 item_drop_chances_ptr;
  uint16 vulnerability_ptr;
  uint16 name_ptr;
} EnemyDef;
static inline EnemyDef *get_EnemyDef_A2(uint16 a) {  /* $A0:a (copia; 4 a la vez) */
  static EnemyDef b[4]; static uint8 i;
  EnemyDef *d = &b[i++ & 3];
  d->tile_data_size = BW_A0(a + 0);
  d->palette_ptr = BW_A0(a + 2);
  d->health = BW_A0(a + 4);
  d->damage = BW_A0(a + 6);
  d->x_radius = BW_A0(a + 8);
  d->y_radius = BW_A0(a + 10);
  d->bank = BB_A0(a + 12);
  d->hurt_ai_time = BB_A0(a + 13);
  d->hurt_sfx = BW_A0(a + 14);
  d->boss_fight_value = BW_A0(a + 16);
  d->ai_init = BW_A0(a + 18);
  d->num_parts = BW_A0(a + 20);
  d->field_16 = BW_A0(a + 22);
  d->main_ai = BW_A0(a + 24);
  d->grapple_ai = BW_A0(a + 26);
  d->hurt_ai = BW_A0(a + 28);
  d->frozen_ai = BW_A0(a + 30);
  d->time_is_frozen_ai = BW_A0(a + 32);
  d->death_anim = BW_A0(a + 34);
  d->field_24 = BW_A0(a + 36);
  d->field_26 = BW_A0(a + 38);
  d->powerbomb_reaction = BW_A0(a + 40);
  d->field_2A = BW_A0(a + 42);
  d->field_2C = BW_A0(a + 44);
  d->field_2E = BW_A0(a + 46);
  d->touch_ai = BW_A0(a + 48);
  d->shot_ai = BW_A0(a + 50);
  d->field_34 = BW_A0(a + 52);
  d->tile_data = BW_A0(a + 54) | (uint32)BB_A0(a + 56) << 16;
  d->layer = BB_A0(a + 57);
  d->item_drop_chances_ptr = BW_A0(a + 58);
  d->vulnerability_ptr = BW_A0(a + 60);
  d->name_ptr = BW_A0(a + 62);
  return d;
}
typedef struct Vulnerability {
  uint8 power;
  uint8 wave;
  uint8 ice;
  uint8 ice_wave;
  uint8 spazer;
  uint8 spazer_wave;
  uint8 spazer_ice;
  uint8 spazer_ice_wave;
  uint8 plasma;
  uint8 plasma_wave;
  uint8 plasma_ice;
  uint8 plasma_ice_wave;
  uint8 missile;
  uint8 super_missile;
  uint8 bomb;
  uint8 power_bomb;
  uint8 speed_booster;
  uint8 shinespark;
  uint8 screw_attack;
  uint8 charged_beam;
  uint8 pseudo_screw_attack;
  uint8 field_15;
} Vulnerability;
static inline Vulnerability *get_Vulnerability(uint16 a) {  /* $B4:a (copia; 4 a la vez) */
  static Vulnerability b[4]; static uint8 i;
  Vulnerability *d = &b[i++ & 3];
  d->power = BB_B4(a + 0);
  d->wave = BB_B4(a + 1);
  d->ice = BB_B4(a + 2);
  d->ice_wave = BB_B4(a + 3);
  d->spazer = BB_B4(a + 4);
  d->spazer_wave = BB_B4(a + 5);
  d->spazer_ice = BB_B4(a + 6);
  d->spazer_ice_wave = BB_B4(a + 7);
  d->plasma = BB_B4(a + 8);
  d->plasma_wave = BB_B4(a + 9);
  d->plasma_ice = BB_B4(a + 10);
  d->plasma_ice_wave = BB_B4(a + 11);
  d->missile = BB_B4(a + 12);
  d->super_missile = BB_B4(a + 13);
  d->bomb = BB_B4(a + 14);
  d->power_bomb = BB_B4(a + 15);
  d->speed_booster = BB_B4(a + 16);
  d->shinespark = BB_B4(a + 17);
  d->screw_attack = BB_B4(a + 18);
  d->charged_beam = BB_B4(a + 19);
  d->pseudo_screw_attack = BB_B4(a + 20);
  d->field_15 = BB_B4(a + 21);
  return d;
}
static inline uint16 abs16(uint16 t) { return sign16(t) ? (uint16)-t : t; }
void CallEnemyPreInstr(uint32 ea);
typedef void Func_V(void);
typedef uint8 Func_U8(void);
typedef void Func_Y_V(uint16 j);
extern uint16 eproj_spawn_x, eproj_spawn_y, eproj_spawn_varE24;
extern uint16 nt_gfx_hook_addr; extern uint8 nt_gfx_hook_bank;
uint8 port_call_void(uint32 ea);
uint16 CreateSpriteAtPos(uint16 x, uint16 y, uint16 ilist, uint16 pal);
uint32 Multiply16x16(uint16 a, uint16 j);
void SpawnPalfxObject(uint16 j);
void SpawnEprojWithGfx(uint16 a, uint16 k, uint16 j);
uint8 EprojBlockCollisition_Horiz(uint16 k);
uint8 EprojBlockCollisition_Vertical(uint16 k);
uint8 port_CallEprojInit(uint32 ea, uint16 j);
uint8 port_CallEprojPreInstr(uint32 ea, uint16 k);
uint16 port_CallEprojInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);
typedef void (*PortFnV)(void);
typedef void (*PortFnK)(uint16);
void port_nop_v(void);
void port_nop_k(uint16 k);
PortFnV port_FindCallEnemyAi(uint32 ea);
PortFnK port_FindCallEnemyPreInstr(uint32 ea);
PortFnK port_FindCallEprojInit(uint32 ea);
PortFnK port_FindCallEprojPreInstr(uint32 ea);
uint8 port_CallEnemyAi(uint32 ea);
uint16 port_CallEnemyInstr(uint32 ea, uint16 k, uint16 jp, uint8 *ok);
uint8 port_CallEnemyPreInstr(uint32 ea);
uint16 EnemyInstrWord(uint8 bank, uint16 a);
#define IW_EN(a) EnemyInstrWord(gEnemyData(k)->bank, (a))  /* lista del banco del enemigo k */
uint8 port_bank_word(uint8 bank, uint16 a, uint16 *w);
#endif
