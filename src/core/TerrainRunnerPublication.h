#ifndef DOWNWELL_TERRAIN_RUNNER_PUBLICATION_H
#define DOWNWELL_TERRAIN_RUNNER_PUBLICATION_H
#include "RunnerRoomOrderState.h"
#include "RunnerLogicalLayerOrder.h"
#include "RunnerSpatialState.h"
namespace downwell {
struct TerrainAppearance {
    int sprite=-1;
    RunnerImageValue index{};
    unsigned char model=0;
    TerrainAppearance shifted(unsigned)const{return *this;}
    bool same(const TerrainAppearance& b)const{
        return sprite==b.sprite&&index.bits==b.index.bits&&model==b.model;
    }
};
// Captured order bases, owned by the same run as identity and geometry.
// Membership visibility is separate: a newly allocated cell must not expose
// inherited state before its original registration/room publication boundary.
struct TerrainRunnerPublication {
    RunnerRegistrationOrder registration;
    RunnerRoomOrderState room;
    RunnerRetainedLayerState layer;
    TerrainAppearance appearance;
    // Literal state only for an observed/mutated cell. Ordinary generated
    // state comes from run geometry and the cell's publication stage.
    RunnerSpatialState spatial;
    RunnerRegistrationOrder spatialDirty,collisionDirty;
    TerrainRunnerPublication shifted(unsigned offset)const{
        auto result=*this;
        if(registration.present()){
            result.registration.low+=offset;
            if(result.registration.low<registration.low)++result.registration.high;
        }
        result.room=room.shifted(offset);result.layer=layer.shifted(offset);
        result.spatialDirty=spatialDirty.shifted(offset);result.collisionDirty=collisionDirty.shifted(offset);
        return result;
    }
    bool same(const TerrainRunnerPublication& b)const{
        return registration.low==b.registration.low&&registration.high==b.registration.high&&
            room.same(b.room)&&layer.same(b.layer)&&appearance.same(b.appearance)&&spatial.same(b.spatial)&&
            spatialDirty.same(b.spatialDirty)&&collisionDirty.same(b.collisionDirty);
    }
};
}
#endif
