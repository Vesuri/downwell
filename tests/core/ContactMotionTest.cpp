#include "core/PlayerDamageNumber.h"
#include "core/TableAtan.h"
#include "core/RunnerPointDirection.h"
#include "core/SmokeTrig.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
static bool retained(MotionBinary64 length,MotionBinary64 direction,MotionBinary64& output){
    RunnerImageValue speed,angle,radians;MotionBinary64 cosine;
    return speed.assign(length)&&angle.assign(direction)&&SmokeDirection::radians(angle,radians)&&
        smokeCosine(radians.promoted(),cosine)&&SmokeDirection::component(speed,cosine,false,output);
}
int main(){
    unsigned cases=0;
    for(unsigned q=0;q<=table_atan_detail::pi;q+=199)for(unsigned sign=0;sign<2;++sign){
        auto radians=RunnerImageValue{table_atan_detail::rounded(q<<14)|(sign<<31)}.promoted();
        MotionBinary64 direction;
        assert(RunnerPointDirection::evaluate({}, {},MotionBinary64::fromInteger(1),MotionBinary64::fromInteger(1),direction,
            [&](auto,auto,auto& output){output=radians;return true;}));
        PlayerDamageNumber actual,reference;
        assert(actual.create(MotionBinary64::fromInteger(241),MotionBinary64::fromInteger(777)));
        assert(actual.assignDamage(MotionBinary64::fromInteger(1),direction));
        actual.certifyTableContact();reference=actual;
        unsigned actualCalls=0,referenceCalls=0;
        for(unsigned tick=0;tick<60;++tick){
            actual.advanceAlarm(0);reference.advanceAlarm(0);
            assert(actual.step([&](auto l,auto d,auto& out){++actualCalls;return retained(l,d,out);},
                [](bool,const auto&){return true;},true));
            assert(reference.step([&](auto l,auto d,auto& out){++referenceCalls;return retained(l,d,out);},
                [](bool,const auto&){return true;}));
            assert(actual.x.bits==reference.x.bits&&actual.y.bits==reference.y.bits&&
                same(actual.xSpeed,reference.xSpeed)&&same(actual.ySpeed,reference.ySpeed)&&
                same(actual.direction,reference.direction)&&actual.alarm0==reference.alarm0&&
                actual.destroyRequested==reference.destroyRequested);
        }
        assert(actualCalls==0&&referenceCalls==1);++cases;
    }
    // Certificates cannot bypass arbitrary creation or a changed direction.
    for(unsigned mutation=0;mutation<3;++mutation){
        PlayerDamageNumber effect;assert(effect.create({},{}));
        assert(effect.assignDamage(MotionBinary64::fromInteger(1),MotionBinary64::fromInteger(23)));
        if(mutation){effect.certifyTableContact();
            if(mutation==1)effect.direction=MotionBinary64::fromInteger(24);
            else assert(effect.assignDamage(MotionBinary64::fromInteger(2),effect.direction));}
        unsigned calls=0;
        assert(effect.step([&](auto l,auto d,auto& out){++calls;return retained(l,d,out);},
            [](bool,const auto&){return true;},true));
        assert(calls==1);
    }
    std::printf("Contact motion: %u complete 60-tick trajectories and mutation/fallback guards match retained arithmetic\n",cases);
}
