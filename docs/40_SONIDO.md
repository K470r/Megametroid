# 40 — Sonido en la Genesis

Estado: solo las voces de la intro. La música y los efectos vendrán con un controlador propio para el Z80.

## Cómo pide el motor la música

Igual que el SNES (`HandleMusicQueue`, `native/src/door.c`): `music_upload(conjunto)` al cargar un conjunto de música
y `music_play(pista)` al tocar una pista. La capa de la Genesis es `native/md/md_snd.c`.

## Voces de la intro

En el SNES son la pista 5 del conjunto $3F ("The last Metroid is in captivity", 60 cuadros después de que aparece el
texto) y la pista 5 del conjunto $42 ("The galaxy is at peace", 200 cuadros después). La Genesis las toca con el
controlador PCM de SGDK (un canal, 16 kHz, 8 bits).

Los datos salen de tu ROM: son muestras del SPC que `tools/sm_extract/extract_all.py` deja en
`data/extracted_full/audio` ("The last Metroid" y "is in captivity" en el songset 3F, "The galaxy" e "is at peace" en el
42: `sample_18_B516` y `sample_19_*`). `native/tools/mkvoice.py` (lo llama `native/md/build.sh`) las toma como
16 kHz (el extractor las rotula a 32 kHz; la intro las toca a la mitad), las pasa a 8 bits con signo y genera
`native/md/gen/md_voice.s`. Antes se usaban WAV en `res/` con las mismas muestras; ya no hacen falta.

## El bus del Z80

La muestra está en la ROM y el Z80 la lee por el bus del 68000. Mientras suena, el vblank (que hace las DMA a la
VRAM) le pide el bus al Z80 y se lo devuelve al terminar (`md_vblank_run`), como hace SGDK durante sus DMA: así la
DMA no se cruza con una lectura del Z80.

Las muestras están en la parte fija de la ROM (debajo de 0x380000), no en una página del SSF2: el Z80 no sabe qué
página está puesta.
