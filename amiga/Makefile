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
GEMHIGH_VERIFY ?= 0
LASER_VERIFY ?= 0
RESULTS_RESTART_VERIFY ?= 0
BOB_VERIFY ?= 0
EXIT_WIPE_VERIFY ?= 0
CLEAR_VERIFY ?= 0
SKIP_INTRO ?= 0
SKIP_TO_GAME ?= 0
SKIP_LEVEL ?= 0
PROBES ?= 0
FPSCOUNT ?= 0
PROFILE_AUTORUN ?= 0
PROFILE_NAVIGATE ?= 0
PROFILE_LASER ?= 0
# Keep the measured default; allow controlled compiler comparisons.
AMIGA_OPTIMIZATION ?= -O3
ifneq ($(words $(AMIGA_OPTIMIZATION)),1)
$(error AMIGA_OPTIMIZATION must be -O2, -O3 or -Os)
endif
ifneq ($(filter-out -O2 -O3 -Os,$(AMIGA_OPTIMIZATION)),)
$(error AMIGA_OPTIMIZATION must be -O2, -O3 or -Os)
endif
ifeq ($(PROBES)$(FPSCOUNT),11)
$(error Use PROBES or FPSCOUNT separately)
endif
ifeq ($(PROFILE_AUTORUN)$(PROBES)$(FPSCOUNT),100)
$(error PROFILE_AUTORUN requires PROBES=1 or FPSCOUNT=1)
endif

CFLAGS := -g $(AMIGA_OPTIMIZATION) -flto -m68000 -msoft-float -nostdlib -MP -MMD \
	-fno-tree-loop-distribution -ffunction-sections -fdata-sections \
	-Wall -Wextra -Wpedantic -DNO_ASSEMBLER -DDOWNWELL_PLATFORM_AMIGA
ifeq ($(EXIT_WIPE_VERIFY),1)
CFLAGS += -DDOWNWELL_EXIT_WIPE_VERIFY
endif
ifeq ($(CLEAR_VERIFY),1)
CFLAGS += -DDOWNWELL_CLEAR_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(REPLAY_VERIFY),1)
CFLAGS += -DDOWNWELL_REPLAY_VERIFY
endif
ifeq ($(SCENE_VERIFY),1)
CFLAGS += -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(GEMHIGH_VERIFY),1)
CFLAGS += -DDOWNWELL_GEMHIGH_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(LASER_VERIFY),1)
CFLAGS += -DDOWNWELL_LASER_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(RESULTS_RESTART_VERIFY),1)
CFLAGS += -DDOWNWELL_RESULTS_RESTART_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(BOB_VERIFY),1)
CFLAGS += -DDOWNWELL_BOB_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(SKIP_INTRO),1)
CFLAGS += -DDOWNWELL_SKIP_INTRO
endif
ifneq ($(filter 1,$(SKIP_TO_GAME) $(SKIP_LEVEL)),)
CFLAGS += -DDOWNWELL_SKIP_TO_GAME
endif
ifeq ($(SKIP_LEVEL),1)
CFLAGS += -DDOWNWELL_SKIP_LEVEL
endif
ifeq ($(PROBES),1)
CFLAGS += -DDOWNWELL_PROBES
endif
ifeq ($(FPSCOUNT),1)
CFLAGS += -DDOWNWELL_FPSCOUNT
endif
ifeq ($(PROFILE_AUTORUN),1)
CFLAGS += -DDOWNWELL_PROFILE_AUTORUN
endif
ifeq ($(PROFILE_NAVIGATE),1)
CFLAGS += -DDOWNWELL_PROFILE_NAVIGATE
endif
ifneq ($(filter 1,$(PROBES) $(FPSCOUNT)),)
ifeq ($(PROFILE_LASER),1)
CFLAGS += -DDOWNWELL_PROFILE_LASER
endif
ifneq ($(filter 1,$(SCENE_VERIFY) $(REPLAY_VERIFY) $(BOB_VERIFY) $(RESULTS_RESTART_VERIFY) $(GEMHIGH_VERIFY) $(LASER_VERIFY)),)
$(error Profiling must not be combined with verification instrumentation)
endif
endif
CPPFLAGS := $(CFLAGS) -x c++ -std=gnu++17 -fno-rtti -fno-exceptions \
	-fno-use-cxa-atexit -DDOWNWELL_FREESTANDING \
	-I$(AMIGA_SRC)/compat -I$(REPO)/src
ASFLAGS := -mcpu=68000 -g --register-prefix-optional -I"$(SDKDIR)" -I"$(CURDIR)"
LDFLAGS := -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

AMIGA_HUD := $(REPO)/generated/amiga-hud.dwap
AMIGA_GAME := $(REPO)/generated/amiga-game.dwap
AMIGA_EXIT_MASK := $(REPO)/generated/amiga-game.exit-mask
AMIGA_SELECTION_MASK := $(REPO)/generated/amiga-game.selection-mask
AMIGA_RESULTS := $(REPO)/generated/amiga-results.dwap
AMIGA_SHOP := $(REPO)/generated/amiga-shop.dwap
AMIGA_CLEAR := $(REPO)/generated/amiga-clear.dwap
ROOM_PACK := $(REPO)/generated/downwell-rooms.dwrm
OBJECT_PACK := $(REPO)/generated/downwell-objects.dwob
BUILDER_PACK := $(REPO)/generated/downwell-builder-patterns.dwbp
OBJECTS := obj/DownwellAmiga.o obj/AmigaProfile.o obj/AmigaInput.o obj/AmigaPlayer.o \
	obj/AmigaPortRuntime.o obj/AmigaSceneRuntime.o \
	obj/DownwellCopperLists.o \
	obj/AmigaTerrain.o obj/AmigaMenu.o obj/AmigaMenuScroll.o \
	obj/AmigaMenuCache.o obj/AmigaSplashCache.o obj/AmigaPublisherScene.o obj/AmigaCreditsScene.o obj/AmigaControlsScene.o obj/AmigaStyleRenderer.o obj/AmigaStyleScene.o obj/AmigaUpgradeScene.o obj/AmigaSelectionLayer.o \
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

.PHONY: FORCE_STARTUP_OPTIONS
obj/startup-options: FORCE_STARTUP_OPTIONS
	@mkdir -p obj
	@printf '%s\n' '$(SKIP_INTRO)' '$(SKIP_TO_GAME)' '$(SKIP_LEVEL)' '$(AMIGA_OPTIMIZATION)' '$(LASER_VERIFY)' '$(PROFILE_LASER)' > $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@

$(OBJECTS): obj/startup-options
$(OBJECTS): $(REPO)/generated/downwell-goal-mask.h

$(REPO)/generated/downwell-goal-mask.h: $(REPO)/tools/generate-goal-mask.py $(REPO)/tools/export-goal-mask.csx $(REPO)/data/data.win
	$(MAKE) -C $(REPO) generated/downwell-goal-mask.h

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

obj/AmigaProfile.o: $(AMIGA_SRC)/AmigaProfile.cpp $(AMIGA_SRC)/AmigaProfile.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPlayer.o: $(REPO)/src/core/PlayerMotion.cpp $(REPO)/src/core/PlayerMotion.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPortRuntime.o: $(REPO)/src/core/GameplayWorld.cpp \
		$(REPO)/src/core/GameplayWorld.h $(REPO)/src/core/PlayerMotion.h
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

obj/AmigaUpgradeScene.o: $(AMIGA_SRC)/AmigaUpgradeScene.cpp $(AMIGA_SRC)/AmigaUpgradeScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaStyleScene.o: $(AMIGA_SRC)/AmigaStyleScene.cpp $(AMIGA_SRC)/AmigaStyleScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPublisherScene.o: $(AMIGA_SRC)/AmigaPublisherScene.cpp $(AMIGA_SRC)/AmigaPublisherScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaCreditsScene.o: $(AMIGA_SRC)/AmigaCreditsScene.cpp $(AMIGA_SRC)/AmigaCreditsScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaControlsScene.o: $(AMIGA_SRC)/AmigaControlsScene.cpp $(AMIGA_SRC)/AmigaControlsScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSelectionLayer.o: $(AMIGA_SRC)/AmigaSelectionLayer.cpp $(AMIGA_SRC)/AmigaSelectionLayer.h
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

$(AMIGA_RESULTS): $(REPO)/tools/generate-amiga-hud.py
	$(MAKE) -C $(REPO) generate-amiga-results

$(AMIGA_SHOP): $(REPO)/tools/generate-amiga-hud.py $(REPO)/data/language.ini
	$(MAKE) -C $(REPO) generate-amiga-shop

$(AMIGA_CLEAR): $(REPO)/tools/generate-amiga-hud.py $(REPO)/data/language.ini
	$(MAKE) -C $(REPO) generate-amiga-clear

$(ROOM_PACK):
	$(MAKE) -C $(REPO) generate-room-pack

$(OBJECT_PACK):
	$(MAKE) -C $(REPO) generate-object-pack

$(BUILDER_PACK):
	$(MAKE) -C $(REPO) generate-builder-pattern-pack

$(AMIGA_EXIT_MASK): $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_SELECTION_MASK): $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

obj/AmigaAssets.o: $(AMIGA_SRC)/AmigaAssets.s $(AMIGA_HUD) $(AMIGA_GAME) $(AMIGA_EXIT_MASK) $(AMIGA_SELECTION_MASK) \
		$(AMIGA_RESULTS) $(AMIGA_SHOP) $(AMIGA_CLEAR) $(ROOM_PACK) $(OBJECT_PACK) $(BUILDER_PACK)
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
