"""ROM data patch for the port's own branding: the boot "Nintendo" logo becomes "Síndrome" and the title "SUPER METROID"
becomes "MEGA METROID".  Both logos are sprites of the title sheet (0x9580D8, decompressed); the sprite layouts come from
extracted/art/logo_layout.json (OAM snapshots of the running engine)."""
import json, os
import numpy as np
import logo_tool as Lt, logo_sindrome as LS, logo_mega as LM

SHEET = 0x9580D8

def apply(data):
    lay = json.load(open(os.path.join(Lt.HERE, '..', '..', 'extracted', 'art', 'logo_layout.json')))
    sh = bytearray(data)
    for name, builder in (('nintendo', lambda i: LS.build(i)[0]), ('title', LM.build)):
        spr = lay[name]
        Lt.recut(sh, spr, builder(Lt.compose(sh, spr)))
    return bytes(sh)
