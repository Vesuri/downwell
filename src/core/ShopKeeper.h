#ifndef DOWNWELL_CORE_SHOP_KEEPER_H
#define DOWNWELL_CORE_SHOP_KEEPER_H
#include "ShopFeedback.h"
#include "AlarmBank.h"
#include "HudReal.h"
namespace downwell {
// Recovered shopKeeper events. Animation advancement is explicit so adapters
// can keep it separate from alarm/Step dispatch. HudReal supplies positive
// binary64 arithmetic without a target FPU or decimal phase approximation.
struct ShopKeeper {
    int sprite = 485;
    HudReal image;
    HudReal speed{0x170a3d70a3d70aULL, -4}; // literal double 0.09
    int alarm0 = -1, alarm1 = -1, alarm2 = -1;
    bool present = true, spawnEnemy = false;
    unsigned frame() const { return image.floor(); }
    template<class Range>
    void alarms(ShopFeedback& feedback, Range random) {
        if (!present) return;
        // Runner alarm slots are processed in ascending order, reading the
        // current counter after preceding handlers. Alarm 0's new alarm 1
        // therefore becomes 4 here; alarm 1's new alarm 0 waits until next tick.
        if (AlarmBank::advanceValue(alarm0) && sprite == 487) {
            image = HudReal::integer(1); alarm1 = 5;
        }
        if (AlarmBank::advanceValue(alarm1) && sprite == 487) {
            image = {}; alarm0 = random(20, 60);
        }
        if (AlarmBank::advanceValue(alarm2)) feedback.thankful = feedback.goodbye = feedback.confused = 0;
    }
    template<class Range>
    void step(ShopFeedback& feedback, int playerX, int x, bool criminal,
              unsigned area, Range random) {
        if (!present) return;
        if (feedback.resetKeeperImage) image = {};
        if (!feedback.goodbye && !feedback.thankful && !feedback.confused) {
            if (playerX < x) {
                sprite = 485; speed = {0x170a3d70a3d70aULL, -4};
            } else {
                if (sprite != 487) { sprite = 487; image = {}; alarm0 = random(20, 60); }
                speed = {};
            }
            if (criminal && playerX > 170) { present = false; spawnEnemy = true; }
        } else if (feedback.thankful == 1) {
            alarm2 = 0; speed = {0x1999999999999aULL, -2}; sprite = 486;
            if (HudReal::integer(4).less(image)) { speed = {}; alarm2 = 60; feedback.thankful = 2; }
            feedback.confused = 0;
        } else if (feedback.goodbye == 1) {
            speed = {0x1999999999999aULL, -3}; sprite = 496; alarm2 = 240;
            feedback.thankful = feedback.confused = 0; feedback.goodbye = 2;
        } else if (feedback.confused == 1) {
            speed = {0x1999999999999aULL, -3}; sprite = 497;
            if (HudReal::integer(1).less(image)) { speed = {}; alarm2 = 60; feedback.confused = 2; }
            feedback.thankful = 0;
        }
        if (feedback.goodbye) speed = {0x1999999999999aULL, -3};
        if (playerX > x) feedback.thankful = feedback.confused = feedback.goodbye = 0;
        if (area == 3 && speed.mantissa) --speed.exponent;
    }
    void advanceAnimation() {
        if (!present) return;
        image = image.add(speed);
        const auto count = HudReal::integer(sprite == 486 ? 5 : sprite == 487 || sprite == 497 ? 2 : 4);
        while (!image.less(count)) image = image.subtract(count);
    }
};
}
#endif
