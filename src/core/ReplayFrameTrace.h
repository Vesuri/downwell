#ifndef DOWNWELL_REPLAY_FRAME_TRACE_H
#define DOWNWELL_REPLAY_FRAME_TRACE_H
#include <cstdint>
namespace downwell {
// Test-only diagnostics: preserve per-frame checkpoints so an aggregate
// mismatch identifies its first divergent frame. No backend pointers/padding.
struct ReplayFrameTrace {
    static constexpr unsigned capacity=64,words=12;
    template<class World> [[gnu::noinline]] static void capture(const World& world,volatile std::uint32_t* out){
        const auto checkpoint=world.checkpoint();const auto& p=world.player();
        out[0]=std::uint32_t(checkpoint>>32);out[1]=std::uint32_t(checkpoint);
        out[2]=std::uint32_t(p.offsetXFixed());out[3]=std::uint32_t(p.offsetYFixed());
        out[4]=std::uint32_t(p.speedXFixed());out[5]=std::uint32_t(p.speedYFixed());
        out[6]=std::uint32_t(p.ammo());out[7]=p.grounded();out[8]=world.ringOrigin();
        out[9]=world.bulletActive();out[10]=std::uint32_t(world.bulletX());out[11]=std::uint32_t(world.bulletY());
    }
};
}
#endif
