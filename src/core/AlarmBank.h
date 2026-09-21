#ifndef DOWNWELL_CORE_ALARM_BANK_H
#define DOWNWELL_CORE_ALARM_BANK_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

// GameMaker exposes twelve built-in alarms per instance. Inactive alarms are
// -1; positive values count down once per simulation step and dispatch on zero.
class AlarmBank {
public:
    static constexpr std::size_t count = 12;
    static constexpr std::int32_t inactive = -1;
    using FiredMask = std::uint16_t;
    static constexpr FiredMask allEvents = (1U << count) - 1;

    AlarmBank() { reset(); }

    void reset() { values_.fill(inactive); }
    std::int32_t get(std::size_t index) const { return values_[index]; }
    void set(std::size_t index, std::int32_t value) { values_[index] = value; }

    FiredMask advance(FiredMask implementedEvents = allEvents) {
        FiredMask fired = 0;
        for (std::size_t index = 0; index < count; ++index) {
            const FiredMask eventBit = static_cast<FiredMask>(1U << index);
            if ((implementedEvents & eventBit) == 0 || values_[index] < 0)
                continue;
            if (values_[index] == 0) {
                values_[index] = inactive;
                continue;
            }
            --values_[index];
            if (values_[index] == 0) {
                fired |= eventBit;
            }
        }
        return fired;
    }

    static bool fired(FiredMask mask, std::size_t index) {
        return (mask & static_cast<FiredMask>(1U << index)) != 0;
    }

private:
    std::array<std::int32_t, count> values_;
};

} // namespace downwell

#endif
