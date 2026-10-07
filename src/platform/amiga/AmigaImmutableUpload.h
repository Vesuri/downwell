#pragma once

// Bounded staging of immutable elements into a shared Chip bank. The owner must
// invalidate before another scene writes that bank. Completion is a residency
// certificate, not permission to expose a partially uploaded image to DMA.
template<class Element> class AmigaImmutableUploadOf {
    Element* destination_ = nullptr;
    const Element* source_ = nullptr;
    unsigned words_ = 0, copied_ = 0;
public:
    void invalidate() { destination_ = nullptr; source_ = nullptr; words_ = copied_ = 0; }
    bool step(Element* destination, const Element* source,
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
using AmigaImmutableUpload = AmigaImmutableUploadOf<unsigned short>;
using AmigaImmutableByteUpload = AmigaImmutableUploadOf<unsigned char>;
