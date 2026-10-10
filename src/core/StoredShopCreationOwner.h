#ifndef DOWNWELL_CORE_STORED_SHOP_CREATION_OWNER_H
#define DOWNWELL_CORE_STORED_SHOP_CREATION_OWNER_H
#include "StoredShopChildRegistration.h"
#include "ShopWorld.h"
#include "ShopLifetimeEvents.h"
#include "ShopRadioEvents.h"
namespace downwell {
// Concrete owner for ShopWorld::createRegistered. The compact shop owns stock
// and menu state; its retained room record owns identity and builtins. Children
// borrow exactly the same allocator and lifecycle publisher as the parent.
// Keep this owner and ShopWorld at stable addresses until room teardown.
template<unsigned WorldCapacity,unsigned ShopCapacity,class Audio> class StoredShopCreationOwner {
    static_assert(ShopCapacity>0,"shop creation needs parent storage");
public:
    struct Parent {
        InstanceHandle key{};unsigned id=0;ShopWorld::Instance* shop=nullptr;
        int speaking=0,checkThis=0,interactOk=1,textOver=0,choice0=-1;
        bool inheritedInitialized=false;
    };
private:
    Parent parents_[ShopCapacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack& objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;Audio& audio_;
    StoredShopChildRegistration<WorldCapacity,ShopCapacity*10> children_;
    bool fail(){good_=false;return false;}
    bool live(const Parent& p)const{
        const auto* r=records_.resolve(p.key);
        return p.id&&r&&r->instanceId==p.id&&r->objectId==277;
    }
    static float storedValue(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
public:
    StoredShopCreationOwner(InstanceStore<WorldCapacity>& records,const ObjectPack& objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good,Audio& audio)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good),audio_(audio),
         children_(records,objects,lifecycle,nextId,room,good){}
    StoredShopCreationOwner(const StoredShopCreationOwner&)=delete;
    StoredShopCreationOwner& operator=(const StoredShopCreationOwner&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records,const ObjectPack* objects)const{
        return &records_==&records && &objects_==objects;
    }
    bool valid()const{return good_;}
    const Parent* parent(unsigned id)const{
        for(const auto& p:parents_)if(id&&p.id==id&&live(p))return &p;
        return nullptr;
    }
    auto& children(){return children_;}
    template<class Environment> bool parentRadioStep(unsigned id,Environment& environment){
        const auto* p=parent(id);auto* r=p?records_.resolve(p->key):nullptr;
        if(!good_||!r||!p->shop)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            StoredShopCreationOwner& owner;const Parent& p;InstanceRecord& r;Environment& env;
            static MotionBinary64 real(float f){RunnerImageValue v;std::memcpy(&v.bits,&f,4);return v.promoted();}
            bool timeStop(bool& out){return env.timeStop(out)&&owner.good_;}
            bool death(bool& out){return env.death(out)&&owner.good_;}
            bool distance(MotionBinary64& out){return env.distance(real(r.preciseX),real(r.preciseY),out)&&owner.good_;}
            bool inView(bool& out){return env.inView(real(r.preciseX),real(r.preciseY),out)&&owner.good_;}
            bool gain(MotionBinary64 value,int ms){return env.gain(p.shop->radio,value,ms)&&owner.good_;}
            bool noBgm(bool& out){return env.noBgm(out)&&owner.good_;}
            bool paused(bool& out){return env.paused(p.shop->radio,out)&&owner.good_;}
            bool resume(){return env.resume(p.shop->radio)&&owner.good_;}
        } ops{*this,*p,*r,environment};
        return ShopRadioEvents::step(p->shop->shopBgm,ops)||fail();
    }
    template<class Environment> bool parentLifetimeEvent(unsigned id,bool roomEnd,Environment& environment){
        const auto* p=parent(id);auto* r=p?records_.resolve(p->key):nullptr;
        if(!good_||!r||!p->shop)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            StoredShopCreationOwner& owner;const Parent& p;InstanceRecord& r;Environment& env;
            bool viewY(MotionBinary64& out){return env.viewY(out)&&owner.good_;}
            bool room(int& out){out=owner.room_;return true;}
            bool isPlaying(bool& out){return env.isPlaying(p.shop->radio,out)&&owner.good_;}
            bool stop(){return env.stop(p.shop->radio)&&owner.good_;}
            bool destroy(){
                if(!owner.lifecycle_.markDestroyed(p.key,r)||!owner.good_)return false;
                r.destroyRequested=true;return true;
            }
        } ops{*this,*p,*r,environment};
        RunnerImageValue y;std::memcpy(&y.bits,&r->preciseY,4);
        return (roomEnd?ShopLifetimeEvents::roomEnd(ops):ShopLifetimeEvents::endStep(y.promoted(),ops))||fail();
    }
    bool begin(ShopWorld::Instance& shop){
        if(!good_||shop.sourceId||!nextId_||nextId_>0x7fffffffu)return fail();
        Parent* free=nullptr;
        for(auto& p:parents_){if(p.shop==&shop)return fail();if(!p.id&&!free)free=&p;}
        PackedObject definition;RunnerImageValue x,y;std::int32_t ix,iy;
        if(!free||!objects_.findObject(277,definition)||!x.assignInteger(shop.x)||!y.assignInteger(shop.y)||
           !x.roundPixel(ix)||!y.roundPixel(iy))return fail();
        if(records_.create(nextId_,277,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->shop=&shop;
        auto* r=records_.resolve(free->key);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=storedValue(x);r->preciseY=storedValue(y);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        shop.sourceId=free->id;
        if(!lifecycle_.birthAt(free->key,*r,x,y)||!good_)return fail();
        // parentInteractable Create: speaking/checkThis/textOver=0,
        // interactOk=1, choice[0]=-1. No inherited births or RNG.
        free->inheritedInitialized=true;return true;
    }
    bool play(int sound,int priority,bool loop,int& handle){
        return (good_&&audio_.play(sound,priority,loop,handle)&&good_)||fail();
    }
    bool setGain(int handle,MotionBinary64 gain,int duration){
        return (good_&&audio_.setGain(handle,gain,duration)&&good_)||fail();
    }
    unsigned create(int object,int x,int y){return children_.create(object,x,y);}
    bool readX(unsigned id,int& x){return children_.readX(id,x);}
    bool position(unsigned id,int& x,int& y){return children_.position(id,x,y);}
    bool upgrade(unsigned id,unsigned value){return children_.upgrade(id,value);}
    bool stored(unsigned id,unsigned value){return children_.stored(id,value);}
    bool retireParent(unsigned id){
        if(!good_)return false;
        for(auto& p:parents_)if(p.id==id){
            const auto* r=records_.resolve(p.key);
            if(!live(p)||!r->destroyRequested||
               !records_.erase(id,[&](auto h,const auto& record){return lifecycle_.retire(h,record);})||!good_)return fail();
            p={};return true;
        }
        return fail();
    }
    // Explicit teardown, before ShopWorld::reset. No Destroy events are
    // synthesized here; the room scheduler must dispatch them where required.
    bool clear(){
        if(!children_.clear())return fail();
        for(auto& p:parents_)if(p.id){
            if(!live(p)||!records_.erase(p.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            p={};
        }
        return true;
    }
};
}
#endif
