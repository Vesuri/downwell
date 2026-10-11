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
    static_assert(Layers<256,"Draw projection layer must fit a byte");
    struct Node {
        std::uint16_t previous=End,next=End,token=0,drawNext=End;
        Action action=absent;
        std::uint8_t layer=0;
    };
    Node nodes_[Elements]{};
    std::uint16_t heads_[Layers]{};
    std::uint16_t drawHeads_[Layers]{};
    std::uint32_t revision_=0;
    void changed(){if(revision_!=0xffffffffu)++revision_;}
    static bool relevant(Action action){return action==draw||action==endLayer;}
    // Full links retain the source insertion order. The second chain contains
    // only executable Draw slots and inactive barriers, not room placeholders
    // or active scenery. Find its predecessor only at a membership mutation.
    unsigned drawPredecessor(unsigned e)const{
        unsigned previous=nodes_[e].previous;
        while(previous!=End&&!relevant(nodes_[previous].action))previous=nodes_[previous].previous;
        return previous;
    }
    void attachDraw(unsigned e){
        const unsigned previous=drawPredecessor(e);
        auto& link=previous==End?drawHeads_[nodes_[e].layer]:nodes_[previous].drawNext;
        nodes_[e].drawNext=link;link=e;
    }
    void detachDraw(unsigned e){
        const unsigned previous=drawPredecessor(e);
        auto& link=previous==End?drawHeads_[nodes_[e].layer]:nodes_[previous].drawNext;
        link=nodes_[e].drawNext;nodes_[e].drawNext=End;
    }
    unsigned nextDraw(unsigned e)const{
        if(nodes_[e].action!=skip)return nodes_[e].drawNext;
        // A Draw callback can turn its own slot into scenery and activate a
        // previously skipped successor. Follow source links in that case.
        unsigned next=nodes_[e].next;
        while(next!=End&&!relevant(nodes_[next].action))next=nodes_[next].next;
        return next;
    }
public:
    RunnerPreparedDrawList(){clear();}
    void clear(){for(auto& node:nodes_)node=Node{};for(auto& head:heads_)head=End;
        for(auto& head:drawHeads_)head=End;changed();}
    std::uint32_t revision()const{return revision_;}
    void invalidateOrder(){changed();}
    bool contains(unsigned e)const{return e<Elements&&nodes_[e].action!=absent;}
    bool attach(unsigned layer,unsigned e,unsigned previous){
        if(layer>=Layers||e>=Elements||contains(e)||
           (previous!=End&&!contains(previous)))return false;
        auto& node=nodes_[e];node.action=skip;node.previous=previous;node.layer=layer;
        node.next=previous==End?heads_[layer]:nodes_[previous].next;
        if(node.next!=End)nodes_[node.next].previous=e;
        if(previous==End)heads_[layer]=e;else nodes_[previous].next=e;
        changed();return true;
    }
    bool detach(unsigned layer,unsigned e){
        if(layer>=Layers||!contains(e))return false;
        auto& node=nodes_[e];
        if(relevant(node.action))detachDraw(e);
        if(node.previous==End)heads_[layer]=node.next;else nodes_[node.previous].next=node.next;
        if(node.next!=End)nodes_[node.next].previous=node.previous;
        node=Node{};changed();return true;
    }
    bool publish(unsigned e,unsigned token,Action action){
        if(!contains(e)||token>65535||action==absent)return false;
        auto& node=nodes_[e];
        if(node.token!=token||node.action!=action){
            if(relevant(node.action)&&!relevant(action))detachDraw(e);
            else if(!relevant(node.action)&&relevant(action))attachDraw(e);
            node.token=token;node.action=action;changed();
        }
        return true;
    }
    bool reposition(unsigned layer,unsigned e,unsigned previous){
        if(!contains(e)||previous==e)return false;
        const auto token=nodes_[e].token;const auto action=nodes_[e].action;
        return detach(layer,e)&&attach(layer,e,previous)&&publish(e,token,action);
    }
    // Depth merge/reconciliation may reorder complete layers. Relink at that
    // boundary from the authoritative element order, retaining prepared data.
    void beginRelink(){for(auto& head:heads_)head=End;for(auto& head:drawHeads_)head=End;changed();}
    bool prependExisting(unsigned layer,unsigned e){
        if(layer>=Layers||!contains(e))return false;
        auto& node=nodes_[e];node.previous=End;node.next=heads_[layer];node.layer=layer;
        if(node.next!=End)nodes_[node.next].previous=e;
        heads_[layer]=e;
        node.drawNext=End;
        if(relevant(node.action)){node.drawNext=drawHeads_[layer];drawHeads_[layer]=e;}
        return true;
    }
    // A logical owner may retain invisible terrain outside this physical
    // list. Its inactive cutoff can end the layer before the next Draw node.
    // Reaching that cutoff is success; a failing Draw remains failure.
    template<class Before,class Visit> bool visitOrdered(unsigned layer,Before before,Visit visitor)const{
        if(layer>=Layers)return false;
        for(unsigned e=drawHeads_[layer];e!=End;e=nextDraw(e)){
            const auto& node=nodes_[e];
            if(node.action==endLayer)break;
            if(!before(e))break;
            if(!visitor(node.token))return false;
        }
        return true;
    }
    // Portable oracle retaining the original full-chain traversal.
    template<class Before,class Visit> bool visitOrderedReference(unsigned layer,Before before,Visit visitor)const{
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
