#include "core/WhiteJellyStep.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <vector>
#include <cstdio>
using S=downwell::WhiteJellyStep;
using N=downwell::MotionBinary64;
using F=downwell::RunnerImageValue;
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
struct Ops {
    S& s;downwell::GameMakerRandom& rng;
    std::vector<int> calls;unsigned reject=0;bool collide=false;
    downwell::GameMakerRandom oracle{42};
    bool call(int id){calls.push_back(id);return calls.size()!=reject;}
    bool direction(N in,N& out){out=in;return call(1);}
    bool imageSpeed(F in,F& out){out=in;return call(2);}
    bool imageIndex(int n){assert(n==0&&s.moveSpeed.high==N::fromInteger(3).high);return call(3);}
    bool sound(int id,int p,int z,int one,const char*){assert(p==50&&z==0&&one==1);return call(id);}
    bool alarm(int a,int n){assert(a==1&&n==5&&s.hitStun);return call(4);}
    bool enemyPlayerY(N& out){out=N::fromInteger(20);return call(5);}
    bool enemyPlayerX(N& out){out=N::fromInteger(10);return call(6);}
    bool pointDirection(N,N,N x,N y,N& out){assert(same(x,N::fromInteger(10))&&same(y,N::fromInteger(20)));out={};return call(7);}
    bool lengthdir(bool y,N,N,N& out){out=N::fromInteger(y?(collide?-2:2):1);return call(y?9:8);}
    bool meeting(N,N,int target,bool& hit){assert(target==57);hit=collide;return call(10);}
    bool position(bool y,F in,F& out){out=in;return call(y?12:11);}
    bool enemyDeath(){assert(!s.alive);return call(13);}
    bool smoke(N,N,int a,int b){assert(a==1&&b==0);return call(14);}
    bool movingFx(N,N,int sprite,N rate,N angle,N speed){
        assert(sprite==704&&rate.valid()&&angle.valid()&&speed.valid());
        N es,ea,er,span{0x3fd33333u,0x33333333u};
        assert(span.add({0xbfc99999u,0x9999999au}));
        assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(1),es)&&es.add(N::fromInteger(1)));
        assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(359),ea));
        assert(downwell::randomBinary64(oracle.nextU32(),span,er)&&er.add({0x3fc99999u,0x9999999au}));
        assert(same(speed,es)&&same(angle,ea)&&same(rate,er));
        oracle.nextU32();
        // Child Create consumes RNG synchronously, before the next iteration.
        rng.nextU32();return call(15);
    }
    bool flash(N,N,int a,int b,int c){assert(a==1&&b==0&&c==0);return call(16);}
    bool currency(int money){assert(money==6);return call(17);}
    bool destroy(){return call(18);}
    bool sprite(int id){assert(id==(s.hitStun?175:176));return call(19);}
};
static S initial(){
    S s;s.initialized=s.alive=s.scriptActive=true;s.health=25;s.money=6;s.cone=10;
    s.imageRate=s.moveSpeed=s.maxSpeed={0x3fc99999u,0x9999999au};
    assert(s.imageSpeed.assign(s.imageRate));
    s.acceleration={0x3fa99999u,0x9999999au};s.rotateSpeed={0x3ff80000u,0};
    return s;
}
int main(){
    for(int mode=0;mode<5;++mode){
        auto baseline=initial();
        if(mode==1)baseline.hit=true;
        if(mode==2)baseline.scriptActive=false;
        if(mode==3)baseline.health=0;
        const bool stopped=mode==3;
        downwell::GameMakerRandom r(42);Ops success{baseline,r,{},0,mode==4};
        assert(baseline.step(stopped,r,success));
        const std::vector<int> expected=mode==1?std::vector<int>{1,3,118,4,11,12,19}:
            mode==2?std::vector<int>{11,12,19}:
            mode==3?std::vector<int>{2,13,14,15,15,15,15,16,17,117,18,19}:
            mode==4?std::vector<int>{5,6,7,8,9,10,10,10,10,1,11,12,19}:
                    std::vector<int>{5,6,7,8,9,10,10,10,11,12,19};
        assert(success.calls==expected);
        if(mode==0){assert(same(baseline.xx,N::fromInteger(1))&&same(baseline.yy,N::fromInteger(2)));}
        if(mode==4){assert(baseline.xx.zero()&&baseline.yy.zero()&&same(baseline.direction,N::fromInteger(270)));}
        if(mode==3){
            downwell::GameMakerRandom oracle(42);
            for(unsigned i=0;i<16;++i)oracle.nextU32();
            assert(r.drawCount()==16&&r.stateIndex()==oracle.stateIndex());
            for(unsigned i=0;i<16;++i)assert(r.stateWord(i)==oracle.stateWord(i));
        }else assert(r.drawCount()==0);
        for(unsigned fail=1;fail<=expected.size();++fail){
            auto s=initial();if(mode==1)s.hit=true;if(mode==2)s.scriptActive=false;if(mode==3)s.health=0;
            downwell::GameMakerRandom rng(42);Ops ops{s,rng,{},fail,mode==4};
            assert(!s.step(stopped,rng,ops)&&ops.calls.size()==fail);
            for(unsigned i=0;i<fail;++i)assert(ops.calls[i]==expected[i]);
            if(mode==1){assert(!s.hit);if(fail>=5)assert(s.hitStun);}
        }
    }
    auto s=initial();downwell::GameMakerRandom rng(1);Ops ops{s,rng,{}};
    assert(s.step(true,rng,ops)&&ops.calls==std::vector<int>({2,19})&&s.xx.zero());
    std::puts("White jelly Step: motion, hit stun, wall response, time-stop death and failure ordering passed");
}
