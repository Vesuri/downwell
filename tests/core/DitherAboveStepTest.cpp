#include "core/DitherAboveStep.h"
using namespace downwell;
struct DitherProbe {
    RunnerImageValue x{},positionY{},cameraPositionX{},cameraPositionY{};
    bool low=false,end=false,destroyed=false;
    unsigned calls=0,reject=0,writes=0;
    char events[16]{};
    bool event(char c){events[calls++]=c;return calls!=reject;}
    bool cameraX(MotionBinary64& out){if(!event('A'))return false;out=cameraPositionX.promoted();return true;}
    bool cameraY(MotionBinary64& out){if(!event('B'))return false;out=cameraPositionY.promoted();return true;}
    bool lowSpec(bool& out){if(!event('L'))return false;out=low;return true;}
    bool ending(bool& out){if(!event('E'))return false;out=end;return true;}
    bool y(RunnerImageValue& out){if(!event('r'))return false;out=positionY;return true;}
    bool setX(RunnerImageValue v){if(!event('x'))return false;x=v;return true;}
    bool setY(RunnerImageValue v){if(!event('y'))return false;positionY=v;++writes;return true;}
    bool destroySelf(){if(!event('D'))return false;destroyed=true;return true;}
};
bool nativeDitherAboveProbe(){
    DitherProbe p;
    if(!p.positionY.assignInteger(-80)||!p.cameraPositionX.assignInteger(240)||
        !p.cameraPositionY.assignInteger(192))return false;
    if(!DitherAboveStep::run(p)||p.writes!=2||p.destroyed)return false;
    std::int32_t y;
    return p.positionY.roundPixel(y)&&y==-64&&p.calls==10;
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include "core/GameMakerRounding.h"
#include "core/GameplayDitherAbove.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
static RunnerImageValue encoded(float f){RunnerImageValue v;std::memcpy(&v.bits,&f,4);return v;}
struct PixelOwner {
    using Record=GameplayDitherAbove::Record;
    unsigned count=0,reject=0;
    struct Write {std::uint32_t x,y;bool destroy;};
    Write writes[8]{};
    bool birth(InstanceHandle,const Record&){return true;}
    bool retire(InstanceHandle,const Record&){return true;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool sprite(InstanceHandle,const Record&,int){assert(false);return false;}
    bool mask(InstanceHandle,const Record&,int){assert(false);return false;}
    bool imageIndex(InstanceHandle,const Record&,RunnerImageValue){assert(false);return false;}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){assert(false);return false;}
    bool move(InstanceHandle,const Record&,RunnerImageValue x,RunnerImageValue y){
        writes[count++]={x.bits,y.bits,false};return count!=reject;
    }
    bool markDestroyed(InstanceHandle,const Record& r){
        writes[count++]={r.x.bits,r.y.bits,true};return count!=reject;
    }
};
struct PublicationOwner:PixelOwner {
    RunnerImageValue x{},y{};
    std::uint32_t flags=0;
    unsigned acquisitions=0;
    bool rejectAcquire=false,empty=false,partial=false;
    bool birth(InstanceHandle,const Record& r){x=r.x;y=r.y;return true;}
    bool move(InstanceHandle h,const Record& r,RunnerImageValue px,RunnerImageValue py){
        if(!PixelOwner::move(h,r,px,py))return false;
        if(!RunnerSpatialSplit::equal(x,px)||!RunnerSpatialSplit::equal(y,py)){
            x=px;y=py;flags|=8u;
        }
        return true;
    }
    static bool acquire(void* context,const Record&,GameplayDitherAbove::Publication& out){
        auto& p=*static_cast<PublicationOwner*>(context);++p.acquisitions;
        if(p.rejectAcquire)return false;
        if(!p.empty)out={&p.x,p.partial?nullptr:&p.y,&p.flags};
        return true;
    }
};
static void verifyPhasePublication(){
    for(float initial:{-80.5F,-80.0F,-0.0F,0.0F,0.5F,16777216.0F})
    for(int camera:{-100,128,192,272,4096})for(bool low:{false,true})for(bool ending:{false,true}){
        PublicationOwner a,b;
        GameplayDitherAbove reference(GameplayDitherAbove::Binding::bindDestruction(a));
        GameplayDitherAbove candidate(GameplayDitherAbove::Binding::bindDestruction(b),&b,PublicationOwner::acquire);
        GameplayDitherAbove::Snapshot state;state.sourceId=100854;state.y=encoded(initial);
        assert(reference.adopt(state)&&candidate.adopt(state));
        unsigned readsA=0,readsB=0;
        assert(reference.beginStepPixels(240,[&]{++readsA;assert(a.x.bits==encoded(240).bits);return camera;},low,ending));
        assert(candidate.beginStepPublishedPixels(240,[&]{++readsB;assert(b.x.bits==a.x.bits);return camera;},low,ending));
        const auto* ar=reference.find(reference.handle());const auto* br=candidate.find(candidate.handle());
        assert(readsA==1&&readsB==1&&b.acquisitions==1);
        assert(a.x.bits==b.x.bits&&a.y.bits==b.y.bits&&a.flags==b.flags);
        assert(ar->x.bits==br->x.bits&&ar->y.bits==br->y.bits&&ar->destroyRequested==br->destroyRequested);
        assert(b.count==unsigned(ending)); // No per-setter owner callbacks.
        assert(candidate.clear()&&candidate.create(100854,0,-80));
        assert(candidate.beginStepPublishedPixels(241,[]{return 4096;},false,false)&&b.acquisitions==2);
    }
    for(unsigned mode=0;mode<3;++mode){
        PublicationOwner p;p.rejectAcquire=mode==0;p.partial=mode==1;p.empty=mode==2;
        GameplayDitherAbove candidate(GameplayDitherAbove::Binding::bindDestruction(p),&p,PublicationOwner::acquire);
        assert(candidate.create(100854,0,-80));unsigned reads=0;
        assert(candidate.beginStepPublishedPixels(240,[&]{++reads;return 4096;},false,false)==(mode==2));
        assert(p.acquisitions==1&&reads==unsigned(mode==2));
        assert(p.count==(mode==2?3u:0u));
        if(mode!=2)assert(!candidate.find(candidate.handle())->x.bits);
    }
    std::puts("Dither phase publication: mirrors, camera-read ordering, rebirth and admission failures passed");
}
static void verifyPixelController(){
    unsigned cases=0;
    for(float initial:{-8388592.0F,-8388591.0F,-80.5F,-80.0F,-0.0F,0.0F,0.5F,
                       1.0F,8388591.0F,8388592.0F,16777216.0F})
    for(int camera:{(-2147483647-1),-8388592,-8388591,-1000,127,128,191,192,193,272,1000,8388591,8388592,2147483647})
    for(bool low:{false,true})for(bool ending:{false,true})for(unsigned reject=0;reject<5;++reject){
        PixelOwner a,b;a.reject=b.reject=reject;
        GameplayDitherAbove reference(GameplayDitherAbove::Binding::bindMaskDestruction(a));
        GameplayDitherAbove candidate(GameplayDitherAbove::Binding::bindMaskDestruction(b));
        GameplayDitherAbove::Snapshot state;state.sourceId=100854;state.y=encoded(initial);
        assert(reference.adopt(state)&&candidate.adopt(state));
        unsigned readsA=0,readsB=0;
        const bool expected=reference.beginStep([](MotionBinary64& x){x=MotionBinary64::fromInteger(240);return true;},
            [&](MotionBinary64& y){assert(a.count==1);++readsA;y=MotionBinary64::fromInteger(camera);return true;},low,ending);
        assert(candidate.beginStepPixels(240,[&]{assert(b.count==1);++readsB;return camera;},low,ending)==expected);
        assert(readsA==readsB);
        assert(a.count==b.count&&reference.valid()==candidate.valid());
        for(unsigned i=0;i<a.count;++i)assert(a.writes[i].x==b.writes[i].x&&a.writes[i].y==b.writes[i].y&&
            a.writes[i].destroy==b.writes[i].destroy);
        const auto* ar=reference.find(reference.handle());const auto* br=candidate.find(candidate.handle());
        assert(ar&&br&&ar->x.bits==br->x.bits&&ar->y.bits==br->y.bits&&ar->destroyRequested==br->destroyRequested);
        ++cases;
    }
    GameplayDitherAbove reference,candidate;
    for(unsigned run=0;run<2;++run){
        assert(reference.create(100854,240,-80)&&candidate.create(100854,240,-80));
        for(int tick=0;tick<10000;++tick){
            const int x=tick%320-160,y=tick*7-100;
            assert(reference.beginStep([&](MotionBinary64& v){v=MotionBinary64::fromInteger(x);return true;},
                [&](MotionBinary64& v){v=MotionBinary64::fromInteger(y);return true;},false,false));
            assert(candidate.beginStepPixels(x,[&]{return y;},false,false));
            assert(reference.find(reference.handle())->y.bits==candidate.find(candidate.handle())->y.bits);
        }
        assert(reference.clear()&&candidate.clear());
    }
    std::printf("Pixel controller: %u lifecycle/fallback cases and 20000 trajectory updates passed\n",cases);
}
int main(){
    verifyPhasePublication();
    verifyPixelController();
    assert(nativeDitherAboveProbe());
    unsigned checked=0;
    for(bool low:{false,true})for(bool end:{false,true})
    for(float initial:{-80.5F,-79.5F,-0.5F,0.0F,0.5F,1.5F,1000.25F})
    for(float offset:{-0.25F,0.0F,0.25F,4096.0F}){
        DitherProbe p;p.low=low;p.end=end;p.positionY=encoded(initial);
        p.cameraPositionX=encoded(240.25F);
        const float camera=initial+float(low?208:272)+offset;
        p.cameraPositionY=encoded(camera);
        const bool advance=double(camera)-double(initial)>=double(low?208:272);
        float expected=advance?float(double(initial)+16.0):initial;
        expected=float(roundGameMakerCoordinate(double(expected)));
        assert(DitherAboveStep::run(p)&&p.x.bits==encoded(240.25F).bits&&
            p.positionY.bits==encoded(expected).bits&&p.destroyed==end&&p.writes==(advance?2u:1u));
        assert(!std::strcmp(p.events,advance?(end?"AxLBrryryED":"AxLBrryryE"):(end?"AxLBrryED":"AxLBrryE")));
        ++checked;
    }
    // No catch-up: even a distant camera advances by exactly one 16px step.
    // A rejected operation retains all preceding writes and nothing after it.
    for(unsigned reject=1;reject<=11;++reject){
        DitherProbe p;p.end=true;p.reject=reject;p.positionY=encoded(-80.5F);
        p.cameraPositionX=encoded(241.25F);p.cameraPositionY=encoded(4096.0F);
        assert(!DitherAboveStep::run(p)&&p.calls==reject&&!p.destroyed);
        assert(p.x.bits==(reject<=2?0u:encoded(241.25F).bits));
        const float y=reject<=7?-80.5F:reject<=9?-64.5F:-64.0F;
        assert(p.positionY.bits==encoded(y).bits);
    }
    DitherProbe invalid;invalid.positionY.bits=0x7fc00000u;
    assert(!DitherAboveStep::run(invalid)&&invalid.writes==0);
    std::printf("Dither Above: %u boundary cases, ordered setters and 11 failure prefixes passed\n",checked);
}
#endif
