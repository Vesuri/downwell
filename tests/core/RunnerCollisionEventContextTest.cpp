#include "core/RunnerCollisionEventContext.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
using Context=RunnerCollisionEventContext;
static bool lookup(InstanceHandle,InstanceHandle b,int& subtype,bool& found,PackedObjectEvent& event){
    subtype=100+int(b.slot);found=true;event={82,4,35,2081};return true;
}
static bool recurse(Context& context,unsigned limit){
    return context.dispatch({0,1},{1,1},{true,false},lookup,[&](const Context::Frame&){
        return context.depth()==limit||recurse(context,limit);
    });
}
int main(){
    for(bool present:{false,true})for(bool pending:{false,true}){
        Context context;unsigned looked=0,executed=0;
        assert(context.dispatch({0,1},{1,1},{present,pending},
            [&](auto a,auto b,int& subtype,bool& found,PackedObjectEvent& event){
                ++looked;return lookup(a,b,subtype,found,event);
            },[&](const Context::Frame& f){
                ++executed;assert(f.self.slot==0&&f.other.slot==1&&f.subtype==101&&
                    f.owner==82&&f.code==2081&&f.type==4&&context.depth()==1);return true;
            }));
        assert(looked==unsigned(present&&!pending)&&executed==looked&&!context.current());
    }
    for(bool innerSucceeds:{false,true}){
        Context context;
        assert(context.dispatch({0,1},{1,1},{true,false},lookup,[&](const Context::Frame& outer){
            const auto saved=outer;
            const bool ok=context.dispatch({1,1},{0,1},{true,false},lookup,[&](const Context::Frame& inner){
                assert(context.depth()==2&&inner.self.slot==1&&inner.subtype==100);return innerSucceeds;
            });
            assert(ok==innerSucceeds&&context.depth()==1&&context.current()->self.slot==saved.self.slot&&
                context.current()->subtype==saved.subtype);
            return true; // Ignoring a nested failure must not recover the context.
        })==innerSucceeds);
        assert(!context.current()&&context.depth()==0&&context.succeeded()==innerSucceeds);
    }
    Context limit,overflow;
    assert(recurse(limit,32)&&!limit.current());
    assert(!recurse(overflow,33)&&!overflow.succeeded()&&!overflow.current());
    Context miss;unsigned calls=0;
    assert(miss.dispatch({0,1},{1,1},{true,false},
        [](auto,auto,int&,bool& found,PackedObjectEvent&){found=false;return true;},
        [&](const auto&){++calls;return true;})&&calls==0);
    // A room request by the first event suppresses only the later event call;
    // the pair wrapper still runs its post-event solid response separately.
    Context changed;Context::RoomState room{true,false};
    assert(changed.dispatch({0,1},{1,1},room,lookup,[&](const auto&){++calls;room.pendingChange=true;return true;}));
    assert(changed.dispatch({1,1},{0,1},room,lookup,[&](const auto&){++calls;return true;})&&calls==1);
    std::puts("Collision event context: room guards, inherited binding context, nested restoration and recursion limit passed");
}
