#include "core/RunnerLayerDraw.h"
#include "core/RunnerLayers.h"
#include "core/RunnerRoomMembership.h"
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
using namespace downwell;
void require(bool v,const char* m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
using Action=RunnerLayerDraw::Action;
void dispatch(){
    // All combinations of the five gated flag bits, sprite variant bit and
    // null instance/object, event and sprite lookup outcomes.
    const unsigned bits[]={1,2,0x10,0x10000,0x100000,0x4000};
    for(unsigned combination=0;combination<64;++combination)for(unsigned state=0;state<16;++state){
        unsigned flags=0;for(unsigned i=0;i<6;++i)if(combination&(1u<<i))flags|=bits[i];
        RunnerLayerDraw::Element e{2,flags,bool(state&1),bool(state&2),bool(state&4),bool(state&8)};
        Action expected=Action::skip;
        if(e.instance){
            if(flags&2)expected=Action::endLayer;
            else if(!(flags&1)&&!(flags&0x10000)&&!(flags&0x100000)&&(flags&0x10)&&e.object){
                if(e.drawEvent)expected=Action::drawEvent;
                else if(e.sprite)expected=flags&0x4000?Action::alternateSprite:Action::sprite;
            }
        }
        require(RunnerLayerDraw::action(e)==expected,"dispatch matrix");
    }
    for(unsigned type=0;type<10;++type)if(type!=2)
        require(RunnerLayerDraw::action({type,2,true,true,true,true})==
            (type>=1&&type<=8?Action::other:Action::skip),"only instance element can end layer");
}
void traversal(){
    RunnerLayers<3,6> layers;unsigned low=3,high=3,equal=3;
    require(layers.insertAfterEquals(10,-60000,true,low)&&layers.insertAfterEquals(20,0,true,high)&&
        layers.insertAfterEquals(30,0,true,equal),"layers");
    for(unsigned i=0;i<6;++i)require(layers.attach(i<3?high:i<5?low:equal,i,true,true,[](unsigned){return false;}),"elements");
    RunnerLayerDraw::Element states[6]={{2,0x10,true,true,true,false},{2,2,true,false,false,false},
        {1,0,false,false,false,false},{2,0x10,true,true,false,true},{2,0x10,false,true,true,true},
        {2,0x4010,true,true,false,true}};
    std::vector<int> trace;
    auto begin=[&](unsigned,const auto& l){trace.push_back(l.id);return true;};
    auto end=[&](unsigned,const auto& l){trace.push_back(-l.id);return true;};
    auto lookup=[&](unsigned e,auto& out){out=states[e];return true;};
    auto draw=[&](unsigned,unsigned e,Action){trace.push_back(100+int(e));return true;};
    require(RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},begin,lookup,draw,end),"draw");
    require(trace==std::vector<int>({30,105,-30,20,100,-20,10,103,-10}),"reverse layers, forward elements, inactive ends only current layer");
    trace.clear();require(RunnerLayerDraw::visit(layers,[](unsigned,const auto& l){return l.id!=20;},begin,lookup,draw,end),"hidden layer");
    require(trace==std::vector<int>({30,105,-30,10,103,-10}),"hidden layer no callbacks");
    trace.clear();require(!RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},begin,
        [](unsigned,auto&){return false;},draw,end)&&trace==std::vector<int>({30}),"lookup failure no layer-end callback");
    trace.clear();require(!RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},begin,lookup,
        [](unsigned,unsigned,Action){return false;},end)&&trace==std::vector<int>({30}),"draw failure terminal");
    trace.clear();require(!RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},
        [](unsigned,const auto&){return false;},lookup,draw,end)&&trace.empty(),"begin failure terminal");
    trace.clear();require(!RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},begin,lookup,draw,
        [](unsigned,const auto&){return false;})&&trace==std::vector<int>({30,105}),"end failure terminal");
}
void roomActivation(){
    RunnerRoomMembership<3> room;RunnerLayers<1,3> layers;unsigned layer=1;
    require(layers.insertAfterEquals(10,0,true,layer),"room layer");
    bool active[3]={true,true,true};std::vector<unsigned> calls;
    for(unsigned i=0;i<3;++i)require(room.bind(i,{})&&layers.attach(layer,i,true,false,[](unsigned){return false;}),"room actor birth");
    auto reconcile=[&](){return room.reconcileChecked([&](unsigned i){return !active[i];},[&](unsigned i,bool registered){
        calls.push_back(i);return layers.reconcileInstanceActivation(i,!registered);
    });};
    auto draw=[&](){
        std::vector<unsigned> result;
        require(RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},[](unsigned,const auto&){return true;},
            [&](unsigned i,auto& e){e={2,active[i]?0x10u:0x12u,true,true,true,false};return true;},
            [&](unsigned,unsigned i,Action){result.push_back(i);return true;},[](unsigned,const auto&){return true;}),"room Draw");
        return result;
    };
    require(draw()==std::vector<unsigned>({2,1,0}),"initial layer order");
    // Flag writes can occur in any order; layer updates follow room scans.
    active[1]=false;active[0]=false;
    require(reconcile()&&calls==std::vector<unsigned>({0,1}),"deactivation follows active-room order");
    require(draw()==std::vector<unsigned>({2}),"inactive suffix does not suppress remaining active actor");
    calls.clear();active[1]=true;active[0]=true;
    require(reconcile()&&calls==std::vector<unsigned>({0,1}),"promotion follows inactive-room order");
    require(draw()==std::vector<unsigned>({1,0,2}),"promotions enter layer head");
    calls.clear();require(reconcile()&&calls.empty()&&draw()==std::vector<unsigned>({1,0,2}),"unchanged reconciliation does not reorder");
}
void drawBirths(){
    // Real Draw dispatch with synchronous insertion, not a precomputed list.
    // Head births are deferred; tail births are visible this frame only if
    // an unvisited predecessor will subsequently read their link.
    for(unsigned birthAt=0;birthAt<3;++birthAt)for(bool append:{false,true}){
        RunnerLayers<1,4> layers;unsigned layer=1;
        require(layers.insertAfterEquals(10,0,false,layer),"birth test layer");
        for(unsigned e=0;e<3;++e)
            require(layers.attach(layer,e,true,true,[](unsigned){return false;}),"birth test initial elements");
        std::vector<unsigned> trace;
        bool spawned=false;
        auto run=[&](){return RunnerLayerDraw::visit(layers,
            [](unsigned,const auto&){return true;},[](unsigned,const auto&){return true;},
            [](unsigned,auto& out){out={2,0x10,true,true,true,false};return true;},
            [&](unsigned,unsigned e,Action){
                trace.push_back(e);
                if(e==birthAt&&!spawned){
                    spawned=true;
                    return layers.attach(layer,3,true,append,[](unsigned){return false;});
                }
                return true;
            },[](unsigned,const auto&){return true;});};
        require(run(),"Draw birth insertion");
        auto expected=std::vector<unsigned>({0,1,2});
        if(append&&birthAt<2)expected.push_back(3);
        require(trace==expected,"saved successor controls same-frame birth eligibility");
        trace.clear();require(run(),"Draw after insertion");
        require(trace==(append?std::vector<unsigned>({0,1,2,3}):std::vector<unsigned>({3,0,1,2})),
            "next Draw includes birth exactly once in link order");
    }
}
}
int main(){dispatch();traversal();roomActivation();drawBirths();std::cout<<"Selected layer Draw order, synchronous births and 1024 instance-dispatch cases verified\n";}
