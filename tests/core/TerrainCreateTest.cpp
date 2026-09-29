#include "core/TerrainCreate.h"
#include "core/CavernEnemyChoice.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace downwell;
int main(){
    const int expected[]={76,51,74,72,70,55};
    for(int i=0;i<6;++i)assert(terrainTileForArea(i)==expected[i]);
    assert(terrainTileForArea(-1)==-1&&terrainTileForArea(6)==-1);
    for(int object:{60,93})for(bool glitch:{false,true})for(unsigned seed=0;seed<128;++seed){
        GameMakerRandom random(seed),oracle(seed);std::vector<int> trace;
        std::vector<int> want{1000+74};
        if(object==60&&glitch){const int choices[]={51,52,53,55,57};want.push_back(1000+choices[oracle.chooseIndexSmall(5)]);}
        if(object==93)want.push_back(2023);
        want.push_back(3000);
        assert(terrainCreateProperties(object,74,glitch,random,
            [&](int v){trace.push_back(1000+v);return true;},
            [&](int v){trace.push_back(2000+v);return true;},
            [&](int v){trace.push_back(3000+v);return true;}));
        assert(trace==want&&random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        for(unsigned fail=1;fail<=want.size();++fail){
            GameMakerRandom failed(seed);trace.clear();
            auto call=[&](int v){trace.push_back(v);return trace.size()!=fail;};
            assert(!terrainCreateProperties(object,74,glitch,failed,
                [&](int v){return call(1000+v);},[&](int v){return call(2000+v);},[&](int v){return call(3000+v);}));
            assert(trace==std::vector<int>(want.begin(),want.begin()+fail));
            assert(failed.drawCount()==unsigned(object==60&&glitch&&fail>1));
        }
    }
    // Counterexample for an eager span: total RNG draws and final WELL state
    // can agree even though the generated artwork is observably different.
    // A future bulk owner must keep cell draw -> completed Create interleaved.
    unsigned differentArt=0;
    for(unsigned seed=0;seed<128;++seed){
        GameMakerRandom immediate(seed),eager(seed);
        std::vector<int> sourceArt,eagerArt;
        constexpr unsigned walls=8;
        for(unsigned cell=0;cell<walls;++cell){
            assert(cavernEnemyChoice('1',1,immediate)==-1);
            assert(terrainCreateProperties(60,51,true,immediate,
                [&](int value){sourceArt.push_back(value);return true;},
                [](int){return true;},[](int){return true;}));
        }
        for(unsigned cell=0;cell<walls;++cell)assert(cavernEnemyChoice('1',1,eager)==-1);
        for(unsigned cell=0;cell<walls;++cell)
            assert(terrainCreateProperties(60,51,true,eager,
                [&](int value){eagerArt.push_back(value);return true;},
                [](int){return true;},[](int){return true;}));
        assert(immediate.drawCount()==2*walls&&immediate.drawCount()==eager.drawCount()&&
            immediate.stateIndex()==eager.stateIndex());
        for(unsigned word=0;word<16;++word)assert(immediate.stateWord(word)==eager.stateWord(word));
        differentArt+=sourceArt!=eagerArt;
    }
    assert(differentArt>0);
    std::printf("Terrain batching: %u/128 eager spans change artwork despite identical final RNG state\n",differentArt);
    std::puts("Terrain Create: tile table, ordered property writes, glitch RNG and terminal prefixes passed");
}
