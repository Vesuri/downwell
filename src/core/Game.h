#ifndef DOWNWELL_CORE_GAME_H
#define DOWNWELL_CORE_GAME_H

#include "Input.h"
#include "Renderer.h"
#include "StartupRuntime.h"
#include "CurrencySmokeComposition.h"
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
    void configureCollisionPass(bool (*pass)(void*,StartupRuntime&),void* context){
        collisionPass_=pass;collisionContext_=context;
    }

    // Platform entropy for ResetPlayer/randomize; fixed default for replays.
    void update(const Input& input, std::uint32_t restartSeed = 2);
    void render(Renderer& renderer) const;
    bool finished() const { return finished_; }
    bool renderingSucceeded() const { return renderingSucceeded_; }
    bool simulationSucceeded() const { return simulationSucceeded_; }
    std::uint64_t tick() const { return tick_; }
    const StartupRuntime& startup() const { return startup_; }

private:
    void renderMenu(Renderer& renderer) const;
    void renderPlayMenu(Renderer& renderer) const;
    void renderMain(Renderer& renderer) const;
    void renderLevelClear(Renderer& renderer) const;
    void renderLandscapeHud(Renderer& renderer) const;

    std::uint64_t tick_ = 0;
    StartupRuntime startup_;
    const ObjectPack* objectPack_ = nullptr;
    const EffectMaskAssets* effectAssets_ = nullptr;
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
