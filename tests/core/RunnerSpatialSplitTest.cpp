#include "core/RunnerSpatialSplit.h"
#include "core/RunnerSpatialNode.h"
#include "core/RunnerSpatialIndex.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>
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
static Split::Box packed(Box b){return {real(b[0]),real(b[1]),real(b[2]),real(b[3])};}
struct ReferenceTree {
    struct Entry {Box box;unsigned link;};
    struct Node {int level;std::vector<Entry> entries;};
    std::vector<Node> nodes={{0,{}}};unsigned root=0;
    Box bounds(unsigned n)const{
        Box out=nodes[n].entries[0].box;
        for(const auto& e:nodes[n].entries)out=merge(out,e.box);
        return out;
    }
    int add(unsigned n,Entry e,int level=0){
        if(nodes[n].level>level){
            unsigned best=0;float growth=INFINITY,oldArea=INFINITY;
            for(unsigned i=0;i<nodes[n].entries.size();++i){
                Box b=nodes[n].entries[i].box;float size=area(b),delta=area(merge(b,e.box))-size;
                if(delta<growth || (delta==growth && size<oldArea)){best=i;growth=delta;oldArea=size;}
            }
            unsigned child=nodes[n].entries[best].link;int split=add(child,e,level);
            nodes[n].entries[best].box=bounds(child);
            if(split<0)return -1;
            e={bounds(unsigned(split)),unsigned(split)};
        }
        nodes[n].entries.push_back(e);
        if(nodes[n].entries.size()<7)return -1;
        Box boxes[7];for(unsigned i=0;i<7;++i)boxes[i]=nodes[n].entries[i].box;
        auto groups=reference(boxes);auto entries=nodes[n].entries;
        unsigned sibling=unsigned(nodes.size());nodes.push_back({nodes[n].level,{}});
        nodes[n].entries.clear();
        for(unsigned i=0;i<7;++i)nodes[groups[i]?sibling:n].entries.push_back(entries[i]);
        return int(sibling);
    }
    void insert(Box b,unsigned id){
        insertEntry({b,id},0);
    }
    void insertEntry(Entry entry,int level){
        int sibling=add(root,entry,level);if(sibling<0)return;
        Node parent{nodes[root].level+1,{{bounds(root),root},{bounds(unsigned(sibling)),unsigned(sibling)}}};
        root=unsigned(nodes.size());nodes.push_back(parent);
    }
    bool erase(unsigned n,Box query,unsigned id,std::vector<unsigned>& detached){
        for(unsigned i=0;i<nodes[n].entries.size();++i){
            Entry entry=nodes[n].entries[i];
            if(nodes[n].level){
                auto b=entry.box;
                if(query[0]>b[2] || query[2]<b[0] || query[1]>b[3] || query[3]<b[1])continue;
                if(!erase(entry.link,query,id,detached))continue;
                if(nodes[entry.link].entries.size()>=2){nodes[n].entries[i].box=bounds(entry.link);return true;}
                detached.push_back(entry.link);
            }else if(entry.link!=id)continue;
            nodes[n].entries[i]=nodes[n].entries.back();nodes[n].entries.pop_back();return true;
        }
        return false;
    }
    bool remove(Box query,unsigned id,bool retry){
        std::vector<unsigned> detached;
        bool found=erase(root,query,id,detached);
        if(!found && retry && !nodes[root].entries.empty())found=erase(root,bounds(root),id,detached);
        if(!found)return false;
        for(auto it=detached.rbegin();it!=detached.rend();++it){
            auto entries=nodes[*it].entries;int level=nodes[*it].level;
            for(auto e:entries)insertEntry(e,level);
        }
        if(nodes[root].level && nodes[root].entries.size()==1)root=nodes[root].entries[0].link;
        return true;
    }
    void visit(unsigned n,Box query,std::vector<unsigned>& hits)const{
        for(const auto& e:nodes[n].entries){
            const auto b=e.box;
            if(query[0]>b[2] || query[2]<b[0] || query[1]>b[3] || query[3]<b[1])continue;
            if(nodes[n].level)visit(e.link,query,hits);else hits.push_back(e.link);
        }
    }
};
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
    // Retain the previous binary64 path as an independent arithmetic oracle.
    auto productCheck=[](std::uint32_t a,std::uint32_t b){
        Split::Real left{a},right{b},actual{0x12345678},expected=actual;
        downwell::MotionBinary64 product;
        const bool valid=Split::valid(left)&&Split::valid(right);
        const bool ok=valid&&downwell::multiplyBinary64(left.promoted(),right.promoted(),product)&&expected.assign(product);
        assert(downwell::multiplyBinary32(left,right,actual)==ok);
        assert(actual.bits==expected.bits);
    };
    const std::uint32_t fractions[]={0,1,0x3fffff,0x400000,0x7ffffe,0x7fffff};
    for(unsigned a=1;a<255;++a)for(unsigned b=1;b<255;++b)
        for(auto fraction:fractions)
            productCheck((a<<23)|fraction,(b<<23)|(0x7fffff-fraction)|0x80000000u);
    const std::uint32_t productEdges[]={0,0x80000000,1,0x800000,0x00800001,0x3f800000,
        0x3f800001,0x3fc00000,0x7f7fffff,0x7f800000,0x7fc00000,0xffffffff};
    for(auto a:productEdges)for(auto b:productEdges)productCheck(a,b);
    auto subtractCheck=[](std::uint32_t a,std::uint32_t b){
        Split::Real left{a},right{b},actual{0x12345678},expected=actual;
        auto wide=left.promoted();
        const bool ok=wide.add(Split::negate(right).promoted())&&expected.assign(wide);
        assert(Split::sub(left,right,actual)==ok&&actual.bits==expected.bits);
        auto aliasLeft=left,aliasRight=right;
        assert(Split::sub(aliasLeft,right,aliasLeft)==ok);
        assert(aliasLeft.bits==(ok?expected.bits:left.bits));
        assert(Split::sub(left,aliasRight,aliasRight)==ok);
        assert(aliasRight.bits==(ok?expected.bits:right.bits));
    };
    for(auto a:productEdges)for(auto b:productEdges)subtractCheck(a,b);
    for(unsigned a=1;a<255;++a)for(unsigned b=1;b<255;++b)
        for(auto fraction:fractions)for(unsigned signs=0;signs<4;++signs){
            subtractCheck((a<<23)|fraction|((signs&1u)<<31),
                (b<<23)|(0x7fffff-fraction)|((signs&2u)<<30));
            // Close cancellation, sticky tails and halfway rounding.
            subtractCheck((a<<23)|fraction|((signs&1u)<<31),
                (b<<23)|fraction|((signs&2u)<<30));
        }
    std::uint32_t seed=0x612de529;
    for(unsigned i=0;i<1000000;++i){
        seed=seed*1664525u+1013904223u;const auto a=seed;
        seed=seed*1664525u+1013904223u;productCheck(a,seed);
        subtractCheck(a,seed);subtractCheck(a,a);
        subtractCheck(a,0);subtractCheck(a,0x80000000u);
        subtractCheck(0,a);subtractCheck(0x80000000u,a);
    }
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
    // In-place compaction must preserve an incoming entry that aliases either
    // destination. Compare it with the same split using a detached copy.
    for(unsigned alias=0;alias<7;++alias){
        Node source;
        for(unsigned i=0;i<6;++i)assert(source.append({{},i+1,i+11}));
        Node other;assert(other.append({{},99,199}));
        Node reference=source,referenceOther=other;
        const Node::Entry incoming=alias<6?source.entries[alias]:other.entries[0];
        assert(reference.split(incoming,referenceOther));
        assert(source.split(alias<6?source.entries[alias]:other.entries[0],other));
        for(unsigned output=0;output<2;++output){
            const auto& actual=output?other:source;
            const auto& expected=output?referenceOther:reference;
            assert(actual.count==expected.count);
            for(unsigned i=0;i<6;++i){
                assert(actual.entries[i].child==expected.entries[i].child);
                assert(actual.entries[i].instance==expected.entries[i].instance);
            }
        }
    }
    Box boxes[7]={};check(boxes);
    // Population tie-breaks still change even when a group's rectangle does
    // not grow. Exercise every insertion order of equal/nested rectangles.
    const Box nested[7]={{-8,-8,8,8},{-8,-8,8,8},{-4,-4,4,4},
        {-2,-2,2,2},{-8,-2,-4,2},{4,-2,8,2},{0,0,0,0}};
    unsigned permutation[]={0,1,2,3,4,5,6},permutations=0;
    do{
        for(unsigned i=0;i<7;++i)boxes[i]=nested[permutation[i]];
        check(boxes);++permutations;
    }while(std::next_permutation(permutation,permutation+7));
    assert(permutations==5040);
    Split::Box identical[7]={};Split::Result result;
    assert(Split::partition(identical,result));
    // Equal growth goes to group zero until the minimum-fill rule intervenes.
    const unsigned tied[7]={0,1,0,0,0,0,1};
    for(unsigned i=0;i<7;++i)assert(result.group[i]==tied[i]);
    std::uint32_t state=0x217fea93u;
    auto random=[&](){state=state*1664525u+1013904223u;return state;};
    const auto partitionSeed=state;
    unsigned pixelAccepted=0,pixelFallback=0;
    unsigned pixelPartitions=0;
    auto pixelPartitionCheck=[&](const Split::Box (&input)[7]){
        Split::Result expected,actual,fast;
        const bool reference=Split::partitionBinary32(input,expected);
        assert(Split::partition(input,actual)==reference);
        const bool accepted=Split::partitionPixels(input,fast);
        if(accepted)++pixelPartitions;
        if(reference){
            for(unsigned i=0;i<7;++i)assert(actual.group[i]==expected.group[i]&&(!accepted||fast.group[i]==expected.group[i]));
            for(unsigned i=0;i<2;++i)assert(actual.count[i]==expected.count[i]&&(!accepted||fast.count[i]==expected.count[i]));
        }else assert(!accepted);
        return accepted;
    };
    auto childCheck=[&](const Split::Box* input,unsigned count,Split::Box incoming){
        unsigned reference=999,actual=999,fast=999;
        const bool expected=Split::chooseBinary32Child(input,count,incoming,reference);
        assert(Split::chooseChild(input,count,incoming,actual)==expected&&actual==reference);
        const bool accepted=Split::choosePixelChild(input,count,incoming,fast);
        if(accepted){++pixelAccepted;assert(expected&&fast==reference);}
        else{++pixelFallback;assert(fast==999);}
        return accepted;
    };
    for(unsigned trial=0;trial<32768;++trial){
        Split::Box input[7];
        const int originX=int(random()%20000000)-10000000,originY=int(random()%20000000)-10000000;
        for(auto& b:input){
            const int x=originX+int(random()%256),y=originY+int(random()%16384);
            b=packed({float(x),float(y),float(x+int(random()%256)),float(y+int(random()%128))});
        }
        childCheck(input,1+trial%6,input[6]);
        if(trial<4096)assert(pixelPartitionCheck(input));
    }
    {
        Split::Box input[6]{};
        input[0]=packed({0,0,4096,4096});
        assert(childCheck(input,1,input[0])); // Exactly 2^24 area.
        input[0]=packed({0,0,4097,4096});
        assert(!childCheck(input,1,input[0])); // Possible rounded products: fall back.
        input[0]=packed({0,0,65535,0});assert(childCheck(input,1,input[0]));
        input[0]=packed({0,0,65536,0});assert(!childCheck(input,1,input[0]));
        input[0]=packed({0.5f,0,4,4});assert(!childCheck(input,1,input[0]));
        input[0]=packed({-16777215,-16777215,-16777200,-16777200});assert(childCheck(input,1,input[0]));
        input[0]=packed({16777216,0,16777216,0});assert(!childCheck(input,1,input[0]));
        for(auto& b:input)b=packed({-0.0f,-0.0f,0.0f,0.0f});
        assert(childCheck(input,6,input[0]));
        unsigned tiedChild=999;assert(Split::choosePixelChild(input,6,input[0],tiedChild)&&tiedChild==0);
        childCheck(input,0,input[0]);childCheck(input,7,input[0]);
        input[0].left.bits=0x7fc00000u;childCheck(input,1,input[0]);
    }
    assert(pixelAccepted>30000&&pixelFallback>0);
    assert(pixelPartitions==4096);
    {
        Split::Box input[7];
        for(auto& box:input)box=packed({0,0,4096,4096});
        assert(pixelPartitionCheck(input)); // Source's -whole-1 seed rounds here.
        input[0]=packed({0,0,4097,4096});assert(!pixelPartitionCheck(input));
        input[0]=packed({0.5f,0,4,4});assert(!pixelPartitionCheck(input));
        for(auto& box:input)box=packed({0.0f,0.0f,-0.0f,-0.0f});
        assert(pixelPartitionCheck(input));
    }
    state=partitionSeed; // Preserve the established partition/mutation corpus.
    for(unsigned trial=0;trial<8192;++trial){
        for(auto& b:boxes){
            float x=(int(random()%16384)-8192)/8.0f,y=(int(random()%16384)-8192)/8.0f;
            b={x,y,x+float(random()%2048)/8,y+float(random()%2048)/8};
        }
        check(boxes);
    }
    for(unsigned trial=0;trial<32;++trial){
        downwell::RunnerSpatialIndex<256> tree;ReferenceTree scalar;
        Box live[200];
        for(unsigned id=0;id<200;++id){
            float x=(int(random()%4096)-2048)/8.0f,y=(int(random()%4096)-2048)/8.0f;
            Box b={x,y,x+float(random()%512)/8,y+float(random()%512)/8};
            live[id]=b;
            assert(tree.insert(packed(b),id));scalar.insert(b,id);
            assert(tree.height()==scalar.nodes[scalar.root].level);
            std::vector<unsigned> actual,expected;
            assert(tree.query(packed(b),[&](unsigned hit){actual.push_back(hit);return true;}));
            scalar.visit(scalar.root,b,expected);assert(actual==expected);
            unsigned first=999,calls=0;
            assert(tree.query(packed(b),[&](unsigned hit){first=hit;++calls;return false;}));
            assert(calls==1 && first==expected.front());
        }
        // Complete traversal checks order even for entries outside local queries.
        Box all={-10000,-10000,10000,10000};std::vector<unsigned> actual,expected;
        assert(tree.query(packed(all),[&](unsigned hit){actual.push_back(hit);return true;}));
        scalar.visit(scalar.root,all,expected);assert(actual==expected && actual.size()==200);
        auto compare=[&](){
            actual.clear();expected.clear();
            assert(tree.query(packed(all),[&](unsigned hit){actual.push_back(hit);return true;}));
            scalar.visit(scalar.root,all,expected);assert(actual==expected);
            assert(tree.height()==scalar.nodes[scalar.root].level);
        };
        for(unsigned move=0;move<128;++move){
            unsigned id=random()%200;bool removed=false;
            // Deliberately stale pruning bounds exercise the root retry path.
            Box old=move%3?live[id]:Box{9000,9000,9001,9001};
            assert(tree.remove(packed(old),id,true,removed) && removed);
            assert(scalar.remove(old,id,true));compare();
            float x=(int(random()%4096)-2048)/8.0f,y=(int(random()%4096)-2048)/8.0f;
            live[id]={x,y,x+8,y+8};
            assert(tree.insert(packed(live[id]),id));scalar.insert(live[id],id);compare();
        }
        bool removed=true;
        assert(tree.remove(packed(all),999,false,removed) && !removed);compare();
        for(unsigned id=0;id<200;++id){
            assert(tree.remove(packed(live[id]),id,false,removed) && removed);
            assert(scalar.remove(live[id],id,false));compare();
        }
        assert(tree.height()==0 && tree.nodeCount()==1);
        // Repeated full refill must reuse reclaimed slots, not exhaust the pool.
        for(unsigned id=0;id<200;++id)assert(tree.insert(packed(live[id]),id));
        tree.clear();assert(tree.valid() && tree.height()==0 && tree.nodeCount()==1);
        assert(tree.query(packed(all),[](unsigned){assert(false);return true;}));
    }
    // Native production ownership has thousands of slots; exercise a deeper
    // descent/unwind than the small randomized mutation tapes above.
    auto deep=std::make_unique<downwell::RunnerSpatialIndex<4096>>();
    ReferenceTree deepReference;
    for(unsigned id=0;id<2400;++id){
        const float x=float(id%32)*16,y=float(id/32)*16;
        const Box b={x,y,x+16,y+16};
        assert(deep->insert(packed(b),id));deepReference.insert(b,id);
    }
    assert(deep->height()>=4&&deep->height()==deepReference.nodes[deepReference.root].level);
    for(unsigned row=0;row<75;++row){
        const Box b={0,float(row)*16,512,float(row)*16+16};
        std::vector<unsigned> actual,expected;
        assert(deep->query(packed(b),[&](unsigned id){actual.push_back(id);return true;}));
        deepReference.visit(deepReference.root,b,expected);assert(actual==expected);
    }
    downwell::RunnerSpatialIndex<3> equalTree;
    for(unsigned i=0;i<7;++i)assert(equalTree.insert({},i));
    unsigned ordered[7]={0,2,3,4,5,1,6},position=0;
    assert(equalTree.query({},[&](unsigned id){assert(id==ordered[position++]);return true;}));
    assert(position==7);
    assert(equalTree.query(packed({1,1,2,2}),[](unsigned){assert(false);return true;}));
    Split::Box invalid{};invalid.left.bits=0x7fc00000u;
    assert(!equalTree.query(invalid,[](unsigned){assert(false);return true;}));
    assert(equalTree.valid()); // Invalid read does not poison a valid tree.
    assert(!equalTree.insert(invalid,100) && !equalTree.valid());
    downwell::RunnerSpatialIndex<3> edges;
    assert(edges.insert(packed({0,0,1,1}),10));
    unsigned touches=0;
    assert(edges.query(packed({1,1,2,2}),[&](unsigned id){assert(id==10);++touches;return true;}));
    assert(touches==1); // Inclusive broad-phase corner contact.
    downwell::RunnerSpatialIndex<1> small;
    for(unsigned i=0;i<6;++i)assert(small.insert({},i));
    assert(!small.insert({},6) && !small.valid());
    bool removalResult=true;
    assert(!small.remove({},0,true,removalResult) && removalResult);
    assert(!small.query({},[](unsigned){assert(false);return true;}));
    assert(!small.insert({},7));small.clear();assert(small.insert({},8));
    assert(!small.remove(invalid,8,true,removalResult) && removalResult && !small.valid());
    // Capacity for the split sibling but not its new root also fails closed.
    downwell::RunnerSpatialIndex<2> rootFailure;
    for(unsigned i=0;i<6;++i)assert(rootFailure.insert({},i));
    assert(!rootFailure.insert({},6) && !rootFailure.valid());
    assert(!rootFailure.query({},[](unsigned){assert(false);return true;}));
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
    std::puts("Spatial index: 6400 insertion/first-hit and 14624 mutation-order comparisons, 4096 moves, root retry/collapse/reuse; 8192 partitions passed");
}
