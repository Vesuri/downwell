#ifndef DOWNWELL_CORE_GARDEN_GROUND_H
#define DOWNWELL_CORE_GARDEN_GROUND_H

// W1.14: rmMenu's solids, as the garden's objPlayer_n meets them. Room
// coordinates in pixels; all boxes are inclusive.
// - 162 objWall_n 60 (sprTile1, origin 8), material foliage: columns
//   x -32..288 and 368..448, rows y 528..608, so the floor top is 520 and the
//   well's gap is x 296..359.
// - objWell 315 at (272,512) creates four objInvisWall 98 (rock) at x 272,
//   288, 368 and 384, y 512: the rim, a 16 px step above the floor.
// - objInvisWall 104223 (-16,288) and 104222 (432,480), scaleY 100: the room
//   ends. 104226 (288,704) and 104225 (368,704), scaleY 11: the shaft.
// - groundBench 318 (parentThinwall 87, metal) at (112,512): sprBench's top
//   row 514. A thin wall carries the player only from above.
// The player's mask is sprPlayerIdle's x-4..x+3, y-4..y+7 whatever it shows.
namespace downwell {

struct GardenGround {
    enum Material : unsigned char { foliage, rock, metal };
    // soundLand's global.sfxJl and soundFootstep's global.sfxFs first ids.
    static int landingSound(Material m) { return m == metal ? 77 : m == rock ? 73 : 69; }
    static int footstepSound(Material m) { return m == metal ? 59 : m == rock ? 55 : 51; }

    static constexpr int floorStand = 512, rimStand = 496, benchStand = 506;
    // No support below (the shaft): objTeleportMain ends the fall first.
    static constexpr int noLanding = 4096;

    static bool span(int left, int right, int a, int b) { return right >= a && left <= b; }
    static bool floorColumns(int left, int right) {
        return span(left, right, -40, 295) || span(left, right, 360, 455);
    }
    static bool rimColumns(int left, int right) {
        return span(left, right, 264, 295) || span(left, right, 360, 391);
    }
    static bool benchColumns(int left, int right) { return span(left, right, 101, 124); }

    // sParentSolid at one pixel, except the bench (a thin wall never blocks
    // from the side or below).
    static bool solid(int x, int y) {
        if (y >= 520 && y <= 615 && floorColumns(x, x)) return true;
        if (y >= 504 && y <= 519 && rimColumns(x, x)) return true;
        if (y >= 616 && y <= 791 && (span(x, x, 280, 295) || span(x, x, 360, 375))) return true;
        if (x >= -24 && x <= -9 && y >= -512 && y <= 1087) return true;
        return x >= 424 && x <= 439 && y >= -320 && y <= 1279;
    }

    // The standing y of the highest support at or below minimumY for a
    // player at x: the floor, the rim tops, or the bench from above.
    static int landing(int x, int minimumY) {
        const int left = x - 4, right = x + 3;
        int nearest = noLanding;
        if (minimumY <= benchStand && benchColumns(left, right)) nearest = benchStand;
        if (minimumY <= rimStand && rimColumns(left, right) && rimStand < nearest) nearest = rimStand;
        if (minimumY <= floorStand && floorColumns(left, right) && floorStand < nearest) nearest = floorStand;
        return nearest;
    }

    // soundLand/soundFootstep's instance_place(x, y + 2, sParentSolid).
    // Under the bench the probe meets both the bench and the floor; the
    // floor's walls come first in the room's instance order (approximation:
    // the runner's parent iteration order is not confirmed).
    static Material under(int x, int y) {
        const int left = x - 4, right = x + 3, top = y - 2, bottom = y + 9;
        if (span(top, bottom, 504, 519) && rimColumns(left, right)) return rock;
        if (span(top, bottom, 520, 615) && floorColumns(left, right)) return foliage;
        if (span(top, bottom, 514, 515) && benchColumns(left, right)) return metal;
        return foliage;
    }
};

} // namespace downwell

#endif
