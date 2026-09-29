#ifndef DOWNWELL_RUNNER_REGISTRATION_ORDER_H
#define DOWNWELL_RUNNER_REGISTRATION_ORDER_H
#include <cstdint>
namespace downwell {
// Ordering only, never a GameMaker numeric instance ID. Explicit words avoid
// a target runtime helper and keep ordering beyond a 32-bit publication count.
struct RunnerRegistrationOrder {
    std::uint32_t low=0,high=0;
    bool present()const{return low||high;}
    void advance(){if(!++low)++high;}
    bool before(const RunnerRegistrationOrder& b)const{
        return high<b.high||(high==b.high&&low<b.low);
    }
};
}
#endif
