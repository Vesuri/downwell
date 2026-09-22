# Downwell — native Amiga 68020 cross-build.

OUT      := out/Downwell
CC       := m68k-amiga-elf-gcc
AS       := m68k-amiga-elf-as
OBJDUMP  := m68k-amiga-elf-objdump
SDKDIR   := $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  := $(abspath $(dir $(shell which $(CC)))../../support)
REPO     := ..
AMIGA_SRC := $(REPO)/src/platform/amiga

CFLAGS := -g -O2 -m68020 -msoft-float -nostdlib -MP -MMD \
	-fno-tree-loop-distribution -ffunction-sections -fdata-sections \
	-Wall -Wextra -Wpedantic -DDOWNWELL_PLATFORM_AMIGA
CPPFLAGS := $(CFLAGS) -x c++ -std=gnu++17 -fno-rtti -fno-exceptions \
	-fno-use-cxa-atexit
ASFLAGS := -mcpu=68020 -g --register-prefix-optional -I"$(SDKDIR)" -I"$(CURDIR)"
LDFLAGS := -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

AMIGA_HUD := $(REPO)/generated/amiga-hud.dwap
AMIGA_GAME := $(REPO)/generated/amiga-game.dwap
OBJECTS := obj/DownwellAmiga.o obj/AmigaRenderer.o obj/AmigaHardware.o \
	obj/CopperList.o obj/AmigaAssets.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o

.PHONY: all check clean audit

all: check $(OUT).exe

check:
	@set -e; for tool in $(CC) $(AS) $(OBJDUMP) elf2hunk; do \
		command -v $$tool >/dev/null || { echo "missing Amiga tool: $$tool"; exit 1; }; \
	done
	@echo "Amiga 68020 toolchain OK"

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

obj/AmigaHardware.o: $(AMIGA_SRC)/framework/AmigaHardware.cpp \
		$(AMIGA_SRC)/framework/AmigaHardware.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/CopperList.o: $(AMIGA_SRC)/framework/CopperList.cpp \
		$(AMIGA_SRC)/framework/CopperList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaRenderer.o: $(AMIGA_SRC)/AmigaRenderer.c $(AMIGA_SRC)/AmigaRenderer.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(AMIGA_HUD):
	$(MAKE) -C $(REPO) generate-amiga-hud

$(AMIGA_GAME):
	$(MAKE) -C $(REPO) generate-amiga-game

obj/AmigaAssets.o: $(AMIGA_SRC)/AmigaAssets.s $(AMIGA_HUD) $(AMIGA_GAME)
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

audit:
	@if $(OBJDUMP) -d $(OUT).elf | grep -qE '__(u?div|u?mod)si3'; then \
		echo "*** unexpected 32-bit software division in native scaffold"; exit 1; \
	else echo "68020 division audit: clean"; fi

clean:
	$(RM) -r obj out .run

-include $(OBJECTS:.o=.d)
