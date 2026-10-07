#include "platform/amiga/AmigaRoomTerrainPlan.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
static void check(bool yes,const char* what){
    if(!yes){std::cerr<<what<<'\n';std::exit(1);}
}
int main(int argc,char** argv){
    check(argc==2,"room pack required");
    std::ifstream input(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};
    downwell::RoomPack rooms(bytes.data(),bytes.size());
    downwell::PackedRoom room;
    check(rooms.findRoom(11,room),"main room");
    downwell::PreparedRoomTerrain source;
    check(source.prepare(rooms,room),"immutable placements");
    static AmigaTerrainIndex reference,candidate;
    AmigaRoomTerrainPlan plan;
    check(plan.prepare(source,candidate),"compile main room backend");
    const AmigaTerrainCollisionContext empty{nullptr,nullptr,nullptr,nullptr,0,nullptr};
    unsigned checks=0;
    for(unsigned overlaps:{0u,1u,2u,3u,4u,260u}){
        check(amigaBuildTerrainIndex(empty,reference)&&amigaBuildTerrainIndex(empty,candidate),"fresh indexes");
        // Non-pristine occupancy must retain scalar merging and overflow
        // behavior; exact expected preimages are an admission guard, not a hint.
        for(unsigned i=0;i<source.count();++i){
            const auto& r=source.at(i);
            if(r.objectId!=60)continue;
            for(unsigned n=0;n<overlaps;++n)
                check(amigaAddTerrainObject(reference,60,r.x,r.y,true)==
                    amigaAddTerrainObject(candidate,60,r.x,r.y,true),"stale occupancy setup");
            break;
        }
        check(plan.begin(&source),"rewind without recompilation");
        for(unsigned i=0;i<source.count();++i){
            const auto& r=source.at(i);if(r.objectId==35)continue;
            const bool expected=amigaAddTerrainObject(reference,r.objectId,r.x,r.y,true);
            check(plan.emit(candidate,r)==expected,"publication result");
            check(!std::memcmp(reference.cells,candidate.cells,sizeof(reference.cells)),"every occupancy prefix");
            check(!std::memcmp(&reference.nextLanding,&candidate.nextLanding,sizeof(reference.nextLanding)),"every landing prefix");
            check(amigaTerrainContactCount(reference)==amigaTerrainContactCount(candidate),"contact counts");
            check(amigaTerrainContacts(reference,r.objectId,r.x,r.y)==
                amigaTerrainContacts(candidate,r.objectId,r.x,r.y),"contact location and family");
            ++checks;if(!expected)break;
        }
    }
    check(!plan.begin(nullptr),"unbound source rejects admission");
    for(unsigned i=0;i<source.count();++i){
        auto& r=const_cast<downwell::PackedRoomInstance&>(source.at(i));
        if(r.objectId!=60)continue;
        const auto saved=r;
        r.x|=1;
        check(!plan.prepare(source,candidate)&&!plan.begin(&source),"unaligned geometry invalidates plan");
        r=saved;r.objectId=92;
        check(!plan.prepare(source,candidate)&&!plan.begin(&source),"unsupported Create retains scalar backend");
        r=saved;
        check(plan.prepare(source,candidate),"explicit resource replacement rebuilds plan");
        break;
    }
    std::cout<<"Compiled room terrain: "<<checks<<" publication prefixes agree\n";
}
