#ifndef DOWNWELL_CORE_SKELETON_ANIMATION_H
#define DOWNWELL_CORE_SKELETON_ANIMATION_H
#include "SkeletonTiming.h"
#include "UnitSequenceFrames.h"
#include "RunnerSpriteChange.h"
namespace downwell {
// Animation ownership for the recovered skeleton's idle/throw/hit sprites.
// Skeleton events only write image_index=0; arbitrary nonzero GML writes are
// deliberately rejected until the general inverse mapping is implemented.
struct SkeletonAnimation : SkeletonTiming {
    int sprite = 208; // Serialized enmSkeleton default: idle, six frames.
    RunnerImageValue timeline{};
    static unsigned frames(int sprite) {
        return sprite == 208 ? 6 : sprite == 207 ? 8 : sprite == 205 ? 1 : 0;
    }
    bool setSprite(int next) {
        const unsigned count = frames(next);
        if (!count) return false;
        if (next == sprite) return true;
        RunnerImageValue index;
        RunnerSpriteChange::Result change;
        if (!index.assign(imageIndex) || !RunnerSpriteChange::plan(sprite,next,index,count,change)) return false;
        if(change.resetIndex)imageIndex={};
        if(change.resetTimeline)timeline={};
        sprite = next;
        return true;
    }
    // Call before Begin Step and alarms. Those events may reset imageIndex;
    // their write is reconciled on the following animation phase.
    bool advanceImage(bool& crossed) {
        RunnerImageValue written, speed, evaluated;
        const unsigned count = frames(sprite);
        if (!count || !written.assign(imageIndex) || !speed.assign(imageSpeed)) return false;
        auto candidate = timeline;
        bool reconcile = false;
        if (!UnitSequenceFrames::evaluate(candidate,count,evaluated)) {
            // Changing to the one-frame hit sprite after an explicit zero
            // write can leave the old timeline outside the new track.
            if (written.bits & 0x7fffffffu) return false;
            reconcile = true;
        } else if (!UnitSequenceFrames::needsReconcile(written,evaluated,reconcile)) return false;
        if (reconcile) {
            if (written.bits & 0x7fffffffu) return false;
            candidate = {}; // Inverse track mapping for an explicit zero.
        }
        bool boundary;
        if (!UnitSequenceFrames::advance(candidate,speed,count,evaluated,boundary)) return false;
        timeline = candidate;
        imageIndex = evaluated.promoted();
        crossed = boundary;
        return true;
    }
};
}
#endif
