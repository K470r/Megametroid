/* MegaMetroid — compresor de los trozos de tiles de las salas (formato MZ, docs/48). Lo usa mkzebes.py.
 *
 * Formato (bits de mayor a menor, mezclados con los bytes: cuando hace falta un bit nuevo se toma el byte que sigue,
 * como ZX0). El largo de la salida se sabe de antes (tiles x 32): no hay marca de fin.
 *   gamma(v), v >= 1: Elias gamma intercalado: mientras llegue un 0, v = v * 2 + el bit que sigue; termina con un 1.
 *   empieza con LITERALES:
 *     LITERALES: n = gamma, n bytes tal cual. Después: bit 0 -> REPETIR, bit 1 -> NUEVO
 *     REPETIR:   n = gamma, copia n bytes desde el último desplazamiento. Después: bit 0 -> LITERALES, 1 -> NUEVO
 *     NUEVO:     h = gamma, b = byte: d = (h - 1) * 128 + (b >> 1) + 1; n = gamma + 1, con el primer bit de ese gamma
 *                en b & 1 (como ZX0). Después: bit 0 -> LITERALES, 1 -> NUEVO
 *   (el último desplazamiento empieza en 1)
 * El parseo es óptimo con el estado "termina en literales / en copia" y el desplazamiento del mejor camino.
 *
 * Uso: mzpack < entrada > salida. Entrada: repetido (u32 largo, bytes). Salida: repetido (u32 largo, bytes). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXN 65536
#define INF 0x3FFFFFFF

static int gbits(unsigned v) { int n = 0; while (v > 1) { v >>= 1; n++; } return 2 * n + 1; }

/* camino: por posición y estado (0 literales, 1 copia) */
typedef struct { int cost; int from; uint8_t fst; uint8_t kind; int len; int off; int last; } Node;
static Node nd[MAXN + 1][2];

static uint8_t outb[MAXN * 2 + 64];
static int on, bitpos, nbit;
static void put_bit(int b) {
  if (nbit == 0) { bitpos = on++; outb[bitpos] = 0; nbit = 8; }
  nbit--;
  if (b) outb[bitpos] |= 1 << nbit;
}
static void put_gamma(unsigned v) {
  int n = 0; unsigned t = v;
  while (t > 1) { t >>= 1; n++; }
  for (int i = n - 1; i >= 0; i--) { put_bit(0); put_bit((v >> i) & 1); }
  put_bit(1);
}
static void put_byte(int b) { outb[on++] = (uint8_t)b; }

static int pack(const uint8_t *in, int n) {
  for (int i = 0; i <= n; i++) nd[i][0].cost = nd[i][1].cost = INF;
  /* empieza con literales: desde el "estado copia" ficticio en 0 sin bit de control */
  nd[0][1].cost = 0; nd[0][1].last = 1;
  static int mlen[MAXN];
  for (int i = 0; i < n; i++) {
    /* LITERALES desde un estado copia (o el principio) */
    if (nd[i][1].cost < INF) {
      int base = nd[i][1].cost + (i ? 1 : 0);              /* (bit 0 después de una copia; al principio, nada) */
      for (int L = 1; i + L <= n && L <= 4096; L++) {
        int c = base + gbits(L) + 8 * L;
        if (c < nd[i + L][0].cost) {
          nd[i + L][0] = (Node){ c, i, 1, 0, L, 0, nd[i][1].last };
        }
      }
    }
    if (i == 0) continue;
    /* copias: para cada largo, el desplazamiento más chico que lo da */
    int maxl = 0;
    int lim = i < 2048 ? i : 2048;
    for (int d = 1; d <= lim; d++) {
      int l = 0;
      while (i + l < n && in[i + l] == in[i + l - d]) l++;
      mlen[d] = l;
      if (l > maxl && l >= 2) {
        /* NUEVO desde literales o desde copia (bit 1) */
        int hc = gbits(((d - 1) >> 7) + 1) + 8 - 1;   /* (un bit del largo va en el byte) */
        for (int s = 0; s < 2; s++) {
          if (nd[i][s].cost >= INF) continue;
          int base = nd[i][s].cost + 1 + hc;
          for (int L = maxl + 1 > 2 ? maxl + 1 : 2; L <= l; L++) {
            int c = base + gbits(L - 1);
            if (c < nd[i + L][1].cost) nd[i + L][1] = (Node){ c, i, (uint8_t)s, 2, L, d, d };
          }
        }
        maxl = l;
      }
    }
    /* REPETIR desde literales (bit 0) con el desplazamiento de ese camino */
    if (nd[i][0].cost < INF) {
      int d = nd[i][0].last;
      if (d <= i) {
        int l = d <= lim ? mlen[d] : 0;
        if (d > lim) { l = 0; while (i + l < n && in[i + l] == in[i + l - d]) l++; }
        int base = nd[i][0].cost + 1;
        for (int L = 1; L <= l; L++) {
          int c = base + gbits(L);
          if (c < nd[i + L][1].cost) nd[i + L][1] = (Node){ c, i, 0, 1, L, d, d };
        }
      }
    }
  }
  /* el mejor final y el camino hacia atrás */
  static int pth[MAXN + 1][2];
  int s = nd[n][0].cost <= nd[n][1].cost ? 0 : 1, i = n, k = 0;
  while (i > 0) {
    pth[k][0] = i; pth[k][1] = s; k++;
    Node *x = &nd[i][s];
    s = x->fst; i = x->from;
  }
  on = 0; nbit = 0;
  for (int j = k - 1; j >= 0; j--) {
    Node *x = &nd[pth[j][0]][pth[j][1]];
    int first = j == k - 1;
    if (x->kind == 0) {                                      /* literales */
      if (!first) put_bit(0);
      put_gamma(x->len);
      for (int t = 0; t < x->len; t++) put_byte(in[x->from + t]);
    } else if (x->kind == 1) {                               /* repetir */
      put_bit(0);
      put_gamma(x->len);
    } else {                                                 /* nuevo */
      put_bit(1);
      put_gamma(((x->off - 1) >> 7) + 1);
      /* el largo: su primer bit (1: termina; 0: sigue) va en el bit 0 del byte del desplazamiento */
      unsigned v = (unsigned)x->len - 1; int nb = 0; unsigned t = v;
      while (t > 1) { t >>= 1; nb++; }
      put_byte((((x->off - 1) & 0x7F) << 1) | (nb == 0));
      for (int q = nb - 1; q >= 0; q--) {
        if (q != nb - 1) put_bit(0);
        put_bit((v >> q) & 1);
      }
      if (nb) put_bit(1);
    }
  }
  return on;
}

int main(void) {
  static uint8_t in[MAXN];
  uint32_t n;
  while (fread(&n, 4, 1, stdin) == 1) {
    if (n > MAXN || fread(in, 1, n, stdin) != n) { fprintf(stderr, "mzpack: entrada mala\n"); return 1; }
    uint32_t m = n ? (uint32_t)pack(in, (int)n) : 0;
    fwrite(&m, 4, 1, stdout);
    fwrite(outb, 1, m, stdout);
  }
  return 0;
}
