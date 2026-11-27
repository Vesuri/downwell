#include "core/SpawnerFixedStep.h"
#include "core/SpawnerStep.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The compact world's Q8 spawner against SpawnerStep, its VM form: the same
// player path, fall depths, view, walls and enemies give the same RNG draws,
// masks and creations, in both difficulties and all three levels.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Spawner fixed check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
namespace {
N q8(std::int32_t v){N r=N::fromInteger(v);if(!r.zero())r.high-=8u<<20;return r;}
int pixel(N v){std::int32_t n;checkAt(v.roundPixel(n),__LINE__);return int(n);}
struct World {
    std::int32_t playerYQ8=0,fallDepthQ8=0,viewYQ8=0;
    int enemies=0;bool dead=false,end=false;unsigned salt=0;
    // Walls and enemies at a few grid cells, varying with the salt.
    bool hit(int x,int y,int object)const{
        const unsigned h=unsigned(x*73856093)^unsigned(y*19349663)^unsigned(object*83492791)^salt;
        return (h>>7)%5==0;
    }
};
struct Log {int creates=0,masks=0;int last[3]{};};
struct ReferenceOps {
    const World& world;Log& log;
    bool playerY(N& v){v=q8(world.playerYQ8);return true;}
    bool area(int& v){v=1;return true;}
    bool fallDepth(N& v){v=q8(world.fallDepthQ8);return true;}
    bool death(bool& v){v=world.dead;return true;}
    bool endReached(bool& v){v=world.end;return true;}
    bool enemyCount(int& v){v=world.enemies;return true;}
    bool objectMask(int object,int& v){v=object+1000;return true;}
    bool mask(int v){log.masks+=v;return true;}
    bool meeting(N x,N y,int object,bool& hit){hit=world.hit(pixel(x),pixel(y),object);return true;}
    bool tile(int depth,N x,N y,bool& hit){hit=world.hit(pixel(x),pixel(y),depth);return true;}
    bool viewY(N& v){v=q8(world.viewYQ8);return v.add(N::fromInteger(34));}
    bool viewHeight(N& v){v=N::fromInteger(284);return true;}
    bool create(int object,N x,N y,unsigned& id){
        ++log.creates;log.last[0]=object;log.last[1]=pixel(x);log.last[2]=pixel(y);id=1;return true;
    }
    bool deactivate(unsigned){return true;}
};
struct FixedOps {
    const World& world;Log& log;
    bool playerYQ8(std::int32_t& v){v=world.playerYQ8;return true;}
    bool area(int& v){v=1;return true;}
    bool fallDepthQ8(std::int32_t& v){v=world.fallDepthQ8;return true;}
    bool death(bool& v){v=world.dead;return true;}
    bool endReached(bool& v){v=world.end;return true;}
    bool enemyCount(int& v){v=world.enemies;return true;}
    bool objectMask(int object,int& v){v=object+1000;return true;}
    bool mask(int v){log.masks+=v;return true;}
    bool meeting(int x,int y,int object,bool& hit){hit=world.hit(x,y,object);return true;}
    bool tile(int depth,int x,int y,bool& hit){hit=world.hit(x,y,depth);return true;}
    bool viewYQ8(std::int32_t& v){v=world.viewYQ8+34*256;return true;}
    bool viewHeight(int& v){v=284;return true;}
    bool create(int object,int x,int y,unsigned& id){
        ++log.creates;log.last[0]=object;log.last[1]=x;log.last[2]=y;id=1;return true;
    }
    bool deactivate(unsigned){return true;}
};
}
int main(){
    // The view grid on every Q8 offset of a few 16-pixel cells, both ways.
    for(std::int32_t y=-70000;y<=70000;y+=37)for(bool above:{false,true}){
        N reference;int fixed;
        check(downwell::SpawnerStep::viewGrid(q8(y),N::fromInteger(284),above,reference));
        check(downwell::SpawnerFixedStep::viewGrid(y,284,above,fixed)&&pixel(reference)==fixed);
    }
    unsigned steps=0,creates=0;
    for(bool hard:{false,true})for(int level=1;level<=3;++level)for(unsigned seed=0;seed<24;++seed){
        downwell::SpawnerStep reference;downwell::SpawnerFixedStep fixed;
        check(reference.createCaverns(level,hard)&&fixed.createCaverns(level,hard));
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),path(seed+500);
        World world;Log referenceLog,fixedLog;
        ReferenceOps referenceOps{world,referenceLog};FixedOps fixedOps{world,fixedLog};
        const int spawnerY=200+16*int(path.irandomSmall(10));
        for(unsigned tick=0;tick<3000;++tick){
            // The player falls in Q8 steps, sometimes resting or rising.
            const std::int32_t fall=std::int32_t(path.irandomSmall(1400))-200;
            world.playerYQ8+=fall;world.fallDepthQ8=world.playerYQ8-spawnerY*256+3*256;
            world.viewYQ8=world.playerYQ8-120*256+std::int32_t(path.irandomSmall(255));
            world.enemies=int(path.irandomSmall(24));
            world.dead=tick>2900&&path.irandomSmall(3)==0;world.end=path.irandomSmall(200)==0;
            world.salt=unsigned(tick);
            check(reference.step(N::fromInteger(spawnerY),referenceRandom,referenceOps)&&
                  fixed.step(spawnerY,fixedRandom,fixedOps));
            check(referenceRandom.drawCount()==fixedRandom.drawCount()&&reference.scriptActive==fixed.scriptActive);
            check(referenceLog.creates==fixedLog.creates&&referenceLog.masks==fixedLog.masks);
            for(int k=0;k<3;++k)check(referenceLog.last[k]==fixedLog.last[k]);
            for(unsigned c=0;c<reference.channelCount;++c){
                std::int32_t n;
                N scaled=reference.channels[c].countdown;
                if(!scaled.zero())scaled.high+=8u<<20;
                check(scaled.roundPixel(n)&&n==fixed.channels[c].countdownQ8);
            }
            ++steps;
        }
        creates+=unsigned(fixedLog.creates);
    }
    std::printf("Spawner Q8: %u steps, %u creations, RNG, masks, countdowns and placements equal to SpawnerStep; "
                "view grid equal on -70000..70000 Q8\n",steps,creates);
    check(creates>1000);
}
