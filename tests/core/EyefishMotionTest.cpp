#include "core/EyefishMotion.h"
#include "core/CheckCollisionWithScript.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The Q16 eyefish against its VM form (scrCheckCollisionWith on binary64,
// kept here as the oracle): the same walls give the same pixels and turns.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Eyefish check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
namespace {
struct Reference {
    N xx{},yy{},xsp{};
    void create(int x,int y,bool left){xx=N::fromInteger(x);yy=N::fromInteger(y);xsp={left?0xbfe00000u:0x3fe00000u,0};}
    bool left()const{return (xsp.high>>31)!=0;}
    template<class Query> bool step(Query query,std::int32_t& x){
        downwell::CheckCollisionWithScript script;
        if(!script.run(57,xx,yy,xsp,N{},query))return false;
        if(script.xcollision)xsp={script.xcollision>0?0xbfe00000u:0x3fe00000u,0};
        return xx.add(xsp)&&xx.roundPixel(x);
    }
};
unsigned salt=0;
bool solid(int x,int y){
    if(x<32||x>=288)return true;
    const unsigned h=unsigned((x>>4)*73856093)^unsigned((y>>4)*19349663)^salt;
    return (h>>9)%7==0;
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
}
int main(){
    unsigned steps=0,turns=0;
    for(unsigned seed=0;seed<200;++seed){
        downwell::GameMakerRandom path(seed);salt=seed*2654435761u;
        const int x=48+16*int(path.irandomSmall(14)),y=16*int(path.irandomSmall(30));
        const bool left=path.chooseIndexSmall(2)!=0;
        Reference reference;downwell::EyefishMotion fixed;
        reference.create(x,y,left);fixed.create(x,y,left);
        for(unsigned tick=0;tick<800;++tick){
            std::int32_t rx,fx;
            check(reference.step([](N px,N py,int object,bool& hit){
                downwell::ContactBounds b;
                if(object!=57||!downwell::CurrencyCollisionBounds::calculateMask(289,px,py,b))return false;
                hit=overlaps(b);return true;},rx));
            check(fixed.step([](std::int32_t px,std::int32_t py,bool& hit){
                downwell::ContactBounds b;
                if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(289,px,py,b))return false;
                hit=overlaps(b);return true;},fx));
            check(rx==fx&&reference.left()==fixed.left());
            turns+=fixed.left()!=left;
            ++steps;
        }
    }
    std::printf("Eyefish Q16: %u steps equal to the binary64 script (%u steps after a turn)\n",steps,turns);
    check(turns>steps/10);
}
