# Scene and rendering contracts

Game rules belong in `src/core`; scene classes own platform presentation.
The sole backlog is [remaining work](open-work.md). Implementation history and
past test results belong in the git log.

## Encounter session boundary

`EncounterBoundary` owns a frontend session's startup/update/Draw/failure state.
Room changes and Retry run inside its update; they must not reset a terminal
failure. Event callbacks may nest inside one operation, but invoking another
whole update or Draw recursively is a binding failure. A callback cannot hide
a failed nested operation by returning success. Retirement prevents further
work. There is no per-frame catalogue scan in this boundary.

SDL constructs `StoredEncounterSession` after attaching its collision owner and
loading the declared sound resources, before applying any startup shortcut.
`Game` must be constructed with deferred shortcuts for that ordering. The
session installs its single collision scope for each update, including the
fast-forwarded shortcut Steps; the scope is removed on every return. The
frontend no longer retains a callback pointing at a separate local Contacts
structure. Devices and immutable resource packs outlive the session.

The currently admitted stored collision environment is authored dither plus
time-field wind, not the full encounter dispatcher. It requires all seven wind
resources (349–355), with actual decoded samples. An unloaded resource prevents
startup rather than deferring a silent sound failure into a later contact.
Settings used by this environment come from the session, not an independent
constant in the collision callback. Adding a family requires its concrete
update/Draw scopes and resource dependencies; having a registered record alone
does not authorize skipping its unbound events.

`GameplayDitherCollisionPass` supplies the equivalent narrow native admission.
It collects the complete owner stream before selecting the 61/60 and
255-or-371/35 pairs; it does not build a second spatial index or scan fields in
storage order. `executeSelectedCollisionPairs` shares the complete dispatch
wrapper with the retained reference path. Room presence/pending-change is a
live caller-supplied read for each event direction. The pass requires explicit
collision backing and masks; an external-only owner cannot silently run it.
Its host differential combines overlapping fields and authored wall pruning,
checks the selected field's sound/RNG against the reference owner, and exercises
the real world collision-phase hook and Draw-time retirement. This fixture is
not evidence that the Amiga frontend has acquired those production leases.

`GameplayEncounterEnvironment` borrows that pass, audio device and settings
for a split native frame. `Frame::bindUpdateEnvironment` requires the session's
input sample first and rejects rebinding or installation after Draw. Dependency
checks run once per environment lifetime (the immutable scene resources must
outlive it); device readiness and settings are checked at update entry.
Draw entry, explicit frame completion and abandoned-frame cleanup all remove
the world's borrowed callbacks. A failed callback cannot become a publishable
frame. In-place Retry preserves both callback identities until the owning
frame releases them; it does not copy or reconstruct a device/resource lease.
Reference lexical scopes remain available and cannot coexist with this binding.

`EncounterSettings` keeps `gameVolume` separate from device master gain.
`scrLoadSave` assembly loads the INI `option/masterGain` default 0.6 and calls
`audio_master_gain`; it does not overwrite the per-sound gain. The stored session
publishes that setting before running an update, including startup shortcuts.
The admitted master range is 0–1; unsupported settings fail rather than silently
clamp. SDL applies it to the mixed output before clipping. The native wind
device combines it with voice gain when computing Paula volume (floor to 0–64),
while `getGain` continues to report the independent source control. This is a
device-resolution adaptation, not a claim of identical Windows DSP output.

The session also binds run-audio reset around startup shortcuts and updates.
`ResetPlayer`/`scrInitialize` stop all voices; the shared restart entry points
invoke that binding before clearing `global.voidWind` and the overlap channels.
A failed device reset aborts the restart and marks the world failed. Native
world reconstruction preserves the lexical binding, not an audio device or
resource bank. SDL startup and retirement stop its owned voices as well.
This binding covers the admitted winds, not the still-unbound reset sound 321
or the later music/resource-group events. `AmigaWindAudio::stopAll` publishes a
VBlank stop command; a same-update play replaces it with a stop/start command,
and pending reload IRQs cannot consume the new sample before that boundary.

Each production session owns `EncounterInput`: raw held history and one mutable
edge publication. Startup, menus and gameplay sample it once per update;
`ShopWorld::inputState()` and registered parent scopes borrow that publication
and the world's currency account, rather than copying input for each visit.
Consumption never alters raw held history. Room changes and Retry preserve that
history; run reset clears only the publication, so a held control cannot become
a fresh press after reset. Stored updates sample at `Game::update` entry,
including level-clear updates; shortcut steps sample their virtual inputs.
Native frames sample before scene dispatch, and gameplay Draw rejects a missing
or duplicate sample. Session retirement unbinds the borrowed publication.
Cancel is a separate held input/replay bit, never an alias for Pause.
Level-clear updates borrow and consume that same mutable Confirm edge. The VM
assembly for `scrControlInput` assigns `global.dUp` from pressed inputs and
`global.dUpHeld` separately; `scrResult` reads/clears the former. Holding Fire
through the upgrade entrance therefore cannot select when the 30-tick wait
expires. Both worlds' `stepLevelClear` boundaries choose the session publication
when bound; explicit input arguments remain for standalone controller fixtures.
DeathMenu Begin Step likewise reads the session's current Confirm/navigation
publication rather than deriving another edge from its own room-local history.
Unlike `scrResult`, its assembly does not clear `global.dUp`, so this borrow is
read-only. Standalone `DeathResults` fixtures retain local-history input.
The SDL controller backend supplies face 2 (B), including device hotplug and
removal. Native scene dispatch uses the same session edges; standalone scene and
Shop fixtures retain their local-history reference paths. This does not add the
remaining gamepad axis/direction semantics.
Isolated event fixtures may continue to provide explicit input/currency through
the reference overloads.

Native Cancel is the second button on a two-button joystick in port 2 (pin 9,
POTGOR bit 14, active low); fire remains CIA-A PRA bit 7. `AmigaInput` leases
that pin through `potgo.resource` before takeover and drives OUTRY/DATRY high
for the button pull-up prescribed by RKM chapter 8, "Digital I/O On The
Controller Port". Resource writes mask only those two bits, preserving the
other port and pins. Missing resource/allocation returns 20 before takeover.
Retirement returns the pin to input mode and frees the allocation; the resource
has no CloseResource operation. No per-update resource calls are needed.
`make test-amiga-input` compiles the real acquisition/cleanup implementation
against an instrumented resource and checks all 16 input combinations, unrelated
pins, replay transport and held Cancel across reset. This is not electrical
testing of a physical joystick or support for CD32 serial-pad decoding.

X is the approved keyboard Cancel binding on SDL and Amiga; Escape quits,
and Pause remains separate. Keyboard and controller Cancel are ORed before
the shared session derives edges. SDL retains held X and short taps, clearing
it on focus loss. Amiga takes the CIA-A serial vector immediately before the
frame loop, using Revs' decode/handshake and displaced-vector ownership model;
it restores the vector before platform restoration. Failed acquisition returns
20 through normal cleanup. `make test-amiga-keyboard` checks resource failure,
vector replacement/rollback/restoration and serial X/Escape decoding; the
interrupt-masked poll acknowledges the short-tap latch without losing a press.

`make test-encounter-session` exercises full startup and `SKIP_TO_GAME`, then
level completion and Retry through the session. Exit placement and lethal
health subtraction are explicit lifecycle fixtures, not natural-encounter
evidence. Missing resources, unbound registered Draw, recursive entry and a
caller ignoring nested failure are negative cases. Cross-target session
integration remains governed by §1.01 in the work queue.

The Amiga frontend retains `GameplayEncounterSession` across its scene loop.
Its lexical frame starts in update, explicitly enters Draw, checks health before
publishing a rendered Copper list, and explicitly completes. Early breaks leave
an incomplete frame and latch failure; room handoffs may complete without Draw.
The gameplay Draw boundary owns the existing deferred-destruction sweep. Blank
lists used to retire DMA before replacing assets are transition staging, not
successful scene Draw publication. A normal quit retires the session; a failed
session returns 20 even if a later world reset cleared a room-local status.
This boundary does not itself admit additional event families or supply missing
audio/environment callbacks. Native ownership's existing observed-failure latch
makes health checks constant-time rather than another family/catalogue walk.
`make verify-amiga-encounter-failure` uses the existing results-verification
build and a disposable `Downwell.reject-draw` marker in the mounted test drive.
The target reads the marker before takeover, then executes a rejecting checked
callback in style Draw. The verifier requires zero subsequent style publications
and an actual return code of 20. The launcher removes the marker before and
after runs; normal builds contain neither the marker read nor the callback.

`make verify-amiga-encounter-update-failure` exercises the candidate native
collision session through the level-end shortcut. Its verification-only audio
device rejects `isPlaying` inside the real exit-field/player collision event.
The debugger checks the bound update context at that callback, then requires
return code 20, no subsequent gameplay Draw entry or publication, and released
world callbacks and audio device. Direct Draw/publication counters are used:
the profiler counts aborted update scopes too and is not a publication witness.
The target enables the candidate through `encounter_collision_verify.mk` only
for this test and restores the normal build on exit. It does not enable that
backend for shipping or prove its remaining performance and transition gates.

`make verify-amiga-encounter-reentry` runs the native collision candidate through
the existing results fixture: Retry, style select, garden and subsequent
gameplay. It retains the rendering/residency assertions and additionally
requires one run-history lease, three encounter scene loads, two releases and
a live audio device on final reentry. Results readiness is injected; reset,
input confirmation, resource retirement and reentry use the frontend paths.
It is not a natural-death or level-completion witness. The results fixture does
not execute the reference terrain-mask query probe, so it must not link that
probe's full sprite-mask pack; the candidate loads its scene encounter bank.
An early executable return fails the verifier immediately. The make target
restores the normal build whether verification succeeds or fails.

`make verify-amiga-encounter-clear` uses the native candidate for the existing
two-upgrade fixture. Exit births are injected; field contact, upgrade selection,
room replacement and subsequent gameplay use the frontend paths. It retains
the independent planar comparisons and checks that the run's collision/audio
lease survives both upgrades. This is not a natural-exit witness. Candidate
fixtures use native ownership and the scene mask bank, never the reference
owner's full mask-pack probe.

The clear fixture's six comparison canvases occupy 173,952 bytes of Fast RAM.
`AmigaClearOracleBuffers` acquires them only after gameplay art retires and
releases them before next-room admission. They must not occupy permanent BSS
alongside gameplay banks. Allocation failure returns 20; repeated acquire and
release are safe. This storage change does not remove any pixel comparisons
or affect shipping builds.

## Scene owners

The Amiga scene translation units are `AmigaPublisherScene`,
`AmigaCreditsScene`, `AmigaControlsScene`, `AmigaGardenScene`,
`AmigaStyleScene`, `AmigaGameplayScene` and `AmigaUpgradeScene`.
Class existence alone does not prove complete ownership; outstanding separation
is tracked in [the queue](open-work.md#6-finish-scene-and-backend-separation).

Splash scenes share `AmigaSplashCache` and splash Copper machinery.
Style and upgrade selection share `AmigaSelectionLayer`, but preserve their
distinct source selection trajectories.
The independent full-frame style renderer and its eleven prepared artwork
wrappers exist only in `SCENE_VERIFY` builds. Normal scene preparation must not
construct verification-only state; playable selection uses `AmigaSelectionLayer`.

## Display lifecycle

### Transient artwork leases

Shop, death-results and upgrade artwork are files, not shipping executable
payloads. `AmigaTransientBank` grants a view only to the declared owner. New
contracts need explicit byte budgets in `generate-amiga-transient-banks.py`;
the generator rejects growth beyond them and records input hashes/tool version.
The shipping link audit rejects accidentally re-embedded transient banks.
Verification builds retain linked shop/results bytes only to compare the
file-loaded copies. The upgrade pixel oracle reads the loaded original pack;
it still independently composes the expected display, without a second 169 KiB
executable copy. The generated manifest pins the source file's SHA-256.

Style selection and ordinary gameplay must not prepare death-results art.
Shop and results load directly into their mutually exclusive Chip bank, without
an ordinary-memory source copy. Repeated frames with the same lease perform no
loading or validation. Failed replacement publishes neither the old nor a
partially read new bank. Shop closure repairs its old footprint before releasing
the lease. Cover DMA invalidates the overlay lease before borrowing its memory.

Upgrade selection allocates its ordinary-memory source only on entry. Selected
records are copied to the scene's Chip bank; the source remains available for
offer/inventory changes and is freed after scene retirement. Exiting the program
also frees it. Acquire failures return code 20, never an empty successful scene.
The link audit, lease tests, entry profiler and clear/restart verifiers enforce
these boundaries. These contracts do not yet partition the gameplay resource,
room, collision-mask or SDL texture catalogues; that scope remains in the queue.

Production gameplay admission is explicit: `AmigaGameplayAssetAdmission`
admits the cavern bank for area 1 (and bootstrap area 0). Unknown/later areas
fail before reuse or allocation; they must not inherit an already-resident
bank or load the reference union. Current Amiga progression stops after the
third cavern level. Enabling another area requires declared, budgeted CPU/DMA
dependencies and verified transition overlap. Replay and plane pixel oracles
have a separate compile-time full-catalogue path. The native load-path audit
rejects `Downwell.game`/`Downwell.planes` loads outside those two oracle builds.
Exporter growth cannot change this admission policy.

`AmigaSceneAdmission` also guards runtime boundaries in normal builds:
gameplay art acquisition requires the gameplay scene; gameplay Step requires
that scene and live CPU art, DMA art, entry-mask and terrain-raster leases.
Rejected admission returns 20, with `gDownwellSceneAdmissionFailure` identifying
acquisition (1) or dispatch (2). Only replay/plane pixel oracles can acquire
gameplay art before presentation. This constant-sized boundary check does not
walk resources or actors, and does not replace family-level dispatch/residency
contracts. Startup and results re-entry verifiers require no guard failure.

Use the platform framework for Copper publication and DMA retirement.
Prepare an inactive list or patch only safe, relevant words; do not rebuild
complete lists for pointer-only scrolling. Failed preparation must prevent
publication. Drain outstanding blits before publication or asset-bank replacement.

Keep asset conversion offline. Use prebuilt hardware sprite images and change
their pointers/control words rather than reconstructing images each frame.

## Retained selection layers

The content canvas is 160 pixels wide, with 176-pixel DMA rows and 608 world
rows. Each plane occupies 13,376 bytes. A viewport for top rows 0..324 changes
pointers, not pixels. Initial clearing includes the guard word; local damage
repair must preserve neighboring pixels, including partial words.

Commands remain in world coordinates. Repair changed commands' old and new
bounds and intersecting retained commands in painter order. Identical command
lists and camera-only movement require no repaint.

Callers supply Chip-resident buffers/art. Invalidate after in-place artwork
replacement unless referenced resources are proven equivalent and explicitly
rebound. Preserve scene-specific palette, mask and playfield-priority rules.

Style selection's immutable border and two opacity masks occupy a different
part of the shared cache from its reference-frame scratch area. Garden entry
begins a preparation plan without writing that cache. Starting with the second
garden frame, after the preceding garden list has replaced splash/gameplay DMA,
service one drained block operation or tile pair per frame. Garden scrolling
uses separate menu planes and leaves the preparation destination untouched.
Only that direct garden-to-style transition may preserve the border
cache, and only with the same asset-bank addresses and a valid prior
preparation plan. Gameplay reuses the storage, so other entries rebuild it.
Both content canvases and both borrowed Copper slots are invalidated even when
the border is retained. Cache retention never implies display-buffer retention.
Partial preparation is never renderable. Direct-to-style entry finishes the
same plan synchronously before publication; an early transition resumes a
pending plan instead of restarting it. Failure latches until a new preparation.
This schedules only immutable pixels, never game state, RNG or input decisions.

For the original 16-by-16 border tiles with vertical origin 8, compose two
anchor rows to establish the first complete 16-row period, then extend it by
disjoint planar copies. Clear only the seed strip: copies write every remaining
pixel, including guard words. Changed tile geometry takes the general drawing
path. The native capture verifier compares all 300 border rows against original
texture pixels, independently of the repeated-strip implementation.

The mask edge comes from offline planar art. Staging fills the two constant
tails and copies the edge through `Bitmap`, with a separate zero guard word
for gradient rows. The regions do not overlap; there is no clear-then-redraw
of the same mask pixels. Drain completion before publishing the mask to DMA.

## Canonical level-clear motion

`LevelClearMotion` owns the same source player/camera state on both targets.
Create sets player Y to zero and camera Y to 142. Alarm 4 exposes choices
before Step 60, changing the player target/divisor to 302/10 and camera target
to 340. The recurrence remains the source subtraction, division, then addition.

The offline `generate-clear-motion` program records the first 59 updates and
the choice sequence through its binary64 fixed point. It compares the portable
arithmetic with independent host binary64 at every generated step. The table
contains full states, never rounded pixels; the generated manifest records
input hashes and extraction provenance. Before each table advance the owner
checks all state inputs. Altered inputs, noncanonical choice timing and all
departure updates permanently fall back to the retained arithmetic for that
motion instance. No RNG, event dispatch, carousel input or rendering is moved
ahead of its original phase. `REFERENCE_CLEAR_MOTION=1` selects arithmetic
throughout. The table resides in ordinary executable memory, not Chip RAM.

## Gameplay presentation boundaries

Terrain raster pixels are a gameplay-only ordinary-memory lease (1,081,344
bytes), not embedded in the startup BSS. Bind before terrain preparation;
retire the raster view before releasing the allocation. Front-end scenes and
upgrade selection hold no raster pixel lease. Re-entry acquires fresh storage
and invalidates readiness, borrowed initial rows and shop revision state.
The existing clean-row contract requires initially zero pixels; clearing occurs
once at allocation, not during ordinary frames. Allocation cannot fall back to
Chip RAM and failure prevents gameplay publication with return code 20.
The host fixture caps the owner object's size; native startup and clear/re-entry
checks assert residency at their respective scene boundaries.

- `AmigaGameplayGeneration`: synchronous builder registration and generation
  damage. Game state and RNG effects remain in core.
- `AmigaTerrainPainter`: fresh terrain composition, with explicit data, artwork
  and view inputs. It does not allocate, step simulation or schedule DMA.
- `AmigaTerrainRaster`: backing raster, dirty rows, shop revisions and ring
  transfers.
- `AmigaGameplayRows`: independent damage/copy debts for both visible buffers.
- `AmigaGameplayScene`: Copper lifecycle and backing/staging validity.
- `AmigaGameplayDrawList`: read-only projection of shared state in painter order.
- `AmigaGameplayActors` and sprite assignment: planar actors and hardware
  channel assignment without changing simulation.

Use a circular terrain backing store and pointer-driven scrolling. Restore
moving-object damage from retained terrain and draw newly exposed rows.
A discontinuous camera jump prepares the destination interval, not every skipped
row. Commit staging validity only after successful transfer; failed or replaced
content invalidates the corresponding range.

Partition mixed cache requests into contiguous clean and dirty row runs. Copy
clean runs from the retained raster; compose and store only dirty runs. Explicit
partial-width repairs remain separate, preserving the other pixels in each row.
Before CPU composition follows a blitter clear, drain queued work as well as
the active DMA. Waiting for the active blit alone does not establish that a
queued clear has completed.

Side-view staging borrows retired storage, never the active restoration backing.
Each update copies at most 48 cached rows and composes at most 16 dirty rows;
completed fresh runs also update the retained raster. This is read-only rendering
of already-created world state, not deferred generation or Create dispatch.
Staging cannot clear either visible buffer's repair debts or invoke their
pipelined publication copies. Commit only the prepared contiguous interval.
For upward preparation, bound the adjacent suffix to the paint budget so a
partial operation cannot claim an untouched gap. World changes still invalidate
staging before a camera switch can use it.

Prefetch only a column the player's resolved horizontal velocity approaches.
Do not infer movement from held input: wall collisions can zero velocity while
the key remains held. A stopped, dead or retreating player leaves completed
staging intact without preparing more rows. This is a rendering prediction,
not an entry rule; camera transitions still use shared simulation and fall back
to destination composition whenever the predicted interval is incomplete.

Retire and publish buffers independently: damage acknowledged in one display
must not clear the other's debt. A Copper rebuild does not itself invalidate
retained pixels. Fixed-capacity draw-list failure must prevent publication,
not silently omit actors.

## Terrain landing candidates

`AmigaLandingIndex` stores candidate occupancy in 128 columns of 512 bits
(8 KiB). Insertion sets one bit synchronously during Create; it cannot defer
visibility until the next frame or pattern cell. Queries search at or below the
requested row and return row+1, or zero when no candidate exists.

Occupancy is not breakable lifetime state. The collision resolver must still
reject inactive breakables and continue to the next candidate. Keep room-build
and live-insertion eligibility unchanged, including their distinct solid/landing
flags. The bitmap test compares every intermediate insertion against the old
nearest-floor table; live terrain tests cover generation and collision behavior.

## Retained ammo HUD contract

Compose damaged HUD strips in the retained Fast-RAM image and publish only
differing words independently in each plane. Do not clear visible Chip RAM
before repainting the same content. The palette is `01=white`, `10=red`,
`11=opaque black`; toggling only the first plane cannot represent every
white/black transition. Preserve genuine rim movement during recharge jiggle.

Cache divider geometry by capacity and recharge offset, but retain the general
source-equivalent path for unsupported cache ranges and consumption values.
Derive constant rim spans from artwork rather than assumed dimensions. Damage
must include old/new label ink, pending wide-label backing repairs and exposed
edges. Scene reconstruction invalidates the retained image. Validate optimized
composition against the complete ordered draw sequence, including clipping,
capacity changes and every meter offset.

## Family routing is not a family scan

Native owner validation observes terminal component failures instead of polling
the implemented family catalogue. Most components use `FailureSignal`.
Dispatchers that pass a mutable `bool&` to synchronous event bodies use
`CallbackFailureSignal`: a stack-scoped borrow exposes that live status to the
owner until the callback and its nested publications return. Nested validation
must see an outer callback's failure even while an inner callback is healthy.
Outside those borrows, assignment notifies the owner immediately; retiring a
borrow latches its final failure. Component recovery cannot clear the owner's
terminal failure. Bind observers before dispatch, and never retain an event's
borrowed status reference after returning. No inactive-family list is walked.
Reference ownership and `REFERENCE_VALIDATION=1` retain full status polling.

Activation setters and inactive births record their slot in
`RunnerActivationChanges`. At the next phase boundary, room runs select only
those slots for eligibility reads. The set does not impose request order:
inactive-room order still precedes active-room order, and reactivation still
performs the original registration, layer and Draw publication. Unchanged
terrain must not resolve identities or read flags because an actor toggled.
The run traversal retains the full reconciliation as its reference; scalar
ownership uses that reference in the shared-owner replay fixtures. Publication
callbacks must not change activation flags or room ownership during this pass.
The change set is cleared only after successful reconciliation; failure is
terminal with the original completed publication prefix. New activation
writers must call `activationChanged`, not just set a global pending flag.
The room fixture grows unrelated membership from 32 to 1,024 entries and
requires one eligibility read for one inactive request, alongside randomized
publication-order, slot-reuse and failure-prefix comparisons.

Internal collision handles use the existing disjoint slot ranges, not Windows
numeric instance IDs. `GameplayCollisionView::decode` reads a compile-time
slot-to-family tag and that family's base. It never walks other families to
identify the requested one. Adding an unrelated family cannot add comparisons
to an existing actor's lookup. Metadata has a 4 KiB compile-time budget, with
no room-entry construction, per-frame preparation or BSS allocation.

Each slot must belong to exactly one declared range; gaps, overlaps and missing
families fail compilation. The previous decoder remains the portable oracle.
`verify-slot-family-map` compares every slot and representative generations,
including failures and encode/decode round trips, and checks unchanged routing
when unrelated families are added. Routing does not prove lifetime validity:
callers still resolve generations and live eligibility at the original event
boundaries. It does not replace the remaining membership-driven scheduling work.

## Stationary thin-platform retirement

Object 93's pinned End Step only calls `scrOutofview`: strict Y comparisons
against view top minus 160 and plus 3000, followed by the main-room check.
Native terrain owns immutable Y coordinates. `StationaryRetirementWindow`
retains the inclusive camera interval in which the captured eligible set cannot
be destroyed. Inside that interval the retirement walk does no work. Outside
it the original registration-ordered walk remains authoritative; destruction
is never reordered or delayed. The scalar owner remains the replay oracle.

Terrain births extend the existing proof and lifetime ceiling directly.
Ordinary walls add no Y constraint; including a newborn platform before its
eligible phase is conservative. Activation, registration reconciliation,
destruction marks and retirement invalidate the proof. Mutations during a walk
prevent publishing its stale proof. Unsupported coordinate ranges and revision
exhaustion retain the original walk. This is specific to the pinned stationary
event, not a general license to skip actor End Step or source callbacks.

## Native no-op End Step admission

The external-collision native scene can omit the mixed End Step capture and
dispatch only when its pinned event bodies have no effects beyond the player's
`playerHit` update. `tryNativeActorEndStepInterior` first checks the existing
stationary box and thin-platform proofs at the current lifetime ceilings.
It then checks the small native enemy/projectile stores against the same
retirement bounds; registration and live exclusion flags still govern which
actors participate. Projectile Y comes from published `sourceMotion`, including
collision writes, not artwork coordinates. It does not walk terrain or the
whole room, allocate a second roster, or construct a new generic snapshot.

No event executes until every proof succeeds. Player 35's pinned body only
advances/clears `playerHit`, so moving that update past these read-only tests
cannot affect admission. Inactive/unregistered players receive no update.
A boundary crossing, nonordinary End Step family, full collision service,
unsupported numeric range or invalidated stationary proof retains the original
captured, ordered dispatcher. That path remains responsible for synchronous
destruction callbacks, birth cutoffs and live activation changes. This is not
a cached result for an arbitrary event callback, nor permission to omit a newly
implemented event: additional actor families require explicit admission proof.

## Read-only actor ordering

Native and reference spatial refresh share the immutable
`PreparedSpriteGeometry` built during owner preparation. Zero-angle,
signed-unit-scale sprites at admitted integer positions translate those
prepared bounds instead of reconstructing the resource shape. Mask IDs retain
their precedence over sprite IDs; missing resources, fractional/oversized
positions and other transforms use the generic refresh. Published bounds,
precise-mask flags, eligibility and ordered spatial insertion are still live
state. No per-instance result or callback is cached by this admission.

`GameplayWorld::visitRoomActors` remains the live layer traversal. Command
collectors that do not change lifetimes, eligibility, sprites or depth may use
`visitRoomActorsReadOnly`. Its owner retains ordered actor handles and depths;
positions, animation frames and Draw side effects are not cached.

Native ownership maintains a `RunnerPreparedDrawList` at mutations. Birth and
retirement splice individual nodes in authoritative layer-element order;
activation reconciliation repositions the affected node. Serviced depth
changes relink from the source layer topology, including layer merges and their
inactive-tail ordering. Registration, exclusions and sprite changes publish
prepared Draw eligibility. Inactive nodes remain as layer cutoffs; scenery-only
population leases and unborn serialized placeholders are not drawable nodes.

The projection retains full ordering links for mutation and a separate sparse
chain for execution. Only `draw` and `endLayer` actions enter that chain;
`skip` nodes cannot add work to an ordinary refill. Predecessor discovery and
chain updates happen on admission, action changes, removal or relinking, not
on every traversal. Inactive barriers must remain even though they paint no
pixels. A callback hiding its own node follows the full successor links so a
newly activated successor is still seen; a newly inserted head is not visited
retroactively. `visitOrderedReference` retains the full-chain oracle for the
mutation and cutoff fixtures. This projection does not cache Draw results or
discard source lifecycle ownership.

Each actor token names a prepared family and generation handle, not a borrowed
record pointer. Enemy compaction therefore cannot invalidate it. Collectors
reuse the flat actor/depth array until this prepared list changes. A refill
walks prepared nodes without resolving lifetimes, object metadata or sprites,
and invokes callbacks as it fills, preserving successful failure prefixes.
Layer depths use their mutation-time binary32 encoding. No source sorting by
numeric resource ID, handle or depth alone substitutes for element order.

The original cached layer traversal is retained by `REFERENCE_DRAW_LIST=1`;
registered ownership also keeps that path, and scalar ownership always uses
the live traversal. Saturated revision counters disable flat-array reuse.
Visitors that mutate ownership must use the live path, not the read-only
projection. Retry preserves both callback bindings and resets the projection.

The flat actor array occupies 3,376 bytes plus bookkeeping. Prepared links and
identities add about 29 KiB in ordinary memory, with no Chip RAM allocation.
Native verification compares kind, generation handle and depth against live
Draw on every tested frame, without consuming Draw RNG.

## Verification scope

Fresh scalar composition provides a retained-buffer oracle. Host tests do not
execute the hardware blitter or prove Copper timing; bounded native runs are
required for those paths. Synthetic camera/shop fixtures do not establish
natural traversal, original Windows raster fidelity or the 20 ms frame budget.

Use the reproducible gates in the Makefile and the procedures in
[profiling](amiga-profiling.md), [replays](replay-format.md) and
[natural routes](natural-route-verification.md).
