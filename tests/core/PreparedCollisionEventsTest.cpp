#include "core/PreparedCollisionEvents.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace downwell;
int main(){
    std::ifstream in("generated/downwell-objects.dwob",std::ios::binary);assert(in);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in),{}};
    ObjectPack pack(bytes.data(),bytes.size());assert(pack.valid());
    PreparedCollisionEvents<386,512> prepared;assert(prepared.prepare(pack));
    // Collection admission must inspect incoming events as well as outgoing
    // handlers. These resource witnesses are deliberately broader than the
    // currently admitted dither/wind dispatch subset.
    const std::vector<unsigned> wallPartners{61,110,142,153,188,223,225,226,259,265,269,276};
    for(int terrain:{60,99,93}){
        std::vector<unsigned> partners;
        for(unsigned other=0;other<pack.objectCount();++other){
            bool outgoing=false,incoming=false;PackedObjectEvent event;
            assert(prepared.collisionEvent(terrain,other,outgoing,event)&&
                prepared.collisionEvent(other,terrain,incoming,event));
            assert(!outgoing);
            if(incoming)partners.push_back(other);
        }
        assert(partners==(terrain==93?std::vector<unsigned>{110,188}:wallPartners));
    }
    unsigned checks=0;
    for(int a=-1;a<=int(pack.objectCount());++a)for(int b=-1;b<=int(pack.objectCount());++b){
        bool af=true,bf=true;PackedObjectEvent ae{123,456,789,321},be=ae;
        assert(pack.collisionEvent(a,b,af,ae)==prepared.collisionEvent(a,b,bf,be));
        assert(af==bf&&ae.ownerObjectId==be.ownerObjectId&&ae.eventType==be.eventType&&
            ae.eventSubtype==be.eventSubtype&&ae.codeId==be.codeId);++checks;
    }
    PreparedCollisionEvents<386,1> shortEvents;assert(!shortEvents.prepare(pack)&&!shortEvents.ready());
    PreparedCollisionEvents<1,512> shortObjects;assert(!shortObjects.prepare(pack)&&!shortObjects.ready());
    ObjectPack invalid(nullptr,0);assert(!prepared.prepare(invalid)&&!prepared.ready());
    bool found=true;PackedObjectEvent untouched{123,456,789,321};
    assert(!prepared.collisionEvent(0,0,found,untouched)&&found&&untouched.codeId==321);
    assert(prepared.prepare(pack));
    std::printf("Prepared collision events: %u original pair lookups, invalid IDs, miss outputs and failed/repeated preparation match pack oracle\n",checks);
}
