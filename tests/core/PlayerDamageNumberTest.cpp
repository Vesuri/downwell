#include "core/PlayerDamageNumber.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace downwell;
static MotionBinary64 number(double v){std::uint64_t bits;std::memcpy(&bits,&v,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};}
static std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
static bool equal(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct Draw {
    struct Text {MotionBinary64 x,y,amount;int color;};
    std::vector<int> events;std::vector<Text> texts;
    int currentColor=99,currentAlign=99,fail=0;
    void align(int v){currentAlign=v;events.push_back(10+v);}
    void color(int v){currentColor=v;events.push_back(v);}
    bool text(MotionBinary64 x,MotionBinary64 y,MotionBinary64 amount){
        assert(currentAlign==1);events.push_back(1000);texts.push_back({x,y,amount,currentColor});
        return int(texts.size())!=fail;
    }
};
int main(){
    for(unsigned sample=0;sample<256;++sample){
        PlayerDamageNumber effect;
        double px=240.125+double(sample)/16,py=-96.375+double(sample)*4;
        assert(effect.create(number(px),number(py)));
        assert(equal(effect.amount,{})&&equal(effect.direction,{})&&equal(effect.xSpeed,number(999))&&effect.alarm0==60);
        assert(effect.assignDamage(number(1+sample%4),number(sample)));
        float x=float(px),y=float(py);
        double vx=(int(sample)-128)/64.0,vy=3;
        unsigned calls=0,publications=0;
        auto lengthdir=[&](auto length,auto angle,auto& out){
            ++calls;assert(equal(length,number(2))&&equal(angle,number(sample)));out=number(vx);return true;
        };
        for(unsigned tick=0;tick<59;++tick){
            effect.advanceAlarm(1);assert(effect.alarm0==60-int(tick));
            effect.advanceAlarm(0);assert(!effect.destroyRequested);
            const float beforeX=x;
            y=float(double(y)-vy);x=float(double(x)+vx);
            assert(effect.step(lengthdir,[&](bool horizontal,const auto& state){
                assert(horizontal==bool(publications&1));++publications;
                assert(state.y.bits==bits(y)&&state.x.bits==bits(horizontal?x:beforeX));
                assert(equal(state.ySpeed,number(vy))&&equal(state.xSpeed,number(vx)));
                return true;
            }));
            vy*=0.8;vx*=0.8;
            assert(effect.x.bits==bits(x)&&effect.y.bits==bits(y));
            assert(equal(effect.xSpeed,number(vx))&&equal(effect.ySpeed,number(vy)));
        }
        assert(calls==1&&publications==118);
        Draw draw;
        assert(effect.draw(draw)&&draw.texts.size()==5&&draw.currentAlign==0&&draw.currentColor==255);
        assert((draw.events==std::vector<int>{11,0,1000,1000,1000,1000,255,1000,10}));
        for(unsigned i=0;i<5;++i){
            const auto& t=draw.texts[i];
            assert(equal(t.x,number(double(x)+(i==0?1:i==1?-1:0))));
            assert(equal(t.y,number(double(y)+(i==2?1:i==3?-1:0))));
            assert(equal(t.amount,effect.amount)&&t.color==(i==4?255:0));
        }
        effect.advanceAlarm(0);assert(effect.destroyRequested&&effect.alarm0==0);
        assert(effect.step(lengthdir,[&](bool,const auto&){assert(false);return false;}));
        Draw retired;assert(effect.draw(retired)&&retired.events.empty()&&calls==1);
    }
    {
        PlayerDamageNumber effect;assert(effect.create(number(0.25),number(0.5)));
        effect.alarm0=0;effect.advanceAlarm(0);assert(effect.alarm0==-1&&!effect.destroyRequested);
        effect.advanceAlarm(0);assert(effect.alarm0==-1);
        unsigned calls=0;
        assert(!effect.step([&](auto,auto,auto&){++calls;return false;},[](bool,const auto&){assert(false);return false;}));
        assert(!effect.valid&&calls==1&&effect.x.bits==bits(0.25F)&&effect.y.bits==bits(0.5F));
        assert(!effect.step([&](auto,auto,auto&){assert(false);return false;},[](bool,const auto&){return true;}));
    }
    for(bool failX:{false,true}){
        PlayerDamageNumber effect;assert(effect.create(number(10.25),number(20.5)));
        unsigned calls=0;
        assert(!effect.step([](auto,auto,auto& out){out=number(2);return true;},[&](bool x,const auto&){++calls;return x!=failX;}));
        assert(!effect.valid&&calls==unsigned(failX?2:1));
        assert(effect.y.bits==bits(17.5F)&&effect.x.bits==bits(failX?12.25F:10.25F));
        assert(equal(effect.ySpeed,number(3))&&equal(effect.xSpeed,number(2))); // No damping after failure.
    }
    for(int failure=1;failure<=5;++failure){
        PlayerDamageNumber effect;assert(effect.create(number(0),number(0)));
        Draw draw;draw.fail=failure;assert(!effect.draw(draw)&&int(draw.texts.size())==failure&&draw.currentAlign==1);
    }
    PlayerDamageNumber bad;
    assert(!bad.create({0x7ff00000u,0},{}));
    assert(bad.create({},{}));assert(!bad.assignDamage({}, {0x7ff00000u,0}));
    std::puts("Player damage numbers: 15104 binary32-position/binary64-velocity steps, publication order, alarm lifetime, outline commands and failures passed");
}
