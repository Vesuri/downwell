#ifndef DOWNWELL_CORE_RUNNER_LAYER_DRAW_H
#define DOWNWELL_CORE_RUNNER_LAYER_DRAW_H
#include <cstdint>
namespace downwell {
// Ordinary layer Draw at 0x490370, selected when room layer count is positive.
// Presentation dispatch only: adapters supply source flags, object/event/sprite
// lookup, layer visibility, and backend/layer-script rendering operations.
struct RunnerLayerDraw {
    struct Element {
        unsigned type=0;
        std::uint32_t flags=0;
        bool instance=false,object=false,drawEvent=false,sprite=false;
    };
    enum class Action {skip,endLayer,other,drawEvent,sprite,alternateSprite};
    static Action action(const Element& e){
        if(e.type<1||e.type>8)return Action::skip;
        if(e.type!=2)return Action::other;
        if(!e.instance)return Action::skip;
        if(e.flags&2u)return Action::endLayer;
        if((e.flags&0x110013u)!=0x10u||!e.object)return Action::skip;
        if(e.drawEvent)return Action::drawEvent;
        if(!e.sprite)return Action::skip;
        return e.flags&0x4000u?Action::alternateSprite:Action::sprite;
    }
    // Callback failure stops the whole traversal. endLayer still runs the
    // layer's end callback and proceeds to the next layer. Hidden layers run
    // neither callback. Callbacks must not mutate layer/element links here;
    // original Draw-time structural mutation requires an adapter audit.
    template<class Layers,class Visible,class Begin,class Lookup,class Draw,class End>
    static bool visit(const Layers& layers,Visible visible,Begin begin,Lookup lookup,Draw draw,End end){
        if(!layers.succeeded())return false;
        return layers.visitLayersReverse([&](unsigned slot,const auto& layer){
            if(!visible(slot,layer))return true;
            if(!begin(slot,layer))return false;
            bool stopped=false;
            const bool ok=layers.visitElements(slot,[&](unsigned element){
                Element state;if(!lookup(element,state))return false;
                const auto result=action(state);
                if(result==Action::endLayer){stopped=true;return false;}
                return result==Action::skip||draw(slot,element,result);
            });
            return (ok||stopped)&&end(slot,layer);
        });
    }
};
}
#endif
