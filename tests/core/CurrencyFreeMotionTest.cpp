#include "core/CurrencyFreeMotion.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {unsigned(b>>32),unsigned(b)};}
static bool equal(MotionBinary64 a,double d){auto b=real(d);return a.high==b.high&&a.low==b.low;}
struct Ops {
    unsigned mode,fail=0,calls=0,sounds=0,directions=0;
    bool next(){return ++calls!=fail;}
    bool collision(int object,CurrencyCreate& s){
        if(!next())return false;s.checkTarget=object;
        s.xCollision=object==57&&bool(mode&2);s.yCollision=object==87?bool(mode&1):bool(mode&4);return true;
    }
    bool meeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){assert(object==87&&equal(x,12.25)&&equal(y,-3.5));hit=false;return next();}
    bool viewTop(MotionBinary64& top){top=real(-100);return next();}
    bool sound(int,int priority,bool loop,bool cut,int& voice){assert(priority==30&&!loop&&cut);if(!next())return false;voice=700+int(++sounds);return true;}
    bool pointDirection(MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64& out){if(!next())return false;out=real(++directions==1?90:180);return true;}
    bool water(MotionBinary64 x,MotionBinary64 y,bool& hit){assert(equal(x,12.25)&&equal(y,-3.5));hit=mode&8;return next();}
};
int main(){
    unsigned cases=0,prefixes=0;
    for(unsigned seed=0;seed<32;++seed)for(unsigned mode=0;mode<256;++mode){
        CurrencyCreate initial;initial.xx=real(12.25);initial.yy=real(-3.5);initial.x.assign(initial.xx);initial.y.assign(initial.yy);
        const double initialY=mode&128?3.99:mode&64?4.5:mode&32?-1:1;
        initial.xsp=real(.02);initial.ysp=real(initialY);initial.suckable=true;
        const double px=mode&16?12.25:999,py=-3.5;
        GameMakerRandom random(seed),oracle(seed);CurrencyCreate s=initial;Ops ops{mode};bool good=true;
        double vx=.02,vy=initialY<4?initialY+.035:4;unsigned sounds=0;
        auto bounce=[&]{if(vy<=.00001)vy*=0;else vy=int(oracle.irandomSmall(1))-1;vx*=.7;++sounds;};
        if(mode&1)bounce();if(mode&2){vx*=-1;vx*=.7;}if(mode&4)bounce();
        if(std::abs(vx)-.05<-.00001)vx=(vx<0?-1:vx>0?1:0)*.05;
        if(mode&8){vx*=.99;vy*=.99;}
        assert(CurrencyFreeMotion::run(s,real(px),real(py),random,ops,good)&&good);
        assert(equal(s.xsp,vx)&&equal(s.ysp,vy)&&equal(s.xx,12.25+vx)&&equal(s.yy,-3.5+vy)&&
            s.attracted==bool(mode&16)&&equal(s.direction,mode&16?180:90)&&ops.sounds==sounds);
        if(mode&16)assert(equal(s.gemSpeed,4)&&equal(s.rotationSpeed,32));
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        for(unsigned fail=1;fail<=ops.calls;++fail){
            CurrencyCreate partial=initial;GameMakerRandom rng(seed);Ops rejected{mode,fail};bool ok=true;
            assert(!CurrencyFreeMotion::run(partial,real(px),real(py),rng,rejected,ok)&&!ok&&rejected.calls==fail);
            const auto draws=rng.drawCount();assert(!CurrencyFreeMotion::run(partial,real(px),real(py),rng,rejected,ok)&&rejected.calls==fail&&rng.drawCount()==draws);++prefixes;
        }
        ++cases;
    }
    std::printf("Currency free motion: %u collision/drag/attraction/RNG cases and %u failure prefixes passed\n",cases,prefixes);
}
