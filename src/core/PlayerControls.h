#ifndef DOWNWELL_CORE_PLAYER_CONTROLS_H
#define DOWNWELL_CORE_PLAYER_CONTROLS_H

namespace downwell {

// Transitional arithmetic adapters. Rules below are shared; neither adapter
// establishes the original VM's binary64 storage/rounding semantics.
struct FixedPlayerMath {
    using Number = long;
    static constexpr Number ratio(int numerator, int denominator = 1) {
        return (numerator * 256L + denominator / 2) / denominator;
    }
};
struct FloatPlayerMath {
    using Number = float;
    static constexpr Number ratio(int numerator, int denominator = 1) {
        return static_cast<float>(numerator) / denominator;
    }
};

// Default, non-water scrPlayerInit + scrPlMovement. No platform dependencies.
struct NoPlayerFacingPublication { bool operator()(int)const{return true;} };
template<class Math,class Publish=NoPlayerFacingPublication>
bool applyPlayerHorizontalInput(typename Math::Number& speed, int& facing,
                                bool grounded, bool left, bool right,
                                bool controlsLocked = false,bool writeFacing=true,
                                Publish publish={}) {
    if (controlsLocked) {
        speed = 0; // recovered global.noControl branch
        return true;
    }
    const auto acceleration = Math::ratio(1, 5);
    const auto turnBoost = Math::ratio(grounded ? 2 : 1);
    const auto maximum = grounded ? Math::ratio(2) : Math::ratio(5, 2);
    const auto friction = grounded ? Math::ratio(2, 5) : Math::ratio(1, 10);
    const auto threshold = Math::ratio(3, 10);
    if (left) {
        speed -= acceleration;
        if (speed > 0) speed -= turnBoost;
        if (speed < -maximum) speed = -maximum;
        if(writeFacing){if(!publish(-1))return false;facing=-1;}
    }
    if (right) {
        speed += acceleration;
        if (speed < 0) speed += turnBoost;
        if (speed > maximum) speed = maximum;
        if(writeFacing){if(!publish(1))return false;facing=1;}
    }
    if (!left&&!right) {
        // scrPlMovement blocks 77..80: subtract sign(xsp)*friction, then
        // test abs(new xsp). Crossing zero alone does not stop movement.
        if (speed < 0) speed += friction;
        else if (speed > 0) speed -= friction;
        if (speed > -threshold && speed < threshold) speed = 0;
    }
    if(left&&right)speed=0;
    return true;
}
} // namespace downwell
#endif
