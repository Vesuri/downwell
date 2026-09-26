#include "platform/amiga/AmigaGameplayDrawList.h"
#include "core/ShopFixtures.h"
#include "core/GameMakerRounding.h"
#include <cassert>
#include <cstdio>
#include <vector>
static bool boundsOK=true;
int amigaRendererAssetBounds(const UBYTE*,const UBYTE*,UWORD,UWORD,int x,int y,int,
    int* left,int* top,int* width,int* height) {
    *left=x-8; *top=y-8; *width=*height=16; return boundsOK;
}
int main() {
    {
        GameplayWorld scene(0,true,0,false);UBYTE pack[8]{};AmigaGameplayDrawList list;
        auto n=[](int v){return downwell::MotionBinary64::fromInteger(v);};
        assert(scene.emitCorpseExpiry(n(240),n(120)));
        assert(scene.emitDamageNumber(n(240),n(124),n(12),{}));
        assert(list.collect(scene,pack,pack+8,160,34));
        unsigned text=0,expiry=0;
        for(unsigned i=0;i<list.count;++i){
            auto a=list.actors[i];
            if(a.sprite==gameplayDamageTextMarker){assert(!expiry&&a.x==190&&a.y==86&&a.flip==12);++text;}
            if(a.sprite==102){assert(text==1);++expiry;}
        }
        assert(text==1&&expiry==1);
        assert(scene.emitDamageNumber(n(240),n(124),{0x3fe00000u,0},{}));
        assert(!list.collect(scene,pack,pack+8,160,34)&&list.failedSprite==gameplayDamageTextMarker);
    }
    GameplayWorld world(300,true,0,true);
    assert(world.grantUpgrade(12));
    world.registerShop(240,210);
    assert(world.configureEnemy(0,112,169,40,0,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
    UBYTE pack[8]{};
    AmigaGameplayDrawList list;
    unsigned observed=0;
    [[maybe_unused]] unsigned flashDraws=0;
    for(unsigned tick=0;tick<600;++tick) {
        world.step((tick%13==0 ? 4 : 0) | (tick%80<40 ? 1 : 2));
        const int vx=160+world.cameraOffsetX(), vy=34+world.cameraOffsetY();
        if(tick%61==0) assert(world.emitFlashBurst(downwell::MotionBinary64::fromInteger(vx+40),
                                                  downwell::MotionBinary64::fromInteger(vy+60)));
        if(tick%61==0)assert(world.emitCorpseExpiry(downwell::MotionBinary64::fromInteger(vx+50),
                                                  downwell::MotionBinary64::fromInteger(vy+80)));
        const auto checkpoint=world.checkpoint();
        assert(list.collect(world,pack,pack+8,vx,vy));
        assert(world.checkpoint()==checkpoint);
        std::vector<GameplayBob> expected;
        auto emit=[&](int sprite,unsigned frame,int x,int y,int flip=0) {
            if(x+8>110 && x-8<270 && y+8>0 && y-8<284)
                expected.push_back({UWORD(sprite),UWORD(frame),x,y,flip});
        };
#ifndef DOWNWELL_REPLAY_VERIFY
        for(unsigned i=0;i<world.enemyCount();++i) {
            const auto& e=world.enemyAt(i);
            if(e.active && e.awake) emit(e.spriteId,e.drawFrame(),190+e.x-world.cameraOffsetX(),
                142+e.y-world.cameraOffsetY(),e.objectId!=119 && e.facing<0);
        }
#endif
        downwell::drawShopKeepers(world.shops(),[&](int s,unsigned f,int x,int y) { emit(s,f,110+x-vx,y-vy); });
        downwell::drawShopPrompts(world.shops(),false,[&](int s,unsigned f,int x,int y) { emit(s,f,110+x-vx,y-vy); });
#ifndef DOWNWELL_REPLAY_VERIFY
        for(unsigned i=0;i<world.gemSlotEnd();++i) {
            const auto& g=world.gemAt(i);
            if(g.active && g.motion.lifetime.visible()) emit(g.large?134:133,g.motion.animation.frame(),
                190+g.x-world.cameraOffsetX(),142+g.y-world.cameraOffsetY());
        }
#endif
        for(unsigned i=0;i<world.bulletSlotEnd();++i) {
            const auto& b=world.bulletAt(i);
            if(b.active) emit(downwell::machineGunProfile(b.gemHigh).sprite,768+b.animation.frame(),
                190+b.x-world.cameraOffsetX(),142+b.y-world.cameraOffsetY());
        }
        assert(world.corpseExpiryEffects().visitNewest([&](unsigned,const downwell::CorpseExpiryEffect& effect){
            std::int32_t x,y;assert(effect.x.promoted().roundPixel(x)&&effect.y.promoted().roundPixel(y));
            emit(102,effect.drawFrame(),110+x-vx,y-vy);
        }));
        if(world.playerVisible()) emit(world.player().spriteId(world.deathSequence().movementDead()),
            world.player().spriteFrame(world.deathSequence().movementDead(),world.deathSequence().animation),
            190+downwell::roundGameMakerQ8(world.player().offsetXFixed())-world.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
            200+world.player().offsetY(),
#else
            142+downwell::roundGameMakerQ8(world.player().offsetYFixed())-world.cameraOffsetY(),
#endif
            world.player().facing()<0);
        assert(list.rayVisible==!world.goalStopped());
        if(list.rayVisible) expected.push_back({gameplayRayMarker,0,0,0,0});
#ifndef DOWNWELL_REPLAY_VERIFY
        world.flashEffects().visitNewest([&](const downwell::FlashBallEffect& effect){
            emit(96,effect.drawFrame(),110+effect.pixelX()-vx,effect.pixelY()-vy);++flashDraws;
        });
#endif
        assert(list.count==expected.size());
        int previousDepth=20000;
        for(unsigned i=0;i<list.count;++i){
            const unsigned sprite=list.actors[i].sprite;
            int depth;
            if(sprite==133||sprite==134)depth=10000;
            else if(sprite==458||sprite==459)depth=0;
            else if(sprite==102)depth=-1000;
            else if(sprite==unsigned(world.player().spriteId(world.deathSequence().movementDead())))depth=-50000;
            else if(sprite==96)depth=-200000;
            else continue;
            assert(depth<=previousDepth);previousDepth=depth;
        }
        for(unsigned i=0;i<list.count;++i) {
            const auto& a=list.actors[i]; const auto& b=expected[i];
            assert(a.sprite==b.sprite && a.frame==b.frame && a.x==b.x && a.y==b.y && a.flip==b.flip);
        }
        observed+=list.count;
        assert(list.collect(world,pack,pack+8,vx,vy,true));
        unsigned cursor=0;
        for(const auto& b:expected) {
            if(b.sprite==133 || b.sprite==134)continue;
            assert(cursor<list.count);
            const auto& a=list.actors[cursor++];
            assert(a.sprite==b.sprite && a.frame==b.frame && a.x==b.x && a.y==b.y && a.flip==b.flip);
        }
        assert(cursor==list.count && world.checkpoint()==checkpoint);
    }
#ifndef DOWNWELL_REPLAY_VERIFY
    assert(flashDraws>100);
#endif
    GameplayWorld initial(0);
    boundsOK=false;
    assert(!list.collect(initial,pack,pack+8,160,34) && list.failedSprite!=0);
    boundsOK=true;
    assert(list.collect(initial,pack,pack+8,160,34));
    assert(list.failedSprite==0 && list.failedFrame==0);
    assert(observed>600);
    std::printf("Gameplay collection: 600 ordered world projections, %u commands, no simulation mutation\n",observed);
}
