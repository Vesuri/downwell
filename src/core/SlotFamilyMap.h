#ifndef DOWNWELL_CORE_SLOT_FAMILY_MAP_H
#define DOWNWELL_CORE_SLOT_FAMILY_MAP_H
#include <cstdint>
namespace downwell {
// Immutable routing metadata, constructed by the compiler, not a room walk.
// Lookup work is independent of the number/order of unrelated families.
template<unsigned Slots, unsigned Families> struct SlotFamilyMap {
    static_assert(Slots <= 65535 && Families < 255, "routing metadata bounds");
    struct Range { unsigned first, last, family; };
    std::uint8_t families[Slots]{};
    std::uint16_t bases[Families]{};
    bool valid = true;
    constexpr SlotFamilyMap(const Range (&ranges)[Families]) {
        bool seen[Families]{};
        for (unsigned i=0; i<Slots; ++i) families[i]=255;
        for (const auto& r:ranges) {
            if (r.first>=r.last || r.last>Slots || r.family>=Families) { valid=false; continue; }
            if (seen[r.family]) valid=false;
            seen[r.family]=true; bases[r.family]=static_cast<std::uint16_t>(r.first);
            for (unsigned i=r.first; i<r.last; ++i) {
                if (families[i]!=255) valid=false;
                families[i]=static_cast<std::uint8_t>(r.family);
            }
        }
        for (unsigned i=0; i<Slots; ++i) if (families[i]==255) valid=false;
    }
};
}
#endif
