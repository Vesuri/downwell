#ifndef DOWNWELL_TEST_SHOP_TEXT_DRAW_CHECKS_H
#define DOWNWELL_TEST_SHOP_TEXT_DRAW_CHECKS_H
#include "core/ShopTextDraw.h"
struct ShopTextDrawProbe {
    unsigned reject=0,calls=0,draws=0;bool good=true;
    int width=41,viewX=480,expectedTwiceX=1021,expectedY=1982;
    bool event(){return ++calls!=reject;}
    bool widthHashText(const char* text,downwell::MotionBinary64& out){
        good=good&&text&&calls==0;out=downwell::MotionBinary64::fromInteger(width);return event();
    }
    bool view(unsigned axis,downwell::MotionBinary64& out){
        good=good&&calls>=1&&calls<=3&&axis==(calls==2?2u:0u);
        out=downwell::MotionBinary64::fromInteger(axis==2?160:viewX);return event();
    }
    bool align(unsigned value){good=good&&value==(calls==4?1u:0u);return event();}
    bool borderTextBlack(const char*,downwell::MotionBinary64 x,downwell::MotionBinary64 y){
        std::int32_t px,py;++draws;
        good=good&&calls==5&&x.add(x)&&x.roundPixel(px)&&y.roundPixel(py)&&
            px==expectedTwiceX&&py==expectedY;return event();
    }
};
inline bool shopTextDrawPrefixes(){
    using namespace downwell;
    for(unsigned reject=0;reject<=7;++reject){
        ShopChildState state;state.text="test";state.drawing=1;
        RunnerImageValue x,y;if(!x.assignInteger(500)||!y.assignInteger(1982))return false;
        ShopTextDrawProbe ops;ops.reject=reject;
        if(ShopTextDraw::draw(state,x,y,ops,[]{return true;})!=!reject||!ops.good||
            ops.calls!=(reject?reject:7))return false;
    }
    return true;
}
#endif
