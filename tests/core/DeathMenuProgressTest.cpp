#include "core/DeathMenuProgress.h"
#include "core/RunGemStatistics.h"
#include "core/DeathResults.h"
#include "core/DeathResultsProgressBar.h"
#include "core/DeathResultsProgressLabels.h"
#include <vector>
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    struct Labels {
        struct Glyph { unsigned code; int x, y; unsigned pen; };
        std::vector<Glyph> glyphs;
        unsigned assets = 0;
        void asset(int id, unsigned frame, int x, int y) {
            require(id == 402 && frame == 0 && x == 80 && y == 158, "progress label source art anchor");
            ++assets;
        }
        void progressGlyph(unsigned code, int x, int y, unsigned pen) { glyphs.push_back({code,x,y,pen}); }
    } labels;
    downwell::drawProgressNumber(labels, 99999999999ULL, 72, 166, true);
    require(labels.glyphs.size() == 55, "wide lifetime total is eleven digits plus four outline passes");
    require(labels.glyphs[44].x == -12 && labels.glyphs[54].x == 68,
            "right aligned total uses string advance, with viewport clipping left to backend");
    for (unsigned i = 0; i < labels.glyphs.size(); ++i)
        require(labels.glyphs[i].code == 9 && labels.glyphs[i].pen == (i < 44 ? 3u : 1u),
                "complete black text outlines precede every foreground glyph");
    downwell::DeathResults labelResults;
    labelResults.open(true, 84, 84);
    labels = {};
    downwell::drawDeathResultsProgressLabels(labels, labelResults);
    require(labels.glyphs.empty() && labels.assets == 0, "hidden progress row has no labels");
    labelResults.menu.visible[5] = true;
    labelResults.frame.displayedTotal = 84;
    downwell::drawDeathResultsProgressLabels(labels, labelResults);
    require(labels.assets == 1 && labels.glyphs.size() == 35 &&
                labels.glyphs[8].code == 8 && labels.glyphs[8].x == 60 &&
                labels.glyphs[9].code == 4 && labels.glyphs[9].x == 68 &&
                labels.glyphs[14].code == 10 && labels.glyphs[14].x == 80 &&
                labels.glyphs[31].code == 1 && labels.glyphs[31].x == 92,
            "total is right aligned, slash centered and goal left aligned");
    labels = {}; labelResults.progress.maxed = true;
    downwell::drawDeathResultsProgressLabels(labels, labelResults);
    require(labels.glyphs.size() == 17 && labels.glyphs[15].code == 11 &&
                labels.glyphs[15].pen == 2 && labels.glyphs[15].y == 167 &&
                labels.glyphs[16].pen == 1 && labels.glyphs[16].y == 166,
            "maxed label uses source red drop shadow instead of black outline");
    using Bar = downwell::DeathResultsProgressBar;
    for (const std::uint64_t span : {1000ULL, 2500ULL, 99999499999ULL})
        for (unsigned i = 0; i <= 100; ++i) {
            const auto value = span * i / 100;
            require(Bar::pixels(value, span) == value * 96 / span,
                    "native fraction calculation matches wide host arithmetic");
        }
    downwell::DeathResults barResults;
    barResults.open(true, 1500, 1000);
    barResults.menu.visible[5] = barResults.menu.visible[6] = true;
    barResults.presentationStep();
    auto bar = Bar::from(barResults);
    require(bar.visible && bar.length == 48 && bar.separator == 48 && bar.y == 183,
            "bar uses pre-increment displayed total, original separator and reveal offset");
    barResults.progress.separatorTotal = 31;
    require(Bar::from(barResults).separator == -1, "separator below three pixels hidden");
    barResults.progress.separatorTotal = 32;
    require(Bar::from(barResults).separator == 3, "fractional separator above three survives flooring");
    struct BarSink {
        unsigned red = 0, white = 0;
        void barRect(int x, int y, int w, int h, bool isRed) {
            require(x >= 31 && x <= 129 && y >= 176 && y <= 184 && w > 0 && h == 1,
                    "bar stroke bounds and positive widths");
            if (isRed) ++red; else ++white;
        }
    } sink;
    bar.draw(sink);
    require(sink.red == 6 && sink.white == 3, "source rounded bar and previous-total marker strokes");
    bar.maxed = true; bar.separator = -1; sink = {};
    bar.draw(sink);
    require(sink.red == 0 && sink.white == 0, "maxed mode suppresses ordinary bar");
    downwell::DeathResults results;
    downwell::Input held;
    held.fire = true;
    results.beginStep(held);
    results.open(true, 80, 84);
    results.presentationStep();
    require(results.active && results.progress.animatedTotal == 0,
            "entry uses saved lifetime and current run gems separately");
    results.beginStep(held);
    require(results.menu.resultShown == -1 && results.menu.revealAlarm == 44,
            "held skip input is not replayed when results opens");
    results.open(true, 999, 999);
    require(results.progress.total == 80, "repeated menu-ready flag cannot recreate results");
    results.beginStep({});
    results.beginStep(held);
    require(results.menu.resultShown == 6 && results.menu.inputDisabled,
            "new press reaches shared results sequence");
    downwell::RunGemStatistics stats;
    require(!stats.observeDeathMovement(false, 80), "no save at health depletion/publication");
    require(stats.observeDeathMovement(true, 80) && stats.lifetime == 80 && stats.record == 80,
            "first death movement saves run once");
    require(!stats.observeDeathMovement(true, 84) && stats.lifetime == 80 && stats.savedRunGems == 80,
            "post-death gems do not retroactively change saved statistics");
    stats.beginRun();
    require(stats.observeDeathMovement(true, 20) && stats.lifetime == 100 && stats.record == 80,
            "new run preserves lifetime and best-run record");
    stats.beginRun();
    require(stats.observeDeathMovement(true, 90) && stats.lifetime == 190 && stats.record == 90,
            "new high score replaces record");
    using downwell::DeathMenuProgress;
    using downwell::DeathMenuSequence;
    for (const unsigned divisor : {15u, 30u})
        for (const std::uint64_t base : {0ULL, 1000ULL, 0xffffffffULL, 99999999999ULL})
            for (unsigned offset = 0; offset < 64; ++offset)
                require(DeathMenuProgress::ceilingStep(base + offset, divisor) ==
                            (base + offset + divisor - 1) / divisor,
                        "68000-friendly division matches wide integer arithmetic at boundaries");
    DeathMenuProgress progress;
    DeathMenuSequence menu;
    require(progress.initialize(84, 84, false), "initialize first run");
    progress.advance(menu);
    require(progress.animatedTotal == 0, "hidden progress does not fill");
    menu.resultShown = 6;
    menu.beginStep(false, false, false);
    for (unsigned tick = 1; tick < 200 && !menu.sequenceOver; ++tick) {
        const auto before = progress.animatedTotal;
        const auto frame = progress.advance(menu);
        require(frame.displayedTotal == before, "display captures value before filling");
        if (before < 84) {
            require(progress.animatedTotal == before + (84 - before + 14) / 15,
                    "remaining total uses source ceiling division");
            require(frame.fillSound == (tick % 3 == 0) && !frame.completedSound,
                    "fill sound every third increment, completion on subsequent phase");
        } else require(frame.completedSound, "completion publishes once after reaching total");
    }
    require(menu.sequenceOver && progress.animatedTotal == 84, "first run completes");
    require(!progress.advance(menu).completedSound, "completion sound is one-shot");

    menu = {};
    menu.resultShown = 6;
    menu.beginStep(false, false, false);
    require(progress.initialize(2010, 1020, false), "initialize crossing two goals");
    auto frame = progress.advance(menu);
    require(frame.displayedTotal == 990 && progress.animatedTotal == 1024 && frame.unlockedGoal == -1,
            "increment may overshoot; unlock is not retroactive");
    frame = progress.advance(menu);
    require(frame.unlockedGoal == 0 && progress.animatedTotal == 1000 && menu.goalNotice == 1,
            "next phase clamps to threshold and starts notice");
    for (int i = 0; i < 60; ++i) {
        menu.advanceAlarms();
        require(progress.advance(menu).unlockedGoal == -1 && progress.animatedTotal == 1000,
                "notice holds total and never duplicates unlock request");
    }
    progress.acknowledge(menu.beginStep(true, false, false));
    require(progress.goal == 1 && !menu.goalPassed && menu.inputDisabled,
            "acknowledgement advances goal and preserves sequence input lock");
    unsigned unlocks = 1;
    for (int i = 0; i < 1000 && !menu.sequenceOver; ++i) {
        auto effects = menu.beginStep(menu.goalNotice >= 3, false, false);
        progress.acknowledge(effects);
        menu.advanceAlarms();
        frame = progress.advance(menu);
        if (frame.unlockedGoal >= 0) {
            require(frame.unlockedGoal == 1, "second notice is the second threshold");
            ++unlocks;
        }
    }
    require(unlocks == 2 && menu.sequenceOver && progress.animatedTotal == 2010,
            "multiple unlocks resume filling to actual lifetime total");
    require(progress.separatorTotal == 990, "prior total marker remains unchanged");
    for (unsigned goal = 0; goal < 41; ++goal) {
        menu = {};
        menu.resultShown = 6;
        menu.beginStep(false, false, false);
        const auto threshold = DeathMenuProgress::goals[goal];
        require(progress.initialize(threshold, 1, false), "initialize exact unlock boundary");
        progress.advance(menu);
        require(!menu.goalPassed && !menu.sequenceOver,
                "reaching an exact threshold does not complete early");
        frame = progress.advance(menu);
        require(frame.unlockedGoal == static_cast<int>(goal), "all 41 threshold indices preserved");
        for (int i = 0; i < 60; ++i) menu.advanceAlarms();
        progress.acknowledge(menu.beginStep(true, false, false));
        require(progress.goal == goal + 1 && progress.maxed == (goal == 40),
                "only the final unlock acknowledgement sets maxed");
        require(progress.advance(menu).completedSound && menu.sequenceOver,
                "exact-boundary run completes after acknowledgement");
    }
    require(progress.initialize(500000, 0, true) && progress.goal == 41,
            "already-earned thresholds skipped on entry");
    require(progress.initialize(99999999998ULL, 0, true), "sentinel needs wider than 32 bits");
    require(!progress.initialize(99999999999ULL, 0, true), "invalid total rejected without table overrun");
    require(progress.initialize(99999999999ULL, 100, true) &&
            progress.total == 99999999990ULL && progress.animatedTotal == 99999999899ULL,
            "source caps lifetime total only after computing previous run total");
    require(progress.initialize(10, 20, false) && progress.animatedTotal == 0,
            "negative inferred previous total clamps to zero");
    std::cout << "shared death-menu progress tests passed\n";
}
