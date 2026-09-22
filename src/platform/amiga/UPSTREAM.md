# Amiga platform provenance

The hardware setup is based on the proven GCC/NDK patterns in the local Rescue
on Fractalus and Revs ports. Downwell carries deliberately small adaptations
of their `AmigaHardware` and `CopperList` classes rather than importing the
complete blitter/bitmap framework before it is needed. The adaptations retain
the sibling ports' register ownership, deferred COP1LC publication, repeated
interrupt acknowledgement, and fixed-capacity copper construction while
specialising display geometry for Downwell's 2+2 dual playfield.

Reference implementations inspected:

- `Rescue on Fractalus/src/platform/amiga`
- `Revs/src/platform/amiga`
- `dA JoRMaS/Template/C++`

The freestanding compiler support is supplied by the shared Amiga toolchain at
build time (`~/.local/support/gcc8_{a,c}_support.*`), as in the existing ports.
