#ifndef DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#define DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#include "AmigaGameplayActors.h"
#include "core/GameplayWorld.h"
#include "core/EntryDitherPlacement.h"
#include "core/EffectMaskAssets.h"
#include "core/LevelNumberSurfaceDraw.h"
class AmigaGameplayDrawList {
public:
    // Two particle layers for up to three TorchFires in view.
    static constexpr unsigned compactFireLayerCapacity = 3 * 2 * downwell::CompactTorchFireRecord::parts;
    static constexpr unsigned capacity = 4 + 32 + 32 + 32 + 32 + 32 + 8 + 8 + GameplayWorld::bulletCapacity +
        downwell::CompactGuns::casingCapacity + downwell::CompactGuns::trailCapacity + 2 * downwell::CompactGuns::moduleCapacity +
        downwell::CompactGuns::textCapacity * downwell::CompactModuleTextRecord::maximumLength +
        downwell::CompactGuns::noticeCapacity * 4 +
        GameplayWorld::enemyCapacity + GameplayWorld::gemCapacity + GameplayWorld::boneCapacity + downwell::scriptSpawnEffectCapacity +
        2 * downwell::ShopWorld::capacity + downwell::GameplayScattered::capacity + downwell::GameplayWhiteJellies::capacity +
        downwell::gameplayCompactNoLoopCapacity + downwell::gameplayCompactMovingCapacity + compactFireLayerCapacity +
        // W1.7: combo counter (10 digits, 3 icons), each reward text (12
        // digits/icons, COMBO!, reward, flash) and nine gem-high afterimages.
        13 + 4 * 18 + 9;
    static constexpr int playerStartY = 142, replayGroundY = 200;
    GameplayBob actors[capacity]{};
    unsigned count = 0;
    UWORD failedSprite = 0, failedFrame = 0;
    downwell::LaserSightLine rayLine{};
    bool rayVisible = false;
    bool entryCoverVisible = false;
    downwell::EntryDitherPlacement entryCover{};
    // Borrow current text state and offline masks for the enclosing scene.
    // No implicit font/alignment/alpha defaults; clear before their owner dies.
    bool bindLevelNumberText(const downwell::FontGlyphMasks* glyphs,downwell::LevelNumberTextDrawState* state){
        if(bool(glyphs)!=bool(state))return false;
        titleGlyphs_=glyphs;titleText_=state;return true;
    }
    template<class Emit> bool drawLevelNumber(const GameplayWorld& world,downwell::InstanceHandle key,
        const downwell::EffectSurfacePlacement::View& view,Emit& emit)const{
        if(!titleGlyphs_||!titleText_)return false;
        struct Target {
            const downwell::EffectSurfacePlacement::View& camera;
            bool exists(bool& present){present=true;return true;}
            bool begin(){return true;}bool end(){return true;}
            bool view(unsigned axis,downwell::MotionBinary64& value){
                if(axis>1)return false;
                value=axis?camera.y:camera.x;return true;
            }
        } target{view};
        downwell::LevelNumberSurfaceDraw<Target,Emit> draw(*titleGlyphs_,*titleText_,target,emit);
        return world.drawLevelNumber(key,draw);
    }
    // Read-only projection of shared simulation into original painter order.
    // No simulation stepping, asset conversion or framebuffer writes.
    bool collect(const GameplayWorld& world, const UBYTE* pack,
        const UBYTE* packEnd, int worldViewX, int worldViewY, bool compositeCurrency = false,
        const AmigaActorFrames* prepared = nullptr);
    bool entryCoverHardwareEligible(const GameplayWorld& world,int viewX,int viewY,bool& eligible)const;
    // Conservative presence gate for controller surfaceFx work. Counts keep
    // inactive/marked/off-screen records until their owners retire them. Shade
    // is only a subtractor: it cannot produce pixels on an empty surface.
    bool hasControllerProducers(const GameplayWorld& world)const{
        return entryCoverVisible||world.gemSlotEnd()||world.smokeEffects().size()||
            world.compactDebris().size()||world.compactCorpses().size()||
            (world.registeredSmoke()&&world.registeredSmoke()->size())||
            (world.registeredCorpses()&&world.registeredCorpses()->size())||
            (world.debrisInstances()&&world.debrisInstances()->size())||
            (world.timeShardInstances()&&world.timeShardInstances()->size())||
            (world.levelNumbers()&&world.levelNumbers()->size());
    }
    // Currency Draw walks slots newest to oldest. Promote only its topmost
    // contiguous suffix when shade requires a retained raster. A completely
    // unshaded pass uses the actor compositor, including BOB channel overflow.
    // Ordinary actors remain below both partitions. currencySpriteCount is
    // the admitted controller suffix length before sprite/BOB assignment.
    unsigned currencyRasterFirst=0,currencySpriteCount=0;
    bool currencyRasterRequired=true;
    bool promoteCurrency(const GameplayWorld& world,int viewX,int viewY,
        const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames);
    // Retained all-or-nothing reference for projection tests.
    bool promoteCurrencyComplete(const GameplayWorld& world,int viewX,int viewY,
        const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames);
private:
    template<class Append> bool drawCompactRoomEffects(const GameplayWorld& world,Append& append,int viewX,int viewY);
    // W1.4: GunModule (-10100) and the depth-0 casings and laser trails.
    template<class Append> bool drawCompactGuns(const GameplayWorld& world,Append& append,int viewX,int viewY);
    // moduleText (-1000) and itemNotif (-50000): bordered glyph markers.
    template<class Append> bool drawCompactGunTexts(const GameplayWorld& world,Append& append,bool notices,int viewX,int viewY);
    template<class Append> bool drawCompactHudTexts(const GameplayWorld& world,Append& append,int viewY);
    bool glyph(unsigned id,int x,int y,unsigned pen);
    const downwell::FontGlyphMasks* titleGlyphs_=nullptr;
    downwell::LevelNumberTextDrawState* titleText_=nullptr;
    // Admission workspace, never published until the source-ordered pass
    // succeeds. Keep the full family off the small AmigaDOS stack. This also
    // avoids the m68k toolchain's incorrect word-member stack displacement
    // for a large automatic GameplayBob array's aggregate assignments.
    GameplayBob currencyPending_[GameplayWorld::gemCapacity]{};
    unsigned currencySlots_[GameplayWorld::gemCapacity]{};
};
#endif
