#ifndef DOWNWELL_AMIGA_ENTRY_MASK_H
#define DOWNWELL_AMIGA_ENTRY_MASK_H
// Offline opacity of sprEnterDither: select an opaque-black playfield pen,
// not a white sprite. One immutable 384-wide plane; the scrolling gameplay
// copper owns the DMA crop/modulo just as it does for the exit cover.
struct AmigaEntryMask {
    static constexpr unsigned stride=48,height=648,bytes=stride*height;
    static bool offset(int screenY,unsigned& result){
        // Sprite bottom is screenY+400. The compact solid tail is valid only
        // while that bottom lies at/beyond display row 284.
        if(screenY < -116)return false;
        // Branch before subtraction to avoid overflow for extreme input.
        unsigned row=screenY>=684?0:screenY<=320?364:unsigned(684-screenY);
        result=row*stride;return true;
    }
};
#endif
