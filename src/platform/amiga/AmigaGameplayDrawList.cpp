#include "AmigaGameplayDrawList.h"
#include "AmigaProfile.h"
#include "core/ShopFixtures.h"
#include "core/GameMakerRounding.h"

bool AmigaGameplayDrawList::collect(const GameplayWorld& port, const UBYTE* pack,
    const UBYTE* packEnd, int worldViewX, int worldViewY, bool compositeCurrency) {
    count = 0; failedSprite = failedFrame = 0;
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
    for (unsigned index = 0; index < port.enemyCount(); ++index) {
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
    if (port.playerVisible() && !append(
            (UWORD)port.player().spriteId(port.deathSequence().movementDead()),
            (UWORD)port.player().spriteFrame(port.deathSequence().movementDead(), port.deathSequence().animation),
            downwellPlayfieldX + 80 + downwell::roundGameMakerQ8(port.player().offsetXFixed()) - port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
            replayGroundY + port.player().offsetY(),
#else
            playerStartY + downwell::roundGameMakerQ8(port.player().offsetYFixed()) - port.cameraOffsetY(),
#endif
            port.player().facing() < 0)) ready = false;
    rayLine = {};
    rayVisible = port.upgrades().levels[12] && !port.goalStopped();
    if (rayVisible) {
        DW_PROFILE_SCOPE(profileLaserTrace);
        using Ray = downwell::LaserSightRay;
        rayLine = downwell::LaserSightLine::from(
            Ray::fromQ8(port.player().offsetXFixed(), 240),
            Ray::fromQ8(port.player().offsetYFixed(), 176), port.laserSightEndpoint(),
            worldViewX - downwellPlayfieldX, worldViewY);
        if (count == capacity) { failedSprite = gameplayRayMarker; ready = false; }
        else actors[count++] = {gameplayRayMarker, 0, 0, 0, 0};
    }
    for (unsigned index = 0; index < port.bulletSlotEnd(); ++index) {
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
#ifndef DOWNWELL_REPLAY_VERIFY
    for (unsigned index = 0; !compositeCurrency && index < port.gemSlotEnd(); ++index) {
        const auto& gem = port.gemAt(index);
        if (gem.active && gem.motion.lifetime.visible() && !append(
                (UWORD)(gem.large ? 134 : 133), (UWORD)gem.motion.animation.frame(),
                downwellPlayfieldX + 80 + gem.x - port.cameraOffsetX(),
                playerStartY + gem.y - port.cameraOffsetY(), 0))
            ready = false;
    }
#endif

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
