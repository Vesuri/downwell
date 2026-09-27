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
    for(unsigned move=0;move<512;++move){
        auto& w=walls[(move*37u)%walls.size()];
        assert(index.removeOne(w.object,w.x,w.y));
        w.x=int(move%31)*16-128;w.y=int(move%101)*16;
        assert(index.append(w.object,w.x,w.y)&&index.size()==walls.size());
        const downwell::ContactBounds q{w.x-9,w.x+9,w.y-8,w.y+8};
        unsigned wanted=0,actual=0;
        for(const auto& other:walls){
            const int l=other.x-(other.object==293?9:8),r=other.x+(other.object==293?10:8);
            const int t=other.y-(other.object==293?7:8),b=other.y+(other.object==293?9:8);
            wanted+=q.right>l&&q.left<r&&q.bottom>t&&q.top<b;
        }
        assert(index.visit(q,[](auto...){return true;},[&](){++actual;return true;})&&actual==wanted);
    }
    assert(!index.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[](){return false;}));
    index.clear();count=0;
    assert(index.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[&](){++count;return true;}) && !count);
    downwell::TerrainWallContacts<2> small;
    assert(small.append(60,0,0) && small.append(60,0,0) && !small.append(60,16,0));
    assert(small.append(93,0,0) && small.size()==2);
    assert(!small.removeOne(93,0,0)&&!small.removeOne(60,16,0)&&small.size()==2);
    assert(small.removeOne(60,0,0)&&small.size()==1);
    count=0;
    assert(small.visit({-8,8,-8,8},[](auto...){return true;},[&](){++count;return true;})&&count==1);
    assert(small.append(293,32,32)&&small.size()==2);
    // Repeatedly move the reused slot between rows while the duplicate wall
    // remains. This catches stale bucket links and append-only capacity leaks.
    int previousX=32,previousY=32;
    for(unsigned i=0;i<8192;++i){
        assert(small.removeOne(293,previousX,previousY)&&small.size()==1);
        previousX=int(i%32)*16;previousY=int(i%512)*16;
        assert(small.append(293,previousX,previousY)&&small.size()==2);
        count=0;
        assert(small.visit({INT_MIN,INT_MAX,INT_MIN,INT_MAX},[](auto...){return true;},[&](){++count;return true;})&&count==2);
    }
    assert(small.removeOne(60,0,0)&&!small.removeOne(60,0,0));
    assert(small.removeOne(293,previousX,previousY)&&small.size()==0);
    small.clear();assert(!small.append(60,INT_MIN,0) && !small.append(92,0,8192) && !small.append(293,1,0));
    std::printf("Terrain wall contacts: %u scalar queries preserve duplicates, retirement, edges and failure\n",queries);
}
