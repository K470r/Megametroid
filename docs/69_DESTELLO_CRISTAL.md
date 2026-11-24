# 69 — El destello de cristal

Con poca energía (menos de 50), sin reserva y al menos 10 misiles, 10 superbombas y 10 bombas de poder, si Samus en
bola pone una bomba de poder, se queda quieta donde la puso y mantiene L + R + abajo + disparo, al terminar la
explosión se cambian las municiones por energía.

## Cómo se hace en la Genesis

1. Tener menos de 50 de energía, los tanques de reserva vacíos y al menos 10 misiles, 10 superbombas y 10 bombas de
   poder.
2. En bola, elegir la bomba de poder y ponerla. No moverse: Samus tiene que quedar donde la puso.
3. Antes de que termine la explosión, mantener apretado:
   - **mando de 6 botones**: abajo + C + Z + Y (C = L, Z = R, Y = disparo);
   - **mando de 3 botones**: abajo + A + B + C.
4. Mantenerlo hasta que la explosión termine: Samus sube, se rodea de la burbuja y cada 8 cuadros cambia una munición
   por energía (primero misiles, después superbombas y bombas de poder).

Como en el SNES, tiene que ser exactamente esa combinación (con izquierda, derecha o arriba no sale).

## El mando de 3 botones

No tiene L ni R en el juego. `md_main.c` (lectura del mando): **solo mientras hay una bomba de poder puesta**
(`power_bomb_flag`, desde que se pone hasta que termina la explosión o el destello) y sin START, abajo + A + B + C se
convierte en L + R + abajo + disparo (`button_config_shoot_x`). El juego recibe lo mismo que en el SNES; la lógica no
cambia.

Fuera de eso los botones hacen lo de siempre. Con la bomba puesta, mantener los tres a la vez con abajo:
- si se dan las condiciones del destello, el destello;
- si no (energía de sobra, munición que no alcanza, Samus se movió): lo mismo que L + R + abajo + disparo en el SNES.
  En bola eso es disparar (poner bombas normales; otra bomba de poder no, hasta que termine la que está) sin saltar ni
  correr mientras se mantienen. Al soltar cualquiera de los cuatro, todo vuelve a lo normal.


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

`tools/compare/cristal.sh`: el Parlor, lado a lado con el SNES; y con el mando de 3 botones (la grabación termina al
poner la bomba y sigue el mando de retrorun, `RR_PAD=3`):

| botones mantenidos | energía | misiles / superbombas / bombas de poder |
|---|---|---|
| abajo + A + B + C | 99 | 0 / 0 / 0 (destello) |
| solo abajo | 40 | 10 / 10 / 10 |
| A + B + C sin abajo | 40 | 10 / 10 / 10 |
