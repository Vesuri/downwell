# Activation lifecycle correction

Current checkpoint: the historical extended-combat failures recorded below
are superseded by the passing two-cadence, 6,000-update checks in
[the fidelity audit](original-behavior-audit.md#current-extended-combat-checkpoint).
Exact GameMaker region/mask boundary behavior remains an open fidelity item.

Source evidence is `gml_Object_objPlayer_n_Step_1.asm`: its first block tests
view bottom > activatey - 112, adds exactly 16 to activatey, then activates the
region before gravity and movement. `scrPlayerInit` initializes activatey=0.
`gml_GlobalScript_scrOutofview.asm` calls instance_destroy, not deactivation.

Amiga now maintains this advancing frontier instead of a constant camera-distance
activation test. SDL performs its frontier update at the start of player Begin
Step rather than after all gameplay updates. Deferred inactive instances carry
an explicit activationPending flag; activation clears it. Destroyed/retired
instances cannot be reactivated merely because their active flag is false.
The pending state is included in the SDL deterministic digest.
Its pinned startup digest is now `5ebea3e340215cdc`; this is an explicit
checksum-layout change, not a new Windows behavior reference.

Tests cover ten 16-pixel frontier advances leaving a world-y 176 centre dormant,
the eleventh activating it before enemy motion, deferred insertion preserving
dormancy, and retired live-tape enemies no longer awaiting activation.
The existing centre-based region inclusion is still an approximation: exact
GameMaker region/mask intersection boundaries remain to be established. These
tests prove frontier progression and scheduling, not those boundary semantics.

The extended combat gate still fails at the tick-600 snake hit and tick-666
player movement. The bat at world y2368 still activates earlier in SDL; camera
fraction/frame alignment and region boundaries remain under investigation.
Do not claim full activation fidelity from matching the frontier algorithm.

Validation: full host suite passes; the bounded A500 canonical replay passes
with `bfbbd762b9b62a59`, and 68000 helper/instruction audits are clean. SDL was
rebuilt. The short target replay is a platform regression check, not coverage
of the disputed bat boundary in the extended host tape.
