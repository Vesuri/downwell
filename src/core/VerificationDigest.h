#ifndef DOWNWELL_CORE_VERIFICATION_DIGEST_H
#define DOWNWELL_CORE_VERIFICATION_DIGEST_H
#include "StateDigest.h"

namespace downwell {
// Verification-only call boundaries prevent GCC/LTO from accumulating many
// inlined volatile FNV temporaries on the small Amiga task stack. The underlying
// hash algorithm, byte order and resulting digest are exactly StateDigest's.
class VerificationDigest : public StateDigest {
public:
    [[gnu::noinline]] void addBool(bool value) { StateDigest::addBool(value); }
    [[gnu::noinline]] void addI32(std::int32_t value) { StateDigest::addI32(value); }
    [[gnu::noinline]] void addU32(std::uint32_t value) { StateDigest::addU32(value); }
    [[gnu::noinline]] void addU64(std::uint64_t value) { StateDigest::addU64(value); }
};
} // namespace downwell
#endif
