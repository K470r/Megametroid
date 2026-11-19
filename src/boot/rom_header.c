#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header = {
    "SEGA MEGA DRIVE ",
    "(C)MEGAMETROID  ",
    "MEGAMETROID - CERES (MOTOR NATIVO)              ",
    "MEGAMETROID - CERES (NATIVE ENGINE)             ",
    "GM 00000000-00",
    0x000,
    "J6              ",
    0x00000000,
    0x003FFFFF,
    0xE0FF0000,
    0xE0FFFFFF,
    "RA",                                         /* SRAM: partidas guardadas (md_sram.c) */
    0xF820,
    0x00200001,
    0x00203FFF,
    "            ",
    "MEGAMETROID: MOTOR NATIVO PARA 68000    ",
    "JUE             "
};
