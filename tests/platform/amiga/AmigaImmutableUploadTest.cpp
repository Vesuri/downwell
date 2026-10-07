#include "platform/amiga/AmigaImmutableUpload.h"
#include <cassert>

int main() {
    unsigned short source[1031], target[1033], second[1033];
    for (unsigned i=0;i<1031;++i) source[i]=i*97;
    for (unsigned i=0;i<1033;++i) target[i]=second[i]=0xdead;
    AmigaImmutableUpload upload;
    for (unsigned chunk=0;chunk<5;++chunk) {
        assert(upload.step(target+1,source,1031,256)==(chunk==4));
        const unsigned end=(chunk+1)*256<1031?(chunk+1)*256:1031;
        for (unsigned i=0;i<1031;++i)
            assert(target[i+1]==(i<end?source[i]:0xdead));
        assert(target[0]==0xdead && target[1032]==0xdead);
    }
    // A completed upload must not touch live bytes again.
    target[1]=0xbeef;
    assert(upload.step(target+1,source,1031,256) && target[1]==0xbeef);
    // Invalidation, a changed bank, and a synchronous finish all rebind safely.
    upload.invalidate();
    assert(!upload.step(target+1,source,1031,1) && target[1]==source[0]);
    assert(!upload.step(second+1,source,1031,256));
    assert(second[257]==0xdead);
    assert(upload.step(second+1,source,1031,1031));
    for(unsigned i=0;i<1031;++i) assert(second[i+1]==source[i]);
    assert(second[0]==0xdead && second[1032]==0xdead);
    assert(!upload.step(nullptr,source,1031,256));
}
