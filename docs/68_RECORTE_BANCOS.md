# 68 — Las copias de los bancos del SNES: qué se lee y qué se recorta

El motor lee tablas, listas de instrucciones y datos de los bancos del SNES desde copias en la ROM de la Genesis
(`kBankXXB`, `tools/conv/mkdata.py`): 18 bancos de 32 KB en la parte común (la que se ve siempre, los primeros 3 MB) y
otros en páginas del SSF2 (la fría, 7; la de los jefes). La parte común estaba casi llena.

## Qué se lee (medido)

`host/bankcov.c`: enlazado con un programa del PC, protege la memoria de las copias y anota cada byte que se lee (la
instrucción corre sola, paso a paso, y la página se vuelve a cerrar). Con `MM_BANKCOV=archivo` suma lo leído:

    G=res/gen/engine; CF="-O1 -g -w -Ihost -Iinc/core -I$G"
    gcc $CF -o host/check_cov host/check.c host/pc_stubs.c host/bankcov.c src/core/*.c $G/vars.c $G/vars_map.c $G/tables.c
    gcc $CF -o host/zcover_cov host/zcover.c host/pc_stubs.c host/bankcov.c src/core/*.c $G/vars.c $G/vars_map.c $G/tables.c
    MM_BANKCOV=work/bankcov.bin host/check_cov work/inputs/ref_full.bin sm/sm.smc 97 99999 0      # Ceres -> Zebes
    MM_BANKCOV=work/bankcov.bin host/check_cov work/inputs/ref_intro.bin sm/sm.smc 0 99999 0      # la intro
    MM_BANKCOV=work/bankcov.bin host/check_cov work/inputs/ref_tu.bin sm/sm.smc 97 99999 0        # tiempo agotado
    python3 tools/rooms/salas.py sm/sm.smc > work/salas.txt                                       # las 194 salas
    MM_BANKCOV=work/bankcov.bin host/zcover_cov work/inputs/ref_full.bin sm/sm.smc 10740 \
        "$(awk '{print $2}' work/salas.txt | paste -sd,)" 1500 1                                  # 1500 cuadros en cada una
    python3 tools/conv/bankcov_check.py sm/sm.smc work/bankcov.bin

Resultado (2026-11-19): de cada banco se leen pocos KB (de 0 a 12 KB; el $8F, las salas, el que más). Casi todo lo
demás es el código del 65816 del SNES, que el port reescribió en C y nunca lee.

## Hecho: el relleno del final

Al final de cada banco de la ROM del SNES hay bytes 0xFF que no usa nadie. `mkdata.py` (`bank_data`) corta cada copia
donde empieza ese relleno (más dos bytes, por las lecturas de palabra del último). Nada cambia para el código: sigue
siendo "base + dirección - 0x8000". `bankcov_check.py` comprueba que nada de lo medido cae en lo cortado.

- 71 KB fuera; **58 KB en la parte común** (`__mm_common_end`: 0x2FF040 -> 0x2F0E40) y el resto en las páginas.
- `host/check` (intro, Ceres -> Zebes, tiempo agotado): las mismas 3 diferencias de siempre. Regresión de imagen:
  0 distintos en las tres escenas.

## Lo que se podría hacer después

- **Sacar el código del SNES del medio de los bancos**: `tools/conv/bankmap.py` sigue el flujo del 65816 desde cada
  función del decomp (con el ancho de A y X de REP/SEP) y encuentra ~139 KB de código seguro en los 18 bancos comunes.
  Para sacarlo, la copia se guarda en trozos (de 64 bytes: ~84 KB netos, con una tabla de 16 bits por trozo) y cada
  lectura pasa por la tabla: unos 30 ciclos más por lectura, en el código de los enemigos, que es lo más pesado. Solo
  si hace falta más lugar.
- No conviene cortar por lo medido solamente (lo que no se leyó en las pruebas): un camino no probado leería basura.
