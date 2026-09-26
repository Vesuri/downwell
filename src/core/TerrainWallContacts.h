#ifndef DOWNWELL_CORE_TERRAIN_WALL_CONTACTS_H
#define DOWNWELL_CORE_TERRAIN_WALL_CONTACTS_H
#include "EnemyContact.h"
#include <cstdint>
namespace downwell {
// Cavern Create records, independent of occupancy unions. Duplicate instances
// remain separate contacts. Row buckets bound queries to nearby instance rows.
template<unsigned Capacity=4096> class TerrainWallContacts {
    static_assert(Capacity>0 && Capacity<65536,"16-bit links required");
public:
    static constexpr unsigned rows=512;
    struct Record {std::int16_t x;std::uint16_t y,object,next;};
    void clear(){for(auto& head:heads_)head=0;count_=0;}
    unsigned size()const{return count_;}
    bool append(int object,int x,int y){
        if(object!=60 && object!=92 && object!=293)return true;
        if(count_==Capacity || x<-512 || x>=1536 || y<0 || y>=8192 || (x&15) || (y&15))return false;
        const unsigned row=unsigned(y)/16;
        records_[count_]={std::int16_t(x),std::uint16_t(y),std::uint16_t(object),heads_[row]};
        heads_[row]=++count_;return true;
    }
    // Active supplies retirement/breakable state from the owning simulation.
    // Emit is called once per matching instance, never once per occupied cell.
    template<class Active,class Emit>
    bool visit(const ContactBounds& query,Active active,Emit emit)const{
        if(query.left>=query.right || query.top>=query.bottom || query.right<=-521 ||
           query.left>=1546 || query.bottom<=-8 || query.top>=8201)return true;
        int first=query.top<=9?0:(query.top-9)/16;
        int last=query.bottom>=8184?511:(query.bottom+8)/16;
        if(first>511 || last<0)return true;
        for(int row=last;row>=first;--row)
            for(unsigned id=heads_[row];id;id=records_[id-1].next){
                const auto& r=records_[id-1];
                const auto local=contactSpriteBounds(r.object==293?569:585);
                const ContactBounds bounds{r.x+local.left,r.x+local.right,int(r.y)+local.top,int(r.y)+local.bottom};
                if(contactBoundsOverlap(query,bounds) && active(r.object,r.x,int(r.y)) && !emit())return false;
            }
        return true;
    }
private:
    Record records_[Capacity];
    std::uint16_t heads_[rows]{};
    unsigned count_=0;
};
}
#endif
