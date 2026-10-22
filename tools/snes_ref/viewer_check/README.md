# Comprobación del visor de salas contra el SNES

El visor de la Genesis (START + izquierda/derecha) entra a cada sala de Zebes con una transición de ascensor por una
puerta de esa sala, sin enemigos. Para comparar el motor nativo con el SNES en todas las salas:

1. `build_hlref.sh ~/smsrc/src hlref_v`: hlref con el decomp parcheado (sin enemigos con HL_WARP).
2. `mkwarp.py "sala:puerta ..." n0 cada entrada.txt`: arma la entrada (la del aterrizaje `in_z2.txt` + movimientos
   cortos en cada sala) e imprime los saltos `n:puerta` (n = cuadro grabado).
3. `HL_WARP=<saltos> setarch -R hlref_v sm.smc entrada.txt ref.bin 23500` (setarch -R: sin direcciones al azar).
4. `HL_WARP=<saltos> native/host/check ref.bin sm.smc 1 4000 0`.

Las 254 salas del visor, en lotes de 20 (150 cuadros por sala): 0 diferencias.
