#include "core/GameplayShopProfileInput.h"
#include <cassert>
int main(){
    constexpr unsigned char first[]={0,1,2,4,7},second[]={5,6};
    downwell::GameplayShopProfileInput tape(first,5,second,2);
    for(unsigned i=0;i<5;++i){
        assert(tape.packed(1,false)==first[i]);
        assert(tape.packed(1,false)==first[i]); // Presentation must not consume.
        assert(tape.advance(1));
    }
    for(unsigned i=0;i<211;++i)assert(tape.packed(1,true)==4);
    assert(tape.frames(1)==5&&tape.frames(2)==0&&!tape.done());
    assert(!tape.advance(1)&&tape.packed(1,false)==0);
    for(unsigned i=0;i<2;++i){assert(tape.packed(2,false)==second[i]);assert(tape.advance(2));}
    assert(tape.done()&&!tape.advance(2)&&!tape.advance(0)&&!tape.advance(3));
    assert(tape.packed(0,false)==0&&tape.packed(3,false)==0);
}
