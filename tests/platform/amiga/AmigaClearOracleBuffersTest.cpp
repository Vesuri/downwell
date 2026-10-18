#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "platform/amiga/AmigaClearOracleBuffers.h"
static unsigned allocations=0,releases=0;
static bool reject=false;
static void* live=nullptr;
void* AllocMem(std::size_t bytes,unsigned flags){
    assert(!live&&bytes==AmigaClearOracleBuffers::bytes&&flags==(MEMF_FAST|MEMF_CLEAR));
    ++allocations;return reject?nullptr:(live=std::calloc(1,bytes));
}
void FreeMem(void* memory,std::size_t bytes){
    assert(memory==live&&bytes==AmigaClearOracleBuffers::bytes);
    ++releases;std::free(memory);live=nullptr;
}
int main(){
    {
        AmigaClearOracleBuffers buffers;assert(!allocations&&!live);
        reject=true;assert(!buffers.acquire());reject=false;
        assert(buffers.acquire()&&buffers.acquire()&&allocations==2);
        for(unsigned bank=0;bank<3;++bank){
            assert(buffers.game(bank)==static_cast<unsigned char*>(live)+bank*downwellGameRingBytes);
            assert(buffers.hud(bank)==static_cast<unsigned char*>(live)+3*downwellGameRingBytes+bank*downwellLayerBytes);
            for(unsigned i=0;i<downwellGameRingBytes;++i){assert(!buffers.game(bank)[i]);buffers.game(bank)[i]=bank+1;}
            for(unsigned i=0;i<downwellLayerBytes;++i){assert(!buffers.hud(bank)[i]);buffers.hud(bank)[i]=bank+4;}
        }
        buffers.release();buffers.release();assert(!live&&releases==1);
        assert(buffers.acquire()); // Next upgrade gets clean oracle canvases.
        for(unsigned i=0;i<AmigaClearOracleBuffers::bytes;++i)assert(!static_cast<unsigned char*>(live)[i]);
    }
    assert(!live&&allocations==3&&releases==2);
    std::puts("Upgrade oracle buffers: scene allocation, failure, independent canvases, reset and retirement passed");
}
