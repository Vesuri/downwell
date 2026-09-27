#ifndef DOWNWELL_CORE_RUNNER_LINE_BOUNDS_H
#define DOWNWELL_CORE_RUNNER_LINE_BOUNDS_H
#include "RunnerSpatialLine.h"
namespace downwell {
// Ordinary instance candidate's line/bounds gate (0x46517c..0x4652ce).
// Unlike tree traversal, it clips against the EXTENDED right edge and keeps
// the input endpoint order on equal X. It never reconstructs x2 from x1+dx.
struct RunnerLineBounds {
    using Math=RunnerSpatialSplit;using Real=RunnerImageValue;using Ops=RunnerSpatialLine;
    static bool test(RunnerLineArguments args,Math::Box box,bool compatibility,
        bool excluded,bool& hit){
        if(!Math::valid(box)||!Math::valid(args.x1)||!Math::valid(args.y1)||
           !Math::valid(args.x2)||!Math::valid(args.y2))return false;
        const Real extension=compatibility?Real{0x3f800000}:Real{0xb727c5ac};
        Real right,bottom;
        if(!Ops::add(box.right,extension,right)||!Ops::add(box.bottom,extension,bottom))return false;
        auto sx=args.x1,sy=args.y1,ex=args.x2,ey=args.y2;
        const auto miss=[&]{hit=false;return true;};
        if(!Math::less(Math::less(sx,ex)?sx:ex,right)||Math::less(Math::less(sx,ex)?ex:sx,box.left)||
           !Math::less(Math::less(sy,ey)?sy:ey,bottom)||Math::less(Math::less(sy,ey)?ey:sy,box.top)||excluded)return miss();
        if(Math::less(ex,sx)){auto t=sx;sx=ex;ex=t;t=sy;sy=ey;ey=t;}
        Real a,b,c;
        if(Math::less(sx,box.left)){
            if(!Math::sub(box.left,sx,a)||!Math::sub(ey,sy,b)||!Ops::mul(a,b,a)||
               !Math::sub(ex,sx,c)||!Ops::div(a,c,a)||!Ops::add(sy,a,sy))return false;
            sx=box.left;
        }
        if(Math::less(right,ex)){
            if(!Math::sub(ey,sy,b)||!Math::sub(right,ex,a)||!Math::sub(ex,sx,c)||
               !Ops::mul(a,b,a)||!Ops::div(a,c,a)||!Ops::add(ey,a,ey))return false;
        }
        if(Math::less(sy,box.top)&&Math::less(ey,box.top))return miss();
        if(!Math::less(sy,bottom)&&!Math::less(ey,bottom))return miss();
        hit=true;return true;
    }
};
}
#endif
