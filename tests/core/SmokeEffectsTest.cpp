#include "core/SmokeEffects.h"
#include "core/GameMakerRandom.h"
#include "core/SmokeEffect.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
bool nativeBoxSmoke(downwell::SmokeEffects<32>& effects,downwell::Fixed16 x,
                    downwell::Fixed16 y,downwell::GameMakerRandom& random){
    return effects.boxBurst(x,y,random);
}
#else
#include <vector>
#include <cstdlib>
#include <cstdio>
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Smoke owner check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
int main(){
    unsigned births=0;
    long worst=0;unsigned lifeDifferences=0,frameSteps=0,frameDifferences=0,pixelSteps=0,pixelDifferences=0,escapeDifferences=0;
    for(unsigned seed=0;seed<128;++seed)for(bool box:{false,true}){
        downwell::SmokeEffects<16> store;
        downwell::GameMakerRandom rng(seed),reference(seed);
        const auto prior=store.beginPhase();
        check(box?store.boxBurst({}, {},rng):store.skeletonBurst({}, {},rng));
        const unsigned count=(box?1:4)+reference.irandom(box?0:2);
        std::vector<downwell::SmokeEffect> expected(count);
        for(auto& effect:expected){
            const int y=int(reference.nextU32()%5)-2,x=int(reference.nextU32()%5)-2;
            check(effect.create(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(y),reference));
        }
        check(store.size()==count && store.advanceAnimation(prior) && store.step(prior,rng));
        for(unsigned tick=0;tick<100;++tick){
            const auto phase=store.beginPhase();
            check(store.advanceAnimation(phase));
            for(auto& effect:expected)if(!effect.destroyRequested)check(effect.advanceImage());
            // Explicit newest-first reference, independent of reusable slots.
            check(store.step(phase,rng));
            for(auto it=expected.rbegin();it!=expected.rend();++it)if(!it->destroyRequested)
                check(it->step(reference,[](auto speed,auto angle,auto& x,auto& y){
                    return downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrig);
                }));
            unsigned live=0;
            for(unsigned i=0;i<count;++i){
                const auto* actual=store.get(i+1);const auto& wanted=expected[i];
                // The Q16 animation position may end a smoke one tick
                // before or after the binary32 timeline.
                if(bool(actual)!=!wanted.destroyRequested){++lifeDifferences;live+=actual!=nullptr;continue;}
                if(actual){++live;
                    // Q16 motion against the VM form: positions within 1/64
                    // pixel, published pixels at most one apart.
                    downwell::Fixed16 x,y;std::int32_t px,py;
                    check(downwell::Fixed16::fromBinary64(wanted.xx,x) && downwell::Fixed16::fromBinary64(wanted.yy,y));
                    const long ex=std::labs(long(actual->xx)-x.v),ey=std::labs(long(actual->yy)-y.v);
                    if(ex>worst)worst=ex;
                    if(ey>worst)worst=ey;
                    check(ex<=1024 && ey<=1024);
                    check(wanted.x.promoted().roundPixel(px) && wanted.y.promoted().roundPixel(py));
                    check(std::abs(actual->x-px)<=1 && std::abs(actual->y-py)<=1);
                    ++pixelSteps;pixelDifferences+=(actual->x!=px)+(actual->y!=py);
                    check(actual->allSet==wanted.allSet);
                    // The escape test reads published pixels: only a pixel
                    // that rounded the other way can move it by a tick.
                    if(actual->noCollision!=wanted.noCollision){
                        check(actual->x!=px||actual->y!=py);++escapeDifferences;
                    }
                    const unsigned exponent=(wanted.imageIndex.bits>>23)&255u;
                    const unsigned frame=exponent<127?0:((wanted.imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
                    check(actual->drawFrame()==frame||actual->drawFrame()+1==frame||actual->drawFrame()==frame+1);
                    ++frameSteps;frameDifferences+=actual->drawFrame()!=frame;
                    // The Q16 rate: a product of two nearest-Q16 samples.
                    downwell::Fixed16 rate;check(downwell::Fixed16::fromBinary32(wanted.imageSpeed.bits,rate));
                    check(std::labs(long(actual->rate16)-rate.v)<=4);}
            }
            check(store.size()==live && rng.drawCount()==reference.drawCount());
            for(unsigned i=0;i<16;++i)check(rng.stateWord(i)==reference.stateWord(i));
        }
        check(store.size()==0);births+=count;
    }
    downwell::SmokeEffects<2> reuse;downwell::GameMakerRandom rng(42);
    const auto a=reuse.create({}, {},rng),b=reuse.create({}, {},rng);
    const auto before=reuse.beginPhase();const auto draws=rng.drawCount();
    check(!reuse.create({}, {},rng) && draws==rng.drawCount());
    reuse.get(a)->destroyRequested=true;reuse.retireRequested();
    const auto c=reuse.create({}, {},rng);check(c>b && !reuse.get(a));
    check(reuse.step(before,rng) && reuse.get(b)->allSet && !reuse.get(c)->allSet);
    std::vector<unsigned> order;reuse.visitNewest([&](unsigned handle,const auto&){order.push_back(handle);});
    check(order==std::vector<unsigned>({c,b}));
    const auto stale=reuse.beginPhase();reuse.clear();
    const auto d=reuse.create({}, {},rng);check(d>c && !reuse.get(b) && !reuse.get(c));
    check(reuse.advanceAnimation(stale) && reuse.step(stale,rng) && !reuse.get(d)->allSet);
    reuse.get(d)->destroyRequested=true;
    const auto beforeRetirement=rng.drawCount();
    check(reuse.advanceAnimation(reuse.beginPhase()) && reuse.size()==0);
    check(reuse.step(reuse.beginPhase(),rng) && rng.drawCount()==beforeRetirement);
    downwell::SmokeEffects<4> collisions;
    const auto older=collisions.create(downwell::Fixed16::fromInteger(20),
        downwell::Fixed16::fromInteger(30),rng);
    const auto phase=collisions.beginPhase();
    const auto newer=collisions.create({}, {},rng);
    collisions.get(newer)->noCollision=false;
    unsigned queries=0;
    // nocol skips querying, and post-cutoff births cannot receive this phase.
    check(collisions.collideWalls(phase,[&](auto,auto){++queries;return true;}));
    check(!queries);
    auto& smoke=*collisions.get(older);
    smoke.noCollision=false;
    smoke.xx=999*65536; // NOT the collision coordinate
    const auto oldSpeed=smoke.speed;const auto oldXsp=smoke.xsp,oldYsp=smoke.ysp;
    const auto oldRate=smoke.rate16;
    const auto collisionDraws=rng.drawCount();
    auto expectedSpeed=oldSpeed;
    for(unsigned i=0;i<3;++i)expectedSpeed=downwell::Fixed16::multiply(expectedSpeed,downwell::Fixed16::raw(13107));
    check(collisions.collideWalls(phase,[&](auto bounds,auto emit){
        ++queries;check(bounds.left==18 && bounds.right==22 && bounds.top==29 && bounds.bottom==32);
        return emit() && emit() && emit();
    }));
    check(queries==1 && smoke.speed==expectedSpeed && smoke.xsp==oldXsp &&
        smoke.ysp==oldYsp && smoke.rate16==oldRate && rng.drawCount()==collisionDraws);
    check(!collisions.collideWalls(phase,[](auto,auto){return false;}));
    smoke.destroyRequested=true;
    check(collisions.collideWalls(phase,[](auto,auto){check(false);return true;}));
    std::printf("Smoke owner: %u births, phase exclusion, retirement, capacity and stable reuse passed; "
                "Q16 motion within %.5f px of the VM form, %u of %u published pixels differ by one, "
                "%u escapes a tick apart; Q16 animation: %u of %u frames differ by one, %u lifetimes a tick apart\n",
                births,worst/65536.0,pixelDifferences,2*pixelSteps,escapeDifferences,frameDifferences,frameSteps,lifeDifferences);
    check(pixelDifferences*1000<2*pixelSteps && escapeDifferences*100<births &&
          frameDifferences*100<frameSteps && lifeDifferences*100<births);
}
#endif
