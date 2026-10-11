#ifndef DOWNWELL_CORE_GAMEPLAY_BUILDER_CONTROLLER_H
#define DOWNWELL_CORE_GAMEPLAY_BUILDER_CONTROLLER_H
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
#include "RunnerSpatialSplit.h"
namespace downwell {
// Authored objBuilder identity and published builtins. Generation/RNG state
// remains in AreaOneBuilder; this is not a second generator or numeric allocator.
class GameplayBuilderController {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    // A scene owner may lend its stable published position for one generation
    // phase. Source coordinates and the public mirror are still written on
    // every row, before any nested Create. An empty lease uses the callback
    // reference. Only the owner can establish that no spatial queue consumes it.
    struct Publication {
        RunnerImageValue* x=nullptr;
        RunnerImageValue* y=nullptr;
        std::uint32_t* appearance=nullptr;
    };
    using AcquirePublication=bool (*)(void*,const Record&,Publication&);
    explicit GameplayBuilderController(Binding binding={},void* context=nullptr,
        AcquirePublication acquire=nullptr):binding_(binding),publicationContext_(context),acquire_(acquire){}
    template<class Generate> bool withGeneration(Generate generate){
        if(phase_)return fail();
        Phase phase;phase_=&phase;
        struct Reset {GameplayBuilderController& owner;~Reset(){owner.phase_=nullptr;}} reset{*this};
        return generate()&&good_;
    }
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
        if(!good_||!record_.lifetimeId||!px.assignInteger(x)||!py.assignInteger(y))return fail();
        if(phase_&&!phase_->acquired){
            if(acquire_&&!acquire_(publicationContext_,record_,phase_->publication))return fail();
            const auto& p=phase_->publication;
            if(bool(p.x)!=bool(p.y)||bool(p.x)!=bool(p.appearance))return fail();
            phase_->acquired=true;
        }
        if(phase_&&phase_->publication.x){
            const auto& p=phase_->publication;
            if(!RunnerSpatialSplit::equal(*p.x,px)||!RunnerSpatialSplit::equal(*p.y,py)){
                *p.x=px;*p.y=py;*p.appearance|=8u;
            }
        }else if(binding_.hasOwner()&&!binding_.publishedPosition(handle(),record_,px,py))return fail();
        record_.x=px;record_.y=py;return true;
    }
    bool clear(){
        if(!good_||(phase_&&phase_->publication.x)||
           (record_.lifetimeId&&!binding_.retire(handle(),record_)))return fail();
        record_={};return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record record_{};
    Binding binding_{};
    struct Phase {Publication publication{};bool acquired=false;};
    void* publicationContext_=nullptr;
    AcquirePublication acquire_=nullptr;
    Phase* phase_=nullptr;
    unsigned next_=1;
    FailureSignal<> good_=true;
};
}
#endif
