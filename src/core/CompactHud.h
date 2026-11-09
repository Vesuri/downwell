#ifndef DOWNWELL_CORE_COMPACT_HUD_H
#define DOWNWELL_CORE_COMPACT_HUD_H
#include <cstdint>
namespace downwell {
// Compact world-one combo and controller-HUD owners (W1.7): levelNumber 372,
// comboRewardText 383, objPlayer_n's combo counter and objControlerN's
// scrDrawHud4x3/scrDrawHud4x3Top Draw state. Draw-time mutations (offsets,
// slide-ins, flash frames, gem line, currency count) advance once per update
// at the world's Draw boundary; the backends only read. Q16 reals.

// levelNumber 372: Create from levelBeginCue. Alarm 0 reveals a letter every
// 4 updates, then Alarm 1 waits 120 and Alarm 2 lets one letter fall every 4.
// Draw raises each shown letter from -2 to 1; on exit, letters up to txtExit
// fall to 5 and those below 4 stay drawn. Font 2 advances eight pixels.
struct CompactLevelTitle {
    static constexpr unsigned capacity = 15;
    bool live = false;
    unsigned char text[capacity + 1] = {}; // one-based, as txtAt
    int offset[capacity + 1] = {};
    int length = 0, shown = 1, exit = 0;
    int alarm[3] = {-1, -1, -1};
    // title[] holds the area name (ASCII, no '#'); levels below 10 only need
    // two more cells, but any non-negative level is formatted.
    bool create(const unsigned* title, unsigned titleLength, int level) {
        unsigned n = 0;
        unsigned char cells[capacity];
        for (unsigned i = 0; i < titleLength; ++i) {
            if (n == capacity || title[i] > 0x7f) return false;
            cells[n++] = (unsigned char)(title[i] == '*' ? '#' : title[i]);
        }
        if (!titleLength) { if (n == capacity) return false; cells[n++] = ' '; }
        if (n == capacity) return false;
        cells[n++] = '-';
        unsigned magnitude = level < 0 ? 0u : unsigned(level), digits[10], count = 0;
        do { digits[count++] = magnitude % 10; magnitude /= 10; } while (magnitude);
        if (n + count > capacity) return false;
        while (count) cells[n++] = (unsigned char)('0' + digits[--count]);
        *this = {};
        live = true; length = int(n);
        for (int i = 1; i <= length; ++i) { text[i] = cells[i - 1]; offset[i] = -2; }
        alarm[0] = 4;
        return true;
    }
    // Draw x of the first letter cell in view coordinates: 80 - round(w/2).
    int left() const { return 80 - length * 4; }
    static constexpr int top = 84;
    void advanceAlarms() {
        if (!live) return;
        for (unsigned index = 0; index < 3 && live; ++index) {
            if (alarm[index] <= 0 || --alarm[index] != 0) continue;
            alarm[index] = -1;
            if (index == 0) {
                if (shown < length) { ++shown; alarm[0] = 4; } else alarm[1] = 120;
            } else if (index == 1) { exit = 1; alarm[2] = 4; }
            else if (exit < length) { ++exit; alarm[2] = 4; }
            else live = false;
        }
    }
    // Draw: is letter i drawn this update, and at which offset? Call before
    // advanceDraw for the update's picture.
    bool drawn(int i) const { return !exit ? i <= shown : offset[i] + (i <= exit && offset[i] <= 4) <= 3; }
    int drawOffset(int i) const { return exit && i <= exit && offset[i] <= 4 ? offset[i] + 1 : offset[i]; }
    void advanceDraw() {
        if (!live) return;
        for (int i = 1; i <= length; ++i) {
            if (!exit) { if (i <= shown && offset[i] <= 0) ++offset[i]; }
            else if (i <= exit && offset[i] <= 4) ++offset[i];
        }
    }
};

// comboRewardText 383, created by comboDone at (player xx, view y + 128).
// relativex is fixed at Create; Step sets relativey = y - (y - camera y)/2,
// which overrides Alarm 2's relativey -= 8 before any Draw. Draw clamps the
// view y to 80 and the centre to 30..130.
struct CompactComboRewardRecord {
    bool live = false;
    long relativeX = 0; // Q16 view x
    long y = 0, relativeY = 0; // Q16 room y
    unsigned combo = 0;
    int showBonus = 0, showBonusMax = 0;
    int alarm[3] = {-1, -1, -1};
    bool flashing = false;
    int drawing = 1;
    int powanReward = 4, powanGem = 4, powanBattery = 4, powanHealth = 4;
    int flashQuarters = 0; // flashFrame += 0.75 per drawn update
    void create(long playerXQ16, long viewXQ16, long viewYQ16, long cameraYQ16,
                unsigned count, int bonusMax) {
        *this = {};
        live = true; combo = count; showBonusMax = bonusMax;
        relativeX = playerXQ16 - viewXQ16;
        y = viewYQ16 + (128L << 16);
        alarm[2] = 2;
        alarm[1] = 45 + 15 * bonusMax + 45;
        alarm[0] = alarm[1] + 15;
        step(cameraYQ16);
    }
    void advanceAlarms() {
        // Alarm 0 destroys; 1 starts flashing; 2 reveals a bonus line.
        for (unsigned index = 0; index < 3 && live; ++index) {
            if (alarm[index] <= 0 || --alarm[index] != 0) continue;
            alarm[index] = -1;
            if (index == 0) live = false;
            else if (index == 1) flashing = true;
            else if (showBonus < showBonusMax) { ++showBonus; alarm[2] = 10; }
        }
    }
    void step(long cameraYQ16) {
        // y - (y - camera)/2; the halving of a real is exact in Q16 to 1/65536.
        relativeY = y - (y - cameraYQ16) / 2;
        if (flashing) drawing = -drawing;
    }
    // View position of the reward sprite (yyy, xxx), rounded to pixels.
    int drawX() const {
        long x = relativeX;
        if (x > (130L << 16)) x = 130L << 16;
        else if (x < (30L << 16)) x = 30L << 16;
        return int((x + 0x8000) >> 16);
    }
    int drawY(long viewYQ16) const {
        long v = relativeY - viewYQ16;
        if (v > (80L << 16)) v = 80L << 16;
        return int((v + 0x8000) >> 16);
    }
    bool visible() const { return drawing > 0; }
    int flashFrame() const { return flashQuarters < 32 ? flashQuarters / 4 : -1; }
    void advanceDraw() {
        if (!visible()) return;
        if (showBonus >= 1 && powanReward > 0) --powanReward;
        if (showBonus >= 2 && powanGem > 0) --powanGem;
        if (showBonus >= 3 && powanBattery > 0) --powanBattery;
        if (showBonus >= 4 && powanHealth > 0) --powanHealth;
        if (flashQuarters < 32) flashQuarters += 3;
    }
};

// scrDrawHud4x3Top's gem streak block (controller-local Draw state; the
// ascent is GemStreakRise) and scrDrawHud4x3's currencyText count.
struct CompactGemStreakHud {
    long gemLine = 0;      // Q16, multiples of 1/4
    long gemhighFrame = 0; // Q16
    unsigned time = 0;
    bool show = false;
    std::uint32_t currencyText = 0;
    static constexpr unsigned threshold = 100, timerStart = 300;
    void reset() { gemLine = gemhighFrame = 0; time = 0; show = false; }
    // Returns whether the streak block draws at all this update.
    bool draw(unsigned streak, unsigned timer, bool death, bool stopped) {
        if (!streak || death) return false;
        ++time;
        show = timer >= 120 || !(time & 1);
        if (stopped) { show = true; time = 0; }
        // 0.4 - (0.35 - 0.35 * timer / 300) = 0.05 + 0.35 * timer / 300.
        else gemhighFrame += 3277 + long((timer * 22938UL) / 300);
        if (!show) return true;
        const bool high = streak >= threshold;
        const long actual = high ? roundRatio(58UL * timer, timerStart)
                                 : roundRatio(54UL * streak, threshold);
        if (gemLine < (actual << 16)) {
            // ceil(actual - gemLine) / 4
            const long ceilDiff = ((actual << 16) - gemLine + 0xffff) >> 16;
            gemLine += ceilDiff << 14;
        }
        if (gemLine > (actual << 16)) gemLine = actual << 16;
        return true;
    }
    // round() of a non-negative ratio: GameMaker rounds half to even.
    static long roundRatio(unsigned long numerator, unsigned long denominator) {
        const unsigned long q = numerator / denominator, r = numerator % denominator;
        if (2 * r > denominator || (2 * r == denominator && (q & 1))) return long(q + 1);
        return long(q);
    }
    bool high(unsigned streak) const { return streak >= threshold; }
    int lineColumns() const { return int(gemLine >> 16); } // i <= gemLine
    // A sprite anchored at a Q16 x starts at ceil(x - 0.5).
    static int anchor(long q16) { return int((q16 - 0x8000 + 0xffff) >> 16); }
    int frameHigh() const { return int((gemhighFrame >> 16) % 14); }
    int capFrame() const { return int(time % 3); }
    void countCurrency(std::uint32_t currency) {
        const std::uint32_t diff = currencyText > currency ? currencyText - currency : currency - currencyText;
        const std::uint32_t step = diff > 500 ? 100 : diff > 100 ? 10 : diff > 50 ? 5 : 1;
        if (currencyText > currency) currencyText -= step;
        else if (currencyText < currency) currencyText += step;
    }
};

// drawComboNumber(x, y - 11, comboCount) in playerDraw while comboCount >= 5:
// sprComboNumber 404 digits every six pixels, centred by 3 per extra digit,
// then one to three milestone icons (11, 12, 13) six pixels above.
template<class Draw> bool drawComboNumber(int x, int y, unsigned count, Draw draw) {
    unsigned digits[10], n = 0, value = count;
    do { digits[n++] = value % 10; value /= 10; } while (value);
    const int adjust = n >= 2 ? -int(n - 1) * 3 : 0;
    for (unsigned i = 0; i < n; ++i)
        if (!draw(404, digits[n - 1 - i], x + int(i) * 6 + adjust, y)) return false;
    if (count >= 25) return draw(404, 11, x - 6, y - 6) && draw(404, 12, x, y - 6) && draw(404, 13, x + 6, y - 6);
    if (count >= 15) return draw(404, 11, x - 3, y - 6) && draw(404, 12, x + 3, y - 6);
    if (count >= 8) return draw(404, 11, x, y - 6);
    return true;
}

// comboRewardText Draw (the surface branch) at view position (x, y).
template<class Draw> bool drawComboReward(const CompactComboRewardRecord& r, int x, int y, Draw draw) {
    if (!r.visible()) return true;
    // drawComboExc(x, y - 13, comboNum): digits, then sprComboExc 406.
    unsigned digits[10], n = 0, value = r.combo;
    do { digits[n++] = value % 10; value /= 10; } while (value);
    const int adjust = r.combo >= 10 ? -21 : -18;
    unsigned i = 0;
    for (; i < n; ++i)
        if (!draw(404, digits[n - 1 - i], x + int(i) * 6 + adjust, y - 13)) return false;
    if (!draw(406, 0, x + int(i) * 6 + adjust, y - 13)) return false;
    if (r.showBonus >= 1 && !draw(405, 0, x, y)) return false;
    if (r.showBonus >= 2) {
        const int cy = y + 12 + r.powanGem;
        if (!draw(404, 10, x - 12, cy) || !draw(404, 1, x - 6, cy) || !draw(404, 0, x, cy) ||
            !draw(404, 0, x + 6, cy) || !draw(404, 11, x + 13, cy)) return false;
    }
    if (r.showBonus >= 3) {
        const int cy = y + 12 + r.powanBattery + 7;
        if (!draw(404, 10, x - 6, cy) || !draw(404, 1, x, cy) || !draw(404, 12, x + 6, cy)) return false;
    }
    if (r.showBonus >= 4) {
        const int cy = y + 12 + r.powanHealth + 14;
        if (!draw(404, 10, x - 6, cy) || !draw(404, 1, x, cy) || !draw(404, 13, x + 6, cy)) return false;
    }
    const int flash = r.flashFrame();
    return flash < 0 || draw(118, unsigned(flash), x - 1, y - 13);
}
} // namespace downwell
#endif
