#ifndef DOWNWELL_NATIVE_ENEMY_ALARM_ROSTER_H
#define DOWNWELL_NATIVE_ENEMY_ALARM_ROSTER_H
#include "InstanceHandle.h"
#include <cstdint>
namespace downwell {
// Native object identity is immutable within a generation. Maintain the alarm
// phase's exact-object/registration ordering at its mutation points. A phase
// borrows a bank; only a mutation while that bank is pinned makes a copy.
template<unsigned Capacity> class NativeEnemyAlarmRoster {
public:
    struct Entry {InstanceHandle handle;int object;std::uint32_t order;unsigned alarms;};
    struct Phase {const Entry* entries=nullptr;unsigned count=0,alarms=0;};
private:
    struct Bank {Entry entries[Capacity];unsigned count=0,alarms=0;} banks_[2]{};
    unsigned current_=0;
    int pinned_=-1;
    Bank& writable(){
        if(pinned_==int(current_)){
            const unsigned next=current_^1;
            banks_[next]=banks_[current_];current_=next;
        }
        return banks_[current_];
    }
    static void mask(Bank& bank){
        bank.alarms=0;
        for(unsigned i=0;i<bank.count;++i)bank.alarms|=bank.entries[i].alarms;
    }
    static bool before(const Entry& a,const Entry& b){
        return a.object<b.object||(a.object==b.object&&a.order<b.order);
    }
public:
    // Read-only phase admission. A pinned bank still requires the ordinary
    // begin() failure path; absence of handlers is stronger than zero actors.
    bool noAlarmHandlers()const{return pinned_<0&&!banks_[current_].alarms;}
    bool begin(Phase& out){
        if(pinned_>=0)return false;
        pinned_=int(current_);const auto& bank=banks_[current_];
        out={bank.entries,bank.count,bank.alarms};return true;
    }
    void end(){pinned_=-1;}
    void clear(){auto& bank=writable();bank.count=bank.alarms=0;}
    bool erase(unsigned slot){
        const auto& old=banks_[current_];unsigned i=0;
        while(i<old.count&&old.entries[i].handle.slot!=slot)++i;
        if(i==old.count)return true;
        auto& bank=writable();
        for(unsigned j=i+1;j<bank.count;++j)bank.entries[j-1]=bank.entries[j];
        --bank.count;mask(bank);return true;
    }
    bool put(Entry entry){
        if(!entry.handle.generation||entry.object<0||!entry.order)return false;
        const auto& old=banks_[current_];
        unsigned found=old.count;
        for(unsigned i=0;i<old.count;++i)if(old.entries[i].handle.slot==entry.handle.slot){
            const auto& e=old.entries[i];
            if(e.handle.generation==entry.handle.generation&&e.object==entry.object&&
               e.order==entry.order&&e.alarms==entry.alarms)return true;
            found=i;break;
        }
        if(found==old.count&&old.count==Capacity)return false;
        erase(entry.handle.slot);auto& bank=writable();
        unsigned at=0;while(at<bank.count&&!before(entry,bank.entries[at]))++at;
        for(unsigned i=bank.count;i>at;--i)bank.entries[i]=bank.entries[i-1];
        bank.entries[at]=entry;++bank.count;bank.alarms|=entry.alarms;return true;
    }
};
}
#endif
