#ifndef DOWNWELL_AMIGA_EXIT_MASK_H
#define DOWNWELL_AMIGA_EXIT_MASK_H
// Offline single-plane cover, positioned at logical x=110. Shared odd/even
// DMA modulo is 48 - 44 bytes; no runtime planar conversion or shifting.
struct AmigaExitMask {
    static constexpr unsigned stride = 48, height = 648, bytes = stride * height;
    static constexpr unsigned selectionStride = 22, selectionBytes = selectionStride * height;
    static unsigned offset(int screenY) {
        // Clamp before subtraction: an extreme negative view-relative
        // coordinate must not overflow and select a DMA pointer outside the bank.
        const unsigned row=screenY>=284?0:screenY<=-80?364:unsigned(284-screenY);
        return row * stride;
    }
};
#endif
