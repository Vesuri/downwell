#include "core/BoxDestructionScript.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
struct Ops {
    using Handle=unsigned;
    bool soundOverlap(int,int,int,int,const char*);
    bool smoke(downwell::MotionBinary64,downwell::MotionBinary64,int,int);
    bool flash(downwell::MotionBinary64,downwell::MotionBinary64,int,int,int);
    bool effect(downwell::MotionBinary64,downwell::MotionBinary64,int,downwell::MotionBinary64,downwell::MotionBinary64,int);
    bool chainReaction();bool lowSpec();int area();
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,Handle&);
    bool myBullet(Handle);bool bulletDirection(Handle,int);bool bulletDamage(Handle,int);
    bool sound(int,int,int,int);
    bool place(int,downwell::MotionBinary64,downwell::MotionBinary64,Handle&,bool&);
    bool target(Handle);bool targetAlarm(Handle,int,int);bool targetChecked(Handle,bool);
    bool destroySelf();bool currency(int);bool alarmStop(int);
};
bool nativeBoxStep(const downwell::BreakableState& b,downwell::MotionBinary64 x,
                   downwell::MotionBinary64 y,Ops& ops,bool& ok){
    return downwell::BoxDestructionScript::run(b,x,y,ops,ok);
}
struct Reader {bool operator()(downwell::BoxDestructionScript::Caller&)const;};
bool nativeRetainedBoxStep(Reader read,Ops& ops,bool& ok){
    return downwell::BoxDestructionScript::runRetained(read,ops,ok);
}
#else
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace downwell;
enum Event {Sound,Smoke,Flash,Effect,Drone,MyBullet,Direction,Damage,DroneSound,
    Place,Target,Alarm,Checked,Destroy,Debris,Currency,Stop};
struct Ops {
    using Handle=unsigned;
    bool chain,low;int roomArea;unsigned mask,fail,queries=0;
    bool destroyed=false,disableChainOnBirth=false;
    std::vector<Event> calls;
    Ops(bool c,bool l,int a,unsigned m,unsigned f):chain(c),low(l),roomArea(a),mask(m),fail(f){}
    bool event(Event e){calls.push_back(e);return calls.size()!=fail;}
    static void position(MotionBinary64 x,MotionBinary64 y,int dy=0){
        auto expected=MotionBinary64::fromInteger(512+dy);
        assert(expected.add({0x3fd00000u,0})); // Keep fractional coordinates.
        assert(x.high==0x406e0000u&&!x.low&&y.high==expected.high&&y.low==expected.low);
    }
    bool soundOverlap(int s,int volume,int pitch,int scale,const char* group){
        assert(s==174&&volume==50&&pitch==0&&scale==1&&!std::strcmp(group,"waterThings"));return event(Sound);
    }
    bool smoke(MotionBinary64 x,MotionBinary64 y,int n,int d){position(x,y,4);assert(n==1&&d==0);return event(Smoke);}
    bool flash(MotionBinary64 x,MotionBinary64 y,int a,int b,int c){position(x,y);assert(a==1&&b==0&&c==2);return event(Flash);}
    bool effect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,MotionBinary64 angle,int depth){
        position(x,y,12);assert(sprite==603&&speed.high==0x3fe00000u&&!speed.low&&
            angle.high==0x4070e000u&&!angle.low&&depth==-60000);return event(Effect);
    }
    bool chainReaction(){return chain;}
    bool lowSpec(){return low;}
    int area(){return roomArea;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
        position(x,y);assert(object==266||object==370);
        if(object==266){assert(!destroyed);h=99;if(disableChainOnBirth)chain=false;return event(Drone);}
        assert(destroyed);h=100;return event(Debris);
    }
    bool myBullet(Handle h){assert(h==99);return event(MyBullet);}
    bool bulletDirection(Handle h,int n){assert(h==99&&n==90);return event(Direction);}
    bool bulletDamage(Handle h,int n){assert(h==99&&n==20);return event(Damage);}
    bool sound(int s,int v,int p,int a){assert(s==342&&v==50&&p==0&&a==1);return event(DroneSound);}
    bool place(int object,MotionBinary64 x,MotionBinary64 y,Handle& h,bool& truth){
        const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
        assert(object==92&&queries<4);
        assert(x.add(MotionBinary64::fromInteger(-dx[queries])));position(x,y,dy[queries]);
        truth=mask&(1u<<queries);h=truth?queries+1:0; ++queries;return event(Place);
    }
    bool target(Handle h){assert(h==0||h==queries);return event(Target);}
    bool targetAlarm(Handle h,int a,int n){assert(h==queries&&a==0&&n==5);return event(Alarm);}
    bool targetChecked(Handle h,bool checked){assert(h==queries&&!checked);return event(Checked);}
    bool destroySelf(){if(!event(Destroy))return false;destroyed=true;return true;}
    bool currency(int amount){assert(destroyed&&(amount==3||amount==10));return event(Currency);}
    bool alarmStop(int a){assert(a==0);return event(Stop);}
};
struct MutableCallerOps:Ops {
    BoxDestructionScript::Caller caller;
    unsigned reads=0,failRead=0;
    MutableCallerOps():Ops(false,false,1,0,0){
        caller.state.wallHp={};caller.state.breakSound=174;caller.state.haveGem=1;
        caller.x=MotionBinary64::fromInteger(240);caller.y=MotionBinary64::fromInteger(512);
        assert(caller.y.add({0x3fd00000u,0}));
    }
    bool read(BoxDestructionScript::Caller& out){
        if(++reads==failRead)return false;
        out=caller;return true;
    }
    void move(){assert(caller.x.add(MotionBinary64::fromInteger(1))&&caller.y.add(MotionBinary64::fromInteger(2)));}
    void positionNow(MotionBinary64 x,MotionBinary64 y,int offset=0){
        auto expected=caller.y;assert(expected.add(MotionBinary64::fromInteger(offset)));
        assert(x.high==caller.x.high&&x.low==caller.x.low&&y.high==expected.high&&y.low==expected.low);
    }
    bool soundOverlap(int s,int v,int p,int c,const char* group){
        const bool ok=Ops::soundOverlap(s,v,p,c,group);move();return ok;
    }
    bool smoke(MotionBinary64 x,MotionBinary64 y,int,int){positionNow(x,y,4);move();return event(Smoke);}
    bool flash(MotionBinary64 x,MotionBinary64 y,int,int,int){positionNow(x,y);move();return event(Flash);}
    bool effect(MotionBinary64 x,MotionBinary64 y,int,MotionBinary64,MotionBinary64,int){
        positionNow(x,y,12);move();return event(Effect);
    }
    bool destroySelf(){const bool ok=Ops::destroySelf();move();return ok;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
        assert(object==370&&destroyed);positionNow(x,y);move();h=100;return event(Debris);
    }
    bool currency(int amount){
        const bool ok=Ops::currency(amount);
        if(amount==3)caller.state.haveGem=2;
        return ok;
    }
};
int main(){
    MutableCallerOps mutation;bool healthy=true;
    assert(BoxDestructionScript::runRetained([&](auto& c){return mutation.read(c);},mutation,healthy)&&healthy);
    assert(mutation.calls==std::vector<Event>({Sound,Smoke,Flash,Effect,Destroy,Debris,Debris,Currency,Currency,Stop}));
    for(unsigned failure=1;failure<=mutation.reads;++failure){
        MutableCallerOps rejected;rejected.failRead=failure;bool okay=true;
        auto read=[&](auto& c){return rejected.read(c);};
        assert(!BoxDestructionScript::runRetained(read,rejected,okay)&&!okay&&rejected.reads==failure);
        const auto prefix=rejected.calls;
        assert(!BoxDestructionScript::runRetained(read,rejected,okay)&&rejected.reads==failure&&rejected.calls==prefix);
    }
    for(int damage:{9,15}){
        BreakableState state;bool applied=false;
        assert(state.bulletHit(MotionBinary64::fromInteger(damage),applied)&&applied&&state.hit&&state.exhausted());
        const auto expected=MotionBinary64::fromInteger(1-damage);
        assert(state.wallHp.high==expected.high&&state.wallHp.low==expected.low);
        assert(state.bulletHit(MotionBinary64::fromInteger(damage),applied)&&!applied&&state.wallHp.high==expected.high);
    }
    {BreakableState state;bool applied=false;state.wallHp={0x3fe00000u,0};
        assert(state.bulletHit({0x3fd00000u,0},applied)&&applied&&!state.exhausted()&&state.wallHp.high==0x3fd00000u);
        const auto before=state.wallHp;
        assert(!state.bulletHit({0x7ff00000u,0},applied)&&state.wallHp.high==before.high&&state.wallHp.low==before.low);}
    for(int value:{-2,-1,0,1,5})for(bool allowed:{false,true}){
        BreakableState state;state.scriptIndex=17;auto alarm=value;
        assert(state.stopAlarmZero(allowed,alarm));
        assert(alarm==value+(!allowed&&value>0?1:0)&&state.scriptIndex==(allowed?17:1));
    }
    {BreakableState state;state.scriptIndex=17;std::int32_t alarm=0x7fffffff;
        assert(!state.stopAlarmZero(false,alarm)&&alarm==0x7fffffff&&state.scriptIndex==0);}
    const auto x=MotionBinary64::fromInteger(240);
    auto y=MotionBinary64::fromInteger(512);assert(y.add({0x3fd00000u,0}));
    unsigned routes=0,failures=0;
    for(bool chain:{false,true})for(bool low:{false,true})for(bool tile:{false,true})
    for(int area:{1,4})for(unsigned gems:{0u,1u,2u})for(unsigned mask=0;mask<16;++mask){
        BreakableState b;b.wallHp={0xbfe00000u,0};b.breakSound=174;b.haveGem=gems;b.autoTile=tile;
        std::vector<Event> expected{Sound,Smoke,Flash,Effect};
        if(chain){
            for(auto e:{Drone,MyBullet,Direction,Damage,DroneSound})expected.push_back(e);
            for(unsigned i=0;i<4;++i){
                expected.push_back(Place);expected.push_back(Target);
                if(mask&(1u<<i)){expected.push_back(Alarm);if(tile)expected.push_back(Checked);}
            }
        }
        for(auto e:{Destroy,Debris})expected.push_back(e);
        if(!low)expected.push_back(Debris);
        if(area!=4&&gems)expected.push_back(Currency);
        expected.push_back(Stop);
        for(unsigned fail=0;fail<=expected.size();++fail){
            Ops ops{chain,low,area,mask,fail};bool ok=true;
            assert(BoxDestructionScript::run(b,x,y,ops,ok)==(fail==0)&&ok==(fail==0));
            assert(ops.calls==std::vector<Event>(expected.begin(),fail?expected.begin()+fail:expected.end()));
            if(fail){
                assert(!BoxDestructionScript::run(b,x,y,ops,ok)&&ops.calls.size()==fail);++failures;
            }
        }
        ++routes;
    }
    BreakableState b;b.breakSound=174;bool ok=true;Ops intact{false,false,1,0,0};
    assert(BoxDestructionScript::run(b,x,y,intact,ok)&&intact.calls==std::vector<Event>{Stop});
    b.wallHp={};Ops changed{true,true,4,15,0};changed.disableChainOnBirth=true;
    assert(BoxDestructionScript::run(b,x,y,changed,ok)&&changed.queries==0&&changed.destroyed);
    b.wallHp={0x7ff00000u,0};Ops bad{false,false,1,0,0};
    assert(!BoxDestructionScript::run(b,x,y,bad,ok)&&!ok&&bad.calls.empty());
    std::printf("Box destruction: %u routes, %u failure prefixes; fractional offsets and post-destroy children verified\n",routes,failures);
}
#endif
