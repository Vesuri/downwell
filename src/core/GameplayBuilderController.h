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
    // Deferred rows land when the phase ends, including after a failure:
    // each was a completed setter.
    template<class Generate> bool withGeneration(Generate generate){
        if(phase_)return fail();
        Phase phase;phase_=&phase;
        struct Reset {GameplayBuilderController& owner;~Reset(){owner.publishPosition();owner.phase_=nullptr;}} reset{*this};
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
        integerX_=x;integerY_=y;return true;
    }
    bool position(int x,int y){
        if(!good_||!record_.lifetimeId)return fail();
        if(phase_&&!phase_->acquired){
            if(acquire_&&!acquire_(publicationContext_,record_,phase_->publication))return fail();
            const auto& p=phase_->publication;
            if(bool(p.x)!=bool(p.y)||bool(p.x)!=bool(p.appearance))return fail();
            phase_->acquired=true;
        }
        if(phase_&&phase_->publication.x){
            // A lent position cannot fail and no spatial queue consumes it.
            // Newline rows (same x, exactly representable increasing y) land
            // together at the next reader: publishPosition() precedes every
            // Create but callback-free terrain, and the end of the phase.
            auto& phase=*phase_;
            const int last=phase.pending?phase.y:integerY_;
            if(x==integerX_&&y>last&&y<=0x1000000&&y>=-0x1000000){phase.y=y;++phase.pending;return true;}
            publishPosition();
        }
        // record_ holds the exact conversion of integerX_/integerY_; rows
        // usually keep x, so convert only a changed coordinate.
        RunnerImageValue px=record_.x,py=record_.y;
        if((x!=integerX_&&!px.assignInteger(x))||(y!=integerY_&&!py.assignInteger(y)))return fail();
        if(phase_&&phase_->publication.x){
            const auto& p=phase_->publication;
            if(!RunnerSpatialSplit::equal(*p.x,px)||!RunnerSpatialSplit::equal(*p.y,py)){
                *p.x=px;*p.y=py;*p.appearance|=8u;
            }
        }else if(binding_.hasOwner()&&!binding_.publishedPosition(handle(),record_,px,py))return fail();
        record_.x=px;record_.y=py;integerX_=x;integerY_=y;return true;
    }
    // Lands deferred newline rows; cannot fail. The same comparisons as one
    // setter per row: exact increasing y values all differ, so more than one
    // row always changes the lent value.
    void publishPosition(){
        if(!phase_||!phase_->pending)return;
        auto& phase=*phase_;const auto& p=phase.publication;
        RunnerImageValue py;py.assignInteger(phase.y);
        if(phase.pending>1||!RunnerSpatialSplit::equal(*p.x,record_.x)||!RunnerSpatialSplit::equal(*p.y,py)){
            *p.x=record_.x;*p.y=py;*p.appearance|=8u;
        }
        record_.y=py;integerY_=phase.y;phase.pending=0;
    }
    bool clear(){
        if(!good_||(phase_&&phase_->publication.x)||
           (record_.lifetimeId&&!binding_.retire(handle(),record_)))return fail();
        record_={};return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record record_{};
    int integerX_=0,integerY_=0;
    Binding binding_{};
    struct Phase {Publication publication{};bool acquired=false;unsigned pending=0;int y=0;};
    void* publicationContext_=nullptr;
    AcquirePublication acquire_=nullptr;
    Phase* phase_=nullptr;
    unsigned next_=1;
    FailureSignal<> good_=true;
};
}
#endif
