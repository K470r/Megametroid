# 69 — El destello de cristal

Con poca energía (menos de 50), sin reserva y al menos 10 misiles, 10 superbombas y 10 bombas de poder, si Samus en
bola pone una bomba de poder, se queda quieta donde la puso y mantiene L + R + abajo + disparo (en la Genesis de 6
botones: C + Z + abajo + Y), al terminar la explosión se cambian las municiones por energía. Con el mando de 3 botones
no se puede: en el juego no tiene L ni R (falta decidir una combinación).

## En el motor (a mano desde el decomp)

- `Hdmaobj_CrystalFlash` ($90:D5A2, samus_move.c): las condiciones; pone la pose D3/D4 y el manejador
  `SamusMoveHandler_CrystalFlashStart` ($90:D678): sube 2 píxeles por cuadro, 10 cuadros, y crea los objetos de HDMA
  (`SpawnCrystalFlashHdmaObjs` $88:A2A6, hdmaobj.c); `…Main` ($90:D6CE): cada 8 cuadros una munición por 50 de
  energía (misiles, superbombas, bombas de poder); `…Finish` ($90:D75B): vuelve a la bola en la y del comienzo.
- Los objetos de HDMA: `CrystalFlash_CustomLayerBlend`, `Stage1_Explosion` (el círculo, las mismas tablas que la
  bomba de poder) y `Stage2_AfterGlow`.
- `host/check` con la grabación del destello (salto al aterrizaje 89B2): 0 diferencias.

## En la Genesis

- La luz del círculo: la misma de la bomba de poder (md_cone.c, docs/58: sprites de sombra/luz), con la forma
  `0x3000 | radio`. Mientras la luz no puede ir (las celdas todavía cambiando de prioridad), el SNES solo mezcla
  dentro del círculo (la ventana 2, del tamaño de la burbuja): la Genesis no tiñe la pantalla entera (sí con la bomba
  de poder).
- Samus y la burbuja: las piezas de las poses D3/D4 usan la paleta 6 del SNES (palette_buffer 224-239, que cambia
  `Samus_HandleCrystalFlashPals`), algunas del cuerpo la 4. La Genesis dibuja a Samus en una sola línea: mientras dura
  la pose, esa línea sale de 224-239 (`samus_line_src`, md_engine.c). La burbuja queda blanca como en el SNES.

## Diferencias que quedan

- En el SNES los sprites de las paletas 4-7 entran en la mezcla de color: Samus dentro de la burbuja se ve oscura
  (azul); en la Genesis sale con los colores de la paleta 6 sin mezclar (negro, rojo y naranja).
- Las de la bomba de poder (docs/58): el resplandor que queda después es gris y no se va apagando; en las salas de
  jefe la luz no va (comparte 0xF000 con los tiles del jefe).

## Prueba

`tools/compare/cristal.sh`: el Parlor, lado a lado con el SNES.
