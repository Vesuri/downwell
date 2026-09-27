#include "core/RunnerCollisionDispatch.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace downwell;
struct Ops {
    std::string trace;
    unsigned calls=0,failAt=0,overlaps=0;
    std::uint32_t bits[2]{};
    bool initialHit=true,secondHit=false;
    int changeSolid=-1;
    bool deactivateInFirst=false;
    bool call(char c){trace+=c;return ++calls!=failAt;}
    bool overlap(InstanceHandle,InstanceHandle,bool& hit){hit=overlaps++?secondHit:initialHit;return call('O');}
    bool flags(InstanceHandle h,std::uint32_t& f){f=bits[h.slot];return call(h.slot?'b':'a');}
    bool queueCollision(InstanceHandle h){assert(h.slot==1);return call('Q');}
    bool rollback(InstanceHandle h){return call(h.slot?'s':'r');}
    bool dispatch(InstanceHandle h,InstanceHandle other){
        assert(h.slot!=other.slot);
        if(!h.slot){
            if(changeSolid>=0)bits[0]=changeSolid?0x20u:0u;
            if(deactivateInFirst)bits[1]|=2u;
        }
        return call(h.slot?'B':'A');
    }
    bool prepareMotion(InstanceHandle h){return call(h.slot?'n':'m');}
    bool advanceMotion(InstanceHandle h){return call(h.slot?'y':'x');}
};
int main(){
    const InstanceHandle a{0,1},b{1,1};
    for(unsigned initialSolid=0;initialSolid<3;++initialSolid)
    for(int changed=-1;changed<2;++changed)for(bool hit:{false,true}){
        Ops baseline;baseline.bits[0]=initialSolid==1?0x20u:0u;
        baseline.bits[1]=initialSolid==2?0x20u:0u;
        baseline.changeSolid=changed;baseline.secondHit=hit;
        const bool before=initialSolid!=0;
        const bool after=changed<0?before:(changed==1||initialSolid==2);
        std::string expected="OabQ";
        expected+=initialSolid==1?"a":"ab";
        if(before)expected+="rs";
        expected+="AB";
        expected+=(changed==1||(changed<0&&initialSolid==1))?"a":"ab";
        if(after){expected+="mnxyO";if(hit)expected+="rs";}
        Ops successful=baseline;RunnerCollisionDispatch pass;
        assert(pass.pair(a,b,successful)&&successful.trace==expected);
        // Each failed operation cuts off the exact prefix and prevents retry.
        for(unsigned fail=1;fail<=expected.size();++fail){
            Ops failed=baseline;failed.failAt=fail;RunnerCollisionDispatch broken;
            assert(!broken.pair(a,b,failed)&&!broken.succeeded());
            assert(failed.trace==expected.substr(0,fail));
            assert(!broken.pair(a,b,failed)&&failed.trace.size()==fail);
        }
    }
    for(unsigned inactive=0;inactive<3;++inactive){
        Ops ops;ops.initialHit=inactive!=0;
        if(inactive)ops.bits[inactive-1]=2;
        RunnerCollisionDispatch pass;assert(pass.pair(a,b,ops));
        assert(ops.trace==(inactive==0?"O":inactive==1?"Oa":"Oab"));
    }
    Ops changed;changed.deactivateInFirst=true;RunnerCollisionDispatch pass;
    assert(pass.pair(a,b,changed)&&changed.trace=="OabQabABab");
    std::puts("Collision dispatch: overlap, inactivity, queue, paired events, solid mutations and failure prefixes passed");
}
