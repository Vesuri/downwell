#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_LINE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_LINE_H
#include "RunnerLineArguments.h"
#include "RunnerSpatialSplit.h"
#include "DivideBinary64.h"
namespace downwell {
// Float operation order of 0x4fd3bc and the box gates in 0x500860.
// Normal/zero inputs, nearest-even arithmetic, masked SSE exceptions. The
// right-edge zero-divisor case below preserves the resulting infinity/NaN
// comparisons without requiring a floating-point unit. Other unsupported
// arithmetic fails the query instead of inventing a miss.
struct RunnerSpatialLine {
    using Math=RunnerSpatialSplit;
    using Real=RunnerImageValue;
    Real x{},y{},dx{},dy{};
    static bool add(Real a,Real b,Real& out){return Math::sub(a,Math::negate(b),out);}
    static bool mul(Real a,Real b,Real& out){MotionBinary64 n;
        return multiplyBinary64(a.promoted(),b.promoted(),n)&&out.assign(n);}
    static bool div(Real a,Real b,Real& out){MotionBinary64 n;
        return divideBinary64(a.promoted(),b.promoted(),n)&&out.assign(n);}
    bool assign(const RunnerLineArguments& args){
        if(!Math::valid(args.x1)||!Math::valid(args.y1)||!Math::valid(args.x2)||!Math::valid(args.y2))return false;
        RunnerSpatialLine next;
        const bool forward=Math::less(args.x1,args.x2); // Ties reverse endpoints.
        next.x=forward?args.x1:args.x2;next.y=forward?args.y1:args.y2;
        if(!Math::sub(forward?args.x2:args.x1,next.x,next.dx)||
           !Math::sub(forward?args.y2:args.y1,next.y,next.dy))return false;
        *this=next;return true;
    }
    bool overlaps(Math::Box box,bool& hit)const{
        if(!Math::valid(box)||!Math::valid(x)||!Math::valid(y)||!Math::valid(dx)||!Math::valid(dy)||Math::less(dx,{}))return false;
        Real ex,ey,right,bottom;
        if(!add(x,dx,ex)||!add(y,dy,ey)||!add(box.right,{0x3f800000},right)||
           !add(box.bottom,{0x3f800000},bottom))return false;
        const auto miss=[&]{hit=false;return true;};
        if(!Math::less(x,right)||Math::less(ex,box.left)||
           !Math::less(Math::less(y,ey)?y:ey,bottom)||Math::less(Math::less(y,ey)?ey:y,box.top))return miss();
        auto sx=x,sy=y;
        Real a,b,c;
        if(Math::less(sx,box.left)){
            if(!Math::sub(box.left,sx,a)||!Math::sub(ey,sy,b)||!mul(a,b,a)||
               !Math::sub(ex,sx,c)||!div(a,c,a)||!add(sy,a,sy))return false;
            sx=box.left;
        }
        if(Math::less(box.right,ex)){
            if(!Math::sub(box.right,ex,a)||!Math::sub(ey,sy,b)||!Math::sub(ex,sx,c)||
               !mul(a,b,a))return false;
            if(!(c.bits&0x7fffffffu)){
                // 0x500a98 DIVSS followed by ADDSS: finite + (+/-inf) is
                // +/-inf; 0/0 yields NaN. COMISS/JA at 0x500aad and
                // COMISS/JAE at 0x500ab7 both reject unordered comparisons.
                // sy, top and bottom are still finite here. These are the
                // exact final gates after substituting that exceptional ey.
                if(!(a.bits&0x7fffffffu)){hit=true;return true;}
                const bool negative=((a.bits^c.bits)&0x80000000u)!=0;
                hit=negative?!Math::less(sy,box.top):Math::less(sy,bottom);
                return true;
            }
            if(!div(a,c,a)||!add(ey,a,ey))return false;
        }
        if(Math::less(sy,box.top)&&Math::less(ey,box.top))return miss();
        if(!Math::less(sy,bottom)&&!Math::less(ey,bottom))return miss();
        hit=true;return true;
    }
};
}
#endif
