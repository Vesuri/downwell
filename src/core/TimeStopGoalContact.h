#ifndef DOWNWELL_CORE_TIME_STOP_GOAL_CONTACT_H
#define DOWNWELL_CORE_TIME_STOP_GOAL_CONTACT_H
#include "GoalTimeField.h"
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
#include "DivideBinary64.h"
namespace downwell {
// Unrotated sprite 31/91 versus integral-centred, scale-2 goal field.
// Compatibility bounds use nearest-even conversion. Angled masks require a
// different path; callers must not silently discard an instance's rotation.
struct TimeStopGoalContact {
    enum class Mask { Player, Shard };
    static bool query(Mask mask,RunnerImageValue x,RunnerImageValue y,
                      RunnerImageValue xs,RunnerImageValue ys,
                      const GoalTimeField& field,bool& hit){
        if(field.x < -1048576 || field.x > 1048576 ||
           field.y < -1048576 || field.y > 1048576)return false;
        const int top=mask==Mask::Player?8:10, bottom=mask==Mask::Player?20:13;
        Axis horizontal,vertical;
        if(!horizontal.prepare(x,xs,8,16) || !vertical.prepare(y,ys,top,bottom))return false;
        const int l=horizontal.low>field.x-128?horizontal.low:field.x-128;
        const int r=horizontal.high<field.x+128?horizontal.high:field.x+128;
        const int t=vertical.low>field.y-128?vertical.low:field.y-128;
        const int b=vertical.high<field.y+128?vertical.high:field.y+128;
        for(int px=l;px<r;++px){
            bool inside;
            if(!horizontal.contains(px,8,16,inside))return false;
            if(!inside)continue;
            for(int py=t;py<b;++py){
                if(!vertical.contains(py,top,bottom,inside))return false;
                if(!inside)continue;
                const int fx=(px-field.x+128)/2,fy=(py-field.y+128)/2;
                if(fx>=goalMaskRows[fy][0] && fx<goalMaskRows[fy][1]){hit=true;return true;}
            }
        }
        hit=false;return true;
    }
private:
    static RunnerImageValue integer(int n){RunnerImageValue v;v.assign(MotionBinary64::fromInteger(n));return v;}
    static bool valid(RunnerImageValue v){const auto e=(v.bits>>23)&255u;return e!=255 && (e || !(v.bits&0x7fffffffu));}
    static bool add(RunnerImageValue a,RunnerImageValue b,RunnerImageValue& out){
        auto n=a.promoted();return n.add(b.promoted()) && out.assign(n);
    }
    static bool multiply(RunnerImageValue a,RunnerImageValue b,RunnerImageValue& out){
        MotionBinary64 n;return multiplyBinary64(a.promoted(),b.promoted(),n) && out.assign(n);
    }
    struct Axis {
        RunnerImageValue position,inverse;std::int32_t low=0,high=0;
        bool prepare(RunnerImageValue p,RunnerImageValue scale,int first,int end){
            if(!valid(p) || !valid(scale) || !(scale.bits&0x7fffffffu))return false;
            RunnerImageValue start,width,finish;MotionBinary64 reciprocal;
            // Upper edge is lower + scaled width, not position + scaled end.
            if(!multiply(integer(first-12),scale,start) || !add(start,p,start) ||
               !multiply(integer(end-first),scale,width) || !add(width,start,finish) ||
               !start.promoted().roundPixel(low) || !finish.promoted().roundPixel(high) ||
               !divideBinary64(MotionBinary64::fromInteger(1),scale.promoted(),reciprocal) ||
               !inverse.assign(reciprocal))return false;
            if(low>high){const int tmp=low;low=high;high=tmp;}
            position=p;
            return !(inverse.bits&0x80000000u) || add(position,integer(-1),position);
        }
        bool contains(int pixel,int first,int end,bool& hit)const{
            auto negative=position;negative.bits^=0x80000000u;
            RunnerImageValue sample;
            if(!add(integer(pixel),negative,sample) || !multiply(sample,inverse,sample) ||
               !add(sample,integer(12),sample))return false;
            // Stored solid rectangles have positive integral edges. Comparing
            // here is equivalent to cvttss2si followed by their bitmap lookup.
            hit=!(sample.bits&0x80000000u) && sample.bits>=integer(first).bits && sample.bits<integer(end).bits;
            return true;
        }
    };
};
}
#endif
