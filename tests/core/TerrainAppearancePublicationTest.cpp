#include "core/GameplayTerrainInstancesRuns.h"
#include "core/RunnerTerrainProperties.h"
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
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    PreparedObjectDefaults<386> defaults;assert(defaults.prepare(objects,sprites));
    using P=RunnerSpatialInstances<4,16>::Properties;
    using Terrain=GameplayTerrainInstancesRuns;
    using Properties=RunnerTerrainProperties<P,2050,1,2049,Terrain>;
    for(int object:{60,93,99})for(unsigned stop=0;stop<=2;++stop){
        Terrain terrain;Properties properties;assert(properties.bind(terrain));properties.prepare(defaults);
        const auto* d=defaults.get(object);assert(d);
        for(unsigned i=0;i<4;++i){
            const unsigned stage=i==3?stop:2;
            const bool completed=terrain.appendInitializedCell(object,int(i)*16,0,true,
                [&](InstanceHandle h,auto&,bool&,bool){
                    const unsigned slot=h.slot+1;
                    auto p=properties.read(slot);
                    assert(p->sprite.spriteId==-1&&p->sprite.maskId==-1&&p->depth.bits==0);
                    if(stage>=1){properties.birth(slot);p=properties.read(slot);
                        assert(p->sprite.spriteId==d->spriteId&&p->sprite.maskId==d->maskId&&
                            p->sprite.imageIndex.bits==0&&p->depth.bits==d->creationDepth.bits&&
                            p->sprite.scaleX.bits==0x3f800000u&&p->sprite.scaleY.bits==0x3f800000u&&
                            p->timeline.bits==0&&p->cachedFrame.bits==0xbf800000u);
                    }
                    if(stage>=2)properties.created(slot,51,{0x41700000u});
                    for(unsigned j=0;j<i;++j){const auto old=properties.read(j+1);
                        assert(old->sprite.spriteId==51&&old->sprite.imageIndex.bits==0x41700000u);
                    }
                    return i<3||stage==2;
                });
            assert(completed==(i<3||stage==2));
        }
        assert(terrain.ownershipRunCount()==1);
        if(stop<2){
            assert(!terrain.valid());const auto last=properties.read(4);
            assert(last->sprite.spriteId==(stop?d->spriteId:-1));
            continue;
        }
        // Two terrain reads own independent projections. Updating one cell
        // must neither invalidate them nor change a sibling's appearance.
        const auto first=properties.read(1),second=properties.read(2);
        {auto edit=properties.edit(1);edit.get().sprite.imageIndex.bits=0;
            edit.publish();assert(properties.read(1)->sprite.imageIndex.bits==0);}
        assert(first->sprite.imageIndex.bits==0x41700000u&&second->sprite.imageIndex.bits==0x41700000u&&
            properties.read(2)->sprite.imageIndex.bits==0x41700000u);
        assert(terrain.ownershipRunCount()==2&&properties.valid()&&terrain.valid());
        // Actors continue to borrow their original mutable full records.
        for(unsigned slot:{0u,2049u}){
            const auto actor=properties.read(slot);
            auto edit=properties.editDomain<false>(slot);edit.get().sprite.spriteId=777;
            edit.publish();assert(actor->sprite.spriteId==777);
            {auto terrainEdit=properties.editDomain<true>(1);
                terrainEdit.get().sprite.imageIndex.bits=0;terrainEdit.publish();
                assert(actor->sprite.spriteId==777);}
            assert(actor->sprite.spriteId==777);
        }
    }
    std::puts("Run appearance publication: pristine/constructor/Create prefixes, sibling isolation and simultaneous reads passed");
}
