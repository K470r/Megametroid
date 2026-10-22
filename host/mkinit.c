/* MegaMetroid — estado inicial del motor nativo para la Genesis (docs/32).
 *
 * El motor arranca en el cuadro en que empieza el juego en Ceres (estado 8, Samus en el ascensor). Mientras
 * los estados 6/7 (carga de partida y fundido de entrada) no estén escritos para la Genesis, sus resultados
 * se toman de la grabación del SNES (tools/snes_ref/ceres_bot/hlref) y se guardan ya en el formato del
 * 68000: cada variable del motor en big-endian, en el orden de gen/vars_map.c. La Genesis solo copia.
 *
 * Uso: mkinit ref.bin cuadro salida_dir [nombre]   -> init_state.bin + init_state.c (o nombre.bin/.c; cuadro -1:
 *      la RAM de antes del primer cuadro grabado)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samus.h"
#include "game.h"
#include "enemies.h"
#include "room.h"
#include "vars_map.h"

void nt_unimpl(const char *what) {}
static uint16_t rw(const uint8_t *r, unsigned a) { return r[a] | r[a + 1] << 8; }
#include "lvl_host.h"

static FILE *fb, *fc;
static long off;
static void put16(uint16_t v) { fputc(v >> 8, fb); fputc(v & 0xFF, fb); off += 2; }
static void put8(uint8_t v) { fputc(v, fb); off++; }

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "uso: mkinit ref.bin cuadro salida_dir [nombre]\n"); return 2; }
  FILE *ref = fopen(argv[1], "rb");
  long f = atol(argv[2]);
  static uint8_t r[0x20000];
  /* cuadro -1: la RAM de la cabecera de la grabación (antes del primer cuadro grabado) */
  fseek(ref, f < 0 ? 0 : 0x30000 + f * 0x20002L + 2, SEEK_SET);
  if (fread(r, 1, 0x20000, ref) != 0x20000) { fprintf(stderr, "no puedo leer el cuadro %ld\n", f); return 1; }
  char p[600];
  const char *name = argc > 4 ? argv[4] : "init_state";   /* otro nombre: solo el blob (las entradas son las mismas) */
  snprintf(p, sizeof p, "%s/%s.bin", argv[3], name); fb = fopen(p, "wb");
  snprintf(p, sizeof p, "%s/%s.c", argv[3], name); fc = fopen(p, "w");
  fprintf(fc, "/* Generado por host/mkinit (cuadro %ld de la grabación del SNES): estado del motor al empezar\n"
              "   el juego en Ceres, en el orden del 68000. No editar. */\n"
              "#include \"samus.h\"\n#include \"enemies.h\"\n#include \"room.h\"\n"
              "#include \"init_state.h\"\n\nconst InitEnt kInitState[] = {\n", f);
  for (const VarMap *v = g_vars_map; v->name; v++) {
    long o = off;
    for (unsigned i = 0; i < v->count; i++) {
      unsigned a = v->addr + v->size * i;
      if (v->size == 2) put16(rw(r, a)); else put8(r[a]);
    }
    if (off & 1) put8(0);   /* cada variable empieza en dirección par en el blob */
    fprintf(fc, "  { (void *)%s%s, %u },\n", v->count > 1 ? "" : "&", v->name, v->size * v->count);
    (void)o;
  }
  /* enemigos: EnemyData (23 palabras, 2 bytes, 8 palabras), EnemySpawnData (32 palabras), RAM extra */
  for (int k = 0; k < ENEMY_COUNT; k++) {
    const uint8_t *e = r + 0xF78 + 64 * k;
    for (int i = 0; i < 23; i++) put16(rw(e, 2 * i));
    put8(e[46]); put8(e[47]);
    for (int i = 0; i < 8; i++) put16(rw(e, 48 + 2 * i));
  }
  fprintf(fc, "  { (void *)enemy_data, sizeof(enemy_data) },\n");
  for (int k = 0; k < ENEMY_COUNT; k++)
    for (int i = 0; i < 32; i++) put16(rw(r, 0x7000 + 64 * k + 2 * i));
  fprintf(fc, "  { (void *)enemy_spawn_data, sizeof(enemy_spawn_data) },\n");
  for (int i = 0; i < 0x1000; i++) put16(rw(r, 0x7800 + 2 * i));
  fprintf(fc, "  { (void *)enemy_ram7800, sizeof(enemy_ram7800) },\n");
  /* nivel de la sala: el de la ROM más los bloques cambiados (room.h) */
  for (const VarMap *v = g_vars_map; v->name; v++)
    if (!strcmp(v->name, "roomdefroomstate_ptr")) roomdefroomstate_ptr = rw(r, v->addr);
  lvl_from_snes(r);
  put16(lvl_index);
  fprintf(fc, "  { (void *)&lvl_index, sizeof(lvl_index) },\n");
  for (unsigned i = 0; i < sizeof lvl_mod; i++) put8(lvl_mod[i]);
  fprintf(fc, "  { (void *)lvl_mod, sizeof(lvl_mod) },\n");
  for (unsigned i = 0; i < LVL_OV_N; i++) { put16(lvl_ov[i].idx); put16(lvl_ov[i].lvl); put8(lvl_ov[i].bts); put8(lvl_ov[i].used); }
  fprintf(fc, "  { (void *)lvl_ov, sizeof(lvl_ov) },\n  { 0, 0 }\n};\n");
  fclose(fb); fclose(fc);
  /* comprobación de tamaños: el blob debe medir lo mismo que las variables */
  long expect = 0;
  for (const VarMap *v = g_vars_map; v->name; v++) { expect += v->size * v->count; if (expect & 1) expect++; }
  expect += sizeof(EnemyData) * ENEMY_COUNT + sizeof(EnemySpawnData) * ENEMY_COUNT + 0x2000 + sizeof lvl_index + sizeof lvl_mod + sizeof lvl_ov;
  printf("init_state: %ld bytes (esperado %ld), EnemyData %zu\n", off, expect, sizeof(EnemyData));
  return off != expect;
}
