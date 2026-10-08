#ifndef DOWNWELL_CORE_STORED_CURRENCY_REGISTRATION_H
#define DOWNWELL_CORE_STORED_CURRENCY_REGISTRATION_H
#include "CurrencyCreate.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Registered object-248 Create and script setters. Borrow at a fixed address
// and clear before room teardown. This does not replace the provisional gem
// motion array: animation, Step and Draw must be bound before live admission.
template<unsigned WorldCapacity,unsigned Capacity> class StoredCurrencyRegistration {
    struct Entry {InstanceHandle key{};unsigned id=0;CurrencyCreate state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==248?r:nullptr;
    }
    bool publish(Entry& e,RunnerImageValue x,RunnerImageValue y){
        auto* r=record(e);std::int32_t px,py;
        if(!r||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
            !lifecycle_.publishedPosition(e.key,*r,x,y))return fail();
        r=record(e);if(!r)return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
public:
    StoredCurrencyRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredCurrencyRegistration(const StoredCurrencyRegistration&)=delete;
    StoredCurrencyRegistration& operator=(const StoredCurrencyRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    const CurrencyCreate* find(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    template<class Random,class Tile> bool create(MotionBinary64 x,MotionBinary64 y,int area,bool attractor,
        Random& random,Tile tile,InstanceHandle& output){
        if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(248,definition)||!nextId_||nextId_>0x7fffffffu||
            !px.assign(x)||!py.assign(y)||!px.promoted().roundPixel(ix)||!py.promoted().roundPixel(iy))return fail();
        if(records_.create(nextId_,248,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py))return fail();
        struct Queries {
            StoredCurrencyRegistration& owner;Entry& e;Tile& tiles;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
                auto* r=owner.record(e);
                return (r&&owner.lifecycle_.placeMeeting(e.key,*r,57,x,y,hit))||owner.fail();
            }
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){return tiles(e.key,100,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){return owner.publish(e,x,y);}
        } queries{*this,*free,tile};
        const bool created=free->state.create(x,y,area,attractor,random,queries);
        r=record(*free);if(!r)return fail();
        r->imageSpeed=stored(free->state.imageSpeed);r->alarms=free->state.alarms;
        if(!created||!good_)return fail();
        output=free->key;return true;
    }
    bool amount(InstanceHandle h,int n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.getAmount=n;return true;}
    bool size(InstanceHandle h,int n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.size=n;return true;}
    bool attracted(InstanceHandle h,bool n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.attracted=n;return true;}
    bool clear(){
        for(auto& e:entries_)if(e.id){
            if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            e={};
        }
        return true;
    }
};
}
#endif
