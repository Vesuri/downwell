#include "core/SpriteMaskPack.h"
#include "core/SpriteCollisionPack.h"
#include "core/RunnerMaskTransform.h"
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using Pack=downwell::SpriteMaskPack;
using Real=downwell::RunnerImageValue;
Real packed(float x){Real value;std::memcpy(&value.bits,&x,4);return value;}
float unpacked(Real value){float x;std::memcpy(&x,&value.bits,4);return x;}
int main(int argc,char** argv){
    assert(argc==3);
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    Pack pack(bytes.data(),bytes.size());assert(pack.valid()&&pack.count()==741);
    std::ifstream metadataInput(argv[2],std::ios::binary);
    std::vector<std::uint8_t> metadataBytes((std::istreambuf_iterator<char>(metadataInput)),{});
    downwell::SpriteCollisionPack metadata(metadataBytes.data(),metadataBytes.size());
    assert(metadata.valid()&&metadata.spriteCount()==741);
    unsigned allPixels=0,rectangles=0,nonRectangularMasks=0;
    for(unsigned id=0;id<741;++id){
        Pack::Mask mask;assert(pack.mask(id,0,mask));
        downwell::PackedSpriteCollision sprite;assert(metadata.sprite(id,sprite));
        assert(sprite.collisionMaskCount==1);
        bool differs=false;
        for(int y=-1;y<=int(mask.height);++y)for(int x=-1;x<=int(mask.width);++x){
            const bool inside=x>=0&&y>=0&&x<int(mask.width)&&y<int(mask.height);
            const bool expected=inside&&((mask.bytes[y*mask.stride+x/8]>>(7-x%8))&1);
            assert(mask.contains(x,y)==expected);++allPixels;
            if(inside)differs|=expected!=(x>=sprite.marginLeft&&x<=sprite.marginRight&&
                y>=sprite.marginTop&&y<=sprite.marginBottom);
        }
        if(sprite.collisionMode==0){++rectangles;nonRectangularMasks+=differs;}
        assert(!pack.mask(id,1,mask));
    }
    assert(rectangles==737&&nonRectangularMasks==98);
    // Rectangular-mode partners still have stored masks used by precise-pair
    // tests. Neither their artwork canvas nor alpha is the collision shape.
    Pack::Mask blood;assert(pack.mask(84,0,blood));
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)
        assert(blood.contains(x,y)==(x>=10&&x<=13&&y>=11&&y<=13));
    const unsigned ids[]={577,597,613,616},widths[]={48,48,72,128},heights[]={40,48,72,128};
    const unsigned sourceHits[]={1044,1741,3969,12645};
    unsigned checked=0;
    for(unsigned i=0;i<4;++i){
        Pack::Mask mask;assert(pack.mask(ids[i],0,mask));
        assert(mask.width==widths[i]&&mask.height==heights[i]);
        unsigned hits=0;
        for(int y=-1;y<=int(mask.height);++y)for(int x=-1;x<=int(mask.width);++x){
            bool expected=false;
            if(x>=0&&y>=0&&x<int(mask.width)&&y<int(mask.height))
                expected=((mask.bytes[y*mask.stride+x/8]>>(7-x%8))&1)!=0;
            assert(mask.contains(x,y)==expected);hits+=mask.contains(x,y);++checked;
        }
        assert(hits==sourceHits[i]);
        const auto* saved=mask.bytes;
        assert(!pack.mask(ids[i],1,mask)&&mask.bytes==saved);
        assert(!pack.mask(9999,0,mask)&&mask.bytes==saved);
    }
    assert(!Pack(nullptr,0).valid());
    for(std::size_t size=0;size<bytes.size();++size)assert(!Pack(bytes.data(),size).valid());
    for(unsigned offset:{0u,4u,8u,16u,20u,32u,36u,40u}){
        auto bad=bytes;for(unsigned j=0;j<4;++j)bad[offset+j]=255;
        assert(!Pack(bad.data(),bad.size()).valid());
    }
    auto duplicate=bytes;for(unsigned i=0;i<8;++i)duplicate[44+i]=duplicate[24+i];
    assert(!Pack(duplicate.data(),duplicate.size()).valid());
    auto trailing=bytes;trailing.push_back(0);assert(!Pack(trailing.data(),trailing.size()).valid());
    unsigned transformed=0;
    unsigned axisTransformed=0;
    Pack::Mask circle;assert(pack.mask(597,0,circle));
    for(unsigned i=0;i<4096;++i){
        const float scales[]={-2.3f,-1.f,-0.125f,0.3f,1.f,2.f};
        const float sx=scales[i%6],sy=scales[(i/6)%6],x=float(int(i%37)-18)*0.31f,y=float(int(i%29)-14)*0.17f;
        const float c=float(std::cos(-double(i)*0.017)),s=float(std::sin(-double(i)*0.017));
        downwell::RunnerMaskTransform transform;
        assert(transform.prepare(packed(x),packed(y),packed(sx),packed(sy),packed(c),packed(s),24,24));
        const float ax=sx<0?x-1.f:x,ay=sy<0?y-1.f:y,rx=1.f/sx,ry=1.f/sy;
        for(int wx:{-19,-1,0,23})for(int wy:{-25,-1,0,21}){
            const float dx=float(wx)-ax,dy=float(wy)-ay;
            float expectedX=(dy*s)+(dx*c),expectedY=(dy*c)-(dx*s);
            expectedX=expectedX*rx;expectedY=expectedY*ry;
            expectedX=expectedX+24.f;expectedY=expectedY+24.f;
            Real actualX,actualY;assert(transform.sample(wx,wy,actualX,actualY));
            assert(unpacked(actualX)==expectedX&&unpacked(actualY)==expectedY);
            bool hit=true;assert(transform.contains(circle,wx,wy,hit));
            const bool expected=expectedX>=0&&expectedY>=0&&expectedX<48&&expectedY<48&&
                circle.contains(int(expectedX),int(expectedY));
            assert(hit==expected);++transformed;
        }
        assert(transform.prepareAxisAligned(packed(x),packed(y),packed(sx),packed(sy),24,24));
        for(int wx:{-19,-1,0,23})for(int wy:{-25,-1,0,21}){
            float expectedX=float(wx)-ax,expectedY=float(wy)-ay;
            expectedX=expectedX*rx;expectedY=expectedY*ry;
            expectedX=expectedX+24.f;expectedY=expectedY+24.f;
            Real actualX,actualY;assert(transform.sample(wx,wy,actualX,actualY));
            assert(unpacked(actualX)==expectedX&&unpacked(actualY)==expectedY);
            bool hit=true;assert(transform.contains(circle,wx,wy,hit));
            assert(hit==circle.contains(int(expectedX),int(expectedY)));++axisTransformed;
        }
        assert(!transform.prepare({}, {},{},packed(1),packed(1),{},0,0));
        Real untouched{0x3f800000},other=untouched;
        assert(!transform.sample(0,0,untouched,other)&&untouched.bits==0x3f800000&&other.bits==0x3f800000);
    }
    // Same algebra with zero rotation is NOT equivalent at the negative edge.
    const std::uint8_t filled[]={0x80};Pack::Mask onePixel{filled,1,1,1};
    downwell::RunnerMaskTransform edge;
    assert(edge.prepareAxisAligned(packed(1),{},packed(2),packed(2),0,0));
    bool hit=false;assert(edge.contains(onePixel,0,0,hit)&&hit); // -0.5 -> 0
    assert(edge.contains(onePixel,-1,0,hit)&&!hit); // -1 stays outside
    assert(edge.prepare(packed(1),{},packed(2),packed(2),packed(1),{},0,0));
    assert(edge.contains(onePixel,0,0,hit)&&!hit); // Rotated path rejects -0.5
    std::printf("Collision mask pack: %u all-resource and %u precise pixel/boundary checks, all truncations and malformed metadata passed\n",allPixels,checked);
    std::printf("Rotated mask transform: %u fractional/mirrored samples passed\n",transformed);
    std::printf("Scaled near-zero mask transform: %u fractional/mirrored samples passed\n",axisTransformed);
}
