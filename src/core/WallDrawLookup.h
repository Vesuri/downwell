#ifndef DOWNWELL_CORE_WALL_DRAW_LOOKUP_H
#define DOWNWELL_CORE_WALL_DRAW_LOOKUP_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

namespace downwell {
// Presentation-only membership, rebuilt from the current active records.
// This is NOT the runner's collision index and must not select instances.
template<std::size_t Capacity>
class WallDrawLookup {
public:
    template<class RecordAt>
    void rebuild(std::size_t count, RecordAt at) {
        assert(count <= Capacity);
        size_ = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const auto& record = at(i);
            if (record.active && record.objectId == 60)
                positions_[size_++] = {record.x, record.y};
        }
        std::sort(positions_.begin(), positions_.begin() + size_);
    }
    bool contains(std::int32_t x, std::int32_t y) const {
        return std::binary_search(positions_.begin(), positions_.begin() + size_,
                                 std::make_pair(x, y));
    }
private:
    std::array<std::pair<std::int32_t, std::int32_t>, Capacity> positions_;
    std::size_t size_ = 0;
};
}
#endif
