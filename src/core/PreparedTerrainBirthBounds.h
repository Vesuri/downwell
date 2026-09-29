#ifndef DOWNWELL_PREPARED_TERRAIN_BIRTH_BOUNDS_H
#define DOWNWELL_PREPARED_TERRAIN_BIRTH_BOUNDS_H
#include "PreparedObjectDefaults.h"
#include "PreparedUnitSpriteBounds.h"
#include "Profile.h"
namespace downwell {
// Only immutable default geometry: no instances, RNG, memberships or Create
// effects are published during preparation. Other positions use the reference.
class PreparedTerrainBirthBounds {
    static constexpr unsigned rows=528,offsetCapacity=4;
    struct Shape {unsigned char left=0,right=0,top=0,bottom=0;bool ready=false,precise=false;};
    Shape shapes_[3];
    RunnerImageValue values_[rows][offsetCapacity];
    static unsigned shape(int object){return object==60?0:object==93?1:object==99?2:3;}
public:
    template<unsigned N> bool prepare(const PreparedObjectDefaults<N>& objects,
        const SpriteCollisionPack& sprites){
        DW_TERRAIN_PREPARATION_SCOPE;
        for(auto& s:shapes_)s.ready=false;
        if(!sprites.valid())return false;
        // Offset zero is the position itself. Bounds often share endpoints
        // across axes/objects; prepare each distinct offset only once.
        int offsets[offsetCapacity]={};unsigned count=1;
        const int ids[]={60,93,99};
        for(unsigned i=0;i<3;++i){
            const auto* object=objects.get(ids[i]);if(!object)continue;
            PackedSpriteCollision resource;
            if(!sprites.collisionSprite(object->spriteId,object->maskId,resource))continue;
            PreparedUnitSpriteBounds bounds;bounds.prepare(resource);
            if(!bounds.available)continue;
            int candidate[offsetCapacity];
            for(unsigned j=0;j<count;++j)candidate[j]=offsets[j];
            unsigned used=count;unsigned char indices[4];
            const int edges[]={bounds.left,bounds.right,bounds.top,bounds.bottom};
            bool fits=true;
            for(unsigned edge=0;edge<4;++edge){
                unsigned j=0;while(j<used&&candidate[j]!=edges[edge])++j;
                if(j==used){
                    if(used==offsetCapacity){fits=false;break;}
                    candidate[used++]=edges[edge];
                }
                indices[edge]=static_cast<unsigned char>(j);
            }
            // A changed resource needing more offsets retains general bounds.
            // Do not let a rejected shape consume slots needed by later ones.
            if(!fits)continue;
            count=used;for(unsigned j=0;j<count;++j)offsets[j]=candidate[j];
            auto& s=shapes_[i];
            s.left=indices[0];s.right=indices[1];s.top=indices[2];s.bottom=indices[3];
            s.precise=resource.collisionMode==1;s.ready=true;
        }
        for(unsigned row=0;row<rows;++row)for(unsigned j=0;j<count;++j)
            values_[row][j].assignInteger(int(row)*16-256+offsets[j]);
        return true;
    }
    bool at(int object,int x,int y,RunnerImageValue& positionX,RunnerImageValue& positionY,
        RunnerSpatialSplit::Box& box,bool& precise)const{
        const unsigned id=shape(object);
        if(id==3||!shapes_[id].ready||x < -256||x>=768||y<0||y>=8192||
            (unsigned(x)&15u)||(unsigned(y)&15u))return false;
        const unsigned column=unsigned(x+256)>>4,row=unsigned(y+256)>>4;
        const auto& s=shapes_[id];
        const auto* horizontal=values_[column];const auto* vertical=values_[row];
        positionX=horizontal[0];positionY=vertical[0];
        box.left=horizontal[s.left];box.right=horizontal[s.right];
        box.top=vertical[s.top];box.bottom=vertical[s.bottom];
        precise=s.precise;return true;
    }
};
}
#endif
