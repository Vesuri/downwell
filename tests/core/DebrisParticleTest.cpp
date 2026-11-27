#include "core/DebrisParticle.h"
#include "core/DebrisAnimation.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The compact world's Q16 debris against DebrisAnimation, its VM form,
// stepped side by side in a walled pit while their RNG streams agree:
// published pixels stay within one, hits (which draw RNG) rarely move and
// image_index (Q27 after the first advance) matches bit for bit.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Debris particle check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
namespace {
constexpr int mask=598;
bool solid(int x,int y){
    if(x<40||x>=280||y>=200)return true;
    return (y>>4)==11&&((x>>4)%4==1);
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
int pixel(downwell::RunnerImageValue v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
}
int main(){
    unsigned timed=0,births=0,steps=0,pixelDifferences=0,streamsParted=0,meetings=0,hits=0;
    for(unsigned seed=0;seed<512;++seed){
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),place(seed+7000);
        const int ix=60+int(place.irandomSmall(200)),iy=160+int(place.irandomSmall(30));
        const N x=N::fromInteger(ix),y=N::fromInteger(iy);
        downwell::DebrisAnimation reference;downwell::DebrisParticle fixed;
        const int area=1+int(seed%5);
        check(reference.create(x,y,area,referenceRandom)&&
              fixed.create(downwell::Fixed16::fromInteger(ix),downwell::Fixed16::fromInteger(iy),area,fixedRandom));
        check(referenceRandom.drawCount()==fixedRandom.drawCount()&&reference.spriteId==fixed.spriteId);
        // The Q16 birth is the nearest Q16 of the VM's samples; the frame
        // and rate are its binary32 values.
        {downwell::Fixed16 vx,vy;
         check(downwell::Fixed16::fromBinary64(reference.xsp,vx)&&downwell::Fixed16::fromBinary64(reference.ysp,vy)&&
               vx.v==fixed.xsp&&vy.v==fixed.ysp&&reference.imageSpeed.bits==fixed.imageSpeed.bits&&
               reference.imageIndex.bits==fixed.imageIndex.bits);}
        ++births;
        for(unsigned tick=0;tick<60&&!reference.destroyRequested;++tick){
            check(reference.advanceImage()&&fixed.advanceImage());
            for(unsigned a=0;a<3;++a){reference.advanceAlarm(a);fixed.advanceAlarm(a);}
            if(reference.destroyRequested)break;
            check(reference.step(false,referenceRandom,[&](N px,N py,bool& hit){
                downwell::ContactBounds b;
                if(!downwell::CurrencyCollisionBounds::calculateMask(mask,px,py,b))return false;
                hit=overlaps(b);return true;
            }));
            check(fixed.step(false,fixedRandom,[&](std::int32_t px,std::int32_t py,bool& hit){
                downwell::ContactBounds b;
                if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,px,py,b))return false;
                hit=overlaps(b);++meetings;hits+=hit;return true;
            },0));
            if(referenceRandom.drawCount()!=fixedRandom.drawCount()){++streamsParted;break;}
            ++steps;timed+=fixed.timed;
            const int rx=pixel(reference.x),ry=pixel(reference.y);
            check(std::abs(rx-fixed.pixelX())<=1&&std::abs(ry-fixed.pixelY())<=1);
            pixelDifferences+=(rx!=fixed.pixelX())+(ry!=fixed.pixelY());
            check(reference.imageSpeed.bits==fixed.imageSpeed.bits&&reference.imageIndex.bits==fixed.imageIndex.bits&&
                  reference.flash==fixed.flash&&
                  reference.destroyRequested==fixed.destroyRequested);
        }
    }
    std::printf("Debris Q16: %u births, %u steps, %u meetings (%u solid), %u of %u published pixels differ by one, "
                "%u RNG streams parted by a moved hit; image_index timed in Q27 on %u steps\n",births,steps,meetings,hits,pixelDifferences,2*steps,streamsParted,timed);
    check(timed*10>steps*8&&hits*10>meetings&&pixelDifferences*100<2*steps&&streamsParted*50<births);
}
