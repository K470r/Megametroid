/* v3: por sala: descenso en zigzag en el pozo; en las demás, correr hacia la meta y saltar si se traba */
static int stuck, jump_t, phase, dir; static uint16_t lastx, lasty, lastroom; static long still;
static void bot_init(void) { stuck = 0; jump_t = 0; lastx = 0; phase = 0; still = 0; dir = RI; lastroom = 0; }

/* subida en la huida: puntos (x, y, salto) por sala; y = centro de Samus parada */
typedef struct { uint16_t room; short x, y, jump, lx; } Wp;
static const Wp kWp[] = {
  {0xDFD7, 44, 347, 0}, {0xDFD7, 84, 267, 1}, {0xDFD7, 204, 219, 0}, {0xDFD7, 150, 139, 1}, {0xDFD7, 8, 139, 0},
  {0xDF45, 170, 667, 0}, {0xDF45, 95, 571, 2}, {0xDF45, 80, 571, 0}, {0xDF45, 110, 475, 1}, {0xDF45, 115, 475, 0},
  {0xDF45, 70, 379, 2}, {0xDF45, 40, 379, 0}, {0xDF45, 178, 363, 4, 86}, {0xDF45, 188, 363, 0}, {0xDF45, 140, 267, 1}, {0xDF45, 138, 267, 0},
  {0xDF45, 180, 171, 1}, {0xDF45, 190, 171, 0}, {0xDF45, 128, 75, 1},
  {0, 0, 0, 0} };
static int wpi = -1, grounded, aheld, hopt; static uint16_t wroom, py;
static int waypoint(long f, uint16_t room, uint16_t x, uint16_t y) {
  if (wroom != room) { wroom = room; wpi = -1; for (int i = 0; kWp[i].room; i++) if (kWp[i].room == room) { wpi = i; break; } }
  if (wpi < 0 || kWp[wpi].room != room) return -1;
  if (w16(0xB36) == 0 && y == py) grounded++; else grounded = 0; py = y;
  const Wp *w = &kWp[wpi];
  int dx = w->x - x;
  if (abs(dx) < 10 && y <= w->y + 4 && grounded > 2) { fprintf(flog, "%ld WP %d ok x=%d y=%d\n", f, wpi, x, y); wpi++; return 0; }
  int in = dx > 4 ? RI : dx < -4 ? LE : 0;
  if (w->jump == 4) {                                            /* carrera (B) y salto al llegar a lx */
    in |= B_;
    if (grounded > 2 && aheld) { aheld = 0; return in & ~A_; }
    if (!aheld && grounded > 0 && ((in & RI) ? x >= w->lx : x <= w->lx)) aheld = 1;
    if (aheld == 1 && w16(0xB36) != 2) in |= A_; else if (aheld) aheld = 2;
    return in;
  }
  if (w->jump && (y > w->y + 8 || aheld)) {
    if (grounded > 2 && aheld) { aheld = 0; return 0; }       /* soltar A para volver a saltar */
    if (grounded > 2) hopt = 0; else hopt++;
    if (w->jump == 3 && aheld == 1 && hopt > 8) aheld = 2;
    if (grounded > 2 || (aheld == 1 && w16(0xB36) != 2)) { aheld = 1; in |= A_; }
    else if (aheld && w16(0xB36) == 2) aheld = 2;
    if (w->jump == 1 && (grounded > 0 || y > w->y - 2)) in &= ~(LE | RI);          /* derecho hacia arriba hasta pasar la saliente */
  } else aheld = 0;
  return in;
}
static int bot(long f) {
  uint16_t st = w16(0x998), room = w16(0x79B), x = w16(0xAF6), y = w16(0xAFA), hp = w16(0x9C2), cs = w16(0x93F);
  if (f % 20 == 0 || (f > 18081 && f < 18200)) fprintf(flog, "%ld st=%d room=%04X x=%d y=%d hp=%d ceres=%04X ph=%d yd=%d\n", f, st, room, x, y, hp, cs, phase, w16(0xB36));
  if (room == 0xDF45 && cs & 0x8000) { static FILE *sw; if (!sw) sw = fopen("sway.txt","w"); fprintf(sw, "%ld %d %d %d %d %d %d %d %d\n", f, (int16_t)w16(0x78), (int16_t)w16(0x7A), (int16_t)w16(0x7C), w16(0x80), w16(0x82), w16(0x7E1), (int16_t)w16(0x7E3), st); }
  if (st != 8) return 0;
  if (room != lastroom) { lastroom = room; stuck = -30; jump_t = 0; dir = (cs != 0) ? LE : RI; fprintf(flog, "%ld SALA %04X x=%d y=%d\n", f, room, x, y); }
  { static uint16_t dumped[64]; static int nd; int k; for(k=0;k<nd;k++) if(dumped[k]==room) break;
    if(k==nd && nd<64 && f % 20 == 10){ dumped[nd++]=room; char fn[64]; sprintf(fn,"lvl_%04X.bin",room); FILE*q=fopen(fn,"wb"); fwrite(g_ram+0x10000,1,0x6000,q); fwrite(g_ram+0x7A5,1,2,q); fclose(q);} }
  if (phase == 0) {
    if (y == lasty) still++; else still = 0;
    lasty = y;
    if (still > 30) phase = 1;
    return 0;
  }
  bool escape = cs != 0;
  if (x == lastx) stuck++; else stuck = 0;
  lastx = x;
  if ((room == 0xDF45 && !escape && y < 650) || (room == 0xDFD7 && !escape && y < 380)) {          /* pozo: zigzag hacia abajo */
    if (stuck > 6) { dir = dir == RI ? LE : RI; stuck = 0; }
    return dir;
  }
  if (escape) { int r = waypoint(f, room, x, y); if (r >= 0) return r; }
  if (escape && x < 30 && y < 100) { jump_t = 0; stuck = 0; return LE | ((f & 15) < 3 ? X_ : 0); }
  int want = escape ? LE : RI;
  int in = want | B_;
  if (room == 0xE0B5 && !escape) return 0;          /* Ridley: esperar (se va con la energía baja) */
  if (stuck > 8 && stuck < 40) { if ((stuck & 7) < 2) in |= X_; return in; }   /* trabado: disparar (puertas) */
  if (jump_t > 0) { jump_t--; in |= A_; }
  else if (stuck >= 40) { jump_t = 45; stuck = 0; }
  return in;
}
