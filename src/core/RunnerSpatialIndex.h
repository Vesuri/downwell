#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_INDEX_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_INDEX_H
#include "RunnerSpatialNode.h"
#include "RunnerSpatialLine.h"
#include "Profile.h"
namespace downwell {
// Source-ordered spatial owner. It does not select eligible instances or
// maintain the runner's dirty queue.
// A failed mutation latches failure: partially changed trees cannot be queried.
// Clear is required before reuse. Callbacks must not mutate this owner.
template<unsigned Capacity,bool IntegralStorage=true> class RunnerSpatialIndex {
    static_assert(Capacity>0,"Spatial index needs a root");
    using Math=RunnerSpatialSplit;
    // One representation per tree. The union adds no per-node memory. Whole
    // union assignment switches the active member when the tree falls back.
    union StoredBox {
        Math::Box binary;
        Math::PixelBox pixel;
        StoredBox():pixel{}{}
        explicit StoredBox(Math::Box b):binary(b){}
        explicit StoredBox(Math::PixelBox b):pixel(b){}
    };
    using Node=RunnerSpatialNodeStorage<StoredBox>;
    using Entry=typename Node::Entry;
    static_assert(sizeof(Node)==sizeof(RunnerSpatialNode),"Integer bounds must not enlarge the node pool");
    struct Search {
        Math::Box binary;
        Math::PixelBox pixel{};
        bool integral;
        Search(Math::Box b,bool pixelMode):binary(b),integral(pixelMode&&Math::decodePixels(b,pixel)){}
    };
public:
    using Box=Math::Box;
    void clear(){
        used_=live_=1;root_=0;free_=Capacity;good_=true;pixelMode_=IntegralStorage;
        nodes_[0].count=0;nodes_[0].level=0;
    }
    bool valid()const{return good_;}
    unsigned nodeCount()const{return live_;}
    int height()const{return nodes_[root_].level;}
    bool insert(Box box,std::uint32_t instance){
        DW_PROFILE_SCOPE(profileSpatialInsert);
        if(!good_)return false;
        if(!Math::valid(box)){good_=false;return false;}
        Math::PixelBox pixel;
        if(pixelMode_&&(!Math::decodePixels(box,pixel)||negativeZero(box)))makeBinary();
        const StoredBox stored=pixelMode_?StoredBox(pixel):StoredBox(box);
        if(!insertRoot({stored,0,instance},0)){good_=false;return false;}
        return true;
    }
    // Source wrapper can retry against the complete root bounds when the old
    // bounds no longer locate the instance. Not-found is a successful operation
    // with removed=false; an invalid/failed mutation preserves the output flag.
    bool remove(Box oldBounds,std::uint32_t instance,bool retry,bool& removed){
        DW_PROFILE_SCOPE(profileSpatialRemove);
        if(!good_)return false;
        if(!Math::valid(oldBounds)){good_=false;return false;}
        unsigned detached=Capacity;
        bool found=eraseAt(root_,Search(oldBounds,pixelMode_),instance,detached);
        if(!found && retry && nodes_[root_].count)
            found=eraseAt(root_,Search(binary(bounds(root_)),pixelMode_),instance,detached);
        if(!found){removed=false;return true;}
        // Detachment prepends while recursion unwinds: higher-level nodes are
        // processed first. Reinsert entries at their original node level.
        while(detached!=Capacity){
            const unsigned node=detached;detached=links_[node];
            for(unsigned i=0;i<nodes_[node].count;++i)
                if(!insertRoot(nodes_[node].entries[i],nodes_[node].level)){good_=false;return false;}
            release(node);
        }
        if(nodes_[root_].count==1 && nodes_[root_].level>0){
            const unsigned old=root_;root_=nodes_[old].entries[0].child;release(old);
        }
        removed=true;return true;
    }
    // False means invalid query/index, not an early-stop callback. A callback
    // returning false stops all traversal after that hit, just like the runner.
    template<class Visit> bool query(Box box,Visit visit)const{
        if(!good_ || !Math::valid(box))return false;
        visitAt(root_,Search(box,pixelMode_),visit);return true;
    }
    // 0x500860 walks entries in their stored order at every tree level. It
    // does not clip the line passed to children or sort leaves by hit distance.
    template<class Visit> bool queryLine(const RunnerLineArguments& args,Visit visit)const{
        RunnerSpatialLine line;
        if(!good_||!line.assign(args))return false;
        bool good=true;visitLineAt(root_,line,visit,good);return good;
    }
private:
    static constexpr unsigned depthLimit=[](){unsigned n=Capacity,bits=0;
        do{++bits;n>>=1;}while(n);return bits;}();
    // Mutation is non-reentrant. Retain bounded descent scratch with its owner
    // rather than stacking it under generated-instance Create and world Step.
    unsigned parents_[depthLimit]{},selections_[depthLimit]{};
    Node nodes_[Capacity]={};unsigned links_[Capacity]={};
    unsigned used_=1,live_=1,root_=0,free_=Capacity;bool good_=true;
    bool pixelMode_=IntegralStorage;
    static bool negativeZero(const Box& b){
        return b.left.bits==0x80000000u||b.top.bits==0x80000000u||
            b.right.bits==0x80000000u||b.bottom.bits==0x80000000u;
    }
    Box binary(const StoredBox& b)const{return pixelMode_?Math::encodePixels(b.pixel):b.binary;}
    void makeBinary(){
        // Also convert detached/free nodes: they may retain entries, and a
        // detached node can be pending reinsertion. No callbacks run here.
        for(unsigned n=0;n<used_;++n)for(unsigned i=0;i<nodes_[n].count;++i){
            auto& b=nodes_[n].entries[i].bounds;b=StoredBox(Math::encodePixels(b.pixel));
        }
        pixelMode_=false;
    }
    StoredBox unite(const StoredBox& a,const StoredBox& b)const{
        if(!pixelMode_)return StoredBox(Math::unite(a.binary,b.binary));
        const auto& x=a.pixel;const auto& y=b.pixel;
        return StoredBox(Math::PixelBox{x.left<y.left?x.left:y.left,x.top<y.top?x.top:y.top,
            x.right>y.right?x.right:y.right,x.bottom>y.bottom?x.bottom:y.bottom});
    }
    bool overlaps(const Search& a,const StoredBox& b)const{
        if(a.integral){
            const auto& x=a.pixel;const auto& y=b.pixel;
            return x.right>=y.left&&y.right>=x.left&&x.bottom>=y.top&&y.bottom>=x.top;
        }
        return Math::overlaps(a.binary,binary(b));
    }
    bool choose(const Node& n,const StoredBox& incoming,unsigned& selected)const{
        DW_PROFILE_SCOPE(profileSpatialChoose);
        if(pixelMode_&&Math::choosePixelValues([&](unsigned i,Math::PixelBox& out){
            out=n.entries[i].bounds.pixel;return true;
        },n.count,incoming.pixel,selected))return true;
        Box boxes[6];for(unsigned i=0;i<n.count;++i)boxes[i]=binary(n.entries[i].bounds);
        return Math::chooseBinary32Child(boxes,n.count,binary(incoming),selected);
    }
    bool partition(const StoredBox (&boxes)[7],Math::Result& groups)const{
        DW_PROFILE_SCOPE(profileSpatialPartition);
        if(pixelMode_){
            Math::PixelBox pixels[7];for(unsigned i=0;i<7;++i)pixels[i]=boxes[i].pixel;
            if(Math::partitionPixelValues(pixels,groups))return true;
        }
        Box values[7];for(unsigned i=0;i<7;++i)values[i]=binary(boxes[i]);
        return Math::partitionBinary32(values,groups);
    }
    bool insertRoot(const Entry& entry,int level){
        unsigned sibling=Capacity;
        if(!insertAt(root_,entry,level,sibling))return false;
        if(sibling!=Capacity){
            unsigned newRoot;
            if(!allocate(nodes_[root_].level+1,newRoot))return false;
            nodes_[newRoot].append({bounds(root_),root_,0});
            nodes_[newRoot].append({bounds(sibling),sibling,0});root_=newRoot;
        }
        return true;
    }
    bool allocate(int level,unsigned& result){
        if(free_!=Capacity){result=free_;free_=links_[result];}
        else {if(used_==Capacity)return false;result=used_++;}
        // Only [0,count) is live. Every append/split overwrites its new entry;
        // stale union members beyond count must never be read or converted.
        ++live_;nodes_[result].count=0;nodes_[result].level=level;return true;
    }
    void release(unsigned node){links_[node]=free_;free_=node;--live_;}
    StoredBox bounds(unsigned node)const{
        const Node& n=nodes_[node];StoredBox b=n.entries[0].bounds;
        for(unsigned i=1;i<n.count;++i)b=unite(b,n.entries[i].bounds);
        return b;
    }
    bool add(unsigned node,const Entry& entry,unsigned& sibling){
        if(nodes_[node].count<6)return nodes_[node].append(entry);
        if(!allocate(nodes_[node].level,sibling))return false;
        return nodes_[node].splitUsing(entry,nodes_[sibling],[&](const StoredBox (&boxes)[7],Math::Result& groups){
            return partition(boxes,groups);
        });
    }
    bool insertAt(unsigned node,const Entry& entry,int level,unsigned& sibling){
        // Every internal node has at least two children: the bit width of
        // Capacity bounds tree height. Keep only the descent path, rather than
        // a complete split/child-selection call frame at every tree level.
        unsigned depth=0;
        if(nodes_[node].level!=level){
            while(nodes_[node].level!=level){
                const Node& n=nodes_[node];
                if(n.level<level||depth==depthLimit)return false;
                unsigned selected;
                if(!choose(n,entry.bounds,selected))return false;
                parents_[depth]=node;selections_[depth++]=selected;
                node=n.entries[selected].child;
            }
        }
        unsigned split=Capacity;
        if(!add(node,entry,split))return false;
        while(depth){
            const unsigned child=node;node=parents_[--depth];
            auto& selected=nodes_[node].entries[selections_[depth]];
            if(split==Capacity)selected.bounds=unite(selected.bounds,entry.bounds);
            else {
                selected.bounds=bounds(child);
                const unsigned newChild=split;split=Capacity;
                if(!add(node,{bounds(newChild),newChild,0},split))return false;
            }
        }
        sibling=split;return true;
    }
    bool eraseAt(unsigned node,const Search& box,std::uint32_t instance,unsigned& detached){
        Node& n=nodes_[node];
        if(n.level==0)return n.eraseInstance(instance);
        for(unsigned i=0;i<n.count;++i){
            auto& entry=n.entries[i];if(!overlaps(box,entry.bounds))continue;
            const unsigned child=entry.child;
            if(!eraseAt(child,box,instance,detached))continue;
            if(nodes_[child].count>=2)entry.bounds=bounds(child);
            else {links_[child]=detached;detached=child;n.erase(i);}
            return true;
        }
        return false;
    }
    template<class Visit> bool visitAt(unsigned node,const Search& box,Visit& visit)const{
        const Node& n=nodes_[node];
        for(unsigned i=0;i<n.count;++i){
            const auto& e=n.entries[i];if(!overlaps(box,e.bounds))continue;
            if(n.level>0){if(!visitAt(e.child,box,visit))return false;}
            else if(!visit(e.instance))return false;
        }
        return true;
    }
    template<class Visit> bool visitLineAt(unsigned node,const RunnerSpatialLine& line,Visit& visit,bool& good)const{
        const auto& n=nodes_[node];
        for(unsigned i=0;i<n.count;++i){
            const auto& entry=n.entries[i];bool hit=false;
            if(!line.overlaps(binary(entry.bounds),hit)){good=false;return false;}
            if(!hit)continue;
            if(n.level>0){if(!visitLineAt(entry.child,line,visit,good))return false;}
            else if(!visit(entry.instance))return false;
        }
        return true;
    }
};
}
#endif
