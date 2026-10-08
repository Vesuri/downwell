#include "core/DitherAboveStep.h"
using namespace downwell;
struct DitherProbe {
    RunnerImageValue x{},positionY{},cameraPositionX{},cameraPositionY{};
    bool low=false,end=false,destroyed=false;
    unsigned calls=0,reject=0,writes=0;
    char events[16]{};
    bool event(char c){events[calls++]=c;return calls!=reject;}
    bool cameraX(MotionBinary64& out){if(!event('A'))return false;out=cameraPositionX.promoted();return true;}
    bool cameraY(MotionBinary64& out){if(!event('B'))return false;out=cameraPositionY.promoted();return true;}
    bool lowSpec(bool& out){if(!event('L'))return false;out=low;return true;}
    bool ending(bool& out){if(!event('E'))return false;out=end;return true;}
    bool y(RunnerImageValue& out){if(!event('r'))return false;out=positionY;return true;}
    bool setX(RunnerImageValue v){if(!event('x'))return false;x=v;return true;}
    bool setY(RunnerImageValue v){if(!event('y'))return false;positionY=v;++writes;return true;}
    bool destroySelf(){if(!event('D'))return false;destroyed=true;return true;}
};
bool nativeDitherAboveProbe(){
    DitherProbe p;
    if(!p.positionY.assignInteger(-80)||!p.cameraPositionX.assignInteger(240)||
        !p.cameraPositionY.assignInteger(192))return false;
    if(!DitherAboveStep::run(p)||p.writes!=2||p.destroyed)return false;
    std::int32_t y;
    return p.positionY.roundPixel(y)&&y==-64&&p.calls==10;
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include "core/GameMakerRounding.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
static RunnerImageValue encoded(float f){RunnerImageValue v;std::memcpy(&v.bits,&f,4);return v;}
int main(){
    assert(nativeDitherAboveProbe());
    unsigned checked=0;
    for(bool low:{false,true})for(bool end:{false,true})
    for(float initial:{-80.5F,-79.5F,-0.5F,0.0F,0.5F,1.5F,1000.25F})
    for(float offset:{-0.25F,0.0F,0.25F,4096.0F}){
        DitherProbe p;p.low=low;p.end=end;p.positionY=encoded(initial);
        p.cameraPositionX=encoded(240.25F);
        const float camera=initial+float(low?208:272)+offset;
        p.cameraPositionY=encoded(camera);
        const bool advance=double(camera)-double(initial)>=double(low?208:272);
        float expected=advance?float(double(initial)+16.0):initial;
        expected=float(roundGameMakerCoordinate(double(expected)));
        assert(DitherAboveStep::run(p)&&p.x.bits==encoded(240.25F).bits&&
            p.positionY.bits==encoded(expected).bits&&p.destroyed==end&&p.writes==(advance?2u:1u));
        assert(!std::strcmp(p.events,advance?(end?"AxLBrryryED":"AxLBrryryE"):(end?"AxLBrryED":"AxLBrryE")));
        ++checked;
    }
    // No catch-up: even a distant camera advances by exactly one 16px step.
    // A rejected operation retains all preceding writes and nothing after it.
    for(unsigned reject=1;reject<=11;++reject){
        DitherProbe p;p.end=true;p.reject=reject;p.positionY=encoded(-80.5F);
        p.cameraPositionX=encoded(241.25F);p.cameraPositionY=encoded(4096.0F);
        assert(!DitherAboveStep::run(p)&&p.calls==reject&&!p.destroyed);
        assert(p.x.bits==(reject<=2?0u:encoded(241.25F).bits));
        const float y=reject<=7?-80.5F:reject<=9?-64.5F:-64.0F;
        assert(p.positionY.bits==encoded(y).bits);
    }
    DitherProbe invalid;invalid.positionY.bits=0x7fc00000u;
    assert(!DitherAboveStep::run(invalid)&&invalid.writes==0);
    std::printf("Dither Above: %u boundary cases, ordered setters and 11 failure prefixes passed\n",checked);
}
#endif
