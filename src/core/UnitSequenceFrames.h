#ifndef DOWNWELL_CORE_UNIT_SEQUENCE_FRAMES_H
#define DOWNWELL_CORE_UNIT_SEQUENCE_FRAMES_H
#include "SequenceTimeline.h"
namespace downwell {
// Only for verified consecutive keys (time i, length 1, frame value i),
// The pinned export verifies this shape for every nonempty original sprite.
// Empty sprites and per-second playback conversion remain caller concerns.
// Not a generic arbitrary-track evaluator.
struct UnitSequenceFrames {
    static bool evaluate(RunnerImageValue position, unsigned frames, RunnerImageValue& image) {
        if (!frames || frames > 65535 || !SequenceTimeline::valid(position)) return false;
        if ((position.bits & 0x80000000u) && (position.bits & 0x7fffffffu)) return false;
        const RunnerImageValue bound{SequenceTimeline::integerBits(frames)};
        if ((position.bits & 0x7fffffffu) >= bound.bits) return false;
        // Runner interpolation: i + (position-i)/1 * 1. For consecutive
        // integer keys these operations recover position exactly. Adding +0
        // canonicalizes the negative-zero case in the first key.
        image.bits = position.bits & 0x7fffffffu;
        return true;
    }
    static bool needsReconcile(RunnerImageValue written, RunnerImageValue evaluated, bool& needed) {
        if (!SequenceTimeline::valid(written) || !SequenceTimeline::valid(evaluated)) return false;
        auto difference = written.promoted();
        auto subtract = evaluated.promoted(); subtract.high ^= 0x80000000u;
        RunnerImageValue rounded;
        if (!difference.add(subtract) || !rounded.assign(difference)) return false;
        rounded.bits &= 0x7fffffffu;
        const auto magnitude = rounded.promoted();
        // Original runner double at 0x8a6c28, after single-rounded difference.
        needed = magnitude.high > 0x3ee4f8b5u ||
            (magnitude.high == 0x3ee4f8b5u && magnitude.low > 0x88e368f1u);
        return true;
    }
    static bool advance(RunnerImageValue& timeline, RunnerImageValue speed, unsigned frames,
                        RunnerImageValue& image, bool& crossed) {
        auto candidate = timeline;
        RunnerImageValue evaluated;
        bool boundary;
        if (!SequenceTimeline::advance(candidate,speed,frames,boundary) ||
            !evaluate(candidate,frames,evaluated)) return false;
        timeline = candidate; image = evaluated; crossed = boundary;
        return true;
    }
};
}
#endif
