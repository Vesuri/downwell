#include "core/SkeletonDecisions.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void check(bool v) { if (!v) std::abort(); }
static downwell::MotionBinary64 number(double value) {
    std::uint64_t bits; std::memcpy(&bits,&value,8);
    return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static bool same(downwell::MotionBinary64 a,double b) {
    const auto v = number(b); return a.high==v.high && a.low==v.low;
}
struct Ops {
    downwell::SkeletonDecisions& state;
    int hurt = 0, throws = 0, direction = 0;
    bool allocated = true;
    void hurtSound(int sound) {
        check(sound == 133 && state.active && state.hitStun && state.hitPending);
        check(same(state.imageIndex,0) && same(state.rate,0.3));
        ++hurt;
    }
    bool throwBone(int facing) { ++throws; direction = facing; return allocated; }
};
int main() {
    unsigned cases = 0;
    for (int sprite : {205,207,208}) for (double frame : {0.0,2.9,3.0,4.0,4.1,6.0,6.1,7.0})
    for (bool active : {false,true}) for (bool hit : {false,true})
    for (bool stopped : {false,true}) for (bool shot : {false,true})
    for (bool below : {false,true}) for (bool left : {false,true}) for (int movement : {-1,0,1}) {
        downwell::SkeletonDecisions s;
        s.sprite=sprite; s.imageIndex=number(frame); s.rate=s.throwRate();
        s.active=active; s.hitPending=hit; s.shot=shot; s.facing=-1;
        int expectedSprite=sprite, facing=-1, hurt=0, throws=0, direction=0, alarm1=-1, alarm2=-1, alarm3=-1;
        double index=frame, rate=0.3; bool stun=false;
        if (!stopped) {
            if (below) active=true;
            if (hit) { active=true; stun=true; alarm1+=5; index=0; hurt=1; rate=0; alarm3=30; hit=false; }
            auto change = [&](int next,int frames) {
                if (expectedSprite != next && int(index) >= frames) index=0;
                expectedSprite=next;
            };
            if (!active) { change(208,6); rate=0.2; }
            else {
                if (expectedSprite==208) { index=0; rate=0.3; }
                change(207,8);
                if (index-3 < -0.00001) facing=left?-1:1;
                if (index-4 > 0.00001 && !shot) { throws=1; direction=facing; shot=true; }
                if (index-6 > 0.00001 && rate!=0) { rate=0; alarm2=30; }
            }
            if (movement) facing=movement;
        }
        Ops ops{s};
        check(s.stepDecisions(stopped,below,left,movement,ops));
        check(s.active==active && s.hitPending==hit && s.hitStun==stun && s.shot==shot && s.facing==facing);
        check(s.sprite==expectedSprite && same(s.imageIndex,index) && same(s.rate,rate));
        check(same(s.imageSpeed,stopped?0:double(0.3f)));
        check(s.alarms[1]==alarm1 && s.alarms[2]==alarm2 && s.alarms[3]==alarm3);
        check(ops.hurt==hurt && ops.throws==throws && ops.direction==direction);
        check(s.finishPresentation());
        check(s.sprite==(stun?205:expectedSprite) && same(s.imageIndex,stun?0:index));
        ++cases;
    }
    // Every adjacent binary32 value in a band around all three source gates.
    for (unsigned gate : {3u,4u,6u}) {
        float centre=float(gate); std::uint32_t bits;
        std::memcpy(&bits,&centre,4);
        for (int offset=-64; offset<=64; ++offset) {
            const std::uint32_t raw=bits+offset;
            float frame; std::memcpy(&frame,&raw,4);
            downwell::SkeletonDecisions s; s.imageIndex=number(double(frame));
            check(s.frameBelow(gate)==(double(frame)-gate < -0.00001));
            check(s.frameAbove(gate)==(double(frame)-gate > 0.00001));
        }
    }
    // The twentieth 0.3f advance is slightly ABOVE six, but not past the
    // runner's GT gate. Stopping here would shorten the original animation.
    downwell::SkeletonDecisions sequence; sequence.sprite=207;
    sequence.rate=sequence.throwRate(); sequence.shot=true;
    for (int tick=0; tick<20; ++tick) {
        sequence.beginStep(false); bool crossed=false;
        check(sequence.advanceImage(crossed) && !crossed);
    }
    check(!sequence.frameAbove(6));
    bool crossed=false; check(sequence.advanceImage(crossed));
    check(sequence.frameAbove(6));
    downwell::SkeletonDecisions failed;
    failed.sprite=207; failed.imageIndex=number(5);
    Ops failure{failed}; failure.allocated=false;
    check(!failed.stepDecisions(false,false,false,0,failure) && !failed.shot);
    std::printf("Skeleton decisions: %u activation/hit/throw/facing cases and allocation failure passed\n",cases);
}
