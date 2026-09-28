#include "AmigaGameplayDrawList.h"
#include "AmigaProfile.h"
#include "core/ShopFixtures.h"
#include "core/GameMakerRounding.h"
#include "core/DamageNumberText.h"
#include "core/RunnerSpatialSplit.h"
#include "core/DebrisDraw.h"

bool AmigaGameplayDrawList::collect(const GameplayWorld& port, const UBYTE* pack,
    const UBYTE* packEnd, int worldViewX, int worldViewY, bool compositeCurrency) {
    count = 0; failedSprite = failedFrame = 0;
    rayLine = {};rayVisible=false;
#ifdef DOWNWELL_REPLAY_VERIFY
    (void)compositeCurrency;
#endif
    bool ready = true;
    auto append = [&](UWORD sprite, UWORD frame, int x, int y, int flip) {
        // Offline gameplay actors have <=64-pixel canvases and origins inside
        // them. Avoid Chip-directory reads for actors far outside the viewport.
        if (x < downwellPlayfieldX - 64 || x > downwellPlayfieldX + downwellPlayfieldWidth + 64 ||
            y < -64 || y > downwellDisplayHeight + 64) return true;
        int left, top, width, height;
        if (!amigaRendererAssetBounds(pack, packEnd,
                sprite, frame, x, y, flip, &left, &top, &width, &height)) {
            failedSprite = sprite;
            failedFrame = frame;
            return false;
        }
        if (left < downwellPlayfieldX + downwellPlayfieldWidth &&
            left + width > downwellPlayfieldX && top < downwellDisplayHeight && top + height > 0)
        {
            if (count == capacity) { failedSprite = sprite; failedFrame = frame; return false; }
            actors[count++] = {sprite, frame, x, y, flip};
        }
        return true;
    };
#ifndef DOWNWELL_REPLAY_VERIFY
    for (unsigned index = 0; !port.hasRoomActorDraw() && index < port.enemyCount(); ++index) {
        const auto& enemy = port.enemyAt(index);
        if (!enemy.active || !enemy.awake) continue;
        if (!append(
                (UWORD)enemy.spriteId, (UWORD)enemy.drawFrame(),
                downwellPlayfieldX + 80 + enemy.x - port.cameraOffsetX(),
                playerStartY + enemy.y - port.cameraOffsetY(),
                enemy.objectId != 119 && enemy.facing < 0)) ready = false;
    }
#endif
    downwell::drawShopKeepers(port.shops(), [&](int sprite, unsigned frame, int x, int y) {
        if (!append( sprite, frame,
            downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) ready = false;
    });
    downwell::drawShopPrompts(port.shops(), false, [&](int sprite, unsigned frame, int x, int y) {
        if (!append( sprite, frame,
            downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) ready = false;
    });
#ifndef DOWNWELL_REPLAY_VERIFY
    for (unsigned index = 0; !compositeCurrency && index < port.gemSlotEnd(); ++index) {
        // Direct Draw depth 10000 precedes bullet 0 and player -50000.
        const auto& gem = port.gemAt(index);
        if (gem.active && gem.motion.lifetime.visible() && !append(
                (UWORD)(gem.large ? 134 : 133), (UWORD)gem.motion.animation.frame(),
                downwellPlayfieldX + 80 + gem.x - port.cameraOffsetX(),
                playerStartY + gem.y - port.cameraOffsetY(), 0))
            ready = false;
    }
#endif

    bool foregroundDrawn=false;
    const auto drawForeground=[&](){
        if(foregroundDrawn)return true;
        foregroundDrawn=true;
        return port.visitForegroundEffects([&](const downwell::CorpseExpiryEffect& effect){
            std::int32_t x,y;
            if(!effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)){
                failedSprite=effect.spriteId;return false;
            }
            return append(effect.spriteId,effect.drawFrame(),downwellPlayfieldX+x-worldViewX,y-worldViewY,0);
        },[&](const downwell::PlayerDamageNumber& effect){
            downwell::DamageNumberText label;std::int32_t x,y;
            auto fail=[&](){failedSprite=gameplayDamageTextMarker;return false;};
            if(!label.format(effect.amount)||!effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y))return fail();
            const std::int64_t sx=std::int64_t(downwellPlayfieldX)+x-worldViewX,sy=std::int64_t(y)-worldViewY;
            const int half=int(label.length)*4;
            if(sx+half+1<=downwellPlayfieldX||sx-half-1>=downwellPlayfieldX+downwellPlayfieldWidth||sy+11<=0||sy-1>=downwellDisplayHeight)return true;
            for(unsigned i=0;i<label.length;++i){
                int left,top,width,height;
                if(!amigaRendererAssetBounds(pack,packEnd,label.glyph(label.text[i]),0,0,0,0,&left,&top,&width,&height))return fail();
            }
            if(count==capacity)return fail();
            actors[count++]={gameplayDamageTextMarker,0,int(sx),int(sy),int(label.integerValue)};
            return true;
        });
    };
    const auto drawPlayer=[&](){
        if (port.playerVisible() && !append(
                (UWORD)port.player().spriteId(port.deathSequence().movementDead()),
                (UWORD)port.player().spriteFrame(port.deathSequence().movementDead(), port.deathSequence().animation),
                downwellPlayfieldX + 80 + downwell::roundGameMakerQ8(port.player().offsetXFixed()) - port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
                replayGroundY + port.player().offsetY(),
#else
                playerStartY + downwell::roundGameMakerQ8(port.player().offsetYFixed()) - port.cameraOffsetY(),
#endif
                port.player().facing() < 0)) return false;
        rayVisible = port.upgrades().levels[12] && !port.goalStopped();
        if (rayVisible) {
            DW_PROFILE_SCOPE(profileLaserTrace);
            using Ray = downwell::LaserSightRay;
            rayLine = downwell::LaserSightLine::from(
                Ray::fromQ8(port.player().offsetXFixed(), 240),
                Ray::fromQ8(port.player().offsetYFixed(), 176), port.laserSightEndpoint(),
                worldViewX - downwellPlayfieldX, worldViewY);
            if (count == capacity) { failedSprite = gameplayRayMarker; return false; }
            actors[count++] = {gameplayRayMarker, 0, 0, 0, 0};
        }
        return true;
    };
    if(port.hasRoomActorDraw()) {
        downwell::RunnerImageValue foregroundDepth;foregroundDepth.assign(downwell::MotionBinary64::fromInteger(-1000));
        if(!port.visitRoomActors([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){
            if(downwell::RunnerSpatialSplit::less(depth,foregroundDepth)&&!drawForeground())return false;
            if(kind==GameplayWorld::DrawActor::player)return port.findPlayer(key)&&drawPlayer();
            if(kind==GameplayWorld::DrawActor::enemy){
#ifndef DOWNWELL_REPLAY_VERIFY
                const auto* enemy=port.resolveEnemyLifetime(key);
                if(!enemy)return false;
                return !enemy->active||!enemy->awake||append(enemy->spriteId,enemy->drawFrame(),
                    downwellPlayfieldX+80+enemy->x-port.cameraOffsetX(),
                    playerStartY+enemy->y-port.cameraOffsetY(),enemy->objectId!=119&&enemy->facing<0);
#else
                return true;
#endif
            }
            if(kind==GameplayWorld::DrawActor::bullet){
                const auto* bullet=port.resolveBulletLifetime(key);
                return bullet&&(!bullet->active||append(downwell::machineGunProfile(bullet->gemHigh).sprite,
                    768+bullet->animation.frame(),downwellPlayfieldX+80+bullet->x-port.cameraOffsetX(),
                    playerStartY+bullet->y-port.cameraOffsetY(),0));
            }
            if(kind==GameplayWorld::DrawActor::debris){
                if(compositeCurrency)return true; // Published by surfaceFx, not a second world draw.
                const auto* record=port.debrisInstances()?port.debrisInstances()->find(key):nullptr;
                downwell::DebrisDraw::Command command;unsigned frame;std::int32_t x,y;
                using Real=downwell::MotionBinary64;
                const downwell::EffectSurfacePlacement::View view{};
                if(!record||!record->effect||!downwell::DebrisDraw::prepare(*record->effect,view,true,false,false,command))return false;
                if(command.placement.target==downwell::EffectSurfacePlacement::hidden)return true;
                failedSprite=command.sprite;
                if(!downwell::DebrisDraw::frame(command.sprite,command.imageIndex,frame)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.x,Real::fromInteger(worldViewX),command.placement.x)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.y,Real::fromInteger(worldViewY),command.placement.y)||
                   !command.placement.x.roundPixel(x)||!command.placement.y.roundPixel(y))return false;
                failedSprite=0;
                return append(command.sprite,frame,downwellPlayfieldX+x,y,0);
            }
            const auto* record=port.scriptSpawnInstances()?port.scriptSpawnInstances()->find(key):nullptr;
            if(!record||!record->effect)return false;
            const auto& effect=*record->effect;
            failedSprite=effect.spriteId;failedFrame=effect.drawFrame();
            std::int32_t x,y;downwell::RunnerImageValue ix,iy;
            // The wall impact uses eight unrotated frames. The offline muzzle
            // proof covers integer anchors and either no rotation or 267..273
            // degrees. Reject other transforms explicitly;
            // no runtime conversion or silently rounded fractional artwork.
            const bool wallImpact=effect.spriteId==101;
            if(!effect.valid||(!wallImpact&&effect.spriteId!=602&&effect.spriteId!=603)||
               effect.drawFrame()>=(wallImpact?8u:1u)||
               !effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)||
               !ix.assign(downwell::MotionBinary64::fromInteger(x))||
               !iy.assign(downwell::MotionBinary64::fromInteger(y))||
               ((ix.bits!=effect.x.bits)&&((ix.bits|effect.x.bits)&0x7fffffffu))||
               ((iy.bits!=effect.y.bits)&&((iy.bits|effect.y.bits)&0x7fffffffu)))return false;
            unsigned frame=effect.drawFrame();
            if(effect.angle.bits&0x7fffffffu){
                if(wallImpact)return false;
                downwell::RunnerImageValue lo,hi;
                lo.assign(downwell::MotionBinary64::fromInteger(267));
                hi.assign(downwell::MotionBinary64::fromInteger(273));
                if(effect.angle.bits<lo.bits||effect.angle.bits>hi.bits)return false;
                frame=768;
            }
            failedSprite=failedFrame=0;
            return append(effect.spriteId,frame,downwellPlayfieldX+x-worldViewX,y-worldViewY,0);
        }))return false;
    }
    for (unsigned index = 0; !port.hasRoomActorDraw() && index < port.bulletSlotEnd(); ++index) {
        const auto& bullet = port.bulletAt(index);
        // The pack generator checks every possible nearest-pixel
        // sample over 267..273 degrees for all twelve source frames:
        // the spread-adjusted artwork equals this offline quarter
        // turn. Other angles/guns need separately verified variants.
        if (bullet.active && !append(
                downwell::machineGunProfile(bullet.gemHigh).sprite, 768 + bullet.animation.frame(),
                downwellPlayfieldX + 80 + bullet.x - port.cameraOffsetX(),
                playerStartY + bullet.y - port.cameraOffsetY(), 0))
            ready = false;
    }
    if(!drawForeground())ready=false;
    if(!port.hasRoomActorDraw()&&!drawPlayer())ready=false;

    // Original compatibility depth -200000: ahead of ordinary gameplay,
    // but behind the controller's effect-surface publication (-300000).
#ifndef DOWNWELL_REPLAY_VERIFY
    port.flashEffects().visitNewest([&](const downwell::FlashBallEffect& effect) {
        if(!append(96,effect.drawFrame(),downwellPlayfieldX+effect.pixelX()-worldViewX,
                   effect.pixelY()-worldViewY,0)) ready=false;
    });
#endif
    return ready;
}
