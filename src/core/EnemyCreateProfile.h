#ifndef DOWNWELL_CORE_ENEMY_CREATE_PROFILE_H
#define DOWNWELL_CORE_ENEMY_CREATE_PROFILE_H
namespace downwell {
struct EnemyCreateProfile {
    int health, reward;
    unsigned animationRate; // twentieths of one image frame
};
// Area-one Create-event constants. Checked against pinned VM assembly by
// verify-enemy-create.py; no platform-specific copies of these rules.
constexpr EnemyCreateProfile enemyCreateProfile(int object) {
    switch (object) {
        case 111: return {5, 6, 5};
        case 112: return {5, 14, 3};
        case 113: return {35, 20, 0};
        case 116: return {15, 12, 4};
        case 117: return {20, 24, 2};
        case 119: return {3, 6, 2};
        default: return {0, 0, 0};
    }
}
// Transitional SDL instance adapter; target simulation keeps the integer rate.
template<class Instance>
void applyEnemyCreateStats(Instance& instance, int object) {
    const auto profile = enemyCreateProfile(object);
    instance.health = profile.health;
    instance.reward = profile.reward;
    instance.imageSpeed = profile.animationRate / 20.0F;
}
} // namespace downwell
#endif
