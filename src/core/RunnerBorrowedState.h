#ifndef DOWNWELL_CORE_RUNNER_BORROWED_STATE_H
#define DOWNWELL_CORE_RUNNER_BORROWED_STATE_H
#include "PreparedTerrainBirthBounds.h"
#include "FailureSignal.h"
namespace downwell {
// Reference-only arrays are owned by the caller, not by a native instance
// owner borrowing the reference spatial journal. No allocation or lazy fallback.
template<class Record,unsigned Capacity> class RunnerBorrowedRecords {
    Record* records_=nullptr;
    Record failedEdit_{};
    mutable FailureSignal<> good_=true;
public:
    RunnerBorrowedRecords()=default;
    RunnerBorrowedRecords(const RunnerBorrowedRecords&)=delete;
    RunnerBorrowedRecords& operator=(const RunnerBorrowedRecords&)=delete;
    void observeFailure(bool& owner){good_.observe(owner);}
    using Read=const Record*;
    struct Edit {Record& p;Record& get(){return p;}void publish(){}};
    bool bindStorage(Record* records) {
        if(!records||records_)return good_=false;
        records_=records;return good_;
    }
    Read read(unsigned slot)const {
        if(!records_||slot>=Capacity){good_=false;return nullptr;}
        return records_+slot;
    }
    Edit edit(unsigned slot) {
        if(!records_||slot>=Capacity){good_=false;return {failedEdit_};}
        return {records_[slot]};
    }
    template<bool> Edit editDomain(unsigned slot){return edit(slot);}
    bool valid()const{return good_;}
};
template<class State,unsigned Capacity>
class RunnerBorrowedSpatialStates:public RunnerBorrowedRecords<State,Capacity> {
    using Base=RunnerBorrowedRecords<State,Capacity>;
public:
    static constexpr bool runOwned=false;
    std::uint32_t flags(unsigned slot)const{const auto* s=Base::read(slot);return s?s->flags:0;}
    bool special(unsigned slot)const{const auto* s=Base::read(slot);return s&&s->special;}
    unsigned kind(unsigned slot)const{const auto* s=Base::read(slot);return s?s->kind:0;}
    bool capture(unsigned slot){auto& s=Base::edit(slot).get();s.previous={s.x,s.y};return Base::valid();}
    void fresh(unsigned slot,const State& state){auto& s=Base::edit(slot).get();s=state;s.previous={s.x,s.y};}
    void cache(unsigned slot,const decltype(State{}.bounds)& box,PreparedTerrainBirthBounds::Key){Base::edit(slot).get().bounds=box;}
    template<class T> bool bind(T&){return Base::valid();}
    void geometry(const PreparedTerrainBirthBounds&){}
    template<unsigned N> void prepare(const PreparedObjectDefaults<N>&){}
    void clear(){}
};
}
#endif
