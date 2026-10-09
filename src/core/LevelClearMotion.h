#ifndef DOWNWELL_CORE_LEVEL_CLEAR_MOTION_H
#define DOWNWELL_CORE_LEVEL_CLEAR_MOTION_H
#include "HudReal.h"
#include "MotionBinary64.h"
namespace downwell {
#ifndef DOWNWELL_REFERENCE_CLEAR_MOTION
namespace clear_motion_detail {
#include "../../generated/clear-motion.inc"
}
#endif
// Result Create and scrResult's final player/camera easing. Positions remain
// positive in this path; the horizontally signed carousel is separate.
struct LevelClearMotion {
    HudReal cameraY = HudReal::integer(142), cameraTarget = cameraY;
    HudReal playerY, playerTarget = HudReal::integer(160), acceleration;
    unsigned playerDivisor = 20;
    void showChoices() {
#ifndef DOWNWELL_REFERENCE_CLEAR_MOTION
        prepared_ = prepared_ && !choices_ && cursor_ == 59 && matches();
        choices_ = true; cursor_ = 0;
#endif
        cameraTarget = HudReal::integer(340); // levely + 70
        playerTarget = HudReal::integer(302); // levely + 32
        playerDivisor = 10;
    }
    static HudReal approach(HudReal value, HudReal target, unsigned divisor) {
        return target.less(value) ? value.subtract(value.subtract(target).divideSmall(divisor)) :
            value.add(target.subtract(value).divideSmall(divisor));
    }
    void step(bool departureBranch) {
#ifndef DOWNWELL_REFERENCE_CLEAR_MOTION
        if(!departureBranch && prepared_ && matches()) {
            if(choices_) {
                constexpr unsigned count=sizeof(clear_motion_detail::choices)/sizeof(clear_motion_detail::choices[0]);
                if(cursor_+1<count)++cursor_;
                playerY=decode(clear_motion_detail::choices[cursor_][0]);
                cameraY=decode(clear_motion_detail::choices[cursor_][1]);
                return;
            }
            if(cursor_<59){playerY=decode(clear_motion_detail::entrance[++cursor_]);return;}
        }
        prepared_=false;
#endif
        if (departureBranch) {
            playerTarget = playerTarget.add(acceleration);
            acceleration = acceleration.add({0x1999999999999aULL, -3}); // binary64 0.2
        }
        playerY = approach(playerY, playerTarget, playerDivisor);
        cameraY = approach(cameraY, cameraTarget, 10);
    }
    int viewY() const { return static_cast<int>(cameraY.add({0x10000000000000ULL, -1}).floor()) - 142; }
    // scrResult stores round(yy) in the Result instance's y builtin. yy is
    // cameraY, not the independently eased global.ply. Preserve ties-to-even.
    bool instanceY(std::int32_t& y)const{
        if(!cameraY.mantissa){y=0;return true;}
        if(cameraY.exponent < -1022 || cameraY.exponent > 30 ||
           cameraY.mantissa < (std::uint64_t{1}<<52) ||
           cameraY.mantissa >= (std::uint64_t{1}<<53))return false;
        const MotionBinary64 value{(unsigned(cameraY.exponent+1023)<<20)|
            (std::uint32_t(cameraY.mantissa>>32)&0xfffffu),std::uint32_t(cameraY.mantissa)};
        return value.roundPixel(y);
    }
#ifndef DOWNWELL_REFERENCE_CLEAR_MOTION
private:
    // Canonical Create -> 59 Steps -> Alarm 4/choices. These are exact full
    // states, not pixel-rounded samples. Every public input is checked before
    // advancing; departures, edits and noncanonical timing retain arithmetic.
    unsigned cursor_=0;
    bool prepared_=true,choices_=false;
    static HudReal decode(std::uint64_t bits) {
        return bits ? HudReal{(bits&0xfffffffffffffULL)|(1ULL<<52),int(bits>>52)-1023} : HudReal{};
    }
    static bool same(HudReal a,HudReal b){return a.mantissa==b.mantissa&&a.exponent==b.exponent;}
    bool matches()const {
        const auto player=choices_?clear_motion_detail::choices[cursor_][0]:clear_motion_detail::entrance[cursor_];
        const auto camera=choices_?decode(clear_motion_detail::choices[cursor_][1]):HudReal::integer(142);
        return same(playerY,decode(player))&&same(cameraY,camera)&&!acceleration.mantissa&&
            same(playerTarget,HudReal::integer(choices_?302:160))&&
            same(cameraTarget,HudReal::integer(choices_?340:142))&&playerDivisor==(choices_?10u:20u);
    }
#endif
};
}
#endif
