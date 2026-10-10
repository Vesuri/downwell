#ifndef DOWNWELL_CORE_RUNNER_PREPARED_DRAW_LIST_H
#define DOWNWELL_CORE_RUNNER_PREPARED_DRAW_LIST_H
#include <cstdint>
namespace downwell {
// Read-only Draw projection. Source layers still own ordering; their mutation
// adapter supplies each insertion's nearest live predecessor. Inactive nodes
// remain in the list because they terminate a layer, rather than just hiding
// themselves. Tokens refer to prepared renderer identities, never positions.
template<unsigned Layers,unsigned Elements> class RunnerPreparedDrawList {
public:
    static constexpr unsigned End=Elements;
    enum Action : std::uint8_t {absent,skip,endLayer,draw};
private:
    static_assert(Elements<65535,"Draw projection links must fit a word");
    struct Node {std::uint16_t previous=End,next=End,token=0;Action action=absent;};
    Node nodes_[Elements]{};
    std::uint16_t heads_[Layers]{};
    std::uint32_t revision_=0;
    void changed(){if(revision_!=0xffffffffu)++revision_;}
public:
    RunnerPreparedDrawList(){clear();}
    void clear(){for(auto& node:nodes_)node=Node{};for(auto& head:heads_)head=End;changed();}
    std::uint32_t revision()const{return revision_;}
    void invalidateOrder(){changed();}
    bool contains(unsigned e)const{return e<Elements&&nodes_[e].action!=absent;}
    bool attach(unsigned layer,unsigned e,unsigned previous){
        if(layer>=Layers||e>=Elements||contains(e)||
           (previous!=End&&!contains(previous)))return false;
        auto& node=nodes_[e];node.action=skip;node.previous=previous;
        node.next=previous==End?heads_[layer]:nodes_[previous].next;
        if(node.next!=End)nodes_[node.next].previous=e;
        if(previous==End)heads_[layer]=e;else nodes_[previous].next=e;
        changed();return true;
    }
    bool detach(unsigned layer,unsigned e){
        if(layer>=Layers||!contains(e))return false;
        auto& node=nodes_[e];
        if(node.previous==End)heads_[layer]=node.next;else nodes_[node.previous].next=node.next;
        if(node.next!=End)nodes_[node.next].previous=node.previous;
        node=Node{};changed();return true;
    }
    bool publish(unsigned e,unsigned token,Action action){
        if(!contains(e)||token>65535||action==absent)return false;
        auto& node=nodes_[e];
        if(node.token!=token||node.action!=action){node.token=token;node.action=action;changed();}
        return true;
    }
    bool reposition(unsigned layer,unsigned e,unsigned previous){
        if(!contains(e)||previous==e)return false;
        const auto token=nodes_[e].token;const auto action=nodes_[e].action;
        return detach(layer,e)&&attach(layer,e,previous)&&publish(e,token,action);
    }
    // Depth merge/reconciliation may reorder complete layers. Relink at that
    // boundary from the authoritative element order, retaining prepared data.
    void beginRelink(){for(auto& head:heads_)head=End;changed();}
    bool prependExisting(unsigned layer,unsigned e){
        if(layer>=Layers||!contains(e))return false;
        auto& node=nodes_[e];node.previous=End;node.next=heads_[layer];
        if(node.next!=End)nodes_[node.next].previous=e;
        heads_[layer]=e;return true;
    }
    // A logical owner may retain invisible terrain outside this physical
    // list. Its inactive cutoff can end the layer before the next Draw node.
    // Reaching that cutoff is success; a failing Draw remains failure.
    template<class Before,class Visit> bool visitOrdered(unsigned layer,Before before,Visit visitor)const{
        if(layer>=Layers)return false;
        for(unsigned e=heads_[layer];e!=End;e=nodes_[e].next){
            const auto& node=nodes_[e];
            if(node.action==endLayer)break;
            if(node.action==draw){
                if(!before(e))break;
                if(!visitor(node.token))return false;
            }
        }
        return true;
    }
    template<class Visit> bool visit(unsigned layer,Visit visitor)const{
        return visitOrdered(layer,[](unsigned){return true;},visitor);
    }
};
}
#endif
