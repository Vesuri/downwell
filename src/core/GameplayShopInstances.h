#ifndef DOWNWELL_CORE_GAMEPLAY_SHOP_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_SHOP_INSTANCES_H
#include "ShopWorld.h"
#include "ShopChildCreate.h"
#include "ObjectPack.h"
#include "InstanceLifecycle.h"
#include "AlarmBank.h"
namespace downwell {
// Native ownership of Shop and its ten synchronous children. Numeric IDs
// belong to the enclosing room allocator, never to this generation pool.
// The pool must outlive its ShopWorld and every borrowed creation adapter.
class GameplayShopInstances {
public:
    static constexpr unsigned capacity=ShopWorld::capacity*11;
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        int objectId=-1,spriteId=-1,maskId=-1;
        RunnerImageValue x{},y{},imageIndex{},imageSpeed{0x3f800000u};
        AlarmBank alarms;
        ShopChildState state;
        ShopWorld::Instance* shop=nullptr;
        int speaking=0,checkThis=0,interactOk=1,textOver=0,choice0=-1;
        bool active=true,destroyRequested=false,initialized=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayShopInstances(const ObjectPack& objects,Binding binding={}):objects_(objects),binding_(binding){}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    InstanceHandle handle(unsigned slot)const{
        return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};
    }
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle sourceHandle(unsigned source)const{
        for(unsigned i=0;i<capacity;++i)if(source&&records_[i].lifetimeId&&records_[i].sourceId==source)return handle(i);
        return {};
    }
    template<class Allocate,class Audio> class Creation {
        GameplayShopInstances& owner_;Allocate& allocate_;Audio& audio_;
    public:
        Creation(GameplayShopInstances& owner,Allocate& allocate,Audio& audio):owner_(owner),allocate_(allocate),audio_(audio){}
        bool valid()const{return owner_.valid();}
        bool begin(ShopWorld::Instance& shop){
            if(shop.sourceId)return owner_.fail();
            for(const auto& r:owner_.records_)if(r.lifetimeId&&r.shop==&shop)return owner_.fail();
            return owner_.create(277,shop.x,shop.y,allocate_,&shop)!=0;
        }
        bool play(int sound,int priority,bool loop,int& voice){
            return (valid()&&audio_.play(sound,priority,loop,voice)&&valid())||owner_.fail();
        }
        bool setGain(int voice,MotionBinary64 gain,int duration){
            return (valid()&&audio_.setGain(voice,gain,duration)&&valid())||owner_.fail();
        }
        unsigned create(int object,int x,int y){return owner_.create(object,x,y,allocate_,nullptr);}
        bool position(unsigned source,int& x,int& y){
            const auto* r=owner_.find(owner_.sourceHandle(source));std::int32_t px,py;
            if(!valid()||!r||!r->initialized||!r->x.roundPixel(px)||!r->y.roundPixel(py))return owner_.fail();
            x=px;y=py;return true;
        }
        bool readX(unsigned source,int& x){int y;return position(source,x,y);}
        bool upgrade(unsigned source,unsigned value){return owner_.write(source,283,value);}
        bool stored(unsigned source,unsigned value){return owner_.write(source,279,value);}
    };
    template<class Allocate,class Audio> Creation<Allocate,Audio> creation(Allocate& allocate,Audio& audio){
        return {*this,allocate,audio};
    }
    // Cleanup retains generation monotonicity, including a rejected Create's
    // published prefix. No Destroy event is synthesized by room teardown.
    bool clear(){
        if(creating_)return fail();
        for(unsigned i=0;i<capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire(handle(i),records_[i]))return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    bool write(unsigned source,int object,unsigned value){
        const auto h=sourceHandle(source);const auto* r=find(h);
        if(!good_||!r||!r->initialized||r->objectId!=object||value>0x7fffffffu)return fail();
        if(object==283)records_[h.slot].state.whichUg=value;
        else records_[h.slot].state.storedItem=int(value);
        return true;
    }
    template<class Allocate> unsigned create(int object,int x,int y,Allocate& allocate,ShopWorld::Instance* shop){
        if(!good_||creating_||!next_||count_==capacity||
           (object!=277&&object!=278&&object!=279&&object!=98&&object!=280&&object!=212&&object!=283)||
           (object==277)!=bool(shop)){fail();return 0;}
        PackedObject definition;RunnerImageValue px,py;
        if(!objects_.findObject(object,definition)||!px.assignInteger(x)||!py.assignInteger(y)){fail();return 0;}
        creating_=true;struct Reset{bool& value;~Reset(){value=false;}} reset{creating_};
        unsigned source=0;
        if(!allocate(object,source)||!good_||!source||source>0x7fffffffu||sourceHandle(source).generation){fail();return 0;}
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;r.objectId=object;
        r.spriteId=definition.spriteId;r.maskId=definition.textureMaskId;r.x=px;r.y=py;r.shop=shop;++count_;
        const auto h=handle(slot);
        if(shop)shop->sourceId=source;
        if(!binding_.birthAt(h,r,px,py)||!good_){fail();return 0;}
        if(object!=277){
            struct Builtins {
                GameplayShopInstances& owner;Record& r;InstanceHandle h;
                bool speed(RunnerImageValue v){r.imageSpeed=v;return true;}
                bool alarm(unsigned n,int v){r.alarms.set(n,v);return true;}
                bool index(RunnerImageValue v){
                    if(!owner.binding_.imageIndex(h,r,v)||!owner.good_)return false;
                    r.imageIndex=v;return true;
                }
            } builtins{*this,r,h};
            if(!r.state.create(object,builtins)){fail();return 0;}
        }
        // Parent's inherited variables have their source defaults in Record.
        r.initialized=true;return source;
    }
    const ObjectPack& objects_;Binding binding_;Record records_[capacity]{};
    unsigned count_=0,next_=1;bool good_=true,creating_=false;
};
}
#endif
