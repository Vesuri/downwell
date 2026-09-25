# Natural cavern route verification

## Eight-seed gameplay sweep

`make verify-route-sweep` runs seeds 0–7, requiring every route to reach its
naturally generated first-cavern exit alive within 2,500 updates. It retains
the per-update actor, player, camera and animation comparisons and additionally
checks currency, death/exit state, RNG draw count/index and all 16 RNG words
on every update. This is a host adapter test, not native rendering or Windows
execution evidence.

The sweep's input controller adds deterministic recovery when no new downward
progress exceeds eight pixels for 120 updates: alternate horizontal targets
every 120 updates and press/release jump/fire every 12 updates. This is input
only; no health, position, terrain, shop stock, currency or RNG injection is
used. The original two-seed controller and native seed-4 gate are unchanged.
The first exploration without recovery stalled on seven seeds; those budget
outcomes were not evidence of successful full-level coverage. The final gate
fails on death or a budget outcome rather than silently accepting a stall.

| Seed | Updates to exit | Health | Gems | Generated enemies |
| --- | ---: | ---: | ---: | ---: |
| 0 | 2401 | 2 | 26 | 22 |
| 1 | 2086 | 2 | 38 | 18 |
| 2 | 1763 | 3 | 40 | 10 |
| 3 | 2104 | 3 | 0 | 12 |
| 4 | 1477 | 4 | 10 | 13 |
| 5 | 2076 | 2 | 60 | 14 |
| 6 | 1932 | 2 | 60 | 17 |
| 7 | 2107 | 1 | 40 | 14 |

Total: 15,946 compared gameplay updates and 120 generated enemies. No adapter
or RNG mismatch was found. This expands seed/encounter coverage, not the set
of implemented game features. The separate two-seed gate below remains
responsible for upgrade/re-entry and generated-shop traversal.
The strict eight-seed rerun and unchanged two-seed gate both pass. Logs:
`/tmp/downwell-route-sweep-verified.log` and
`/tmp/downwell-route-sweep-original-gates.log`. No production code, assets or
emulator build options were changed for this test-only addition.

## Exit, upgrade and shop route

Run `make verify-natural-exit`. This host test runs both runtime adapters using
the decoded room, object and builder packs. It does not run an emulator or
establish original-runner visual fidelity.

The deterministic controller inspects terrain landing queries every 16 updates
and chooses ordinary left/right/fire input. It never changes player position,
camera, health, currency, generated geometry or exit placement. Both adapters
receive the same input, with supported gameplay state compared every update.

Seeds 99 and 4 reach naturally generated first-level exits after 1,883 and 1,477
updates. Both then spend 211 updates in upgrade selection and enter cavern two.
Upgrade effects, offers, inventory and full RNG state are compared. The first
120 updates of the next level are also compared.

Seed 4 continues another 662 updates to its generated shop at (368, 2208).
The controller approaches the counter using its generated position. Opening,
holding fire, attempting purchase and selecting Leave compare menu/control
ownership and all three stock entries, without replacing items or granting
test funds. This route has 22 gems versus a 300-gem price: the attempted purchase
is rejected with unchanged stock/currency. Held opening input is independently
checked not to spend or buy. Returning left takes 43 updates and moves the
camera from offset 160 to 0; gameplay/camera parity is checked throughout.
Successful naturally funded purchasing remains unverified.

## Native gate

`make verify-amiga-natural-route` first runs the paired host test, then builds
`SKIP_TO_GAME=1 NATURAL_VERIFY=1`. The test-only native controller follows the
seed-4 first-cavern route, confirms the default upgrade and runs 120 entrance
updates in cavern two, then approaches the shop, opens it, attempts the
unfunded purchase, selects Leave and walks back. It does not inject an exit.
The seed-4 entry checkpoint (437
RNG draws and side-room indices 8/13) is checked against decoded startup,
including all 16 RNG words; each first-cavern input is checked against the host
policy. Shop and return inputs are also checked individually against the host
route. This deliberately skips startup presentation, not generator gameplay.

The native debugger gate requires the natural exit at update 1,477, one
re-entry, upgrade/exit-field pixel comparisons with zero failures, and an
intact task-stack guard. It additionally requires 832 second-cavern updates,
43 return updates, no shop-state failures and retained shop HUD frames matching
fresh scalar composition. It has a 360-second emulator deadline and restores
the ordinary build on success or failure.

Previous native result (before extending into the shop): passed at exit update 1,477, with 211 upgrade frames, 135
exit-field frames, one re-entry and 120 second-cavern updates. Both pixel
failure counters are zero and the stack guard is intact. Evidence:
`/tmp/downwell-natural-native.log` and
`/tmp/downwell-natural-native-gdb.log`. These are renderer/adapter checks,
not original Windows screenshots and not a 20 ms performance claim.

The extended native gate also passes: 832 updates in cavern two, 49 retained
shop-HUD comparisons with zero pixel failures, and a 43-update return to the
main shaft. Natural shop logic checks and the task-stack guard pass. Evidence:
`/tmp/downwell-natural-shop-native.log` and
`/tmp/downwell-natural-shop-native-gdb.log`. Normal startup is restored and
both 68000 audits pass. This verifies the generated shop route, not a funded
purchase, all shop layouts, or fresh composition of every terrain pixel.

The original `scrLevelPat1` VM assembly sets `shopMade=1` and
`rightRoomAmt=2` when area=1, level=1 and playStyle!=4. This explains why the
usual-style shop route starts in cavern two, rather than forcing a shop into
the first level. Source: local UTMT 0.9.2.0 export,
`Assembly/gml_GlobalScript_scrLevelPat1.asm`, branches 6–11.

Evidence: `/tmp/downwell-natural-shop-return.log`. Both routes and natural shop
interaction pass. The seed-99 second-level exploratory policy stalled against
terrain; it was not promoted into a passing shop test. This is bounded route
coverage, not proof of all generated layouts or all original game behavior.
