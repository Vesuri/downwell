#ifndef DOWNWELL_CORE_SPRITE_MASK_PACK_H
#define DOWNWELL_CORE_SPRITE_MASK_PACK_H
#include <cstddef>
#include <cstdint>
namespace downwell {
// Borrowed original MSB-first mask bytes. No artwork-alpha reconstruction.
class SpriteMaskPack {
public:
    struct Mask {
        const std::uint8_t* bytes=nullptr;
        unsigned width=0,height=0,stride=0;
        bool contains(int x,int y)const{
            return bytes&&x>=0&&y>=0&&unsigned(x)<width&&unsigned(y)<height&&
                (bytes[std::size_t(y)*stride+unsigned(x)/8]&(128u>>(unsigned(x)%8)));
        }
    };
    SpriteMaskPack(const std::uint8_t* bytes,std::size_t size):bytes_(bytes){
        if(!bytes||size<24||u32(bytes)!=0x4d535744u||u32(bytes+4)!=0x00180001u||u32(bytes+20)!=3)return;
        const std::uint8_t hash[]={0xc0,0x7d,0xaf,0x5f,0xbe,0x94,0x0a,0xe0};
        for(unsigned i=0;i<8;++i)if(bytes[8+i]!=hash[i])return;
        const auto count=u32(bytes+16);
        if(count>(size-24)/20)return;
        std::size_t end=24+std::size_t(count)*20;
        std::uint32_t previousSprite=0,previousFrame=0;
        for(std::uint32_t i=0;i<count;++i){
            const auto* p=bytes+24+std::size_t(i)*20;
            const auto sprite=u32(p),frame=u32(p+4),width=u32(p+8),height=u32(p+12),offset=u32(p+16);
            if(i&&(sprite<previousSprite||(sprite==previousSprite&&frame<=previousFrame)))return;
            if(!width||!height||width>0x7fffffffu||height>0x7fffffffu||offset!=end)return;
            const auto stride=(width+7)/8;
            if(end>size||height>(size-end)/stride)return;
            end+=std::size_t(stride)*height;previousSprite=sprite;previousFrame=frame;
        }
        if(end!=size)return;
        count_=count;valid_=true;
    }
    bool valid()const{return valid_;}
    unsigned count()const{return count_;}
    // Exact stored mask-frame index, not image_index. Frame normalization and
    // single-mask reuse are the collision caller's responsibility.
    bool mask(std::uint32_t sprite,std::uint32_t frame,Mask& out)const{
        if(!valid_)return false;
        unsigned first=0,last=count_;
        while(first<last){
            const unsigned mid=first+(last-first)/2;
            const auto* p=bytes_+24+std::size_t(mid)*20;
            const auto id=u32(p),index=u32(p+4);
            if(id<sprite||(id==sprite&&index<frame)){first=mid+1;continue;}
            if(id>sprite||(id==sprite&&index>frame)){last=mid;continue;}
            const auto width=u32(p+8);
            out={bytes_+u32(p+16),width,u32(p+12),(width+7)/8};return true;
        }
        return false;
    }
private:
    static std::uint32_t u32(const std::uint8_t* p){return std::uint32_t(p[0])|
        (std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
    const std::uint8_t* bytes_=nullptr;unsigned count_=0;bool valid_=false;
};
}
#endif
