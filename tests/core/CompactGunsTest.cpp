#include "core/CompactGuns.h"
#include "core/GameMakerRandom.h"
#include "core/GunTable.h"
#include "core/RangedShot.h"
#include <cassert>
#include <cstdio>
// Event-level checks for W1.4's compact gun records and the ranged shot's
// 32-bit arithmetic. The slice route (AmigaSceneRuntimeTest) collects a
// module and fires the laser; these cover timings the route only samples.
using namespace downwell;
namespace {
void rangedShot(){
    assert(RangedShot::degreesToTurns(270)==0xc0000000u);
    assert(RangedShot::degreesToTurns(90)==0x40000000u&&RangedShot::degreesToTurns(0)==0);
    assert(RangedShot::degreesToTurns(-90)==0xc0000000u&&RangedShot::degreesToTurns(630)==0xc0000000u);
    // Full-scale components: |sin| 1 (Q30) times 8 px (Q20) is 8 px.
    assert(RangedShot::component(1u<<30,8u<<20,false)==(8<<20));
    assert(RangedShot::component(1u<<30,8u<<20,true)==-(8<<20));
    for(int gun=0;gun<gunCount;++gun)for(int high=0;high<2;++high){
        const auto& stats=gunStats(gun,high);
        if(stats.object!=261)continue;
        GameMakerRandom random(1000+unsigned(gun)*2+unsigned(high));
        RangedShot shot;
        const auto before=random.drawCount();
        assert(shot.create(random,10,20,270,stats,false));
        assert(random.drawCount()==before+2); // imgSp, then bSpeed
        assert(shot.speed>=std::uint32_t(stats.speed-stats.rangeRandom)<<20&&shot.speed<=std::uint32_t(stats.speed)<<20);
        assert(shot.decelerateAlarm==stats.rangeTimer);
        shot.prepareStep(random);
        assert(random.drawCount()==before+3&&shot.allSet); // first-Step spread
        // 270 degrees points down (GameMaker y grows downwards): gunboots.
        assert(shot.velocityY>0);
        const std::int64_t spread=std::int64_t(std::int32_t(shot.direction-0xc0000000u));
        assert(spread*720<=(std::int64_t(stats.accuracyHalves)<<32)+720&&
               -spread*720<=(std::int64_t(stats.accuracyHalves)<<32)+720);
        // Alarm 0 starts the 0.8 deceleration; below 3 px the animation starts.
        for(int i=0;i<stats.rangeTimer;++i)shot.alarm();
        assert(shot.decelerate);
        unsigned steps=0;
        while(!shot.animation&&steps<64){shot.prepareStep(random);shot.advance();++steps;}
        assert(shot.animation&&shot.speed<(3u<<20)&&random.drawCount()==before+3);
        bool ended=false;unsigned frames=0;
        while(!ended&&frames<64){assert(shot.advanceAnimation(6,ended));++frames;}
        assert(ended&&frames>=10&&frames<=15); // 6 frames at imgSp 0.4..0.6
    }
    // Laser Sight's myBullet keeps its direction exactly.
    GameMakerRandom random(5);RangedShot shot;
    assert(shot.create(random,0,0,270,gunStats(gunShotgun,false),true));
    shot.zeroSpread=true;shot.prepareStep(random);
    assert(shot.direction==0xc0000000u&&shot.decelerateAlarm==4+4/3);
}
void casing(){
    GameMakerRandom random(42);
    CompactCasingRecord c;c.live=true;
    const auto before=random.drawCount();
    c.create(random,100,200,1);
    assert(random.drawCount()==before+3);
    assert(c.xsp>=-(3L<<16)&&c.xsp<=-(1L<<16)&&c.ysp>=-(3L<<16)&&c.ysp<=0);
    CompactCasingRecord mirrored;mirrored.live=true;
    GameMakerRandom same(42);mirrored.create(same,100,200,-1);
    assert(mirrored.xsp==-c.xsp&&mirrored.ysp==c.ysp);
    // Alarm 0 at 15 starts flashing and sets Alarm 1 = 15, which the same
    // index-major pass already counts down: destroyed on the 29th Step.
    unsigned alarms=0;
    while(c.live&&alarms<64){c.alarmEvent(0);c.alarmEvent(1);++alarms;if(alarms<=15)assert(c.visible());}
    assert(!c.live&&alarms==29&&c.flashing);
}
void module(){
    CompactGunModuleRecord m;m.live=true;m.type=0;m.gun=gunLaser;
    assert(m.sprite()==139&&m.letter()==gunTable[gunLaser][0].letter);
    unsigned steps=0;
    while(!m.obtainable){for(unsigned i=0;i<4;++i)m.alarmEvent(i);++steps;}
    assert(steps==10&&m.image==1); // Alarm 3 also fired at 10
    for(unsigned i=0;i<5;++i)for(unsigned k=0;k<4;++k)m.alarmEvent(k);
    assert(m.image==0);
    for(unsigned i=0;i<60;++i)for(unsigned k=0;k<4;++k)m.alarmEvent(k);
    assert(m.image==1);
    m.type=1;assert(m.sprite()==140);
}
void texts(){
    CompactModuleTextRecord t;t.live=true;t.gun=gunLaser;t.type=0;t.amount=2;
    t.step();
    assert(t.allSet&&t.length==5&&t.shown==1&&t.character(1)=='L'&&t.character(5)=='R');
    unsigned step=0,notice=0;
    while(t.live&&step<1000){
        for(unsigned i=0;i<4;++i)t.alarmEvent(i);
        t.step();
        if(t.drawEvent()){assert(!notice);notice=step;}
        ++step;
    }
    // After the first Step's Alarm 0 = 5: letters every five Steps (2..5 at
    // 4, 9, 14, 19), the last one's frame reaches 8 by Alarm 3 every two
    // Steps (itemNotif at 33); Alarm 1 = 120, then a letter leaves every four.
    assert(notice==33&&!t.live&&step==163);
    CompactModuleTextRecord wide;wide.x=20;wide.gun=gunMachine;wide.step();
    assert(wide.left(0,160)==10&&wide.width()==80);
    CompactItemNoticeRecord n;n.live=true;n.amount=2;
    const long start=n.y;unsigned life=0;
    while(n.live&&life<200){n.alarmEvent(0);n.alarmEvent(1);if(n.live)n.step();++life;}
    assert(life==90&&n.y<start-(10L<<16)&&n.flashing);
    assert(n.left(0,160)==22&&CompactModuleTextRecord::bounce(0)==10&&CompactModuleTextRecord::bounce(8)==0);
}
}
int main(){
    rangedShot();casing();module();texts();
    std::puts("Compact guns: ranged shot arithmetic, casing, module, moduleText and itemNotif timings passed");
    return 0;
}
