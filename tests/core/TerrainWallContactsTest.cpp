#include "core/TerrainWallContacts.h"
#include <cassert>
#include <cstdio>
#include <climits>
#include <vector>
int main(){
    downwell::TerrainWallContacts<4096> index;
    struct Wall{int object,x,y;};std::vector<Wall> walls;
    for(unsigned i=0;i<2000;++i){
        const int object=i%3==0?60:i%3==1?92:293;
        const int x=int(i%20)*16-64,y=int(i%40)*16;
        assert(index.append(object,x,y));walls.push_back({object,x,y});
    }
    unsigned queries=0,multiple=0;
    for(int y=-10;y<660;y+=3)for(int x=-80;x<272;x+=3){
        const downwell::ContactBounds q{x-2,x+2,y-1,y+2};
        for(unsigned broken=0;broken<2;++broken){
            unsigned wanted=0,actual=0;
            for(const auto& w:walls){
                if(broken && w.object==92)continue;
                const int l=w.x-(w.object==293?9:8),r=w.x+(w.object==293?10:8);
                const int t=w.y-(w.object==293?7:8),b=w.y+(w.object==293?9:8);
                wanted+=x+2>l && x-2<r && y+2>t && y-1<b;
            }
            assert(index.visit(q,[&](unsigned object,int,int){return !broken || object!=92;},[&](){++actual;return true;}));
            assert(actual==wanted);multiple+=actual>1;++queries;
        }
    }
    unsigned count=0;
    assert(index.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[&](){++count;return true;}));
    assert(count==walls.size() && multiple);
    assert(!index.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[](){return false;}));
    index.clear();count=0;
    assert(index.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[&](){++count;return true;}) && !count);
    downwell::TerrainWallContacts<2> small;
    assert(small.append(60,0,0) && small.append(60,0,0) && !small.append(60,16,0));
    assert(small.append(93,0,0) && small.size()==2);
    small.clear();assert(!small.append(60,INT_MIN,0) && !small.append(92,0,8192) && !small.append(293,1,0));
    std::printf("Terrain wall contacts: %u scalar queries preserve duplicates, retirement, edges and failure\n",queries);
}
