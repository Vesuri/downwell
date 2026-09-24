#ifndef DOWNWELL_CORE_GAME_H
#define DOWNWELL_CORE_GAME_H

#include "Input.h"
#include "Renderer.h"
#include "StartupRuntime.h"
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
                  const BuilderPatternPack* builderPatternPack = nullptr);

    // Platform entropy for ResetPlayer/randomize; fixed default for replays.
    void update(const Input& input, std::uint32_t restartSeed = 2);
    void render(Renderer& renderer) const;
    bool finished() const { return finished_; }
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
    bool fire_ = false;
    bool left_ = false, right_ = false, heartGoal_ = false;
    bool finished_ = false;
};

} // namespace downwell

#endif
