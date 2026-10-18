#include "core/RunnerSpatialCapacity.h"
#include "core/RunnerSpatialIndex.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using Math=downwell::RunnerSpatialSplit;
struct CountedNode {
    static unsigned constructions;
    unsigned value;
    CountedNode():value(0){++constructions;}
};
unsigned CountedNode::constructions=0;
static void construction(){
    downwell::RunnerSpatialNodePool<CountedNode,128,true> lazy;
    assert(CountedNode::constructions==0);
    lazy.start(0);lazy.start(1);lazy[1].value=17;
    assert(CountedNode::constructions==2&&lazy[1].value==17);
    lazy.start(1);assert(CountedNode::constructions==3&&lazy[1].value==0);
    downwell::RunnerSpatialNodePool<CountedNode,128,false> eager;
    assert(CountedNode::constructions==131);
    eager.start(0);assert(CountedNode::constructions==131&&eager[0].value==0);
}
static void resetAndFallback(){
    downwell::RunnerSpatialIndex<128,true,true> lazy;
    downwell::RunnerSpatialIndex<128,true,false> eager;
    for(unsigned pass=0;pass<6;++pass){
        lazy.clear();eager.clear();
        std::vector<Math::Box> boxes;
        for(unsigned i=0;i<100;++i){
            boxes.push_back(Math::encodePixels({int(i)*2,0,int(i)*2+1,1}));
            assert(lazy.insert(boxes.back(),i)&&eager.insert(boxes.back(),i));
        }
        for(unsigned i=0;i<80;++i){bool a=false,b=false;
            assert(lazy.remove(boxes[i],i,true,a)&&eager.remove(boxes[i],i,true,b)&&a==b);
        }
        // Conversion visits released nodes as well as the live tree. Only
        // constructed high-water nodes may be inspected after clear/reuse.
        auto fractional=Math::encodePixels({0,0,1,1});fractional.left.bits=0x3f000000u;
        assert(lazy.insert(fractional,100)&&eager.insert(fractional,100));
        std::vector<unsigned> a,b;
        const auto query=Math::encodePixels({-100,-100,1000,1000});
        assert(lazy.query(query,[&](unsigned id){a.push_back(id);return true;}));
        assert(eager.query(query,[&](unsigned id){b.push_back(id);return true;}));
        assert(a==b&&lazy.nodeCount()==eager.nodeCount()&&lazy.height()==eager.height());
    }
}
template<unsigned N> static void exercise(unsigned seed){
    auto small=std::make_unique<downwell::RunnerSpatialIndex<downwell::RunnerSpatialCapacity<N>::nodes>>();
    auto oracle=std::make_unique<downwell::RunnerSpatialIndex<2*N+64,true,false>>();
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
int main(){construction();resetAndFallback();exercise<1>(1);exercise<7>(2);exercise<64>(3);exercise<3963>(4);
    std::puts("spatial capacity: ordered mutations match oversized oracle");}
