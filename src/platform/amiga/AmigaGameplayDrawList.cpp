#include "../../../generated/downwell-rising-texts.h"
#include "AmigaGameplayDrawList.h"
#include "AmigaProfile.h"
#include "core/ShopFixtures.h"
#include "core/ShopChildDraw.h"
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
    if(world.levelNumbers()&&world.levelNumbers()->size())other=true;
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
    if(world.levelNumbers()&&world.levelNumbers()->size())other=true;
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

namespace {
// Nearest offline quarter turn (frame page turn*256) for an image_angle.
bool quarterTurn(downwell::RunnerImageValue angle,unsigned& turn){
    std::int32_t degrees;
    if(!angle.promoted().roundPixel(degrees))return false;
    degrees%=360;if(degrees<0)degrees+=360;
    turn=unsigned((degrees+45)/90)&3u;return true;
}
}

// Compact scrEffectSpawn and emitMovingFx records in birth order, then each TorchFire's red and white particle
// layers. The offline bank holds the reviewed sprites and quarter turns.
template<class Append> bool AmigaGameplayDrawList::drawCompactRoomEffects(const GameplayWorld& port,
    Append& append,int viewX,int viewY){
    const auto& fx=port.compactEffects();
    const auto place=[&](const downwell::RunnerImageValue& rx,const downwell::RunnerImageValue& ry,int sprite,unsigned frame){
        std::int32_t x,y;
        if(!rx.promoted().roundPixel(x)||!ry.promoted().roundPixel(y)){failedSprite=UWORD(sprite);failedFrame=UWORD(frame);return false;}
        // A 62000+id record (the landing dust's mirrored half) is the
        // prepared frame's flipped side, not a separate prepared sprite.
        const bool mirrored=sprite>=62000&&sprite<63000;
        return append(UWORD(mirrored?sprite-62000:sprite),UWORD(frame),downwellPlayfieldX+x-viewX,y-viewY,mirrored?1:0);
    };
    for(unsigned i=0;i<fx.noLoopCount();++i){
        const auto& e=fx.noLoop(i).effect;unsigned turn;
        if(!e.valid||!quarterTurn(e.angle,turn))return false;
        // 603 and 605 carry offline 0 and 270 degree pages; 605 also 90
        // (W1.9: Gunpowder Blocks' upward shot splashes at its angle).
        if(turn!=0&&turn!=3&&!(turn==1&&e.spriteId==605)){failedSprite=UWORD(e.spriteId);return false;}
        if(!place(e.x,e.y,e.spriteId,turn*256+e.drawFrame()))return false;
    }
    {   // fxBlood 223 (Knife and Fork): sprWaterSplash 92, default Draw.
        unsigned end=0;const auto* blood=port.compactBlood(end);
        for(unsigned i=0;i<end;++i){const auto& b=blood[i];unsigned frame;
            if(!b.live)continue;
            if(!b.effect.drawFrame(frame))return false;
            if(!place(b.effect.x,b.effect.y,92,frame))return false;
        }
    }
    for(unsigned i=0;i<fx.movingCount();++i){
        const auto& e=fx.moving(i).effect;unsigned turn=0;
        if(!e.valid)return false;
        // Only the red splash rotates in the bank; smoke puffs, sparkles and
        // the opening flash are drawn upright.
        if(e.spriteId==704&&!quarterTurn(e.angle,turn))return false;
        if(!place(e.x,e.y,e.spriteId,turn*256+e.drawFrame()))return false;
    }
    const auto& sc=port.compactScenery();
    for(unsigned f=0;f<sc.fireEnd;++f){
        const auto& fire=sc.fires[f];
        if(!fire.live||fire.y<viewY-32||fire.y>viewY+downwellDisplayHeight+32)continue;
        for(unsigned layer=0;layer<(fire.active?2u:1u);++layer)
            for(const auto& part:fire.part){
                std::int32_t x,y,life=0;
                if(!part.x.roundPixel(x)||!part.y.roundPixel(y))return false;
                int order;
                if(!downwell::compareRunnerReal(part.life,downwell::MotionBinary64::fromInteger(8),order))return false;
                if(order>=0)continue;
                // draw_sprite floors the fractional subimage. Life is a
                // nonnegative double below 8: take its integer part.
                {
                    const int exponent=int((part.life.high>>20)&0x7ffu)-1023;
                    if((part.life.high>>31)||exponent<0)life=0;
                    else life=std::int32_t(((part.life.high&0xfffffu)|0x100000u)>>(20-exponent));
                }
                const int sprite=fire.active?(layer?712:711):83;
                if(!append(UWORD(sprite),UWORD(life&7),downwellPlayfieldX+x-viewX,y-viewY,0))return false;
            }
    }
    // Compact fxTimeShard 238 (W1.5): Draw is sprite 91 while dflash<=0.
    bool shardsOk=true;
    port.compactTimeShards().visitNewest([&](auto,const auto& e){
        if(!shardsOk||e.dflash>0)return;
        std::int32_t x,y;unsigned frame;
        if(!e.x.promoted().roundPixel(x)||!e.y.promoted().roundPixel(y)||
           !downwell::DebrisDraw::frame(91,e.imageIndex,frame)){failedSprite=91;shardsOk=false;return;}
        if(!append(UWORD(91),UWORD(frame),downwellPlayfieldX+x-viewX,y-viewY,0))shardsOk=false;
    });
    return shardsOk;
}

// W1.4 compact gun owners, all in room coordinates.
template<class Append> bool AmigaGameplayDrawList::drawCompactGuns(const GameplayWorld& port,
    Append& append,int viewX,int viewY){
    const auto& g=port.compactGuns();
    const auto place=[&](int sprite,unsigned frame,int x,int y,int flip){
        return append(UWORD(sprite),UWORD(frame),downwellPlayfieldX+x-viewX,y-viewY,flip);
    };
    // GunModule Draw: draw_self (the flashing frame), then sprModText's gun
    // letter at the same anchor.
    for(unsigned i=0;i<g.moduleEnd;++i){const auto& m=g.modules[i];
        if(!m.live)continue;
        if(!place(m.sprite(),unsigned(m.image),m.x,m.y,0)||!place(141,unsigned(m.letter()),m.x,m.y,0))return false;
    }
    // bulletCasing: draw_self while dFlash is positive; never mirrored.
    for(unsigned i=0;i<g.casingEnd;++i){const auto& c=g.casings[i];
        if(c.live&&c.visible()&&!place(457,c.frame(),int((c.x+(1L<<15))>>16),int((c.y+(1L<<15))>>16),0))return false;
    }
    // fxLaserTrail: image_angle bDir + 90 = 360 for the vertical laser.
    for(unsigned i=0;i<g.trailEnd;++i){const auto& t=g.trails[i];
        if(t.live&&!place(t.sprite,t.frame(),t.x,t.y,0))return false;
    }
    return true;
}

bool AmigaGameplayDrawList::glyph(unsigned id,int x,int y,unsigned pen){
    if(x+5<=downwellPlayfieldX||x-5>=downwellPlayfieldX+downwellPlayfieldWidth||y+11<=0||y-1>=downwellDisplayHeight)return true;
    if(count==capacity){failedSprite=gameplayGlyphMarker;failedFrame=UWORD(id);return false;}
    actors[count++]={gameplayGlyphMarker,UWORD(id),x,y,int(pen)};
    return true;
}

namespace {
unsigned borderGlyph(char c){return amigaBorderGlyph(c);}
}

// moduleText Draw (font 2, eight-pixel advance, top-left aligned): letters
// bounce by their frame before the exit, then fall away by it. itemNotif:
// centred "+ n" and sprItemNotif frame notifType at the same anchor.
template<class Append> bool AmigaGameplayDrawList::drawCompactGunTexts(const GameplayWorld& port,Append& append,bool notices,int viewX,int viewY){
    const auto& g=port.compactGuns();
    if(!notices){
        for(unsigned i=0;i<g.textEnd;++i){const auto& t=g.texts[i];
            if(!t.live||!t.allSet)continue;
            const int left=t.left(viewX,160),top=t.y-32;
            for(int k=1;k<=t.length;++k){
                const int f=t.frames[k];
                int dy;
                if(!t.exit){if(k>t.shown)continue;dy=downwell::CompactModuleTextRecord::bounce(f);}
                else{if(f>3)continue;dy=f;}
                // Glyph origin (4,0): the anchor is the cell's centre column.
                if(!glyph(borderGlyph(t.character(k)),downwellPlayfieldX+left+8*(k-1)+4-viewX,top+dy-viewY,1))return false;
            }
        }
        return true;
    }
    for(unsigned i=0;i<g.noticeEnd;++i){const auto& n=g.notices[i];
        if(!n.live||!n.visible())continue;
        const int x=n.left(viewX,160),y=n.pixelY();
        if(n.fixedText==downwell::CompactItemNoticeRecord::omnom){
            // fxRisingText omnomText: centred eight-pixel cells, no icon.
            const char* word=downwell::rising_texts::omnom;
            const int cells=int(sizeof(downwell::rising_texts::omnom))-1;
            const int left=n.left(viewX,160,8*cells);
            for(int k=0;k<cells;++k)
                if(word[k]!=' '&&!glyph(borderGlyph(word[k]),downwellPlayfieldX+left-4*cells+8*k+4-viewX,y-5-viewY,1))
                    return false;
            continue;
        }
        if(n.empty){
            // fxRisingText "EMPTY!": six centred cells, no icon.
            static const char word[]="EMPTY!";
            for(int k=0;k<6;++k)
                if(!glyph(borderGlyph(word[k]),downwellPlayfieldX+x-24+8*k+4-viewX,y-5-viewY,1))return false;
            continue;
        }
        const char text[]={'+',' ',char('0'+n.amount),0};
        // fa_center/fa_middle: three eight-pixel cells around x, ten rows.
        for(int k=0;k<3;++k)
            if(text[k]!=' '&&!glyph(borderGlyph(text[k]),downwellPlayfieldX+x-12+8*k+4-viewX,y-5-viewY,1))return false;
        if(!append(UWORD(713),UWORD(n.type),downwellPlayfieldX+x-viewX,y-viewY,0))return false;
    }
    return true;
}

// The merchant's fxRisingText (scrRisingTextShop), drawn like "EMPTY!":
// centred font-2 cells, spaces skipped.
bool AmigaGameplayDrawList::drawCompactShopSpeech(const GameplayWorld& port,int viewX,int viewY){
    const auto* speech=port.compactShopSpeech();
    for(unsigned i=0;i<downwell::ShopWorld::capacity;++i){const auto& r=speech[i];
        if(!r.rise.live||!r.rise.visible()||!r.text)continue;
        const int x=r.left(viewX,160)-4*r.length,y=r.rise.pixelY();
        for(int k=0;k<r.length;++k)
            if(r.text[k]!=' '&&!glyph(borderGlyph(r.text[k]),downwellPlayfieldX+x+8*k+4-viewX,y-5-viewY,1))return false;
    }
    return true;
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
#ifdef DOWNWELL_FRAME_WORK_CANDIDATE
    if(port.shops().allocatedCount){
#endif
    downwell::drawShopKeepers(port.shops(), [&](int sprite, unsigned frame, int x, int y) {
        if (!append( sprite, frame,
            downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) ready = false;
    });
    downwell::drawShopPrompts(port.shops(), false, [&](int sprite, unsigned frame, int x, int y) {
        if (!append( sprite, frame,
            downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) ready = false;
    });
#ifdef DOWNWELL_FRAME_WORK_CANDIDATE
    }
#endif
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

#ifndef DOWNWELL_REPLAY_VERIFY
    // Below the room actors (enemies, player) in both world compositions.
    if(!drawCompactRoomEffects(port,append,worldViewX,worldViewY))ready=false;
    if(!port.compactGuns().empty()&&!drawCompactGuns(port,append,worldViewX,worldViewY))ready=false;
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
#ifndef DOWNWELL_REPLAY_VERIFY
        // playerDraw's gem-high afterimages, oldest first, behind the player
        // and drawn through the hit blink. Each pose has two red striped
        // pages; the one starting on an odd room column matches
        // shaderAfterImage1's mod(x, 2) test.
        for (unsigned i = port.afterImagesDrawn(); i-- > 0;) {
            const auto& a = port.afterImage(i);
            const int x = downwellPlayfieldX + 240 + int(downwell::roundGameMakerQ8(a.xFixed)) + port.afterImageDx(i) - worldViewX;
            const int y = 176 + int(downwell::roundGameMakerQ8(a.yFixed)) + port.afterImageDy(i) - worldViewY;
            const unsigned page = 1024u + (a.facing < 0 ? 512u : 0u) + a.frame;
            int left, top, width, height;
            if (const auto* frame = prepared ? prepared->find(UWORD(a.sprite), UWORD(page)) : nullptr)
                frame->bounds(x, y, 0, left, top, width, height);
            else if (prepared || !amigaRendererAssetBounds(pack, packEnd, UWORD(a.sprite), UWORD(page),
                         x, y, 0, &left, &top, &width, &height)) {
                failedSprite = UWORD(a.sprite); failedFrame = UWORD(page); return false;
            }
            const bool odd = ((left - downwellPlayfieldX + worldViewX) & 1) != 0;
            if (!append(UWORD(a.sprite), UWORD(page + (odd ? 0u : 256u)), x, y, 0)) return false;
        }
#endif
        // The pose the world evaluated at this update's Draw boundary.
        const auto& pose = port.presentedPlayerPose();
        if (port.playerVisible() && !append(
                (UWORD)pose.sprite, (UWORD)pose.frame,
                downwellPlayfieldX + 80 + downwell::roundGameMakerQ8(pose.xFixed) - port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
                replayGroundY + port.player().offsetY(),
#else
                playerStartY + downwell::roundGameMakerQ8(pose.yFixed) - port.cameraOffsetY(),
#endif
                pose.facing < 0)) return false;
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
#ifndef DOWNWELL_REPLAY_VERIFY
        // playerDraw ends with drawComboNumber(x, y - 11, comboCount).
        if(port.comboCount()>=5&&!port.goalStopped()&&
           !downwell::drawComboNumber(
                downwellPlayfieldX+80+downwell::roundGameMakerQ8(port.player().offsetXFixed())-port.cameraOffsetX(),
                playerStartY+downwell::roundGameMakerQ8(port.player().offsetYFixed())-port.cameraOffsetY()-11,
                port.comboCount(),[&](unsigned sprite,unsigned frame,int x,int y){
                    return append(UWORD(sprite),UWORD(frame),x,y,0);
                }))return false;
#endif
        return true;
    };
    // W1.9 compact bulletBlast 276 (depth -80000, default Draw): above the
    // player (-50000), below objDitherAbove (-100000). Newest first within
    // the depth, as the runner's layer lists insert.
    bool blastsDrawn=false;
    const auto drawBlasts=[&](){
        if(blastsDrawn)return true;
        blastsDrawn=true;
        unsigned end=0;const auto* blasts=port.compactBlasts(end);
        for(unsigned i=end;i-->0;){const auto& b=blasts[i];
            if(!b.live||b.object!=276)continue;
            if(!append(UWORD(b.sprite),UWORD(b.frame()),downwellPlayfieldX+b.x-worldViewX,b.y-worldViewY,b.xscale<0))return false;
        }
        return true;
    };
    // objDitherAbove 61 Draw (depth -100000): black above its y, then the
    // dither edge below it.
    const auto drawDitherAbove=[&](const downwell::GameplayDitherAbove::Record* record){
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
        };
    if(port.hasRoomActorDraw()) {
        DW_PROFILE_SCOPE(profileActorLayerWalk);
        downwell::RunnerImageValue foregroundDepth;foregroundDepth.assign(downwell::MotionBinary64::fromInteger(-1000));
        downwell::RunnerImageValue blastDepth;blastDepth.assign(downwell::MotionBinary64::fromInteger(-80000));
        if(!port.visitRoomActorsReadOnly([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){
            DW_PROFILE_SCOPE(profileActorPreparation);
            if(downwell::RunnerSpatialSplit::less(depth,foregroundDepth)&&!drawForeground())return false;
            if(downwell::RunnerSpatialSplit::less(depth,blastDepth)&&!drawBlasts())return false;
            if(kind==GameplayWorld::DrawActor::player)return port.findPlayer(key)&&drawPlayer();
            // Registered Shop Draw events need their own source handlers.
            // Never reinterpret a Shop-local handle as a script-effect slot,
            // or silently omit it while drawing compact Shop copies.
            if(kind==GameplayWorld::DrawActor::shop){
                const auto* owner=port.shopInstances();const auto* record=owner?owner->find(key):nullptr;
                if(!record)return false;
                struct Ops {
                    const downwell::GameplayShopInstances& owner;downwell::InstanceHandle key;
                    const downwell::GameplayShopInstances::Record& r;decltype(append)& paint;
                    int viewX,viewY;unsigned currentArea;
                    downwell::MotionBinary64 x(){return r.x.promoted();}
                    downwell::MotionBinary64 y(){return r.y.promoted();}
                    bool area(unsigned& value){value=currentArea;return true;}
                    bool sprite(int id,unsigned frame,downwell::MotionBinary64 x,downwell::MotionBinary64 y){
                        std::int32_t px,py;
                        return x.add(downwell::MotionBinary64::fromInteger(downwellPlayfieldX-viewX))&&
                            y.add(downwell::MotionBinary64::fromInteger(-viewY))&&x.roundPixel(px)&&y.roundPixel(py)&&
                            paint(id,frame,px,py,0);
                    }
                    bool self(){unsigned frame;return r.spriteId<0||
                        (owner.drawFrame(key,frame)&&sprite(r.spriteId,frame,x(),y()));}
                } ops{*owner,key,*record,append,worldViewX,worldViewY,unsigned(port.runStage().area)};
                return downwell::ShopChildDraw::draw(record->objectId,ops);
            }
            if(kind==GameplayWorld::DrawActor::redBall){
                const auto* record=port.redBalls()?port.redBalls()->find(key):nullptr;
                if(!record)return false;
                struct Ops {
                    using N=downwell::MotionBinary64;
                    const downwell::GameplayRedBalls::Record& actor;
                    decltype(append)& paint;
                    int viewX,viewY;bool noControl;
                    bool shade(){
                        // The common effect-surface pass consumes this same
                        // registered actor via visitRegisteredEnemyShades.
                        // Validate its placement here; do not paint the shade
                        // as a visible body sprite or duplicate the surface.
                        downwell::EffectSurfacePlacement::Draw placement;
                        const downwell::EffectSurfacePlacement::View view{N::fromInteger(viewX),N::fromInteger(viewY),
                            N::fromInteger(160),N::fromInteger(284)};
                        return downwell::EffectSurfacePlacement::enemyShade(actor.x.promoted(),actor.y.promoted(),
                            view,false,noControl,true,placement);
                    }
                    bool drawSelf(){
                        std::int32_t x,y,scale;unsigned frame;
                        return actor.x.roundPixel(x)&&actor.y.roundPixel(y)&&actor.scaleX.roundPixel(scale)&&
                            (scale==1||scale==-1)&&downwell::GameplayRedBalls::drawFrame(actor.spriteId,actor.imageIndex,frame)&&
                            paint(actor.spriteId,frame,downwellPlayfieldX+x-viewX,y-viewY,scale<0);
                    }
                    bool imageIndex(N& frame){frame=actor.imageIndex.promoted();return true;}
                    bool drawSpriteExt(int sprite,N index,N x,N y,int sx,int sy,int angle,unsigned color,int alpha){
                        const auto expected=actor.imageIndex.promoted();
                        std::int32_t px,py;unsigned frame;
                        // All five eye resources have five frames. The body
                        // supplies its unchanged index in [0,4), so the same
                        // floor selects both images, including stun frame 0.
                        return sprite>=181&&sprite<=185&&(sx==1||sx==-1)&&sy==1&&!angle&&color==0xffffffu&&alpha==1&&
                            index.high==expected.high&&index.low==expected.low&&x.roundPixel(px)&&y.roundPixel(py)&&
                            downwell::GameplayRedBalls::drawFrame(actor.spriteId,actor.imageIndex,frame)&&
                            paint(sprite,frame,downwellPlayfieldX+px-viewX,py-viewY,sx<0);
                    }
                } ops{*record,append,worldViewX,worldViewY,port.shops().noControl};
                return port.drawRegisteredRedBall(key,ops);
            }
            if(kind==GameplayWorld::DrawActor::whiteJelly){
                const auto* record=port.whiteJellies()?port.whiteJellies()->find(key):nullptr;
                std::int32_t x,y;unsigned frame;
                if(!record||!record->x.roundPixel(x)||!record->y.roundPixel(y))return false;
                // draw_self selects floor(image_index), not round(image_index).
                if(!downwell::GameplayWhiteJellies::drawFrame(record->spriteId,record->imageIndex,frame))return false;
                return append(record->spriteId,frame,downwellPlayfieldX+x-worldViewX,y-worldViewY,false);
            }
            if(kind==GameplayWorld::DrawActor::scattered){
                const auto* record=port.scattered()?port.scattered()->find(key):nullptr;
                std::int32_t x,y,frame,scale;
                if(!record||!record->x.roundPixel(x)||!record->y.roundPixel(y)||
                   !record->imageIndex.roundPixel(frame)||!record->scaleX.roundPixel(scale)||
                   (scale!=1&&scale!=-1)||frame<0||frame>16)return false;
                return append(record->spriteId,frame,downwellPlayfieldX+x-worldViewX,
                    y-worldViewY,scale<0);
            }
            if(kind==GameplayWorld::DrawActor::ditherAbove)
                return drawDitherAbove(port.ditherAbove()?port.ditherAbove()->find(key):nullptr);
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
                if(!enemy->active||!enemy->awake)return true;
                const int x=downwellPlayfieldX+80+enemy->x-port.cameraOffsetX();
                const int y=playerStartY+enemy->y-port.cameraOffsetY();
                if(!append(enemy->spriteId,enemy->drawFrame(),x,y,enemy->objectId!=119&&enemy->facing<0))return false;
                // enmRedBall Draw: the eye follows draw_self at (x,y-5),
                // with the body's image index and the sector's own mirror.
                int eye,mirror;
                return enemy->objectId!=118||!port.redBallEye(*enemy,eye,mirror)||
                    append(eye,enemy->drawFrame(),x,y-5,mirror<0);
#else
                return true;
#endif
            }
            if(kind==GameplayWorld::DrawActor::bullet){
                const auto* bullet=port.resolveBulletLifetime(key);
                return bullet&&(!bullet->active||append(bullet->sprite,
                    bullet->drawFrame(),downwellPlayfieldX+80+bullet->x-port.cameraOffsetX(),
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
            // Mutable title Draw belongs to the unconditional surface pass,
            // never to BOB collection. Missing bindings must still fail.
            if(kind==GameplayWorld::DrawActor::levelNumber)return titleGlyphs_&&titleText_;
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
            if(kind==GameplayWorld::DrawActor::limboShard){
                const auto* r=port.limboShards()?port.limboShards()->find(key):nullptr;
                if(!r)return false;
                if(!r->state.drawsSelf())return true;
                unsigned frame;std::int32_t x,y;
                if(!downwell::DebrisDraw::frame(91,r->state.imageIndex,frame))return false;
                auto px=r->x.promoted(),py=r->y.promoted();using Real=downwell::MotionBinary64;
                if(!downwell::EffectSurfacePlacement::relative(px,Real::fromInteger(worldViewX),px)||
                   !downwell::EffectSurfacePlacement::relative(py,Real::fromInteger(worldViewY),py)||
                   !px.roundPixel(x)||!py.roundPixel(y))return false;
                return append(91,frame,downwellPlayfieldX+x,y,0);
            }
            if(kind==GameplayWorld::DrawActor::blast){
                const auto* r=port.registeredBlasts()?port.registeredBlasts()->find(key):nullptr;
                unsigned frame;std::int32_t x,y;
                if(!r||!r->state.drawFrame(frame))return false;
                auto px=r->x.promoted(),py=r->y.promoted();using Real=downwell::MotionBinary64;
                if(!downwell::EffectSurfacePlacement::relative(px,Real::fromInteger(worldViewX),px)||
                   !downwell::EffectSurfacePlacement::relative(py,Real::fromInteger(worldViewY),py)||
                   !px.roundPixel(x)||!py.roundPixel(y))return false;
                return append(471,frame,downwellPlayfieldX+x,y,r->publishedScale.bits>>31);
            }
            if(kind==GameplayWorld::DrawActor::flashBall){
                const auto* r=port.registeredFlash()?port.registeredFlash()->find(key):nullptr;
                if(!r)return false;
                auto px=r->x.promoted(),py=r->y.promoted();using Real=downwell::MotionBinary64;
                std::int32_t x,y;
                if(!downwell::EffectSurfacePlacement::relative(px,Real::fromInteger(worldViewX),px)||
                   !downwell::EffectSurfacePlacement::relative(py,Real::fromInteger(worldViewY),py)||
                   !px.roundPixel(x)||!py.roundPixel(y))return false;
                return append(96,r->state.drawFrame(),downwellPlayfieldX+x,y,0);
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
            const bool blastImpact=effect.spriteId==605;
            if(!effect.valid||(!wallImpact&&!boneImpact&&!blastImpact&&effect.spriteId!=602&&effect.spriteId!=603)||
               effect.drawFrame()>=(wallImpact?8u:boneImpact?6u:blastImpact?3u:1u)||
               !effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)||
               !ix.assign(downwell::MotionBinary64::fromInteger(x))||
               !iy.assign(downwell::MotionBinary64::fromInteger(y))||
               ((ix.bits!=effect.x.bits)&&((ix.bits|effect.x.bits)&0x7fffffffu))||
               ((iy.bits!=effect.y.bits)&&((iy.bits|effect.y.bits)&0x7fffffffu)))return false;
            unsigned frame=effect.drawFrame();
            if(blastImpact){
                if(effect.angle.bits!=0x43870000u)return false; // Source blast direction: 270 degrees.
                frame+=768;
            }else if(effect.angle.bits&0x7fffffffu){
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
        if(!drawBlasts())ready=false;
    } else {
        // Without a collision owner the world-1 room actors follow the
        // runner's layer order directly: bullets (depth 0), the foreground
        // effects, enemies by descending depth and, within a depth, newest
        // layer entry first, then the player (-50000) and objDitherAbove
        // (-100000). Bullets draw newest slot first.
        for (unsigned index = port.bulletSlotEnd(); index-- > 0;) {
            const auto& bullet = port.bulletAt(index);
            // The pack generator checks every possible nearest-pixel
            // sample over 267..273 degrees for all twelve source frames:
            // the spread-adjusted artwork equals this offline quarter
            // turn. Other angles/guns need separately verified variants.
            if (bullet.active && !append(bullet.sprite, bullet.drawFrame(),
                    downwellPlayfieldX + 80 + bullet.x - port.cameraOffsetX(),
                    playerStartY + bullet.y - port.cameraOffsetY(), 0))
                ready = false;
        }
#ifndef DOWNWELL_REPLAY_VERIFY
        unsigned char order[GameplayWorld::enemyCapacity];unsigned drawn=0;
        const auto later=[&](unsigned a,unsigned b){
            const auto& x=port.enemyAt(a);const auto& y=port.enemyAt(b);
            const int dx=downwell::enemyDrawDepth(x.objectId),dy=downwell::enemyDrawDepth(y.objectId);
            if(dx!=dy)return dx<dy;
            if(x.activatedTick!=y.activatedTick)return x.activatedTick<y.activatedTick;
            return x.lifetimeId<y.lifetimeId;
        };
        for (unsigned index = 0; index < port.enemyCount(); ++index) {
            const auto& enemy = port.enemyAt(index);
            if (!enemy.active || !enemy.awake) continue;
            unsigned at=drawn++;
            while(at&&later(order[at-1],index)){order[at]=order[at-1];--at;}
            order[at]=static_cast<unsigned char>(index);
        }
        for (unsigned i = 0; i < drawn; ++i) {
            const auto& enemy = port.enemyAt(order[i]);
            if(downwell::enemyDrawDepth(enemy.objectId)<-1000&&!drawForeground())ready=false;
            const int x=downwellPlayfieldX+80+enemy.x-port.cameraOffsetX();
            const int y=playerStartY+enemy.y-port.cameraOffsetY();
            if(!append(enemy.spriteId,enemy.drawFrame(),x,y,enemy.objectId!=119&&enemy.facing<0))ready=false;
            int eye,mirror;
            if(enemy.objectId==118&&port.redBallEye(enemy,eye,mirror)&&
               !append(eye,enemy.drawFrame(),x,y-5,mirror<0))ready=false;
        }
#endif
        if(!drawForeground()||!drawPlayer()||!drawBlasts())ready=false;
        if(const auto* above=port.ditherAbove()){
            const auto* record=above->find(above->handle());
            if(record&&record->active&&!record->destroyRequested&&!drawDitherAbove(record))ready=false;
        }
    }
#ifndef DOWNWELL_REPLAY_VERIFY
    if(port.compactGuns().textEnd&&!drawCompactGunTexts(port,append,false,worldViewX,worldViewY))ready=false;
#endif
    if(!drawForeground())ready=false;
#ifndef DOWNWELL_REPLAY_VERIFY
    if(port.compactGuns().noticeEnd&&!drawCompactGunTexts(port,append,true,worldViewX,worldViewY))ready=false;
    if(port.compactShopSpeechLive()&&!drawCompactShopSpeech(port,worldViewX,worldViewY))ready=false;
#endif

    // Original compatibility depth -200000: ahead of ordinary gameplay,
    // but behind the controller's effect-surface publication (-300000).
#ifndef DOWNWELL_REPLAY_VERIFY
    port.flashEffects().visitNewest([&](const downwell::FlashBallEffect& effect) {
        if(!append(96,effect.drawFrame(),downwellPlayfieldX+effect.pixelX()-worldViewX,
                   effect.pixelY()-worldViewY,0)) ready=false;
    });
    if(!drawCompactHudTexts(port,append,worldViewY))ready=false;
#endif
    return ready;
}

// Effect-surface texts in view coordinates: levelNumber 372 (scrDraw-
// BorderTextBlack, font 2, top-left aligned like moduleText) and
// comboRewardText 383.
template<class Append> bool AmigaGameplayDrawList::drawCompactHudTexts(const GameplayWorld& port,Append& append,int viewY){
    // levelNumber is drawn on the fixed HUD layer (AmigaLevelTitleHud).
    if(!port.compactComboRewardsLive())return true;
    const auto* rewards=port.compactComboRewards();
    for(unsigned i=0;i<GameplayWorld::compactComboRewardCapacity;++i){
        const auto& r=rewards[i];
        if(!r.live)continue;
        if(!downwell::drawComboReward(r,r.drawX(),r.drawY(long(viewY)<<16),[&](unsigned sprite,unsigned frame,int x,int y){
                return append(UWORD(sprite),UWORD(frame),downwellPlayfieldX+x,y,0);
            }))return false;
    }
    return true;
}
