# Natural cavern route verification

## Full startup scene gate

`make verify-amiga-scenes` builds `SCENE_DESCENT_VERIFY=1`, starts at the
publisher sequence and drives garden, well entry and style selection. After
the first gameplay landing, `SceneDescentVerification.h` seeks room X=240
with a two-pixel dead band and the existing two-tap firing cadence. This is
test input, not game behavior. It does not edit terrain, move the player,
grant health or alter RNG. The older alternating movement tape remains in
other scene/capture fixtures so their checkpoints do not silently change.

The gate still requires 256 pixels of camera descent beyond the first landing
and an awake enemy, as well as splash intervals, style composition, Copper
patch consistency and the stack guard. It compares all three initial frame
hashes. The emulator deadline defaults to 180 seconds and the build is restored
on either success or failure. Progress markers identify a stalled stage;
reaching a stage does not substitute for the final assertions.

For the same post-landing inputs on SDL, build `build/capture-sdl-landing` and
run it with `DOWNWELL_CAPTURE_DESCENT=1`, an output directory and a simulation
tick (for example 600). The capture reports position, health, jumps and shots;
it is a diagnostic capture, not an independent original-game oracle.

## Paired host gate

`make verify-natural-exit` runs both runtime adapters against decoded room,
object and builder packs. Seeds 99 and 4 must reach their generated first-level
exits alive, complete upgrade selection and enter cavern two. The seed-4 route
also approaches its generated shop, opens the menu, holds the opening input,
attempts an unfunded purchase, selects Leave and returns to the main shaft.

The controller uses ordinary left/right/fire inputs. It queries terrain every
16 updates and recovers from ledges when downward progress has not exceeded
eight pixels for 120 updates: alternate horizontal targets every 120 updates
and pulse fire every 12 updates. In cavern two, a nearby shop takes priority
over the descent target. Recovery resets at the start of each approach.
No position, health, currency, terrain, stock, exit or RNG is injected.

Compare supported player, actor, camera and field state each update. First-level
updates compare the complete RNG state; upgrade selection compares offers,
effects, inventory and RNG. Shop tests preserve actual stock/prices and verify
input filtering, insufficient funds and camera/control release.

`DOWNWELL_TRACE_SHOP_ROUTE=1` adds periodic approach diagnostics. A stalled
controller is a failed route, not a successful parity result. After source-backed
RNG corrections, inspect changed terrain and adjust only the test input policy;
do not restore an incorrect random stream to preserve old checkpoints.

## Native gate

`make verify-amiga-natural-route` first runs the paired host gate, then builds
`SKIP_TO_GAME=1 NATURAL_VERIFY=1`. Its seed-4 entry checkpoint is checked against
decoded startup, including every RNG word. Host checks independently reproduce
the native controller's inputs through exit, reentry, shop and return.

The debugger checks timing checkpoints, upgrade and exit-field pixels, retained
shop HUD composition, logic failure counters and the task-stack guard. Timing
assertions live in `amiga/verify_natural.gdb` and the paired test; update them
only after verifying the route. The emulator has a 360-second deadline and the
wrapper restores an ordinary build on success or failure.

## Seed sweep and evidence limits

`make verify-route-sweep` checks seeds 0–7, requiring each generated first-level
exit alive within 2,500 updates. It compares gameplay state and every RNG word;
death or budget exhaustion fails the gate. It does not test shop traversal.

These gates establish bounded adapter and native-renderer agreement, not original
Windows presentation fidelity, all generated layouts, naturally funded purchases
or the 20 ms frame budget. Remaining coverage belongs in [open work](open-work.md).
Record measured results and implementation history in commits, not this runbook.

The original `scrLevelPat1` VM assembly sets `shopMade=1` and `rightRoomAmt=2`
when area=1, level=1 and playStyle!=4. Therefore the usual-style shop route starts
in cavern two rather than forcing a shop into cavern one. Source: local UTMT
0.9.2.0 export, `Assembly/gml_GlobalScript_scrLevelPat1.asm`, branches 6–11.
