// Count projection rebuilds without a platform clock. The test must prove
// maintained links avoid whole-room rebuilds, not merely produce equal output.
#define DOWNWELL_PROBES
#include "core/RunnerLayerDraw.h"
#include "core/RunnerLayers.h"
#include "core/RunnerRoomMembership.h"
#include "core/RunnerRunRoomMembership.h"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <vector>
#include <type_traits>
volatile unsigned long gDownwellProfileCounters[profileCounterCount]{};
unsigned downwellProfileEnter(unsigned){return 0;}
void downwellProfilePhase(unsigned){}
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
        RunnerRoomMembership<64> room;
        RunnerImageValue keys[64];std::vector<unsigned> expected,actual;
        unsigned seed=0x98315u;
        for(unsigned trial=0;trial<8192;++trial){
            seed=seed*1664525u+1013904223u;const unsigned slot=(seed>>16)%64;
            auto old=std::find(expected.begin(),expected.end(),slot);
            if(old!=expected.end())expected.erase(old);
            if(trial%13==0){room.remove(slot);}
            else{
                // Repeated interior depths, signed-zero ties, and arbitrary
                // rebinding invalidate stale insertion positions naturally.
                const unsigned bits[]={0,0x80000000u,0x3f800000u,0xbf800000u,0x40000000u};
                keys[slot].bits=bits[(trial/7)%5];
                auto pos=expected.begin();
                while(pos!=expected.end()&&!RunnerSpatialSplit::less(keys[slot],keys[*pos]))++pos;
                expected.insert(pos,slot);require(room.bind(slot,keys[slot]),"room insertion rebind");
            }
            actual.clear();require(room.visitAnimationChecked([&](unsigned s){actual.push_back(s);return true;})&&
                actual==expected,"room insertion equals independent stable upper bound after reuse");
            if(trial%127==0){room.clear();expected.clear();}
        }
    }
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
template<bool Runs=false> void animationProjection(bool retained){
    using Room=typename std::conditional<Runs,RunnerRunRoomMembership<6,1,4>,RunnerRoomMembership<6>>::type;
    Room room;
    typename Room::AnimationProjection projection;
    if(retained)require(room.prepareRetainedAnimation(1,4),"prepare animation before births");
    for(unsigned i=0;i<6;++i)require(room.bind(i,{}),"animation projection births");
    bool dirty=false;std::vector<unsigned> trace;
    auto projected=[&](auto visit){
        return retained?room.visitAnimationRetainedChecked(1,4,dirty,visit):
            room.visitAnimationProjectedChecked(projection,1,4,dirty,visit);
    };
    auto run=[&](int writeAt=-1){
        trace.clear();return projected([&](unsigned slot){
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
    trace.clear();require(!projected([&](unsigned slot){
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
            projected([&](unsigned slot){return visit(1,slot);}),
            "snapshot differential walks");
        for(unsigned i=0;i<6;++i)require(current[0][i]==current[1][i]&&previous[0][i]==previous[1][i],
            "projected snapshot values match source-order full walk");
    }
}
template<bool Runs=false> void retainedAnimationMutations(){
    RunnerRoomMembership<64> reference;
    typename std::conditional<Runs,RunnerRunRoomMembership<64,16,48>,RunnerRoomMembership<64>>::type retained;
    unsigned first=16,last=48,random=0x1735486;
    require(retained.prepareRetainedAnimation(first,last),"prepare retained room order");
    require(!retained.prepareRetainedAnimation(4,3)&&!retained.prepareRetainedAnimation(0,65),
        "invalid preparation leaves retained projection unchanged");
    for(unsigned step=0;step<8192;++step){
        random=random*1664525u+1013904223u;
        const unsigned slot=(random>>8)&63u,operation=(random>>18)%5;
        RunnerImageValue depth;require(depth.assignInteger(int((random>>24)%9)-4),"random room depth");
        auto mutate=[&](auto& room){
            if(operation<2)return room.bind(slot,depth);
            if(operation==2)return room.remove(slot);
            if(operation==3){
                unsigned calls=0;
                return room.reconcileChecked([&](unsigned i){return ((i+step)%7)<3;},
                    [&](unsigned,bool){return step%13||++calls!=2;});
            }
            if(step%31==0)room.clear();
            return true;
        };
        require(mutate(reference)==mutate(retained),"retained mutation preserves failure result");
        if(step%257==0){
            first=(step/257)%3==0?0:16;last=(step/257)%3==1?16:64;
            require(retained.prepareRetainedAnimation(first,last),"change prepared omitted range");
        }
        std::vector<unsigned> expected,actual,raw;
        const bool full=step%11==0;bool dirty=full;
        require(reference.visitAnimationChecked([&](unsigned i){
            raw.push_back(i);if(full||i<first||i>=last)expected.push_back(i);return true;
        }),"raw room oracle");
        const auto rebuilds=gDownwellProfileCounters[counterAnimationProjectionBuild];
        require(retained.visitAnimationRetainedChecked(first,last,dirty,[&](unsigned i){actual.push_back(i);return true;})&&
            expected==actual&&!dirty,"retained links preserve raw order after mutation");
        require(gDownwellProfileCounters[counterAnimationProjectionBuild]==rebuilds,
            "prepared mutation path never rebuilds whole-room animation links");
        actual.clear();
        require(retained.visitAnimationChecked([&](unsigned i){actual.push_back(i);return true;})&&raw==actual,
            "projection maintenance never changes room links");
        if(step%503==0){
            auto copy=retained;actual.clear();dirty=full;
            require(copy.visitAnimationRetainedChecked(first,last,dirty,[&](unsigned i){actual.push_back(i);return true;})&&actual==expected,
                "copied owner rebuilds rather than following another room cache");
        }
    }
}
void runRoomMutations(){
    RunnerRoomMembership<64> scalar;
    RunnerRunRoomMembership<64,8,56> runs;
    unsigned random=71391;
    auto compare=[&](){
        for(unsigned s=0;s<64;++s)require(scalar.contains(s)==runs.contains(s)&&
            scalar.active(s)==runs.active(s)&&scalar.marked(s)==runs.marked(s),"run room slot state");
        for(unsigned mode=0;mode<4;++mode){
            std::vector<unsigned> a,b;
            auto walk=[&](auto& room,auto& out){auto visit=[&](unsigned s){out.push_back(s);return true;};
                if(mode==0)return room.visitAnimationChecked(visit);
                if(mode==1)return room.visitDrawChecked(visit);
                if(mode==2)return room.visitCleanupChecked(visit);
                return room.visitMarkedCleanupChecked(visit);
            };
            require(walk(scalar,a)&&walk(runs,b)&&a==b,"run room ordered enumeration");
        }
    };
    for(unsigned round=0;round<64;++round){
        scalar.clear();runs.clear();
        for(unsigned s=0;s<64;++s)require(scalar.bind(s,{})&&runs.bind(s,{}),"contiguous room births");
        require(runs.runCount()==17,"48 terrain entries share one room run, actors remain singletons");
        for(unsigned step=0;step<512;++step){
            random=random*1664525u+1013904223u;
            const unsigned slot=(random>>8)&63,operation=(random>>20)%6;
            std::vector<unsigned> a,b;
            auto mutate=[&](auto& room,auto& log){
                if(operation==0){RunnerImageValue key;key.assignInteger(int((random>>24)%7)-3);return room.bind(slot,key);}
                if(operation==1)return room.remove(slot);
                if(operation==2)return room.mark(slot);
                unsigned calls=0;
                if(operation==3)return room.reconcileChecked([&](unsigned s){log.push_back(s);return ((s+step)%7)<3;},
                    [&](unsigned s,bool active){log.push_back(128+s+128*active);return step%13||++calls!=2;});
                auto visit=[&](unsigned s){log.push_back(s);if(step%17==0&&++calls==2)return false;return room.remove(s);};
                if(operation==4)return room.visitMarkedCleanupChecked(visit);
                return room.visitCleanupChecked(visit);
            };
            require(mutate(scalar,a)==mutate(runs,b)&&a==b,"run room mutation callback order and terminal prefix");
            compare();
        }
    }
    RunnerImageValue invalid{0x7fc00000u};
    require(!runs.bind(0,invalid)&&!runs.bind(64,{})&&!runs.mark(64)&&!runs.remove(64),"run room invalid inputs");
    std::cout<<"Room storage (64 slots): scalar="<<sizeof(scalar)<<" runs="<<sizeof(runs)<<'\n';
}
}
int main(){dispatch();traversal();roomActivation();drawBirths();animationProjection(false);animationProjection(true);animationProjection<true>(false);animationProjection<true>(true);retainedAnimationMutations();retainedAnimationMutations<true>();runRoomMutations();cleanupProjection();std::cout<<"Selected layer Draw order, synchronous births, scalar/run animation projections, mutation/cleanup equivalence and 1024 instance-dispatch cases verified\n";}
