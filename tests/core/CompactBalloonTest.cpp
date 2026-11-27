#include "core/CompactUpgrades.h"
#include "core/DivideBinary64.h"
#include "core/PlayerHorizontalBinary64.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
// The Q16 Heart Balloon against its binary64 Step (decoyBalloon), kept here
// as the oracle: random held walks and loose rises stay within a pixel of
// the VM's position, and the held art agrees except at the pull bounds.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Balloon check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using R=downwell::MotionBinary64;
namespace {
R real(double v){std::uint64_t b;std::memcpy(&b,&v,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
double value(R v){const std::uint64_t b=(std::uint64_t(v.high)<<32)|v.low;double d;std::memcpy(&d,&b,8);return d;}
R negative(R v){if(!v.zero())v.high^=0x80000000u;return v;}
R magnitude(R v){v.high&=0x7fffffffu;return v;}
bool less(R a,R c){return downwell::PlayerHorizontalBinary64::less(a,c);}
struct Reference {
    R xx{},yy{},ysp{};
    int sprite=526,frame=-1;
    bool held(R plx,R ply,int climb,int stringLength){
        R pointy=ply,dx=plx,half,xMove,dy,step,limit=ply;
        if(!pointy.add(R::fromInteger(-climb))||!dx.add(negative(xx))||
           !downwell::divideBinary64(magnitude(dx),R::fromInteger(2),half)||!pointy.add(half)||
           !downwell::divideBinary64(dx,R::fromInteger(stringLength/2),xMove)||!xx.add(xMove)||
           !limit.add(R::fromInteger(-stringLength))||!dy.add(pointy)||!dy.add(negative(yy))||
           !downwell::divideBinary64(dy,R::fromInteger(less(yy,limit)?4:10),step)||!yy.add(step))
            return false;
        if(less({0x3fc99999u,0x9999999au},magnitude(xMove))){
            sprite=700;
            const R pull{0x3ff33333u,0x33333333u};
            frame=less({},xMove)?(less(pull,xMove)?1:0):(less(xMove,negative(pull))?3:2);
        }else{sprite=701;frame=-1;}
        return true;
    }
    bool loose(bool hit){
        if(hit)ysp={};
        else{
            const R maximum{0xbfe99999u,0x9999999au};
            if(less(maximum,ysp)){if(!ysp.add({0xbf947ae1u,0x47ae147bu}))return false;}
            else ysp=maximum;
        }
        return yy.add(ysp);
    }
};
int pixel(R v){std::int32_t p;check(v.roundPixel(p));return p;}
}
int main(){
    double worst=0;
    unsigned steps=0,poses=0,poseDiffs=0,pixelDiffs=0;
    for(unsigned seed=0;seed<300;++seed){
        downwell::GameMakerRandom random(seed);
        Reference reference;downwell::CompactBalloonRecord fixed;
        const int x0=48+int(random.irandomSmall(200)),y0=int(random.irandomSmall(4000));
        reference.xx=R::fromInteger(x0);reference.yy=R::fromInteger(y0);
        fixed.xx=x0*65536;fixed.yy=y0*65536;
        long px=long(x0-240)*256,py=long(y0-176+30)*256,vx=0,vy=0;
        for(unsigned tick=0;tick<1500;++tick){
            const unsigned phase=(tick/300)%2;
            if(phase==0){ // held: the player wanders and falls in Q8
                if(random.irandomSmall(20)==0)vx=long(random.irandomSmall(1024))-512;
                vy+=int(random.irandomSmall(64));if(vy>2048)vy=-768;
                px+=vx;py+=vy;if(px< -40*256||px>40*256)vx=-vx;
                const int climb=int(random.irandomSmall(24)),length=random.irandomSmall(4)==0?10:20;
                fixed.stringLength=length;
                std::int32_t xMove;
                check(reference.held(real((px+240*256)/256.0),real((py+176*256)/256.0),climb>20?length+1:climb,length));
                check(fixed.ease(std::int32_t(px)*256+240*65536,std::int32_t(py)*256+176*65536,climb>20?length+1:climb,xMove)&&fixed.pose(xMove));
                ++poses;
                std::int32_t frame=-1;
                if(fixed.sprite==700)check(fixed.imageIndex.roundPixel(frame));
                poseDiffs+=fixed.sprite!=reference.sprite||(fixed.sprite==700&&frame!=reference.frame);
            }else{
                const bool hit=random.irandomSmall(30)==0;
                check(reference.loose(hit)&&fixed.rise(hit));
            }
            fixed.publish();
            const int dx=fixed.x-pixel(reference.xx),dy=fixed.y-pixel(reference.yy);
            if(!(dx>=-1&&dx<=1&&dy>=-1&&dy<=1))std::fprintf(stderr,"seed %u tick %u ref %.6f %.6f q16 %.6f %.6f\n",seed,tick,value(reference.xx),value(reference.yy),fixed.xx/65536.0,fixed.yy/65536.0);
            check(dx>=-1&&dx<=1&&dy>=-1&&dy<=1);
            const double ex=std::abs(value(reference.xx)*65536-fixed.xx),ey=std::abs(value(reference.yy)*65536-fixed.yy);
            if(ex>worst)worst=ex;if(ey>worst)worst=ey;
            if(!(ex<64&&ey<64))std::fprintf(stderr,"seed %u tick %u drift %.1f %.1f\n",seed,tick,ex,ey);
            check(ex<64&&ey<64);
            pixelDiffs+=dx!=0||dy!=0;
            ++steps;
        }
    }
    std::printf("Balloon Q16: %u steps within a pixel of binary64 (%u differ by one), %u held poses (%u differ), worst drift %.1f Q16\n",
        steps,pixelDiffs,poses,poseDiffs,worst);
    check(pixelDiffs*1000<steps&&poseDiffs*1000<poses);
}
