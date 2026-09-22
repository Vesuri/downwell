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
ASFLAGS := -mcpu=68020 -g --register-prefix-optional -I"$(SDKDIR)" -I"$(CURDIR)"
LDFLAGS := -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

OBJECTS := obj/DownwellAmiga.o obj/gcc8_c_support.o obj/gcc8_a_support.o

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

obj/DownwellAmiga.o: $(AMIGA_SRC)/DownwellAmiga.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

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
