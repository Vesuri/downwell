#ifndef DOWNWELL_CORE_SPRITE_COLLISION_PACK_H
#define DOWNWELL_CORE_SPRITE_COLLISION_PACK_H
#include <cstddef>
#include <cstdint>
namespace downwell {
struct PackedSpriteCollision {
    std::int32_t originX,originY,marginLeft,marginTop,marginRight,marginBottom;
    std::uint32_t collisionMode,collisionMaskCount,frameCount;
};
// Borrowed collision-only metadata. No artwork, atlas or mask-pixel payload.
// Implicit record IDs retain the complete contiguous original sprite table.
class SpriteCollisionPack {
public:
    static constexpr unsigned headerSize=24,recordSize=36;
    SpriteCollisionPack(const std::uint8_t* bytes,std::size_t size):bytes_(bytes){
        if(!bytes||size<headerSize||u32(bytes)!=0x43535744u||u32(bytes+4)!=0x00180001u)return;
        const std::uint8_t hash[]={0xc0,0x7d,0xaf,0x5f,0xbe,0x94,0x0a,0xe0};
        for(unsigned i=0;i<8;++i)if(bytes[8+i]!=hash[i])return;
        const auto count=u32(bytes+16);
        if(u32(bytes+20)!=3||count>(size-headerSize)/recordSize||size-headerSize!=std::size_t(count)*recordSize)return;
        for(std::uint32_t i=0;i<count;++i)if(u32(bytes+headerSize+std::size_t(i)*recordSize+24)>1)return;
        count_=count;valid_=true;
    }
    bool valid()const{return valid_;}
    std::uint32_t spriteCount()const{return count_;}
    // Optional pre-game preparation. The caller owns this native-endian
    // table and must keep it immutable/alive for this view and any copies.
    // Original packed bytes remain the reference; no RNG or instance work.
    bool prepare(PackedSpriteCollision* storage,std::size_t capacity){
        if(!valid_||!storage||capacity<count_)return false;
        for(std::uint32_t id=0;id<count_;++id)decode(id,storage[id]);
        prepared_=storage;
        return true;
    }
    // Sprite changes need only the frame count, not collision bounds. Decode
    // that field directly rather than unpacking nine words into a temporary.
    bool frameCount(std::uint32_t id,std::uint32_t& out)const{
        if(!valid_||id>=count_)return false;
        out=prepared_?prepared_[id].frameCount:u32(bytes_+headerSize+std::size_t(id)*recordSize+32);
        return true;
    }
    bool sprite(std::uint32_t id,PackedSpriteCollision& out)const{
        if(!valid_||id>=count_)return false;
        if(prepared_)out=prepared_[id];else decode(id,out);
        return true;
    }
    bool collisionSprite(std::int32_t spriteId,std::int32_t maskId,PackedSpriteCollision& out)const{
        const auto id=maskId<0?spriteId:maskId;
        return id>=0&&sprite(static_cast<std::uint32_t>(id),out);
    }
private:
    void decode(std::uint32_t id,PackedSpriteCollision& out)const{
        const auto* p=bytes_+headerSize+std::size_t(id)*recordSize;
        out={i32(p),i32(p+4),i32(p+8),i32(p+12),i32(p+16),i32(p+20),u32(p+24),u32(p+28),u32(p+32)};
    }
    static std::uint32_t u32(const std::uint8_t* p){
        return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
    }
    static std::int32_t i32(const std::uint8_t* p){
        const auto n=u32(p);return n<=0x7fffffffu?std::int32_t(n):-std::int32_t(0xffffffffu-n)-1;
    }
    const PackedSpriteCollision* prepared_=nullptr;
    const std::uint8_t* bytes_=nullptr;std::uint32_t count_=0;bool valid_=false;
};
}
#endif
