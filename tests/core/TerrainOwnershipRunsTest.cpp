#include "core/TerrainOwnershipRuns.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
using Store=downwell::TerrainOwnershipRuns<64>;
static void require(bool ok){if(!ok)std::abort();}
static bool same(const Store::Identity& a,const Store::Identity& b){
    return a.handle.slot==b.handle.slot&&a.handle.generation==b.handle.generation&&
        a.object==b.object&&a.x==b.x&&a.y==b.y&&a.creationCode==b.creationCode&&a.source==b.source;
}
int main(){
    Store runs;std::vector<Store::Identity> scalar;
    unsigned comparisons=0;
    auto compare=[&](){
        require(runs.size()==scalar.size());unsigned count=0;
        require(runs.visit([&](const auto& r){require(count<scalar.size()&&same(r,scalar[count++]));return true;}));
        require(count==scalar.size());
        require(runs.handle(count).generation==0);
        for(unsigned i=0;i<count;++i){const auto h=runs.handle(i);
            require(h.slot==scalar[i].handle.slot&&h.generation==scalar[i].handle.generation);
        }
        for(unsigned s=0;s<64;++s){
            auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==s;});
            require(runs.occupied(s)==(found!=scalar.end()));
            if(found!=scalar.end()){
                Store::Identity actual;
                require(runs.contains(found->handle)&&runs.read(found->handle,actual)&&same(actual,*found));
                actual.x=12345;
                require(!runs.read({s,found->handle.generation+1},actual)&&actual.x==12345);
            }
        }
        ++comparisons;
    };
    auto append=[&](unsigned slot,unsigned gen,int object,int x,int y,unsigned source=0,int code=-1){
        const bool present=std::any_of(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==slot;});
        const bool expected=slot<64&&gen&&!present&&(object==60||object==93||object==99);
        require(runs.append({slot,gen},object,x,y,source,code)==expected);
        if(expected)scalar.push_back({{slot,gen},object,x,y,code,source});compare();
    };
    auto retire=[&](downwell::InstanceHandle h){
        const auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==h.slot&&r.handle.generation==h.generation;});
        require(runs.retire(h)==(found!=scalar.end()));
        if(found!=scalar.end())scalar.erase(found);compare();
    };
    for(unsigned s=0;s<64;++s)append(s,1+s,60,16*int(s),32);
    require(runs.runCount()==1);
    for(unsigned s=1;s<64;s+=2)retire({s,1+s});
    require(runs.runCount()==1); // Destruction does not split survivor geometry.
    for(unsigned s=1;s<64;s+=2)append(s,100+s,93,-16*int(s),48);
    require(runs.size()==64);
    while(!scalar.empty())retire(scalar.front().handle);
    require(!runs.runCount());
    for(unsigned s=0;s<64;++s)append(s,200+s,60,1024-16*int(s),64);
    require(runs.runCount()==1);
    unsigned stopped=0;require(!runs.visit([&](const auto&){return ++stopped!=3;})&&stopped==3);
    while(!scalar.empty())retire(scalar.back().handle);
    // Authored identities and scripts must never be folded into generated runs.
    for(unsigned s=0;s<64;++s)append(s,300+s,60,16*int(s),32,s+1,676);
    require(runs.runCount()==64);
    unsigned random=71913,next=1000;
    for(unsigned i=0;i<20000;++i){
        random=random*1664525u+1013904223u;const unsigned slot=(random>>9)&63;
        auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==slot;});
        if((random&1)&&found!=scalar.end())retire(found->handle);
        else append(slot,next++,random&2?60:93,int(slot)*16,int((random>>16)&127)*16);
    }
    while(!scalar.empty())retire(scalar.front().handle);
    append(0,0xffffffffu,60,2147483647,0);
    append(1,1,60,(-2147483647-1),0); // No signed coordinate/generation wrap merge.
    append(64,1,60,0,0);append(2,0,60,0,0);append(2,1,35,0,0);
    std::printf("Terrain identity/geometry/order runs: %u scalar comparisons passed\n",comparisons);
}
