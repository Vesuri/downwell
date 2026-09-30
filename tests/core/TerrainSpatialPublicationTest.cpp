#include "core/GameplayTerrainInstancesRuns.h"
#include "core/RunnerSpatialStates.h"
#include "core/RunnerSpatialInstances.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace downwell;
static std::vector<unsigned char> read(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {std::istreambuf_iterator<char>(file),{}};
}
int main(){
    // The plan replaces repeated runtime decisions, not their semantics.
    // Cover every relevant instance/object flag combination, including
    // inactive/precise/default membership and dirty-bit changes in Create.
    using Oracle=RunnerSpatialDirtyQueue<4>;
    using PlanOwner=RunnerSpatialInstances<4,16>;
    const unsigned bits[]={1u,2u,8u,0x20u,0x200u,0x8000u,0x100000u};
    for(unsigned mask=0;mask<128;++mask)for(unsigned objectFlags=0;objectFlags<128;++objectFlags)
        for(bool inactive:{false,true})for(bool precise:{false,true}){
            PreparedObjectDefault d;d.initialFlags=0;
            for(unsigned b=0;b<7;++b)if(mask&(1u<<b))d.initialFlags|=bits[b];
            const auto plan=PlanOwner::generatedTerrainContinuation(d,objectFlags,inactive);
            const unsigned initial=d.initialFlags|(inactive?2u:0u);
            const unsigned bounded=(initial&~0x208u)|(precise?0x200u:0u);
            assert(plan.queue==Oracle::shouldQueue(true,objectFlags,initial));
            assert(plan.queue==Oracle::shouldQueue(true,objectFlags,bounded|8u));
            assert(plan.publish==Oracle::shouldPublishBounds(objectFlags,bounded,false));
            assert(plan.collision==bool(objectFlags&8u));
        }
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    PreparedObjectDefaults<386> defaults;assert(defaults.prepare(objects,sprites));
    PreparedTerrainBirthBounds geometry;assert(geometry.prepare(defaults,sprites));
    using Terrain=GameplayTerrainInstancesRuns;
    using States=RunnerSpatialStates<RunnerSpatialState,2050,1,2049,Terrain>;
    unsigned compared=0;
    for(int object:{60,93,99})for(bool active:{false,true})for(unsigned stop=1;stop<=6;++stop){
        Terrain terrain;States states;assert(states.bind(terrain));states.geometry(geometry);states.prepare(defaults);
        const auto* d=defaults.get(object);assert(d);
        for(unsigned i=0;i<4;++i){
            const unsigned last=i==3?stop:6;
            assert(terrain.appendInitializedCell(object,64-int(i)*16,256,active,
                [&](InstanceHandle h,auto&,bool&,bool){
                    const unsigned slot=h.slot+1;
                    assert(states.read(slot)->same(RunnerSpatialState{}));
                    terrain.spatialPublish(h.slot);
                    RunnerSpatialState expected;bool precise;
                    RunnerSpatialSplit::Box bounds;
                    assert(geometry.at(object,64-int(i)*16,256,expected.x,expected.y,bounds,precise));
                    expected.previous={expected.x,expected.y};expected.flags=8u|(active?0u:2u);
                    states.freshGenerated(slot,!active);
                    assert(states.read(slot)->same(expected));++compared;
                    for(unsigned stage=2;stage<=last;++stage){
                        states.stageGenerated(slot,stage);
                        expected.flags=d->initialFlags|(active?0u:2u);
                        if(stage>=3){expected.bounds=bounds;expected.flags=(expected.flags&~0x208u)|(precise?0x200u:0u);}
                        if(stage==4||stage==6)expected.flags|=0x8000u;
                        if(stage==5||stage==6)expected.flags|=8u;
                        assert(states.read(slot)->same(expected)&&states.flags(slot)==expected.flags);++compared;
                    }
                    for(unsigned j=0;j<i;++j){const auto sibling=states.read(j+1);
                        std::int32_t x;assert(sibling->x.roundPixel(x)&&x==64-int(j)*16);
                        assert((sibling->flags&0x8008u)==0x8008u);
                    }
                    return true;
                }));
        }
        // Publication stages alone must not fragment an ordinary terrain run.
        assert(terrain.ownershipRunCount()==1);
        const auto first=states.read(1),second=states.read(2);
        {
            auto outer=states.edit(1);auto& s=outer.get();
            s.x.bits=0x42f68000u;s.previous.y.bits=0x80000000u;
            const auto moved=states.read(1);assert(moved->x.bits==s.x.bits);
            {
                auto nested=states.edit(1);nested.get().flags^=2u;
                assert(states.flags(1)==nested.get().flags&&outer.get().flags==nested.get().flags);
                auto other=states.edit(2);other.get().bounds.left.bits=0x3f400000u;
                assert(states.read(1)->x.bits==s.x.bits&&states.read(2)->bounds.left.bits==0x3f400000u);
            }
            assert(states.read(2)->bounds.left.bits==0x3f400000u);
        }
        assert(first->x.bits!=states.read(1)->x.bits&&second->bounds.left.bits!=states.read(2)->bounds.left.bits);
        assert(states.read(1)->previous.y.bits==0x80000000u);
        assert(states.capture(1)&&states.read(1)->previous.x.bits==states.read(1)->x.bits);
        assert(terrain.ownershipRunCount()==3&&terrain.valid()&&states.valid());
        // Full actor storage remains borrowed and independently mutable.
        for(unsigned slot:{0u,2049u}){
            const auto actor=states.read(slot);
            auto edit=states.editDomain<false>(slot);auto& s=edit.get();
            s.flags=123;s.x.bits=0x42000000u;s.y.bits=0x42800000u;
            assert(actor->flags==123&&actor->x.bits==s.x.bits);
            assert(states.capture(slot)&&actor->previous.x.bits==s.x.bits);
            {auto terrainEdit=states.editDomain<true>(1);
                terrainEdit.get().flags^=2u;
                assert(actor->flags==123&&states.read(1)->flags==terrainEdit.get().flags);}
            RunnerSpatialState replacement;replacement.x.bits=0x43000000u;
            states.fresh(slot,replacement);
            assert(actor->x.bits==replacement.x.bits&&actor->previous.x.bits==replacement.x.bits);
            auto box=replacement.bounds;box.left.bits=0x3f800000u;
            states.cache(slot,box,{});assert(actor->bounds.left.bits==box.left.bits);
        }
    }
    std::printf("Run spatial publication: %u stage comparisons, nested query edits and sibling isolation passed\n",compared);
}
