#include "core/StateDigest.h"
#include <cstdio>
#include <cstdlib>

int main() {
    downwell::StateDigest digest;
    std::uint64_t oracle=14695981039346656037ULL;
    unsigned count=0;
    const auto feed=[&](std::uint64_t value) {
        digest.addU64(value);
        // Independent scalar byte-stream oracle; host-only 64-bit arithmetic.
        for(unsigned byte=0;byte<8;++byte) {
            oracle^=static_cast<unsigned char>(value);
            oracle*=1099511628211ULL;
            value>>=8;
        }
        if(digest.value()!=oracle)std::abort();
        ++count;
    };
    feed(0);feed(~std::uint64_t(0));
    for(unsigned bit=0;bit<64;++bit)feed(std::uint64_t(1)<<bit);
    std::uint32_t random=42;
    for(unsigned i=0;i<10000;++i) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        const auto low=random;
        random^=random<<13;random^=random>>17;random^=random<<5;
        feed((std::uint64_t(random)<<32)|low);
    }
    std::printf("State digest: %u ordered byte-stream comparisons passed\n",count);
}
