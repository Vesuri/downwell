# Downwell Amiga 68020 target bootstrap.
# The first native milestone will add the dA JoRMaS framework and link a real
# executable. Until then this target verifies that the shared toolchain selected
# by env.sh is complete and reports the intentional boundary clearly.

.PHONY: all check clean

TOOLS := m68k-amiga-elf-gcc m68k-amiga-elf-as m68k-amiga-elf-objdump \
	elf2hunk vasmm68k_mot fs-uae

all: check
	@echo "Amiga 68020 scaffold ready; native display target not implemented yet."

check:
	@set -e; for tool in $(TOOLS); do \
		command -v $$tool >/dev/null || { echo "missing Amiga tool: $$tool"; exit 1; }; \
	done
	@echo "Amiga toolchain OK"

clean:
	$(RM) -r obj out .run

