#include "core/ReplayFrameTrace.h"
#include <cassert>
#include <cstdio>
struct Fixture {
    const Fixture& player()const{return *this;}
    std::uint64_t checkpoint()const{return 0x123456789abcdef0ULL;}
    long offsetXFixed()const{return -257;}
    long offsetYFixed()const{return 513;}
    long speedXFixed()const{return -128;}
    long speedYFixed()const{return 256;}
    int ammo()const{return 7;}
    bool grounded()const{return true;}
    unsigned ringOrigin()const{return 300;}
    bool bulletActive()const{return false;}
    int bulletX()const{return -3;}
    int bulletY()const{return 45;}
};
int main(){
    volatile std::uint32_t trace[downwell::ReplayFrameTrace::words+1]{};
    trace[downwell::ReplayFrameTrace::words]=0xfeedbeefu;
    downwell::ReplayFrameTrace::capture(Fixture{},trace);
    const std::uint32_t expected[]={0x12345678u,0x9abcdef0u,0xfffffeffu,513,
        0xffffff80u,256,7,1,300,0,0xfffffffdu,45,0xfeedbeefu};
    for(unsigned i=0;i<sizeof(expected)/sizeof(*expected);++i)assert(trace[i]==expected[i]);
    std::puts("Replay trace: fixed-width signed fields, checkpoint halves and storage boundary passed");
}
