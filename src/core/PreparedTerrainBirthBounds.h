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
    Shape shapes_[4];
    RunnerSpatialSplit::PixelBox pixelOffsets_[4]{};
    int offsets_[offsetCapacity]{};
    RunnerImageValue values_[rows][offsetCapacity];
    static unsigned shape(int object){return object==60?0:object==93?1:object==99?2:object==92?3:4;}
public:
    // Immutable geometry identity, not an instance ID or a pointer into live
    // terrain. It can be retained in publication history after that instance
    // moves, changes sprite, retires or reuses its slot. The prepared resource
    // table must outlive all keys and must not be re-prepared while they exist.
    struct Key {std::uint32_t value=0;};
    bool validKey(Key key)const{
        const unsigned id=key.value&3u,row=key.value>>8;
        return shapes_[id].ready&&row>=16&&row<rows;
    }
    bool key(int object,int x,int y,Key& out)const{
        const unsigned id=shape(object);
        if(id==4||!shapes_[id].ready||x < -256||x>=768||y<0||y>=8192||
            (unsigned(x)&15u)||(unsigned(y)&15u))return false;
        out.value=(unsigned(y+256)>>4)*256u+(unsigned(x+256)>>4)*4u+id;
        return true;
    }
    bool bounds(Key key,RunnerSpatialSplit::Box& out,bool normalize=false)const{
        const unsigned id=key.value&3u,column=(key.value>>2)&63u,row=key.value>>8;
        if(!shapes_[id].ready||row<16||row>=rows)return false;
        const auto& s=shapes_[id];
        const auto* horizontal=values_[column];const auto* vertical=values_[row];
        out={horizontal[s.left],vertical[s.top],horizontal[s.right],vertical[s.bottom]};
        if(normalize){
            const auto& p=pixelOffsets_[id];
            if(p.right<p.left){const auto v=out.left;out.left=out.right;out.right=v;}
            if(p.bottom<p.top){const auto v=out.top;out.top=out.bottom;out.bottom=v;}
        }
        return true;
    }
    // Publication history needs integer geometry, not a transient expansion
    // into runner position/bounds fields. Decode only at span admission;
    // consecutive builder cells can advance these endpoints by their stride.
    bool pixels(Key key,RunnerSpatialSplit::PixelBox& out)const{
        if(!validKey(key))return false;
        const auto& p=pixelOffsets_[key.value&3u];
        const int x=int((key.value>>2)&63u)*16-256,y=int(key.value>>8)*16-256;
        out={x+p.left,y+p.top,x+p.right,y+p.bottom};
        return true;
    }
    // Constructor publication can retain this immutable key directly. The
    // journal still admits every insertion in source order and owns pressure
    // handling; no instance or tree-membership decision is cached here.
    bool publication(int object,int x,int y,Key& out,
                     RunnerSpatialSplit::PixelBox& box,bool& precise)const{
        if(!key(object,x,y,out)||!pixels(out,box))return false;
        precise=shapes_[out.value&3u].precise;
        if(box.right<box.left){const auto v=box.left;box.left=box.right;box.right=v;}
        if(box.bottom<box.top){const auto v=box.top;box.top=box.bottom;box.bottom=v;}
        return true;
    }
    template<unsigned N> bool prepare(const PreparedObjectDefaults<N>& objects,
        const SpriteCollisionPack& sprites){
        DW_TERRAIN_PREPARATION_SCOPE;
        for(auto& s:shapes_)s.ready=false;
        if(!sprites.valid())return false;
        // Offset zero is the position itself. Bounds often share endpoints
        // across axes/objects; prepare each distinct offset only once.
        int offsets[offsetCapacity]={};unsigned count=1;
        const int ids[]={60,93,99,92};
        for(unsigned i=0;i<4;++i){
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
            pixelOffsets_[i]={bounds.left,bounds.top,bounds.right,bounds.bottom};
            s.precise=resource.collisionMode==1;s.ready=true;
        }
        for(unsigned row=0;row<rows;++row)for(unsigned j=0;j<count;++j)
            values_[row][j].assignInteger(int(row)*16-256+offsets[j]);
        for(unsigned j=0;j<count;++j)offsets_[j]=offsets[j];
        return true;
    }
    bool at(int object,int x,int y,RunnerImageValue& positionX,RunnerImageValue& positionY,
        RunnerSpatialSplit::Box& box,bool& precise,RunnerSpatialSplit::PixelBox* pixels=nullptr,Key* key=nullptr)const{
        return atPrepared<true>(object,x,y,positionX,positionY,box,precise,pixels,key);
    }
    bool atReference(int object,int x,int y,RunnerImageValue& positionX,RunnerImageValue& positionY,
        RunnerSpatialSplit::Box& box,bool& precise,RunnerSpatialSplit::PixelBox* pixels=nullptr)const{
        return atPrepared<false>(object,x,y,positionX,positionY,box,precise,pixels,nullptr);
    }
private:
    template<bool PreparedPixels>
    bool atPrepared(int object,int x,int y,RunnerImageValue& positionX,RunnerImageValue& positionY,
        RunnerSpatialSplit::Box& box,bool& precise,RunnerSpatialSplit::PixelBox* pixels,Key* key)const{
        const unsigned id=shape(object);
        if(id==4||!shapes_[id].ready||x < -256||x>=768||y<0||y>=8192||
            (unsigned(x)&15u)||(unsigned(y)&15u))return false;
        const unsigned column=unsigned(x+256)>>4,row=unsigned(y+256)>>4;
        if(key)key->value=row*256u+column*4u+id;
        const auto& s=shapes_[id];
        const auto* horizontal=values_[column];const auto* vertical=values_[row];
        positionX=horizontal[0];positionY=vertical[0];
        box.left=horizontal[s.left];box.right=horizontal[s.right];
        box.top=vertical[s.top];box.bottom=vertical[s.bottom];
        if(pixels){
            if constexpr(PreparedPixels){
                // Resource-wide offset selection belongs in prepare(), not
                // four dictionary lookups for every generated instance.
                const auto& offset=pixelOffsets_[id];
                pixels->left=x+offset.left;pixels->top=y+offset.top;
                pixels->right=x+offset.right;pixels->bottom=y+offset.bottom;
            }else *pixels={x+offsets_[s.left],y+offsets_[s.top],
                x+offsets_[s.right],y+offsets_[s.bottom]};
        }
        precise=s.precise;return true;
    }
};
}
#endif
