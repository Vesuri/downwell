#ifndef DOWNWELL_AMIGA_FIELD_COMPOSITE_H
#define DOWNWELL_AMIGA_FIELD_COMPOSITE_H
#include "AmigaFieldBands.h"

// Retire only the old mask outside the new opaque arc rectangles. The caller
// must own all pixels within those rectangles and compose them from pristine
// terrain AFTER dirty-row synchronization. This helper does not grant that
// ownership: retained lasers or additional fields require the masked path.
template<class Retire> bool amigaFieldRetireOutside(
    const downwell::TimeFieldOutlines::Circle& old,
    const downwell::TimeFieldOutlines::Circle& next,unsigned origin,Retire retire){
    if(old.x!=next.x||old.y!=next.y||old.radius<128*256||old.radius>=130*256)return false;
    struct Span {unsigned first,end;};Span spans[4];unsigned count=0;
    if(!amigaFieldBandParts(next,origin,[&](unsigned,unsigned row,unsigned,unsigned,unsigned height){
        spans[count++]={row,row+height};
    }))return false;
    bool okay=true;
    const bool admitted=amigaFieldBandParts(old,origin,
        [&](unsigned image,unsigned row,unsigned sx,unsigned sy,unsigned height){
            const unsigned start=row,end=row+height;
            while(okay&&row<end){
                unsigned stop=end,covered=row;
                for(unsigned i=0;i<count;++i){
                    if(spans[i].first<=row&&row<spans[i].end){
                        if(spans[i].end>covered)covered=spans[i].end;
                    }else if(spans[i].first>row&&spans[i].first<stop)stop=spans[i].first;
                }
                if(covered>row){row=covered<end?covered:end;continue;}
                okay=retire(image,row,sx,sy+row-start,stop-row);row=stop;
            }
        });
    return admitted&&okay;
}
#endif
