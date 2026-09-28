#ifndef DOWNWELL_RUNNER_LAYER_ALLOCATION_VERIFICATION_H
#define DOWNWELL_RUNNER_LAYER_ALLOCATION_VERIFICATION_H
#include "RunnerLayerInstances.h"
namespace downwell {
struct RunnerLayerAllocationVerification {
    static bool run(unsigned steps=512){
        using Registry=RunnerLayerInstances<1,96>;
        Registry registry;Registry::Layers layers;
        unsigned layer;std::uint32_t random=0x481fe012u;
        if(!layers.insertAfterEquals(1,0,false,layer))return false;
        for(unsigned n=0;n<steps;++n){
            random=random*1664525u+1013904223u;
            const unsigned slot=(random>>16)%Registry::End;
            const auto* entry=registry.get(slot);
            switch((random>>8)&3){
            case 0:case 1:{
                unsigned expected=0;
                while(expected<Registry::End&&(registry.get(expected)||
                    layers.elementOwner(expected)!=Registry::Layers::End))++expected;
                RunnerDepthService::Instance state;
                const InstanceHandle h{n,1};
                const bool ok=registry.attach(layers,layer,0,h,state,false);
                if(ok!=(expected!=Registry::End)||
                    (ok&&registry.find(h)!=expected))return false;
                break;
            }
            case 2:
                if(entry){if(!registry.retire(layers,entry->sourceId,entry->instance))return false;}
                else if(layers.elementOwner(slot)!=Registry::Layers::End){
                    if(n&1){if(!layers.detach(slot))return false;}
                    else if(!layers.detachForRebind(slot))return false;
                }
                break;
            case 3:
                if(!entry&&layers.elementOwner(slot)==Registry::Layers::End&&
                    !layers.attach(layer,slot,false,false,[](unsigned){return false;}))return false;
                break;
            }
        }
        return true;
    }
};
}
#endif
