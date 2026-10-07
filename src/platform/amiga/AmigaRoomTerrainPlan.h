#ifndef DOWNWELL_AMIGA_ROOM_TERRAIN_PLAN_H
#define DOWNWELL_AMIGA_ROOM_TERRAIN_PLAN_H
#include "AmigaTerrain.h"
#include "core/PreparedRoomTerrain.h"
namespace amiga_room_plan_detail {
struct Command {
    std::uint32_t instanceId;
    std::uint16_t slot,object;
    std::int16_t x,y;
    std::uint16_t cell,bit;
    std::uint8_t before,after;
};
#include "../../../generated/amiga-room-terrain-plan.inc"
}

// Immutable placement metadata, not a second occupancy owner. Each command
// executes at its original room callback; no later wall is visible early.
// Source and index must outlive the plan. Reprepare after replacing either;
// preparation reads only resource metadata, never live occupancy or RNG.
class AmigaRoomTerrainPlan {
    static_assert(AmigaTerrainIndex::columns==128&&AmigaTerrainIndex::rows==512&&
        AmigaTerrainIndex::minX==-512,"compiled room index geometry");
    struct Write {
        const downwell::PackedRoomInstance* source=nullptr;
        unsigned char* cell=nullptr;
        std::uint16_t* landing=nullptr;
        std::uint16_t bit=0;
        unsigned char before=0,after=0;
    };
    Write writes_[256];
    const downwell::PreparedRoomTerrain* source_=nullptr;
    unsigned count_=0,cursor_=0;
public:
    bool prepare(const downwell::PreparedRoomTerrain& source,AmigaTerrainIndex& index){
        source_=nullptr;count_=0;cursor_=0;
        if(source.count()!=amiga_room_plan_detail::sourceCount)return false;
        for(const auto& command:amiga_room_plan_detail::commands){
            if(command.slot>=source.count())return false;
            const auto& r=source.at(command.slot);
            if(r.instanceId!=command.instanceId||r.objectId!=command.object||
                r.x!=command.x||r.y!=command.y)return false;
            if(count_==256)return false;
            Write w;w.source=&r;
            if(command.bit){
                w.cell=index.cells+command.cell;
                w.before=command.before;w.after=command.after;
                w.landing=index.nextLanding.publicationRow(command.cell/128)+(command.cell%128)*32;
                w.bit=command.bit;
            }
            writes_[count_++]=w;
        }
        source_=&source;return true;
    }
    bool begin(const downwell::PreparedRoomTerrain* source){
        cursor_=0;return source&&source==source_;
    }
    bool emit(AmigaTerrainIndex& index,const downwell::PackedRoomInstance& r){
        if(cursor_>=count_||writes_[cursor_].source!=&r)
            return amigaAddTerrainObject(index,r.objectId,r.x,r.y,true);
        const auto& w=writes_[cursor_++];
        if(!w.cell)return true;
        if(*w.cell!=w.before)return amigaAddTerrainObject(index,r.objectId,r.x,r.y,true);
        *w.cell=w.after;
        *w.landing|=w.bit;
        return true;
    }
};
#endif
