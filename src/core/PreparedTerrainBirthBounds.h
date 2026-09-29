#ifndef DOWNWELL_PREPARED_TERRAIN_BIRTH_BOUNDS_H
#define DOWNWELL_PREPARED_TERRAIN_BIRTH_BOUNDS_H
#include "PreparedObjectDefaults.h"
#include "PreparedUnitSpriteBounds.h"
namespace downwell {
// Only immutable default geometry: no instances, RNG, memberships or Create
// effects are published during preparation. Other positions use the reference.
class PreparedTerrainBirthBounds {
    static constexpr unsigned columns=64,rows=512;
    struct Axis {RunnerImageValue near,far;};
    struct Shape {Axis x[columns],y[rows];bool ready=false,precise=false;};
    Shape shapes_[3];
    RunnerImageValue x_[columns],y_[rows];
    static unsigned shape(int object){return object==60?0:object==93?1:object==99?2:3;}
public:
    template<unsigned N> bool prepare(const PreparedObjectDefaults<N>& objects,
        const SpriteCollisionPack& sprites){
        for(auto& s:shapes_)s.ready=false;
        if(!sprites.valid())return false;
        for(unsigned i=0;i<columns;++i)x_[i].assignInteger(int(i)*16-256);
        for(unsigned i=0;i<rows;++i)y_[i].assignInteger(int(i)*16);
        const int ids[]={60,93,99};
        for(unsigned i=0;i<3;++i){
            const auto* object=objects.get(ids[i]);if(!object)continue;
            PackedSpriteCollision resource;
            if(!sprites.collisionSprite(object->spriteId,object->maskId,resource))continue;
            PreparedUnitSpriteBounds bounds;bounds.prepare(resource);
            if(!bounds.available)continue;
            auto& s=shapes_[i];
            for(unsigned j=0;j<columns;++j){
                s.x[j].near.assignInteger(int(j)*16-256+bounds.left);
                s.x[j].far.assignInteger(int(j)*16-256+bounds.right);
            }
            for(unsigned j=0;j<rows;++j){
                s.y[j].near.assignInteger(int(j)*16+bounds.top);
                s.y[j].far.assignInteger(int(j)*16+bounds.bottom);
            }
            s.precise=resource.collisionMode==1;s.ready=true;
        }
        return true;
    }
    bool at(int object,int x,int y,RunnerImageValue& positionX,RunnerImageValue& positionY,
        RunnerSpatialSplit::Box& box,bool& precise)const{
        const unsigned id=shape(object);
        if(id==3||!shapes_[id].ready||x < -256||x>=768||y<0||y>=8192||
            (unsigned(x)&15u)||(unsigned(y)&15u))return false;
        const unsigned column=unsigned(x+256)>>4,row=unsigned(y)>>4;
        const auto& s=shapes_[id];
        positionX=x_[column];positionY=y_[row];
        box.left=s.x[column].near;box.right=s.x[column].far;
        box.top=s.y[row].near;box.bottom=s.y[row].far;
        precise=s.precise;return true;
    }
};
}
#endif
