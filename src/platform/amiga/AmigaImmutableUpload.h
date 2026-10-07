#pragma once

// Bounded staging of immutable words into a shared Chip bank. The owner must
// invalidate before another scene writes that bank. Completion is a residency
// certificate, not permission to expose a partially uploaded image to DMA.
class AmigaImmutableUpload {
    unsigned short* destination_ = nullptr;
    const unsigned short* source_ = nullptr;
    unsigned words_ = 0, copied_ = 0;
public:
    void invalidate() { destination_ = nullptr; source_ = nullptr; words_ = copied_ = 0; }
    bool step(unsigned short* destination, const unsigned short* source,
              unsigned words, unsigned budget) {
        if (!destination || !source) return false;
        if (destination_ != destination || source_ != source || words_ != words) {
            destination_ = destination; source_ = source; words_ = words; copied_ = 0;
        }
        const unsigned count = words - copied_ < budget ? words - copied_ : budget;
        const unsigned end = copied_ + count;
        for (; copied_ < end; ++copied_) destination_[copied_] = source_[copied_];
        return copied_ == words_;
    }
};
