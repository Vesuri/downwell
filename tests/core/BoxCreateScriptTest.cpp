#include "core/BoxCreateScript.h"
#include "core/GameMakerRandom.h"
#include "core/GeneratedBreakableCreate.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static bool same(const BreakableState& a,const BreakableState& b){
    return a.wallHp.high==b.wallHp.high&&a.wallHp.low==b.wallHp.low&&a.breakSound==b.breakSound&&
        a.haveGem==b.haveGem&&a.hit==b.hit&&a.drilled==b.drilled&&a.autoTile==b.autoTile&&a.checked==b.checked&&
        a.materialBreakable==b.materialBreakable&&a.groundRoomResult==b.groundRoomResult&&a.scriptIndex==b.scriptIndex;
}
struct Event {int field,value;BreakableState state;};
struct Trace {
    BreakableState& state;unsigned fail;std::vector<Event> events;
    bool put(int field,int value){events.push_back({field,value,state});return !fail||events.size()!=fail;}
};
// Independent straight-line transcription of VM 1085 and groundRoom. Choice
// operands here use raw WELL words, rather than the production choice adapter.
static bool reference(BreakableState& s,int area,int room,bool glitch,GameMakerRandom& r,Trace& trace){
    auto choice=[&](unsigned n){return r.nextU32()%n;};
    s.materialBreakable=true;
    if(!trace.put(0,74))return false;
    s.autoTile=false;s.checked=false;s.breakSound=171;
    switch(area){
    case 1:case 3:s.breakSound=171+choice(4);break;
    case 2:s.breakSound=175+choice(4);break;
    case 4:s.breakSound=179+choice(4);break;
    }
    if(glitch){const int sprites[]={51,52,53,55,57};if(!trace.put(0,sprites[choice(5)]))return false;}
    if(!trace.put(1,0))return false;
    if(!trace.put(2,16+choice(2)))return false;
    s.wallHp={0x3ff00000u,0};s.drilled=false;
    s.haveGem=r.irandomSmall(50)==0;
    if(area==3)s.haveGem=0;
    s.groundRoomResult=false;
    switch(room){case 20:case 21:case 22:case 23:case 24:s.groundRoomResult=true;break;}
    if(s.groundRoomResult)s.haveGem=0;
    if(s.haveGem==1){if(!trace.put(2,18+choice(2)))return false;s.haveGem=2;}
    if(s.haveGem==2&&!trace.put(2,22))return false;
    return true;
}
int main(){
    unsigned cases=0,gemCases=0;
    for(unsigned seed=0;seed<128;++seed)for(int area=0;area<6;++area)
    for(int room:{11,19,20,21,22,23,24,25})for(bool glitch:{false,true})for(unsigned fail=0;fail<=6;++fail){
        BreakableState actual;actual.materialBreakable=false;actual.autoTile=actual.checked=actual.drilled=true;
        actual.breakSound=999;actual.haveGem=9;actual.wallHp=MotionBinary64::fromInteger(10);
        actual.groundRoomResult=true;actual.scriptIndex=7;
        auto expected=actual;GameMakerRandom random(seed),oracle(seed);
        Trace trace{actual,fail,{}},wanted{expected,fail,{}};
        const bool ok=boxCreateScript(actual,area,74,room,glitch,random,
            [&](int n){return trace.put(0,n);},[&](int n){return trace.put(1,n);},[&](int n){return trace.put(2,n);});
        assert(ok==reference(expected,area,room,glitch,oracle,wanted));
        assert(same(actual,expected)&&trace.events.size()==wanted.events.size()&&random.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<trace.events.size();++i){
            const auto& a=trace.events[i];const auto& b=wanted.events[i];
            assert(a.field==b.field&&a.value==b.value&&same(a.state,b.state));
        }
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        ++cases;if(!fail&&actual.haveGem==2)++gemCases;
    }
    assert(gemCases);
    for(unsigned seed=0;seed<4096;++seed){
        GameMakerRandom random(seed),oracle(seed);BreakableState state;int frame=-1;
        assert(boxCreateScript(state,1,51,11,false,random,[](int){return true;},[](int){return true;},[&](int n){frame=n;return true;}));
        const auto prepared=GeneratedBreakableCreate::evaluate(oracle);
        assert(frame==int(prepared.frame)&&state.breakSound==prepared.sound&&state.haveGem==prepared.gems&&random.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
    }
    std::printf("Box Create: %u ordered/failure cases; 4096 cavern-adoption comparisons passed\n",cases);
}
