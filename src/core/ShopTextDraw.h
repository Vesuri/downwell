#ifndef DOWNWELL_CORE_SHOP_TEXT_DRAW_H
#define DOWNWELL_CORE_SHOP_TEXT_DRAW_H
#include "ShopChildCreate.h"
#include "MultiplyBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 212 ordinary Draw. The enclosing layer traversal owns selection;
// operations borrow live font/view state and the original border-text script.
struct ShopTextDraw {
    template<class Ops,class Valid>
    static bool draw(ShopChildState& s,RunnerImageValue px,RunnerImageValue py,Ops& ops,Valid valid){
        if(!ops.widthHashText(s.text,s.textWidth)||!valid())return false;
        s.drawX=px.promoted();MotionBinary64 half,viewX,viewWidth;
        if(!multiplyBinary64(s.textWidth,{0x3fe00000u,0},half))return false;
        auto over=px.promoted();
        if(!over.add(half)||!ops.view(0,viewX)||!valid()||!ops.view(2,viewWidth)||!valid()||!viewX.add(viewWidth))return false;
        viewX.high^=0x80000000u;if(!over.add(viewX))return false;s.overXview=over;
        auto under=px.promoted();half.high^=0x80000000u;
        if(!under.add(half)||!ops.view(0,viewX)||!valid())return false;
        viewX.high^=0x80000000u;if(!under.add(viewX))return false;s.underXview=under;
        int comparison;
        if(!compareRunnerReal(over,MotionBinary64::fromInteger(-10),comparison))return false;
        if(comparison>0){
            over.high^=0x80000000u;
            if(!s.drawX.add(over)||!s.drawX.add(MotionBinary64::fromInteger(-10)))return false;
        }else{
            if(!compareRunnerReal(under,MotionBinary64::fromInteger(10),comparison))return false;
            if(comparison<0){
                under.high^=0x80000000u;
                if(!s.drawX.add(under)||!s.drawX.add(MotionBinary64::fromInteger(10)))return false;
            }
        }
        if(!ops.align(1)||!valid())return false;
        // Source numeric truth is positive, not C++ nonzero truth: -1 hides.
        if(s.drawing>0&&(!ops.borderTextBlack(s.text,s.drawX,py.promoted())||!valid()))return false;
        return ops.align(0)&&valid();
    }
};
}
#endif
