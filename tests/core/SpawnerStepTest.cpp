#include "core/SpawnerStep.h"
#include "core/StoredSpawnerStates.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using downwell::SpawnerStep;
using Real=downwell::MotionBinary64;
static int integer(Real n){int value;assert(n.roundPixel(value));return value;}
struct Ops {
    downwell::GameMakerRandom& random;
    explicit Ops(downwell::GameMakerRandom& source):random(source){}
    std::vector<int> calls;
    int obstruct=0,wallCalls=0,births=0,areaValue=1,count=0;
    bool dead=false,end=false,failMask=false,failCreate=false;
    Real player=Real::fromInteger(101),fall=Real::fromInteger(160);
    bool playerY(Real& v){calls.push_back(1);v=player;return true;}
    bool area(int& v){calls.push_back(2);v=areaValue;return true;}
    bool fallDepth(Real& v){calls.push_back(3);v=fall;return true;}
    bool death(bool& v){calls.push_back(4);v=dead;return true;}
    bool endReached(bool& v){calls.push_back(5);v=end;return true;}
    bool enemyCount(int& v){calls.push_back(6);v=count;return true;}
    bool objectMask(int object,int& mask){calls.push_back(7);assert(object==115);mask=174;return true;}
    bool mask(int v){calls.push_back(8);assert(v==174);return !failMask;}
    bool viewY(Real& v){calls.push_back(9);v=Real::fromInteger(34);return true;}
    bool viewHeight(Real& v){calls.push_back(10);v=Real::fromInteger(284);return true;}
    bool meeting(Real,Real,int object,bool& hit){
        calls.push_back(object);hit=object==57&&wallCalls++<obstruct;return true;
    }
    bool tile(int depth,Real,Real,bool& hit){calls.push_back(depth);hit=false;return depth==100;}
    bool create(int object,Real x,Real y,unsigned& child){
        calls.push_back(11);assert(object==115&&integer(x)>=176&&integer(x)<=304);
        assert(integer(y)>=-48&&integer(y)<=416);child=105200+births++;
        random.chooseIndexSmall(3);return !failCreate; // Observable synchronous child RNG.
    }
    bool deactivate(unsigned child){calls.push_back(12);return child==unsigned(105199+births);}
};
static SpawnerStep initial(){
    SpawnerStep s;assert(s.createCaverns(1,false));return s;
}
static void same(const downwell::GameMakerRandom& a,const downwell::GameMakerRandom& b){
    assert(a.drawCount()==b.drawCount()&&a.stateIndex()==b.stateIndex());
    for(unsigned i=0;i<16;++i)assert(a.stateWord(i)==b.stateWord(i));
}
int main(int argc,char**){
    if(argc>1){
        for(bool hard:{false,true})for(int level=1;level<=3;++level){
            SpawnerStep s;assert(s.createCaverns(level,hard));
            assert(s.enemyMaxCount==20&&s.totalDepthCount==500&&!s.fromAbove&&!s.scriptActive&&
                s.depthMeasure.zero()&&s.fallDepthMemory.zero()&&s.chan==int(s.channelCount)-1);
            for(unsigned j=0;j<s.channelCount;++j){const auto& c=s.channels[j];
                std::printf("%d %d %u %d %d %d %d %d %d",int(hard),level,j,integer(c.countdown),
                    c.interval,c.amplitude,c.minimum,c.maximum,int(c.above));
                for(unsigned e=0;e<c.enemyCount;++e)std::printf(" %d",c.enemies[e]);
                std::puts("");
            }
        }
        return 0;
    }
    for(bool above:{false,true})for(int obstruction:{0,1,31,32,33}){
        auto state=initial();state.channels[0].above=above;
        downwell::GameMakerRandom random(23),oracle(23);Ops ops{random};ops.obstruct=obstruction;
        assert(state.step(Real::fromInteger(100),random,ops));
        const int jitter=int(oracle.irandomSmall(32));const int sign=oracle.chooseIndexSmall(2)?1:-1;
        assert(integer(state.channels[0].countdown)==160+jitter*sign);
        oracle.irandomSmall(0);oracle.irandomSmall(0); // count and singleton enemy choice
        int expectedX=160+16*(2+int(oracle.irandomSmall(6)));
        const int grid=above?32:336;
        int expectedY=grid+(above?-16:16)*(1+int(oracle.irandomSmall(4)));
        for(int i=0;i<obstruction;++i){
            expectedX=160+16*((above?2:1)+int(oracle.irandomSmall(above?6:8)));
            expectedY=grid+(above?-16:16)*(1+int(oracle.irandomSmall(4)));
        }
        if(obstruction<32)oracle.chooseIndexSmall(3);
        same(random,oracle);
        assert(state.scriptActive&&integer(state.depthMeasure)==160&&integer(state.fallDepthMemory)==160);
        assert(state.wl==obstruction&&state.i==1&&state.maskIndex==174&&state.spawnAmt==1);
        assert(integer(state.randxGrid)==expectedX&&integer(state.randyGrid)==expectedY);
        assert(ops.births==(obstruction<32?1:0));
        std::vector<int> expected{1,2,3,3,4,5,6,7,8,9};if(!above)expected.push_back(10);
        for(int i=0;i<obstruction;++i)expected.push_back(57);
        if(obstruction<33){expected.push_back(57);expected.push_back(83);if(!above)expected.push_back(100);}
        expected.push_back(57); // Final wall query, including exhausted attempts.
        if(obstruction<32){expected.push_back(11);expected.push_back(12);}
        assert(ops.calls==expected);
    }
    {
        downwell::InstanceStore<2> records;
        downwell::StoredSpawnerStates<2,1> owner(records);
        assert(records.create(100,78,11,false)==downwell::CreateInstanceResult::success);
        const auto first=records.handleAt(0);
        records.find(100)->preciseY=100;
        assert(owner.createCaverns(first,1,false)&&owner.state(100)&&!owner.createCaverns(first,1,false));
        // Script active=0 does not deactivate the room record.
        assert(records.find(100)->active&&!owner.state(100)->scriptActive);
        downwell::GameMakerRandom random(23);Ops ops{random};
        assert(owner.step(first,random,ops)&&owner.state(100)->scriptActive&&ops.births==1);
        assert(records.create(101,78,11,false)==downwell::CreateInstanceResult::success);
        const auto second=records.handleAt(1);
        assert(!owner.createCaverns(second,1,false)); // Real capacity failure, not shared singleton state.
        assert(records.erase(100)&&!owner.state(100)&&!owner.step(first,random,ops));
        assert(owner.createCaverns(second,2,true)&&owner.state(101)->channels[0].above);
        assert(records.erase(101));
        assert(records.create(100,78,11,false)==downwell::CreateInstanceResult::success);
        assert(!owner.state(100)&&owner.createCaverns(records.handleAt(0),3,false));
        assert(owner.state(100)->channelCount==3&&!owner.state(100)->scriptActive);
        owner.clear();assert(!owner.state(100));
    }
    for(int guard=0;guard<5;++guard){
        auto s=initial();downwell::GameMakerRandom random(2);Ops ops{random};
        if(guard==0)ops.player=Real::fromInteger(100);
        if(guard==1)ops.areaValue=5;
        if(guard==2)ops.dead=true;
        if(guard==3)ops.end=true;
        if(guard==4)ops.count=20;
        assert(s.step(Real::fromInteger(100),random,ops)&&!random.drawCount());
        assert(integer(s.channels[0].countdown)==160&&s.scriptActive==(guard>=2));
        assert(integer(s.fallDepthMemory)==(guard>=2?160:0));
        // A blocked active tick still consumes fall-depth progression.
        if(guard>=2){ops.dead=false;ops.end=false;ops.count=0;
            assert(s.step(Real::fromInteger(100),random,ops)&&!random.drawCount());}
    }
    for(bool maskFailure:{false,true}){
        auto s=initial();downwell::GameMakerRandom random(23);Ops ops{random};
        ops.failMask=maskFailure;ops.failCreate=!maskFailure;
        assert(!s.step(Real::fromInteger(100),random,ops));
        assert(ops.calls.back()==(maskFailure?8:11)); // Never deactivate a failed birth.
        assert(s.maskIndex==(maskFailure?-1:174)&&s.i==0&&s.spawnAmt==1);
    }
    {
        auto s=initial();s.channels[1]=s.channels[0];s.channelCount=2;
        downwell::GameMakerRandom random(42),oracle(42);Ops ops{random};
        assert(s.step(Real::fromInteger(100),random,ops)&&ops.births==2&&s.i==2);
        for(unsigned i=0;i<2;++i){
            const int jitter=int(oracle.irandomSmall(32));const int sign=oracle.chooseIndexSmall(2)?1:-1;
            assert(integer(s.channels[i].countdown)==160+jitter*sign);
            oracle.irandomSmall(0);oracle.irandomSmall(0);oracle.irandomSmall(6);oracle.irandomSmall(4);
            oracle.chooseIndexSmall(3); // First child's Create changes the next channel's reset.
        }
        same(random,oracle);
    }
    Real out;
    assert(SpawnerStep::viewGrid(Real::fromInteger(-24),{},true,out)&&integer(out)==-32);
    assert(SpawnerStep::viewGrid(Real::fromInteger(-8),{},true,out)&&integer(out)==0);
    assert(SpawnerStep::viewGrid(Real::fromInteger(-17),{},false,out)&&integer(out)==0);
    assert(SpawnerStep::viewGrid(Real::fromInteger(32),{},false,out)&&integer(out)==48);
    assert(SpawnerStep::viewGrid({0x40400000u,1},{},false,out)&&integer(out)==64);
    std::puts("Spawner Step: query/RNG order, retries 31/32/33, guards, child failure and view-grid rounding passed");
}
