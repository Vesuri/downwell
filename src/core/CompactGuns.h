#ifndef DOWNWELL_CORE_COMPACT_GUNS_H
#define DOWNWELL_CORE_COMPACT_GUNS_H
#include "CompactScenery.h"
#include "GunTable.h"
#include "UnitSequenceFrames.h"
#include <cstdint>
namespace downwell {
// Compact world-one gun owners (W1.4): bulletCasing 267, fxLaserTrail 270,
// GunModule 109, moduleText 373 and itemNotif 374. Records keep creation
// order; births of a Step wait for the next one (birth == tick). The world
// supplies collision queries; the event bodies that need none live here.
// Q16 reals: the compact world's arithmetic, not the binary64 VM path.

// bulletCasing: Create xsp = random_range(-3,-1)*sign(player image_xscale),
// ysp = random_range(-3,0), image_speed = choose(-0.5,-0.3,0,0.3,0.5),
// Alarm 0 = 15 (flashing, Alarm 1 = 15), Alarm 1 destroys. Upgrade 8 (Hot
// Casing, pugLeak) is W1.9: without it the casing deals no damage.
struct CompactCasingRecord {
    bool live = false;
    long x = 0, y = 0, xsp = 0, ysp = 0; // room coordinates, Q16
    int frameTenths = 0, speedTenths = 0;
    int alarm[2] = {15, -1};
    bool flashing = false;
    int dFlash = 1;
    std::uint32_t birth = 0;
    static constexpr int frames = 8; // sprBulletLeak 457
    template<class Random> void create(Random& random, int px, int py, int facing) {
        x = long(px) << 16; y = long(py) << 16;
        // floor(word * width / 2^16) in 32-bit steps (68000: no 64-bit helpers).
        const auto scaled = [](std::uint32_t word, std::uint32_t width) {
            return long((word >> 16) * width + (((word & 0xffffu) * width) >> 16));
        };
        const long u = scaled(random.nextU32(), 2u);
        xsp = (u - (3L << 16)) * (facing < 0 ? -1 : 1);
        ysp = scaled(random.nextU32(), 3u) - (3L << 16);
        static constexpr int speeds[] = {-5, -3, 0, 3, 5};
        speedTenths = speeds[random.chooseIndexSmall(5)];
    }
    void advanceAnimation() {
        frameTenths = (frameTenths + speedTenths + frames * 10) % (frames * 10);
    }
    unsigned frame() const { return unsigned(frameTenths / 10); }
    bool visible() const { return dFlash > 0; }
    // Alarm index 0 or 1; returns false when Alarm 1 destroys the casing.
    bool alarmEvent(unsigned index) {
        if (alarm[index] <= 0 || --alarm[index] != 0) return true;
        alarm[index] = -1;
        if (index == 0) { flashing = true; alarm[1] = 15; return true; }
        live = false; return false;
    }
    // Step before its wall queries: the flash toggle and gravity.
    void beginStep() {
        if (flashing) dFlash = -dFlash;
        ysp += ysp < -(1L << 16) ? 13107 : 5243; // ugravhard 0.2, ugrav 0.08
    }
};

// fxLaserTrail: image_index from bulletLaser, image_speed 0.8, Animation
// End destroys. bulletLaser's trails never move (xsp = ysp = 0).
struct CompactLaserTrailRecord {
    bool live = false;
    int x = 0, y = 0, sprite = 466;
    RunnerImageValue index{};
    std::uint32_t birth = 0;
    bool advanceAnimation() {
        RunnerImageValue image; bool crossed = false;
        if (!UnitSequenceFrames::advance(index, {0x3f4ccccdu}, 13, image, crossed)) return false;
        if (crossed) live = false;
        return true;
    }
    unsigned frame() const {
        const unsigned exponent = (index.bits >> 23) & 255u;
        return exponent < 127 ? 0 : ((index.bits & 0x7fffffu) | 0x800000u) >> (150u - exponent);
    }
};

// GunModule: the builder already drew moduleType/moduleNum. Alarm 0 (10)
// makes it obtainable; obtime is 0, so Alarm 1 and the disappearing branch
// never run. Alarm 3 flashes frame 1 for 5 Steps every 65.
struct CompactGunModuleRecord {
    bool live = false;
    int x = 0, y = 0;
    long yy = 0, ysp = 0; // Q16
    int type = 0, gun = 0, image = 0, objHp = 1;
    int alarm0 = 10, alarm3 = 10;
    bool obtainable = false;
    std::uint32_t birth = 0;
    int sprite() const { return type ? 140 : 139; } // sprModHeart, sprModBtry
    int letter() const { return gunTable[gun][0].letter; } // bStatGunSprite
    void alarmEvent(unsigned index) {
        if (index == 0 && alarm0 > 0 && --alarm0 == 0) { alarm0 = -1; obtainable = true; }
        if (index == 3 && alarm3 > 0 && --alarm3 == 0) {
            if (image == 1) { image = 0; alarm3 = 60; } else { image = 1; alarm3 = 5; }
        }
    }
};

// moduleText: the gun name, letter by letter, then itemNotif from Draw.
struct CompactModuleTextRecord {
    static constexpr int maximumLength = 10; // MACHINEGUN
    bool live = false;
    int x = 0, y = 0, gun = 0, type = 0, amount = 0;
    int length = 0, shown = 0, exit = 0;
    int frames[maximumLength + 1] = {};      // txtAt[i][2], 1-based
    int alarm[4] = {-1, -1, -1, 2};
    bool allSet = false, allShown = false;
    std::uint32_t birth = 0;
    int width() const { return length * 8; } // font 2 advances eight pixels
    char character(int i) const { return gunTable[gun][0].name[i - 1]; }
    // First Step: txtShown = 1, Alarm 0 = txtApInterval.
    void step() {
        if (allSet) return;
        length = 0;
        while (gunTable[gun][0].name[length]) ++length;
        shown = 1;
        for (int i = 1; i <= length; ++i) frames[i] = 0;
        alarm[0] = 5; exit = 0; allSet = true;
    }
    // Returns false when Alarm 2 destroys the record.
    bool alarmEvent(unsigned index) {
        if (alarm[index] <= 0 || --alarm[index] != 0) return true;
        alarm[index] = -1;
        switch (index) {
        case 0:
            if (shown < length) { ++shown; alarm[0] = 5; } // soundPlay(334) is W2
            else alarm[1] = 120;
            return true;
        case 1: exit = 1; alarm[2] = 4; return true;
        case 2:
            if (exit < length) { ++exit; alarm[2] = 4; return true; }
            live = false; return false;
        default:
            for (int i = 1; i <= length; ++i)
                if (i <= shown && frames[i] < 8) ++frames[i];
            alarm[3] = 2;
            return true;
        }
    }
    // Draw's state changes: the exit branch's frame advance and the one
    // itemNotif birth. Returns true when itemNotif must be created now.
    bool drawEvent() {
        if (!allSet) return false;
        if (!exit) {
            if (!allShown && frames[length] >= 8) { allShown = true; return true; }
            return false;
        }
        for (int i = 1; i <= length; ++i)
            if (i <= exit && frames[i] <= 4) ++frames[i];
        return false;
    }
    // Drawn letter offset for frame f before exit (stepy * 2).
    static int bounce(int frame) {
        static constexpr int offsets[] = {5, 3, 1, -1, -3, -2, -1, 1, 0};
        return offsets[frame < 0 ? 0 : frame > 8 ? 8 : frame] * 2;
    }
    // Left edge after the view clamp (view left/width in room pixels).
    int left(int viewLeft, int viewWidth) const {
        int txtx = x - width() / 2;
        const int over = x + width() / 2 - (viewLeft + viewWidth);
        const int under = x - width() / 2 - viewLeft;
        if (over > -10) txtx = txtx - over - 10;
        else if (under < 10) txtx = txtx - under + 10;
        return txtx;
    }
};

// itemNotif: rises with ysp *= 0.85, flashes after Alarm 1 (60), Alarm 0
// (90) destroys it. Draws "+ n" centred and sprItemNotif frame notifType.
// fxRisingText has the same motion, alarms and view clamp; `empty` selects
// scrPlayerShootN's centred "EMPTY!" without an icon.
struct CompactItemNoticeRecord {
    bool live = false, empty = false;
    int x = 0, type = 0, amount = 0;
    long y = 0, ysp = -(3L << 16); // Q16
    int alarm[2] = {90, 60};
    bool flashing = false;
    int drawing = 1;
    std::uint32_t birth = 0;
    int pixelY() const { return int((y + (1L << 15)) >> 16); }
    bool visible() const { return drawing > 0; }
    bool alarmEvent(unsigned index) {
        if (alarm[index] <= 0 || --alarm[index] != 0) return true;
        alarm[index] = -1;
        if (index == 1) { flashing = true; return true; }
        live = false; return false;
    }
    void step() {
        y += ysp;
        ysp = (ysp * 17) / 20; // decreaseBy 0.85
        if (flashing) drawing = -drawing;
    }
    // text width "+ n": eight pixels per character.
    int left(int viewLeft, int viewWidth) const {
        const int width = empty ? 48 : amount >= 10 ? 32 : 24;
        int xxx = x;
        const int over = x + width / 2 - (viewLeft + viewWidth);
        const int under = x - width / 2 - viewLeft;
        if (over > -10) xxx = x - over - 10;
        else if (under < 10) xxx = x - under + 10;
        return xxx;
    }
};

struct CompactGuns {
    static constexpr unsigned casingCapacity = 16, trailCapacity = 96, moduleCapacity = 8;
    static constexpr unsigned textCapacity = 4, noticeCapacity = 4;
    CompactCasingRecord casings[casingCapacity];
    CompactLaserTrailRecord trails[trailCapacity];
    CompactGunModuleRecord modules[moduleCapacity];
    CompactModuleTextRecord texts[textCapacity];
    CompactItemNoticeRecord notices[noticeCapacity];
    unsigned casingEnd = 0, trailEnd = 0, moduleEnd = 0, textEnd = 0, noticeEnd = 0;
    void clear() { casingEnd = trailEnd = moduleEnd = textEnd = noticeEnd = 0; }
    bool empty() const { return !casingEnd && !trailEnd && !moduleEnd && !textEnd && !noticeEnd; }
    template<class Record, unsigned Capacity>
    static Record* append(Record (&records)[Capacity], unsigned& end) {
        return CompactScenery::append(records, end);
    }
};
}
#endif
