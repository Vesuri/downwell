#include "core/CatacombBuilderSymbols.h"
#include "core/CatacombBuilderDispatch.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
void require(bool value) { if (!value) std::abort(); }
struct Ops {
    std::vector<std::string> calls;
    unsigned first = 0, second = 0, choices = 0;
    unsigned creates = 0, failAt = 0;
    unsigned writes = 0, failWrite = 0;
    bool below = false;
    bool spawnChance(int value){
        calls.push_back("spawnChance:"+std::to_string(value));
        return ++writes!=failWrite;
    }
    int choose(const int* values, unsigned count) {
        std::string call = "choose";
        for (unsigned i = 0; i < count; ++i) call += ":" + std::to_string(values[i]);
        calls.push_back(call);
        const unsigned index = choices++ == 0 ? first : second;
        require(index < count);
        return values[index];
    }
    int irandom(unsigned upper) {
        calls.push_back("irandom:" + std::to_string(upper));
        return upper; // Nonzero: catches failure to overwrite this result.
    }
    bool randomFiveBelow(unsigned level) {
        calls.push_back("random5<" + std::to_string(level));
        return below;
    }
    bool create(int id, int x, int y) {
        calls.push_back("create:" + std::to_string(id) + ":" + std::to_string(x) + ":" + std::to_string(y));
        // Marker represents synchronous Create-event work, potentially RNG.
        calls.push_back("event");
        return ++creates != failAt;
    }
};
using Calls = std::vector<std::string>;
void appendCreate(Calls& calls, int id, int x = 240) {
    calls.push_back("create:" + std::to_string(id) + ":" + std::to_string(x) + ":720");
    calls.push_back("event");
}
unsigned cases = 0;
void check(char symbol, unsigned level, bool hard, Ops ops, Calls expected, bool result = true) {
    require(downwell::emitCatacombSymbol(symbol, 240, 720, level, hard, ops) == result);
    if (ops.calls != expected) {
        std::cerr << "call-order mismatch: " << symbol << " level " << level << '\n';
        std::abort();
    }
    ++cases;
}
}
int main() {
    // Expectations transcribed from builderCatacomb VM block operands, not RNG
    // approximations. Exhaust each choice index, including duplicate values.
    for (unsigned level = 1; level <= 3; ++level) for (bool hard : {false, true}) {
        for (auto item : {std::pair<char,int>{'A',97}, {'M',102}, {'Z',297}, {'P',300}, {'i',204}}) {
            Calls expected; appendCreate(expected, item.second);
            check(item.first, level, hard, {}, expected);
            Ops failure; failure.failAt = 1;
            check(item.first, level, hard, failure, expected, false);
        }
        for (char symbol : std::string("I ?@")) check(symbol, level, hard, {}, {});
        struct Conditional { char symbol; unsigned count; const char* call; unsigned minimum; int id; };
        for (auto item : {Conditional{'J',3,"choose:0:1:1",1,296},
                         {'Y',3,"choose:0:1:1",1,299}, {'a',2,"choose:0:1",1,111},
                         {'b',4,"choose:0:0:0:1",3,116}, {'j',2,"choose:0:1",1,120}}) {
            for (unsigned index = 0; index < item.count; ++index) {
                Ops ops; ops.first = index;
                Calls expected{item.call};
                if (index >= item.minimum) appendCreate(expected, item.id);
                check(item.symbol, level, hard, ops, expected);
            }
        }
        for (unsigned index = 0; index < 2; ++index) {
            Ops ops; ops.first = index;
            Calls c{"choose:0:1"};
            if (index) { c.push_back("choose:126"); appendCreate(c,126); }
            check('c', level, hard, ops, c);
            Calls d{"choose:0:1"};
            if (index) { appendCreate(d,126,248); appendCreate(d,126,232); }
            check('d', level, hard, ops, d);
            ops.below = index != 0;
            Calls e{"random5<" + std::to_string(level)};
            if (index) appendCreate(e,120);
            check('e', level, hard, ops, e);
        }
        for (unsigned failAt : {1u,2u}) {
            Ops ops; ops.first = 1; ops.failAt = failAt;
            Calls d{"choose:0:1"}; appendCreate(d,126,248);
            if (failAt == 2) appendCreate(d,126,232);
            check('d',level,hard,ops,d,false);
        }
        const unsigned count = level == 1 ? 3 : level == 2 ? 2 : 5;
        for (unsigned index = 0; index < count; ++index) {
            const bool spawn = hard || index >= (level == 2 ? 1u : 2u);
            for (unsigned enemy = 0; enemy < (level == 3 ? 2u : 3u); ++enemy) {
                Ops ops; ops.first = index; ops.second = enemy;
                Calls expected{"irandom:" + std::to_string(level),"spawnChance:"+std::to_string(level),
                    level == 1 ? "choose:0:0:1" : level == 2 ? "choose:0:1" : "choose:0:0:1:1:1",
                    "spawnChance:"+std::to_string(index >= (level == 2 ? 1u : 2u)?1:0)};
                if(hard)expected.push_back("spawnChance:1");
                for(unsigned write=1;write<=(hard?3u:2u);++write){
                    Ops failure=ops;failure.failWrite=write;
                    Calls prefix(expected.begin(),expected.begin()+(write==1?2:write==2?4:5));
                    check('%',level,hard,failure,prefix,false);
                }
                if (spawn) {
                    expected.push_back(level == 3 ? "choose:126:126" : "choose:126:126:111");
                    appendCreate(expected, level < 3 && enemy == 2 ? 111 : 126);
                }
                check('%',level,hard,ops,expected);
                if (spawn) { ops.failAt = 1; check('%',level,hard,ops,expected,false); }
            }
        }
    }
    for (unsigned level : {0u,4u,999u}) check('%',level,true,{}, {},false);
    unsigned randomCases=0;
    for(unsigned seed=0;seed<64;++seed)for(unsigned level=1;level<=3;++level)for(bool hard:{false,true}){
        downwell::GameMakerRandom actual(seed),reference(seed);
        downwell::CatacombBuilderDispatch state;
        for(char symbol:std::string("ecjd%JYab")){
            Calls wanted,seen;int chance=state.spawnChance;
            const auto choose=[&](std::initializer_list<int> list){return *(list.begin()+reference.nextU32()%list.size());};
            const auto child=[&](int id,int x){wanted.push_back(std::to_string(id)+":"+std::to_string(x));reference.nextU32();};
            switch(symbol){
                case 'e':if(std::ldexp(double(reference.nextU32()),-32)*5-double(level)<-0.00001)child(120,240);break;
                case 'c':if(choose({0,1}))child(choose({126}),240);break;
                case 'j':if(choose({0,1}))child(120,240);break;
                case 'd':if(choose({0,1})){child(126,248);child(126,232);}break;
                case '%':
                    reference.irandomSmall(level);
                    chance=level==1?choose({0,0,1}):level==2?choose({0,1}):choose({0,0,1,1,1});
                    if(hard)chance=1;
                    if(chance)child(level<=2?choose({126,126,111}):choose({126,126}),240);break;
                case 'J':if(choose({0,1,1}))child(296,240);break;
                case 'Y':if(choose({0,1,1}))child(299,240);break;
                case 'a':if(choose({0,1}))child(111,240);break;
                case 'b':if(choose({0,0,0,1}))child(116,240);break;
            }
            require(state.cell(symbol,240,720,level,hard,actual,[&](int id,int x,int y){
                require(y==720&&state.spawnChance==chance);seen.push_back(std::to_string(id)+":"+std::to_string(x));
                actual.nextU32();return true;
            })&&seen==wanted&&state.spawnChance==chance&&actual.drawCount()==reference.drawCount());
            for(unsigned i=0;i<16;++i)require(actual.stateWord(i)==reference.stateWord(i));
            ++randomCases;
        }
    }
    {
        downwell::GameMakerRandom random(7);downwell::CatacombBuilderDispatch state;
        unsigned children=0;
        require(!state.cell('%',240,720,1,true,random,[&](int,int,int){++children;random.nextU32();return false;})&&
            children==1&&!state.succeeded&&state.spawnChance==1);
        const auto draws=random.drawCount();
        require(!state.cell('e',240,720,1,false,random,[](int,int,int){return true;})&&random.drawCount()==draws);
    }
    std::cout << cases << " catacomb symbol/call-order cases passed\n";
    std::cout << randomCases << " WELL-backed cell/Create interleaving cases passed\n";
}
