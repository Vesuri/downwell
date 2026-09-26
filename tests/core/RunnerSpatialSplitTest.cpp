#include "core/RunnerSpatialSplit.h"
#include "core/RunnerSpatialNode.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using Split=downwell::RunnerSpatialSplit;
using Box=std::array<float,4>;
static Box merge(Box a,Box b){return {std::min(a[0],b[0]),std::min(a[1],b[1]),std::max(a[2],b[2]),std::max(a[3],b[3])};}
static float area(Box b){return (b[2]-b[0])*(b[3]-b[1]);}
// Native binary32 scalar oracle, structurally separate from integer arithmetic.
static std::array<unsigned,7> reference(const Box (&b)[7]){
    unsigned a=0,c=1;float score=-INFINITY;
    for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<7;++j){
        const float candidate=(area(merge(b[i],b[j]))-area(b[i]))-area(b[j]);
        if(candidate>score){score=candidate;a=i;c=j;}
    }
    std::array<unsigned,7> groups;groups.fill(2);groups[a]=0;groups[c]=1;
    unsigned counts[2]={1,1};Box bounds[2]={b[a],b[c]};
    for(unsigned assigned=2;assigned<7;++assigned){
        if(counts[0]==5 || counts[1]==5){
            unsigned g=counts[0]==5?1:0;
            for(auto& item:groups)if(item==2)item=g;
            break;
        }
        unsigned pick=0,side=0;float largest=-1;
        for(unsigned i=0;i<7;++i)if(groups[i]==2){
            float difference=(area(merge(bounds[1],b[i]))-area(bounds[1]))-
                             (area(merge(bounds[0],b[i]))-area(bounds[0]));
            unsigned preferred=difference<0?1:0;
            float magnitude=std::fabs(difference);
            if(magnitude>largest || (magnitude==largest && counts[preferred]<counts[side])){
                largest=magnitude;pick=i;side=preferred;
            }
        }
        groups[pick]=side;++counts[side];bounds[side]=merge(bounds[side],b[pick]);
    }
    return groups;
}
static Split::Real real(float f){Split::Real r;std::memcpy(&r.bits,&f,4);return r;}
static void check(const Box (&boxes)[7]){
    Split::Box input[7];
    for(unsigned i=0;i<7;++i)input[i]={real(boxes[i][0]),real(boxes[i][1]),real(boxes[i][2]),real(boxes[i][3])};
    Split::Result result;assert(Split::partition(input,result));
    auto expected=reference(boxes);unsigned counts[2]={};
    for(unsigned i=0;i<7;++i){assert(result.group[i]==expected[i]);++counts[result.group[i]];}
    assert(counts[0]==result.count[0] && counts[1]==result.count[1]);
    assert(counts[0]>=2 && counts[1]>=2 && counts[0]+counts[1]==7);
}
int main(){
    using Node=downwell::RunnerSpatialNode;
    for(unsigned removed=0;removed<6;++removed){
        Node node;
        for(unsigned i=0;i<6;++i)assert(node.append({{},100+i,200+i}));
        assert(!node.append({}));
        assert(node.eraseInstance(200+removed));assert(node.count==5);
        for(unsigned i=0;i<5;++i){
            const unsigned expected=i==removed?5:i;
            assert(node.entries[i].instance==200+expected);
            assert(node.entries[i].child==100+expected);
        }
        assert(!node.eraseInstance(200+removed));assert(!node.erase(5));
        assert(node.append({{},999,200+removed}));
        assert(node.entries[5].instance==200+removed);
        Node sibling;sibling.level=9;
        assert(node.split({{},888,777},sibling));
        assert(node.count==5 && sibling.count==2 && sibling.level==0);
        // Equal boxes seed indices 0/1; distribution is array order, not
        // seed/assignment order. The reinserted instance remains at index 5.
        assert(node.entries[4].instance==200+removed);
        assert(sibling.entries[0].instance==(removed==1?205:201));
        assert(sibling.entries[1].instance==777);
    }
    Node branch;branch.level=2;
    for(unsigned i=0;i<6;++i)assert(branch.append({{},i+1,i+101}));
    assert(!branch.eraseInstance(101)); // Internal nodes are removed by slot.
    assert(branch.erase(0));assert(branch.entries[0].child==6);
    assert(branch.append({{},7,107}));
    Node sibling;sibling.level=8;
    Node::Entry bad;bad.bounds.left.bits=0x7f800000u;
    assert(!branch.split(bad,sibling));
    assert(branch.count==6 && branch.entries[0].child==6 && sibling.level==8);
    assert(!branch.split({},branch));assert(branch.count==6);
    assert(branch.split({},sibling));assert(sibling.level==2 && branch.level==2);
    Box boxes[7]={};check(boxes);
    Split::Box identical[7]={};Split::Result result;
    assert(Split::partition(identical,result));
    // Equal growth goes to group zero until the minimum-fill rule intervenes.
    const unsigned tied[7]={0,1,0,0,0,0,1};
    for(unsigned i=0;i<7;++i)assert(result.group[i]==tied[i]);
    std::uint32_t state=0x217fea93u;
    auto random=[&](){state=state*1664525u+1013904223u;return state;};
    for(unsigned trial=0;trial<8192;++trial){
        for(auto& b:boxes){
            float x=(int(random()%16384)-8192)/8.0f,y=(int(random()%16384)-8192)/8.0f;
            b={x,y,x+float(random()%2048)/8,y+float(random()%2048)/8};
        }
        check(boxes);
    }
    for(auto bad:{0x7f800000u,0x7fc00000u,1u,0xff800000u}){
        identical[3].left.bits=bad;Split::Result before=result;
        assert(!Split::partition(identical,result));
        for(unsigned i=0;i<7;++i)assert(result.group[i]==before.group[i]);
        assert(result.count[0]==before.count[0]&&result.count[1]==before.count[1]);
    }
    identical[3]={real(2),real(0),real(1),real(1)};
    assert(!Split::partition(identical,result)); // Inverted source bounds.
    identical[3]={real(0),real(0),Split::Real{0x7f7fffffu},Split::Real{0x7f7fffffu}};
    assert(!Split::partition(identical,result)); // Area overflow is not infinity.
    for(unsigned i=0;i<7;++i)assert(result.group[i]==tied[i]);
    assert(result.count[0]==5 && result.count[1]==2);
    std::puts("Spatial nodes: swap-last removal/reinsert, stable split distribution; 8192 binary32 partitions, tie/minimum-fill and atomic failure checks passed");
}
