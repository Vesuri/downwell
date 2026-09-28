#ifndef DOWNWELL_CORE_ENTRY_DITHER_MASK_H
#define DOWNWELL_CORE_ENTRY_DITHER_MASK_H
namespace downwell {
// Offline centered viewport crop of sprite 129: transparent padding, its
// original 80-row edge, then solid padding. Shared host/Chip-RAM layout.
struct EntryDitherMask {
    static constexpr unsigned stride=48,height=648,bytes=stride*height;
    static bool offset(int screenY,unsigned& result){
        // Finite sprite bottom must remain at/below display row 284.
        if(screenY < -116)return false;
        const unsigned row=screenY>=684?0:screenY<=320?364:unsigned(684-screenY);
        result=row*stride;return true;
    }
};
}
#endif
