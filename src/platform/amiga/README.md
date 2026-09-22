# Amiga backend

This directory will contain the native Amiga implementation of the platform
boundary used by `src/core`.

The display is specified in `docs/amiga-display.md`: a 380x284 visible canvas
on a 384-pixel DMA backing raster, with PF1 assigned to the centred game well
and PF2 assigned to the HUD and side panels.

The initial target is deliberately smaller than the general dA JoRMaS C++
framework: it establishes the exact display and lifecycle directly, then will
adopt proven framework pieces as the renderer needs them. Provenance is tracked
in `UPSTREAM.md`.

The current executable displays a two-layer diagnostic picture and exits back
to Workbench on a fresh left-mouse click.
