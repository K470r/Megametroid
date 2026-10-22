#!/usr/bin/env python3
"""Copia el decomp (snesrev/sm, src/) a otra carpeta y le agrega el filtro de enemigos del visor de salas:
con g_viewer_noenemy = 1 (hlref lo enciende con HL_WARP) las salas de Zebes se cargan sin enemigos, salvo la nave
de Samus, igual que el visor de la Genesis (native/src/viewer.c).
Uso: patch_decomp.py smsrc/src destino"""
import shutil, sys
from pathlib import Path
src, dst = Path(sys.argv[1]), Path(sys.argv[2])
if dst.exists():
    shutil.rmtree(dst)
shutil.copytree(src, dst)
p = dst / 'sm_a0.c'
s = p.read_text()
old = '''  do {
    LoadEnemyGfxIndexes(v4, v5);
    EnemyPopulation *EP = get_EnemyPopulation(0xa1, v4);'''
assert old in s, 'no encuentro el bucle de InitializeEnemies'
s = s.replace(old, '''  do {
    { extern int g_viewer_noenemy; uint16 ep = get_EnemyPopulation(0xa1, v4)->enemy_ptr;
      if (g_viewer_noenemy && area_index != 6 && ep != 0xD07F && ep != 0xD0BF) { v4 += 16; continue; } }   /* MegaMetroid: visor */
    LoadEnemyGfxIndexes(v4, v5);
    EnemyPopulation *EP = get_EnemyPopulation(0xa1, v4);''')
p.write_text(s + '\nint g_viewer_noenemy;\n')
# HL_NOM7 (snap_getaway.py): la imagen sin la capa del modo 7 (BG1 en el modo 7 apagado al dibujar)
p = dst / 'snes' / 'ppu.c'
s = p.read_text()
old = '      layerActive = ppu->layer[curLayer].mainScreenEnabled && ('
assert old in s, 'no encuentro la prueba de capa activa de la PPU'
s = s.replace(old, '      layerActive = !(g_hl_nom7 && ppu->mode == 7 && curLayer == 0) && ppu->layer[curLayer].mainScreenEnabled && (')
old = '    PpuDrawBackground_mode7(ppu, y, sub, 0x5000);'
assert old in s, 'no encuentro el dibujo del modo 7 (camino rápido)'
s = s.replace(old, '    if (!g_hl_nom7) PpuDrawBackground_mode7(ppu, y, sub, 0x5000);')
p.write_text('int g_hl_nom7;\n' + s)
