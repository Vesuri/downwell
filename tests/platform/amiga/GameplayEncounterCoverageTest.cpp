#include "GameplayEncounterCoverage.h"
#include "core/GameplayContactProfileInput.h"
#include "core/GemLifetime.h"
#include <cassert>
#include <vector>

namespace {
struct Coordinate {
    int value=0;bool valid=true;
    const Coordinate& promoted()const{return *this;}
    bool roundPixel(std::int32_t& out)const{out=value;return valid;}
};
struct Actor {
    int x=0,y=0;bool active=true,destroyRequested=false;
    struct Motion {downwell::GemLifetime lifetime;} motion;
};
struct Effect {Coordinate x{},y{};bool destroyRequested=false;int flash=0;};
struct Effects {
    std::vector<Effect> values;
    template<class Visit> bool visitNewest(Visit visit)const{
        for(const auto& e:values)visit(0,e);return true;
    }
};
struct World {
    std::vector<Actor> enemies,bullets,gems;
    Effects smoke,debris;int cameraX=0,cameraY=0;
    int cameraOffsetX()const{return cameraX;}
    int cameraOffsetY()const{return cameraY;}
    unsigned enemyCount()const{return enemies.size();}
    unsigned bulletSlotEnd()const{return bullets.size();}
    unsigned gemSlotEnd()const{return gems.size();}
    const Actor& enemyAt(unsigned i)const{return enemies[i];}
    const Actor& bulletAt(unsigned i)const{return bullets[i];}
    const Actor& gemAt(unsigned i)const{return gems[i];}
    const Effects& smokeEffects()const{return smoke;}
    const Effects& debrisEffects()const{return debris;}
};
}
int main(){
    const unsigned movement[]={30,60,120,240,360,480,720,1800};
    const unsigned fire[]={2,4,6,10,16,24,40,60};
    for(unsigned route=0;route<64;++route)for(unsigned tick=0;tick<1800;++tick)
        for(int y:{1263,1264})for(int x:{237,238,240,242,243}){
            downwell::Input input;input.quit=input.pause=true;
            assert(downwell::gameplayEncounterProfileInput(tick,x,y,route,input));
            const bool right=(tick/movement[route/8])%2==0;
            assert(input.right==(y<1264?x<238:right));
            assert(input.left==(y<1264?x>242:!right));
            assert(input.fire==(tick%fire[route%8]<fire[route%8]/2));
            assert(!input.quit&&!input.pause);
        }
    downwell::Input unchanged;unchanged.quit=unchanged.pause=unchanged.fire=true;
    assert(!downwell::gameplayEncounterProfileInput(0,240,1200,64,unchanged));
    assert(unchanged.quit&&unchanged.pause&&unchanged.fire&&!unchanged.left&&!unchanged.right);
    World world;
    // Source camera bounds [160,320)x[34,318); actor coordinates are relative
    // to (240,176), whereas particles are absolute. Boundary pixels matter.
    world.enemies={{-80,-142},{79,141},{80,0},{0,142},{-81,0},{0,-143},
        {0,0,false,false},{0,0,true,true}};
    world.bullets={{0,0},{0,0,false,false},{0,0,true,true}};
    world.gems={{0,0},{0,0,false,false}};
    world.smoke.values={{{160},{34}},{{320},{34}},{{240},{176},true}};
    world.debris.values={{{319},{317}},{{240},{176},false,1}};
    GameplayEncounterCoverage coverage;
    assert(coverage.sample(world,1));
    assert((coverage.peak==GameplayEncounterCoverage::Counts{2,1,1,1,1}));
    assert(coverage.peakTotal==6&&coverage.peakFrame==1);
    world.cameraX=160;assert(coverage.sample(world,2));
    assert(coverage.peakTotal==6&&coverage.peakFrame==1);
    GameplayEncounterCoverage shifted;
    assert(shifted.sample(world,2));
    assert((shifted.peak==GameplayEncounterCoverage::Counts{1,0,0,1,0}));
    world={};world.bullets.resize(24);
    assert(coverage.sample(world,3)&&coverage.sample(world,4));
    assert(coverage.denseFrames==2&&coverage.longestDenseRun==2);
    world.bullets.clear();assert(coverage.sample(world,5));
    world.gems.resize(24);assert(coverage.sample(world,6));
    assert(coverage.denseFrames==3&&coverage.longestDenseRun==2);
    assert(coverage.peakFrame==3&&coverage.peak[1]==24&&coverage.peak[2]==0);
    assert(coverage.maxima[1]==24&&coverage.maxima[2]==24);
    // Live centres are not visible sprite parts: disappearance flashes still
    // count as live actors but must not qualify a raster-overflow workload.
    World flashing;flashing.gems.resize(9);
    flashing.gems[0].motion.lifetime.age=downwell::GemLifetime::disappearingAge;
    GameplayEncounterCoverage visible;
    assert(visible.sample(flashing,10));
    assert(visible.maxima[2]==9&&visible.maxVisibleCurrency==8&&visible.visibleCurrencyFrame==10);
    ++flashing.gems[0].motion.lifetime.age;
    assert(visible.sample(flashing,11));
    assert(visible.maxVisibleCurrency==9&&visible.visibleCurrencyFrame==11);
    flashing.cameraX=160;
    GameplayEncounterCoverage offscreen;
    assert(offscreen.sample(flashing,12)&&offscreen.maxVisibleCurrency==0);
    world.smoke.values={{{0,false},{0}}};assert(!coverage.sample(world,7));
    std::cout<<"Encounter census bounds, lifetimes, families and runs passed\n";
}
