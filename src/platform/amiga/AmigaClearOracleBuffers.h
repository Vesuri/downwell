#ifndef DOWNWELL_AMIGA_CLEAR_ORACLE_BUFFERS_H
#define DOWNWELL_AMIGA_CLEAR_ORACLE_BUFFERS_H
#include "AmigaRenderer.h"
#include <exec/memory.h>
#include <proto/exec.h>
// Verification-only CPU framebuffers. Never resident alongside gameplay's
// banks: acquire after their retirement, release before next-room admission.
class AmigaClearOracleBuffers {
    UBYTE* bytes_=nullptr;
public:
    static constexpr unsigned bytes=3*(downwellGameRingBytes+downwellLayerBytes);
    bool acquire(){
        if(!bytes_)bytes_=static_cast<UBYTE*>(AllocMem(bytes,MEMF_FAST|MEMF_CLEAR));
        return bytes_!=nullptr;
    }
    void release(){if(bytes_){FreeMem(bytes_,bytes);bytes_=nullptr;}}
    ~AmigaClearOracleBuffers(){release();}
    AmigaClearOracleBuffers()=default;
    AmigaClearOracleBuffers(const AmigaClearOracleBuffers&)=delete;
    AmigaClearOracleBuffers& operator=(const AmigaClearOracleBuffers&)=delete;
    UBYTE* game(unsigned buffer){return bytes_+buffer*downwellGameRingBytes;}
    UBYTE* hud(unsigned buffer){return bytes_+3*downwellGameRingBytes+buffer*downwellLayerBytes;}
};
#endif
