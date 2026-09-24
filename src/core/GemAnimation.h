#ifndef DOWNWELL_CORE_GEM_ANIMATION_H
#define DOWNWELL_CORE_GEM_ANIMATION_H
namespace downwell {
// objCurrency Create chooses one of four signed image speeds. Both gem
// sprites have four frames; exact tenths avoid platform-specific float drift.
struct GemAnimation {
    int rateTenths = 5;
    unsigned phaseTenths = 0;
    template<class Random>
    void initialize(Random& random) {
        static const int rates[] = {-5, -3, 3, 5};
        rateTenths = rates[random.irandomSmall(3)];
        phaseTenths = 0;
    }
    void advance() {
        int next = static_cast<int>(phaseTenths) + rateTenths;
        if (next < 0) next += 40;
        else if (next >= 40) next -= 40;
        phaseTenths = static_cast<unsigned>(next);
    }
    unsigned frame() const { return phaseTenths / 10; }
};
} // namespace downwell
#endif
