"""MegaMetroid — qué efectos de sonido suenan en Zebes (docs/55).

Edwin: después de la llegada a Zebes, por ahora solo los efectos de Samus y de las puertas. Se sacan del motor y de tu
ROM, no se escriben a mano:
  - los que piden el código de Samus, de los proyectiles, del HUD y de las puertas (src/core/samus_*.c, proj.c,
    hud.c, door.c: llamadas con número fijo, más las tablas que leen: los disparos de cada rayo...);
  - los de las listas de instrucciones de los PLM de puertas (cabeceras con PlmSetup_Door_*, PlmSetup_C794_GreyDoor y
    las que cierran la puerta al entrar: kDoorClosingPlmIds, $8F:E68A), con tools/conv/plmscan.py.
Devuelve {(biblioteca, efecto)}."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "conv"))
from plmscan import instr_table                      # noqa: E402

SAMUS_FILES = ["samus_codes.c", "samus_move.c", "samus_pose.c", "samus_draw.c", "proj.c", "hud.c", "door.c"]
DOOR_SETUPS = {0xC794, 0xC7B1, 0xC7BB, 0xC7E2}       # PlmSetup_C794_GreyDoor, _Door_Colored, _Door_Blue, _Door_Strange


def from_code():
    out = set()
    for fn in SAMUS_FILES:
        src = (ROOT / "src" / "core" / fn).read_text()
        for lib, num in re.findall(r"QueueSfx([123])_Max\d+\((0x[0-9A-Fa-f]+|\d+)\)", src):
            out.add((int(lib), int(num, 0)))
        # números elegidos en el código: Samus_Move ($90:...: pasos en el agua, 15 o 17) y las tablas de door.c
        for lib, a, b in re.findall(r"QueueSfx([123])_Max\d+\(\(NextRandom\(\) & 1\) \? (\d+) : (\d+)\)", src):
            out.update({(int(lib), int(a)), (int(lib), int(b))})
        # tablas de números (kSfx de door.c, kUnchargedProjectile_Sfx de proj.c...): la biblioteca es la de la llamada
        for lib, tab in re.findall(r"QueueSfx([123])_Max\d+\((k\w+)\[", src):
            m = re.search(r"static const uint(?:8|16) %s\[\d+\] = \{([^}]*)\}" % tab, src)
            out.update((int(lib), int(x, 0)) for x in m.group(1).replace(" ", "").split(",") if int(x, 0))
    return out


def from_door_plms(rom):
    def w(a):
        o = ((a >> 16) & 0x7F) * 0x8000 + (a & 0x7FFF)
        return rom[o] | rom[o + 1] << 8
    tab = instr_table()
    heads = set()
    for a in range(0x8000, 0xFFFC, 2):
        if w(0x840000 | a) in DOOR_SETUPS and w(0x840000 | a + 2) >= 0x8000:
            heads.add(a)
    for i in range(8):
        h = w(0x8FE68A + 2 * i)
        if h >= 0x8000:
            heads.add(h)
    out, seen = set(), set()
    todo = [w(0x840000 | h + off) for h in heads for off in (2, 4)]
    while todo:
        p = todo.pop()
        while 0x8000 <= p < 0x10000 and p not in seen:
            seen.add(p)
            f = w(0x840000 | p)
            if f < 0x8000:
                p += 4
                continue
            t = tab.get(f)
            if t is None:
                break
            ln, br, stop, name = t
            m = re.match(r"PlmInstr_QueueSfx([123])_Max\d+", name)
            if m:
                o = ((0x840000 | p + 2) >> 16 & 0x7F) * 0x8000 + ((p + 2) & 0x7FFF)
                out.add((int(m.group(1)), rom[o]))
            for b in br:
                todo.append(w(0x840000 | p + 2 + b))
            if name == "PlmInstr_Call":
                todo.append(p + 4)
            if stop or name in ("PlmInstr_Goto", "PlmInstr_Return", "PlmInstr_Call", "PlmInstr_Sleep", "PlmInstr_Delete"):
                break
            p += 2 + ln
    return out


def zebes_sfx(rom):
    s = from_code() | from_door_plms(rom)
    return {x for x in s if x[1]}


if __name__ == "__main__":
    rom = open(sys.argv[1], "rb").read()
    rom = rom[len(rom) & 0x7FFF:]
    print(sorted(from_code()))
    print(sorted(from_door_plms(rom)))
