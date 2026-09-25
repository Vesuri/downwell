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
holding fire and selecting Leave compare menu/control ownership and all three
stock entries, without replacing items or granting test funds. The shop test
does not yet purchase anything or walk back into the main shaft.

The original `scrLevelPat1` VM assembly sets `shopMade=1` and
`rightRoomAmt=2` when area=1, level=1 and playStyle!=4. This explains why the
usual-style shop route starts in cavern two, rather than forcing a shop into
the first level. Source: local UTMT 0.9.2.0 export,
`Assembly/gml_GlobalScript_scrLevelPat1.asm`, branches 6–11.

Evidence: `/tmp/downwell-natural-shop-test.log`. Both routes and natural shop
interaction pass. The seed-99 second-level exploratory policy stalled against
terrain; it was not promoted into a passing shop test. This is bounded route
coverage, not proof of all generated layouts or all original game behavior.
