#include "core/DebrisDraw.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
static MotionBinary64 integer(int n){return MotionBinary64::fromInteger(n);}
int main(){
    GameMakerRandom random(42);
    DebrisAnimation effect;
    assert(effect.create(integer(12),integer(24),1,random));
    // No conversion to an integer sprite frame is allowed in draw routing.
    effect.imageIndex.bits=0x40d80000u; // 6.75, including a pre-animation write.
    const EffectSurfacePlacement::View view{integer(10),integer(20),integer(160),integer(284)};
    unsigned cases=0;
    for(int area:{1,2,4})for(int x:{9,10,11,169,170,171})
    for(int y:{19,20,21,303,304,305})for(unsigned flags=0;flags<8;++flags)
    for(int flash:{-1,0,1})for(bool destroyed:{false,true}){
        effect.spriteId=area==1?115:area==2?116:91;
        assert(effect.x.assign(integer(x))&&effect.y.assign(integer(y)));
        effect.flash=flash;effect.destroyRequested=destroyed;
        const bool death=flags&1,lowSpec=flags&2,exists=flags&4;
        const auto before=random;
        DebrisDraw::Command command;
        assert(DebrisDraw::prepare(effect,view,death,lowSpec,exists,command));
        const bool draw=!destroyed&&flash<=0;
        const auto target=!draw?EffectSurfacePlacement::hidden:
            death||lowSpec?EffectSurfacePlacement::direct:
            exists&&x>10&&x<170&&y>20&&y<304?EffectSurfacePlacement::effects:
            EffectSurfacePlacement::hidden;
        assert(command.placement.target==target);
        if(target!=EffectSurfacePlacement::hidden){
            assert(command.sprite==unsigned(effect.spriteId)&&command.imageIndex.bits==0x40d80000u);
            const auto px=integer(target==EffectSurfacePlacement::direct?x:x-10);
            const auto py=integer(target==EffectSurfacePlacement::direct?y:y-20);
            assert(command.placement.x.high==px.high&&command.placement.x.low==px.low);
            assert(command.placement.y.high==py.high&&command.placement.y.low==py.low);
        }else assert(command.sprite==0&&command.imageIndex.bits==0);
        assert(random.drawCount()==before.drawCount());
        ++cases;
    }
    // Failed arithmetic must not publish a partially changed command.
    effect.destroyRequested=false;effect.flash=-1;
    auto invalid=view;invalid.width={0x7ff00000u,0};
    DebrisDraw::Command unchanged;unchanged.sprite=123;
    assert(!DebrisDraw::prepare(effect,invalid,false,false,true,unchanged));
    assert(unchanged.sprite==123);
    assert(DebrisDraw::sourceDepth==-1000);
    effect.x.bits=0x41440000u;effect.y.bits=0x41c40000u; // 12.25, 24.5
    assert(DebrisDraw::prepare(effect,view,false,false,true,unchanged));
    assert(unchanged.placement.x.high==0x40020000u&&unchanged.placement.x.low==0); // 2.25
    assert(unchanged.placement.y.high==0x40120000u&&unchanged.placement.y.low==0); // 4.5
    assert(DebrisDraw::prepare(effect,view,true,false,false,unchanged));
    assert(unchanged.placement.x.high==0x40288000u&&unchanged.placement.x.low==0);
    assert(unchanged.placement.y.high==0x40388000u&&unchanged.placement.y.low==0);
    std::printf("Debris Draw: %u visibility/route/flash cases retain source subimage and coordinates\n",cases);
}
