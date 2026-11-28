#include "core/CorpseParticle.h"
#include "core/CorpseEffects.h"
#include "core/DivideBinary64.h"
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
struct PixelQueries {
    bool wall(int x,int y,bool& blocked){
        downwell::ContactBounds b;
        if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,x*65536,y*65536,b))return false;
        blocked=overlaps(b);return true;
    }
    bool tile(int,int,bool& hit){hit=false;return true;}
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
};
template<class Q> struct DeathHooks {
    unsigned sprites=0,facings=0;
    bool birth(std::uint32_t,const downwell::CorpseAnimation&){return true;}
    Q queries(std::uint32_t,const downwell::CorpseAnimation&){return {};}
    bool sprite(std::uint32_t,const downwell::CorpseAnimation&){++sprites;return true;}
    bool facing(std::uint32_t,const downwell::CorpseAnimation&){++facings;return true;}
};
// scrDeadBody through createDeathQ16 against createFromDeath on the VM form:
// equal RNG, builtins (relocation included), sprite, facing, explosion,
// image speed and alarms; the speeds are the nearest Q16 of the VM's.
unsigned deathBirths(){
    unsigned relocated=0,births=0;
    const int sprites[]={165,337,172,158,179,281,291};
    for(unsigned seed=0;seed<4000;++seed){
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),place(seed+77);
        // Inside or near the pit's walls and ledges, so some births relocate.
        const int x=30+int(place.irandomSmall(260)),y=100+int(place.irandomSmall(120));
        const int damage=int(place.irandomSmall(30)),facing=place.chooseIndexSmall(2)?1:-1;
        const int speedX=int(place.irandomSmall(1200))-600; // Q8
        const int sprite=sprites[seed%7];const bool rip=seed%3==0;
        downwell::CorpseEffects<2,downwell::CorpseAnimation> reference;
        downwell::CorpseEffects<2,downwell::CorpseParticle> fixed;
        DeathHooks<Queries> rh;DeathHooks<PixelQueries> fh;
        downwell::DeadBodySource source;
        source.x=N::fromInteger(x);source.y=N::fromInteger(y);
        source.xsp=N::fromInteger(speedX);if(speedX)source.xsp.high-=8u<<20;
        source.imageXscale=N::fromInteger(facing);
        N impact;check(downwell::divideBinary64(N::fromInteger(damage),N::fromInteger(5),impact));
        check(reference.createFromDeath(source,sprite,impact,102,rip,referenceRandom,rh));
        check(fixed.createDeathQ16(x,y,sprite,damage,speedX*256,facing,102,rip,fixedRandom,fh));
        check(referenceRandom.drawCount()==fixedRandom.drawCount()&&rh.sprites==fh.sprites&&rh.facings==fh.facings);
        const auto* r=reference.get(1);auto* f=fixed.get(1);check(r&&f);
        check(pixel(r->x)==pixel(f->x)&&pixel(r->y)==pixel(f->y)&&r->spriteId==f->spriteId&&
              r->imageXscale.bits==f->imageXscale.bits&&r->explosionFx==f->explosionFx&&
              r->imageSpeed.bits==f->imageSpeed.bits&&r->hitEffect==f->hitEffect);
        for(unsigned a=0;a<4;++a)check(r->alarms.get(a)==f->alarms.get(a));
        downwell::Fixed16 vx,vy;
        check(downwell::Fixed16::fromBinary64(r->xsp,vx)&&downwell::Fixed16::fromBinary64(r->ysp,vy));
        check(f->xspUnits==downwell::CorpseParticle::speedUnits(vx.v)&&f->yspUnits==downwell::CorpseParticle::speedUnits(vy.v));
        check(f->x16==pixel(f->x)*65536&&f->y16==pixel(f->y)*65536&&f->fixed);
        relocated+=pixel(r->x)!=x||pixel(r->y)!=y;++births;
    }
    std::printf("Corpse Q16 deaths: %u scrDeadBody births equal to createFromDeath (%u relocated)\n",births,relocated);
    check(relocated>births/20);
    return births;
}
}
int main(){
    deathBirths();
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
            if(rip&&tick==120){reference.health={};fixed.hp=0;}
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
