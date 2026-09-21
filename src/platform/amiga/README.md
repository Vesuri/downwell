# Amiga backend

This directory will contain the native Amiga implementation of the platform
boundary used by `src/core`.

The display is specified in `docs/amiga-display.md`: a 380x284 visible canvas
on a 384-pixel DMA backing raster, with PF1 assigned to the centred game well
and PF2 assigned to the HUD and side panels.

The established ports vendor the useful subset of the dA JoRMaS C++ framework
here—`AmigaHardware`, `Bitmap`, `CopperList`, `Palette`, `Sprite`, and `Util`—and
keep the cross-build itself under the repository-level `amiga/` directory.

Vendoring is deliberately deferred until the first display prototype fixes the
required API and chipset/memory assumptions. The sibling ports have diverged
from the template in meaningful ways, so blindly copying one fork now would
make an accidental choice about display layout and 68000 compatibility. This
port explicitly targets a 68020 and will record its selected upstream and local
changes in `framework/UPSTREAM.md` when brought in.
