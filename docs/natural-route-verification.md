# Natural cavern route verification

## After the cavern level-dependent spawning correction

The shared builder now applies the original level-two/three spawn probability
and crawler-cell choice. First-level exits remain 1,339/1,646 updates for seeds
99/4, with 211 upgrade and 120 second-level updates compared for each. The
unchanged seed-4 input controller now takes 518 additional updates to reach
the naturally generated shop, rejects an item priced at 150 with 18 gems,
and takes 43 return updates (the host diagnostic's zero-based last index is
42). The total second-level route is 688 updates. All paired state checks pass.

The bounded native gate passes those timings, 211 clear-frame comparisons,
168 exit-field comparisons, 49 retained HUD comparisons and the stack guard,
with zero failure counters. The emulator was stopped and the ordinary build
restored by the gate. Evidence: `/tmp/downwell-cavern-spawn-native.log` and
`/tmp/downwell-cavern-spawn-native-gdb.log`. SDL and native builds and the
68000 audits pass; shared generation also passes ASan/UBSan. These are
correctness checks, not fresh frame-time measurements or Windows captures.
The earlier timings below are historical and superseded for cavern two.

## After the Windows choose correction

The corrected two-seed host gate passes: seed 99 exits at 1,339 updates and
seed 4 at 1,646; each
compares 211 upgrade and 120 second-level updates. Seed 4 then reaches its
generated shop after 259 more updates, rejects a 300-gem item with 16 gems,
and returns to the shaft in 45 updates. Second-level total is 431 updates.
Decoded entry checkpoints are seed 2: 330 words/rows 8,3 and seed 4:
324 words/rows 8,4, checked against every RNG state word. See
`runner-choose.md` for the audit and scope. The eight-seed sweep below has also
been revalidated against the corrected stream.

The corrected native seed-4 gate also passes: 211 upgrade comparisons,
168 exit-field comparisons, 51 retained shop-HUD comparisons, 45 return updates,
zero failure counters and intact stack guard. Its emulator run was time-limited
and ordinary build options are restored. Current evidence:
`/tmp/downwell-corrected-native-route.log`,
`/tmp/downwell-corrected-native-gdb.log`.

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
| 0 | 1935 | 2 | 36 | 17 |
| 1 | 2132 | 3 | 12 | 10 |
| 2 | 1995 | 3 | 58 | 8 |
| 3 | 1807 | 4 | 20 | 12 |
| 4 | 1646 | 3 | 16 | 9 |
| 5 | 2175 | 3 | 4 | 13 |
| 6 | 1935 | 3 | 24 | 12 |
| 7 | 1493 | 4 | 18 | 14 |

Total: 15,118 compared gameplay updates and 95 generated enemies. No adapter
or RNG mismatch was found. This expands seed/encounter coverage, not the set
of implemented game features. The separate two-seed gate below remains
responsible for upgrade/re-entry and generated-shop traversal.
The corrected strict eight-seed rerun passes. Log:
`/tmp/downwell-corrected-sweep.log`. No production code, assets or emulator
build options were changed while revalidating this sweep. The former
15,946-update / 120-enemy sweep predates the RNG correction and is superseded.

## Historical exit, upgrade and shop route (before choose correction)

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
