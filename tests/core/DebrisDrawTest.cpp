#include "core/DebrisDraw.h"
#include "core/CorpseDraw.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
static MotionBinary64 integer(int n){return MotionBinary64::fromInteger(n);}
static Fixed16 q16(int n){return Fixed16::fromInteger(n);}
int main(){
    GameMakerRandom random(42);
    DebrisAnimation effect;
    assert(effect.create(integer(12),integer(24),1,random));
    // No conversion to an integer sprite frame is allowed in draw routing.
    effect.imageIndex.bits=0x40d80000u; // 6.75, including a pre-animation write.
    const EffectSurfacePlacement::View view{q16(10),q16(20),q16(160),q16(284)};
    unsigned cases=0;
    for(unsigned sprite:{91u,115u,116u})for(unsigned quarter=0;quarter<=28;++quarter){
        RunnerImageValue index;auto n=integer(quarter);n.high-=2u<<20; // quarter/4, except zero
        if(!quarter)n={};
        assert(index.assign(n));unsigned frame=99;
        assert(DebrisDraw::frame(sprite,index,frame)&&frame==(quarter/4)%(sprite==91?6:4));
    }
    for(auto bad:{0xbf000000u,0x41000000u,0x7f800000u,0x7fc00000u,1u}){
        unsigned frame=99;assert(!DebrisDraw::frame(115,{bad},frame)&&frame==99);
    }
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
        CorpseAnimation corpse;corpse.x=effect.x;corpse.y=effect.y;corpse.flash=flash;
        corpse.destroyRequested=destroyed;corpse.spriteId=206;corpse.imageIndex={0x3f400000u};
        corpse.imageXscale={0xbf800000u};CorpseDraw::Command body;
        assert(CorpseDraw::prepare(corpse,view,death,lowSpec,exists,body));
        assert(body.placement.target==command.placement.target);
        if(body.placement.target!=EffectSurfacePlacement::hidden){
            assert(body.sprite==206&&body.imageIndex.bits==0x3f400000u&&body.scaleX.bits==0xbf800000u);
            assert(body.placement.x==command.placement.x&&body.placement.y==command.placement.y);
        }
        const bool draw=!destroyed&&flash<=0;
        const auto target=!draw?EffectSurfacePlacement::hidden:
            death||lowSpec?EffectSurfacePlacement::direct:
            exists&&x>10&&x<170&&y>20&&y<304?EffectSurfacePlacement::effects:
            EffectSurfacePlacement::hidden;
        assert(command.placement.target==target);
        if(target!=EffectSurfacePlacement::hidden){
            assert(command.sprite==unsigned(effect.spriteId)&&command.imageIndex.bits==0x40d80000u);
            const auto px=q16(target==EffectSurfacePlacement::direct?x:x-10);
            const auto py=q16(target==EffectSurfacePlacement::direct?y:y-20);
            assert(command.placement.x==px&&command.placement.y==py);
        }else assert(command.sprite==0&&command.imageIndex.bits==0);
        assert(random.drawCount()==before.drawCount());
        ++cases;
    }
    // Failed arithmetic must not publish a partially changed command.
    effect.destroyRequested=false;effect.flash=-1;
    const auto savedX=effect.x;effect.x.bits=0x7fc00000u; // NaN has no Q16 value.
    DebrisDraw::Command unchanged;unchanged.sprite=123;
    assert(!DebrisDraw::prepare(effect,view,false,false,true,unchanged));
    effect.x=savedX;
    assert(unchanged.sprite==123);
    assert(DebrisDraw::sourceDepth==-1000);
    assert(CorpseDraw::sourceDepth==1000);
    CorpseAnimation invalidCorpse;invalidCorpse.valid=false;
    CorpseDraw::Command preserved;preserved.sprite=123;
    assert(!CorpseDraw::prepare(invalidCorpse,view,false,false,true,preserved)&&preserved.sprite==123);
    effect.x.bits=0x41440000u;effect.y.bits=0x41c40000u; // 12.25, 24.5
    assert(DebrisDraw::prepare(effect,view,false,false,true,unchanged));
    assert(unchanged.placement.x==Fixed16::raw(147456)); // 2.25
    assert(unchanged.placement.y==Fixed16::raw(294912)); // 4.5
    assert(DebrisDraw::prepare(effect,view,true,false,false,unchanged));
    assert(unchanged.placement.x==Fixed16::raw(802816)); // 12.25
    assert(unchanged.placement.y==Fixed16::raw(1605632)); // 24.5
    std::printf("Debris Draw: %u visibility/route/flash cases retain source subimage and coordinates\n",cases);
}
