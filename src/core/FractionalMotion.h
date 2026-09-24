#ifndef DOWNWELL_CORE_FRACTIONAL_MOTION_H
#define DOWNWELL_CORE_FRACTIONAL_MOTION_H

namespace downwell {

// Position/velocity are Q8 wholes plus nonnegative fifth-unit remainders.
// Euclidean division retains sub-Q8 displacement for either velocity sign.
inline void integrateFifthsQ8(long& position, unsigned char& positionRemainder,
    long velocity, unsigned char velocityRemainder) {
    const long displacement = velocity * 5 + velocityRemainder + positionRemainder;
    long whole = displacement / 5;
    long fraction = displacement % 5;
    if (fraction < 0) { --whole; fraction += 5; }
    position += whole;
    positionRemainder = static_cast<unsigned char>(fraction);
}

} // namespace downwell
#endif
