# Pruebas por sala (docs/41, 44)

Graban en el SNES (`hlref` con `HL_WARP`: salta a cada sala con la transición del ascensor) un lote de salas con un
patrón de disparos y lo comparan con el motor en el PC (`native/host/check` con las mismas variables).

- `crall.sh`: las 34 salas de Crateria con enemigos (lotes `rooms_cNN.txt`, en paralelo de a 8) → una línea por sala:
  `cuadros 2-30000: N con diferencias` y lo que se llamó sin portar.
- `crbatch.sh NN` / `brbatch.sh SALA`: un lote de Crateria / una sala de Brinstar (`rooms_brSALA.txt`).
- `gdbatch.sh NN [c|br]`: lo mismo con Samus invencible (`HL_GOD=1`) y 1500 cuadros por sala.
- `mkwarp_e.py`: arma la entrada (la del bot hasta Zebes, `in_z2.txt`, más el patrón por sala) y la lista `HL_WARP`.

Pendiente: los scripts tienen las rutas de la máquina donde se escribieron (`/tmp/claude-0/warp` como carpeta de
trabajo, `/tmp/claude-0/hlb/hlref_e`, `/home/claude/mm`). Para usarlos: cambiar esas rutas por `work/rooms`, el
`hlref` de `tools/inputs/make_inputs.sh` (`work/inputs/hlref`) y la raíz del proyecto.
