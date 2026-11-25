#include "core/WhiteJellyFixedStep.h"
#include "core/WhiteJellyStep.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/RunnerDirectionValue.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The compact world's Q16 white jelly against WhiteJellyStep, its VM form,
// driven side by side through the same walls, target path and hits: the
// published pixels stay within one of each other and rarely differ.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"White jelly fixed check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
using F=downwell::RunnerImageValue;
namespace {
constexpr int mask=174;
// A walled shaft 32..288 wide with a few ledges, on a 16-pixel grid.
bool solid(int x,int y){
    if(x<32||x>=288)return true;
    const int cx=x>>4,cy=y>>4;
    return (cy%9==4&&cx%5!=0)||(cy%13==7&&cx>=9&&cx<=12);
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
struct World {
    int targetX=160,targetY=200;
    unsigned meetings=0,hits=0;
};
struct CommonOps {
    World& world;
    bool playerX(N& v){v=N::fromInteger(world.targetX);return true;}
    bool playerY(N& v){v=N::fromInteger(world.targetY);return true;}
    bool enemyPlayerX(N& v){return playerX(v);}
    bool enemyPlayerY(N& v){return playerY(v);}
    bool pointDirection(N x,N y,N tx,N ty,N& v){
        downwell::Fixed16 ax,ay,bx,by;
        if(!downwell::Fixed16::fromBinary64(x,ax)||!downwell::Fixed16::fromBinary64(y,ay)||
           !downwell::Fixed16::fromBinary64(tx,bx)||!downwell::Fixed16::fromBinary64(ty,by))return false;
        v=downwell::Fixed16::raw(downwell::FixedTrig::pointDirection(bx.v-ax.v,by.v-ay.v)).binary64();return true;
    }
    bool direction(N wanted,N& accepted){
        F value;if(!downwell::RunnerDirectionValue::normalize(wanted,value))return false;
        accepted=value.promoted();return true;
    }
    bool position(bool,F value,F& accepted){accepted=value;return true;}
    bool imageSpeed(F value,F& accepted){accepted=value;return true;}
    bool alarm(int,int){return true;}
    bool imageIndex(int){return true;}
    bool sound(int,int,int,int,const char*){return true;}
    bool lengthdir(bool vertical,N speed,N angle,N& v){return downwell::WhiteJellyStep::movementComponent(vertical,speed,angle,v);}
    bool meeting(N x,N y,int object,bool& hit){
        downwell::ContactBounds b;
        if(object!=57||!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,b))return false;
        hit=overlaps(b);return true;
    }
    bool enemyPlayerQ16(bool vertical,std::int32_t& v){v=(vertical?world.targetY:world.targetX)*65536;return true;}
    bool meetingQ16(std::int32_t x,std::int32_t y,int object,bool& hit){
        downwell::ContactBounds b;
        if(object!=57||!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,x,y,b))return false;
        ++world.meetings;hit=overlaps(b);world.hits+=hit;return true;
    }
    bool enemyDeath(){return true;}
    bool smoke(N,N,int,int){return true;}
    bool movingFx(N,N,int,N,N,N){return true;}
    bool flash(N,N,int,int,int){return true;}
    bool currency(int){return true;}
    bool destroy(){return true;}
    bool sprite(int){return true;}
};
int pixel(F v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
}
int main(){
    // Q16 mask bounds equal the binary32 ones on whole and half pixels.
    for(int y=-40;y<=600;++y)for(int half=0;half<2;++half){
        downwell::ContactBounds a,b;
        const N real=N::fromInteger(2*y+half);N position=real;
        if(!position.zero())position.high-=1u<<20;
        check(downwell::CurrencyCollisionBounds::calculateMask(mask,position,position,a));
        check(downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,y*65536+half*32768,y*65536+half*32768,b));
        check(a.left==b.left&&a.right==b.right&&a.top==b.top&&a.bottom==b.bottom);
    }
    unsigned steps=0,pixelDifferences=0,headingDifferences=0;long worst=0;
    World totals;
    for(unsigned seed=0;seed<64;++seed){
        World world;
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),path(seed+1000);
        downwell::WhiteJellyStep reference;downwell::WhiteJellyFixedStep fixed;
        CommonOps referenceOps{world},fixedOps{world};
        const int sx=48+16*int(path.irandomSmall(14)),sy=16*int(path.irandomSmall(20));
        F px,py;check(px.assignInteger(sx)&&py.assignInteger(sy));
        check(reference.create(px,py,referenceRandom,referenceOps)&&fixed.create(px,py,fixedRandom,fixedOps));
        for(unsigned tick=0;tick<600;++tick){
            // The target wanders the shaft; now and then a shot lands.
            if(tick%40==0){world.targetX=48+int(path.irandomSmall(224));world.targetY=sy-64+int(path.irandomSmall(400));}
            if(tick%97==50){
                const N angle=N::fromInteger(int(path.irandomSmall(359)));
                reference.hit=fixed.hit=true;reference.hitDirection=fixed.hitDirection=angle;
            }
            if(tick%97==55)reference.hitStun=fixed.hitStun=false; // Alarm 1
            check(reference.step(false,referenceRandom,referenceOps)&&fixed.step(false,fixedRandom,fixedOps));
            {
                downwell::Fixed16 ex,ey;
                check(downwell::Fixed16::fromBinary64(reference.xx,ex)&&downwell::Fixed16::fromBinary64(reference.yy,ey));
                const long dx=std::labs(long(fixed.x16)-ex.v),dy=std::labs(long(fixed.y16)-ey.v);
                if(dx>worst)worst=dx;
                if(dy>worst)worst=dy;
            }
            const int rx=pixel(reference.x),ry=pixel(reference.y),fx=pixel(fixed.x),fy=pixel(fixed.y);
            check(std::abs(rx-fx)<=1&&std::abs(ry-fy)<=1);
            pixelDifferences+=(rx!=fx)+(ry!=fy);
            downwell::Fixed16 heading;check(downwell::Fixed16::fromBinary64(reference.direction,heading));
            headingDifferences+=std::labs(long(heading.v)-fixed.direction16)>65536;
            check(referenceRandom.drawCount()==fixedRandom.drawCount());
            ++steps;
        }
        totals.meetings+=world.meetings;totals.hits+=world.hits;
    }
    std::printf("White jelly Q16: %u steps, %u meetings (%u walls), positions within %.4f px of WhiteJellyStep, "
                "%u of %u published pixels differ by one, %u headings over a degree apart\n",
                steps,totals.meetings,totals.hits,worst/65536.0,pixelDifferences,2*steps,headingDifferences);
    check(totals.hits*20>totals.meetings&&worst<=1024&&pixelDifferences*100<2*steps&&headingDifferences*100<steps);
}
