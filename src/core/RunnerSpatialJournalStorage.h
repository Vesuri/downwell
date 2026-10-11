#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_STORAGE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_STORAGE_H
namespace downwell {
template<class Operation,unsigned Count> struct RunnerSpatialJournalInline {
    Operation entries[Count]{};
    bool prepare(){return true;}
    void clear(){}
    Operation& operator[](unsigned i){return entries[i];}
    const Operation& operator[](unsigned i)const{return entries[i];}
};
// Own history only when the admitted scene actually publishes it. Acquisition
// precedes the first mutation; this is not reconstruction from current state.
template<class Operation,unsigned Count> class RunnerSpatialJournalLease {
public:
    struct Bank {Operation entries[Count];};
    struct Provider {
        void* context=nullptr;
        Bank* (*acquire)(void*)=nullptr;
        void (*release)(void*,Bank*)=nullptr;
    };
    bool bind(Provider provider){
        if(provider_.acquire||bank_||!provider.acquire||!provider.release)return false;
        provider_=provider;return true;
    }
    bool prepare(){
        if(!bank_)bank_=provider_.acquire?provider_.acquire(provider_.context):nullptr;
        return bank_!=nullptr;
    }
    void clear(){
        if(bank_)provider_.release(provider_.context,bank_);
        bank_=nullptr;
    }
    Operation& operator[](unsigned i){return bank_->entries[i];}
    const Operation& operator[](unsigned i)const{return bank_->entries[i];}
    RunnerSpatialJournalLease()=default;
    ~RunnerSpatialJournalLease(){clear();}
    RunnerSpatialJournalLease(const RunnerSpatialJournalLease&)=delete;
    RunnerSpatialJournalLease& operator=(const RunnerSpatialJournalLease&)=delete;
private:
    Provider provider_{};
    Bank* bank_=nullptr;
};
}
#endif
