#ifndef DOWNWELL_CORE_ROOM_ENTRY_VIEWS_H
#define DOWNWELL_CORE_ROOM_ENTRY_VIEWS_H
#include "RoomPack.h"
#include "RunnerImageValue.h"
namespace downwell {
// Initial cache copied from room resources before instance events. Kept
// separate from a followed actor and from subsequent script camera setters.
class RoomEntryViews {
public:
    struct View { RunnerImageValue x,y,width,height; bool enabled=false; };
    bool load(const RoomPack& pack,int roomId) {
        clear();
        for(unsigned i=0;i<pack.roomCount();++i){
            PackedRoom room;
            if(!pack.room(i,room))return false;
            if(room.roomId!=roomId)continue;
            for(unsigned slot=0;slot<RoomPack::viewCount;++slot){
                PackedRoomView source;
                if(!pack.roomView(i,slot,source))return false;
                auto& v=views_[slot];
                // Integers are exact in binary64: one binary32 rounding.
                if(!v.x.assignInteger(source.x)||!v.y.assignInteger(source.y)||
                   !v.width.assignInteger(source.width)||!v.height.assignInteger(source.height))return false;
                v.enabled=source.enabled;
            }
            valid_=true;return true;
        }
        return false;
    }
    void clear(){valid_=false;}
    const View* get(unsigned slot)const{
        return valid_&&slot<RoomPack::viewCount?&views_[slot]:nullptr;
    }
private:
    View views_[RoomPack::viewCount]{};
    bool valid_=false;
};
}
#endif
