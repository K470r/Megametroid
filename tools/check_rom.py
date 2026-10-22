#!/usr/bin/env python3
"""MegaMetroid — comprueba la ROM del SNES que cada quien saca de su propio cartucho (docs/43).

Uso: python3 tools/check_rom.py [archivo]      (sin archivo: sm/sm.smc)

Acepta la ROM con o sin la cabecera de copiadora de 512 bytes; si la tiene, escribe sm/sm.smc sin ella.
La única versión que sirve es Super Metroid (Japón / EE. UU.), 3 MB:
    SHA-1  da957f0d63d14cb441d215462904c4fa8519c613
    MD5    21f3e98df4780ee1c667b84e57d88675
"""
import hashlib
import pathlib
import sys

SHA1 = "da957f0d63d14cb441d215462904c4fa8519c613"
ROOT = pathlib.Path(__file__).resolve().parent.parent
DEST = ROOT / "sm" / "sm.smc"


def main():
    src = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else DEST
    if not src.exists():
        print(f"no encuentro {src}: copia el volcado de tu cartucho en sm/sm.smc")
        return 1
    data = src.read_bytes()
    if len(data) % 0x8000 == 512:                     # cabecera de copiadora
        data = data[512:]
    sha1 = hashlib.sha1(data).hexdigest()
    if sha1 != SHA1:
        print(f"{src}: SHA-1 {sha1}, no es la ROM esperada ({SHA1})")
        print("hace falta Super Metroid (JU), 3 MB, sin parches ni traducciones")
        return 1
    if src.resolve() != DEST.resolve() or len(data) != src.stat().st_size:
        DEST.parent.mkdir(parents=True, exist_ok=True)
        DEST.write_bytes(data)
        print(f"ROM correcta, copiada a {DEST}")
    else:
        print(f"ROM correcta: {DEST}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
