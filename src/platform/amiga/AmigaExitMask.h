#ifndef DOWNWELL_AMIGA_EXIT_MASK_H
#define DOWNWELL_AMIGA_EXIT_MASK_H
// Offline single-plane cover, positioned at logical x=110. Shared odd/even
// DMA modulo is 48 - 44 bytes; no runtime planar conversion or shifting.
struct AmigaExitMask {
    static constexpr unsigned stride = 48, height = 648, bytes = stride * height;
    static constexpr unsigned selectionStride = 22, selectionBytes = selectionStride * height;
    static unsigned offset(int screenY) {
        int row = 284 - screenY;
        if (row < 0) row = 0;
        if (row > 364) row = 364;
        return static_cast<unsigned>(row) * stride;
    }
};
#endif
