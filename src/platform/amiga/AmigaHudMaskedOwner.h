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
        composed_ = amigaHudStageBegin(&stage_, x, y, words, rows) != 0;
        return composed_;
    }
    // Republish the last composed stage at row y without composing it
    // again: false once a publish folded an erase into it (mergeErase), or
    // before anything was composed.
    bool move(int y) {
        if (!composed_) return false;
        stage_.y = y;
        return true;
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
        amigaRendererPublishHudStageShown(renderer, &stage_, shown_);
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
        amigaRendererEraseHudMask(renderer, x_, y_, words_, rows_, shown_);
    }
    void mergeErase() {
        composed_ = false;
        for (unsigned r = 0; r < rows_; ++r)
            for (unsigned w = 0; w < words_; ++w) {
                const UWORD old = UWORD(shown_[r][w] & ~stage_.mask[r][w]);
                if (!old) continue;
                stage_.low[r][w] = UWORD(stage_.low[r][w] & ~old);
                stage_.high[r][w] = UWORD(stage_.high[r][w] & ~old);
                stage_.mask[r][w] |= old;
            }
    }
    AmigaHudStage stage_;
    UWORD shown_[amigaHudStageRows][amigaHudStageWords] = {};
    bool published_ = false, composed_ = false;
    int x_ = 0, y_ = 0;
    unsigned words_ = 0, rows_ = 0;
};
#endif
