"""Logo editing helpers: the boot "Nintendo" logo and the title logo are OBJ sprites of the title sheet (0x9580D8,
decompressed).  compose() rebuilds them as palette-index images from an OAM snapshot; the edited images are cut back
into the same sheet tiles by recut()."""
import json, numpy as np, os
HERE = os.path.dirname(os.path.abspath(__file__))

def snapshots(frames):
    log = json.load(open(os.path.join(HERE, 'logs', 'escape_log.json')))
    d = ctypes.CDLL(os.path.join(HERE, 'samus_fast.dll')); assert d.port_host_load_rom(b'C:/proyectos/sm/sm.smc') == 0
    d.port_host_cold_reset(); d.port_host_gb_init()
    d.port_host_oam.restype = ctypes.POINTER(ctypes.c_uint8); d.port_host_cgram.restype = ctypes.POINTER(ctypes.c_uint8)
    out = {}
    for f, (k, p) in enumerate(log):
        for a, vv in p: d.port_host_poke16(a, vv)
        d.port_host_frame(k)
        if f in frames: out[f] = (bytes(d.port_host_oam()[0:544]), bytes(d.port_host_cgram()[0:512]))
        if f >= max(frames): break
    return out

def sheet():
    return bytearray(sm_lz.decompress(0x9580D8))

def tile_px(sh, t):
    p = sh[t * 32:(t + 1) * 32]; px = np.zeros((8, 8), np.uint8)
    for y in range(8):
        for x in range(8):
            m = 7 - x; px[y, x] = (p[y*2] >> m & 1) | (p[y*2+1] >> m & 1) << 1 | (p[16+y*2] >> m & 1) << 2 | (p[16+y*2+1] >> m & 1) << 3
    return px

def set_tile(sh, t, px):
    b = bytearray(32)
    for y in range(8):
        for x in range(8):
            v = int(px[y, x]); m = 7 - x
            b[y*2] |= (v & 1) << m; b[y*2+1] |= (v >> 1 & 1) << m; b[16+y*2] |= (v >> 2 & 1) << m; b[16+y*2+1] |= (v >> 3 & 1) << m
    sh[t * 32:(t + 1) * 32] = b

def sprites(o):
    spr = []
    for i in range(128):
        y = o[i*4+1]
        if y >= 0xe0: continue
        hi = (o[512 + (i >> 2)] >> ((i & 3) * 2)) & 3
        x = o[i*4] | (hi & 1) << 8
        if x >= 256: x -= 512
        fl = o[i*4+3]
        spr.append(dict(x=x, y=y, t=o[i*4+2] | (fl & 1) << 8, pal=(fl >> 1) & 7, big=hi >> 1, hf=(fl >> 6) & 1, vf=(fl >> 7) & 1, pri=(fl >> 4) & 3))
    return spr

def cells(s):
    """(sheet tile, screen x, screen y) of the 8x8 cells of a sprite (no flips in the logos)"""
    n = 2 if s['big'] else 1; t = s['t']
    for r in range(n):
        for c in range(n):
            tt = (t & 0x100) | ((((t >> 4) & 15) + r) & 15) << 4 | (((t & 15) + c) & 15)
            yield tt, s['x'] + c * 8, s['y'] + r * 8

def compose(sh, spr):
    """index image (-1 transparent, else pal*16 + colour) of the sprites, first sprite on top"""
    idx = np.full((224, 256), -1, np.int16)
    for s in reversed(spr):
        assert not s['hf'] and not s['vf']
        for tt, X, Y in cells(s):
            px = tile_px(sh, tt)
            for yy in range(8):
                for xx in range(8):
                    if px[yy, xx] and 0 <= X + xx < 256 and 0 <= Y + yy < 224: idx[Y + yy, X + xx] = s['pal'] * 16 + px[yy, xx]
    return idx

def rgb(idx, cg):
    img = np.zeros(idx.shape + (3,), np.uint8)
    for (y, x), v in np.ndenumerate(idx):
        if v >= 0:
            ci = 128 + v; c = cg[ci * 2] | cg[ci * 2 + 1] << 8
            img[y, x] = ((c & 31) * 255 // 31, (c >> 5 & 31) * 255 // 31, (c >> 10 & 31) * 255 // 31)
    return img

def recut(sh, spr, new):
    """write the edited index image back into the sheet tiles of the sprites; a tile used at two places must agree"""
    seen = {}
    for s in spr:
        for tt, X, Y in cells(s):
            px = np.zeros((8, 8), np.uint8)
            for yy in range(8):
                for xx in range(8):
                    v = new[Y + yy, X + xx] if 0 <= Y + yy < 224 and 0 <= X + xx < 256 else -1
                    if v >= 0:
                        assert v // 16 == s['pal'], (tt, v, s['pal'])
                        px[yy, xx] = v % 16
            if tt in seen: assert (seen[tt] == px).all(), 'tile %d used twice with different pixels' % tt
            seen[tt] = px
            set_tile(sh, tt, px)

def png(path, img, scale=3):
    import zlib, struct
    img = img.repeat(scale, 0).repeat(scale, 1); h, w, _ = img.shape
    raw = b''.join(b'\0' + img[y].tobytes() for y in range(h))
    def ch(t, dd): return struct.pack('>I', len(dd)) + t + dd + struct.pack('>I', zlib.crc32(t + dd) & 0xffffffff)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + ch(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + ch(b'IDAT', zlib.compress(raw)) + ch(b'IEND', b''))
