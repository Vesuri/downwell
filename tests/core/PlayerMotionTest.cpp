#include "core/PlayerMotion.h"
#include "core/StateDigest.h"

#include <cstdlib>
#include <cmath>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

struct SolidFixture {
    bool wall;
};

bool resolveSolid(void* opaque, int x, int y) {
    const SolidFixture& fixture = *static_cast<SolidFixture*>(opaque);
    return fixture.wall && x >= 24 && x <= 39 && y >= -8 && y <= 15;
}

struct EdgeFixture {
    int rightEdge;
};

bool resolveLedge(void* opaque, int x, int y) {
    const EdgeFixture& fixture = *static_cast<EdgeFixture*>(opaque);
    return x < fixture.rightEdge && y >= 8 && y <= 16;
}

} // namespace

int main() {
    PlayerMotion goalStopped(false, 10000);
    downwell::Input goalInput;
    goalInput.right = true;
    unsigned movementPhases = 0;
    goalStopped.configureMovementBoundary([](void* context) {
        ++*static_cast<unsigned*>(context);
    }, &movementPhases);
    for (unsigned tick = 0; tick < 40; ++tick) {
        const auto delta = goalStopped.step(goalInput, false, false, false, true);
        require(delta.deltaX == 0 && delta.deltaY == 0 &&
            goalStopped.offsetXFixed() == 0 && goalStopped.offsetYFixed() == 0,
            "goalStop suppresses both position integrations");
    }
    require(goalStopped.speedXFixed() > 0 && goalStopped.speedYFixed() > 0 &&
        movementPhases == 40,
        "goalStop preserves movement/gravity processing rather than freezing the update");
    const auto resumedGoal = goalStopped.step(goalInput);
    require(resumedGoal.deltaX > 0 && resumedGoal.deltaY > 0,
        "clearing goalStop resumes retained velocity");
    PlayerMotion goalLanding(false, 1);
    for (unsigned tick = 0; tick < 10 && !goalLanding.grounded(); ++tick)
        goalLanding.step({}, true, false, false, true);
    require(goalLanding.grounded() && goalLanding.offsetY() == 1,
        "goalStop retains predicted floor correction and landing");
    PlayerMotion entry(false, 10000);
    entry.initializeEntryInput(false);
    PlayerMotion shortDelay(false, 10000);
    downwell::StateDigest entryDigest, shortDigest;
    entry.appendStateDigest(entryDigest);
    shortDelay.appendStateDigest(shortDigest);
    require(entry.shotDelayed() && shortDelay.shotDelayed() &&
            entry.offsetYFixed() == shortDelay.offsetYFixed() &&
            entryDigest.value() != shortDigest.value(),
            "authoritative digest distinguishes different countdowns behind identical display mirrors");
    downwell::Input heldFire;
    heldFire.fire = true;
    for (unsigned i = 0; i < 29; ++i) entry.step(heldFire);
    require(entry.shots() == 0 && entry.shotDelayed(),
            "scrPlayerInit's thirty-frame gun delay is shared state");
    entry.step(heldFire);
    require(entry.shots() == 1, "the shared entry delay expires after thirty updates");
    PlayerMotion heldEntry(false, 10000);
    heldEntry.initializeEntryInput(true);
    for (unsigned i = 0; i < 35; ++i) heldEntry.step(heldFire);
    require(heldEntry.shots() == 0 && heldEntry.jumpShootLocked(),
            "carrying held fire into a room cannot immediately shoot");
    heldEntry.step(downwell::Input{});
    heldEntry.step(heldFire);
    require(heldEntry.shots() == 1 && !heldEntry.jumpShootLocked(),
            "release unlocks shared entry gun state");
    PlayerMotion locked;
    downwell::Input lockedInput = heldFire;
    lockedInput.right = true;
    locked.step(lockedInput, true);
    require(locked.offsetXFixed() == 0 && locked.jumps() == 0,
            "source noControl suppresses horizontal control and jumping");
    PlayerMotion lockedAir(false, 10000);
    for (unsigned i = 0; i < 40; ++i) lockedAir.step(heldFire, true);
    require(lockedAir.shots() == 0 && lockedAir.ammo() == 8 && lockedAir.speedYFixed() > 3 * 256,
            "noControl also suppresses airborne firing without stopping gravity");
    lockedAir.step(heldFire);
    require(lockedAir.shots() == 1, "held airborne fire resumes when noControl is released");
    PlayerMotion lockedRelease;
    lockedRelease.step(heldFire);
    lockedRelease.step(downwell::Input{}, true);
    require(lockedRelease.jumpShootLocked() && lockedRelease.speedYFixed() < -4 * 256,
            "release while locked neither shortens jump nor unlocks shooting");
    lockedRelease.step(downwell::Input{});
    require(lockedRelease.jumpShootLocked(), "ignored release is not replayed on unlocking controls");
    lockedRelease.step(heldFire);
    lockedRelease.step(downwell::Input{});
    require(!lockedRelease.jumpShootLocked(), "a new unlocked release still clears jump lock");
    PlayerMotion interacting;
    interacting.step(heldFire, false, false, true);
    require(interacting.grounded() && interacting.jumps() == 0,
            "source interactable guard reserves grounded confirmation for interaction");
    interacting.step(heldFire);
    require(interacting.jumps() == 0, "leaving interaction does not replay the consumed press edge");
    interacting.step(downwell::Input{});
    interacting.step(heldFire);
    require(interacting.jumps() == 1, "a new press away from interaction jumps normally");
    PlayerMotion staleInteraction(false, 10000);
    staleInteraction.step(heldFire, false, false, true);
    require(staleInteraction.shots() == 1,
            "interactable gates the press/jump branch, not source airborne held-fire handling");

    PlayerMotion ceilingPlayer(true);
    ceilingPlayer.configureSolidResolver([](void*, int, int y) {
        return y <= -12;
    }, nullptr, -60, 60);
    downwell::Input jumpAtCeiling;
    jumpAtCeiling.fire = true;
    ceilingPlayer.step(jumpAtCeiling);
    ceilingPlayer.step(jumpAtCeiling);
    require(ceilingPlayer.offsetYFixed() == -7 * 256 &&
            !ceilingPlayer.grounded(),
            "shared upward contact stops below the ceiling without becoming grounded");
    ceilingPlayer.step(jumpAtCeiling);
    require(ceilingPlayer.offsetYFixed() > -7 * 256,
            "gravity resumes downward motion on the frame after ceiling contact");

    PlayerMotion gravityPrecision(false, 10000);
    // Source scrGravity_n: exactly .2 per update, capped at 7.
    long fifthsPosition = 0;
    long fifthsSpeed = 0;
    for (unsigned tick = 0; tick < 100; ++tick) {
        const long frameStartY = gravityPrecision.offsetYFixed();
        fifthsSpeed += 256;
        if (fifthsSpeed > 7 * 256 * 5) fifthsSpeed = 7 * 256 * 5;
        fifthsPosition += fifthsSpeed;
        gravityPrecision.step(downwell::Input{});
        require(gravityPrecision.frameStartYFixed() == frameStartY,
                "enemy targeting retains scrPlMovement's pre-integration player position");
        require(gravityPrecision.offsetYFixed() == fifthsPosition / 5,
                "shared player preserves exact .2 gravity and position integration through terminal speed");
    }
    PlayerMotion emptyGun(false, 10000);
    downwell::Input trigger;
    trigger.fire = true;
    for (unsigned shot = 0; shot < 8; ++shot) {
        emptyGun.step(downwell::Input{});
        emptyGun.step(trigger);
    }
    require(emptyGun.ammo() == 0 && emptyGun.shots() == 8,
            "empty-charge fixture consumes all eight rounds in midair");
    for (unsigned frame = 0; frame < 40; ++frame) emptyGun.step(downwell::Input{});
    PlayerMotion lockedEmpty = emptyGun;
    lockedEmpty.step(trigger, true);
    require(lockedEmpty.speedYFixed() > 3 * 256 && !lockedEmpty.shotDelayed(),
            "noControl suppresses empty-charge braking and its delay too");
    long previousY = emptyGun.offsetYFixed();
    emptyGun.step(trigger);
    require(emptyGun.offsetYFixed() - previousY == 3 * 256 &&
            emptyGun.ammo() == 0 && emptyGun.shots() == 8,
            "original empty gun brakes the fall to three without producing a bullet");
    for (unsigned frame = 1; frame < 16; ++frame) {
        previousY = emptyGun.offsetYFixed();
        emptyGun.step(trigger);
        require(emptyGun.offsetYFixed() - previousY > 3 * 256,
                "held empty gun respects the sixteen-frame delay");
    }
    previousY = emptyGun.offsetYFixed();
    emptyGun.step(trigger);
    require(emptyGun.offsetYFixed() - previousY == 3 * 256,
            "held empty gun brakes again after the recovered delay");

    // Source-backed default-style rules, independent of SDL comparison.
    // scrPlayerInit: maxsp=2, airMaxsp=2.5, decclsp=.4, airdeccl=.1.
    // scrPlMovement: reversal boost=2 grounded / 1 airborne; both keys stop.
    PlayerMotion groundControls;
    downwell::Input right;
    right.right = true;
    for (unsigned frame = 0; frame < 12; ++frame) groundControls.step(right);
    require(groundControls.speedXFixed() == 512,
            "original grounded horizontal speed caps at two pixels");
    groundControls.step(downwell::Input{});
    require(groundControls.speedXFixed() == 410,
            "ground friction uses .4 rather than airborne .1 (Q8 approximation)");
    downwell::Input left;
    left.left = true;
    groundControls.step(left);
    require(groundControls.speedXFixed() == -153,
            "ground reversal subtracts two pixels after acceleration");
    downwell::Input both = right;
    both.left = true;
    const int beforeBoth = groundControls.offsetX();
    groundControls.step(both);
    require(groundControls.speedXFixed() == 0 &&
            groundControls.offsetX() == beforeBoth,
            "simultaneous directions stop immediately rather than applying friction");

    PlayerMotion edgeFall;
    edgeFall.configureLandingResolver([](void*, int x, int) { return x < 1 ? 0 : 100; }, nullptr);
    downwell::Input walkOff;
    walkOff.right = true;
    for (unsigned frame = 0; frame < 10 && edgeFall.grounded(); ++frame)
        edgeFall.step(walkOff);
    require(!edgeFall.grounded() && edgeFall.offsetYFixed() > 0,
            "walking off support applies gravity on the first airborne frame");

    PlayerMotion player;
    PlayerMotion spin;
    downwell::Input spinInput;
    spinInput.right = true;
    spinInput.fire = true;
    spin.step(spinInput);
    require(spin.spinJumping() && spin.spriteId() == 1 && spin.spriteFrame() == 0,
            "directional jump selects recovered spin sprite and resets image index");
    spinInput.right = false;
    spinInput.left = true;
    for (unsigned i = 0; i < 10; ++i) spin.step(spinInput);
    require(spin.spriteFrame() == 3 && spin.facing() == 1 && spin.speedXFixed() < 0,
            "spin advances at 0.3 while reverse input steers without flipping facing");
    spinInput.fire = false;
    spin.step(spinInput);
    require(spin.spinJumping(), "release unlocks shooting but does not itself cancel spin");
    spinInput.fire = true;
    spin.step(spinInput);
    require(!spin.spinJumping() && spin.spriteId() == 3,
            "unlocked fire cancels spin and selects shooting presentation");
    spin.stompBounce();
    require(spin.spinJumping() && spin.spriteId() == 1,
            "stomp forced jump takes precedence over prior airborne shooting");
    for (unsigned i = 0; i < 100 && !spin.grounded(); ++i)
        spin.step(downwell::Input{});
    require(spin.grounded() && !spin.spinJumping(), "landing clears the shared spin state");
    PlayerMotion loopingSpin(false, 10000);
    loopingSpin.stompBounce();
    const unsigned inheritedFrame = loopingSpin.spriteFrame();
    unsigned seenSpinFrames = 0;
    for (unsigned i = 0; i < 80; ++i) {
        loopingSpin.step(downwell::Input{});
        seenSpinFrames |= 1u << loopingSpin.spriteFrame();
    }
    require(seenSpinFrames == 255 && loopingSpin.spriteFrame() == inheritedFrame,
            "spin covers all eight frames and loops three times in eighty updates");
    downwell::Input input;
    require(player.grounded() && player.ammo() == 8,
            "player starts grounded with a full gunboot charge");
    require(player.spriteId() == 31 && player.spriteFrame() == 0,
            "grounded player selects the recovered idle frame");

    downwell::Input neutralInput;
    for (unsigned frame = 0; frame < 10; ++frame)
        player.step(neutralInput);
    require(player.spriteFrame() == 1,
            "the four-frame idle sequence advances at recovered 0.1 speed");
    for (unsigned frame = 0; frame < 30; ++frame)
        player.step(neutralInput);
    require(player.spriteFrame() == 0,
            "the recovered idle sequence loops after forty steps");

    input.fire = true;
    player.step(input);
    require(!player.grounded() && player.jumps() == 1 && player.shots() == 0,
            "grounded fire jumps without emitting a bullet");
    require(player.spriteId() == 37 && player.spriteFrame() == 0,
            "fast upward motion selects the first airborne frame");

    player.step(input);
    require(player.shots() == 0,
            "holding the jump button does not emit an airborne bullet");

    input.fire = false;
    player.step(input);
    require(player.spriteFrame() == 1,
            "variable-height release selects the slow upward frame");
    input.fire = true;
    player.step(input);
    require(player.shots() == 1 && player.ammo() == 7,
            "a later airborne press consumes one charge and shoots");
    require(player.spriteId() == 3,
            "airborne fire selects the recovered shooting pose");
    for (unsigned frame = 0; frame < 6; ++frame) {
        player.step(input);
        if (frame == 3)
            require(player.spriteFrame() == 1,
                    "shooting advances at the recovered 0.25 image speed");
    }
    require(player.shots() == 1,
            "held gunboot fire respects the recovered seven-tick delay");
    player.step(input);
    require(player.shots() == 2 && player.ammo() == 6,
            "held gunboot fire repeats when the delay expires");

    input.fire = false;
    for (unsigned frame = 0; frame < 13; ++frame)
        player.step(input);
    require(player.spriteFrame() == 3,
            "the shooting sequence stops on its recovered final frame");
    for (unsigned frame = 0; frame < 240 && !player.grounded(); ++frame)
        player.step(input);
    require(player.grounded() && player.ammo() == 8,
            "landing restores the full gunboot charge");
    require(player.recharges() == 1, "depleted landing emits one successful recharge");
    player.stompBounce();
    require(player.recharges() == 1, "full-ammo bounce emits no recharge effect");
    for (unsigned frame = 0; frame < 240 && !player.grounded(); ++frame)
        player.step(input);

    input.right = true;
    const PlayerMotionDelta firstMove = player.step(input);
    require(player.spriteId() == 27 && player.spriteFrame() == 0,
            "ground movement selects and resets the recovered run cycle");
    player.step(input);
    player.step(input);
    player.step(input);
    require(player.spriteFrame() == 0,
            "the recovered 0.3 image speed holds run frame zero");
    player.step(input);
    require(player.spriteFrame() == 1,
            "the recovered 0.3 image speed advances after four intervals");
    PlayerMotionDelta laterMove = firstMove;
    for (unsigned frame = 0; frame < 8; ++frame)
        laterMove = player.step(input);
    require(player.offsetX() > 0 && laterMove.deltaX >= firstMove.deltaX,
            "horizontal input accelerates the fixed-point player");
    input.right = false;
    input.left = true;
    player.step(input);
    require(player.facing() == -1,
            "leftward input flips the shared player presentation");

    input.left = false;
    for (unsigned frame = 0; frame < 40 && player.speedXFixed() != 0; ++frame)
        player.step(input);
    require(player.spriteId() == 31 && player.spriteFrame() == 0,
            "friction returns the stationary player to the idle pose");

    EdgeFixture edge = {5};
    PlayerMotion balancing;
    balancing.configureSolidResolver(resolveLedge, &edge, -236, 236);
    input = downwell::Input{};
    balancing.step(input);
    require(balancing.spriteId() == 15 && balancing.spriteFrame() == 0,
            "stationary player facing beyond support uses the ledge pose");
    for (unsigned frame = 0; frame < 4; ++frame)
        balancing.step(input);
    require(balancing.spriteFrame() == 1,
            "the five-frame ledge pose advances at recovered 0.25 speed");
    edge.rightEdge = 6;
    balancing.step(input);
    require(balancing.spriteId() == 31 && balancing.spriteFrame() == 0,
            "support beneath the forward probe restores the idle pose");

    SolidFixture fixture = {true};
    PlayerMotion colliding;
    colliding.configureSolidResolver(resolveSolid, &fixture, -236, 236);
    input = downwell::Input{};
    input.right = true;
    for (unsigned frame = 0; frame < 60; ++frame)
        colliding.step(input);
    require(colliding.offsetX() == 20,
            "recovered player mask stops immediately before a solid wall");
    fixture.wall = false;
    for (unsigned frame = 0; frame < 60; ++frame)
        colliding.step(input);
    require(colliding.offsetX() > 60 && colliding.offsetX() <= 236,
            "decoded terrain replaces the old center-shaft movement clamp");

    PlayerMotion falling(false, 58);
    downwell::Input neutral;
    for (unsigned frame = 0; frame < 120 && !falling.grounded(); ++frame)
        falling.step(neutral);
    require(falling.grounded() && falling.offsetY() == 58,
            "gameplay spawn falls to the configured first platform");

    PlayerMotion overlapping;
    overlapping.contactKnockback(0, 0);
    require(overlapping.speedXFixed() == 0 && !overlapping.grounded(),
            "exact enemy overlap has no invented horizontal knockback");
    PlayerMotion diagonal;
    diagonal.contactKnockback(6, 8);
    require(diagonal.speedXFixed() == -460,
            "diagonal contact normalizes recovered horizontal knockback");

    PlayerMotion damaged(false, 10000);
    downwell::Input movingFire;
    movingFire.right = true;
    movingFire.fire = true;
    damaged.step(movingFire);
    require(damaged.ammo() < 8 && damaged.speedXFixed() > 0,
            "damage fixture is moving with depleted ammunition");
    const long previousSpeed = damaged.speedXFixed();
    damaged.contactKnockback(0, 8);
    require(damaged.speedXFixed() == previousSpeed,
            "vertical impulse uses pre-movement x and preserves existing xsp");
    require(damaged.ammo() == 8 && damaged.speedYFixed() == -512 && !damaged.grounded(),
            "scrFjump recharges damage knockback and applies vertical launch");
    require(damaged.recharges() == 1, "damage recharge emits the shared presentation event");
    damaged.increaseAmmoCapacity(1);
    require(damaged.recharges() == 1 && damaged.ammo() == 9,
            "capacity grant is not a scrRecharge event");
    PlayerMotion stompRefill(false, 10000);
    stompRefill.step(movingFire);
    require(stompRefill.ammo() == 7, "stomp fixture starts depleted");
    stompRefill.stompBounce();
    require(stompRefill.ammo() == 8 && stompRefill.recharges() == 1,
            "depleted stomp emits one successful recharge");

    PlayerMotion shallow;
    shallow.contactKnockback(1, 1);
    require(shallow.speedXFixed() >= -544 && shallow.speedXFixed() <= -543,
            "subpixel distance avoids integer-pixel square-root overestimation");
    for (int dx = -16; dx <= 16; ++dx) {
        for (int dy = -16; dy <= 16; ++dy) {
            if (dx == 0 && dy == 0) continue;
            PlayerMotion sample;
            sample.contactKnockback(-dx, -dy);
            const double expected = 768.0 * dx / std::sqrt(double(dx * dx + dy * dy));
            require(std::fabs(sample.speedXFixed() - expected) < 2.0,
                    "contact-range normalization stays within two Q8 units of double math");
        }
    }

    int landingProfile[PlayerMotion::landingProfileSize];
    for (unsigned index = 0; index < PlayerMotion::landingProfileSize; ++index)
        landingProfile[index] = index < 60 ? 42 : 74;
    PlayerMotion profiled(false, 200);
    profiled.configureLandingProfile(
        landingProfile, PlayerMotion::landingProfileSize);
    input = downwell::Input{};
    input.left = true;
    for (unsigned frame = 0; frame < 120 && !profiled.grounded(); ++frame)
        profiled.step(input);
    require(profiled.grounded() && profiled.offsetY() == 42,
            "horizontal position selects the recovered landing profile");

    for (unsigned index = 0; index < PlayerMotion::landingProfileSize; ++index)
        landingProfile[index] = index < 60 ? 42 : 74;
    profiled.configureLandingProfile(
        landingProfile, PlayerMotion::landingProfileSize);
    input = downwell::Input{};
    input.right = true;
    for (unsigned frame = 0; frame < 120 && profiled.offsetY() != 74;
         ++frame)
        profiled.step(input);
    require(profiled.grounded() && profiled.offsetY() == 74,
            "walking beyond a platform edge starts a fall to lower terrain");

    PlayerMotion corpse;
    require(corpse.spriteId(true) == 0 && corpse.spriteFrame(true) == 0 &&
                corpse.spriteFrame(true, 0) == 1 && corpse.spriteFrame(true, -1) == 0,
            "death pose uses sprite zero and source deathani guards for grounded resting frame");
    corpse.stompBounce();
    require(corpse.spriteId(true) == 0 && corpse.spriteFrame(true, 0) == 0 &&
                corpse.spriteId() == 1,
            "death pose overrides airborne spin without mutating living motion state");
    PlayerMotion runningCorpse;
    input = downwell::Input{};
    input.right = true;
    for (unsigned i = 0; i < 10; ++i) runningCorpse.step(input);
    require(runningCorpse.spriteFrame(true, 0) == 0,
            "grounded corpse still moving faster than half a pixel does not use resting frame");
    const long preDeathSpeed = runningCorpse.speedXFixed();
    input.fire = true;
    runningCorpse.step(input, false, true);
    require(runningCorpse.speedXFixed() == preDeathSpeed - 20 &&
                runningCorpse.shots() == 0 && runningCorpse.jumps() == 0,
            "death applies rounded 0.08 drag instead of living friction and rejects held controls");
    for (unsigned i = 0; i < 30; ++i) runningCorpse.step(input, false, true);
    require(runningCorpse.speedXFixed() == 0,
            "death sprite-control threshold stops residual horizontal drift");
    PlayerMotion risingCorpse(false, 1000);
    input = downwell::Input{};
    input.fire = true;
    risingCorpse.step(input);
    risingCorpse.contactKnockback(0, 1);
    input.fire = false;
    risingCorpse.step(input, false, true);
    require(risingCorpse.speedYFixed() == -230,
            "death halves upward speed after gravity once, not again for synthetic fire release");
    PlayerMotion fallingCorpse(false, 1000);
    fallingCorpse.step(input, false, true);
    require(fallingCorpse.speedYFixed() == 51,
            "death keeps downward gravity rather than halving positive velocity");
    std::cout << "Shared player tests passed\n";
    return 0;
}
