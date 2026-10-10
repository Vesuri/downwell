#ifndef DOWNWELL_CORE_GAMEPLAY_RESULT_INSTANCE_H
#define DOWNWELL_CORE_GAMEPLAY_RESULT_INSTANCE_H
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// rmResult's authored object 325. Selection state remains in RunSession;
// this owns only the source identity and published instance builtins.
class GameplayResultInstance {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayResultInstance(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    InstanceHandle handle()const{return {0,record_.lifetimeId};}
    const Record* find(InstanceHandle h)const{
        return !h.slot&&h.generation&&h.generation==record_.lifetimeId?&record_:nullptr;
    }
    bool create(unsigned source,int x,int y){
        if(!good_||record_.lifetimeId||!next_||!source||source>0x7fffffffu)return fail();
        record_={};record_.sourceId=source;record_.lifetimeId=next_++;
        if(!record_.x.assignInteger(x)||!record_.y.assignInteger(y)||
           !binding_.birth(handle(),record_))return fail();
        return true;
    }
    bool position(int x,int y){
        RunnerImageValue px,py;
        if(!good_||!record_.lifetimeId||!px.assignInteger(x)||!py.assignInteger(y)||
           (binding_.hasOwner()&&!binding_.publishedPosition(handle(),record_,px,py)))return fail();
        record_.x=px;record_.y=py;return true;
    }
    bool clear(){
        if(!good_||(record_.lifetimeId&&!binding_.retire(handle(),record_)))return fail();
        record_={};return true;
    }
    bool publishY(int y){
        RunnerImageValue py;
        if(!good_||!record_.lifetimeId||!py.assignInteger(y)||
           (binding_.hasOwner()&&!binding_.publishedPosition(handle(),record_,record_.x,py)))return fail();
        record_.y=py;return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record record_{};Binding binding_{};unsigned next_=1;FailureSignal<> good_=true;
};
}
#endif
