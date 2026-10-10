#ifndef DOWNWELL_AMIGA_VERIFICATION_SCRATCH_H
#define DOWNWELL_AMIGA_VERIFICATION_SCRATCH_H
#include "AmigaRenderer.h"
#include <exec/memory.h>
#include <proto/exec.h>

// Diagnostic pixel oracles execute synchronously in separate renderer phases.
// Share ordinary-memory storage, never displayed Chip buffers. The largest
// oracle needs both an untouched HUD and an independently composed HUD.
class AmigaVerificationScratch {
    static constexpr unsigned capacity = 2 * downwellLayerBytes;
    static bool& borrowed() { static bool value=false; return value; }
    static UBYTE*& storage() { static UBYTE* bytes=nullptr; return bytes; }
public:
    class Storage {
    public:
        Storage() {
            if (storage()) __builtin_trap();
            storage()=static_cast<UBYTE*>(AllocMem(capacity,MEMF_FAST));
        }
        ~Storage() {
            if (borrowed()) __builtin_trap();
            if (storage()) FreeMem(storage(),capacity);
            storage()=nullptr;
        }
        bool valid() const { return storage()!=nullptr; }
        Storage(const Storage&)=delete;
        Storage& operator=(const Storage&)=delete;
    };
    AmigaVerificationScratch() {
        // A future nested oracle must not silently overwrite its caller.
        if (borrowed() || !storage()) __builtin_trap();
        borrowed()=true;
    }
    ~AmigaVerificationScratch() { borrowed()=false; }
    AmigaVerificationScratch(const AmigaVerificationScratch&)=delete;
    AmigaVerificationScratch& operator=(const AmigaVerificationScratch&)=delete;
    template<unsigned Bytes> UBYTE* data() {
        static_assert(Bytes<=capacity,"Diagnostic framebuffer exceeds shared scratch");
        return storage();
    }
};
#endif
