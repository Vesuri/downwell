# Amiga platform provenance

The initial hardware setup is based on the proven GCC/NDK patterns in the
local Rescue on Fractalus and Revs ports. No framework source file has been
copied verbatim at this stage; Downwell starts with the smaller subset needed
to establish its distinct 380x284, 2+2 dual-playfield display.

Reference implementations inspected:

- `Rescue on Fractalus/src/platform/amiga`
- `Revs/src/platform/amiga`
- `dA JoRMaS/Template/C++`

The freestanding compiler support is supplied by the shared Amiga toolchain at
build time (`~/.local/support/gcc8_{a,c}_support.*`), as in the existing ports.
