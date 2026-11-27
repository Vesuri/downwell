#include "core/CompactScenery.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
// Event-level checks for the compact world-one scenery owners. The slice
// route (AmigaSceneRuntimeTest) opens a gem chunk, lights torches and, since
// W1.5, douses side-room TorchFires; this covers each event in isolation.
using downwell::Fixed16;
using downwell::RunnerImageValue;
namespace {
struct Ops {
    struct Moving {int sprite;RunnerImageValue rate;Fixed16 angle,speed;};
    std::vector<Moving> moving;std::vector<int> sounds;
    int coins=0,attracted=0,flashes=0,appearance=0,masks=0,destroyed=0,fires=0;
    struct Burst {int xRange,xLow,yRange,yLow,amount;bool large;};
    std::vector<Burst> bursts;
    bool visible=true,solidAt=false,solidBelow=false,wallRight=false,wallLeft=false;
    bool inView(int,int,bool& v){v=visible;return true;}
    bool outOfView(int,bool& retained){retained=true;return true;}
    bool sound(int id,int,int,int){sounds.push_back(id);return true;}
    bool movingFx(Fixed16,Fixed16,int sprite,RunnerImageValue rate,Fixed16 angle,Fixed16 speed){
        moving.push_back({sprite,rate,angle,speed});return true;}
    bool currency(int amount){coins+=amount;return true;}
    bool attractGem(){++attracted;return true;}
    bool flash(Fixed16,Fixed16){++flashes;return true;}
    bool appearanceChanged(){++appearance;return true;}
    bool openMask(){++masks;return true;}
    bool burstGem(int a,int b,int c,int d,int amount,bool large){bursts.push_back({a,b,c,d,amount,large});return true;}
    bool solid(int,int y,bool& hit){hit=y%16?solidBelow:solidAt;return true;}
    bool wall(int x,int,bool& hit){hit=x&1?wallRight:wallLeft;return true;}
    bool destroy(){++destroyed;return true;}
    bool createFire(int,int){++fires;return true;}
};
void dousedFire(bool stomped){
    downwell::GameMakerRandom random(1234);Ops ops;
    downwell::CompactTorchFireRecord fire;fire.create(100,200);
    for(int i=0;i<30;++i)assert(fire.step(random,ops));
    assert(fire.active&&ops.moving.empty()&&!ops.coins);
    const auto before=random.drawCount();
    fire.objHp=0;fire.stomped=stomped;
    assert(fire.step(random,ops));
    assert(!fire.active&&!fire.masked);
    assert(ops.sounds.size()==1&&ops.sounds[0]==359);
    assert(ops.moving.size()==2&&ops.moving[0].sprite==84&&ops.moving[1].sprite==84);
    // Two puffs at random(20) and 180-random(20) degrees, speed 1, rate 0.5.
    assert(ops.moving[0].speed==Fixed16::fromInteger(1)&&ops.moving[0].rate.bits==0x3f000000u);
    assert(ops.moving[0].angle.v>=0&&ops.moving[0].angle.v<=20*65536);
    assert(ops.moving[1].angle.v>160*65536&&ops.moving[1].angle.v<=180*65536);
    assert(ops.coins==2&&ops.attracted==(stomped?1:0));
    assert(random.drawCount()>before);
    // Doused fire keeps burning out but neither respawns nor rewards again.
    const auto puffs=ops.moving.size();
    for(int i=0;i<60;++i)assert(fire.step(random,ops));
    assert(ops.moving.size()==puffs&&ops.coins==2&&ops.sounds.size()==1);
}
void offscreenFireDrawsNothing(){
    downwell::GameMakerRandom random(99);Ops ops;ops.visible=false;
    downwell::CompactTorchFireRecord fire;fire.create(10,20);
    const auto before=random.drawCount();
    for(int i=0;i<40;++i)assert(fire.step(random,ops));
    assert(random.drawCount()==before);
}
void torchPlacement(){
    Ops ops;ops.solidBelow=true;
    downwell::CompactTorchRecord torch;torch.x=64;torch.y=96;
    assert(torch.step(ops)&&torch.allSet&&torch.placement==0&&ops.fires==1&&!ops.destroyed);
    assert(torch.step(ops)&&ops.fires==1); // only its first Step acts
    Ops buried;buried.solidAt=true;
    downwell::CompactTorchRecord inWall;inWall.x=64;inWall.y=96;
    assert(inWall.step(buried)&&inWall.placement==-1&&buried.destroyed==1&&!buried.fires);
}
void gemChunk(){
    downwell::GameMakerRandom random(7);Ops ops;
    downwell::CompactGemChunkRecord chunk;
    chunk.wallHp=28;assert(chunk.step(random,ops)&&ops.bursts.empty());
    chunk.wallHp=27;assert(chunk.step(random,ops));
    assert(ops.bursts.size()==1&&ops.bursts[0].amount==2&&!ops.bursts[0].large&&chunk.flashing&&chunk.prvWallHp==27);
    assert(chunk.step(random,ops)&&ops.bursts.size()==1); // flashing until Alarm 0
    for(int hp=24;hp>=0;hp-=3){
        assert(chunk.alarm0(random,ops)&&!chunk.flashing);
        chunk.wallHp=hp;assert(chunk.step(random,ops));
    }
    assert(chunk.destroyed&&!chunk.opened&&ops.bursts.size()==10);
    assert(chunk.step(random,ops)&&chunk.opened&&chunk.sprite==570&&ops.masks==1&&ops.flashes==1);
    assert(ops.moving.size()==1&&ops.moving[0].sprite==118&&ops.bursts.size()==20);
    for(unsigned i=10;i<20;++i)assert(ops.bursts[i].amount==10&&ops.bursts[i].large);
    assert(chunk.step(random,ops)&&ops.bursts.size()==20&&ops.masks==1);
}
// The Q16 particles (W3.20) against an independent double oracle: the
// respawn's draws, split and position within the Q16 error, and the
// create-time sums' published pixels and frames within one.
void fixedParticles(){
    using Fire=downwell::CompactTorchFireRecord;
    unsigned respawns=0,updates=0,pixelDifferences=0;
    for(unsigned seed=1;seed<=20000;++seed){
        downwell::GameMakerRandom random(seed),oracle(seed),pick(seed*7919u);
        Fire fire;
        const int x=int(pick.nextU32()%30000u)-15000,y=int(pick.nextU32()%30000u)-15000;
        fire.create(x,y);
        const bool doused=seed&1;
        const double spread=doused?4:2,low=doused?0.15:0.2,high=doused?0.3:0.4;
        auto& p=fire.part[seed%Fire::parts];
        assert(fire.respawn(p,random,Fixed16::fromInteger(int(spread)),doused?20132659:26843546,
                            doused?40265318:53687091));
        const double speed=oracle.random(0.3)+0.2,divisor=oracle.random(2.5)+2.5;
        const double h=(oracle.chooseIndexSmall(2)?1:-1)*(speed/divisor),v=speed-std::fabs(h);
        const double position=oracle.random(spread*2)-spread,rate=oracle.random(high-low)+low;
        assert(random.drawCount()==oracle.drawCount());
        const auto near=[](std::int32_t q,double d,double units){return std::fabs(q/65536.-d)<=units/65536.;};
        assert(p.life==0&&near(p.vx,h,2)&&near(p.vy,-v,2)&&std::fabs(p.rate/134217728.-rate)<=1.5/134217728.);
        assert(near(p.x,x+position,1)&&near(p.y,y+position,1));
        ++respawns;
    }
    for(unsigned seed=1;seed<=2000;++seed){
        downwell::GameMakerRandom random(seed),pick(seed*31u);Ops ops;
        Fire fire;const int x=int(pick.nextU32()%30000u)-15000,y=int(pick.nextU32()%30000u)-15000;
        fire.create(x,y);
        // Every particle stays below life 8 for the first 15 updates: no draws.
        for(unsigned t=1;t<=15;++t){
            assert(fire.step(random,ops)&&random.drawCount()==0);
            for(unsigned i=0;i<Fire::parts;++i){
                const double life=i/2.+0.2*t,px=x+0.3*t,py=y-0.2*t;
                std::int32_t qx,qy,frame;bool below;
                const auto& p=fire.part[i];
                assert(p.pixels(qx,qy)&&p.lifeBelow8(below)&&below&&p.frame(frame));
                // Only a sum landing on k + 1/2 may round the other way:
                // the VM's tie depends on its binary64 sum's parity.
                const auto tie=[](double v){return std::fabs(v-std::floor(v)-0.5)<1e-9;};
                const int ex=int(std::nearbyint(px)),ey=int(std::nearbyint(py));
                assert(qx==ex||(tie(px)&&std::abs(qx-ex)==1));
                assert(qy==ey||(tie(py)&&std::abs(qy-ey)==1));
                assert(frame==int(std::floor(life))||(std::fabs(life-std::nearbyint(life))<1e-9&&std::abs(frame-int(std::floor(life)))<=1));
                pixelDifferences+=(qx!=ex)+(qy!=ey);
                ++updates;
            }
        }
    }
    std::printf("Torch fire Q16 particles: %u respawns within the Q16 error; %u particle-updates, "
                "%u of %u published pixels differ by one, each on a k + 1/2 tie\n",respawns,updates,pixelDifferences,2*updates);
}
}
int main(){
    fixedParticles();
    dousedFire(false);dousedFire(true);offscreenFireDrawsNothing();torchPlacement();gemChunk();
    std::puts("compact scenery events passed");
}
