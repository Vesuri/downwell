#ifndef DOWNWELL_AMIGA_FIELD_BANDS_H
#define DOWNWELL_AMIGA_FIELD_BANDS_H
#include "AmigaFieldColumns.h"

// Describes prepared transfers only. No per-frame rasterization, source
// copying, or expansion. Other placements use the compiled column renderer.
template<class Transfer> bool amigaFieldBandParts(
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,Transfer transfer){
    using namespace downwell;
    if(circle.radius<128*256||circle.radius>=130*256||circle.x<182||circle.x>198||origin>=320)return false;
    const auto* frame=fieldBandFrames[fieldColumnRadii[circle.radius-128*256]];
    for(unsigned i=0;i<2;++i){
        const auto& image=fieldBandImages[frame[i].image];
        const int top=circle.y+frame[i].top;
        int first=top,end=top+image.height;
        if(first<0)first=0;
        if(end>284)end=284;
        if(first>=end)continue;
        unsigned row=origin+unsigned(first),height=unsigned(end-first);
        if(row>=320)row-=320;
        unsigned part=320-row;if(part>height)part=height;
        const unsigned sx=198-circle.x,sy=first-top;
        transfer(frame[i].image,row,sx,sy,part);
        if(part<height)transfer(frame[i].image,0,sx,sy+part,height-part);
    }
    return true;
}
#endif
