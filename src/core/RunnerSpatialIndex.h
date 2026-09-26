#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_INDEX_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_INDEX_H
#include "RunnerSpatialNode.h"
namespace downwell {
// Source-ordered insertion/query owner. It does not select eligible instances,
// maintain the runner's dirty queue, or implement removal/condensation yet.
// A failed mutation latches failure: partially changed trees cannot be queried.
// Clear is required before reuse. Callbacks must not mutate this owner.
template<unsigned Capacity> class RunnerSpatialIndex {
    static_assert(Capacity>0,"Spatial index needs a root");
    using Node=RunnerSpatialNode;
    using Math=RunnerSpatialSplit;
public:
    using Box=Math::Box;
    void clear(){used_=1;root_=0;good_=true;nodes_[0]=Node{};}
    bool valid()const{return good_;}
    unsigned nodeCount()const{return used_;}
    int height()const{return nodes_[root_].level;}
    bool insert(Box box,std::uint32_t instance){
        if(!good_)return false;
        if(!Math::valid(box)){good_=false;return false;}
        unsigned sibling=Capacity;
        if(!insertAt(root_,{box,0,instance},sibling)){good_=false;return false;}
        if(sibling!=Capacity){
            unsigned newRoot;
            if(!allocate(nodes_[root_].level+1,newRoot)){good_=false;return false;}
            nodes_[newRoot].append({bounds(root_),root_,0});
            nodes_[newRoot].append({bounds(sibling),sibling,0});root_=newRoot;
        }
        return true;
    }
    // False means invalid query/index, not an early-stop callback. A callback
    // returning false stops all traversal after that hit, just like the runner.
    template<class Visit> bool query(Box box,Visit visit)const{
        if(!good_ || !Math::valid(box))return false;
        visitAt(root_,box,visit);return true;
    }
private:
    Node nodes_[Capacity]={};unsigned used_=1,root_=0;bool good_=true;
    bool allocate(int level,unsigned& result){
        if(used_==Capacity)return false;
        result=used_++;nodes_[result]=Node{};nodes_[result].level=level;return true;
    }
    Box bounds(unsigned node)const{
        const Node& n=nodes_[node];Box b=n.entries[0].bounds;
        for(unsigned i=1;i<n.count;++i)b=Math::unite(b,n.entries[i].bounds);
        return b;
    }
    bool add(unsigned node,const Node::Entry& entry,unsigned& sibling){
        if(nodes_[node].count<6)return nodes_[node].append(entry);
        if(!allocate(nodes_[node].level,sibling))return false;
        return nodes_[node].split(entry,nodes_[sibling]);
    }
    bool insertAt(unsigned node,const Node::Entry& entry,unsigned& sibling){
        Node& n=nodes_[node];
        if(n.level==0)return add(node,entry,sibling);
        Box choices[6];for(unsigned i=0;i<n.count;++i)choices[i]=n.entries[i].bounds;
        unsigned selected;
        if(!Math::chooseChild(choices,n.count,entry.bounds,selected))return false;
        unsigned child=n.entries[selected].child,newChild=Capacity;
        if(!insertAt(child,entry,newChild))return false;
        if(newChild==Capacity){
            n.entries[selected].bounds=Math::unite(n.entries[selected].bounds,entry.bounds);
            return true;
        }
        n.entries[selected].bounds=bounds(child);
        return add(node,{bounds(newChild),newChild,0},sibling);
    }
    template<class Visit> bool visitAt(unsigned node,Box box,Visit& visit)const{
        const Node& n=nodes_[node];
        for(unsigned i=0;i<n.count;++i){
            const auto& e=n.entries[i];if(!Math::overlaps(box,e.bounds))continue;
            if(n.level>0){if(!visitAt(e.child,box,visit))return false;}
            else if(!visit(e.instance))return false;
        }
        return true;
    }
};
}
#endif
