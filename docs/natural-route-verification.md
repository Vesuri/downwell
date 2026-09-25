# Natural cavern route verification

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
seed-4 first-cavern route, confirms the default upgrade and runs 120 updates
in cavern two. It does not inject an exit. The seed-4 entry checkpoint (437
RNG draws and side-room indices 8/13) is checked against decoded startup,
including all 16 RNG words; each first-cavern input is checked against the host
policy. This deliberately skips startup presentation, not generator gameplay.

The native debugger gate requires the natural exit at update 1,477, one
re-entry, upgrade/exit-field pixel comparisons with zero failures, and an
intact task-stack guard. It has a 360-second emulator deadline and restores
the ordinary build on success or failure. The native gate currently stops
before shop traversal; the host shop checks above are separate evidence.

Native result: passed at exit update 1,477, with 211 upgrade frames, 135
exit-field frames, one re-entry and 120 second-cavern updates. Both pixel
failure counters are zero and the stack guard is intact. Evidence:
`/tmp/downwell-natural-native.log` and
`/tmp/downwell-natural-native-gdb.log`. These are renderer/adapter checks,
not original Windows screenshots and not a 20 ms performance claim.

The original `scrLevelPat1` VM assembly sets `shopMade=1` and
`rightRoomAmt=2` when area=1, level=1 and playStyle!=4. This explains why the
usual-style shop route starts in cavern two, rather than forcing a shop into
the first level. Source: local UTMT 0.9.2.0 export,
`Assembly/gml_GlobalScript_scrLevelPat1.asm`, branches 6–11.

Evidence: `/tmp/downwell-natural-shop-return.log`. Both routes and natural shop
interaction pass. The seed-99 second-level exploratory policy stalled against
terrain; it was not promoted into a passing shop test. This is bounded route
coverage, not proof of all generated layouts or all original game behavior.
