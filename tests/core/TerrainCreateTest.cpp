#include "core/TerrainCreate.h"
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
    std::puts("Terrain Create: tile table, ordered property writes, glitch RNG and terminal prefixes passed");
}
