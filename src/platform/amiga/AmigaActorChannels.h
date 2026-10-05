#ifndef DOWNWELL_AMIGA_ACTOR_CHANNELS_H
#define DOWNWELL_AMIGA_ACTOR_CHANNELS_H

// Allocate in front-to-back painter order: low-numbered DMA sprites cover
// higher channels. Capacity failure is sticky. The caller separately enforces
// planar occlusion before requesting channels for actors behind a PF1 BOB.
class AmigaActorChannels {
public:
    explicit AmigaActorChannels(unsigned limit = 8) : limit_(limit > 8 ? 8 : limit) {}
    int take(unsigned parts) {
        if (blocked_ || parts == 0 || parts > limit_ - used_) {
            blocked_ = true;
            return -1;
        }
        const unsigned first = used_;
        used_ += parts;
        return static_cast<int>(first);
    }
    unsigned used() const { return used_; }
private:
    unsigned limit_;
    unsigned used_ = 0;
    bool blocked_ = false;
};

#endif
