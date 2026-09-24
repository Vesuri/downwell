#ifndef DOWNWELL_CORE_STANDARD_BULLET_STEP_H
#define DOWNWELL_CORE_STANDARD_BULLET_STEP_H

namespace downwell {
// Vertical usual-gun subset. scrBulCheckSolid suppresses wall processing
// when the complete motion segment intersects an enemy. Spread, other guns
// and exact enemy collision-event scheduling remain separate work. The
// callbacks apply the current adapters' hit effects and return whether the
// ordinary non-piercing bullet was consumed. This is not a general VM Step.
struct StandardBulletPath {
    int x, fromY, toY;
    bool intersects(int left, int right, int top, int bottom) const {
        return x >= left && x < right && fromY < bottom && toY >= top;
    }
};

template<class Enemy, class Wall>
bool stepStandardBullet(int x, int& y, Enemy enemy, Wall wall) {
    const StandardBulletPath path{x, y, y + 8};
    y = path.toY;
    if (enemy(path)) return false;
    return !wall(path);
}
} // namespace downwell
#endif
