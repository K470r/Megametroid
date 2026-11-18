# 61 — Los rayos X

Código: `src/core/hdmaobj.c` (el final: la lista de bloques revelados, el objeto de HDMA, el cono),
`src/core/samus_pose.c` (`Xray_Initialize`, `Samus_Func20`, `ResponsibleForXrayStandupGlitch`,
`Samus_HandleTransitionsB` 5), `src/core/samus_move.c` (`SamusMovementType_Xray`), `src/core/proj.c`
(`HudSelectionHandler_Xray`), `src/platform/md_cone.c` (el cono y el modo sombra/luz), `src/platform/md_zebes.c`
(los bloques revelados en el plano A, la prioridad de los planos), `src/platform/md_engine.c` (el tinte),
`tools/conv/mkzebes.py` (`xray_blocks`), `tools/conv/mkdata.py` (las tablas). Prueba: `tools/compare/xray.sh`.

## Lo que hace el SNES (decomp)

- Con los rayos X elegidos y el botón de correr: `XrayRunHandler` → `Xray_Initialize` ($91:E16D) detiene el tiempo
  (`time_is_frozen_flag`), apaga proyectiles enemigos, PLMs, tiles animados y efectos de paleta, pone la pose de
  rayos X y crea el objeto de HDMA de $91:D223.
- Ese objeto, en ocho cuadros (`Xray_SetupStage1-8`): lee el mapa del BG1 de la VRAM, arma el "mapa de los rayos X"
  (el del BG1 con los bloques revelados), lo sube al BG2 y pone el fondo gris (`palette_buffer[0] = 0x0C63`).
- Bloques revelados (`Xray_GetXrayedBlock`, tabla $91:D2D6 por tipo de bloque y BTS): bloques de disparo, de bomba,
  de supermisil, de bomba de poder, que se desmoronan, de speed booster (cada uno con su ícono), las extensiones
  (`Xray_CombinedMove`) y el aire: un bloque de aire con dibujo (paredes falsas, decorado) sale vacío. Más
  `LoadXrayBlocks` ($84:831A): los objetos escondidos (PLM de objeto no tomado, `kXrayBlockDrawingInstrs`) y los
  casos especiales del estado de sala (`xray_special_casing_ptr`).
- Después `HdmaobjPreInstr_Xray` ($88:86EF): el cono crece (`XrayFunc1`), se mueve con arriba/abajo
  (`MoveXrayUp/Down`), y al soltar el botón se apaga y todo vuelve (`XrayFunc3-5`). Dentro de la ventana se ve el
  BG2 (el mapa revelado, sin el fondo, gris); fuera, todo más oscuro.

## En la Genesis

- La lógica es la del SNES (las mismas funciones), salvo lo de la VRAM del SNES: en vez de armar un mapa,
  `xray_reveal_build` hace la lista de bloques de la pantalla que se ven distintos (índice del bloque y palabra a
  mostrar; en `hdma_table_1`, 7E:9800, que la Genesis no usa en los rayos X: la RAM de la partida no tenía lugar).
  Los de aire que ya se ven vacíos no entran.
- El cono: la misma forma del SNES (`CalculateXrayHdmaTableInner` con los valores de los rayos X en `beam_memo`) y los
  mismos sprites "aclarar" del modo sombra/luz que el haz del ojo (docs/52).
- Lo de afuera, más oscuro: el modo sombra/luz. Para que el VDP oscurezca, los dos planos van con prioridad baja
  mientras duran los rayos X (`zebes_force_prio(2)`, `zebes_plane_b_low`; se reescriben de a 8 columnas por cuadro:
  el tiempo está detenido). Un tinte de +3 (`XRAY_TINT`, md_engine.c) aclara todo un poco para que el cono se
  distinga (con +6 lo de afuera quedaba como sin rayos X).
- Los bloques revelados: `xray_take` (md_zebes.c) dibuja en el plano A, cada cuadro, los de la lista cuyo centro cae
  dentro del cono (la tabla de 7E:9100) y vuelve a dibujar el del nivel cuando salen o se apagan los rayos X.
- Cada sala lleva los gráficos de los bloques que puede revelar (`mkzebes.xray_blocks`: la misma tabla).

Diferencias que quedan con el SNES: dentro del cono se sigue viendo el fondo (plano B; el SNES lo cambia por gris),
los bloques se revelan enteros (el SNES, por píxel en el borde del cono), y lo de afuera queda a la mitad (el SNES,
a un tercio). Probado en Parlor (92FD): los bloques de bomba ocultos (61-66, 11) aparecen dentro del cono.
