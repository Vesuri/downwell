#ifndef DOWNWELL_CORE_RUNNER_LAYER_IDS_H
#define DOWNWELL_CORE_RUNNER_LAYER_IDS_H
#include <cstdint>
namespace downwell {
// Original process globals 0x925f84/88. Keep this owner across room changes;
// layer storage slots and room-local maxima are not source IDs. The globals
// begin in the PE's zero-filled data tail. Negative/wrapped IDs are outside
// the admitted layer-ID domain and fail explicitly, never wrap into -1.
struct RunnerLayerIds {
    std::int32_t loadedMaximum=0,dynamicMaximum=0;
    bool observeLoaded(std::int32_t id){
        if(id<0)return false;
        if(id>loadedMaximum)loadedMaximum=id;
        return true;
    }
    bool allocate(std::int32_t& id){
        if(loadedMaximum<0||dynamicMaximum<0)return false;
        const auto maximum=loadedMaximum>dynamicMaximum?loadedMaximum:dynamicMaximum;
        if(maximum==0x7fffffff)return false;
        id=dynamicMaximum=maximum+1;return true;
    }
    template<class Layers>
    bool createDynamic(Layers& layers,std::int32_t depth,unsigned& slot){
        std::int32_t id;
        // Source publishes the ID before room insertion. A failed operation
        // is terminal for its caller; do not rewind consumed identities.
        return allocate(id)&&layers.insertAfterEquals(id,depth,true,slot);
    }
};
}
#endif
