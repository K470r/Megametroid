/* MegaMetroid — capa de la Genesis: salas de Zebes (md_zebes.c, docs/37) */
#ifndef MD_ZEBES_H
#define MD_ZEBES_H
#include <stdint.h>
#define ZEBES_BASE_ROOM 1                 /* kMdRooms[1] (DF8D): sus sprites (Samus, rayo...) y paletas 2-3 */
extern uint8_t md_zebes;                  /* 1: la sala actual es de Zebes (se dibuja con md_zebes.c) */
extern uint8_t md_cur_page, md_level_page;
extern volatile uint8_t md_hw_page;
void zebes_take(void);
const MdRoom *zebes_use(void);
void zebes_load_vram(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by);
void zebes_frame(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by);
void zebes_vblank(void);
uint8_t zebes_sky_on(void);
void zebes_sky_lines(uint16_t *h, uint16_t ax, uint16_t ay);
uint8_t zebes_obj_on(void);
void zebes_obj_colors(void);
uint8_t zebes_obj_draw(uint16_t sm, uint16_t pal, uint16_t tiles, int16_t x, int16_t y);
void zebes_frz_reset(void);
void zebes_force_prio(uint8_t on);   /* luz de la bomba de poder: el plano A con prioridad alta (md_cone.c) */
uint8_t zebes_prio_busy(void);      /* todavía se reescribe la ventana del plano A con la prioridad nueva */   /* congelados: sus tiles en la VRAM ya no están (la caja de mensaje usó el Window) */
extern uint16_t zobj_col[16];
#endif
