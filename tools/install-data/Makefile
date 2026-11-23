.DEFAULT_GOAL := all
# make -C tools/install-data [amiga|test]
# Source amiga/env.sh before building the Amiga binary.
ROOT := ../..
BUILD ?= $(ROOT)/build/install-data
CC ?= cc
CFLAGS ?= -O2 -g
WARN = -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter
# Our sources: the shared ones, then one line per stage (install.c's STAGES).
SOURCES = install.c dw.c win.c tables.c sound.c sha256.c
SOURCES_TEXT = text.c
SOURCES_ROOMS = rooms.c
SOURCES_MASKS = masks.c
SOURCES_CODE = code.c
SOURCES_EXE = exe.c
SOURCES_ART = art.c
SOURCES_DERIVED = derived.c
SOURCES_AUDIO = audio.c vorbis_heap.c
SOURCES += $(SOURCES_TEXT) $(SOURCES_ROOMS) $(SOURCES_MASKS) $(SOURCES_CODE) $(SOURCES_EXE) $(SOURCES_ART) $(SOURCES_DERIVED) $(SOURCES_AUDIO)
HEADERS = install_config.h io.h sha256.h dw.h win.h tables.h sound.h context.h $(wildcard *.h) $(BUILD)/audio_plan.h

# The sound plan, resampler and fade tables (our own data), made at build time.
$(BUILD)/audio_plan.h: gen_audio.py $(ROOT)/docs/sound-inventory.md $(ROOT)/tools/sfx-bank-policy.json $(ROOT)/tools/sfx-bank.py
	@mkdir -p $(@D)
	python3 gen_audio.py $@

# audio.c includes Tremor's header, which wants <stdio.h>.
$(BUILD)/amiga/audio.c.o: AMIGA_FLAGS += -Ivorbis-include

# Tremor and libogg, unmodified: their allocations go to vorbis_heap.c.
VORBIS_SOURCES = ogg/bitwise.c ogg/framing.c $(addprefix tremor/,block.c codebook.c floor0.c floor1.c info.c mapping0.c mdct.c registry.c res012.c sharedbook.c synthesis.c vorbisfile.c window.c)
VORBIS_INCLUDES = -Itremor -Iogg -Iconfig -include vorbis_alloc.h
HOST_VORBIS = $(patsubst %,$(BUILD)/host/%.o,$(VORBIS_SOURCES))
AMIGA_VORBIS = $(patsubst %,$(BUILD)/amiga/%.o,$(VORBIS_SOURCES))
$(HOST_VORBIS): $(BUILD)/host/%.c.o: %.c vorbis_alloc.h | pinned
	@mkdir -p $(@D)
	$(CC) -O2 -g -std=gnu99 -w -ffp-contract=off $(VORBIS_INCLUDES) -c $< -o $@
$(AMIGA_VORBIS): $(BUILD)/amiga/%.c.o: %.c vorbis_alloc.h | pinned
	@mkdir -p $(@D)
	$(AMIGA_CC) -std=gnu99 -O2 -g $(AMIGA_CPU) -nostdlib -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -fstack-usage -w -Ivorbis-include $(VORBIS_INCLUDES) -c $< -o $@
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

# C++ stages (those that reuse src/core or src/platform/amiga code): no
# exceptions, RTTI or C++ runtime, as the game builds them.
CXX ?= c++
CXXWARN = -std=gnu++17 -Wall -Wextra -Werror -Wno-unused-parameter -fno-exceptions -fno-rtti
CXX_INCLUDES = -I$(ROOT)/src -I.
AMIGA_CXX = m68k-amiga-elf-g++
AMIGA_CXXFLAGS = -std=gnu++17 -O2 -g $(AMIGA_CPU) -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fstack-usage -Wall -Wextra -Wno-unused-parameter -DINSTALL_AMIGA -DDOWNWELL_FREESTANDING -DDOWNWELL_PLATFORM_AMIGA -I$(ROOT)/src/platform/amiga/compat $(CXX_INCLUDES)

all: $(BUILD)/DownwellInstallData
$(BUILD):
	mkdir -p $@
HOST_OBJECTS = $(patsubst %,$(BUILD)/host/%.o,$(SOURCES) io_unix.c)
$(BUILD)/host/%.c.o: %.c $(HEADERS) Makefile | pinned
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(WARN) -I$(BUILD) -Iogg -Iconfig -c $< -o $@
$(BUILD)/host/%.cpp.o: %.cpp $(HEADERS) Makefile | pinned
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) $(CXXWARN) $(CXX_INCLUDES) -c $< -o $@
$(BUILD)/DownwellInstallData: $(HOST_OBJECTS) $(HOST_VORBIS)
	$(CXX) $(CFLAGS) $^ -o $@

AMIGA_OBJECTS = $(patsubst %,$(BUILD)/amiga/%.o,$(SOURCES) io_amiga.c)
$(BUILD)/amiga/%.c.o: %.c $(HEADERS) Makefile | pinned
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_FLAGS) -I$(BUILD) -Iogg -Iconfig -c $< -o $@
$(BUILD)/amiga/%.cpp.o: %.cpp $(HEADERS) Makefile | pinned
	@mkdir -p $(@D)
	$(AMIGA_CXX) $(AMIGA_CXXFLAGS) -c $< -o $@
$(BUILD)/amiga/start.o: start.s | pinned
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_CPU) -c $< -o $@
$(BUILD)/amiga/lb%.o: lb1sf68.S
	@mkdir -p $(@D)
	$(AMIGA_CC) $(AMIGA_CPU) -DL$* -x assembler-with-cpp -c $< -o $@
$(BUILD)/DownwellInstallData.elf: $(BUILD)/amiga/start.o $(AMIGA_OBJECTS) $(AMIGA_VORBIS)
	$(AMIGA_CC) -nostdlib $(AMIGA_CPU) -Wl,--emit-relocs,-Ttext=0,-e,_start,--gc-sections,-Map=$(BUILD)/DownwellInstallData.map $^ -o $@
	$(AMIGA_NM) -u $@ > $(BUILD)/undefined.txt
	@test ! -s $(BUILD)/undefined.txt || (cat $(BUILD)/undefined.txt; false)
$(BUILD)/DownwellInstallData.exe: $(BUILD)/DownwellInstallData.elf
	"$(ELF2HUNK)" $< $@ -s
amiga: $(BUILD)/DownwellInstallData.exe

test: all
	python3 test_install.py $(BUILD)/DownwellInstallData
