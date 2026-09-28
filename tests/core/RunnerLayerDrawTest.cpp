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
void cleanupProjection(){
    {
        // Binary32 upper-bound insertion: signed zeros tie; fractional and
        // extreme normal values must preserve the source's tail-scan order.
        const std::uint32_t bits[]={0,0x80000000u,0x3f000000u,0xbf000000u,
            0x00800000u,0x80800000u,0x7f7fffffu,0xff7fffffu};
        for(unsigned rotation=0;rotation<8;++rotation)for(unsigned burst:{1u,4u,16u}){
            RunnerRoomMembership<64> ordered;RunnerImageValue keys[64];
            std::vector<unsigned> expected,actual;
            for(unsigned i=0;i<64;++i){
                keys[i].bits=bits[((i/burst)*5+rotation)%8];
                auto pos=expected.begin();
                while(pos!=expected.end()&&!RunnerSpatialSplit::less(keys[i],keys[*pos]))++pos;
                expected.insert(pos,i);
                require(ordered.bind(i,keys[i]),"mixed binary32 room depth");actual.clear();
                require(ordered.visitAnimationChecked([&](unsigned s){actual.push_back(s);return true;})&&actual==expected,
                    "repeated-depth insertion equals stable upper bound");
            }
        }
    }
    RunnerRoomMembership<64> room;
    bool live[64]{},marks[64]{},inactive[64]{};
    unsigned seed=0x827513u;
    auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
    auto compare=[&](){
        std::vector<unsigned> full,sparse;
        require(room.visitCleanupChecked([&](unsigned slot){
            require(room.marked(slot)==marks[slot],"mark publication matches model");
            if(marks[slot])full.push_back(slot);return true;
        })&&room.visitMarkedCleanupChecked([&](unsigned slot){sparse.push_back(slot);return true;}),"cleanup walks");
        require(full==sparse,"sparse cleanup preserves active/inactive room ordering");
    };
    for(unsigned trial=0;trial<8192;++trial){
        const unsigned slot=(random()>>16)%64,operation=(random()>>16)%5;
        if(operation==0){
            RunnerImageValue key;require(key.assign(MotionBinary64::fromInteger(int((random()>>16)%7)-3)),"cleanup depth");
            require(room.bind(slot,key),"cleanup birth/rebind");live[slot]=true;marks[slot]=inactive[slot]=false;
        }else if(operation==1){
            require(room.mark(slot)==live[slot],"missing lifetime cannot be marked");
            if(live[slot]){marks[slot]=true;require(room.mark(slot),"mark is idempotent");}
        }else if(operation==2){
            require(room.remove(slot)==live[slot],"cleanup removal");live[slot]=marks[slot]=inactive[slot]=false;
        }else if(operation==3){
            inactive[slot]=!inactive[slot];
            require(room.reconcileChecked([&](unsigned i){return inactive[i];},[](unsigned,bool){return true;}),"marked activation reconciliation");
        }else{
            compare();
            require(room.visitMarkedCleanupChecked([&](unsigned i){
                require(room.remove(i),"retire current marked entry");live[i]=marks[i]=inactive[i]=false;return true;
            }),"sparse retirement saves successor");
            require(room.visitMarkedCleanupChecked([](unsigned){return false;}),"retired projection empty");
        }
        compare();
    }
    room.clear();
    for(unsigned i=0;i<4;++i)require(room.bind(i,{})&&room.mark(i),"failure fixture");
    unsigned calls=0;require(!room.visitMarkedCleanupChecked([&](unsigned i){
        ++calls;if(calls==2)return false;return room.remove(i);
    })&&calls==2,"cleanup failure stops before later entries");
    std::vector<unsigned> remaining;require(room.visitMarkedCleanupChecked([&](unsigned i){remaining.push_back(i);return true;})&&
        remaining==std::vector<unsigned>({1,2,3}),"failed retirement leaves remaining order intact");
    room.clear();require(room.visitMarkedCleanupChecked([](unsigned){return false;}),"clear removes all marks");
}
void animationProjection(){
    RunnerRoomMembership<6> room;
    RunnerRoomMembership<6>::AnimationProjection projection;
    for(unsigned i=0;i<6;++i)require(room.bind(i,{}),"animation projection births");
    bool dirty=false;std::vector<unsigned> trace;
    auto run=[&](int writeAt=-1){
        trace.clear();return room.visitAnimationProjectedChecked(projection,1,4,dirty,[&](unsigned slot){
            trace.push_back(slot);if(int(slot)==writeAt)dirty=true;return true;
        });
    };
    require(run()&&trace==std::vector<unsigned>({0,4,5}),"skip unchanged snapshot-only range");
    dirty=true;require(run()&&trace==std::vector<unsigned>({0,1,2,3,4,5})&&!dirty,"dirty range full walk");
    require(run(0)&&trace==std::vector<unsigned>({0,1,2,3,4,5})&&dirty,"early callback write restores raw successor");
    require(run()&&trace.size()==6&&!dirty,"mid-walk write retained for next frame");
    require(run(4)&&trace==std::vector<unsigned>({0,4,5})&&dirty,"later write cannot revisit earlier snapshots");
    require(run()&&trace.size()==6,"earlier snapshot refreshed next frame");
    require(room.reconcileChecked([](unsigned slot){return slot==4;},[](unsigned,bool){return true;}),"deactivate projected actor");
    require(run()&&trace==std::vector<unsigned>({0,5}),"projection invalidated by deactivation");
    require(room.reconcileChecked([](unsigned){return false;},[](unsigned,bool){return true;}),"reactivate actor");
    require(run()&&trace==std::vector<unsigned>({0,5,4}),"reactivation preserves room insertion order");
    require(room.remove(0)&&room.bind(0,{}),"reuse projection slot");
    require(run()&&trace==std::vector<unsigned>({5,4,0}),"rebound room links invalidate projection");
    trace.clear();require(!room.visitAnimationProjectedChecked(projection,1,4,dirty,[&](unsigned slot){
        trace.push_back(slot);return false;
    })&&trace==std::vector<unsigned>({5}),"projection callback failure is terminal");
    room.clear();require(run()&&trace.empty(),"empty room resets projection");
    for(unsigned i=0;i<6;++i)require(room.bind(i,{}),"snapshot differential births");
    int current[2][6]{},previous[2][6]{};
    dirty=false;
    for(unsigned frame=0;frame<8;++frame){
        auto visit=[&](unsigned side,unsigned slot){
            previous[side][slot]=current[side][slot];
            if(slot==0||slot==4){
                // Writes both ahead of and behind the current position.
                const unsigned target=1+(frame+slot)%3;
                current[side][target]=int(100*frame+slot+1);
                if(side==1)dirty=true;
            }
            return true;
        };
        require(room.visitAnimationChecked([&](unsigned slot){return visit(0,slot);})&&
            room.visitAnimationProjectedChecked(projection,1,4,dirty,[&](unsigned slot){return visit(1,slot);}),
            "snapshot differential walks");
        for(unsigned i=0;i<6;++i)require(current[0][i]==current[1][i]&&previous[0][i]==previous[1][i],
            "projected snapshot values match source-order full walk");
    }
}
}
int main(){dispatch();traversal();roomActivation();drawBirths();animationProjection();cleanupProjection();std::cout<<"Selected layer Draw order, synchronous births, animation projection, 8192 cleanup mutations and 1024 instance-dispatch cases verified\n";}
