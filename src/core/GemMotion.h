#ifndef DOWNWELL_CORE_GEM_MOTION_H
#define DOWNWELL_CORE_GEM_MOTION_H
#include "GemHoming.h"
#include "GemLifetime.h"
#include "GemAnimation.h"
#include "EnemyContact.h"
#include "CurrencyStepSound.h"
#include "GemPosition.h"
namespace downwell {
// Positions in 1/12800 pixels keep .035 gravity exact, with no 64-bit math.
// Terrain probes remain provisional, not the source sprite-mask solver.
template<class Position> struct BasicGemMotion {
    Position x{},y{};
    long vx = 0, vy = 0;
    long gravity = 448, maximumGravity = 4 * 12800L;
    GemHoming homing;
    GemLifetime lifetime;
    GemAnimation animation;
    CurrencyStepSound sound;
    bool attracted = false;
    bool attractorAtBirth = false;
    static long q8(long value) { return value < 0 ? -((-value + 25) / 50) : (value + 25) / 50; }
    static long q8(const GemPosition& value){return value.q8();}
    static int pixel(const GemPosition& value){return static_cast<int>(value.pixel());}
    static long rounded(const GemPosition& value){return value.rounded();}
    static void initializePosition(long& value,int pixel){value=pixel*12800L;}
    static void initializePosition(GemPosition& value,int pixel){value.setPixel(pixel);}
    static int pixel(long value) { return static_cast<int>(value / 12800 - (value % 12800 < 0)); }
    static long rounded(long value) {
        const long whole = value / 12800, fraction = value % 12800;
        if (fraction > 6400 || (fraction == 6400 && whole % 2 != 0)) return whole + 1;
        if (fraction < -6400 || (fraction == -6400 && whole % 2 != 0)) return whole - 1;
        return whole;
    }
    // Serialized AxisAlignedRect masks, with rounded instance positions.
    // Round directly from authoritative units, not through a Q8 midpoint.
    bool collectible(long playerX, long playerY, bool large = false) const {
        const ContactBounds player = contactWorldBounds({playerX, playerY, 1, 1, 31});
        const ContactBounds gem = contactWorldBounds({rounded(x) * 256, rounded(y) * 256,
                                                     1, 1, large ? 134 : 133});
        return lifetime.obtainable() && player.left < gem.right && gem.left < player.right &&
            player.top < gem.bottom && gem.top < player.bottom;
    }
    template<class Random>
    void initialize(int px, int py, bool attract, Random& random, bool attractor = false, int area = 1) {
        lifetime = {};
        sound = {};
        attractorAtBirth = attractor;
        initializePosition(x,px);initializePosition(y,py);attracted = attract;
        vx = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        vy = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        // objCurrency Create consumes both ordinary velocity draws before
        // area 5 replaces ysp. Do not skip the discarded second draw.
        if(area==5)vy=-static_cast<long>(random.randomIntegerSmall(38400));
        gravity=(area==3||area==4)?112:448;
        maximumGravity=(area==3||area==4)?6400:4*12800L;
        animation.initialize(random);
        homing.initialize(random);
    }
    template<class Solid, class Random>
    bool step(long targetX, long targetY, Solid solid, Random& random, bool large=false) {
        if (!lifetime.advance(attracted)) return false;
        sound.begin(large?1:0,random);
        // Animation belongs to the world's earlier animation traversal, not
        // this movement/collection phase. A gem born during Step stays at its
        // Create image index until the next frame's animation traversal.
        const long dx = targetX - q8(x), dy = targetY - q8(y);
        if (attracted) {
            long sx, sy;
            homing.step(dx, dy, sx, sy);
            vx = sx * 50; vy = sy * 50;
            x += vx; y += vy;
            return true;
        }
        if (vy < maximumGravity) vy += gravity;
        else vy = maximumGravity;
        const Position nextX=x+vx;
        if (solid(pixel(nextX), pixel(y))) vx = -vx * 7 / 10;
        else x = nextX;
        const Position nextY=y+vy;
        if (vy > 0 && solid(pixel(x), pixel(nextY) + 4)) {
            // Existing bounce approximation; source irandom_range with
            // fractional negative endpoints still needs runner verification.
            vy = -6400 - static_cast<long>(random.randomIntegerSmall(8960));
            vx = vx * 7 / 10;
        } else y = nextY;
        if (vx > -640 && vx < 640) vx = vx < 0 ? -640 : vx > 0 ? 640 : 0;
        const long radius = (attractorAtBirth ? 48L : 24L) * 256;
        if (lifetime.suckable(attractorAtBirth) && dx > -radius && dx < radius &&
            dy > -radius && dy < radius) {
            attracted = true;
            homing.acquire(dx, dy);
        }
        return true;
    }
};
using ReferenceGemMotion=BasicGemMotion<long>;
#ifdef DOWNWELL_REFERENCE_GEM_POSITION
using GemMotion=ReferenceGemMotion;
#else
using GemMotion=BasicGemMotion<GemPosition>;
#endif
} // namespace downwell
#endif
