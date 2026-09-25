#ifndef DOWNWELL_CORE_SKELETON_THROW_H
#define DOWNWELL_CORE_SKELETON_THROW_H

namespace downwell {
// enmSkeleton Step blocks 21–26. Caller owns the active/image_index>4/
// !eShotDelay gate. createBone must finish the child's entire Create event
// synchronously before returning. Velocities use exact tenths, not Q8-rounded
// constants; the eventual projectile adapter owns its position representation.
// A failed allocation stops without emitting sound or drawing further RNG.
template<class Ops>
bool skeletonThrowAligned(int x, int y, bool horizontallySeparated, int imageScaleX, Ops& ops) {
    if (!ops.createBone(269, x, y - 10)) return false;
    ops.throwSound(134);
    const int vertical[] = {-30, -31, -32};
    const int horizontal[] = {10, 8, 12};
    const int vy = vertical[ops.choose(3)];
    // Alignment skips the horizontal choice, even for a left-facing skeleton.
    const int vx = horizontallySeparated ? imageScaleX * horizontal[ops.choose(3)] : 10;
    ops.setVelocityTenths(vx, vy);
    if (vx) ops.multiplyAnimationSpeed(vx > 0 ? -1 : 1);
    ops.latchShot();
    return true;
}
// Integer-coordinate convenience for existing fixtures. Live owners should
// use skeletonThrowAligned with a predicate from the precise player position.
template<class Ops>
bool skeletonThrow(int x, int y, int playerX, int imageScaleX, Ops& ops) {
    return skeletonThrowAligned(x,y,playerX!=x,imageScaleX,ops);
}
} // namespace downwell
#endif
