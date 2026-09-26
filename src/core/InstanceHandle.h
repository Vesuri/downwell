#ifndef DOWNWELL_CORE_INSTANCE_HANDLE_H
#define DOWNWELL_CORE_INSTANCE_HANDLE_H
#include <cstdint>
namespace downwell {
// Store-local identity, independent of compacted record offsets. Generation
// protects deferred consumers from a retired slot being reused for a new birth.
struct InstanceHandle {
    std::uint32_t slot = 0xffffffffu;
    std::uint32_t generation = 0;
};
}
#endif
