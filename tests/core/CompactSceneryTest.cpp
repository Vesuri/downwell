#include "core/CompactScenery.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
// Event-level checks for the compact world-one scenery owners. The slice
// route (AmigaSceneRuntimeTest) opens a gem chunk, lights torches and, since
// W1.5, douses side-room TorchFires; this covers each event in isolation.
using downwell::MotionBinary64;
namespace {
struct Ops {
    struct Moving {int sprite;MotionBinary64 rate,angle,speed;};
    std::vector<Moving> moving;std::vector<int> sounds;
    int coins=0,attracted=0,flashes=0,appearance=0,masks=0,destroyed=0,fires=0;
    struct Burst {int xRange,xLow,yRange,yLow,amount;bool large;};
    std::vector<Burst> bursts;
    bool visible=true,solidAt=false,solidBelow=false,wallRight=false,wallLeft=false;
    bool inView(int,int,bool& v){v=visible;return true;}
    bool outOfView(int,bool& retained){retained=true;return true;}
    bool sound(int id,int,int,int){sounds.push_back(id);return true;}
    bool movingFx(MotionBinary64,MotionBinary64,int sprite,MotionBinary64 rate,MotionBinary64 angle,MotionBinary64 speed){
        moving.push_back({sprite,rate,angle,speed});return true;}
    bool currency(int amount){coins+=amount;return true;}
    bool attractGem(){++attracted;return true;}
    bool flash(MotionBinary64,MotionBinary64){++flashes;return true;}
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
    assert(ops.moving[0].speed.high==0x3ff00000u&&ops.moving[0].rate.high==0x3fe00000u);
    int order;
    assert(downwell::compareRunnerReal(ops.moving[1].angle,MotionBinary64::fromInteger(160),order)&&order>0);
    assert(downwell::compareRunnerReal(ops.moving[1].angle,MotionBinary64::fromInteger(180),order)&&order<=0);
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
// The Q32 particle sums against the binary64 sums they certify: a twin
// record with every particle held on the binary64 path (the original
// arithmetic) must draw, respawn and consume the RNG identically.
void fixedParticles(){
    using Fire=downwell::CompactTorchFireRecord;
    unsigned decisions=0,fixedParts=0;
    auto hold=[](Fire& f){for(auto& p:f.part)if(!p.exact&&p.steps==0)p.exact=true;};
    for(unsigned seed=1;seed<=3000;++seed){
        downwell::GameMakerRandom ra(seed),rb(seed),pick(seed*7919u);Ops oa,ob;
        Fire a,b;
        const int x=int(pick.nextU32()%60000u)-10000,y=int(pick.nextU32()%300000u)-20000;
        a.create(x,y);b.create(x,y);hold(b);
        const unsigned douse=seed%3?200+pick.nextU32()%400:1000;
        for(unsigned t=0;t<700;++t){
            if(t==douse){a.objHp=b.objHp=0;a.stomped=b.stomped=seed&1;}
            const bool sa=a.step(ra,oa),sb=b.step(rb,ob);hold(b);
            assert(sa&&sb&&ra.drawCount()==rb.drawCount()&&oa.moving.size()==ob.moving.size()&&
                   oa.coins==ob.coins&&oa.sounds==ob.sounds&&a.active==b.active);
            for(unsigned i=0;i<Fire::parts;++i){
                const auto& pa=a.part[i];const auto& pb=b.part[i];
                fixedParts+=!pa.exact;
                std::int32_t xa,ya,xb,yb,fa=0,fb=0;bool ba,bb;
                assert(pa.pixels(xa,ya)&&pb.pixels(xb,yb)&&pa.lifeBelow8(ba)&&pb.lifeBelow8(bb));
                if(ba)assert(pa.frame(fa));
                if(bb)assert(pb.frame(fb));
                if(xa!=xb||ya!=yb||ba!=bb||fa!=fb){
                    std::printf("particle %u differs at seed %u update %u: %d,%d %d %d vs %d,%d %d %d\n",
                        i,seed,t,xa,ya,int(ba),fa,xb,yb,int(bb),fb);
                    assert(false);
                }
                ++decisions;
            }
        }
    }
    std::printf("Torch fire Q32 particles: %u particle-updates match binary64 (%u on Q32 sums)\n",decisions,fixedParts);
}
}
int main(){
    fixedParticles();
    dousedFire(false);dousedFire(true);offscreenFireDrawsNothing();torchPlacement();gemChunk();
    std::puts("compact scenery events passed");
}
