"""Builds "MEGA" in the style of the "SUPER" line of the title logo (palette 2 of the title sprites): upright letter masks,
the perspective of the logo (vertical strokes converge towards a point above the centre), gradient + dither by row, the purple
3D extrusion below / right."""
import numpy as np
Y0, Y1, XC = 16, 38, 128            # face rows 16..37, perspective centre
ALPHA = 0.0115                      # horizontal shrink per row going up

def mask(rows):
    return np.array([[c == '#' for c in r] for r in rows], bool)

def glyph(kind, w, st=7):
    """upright 22-row letters of the logo font: stems st wide, bars 4-5 rows"""
    g = np.zeros((22, w), bool)
    if kind == 'M':
        g[:, :st] = True; g[:, w - st:] = True
        inner0, inner1 = st, w - st - 1                   # columns between the stems
        for r in range(22):
            off = int(r * 0.5)
            lo, hi = inner0 + off, inner1 - off           # the two arms of the V move towards the middle
            if lo > hi: break
            g[r, lo:min(lo + 4, hi + 1)] = True; g[r, max(hi - 3, lo):hi + 1] = True
    elif kind == 'E':
        g[:, :st] = True; g[0:5, :] = True; g[9:13, :w - 3] = True; g[17:22, :] = True
    elif kind == 'G':
        g[0:5, 2:] = True; g[1:5, 1:] = True; g[3:, :st] = True; g[17:22, :] = True
        g[0, :3] = False; g[1, :2] = False; g[2, :1] = False; g[21, :2] = False; g[20, :1] = False
        g[5:8, w - st:] = True                            # the top terminal
        g[10:22, w - st:] = True; g[10:14, w - st - 5:] = True
    elif kind == 'A':
        g[:, :st] = True; g[:, w - st:] = True; g[0:5, :] = True; g[10:14, :] = True
        g[0, :3] = False; g[1, :2] = False; g[2, :1] = False; g[0, w - 3:] = False; g[1, w - 2:] = False; g[2, w - 1:] = False
    return g

M = glyph('M', 26); E_ = glyph('E', 18); G = glyph('G', 21); A = glyph('A', 22)

def fill(y, x):
    """face colour of row y (palette 2 index): gradient 2 (top) .. 6 (bottom) with dithered transitions"""
    d = (x + y) & 1
    if y <= 20: return 2
    if y <= 22: return 3 if d else 2
    if y <= 26: return 3
    if y <= 28: return 4 if d else 3
    if y <= 33: return 4
    if y <= 35: return 5 if d else 4
    if y == 36: return 5
    return 6

def build(idx, letters=None, gap=2):
    letters = letters or (M, E_, G, A)
    base_w = sum(l.shape[1] for l in letters) + gap * (len(letters) - 1)
    bx = XC - base_w // 2
    face_base = np.zeros((Y1 - Y0, 256), bool)
    x = bx
    for l in letters:
        face_base[:, x:x + l.shape[1]] |= l
        x += l.shape[1] + gap
    face = np.zeros((224, 256), bool)
    for y in range(Y0, Y1):
        s = 1 - (Y1 - 1 - y) * ALPHA
        for sx in range(256):
            b = int(round(XC + (sx + 0.5 - XC) / s - 0.5))
            if 0 <= b < 256 and face_base[y - Y0, b]: face[y, sx] = True
    new = idx.copy()
    new[Y0 - 2:Y1, 76:181] = -1                          # the old word, the foot of its S included (METROID starts at row 38)
    ext = {}
    for k, col in ((3, 10), (2, 11), (1, 12)):           # 3D side: nearest the face darkest purple
        for dy, dx in ((k, 0), (k, 1)):
            sh = np.zeros_like(face); sh[dy:, dx:] = face[:-dy or None, :256 - dx]
            for (y, x) in zip(*np.nonzero(sh & ~face)): ext[(y, x)] = col
    for (y, x), c in ext.items():
        if new[y, x] < 0 or new[y, x] // 16 == 2: new[y, x] = 2 * 16 + c
    for y, x in zip(*np.nonzero(face)):
        up = y > 0 and face[y - 1, x]; left = face[y, x - 1]; right = face[y, x + 1]
        c = fill(y, x)
        if not up: c = 1                                 # top edge: white
        elif not left: c = 1 if y < 20 else 4            # left edge: highlight, darker lower down
        elif not right and y >= 18: c = 4                # right edge: shade
        new[y, x] = 2 * 16 + c
    return new

if __name__ == '__main__':
    import logo_tool as Lt, pickle
    idx = np.load('logs/logo_idx_tit.npy'); sn = pickle.load(open('logs/logo_snaps.pkl', 'rb'))
    new = build(idx)
    SP = 'C:/Users/edwin/AppData/Local/Temp/claude/C--sgdk/c8dcc669-e3af-4035-9d19-5a16e7879670/scratchpad/'
    a = Lt.rgb(idx, sn[350][1])[10:84, 40:220]; b = Lt.rgb(new, sn[350][1])[10:84, 40:220]
    Lt.png(SP + 'mega.png', np.concatenate([a, np.zeros((4, 180, 3), np.uint8), b]), 3)
