#ifndef DOWNWELL_RUNNER_FULL_PROPERTIES_H
#define DOWNWELL_RUNNER_FULL_PROPERTIES_H
namespace downwell {
template<class Properties,unsigned Capacity> class RunnerFullProperties {
    Properties records_[Capacity]{};
public:
    using Read=const Properties*;
    struct Edit {Properties& p;Properties& get(){return p;}void publish(){}};
    Read read(unsigned slot)const{return &records_[slot];}
    Edit edit(unsigned slot){return {records_[slot]};}
    template<bool> Edit editDomain(unsigned slot){return edit(slot);}
    bool valid()const{return true;}
};
}
#endif
