# Downwell — native Amiga 68000 cross-build.

OUT      := out/Downwell
CC       := m68k-amiga-elf-gcc
AS       := m68k-amiga-elf-as
OBJDUMP  := m68k-amiga-elf-objdump
SDKDIR   := $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  := $(abspath $(dir $(shell which $(CC)))../../support)
REPO     := ..
AMIGA_SRC := $(REPO)/src/platform/amiga

CFLAGS := -g -O2 -m68000 -msoft-float -nostdlib -MP -MMD \
	-fno-tree-loop-distribution -ffunction-sections -fdata-sections \
	-Wall -Wextra -Wpedantic -DDOWNWELL_PLATFORM_AMIGA
CPPFLAGS := $(CFLAGS) -x c++ -std=gnu++17 -fno-rtti -fno-exceptions \
	-fno-use-cxa-atexit -DDOWNWELL_FREESTANDING \
	-I$(AMIGA_SRC)/compat -I$(REPO)/src
ASFLAGS := -mcpu=68000 -g --register-prefix-optional -I"$(SDKDIR)" -I"$(CURDIR)"
LDFLAGS := -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

AMIGA_HUD := $(REPO)/generated/amiga-hud.dwap
AMIGA_GAME := $(REPO)/generated/amiga-game.dwap
ROOM_PACK := $(REPO)/generated/downwell-rooms.dwrm
OBJECT_PACK := $(REPO)/generated/downwell-objects.dwob
BUILDER_PACK := $(REPO)/generated/downwell-builder-patterns.dwbp
OBJECTS := obj/DownwellAmiga.o obj/AmigaInput.o obj/AmigaRenderer.o \
	obj/AmigaHardware.o \
	obj/AmigaMemory.o obj/CopperList.o obj/RoomPack.o obj/ObjectPack.o \
	obj/BuilderPatternPack.o obj/Replay.o \
	obj/AmigaAssets.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o

.PHONY: all check clean audit

all: check $(OUT).exe

check:
	@set -e; for tool in $(CC) $(AS) $(OBJDUMP) elf2hunk; do \
		command -v $$tool >/dev/null || { echo "missing Amiga tool: $$tool"; exit 1; }; \
	done
	@echo "Amiga 68000 toolchain OK"

$(OUT).exe: $(OUT).elf
	@elf2hunk $< $@
	@echo "Built $@"

$(OUT).elf: $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s
	@$(MAKE) --no-print-directory audit

obj/DownwellAmiga.o: $(AMIGA_SRC)/DownwellAmiga.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaInput.o: $(AMIGA_SRC)/AmigaInput.cpp $(AMIGA_SRC)/AmigaInput.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaHardware.o: $(AMIGA_SRC)/framework/AmigaHardware.cpp \
		$(AMIGA_SRC)/framework/AmigaHardware.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMemory.o: $(AMIGA_SRC)/framework/AmigaMemory.cpp \
		$(AMIGA_SRC)/framework/AmigaMemory.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/CopperList.o: $(AMIGA_SRC)/framework/CopperList.cpp \
		$(AMIGA_SRC)/framework/CopperList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/RoomPack.o: $(REPO)/src/core/RoomPack.cpp $(REPO)/src/core/RoomPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/ObjectPack.o: $(REPO)/src/core/ObjectPack.cpp $(REPO)/src/core/ObjectPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/BuilderPatternPack.o: $(REPO)/src/core/BuilderPatternPack.cpp \
		$(REPO)/src/core/BuilderPatternPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Replay.o: $(REPO)/src/core/Replay.cpp $(REPO)/src/core/Replay.h \
		$(REPO)/src/core/Input.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaRenderer.o: $(AMIGA_SRC)/AmigaRenderer.c $(AMIGA_SRC)/AmigaRenderer.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(AMIGA_HUD):
	$(MAKE) -C $(REPO) generate-amiga-hud

$(AMIGA_GAME):
	$(MAKE) -C $(REPO) generate-amiga-game

$(ROOM_PACK):
	$(MAKE) -C $(REPO) generate-room-pack

$(OBJECT_PACK):
	$(MAKE) -C $(REPO) generate-object-pack

$(BUILDER_PACK):
	$(MAKE) -C $(REPO) generate-builder-pattern-pack

obj/AmigaAssets.o: $(AMIGA_SRC)/AmigaAssets.s $(AMIGA_HUD) $(AMIGA_GAME) \
		$(ROOM_PACK) $(OBJECT_PACK) $(BUILDER_PACK)
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

audit:
	@if $(OBJDUMP) -t $(OUT).elf | grep -qE \
		'__(ashl|ashr|lshr|mul|u?div|u?mod)di3'; then \
		echo "*** unexpected 64-bit software arithmetic in native executable"; exit 1; \
	else echo "68000 64-bit helper audit: clean"; fi
	@if $(OBJDUMP) -d $(OUT).elf | grep -qiE \
		'^[[:space:][:xdigit:]]+:[[:space:][:xdigit:]]+[[:space:]]+(bf[a-z]+|bkpt|callm|cas2?|chk2|cmp2|div[su]l|extb|move16|movec|moves|pack|rt[dm]|trap[a-z]{2}|unpk)([.[:space:]]|$$)'; then \
		echo "*** instruction newer than 68000 in native executable"; exit 1; \
	else echo "68000 instruction audit: clean"; fi

clean:
	$(RM) -r obj out .run

-include $(OBJECTS:.o=.d)
