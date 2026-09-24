#ifndef DOWNWELL_CORE_FROG_PRESENTATION_H
#define DOWNWELL_CORE_FROG_PRESENTATION_H
namespace downwell {
struct FrogVisual {
    int sprite;
    int forcedFrame; // -1 preserves image_index.
};

// enmFrog Create/Step: imgSp is distinct from image_speed. Step copies the
// former at entry, then its final presentation branches may change imgSp for
// the following Step. Rates here are twentieths of a frame, not host floats.
struct FrogPresentation {
    unsigned desiredRate = 0;
    unsigned beginStepRate() const { return desiredRate; }
    FrogVisual finishStep(bool hitStun, bool grounded, bool preparing, bool rising) {
        if (hitStun) return {173, -1};
        if (grounded) {
            if (preparing) return {170, -1};
            desiredRate = 4;
            return {166, -1};
        }
        desiredRate = 0;
        return {168, rising ? 0 : 1};
    }
};
} // namespace downwell
#endif
