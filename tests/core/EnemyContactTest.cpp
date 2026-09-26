#include "core/EnemyContact.h"
#include "core/EnemyContactResponse.h"
#include "core/StandardBulletImpact.h"
#include "core/BatHitState.h"
#include "core/FrogHitState.h"
#include "core/SnailHitState.h"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using namespace downwell;
    {
        constexpr auto normal = machineGunProfileForStreak(99);
        constexpr auto high = machineGunProfileForStreak(100);
        static_assert(normal.damage == 9 && high.damage == 15, "selected source damage table");
        require(normal.object == 261 && normal.sprite == 459 && normal.sound == 18 &&
                normal.muzzle == 603 && normal.interval == 7 && normal.shake == 2 &&
                normal.shakeDuration == 3 && normal.rangeTimer == 12,
                "ordinary machine gun uses selected level-zero resources and timing");
        require(high.object == 261 && high.sprite == 458 && high.sound == 19 &&
                high.muzzle == 602 && high.interval == 6 && high.shake == 3 &&
                high.shakeDuration == 4 && high.rangeTimer == 13,
                "gem-high machine gun uses selected level-one resources and timing");
        for (const auto profile : {normal, high})
            require(profile.speed == 8 && profile.accuracy == 3 && profile.consumption == 1 &&
                    profile.recoil == 0 && profile.rangeRandom == 2 &&
                    !profile.consistent && !profile.wave && !profile.explode,
                    "both machine gun levels retain common source properties");
        auto selected = machineGunProfileForStreak(100);
        const auto born = selected;
        selected = machineGunProfileForStreak(0);
        require(born.level == 1 && born.damage == 15 && selected.level == 0 && selected.damage == 9,
                "value snapshots can survive a later weapon-level change");
    }
    {
        struct Enemy {
            bool active = true;
            int objectId = 116, health = 15;
            BatHitState batHit;
            FrogHitState frogHit;
            SnailHitState snailHit;
        } enemy;
        struct Random {
            unsigned calls = 0, choice = 0;
            unsigned chooseIndexSmall(unsigned bound) {
                require(bound == 2, "ricochet selection uses exactly two source sounds");
                ++calls; return choice;
            }
        } random;
        unsigned defeated = 0, rewards = 0;
        const auto reward = [&] { ++rewards; };
        for (int object : {113, 116, 117}) {
            enemy = Enemy{}; enemy.objectId = object;
            auto impact = applyStandardBulletImpact(enemy, defeated, random, reward);
            require(impact.kind == BulletImpactKind::damaged && impact.sound == 29 &&
                        enemy.active && enemy.health == 6 && random.calls == 0,
                    "usual bullet applies table-selected nine damage without an invented random draw");
            require(object == 113 ? enemy.frogHit.pending : object == 117 ? enemy.snailHit.pending :
                        enemy.batHit.pending && enemy.batHit.pendingDirection == 270,
                    "shared impact queues the appropriate existing enemy hit state");
            impact = applyStandardBulletImpact(enemy, defeated, random, reward);
            require(impact.kind == BulletImpactKind::defeated && !enemy.active && enemy.health == -3,
                    "fatal usual hit emits the implemented retirement/reward response");
            require(applyStandardBulletImpact(enemy, defeated, random, reward).kind == BulletImpactKind::ignored,
                    "already retired targets cannot produce a second defeat");
        }
        require(defeated == 3 && rewards == 3, "each fatal target emits exactly one reward");
        enemy = Enemy{}; enemy.objectId = 112;
        for (unsigned choice = 0; choice < 2; ++choice) {
            random.choice = choice;
            const auto impact = applyStandardBulletImpact(enemy, defeated, random, reward);
            require(impact.kind == BulletImpactKind::immune && impact.sound == static_cast<int>(30 + choice) &&
                        enemy.health == 15 && enemy.active && random.calls == choice + 1 && rewards == 3,
                    "immune turtle preserves health but consumes the source ricochet choice");
        }
        enemy.active = false;
        require(applyStandardBulletImpact(enemy, defeated, random, reward).kind == BulletImpactKind::ignored &&
                    random.calls == 2, "inactive targets consume no ricochet RNG");
        enemy = Enemy{};
        const bool bornHigh = true;
        const auto highImpact = applyStandardBulletImpact(enemy, defeated, random, reward, bornHigh);
        require(highImpact.kind == BulletImpactKind::defeated && enemy.health == 0,
                "powered-up birth profile applies fifteen damage at impact");
        enemy = Enemy{};
        applyStandardBulletImpact(enemy, defeated, random, reward, false, 0xc1000000u);
        require(enemy.health == 6 && enemy.batHit.pendingDirection == 271 &&
                    enemy.batHit.pendingFraction == 26624,
                "surviving live impact queues the supplied spread-adjusted direction");
    }
    {
        struct PlayerProbe {
            int bounces = 0, knocks = 0, knockX = 0, knockY = 0;
            long offsetXFixed() const { return 0; }
            long offsetYFixed() const { return 0; }
            long frameStartYFixed() const { return 0; }
            int facing() const { return 1; }
            void stompBounce() { ++bounces; }
            void contactKnockback(int x, int y) { ++knocks; knockX = x; knockY = y; }
        } motion;
        bool active = true;
        int enemyHealth = 15, health = 4;
        unsigned immunity = 0, defeated = 0, rewards = 0;
        bool candle = false;
        ContactActor contactActor{128, 0, 1, 1, 153};
        const auto apply = [&] {
            const int beforeHealth = health;
            return applyEnemyContact(116, contactActor, 7, 9, active, enemyHealth,
                motion, health, immunity, defeated, [&] { ++rewards; return true; }, candle,
                [&](int x,int y) {
                    require(x==7 && y==9 && immunity==0 && health==beforeHealth-1,
                        "damage-number callback follows health subtraction and precedes immunity");
                    return true;
                });
        };
        require(apply() == EnemyContact::damage && health == 3 && immunity == 90 &&
                    motion.knocks == 1 && motion.knockX == 7 && motion.knockY == 9 &&
                    active && enemyHealth == 15 && rewards == 0,
                "shared damage applies immunity and uses published instance coordinates for knockback");
        require(apply() == EnemyContact::none && health == 3 && motion.knocks == 1,
                "later contacts in the same immunity interval do not repeat damage or impulse");
        contactActor.y = 3 * 256;
        require(apply() == EnemyContact::stomp && !active && enemyHealth == 0 &&
                    defeated == 1 && rewards == 1 && motion.bounces == 1 && health == 3,
                "immunity does not prevent the implemented stomp/reward/bounce response");
        require(apply() == EnemyContact::none && rewards == 1 && defeated == 1,
                "retired enemies cannot issue a second reward");
        active = true; enemyHealth = 15; health = 1; immunity = 0; contactActor.y = 0;
        require(apply() == EnemyContact::damage && health == 0 && motion.knocks == 2,
                "fatal contact still applies the source damage impulse");
        immunity = 0;
        require(apply() == EnemyContact::none && health == 0 && motion.knocks == 2,
                "the implemented zero-health state cannot lose health repeatedly");
        contactActor.y = 3 * 256;
        require(apply() == EnemyContact::none && active && enemyHealth == 15 &&
                    rewards == 1 && defeated == 1 && motion.bounces == 1,
                "a dead player cannot stomp, bounce, kill or collect a new reward");
        health = 4; contactActor.y = 0; candle = true;
        require(apply() == EnemyContact::damage && health == 3 && immunity == 240,
                "owned Candle selects the source 240-update immunity interval");
        candle = false;
        for (unsigned elapsed = 1; elapsed < 240; ++elapsed) {
            --immunity;
            require(apply() == EnemyContact::none && health == 3,
                    "granted immunity is retained until expiry, independent of later ownership");
        }
        --immunity;
        require(apply() == EnemyContact::damage && health == 2 && immunity == 90,
                "first expired contact damages and samples current ownership again");
        immunity=0;
        const int oldKnocks=motion.knocks;
        require(applyEnemyContact(116,contactActor,7,9,active,enemyHealth,motion,
            health,immunity,defeated,[&]{++rewards;return true;},false,[](int,int){return false;})==EnemyContact::none,
            "failed damage birth is not published as a successful contact");
        require(health==1 && immunity==0 && motion.knocks==oldKnocks,
            "failure preserves prior subtraction and does not perform later immunity or knockback");
    }
    {
        struct Player {
            unsigned knocks = 0, bounces = 0;
            long offsetXFixed() const { return 0; }
            long offsetYFixed() const { return 0; }
            long frameStartYFixed() const { return 0; }
            int facing() const { return 1; }
            void stompBounce() { ++bounces; }
            void contactKnockback(int, int) { ++knocks; }
        } motion;
        struct Enemy { int objectId = 116, health = 15; bool active = true; long y = 0; };
        Enemy enemies[5];
        enemies[0].objectId = 1; // A live non-enemy in SDL's mixed store.
        enemies[3].y = 3 * 256;
        enemies[4].active = false;
        struct Combo { unsigned kills = 0; void enemyDefeated() { ++kills; } } combo;
        int health = 4;
        unsigned immunity = 1, defeated = 0, rewards = 0, published = 0, located = 0;
        bool rewardWorks=true;
        auto step = [&] {
            return stepEnemyContacts(5, motion, health, immunity, defeated, combo, false,
                [&](unsigned i) -> Enemy& { return enemies[i]; },
                [&](const Enemy& e) {
                    ++located;
                    return EnemyContactLocation{{128, e.y, 1, 1, 153}, 7, 9};
                },
                [&](Enemy& e) {
                    require(e.active && e.health == 15 && combo.kills == rewards + 1 &&
                            defeated == rewards, "combo precedes reward, which precedes retirement");
                    ++rewards;
                    return rewardWorks;
                },
                [&](EnemyContact contact) {
                    require(contact != EnemyContact::none, "only applied contacts publish effects");
                    ++published;
                }, [](int,int) { return true; });
        };
        step();
        require(health == 3 && immunity == 90 && motion.knocks == 1 &&
                motion.bounces == 1 && defeated == 1 && rewards == 1 &&
                published == 2 && located == 3,
                "one expiry per pass, one damage across two overlaps, then an immune stomp");
        step();
        require(immunity == 89 && health == 3 && published == 2 && rewards == 1,
                "unchanged immune contacts and retired enemies have no repeat effects");
        enemies[3] = Enemy{}; enemies[3].y = 3 * 256;
        health = 1; immunity = 0;
        step();
        require(health == 0 && published == 3 && motion.knocks == 2 &&
                enemies[3].active && rewards == 1 && defeated == 1,
                "fatal damage suppresses later damage and stomp in stable instance order");
        health=4;immunity=5;rewardWorks=false;
        enemies[1].y=enemies[2].y=3*256;
        const auto oldLocated=located,oldPublished=published,oldDefeated=defeated;
        const auto oldBounces=motion.bounces,oldRewards=rewards,oldCombo=combo.kills;
        require(!step(),"failed stomp reward aborts contact traversal");
        require(located==oldLocated+1&&published==oldPublished&&defeated==oldDefeated&&
                motion.bounces==oldBounces&&enemies[1].active&&enemies[1].health==15,
                "failed reward cannot retire, bounce, publish or reach a later contact");
        require(rewards==oldRewards+1&&combo.kills==oldCombo+1,
                "already executed reward callback and combo changes are not rolled back");
    }
    ContactActor player{0, 0, 1, 1, 31};
    ContactActor bat{0, -11 * 256, 1, 1, 153};
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "bat above player at eleven pixels is NOT inside the recovered short mask");
    bat.y = -8 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "pixel-cell edges touching without shared pixel centres do not collide");
    bat.y = -7 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::damage,
            "first overlapping vertical pixel produces damage");
    bat.y = 3 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::stomp,
            "typStompable uses strict enemy yy > global.ply + 2");
    require(classifyEnemyContact(player, bat, 116, 256) == EnemyContact::damage,
            "stomp equality is damage, using the supplied pre-movement player position");
    bat.y = 2 * 256 + 1;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::stomp,
            "stomp decision retains subpixel yy even when the instance rounds down");
    bat.x = 10 * 256; bat.y = 0;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "unflipped bat rectangle starts beyond the player's right edge");
    bat.scaleX = -1;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::damage,
            "asymmetric bat mask flips its pixel-cell edges around the origin");
    require(enemyContactMask(112, 188) == 186 && enemyContactMask(113, 170) == 166,
            "walking turtle and preparing frog retain source fixed masks");
    ContactActor snail{0, 3 * 256, 1, -1, 177};
    require(classifyEnemyContact(player, snail, 117, 0) == EnemyContact::damage,
            "snails cannot be stomped even when below the old player position");
    ContactActor flippedPlayer = player; flippedPlayer.scaleX = -1;
    const auto a = contactWorldBounds(player), b = contactWorldBounds(flippedPlayer);
    require(a.left == b.left && a.right == b.right,
            "player mask's symmetric eight-pixel cell extent is invariant under facing flip");
    ContactActor rounded{128, -384, 1, 1, 31};
    const auto roundedBounds = contactWorldBounds(rounded);
    require(roundedBounds.left == -4 && roundedBounds.top == -6,
            "instance positions use source ties-to-even rounding on both axes");
    bat = {0, 3 * 256, 1, 1, 153};
    const auto relative = classifyEnemyContact(player, bat, 116, 0);
    player.x += 240 * 256; player.y += 176 * 256;
    bat.x += 240 * 256; bat.y += 176 * 256;
    require(classifyEnemyContact(player, bat, 116, 176 * 256) == relative,
            "SDL world and native relative coordinates yield the same contact");
    std::cout << "enemy contact tests passed\n";
}
