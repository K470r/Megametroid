# MegaMetroid

Metroid 3 (Super Metroid) para Sega Genesis/Mega Drive. El código del juego está reescrito a mano para el 68000 a partir del
decomp de snesrev (`sm/`), no incluye código fuente propietario ni código fuente sujeto a derechos de autor; todo lo que 
viene de la ROM se convierte en el PC desde **un dump propio del cartucho**: este repositorio no contiene nada de la first-party 
(ni la ROM, ni datos extraídos, ni ROM compiladas).

Estado (2026-11-19): el juego entero está portado, de la intro al final: Ceres, todo Zebes (salas, enemigos, efectos y
sonido), todos los jefes, la huida, el final con los créditos, la muerte de Samus y las partidas guardadas. Falta
jugarlo de principio a fin para encontrar lo que se escapó, algunos detalles de imagen y la traducción. Lo que anda, lo
que falta y el plan, más abajo ([Estado del port](#estado-del-port)). Detalle técnico: [docs/46](docs/46_ESTADO_Y_CONTINUACION.md). Índice de la
documentación: [docs/README.md](docs/README.md).

## Requisitos

- Linux (Debian/Ubuntu probado) o Windows con WSL (Ubuntu). En una sola línea:

      sudo apt update && sudo apt install git build-essential gcc-m68k-linux-gnu python3 python3-numpy python3-pil util-linux default-jre-headless

  (Java lo usan rescomp, lz4w y xgm2tool de SGDK.) Si `apt` da "404 Not Found", falta el `sudo apt update`.
- Un volcado de tu propio cartucho de **Super Metroid (Japón / EE. UU.)**, 3 MB, sin cabecera de copiadora (la quita `check_rom.py`):
  SHA-1 `da957f0d63d14cb441d215462904c4fa8519c613`, MD5 `21f3e98df4780ee1c667b84e57d88675`.

## Compilar la ROM de la Genesis

En Linux, o en Windows con WSL (Ubuntu). La primera vez tarda un rato (las herramientas, las grabaciones del SNES y
`make full`, este solo de 12 a 15 minutos) y usa ~5 GB de disco; todo queda dentro de la carpeta del proyecto.

1. Instalar lo necesario (una vez): el comando de "Qué hace falta", arriba.
2. Bajar el proyecto y entrar en la carpeta:

       git clone https://github.com/K470r/Megametroid
       cd Megametroid

3. Bajar y compilar las herramientas (decomp, SGDK, emuladores):

       sh tools/setup/setup_tools.sh

4. Poner tu volcado de Super Metroid (lo comprueba y lo copia a `sm/sm.smc`):

       python3 tools/check_rom.py /ruta/a/tu_volcado.smc

5. Extraer y grabar desde tu ROM lo que se necesita (unos minutos, ~4 GB en `work/`):

       sh tools/inputs/make_inputs.sh

   Deja en `work/inputs/` las grabaciones del SNES (`ref_full.bin`, `ref_intro0.bin`...), el SNES de referencia
   (`hlref`) y `env.sh`; en `res/snes_ref/` las capturas (salas, cinemáticas, título, menú y el final). No hay que
   borrar nada de eso: `make full` lo usa.

6. Compilar:

       . work/inputs/env.sh && make full

La ROM queda en **`out/rom.bin`** (15,25 MB de los 16 que permite el mapeador SSF2): se carga en MiSTer, en la Mega EverDrive o en un emulador
con SSF2 (Genesis Plus GX, BlastEm). Sale igual byte a byte para cualquiera que use la misma ROM (ver
[docs/45](docs/45_REPRODUCIBLE.md)): con `md5sum out/rom.bin` se puede comparar con la de otro.

### Actualizar a lo último

    cd Megametroid
    git checkout main
    git pull
    . work/inputs/env.sh && make full

`make full` (12-15 min) cuando cambian los conversores (`tools/`); si solo cambió el código, basta `make`.

Si cambió `tools/setup/`, `tools/emu_harness/` o `tools/inputs/` (o `tools/snes_ref/`: el SNES de referencia y las
capturas), repetir antes los pasos 3 y 5. `make_inputs.sh` compila `hlref` solo si no existe: si cambió
`tools/snes_ref/ceres_bot/hlref.c`, borrarlo antes para que se rehaga:

    sh tools/setup/setup_tools.sh        # herramientas (también retrorun)
    rm -f work/inputs/hlref              # solo si cambió hlref.c
    sh tools/inputs/make_inputs.sh       # grabaciones y capturas
    . work/inputs/env.sh && make full

### Comandos

| Comando | Qué hace |
|---|---|
| `make full` | todo: los datos desde tu ROM (`make data`) y la ROM |
| `make` | solo recompila el código (los datos de `res/gen/` ya hechos) |
| `make data` | solo los datos desde tu ROM (`tools/conv/make_data.sh`) |
| `make snd` | rehace la música y los efectos (`tools/snd/mksnd.py`, con `tools/snd/instrumentos.txt`) y la ROM |
| `make fur` | las canciones del juego como proyectos de Furnace en `work/furnace/` (para elegir instrumentos a oído) |
| `make host` | el motor en el PC (`host/check`: comparación cuadro a cuadro con el SNES) |
| `make TEST=1` | la ROM de prueba con la entrada de mando grabada (la usa la regresión) |
| `make clean` | borra `out/` |

Detalle de la estructura y del `Makefile`: [docs/47](docs/47_ESTRUCTURA_SGDK.md).

### Instrumentos de la música (Furnace)

La música sale de las secuencias del SNES de tu ROM, con un instrumento FM por muestra (`tools/snd/instrumentos.txt`,
en git: solo números). Para ajustarlos a oído ([docs/55](docs/55_SONIDO_XGM2.md)):

    make fur                                                       # proyectos en work/furnace/ (+ muestras/ y la lista)
    # en Furnace: abrir el .fur, cambiar los instrumentos FM (nombres CONJ_MUESTRA, p. ej. 03_25), guardar
    python3 tools/snd/fur2tabla.py work/furnace/03_5_titulo.fur --envolvente-snes --escribir
    make snd                                                       # la ROM con los instrumentos nuevos

## Probar

    sh tests/regress/regress.sh          # la imagen de la intro, Ceres y Zebes contra la aprobada (0 distintos)
    sh tools/rooms/beams.sh              # rayos contra el SNES (también sba.sh, powerbomb.sh, chargebeam.sh...)
    make host                            # el motor en el PC (host/check: comparación con el SNES)

La regresión usa la ROM de prueba (`TEST=1`), que empieza sin el título: después hay que probar también el arranque
normal (`out/rom.bin`) en un emulador.

## Estado del port

**Anda:**
- Encendido (logo), título con su música y los chillidos del bebé metroide.
- Menú de partidas con las tres ranuras (energía, tanques y tiempo), copiar y borrar partidas, opciones, y el idioma
  (inglés o español) guardado en la SRAM ([docs/65](docs/65_PARTIDAS.md)).
- Partidas guardadas: la SRAM es la del SNES byte por byte (se guarda en las estaciones, la nave, la huida de Ceres y
  la partida nueva); al cargar, el mapa de la partida (el planeta y el mapa del área, [docs/67](docs/67_MAPA_PARTIDA.md)).
- Intro completa con voces, música y efectos; Ceres entero (Ridley, la huida, la explosión) y el viaje a Zebes.
- Zebes: las 6 áreas con todas sus salas y sus enemigos (las 194 salas con enemigos probadas contra el SNES), pausa y
  mapa ([docs/53](docs/53_PAUSA_Y_MAPA.md)), bloques con el ícono del arma que los rompe.
- Efectos de las salas: lava (también en las salas calientes de Norfair), ácido, agua, lluvia, niebla y neblina. En la
  Genesis van como un tinte de los colores (no hay una tercera capa que se sume como en el SNES).
- Sonido de todo el juego: la música y los efectos de cada área ([docs/55](docs/55_SONIDO_XGM2.md)).
- Samus completa: movimientos, trajes, gancho, salto espacial, shinespark ([docs/56](docs/56_SAMUS_COMPLETA.md));
  rayos y sus combinaciones, rayo cargado, ataques especiales ([docs/49](docs/49_RAYO_CARGADO.md),
  [57](docs/57_CLAVES_DE_RAYOS.md)); misiles, bombas, bomba de poder con su luz, enemigos congelados
  ([docs/58](docs/58_HIELO_BOMBA_ESTELAS.md)); rayos X ([docs/61](docs/61_RAYOS_X.md)); la muerte de Samus y el
  fin de juego ([docs/64](docs/64_MUERTE.md)).
- Jefes ([docs/59](docs/59_JEFES.md)): Spore Spawn, Bomb Torizo, Golden Torizo, Kraid, Crocomire, Phantoon, Botwoon,
  Draygon, Ridley y Mother Brain (con el rayo arco iris y la bebé). Ojos de las paredes y su haz de luz
  ([docs/52](docs/52_OJO_HAZ.md)). La sala de las estatuas de Tourian.
- La huida de Zebes con su cuenta regresiva y el despegue ([docs/62](docs/62_HUIDA_ZEBES.md)); el final con los tres
  finales según el tiempo y los créditos ([docs/63](docs/63_FINAL.md)).
- Tamaño de la ROM ([docs/48](docs/48_TAMANO_ROM.md)): los tiles de las salas van comprimidos y se descomprimen en
  la transición de puerta.

**Falta.** Para poder terminarlo (bloqueos):
1. (Hecho) El visor de pruebas ya no está en la ROM normal (solo en las de prueba y en el PC); con él Samus no perdía
   energía. Y al cargar una partida Samus ya recibe el control después de la fanfarria (`Samus_Func16`).
2. Jugar el juego entero de principio a fin, con las salidas secretas y los atajos: hasta ahora se probó por partes
   (sala por sala contra el SNES, cada jefe, la huida con saltos de sala).
3. (Hecho) El destello de cristal (crystal flash, [docs/69](docs/69_DESTELLO_CRISTAL.md)): igual al SNES en la lógica;
   Samus dentro de la burbuja sale con otros colores (el SNES la mezcla).

Funcionan, pero no se ven igual que en el SNES:
4. Mother Brain: el tanque de vidrio no desaparece al romperse; la cabeza algo más oscura después de la bebé; el
   efecto de color del hiperrayo (`ResetProjectileData`).
5. Los cadáveres que se pudren (el Torizo y otros jefes: `CorpseRotting`).
6. La huida: el cielo del aterrizaje de un solo color (el SNES: nubes moradas); los colores de la alarma de Tourian.
7. El mapa de la partida: adentro del cuadrado queda negro mientras se abre o se cierra; la cuadrícula algo más oscura.
8. Kraid: una uña se dispara un cuadro antes que en el SNES.
9. Rendimiento: algunas salas pesadas pierden cuadros (la peor, 92B3: [docs/44](docs/44_OPTIMIZACION.md)).
10. La niebla sin la forma de las nubes y la neblina con un color medio en vez del degradado.

Traducción al español ([docs/66](docs/66_TRADUCCION.md); el menú, las opciones y copiar/borrar partidas ya están):
11. La intro (`tools/lang/intro_es.txt` está vacío y la intro todavía no lo usa).
12. Las cajas de mensaje (28: ítems, "¿guardar?", recargas...).
13. La pausa (MAP / SAMUS / EQUIPMENT, los nombres de los ítems).
14. El fin de juego (GAME OVER, FIND THE METROID LARVA, TRY AGAIN, YES / NO).
15. El final (SEE YOU NEXT MISSION, CLEAR TIME, GENESIS DOES...).
16. El mapa de la partida ("PLANET ZEBES", "MAP SCROLL / START / CANCEL").

Extras ya acordados:
17. La música en CD (Mega CD, modo 1): si hay Mega CD y el disco de MegaMetroid, la música sale del CD; si no, la del
    cartucho ([docs/48](docs/48_TAMANO_ROM.md)).
18. Los instrumentos ajustados a oído (el eco del SNES no existe en la Genesis).
19. Los botones dibujados como los de la Genesis: en la pausa "L" / "R" (en el mando de 6 botones son C, al mapa, y
    Z, a SAMUS; con 3 botones, A y B) y los botones que muestran las cajas de mensaje al agarrar un ítem nuevo
    (SELECT, X, Y, L, R del SNES).
20. Espacio en la ROM: recortar las copias de bancos del SNES (782 KB); la parte común (los primeros 3 MB) está casi
    llena y lo nuevo va en las páginas del SSF2.

## Estructura

Organizada como un proyecto de SGDK (`src/`, `inc/`, `res/`, `out/`):

| Ruta | Qué es |
|---|---|
| `Makefile` | compila la ROM (ver "Comandos") |
| `src/core/`, `inc/core/` | el motor (C sin SGDK, igual en el PC y en la Genesis); `port_gen.c/.h` los genera `tools/port` del decomp |
| `src/platform/`, `inc/platform/` | la capa de la Genesis: VDP, dibujo, mando, sonido |
| `src/front/`, `inc/front/` | título y menú (con SGDK) |
| `src/boot/` | cabecera del cartucho (`rom_header.c`) y enlace SSF2 (`md_ssf2.ld`) |
| `res/` | recursos: `resources.res` (rescomp, arte propio), `snes_ref/` (claves de dibujo), `gen/` (lo que sale de tu ROM, fuera de git) |
| `out/` | lo compilado: `rom.bin`, `rom.out`, `rom.map`, `symbol.txt` |
| `host/` | el motor en el PC: comparación con el SNES (`check`), estado inicial (`mkinit`) |
| `tools/conv/` | conversores de la ROM a datos de la Genesis (`mkdata`, `mkmd`, `mkzebes`, `mkzobj`, `mkintro`...) |
| `tools/snd/` | el sonido: el N-SPC del decomp en el PC (`spcrun.c`), `mksnd.py` (XGM2), `mkfur.py` y `fur2tabla.py` (Furnace) |
| `tools/` | traductor del decomp (`port/`), extractor, grabador del SNES (`snes_ref/`), arnés de emulador, medición |
| `tests/regress/` | regresión de imagen: entradas de mando grabadas y hashes por línea de la imagen aprobada |
| `docs/` | documentación (en español) |

## Licencia

MIT ([LICENSE](LICENSE)): se puede usar, copiar, modificar y distribuir sin restricción, siempre que se conserve el
aviso con los autores: **S í n d r o m e y Claude (Anthropic)**. Lo derivado del decomp (snesrev/sm) conserva también su
aviso MIT. SGDK, Genesis Plus GX y snes9x tienen sus propias licencias y no se incluyen (los baja `setup_tools.sh`).
