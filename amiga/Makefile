# Downwell — native Amiga 68000 cross-build.

OUT      := out/Downwell
CC       := m68k-amiga-elf-gcc
AS       := m68k-amiga-elf-as
OBJDUMP  := m68k-amiga-elf-objdump
SDKDIR   := $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  := $(abspath $(dir $(shell which $(CC)))../../support)
REPO     := ..
AMIGA_SRC := $(REPO)/src/platform/amiga
REPLAY_VERIFY ?= 0
SCENE_VERIFY ?= 0

CFLAGS := -g -O2 -m68000 -msoft-float -nostdlib -MP -MMD \
	-fno-tree-loop-distribution -ffunction-sections -fdata-sections \
	-Wall -Wextra -Wpedantic -DNO_ASSEMBLER -DDOWNWELL_PLATFORM_AMIGA
ifeq ($(REPLAY_VERIFY),1)
CFLAGS += -DDOWNWELL_REPLAY_VERIFY
endif
ifeq ($(SCENE_VERIFY),1)
CFLAGS += -DDOWNWELL_SCENE_VERIFY
endif
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
OBJECTS := obj/DownwellAmiga.o obj/AmigaInput.o obj/AmigaPlayer.o \
	obj/AmigaPortRuntime.o obj/AmigaSceneRuntime.o \
	obj/DownwellCopperLists.o \
	obj/AmigaTerrain.o obj/AmigaMenu.o obj/AmigaMenuScroll.o \
	obj/AmigaMenuCache.o obj/AmigaSplashCache.o obj/AmigaStyleRenderer.o \
	obj/AmigaRenderer.o obj/AmigaBlitter.o \
	obj/AmigaHardware.o obj/Bitmap.o obj/Palette.o obj/Sprite.o obj/Util.o \
	obj/GCCRuntime.o obj/AmigaPlatform.o \
	obj/AmigaMemory.o obj/CopperList.o obj/RoomPack.o obj/ObjectPack.o \
	obj/BuilderPatternPack.o obj/AreaOneBuilder.o obj/GameMakerRandom.o \
	obj/Replay.o \
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

obj/DownwellAmiga.o: $(AMIGA_SRC)/DownwellAmiga.cpp \
		$(REPO)/src/core/PortReplay.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaInput.o: $(AMIGA_SRC)/AmigaInput.cpp $(AMIGA_SRC)/AmigaInput.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPlayer.o: $(AMIGA_SRC)/AmigaPlayer.cpp $(AMIGA_SRC)/AmigaPlayer.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPortRuntime.o: $(AMIGA_SRC)/AmigaPortRuntime.cpp \
		$(AMIGA_SRC)/AmigaPortRuntime.h $(AMIGA_SRC)/AmigaPlayer.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSceneRuntime.o: $(AMIGA_SRC)/AmigaSceneRuntime.cpp \
		$(AMIGA_SRC)/AmigaSceneRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/DownwellCopperLists.o: $(AMIGA_SRC)/DownwellCopperLists.cpp \
		$(AMIGA_SRC)/DownwellCopperLists.h \
		$(AMIGA_SRC)/framework/CopperList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaTerrain.o: $(AMIGA_SRC)/AmigaTerrain.cpp \
		$(AMIGA_SRC)/AmigaTerrain.h $(AMIGA_SRC)/AmigaPortRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenu.o: $(AMIGA_SRC)/AmigaMenu.cpp $(AMIGA_SRC)/AmigaMenu.h \
		$(REPO)/src/core/RoomPack.h $(REPO)/src/core/ObjectPack.h \
		$(REPO)/src/core/GameMakerRandom.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenuScroll.o: $(AMIGA_SRC)/AmigaMenuScroll.cpp \
		$(AMIGA_SRC)/AmigaMenuScroll.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenuCache.o: $(AMIGA_SRC)/AmigaMenuCache.cpp \
		$(AMIGA_SRC)/AmigaMenuCache.h $(AMIGA_SRC)/AmigaMenu.h \
		$(AMIGA_SRC)/AmigaMenuScroll.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaStyleRenderer.o: $(AMIGA_SRC)/AmigaStyleRenderer.cpp \
		$(AMIGA_SRC)/AmigaStyleRenderer.h $(AMIGA_SRC)/AmigaSceneRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSplashCache.o: $(AMIGA_SRC)/AmigaSplashCache.cpp \
		$(AMIGA_SRC)/AmigaSplashCache.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaHardware.o: $(AMIGA_SRC)/framework/AmigaHardware.cpp \
		$(AMIGA_SRC)/framework/AmigaHardware.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Bitmap.o: $(AMIGA_SRC)/framework/Bitmap.cpp \
		$(AMIGA_SRC)/framework/Bitmap.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Palette.o: $(AMIGA_SRC)/framework/Palette.cpp \
		$(AMIGA_SRC)/framework/Palette.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Sprite.o: $(AMIGA_SRC)/framework/Sprite.cpp \
		$(AMIGA_SRC)/framework/Sprite.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Util.o: $(AMIGA_SRC)/framework/Util.cpp \
		$(AMIGA_SRC)/framework/Util.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Wno-narrowing -Wno-overflow -c -o $@ $<

obj/GCCRuntime.o: $(AMIGA_SRC)/framework/GCCRuntime.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPlatform.o: $(AMIGA_SRC)/framework/AmigaPlatform.cpp \
		$(AMIGA_SRC)/framework/AmigaPlatform.h
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

obj/AreaOneBuilder.o: $(REPO)/src/core/AreaOneBuilder.cpp \
		$(REPO)/src/core/AreaOneBuilder.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/GameMakerRandom.o: $(REPO)/src/core/GameMakerRandom.cpp \
		$(REPO)/src/core/GameMakerRandom.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Replay.o: $(REPO)/src/core/Replay.cpp $(REPO)/src/core/Replay.h \
		$(REPO)/src/core/Input.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaRenderer.o: $(AMIGA_SRC)/AmigaRenderer.c $(AMIGA_SRC)/AmigaRenderer.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/AmigaBlitter.o: $(AMIGA_SRC)/AmigaBlitter.cpp \
		$(AMIGA_SRC)/framework/AmigaHardware.h \
		$(AMIGA_SRC)/framework/Bitmap.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(AMIGA_HUD):
	$(MAKE) -C $(REPO) generate-amiga-hud

$(AMIGA_GAME): $(REPO)/tools/generate-amiga-hud.py
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
	@if $(OBJDUMP) -d --no-show-raw-insn $(OUT).elf | grep -qiE \
		'^[[:space:][:xdigit:]]+:[[:space:][:xdigit:]]+[[:space:]]+(bf[a-z]+|bkpt|callm|cas2?|chk2|cmp2|div[su]l|extb|move16|movec|moves|pack|rt[dm]|trap[a-z]{2}|unpk)([.[:space:]]|$$)'; then \
		echo "*** instruction newer than 68000 in native executable"; exit 1; \
	else echo "68000 instruction audit: clean"; fi

# Keep .run: it owns the live FS-UAE pid used by run.sh to replace the
# previous project emulator safely.  Removing it leaves a stale window.
clean:
	$(RM) -r obj out

-include $(OBJECTS:.o=.d)
