#!/usr/bin/env python3
"""MegaMetroid — genera gen/enums.h y gen/fn.h desde los encabezados del decomp (docs/31).

enums.h: los enum de ida_types.h (poses, tipos de movimiento, estados, constantes con direcciones de tablas
         `addr_k...`), salvo Buttons (el motor tiene los suyos en nt.h).
fn.h:    las direcciones de funciones del SNES (`#define fnX 0x...` de funcs.h) y FUNC16(x). Son solo números
         de 16 bits que el juego guarda en variables (p. ej. door_transition_function); el motor nativo hace
         switch sobre ellos, igual que el decomp, para que la RAM se pueda comparar con el SNES.

Uso: gen_enums.py <decomp/src> <salida gen/>
"""
import re
import sys
import os

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
SKIP = {"Buttons"}

ida = open(os.path.join(src, "ida_types.h"), encoding="latin-1").read()
blocks = []
for m in re.finditer(r"^(typedef\s+)?enum\s+(\w+)\s*\{(.*?)\}\s*(\w*)\s*;", ida, re.S | re.M):
    name = m.group(2)
    if name in SKIP:
        continue
    body = m.group(3)
    blocks.append("enum %s {%s};\n" % (name, body))

with open(os.path.join(out, "enums.h"), "w") as f:
    f.write("/* generado por native/tools/gen_enums.py desde ida_types.h del decomp — no editar */\n")
    f.write("#ifndef GEN_ENUMS_H\n#define GEN_ENUMS_H\n")
    f.writelines(blocks)
    f.write("#endif\n")

funcs = open(os.path.join(src, "funcs.h"), encoding="latin-1").read()
defs = re.findall(r"^#define (fn\w+) (0x[0-9A-Fa-f]+)", funcs, re.M)
with open(os.path.join(out, "fn.h"), "w") as f:
    f.write("/* generado por native/tools/gen_enums.py desde funcs.h del decomp — no editar */\n")
    f.write("#ifndef GEN_FN_H\n#define GEN_FN_H\n")
    for n, v in defs:
        f.write("#define %s %s\n" % (n, v))
    f.write("#define FUNC16(x) ((uint16)((fn##x) & 0xffff))\n#endif\n")
print("enums:", len(blocks), "fn:", len(defs))
