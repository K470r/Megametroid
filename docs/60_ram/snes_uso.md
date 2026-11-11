# Uso de la RAM del SNES (paso 1 del plan de RAM)

Grabaciones: snes_salas.npz, snes_intro.npz, snes_full.npz, snes_tu.npz; 325980 cuadros. Bytes de 7E:0000-FFFF usados: 47273 de 65536.

## Arreglos con el final sin usar (candidatos)

| Variable | SNES | Declarado | Usado hasta | Sobra (bytes) |
|---|---|---|---|---|
| `explored_map_tiles_saved` | 7E:CD52 | uint16[1024] = 2048 | 1608 | 440 |
| `g_word_7E9C46` | 7E:9C46 | uint16[93] = 186 | 34 | 152 |
| `croco_vline_height` | 7E:069C | uint8[128] = 128 | 0 | 128 |
| `sprite_disable_flag` | 7E:F2F8 | uint16[64] = 128 | 0 | 128 |
| `sprite_x_subpos` | 7E:F178 | uint16[64] = 128 | 0 | 128 |
| `sprite_y_subpos` | 7E:F278 | uint16[64] = 128 | 0 | 128 |
| `enemy_drawing_queue` | 7E:0E84 | uint16[114] = 228 | 104 | 124 |
| `scrolling_sky_bg2_indirect_hdma` | 7E:9F00 | uint8[128] = 128 | 29 | 99 |
| `power_bomb_explosion_right_hdma` | 7E:C506 | uint8[256] = 256 | 191 | 65 |
| `opened_door_bit_array` | 7E:D8B0 | uint8[64] = 64 | 0 | 64 |
| `power_bomb_explosion_left_hdma` | 7E:C406 | uint8[256] = 256 | 192 | 64 |
| `room_chozo_bits` | 7E:D830 | uint8[64] = 64 | 0 | 64 |
| `sprite_instr_list_ptrs` | 7E:EF78 | uint16[64] = 128 | 64 | 64 |
| `sprite_instr_timer` | 7E:EFF8 | uint16[64] = 128 | 64 | 64 |
| `sprite_palettes` | 7E:F078 | uint16[64] = 128 | 64 | 64 |
| `sprite_x_pos` | 7E:F0F8 | uint16[64] = 128 | 64 | 64 |
| `sprite_y_pos` | 7E:F1F8 | uint16[64] = 128 | 64 | 64 |
| `item_bit_array` | 7E:D870 | uint8[64] = 64 | 10 | 54 |
| `scrolling_sky_bg2_hdma_data` | 7E:9F80 | uint16[64] = 128 | 80 | 48 |
| `interactive_enemy_indexes` | 7E:17EC | uint16[32] = 64 | 20 | 44 |
| `map_tiles_explored` | 7E:07F7 | uint8[256] = 256 | 212 | 44 |
| `active_enemy_indexes` | 7E:17AC | uint16[32] = 64 | 26 | 38 |
| `eproj_G` | 7E:1BFB | uint16[18] = 36 | 0 | 36 |
| `eproj_enemy_header_ptr` | 7E:F3C8 | uint16[36] = 72 | 36 | 36 |
| `eproj_flags` | 7E:F380 | uint16[36] = 72 | 36 | 36 |
| `projectiletrail_right_tile_and_attribs` | 7E:D70C | uint16[18] = 36 | 0 | 36 |
| `grapple_segment_anim_instr_ptrs` | 7E:0D62 | uint16[16] = 32 | 0 | 32 |
| `grapple_segment_anim_instr_timers` | 7E:0D42 | uint16[16] = 32 | 0 | 32 |
| `mother_brain_indirect_hdma` | 7E:9C00 | uint8[68] = 68 | 36 | 32 |
| `projectile_unk_A` | 7E:0CA4 | uint16[10] = 20 | 0 | 20 |
| `projectile_variables` | 7E:0C7C | uint16[10] = 20 | 2 | 18 |
| `crocomire_bg2_scroll_hdma_indirect` | 7E:CAE0 | uint8[16] = 16 | 0 | 16 |
| `palettefx_color_indexes` | 7E:1E8D | uint16[16] = 32 | 16 | 16 |
| `used_save_stations_and_elevators` | 7E:D8F8 | uint8[16] = 16 | 1 | 15 |
| `projectile_bomb_instruction_ptr` | 7E:0C40 | uint16[10] = 20 | 6 | 14 |
| `projectile_bomb_instruction_timers` | 7E:0C54 | uint16[10] = 20 | 6 | 14 |
| `projectile_bomb_x_speed` | 7E:0BDC | uint16[10] = 20 | 6 | 14 |
| `projectile_bomb_x_subpos` | 7E:0B8C | uint16[10] = 20 | 6 | 14 |
| `projectile_bomb_y_speed` | 7E:0BF0 | uint16[10] = 20 | 6 | 14 |
| `projectile_bomb_y_subpos` | 7E:0BA0 | uint16[10] = 20 | 6 | 14 |
| `projectile_damage` | 7E:0C2C | uint16[10] = 20 | 6 | 14 |
| `projectile_dir` | 7E:0C04 | uint16[10] = 20 | 6 | 14 |
| `projectile_spritemap_pointers` | 7E:0CB8 | uint16[10] = 20 | 6 | 14 |
| `projectile_timers` | 7E:0C90 | uint16[10] = 20 | 6 | 14 |
| `projectile_type` | 7E:0C18 | uint16[10] = 20 | 6 | 14 |
| `projectile_x_pos` | 7E:0B64 | uint16[10] = 20 | 6 | 14 |
| `projectile_x_radius` | 7E:0BB4 | uint16[10] = 20 | 6 | 14 |
| `projectile_y_pos` | 7E:0B78 | uint16[10] = 20 | 6 | 14 |
| `projectile_y_radius` | 7E:0BC8 | uint16[10] = 20 | 6 | 14 |
| `animtiles_src_ptr` | 7E:1F25 | uint16[6] = 12 | 0 | 12 |
| `animtiles_timers` | 7E:1F01 | uint16[6] = 12 | 0 | 12 |
| `hdma_object_timers` | 7E:1908 | uint16[6] = 12 | 2 | 10 |
| `cinematicbg_arr6` | 7E:19E5 | uint16[4] = 8 | 0 | 8 |
| `hdma_object_A` | 7E:1914 | uint16[6] = 12 | 4 | 8 |
| `hdma_object_B` | 7E:1920 | uint16[6] = 12 | 4 | 8 |
| `hdma_object_C` | 7E:192C | uint16[6] = 12 | 4 | 8 |
| `hdma_object_D` | 7E:1938 | uint16[6] = 12 | 4 | 8 |
| `hdma_object_E` | 7E:1944 | uint16[6] = 12 | 4 | 8 |
| `hdma_object_F` | 7E:1950 | uint16[6] = 12 | 4 | 8 |
| `speed_echo_xpos` | 7E:0AB0 | uint16[4] = 8 | 0 | 8 |
| `speed_echo_ypos` | 7E:0AB8 | uint16[4] = 8 | 0 | 8 |

Total: 2773 bytes en 61 arreglos.

## Variables nunca usadas en las grabaciones

250 variables, 1294 bytes (pueden ser escritas siempre con cero o usadas en casos sin grabar): `croco_vline_height`, `sprite_disable_flag`, `sprite_x_subpos`, `sprite_y_subpos`, `opened_door_bit_array`, `room_chozo_bits`, `eproj_G`, `projectiletrail_right_tile_and_attribs`, `grapple_segment_anim_instr_ptrs`, `grapple_segment_anim_instr_timers`, `projectile_unk_A`, `crocomire_bg2_scroll_hdma_indirect`, `animtiles_src_ptr`, `animtiles_timers`, `cinematicbg_arr6`, `speed_echo_xpos`, `speed_echo_ypos`, `UNUSED_word_7E059C`, `UNUSED_word_7E0731`, `UNUSED_word_7E073D`, `UNUSED_word_7E0745`, `UNUSED_word_7E0759`, `UNUSED_word_7E075B`, `UNUSED_word_7E075D`, `UNUSED_word_7E075F`, `UNUSED_word_7E0761`, `UNUSED_word_7E0A18`, `UNUSED_word_7E0B1A`, `UNUSED_word_7E0B2A`, `UNUSED_word_7E0B38`, `UNUSED_word_7E0DF8`, `UNUSED_word_7E0DFA`, `UNUSED_word_7E177C`, `UNUSED_word_7E1782`, `UNUSEDword_7E0AA4`, `animtiles_object_index`, `bomb_counter`, `bomb_jump_dir`, `bomb_spread_charge_timeout_counter`, `charged_shot_glow_timer`, `cinematicbg_var2`, `collected_beams`, `collected_items`, `croco_cur_vline_idx`, `croco_target_0688`, `croco_word_7E069A`, `custom_draw_instr_num_blocks`, `custom_draw_instr_plm_block`, `custom_draw_instr_zero_terminator`, `debug_disable_sprite_interact`, `debug_enemy_population_pointer`, `debug_invincibility`, `debug_time_frozen_for_enemies`, `demo_num_input_frames`, `demo_set`, `demo_timer_counter`, `distance_to_eject_samus_down`, `distance_to_eject_samus_left`, `distance_to_eject_samus_right`, `distance_to_eject_samus_up`

## RAM extra de enemigos (enemy_ram7800, 7E:7800-97FF, 8 KB)

En la carga de las salas (estados (6, 11)) el SNES usa esta zona como búfer; el motor no (host/check la compara en los demás estados). "En juego": los demás estados.

| Tramo | Bytes usados (todo) | En juego | Estados que la usan en juego |
|---|---|---|---|
| 7E:7800-78FF | 256 | 157 | 5 8 9 19 20 21 22 23 24 25 26 |
| 7E:7900-79FF | 256 | 129 | 5 8 9 19 20 21 22 23 24 25 26 |
| 7E:7A00-7AFF | 256 | 117 | 8 9 |
| 7E:7B00-7BFF | 251 | 72 | 5 8 9 19 20 21 22 23 24 25 26 |
| 7E:7C00-7CFF | 254 | 67 | 5 8 9 19 20 21 22 23 24 25 26 |
| 7E:7D00-7DFF | 256 | 56 | 8 |
| 7E:7E00-7EFF | 255 | 52 | 8 |
| 7E:7F00-7FFF | 256 | 59 | 8 |
| 7E:8000-80FF | 255 | 86 | 8 9 |
| 7E:8100-81FF | 255 | 28 | 8 9 |
| 7E:8200-82FF | 250 | 28 | 8 9 |
| 7E:8300-83FF | 256 | 28 | 8 9 |
| 7E:8400-84FF | 256 | 28 | 8 9 |
| 7E:8500-85FF | 256 | 27 | 8 |
| 7E:8600-86FF | 250 | 28 | 8 |
| 7E:8700-87FF | 256 | 27 | 8 |
| 7E:8800-88FF | 256 | 107 | 8 9 |
| 7E:8900-89FF | 256 | 73 | 8 9 |
| 7E:8A00-8AFF | 252 | 78 | 8 |
| 7E:8B00-8BFF | 253 | 0 |  |
| 7E:8C00-8CFF | 246 | 0 |  |
| 7E:8D00-8DFF | 253 | 0 |  |
| 7E:8E00-8EFF | 255 | 0 |  |
| 7E:8F00-8FFF | 242 | 0 |  |
| 7E:9000-90FF | 247 | 149 | 8 |
| 7E:9100-91FF | 256 | 203 | 8 9 |
| 7E:9200-92FF | 251 | 200 | 8 9 |
| 7E:9300-93FF | 256 | 204 | 8 |
| 7E:9400-94FF | 256 | 163 | 8 |
| 7E:9500-95FF | 256 | 40 | 8 |
| 7E:9600-96FF | 256 | 0 |  |
| 7E:9700-97FF | 256 | 102 | 8 |

Tramos sin usar de 64 bytes o más: 1803 bytes en total. 7E:8AB8 (+1352), 7E:953D (+451)

