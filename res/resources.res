// MegaMetroid — recursos propios para rescomp de SGDK (docs/47). Formato: $(GDK)/bin/rescomp.txt
//
// Aquí va solo el arte nuestro (sin nada de la ROM de Nintendo): logos, fuentes, gráficos nuevos. Lo que sale de la
// ROM lo generan los conversores en res/gen/ (fuera de git) y los redibujos hechos sobre la ROM van en res/private/
// (fuera de git, ver docs/47).
//
// Mientras este archivo no declare nada, `make` no lo compila y la ROM sale igual byte a byte. Al agregar un recurso,
// su .h queda en out/obj/res_resources.h (incluirlo con -I$(OUT)/obj) y sus datos en la parte fija de la ROM
// (máximo 4 MB, docs/46).
//
// Ejemplos:
//   PALETTE pal_logo   "gfx/logo.png"
//   IMAGE   img_logo   "gfx/logo.png"   BEST
//   SPRITE  spr_samus  "sprite/samus.png" 4 4 FAST 5
