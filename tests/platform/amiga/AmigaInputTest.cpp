#include "platform/amiga/AmigaInput.h"
#include "core/EncounterInput.h"
#include "core/Replay.h"
#include <exec/types.h>
#include <cassert>
#include <cstdio>
int main(){
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
    std::puts("Native controls decode independently and drive session edges");
}
