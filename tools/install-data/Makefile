.DEFAULT_GOAL := all
# make -C tools/install-data [amiga|test]
# Source amiga/env.sh before building the Amiga binary.
ROOT := ../..
BUILD ?= $(ROOT)/build/install-data
CC ?= cc
CFLAGS ?= -O2 -g
WARN = -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter
# Our sources.
SOURCES = install.c dw.c win.c tables.c sound.c sha256.c
HEADERS = install_config.h io.h sha256.h dw.h win.h tables.h sound.h context.h
# Xiph's integer Vorbis decoder and Ogg framing (BSD), unmodified.
VORBIS_SOURCES = ogg/bitwise.c ogg/framing.c $(addprefix tremor/,block.c codebook.c floor0.c floor1.c info.c mapping0.c mdct.c registry.c res012.c sharedbook.c synthesis.c vorbisfile.c window.c)
AMIGA_CC = m68k-amiga-elf-gcc
ELF2HUNK ?= elf2hunk
AMIGA_NM = $(dir $(shell command -v $(AMIGA_CC)))../m68k-amiga-elf/bin/nm
AMIGA_CPU = -m68020 -mtune=68020 -msoft-float
AMIGA_FLAGS = -std=c99 -O2 -g $(AMIGA_CPU) -nostdlib -fno-builtin -fno-tree-loop-distribute-patterns -fstack-usage -Wall -Wextra -Wno-unused-parameter -Wno-volatile-register-var -DINSTALL_AMIGA

.PHONY: all amiga test pinned
pinned:
	@cd $(ROOT) && shasum -a 256 -c --quiet manifest/install-data.sha256

all: $(BUILD)/DownwellInstallData
$(BUILD):
	mkdir -p $@
$(BUILD)/DownwellInstallData: $(SOURCES) io_unix.c $(HEADERS) Makefile | $(BUILD) pinned
	$(CC) $(CFLAGS) $(WARN) $(SOURCES) io_unix.c -o $@

AMIGA_OBJECTS = $(patsubst %.c,$(BUILD)/amiga/%.o,$(SOURCES) io_amiga.c)
$(BUILD)/amiga/%.o: %.c $(HEADERS) Makefile | pinned
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_FLAGS) -c $< -o $@
$(BUILD)/amiga/start.o: start.s | pinned
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_CPU) -c $< -o $@
$(BUILD)/amiga/lb%.o: lb1sf68.S
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_CPU) -DL$* -x assembler-with-cpp -c $< -o $@
$(BUILD)/DownwellInstallData.elf: $(BUILD)/amiga/start.o $(AMIGA_OBJECTS)
	$(AMIGA_CC) -nostdlib $(AMIGA_CPU) -Wl,--emit-relocs,-Ttext=0,-e,_start,--gc-sections,-Map=$(BUILD)/DownwellInstallData.map $^ -o $@
	$(AMIGA_NM) -u $@ > $(BUILD)/undefined.txt
	@test ! -s $(BUILD)/undefined.txt || (cat $(BUILD)/undefined.txt; false)
$(BUILD)/DownwellInstallData.exe: $(BUILD)/DownwellInstallData.elf
	"$(ELF2HUNK)" $< $@ -s
amiga: $(BUILD)/DownwellInstallData.exe

test: all
	python3 test_install.py $(BUILD)/DownwellInstallData
