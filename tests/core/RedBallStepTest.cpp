#include "core/RedBallStep.h"
#include "core/RunnerDirectionValue.h"
#include "core/GameMakerRandom.h"
using S=downwell::RedBallStep;using N=downwell::MotionBinary64;using F=downwell::RunnerImageValue;
#ifdef DOWNWELL_FREESTANDING
struct Ops {
    bool direction(N,N&);bool imageSpeed(F,F&);bool imageIndex(int);bool sound(int,int,int,int,const char*);
    bool alarm(int,int);bool enemyPlayerY(N&);bool enemyPlayerX(N&);bool pointDirection(N,N,N,N,N&);
    bool inView(bool&);bool lengthdir(bool,N,N,N&);bool meeting(N,N,int,bool&);bool xscale(int);
    bool position(bool,F,F&);bool enemyDeath();bool movingFx(N,N,int,N,N,N);
    bool flash(N,N,int,int,int);bool currency(int);bool destroy();bool sprite(int);
};
bool nativeRedBallStep(S& state,bool stopped,downwell::GameMakerRandom& random,Ops& ops){
    return state.step(stopped,random,ops);
}
#else
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
static S initial(){
    S s;s.initialized=s.alive=s.scriptActive=true;s.health=30;s.money=20;
    s.moveSpeed={0x3fb99999u,0x9999999au};s.maxSpeed={0x3ff33333u,0x33333333u};
    s.acceleration={0x3fa99999u,0x9999999au};s.deceleration={0x3fd00000u,0};
    s.cone=90;s.rotateSpeed=N::fromInteger(2);s.imageRate={0x3fc99999u,0x9999999au};
    s.imageSpeed={0x3e4ccccdu};return s;
}
struct Ops {
    S& state;downwell::GameMakerRandom& random;unsigned reject=0;
    bool visible=true,collide=false;std::vector<int> calls;
    downwell::GameMakerRandom oracle{42};
    Ops(S& s,downwell::GameMakerRandom& r,unsigned fail=0):state(s),random(r),reject(fail){}
    bool accept(int code){calls.push_back(code);return calls.size()!=reject;}
    bool direction(N value,N& accepted){
        if(!accept(2))return false;F f;assert(downwell::RunnerDirectionValue::normalize(value,f));accepted=f.promoted();return true;
    }
    bool imageSpeed(F value,F& accepted){accepted=value;return accept(1);}
    bool imageIndex(int value){assert(!value&&same(state.moveSpeed,N::fromInteger(3)));return accept(3);}
    bool sound(int id,int p,int loop,int gain,const char*){assert((id==125||id==126)&&p==50&&!loop&&gain==1);return accept(id==126?4:20);}
    bool alarm(int number,int value){assert(number==1&&value==5&&state.hitStun);return accept(5);}
    bool enemyPlayerY(N& value){value=N::fromInteger(20);return accept(6);}
    bool enemyPlayerX(N& value){value=N::fromInteger(10);return accept(7);}
    bool pointDirection(N,N,N x,N y,N& value){assert(same(x,N::fromInteger(10))&&same(y,N::fromInteger(20)));value={};return accept(8);}
    bool inView(bool& value){value=visible;return accept(9);}
    bool lengthdir(bool vertical,N,N,N& value){value=N::fromInteger(vertical?2:1);return accept(vertical?11:10);}
    bool meeting(N,N,int object,bool& hit){assert(object==57);hit=collide;return accept(12);}
    bool xscale(int value){assert(value==1);return accept(13);}
    bool position(bool vertical,F value,F& accepted){accepted=value;return accept(vertical?15:14);}
    bool enemyDeath(){assert(!state.alive);return accept(16);}
    bool movingFx(N,N,int sprite,N rate,N angle,N speed){
        assert(sprite==704);N s,a,r,span{0x3fd33333u,0x33333333u};
        assert(span.add({0xbfc99999u,0x9999999au}));
        assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(1),s)&&s.add(N::fromInteger(1)));
        assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(359),a));
        assert(downwell::randomBinary64(oracle.nextU32(),span,r)&&r.add({0x3fc99999u,0x9999999au}));
        assert(same(s,speed)&&same(a,angle)&&same(r,rate));
        random.nextU32();oracle.nextU32(); // Synchronous child Create interleaving.
        return accept(17);
    }
    bool flash(N,N,int a,int b,int c){assert(a==1&&!b&&!c);return accept(18);}
    bool currency(int money){assert(money==20);return accept(19);}
    bool destroy(){return accept(21);}
    bool sprite(int id){assert(id==(state.hitStun?703:702));return accept(22);}
};
int main(){
    const std::vector<int> normal{6,7,8,9,10,11,12,12,12,13,14,15,22};
    for(unsigned fail=0;fail<=normal.size();++fail){
        S s=initial();downwell::GameMakerRandom random(42);Ops ops{s,random,fail};
        assert(s.step(false,random,ops)==!fail&&ops.calls.size()==(fail?fail:normal.size()));
        for(unsigned i=0;i<ops.calls.size();++i)assert(ops.calls[i]==normal[i]);
        assert(random.drawCount()==0);
        if(!fail)assert(same(s.xx,N::fromInteger(1))&&same(s.yy,N::fromInteger(2)));
    }
    for(bool visible:{false,true}){
        S s=initial();const auto before=s.moveSpeed;downwell::GameMakerRandom random(42);Ops ops{s,random};ops.visible=visible;
        assert(s.step(false,random,ops));auto expected=before;if(visible)assert(expected.add(s.acceleration));
        assert(same(s.moveSpeed,expected));
    }
    {
        S s=initial();s.hit=true;s.hitDirection=N::fromInteger(45);downwell::GameMakerRandom random(42);Ops ops{s,random};
        assert(s.step(false,random,ops)&&s.hitStun&&!s.hit&&same(s.direction,N::fromInteger(43)));
        assert((ops.calls==std::vector<int>{2,3,4,5,6,7,8,2,10,11,12,12,12,13,14,15,22}));
        assert(same(s.xx,N::fromInteger(1))); // Hit stun still steers and moves.
    }
    {
        S s=initial();s.scriptActive=false;downwell::GameMakerRandom random(42);Ops ops{s,random};
        assert(s.step(false,random,ops)&&(ops.calls==std::vector<int>{14,15,22}));
    }
    {
        S s=initial();downwell::GameMakerRandom random(42);Ops ops{s,random};
        assert(s.step(true,random,ops)&&(ops.calls==std::vector<int>{1,22})&&s.imageSpeed.bits==0);
    }
    const std::vector<int> death{1,16,17,17,17,17,18,19,20,21,22};
    for(unsigned fail=0;fail<=death.size();++fail){
        S s=initial();s.health=0;downwell::GameMakerRandom random(42);Ops ops{s,random,fail};
        assert(s.step(true,random,ops)==!fail&&ops.calls.size()==(fail?fail:death.size()));
        for(unsigned i=0;i<ops.calls.size();++i)assert(ops.calls[i]==death[i]);
        assert(random.drawCount()==ops.oracle.drawCount()&&random.stateIndex()==ops.oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==ops.oracle.stateWord(i));
    }
    auto normalize=[](double d){d=std::fmod(d,360.);return d<0?d+360:d;};
    {
        S s=initial();downwell::GameMakerRandom random(42);Ops ops{s,random};ops.collide=true;
        assert(s.step(false,random,ops)&&s.xcollision==1&&s.ycollision==1&&s.xsp.zero()&&s.ysp.zero());
        assert(s.xx.zero()&&s.yy.zero()&&same(s.direction,N::fromInteger(180)));
        for(int call:ops.calls)assert(call!=13); // Cleared X speed must not publish a new scale.
    }
    {
        S s=initial();s.direction=N::fromInteger(45);s.xcollision=s.ycollision=1;
        s.xsp=s.ysp=N::fromInteger(1);downwell::GameMakerRandom random(42);Ops ops{s,random,2};
        assert(!s.reflectWalls(ops)&&same(s.direction,N::fromInteger(315))&&s.xsp.zero()&&s.ysp.zero());
    }
    for(int x:{-1,0,1})for(int y:{-1,0,1})for(int angle:{0,45,90,180,270,315,360}){
        S s=initial();s.direction=N::fromInteger(angle);s.xcollision=x;s.ycollision=y;
        s.xsp=s.ysp=N::fromInteger(1);downwell::GameMakerRandom random(42);Ops ops{s,random};
        double expected=angle;
        if(x==1)expected=normalize(expected>180?expected-(expected-270)*2:expected+(90+expected)*2);
        if(x==-1)expected=normalize(expected>180?expected+(270-expected)*2:expected-(expected-90)*2);
        const bool inside=expected>90&&expected<=270;
        if(y==1)expected=normalize(inside?expected-(expected-180)*2:expected+(360-expected)*2);
        if(y==-1)expected=normalize(inside?expected+(180-expected)*2:expected-expected*2);
        assert(s.reflectWalls(ops));N delta=s.direction;assert(delta.add(N::fromInteger(-int(expected)))&&delta.zero());
        assert(s.xsp.zero()==bool(x)&&s.ysp.zero()==bool(y));
    }
    std::puts("Red-ball Step: steering/view guards, hit-stun motion, reflection order, frozen death RNG and failure prefixes passed");
}
#endif
