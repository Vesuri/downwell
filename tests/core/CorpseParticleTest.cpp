#include "core/CorpseParticle.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The compact world's Q16 corpse against CorpseAnimation, its VM form,
// stepped side by side in a walled pit with ledges: published pixels stay
// within one and the bounce, landing and Rest in Pieces outcomes agree.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Corpse particle check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
namespace {
constexpr int mask=599;
bool solid(int x,int y){
    if(x<40||x>=280||y>=200)return true;
    return (y>>4)==9&&((x>>4)%4==1);
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
int pixel(downwell::RunnerImageValue v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
struct Queries {
    bool wall(N x,N y,bool& blocked){
        downwell::ContactBounds b;
        if(!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,b))return false;
        blocked=overlaps(b);return true;
    }
    bool tile(N,N,bool& hit){hit=false;return true;}
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
};
struct Common {
    int areaValue=1,rip=0;unsigned created=0,destroyed=0;long createdX=0,createdY=0;
    int area(){return areaValue;}
    int restInPieces(){return rip;}
    void destroy(){++destroyed;}
};
struct ReferenceEvents : Common {
    unsigned meetings=0,hits=0;
    bool meeting(N x,N y,bool& hit){
        downwell::ContactBounds b;
        if(!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,b))return false;
        hit=overlaps(b);return true;
    }
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
    bool create(int,N x,N y){
        std::int32_t px,py;if(!x.roundPixel(px)||!y.roundPixel(py))return false;
        ++created;createdX+=px;createdY+=py;return true;
    }
};
struct FixedEvents : Common {
    unsigned meetings=0,hits=0;
    bool meeting(std::int32_t x,std::int32_t y,bool& hit){
        downwell::ContactBounds b;
        if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,x,y,b))return false;
        hit=overlaps(b);++meetings;hits+=hit;return true;
    }
    bool create(int,std::int32_t px,std::int32_t py){++created;createdX+=px;createdY+=py;return true;}
};
}
int main(){
    unsigned births=0,steps=0,pixelDifferences=0,stateDifferences=0,meetings=0,hits=0,grounded=0,explosions=0;
    for(unsigned seed=0;seed<512;++seed){
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),place(seed+9000);
        const N x=N::fromInteger(60+int(place.irandomSmall(200))),y=N::fromInteger(40+int(place.irandomSmall(140)));
        downwell::CorpseAnimation reference;downwell::CorpseParticle fixed;
        const bool rip=seed%5==0;
        Queries rq,fq;
        check(reference.create(x,y,rip,referenceRandom,rq)&&fixed.create(x,y,rip,fixedRandom,fq));
        check(referenceRandom.drawCount()==fixedRandom.drawCount());
        if(seed%3==0){
            // scrDeadBody's impact speeds.
            const N vx=N::fromInteger(int(place.irandomSmall(10))-5),vy=N::fromInteger(-int(place.irandomSmall(4)));
            reference.xsp=fixed.xsp=vx;reference.ysp=fixed.ysp=vy;
        }
        ++births;
        ReferenceEvents re;FixedEvents fe;
        re.areaValue=fe.areaValue=seed%4==3?3:1;
        re.rip=fe.rip=rip?1+int(seed%2):0;
        for(unsigned tick=0;tick<200&&!reference.destroyRequested;++tick){
            if(rip&&tick==8)reference.active=fixed.active=true;
            if(rip&&tick==120)reference.health=fixed.health={};
            check(reference.step(false,re)&&fixed.step(false,fe));
            ++steps;
            const int rx=pixel(reference.x),ry=pixel(reference.y),fx=pixel(fixed.x),fy=pixel(fixed.y);
            check(std::abs(rx-fx)<=1&&std::abs(ry-fy)<=1);
            pixelDifferences+=(rx!=fx)+(ry!=fy);
            stateDifferences+=(reference.grounded!=fixed.grounded)+(reference.bouncing!=fixed.bouncing);
            check(reference.destroyRequested==fixed.destroyRequested&&reference.flash==fixed.flash);
        }
        check(re.created==fe.created&&re.destroyed==fe.destroyed);
        check(std::labs(re.createdX-fe.createdX)<=long(re.created)&&std::labs(re.createdY-fe.createdY)<=long(re.created));
        meetings+=fe.meetings;hits+=fe.hits;grounded+=fixed.grounded;explosions+=fe.created;
        long viewTop=long(place.irandomSmall(400))*256;
        Common ends;
        struct {Common& c;void destroy(){c.destroy();}} rEnd{ends},fEnd{ends};
        check(reference.endStep(true,N::fromInteger(int(viewTop/256)),rEnd)&&fixed.endStep(true,viewTop,fEnd));
        check(reference.destroyRequested==fixed.destroyRequested);
    }
    std::printf("Corpse Q16: %u births, %u steps, %u meetings (%u solid), %u of %u published pixels differ by one, "
                "%u bounce/landing state differences, %u landed, %u Rest in Pieces explosions\n",
                births,steps,meetings,hits,pixelDifferences,2*steps,stateDifferences,grounded,explosions);
    check(hits*10>meetings&&grounded>births/2&&explosions>0&&pixelDifferences*100<2*steps&&stateDifferences*1000<steps);
}
