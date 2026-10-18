#ifndef DOWNWELL_CORE_GAME_H
#define DOWNWELL_CORE_GAME_H

#include "Input.h"
#include "Renderer.h"
#include "StartupRuntime.h"
#include "CurrencySmokeComposition.h"
#include "LevelNumberSurfaceDraw.h"
#include <cstdint>

namespace downwell {

class Game {
public:
    static constexpr int logicalWidth = 380;
    static constexpr int logicalHeight = StartupRuntime::surfaceHeight;
    static constexpr int playfieldX = (logicalWidth - StartupRuntime::surfaceWidth) / 2;
    static constexpr int ticksPerSecond = 60;

    explicit Game(const RoomPack* roomPack = nullptr,
                  const ObjectPack* objectPack = nullptr,
                  const BuilderPatternPack* builderPatternPack = nullptr,
                  const EffectMaskAssets* effectAssets = nullptr,
                  InstanceLifecycleBinding lifecycle = {},
                  bool deferStartupShortcut = false);

    // Resource-backed owners must attach before a shortcut executes Steps.
    // Idempotent so an explicit initialization cannot replay room creation.
    void applyStartupShortcut();
    bool startupShortcutApplied()const{return startupShortcutApplied_;}
    bool bindEncounterInput(EncounterInput* input){return startup_.bindEncounterInput(input);}
    bool boundEncounterInput(const EncounterInput* input)const{return startup_.boundEncounterInput(input);}
    void configureCollisionPass(bool (*pass)(void*,StartupRuntime&),void* context){
        collisionPass_=pass;collisionContext_=context;
    }
    template<class Pass,class Body> bool withCollisionPass(Pass pass,Body body){
        if(collisionPass_)return false; // one collision stream, never two passes
        collisionContext_=&pass;
        collisionPass_=[](void* p,StartupRuntime& world){return (*static_cast<Pass*>(p))(world);};
        struct Leave {Game& game;~Leave(){game.configureCollisionPass(nullptr,nullptr);}} leave{*this};
        return body();
    }
    void configureDitherAboveEnvironment(bool lowSpec,bool ending){
        startup_.configureDitherAboveEnvironment(lowSpec,ending);
    }
    template<class Audio,class Body> bool withRunAudio(Audio& audio,Body body){
        return startup_.withRunAudio(audio,body);
    }

    // Platform entropy for ResetPlayer/randomize; fixed default for replays.
    void update(const Input& input, std::uint32_t restartSeed = 2);
    void render(Renderer& renderer) const;
    bool finished() const { return finished_; }
    bool renderingSucceeded() const { return renderingSucceeded_; }
    bool simulationSucceeded() const { return simulationSucceeded_; }
    std::uint64_t tick() const { return tick_; }
    const StartupRuntime& startup() const { return startup_; }
    // Bind current renderer text state for an enclosing render, never replace
    // it with title-specific defaults. The actor owner is separately borrowed
    // through StartupRuntime::withLevelNumberDraw.
    template<class Render> bool withLevelNumberText(const FontGlyphMasks& glyphs,
        LevelNumberTextDrawState& state,Render render){
        if(titleGlyphs_||titleText_)return false;
        titleGlyphs_=&glyphs;titleText_=&state;
        const bool okay=render();titleGlyphs_=nullptr;titleText_=nullptr;
        return okay;
    }

private:
    void updatePrepared(const Input& input,std::uint32_t restartSeed);
    void renderMenu(Renderer& renderer) const;
    void renderPlayMenu(Renderer& renderer) const;
    void renderMain(Renderer& renderer) const;
    void renderLevelClear(Renderer& renderer) const;
    void renderLandscapeHud(Renderer& renderer) const;

    std::uint64_t tick_ = 0;
    StartupRuntime startup_;
    const ObjectPack* objectPack_ = nullptr;
    const EffectMaskAssets* effectAssets_ = nullptr;
    const FontGlyphMasks* titleGlyphs_=nullptr;
    LevelNumberTextDrawState* titleText_=nullptr;
    mutable CurrencySmokeComposition effects_;
    mutable bool renderingSucceeded_ = true;
    bool fire_ = false;
    bool left_ = false, right_ = false, heartGoal_ = false;
    bool finished_ = false;
    bool simulationSucceeded_ = true;
    bool startupShortcutApplied_ = false;
    bool (*collisionPass_)(void*,StartupRuntime&)=nullptr;
    void* collisionContext_=nullptr;
};

} // namespace downwell

#endif
