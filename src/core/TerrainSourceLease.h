#ifndef DOWNWELL_CORE_TERRAIN_SOURCE_LEASE_H
#define DOWNWELL_CORE_TERRAIN_SOURCE_LEASE_H
#include "CallbackFailureSignal.h"
#include "StationaryRetirementWindow.h"
#include "TerrainBirthRecords.h"
#include <cstdint>
namespace downwell {
// Borrowed consumer counters for callback-free source terrain segments, not
// a second record. The collision owner admits one segment through its whole
// source span, then lends exactly the counters that span updates: identity
// chronology and capture revisions, the terrain layer's retained population,
// the layer and room publication clocks, and the thin-platform retirement
// proof. Later segments of the same tile publish here, with no per-owner
// admission. Only owner health, borrowed callback status, layer topology and
// the bounded counters can change the span's admission while the binding
// lasts; admits() observes each of them. Rebinding revokes the lease.
struct TerrainSourceLease {
    int tile=-1;
    unsigned char flags[2]{};
    int layerId=0;unsigned layer=0;
    std::uint32_t roomDepth=0;
    unsigned capacity=0;
    unsigned *retained=nullptr,*logical=nullptr,*live=nullptr;
    std::uint32_t *drawRevision=nullptr,*nextOrder=nullptr,*bindingRevision=nullptr,*captureRevision=nullptr;
    const TerrainPublicationClock *layerClock=nullptr,*roomClock=nullptr;
    StationaryRetirementWindow* thin=nullptr;
    const bool* good=nullptr;
    const CallbackFailureScopes* scopes=nullptr;
    const std::uint32_t* topology=nullptr;
    std::uint32_t topologyRevision=0;
    static void add(std::uint32_t& revision,unsigned count){
        revision=revision>0xffffffffu-count?0xffffffffu:revision+count;
    }
    bool admits(int segmentTile,unsigned count)const{
        return good&&tile==segmentTile&&*good&&scopes->valid()&&*topology==topologyRevision&&
            count<=capacity-*logical&&*nextOrder&&count-1<=0xffffffffu-*nextOrder;
    }
    // The lender's general birth admission: its validity and chronology.
    bool admitsBirths(unsigned count)const{
        return good&&*good&&scopes->valid()&&*nextOrder&&(!count||count-1<=0xffffffffu-*nextOrder);
    }
    // The record owner has written cells, identity, row and tile. Same
    // effects as the source span: no observer runs between these counters.
    void publish(unsigned count,TerrainBirthFacts& facts){
        facts.flags[0]=flags[0];facts.flags[1]=flags[1];
        facts.registration=*nextOrder;*nextOrder+=count;
        add(*bindingRevision,count);add(*captureRevision,count);
        const auto room=roomClock->basis(),order=layerClock->basis();
        facts.roomLow=room.low;facts.roomHigh=room.high;facts.roomDepth=roomDepth;
        facts.layer=layer;facts.layerId=layerId;facts.layerLow=order.low;facts.layerHigh=order.high;
        *retained+=count;*logical+=count;*live+=count;add(*drawRevision,count);
        unsigned thinCount=0;
        for(unsigned i=0;i<count;++i)thinCount+=facts.cells[2*i]>>7;
        thin->appendSpan(facts.y,thinCount!=0,facts.generation+count-1,count);
        add(*captureRevision,thinCount);
    }
};
}
#endif
