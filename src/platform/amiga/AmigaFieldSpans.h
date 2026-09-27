#ifndef DOWNWELL_AMIGA_FIELD_SPANS_H
#define DOWNWELL_AMIGA_FIELD_SPANS_H
#include "core/TimeFieldOutlines.h"
#include "../../../generated/downwell-field-spans.h"

// Only the backend's final coverage uses this offline cache. Source Draw/RNG,
// collision masks and the portable pixel oracle remain unchanged.
template<class Shape,class Span>
void amigaFieldSpans(const Shape& shape,int left,int top,int right,int bottom,Span span) {
    shape.drawClippedSpans(left,top,right,bottom,span);
}
template<class Span>
void amigaFieldSpans(const downwell::TimeFieldOutlines& shape,
                    int left,int top,int right,int bottom,Span span) {
    using namespace downwell;
    if (left >= right || top >= bottom) return;
    for (unsigned i=0;i<shape.count;++i) {
        const auto& circle=shape.circles[i];
        int radius=circle.radius-24*256;
        if (radius<0 || radius>=512) {
            radius=circle.radius-128*256;
            if (radius<0 || radius>=512) {
                // Keep synthetic/extended shapes defined by the reference.
                GoalFieldOutline fallback;
                fallback.setRasterCircle(circle.x,circle.y,circle.radius);
                fallback.drawClippedSpans(left,top,right,bottom,span);
                continue;
            }
            radius+=512;
        }
        const unsigned id=fieldSpanIndices[radius];
        const int firstY=circle.y+fieldShapeFirstRows[id];
        const int rows=fieldShapeOffsets[id+1]-fieldShapeOffsets[id];
        const int begin=top>firstY?top-firstY:0;
        const int end=bottom<firstY+rows?bottom-firstY:rows;
        for (int y=begin;y<end;++y) {
            const unsigned row=fieldRowIndices[fieldShapeOffsets[id]+y];
            for (unsigned r=fieldRowOffsets[row];r<fieldRowOffsets[row+1];++r) {
                const auto& run=fieldRuns[r];
                int first=circle.x+run.first,last=circle.x+run.end;
                if (first<left) first=left;
                if (last>right) last=right;
                if (first<last) span(first,last,firstY+y,run.pen);
            }
        }
    }
}
#endif
