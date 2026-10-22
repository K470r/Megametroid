/* MegaMetroid — PC: el nivel de la sala desde la RAM del SNES (7F:0002 level_data, 7F:6402 BTS; docs/37).
   El motor tiene el nivel de la ROM y solo guarda los bloques que cambian: se marcan como cambiados los que en el
   SNES no son iguales a la ROM. Hace falta que las variables de la sala (roomdefroomstate_ptr) ya estén cargadas. */
#ifndef LVL_HOST_H
#define LVL_HOST_H
static void lvl_from_snes(const uint8_t *r) {
  lvl_load(room_state(roomdefroomstate_ptr)->level);
  for (unsigned i = 0; i < lvl_blocks; i++) {
    uint16_t v = r[0x10002 + 2 * i] | r[0x10003 + 2 * i] << 8;
    if (v != level_get(i)) level_set(i, v);
    if (r[0x16402 + i] != bts_get(i)) bts_set(i, r[0x16402 + i]);
  }
}
#endif
