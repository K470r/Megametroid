# Medición de velocidad en la Genesis (docs/39)

Guiones de trabajo (rutas del entorno de desarrollo: ajustar `retrorun`, el núcleo de Genesis Plus GX con perfilador
`gpgx_prof` y las grabaciones). Todos compilan la ROM de prueba (`TEST=1`, entrada de `res/gen/md/test_input.s`).

- `bnew.sh BASE EMU [etiqueta]`: líneas medias y máximas por cuadro del motor desde BASE (`MD_BUSYLOG`). Con `OPT=...`
  prueba otras opciones del compilador.
- `pnew.sh A B [función]`: perfil exacto de los cuadros A..B del motor (dos ROM con `STOP_AT`), por función; con una
  función, su código con los ciclos de cada instrucción. `annot.py función` muestra los bloques más pesados.
- `busy.sh BASE EMU`: lo mismo que `bnew.sh`, comparando con una versión anterior del motor.
- `prog.sh E...`: cuadro del motor al que se llegó en cada cuadro del emulador (retraso acumulado).
- `lag.sh`, `perframe.sh`: retrasos por cuadro (`LAGLOG`) y perfil cuadro a cuadro del emulador.
- `lagrun.sh BASE EMU`: cuadros que se vieron más de un vblank (`LAGLOG`, con los cuadros en cadena: lo mide la
  interrupción vertical al tomar cada cuadro).
- `progcmp.sh E...`, `stopcmp.sh N...`: avance del motor y capturas congeladas (`STOP_AT`) de una versión anterior
  (copia en otro directorio) contra la actual.
- `ssum.sh BASE REF`: suma del estado de la Genesis contra el motor del PC (`host/simsum`), 256 cuadros.
- `sprab.sh BASE REF "A" "B"`: lista de sprites de dos variantes de compilación (`MD_SPRSUM`).
