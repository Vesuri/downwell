#include "core/MaskBoundsMemo.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
// The memo against calculateMask itself: a stream with many repeated keys
// (resting objects) and colliding ones, over the masks solidMeeting asks for.
using namespace downwell;
namespace {
std::uint32_t state=12345;
std::uint32_t next(){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
MotionBinary64 number(double d){std::uint64_t n;std::memcpy(&n,&d,8);return {std::uint32_t(n>>32),std::uint32_t(n)};}
}
int main(){
    const int masks[]={91,598,568,599,153,174,702,0,-1};
    struct Key{int mask;MotionBinary64 x,y;};
    Key recent[48];unsigned filled=0;
    MaskBoundsMemo memo;unsigned hits=0,failures=0,total=0;
    for(unsigned i=0;i<2000000;++i){
        Key k;
        if(filled&&next()%4){k=recent[next()%filled];}
        else{
            k.mask=masks[next()%9];
            const double x=double(int(next()%1200)-100)+(next()%3?0.0:double(next()%4096)/4096.0);
            const double y=double(int(next()%30000)-100)+(next()%3?0.0:double(next()%4096)/4096.0)+(next()%5?0.0:0.08);
            k.x=number(x);k.y=number(next()%50?y:-0.0);
            recent[filled<48?filled++:next()%48]=k;
        }
        ContactBounds a{},b{};
        const bool ra=memo.calculate(k.mask,k.x,k.y,a),rb=CurrencyCollisionBounds::calculateMask(k.mask,k.x,k.y,b);
        assert(ra==rb);
        if(!ra){++failures;continue;}
        assert(a.left==b.left&&a.right==b.right&&a.top==b.top&&a.bottom==b.bottom);
        ++total;
    }
    (void)hits;
    // Keys differing in one field that share a bucket: the second must not
    // take the first one's bounds.
    unsigned collisions=0;
    for(unsigned i=0;collisions<20000&&i<4000000;++i){
        Key a{masks[next()%7],number(double(next()%1000)+double(next()%256)/256.0),
              number(double(next()%20000)+double(next()%256)/256.0)};
        Key b=a;
        switch(next()%5){
        case 0:b.mask=int(next()%800);break;
        case 1:b.x.high^=1u<<(next()%20);break;
        case 2:b.x.low^=1u<<(next()%32);break;
        case 3:b.y.high^=1u<<(next()%20);break;
        default:b.y.low^=1u<<(next()%32);break;
        }
        if(b.mask==a.mask&&b.x.high==a.x.high&&b.x.low==a.x.low&&b.y.high==a.y.high&&b.y.low==a.y.low)continue;
        if(MaskBoundsMemo::index(a.mask,a.x,a.y)!=MaskBoundsMemo::index(b.mask,b.x,b.y))continue;
        ContactBounds ma{},mb{},da{},db{};
        const bool ra=memo.calculate(a.mask,a.x,a.y,ma),rb=memo.calculate(b.mask,b.x,b.y,mb);
        assert(ra==CurrencyCollisionBounds::calculateMask(a.mask,a.x,a.y,da));
        assert(rb==CurrencyCollisionBounds::calculateMask(b.mask,b.x,b.y,db));
        if(rb)assert(mb.left==db.left&&mb.right==db.right&&mb.top==db.top&&mb.bottom==db.bottom);
        if(ra&&rb&&(da.left!=db.left||da.right!=db.right||da.top!=db.top||da.bottom!=db.bottom))++collisions;
    }
    assert(collisions==20000);
    assert(failures>0&&total>0);
    // calculateMask's binary32 shortcut against its reference: whole and
    // fractional positions, signs, binade edges, NaN-free random bits.
    unsigned shortcut=0;
    for(unsigned i=0;i<3000000;++i){
        const int mask=masks[next()%9];
        double x,y;
        switch(next()%4){
        case 0:x=double(int(next()%40000)-200)+double(next()%65536)/65536.0;break;
        case 1:x=double(1u<<(next()%23))-double(next()%8)/8.0;break;
        case 2:x=double(next()%64)/double(1+next()%64);break;
        default:{std::uint64_t b=(std::uint64_t(next())<<32)|next();b&=~(std::uint64_t(0x7ff)<<52);
                 b|=std::uint64_t(1000+next()%40)<<52;std::memcpy(&x,&b,8);}
        }
        y=next()%7?double(next()%30000)+double(next()%4096)/4096.0:-double(next()%16)/4.0;
        const auto kx=number(x),ky=number(y);
        ContactBounds a{},b{};
        const bool ra=CurrencyCollisionBounds::calculateMask(mask,kx,ky,a),
                   rb=CurrencyCollisionBounds::calculateMaskReference(mask,kx,ky,b);
        assert(ra==rb);
        if(ra){assert(a.left==b.left&&a.right==b.right&&a.top==b.top&&a.bottom==b.bottom);++shortcut;}
    }
    std::printf("calculateMask shortcut: %u bounds equal the binary32 reference\n",shortcut);
    std::printf("Mask bounds memo: %u bounds equal calculateMask (%u failures passed through), %u colliding keys with different bounds\n",total,failures,collisions);
}
