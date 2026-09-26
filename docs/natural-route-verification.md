# Natural cavern route verification

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
