#ifndef DOWNWELL_CORE_RUNNER_LAYER_BACKGROUNDS_H
#define DOWNWELL_CORE_RUNNER_LAYER_BACKGROUNDS_H
#include "RunnerLayerBackground.h"
#include "RunnerLayers.h"
namespace downwell {
// Bounded background payload owner sharing element slots with instance
// reservations. Slots are NOT source element IDs; the process-wide source ID
// allocator must be connected separately. Never copy an owner with live links.
template<unsigned L,unsigned E,unsigned Capacity,class LayerStorage=RunnerLayers<L,E>> class RunnerLayerBackgrounds {
public:
    using Layers=LayerStorage;
    RunnerLayerBackgrounds()=default;
    RunnerLayerBackgrounds(const RunnerLayerBackgrounds&)=delete;
    RunnerLayerBackgrounds& operator=(const RunnerLayerBackgrounds&)=delete;
    struct Entry {unsigned element=E;RunnerLayerBackground background;};
    const Entry* get(unsigned element)const{
        if(element>=E)return nullptr;
        for(const auto& entry:entries_)if(entry.element==element)return &entry;
        return nullptr;
    }
    template<class ActiveInstance>
    bool create(Layers& layers,unsigned layer,const PackedRoomLayer& source,ActiveInstance active){
        const auto* destination=layers.get(layer);
        if(!layers.succeeded()||!destination||source.layerId>0x7fffffffu||
            destination->id!=std::int32_t(source.layerId))return false;
        unsigned record=0;while(record<Capacity&&entries_[record].element!=E)++record;
        if(record==Capacity)return false;
        const unsigned element=layers.firstFreeElement();
        if(element==E)return false;
        RunnerLayerBackground background;
        if(!RunnerLayerBackground::create(source,background)||
            !layers.attach(layer,element,false,false,active))return false;
        entries_[record]={element,background};return true;
    }
    bool initialize(unsigned element){
        if(element>=E)return false;
        for(auto& entry:entries_)if(entry.element==element){entry.background.initialize();return true;}
        return false;
    }
    bool uninitialize(unsigned element){
        if(element>=E)return false;
        for(auto& entry:entries_)if(entry.element==element){entry.background.initialized=false;return true;}
        return false;
    }
    bool discardUninitialized(){
        for(const auto& entry:entries_)if(entry.element!=E&&entry.background.initialized)return false;
        for(auto& entry:entries_)entry=Entry{};
        return true;
    }
    bool retire(Layers& layers,unsigned element){
        if(element>=E)return false;
        for(auto& entry:entries_)if(entry.element==element){
            if(!layers.detach(element))return false;
            entry=Entry{};return true;
        }
        return false;
    }
private:
    Entry entries_[Capacity?Capacity:1]{};
};
}
#endif
