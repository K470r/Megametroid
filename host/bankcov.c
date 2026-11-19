/* MegaMetroid — qué bytes de las copias de los bancos del SNES (kBankXXB) lee el motor (docs/68).
 *
 * Se enlaza junto a un programa del PC (check, zcover...): al empezar protege la memoria de las copias (mprotect) y
 * cada lectura da una falla; el manejador anota el byte, abre la página y ejecuta esa instrucción sola (el bit de paso
 * a paso, TF), y al volver la cierra otra vez. Al terminar suma lo leído al archivo MM_BANKCOV (un bit por byte, 27
 * bancos de 32 KB; si ya existe se juntan). Solo x86-64 Linux. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#define B(x) extern const uint8_t kBank##x##B[32769];
B(86) B(91) B(94) B(A0) B(A1) B(A2) B(A3) B(A4) B(A5) B(A6) B(A7) B(A8) B(A9) B(AA) B(AD) B(B2) B(B3) B(B4)
B(81) B(82) B(88) B(8B) B(8D) B(87) B(84) B(8F) B(8C)
#undef B
#define NB 27
static const uint8_t *const kBanks[NB] = { kBank86B, kBank91B, kBank94B, kBankA0B, kBankA1B, kBankA2B, kBankA3B,
  kBankA4B, kBankA5B, kBankA6B, kBankA7B, kBankA8B, kBankA9B, kBankAAB, kBankADB, kBankB2B, kBankB3B, kBankB4B,
  kBank81B, kBank82B, kBank88B, kBank8BB, kBank8DB, kBank87B, kBank84B, kBank8FB, kBank8CB };
const uint8_t kBankCovIds[NB] = { 0x86, 0x91, 0x94, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA,
  0xAD, 0xB2, 0xB3, 0xB4, 0x81, 0x82, 0x88, 0x8B, 0x8D, 0x87, 0x84, 0x8F, 0x8C };
static uint8_t cov[NB][32769 / 8 + 1];
static uintptr_t pg_lo, pg_hi, open_pg;
static long psz;

static void mark(uintptr_t a) {
  for (int i = 0; i < NB; i++) {
    uintptr_t b = (uintptr_t)kBanks[i];
    if (a >= b && a < b + 32769) { uintptr_t o = a - b; cov[i][o >> 3] |= 1 << (o & 7); }
  }
}
static void on_segv(int sig, siginfo_t *si, void *ctx) {
  (void)sig;
  uintptr_t a = (uintptr_t)si->si_addr;
  if (a < pg_lo || a >= pg_hi) { signal(SIGSEGV, SIG_DFL); return; }
  mark(a);
  mark(a + 1);                                   /* (lecturas de dos bytes: el otro también) */
  open_pg = a & ~(uintptr_t)(psz - 1);
  mprotect((void *)open_pg, psz * 2, PROT_READ);
  ((ucontext_t *)ctx)->uc_mcontext.gregs[REG_EFL] |= 0x100;
}
static void on_trap(int sig, siginfo_t *si, void *ctx) {
  (void)sig; (void)si;
  if (open_pg) { mprotect((void *)open_pg, psz * 2, PROT_NONE); open_pg = 0; }
  ((ucontext_t *)ctx)->uc_mcontext.gregs[REG_EFL] &= ~0x100;
}
static void save(void) {
  const char *f = getenv("MM_BANKCOV");
  if (!f) return;
  mprotect((void *)pg_lo, pg_hi - pg_lo, PROT_READ);
  static uint8_t old[NB][32769 / 8 + 1];
  FILE *h = fopen(f, "rb");
  if (h) { if (fread(old, 1, sizeof old, h) == sizeof old) for (int i = 0; i < NB; i++) for (unsigned k = 0; k < sizeof old[0]; k++) cov[i][k] |= old[i][k]; fclose(h); }
  h = fopen(f, "wb");
  if (h) { fwrite(cov, 1, sizeof cov, h); fclose(h); }
}
__attribute__((constructor)) static void bankcov_init(void) {
  if (!getenv("MM_BANKCOV")) return;
  psz = sysconf(_SC_PAGESIZE);
  uintptr_t lo = ~(uintptr_t)0, hi = 0;
  for (int i = 0; i < NB; i++) {
    uintptr_t b = (uintptr_t)kBanks[i];
    if (b < lo) lo = b;
    if (b + 32769 > hi) hi = b + 32769;
  }
  pg_lo = lo & ~(uintptr_t)(psz - 1);
  pg_hi = (hi + psz - 1) & ~(uintptr_t)(psz - 1);
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = on_segv; sigaction(SIGSEGV, &sa, 0);
  sa.sa_sigaction = on_trap; sigaction(SIGTRAP, &sa, 0);
  atexit(save);
  mprotect((void *)pg_lo, pg_hi - pg_lo, PROT_NONE);
}
