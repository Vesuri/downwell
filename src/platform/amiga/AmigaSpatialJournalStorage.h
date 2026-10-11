#ifndef DOWNWELL_AMIGA_SPATIAL_JOURNAL_STORAGE_H
#define DOWNWELL_AMIGA_SPATIAL_JOURNAL_STORAGE_H
#include <exec/memory.h>
#include <proto/exec.h>
#include "core/PlacementNew.h"

// Same lifetime as the tree provider: outlives the placement-created owner,
// including early exits. Never borrow Chip RAM for CPU-only mutation history.
template<class Lease> class AmigaSpatialJournalStorage {
    using Bank=typename Lease::Bank;
    Bank* bank_=nullptr;
    static Bank* acquire(void* context){
        auto& self=*static_cast<AmigaSpatialJournalStorage*>(context);
        if(self.bank_)return nullptr;
        auto* memory=AllocMem(sizeof(Bank),MEMF_FAST);
        if(!memory)return nullptr;
        return self.bank_=new(memory) Bank;
    }
    static void release(void* context,Bank* bank){
        auto& self=*static_cast<AmigaSpatialJournalStorage*>(context);
        if(bank!=self.bank_)__builtin_trap();
        bank->~Bank();FreeMem(bank,sizeof(Bank));self.bank_=nullptr;
    }
public:
    typename Lease::Provider provider(){return {this,acquire,release};}
    AmigaSpatialJournalStorage()=default;
    ~AmigaSpatialJournalStorage(){if(bank_)release(this,bank_);}
    AmigaSpatialJournalStorage(const AmigaSpatialJournalStorage&)=delete;
    AmigaSpatialJournalStorage& operator=(const AmigaSpatialJournalStorage&)=delete;
};
#endif
