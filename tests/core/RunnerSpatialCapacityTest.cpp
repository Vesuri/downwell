#include "core/RunnerSpatialCapacity.h"
#include "core/RunnerSpatialIndex.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using Math=downwell::RunnerSpatialSplit;
template<unsigned N> static void exercise(unsigned seed){
    auto small=std::make_unique<downwell::RunnerSpatialIndex<downwell::RunnerSpatialCapacity<N>::nodes>>();
    auto oracle=std::make_unique<downwell::RunnerSpatialIndex<2*N+64>>();
    std::vector<Math::Box> boxes(N);
    auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
    auto compare=[&](){
        assert(small->valid()&&oracle->valid());
        assert(small->nodeCount()==oracle->nodeCount());
        assert(small->height()==oracle->height());
        std::vector<unsigned> a,b;
        const auto all=Math::encodePixels({-32768,-32768,32768,32768});
        assert(small->query(all,[&](unsigned id){a.push_back(id);return true;}));
        assert(oracle->query(all,[&](unsigned id){b.push_back(id);return true;}));
        assert(a==b);
    };
    for(unsigned round=0;round<3;++round){
        for(unsigned i=0;i<N;++i){
            const int x=round==0?int(i%64)*16:int(random()%4096)-2048;
            const int y=round==0?int(i/64)*16:int(random()%4096)-2048;
            boxes[i]=Math::encodePixels({x,y,x+16,y+16});
            assert(small->insert(boxes[i],i)&&oracle->insert(boxes[i],i));
        }
        compare();
        // Repeated delete/reinsert keeps full population while exercising
        // condensation, detached-node reuse and cascading root splits.
        for(unsigned j=0;j<N*3;++j){
            unsigned i=random()%N;bool a=false,b=false;
            assert(small->remove(boxes[i],i,true,a)&&oracle->remove(boxes[i],i,true,b)&&a&&b);
            const int x=int(random()%4096)-2048,y=int(random()%4096)-2048;
            boxes[i]=Math::encodePixels({x,y,x+16,y+16});
            assert(small->insert(boxes[i],i)&&oracle->insert(boxes[i],i));
            if(j%127==0)compare();
        }
        for(unsigned i=N;i;){--i;bool a=false,b=false;
            assert(small->remove(boxes[i],i,true,a)&&oracle->remove(boxes[i],i,true,b)&&a&&b);
            if(i%127==0)compare();
        }
        compare();
    }
}
int main(){exercise<1>(1);exercise<7>(2);exercise<64>(3);exercise<3963>(4);
    std::puts("spatial capacity: ordered mutations match oversized oracle");}
