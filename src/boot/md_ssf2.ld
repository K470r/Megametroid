/* MegaMetroid: md.ld de SGDK (2.12) con las páginas del mapeador SSF2 (docs/29, docs/36).
   Las variables del juego (todo lo que no es de SGDK ni de las pantallas de antes del juego) van juntas en la RAM,
   entre _game_data_start/_end y _game_bss_start/_end: para volver a empezar una partida se ponen como al
   encender (md_main.c) sin reiniciar la consola. */
OUTPUT_ARCH(m68k)
SEARCH_DIR(.)
/*GROUP(-lbcc -lc -lgcc)*/
__DYNAMIC  =  0;

/*
 * Setup the memory map of the SEGA Genesis.
 * stack grows down from high memory.
 *
 * The memory map look like this:
 * +--------------------+ <- low memory
 * | .text              |
 * |        _etext      |
 * |        ctor list   | the ctor and dtor lists are for
 * |        dtor list   | C++ support
 * +--------------------+
 * .                    .
 * .                    .
 * .                    .
 * +--------------------+ <- 0xE0FF0000
 * | .data              | initialized data goes here
 * |        _data       |
 * |        _edata      |
 * +--------------------+
 * | .bss               |
 * |        _bstart     | start of bss, cleared by crt0
 * |        _bend       | start of heap, used by sbrk()
 * +--------------------+
 * .                    .
 * .                    .
 * .                    .
 * |        __stack     | top of stack
 * +--------------------+ <- 0xE1000000
 */
MEMORY
{
	rom (rx) : ORIGIN = 0x00000000, LENGTH = 0xE0000000
	ram : ORIGIN = 0xE0FF0000, LENGTH = 0x00010000
}

/*
 * allocate the stack to be at the top of memory, since the stack
 * grows down
 */

PROVIDE (__stack = 0xE1000000);


SECTIONS
{
  .text 0x00000000:
  {
    KEEP(*(.text.keepboot)) *(EXCLUDE_FILE(*pause.o *md_pause_data.o) .text.* EXCLUDE_FILE(*pause.o *md_pause_data.o) .text)
    . = ALIGN(0x4);
     __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    *(.dtors)
     LONG(0)
    __DTOR_END__ = .;

    *(EXCLUDE_FILE(*pause.o *md_pause_data.o) .rodata EXCLUDE_FILE(*pause.o *md_pause_data.o) .rodata.*)
    *(.gcc_except_table .gcc_except_table.*)

    . = ALIGN(0x4);
    __INIT_SECTION__ = . ;
    *(.init)
    SHORT (0x4E75)	/* rts */

    __FINI_SECTION__ = . ;
    *(.fini)
    SHORT (0x4E75)	/* rts */

    _etext = .;
    *(.lit)

    *(.rodata_bin)
    *(.rodata_binf)
    /* MegaMetroid: datos que solo usa el título, al final. Todo lo de antes queda por debajo de
       0x380000, así la ventana 7 del mapeador SSF2 se puede cambiar de página fuera del título. */
    /* MegaMetroid: lo que se agregó después de aprobar la regresión de imagen (docs/41) al final de lo común, para no
       correr las direcciones de lo demás (los tiempos de la intro y de Ceres dependen de dónde caen los datos que se
       copian por DMA): la pausa (docs/49) */
    . = ALIGN(0x4);
    *pause.o(.text .text.* .rodata .rodata.*)
    *md_pause_data.o(.text .text.* .rodata .rodata.*)
    *(.late_text .late_text.*)
    *(.late_rodata .late_rodata.*)
    . = ALIGN(0x4);
    __mm_title_start = .;
    KEEP(*(.rodata_title))
    __mm_title_end = .;
  } > rom
  ASSERT(__mm_title_start <= 0x380000, "MegaMetroid: codigo o datos comunes por encima de 0x380000 (ventana 7 del SSF2)")
  _stext = SIZEOF (.text);

  .data 0xE0FF0000 :
  AT ( ADDR (.text) + SIZEOF (.text) )
  {
    *(.got.plt) *(.got)
    *(.shdata)
    _game_data_start = .;
    *(EXCLUDE_FILE (*libmd.a:* *libgcc.a:* *front_*.o) .data EXCLUDE_FILE (*libmd.a:* *libgcc.a:* *front_*.o) .data.*)
    . = ALIGN(2);
    _game_data_end = .;
    *(.data .data.*)
    *(.ramprog .ramprog.*)
    _edata = .;
  } > ram
  _sdata = SIZEOF (.data);
  _game_data_rom = LOADADDR (.data) + (_game_data_start - ADDR (.data));   /* copia en la ROM (md_main.c) */

  ASSERT(ADDR (.text) + SIZEOF (.text) + SIZEOF (.data) <= 0x400000, "MegaMetroid: la parte fija de la ROM pasa de 4 MB")

  /* Páginas del mapeador SSF2 más allá de los 4 MB: se ven en la ventana 7 (0x380000-0x3FFFFF)
     después de escribir el número de página en $A130FF (mm_ssf2.h). */
  .bank8 0x380000 : AT (0x400000) { KEEP(*(.bank8)) }
  ASSERT(SIZEOF (.bank8) <= 0x80000, "MegaMetroid: la pagina 8 pasa de 512 KB")
  .bank9 0x380000 : AT (0x480000) { KEEP(*(.bank9)) }
  ASSERT(SIZEOF (.bank9) <= 0x80000, "MegaMetroid: la pagina 9 pasa de 512 KB")
  .bank10 0x380000 : AT (0x500000) { KEEP(*(.bank10)) }
  ASSERT(SIZEOF (.bank10) <= 0x80000, "MegaMetroid: la pagina 10 pasa de 512 KB")
  .bank11 0x380000 : AT (0x580000) { KEEP(*(.bank11)) }
  ASSERT(SIZEOF (.bank11) <= 0x80000, "MegaMetroid: la pagina 11 pasa de 512 KB")
  .bank12 0x380000 : AT (0x600000) { KEEP(*(.bank12)) }
  ASSERT(SIZEOF (.bank12) <= 0x80000, "MegaMetroid: la pagina 12 pasa de 512 KB")
  .bank13 0x380000 : AT (0x680000) { KEEP(*(.bank13)) }
  ASSERT(SIZEOF (.bank13) <= 0x80000, "MegaMetroid: la pagina 13 pasa de 512 KB")
  .bank14 0x380000 : AT (0x700000) { KEEP(*(.bank14)) }
  ASSERT(SIZEOF (.bank14) <= 0x80000, "MegaMetroid: la pagina 14 pasa de 512 KB")
  .bank15 0x380000 : AT (0x780000) { KEEP(*(.bank15)) }
  ASSERT(SIZEOF (.bank15) <= 0x80000, "MegaMetroid: la pagina 15 pasa de 512 KB")
  .bank16 0x380000 : AT (0x800000) { KEEP(*(.bank16)) }
  ASSERT(SIZEOF (.bank16) <= 0x80000, "MegaMetroid: la pagina 16 pasa de 512 KB")
  .bank17 0x380000 : AT (0x880000) { KEEP(*(.bank17)) }
  ASSERT(SIZEOF (.bank17) <= 0x80000, "MegaMetroid: la pagina 17 pasa de 512 KB")
  .bank18 0x380000 : AT (0x900000) { KEEP(*(.bank18)) }
  ASSERT(SIZEOF (.bank18) <= 0x80000, "MegaMetroid: la pagina 18 pasa de 512 KB")
  .bank19 0x380000 : AT (0x980000) { KEEP(*(.bank19)) }
  ASSERT(SIZEOF (.bank19) <= 0x80000, "MegaMetroid: la pagina 19 pasa de 512 KB")
  .bank20 0x380000 : AT (0xA00000) { KEEP(*(.bank20)) }
  ASSERT(SIZEOF (.bank20) <= 0x80000, "MegaMetroid: la pagina 20 pasa de 512 KB")
  .bank21 0x380000 : AT (0xA80000) { KEEP(*(.bank21)) }
  ASSERT(SIZEOF (.bank21) <= 0x80000, "MegaMetroid: la pagina 21 pasa de 512 KB")
  .bank22 0x380000 : AT (0xB00000) { KEEP(*(.bank22)) }
  ASSERT(SIZEOF (.bank22) <= 0x80000, "MegaMetroid: la pagina 22 pasa de 512 KB")
  .bank23 0x380000 : AT (0xB80000) { KEEP(*(.bank23)) }
  ASSERT(SIZEOF (.bank23) <= 0x80000, "MegaMetroid: la pagina 23 pasa de 512 KB")

  .bss 0xE0FF0000 + SIZEOF (.data) :
  {
    _start = . ;
    *(.shbss)
    . = ALIGN(2);
    _game_bss_start = .;
    *(EXCLUDE_FILE (*libmd.a:* *libgcc.a:* *front_*.o) .bss EXCLUDE_FILE (*libmd.a:* *libgcc.a:* *front_*.o) .bss.* EXCLUDE_FILE (*libmd.a:* *libgcc.a:* *front_*.o) COMMON)
    . = ALIGN(2);
    _game_bss_end = .;
    *(.bss .bss.*)
    *(COMMON)
    _bend = . ;
  } > ram
  /* la memoria de SGDK (búfer de DMA, etc.) va de _bend a la pila: con menos de ~4.5 KB la consola no arranca
     (2026-10-21: con _bend en 0xE0FFE644 se quedaba en VDP_init; con 0xE0FFE3FC arrancaba) */
  ASSERT(_bend <= 0xE0FFE3F0, "MegaMetroid: falta RAM para SGDK (las variables pasan de 0xE0FFE3F0)")

  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }

  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }

  .eh_frame 0 (NOLOAD) :
  {
    *(.eh_frame)
  }
}
