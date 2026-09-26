#include "core/PlayerTimeStop.h"
// UTMT 0.9.2.0 gml_Object_objPlayer_n_Step_1.asm, blocks 46..79;
// SHA-256 46a8178a1b7cf78ff8f14f204047096d00717ea59da6e2cd74d908cf1932fed5.
// Input data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
#include <cassert>
#include <vector>
#include <cstdio>
using State=downwell::PlayerTimeStop;
using E=State::Effect;
int main(){
    unsigned cases=0;
    for(unsigned flags=0;flags<512;++flags){
        const bool main=flags&1,side=flags&2,field=flags&4,checked=flags&8,stopped=flags&16;
        const bool playing=flags&32,paused=flags&64,wind=flags&128;
        const bool bgmOn=flags&256;
        State s;s.checkStop=checked;s.stopped=stopped;s.bgmOn=bgmOn;
        std::vector<E> events,wanted;unsigned queries=0,audioQueries=0;
        const bool next=field||(main&&side);
        if(main && !checked && next && field){wanted.push_back(E::shards);wanted.push_back(E::shake);}
        if(main && checked && !next)wanted.push_back(E::shards);
        if(next!=stopped){
            wanted.push_back(E::change);
            if(next){if(playing)wanted.push_back(E::pauseMusic);wanted.push_back(E::shards);wanted.push_back(E::enterSound);}
            else{if(paused)wanted.push_back(E::resumeMusic);if(wind)wanted.push_back(E::stopWind);
                wanted.push_back(E::shards);wanted.push_back(E::exitSound);wanted.push_back(E::duckMusic);}
        }
        if(next)wanted.push_back(E::oxygenFloor);
        assert(s.step(main,side,[&](bool& hit){++queries;hit=field;return true;},
            [&](State::AudioQuery q){++audioQueries;return q==State::AudioQuery::musicPlaying?playing:q==State::AudioQuery::musicPaused?paused:wind;},
            [&](E e){events.push_back(e);if(e==E::change)assert(s.stopped==next);return true;}));
        assert(events==wanted && s.checkStop==next && s.stopped==next);
        assert(queries==1u+unsigned(main&&!checked&&next));
        assert(audioQueries==(next==stopped?0u:next?1u:2u));
        assert(s.bgmOn==(next==stopped?bgmOn:!next));++cases;
    }
    State repeat;unsigned queries=0;std::vector<E> events;
    assert(repeat.step(true,false,[&](bool& hit){hit=++queries==1;return true;},
        [](auto){return false;},[&](E e){events.push_back(e);return true;}));
    assert(queries==2 && events==std::vector<E>({E::change,E::shards,E::enterSound,E::oxygenFloor}));
    State failed;unsigned emitted=0;
    assert(!failed.step(true,true,[](bool& hit){hit=false;return true;},[](auto){assert(false);return false;},
        [&](E e){++emitted;assert(e==E::change);return false;}));
    assert(emitted==1 && failed.checkStop && failed.stopped && failed.bgmOn);
    std::printf("Player time stop: %u transition/audio combinations, repeated queries and failure order passed\n",cases);
}
