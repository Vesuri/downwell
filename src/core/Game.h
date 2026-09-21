#ifndef DOWNWELL_CORE_GAME_H
#define DOWNWELL_CORE_GAME_H

#include "Input.h"
#include "Renderer.h"
#include "StartupRuntime.h"
#include <cstdint>

namespace downwell {

class Game {
public:
    static constexpr int logicalWidth = StartupRuntime::surfaceWidth;
    static constexpr int logicalHeight = StartupRuntime::surfaceHeight;
    static constexpr int ticksPerSecond = 60;

    explicit Game(const RoomPack* roomPack = nullptr,
                  const ObjectPack* objectPack = nullptr,
                  const BuilderPatternPack* builderPatternPack = nullptr);

    void update(const Input& input);
    void render(Renderer& renderer) const;
    bool finished() const { return finished_; }
    std::uint64_t tick() const { return tick_; }
    const StartupRuntime& startup() const { return startup_; }

private:
    void renderMenu(Renderer& renderer) const;
    void renderPlayMenu(Renderer& renderer) const;
    void renderMain(Renderer& renderer) const;

    std::uint64_t tick_ = 0;
    StartupRuntime startup_;
    const ObjectPack* objectPack_ = nullptr;
    bool fire_ = false;
    bool finished_ = false;
};

} // namespace downwell

#endif
