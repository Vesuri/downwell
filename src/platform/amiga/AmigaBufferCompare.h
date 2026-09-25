#ifndef DOWNWELL_AMIGA_BUFFER_COMPARE_H
#define DOWNWELL_AMIGA_BUFFER_COMPARE_H
#include <cstdint>

// Verification only. Both buffers must be word-aligned (the 68000 permits
// longword access at two-byte alignment). memcpy avoids aliasing typed loads
// from byte arrays; the alignment promise lets the target use move.l.
inline unsigned amigaBufferDifferences(const void* actual, const void* expected, unsigned bytes) {
    const auto* a = static_cast<const unsigned char*>(__builtin_assume_aligned(actual, 2));
    const auto* b = static_cast<const unsigned char*>(__builtin_assume_aligned(expected, 2));
    unsigned differences = 0;
    const auto wordEqual = [](const unsigned char* p, const unsigned char* q) {
        std::uint32_t x, y;
        __builtin_memcpy(&x, __builtin_assume_aligned(p, 2), 4);
        __builtin_memcpy(&y, __builtin_assume_aligned(q, 2), 4);
        return x == y;
    };
    // The successful oracle path is overwhelmingly equal. Amortize pointer
    // updates/loop branches without allocating a temporary block on stack.
    while (bytes >= 32) {
        if (!(wordEqual(a,b) && wordEqual(a+4,b+4) && wordEqual(a+8,b+8) && wordEqual(a+12,b+12) &&
              wordEqual(a+16,b+16) && wordEqual(a+20,b+20) && wordEqual(a+24,b+24) && wordEqual(a+28,b+28)))
            for (unsigned i = 0; i < 32; ++i) differences += a[i] != b[i];
        a += 32; b += 32; bytes -= 32;
    }
    while (bytes >= 4) {
        std::uint32_t x, y;
        __builtin_memcpy(&x, a, 4);
        __builtin_memcpy(&y, b, 4);
        if (x != y)
            for (unsigned i = 0; i < 4; ++i) differences += a[i] != b[i];
        a += 4; b += 4; bytes -= 4;
    }
    while (bytes--) differences += *a++ != *b++;
    return differences;
}
#endif
