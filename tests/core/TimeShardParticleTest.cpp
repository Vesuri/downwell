#include "core/TimeShardBurst.h"
#include "core/TimeShardParticle.h"
#include "core/RunnerPointDirection.h"
#include "core/PortableAtan.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrigNarrow.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
// The compact world's Q16 shards against TimeShardAnimation, their VM form:
// a burst from emitFixed beside emit (same RNG draws, velocities within
// 1/256 pixel), then both stepped side by side in a walled pit while their
// RNG streams agree. Published pixels stay within one, hits (which draw
// RNG) rarely move and image_index (Q27 after the first advance) matches.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Time shard particle check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
namespace {
constexpr int mask=91;
bool solid(int x,int y){
    if(x<40||x>=280||y>=200)return true;
    return (y>>4)==8&&((x>>4)%4==1);
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
int pixel(downwell::RunnerImageValue v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
std::int32_t q16(N v){downwell::Fixed16 f;checkAt(downwell::Fixed16::fromBinary64(v,f),__LINE__);return f.v;}
}
int main(){
    {   // playerContact from signs against the binary64 speeds' form.
        unsigned checked=0;
        const N speeds[]={N{},N{0x80000000u,0},N::fromInteger(3),N::fromInteger(-2),N{0x3f700000u,0},N{0xbf700000u,0}};
        const int signs[]={0,0,1,-1,1,-1};
        for(unsigned seed=0;seed<64;++seed)for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b){
            downwell::TimeShardParticle viaReal,viaSign;
            downwell::GameMakerRandom r1(seed),r2(seed);
            check(viaReal.playerContact(speeds[a],speeds[b],r1)&&viaSign.playerContact(signs[a],signs[b],r2));
            check(viaReal.xsp==viaSign.xsp&&viaReal.ysp==viaSign.ysp&&viaReal.noTouch&&viaSign.noTouch&&
                  viaReal.alarm2==viaSign.alarm2&&r1.nextU32()==r2.nextU32());
            ++checked;
        }
        std::printf("Time shard contact: %u sign contacts equal to the binary64 speeds\n",checked);
    }
    // random(7) narrowed to binary32: the native rounding against the VM's
    // binary64 product and its narrowing, on edge words and a stride.
    {
        unsigned checked=0;
        const auto one=[&](std::uint32_t word){
            N index;check(downwell::randomBinary64(word,N::fromInteger(7),index));
            downwell::RunnerImageValue reference;check(reference.assign(index));
            check(downwell::RunnerImageValue::randomSevenths(word)==reference.bits);++checked;
        };
        for(std::uint32_t w=0;w<4096;++w){one(w);one(0xffffffffu-w);one(0x80000000u+w-2048);}
        for(std::uint64_t w=0;w<=0xffffffffu;w+=65521)one(std::uint32_t(w));
        std::printf("Time shard random(7): %u words narrow as the VM does\n",checked);
    }
    // Q16 mask bounds: one rounding per axis plus the whole-pixel margins,
    // against rounding every edge, including exact halves on both parities.
    {
        unsigned checked=0;
        for(int mask:{91,585,598,599,49})for(std::int32_t whole=-40;whole<40;++whole)
            for(std::int32_t fraction:{0,1,0x4000,0x7fff,0x8000,0x8001,0xffff}){
                const std::int32_t v=whole*65536+fraction;
                downwell::ContactBounds b;if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,v,v-3*65536,b))continue;
                const auto l=downwell::contactSpriteBounds(mask);
                const auto r=[](std::int32_t q){return int(downwell::Fixed16::roundQ16(q));};
                check(b.left==r(v+l.left*65536)&&b.right==r(v+l.right*65536)&&
                      b.top==r(v-3*65536+l.top*65536)&&b.bottom==r(v-3*65536+l.bottom*65536));++checked;
            }
        std::printf("Q16 mask bounds: %u positions round as their edges do\n",checked);check(checked>=3*80*7);
    }
    unsigned bursts=0,shards=0,steps=0,pixelDifferences=0,streamsParted=0,meetings=0,hits=0,timed=0;
    std::int32_t worstVelocity=0;
    for(unsigned seed=0;seed<512;++seed){
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),place(seed+9000);
        const int px=60+int(place.irandomSmall(200)),py=100+int(place.irandomSmall(90));
        const bool field=seed%3!=0;
        const int fieldX=60+int(place.irandomSmall(200)),fieldY=40+int(place.irandomSmall(150));
        const int vx=int(place.irandomSmall(6))-3,vy=int(place.irandomSmall(10))-5;
        downwell::RunnerImageValue rx,ry;check(rx.assign(N::fromInteger(px))&&ry.assign(N::fromInteger(py)));
        const auto lookup=[&](downwell::RunnerImageValue,downwell::RunnerImageValue,std::int32_t& id,N& fx,N& fy){
            id=field?1:-4;fx=N::fromInteger(fieldX);fy=N::fromInteger(fieldY);return true;
        };
        downwell::TimeShardEffects<3> reference;downwell::TimeShardEffects<3,downwell::TimeShardParticle> fixed;
        downwell::TimeShardBurst referenceBurst,fixedBurst;
        check(referenceBurst.emit(rx,ry,N::fromInteger(vx),N::fromInteger(vy),false,referenceRandom,reference,lookup,
            [](auto ax,auto ay,auto bx,auto by,auto& angle){
                return downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,angle,downwell::portableAtan);
            },[](auto speed,auto angle,bool vertical,auto& result){
                N x,y;if(!downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrigNarrowed))return false;
                result=vertical?y:x;return true;
            }));
        check(fixedBurst.emitFixed(rx,ry,vx*65536,vy*65536,false,fixedRandom,fixed,
            [&](downwell::RunnerImageValue,downwell::RunnerImageValue,std::int32_t& id,std::int32_t& fx,std::int32_t& fy){
                id=field?1:-4;fx=fieldX;fy=fieldY;return true;}));
        check(referenceRandom.drawCount()==fixedRandom.drawCount()&&reference.size()==3&&fixed.size()==3);
        ++bursts;
        // Both owners hold the newest first.
        downwell::TimeShardAnimation* r[3];downwell::TimeShardParticle* f[3];unsigned n=0;
        reference.visitNewest([&](auto h,const auto&){r[n++]=reference.get(h);});n=0;
        fixed.visitNewest([&](auto h,const auto&){f[n++]=fixed.get(h);});
        for(unsigned i=0;i<3;++i){
            const std::int32_t dx=std::abs(q16(r[i]->xsp)-f[i]->xsp),dy=std::abs(q16(r[i]->ysp)-f[i]->ysp);
            worstVelocity=std::max(worstVelocity,std::max(dx,dy));
            check(dx<=256&&dy<=256&&r[i]->alarm0==f[i]->alarm0&&r[i]->imageIndex.bits==f[i]->imageIndex.bits);
            ++shards;
        }
        // Step them side by side (one at a time keeps both RNG streams aligned).
        for(unsigned i=0;i<3;++i){
            auto& a=*r[i];auto& b=*f[i];
            for(unsigned tick=0;tick<80&&!a.destroyRequested;++tick){
                check(a.advanceImage()&&b.advanceImage());
                for(unsigned k=0;k<3;++k){a.advanceAlarm(k);b.advanceAlarm(k);}
                if(a.destroyRequested||b.destroyRequested){check(a.destroyRequested==b.destroyRequested);break;}
                const auto count=[]{return 3;};
                const auto outside=[](const auto&,bool& retire){retire=false;return true;};
                check(a.step(false,referenceRandom,[&](N x,N y,bool& hit){
                    downwell::ContactBounds bounds;
                    if(!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,bounds))return false;
                    hit=overlaps(bounds);return true;
                },count,outside));
                check(b.step(false,fixedRandom,[&](std::int32_t x,std::int32_t y,bool& hit){
                    downwell::ContactBounds bounds;
                    if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,x,y,bounds))return false;
                    hit=overlaps(bounds);++meetings;hits+=hit;return true;
                },count,outside,[]{return true;}));
                if(referenceRandom.drawCount()!=fixedRandom.drawCount()){++streamsParted;i=3;break;}
                ++steps;timed+=b.timed;
                const int ax=pixel(a.x),ay=pixel(a.y);
                check(std::abs(ax-b.x.v)<=1&&std::abs(ay-b.y.v)<=1);
                pixelDifferences+=(ax!=b.x.v)+(ay!=b.y.v);
                check(a.imageSpeed.bits==b.imageSpeed.bits&&a.imageIndex.bits==b.imageIndex.bits&&
                      a.dflash==b.dflash&&a.destroyRequested==b.destroyRequested);
            }
        }
    }
    std::printf("Time shards Q16: %u bursts, %u shards (velocities within %d/65536 px), %u steps, %u meetings (%u solid), "
                "%u of %u published pixels differ by one, %u RNG streams parted by a moved hit; image_index timed on %u steps\n",
                bursts,shards,worstVelocity,steps,meetings,hits,pixelDifferences,2*steps,streamsParted,timed);
    check(timed*10>steps*8&&hits*20>meetings&&pixelDifferences*50<2*steps&&streamsParted*20<bursts);
}
