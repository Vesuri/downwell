#include "platform/amiga/AmigaEntryMask.h"
bool nativeEntryMaskProbe(){
    for(int y=-116;y<=800;++y){
        unsigned offset=0;
        if(!AmigaEntryMask::offset(y,offset)||offset+284*48>AmigaEntryMask::bytes)return false;
        const int raw=684-y;
        const unsigned expected=unsigned(raw<0?0:raw>364?364:raw)*48;
        if(offset!=expected)return false;
    }
    unsigned retained=123;
    if(AmigaEntryMask::offset(-117,retained)||retained!=123)return false;
    if(AmigaEntryMask::offset(-2147483647-1,retained)||retained!=123)return false;
    return AmigaEntryMask::offset(2147483647,retained)&&retained==0;
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cassert>
#include <cstdio>
int main(){
    assert(nativeEntryMaskProbe());
    std::puts("Entry-mask pointer range, DMA bounds and unsupported positions passed");
}
#endif
