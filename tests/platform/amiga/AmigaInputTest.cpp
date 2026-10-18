#include "platform/amiga/AmigaInput.h"
#include "core/EncounterInput.h"
#include "core/Replay.h"
#include <exec/types.h>
#include <cassert>
#include <cstring>
#include <cstdio>
struct Library{};
static Library resource;
static bool present=true,reserved=false;
static unsigned writes=0,allocations=0,frees=0;
static UWORD latch=0x0a00;
void* OpenResource(CONST_STRPTR name){
    assert(std::strcmp(name,"potgo.resource")==0);return present?&resource:nullptr;
}
UWORD testAllocPotBits(Library* base,UWORD bits){
    assert(base==&resource&&bits==0xc000);++allocations;
    if(reserved)return 0;
    reserved=true;return 0x4000; // OUT bits are not guaranteed in the return mask.
}
void testWritePotgo(Library* base,UWORD word,UWORD mask){
    assert(base==&resource&&reserved&&mask==0xc000);
    assert(word==0xc000||word==0);
    latch=(latch&~mask)|(word&mask);++writes;
}
void testFreePotBits(Library* base,UWORD bits){
    assert(base==&resource&&reserved&&bits==0x4000);
    assert(latch==0x0a00);reserved=false;++frees;
}
int main(){
    present=false;{AmigaInput input;assert(!input.ready());}
    assert(!writes&&!allocations&&!frees);
    present=true;
    {
        AmigaInput input;assert(input.ready()&&latch==0xca00);
        {AmigaInput blocked;assert(!blocked.ready());}
        assert(writes==1&&!frees&&reserved);
    }
    assert(writes==2&&frees==1&&!reserved&&latch==0x0a00);
    {AmigaInput again;assert(again.ready());}
    assert(writes==4&&frees==2&&!reserved&&latch==0x0a00);
    for(unsigned buttons=0;buttons<16;++buttons){
        const auto joy=std::uint16_t(((buttons&1)?0x200:0)|((buttons&2)?2:0));
        const auto cia=std::uint8_t((buttons&4)?0x7f:0xff);
        const auto pot=std::uint16_t((buttons&8)?0xbfff:0xffff);
        const auto input=AmigaInput::decode(joy,cia,pot);
        assert(input.left==bool(buttons&1)&&input.right==bool(buttons&2));
        assert(input.fire==bool(buttons&4)&&input.cancel==bool(buttons&8));
        assert(!input.pause&&!input.quit);
        // Other pins, quadrature bits and CIA lines cannot affect these controls.
        const auto noisy=AmigaInput::decode(joy|0xfdfd,cia^0x7f,pot^0xbfff);
        assert(downwell::packReplayInput(input)==downwell::packReplayInput(noisy));
    }
    downwell::EncounterInput session;
    auto pressed=AmigaInput::decode(0,0xff,0xbfff);
    session.sample(downwell::unpackReplayInput(downwell::packReplayInput(pressed)));
    assert(session.publication().cancel&&!session.publication().confirm);
    session.publication().cancel=false;session.sample(pressed);
    assert(!session.publication().cancel);
    session.resetPublication();session.sample(pressed);assert(!session.publication().cancel);
    session.sample(AmigaInput::decode(0,0xff,0xffff));session.sample(pressed);
    assert(session.publication().cancel);
    std::puts("Native Cancel pin lease, failure, cleanup, independent controls and session edges passed");
}
