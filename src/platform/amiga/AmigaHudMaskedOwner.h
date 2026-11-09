#ifndef DOWNWELL_AMIGA_HUD_MASKED_OWNER_H
#define DOWNWELL_AMIGA_HUD_MASKED_OWNER_H
#include "AmigaRenderer.h"

// A fixed-HUD writer that shares its region with others: each publish erases
// exactly the pixels its previous publish set (never a rectangle), folded
// into the same masked pass when the region is unchanged. Compose into
// stage() between begin() and publish(). Every composed pixel must have a
// non-zero pen; HUD art masks already exclude pen 0.
struct AmigaHudMaskedOwner {
    bool begin(int x, int y, unsigned words, unsigned rows) {
        return amigaHudStageBegin(&stage_, x, y, words, rows) != 0;
    }
    AmigaHudStage* stage() { return &stage_; }
    // Publish the composed stage. fresh: the region was just repainted by
    // another writer, so nothing of ours remains to erase.
    void publish(AmigaRenderer* renderer, bool fresh = false) {
        amigaRendererClearClipRect(renderer);
        if (published_ && !fresh) {
            if (stage_.x != x_ || stage_.y != y_ || stage_.words != words_ || stage_.rows != rows_) eraseSeparately(renderer);
            else mergeErase();
        }
        amigaRendererPublishHudStage(renderer, &stage_, nullptr);
        for (unsigned r = 0; r < stage_.rows; ++r)
            for (unsigned w = 0; w < stage_.words; ++w)
                shown_[r][w] = UWORD(stage_.mask[r][w] & (stage_.low[r][w] | stage_.high[r][w]));
        published_ = true; x_ = stage_.x; y_ = stage_.y; words_ = stage_.words; rows_ = stage_.rows;
    }
    void hide(AmigaRenderer* renderer) {
        if (published_) { amigaRendererClearClipRect(renderer); eraseSeparately(renderer); }
        published_ = false;
    }
    // The HUD layer was repainted by another writer: forget what is shown.
    void invalidate() { published_ = false; }
    bool published() const { return published_; }

private:
    void eraseSeparately(AmigaRenderer* renderer) {
        if (!amigaHudStageBegin(&erase_, x_, y_, words_, rows_)) return;
        for (unsigned r = 0; r < rows_; ++r)
            for (unsigned w = 0; w < words_; ++w) {
                erase_.mask[r][w] = shown_[r][w]; erase_.low[r][w] = erase_.high[r][w] = 0;
            }
        amigaRendererPublishHudStage(renderer, &erase_, nullptr);
    }
    void mergeErase() {
        for (unsigned r = 0; r < rows_; ++r)
            for (unsigned w = 0; w < words_; ++w) {
                const UWORD old = UWORD(shown_[r][w] & ~stage_.mask[r][w]);
                if (!old) continue;
                stage_.low[r][w] = UWORD(stage_.low[r][w] & ~old);
                stage_.high[r][w] = UWORD(stage_.high[r][w] & ~old);
                stage_.mask[r][w] |= old;
            }
    }
    AmigaHudStage stage_, erase_;
    UWORD shown_[amigaHudStageRows][amigaHudStageWords] = {};
    bool published_ = false;
    int x_ = 0, y_ = 0;
    unsigned words_ = 0, rows_ = 0;
};
#endif
