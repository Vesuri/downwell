#include "core/CurrencySpawnScript.h"
#include "core/CurrencySpawnVerification.h"
#include "core/CurrencyCreate.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static bool equal(MotionBinary64 a,double d){const auto b=real(d);return a.high==b.high&&a.low==b.low;}
struct Event {
    int kind,id,value;
    bool operator==(const Event& b)const{return kind==b.kind&&id==b.id&&value==b.value;}
};
struct Ops {
    using Handle=std::uint32_t;
    CurrencySpawnSource<Handle>& source;
    GameMakerRandom random;
    std::vector<CurrencyCreate> children;
    std::vector<Event> events;
    int parentId,area;
    bool upgrade;
    unsigned fail=0;
    bool promoteSmall=false;
    Ops(CurrencySpawnSource<Handle>& s,unsigned seed,int p,int a,bool u,unsigned f=0):
        source(s),random(seed),parentId(p),area(a),upgrade(u),fail(f){}
    bool call(int kind,int id,int value){events.push_back({kind,id,value});return events.size()!=fail;}
    bool parent(int object,int& parent){assert(object==92);parent=parentId;return call(0,object,parent);}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& child){
        assert(object==248&&equal(x,12.25)&&equal(y,-3.5));
        const auto previous=source.myGem;
        if(!call(1,int(children.size()+1),248))return false;
        assert(children.size()<64);children.emplace_back();
        struct Queries {
            CurrencySpawnSource<Handle>& source;Handle previous;
            bool wall(MotionBinary64,MotionBinary64,bool& hit){assert(source.myGem==previous);hit=false;return true;}
            bool tile(MotionBinary64,MotionBinary64,bool& hit){assert(source.myGem==previous);hit=false;return true;}
            bool publish(RunnerImageValue,RunnerImageValue){assert(false);return false;}
        } queries{source,previous};
        assert(children.back().create(x,y,area,upgrade,random,queries));
        if(promoteSmall&&children.size()==1)source.moniAmount=MotionBinary64::fromInteger(20);
        child=children.size();return true;
    }
    bool amount(Handle child,int amount){
        assert(source.myGem==child&&child&&child<=children.size());
        if(!call(2,child,amount))return false;children[child-1].getAmount=amount;return true;
    }
    bool size(Handle child,int size){
        if(!call(3,child,size))return false;children[child-1].size=size;return true;
    }
    bool attracted(Handle child,bool attracted){
        if(!call(4,child,attracted))return false;children[child-1].attracted=attracted;return true;
    }
};
int main(){
    assert(CurrencySpawnVerification::run());
    for(unsigned seed=0;seed<256;++seed)for(int area=1;area<=5;++area){
        GameMakerRandom random(seed),oracle(seed);GemMotion motion;
        motion.initialize(12,-3,false,random,false,area);
        // Compare the existing fixed-point projection to independent VM
        // random_range/random expressions, including the discarded y draw.
        const long vx=long(oracle.random(4)*12800)-25600;
        long vy=long(oracle.random(4)*12800)-25600;
        if(area==5)vy=-long(oracle.random(3)*12800);
        assert(motion.vx==vx&&motion.vy==vy);
        oracle.chooseIndexSmall(4);oracle.irandomSmall(70);oracle.irandomSmall(70);oracle.chooseIndexSmall(2);
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
    }
    unsigned cases=0,prefixes=0;
    for(unsigned seed=0;seed<16;++seed)for(int parent:{83,92})for(int area:{1,5})
    for(bool upgrade:{false,true})for(int stomp:{0,1,2})for(double amount:{-1.,0.,.25,1.,3.,9.75,10.,10.25,21.,35.}){
        CurrencySpawnSource<std::uint32_t> source;source.x=real(12.25);source.y=real(-3.5);
        source.objectIndex=92;source.stomped=real(stomp);source.myGem=77;source.atrState=true;
        Ops ops(source,seed,parent,area,upgrade);bool good=true;
        assert(CurrencySpawnScript::run(source,real(amount),ops,good)&&good);
        const bool attracted=parent==83&&stomp==1;
        std::vector<Event> expected{{0,92,parent}};std::vector<int> amounts;
        double rest=amount;unsigned id=0;
        while(rest>=10){++id;expected.push_back({1,int(id),248});expected.push_back({2,int(id),10});
            expected.push_back({3,int(id),1});if(attracted)expected.push_back({4,int(id),1});
            amounts.push_back(10);rest-=10;}
        while(rest>0){++id;expected.push_back({1,int(id),248});expected.push_back({2,int(id),2});
            if(attracted)expected.push_back({4,int(id),1});amounts.push_back(2);rest-=2;}
        assert(ops.events==expected&&source.atrState==attracted&&equal(source.moniAmount,rest));
        assert(source.myGem==(id?id:77)&&ops.children.size()==id);
        GameMakerRandom oracle(seed);
        for(unsigned i=0;i<id;++i){
            const double vx=oracle.random(4)-2;double vy=oracle.random(4)-2;if(area==5)vy=-oracle.random(3);
            const double rates[]={-.5,-.3,.3,.5};const double rate=rates[oracle.chooseIndexSmall(4)];
            const int second=170+int(oracle.irandomSmall(70)),first=int(oracle.irandomSmall(70))-60;
            const int direction=oracle.chooseIndexSmall(2)?second:first;
            const auto& gem=ops.children[i];
            assert(equal(gem.xsp,vx)&&equal(gem.ysp,vy)&&equal(gem.imgSp,rate)&&gem.direction==direction);
            assert(gem.getAmount==amounts[i]&&gem.size==int(amounts[i]==10)&&gem.attracted==attracted);
            assert(equal(gem.xx,12.25)&&equal(gem.yy,-3.5)&&gem.alarms.get(3)==(upgrade?30:15));
        }
        assert(ops.random.drawCount()==oracle.drawCount()&&ops.random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(ops.random.stateWord(i)==oracle.stateWord(i));++cases;
        for(unsigned fail=1;fail<=expected.size();++fail){
            auto caller=source;caller.myGem=77;Ops bad(caller,seed,parent,area,upgrade,fail);bool valid=true;
            assert(!CurrencySpawnScript::run(caller,real(amount),bad,valid)&&!valid);
            assert(bad.events==std::vector<Event>(expected.begin(),expected.begin()+fail));
            unsigned born=0;for(unsigned j=0;j+1<fail;++j)if(expected[j].kind==1)++born;
            assert(bad.children.size()==born&&caller.myGem==(born?born:77));
            assert(bad.random.drawCount()==born*unsigned(area==5?9:8));
            assert(!CurrencySpawnScript::run(caller,real(amount),bad,valid)&&bad.events.size()==fail);++prefixes;
        }
    }
    {CurrencySpawnSource<std::uint32_t> source;source.x=real(12.25);source.y=real(-3.5);source.objectIndex=92;
        Ops ops(source,9,92,1,false);ops.promoteSmall=true;bool good=true;
        assert(CurrencySpawnScript::run(source,real(1),ops,good)&&ops.children.size()==10);
        for(const auto& gem:ops.children)assert(gem.getAmount==2&&gem.size==0);assert(source.moniAmount.zero());}
    {struct Mutating:Ops {
        using Ops::Ops;
        bool amount(Handle child,int n){if(!Ops::amount(child,n))return false;source.myGem=99;return true;}
        bool attracted(Handle child,bool value){return child==99&&value&&call(4,child,1);}
    };
        CurrencySpawnSource<std::uint32_t> source;source.x=real(12.25);source.y=real(-3.5);source.objectIndex=92;source.stomped=real(1);
        Mutating ops(source,9,83,1,false);bool good=true;
        assert(CurrencySpawnScript::run(source,real(10),ops,good)&&source.myGem==99&&ops.children[0].size==1);
    }
    std::printf("Currency spawn: %u real Create chains, %u failure prefixes and caller mutation boundaries passed\n",cases,prefixes);
}
