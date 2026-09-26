#include "core/GameplayWorld.h"
#include "core/StartupRuntime.h"
#include "core/RoomPack.h"
#include "core/BuilderPatternPack.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using namespace downwell;
static auto n(int v){return MotionBinary64::fromInteger(v);}
static std::vector<unsigned char> load(const char* name){std::ifstream f(name,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static bool equal(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
static PlayerDamageNumber snapshot(const PlayerDamageNumbers<8>& owner){
    PlayerDamageNumber result;unsigned count=0;
    assert(owner.visitNewest([&](auto,const auto& e){result=e;++count;return true;}));assert(count==1);return result;
}
int main(){
    {
        RunSession session;
        assert(session.contactDamageNumber(128,-128,0,0,240,176));
        auto effect=snapshot(session.damageNumbers);
        assert(effect.x.bits==0x43708000u && effect.y.bits==0x432b8000u); // 240.5, 171.5
        assert(equal(effect.direction,n(45)) && equal(effect.amount,n(1)));
    }
    {
        GameplayWorld encounter(0,true,0,false);
        assert(encounter.configureEnemy(0,116,153,1,0,0,0,15,0));
        encounter.step(0);
        assert(encounter.health()==3 && encounter.effectsSucceeded());
        auto effect=snapshot(encounter.damageNumbers());
        assert(effect.alarm0==60 && equal(effect.xSpeed,n(999)));
        assert(effect.x.bits==0x43700000u && effect.y.bits==0x432c0000u); // 240,172
        encounter.step(0);
        assert(encounter.health()==3 && encounter.damageNumbers().size()==1);
        assert(encounter.damageNumbers().atNewest(0)->alarm0==59);
    }
    {
        GameplayWorld encounter(0,true,0,false);
        for(unsigned i=0;i<8;++i)assert(encounter.emitDamageNumber(n(240),n(176),n(1),{}));
        assert(encounter.configureEnemy(0,116,153,1,0,0,0,15,0));
        encounter.step(0);
        assert(encounter.health()==3 && !encounter.effectsSucceeded());
        assert(encounter.damageNumbers().size()==8);
    }
    GameplayWorld target(0,true,0,false);bool born=false;
    target.configureGenerationStep([](void* context,GameplayWorld& world,GameMakerRandom&,int){
        auto& born=*static_cast<bool*>(context);
        if(born)return true;born=true;
        return world.emitDamageNumber(n(240),n(1250),n(1),n(45));
    },&born);
    target.step(0);assert(born&&target.effectsSucceeded());
    auto birth=snapshot(target.damageNumbers());
    assert(equal(birth.xSpeed,n(999))&&birth.alarm0==60&&birth.y.bits==0x449bc000u); // 1246
    auto rooms=load("generated/downwell-rooms.dwrm"),objects=load("generated/downwell-objects.dwob"),patterns=load("generated/downwell-builder-patterns.dwbp");
    RoomPack roomPack(rooms.data(),rooms.size());ObjectPack objectPack(objects.data(),objects.size());
    BuilderPatternPack builderPack(patterns.data(),patterns.size());
    assert(roomPack.valid()&&objectPack.valid()&&builderPack.valid());
    StartupRuntime host(PlatformProfile::desktop,&roomPack,&objectPack,84,&builderPack);
    host.skipToGameForTesting();assert(host.inMainRoom()&&host.roomLoadSucceeded());
    {
        StartupRuntime encounter(PlatformProfile::desktop,&roomPack,&objectPack,84,&builderPack);
        encounter.skipToGameForTesting();
        bool hit=false;
        for(unsigned tick=0;tick<6000&&!hit;++tick){
            Input input{};
            input.right=(tick/90)%2==0;input.left=!input.right;input.fire=tick%10<5;
            if(encounter.mainPlayerY()<1264){
                input.left=encounter.mainPlayerX()>242;input.right=encounter.mainPlayerX()<238;
            }
            const int health=encounter.mainPlayerHealth();
            encounter.step(input,false);
            assert(encounter.effectsSucceeded());
            if(encounter.mainPlayerHealth()<health){
                auto effect=snapshot(encounter.damageNumbers());
                assert(equal(effect.amount,n(health-encounter.mainPlayerHealth())));
                assert(effect.alarm0==60 && equal(effect.xSpeed,n(999)));
                hit=true;
            }
        }
        assert(hit);
    }
    const auto draws=host.randomDrawCount();
    assert(host.emitDamageNumber(n(240),n(1250),n(1),n(45))&&host.randomDrawCount()==draws);
    auto source=snapshot(host.damageNumbers());assert(source.x.bits==birth.x.bits&&source.y.bits==birth.y.bits);
    for(unsigned tick=0;tick<60;++tick){
        if(tick==10){assert(host.emitCorpseExpiry({},{}));assert(target.emitCorpseExpiry({},{}));}
        host.step(false);target.step(0);
        assert(host.effectsSucceeded()&&target.effectsSucceeded());
        assert(host.damageNumbers().size()==target.damageNumbers().size());
        std::vector<unsigned> hostOrder,targetOrder;
        assert(host.visitForegroundEffects([&](const auto&){hostOrder.push_back(1);return true;},
            [&](const auto&){hostOrder.push_back(2);return true;}));
        assert(target.visitForegroundEffects([&](const auto&){targetOrder.push_back(1);return true;},
            [&](const auto&){targetOrder.push_back(2);return true;}));
        assert(hostOrder==targetOrder);
        assert(hostOrder==(tick==59?std::vector<unsigned>{}:tick>=10&&tick<49?
            std::vector<unsigned>{1,2}:std::vector<unsigned>{2}));
        if(tick==59){assert(host.damageNumbers().size()==0);break;}
        auto a=snapshot(host.damageNumbers()),b=snapshot(target.damageNumbers());
        assert(a.alarm0==59-int(tick)&&a.alarm0==b.alarm0);
        assert(a.x.bits==b.x.bits&&a.y.bits==b.y.bits&&equal(a.xSpeed,b.xSpeed)&&equal(a.ySpeed,b.ySpeed));
        assert(!equal(a.xSpeed,n(999)));
    }
    for(unsigned i=0;i<8;++i)assert(host.emitDamageNumber(n(240),n(1250),n(1),{}));
    assert(!host.emitDamageNumber({}, {},n(1),{})&&!host.effectsSucceeded());
    host.reset(PlatformProfile::desktop);assert(host.effectsSucceeded()&&host.damageNumbers().size()==0);
    assert(target.emitDamageNumber({}, {},n(1),{})&&target.emitCorpseExpiry({},{}));
    const auto oldNumber=target.damageNumbers().atNewest(0)->drawBirth;
    const auto oldExpiry=target.corpseExpiryEffects().atNewest(0)->drawBirth;
    unsigned oldKey=0;
    assert(target.damageNumbers().visitNewest([&](auto key,const auto&){oldKey=key;return true;}));
    target.restartFromResults(DeathMenuSequence::Action::Retry,84,0);
    assert(target.effectsSucceeded()&&target.damageNumbers().size()==0&&target.corpseExpiryEffects().size()==0);
    assert(target.emitDamageNumber({}, {},n(1),{})&&target.emitCorpseExpiry({},{}));
    assert(target.damageNumbers().atNewest(0)->drawBirth>oldNumber&&target.corpseExpiryEffects().atNewest(0)->drawBirth>oldExpiry);
    assert(target.damageNumbers().visitNewest([&](auto key,const auto&){assert(key>oldKey);return true;}));
    std::puts("Damage-number worlds: natural SDL damage, native-world contact births/immunity/capacity, phase eligibility, 60 matching ticks and reset passed");
}
