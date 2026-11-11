# Variables de la RAM de la Genesis por modo

Trazas: work/ram/md_intro.npz, work/ram/md_ceres.npz, work/ram/md_zebes.npz, work/ram/md_kraid.npz

| Variable | Bytes | Usados | Modos (game_state; Z = Zebes) |
|---|---|---|---|
| `enemy_ram7800` | 8192 | 89 | 8 9 11 Z:8 Z:11 |
| `cinematic_text_tilemap` | 4096 | 3790 | 0 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `tilemap_stuff` | 4096 | 4096 | 0 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `lvl_ov` | 3072 | 2277 | 6 8 11 30 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `md_scratch` | 2880 | 2294 | 6 30 31 34 Z:6 Z:7 Z:8 Z:11 |
| `enemy_data` | 2048 | 424 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `enemy_spawn_data` | 2048 | 164 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `explored_map_tiles_saved` | 2048 | 10 | 6 34 Z:6 Z:7 Z:8 Z:11 |
| `lvl_mod` | 1600 | 95 | 6 8 11 30 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `md_kcache` | 1024 | 436 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `spr_bank` | 1024 | 1019 | 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `cv_tab` | 928 | 824 | 6 34 Z:6 Z:7 Z:8 Z:11 |
| `rot_h` | 896 | 896 | 6 8 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `zcache` | 896 | 534 | Z:7 Z:8 Z:11 |
| `vdpSpriteCache` | 768 | 0 | nunca |
| `cone_sat` | 704 | 0 | nunca |
| `hdma_table_1` | 512 | 288 | 0 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `palette_buffer` | 512 | 506 | 0 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `qsky_buf` | 512 | 128 | Z:7 Z:8 Z:11 |
| `target_palettes` | 512 | 472 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `text_q` | 512 | 36 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `text_ql` | 512 | 36 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `qrow_buf` | 432 | 72 | Z:8 Z:11 |
| `qcol_buf` | 384 | 64 | Z:8 Z:11 |
| `cache_a` | 352 | 0 | nunca |
| `kRow` | 324 | 0 | nunca |
| `allocStack` | 320 | 320 | 0 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `it_kc` | 320 | 206 | 30 31 |
| `hdma_table_2` | 288 | 16 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `b.13` | 264 | 136 | Z:8 |
| `cram_rev` | 256 | 256 | 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `hz_bd` | 256 | 36 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `map_tiles_explored` | 256 | 14 | 6 8 9 10 11 32 33 34 Z:7 Z:8 Z:11 |
| `mb_buf` | 256 | 0 | nunca |
| `nt_vwq` | 256 | 13 | Z:8 |
| `power_bomb_explosion_left_hdma` | 256 | 192 | 6 34 Z:6 Z:7 Z:8 Z:11 |
| `power_bomb_explosion_right_hdma` | 256 | 191 | 6 34 Z:6 Z:7 Z:8 Z:11 |
| `zfade.4` | 256 | 95 | Z:6 Z:7 Z:8 Z:11 |
| `enemy_drawing_queue` | 228 | 16 | 8 9 11 32 33 Z:7 Z:8 |
| `hud_shown` | 192 | 192 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `hud_tilemap` | 192 | 192 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `hud_vram` | 192 | 192 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `mb_cells` | 192 | 0 | nunca |
| `qblk_c` | 192 | 32 | Z:8 Z:11 |
| `g_word_7E9C46` | 186 | 0 | nunca |
| `gr_tiles` | 160 | 0 | nunca |
| `ep_cache` | 144 | 101 | 6 8 9 10 11 32 33 34 Z:6 Z:8 |
| `mb_spr` | 144 | 0 | nunca |
| `cram` | 128 | 124 | 6 8 9 10 11 30 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `cram_off` | 128 | 126 | 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `cram_out` | 128 | 105 | 6 33 34 |
| `cram_t` | 128 | 110 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `croco_vline_height` | 128 | 0 | nunca |
| `fadeB` | 128 | 0 | nunca |
| `fadeCurrentPal` | 128 | 0 | nunca |
| `fadeG` | 128 | 0 | nunca |
| `fadeR` | 128 | 0 | nunca |
| `fadeSB` | 128 | 0 | nunca |
| `fadeSG` | 128 | 0 | nunca |
| `fadeSR` | 128 | 0 | nunca |
| `it_cram_src` | 128 | 120 | 30 31 |
| `it_eyes` | 128 | 128 | 30 31 |
| `it_eyes_raw` | 128 | 128 | 30 31 |
| `ring_q` | 128 | 50 | 6 8 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `scrolling_sky_bg2_hdma_data` | 128 | 57 | Z:7 Z:8 Z:11 |
| `scrolling_sky_bg2_indirect_hdma` | 128 | 24 | Z:7 Z:8 Z:11 |
| `sprite_disable_flag` | 128 | 0 | nunca |
| `sprite_instr_list_ptrs` | 128 | 20 | Z:8 |
| `sprite_instr_timer` | 128 | 10 | Z:8 |
| `sprite_palettes` | 128 | 0 | nunca |
| `sprite_x_pos` | 128 | 10 | Z:8 |
| `sprite_x_subpos` | 128 | 0 | nunca |
| `sprite_y_pos` | 128 | 20 | Z:8 |
| `sprite_y_subpos` | 128 | 0 | nunca |
| `xjob` | 102 | 0 | nunca |
| `hb_box` | 96 | 88 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `pb_q` | 96 | 0 | nunca |
| `b.11` | 88 | 21 | Z:8 |
| `plm_block_indices` | 80 | 12 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `plm_header_ptr` | 80 | 12 | 8 11 Z:6 Z:7 Z:8 Z:11 |
| `plm_instr_list_ptrs` | 80 | 12 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `plm_instruction_draw_ptr` | 80 | 12 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `plm_instruction_list_link_reg` | 80 | 6 | Z:7 Z:8 Z:11 |
| `plm_instruction_timer` | 80 | 10 | 8 9 10 11 Z:6 Z:7 Z:8 Z:11 |
| `plm_pre_instrs` | 80 | 12 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `plm_room_arguments` | 80 | 4 | Z:6 Z:7 Z:8 Z:11 |
| `plm_timers` | 80 | 3 | 8 Z:6 Z:7 Z:8 |
| `plm_variable` | 80 | 0 | nunca |
| `plm_variables` | 80 | 0 | nunca |
| `eproj_enemy_header_ptr` | 72 | 32 | Z:8 |
| `eproj_flags` | 72 | 0 | nunca |
| `g_hooks` | 72 | 3 | 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `pfn.0` | 72 | 48 | Z:8 |
| `mother_brain_indirect_hdma` | 68 | 0 | nunca |
| `zroom` | 68 | 31 | Z:6 Z:7 Z:8 Z:11 |
| `active_enemy_indexes` | 64 | 24 | 0 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `cea.1` | 64 | 39 | Z:8 Z:11 |
| `cfn.0` | 64 | 39 | Z:8 Z:11 |
| `cram_next` | 64 | 64 | 6 8 9 10 11 30 31 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `interactive_enemy_indexes` | 64 | 17 | 6 8 9 10 11 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `it_slots` | 64 | 47 | 30 31 |
| `item_bit_array` | 64 | 0 | nunca |
| `kEdgeTile` | 64 | 0 | nunca |
| `md_missing_ring` | 64 | 17 | Z:7 Z:8 Z:11 |
| `opened_door_bit_array` | 64 | 0 | nunca |
| `room_chozo_bits` | 64 | 0 | nunca |
| `rot_v` | 64 | 64 | 6 8 32 33 34 Z:6 Z:7 Z:8 Z:11 |
| `zsrc` | 64 | 61 | Z:6 Z:7 Z:8 Z:11 |
