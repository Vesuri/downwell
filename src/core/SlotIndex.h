#ifndef DOWNWELL_CORE_SLOT_INDEX_H
#define DOWNWELL_CORE_SLOT_INDEX_H
#include <cstdint>
namespace downwell {
// Internal pool links include Capacity itself as the end sentinel. Resource
// IDs and generation counters are not pool indices and must remain full width.
template<unsigned Capacity,bool Compact=(Capacity<=0xffffu)> struct SlotIndex {
    using Type=std::uint32_t;
};
template<unsigned Capacity> struct SlotIndex<Capacity,true> {
    using Type=std::uint16_t;
};
}
#endif
