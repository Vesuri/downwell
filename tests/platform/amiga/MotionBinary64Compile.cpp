#include "core/MotionBinary64.h"
// Code-generation fixture only, not linked into the playable executable.
// Variable operands force the compiler to retain the complete arithmetic path.
extern "C" bool motionBinary64Compile(downwell::MotionBinary64* position,
    downwell::MotionBinary64* velocity, std::int32_t* pixel) {
    return velocity->add({0x3fb99999u, 0x9999999au}) &&
        position->add(*velocity) && position->roundPixel(*pixel);
}
