#include "core/RunnerLayerInstances.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
using namespace downwell;
void require(bool v,const char* m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
using Registry=RunnerLayerInstances<2,5>;using Layers=Registry::Layers;
std::vector<unsigned> ids(const Layers& layers,const Registry& registry,unsigned layer){
    std::vector<unsigned> result;
    require(layers.visitElements(layer,[&](unsigned e){
        const auto* r=registry.get(e);require(r!=nullptr,"registered instance element");result.push_back(r->sourceId);return result.size()<=5;
    }),"bounded elements");return result;
}
void permutations(){
    unsigned order[]={0,1,2,3};unsigned count=0;
    do {
        Registry registry;Layers layers;unsigned layer=2;
        require(layers.insertAfterEquals(17,-50000,false,layer),"fixed layer");
        for(unsigned i=0;i<4;++i)require(registry.reserve(layers,layer,100+i),"serialized placeholder");
        for(unsigned i:order){
            RunnerDepthService::Instance instance;
            // Deliberately different source IDs, element slots and store slots.
            const InstanceHandle h{20+i,3};
            require(registry.attach(layers,layer,100+i,h,instance,false),"fill existing element");
            require(instance.attached&&instance.layerId==17,"published layer identity");
            require(instance.depth.bits==0xc7435000u,"published layer depth");
            require(registry.find(h)==registry.find(100+i),"lifetime lookup");
        }
        require(ids(layers,registry,layer)==std::vector<unsigned>({103,102,101,100}),"creation permutation cannot change serialized element order");
        ++count;
    }while(std::next_permutation(order,order+4));
    require(count==24,"all creation orders");
}
void lifecycle(){
    Registry registry;Layers layers;unsigned layer=2;
    require(layers.insertAfterEquals(17,0,false,layer),"lifecycle layer");
    for(unsigned i=0;i<3;++i)require(registry.reserve(layers,layer,100+i),"lifecycle reserve");
    RunnerDepthService::Instance inactive;inactive.append=false;
    require(registry.attach(layers,layer,101,{50,1},inactive,true),"inactive placeholder moves to tail");
    require(ids(layers,registry,layer)==std::vector<unsigned>({102,100,101}),"inactive reuse order");
    RunnerDepthService::Instance live;
    require(registry.attach(layers,layer,100,{51,1},live,false),"active placeholder stays in place");
    require(ids(layers,registry,layer)==std::vector<unsigned>({102,100,101}),"active reuse order");
    RunnerDepthService::Instance duplicate;
    require(!registry.attach(layers,layer,100,{52,1},duplicate,false)&&!duplicate.attached,"duplicate source rejected");
    require(!registry.attach(layers,layer,200,{51,1},duplicate,false),"duplicate lifetime rejected");
    require(!registry.retire(layers,101,{50,2})&&registry.retire(layers,101,{50,1}),"generation checked retirement");
    RunnerDepthService::Instance replacement;
    require(registry.attach(layers,layer,101,{50,2},replacement,false),"new lifetime creates head element");
    require(registry.find(InstanceHandle{50,1})==Registry::End,"old lifetime cannot resolve replacement");
    require(ids(layers,registry,layer)==std::vector<unsigned>({101,102,100}),"new element does not inherit retired position");
    RunnerDepthService::Instance dormant;dormant.append=true;
    require(registry.attach(layers,layer,300,{99,1},dormant,false),"new dormant registration appends independently of inactive flag");
    require(ids(layers,registry,layer)==std::vector<unsigned>({101,102,100,300}),"append uses registration flag");
}
void bounds(){
    RunnerLayerInstances<1,1> registry;RunnerLayers<1,1> layers;unsigned layer=1;
    require(layers.insertAfterEquals(1,0,false,layer)&&registry.reserve(layers,layer,10),"capacity seed");
    require(!registry.reserve(layers,layer,10)&&!registry.reserve(layers,layer,11),"duplicate/capacity reserve");
    RunnerDepthService::Instance state;
    require(!registry.attach(layers,layer,11,{1,1},state,false)&&!state.attached,"capacity leaves instance unchanged");
    require(!registry.attach(layers,layer,10,{},state,false),"invalid handle rejected");
}
}
int main(){permutations();lifecycle();bounds();std::cout<<"Serialized layer placeholders and lifetime-checked attachment verified\n";}
