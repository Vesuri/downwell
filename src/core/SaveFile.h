#ifndef DOWNWELL_CORE_SAVE_FILE_H
#define DOWNWELL_CORE_SAVE_FILE_H
#include <cstdint>
namespace downwell {
// The persistent subset of save.ini that scrLoadSave reads and the pinned
// build's writers (saveStats, comboDone, PlayMenu, OptionMenuPC,
// ShaderMenu/scrShaderToggle) write, with the same sections, keys and
// defaults. GameMaker's -1/1 toggles are kept as booleans (GML truth is
// value > 0.5); masterGain is kept in the tenths its menu steps by.
// Language, display and touch keys have no Amiga meaning and are not kept.
struct SaveFile {
    std::uint64_t totalGems = 0;          // stats.gems
    unsigned style = 0;                   // stats.style
    unsigned shader = 0;                  // stats.shader
    bool noBgm = false;                   // stats.nobgm (-1)
    bool hardUnlocked = false;            // stats.hardUnlocked (-1)
    unsigned furthestReached = 0;         // stats.recordFurthestReached
    std::uint32_t bestRunGems = 0;        // stats.recordRunGem
    std::uint32_t bestCombo = 0;          // stats.recordMaxCombo
    std::int32_t fastestGame = -1;        // stats.recordFastestGame (frames)
    std::int32_t fastestGameHard = -1;    // stats.recordFastestGameHard
    unsigned masterGainTenths = 10;       // option.masterGain: 1.0 (the original 0.6)
    bool hardMode = false;                // option.hardMode (-1)
    bool showTimer = false;               // option.showtimer (-1)

    bool operator==(const SaveFile& o) const {
        return totalGems == o.totalGems && style == o.style && shader == o.shader &&
            noBgm == o.noBgm && hardUnlocked == o.hardUnlocked &&
            furthestReached == o.furthestReached && bestRunGems == o.bestRunGems &&
            bestCombo == o.bestCombo && fastestGame == o.fastestGame &&
            fastestGameHard == o.fastestGameHard && masterGainTenths == o.masterGainTenths &&
            hardMode == o.hardMode && showTimer == o.showTimer;
    }
    bool operator!=(const SaveFile& o) const { return !(*this == o); }

    // GML reads numbers as doubles. Parse an optional sign, digits and a
    // fraction in tenths; anything else on the line leaves the default.
    struct Number { bool valid = false, negative = false; std::uint64_t whole = 0; unsigned tenths = 0; };
    static Number number(const char* p, const char* end) {
        Number n;
        while (p < end && (*p == ' ' || *p == '"')) ++p;
        if (p < end && (*p == '-' || *p == '+')) n.negative = *p++ == '-';
        bool digits = false;
        while (p < end && *p >= '0' && *p <= '9') {
            if (n.whole < 100000000000000ULL) n.whole = n.whole * 10 + unsigned(*p - '0');
            ++p; digits = true;
        }
        if (p < end && *p == '.') {
            ++p;
            // Round the remaining fraction to tenths (0.6 is stored 0.600000;
            // repeated -0.1 steps drift to values like 0.29999999).
            unsigned scaled = 0, place = 0;
            while (p < end && *p >= '0' && *p <= '9') {
                if (place < 3) scaled = scaled * 10 + unsigned(*p - '0');
                ++place; ++p; digits = true;
            }
            while (place < 3) { scaled *= 10; ++place; }
            n.tenths = std::uint16_t(scaled + 50) / 100u; // scaled <= 999
            if (n.tenths == 10) { n.tenths = 0; ++n.whole; }
        }
        n.valid = digits;
        return n;
    }
    static bool truth(const Number& n) { return !n.negative && (n.whole > 0 || n.tenths > 5); }
    static unsigned clampUnsigned(const Number& n, unsigned maximum) {
        if (n.negative) return 0;
        const std::uint64_t value = n.whole + (n.tenths >= 5 ? 1 : 0);
        return value > maximum ? maximum : unsigned(value);
    }
    static std::int32_t frames(const Number& n) {
        if (n.negative) return -1;
        return n.whole > 0x7fffffff ? 0x7fffffff : std::int32_t(n.whole);
    }

    static bool same(const char* a, const char* aEnd, const char* b) {
        while (a < aEnd && *b) if (*a++ != *b++) return false;
        return a == aEnd && !*b;
    }

    // Unknown sections, keys and malformed lines are ignored, as ini_read_real
    // returns the default for them.
    static SaveFile parse(const char* text, unsigned length) {
        SaveFile s;
        const char* p = text;
        const char* const end = text + length;
        enum { none, stats, option } section = none;
        while (p < end) {
            const char* line = p;
            while (p < end && *p != '\n' && *p != '\r') ++p;
            const char* lineEnd = p;
            while (p < end && (*p == '\n' || *p == '\r')) ++p;
            while (line < lineEnd && (*line == ' ' || *line == '\t')) ++line;
            if (line == lineEnd || *line == ';' || *line == '#') continue;
            if (*line == '[') {
                const char* close = line + 1;
                while (close < lineEnd && *close != ']') ++close;
                section = same(line + 1, close, "stats") ? stats :
                          same(line + 1, close, "option") ? option : none;
                continue;
            }
            const char* equals = line;
            while (equals < lineEnd && *equals != '=') ++equals;
            if (equals == lineEnd) continue;
            const char* keyEnd = equals;
            while (keyEnd > line && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t')) --keyEnd;
            const Number n = number(equals + 1, lineEnd);
            if (!n.valid) continue;
            const auto key = [&](const char* name) { return same(line, keyEnd, name); };
            if (section == stats) {
                if (key("gems")) s.totalGems = n.negative ? 0 : n.whole;
                else if (key("style")) s.style = clampUnsigned(n, 4);
                else if (key("shader")) s.shader = clampUnsigned(n, 255);
                else if (key("nobgm")) s.noBgm = truth(n);
                else if (key("hardUnlocked")) s.hardUnlocked = truth(n);
                else if (key("recordFurthestReached")) s.furthestReached = clampUnsigned(n, 1000);
                else if (key("recordRunGem")) s.bestRunGems = n.negative ? 0 : std::uint32_t(n.whole > 0xffffffffULL ? 0xffffffffULL : n.whole);
                else if (key("recordMaxCombo")) s.bestCombo = n.negative ? 0 : std::uint32_t(n.whole > 0xffffffffULL ? 0xffffffffULL : n.whole);
                else if (key("recordFastestGame")) s.fastestGame = frames(n);
                else if (key("recordFastestGameHard")) s.fastestGameHard = frames(n);
            } else if (section == option) {
                if (key("masterGain")) {
                    const unsigned tenths = n.negative ? 0 : unsigned(n.whole > 1 ? 10 : n.whole * 10 + n.tenths);
                    s.masterGainTenths = tenths > 10 ? 10 : tenths;
                }
                else if (key("hardMode")) s.hardMode = truth(n);
                else if (key("showtimer")) s.showTimer = truth(n);
            }
        }
        return s;
    }

    // ini_write_real's text: quoted, six decimals.
    struct Writer {
        char* out; unsigned capacity, length = 0; bool good = true;
        void put(char c) { if (length < capacity) out[length++] = c; else good = false; }
        void text(const char* s) { while (*s) put(*s++); }
        void digits(std::uint64_t v) {
            char buffer[24]; unsigned n = 0;
            // Base 65536 long division on 32-bit halves: 32-bit divisions
            // and no variable 64-bit shifts (no 64-bit helpers on the 68000).
            std::uint32_t high = std::uint32_t(v >> 32), low = std::uint32_t(v);
            do {
                std::uint32_t r = 0, half[2] = {high, low};
                for (auto& word : half) {
                    const std::uint32_t upper = (r << 16) | (word >> 16);
                    r = upper % 10;
                    const std::uint32_t lower = (r << 16) | (word & 0xffff);
                    r = lower % 10;
                    word = ((upper / 10) << 16) | (lower / 10);
                }
                high = half[0]; low = half[1];
                buffer[n++] = char('0' + r);
            } while (high | low);
            while (n) put(buffer[--n]);
        }
        void real(const char* key, bool negative, std::uint64_t whole, unsigned tenths) {
            text(key); text("=\"");
            if (negative) put('-');
            digits(whole); put('.'); put(char('0' + tenths)); text("00000\"\r\n");
        }
        void integer(const char* key, std::int64_t v) {
            real(key, v < 0, std::uint64_t(v < 0 ? -v : v), 0);
        }
        void toggle(const char* key, bool v) { integer(key, v ? 1 : -1); }
    };
    // Returns the byte count, or 0 if the buffer is too small.
    unsigned format(char* out, unsigned capacity) const {
        Writer w{out, capacity};
        w.text("[stats]\r\n");
        w.real("gems", false, totalGems, 0);
        w.integer("style", style);
        w.integer("shader", shader);
        w.toggle("nobgm", noBgm);
        w.toggle("hardUnlocked", hardUnlocked);
        w.integer("recordFurthestReached", furthestReached);
        w.integer("recordRunGem", bestRunGems);
        w.integer("recordMaxCombo", bestCombo);
        w.integer("recordFastestGame", fastestGame);
        w.integer("recordFastestGameHard", fastestGameHard);
        w.text("[option]\r\n");
        w.real("masterGain", false, std::uint16_t(masterGainTenths) / 10u, std::uint16_t(masterGainTenths) % 10u); // [0, 10]
        w.toggle("hardMode", hardMode);
        w.toggle("showtimer", showTimer);
        return w.good ? w.length : 0;
    }
    static constexpr unsigned formatCapacity = 512;
};
}
#endif
