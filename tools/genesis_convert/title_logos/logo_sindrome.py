"""Builds the "Síndrome" boot logo from the letters of the original (index image logs/logo_idx_nin.npy, palette 0)."""
import numpy as np
X0, Y0 = 81, 106                      # letter area: rows 106..117
ROWCOL = [0xd, 0xc, 0xc, 0xb, 0xb, 0x9, 0x9, 0xb, 0xb, 0xc, 0xc, 0xd]   # stroke colour of each letter row

def glyphs(idx):
    L = idx[Y0:Y0 + 12, X0:X0 + 94].copy()
    g = {}
    for name, a, b in [('N', 9, 20), ('i', 22, 25), ('n', 27, 36), ('e', 43, 52), ('d', 65, 74), ('o', 76, 85)]:
        g[name] = L[:, a:b].copy()
    g['o'][0:4, :] = -1                                     # (the ® touches the top of the o column)
    n = g['n']
    r = n[:, 0:8].copy(); r[6:, 3:] = -1                      # r: the stem and the top of the n arch
    g['r'] = r
    g['m'] = np.concatenate([n[:, 0:6], n[:, 0:9]], axis=1)   # m: an arch flowing into a whole n
    i = g['i'].copy(); i[0:2, :] = -1
    acc = np.full((12, 4), -1, np.int16); acc[:, 0:3] = i
    acc[0, 2] = 0xd; acc[0, 3] = 0xd; acc[1, 1] = 0xc; acc[1, 2] = 0xc; acc[0, 1] = 0xe; acc[1, 0] = 0xe     # acute accent
    g['í'] = acc
    S = ['.e####e..',
         'e#######e',
         '###e..e##',
         '###......',
         'e###e....',
         '.e#####e.',
         '..e#####e',
         '....e###.',
         '......###',
         '##e..e###',
         'e#######e',
         '.e#####e.']
    s = np.full((12, 9), -1, np.int16)
    for y, row in enumerate(S):
        for x, ch in enumerate(row):
            if ch == '#': s[y, x] = ROWCOL[y]
            elif ch == 'e': s[y, x] = 0xe
    g['S'] = s
    return g

def build(idx):
    g = glyphs(idx)
    new = idx.copy()
    new[Y0:Y0 + 12, X0 + 5:X0 + 89] = -1                     # clear the old word (the oval and the ® stay)
    x = X0 + 6
    for ch in ['S', 'í', 'n', 'd', 'r', 'o', 'm', 'e']:
        gl = g[ch]
        m = gl >= 0
        new[Y0:Y0 + 12, x:x + gl.shape[1]][m] = gl[m]
        x += gl.shape[1] + 1
    reg = idx[Y0:Y0 + 5, X0 + 84:X0 + 90]                     # the ®, where it was
    dst = new[Y0:Y0 + 5, X0 + 84:X0 + 90]
    m = (reg >= 0) & (dst < 0)
    dst[m] = reg[m]
    return new, x - 1 - X0

if __name__ == '__main__':
    import logo_tool as Lt, pickle
    idx = np.load('logs/logo_idx_nin.npy')
    sn = pickle.load(open('logs/logo_snaps.pkl', 'rb'))
    new, end = build(idx)
    print('word ends at column', end)
    SP = 'C:/Users/edwin/AppData/Local/Temp/claude/C--sgdk/c8dcc669-e3af-4035-9d19-5a16e7879670/scratchpad/'
    img = np.concatenate([Lt.rgb(idx, sn[100][1])[96:128, 76:180], np.zeros((4, 104, 3), np.uint8), Lt.rgb(new, sn[100][1])[96:128, 76:180]])
    Lt.png(SP + 'sindrome.png', img, 4)
    for y in range(106, 118): print(''.join('.' if v < 0 else '%x' % (v % 16) for v in new[y, 81:176]))
