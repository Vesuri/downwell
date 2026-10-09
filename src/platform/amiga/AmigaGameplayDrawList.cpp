#include "AmigaGameplayDrawList.h"
#include "AmigaProfile.h"
#include "core/ShopFixtures.h"
#include "core/GameMakerRounding.h"
#include "core/DamageNumberText.h"
#include "core/RunnerSpatialSplit.h"
#include "core/DebrisDraw.h"
#include "core/TimeShardDraw.h"
#include "core/PackedMaskIntersection.h"
#if defined(DOWNWELL_BOB_VERIFY) || defined(DOWNWELL_DRAW_ORDER_AUDIT)
#include "core/ActorDrawOrderVerification.h"
extern "C" { volatile unsigned gDownwellActorOrderChecks,gDownwellActorOrderFailures; }
#endif

namespace {
bool registeredEnemyShadePresent(const GameplayWorld& world,int viewX,int viewY,bool& present){
    present=false;
    if(world.shops().noControl)return true;
    using Real=downwell::MotionBinary64;
    const downwell::EffectSurfacePlacement::View view{Real::fromInteger(viewX),Real::fromInteger(viewY),
        Real::fromInteger(160),Real::fromInteger(284)};
    return world.visitRegisteredEnemyShades([&](Real x,Real y){
        downwell::EffectSurfacePlacement::Draw shade;
        if(!downwell::EffectSurfacePlacement::enemyShade(x,y,view,false,false,true,shade))return false;
        present|=shade.target==downwell::EffectSurfacePlacement::shade;return true;
    });
}
}

bool AmigaGameplayDrawList::promoteCurrencyComplete(const GameplayWorld& world,int viewX,int viewY,
    const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames){
    using Placement=downwell::EffectSurfacePlacement;
    if(entryCoverVisible||world.dead()||!assets.ready()||
        !Placement::smallPixel(viewX)||!Placement::smallPixel(viewY))return false;
    bool shade;
    if(!registeredEnemyShadePresent(world,viewX,viewY,shade)||shade)return false;
    bool other=false;
    world.smokeEffects().visitNewest([&](auto,const auto&){other=true;});
    if(world.registeredSmoke()&&world.registeredSmoke()->size())other=true;
    if(world.registeredCorpses()&&world.registeredCorpses()->size())other=true;
    if(world.timeShardInstances()&&world.timeShardInstances()->size())other=true;
    if(other)return false;
    if(!world.visitDebrisDraw([&](const auto&){other=true;return true;})||other)return false;
    const Placement::PixelView view{viewX,viewY,160,284};
    GameplayBob pending[8];unsigned used=0,channels=0;
    for(unsigned i=world.gemSlotEnd();i;--i){
        const auto& gem=world.gemAt(i-1);
        if(!gem.active||!gem.motion.lifetime.visible())continue;
        const int x=240+gem.x,y=176+gem.y;
        if(!Placement::smallPixel(x)||!Placement::smallPixel(y))return false;
        if(!Placement::pixelInView(x,y,view))continue;
        const unsigned sprite=gem.large?134:133,frame=gem.motion.animation.frame();
        const auto* art=assets.currency(gem.large,frame);
        const auto* prepared=frames.find(sprite,frame);
        if(!art||!prepared||!prepared->partCount[0]||
            prepared->partCount[0]>8-channels||used==8)return false;
        const int gx=x-viewX-art->originX,gy=y-viewY-art->originY;
        if(!world.shops().noControl)for(unsigned n=0;n<world.enemyCount();++n){
            const auto& enemy=world.enemyAt(n);
            if(!enemy.active||!enemy.awake)continue;
            const int ex=240+enemy.x,ey=176+enemy.y;
            if(!Placement::smallPixel(ex)||!Placement::smallPixel(ey))return false;
            if(!Placement::pixelInView(ex,ey,view))continue;
            const auto& shade=*assets.shade();
            const int sx=Placement::snapShadePixel(ex)-viewX-shade.originX;
            const int sy=Placement::snapShadePixel(ey)-viewY-shade.originY;
            int left=gx>sx?gx:sx,top=gy>sy?gy:sy;
            int right=gx+int(art->mask.width),bottom=gy+int(art->mask.height);
            if(right>sx+int(shade.mask.width))right=sx+int(shade.mask.width);
            if(bottom>sy+int(shade.mask.height))bottom=sy+int(shade.mask.height);
            if(left<0)left=0;if(top<0)top=0;
            if(right>160)right=160;if(bottom>284)bottom=284;
            for(int py=top;py<bottom;++py)for(int px=left;px<right;++px){
                const unsigned a=px-gx,b=px-sx;
                if((art->mask.data[(py-gy)*art->mask.stride+a/8]&(128u>>(a&7)))&&
                    (shade.mask.data[(py-sy)*shade.mask.stride+b/8]&(128u>>(b&7))))return false;
            }
        }
        pending[used++]={UWORD(sprite),UWORD(frame),downwellPlayfieldX+x-viewX,y-viewY,0,prepared};
        channels+=prepared->partCount[0];
    }
    if(!used||used>capacity-count)return false;
    // Controller Draw is after all room actors. Assignment walks this suffix
    // backwards, giving every promoted part a channel before ordinary actors.
    for(unsigned i=0;i<used;++i)actors[count++]=pending[i];
    return true;
}

bool AmigaGameplayDrawList::promoteCurrency(const GameplayWorld& world,int viewX,int viewY,
    const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames){
    using Placement=downwell::EffectSurfacePlacement;
    currencyRasterFirst=currencySpriteCount=0;currencyRasterRequired=true;
    if(count>=capacity||entryCoverVisible||world.dead()||!assets.ready()||
        !Placement::smallPixel(viewX)||!Placement::smallPixel(viewY))return false;
    // A bone's drawShade cuts the controller surface too. Until this family
    // participates in per-gem overlap admission, keep currency on the existing
    // shaded raster rather than promoting pixels which must be erased later.
    bool shade;
    if(!registeredEnemyShadePresent(world,viewX,viewY,shade)||shade)return false;
    bool other=false;
    world.smokeEffects().visitNewest([&](auto,const auto&){other=true;});
    if(world.registeredSmoke()&&world.registeredSmoke()->size())other=true;
    if(world.registeredCorpses()&&world.registeredCorpses()->size())other=true;
    if(world.timeShardInstances()&&world.timeShardInstances()->size())other=true;
    if(other||!world.visitDebrisDraw([&](const auto&){other=true;return true;})||other)return false;
    const Placement::PixelView view{viewX,viewY,160,284};
#if defined(DOWNWELL_REFERENCE_CURRENCY_BOBS) || defined(DOWNWELL_REFERENCE_SHADE_PARTITION)
    constexpr bool planarOverflow=false;
    GameplayBob pending[8];unsigned slots[8];
#else
    constexpr bool planarOverflow=true;
    auto& pending=currencyPending_;auto& slots=currencySlots_;
#endif
    constexpr unsigned pendingCapacity=planarOverflow?GameplayWorld::gemCapacity:8;
    unsigned used=0,channels=0,first=0,hardwareUsed=0;bool dropped=false;
#ifdef DOWNWELL_REFERENCE_SHADE_PARTITION
    constexpr bool topmostFirst=false;
#else
    constexpr bool topmostFirst=true;
#endif
    // Keep a source-topmost suffix. Shade subtraction is after all currency
    // painting, so every shaded gem and all candidates behind it remain in
    // the raster. Inspect the topmost end first and stop at its first shaded
    // gem: no query of the retained prefix is needed. Reverse the admitted
    // suffix on publication to restore source painter order.
    const unsigned end=world.gemSlotEnd();
    for(unsigned cursor=0;cursor<end;++cursor){
        const unsigned i=topmostFirst?cursor+1:end-cursor;
        const auto& gem=world.gemAt(i-1);
        if(!gem.active||!gem.motion.lifetime.visible())continue;
        const int x=240+gem.x,y=176+gem.y;
        if(!Placement::smallPixel(x)||!Placement::smallPixel(y))return false;
        if(!Placement::pixelInView(x,y,view))continue;
        const unsigned sprite=gem.large?134:133,frame=gem.motion.animation.frame();
        const auto* art=assets.currency(gem.large,frame);
        const auto* prepared=frames.find(sprite,frame);
        if(!art||!prepared||!prepared->partCount[0]||prepared->partCount[0]>8)return false;
        bool invalidShade=false;
        const auto shadeFree=[&](){
            if(world.shops().noControl)return true;
            const int gx=x-viewX-art->originX,gy=y-viewY-art->originY;
            for(unsigned n=0;n<world.enemyCount();++n){
                const auto& enemy=world.enemyAt(n);
                if(!enemy.active||!enemy.awake)continue;
                const int ex=240+enemy.x,ey=176+enemy.y;
                if(!Placement::smallPixel(ex)||!Placement::smallPixel(ey)){
                    invalidShade=true;return false;
                }
                if(!Placement::pixelInView(ex,ey,view))continue;
                const auto& shade=*assets.shade();
                const int sx=Placement::snapShadePixel(ex)-viewX-shade.originX;
                const int sy=Placement::snapShadePixel(ey)-viewY-shade.originY;
                int left=gx>sx?gx:sx,top=gy>sy?gy:sy;
                int right=gx+int(art->mask.width),bottom=gy+int(art->mask.height);
                if(right>sx+int(shade.mask.width))right=sx+int(shade.mask.width);
                if(bottom>sy+int(shade.mask.height))bottom=sy+int(shade.mask.height);
                if(left<0)left=0;if(top<0)top=0;
                if(right>160)right=160;if(bottom>284)bottom=284;
#ifndef DOWNWELL_REFERENCE_SHADE_PARTITION
                if(downwell::PackedMaskIntersection::intersects(art->mask,gx,gy,shade.mask,sx,sy,
                    left,top,right,bottom))return false;
#else
                for(int py=top;py<bottom;++py)for(int px=left;px<right;++px){
                    const unsigned a=px-gx,b=px-sx;
                    if((art->mask.data[(py-gy)*art->mask.stride+a/8]&(128u>>(a&7)))&&
                        (shade.mask.data[(py-sy)*shade.mask.stride+b/8]&(128u>>(b&7))))return false;
                }
#endif
            }
            return true;
        };
        if(!shadeFree()){
#ifdef DOWNWELL_REFERENCE_SHADE_PARTITION
            return false;
#else
            if(invalidShade)return false;
            // The ordinary compositor receives this entire prefix and
            // applies every shade before sprite publication.
            dropped=true;break;
#endif
        }
        if(topmostFirst&&(used==capacity-count||used==pendingCapacity||
            (!planarOverflow&&prepared->partCount[0]>8-channels))){
            dropped=true;break;
        }
        // Retire bottommost candidates, never a hole within the selected
        // painter suffix. Multipart assets consume their actual channel count.
        while(!planarOverflow&&used&&(used==capacity-count||prepared->partCount[0]>8-channels)){
            channels-=pending[first].prepared->partCount[0];
            first=(first+1)%8;--used;dropped=true;
        }
        const unsigned at=(first+used)%pendingCapacity;
        pending[at]={UWORD(sprite),UWORD(frame),downwellPlayfieldX+x-viewX,y-viewY,0,prepared};
        slots[at]=i-1;++used;
        if(!planarOverflow)channels+=prepared->partCount[0];
        else if(hardwareUsed==used-1&&prepared->partCount[0]<=8-channels){
            channels+=prepared->partCount[0];++hardwareUsed;
        }
    }
    // A wholly unshaded currency pass is ordinary masked source painting.
    // Keep it in the existing actor compositor: the topmost parts get DMA
    // channels and overflow becomes BOBs after every ordinary room actor.
    // If shade/capacity interrupts admission, only the hardware suffix may
    // escape the retained surface, which is published after all BOBs.
    if(planarOverflow&&dropped)used=hardwareUsed;
    if(!used)return false;
    currencyRasterFirst=dropped?slots[topmostFirst?used-1:first]+1:end;
    currencyRasterRequired=dropped;currencySpriteCount=used;
    // Keep source Draw order; assignment visits backwards, giving the topmost
    // producer the highest sprite priority above the retained controller.
    for(unsigned n=0;n<used;++n)
        actors[count++]=pending[topmostFirst?used-1-n:(first+n)%pendingCapacity];
    return true;
}

bool AmigaGameplayDrawList::entryCoverHardwareEligible(const GameplayWorld& world,int viewX,int viewY,bool& eligible)const{
    eligible=false;
    if(!entryCoverVisible)return true;
    bool shade;
    if(!registeredEnemyShadePresent(world,viewX,viewY,shade))return false;
    if(shade)return true;
    using Real=downwell::MotionBinary64;
    const downwell::EffectSurfacePlacement::View view{Real::fromInteger(viewX),Real::fromInteger(viewY),
        Real::fromInteger(160),Real::fromInteger(284)};
    if(!world.shops().noControl)for(unsigned i=0;i<world.enemyCount();++i){
        const auto& enemy=world.enemyAt(i);
        if(!enemy.active||!enemy.awake)continue;
        downwell::EffectSurfacePlacement::Draw shade;
        if(!downwell::EffectSurfacePlacement::enemyShade(Real::fromInteger(240+enemy.x),
            Real::fromInteger(176+enemy.y),view,false,false,true,shade))return false;
        if(shade.target==downwell::EffectSurfacePlacement::shade)return true;
    }
    eligible=true;return true;
}

bool AmigaGameplayDrawList::collect(const GameplayWorld& port, const UBYTE* pack,
    const UBYTE* packEnd, int worldViewX, int worldViewY, bool compositeCurrency,
    const AmigaActorFrames* prepared) {
    count = 0; failedSprite = failedFrame = 0;
    currencyRasterFirst=currencySpriteCount=0;currencyRasterRequired=true;
#if defined(DOWNWELL_BOB_VERIFY) || defined(DOWNWELL_DRAW_ORDER_AUDIT)
    static downwell::ActorDrawOrderVerification orderOracle;
    ++gDownwellActorOrderChecks;
    if(!orderOracle.verify(port)){++gDownwellActorOrderFailures;return false;}
#endif
    rayLine = {};rayVisible=false;
    entryCoverVisible=false;entryCover={};
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
        const auto* binding=prepared?prepared->find(sprite,frame):nullptr;
        if(binding)binding->bounds(x,y,flip,left,top,width,height);
        else if (prepared || !amigaRendererAssetBounds(pack, packEnd,
                sprite, frame, x, y, flip, &left, &top, &width, &height)) {
            failedSprite = sprite;
            failedFrame = frame;
            return false;
        }
        if (left < downwellPlayfieldX + downwellPlayfieldWidth &&
            left + width > downwellPlayfieldX && top < downwellDisplayHeight && top + height > 0)
        {
            if (count == capacity) { failedSprite = sprite; failedFrame = frame; return false; }
            actors[count++] = {sprite, frame, x, y, flip, binding};
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
        DW_PROFILE_SCOPE(profileActorLayerWalk);
        downwell::RunnerImageValue foregroundDepth;foregroundDepth.assign(downwell::MotionBinary64::fromInteger(-1000));
        if(!port.visitRoomActorsReadOnly([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){
            DW_PROFILE_SCOPE(profileActorPreparation);
            if(downwell::RunnerSpatialSplit::less(depth,foregroundDepth)&&!drawForeground())return false;
            if(kind==GameplayWorld::DrawActor::player)return port.findPlayer(key)&&drawPlayer();
            if(kind==GameplayWorld::DrawActor::ditherAbove){
                const auto* record=port.ditherAbove()?port.ditherAbove()->find(key):nullptr;
                std::int32_t x,y;
                if(!record||!record->x.roundPixel(x)||!record->y.roundPixel(y))return false;
                const std::int64_t sx=std::int64_t(x)-worldViewX+downwellPlayfieldX;
                const std::int64_t sy=std::int64_t(y)-worldViewY;
                if(sx+160<=downwellPlayfieldX||sx-160>=downwellPlayfieldX+downwellPlayfieldWidth||
                    sy+80<=0||sy-720>=downwellDisplayHeight)return true;
                // Entire finite source rectangle, clipped before narrowing.
                const int left=int(sx-160<downwellPlayfieldX?downwellPlayfieldX:sx-160);
                const int right=int(sx+160>downwellPlayfieldX+downwellPlayfieldWidth?
                    downwellPlayfieldX+downwellPlayfieldWidth:sx+160);
                const int top=int(sy-720<0?0:sy-720);
                const int bottom=int(sy>downwellDisplayHeight?downwellDisplayHeight:sy);
                if(bottom>top){
                    if(count==capacity)return false;
                    actors[count++]={gameplayBlackRectMarker,UWORD(right-left),left,top,bottom-top};
                }
                if(sy<downwellDisplayHeight&&sy+80>0){
                    int ax,ay,w,h;
                    if(!amigaRendererAssetBounds(pack,packEnd,gameplayDitherEdge,0,int(sx),int(sy),0,&ax,&ay,&w,&h)||
                        w!=320||h!=80||count==capacity){failedSprite=gameplayDitherEdge;return false;}
                    actors[count++]={gameplayDitherEdge,0,int(sx),int(sy),0};
                }
                return true;
            }
            if(kind==GameplayWorld::DrawActor::entryDither){
                const auto* record=port.entryDithers()?port.entryDithers()->find(key):nullptr;
                std::int32_t sourceDepth;
                if(!record||entryCoverVisible||!depth.promoted().roundPixel(sourceDepth)||sourceDepth!=-200000||
                   !downwell::EntryDitherPlacement::resolve(record->x,record->y,
                       downwell::MotionBinary64::fromInteger(worldViewX),
                       downwell::MotionBinary64::fromInteger(worldViewY),entryCover))return false;
                entryCoverVisible=true;return true; // Immutable cover, never a BOB.
            }
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
            if(kind==GameplayWorld::DrawActor::bone||kind==GameplayWorld::DrawActor::skeleton){
                const bool skeleton=kind==GameplayWorld::DrawActor::skeleton;
                const auto* bone=!skeleton&&port.bones()?port.bones()->find(key):nullptr;
                const auto* actor=skeleton&&port.skeletons()?port.skeletons()->find(key):nullptr;
                if(!bone&&!actor)return false;
                const auto px=actor?actor->x:bone->x,py=actor?actor->y:bone->y;
                const int sprite=actor?actor->spriteId:460;
                unsigned frame;
                if(actor){
                    downwell::RunnerImageValue index;
                    if(!index.assign(actor->state.imageIndex))return false;
                    const unsigned exponent=(index.bits>>23)&255u;
                    if((index.bits&0x80000000u)||exponent>129u)return false;
                    frame=exponent<127u?0:((index.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
                    if(frame>=downwell::SkeletonAnimation::frames(sprite))return false;
                }else frame=bone->state.drawFrame();
                std::int32_t x,y;downwell::RunnerImageValue ix,iy;
                failedSprite=sprite;
                // Source Step publishes integer room positions. Keep this
                // admission explicit: do not silently quantize an unsupported
                // fractional collision-setter result for the planar backend.
                if(!px.promoted().roundPixel(x)||!py.promoted().roundPixel(y)||
                   !ix.assignInteger(x)||!iy.assignInteger(y)||
                   ((ix.bits!=px.bits)&&((ix.bits|px.bits)&0x7fffffffu))||
                   ((iy.bits!=py.bits)&&((iy.bits|py.bits)&0x7fffffffu)))return false;
                failedSprite=0;
                return append(sprite,frame,downwellPlayfieldX+x-worldViewX,y-worldViewY,actor&&actor->publishedFacing<0);
            }
            if(kind==GameplayWorld::DrawActor::timeShard){
                if(compositeCurrency)return true;
                const auto* effect=port.timeShardForDraw(key);
                downwell::TimeShardDraw::Command command;unsigned frame;std::int32_t x,y;
                if(!effect||!downwell::TimeShardDraw::prepare(*effect,{},true,false,false,command))return false;
                if(command.placement.target==downwell::EffectSurfacePlacement::hidden)return true;
                failedSprite=91;
                using Real=downwell::MotionBinary64;
                if(!downwell::DebrisDraw::frame(91,command.imageIndex,frame)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.x,Real::fromInteger(worldViewX),command.placement.x)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.y,Real::fromInteger(worldViewY),command.placement.y)||
                   !command.placement.x.roundPixel(x)||!command.placement.y.roundPixel(y))return false;
                failedSprite=0;return append(91,frame,downwellPlayfieldX+x,y,0);
            }
            if(kind==GameplayWorld::DrawActor::corpseExpiry){
                const auto* record=port.registeredExpiry()?port.registeredExpiry()->find(key):nullptr;
                if(!record||!record->state.valid)return false;
                const auto& effect=record->state;
                auto x=record->x.promoted(),y=record->y.promoted();std::int32_t px,py;
                if(!downwell::EffectSurfacePlacement::relative(x,downwell::MotionBinary64::fromInteger(worldViewX),x)||
                   !downwell::EffectSurfacePlacement::relative(y,downwell::MotionBinary64::fromInteger(worldViewY),y)||
                   !x.roundPixel(px)||!y.roundPixel(py))return false;
                return append(effect.spriteId,effect.drawFrame(),downwellPlayfieldX+px,py,0);
            }
            if(kind==GameplayWorld::DrawActor::corpse){
                if(compositeCurrency)return true;
                const auto* r=port.registeredCorpses()?port.registeredCorpses()->find(key):nullptr;
                downwell::CorpseDraw::Command command;std::int32_t x,y;
                if(!r||!downwell::CorpseDraw::prepare(r->state,{},true,false,false,command))return false;
                if(command.placement.target==downwell::EffectSurfacePlacement::hidden)return true;
                using Real=downwell::MotionBinary64;
                const auto scale=command.scaleX.bits;
                if(!(scale&0x7fffffffu))return true;
                if((scale&0x7fffffffu)!=0x3f800000u||command.imageIndex.bits&0x7fffffffu)return false;
                if(!downwell::EffectSurfacePlacement::relative(command.placement.x,Real::fromInteger(worldViewX),command.placement.x)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.y,Real::fromInteger(worldViewY),command.placement.y)||
                   !command.placement.x.roundPixel(x)||!command.placement.y.roundPixel(y))return false;
                return append(command.sprite,0,downwellPlayfieldX+x,y,scale>>31);
            }
            if(kind==GameplayWorld::DrawActor::consumptionBlood){
                const auto* r=port.consumptionParticles()?port.consumptionParticles()->find(key):nullptr;
                unsigned frame;std::int32_t x,y;
                if(!r||!r->state.drawFrame(frame))return false;
                auto px=r->x.promoted(),py=r->y.promoted();using Real=downwell::MotionBinary64;
                if(!downwell::EffectSurfacePlacement::relative(px,Real::fromInteger(worldViewX),px)||
                   !downwell::EffectSurfacePlacement::relative(py,Real::fromInteger(worldViewY),py)||
                   !px.roundPixel(x)||!py.roundPixel(y))return false;
                return append(92,frame,downwellPlayfieldX+x,y,0); // Default Draw, never surfaceFx.
            }
            if(kind==GameplayWorld::DrawActor::smoke){
                if(compositeCurrency)return true;
                const auto* r=port.registeredSmoke()?port.registeredSmoke()->find(key):nullptr;
                downwell::SmokeDraw::Command command;unsigned frame;std::int32_t x,y;
                if(!r||!downwell::SmokeDraw::prepare(r->state.imageIndex,r->x,r->y,{},true,false,false,command)||
                   !downwell::SmokeDraw::frame(command.imageIndex,frame))return false;
                using Real=downwell::MotionBinary64;
                if(!downwell::EffectSurfacePlacement::relative(command.placement.x,Real::fromInteger(worldViewX),command.placement.x)||
                   !downwell::EffectSurfacePlacement::relative(command.placement.y,Real::fromInteger(worldViewY),command.placement.y)||
                   !command.placement.x.roundPixel(x)||!command.placement.y.roundPixel(y))return false;
                return append(84,frame,downwellPlayfieldX+x,y,0);
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
            // Wall/bone impacts use eight/six unrotated frames. The offline muzzle
            // proof covers integer anchors and either no rotation or 267..273
            // degrees. Reject other transforms explicitly;
            // no runtime conversion or silently rounded fractional artwork.
            const bool wallImpact=effect.spriteId==101;
            const bool boneImpact=effect.spriteId==85;
            if(!effect.valid||(!wallImpact&&!boneImpact&&effect.spriteId!=602&&effect.spriteId!=603)||
               effect.drawFrame()>=(wallImpact?8u:boneImpact?6u:1u)||
               !effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)||
               !ix.assign(downwell::MotionBinary64::fromInteger(x))||
               !iy.assign(downwell::MotionBinary64::fromInteger(y))||
               ((ix.bits!=effect.x.bits)&&((ix.bits|effect.x.bits)&0x7fffffffu))||
               ((iy.bits!=effect.y.bits)&&((iy.bits|effect.y.bits)&0x7fffffffu)))return false;
            unsigned frame=effect.drawFrame();
            if(effect.angle.bits&0x7fffffffu){
                if(wallImpact||boneImpact)return false;
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
