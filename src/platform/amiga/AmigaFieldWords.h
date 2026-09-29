#ifndef DOWNWELL_AMIGA_FIELD_WORDS_H
#define DOWNWELL_AMIGA_FIELD_WORDS_H
#include "AmigaFieldSpans.h"

// Callback receives aligned X, Y, coverage and the two final planar colours.
// Generic/synthetic shapes keep the span oracle, including opaque black.
template<class Shape,class Word>
void amigaFieldWords(const Shape& shape,int left,int top,int right,int bottom,Word word){
    amigaFieldSpans(shape,left,top,right,bottom,[&](int first,int end,int y,int pen){
        for(int x=first;x<end;){
            const int next=(x|15)+1<end?(x|15)+1:end;
            const unsigned mask=(0xffffu>>(x&15))&(0xffffu<<(15-((next-1)&15)));
            word(x&~15,y,mask,pen&1?mask:0u,pen&2?mask:0u);
            x=next;
        }
    });
}
template<class Word>
void amigaFieldWords(const downwell::TimeFieldOutlines& shape,
                    int left,int top,int right,int bottom,Word word){
    using namespace downwell;
    if(left>=right||top>=bottom)return;
    for(unsigned i=0;i<shape.count;++i){
        const auto& circle=shape.circles[i];
        int radius=circle.radius-24*256;
        if(radius<0||radius>=512){
            radius=circle.radius-128*256;
            if(radius<0||radius>=512){
                GoalFieldOutline fallback;
                fallback.setRasterCircle(circle.x,circle.y,circle.radius);
                amigaFieldWords(fallback,left,top,right,bottom,word);continue;
            }
            radius+=512;
        }
        const unsigned id=fieldSpanIndices[radius];
        const int firstY=circle.y+fieldShapeFirstRows[id];
        const int rows=fieldShapeOffsets[id+1]-fieldShapeOffsets[id];
        const int begin=top>firstY?top-firstY:0;
        const int end=bottom<firstY+rows?bottom-firstY:rows;
        const int translated=circle.x-fieldWordOrigin;
        int base=translated/16,shift=translated%16;
        if(shift<0){shift+=16;--base;}
        for(int y=begin;y<end;++y){
            const unsigned row=fieldRowIndices[fieldShapeOffsets[id]+y];
            auto emit=[&](int column,unsigned low,unsigned high){
                const int x=column*16;
                if(x>=right||x+16<=left)return;
                unsigned clip=0xffffu;
                if(x<left)clip&=0xffffu>>(left-x);
                if(x+16>right)clip&=0xffffu<<(x+16-right);
                low&=clip;high&=clip;
                if(low|high)word(x,firstY+y,low|high,low,high);
            };
            if(!shift){
                for(unsigned w=fieldWordOffsets[row];w<fieldWordOffsets[row+1];++w){
                    const auto& source=fieldWords[w];
                    emit(base+source.column,source.low,source.high);
                }
                continue;
            }
            bool pending=false;int column=0;unsigned low=0,high=0;
            for(unsigned w=fieldWordOffsets[row];w<fieldWordOffsets[row+1];++w){
                const auto& source=fieldWords[w];
                const int next=base+source.column;
                if(pending&&column!=next){emit(column,low,high);low=high=0;}
                emit(next,low|(unsigned(source.low)>>shift),high|(unsigned(source.high)>>shift));
                low=(unsigned(source.low)<<(16-shift))&0xffffu;
                high=(unsigned(source.high)<<(16-shift))&0xffffu;
                column=next+1;pending=true;
            }
            if(pending)emit(column,low,high);
        }
    }
}
#endif
