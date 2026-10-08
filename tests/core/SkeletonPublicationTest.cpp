#include "core/SkeletonActor.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace downwell;
static void check(bool value){if(!value)std::abort();}
static MotionBinary64 number(double value){std::uint64_t bits;std::memcpy(&bits,&value,8);return {unsigned(bits>>32),unsigned(bits)};}
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct Trace {
    unsigned fail=0;
    std::vector<int> all,scripts;
    bool emit(int id){all.push_back(id);if(id>=10)scripts.push_back(id);return all.size()!=fail;}
};
struct Publish {
    SkeletonActor& actor;Trace& trace;
    bool imageSpeed(RunnerImageValue v){if(!trace.emit(1))return false;actor.imageSpeed=v.promoted();return true;}
    bool imageIndex(RunnerImageValue v){if(!trace.emit(2))return false;actor.imageIndex=v.promoted();return true;}
    bool sprite(int id){return trace.emit(3)&&actor.setSprite(id);}
    bool facing(int v){if(!trace.emit(4))return false;actor.facing=v;return true;}
    bool alarm(unsigned i,int v){if(!trace.emit(6))return false;actor.alarms[i]=v;return true;}
    bool position(RunnerImageValue x,RunnerImageValue y){
        if(!trace.emit(5))return false;return x.promoted().roundPixel(actor.x)&&y.promoted().roundPixel(actor.y);
    }
    bool destroy(){if(!trace.emit(30))return false;actor.destroyRequested=true;return true;}
};
struct Effects {
    SkeletonActor& actor;Trace& trace;
    bool hurtSound(int id){check(id==133);return trace.emit(11);}
    bool throwBone(int facing){check(facing==actor.facing);return trace.emit(12);}
    bool enemyDeath(){return trace.emit(20);}
    bool blood(int a,int b){check(a==10&&b==3);return trace.emit(21);}
    bool smoke(MotionBinary64 x,MotionBinary64 y,int a,int b){check(same(x,actor.xx)&&same(y,actor.yy)&&a==5&&b==1);return trace.emit(22);}
    bool flash(MotionBinary64 x,MotionBinary64 y,int a,int b,int c){check(same(x,actor.xx)&&same(y,actor.yy)&&a==6&&b==2&&c==10);return trace.emit(23);}
    bool currency(int amount){check(amount==10);return trace.emit(24);}
    bool deathSound(int id){check(id==132);return trace.emit(25);}
    bool deadBody(int id,MotionBinary64 impact,int debris){
        check(id==206&&same(impact,number(9))&&debris==102);
        if(!trace.emit(26))return false;actor.xsp=number(3);return true;
    }
    bool debris(int x,int y){check(x==actor.x&&y==actor.y-5);return trace.emit(27);}
    void destroy(){check(trace.emit(30));}
};
static void compare(const SkeletonActor& a,const SkeletonActor& b){
    check(a.x==b.x&&a.y==b.y&&same(a.xx,b.xx)&&same(a.yy,b.yy)&&same(a.xsp,b.xsp)&&same(a.ysp,b.ysp)&&
        same(a.imageIndex,b.imageIndex)&&same(a.imageSpeed,b.imageSpeed)&&same(a.rate,b.rate)&&
        a.timeline.bits==b.timeline.bits&&a.sprite==b.sprite&&a.facing==b.facing&&a.health==b.health&&
        a.active==b.active&&a.hitPending==b.hitPending&&a.hitStun==b.hitStun&&a.shot==b.shot&&
        a.alive==b.alive&&a.destroyRequested==b.destroyRequested&&same(a.takenImpact,b.takenImpact));
    for(unsigned i=0;i<4;++i)check(a.alarms[i]==b.alarms[i]);
}
int main(){
    unsigned prefixes=0;
    for(unsigned mode=0;mode<2048;++mode){
        SkeletonActor initial;check(initial.create(96,192));
        initial.active=(mode&1)!=0;initial.hitPending=(mode&2)!=0;initial.hitStun=(mode&4)!=0;
        initial.shot=(mode&8)!=0;initial.grounded=(mode&16)!=0;initial.health=mode&32?0:60;
        initial.sprite=mode&64?207:208;
        const double index=mode&64?(mode&128?6.5:4.5):2.5;
        initial.imageIndex=number(index);check(initial.timeline.assign(initial.imageIndex));
        initial.imageSpeed={};initial.rate=SkeletonTiming::throwRate();
        initial.xsp=number(mode&256?-1:1);initial.ysp=number(0.5);initial.takenImpact=number(9);
        initial.alarms[0]=2;
        const bool stopped=(mode&512)!=0;
        const auto playerX=number(mode&1024?80:112),playerY=number(mode&1?300:80);
        auto actual=initial,reference=initial;Trace trace,referenceTrace;
        Publish publish{actual,trace};Effects effects{actual,trace},oracle{reference,referenceTrace};bool good=true;
        check(actual.stepPublished(stopped,playerX,playerY,publish,effects,good)&&good);
        check(reference.stepAtPlayer(stopped,playerX,playerY,oracle));
        compare(actual,reference);check(trace.scripts==referenceTrace.scripts);
        for(unsigned failure=1;failure<=trace.all.size();++failure){
            auto failed=initial;Trace rejected;rejected.fail=failure;Publish p{failed,rejected};Effects e{failed,rejected};bool valid=true;
            check(!failed.stepPublished(stopped,playerX,playerY,p,e,valid)&&!valid&&rejected.all.size()==failure);
            for(unsigned i=0;i<failure;++i)check(rejected.all[i]==trace.all[i]);
            check(!failed.stepPublished(stopped,playerX,playerY,p,e,valid)&&rejected.all.size()==failure);
            if(rejected.all.back()==11)check(failed.hitPending&&failed.hitStun&&failed.alarms[1]==4&&same(failed.rate,initial.rate));
            if(rejected.all.back()==25)check(!failed.alive&&!failed.destroyRequested&&same(failed.takenImpact,number(9)));
            ++prefixes;
        }
    }
    std::printf("Skeleton publication: 2048 scalar Step comparisons and %u terminal callback prefixes passed\n",prefixes);
}
