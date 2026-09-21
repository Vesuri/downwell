#ifndef DOWNWELL_CORE_GAME_H
#define DOWNWELL_CORE_GAME_H

#include "Input.h"
#include "Renderer.h"
#include <cstdint>

namespace downwell {

class Game {
public:
    static constexpr int logicalWidth = 320;
    static constexpr int logicalHeight = 568;
    static constexpr int ticksPerSecond = 60;

    void update(const Input& input);
    void render(Renderer& renderer) const;
    bool finished() const { return finished_; }
    std::uint64_t tick() const { return tick_; }

private:
    std::uint64_t tick_ = 0;
    int markerX_ = logicalWidth / 2;
    bool fire_ = false;
    bool finished_ = false;
};

} // namespace downwell

#endif

