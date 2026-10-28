# MegaMetroid — ROM de la Genesis / Mega Drive (SGDK 2.x, mapeador SSF2). Ver docs/47 (estructura del proyecto).
#
#   make            compila la ROM con los datos ya generados (antes: SKIP_DATA=1 sh native/md/build.sh)
#   make data       rehace desde tu ROM todo lo de res/gen/ (necesita REF y REF_INTRO: . work/inputs/env.sh)
#   make full       data + ROM (antes: sh native/md/build.sh)
#   make host       el motor en el PC (host/check, host/mkinit...)
#   make clean      borra out/
#
# Salida: out/rom.bin (y out/rom.out, out/rom.map, out/symbol.txt).
#
# No usa $(GDK)/makefile.gen: este proyecto necesita su propio enlace (src/boot/md_ssf2.ld: páginas SSF2, variables
# del juego juntas para reiniciar la partida), opciones distintas por parte (el motor sin SGDK, title/menu con SGDK,
# intro.c a -O2) y el orden de enlace de siempre, para que la ROM salga igual byte a byte (docs/45). Sí sigue sus
# convenciones: GDK, src/ inc/ res/ out/, rescomp para res/*.res y la libmd.a/libgcc.a de SGDK.
#
# Variables (de la línea de comandos o del entorno, como antes):
#   GDK (SGDK: third_party/SGDK)  CROSS (m68k-linux-gnu-)  OPT (optimización del motor)
#   TEST=1 (entrada grabada)  TEST_INTRO=1  NOHAZE=1  LAGLOG=1 [LAG_BASE=n]  LATE_MAX=n  STOP_AT=n  EXPFL="..."

ROOT    := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
GDK     ?= $(ROOT)/third_party/SGDK
CROSS   ?= m68k-linux-gnu-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
NM      := $(CROSS)nm
JAVA    ?= java

SRC     := $(ROOT)/src
INC     := $(ROOT)/inc
RES     := $(ROOT)/res
GEN_E   := $(RES)/gen/engine
GEN     := $(RES)/gen/md
OUT     := $(ROOT)/out
OBJ     := $(OUT)/obj
HDR     := $(OUT)/hdr

# --- opciones (las mismas de native/md/build.sh) ---------------------------------------------------------------------
OPT_DEFAULT := -O3 -funroll-loops -finline-limit=1500 -fira-loop-pressure
OPT ?= $(OPT_DEFAULT)
FL := -m68000 -fdisable-tree-bswap $(OPT) -fomit-frame-pointer -ffreestanding -fno-builtin -fno-strict-aliasing \
      -fno-store-merging -U_FORTIFY_SOURCE -ffunction-sections -fdata-sections -Wall -Wno-unused-function \
      -Wno-unused-variable -Wno-unused-but-set-variable -Wno-main -DNT_FADE_LIST -DNT_SSF2
ifneq ($(TEST),)
FL += -DMD_TEST_INPUT
endif
ifneq ($(TEST_INTRO),)
FL += -DMD_TEST_INTRO
endif
ifneq ($(NOHAZE),)
FL += -DMD_NO_HAZE
endif
ifneq ($(LAGLOG),)
FL += -DMD_LAGLOG -DLAG_BASE=$(if $(LAG_BASE),$(LAG_BASE),0)
endif
ifneq ($(LATE_MAX),)
FL += -DLATE_MAX=$(LATE_MAX)
endif
ifneq ($(EXPFL),)
FL += $(EXPFL)
endif
ifneq ($(STOP_AT),)
FL += -DMD_STOP_AT=$(STOP_AT)
endif
# intro.c (la intro y sus cinemáticas, una vez por partida): -O2, el desenrollado de -O3 le sumaba 30 KB a la parte
# fija de la ROM (4 MB)
FL_INTRO := $(subst $(OPT_DEFAULT),-O2,$(FL))
SG := -I$(GDK)/inc -I$(GDK)/res -DSGDK_GCC -fms-extensions
# título y menú: SGDK
FF := -m68000 -fdisable-tree-bswap -O2 -fomit-frame-pointer -fno-builtin -ffunction-sections -fdata-sections -Wall \
      -Wno-unused-parameter -I$(INC)/front -I$(GEN)/front $(SG) -I$(OBJ)
AFL := -x assembler-with-cpp -Wa,--register-prefix-optional,--bitwise-or -m68000

# dónde busca cada parte sus .h (el motor no ve nada de SGDK)
I_CORE := -I$(INC)/core -I$(GEN_E)
I_MD   := -I$(INC)/platform -I$(INC)/core -I$(GEN) -I$(GEN_E)

# --- fuentes ---------------------------------------------------------------------------------------------------------
# el motor (src/core) y lo generado que es parte de él; la capa de la Genesis sin SGDK (md_intro.c lo incluye md_engine.c)
CORE_C := $(wildcard $(SRC)/core/*.c) $(GEN_E)/vars.c $(GEN_E)/tables.c $(GEN_E)/zebes.c
MD_C   := $(SRC)/platform/md_engine.c $(SRC)/platform/md_zebes.c $(SRC)/platform/md_cone.c
GEN_C  := $(GEN)/md_data.c $(GEN)/md_getaway.c $(GEN)/md_m7.c $(GEN)/md_intro_data.c $(GEN)/init_state.c \
          $(GEN)/md_pause_data.c
SGDK_C := $(SRC)/platform/md_main.c $(SRC)/platform/md_snd.c
FRONT  := title menu sound front
GEN_S  := md_bins init_blob md_voice

# los nombres de los objetos son los de siempre: el enlace los toma en el orden de `ls` (docs/45)
OBJS := $(patsubst %.c,$(OBJ)/%.o,$(notdir $(CORE_C) $(MD_C) $(GEN_C) $(SGDK_C))) \
        $(patsubst %,$(OBJ)/front_%.o,$(FRONT)) \
        $(OBJ)/front_title_data_c.o $(OBJ)/front_menu_data_c.o $(OBJ)/front_title_data_s.o $(OBJ)/front_menu_data_s.o \
        $(patsubst %,$(OBJ)/%.o,$(GEN_S))

# recursos propios para rescomp (res/*.res): solo los .res que declaran algo (un .res vacío no cambia la ROM)
# y los de res/private/ (redibujos sobre la ROM, fuera de git: docs/47)
RES_FILES := $(shell grep -lE '^[[:space:]]*[A-Z0-9_]+[[:space:]]+[A-Za-z_]' $(RES)/*.res 2>/dev/null)
RES_PRIV  := $(shell grep -lE '^[[:space:]]*[A-Z0-9_]+[[:space:]]+[A-Za-z_]' $(RES)/private/*.res 2>/dev/null)
OBJS += $(patsubst $(RES)/%.res,$(OBJ)/res_%.o,$(RES_FILES)) \
        $(patsubst $(RES)/private/%.res,$(OBJ)/res_private_%.o,$(RES_PRIV))

ifneq ($(TEST),)
EXTRA_OBJ := $(OBJ)/test_input.o
endif

.PHONY: all release rom data full host clean

# sin los datos de tu ROM no hay nada que compilar: primero `make data` (docs/45)
ifeq ($(filter data full host clean,$(MAKECMDGOALS)),)
ifeq ($(wildcard $(GEN_E)/tables.c $(GEN)/md_data.c $(GEN)/md_pause_data.c),)
$(error faltan los datos generados desde tu ROM (res/gen/): . work/inputs/env.sh && make full)
endif
endif
all release rom: $(OUT)/rom.bin

# --- las opciones con que se compiló: si cambian (TEST=1, OPT...), se recompila todo ---------------------------------
FLAGS_STAMP := $(OBJ)/flags.txt
FLAGS_NOW := $(FL) | $(FF) | $(I_CORE) | $(I_MD) | $(GDK)
$(shell mkdir -p $(OBJ); [ "$$(cat $(FLAGS_STAMP) 2>/dev/null)" = '$(FLAGS_NOW)' ] || printf '%s' '$(FLAGS_NOW)' > $(FLAGS_STAMP))

DEPS := -MMD -MP

$(OBJ)/intro.o: $(SRC)/core/intro.c $(FLAGS_STAMP)
	$(CC) $(FL_INTRO) $(I_CORE) $(DEPS) -c $< -o $@
$(OBJ)/vars.o $(OBJ)/tables.o $(OBJ)/zebes.o: $(OBJ)/%.o: $(GEN_E)/%.c $(FLAGS_STAMP)
	$(CC) $(FL) $(I_CORE) $(DEPS) -c $< -o $@
$(filter-out $(OBJ)/intro.o,$(patsubst %.c,$(OBJ)/%.o,$(notdir $(wildcard $(SRC)/core/*.c)))): $(OBJ)/%.o: $(SRC)/core/%.c $(FLAGS_STAMP)
	$(CC) $(FL) $(I_CORE) $(DEPS) -c $< -o $@
$(patsubst %.c,$(OBJ)/%.o,$(notdir $(MD_C))): $(OBJ)/%.o: $(SRC)/platform/%.c $(FLAGS_STAMP)
	$(CC) $(FL) $(I_MD) $(DEPS) -c $< -o $@
$(patsubst %.c,$(OBJ)/%.o,$(notdir $(GEN_C))): $(OBJ)/%.o: $(GEN)/%.c $(FLAGS_STAMP)
	$(CC) $(FL) $(I_MD) $(DEPS) -c $< -o $@
$(OBJ)/md_main.o $(OBJ)/md_snd.o: $(OBJ)/%.o: $(SRC)/platform/%.c $(FLAGS_STAMP)
	$(CC) $(FL) $(SG) -I$(INC)/front -I$(OBJ) $(DEPS) -c $< -o $@
$(patsubst %,$(OBJ)/front_%.o,$(FRONT)): $(OBJ)/front_%.o: $(SRC)/front/%.c $(FLAGS_STAMP)
	$(CC) $(FF) $(DEPS) -c $< -o $@
$(OBJ)/front_%_c.o: $(GEN)/front/%.c $(FLAGS_STAMP)
	$(CC) $(FF) $(DEPS) -c $< -o $@
$(OBJ)/front_%_s.o: $(GEN)/front/%.s $(FLAGS_STAMP)
	cd $(GEN) && $(CC) -m68000 -c front/$*.s -o $@
# los .s de res/gen/md hacen .incbin con rutas relativas a esa carpeta
$(OBJ)/%.o: $(GEN)/%.s $(FLAGS_STAMP)
	cd $(GEN) && $(CC) -m68000 -c $*.s -o $@
$(GEN)/md_voice.s:
	ONLY_VOICE=1 sh $(ROOT)/tools/conv/make_data.sh

# recursos propios (rescomp de SGDK): res/x.res -> out/obj/res_x.s/.h/.o
$(OBJ)/res_%.o: $(RES)/%.res $(FLAGS_STAMP)
	$(JAVA) -jar $(GDK)/bin/rescomp.jar $< $(OBJ)/res_$*.s -dep $@
	$(CC) $(AFL) $(SG) -c $(OBJ)/res_$*.s -o $@
$(OBJ)/res_private_%.o: $(RES)/private/%.res $(FLAGS_STAMP)
	$(JAVA) -jar $(GDK)/bin/rescomp.jar $< $(OBJ)/res_private_$*.s -dep $@
	$(CC) $(AFL) $(SG) -c $(OBJ)/res_private_$*.s -o $@

# cabecera del cartucho y arranque: el sega.s de SGDK hace .incbin "out/rom_header.bin"
$(OBJ)/rom_header.o: $(SRC)/boot/rom_header.c $(FLAGS_STAMP)
	$(CC) $(FL) $(SG) -c $< -o $@
$(HDR)/out/rom_header.bin: $(OBJ)/rom_header.o
	mkdir -p $(HDR)/out && $(OBJCOPY) -O binary $< $@
$(OBJ)/sega.o: $(HDR)/out/rom_header.bin $(GDK)/src/boot/sega.s
	cp $(GDK)/src/boot/sega.s $(OBJ)/sega.s
	cd $(HDR) && $(CC) $(AFL) $(SG) -c $(OBJ)/sega.s -o $@

$(OUT)/rom.out: $(OBJ)/sega.o $(OBJS) $(EXTRA_OBJ) $(SRC)/boot/md_ssf2.ld
	$(CC) -m68000 -fno-use-linker-plugin -fno-lto -n -T $(SRC)/boot/md_ssf2.ld -nostdlib $(OBJ)/sega.o $$(ls $(OBJS)) $(EXTRA_OBJ) \
	    $(GDK)/lib/libmd.a $(GDK)/lib/libgcc.a -o $@ -Wl,--build-id=none -Wl,-z,noexecstack -Wl,--gc-sections -Wl,-Map,$(OUT)/rom.map
$(OUT)/rom.bin: $(OUT)/rom.out
	$(OBJCOPY) -O binary $< $@
	$(NM) -n $< > $(OUT)/symbol.txt
	python3 $(ROOT)/tools/emu_harness/md_fix_rom.py $@
	ls -l $@

data:
	sh $(ROOT)/tools/conv/make_data.sh
full: data
	$(MAKE) rom
host:
	sh $(ROOT)/host/build.sh
clean:
	rm -rf $(OUT)

-include $(wildcard $(OBJ)/*.d)
