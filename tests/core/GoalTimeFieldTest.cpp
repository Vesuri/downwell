#include "core/GoalTimeField.h"
#include "core/RunSession.h"
#include "core/StateDigest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
static bool exact(downwell::HudReal actual, double expected) {
    std::uint64_t bits;
    std::memcpy(&bits, &expected, sizeof bits);
    return actual.exponent == int((bits >> 52) & 2047) - 1023 &&
        actual.mantissa == ((bits & 0xfffffffffffffULL) | (1ULL << 52));
}
int main() {
    downwell::RunSession session;
    downwell::GameMakerRandom sessionRandom(42);
    session.gameplayPresentationStep(sessionRandom);
    require(sessionRandom.drawCount() == 0, "absent field consumes no Draw RNG");
    session.levelExitPresent = true;
    session.goalStopped = true;
    session.gameplayPresentationStep(sessionRandom);
    require(sessionRandom.drawCount() == 1, "goal stop is not the source pTimeStop guard");
    const auto stoppedRadius = session.goalField.radius;
    session.gameplayPresentationStep(sessionRandom, true);
    require(sessionRandom.drawCount() == 1 && session.goalField.radius.equal(stoppedRadius),
            "explicit time stop retains the Draw radius");
    session.levelExitPresent = false;
    session.gameplayPresentationStep(sessionRandom);
    require(sessionRandom.drawCount() == 1, "retired field consumes no Draw RNG");
    session.results.open(true,100,100);
    session.results.menu.visible[5]=session.results.menu.visible[6]=true;
    const auto resultsDigest=[&] {
        downwell::StateDigest digest;session.results.appendState(digest);return digest.value();
    };
    session.gameplayPresentationStep(sessionRandom);
    require(session.results.progress.animatedTotal>0,"healthy presentation advances results progress");
    const auto beforeFailure=resultsDigest();
    const auto rngBefore=sessionRandom.drawCount();
    session.levelExitPresent=true;
    session.goalFieldBirthBoundary=1; // Invalid: no ordinary fields were born.
    session.gameplayPresentationStep(sessionRandom);
    require(!session.effectsSucceeded && resultsDigest()==beforeFailure &&
            sessionRandom.drawCount()==rngBefore,
            "failed field Draw stops results progression and later RNG consumption");
    session.goalFieldBirthBoundary=0; // Repairing input must not clear a terminal failure.
    session.gameplayPresentationStep(sessionRandom);
    require(!session.effectsSucceeded && resultsDigest()==beforeFailure &&
            sessionRandom.drawCount()==rngBefore,
            "latched failure blocks subsequent presentation even with valid fields");
    downwell::GameMakerRandom random(42), oracle(42);
    downwell::GoalTimeField field;
    field.create(240, 4032, random);
    double radius = 128 + oracle.random(2);
    require(exact(field.radius, radius), "Create radius matches runner binary64");
    for (unsigned tick = 0; tick < 10000; ++tick) {
        const bool stopped = tick % 3 == 0;
        const int cameras[] = {160, 161, 319, 320};
        const int camera = cameras[tick % 4];
        if (!stopped) radius = 128 + oracle.random(2);
        unsigned circles = 0;
        field.advanceDraw(stopped, random);
        field.drawSnapshot(camera, 480,
            [&](int x, int y, downwell::HudReal r, bool red) {
                require(x == 240 && y == 4032, "circle uses world coordinates");
                require(red == (circles == 2), "two white outlines precede red outline");
                require(exact(r, radius + (circles == 1 ? -1 : circles == 2 ? 1 : 0)),
                    "outline radius matches binary64 exactly");
                ++circles;
            });
        require(circles == (camera > 160 && camera < 320 ? 3U : 0U),
            "strict camera boundary controls presentation only");
        require(exact(field.radius, radius) && random.drawCount() == oracle.drawCount(),
            "time stop retains radius; hidden Draw still consumes RNG");
        require(random.stateIndex() == oracle.stateIndex(), "RNG index matches");
        for (unsigned i = 0; i < 16; ++i)
            require(random.stateWord(i) == oracle.stateWord(i), "RNG state matches");
        const auto snapshot = field.radius;
        for (unsigned repaint = 0; repaint < 3; ++repaint)
            field.drawSnapshot(camera, 480, [](int, int, downwell::HudReal, bool) {});
        require(field.radius.equal(snapshot) && random.drawCount() == oracle.drawCount(),
                "repainting a snapshot does not change radius or consume RNG");
    }
    std::puts("Goal time-field radius, outline order and Draw RNG tests passed");
}
