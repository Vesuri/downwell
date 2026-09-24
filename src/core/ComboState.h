#ifndef DOWNWELL_CORE_COMBO_STATE_H
#define DOWNWELL_CORE_COMBO_STATE_H
#include <cstdint>
namespace downwell {
// scrEnemyDeath / comboDone, plus synchronous comboRewardText Create rewards.
// The caller owns collision-phase scheduling, persistence and presentation.
struct ComboState {
    struct Milestones { unsigned gems = 8, ammo = 15, health = 25; };
    struct Reward {
        std::uint32_t combo = 0;
        unsigned gems = 0, ammo = 0, health = 0, showBonusMax = 0;
        // Three independent source branches: sounds 315, 316, 317 respectively.
        unsigned soundMask = 0;
        unsigned hideAlarm = 90, destroyAlarm = 105;
    };
    struct Completion {
        bool rewardCreated = false;
        bool momentDelay = false;
        bool saveRecord = false;
        // Steam achievement IDs 4, 5, 14, in that order.
        unsigned achievementMask = 0;
    };
    std::uint32_t count = 0, best = 0, record = 0;
    void enemyDefeated() { ++count; }
    void beginRun() { count = best = 0; }

    template<class CreateReward>
    Completion finish(const Milestones& milestones, CreateReward createReward) {
        Completion result;
        if (count >= 5) {
            Reward reward;
            reward.combo = count;
            if (count >= milestones.gems) {
                reward.gems = 100; reward.showBonusMax = 2; reward.soundMask |= 1;
            }
            if (count >= milestones.ammo) {
                reward.ammo = 1; reward.showBonusMax = 3; reward.soundMask |= 2;
            }
            if (count >= milestones.health) {
                reward.health = 1; reward.showBonusMax = 4; reward.soundMask |= 4;
            }
            reward.hideAlarm = 90 + 15 * reward.showBonusMax;
            reward.destroyAlarm = reward.hideAlarm + 15;
            // Create runs before highCombo/record updates and comboCount reset.
            // Death destroys the reward's presentation only AFTER these grants.
            createReward(reward);
            result.rewardCreated = result.momentDelay = true;
            if (count >= 10) result.achievementMask |= 1;
            if (count >= 30) result.achievementMask |= 2;
            if (count >= 100) result.achievementMask |= 4;
        }
        if (count > best) best = count;
        if (best > record) { record = best; result.saveRecord = true; }
        count = 0;
        return result;
    }
};
} // namespace downwell
#endif
