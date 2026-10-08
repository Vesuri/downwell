#include "Game.h"
#include "EntryDitherPlacement.h"
#include "WallDrawLookup.h"
#include "WallPresentation.h"
#include "TimeFieldOutlines.h"
#include "EnemyPresentation.h"
#include "HudNumbers.h"
#include "DamageNumberText.h"
#include "AmmoDividers.h"
#include "AmmoFill.h"
#include "DeathResultsBackdrop.h"
#include "DeathResultsRows.h"
#include "DeathResultsChoices.h"
#include "DeathResultsProgressBar.h"
#include "DeathResultsProgressLabels.h"
#include "ObjectPack.h"
#include "RunnerLayerBackgroundDraw.h"
#include "ShopFixtures.h"
#include "LaserSightLine.h"
#include "ShopMenuText.h"
#include "LevelClearLayout.h"
#include "../../generated/amiga-shop.text.h"
#include "../../generated/amiga-clear.text.h"
#include <array>
#include <cmath>
#include <string>
#include <cstring>
namespace downwell {
namespace {
void drawBorderText(Renderer& renderer, float x, float y, const char* text,
                    Color foreground) {
    const Color black{0, 0, 0, 255};
    renderer.drawText(2, text, x + 1.0F, y, black, TextAlign::center);
    renderer.drawText(2, text, x - 1.0F, y, black, TextAlign::center);
    renderer.drawText(2, text, x, y + 1.0F, black, TextAlign::center);
    renderer.drawText(2, text, x, y - 1.0F, black, TextAlign::center);
    renderer.drawText(2, text, x, y, foreground, TextAlign::center);
}

class OffsetRenderer final : public Renderer {
public:
    OffsetRenderer(Renderer& renderer, int x) : renderer_(renderer), x_(x) {}
    void clear(Color color) override { renderer_.clear(color); }
    void setClipRect(int x, int y, int width, int height) override {
        renderer_.setClipRect(x + x_, y, width, height);
    }
    void clearClipRect() override { renderer_.clearClipRect(); }
    void fillRect(int x, int y, int width, int height, Color color) override {
        renderer_.fillRect(x + x_, y, width, height, color);
    }
    void drawCoverageWord(int x,int y,std::uint16_t bits,Color color) override {
        renderer_.drawCoverageWord(x+x_,y,bits,color);
    }
    void drawSpriteTiled(std::int32_t id, std::uint32_t frame,
                         int x, int y) override {
        renderer_.drawSpriteTiled(id, frame, x + x_, y);
    }
    void drawSprite(std::int32_t id, std::uint32_t frame,
                    int x, int y) override {
        renderer_.drawSprite(id, frame, x + x_, y);
    }
    void drawSpriteTransformed(std::int32_t id, std::uint32_t frame,
                               float x, float y, float sx, float sy,
                               float rotation) override {
        renderer_.drawSpriteTransformed(id, frame, x + x_, y, sx, sy, rotation);
    }
    void drawText(std::int32_t id, const char* text, float x, float y,
                  Color color, TextAlign align) override {
        renderer_.drawText(id, text, x + x_, y, color, align);
    }
private:
    Renderer& renderer_;
    int x_;
};
} // namespace

Game::Game(const RoomPack* roomPack, const ObjectPack* objectPack,
           const BuilderPatternPack* builderPatternPack, const EffectMaskAssets* effectAssets,
           InstanceLifecycleBinding lifecycle, bool deferStartupShortcut)
    : startup_(PlatformProfile::desktop, roomPack, objectPack,
               StartupRuntime::defaultRandomSeed, builderPatternPack,lifecycle),
      objectPack_(objectPack), effectAssets_(effectAssets) {
    if (!deferStartupShortcut) applyStartupShortcut();
}

void Game::applyStartupShortcut() {
    if (startupShortcutApplied_) return;
    startupShortcutApplied_ = true;
#ifdef DOWNWELL_SKIP_TO_GAME
    startup_.skipToGameForTesting();
#ifdef DOWNWELL_SKIP_LEVEL
    // Exercise the real collision/alarms/wipe, without generating a whole descent.
    if (startup_.inMainRoom()) startup_.registerLevelExit(static_cast<int>(startup_.mainPlayerY()));
#endif
#elif defined(DOWNWELL_SKIP_INTRO)
    startup_.skipIntroForTesting();
#endif
}

void Game::update(const Input& input, std::uint32_t restartSeed) {
    const auto failed=[&] {
        if(simulationSucceeded_ && startup_.effectsSucceeded())return false;
        simulationSucceeded_=false;finished_=true;return true;
    };
    if(failed())return;
    finished_ = finished_ || input.quit;
    if (startup_.levelClearActive()) {
        bool confirm = input.fire;
        startup_.stepLevelClear(confirm, input.left && !left_, input.right && !right_, heartGoal_,
            [](LevelClearSelection::Effect, unsigned) {});
        if(failed())return;
        if (startup_.nextLevelRequested()) {
            startup_.loadNextAreaOneLevel(input);
            if (!startup_.roomLoadSucceeded()) finished_ = true;
        }
    } else {
        startup_.step(input, input.fire && !fire_);
        if(failed())return;
        if (startup_.levelResultRequested())
            startup_.enterLevelClear(startup_.playStyle(), heartGoal_);
    }
    if(failed())return;
    const auto action = startup_.deathResults().inputEffects.action;
    if (action != DeathMenuSequence::Action::None &&
        !startup_.restartFromResults(action, restartSeed)) finished_ = true;
    fire_ = input.fire;
    left_ = input.left; right_ = input.right;
    ++tick_;
}

void Game::renderLevelClear(Renderer& renderer) const {
    const auto real = [](HudReal value) {
        return std::ldexp(static_cast<double>(value.mantissa), value.exponent - 52);
    };
    const auto sample = [](double value) {
        if (value == 0) return HudReal{};
        std::uint64_t bits; std::memcpy(&bits, &value, sizeof bits);
        return HudReal{(bits & 0xfffffffffffffULL) | (1ULL << 52), int((bits >> 52) & 2047) - 1023};
    };
    const auto& room = startup_.levelClear();
    const auto& frame = startup_.levelClearFrame();
    struct Sink {
        Renderer& renderer;
        const StartupRuntime& runtime;
        decltype(real)& convert;
        int viewY;
        float coordinate(LevelClearCoordinate p) const {
            return static_cast<float>((p.negative ? -1 : 1) * convert(p.magnitude));
        }
        void sprite(unsigned id, unsigned subimage, LevelClearCoordinate x, LevelClearCoordinate y) {
            renderer.drawSpriteTransformed(id, subimage, coordinate(x), coordinate(y) - viewY, 1, 1, 0);
        }
        void player(LevelClearCoordinate x, LevelClearCoordinate y) {
            renderer.drawSpriteTransformed(runtime.mainPlayerSpriteId(), unsigned(runtime.mainPlayerImageIndex()),
                coordinate(x), coordinate(y) - viewY, runtime.mainPlayerFacing(), 1, 0);
        }
        void label(unsigned id, LevelClearCoordinate x, LevelClearCoordinate y, unsigned pen) {
            std::string text = levelClearLabel(id);
            renderer.drawText(2, text.c_str(), coordinate(x), coordinate(y) - viewY,
                pen == 1 ? Color{255,255,255,255} : Color{255,0,0,255}, TextAlign::center);
        }
        void description(unsigned id, LevelClearCoordinate x, LevelClearCoordinate y, unsigned suffix) {
            std::string text = levelClearLabel(id);
            if (suffix) text += " " + std::to_string(suffix);
            drawBorderText(renderer, coordinate(x), coordinate(y) - viewY, text.c_str(), {255,255,255,255});
        }
    } sink{renderer, startup_, real, room.motion.viewY()};
    drawLevelClear(sink, room, frame, startup_.upgrades(),
        sample(std::fabs(std::cos(frame.circle * (3.14159265358979323846 / 180.0)))),
        sample(std::fabs(std::sin(frame.time / 20.0))));
    const auto& dither = startup_.exitDither();
    if (dither.active) renderer.drawSprite(128, 0, dither.x, dither.y - room.motion.viewY());
}

void Game::render(Renderer& renderer) const {
    // GameMaker renders the 160x284 room surface into the centre of the
    // 380x284 PC application surface.  Everything outside it is the HUD
    // layer, whose untouched pen is true black.
    renderer.clear({0, 0, 0, 255});
    renderer.setClipRect(playfieldX, 0, StartupRuntime::surfaceWidth,
                         logicalHeight);
    OffsetRenderer playfield(renderer, playfieldX);
    bool clipReleased = false;
    if (startup_.levelClearActive()) {
        renderLevelClear(playfield);
    } else if (startup_.inMenu()) {
        renderMenu(playfield);
    } else if (startup_.inPlayMenu()) {
        renderPlayMenu(playfield);
    } else if (startup_.inMainRoom()) {
        renderMain(playfield);
    } else {
        const int stage = startup_.showSplash();
        if (stage == 2)
            playfield.drawSprite(696, 0, 80, 140); // sprDevolverLogo
        if (stage == 3) {
            renderer.clearClipRect();
            clipReleased = true;
            drawBorderText(renderer, playfieldX + 80.0F, 130.0F,
                           "a game by\nOJIRO FUMOTO", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX + 80.0F, 150.0F,
                           "@MOPPIN_", {255, 0, 0, 255});
            drawBorderText(renderer, playfieldX + 168.0F, 200.0F,
                           "EIRIK SUHRKE", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX + 168.0F, 210.0F,
                           "@STROTCHY", {255, 0, 0, 255});
            drawBorderText(renderer, playfieldX - 8.0F, 200.0F,
                           "JOONAS TURNER", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX - 8.0F, 210.0F,
                           "@KISSAKOLME", {255, 0, 0, 255});
        }
        if (stage == 4 || stage == 5)
            playfield.drawSprite(695, 0, 80, 140); // sprControls
        if (stage == 5 || stage == 6)
            playfield.drawSpriteTiled(733, startup_.splashDitherFrame(),
                                      -300, 0); // sprDitherFade
    }
    if (!clipReleased)
        renderer.clearClipRect();
    if (startup_.inMainRoom() && !startup_.levelClearActive())
        renderLandscapeHud(renderer);
    if (startup_.inMainRoom() && !startup_.levelClearActive() && startup_.exitDither().active) {
        renderer.setClipRect(playfieldX, 0, 160, logicalHeight);
        const auto& dither = startup_.exitDither();
        playfield.drawSprite(128, 0, dither.x - startup_.mainCameraRenderViewX(),
            dither.y - startup_.mainCameraRenderViewY());
        renderer.clearClipRect();
    }
    if (startup_.inMainRoom() && startup_.deathResults().active) {
        renderer.setClipRect(playfieldX, 0, 160, logicalHeight);
        struct Sink {
            Renderer& renderer;
            void choice(unsigned index, int xTenths, int y, unsigned pen) {
                const char* labels[] = {"RETRY", "STYLE SELECT", "SURFACE"};
                renderer.drawText(2, labels[index], xTenths / 10.0F, y,
                    pen == 2 ? Color{255,0,0,255} : Color{255,255,255,255}, TextAlign::left);
            }
            void sprite(int frame, int x, int y) { renderer.drawSprite(733, frame, x, y); }
            void asset(int id, unsigned frame, int x, int y) { renderer.drawSprite(id, frame, x, y); }
            void barRect(int x, int y, int w, int h, bool red) {
                renderer.fillRect(x, y, w, h, red ? Color{255,0,0,255} : Color{255,255,255,255});
            }
            void progressGlyph(unsigned code, int x, int y, unsigned pen) {
                const char digit[] = {static_cast<char>('0' + code), 0};
                const char* text = code == 11 ? "MAXED!" : code == 10 ? "/" : digit;
                const Color color = pen == 3 ? Color{0,0,0,255} :
                    pen == 2 ? Color{255,0,0,255} : Color{255,255,255,255};
                renderer.drawText(2, text, x, y, color, TextAlign::center);
            }
            void blackRect(int x, int y, int w, int h) {
                renderer.fillRect(x, y, w, h, {0, 0, 0, 255});
            }
            void title(int x, int y, bool black) {
                const std::uint8_t c = black ? 0 : 255;
                renderer.drawText(2, "GAME OVER", x, y, {c, c, c, 255}, TextAlign::center);
            }
        } sink{playfield};
        drawDeathResultsBackdrop(sink, startup_.deathResults().menu.titleOffset,
            startup_.mainCameraRenderViewX(), startup_.mainCameraRenderViewY());
        drawDeathResultsRows(sink, startup_.deathResults().menu,
            startup_.mainGameGemTotal(), startup_.mainEnemiesDefeated(), startup_.runTimeTicks(),
            startup_.runCombo().state.best, startup_.runStage());
        drawDeathResultsProgressLabels(sink, startup_.deathResults());
        DeathResultsProgressBar::from(startup_.deathResults()).draw(sink);
        drawDeathResultsChoices(sink, startup_.deathResults().menu);
        renderer.clearClipRect();
    }
}

void Game::renderMain(Renderer& renderer) const {
    const InstanceRecord* player = startup_.instance(100855);
    if (player == nullptr)
        return;

    const float viewX = static_cast<float>(startup_.mainCameraRenderViewX());
    const float viewY = static_cast<float>(startup_.mainCameraRenderViewY());
    const bool composite = effectAssets_ && !startup_.mainDeathSequence().active;
    if (effectAssets_) {
        using Real = MotionBinary64;
        const EffectSurfacePlacement::View view{Real::fromInteger(int(viewX)),Real::fromInteger(int(viewY)),
            Real::fromInteger(160),Real::fromInteger(284)};
        CurrencySmokeComposition::Settings settings;
        bool shadesSucceeded=true;
        settings.death = startup_.mainDeathSequence().active;
        settings.noControl = startup_.shops().noControl;
        renderingSucceeded_ = effects_.prepareWithSurfaceProducers([&](auto emit) {
            for (std::size_t i = startup_.mainCurrencyCount(); i; --i) {
                const auto& gem = startup_.mainCurrencyAt(i-1);
                if (gem.motion.lifetime.visible())
                    emit(gem.large,gem.motion.animation.frame(),Real::fromInteger(int(gem.x)),Real::fromInteger(int(gem.y)));
            }
        }, startup_.smokeEffects(), *effectAssets_, view, settings, [&](auto emit) {
            return startup_.visitDebrisDraw([&](const DebrisAnimation& effect){
                return DebrisDraw::surface(effect,*effectAssets_,view,emit);
            });
        }, [&](auto emit) {
            return startup_.visitEntryDitherDraw([&](InstanceHandle,const InstanceRecord& record,int depth){
                // Compact original-art mask only supports its verified default
                // transform/depth and centered integer viewport placement.
                if(depth!=-200000||record.spriteId!=129||record.scaleX!=1||record.scaleY!=1||record.rotation!=0)
                    return false;
                RunnerImageValue x,y;std::memcpy(&x.bits,&record.preciseX,4);std::memcpy(&y.bits,&record.preciseY,4);
                EntryDitherPlacement placement;
                if(!EntryDitherPlacement::resolve(x,y,view.x,view.y,placement))return false;
                EffectMaskAssets::Asset mask;
                return effectAssets_->entryDither(80,placement.y,mask)&&emit(mask,view.x,view.y);
            });
        }, [&](auto emit) {
            for (std::size_t i=0;i<startup_.activeInstanceCount();++i) {
                const auto& instance=startup_.instanceAt(i);
                if (instance.active && enemyPresentationSprite(instance.objectId,instance.grounded,instance.auxiliaryState)>=0)
                    emit(Real::fromInteger(instance.x),Real::fromInteger(instance.y));
            }
            shadesSucceeded=startup_.visitRegisteredEnemyShades([&](Real x,Real y){emit(x,y);return true;});
        }, [](Real n,std::int32_t& out){return n.roundPixel(out);});
        renderingSucceeded_=renderingSucceeded_&&shadesSucceeded;
        if (!renderingSucceeded_) return;
    }
    // Direct currency is depth 10000, behind retained wall patches (100) and
    // ordinary walls (0). Surface-composited currency is drawn later.
    for (std::size_t index = 0; !composite && index < startup_.mainCurrencyCount(); ++index) {
        const auto& gem=startup_.mainCurrencyAt(index);
        if(gem.motion.lifetime.visible())renderer.drawSprite(gem.large?134:133,gem.motion.animation.frame(),
            static_cast<int>(gem.x-viewX),static_cast<int>(gem.y-viewY));
    }
    renderingSucceeded_=startup_.wallBackgroundTiles().paintRegion(
        MotionBinary64::fromInteger(int(viewX)),MotionBinary64::fromInteger(int(viewY)),0,0,160,284,
        [](MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);},
        [&](int x,int y,int w,int h){renderer.fillRect(x,y,w,h,{0,0,0,255});return true;});
    if(!renderingSucceeded_)return;
    WallDrawLookup<StartupRuntime::instanceCapacity> walls;
    bool wallsBuilt = false;
    const auto hasWallAt = [this,&walls,&wallsBuilt](std::int32_t x, std::int32_t y) {
        // Production walls retain their End Step-selected frames. Only the
        // legacy fixture path needs adjacency; do not sort the entire growing
        // level every frame when nobody will consult the lookup.
        if (!wallsBuilt) {
            walls.rebuild(startup_.activeInstanceCount(), [this](std::size_t i) -> const InstanceRecord& {
                return startup_.instanceAt(i);
            });
            wallsBuilt = true;
        }
        return walls.contains(x, y);
    };

    // Retain event-selected frames, including authored pillar remapping.
    for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
        const InstanceRecord& instance = startup_.instanceAt(index);
        if(!drawWallPresentation(instance,[&]{
        std::uint32_t frame = 0;
        frame |= hasWallAt(instance.x, instance.y - 16) ? 1U : 0U;
        frame |= hasWallAt(instance.x + 16, instance.y) ? 2U : 0U;
        frame |= hasWallAt(instance.x, instance.y + 16) ? 4U : 0U;
        frame |= hasWallAt(instance.x - 16, instance.y) ? 8U : 0U;
        return frame;
        },[&](int sprite,unsigned frame){
        renderer.drawSpriteTransformed(sprite, frame,
                                       instance.x - viewX,
                                       instance.y - viewY,
                                       1.0F, 1.0F, 0.0F);
        })){renderingSucceeded_=false;return;}
    }

    TimeFieldOutlines field;
    field.set(startup_.timeFields(), startup_.goalField(), startup_.levelExitPresent(),
              startup_.goalFieldBirthBoundary(),
              static_cast<int>(viewX), static_cast<int>(viewY),0,startup_.startTimeField());

    // Enemy Step has already selected its presentation, including hit/wake
    // overrides. Drawing must not reconstruct a default pose from object ID.
    for (unsigned pass = 0; pass < 2; ++pass) {
      if (pass == 1) {
        drawShopFixtures(startup_.shops(), [&](int sprite, int frame, int x, int y) {
            renderer.drawSprite(sprite, frame, x - static_cast<int>(viewX), y - static_cast<int>(viewY));
        });
        field.draw([&](int x, int y, int pen) {
            if (x >= 0 && x < 160 && y >= 0 && y < 284)
                renderer.fillRect(x, y, 1, 1, pen == 1 ? Color{255,255,255,255} : Color{255,0,0,255});
        });
        continue; // Registered actors are merged in room Draw order below.
      }
      for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
        const InstanceRecord& instance = startup_.instanceAt(index);
        if (!instance.active)
            continue;
        const bool enemy = enemyPresentationSprite(instance.objectId,
            instance.grounded, instance.auxiliaryState) >= 0;
        if (enemy != (pass == 1)) continue;
        std::int32_t sprite = -1;
        if (instance.objectId == 93) sprite = 51;
        if (instance.objectId == 92) sprite = 51;
        if (instance.objectId == 289) sprite = 550;
        if (instance.objectId == 293) sprite = 569;
        if (instance.objectId == 296) sprite = 568;
        if (instance.objectId == 109) sprite = 139;
        // objLineToResult (306) is an invisible collision trigger, not art.
        if (enemy)
            sprite = instance.spriteId;
        if (sprite >= 0)
            renderer.drawSpriteTransformed(
                sprite, static_cast<std::uint32_t>(instance.imageIndex),
                instance.x - viewX, instance.y - viewY,
                instance.scaleX, instance.scaleY, instance.rotation);
      }
    }
    drawShopKeepers(startup_.shops(), [&](int sprite, int frame, int x, int y) {
        renderer.drawSprite(sprite, frame, x - static_cast<int>(viewX), y - static_cast<int>(viewY));
    });
    drawShopPrompts(startup_.shops(), false, [&](int sprite, int frame, int x, int y) {
        renderer.drawSprite(sprite, frame, x - static_cast<int>(viewX), y - static_cast<int>(viewY));
    });
    bool foregroundDrawn=false;
    const auto drawForeground=[&](){
        if(foregroundDrawn)return true;
        foregroundDrawn=true;
        return startup_.visitForegroundEffects([&](const CorpseExpiryEffect& effect){
            float x,y;
            std::memcpy(&x,&effect.x.bits,sizeof x);std::memcpy(&y,&effect.y.bits,sizeof y);
            renderer.drawSpriteTransformed(effect.spriteId,effect.drawFrame(),x-viewX,y-viewY,1,1,0);
            return true;
        },[&](const PlayerDamageNumber& effect){
            DamageNumberText label;if(!label.format(effect.amount))return false;
            struct Draw {
                Renderer& renderer;const char* label;float viewX,viewY;unsigned pen=0;
                void align(int){}
                void color(unsigned value){pen=value;}
                bool text(MotionBinary64 x,MotionBinary64 y,MotionBinary64){
                    RunnerImageValue px,py;if(!px.assign(x)||!py.assign(y))return false;
                    float fx,fy;std::memcpy(&fx,&px.bits,4);std::memcpy(&fy,&py.bits,4);
                    renderer.drawText(2,label,fx-viewX,fy-viewY,Color{std::uint8_t(pen),0,0,255},TextAlign::center);
                    return true;
                }
            } draw{renderer,label.text,viewX,viewY};
            return effect.draw(draw);
        });
    };
    const auto drawBullet=[&](const StartupRuntime::MainBulletState& bullet){
        renderer.drawSpriteTransformed(machineGunProfile(bullet.gemHigh).sprite, bullet.animation.frame(),
                            static_cast<int>(bullet.x - viewX),
                            static_cast<int>(bullet.y - viewY), 1.0F, 1.0F,
                            static_cast<float>(bullet.trajectory.direction * (360.0 / 4294967296.0)));
    };
    bool unregisteredBulletsDrawn=false;
    const auto drawUnregisteredBullets=[&]{
        if(unregisteredBulletsDrawn)return;
        unregisteredBulletsDrawn=true;
        // Explicit resource-less fixtures have no projectile records. Keep
        // their fallback behind the foreground bank and player as before.
        for(unsigned i=0;i<startup_.mainBulletCount();++i){
            const auto& bullet=startup_.mainBulletAt(i);
            if(!bullet.instance.generation)drawBullet(bullet);
        }
    };
    const auto drawPlayer=[&]{
        if (startup_.mainPlayerVisible())
            renderer.drawSpriteTransformed(
                startup_.mainPlayerSpriteId(),
                static_cast<std::uint32_t>(startup_.mainPlayerImageIndex()),
                player->x - viewX, player->y - viewY,
                static_cast<float>(startup_.mainPlayerFacing()), 1.0F, 0.0F);
        // playerDraw places the ray outside dFlash but inside the player's
        // Draw event/layer. noDraw's Cat interaction is not yet implemented.
        if (startup_.upgrades().levels[12] && !startup_.goalStopped()) {
            const auto line = LaserSightLine::from(
                LaserSightRay::fromQ8(static_cast<std::int32_t>(startup_.mainPlayerX()*256)),
                LaserSightRay::fromQ8(static_cast<std::int32_t>(startup_.mainPlayerY()*256)),
                startup_.mainLaserSightEndpoint(), static_cast<int>(viewX), static_cast<int>(viewY));
            line.draw([&](int x, int y) {
                renderer.fillRect(x, y, 1, 1, {255, 0, 0, 255});
            });
        }
    };
    renderingSucceeded_=startup_.visitDrawAtDepthChecked([&](InstanceHandle key,const InstanceRecord& instance,int depth){
        if(!instance.active||instance.destroyRequested)return true;
        if(depth < -1000){drawUnregisteredBullets();if(!drawForeground())return false;}
        if(instance.objectId==35){drawPlayer();return true;}
        if(instance.objectId==370){
            if(composite)return true;
            const auto* effect=startup_.debrisForDraw(key,instance);
            DebrisDraw::Command command;unsigned frame;
            const EffectSurfacePlacement::View view{};
            if(!effect||!DebrisDraw::prepare(*effect,view,true,false,false,command))return false;
            if(command.placement.target==EffectSurfacePlacement::hidden)return true;
            if(!DebrisDraw::frame(command.sprite,command.imageIndex,frame))return false;
            float x,y;std::memcpy(&x,&effect->x.bits,4);std::memcpy(&y,&effect->y.bits,4);
            renderer.drawSpriteTransformed(command.sprite,frame,x-viewX,y-viewY,1,1,0);
            return true;
        }
        if(instance.objectId==261){
            for(unsigned i=0;i<startup_.mainBulletCount();++i){
                const auto& bullet=startup_.mainBulletAt(i);
                if(bullet.instance.slot==key.slot&&bullet.instance.generation==key.generation){drawBullet(bullet);return true;}
            }
            return false;
        }
        const bool effect=instance.objectId==219||instance.objectId==220;
        const bool registeredEnemy=instance.objectId==269||instance.objectId==120;
        if(effect||registeredEnemy||enemyPresentationSprite(instance.objectId,instance.grounded,instance.auxiliaryState)>=0){
            if(instance.spriteId<0)return true;
            renderer.drawSpriteTransformed(instance.spriteId,static_cast<unsigned>(instance.imageIndex),
                (effect||registeredEnemy?instance.preciseX:float(instance.x))-viewX,
                (effect||registeredEnemy?instance.preciseY:float(instance.y))-viewY,
                instance.scaleX,instance.scaleY,instance.rotation);
        }
        return true;
    });
    if(!renderingSucceeded_)return;
    drawUnregisteredBullets();
    if(!drawForeground()){renderingSucceeded_=false;return;}
    // __global_object_depths: fxFlashBall is -200000, in front of the
    // player (-50000), ordinary projectiles (0) and currency (10000).
    // It draws directly; the controller's effect surfaces follow at -300000.
    startup_.flashEffects().visitNewest([&](const FlashBallEffect& effect) {
        renderer.drawSprite(96,effect.drawFrame(),effect.pixelX()-static_cast<int>(viewX),
                            effect.pixelY()-static_cast<int>(viewY));
    });
    if (effectAssets_) effects_.surface().visit([&](unsigned x,unsigned y,std::uint16_t a,std::uint16_t lo,std::uint16_t hi) {
        const std::uint16_t masks[4]={std::uint16_t(a&~lo&~hi),std::uint16_t(a&lo&~hi),
            std::uint16_t(a&~lo&hi),std::uint16_t(a&lo&hi)};
        const Color colors[4]={{0,0,0,255},{255,255,255,255},{255,0,0,255},{0,0,0,255}};
        for(unsigned pen=0;pen<4;++pen)if(masks[pen])renderer.drawCoverageWord(x,y,masks[pen],colors[pen]);
    });
    if (startup_.mainDeathSequence().flashVisible())
        renderer.fillRect(0, 0, 160, logicalHeight, {255, 255, 255, 255});
    else if (!startup_.deathResults().active) {
        struct Sink {
            Renderer& renderer;
            void asset(int id, unsigned frame, int x, int y) { renderer.drawSprite(id, frame, x, y); }
            void arrow(int phase, int x, int y, int angle) {
                renderer.drawSpriteTransformed(484, phase < 0 ? 0 : phase / 4,
                    x, y, 1, 1, angle);
            }
            void mask(int id, int x, int y, unsigned pen) {
                const char* label = shopMenuLabel(id);
                int height = 10;
                for (const char* c = label; *c; ++c) if (*c == '\n') height += 12;
                renderer.drawText(2, label, x, y - height / 2,
                    pen == 3 ? Color{0,0,0,255} : pen == 2 ? Color{255,0,0,255} : Color{255,255,255,255},
                    TextAlign::center);
            }
            void text(ShopMenuText kind, unsigned value, int x, int y, bool red) {
                drawShopMenuText(*this, kind, value, x, y, red);
            }
        } sink{renderer};
        for (const auto& shop : startup_.shops().instances)
            drawShopMenu(shop, int(viewX), int(viewY), logicalHeight,
                         startup_.mainCurrencyTotal(), sink);
    }
}

void Game::renderLandscapeHud(Renderer& renderer) const {
    constexpr int origin = playfieldX;
    renderer.drawSprite(408, 0, origin - 2, 0);
    renderer.drawSpriteTransformed(408, 0, origin + 162, 0,
                                   -1.0F, 1.0F, 0.0F);

    // Recovered scrDrawHud4x3 coordinates use the playfield's left edge as
    // zero, so its negative left HUD and positive right HUD land naturally in
    // the 110-pixel side regions after adding origin.
    renderer.drawSprite(415, 1, origin - 102, 6);
    renderer.fillRect(origin - 98, 11,
                      80 * startup_.mainPlayerHealth() / startup_.mainPlayerHealthMaximum(), 6,
                      {255, 0, 0, 255});
    const auto& reserve = startup_.mainHealthReserve();
    drawHeartPieceMeter(origin - 102, 6, reserve.pieces, reserve.piecesPerHeart,
        [&](int x, int y, int w, int h) { renderer.fillRect(x, y, w, h, {255, 255, 255, 255}); },
        [&](unsigned sprite, unsigned frame, int x, int y) { renderer.drawSprite(sprite, frame, x, y); });
    renderer.drawSprite(415, 0, origin - 102, 6);
    drawHudHealthNumber(renderer, origin - 58, 15,
                        startup_.mainPlayerHealth(), startup_.mainPlayerHealthMaximum());

    constexpr int gaugeX = origin + 174;
    const int meterOffset = static_cast<int>(startup_.mainAmmoMeterOffset());
    const int gaugeTop = 43 + meterOffset;
    renderer.drawSprite(416, 0, gaugeX, gaugeTop);
    drawAmmoFill(startup_.mainAmmo(), startup_.mainAmmoCapacity(), gaugeX + 5,
        [&](int x, unsigned y, unsigned w, unsigned h) {
            renderer.fillRect(x, y, w, h, {255, 255, 255, 255});
        });
    startup_.mainAmmoGauge().draw(gaugeX + 4, 229 + 4 * meterOffset,
        [&](unsigned sprite, unsigned frame, int x, int y) { renderer.drawSprite(sprite, frame, x, y); });
    drawAmmoDividers(startup_.mainAmmo(), startup_.mainAmmoCapacity(), 1,
        gaugeX + 4, 229 + 4 * meterOffset,
        [&](unsigned sprite, unsigned frame, int x, unsigned y) { renderer.drawSprite(sprite, frame, x, y); });
    renderer.drawSprite(416, 1, gaugeX, gaugeTop);
    const int ammoOffset = static_cast<int>(startup_.mainAmmoHudOffset());
    drawHudAmmoNumber(renderer, gaugeX + 6 + ammoOffset, 239 + ammoOffset, startup_.mainAmmo());

    constexpr int currencyX = origin + 252;
    renderer.drawSprite(376, 0, currencyX, 18);
    drawHudCurrencyNumber(renderer, currencyX - 8, 18,
                          startup_.mainCurrencyTotal());

    if (startup_.mainGemStreak() > 0 && !startup_.mainDeathSequence().active) {
        std::int32_t streakWorldY=0;
        if(!startup_.drawGemStreakRise(streakWorldY))return;
        const int streakY=streakWorldY-startup_.mainCameraRenderViewY();
        const std::string streak = std::to_string(startup_.mainGemStreak());
        renderer.drawSprite(133, 0, origin + 65, streakY+5);
        drawGemmyTextLayers(origin + 81, streakY+6, [&](int x, int y, Color color) {
            renderer.drawText(2, streak.c_str(), x, y, color, TextAlign::center);
        });
    }
}

void Game::renderMenu(Renderer& renderer) const {
    if (objectPack_ == nullptr)
        return;

    const float viewX = startup_.menuCameraViewX();
    const float viewY = startup_.menuCameraViewY();

    const auto drawInstance = [&](InstanceHandle, const InstanceRecord& instance) {
        PackedObject object;
        if (!instance.active || instance.destroyRequested ||
            instance.objectId == 382 || // ditherFade draws a captured surface
            instance.objectId == 69 ||  // EnterSequence has a custom Draw event
            !objectPack_->findObject(instance.objectId, object) ||
            !object.visible() || object.spriteId < 0)
            return true;

        const bool isPlayer = instance.objectId == 35;
        // playerDraw() suppresses the sprite while the well teleporter holds
        // goalStop, before EnterSequence scrolls the camera into the shaft.
        if (isPlayer && !startup_.menuPlayerVisible())
            return true;

        float roomX = static_cast<float>(instance.x);
        float roomY = static_cast<float>(instance.y);
        if (instance.objectId == 312) { // objBgNightsky Step: parallax view follow
            roomX = std::floor(120.0F + viewX / 1.2F);
            roomY = std::floor((viewY + logicalHeight / 2.0F) / 1.1F);
        }
        std::int32_t spriteId = isPlayer
                                    ? startup_.menuPlayerSpriteId()
                                    : (instance.spriteId >= 0
                                           ? instance.spriteId
                                           : object.spriteId);
        if (instance.objectId == 60) // objWall_n Create selects wallTile
            spriteId = 76; // global.levelTile[0] / sprTileSurface
        const float runtimeImageIndex = isPlayer
                                            ? startup_.menuPlayerImageIndex()
                                            : instance.imageIndex;
        const float imageIndex = runtimeImageIndex < 0.0F
                                     ? 0.0F : runtimeImageIndex;
        if (instance.objectId == 315) { // objWell Draw: black shaft behind sprite
            renderer.fillRect(static_cast<int>(roomX - 24.0F - viewX),
                              static_cast<int>(roomY + 64.0F - viewY),
                              160, 1000, {0, 0, 0, 255});
        }
        renderer.drawSpriteTransformed(
            spriteId, static_cast<std::uint32_t>(imageIndex),
            roomX - viewX, roomY - viewY,
            instance.scaleX * (isPlayer ? startup_.menuPlayerFacing() : 1),
            instance.scaleY, instance.rotation);
        return true;
    };
    // rmMenu uses the runner's positive-layer-count Draw branch, not the
    // legacy room list. Keep translated object Draw bodies at their owned
    // element positions, with the room background in the same traversal.
    renderingSucceeded_ = startup_.visitSourceDrawChecked(
        [&](InstanceHandle key, const InstanceRecord& record, int, RunnerLayerDraw::Action) {
            return drawInstance(key, record);
        },
        [&](const RunnerLayerBackground& background, int) {
            return RunnerLayerBackgroundDraw::visit(background,
                [](int spriteId, bool& exists) {
                    // rmMenu has a solid background. Sprite backgrounds need
                    // a resource-backed tiling/stretch adapter, not a clear.
                    exists = false;
                    return spriteId == -1;
                },
                [&](std::uint32_t color) {
                    renderer.fillRect(0, 0, StartupRuntime::surfaceWidth, logicalHeight,
                        {static_cast<std::uint8_t>(color), static_cast<std::uint8_t>(color >> 8),
                         static_cast<std::uint8_t>(color >> 16), static_cast<std::uint8_t>(color >> 24)});
                    return true;
                },
                [](const RunnerLayerBackground&) { return false; });
        });
}

void Game::renderPlayMenu(Renderer& renderer) const {
    renderer.clear({0, 0, 0, 255});
    const float yOffset = startup_.playMenuYOffset();
    const float centerY = logicalHeight / 2.0F + yOffset;
    const float smoothX = startup_.playMenuSmoothX();
    constexpr std::int32_t styleRuns[] = {27, 25, 26, 28, 29};

    if (!startup_.playMenuSelected()) {
        for (int style = 0; style < 5; ++style) {
            const float drawX = 80.0F + style * 48.0F - smoothX;
            const float distance = drawX - 80.0F;
            const float drawY = centerY - 14.0F - distance * distance / 200.0F;
            // Only the default style is unlocked in the no-save reference state.
            const std::int32_t sprite = style == 0 ? styleRuns[style] : 43;
            const std::uint32_t frame = style == startup_.playMenuCursor()
                                            ? static_cast<std::uint32_t>(
                                                  startup_.playMenuRunFrame())
                                            : 0U;
            renderer.drawSpriteTransformed(sprite, frame, drawX, drawY,
                                           1.0F, 1.0F, 0.0F);
        }

        const int tileY = static_cast<int>(startup_.playMenuTileY());
        for (int row = 0; row <= 36; ++row) {
            renderer.drawSprite(51, 13, 0, tileY + row * 16);
            renderer.drawSprite(51, 7, 160, tileY + row * 16);
        }
        if (startup_.playMenuCursor() < 4)
            renderer.drawSprite(44, 0, 96, static_cast<int>(centerY - 10.0F));
        if (startup_.playMenuCursor() > 0)
            renderer.drawSpriteTransformed(44, 0, 64.0F, centerY - 10.0F,
                                           -1.0F, 1.0F, 0.0F);

        constexpr const char* styleNames[] = {
            "USUAL STYLE", "ARM SPIN STYLE", "BOULDER STYLE",
            "LEVITATE STYLE", "HANDSTAND STYLE"
        };
        constexpr const char* styleDescriptions[] = {
            "standard", "only finds\ngun modules\n\nshops are rare",
            "starts with\nmore HP\n\nless upgrade\noptions", "floaty",
            "no upgrades\n\ncheaper\nshop items"
        };
        const int cursor = startup_.playMenuCursor();
        const float titleY = centerY - 70.0F;
        renderer.drawText(2, "STYLE SELECT", 80.0F, titleY + 1.0F,
                          {255, 0, 0, 255}, TextAlign::center);
        renderer.drawText(2, "STYLE SELECT", 80.0F, titleY,
                          {255, 255, 255, 255}, TextAlign::center);
        const float textY = centerY + 10.0F;
        const char* styleName = cursor == 0 ? styleNames[cursor] : "LOCKED!";
        renderer.drawText(2, styleName, 80.0F, textY + 1.0F,
                          {255, 0, 0, 255}, TextAlign::center);
        renderer.drawText(2, styleName, 80.0F, textY,
                          {255, 255, 255, 255}, TextAlign::center);
        if (cursor == 0)
            renderer.drawText(2, styleDescriptions[cursor], 80.0F,
                              textY + 22.0F, {255, 255, 255, 255},
                              TextAlign::center);
    } else {
        renderer.drawSprite(
            styleRuns[startup_.playStyle()],
            static_cast<std::uint32_t>(startup_.playMenuRunFrame()), 80,
            static_cast<int>(centerY - 14.0F +
                             startup_.playMenuPlayerDescent()));
    }
    renderer.drawSprite(129, 0, 80,
                        static_cast<int>(startup_.playMenuDitherY()));
    if (startup_.playMenuSelected())
        renderer.drawSprite(128, 0, 80,
                            static_cast<int>(
                                startup_.playMenuVerticalDitherY()));
}

} // namespace downwell
