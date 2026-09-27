#include "core/RunnerCollisionPairs.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <array>
#include <algorithm>
using namespace downwell;
int main(){
    unsigned cases=0;
    for(unsigned events=0;events<512;++events){
        std::array<unsigned,3> order{{0,1,2}};
        do{
            std::array<unsigned,3> candidates{{0,1,2}};
            do{
                RunnerCollisionPairs<3,9> actual;std::vector<unsigned> expected;bool done[3]{};
                assert(actual.beginPass());
                for(auto self:order){
                    for(auto other:candidates){
                        if(self!=other&&!done[other]&&((events&(1u<<(self*3+other)))||(events&(1u<<(other*3+self)))))
                            expected.push_back(self*3+other);
                        assert(actual.candidate({self,1},{other,1},[&](auto s,auto o,bool& found){
                            found=(events&(1u<<(s.slot*3+o.slot)))!=0;return true;
                        }));
                    }
                    done[self]=true;assert(actual.finishInstance({self,1}));
                }
                assert(actual.seal()&&actual.size()==expected.size());
                for(unsigned i=0;i<actual.size();++i)assert(actual.at(i)->self.slot*3+actual.at(i)->other.slot==expected[i]);
                ++cases;
            }while(std::next_permutation(candidates.begin(),candidates.end()));
        }while(std::next_permutation(order.begin(),order.end()));
    }
    const InstanceHandle a{0,1},b{1,1},c{2,1};
    RunnerCollisionPairs<3,8> pairs;std::vector<unsigned> lookups;
    auto event=[&](auto self,auto other,bool& found){lookups.push_back(self.slot*10+other.slot);found=self.slot==1;return true;};
    assert(pairs.beginPass());
    assert(pairs.candidate(a,a,event)&&lookups.empty());
    assert(pairs.candidate(a,c,event)&&pairs.size()==0);
    assert(pairs.candidate(a,b,event)&&pairs.size()==1);
    assert((lookups==std::vector<unsigned>{2,20,1,10}));
    assert(pairs.finishInstance(a));lookups.clear();
    assert(pairs.candidate(b,a,event)&&lookups.empty()); // Reverse already collected.
    assert(pairs.candidate(b,c,event)&&pairs.size()==2&&lookups==std::vector<unsigned>{12});
    assert(pairs.finishInstance(b)&&pairs.candidate(c,b,event)&&pairs.finishInstance(c)&&pairs.seal());
    assert(pairs.at(0)->self.slot==0&&pairs.at(0)->other.slot==1&&pairs.at(1)->self.slot==1&&pairs.at(1)->other.slot==2&&!pairs.at(2));
    assert(pairs.beginPass());lookups.clear();
    assert(pairs.candidate(c,b,event)&&pairs.finishInstance(c)&&pairs.candidate(b,a,event)&&pairs.seal());
    assert(pairs.at(0)->self.slot==2&&pairs.at(0)->other.slot==1&&pairs.at(1)->other.slot==0);
    std::vector<unsigned> dispatched;
    assert(pairs.dispatch([&](auto self,auto other){dispatched.push_back(self.slot*10+other.slot);return true;}));
    assert((dispatched==std::vector<unsigned>{21,10}));
    assert(!pairs.dispatch([](auto,auto){assert(false);return true;}));
    for(unsigned failure=0;failure<3;++failure){
        RunnerCollisionPairs<3,4> pass;assert(pass.beginPass());
        auto yes=[](auto,auto,bool& found){found=true;return true;};
        assert(pass.candidate(a,b,yes)&&pass.candidate(a,c,yes)&&pass.seal());
        unsigned calls=0;
        assert(!pass.dispatch([&](auto,auto){
            ++calls;
            if(failure==2){assert(!pass.beginPass());return true;}
            return calls!=failure+1;
        }));
        assert(calls==(failure==2?1u:failure+1)&&!pass.succeeded());
    }
    RunnerCollisionPairs<3,4> empty;assert(empty.beginPass()&&empty.seal());
    assert(empty.dispatch([](auto,auto){assert(false);return true;}));
    // Capacity failure preserves the collected prefix, and cannot be reset.
    RunnerCollisionPairs<3,1> full;assert(full.beginPass()&&full.candidate(a,b,event));
    assert(!full.candidate(b,c,event)&&!full.succeeded()&&full.size()==1&&!full.beginPass());
    for(unsigned failure=0;failure<3;++failure){
        RunnerCollisionPairs<3,4> bad;assert(bad.beginPass());unsigned calls=0;
        assert(!bad.candidate(a,failure==2?InstanceHandle{3,1}:b,[&](auto,auto,bool& found){
            ++calls;found=false;return calls!=(failure==0?1u:2u);
        }));
        assert(calls==(failure==2?0u:failure+1)&&!bad.succeeded()&&bad.size()==0&&!bad.seal());
    }
    RunnerCollisionPairs<3,4> generations;assert(generations.beginPass()&&generations.finishInstance(a));
    assert(generations.candidate(b,{0,2},event)&&generations.size()==1&&generations.at(0)->other.generation==2);
    assert(generations.seal()&&!generations.candidate(a,b,event));
    std::printf("Collision candidates: %u event/traversal cases, ordered lookup, processed suppression, capacity and terminal failure passed\n",cases);
}
