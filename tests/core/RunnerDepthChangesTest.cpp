#include "core/RunnerDepthChanges.h"
#include "core/RunnerLayers.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
using namespace downwell;
void require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
RunnerImageValue real(float value){RunnerImageValue r;std::memcpy(&r.bits,&value,4);return r;}
void queue(){
    RunnerDepthChanges<3> pending;RunnerImageValue a{},b{},c{};
    require(pending.request(7,a,real(-0.0f).promoted())&&a.bits==0&&!pending.size(),"signed zero unchanged");
    require(pending.request(7,a,real(10).promoted())&&a.bits==real(10).bits,"immediate publication");
    require(pending.request(9,b,real(20).promoted()),"second request");
    require(pending.request(7,a,real(30).promoted())&&pending.size()==2,"unique first position");
    require(pending.request(11,c,real(40).promoted())&&pending.retire(9),"stable retirement");
    require(pending.request(9,b,real(50).promoted()),"reuse queues new lifetime last");
    std::vector<unsigned> order;
    require(pending.service([&](unsigned i){order.push_back(i);return true;}),"service");
    require(order==std::vector<unsigned>({7,11,9})&&!pending.size(),"first request order with compaction");
    require(pending.service([](unsigned){return false;}),"empty service");
    RunnerDepthChanges<1> full;
    require(full.request(1,a,real(1).promoted()),"full seed");
    require(!full.request(2,b,real(2).promoted())&&b.bits==real(2).bits&&!full.succeeded(),"capacity failure after publish");
    require(!full.service([](unsigned){return true;}),"terminal capacity failure");
    RunnerDepthChanges<3> failing;
    require(failing.request(1,a,real(10).promoted())&&failing.request(2,b,real(10).promoted())&&
        failing.request(3,c,real(10).promoted()),"failure seed");
    unsigned calls=0;require(!failing.service([&](unsigned){return ++calls!=2;})&&calls==2,"stop at failed migration");
    require(!failing.service([&](unsigned){++calls;return true;})&&calls==2,"never retry migrated prefix");
}
void numeric(){
    const unsigned cases[]={0,0x80000000u,0x3f400000u,0xbf400000u,0x4effffffu,0xceffffffu,
        0xcf000000u,0x4f000000u,0xcf000001u,1,0x7fc00000u,0x7f800000u};
    for(unsigned bits:cases){
        const RunnerImageValue r{bits};float f;std::memcpy(&f,&bits,4);int result=99;
        const bool valid=RunnerSpatialSplit::valid(r)&&double(f)>=-2147483648.0&&double(f)<2147483648.0;
        require(RunnerDepthService::integerDepth(r,result)==valid,"conversion domain");
        require(result==(valid?int(f):99),"truncation/output preservation");
    }
    unsigned rng=17;
    for(unsigned i=0;i<100000;++i){
        rng=rng*1664525u+1013904223u;
        const float f=float(int(rng%2000001)-1000000)/32.0f;
        int out=0;require(RunnerDepthService::integerDepth(real(f),out)&&out==int(f),"signed fractional truncation");
    }
}
using Layers=RunnerLayers<4,8>;
void migrationMatrix(){
    const int depths[]={-1000000000,-30,0,30,1000000000};
    const float deltas[]={-20.001f,-20,-19.999f,-0.75f,0,0.75f,19.999f,20,20.001f};
    for(int initial:depths)for(float delta:deltas)for(unsigned dynamic=0;dynamic<2;++dynamic)
    for(unsigned count=1;count<=2;++count)for(unsigned identity=0;identity<2;++identity){
        Layers layers;unsigned old=99;require(layers.insertAfterEquals(10,initial,dynamic,old),"matrix layer");
        for(unsigned e=0;e<count;++e)require(layers.attach(old,e,true,true,[](unsigned){return false;}),"matrix elements");
        RunnerDepthService::Instance instance{real(float(initial)+delta),10,true,false};
        const auto requested=instance.depth;
        float value;std::memcpy(&value,&requested.bits,4);
        const int target=int(value);const float difference=value-float(initial);
        unsigned creations=0;
        require(RunnerDepthService::apply(layers,instance,0,[&](unsigned e){return identity&&e==0;},
            [&](int depth,unsigned& slot){++creations;return layers.insertAfterEquals(20,depth,true,slot);},
            [](unsigned,int){return true;},[](unsigned){return false;}),"matrix service");
        const bool same=target==initial;
        const bool small=std::fabs(difference)<20&&dynamic&&count==1;
        const bool migration=!same&&!small;
        require(creations==unsigned(migration),"strict 20 threshold/migration gates");
        require(instance.layerId==(migration?20:10)&&instance.attached,"instance membership");
        require(instance.depth.bits==(migration?real(float(target)).bits:requested.bits),"fraction retained only without reattachment");
        if(!migration)require(layers.get(old)->depth==(!same&&small&&identity?target:initial),"mismatch exits without migration");
        else require((layers.find(10)!=Layers::End)==(!dynamic||count==2),"empty dynamic disposal by identity, not reused slot");
    }
}
void connected(){
    Layers layers;unsigned a=Layers::End,b=Layers::End;require(layers.insertAfterEquals(10,0,true,a)&&layers.insertAfterEquals(11,-60000,true,b),"impact seed");
    require(layers.attach(a,0,true,false,[](unsigned){return false;})&&layers.attach(b,1,true,false,[](unsigned){return false;}),"impact elements");
    RunnerDepthService::Instance impact{real(0),10,true,false};RunnerDepthChanges<2> queue;
    require(queue.request(0,impact.depth,real(-60000.75f).promoted())&&impact.layerId==10,"deferred membership");
    require(queue.service([&](unsigned e){return RunnerDepthService::apply(layers,impact,e,
        [&](unsigned found){return found==e;},[](int,unsigned&){return false;},
        [](unsigned,int){return true;},[](unsigned){return false;});}),"queued box-depth migration reuses layer");
    require(impact.layerId==11&&impact.depth.bits==real(-60000).bits&&!layers.get(a),"impact attached integer depth");
    std::vector<unsigned> elements;layers.visitElements(b,[&](unsigned e){elements.push_back(e);return true;});
    require(elements==std::vector<unsigned>({0,1}),"migrated instance enters destination head");
    RunnerDepthService::Instance unattached{real(0),-1,false,false};
    require(RunnerDepthService::apply(layers,unattached,7,[](unsigned){return false;},[](int,unsigned&){return false;},
        [](unsigned,int){return false;},[](unsigned){return false;}),"unattached service no-op");
}
void mergeAndFailure(){
    Layers layers;unsigned a=Layers::End,b=Layers::End;
    require(layers.insertAfterEquals(10,0,true,a)&&layers.insertAfterEquals(20,10,true,b),"merge layers");
    require(layers.attach(a,0,true,false,[](unsigned){return false;})&&layers.attach(b,1,true,false,[](unsigned){return false;}),"merge elements");
    RunnerDepthService::Instance instance{real(10.75f),10,true,false};
    require(RunnerDepthService::apply(layers,instance,0,[](unsigned e){return e==0;},
        [](int,unsigned&){return false;},[&](unsigned e,int id){require(e==0,"merged instance");instance.layerId=id;return true;},
        [](unsigned){return false;}),"small move merges and publishes");
    require(instance.layerId==20&&instance.depth.bits==real(10.75f).bits&&layers.find(10)==Layers::End,"merge retains fractional requested depth");
    instance.depth=real(-60000);RunnerDepthChanges<1> pending;
    require(pending.request(0,instance.depth,real(-60001).promoted()),"failure request");
    require(!pending.service([&](unsigned e){return RunnerDepthService::apply(layers,instance,e,
        [](unsigned){return true;},[](int,unsigned&){return false;},[](unsigned,int){return true;},
        [](unsigned){return false;});}),"failed layer allocation stops service");
    require(!pending.succeeded()&&!instance.attached&&instance.layerId==-1&&layers.elementOwner(0)==Layers::End,
        "failed migration retains detach, no rollback");
    require(layers.elementCount(b)==1,"destination peer remains");
}
}
int main(){queue();numeric();migrationMatrix();connected();mergeAndFailure();std::cout<<"Deferred depth publication and selected layer migration verified\n";}
