# Amiga profiling procedure

Remaining performance work belongs only in [the work queue](open-work.md#5-meet-the-amiga-frame-budget).
Measurements, completed optimizations and rejected experiments belong in the
commit that records them, not a rolling status document.

## Run a bounded comparison

### SDL slowdown checks

For interactive reports, run `DOWNWELL_PROFILE=1 make run`. Every 180 frames
it reports depth, instance count, FPS, update/draw cost, presentation wait and
tick-limiter wait. Keep the output from both the start and the slow section.

For a repeatable descent with the production frame limiter:

```sh
make build/profile-sdl-game
DOWNWELL_PROFILE=1 DOWNWELL_PROFILE_PACED=1 ./build/profile-sdl-game 2400
```

The profiler has a 45-second deadline and exits itself. Omit
`DOWNWELL_PROFILE_PACED` to measure unpaced throughput; that is not the normal
game's FPS. `DOWNWELL_PROFILE_DRAW_EVERY=10` allows deeper simulation coverage
within the deadline, but must not be used to claim full-frame playability.
Compare state digests at identical ticks to check that presentation optimizations
have not changed the replay. Presentation time includes display synchronization.

### Amiga checks

For entrance-field lifecycle correctness (not timing), run
`bash amiga/verify_field_reentry.sh`. It uses a 45-second emulator deadline
(configurable up to 60), then restores the ordinary build. The same tape runs
in `make verify-gameplay-collision-owner`: first-level field removal, the real
exit/upgrade transition, level-two field survival, and Retry invalidation.
The fixture injects an exit at the player before terrain binding; it does not
prove natural descent, presentation parity, full cross-family creation order,
or stock-A500 memory fit. The native gate checks the stack guard as well.

Before the lifecycle tape, `PreparedResourceVerification` checks all 386
original object defaults, all 4,632 inherited alarm slots (including absent
handlers), and all 741 sprite-bound templates at three integer translations
against direct resource reads and the general bounds implementation. Native
completion requires the exact coverage counts, not only a matching digest.
Its large workspace belongs to the test-only static field tape, never the
Amiga task stack or the normal game. This checks the resource preparation
contract on both CPUs; it does not prove runtime mutation ordering, transformed
bounds outside the unit-scale path, or a frame-time improvement.

From the repository root:

```sh
SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=60 bash amiga/profile.sh fps
SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=60 bash amiga/profile.sh probes
```

Use `bash amiga/profile.sh transition` to traverse splash and garden/well into
60 style-reveal frames. This forces all skip toggles off and compiles in the
profiling stop condition (`PROFILE_TRANSITION=1`). It reports
all three scene windows and validates their worst-frame phase totals. It does
not cover later gameplay, and its instrumented rates are not shipping FPS.
Automatic fire advances the splash pages quickly; this is a transition-cost
tape, not a check of their normal display durations. Instrumentation outside
an active frame is excluded, including pre-loop scene/resource construction;
that startup loading cost must be measured separately, not divided by splash
frame counts. The transition report rejects phase brackets larger than their
scene's maximum measured frame.
Use longer gameplay windows (for example `PROFILE_FRAMES=600 SKIP_TO_GAME=1`)
as well: a short initial-room sample can miss synchronous generation bursts.

For an inclusive generation measurement without inner per-birth probes, use
`GENERATION_TIMING=1 SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=180 bash amiga/profile.sh fps`.
This adds two beam-clock reads around each complete generation callback, not
around every instance or suboperation. It reports total/mean/max generation
time, the maximum's frame and generation time within the worst PAL-gap frame.
The initial setup frame is discarded just as in the FPS window. The live
gameplay tape must execute one generation callback per measured frame; the
debugger rejects incomplete coverage or a bracket exceeding its frame interval.
Compare with the same FPS tape without this opt-in flag: coarse timing still
has overhead, and its inclusive total must not be added to nested probe totals.
The script restores the ordinary build with this flag disabled.

To isolate a stage without enabling every nested probe, also set
`GENERATION_TIMING_PHASE=profileGeneratedRegistration` (the complete generated
instance callback), `profileInstanceBirth`, or `profileBoundsPublication`.
The default is `profileGeneration`; names come from `DownwellProfilePhase`.
Only the selected `DW_PROFILE_SCOPE` reads the clock. Nested scopes of the
same selected stage are included once, not double-counted. Measurements cover
all occurrences in the gameplay window, not only occurrences inside generation.
`PHASE coarse` reports the numeric phase, call count, total and mean per-call
time, maximum per-frame sum, its frame, and selected-stage time in the worst
PAL-gap frame. Only the default generation stage requires exactly one call per
gameplay frame; other stages require nonzero coverage. Zero coverage fails the
run rather than presenting an empty measurement as evidence. This option does
not enable `DW_GENERATION_SCOPE` detail-only brackets. Each selected-stage run
still has its own clock overhead; compare against plain FPS and do not add
inclusive results from separately selected parent/child stages.

For statistical attribution with no per-scope clock reads, use
`PHASE_SAMPLING=1 SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=180 bash amiga/profile.sh fps`.
This FPS-only mode publishes a word-sized phase marker on `DW_PROFILE_SCOPE`
entry and restores its parent on exit. The existing VBlank handler counts the
innermost marked phase once per field, both for the full gameplay window and
for `SAMPLE_FRAME` (default 701, one-based after the discarded setup frame).
`SAMPLE phase` numbers follow `DownwellProfilePhase` in `src/core/Profile.h`.
`profileTerrainRecord` covers terrain-pool append outside nested birth scopes;
`profileTerrainCreate` covers the following source Create property writes.
No gameplay callbacks or RNG consumption are moved. The scope markers still
have overhead; compare the same tape with plain FPS. Samples are not precise
phase durations: periodic sampling can alias repeated work and short phases
can receive no samples. Unscoped work, including phase-only markers and waits,
is attributed to its enclosing scope or `profileOther`. Inner
`DW_GENERATION_SCOPE` detail probes remain disabled. This option is mutually
exclusive with full probes and coarse generation timing. The script rejects
an unobserved focus frame and restores the playable build with sampling off.

The FPS report also prints `PREPARATION terrain`, measured once at owner
construction before hardware takeover. It reads the OS graphics vertical-blank
counter directly (no OS call) around the table preparation, with compiler memory
barriers at the boundaries. This is elapsed emulated PAL time at 20 ms resolution,
not a CPU-only cycle count; allow one field of quantization uncertainty. It is
outside the gameplay timing window. Missing or repeated preparation fails the
FPS coverage check. This counter must not be used after takeover, when the
framework owns VERTB and the OS counter no longer advances.

Probe `segments` (the legacy `gDownwellProfileCalls` counters) include parent
resumption after nested scopes: they are not function invocation counts.
`scope_entries` counts only `DW_PROFILE_SCOPE` construction, with both the
whole-window total and the count in the retained worst frame. Explicit phase
switches do not increment it. This distinguishes a large birth burst from a
single expensive operation. These counters exist only in PROBES builds.

Both modes report the first frame attaining the largest PAL-field gap. Compare
this frame number across FPS and PROBES before attributing an FPS stall to the
instrumented worst frame: instrumentation can change which event is worst.
The PROBES exclusive-tick maximum and PAL-field maximum are separate measures.
Frame numbers are one-based within the measured scene window (gameplay's
discarded setup frame is excluded).

Gameplay FPS/probe reports include a `COVERAGE pre-Step` line. It samples once
at the start of each measured gameplay frame: first/last world tick, initial,
last and maximum displayed view Y, maximum room-space player Y in Q8, last
sampled health, health-nonpositive sample count and maximum allocated enemy
record count. The latter is not a visible or active enemy count. The last
sample precedes the last measured Step; do not label it an end-of-frame replay
checkpoint. GDB rejects a sample-count mismatch. These reads are profiling-only
and do not alter inputs, RNG or simulation. They add a small measurement cost.
Coverage discards the first scene-building frame, just like the timing window.

Check this coverage before interpreting FPS. A long tape can stay on a platform
or spend its tail in death/results; frame count alone does not prove deep-well
generation or dense-combat coverage. Use the existing `PROFILE_CONTACT=1`
1800-frame natural-contact tape as a second workload (without `PROFILE_FRAMES`),
and inspect its coverage too; its damage-birth check alone does not prove depth
or survival. Keep route/window settings with comparisons.

The contact tape's host gate installs a terrain identity store and counts actual
birth callbacks (not net changes in live population). It reports and pins the
largest wall/platform burst while retaining the paired state and full-RNG checks.
This classifies the workload; the lightweight host observer is not the native
collision/layer owner and supplies no native timing or ownership-equivalence
proof by itself.

For a short diagnostic prefix, add `PROFILE_FRAMES=11`. This compiles the
stop count into the ordinary gameplay profiling build; it does not rely on
debugger writes to target memory or breakpoints inside measured frames. Zero
(the default) keeps the full 600-update tape. Prefixes cannot be used with
contact or level-clear tapes and cannot establish their performance coverage.
The wrapper restores the ordinary build with the override disabled.

Collection phases 80 and 81 separate owned layer traversal from per-actor
preparation. Phase 80 excludes callback preparation (81); both are excluded
from phase 42. Add 42+80+81 when comparing with older aggregate collection
traces, allowing for the extra instrumentation overhead. Actor preparation
includes coordinate/animation selection, asset bounds and foreground insertion;
it is not planar drawing or DMA publication. Ordinary builds omit these probes.

Birth phases 82–86 separate native instance registration (82), collision bounds
refresh (83), its nonzero-angle trigonometry (84), room-layer attachment (85),
and shared script-effect creation (86). These are exclusive nested scopes:
effect creation excludes instance birth, and instance birth excludes nested
bounds and layer attachment. Bounds refresh also runs outside births. Sum the
relevant nested phases when comparing older caller totals; phase 86 alone is
not the total cost of an effect. Normal and FPS-only builds omit these probes.

Layer registration is further separated into lifetime adoption (119), fallback
depth/layer resolution (120), element attachment (121), and free-slot allocation
(122). Nested times are excluded from their enclosing scopes, including 85;
do not add an enclosing scope's call count to its child count as though they
were separate births. These probes also cover the corresponding operations at
room initialization when that work occurs inside a measured frame. Numeric-ID
lookup only applies to numbered lifetimes: generated terrain currently has zero
source IDs, unlike authored room terrain. A faster numeric-ID search therefore
does not imply faster generated-terrain registration.

Object-population counting has its own exclusive phase (123). It measures the
registration-list walk used before `place`, `point` and `collisionLine` select
their list/tree path. It does not measure tree publication, list-only point
selection, or collision-pair collection. Zero calls only excludes this counting
path in the measured tape; it is not proof that every collision consumer is
inactive. Compare it with spatial publication counters before changing query
data structures or introducing retained population counts.

`Spatial reads` counts calls at the tree's public box-query, line-query,
height and node-count entry points (including rejected query requests). These
are root calls, not visits to individual nodes. They cover collision-pair tree
queries as well as selector queries, but not direct membership-list traversal
or the independent native terrain grid. Like publication counters, their
window starts with the measured scene; zero reads cannot establish that the
tree was unused before that window or will remain unused later in the level.

`Spatial journal` separates queued operations from physically serviced ones,
buffer-pressure drains and eager fallbacks. Serviced operations can exceed
queued operations inside a scene window because work was queued before that
window. Source publication counters remain logical publications, not physical
tree insertions. Compare FPS-only deadlines as well as these instrumented
counts when assessing [idle-time scheduling](spatial-publication-scheduling.md).

The shared element links maintain a two-level free-slot bitmap. Allocation
uses it to reproduce the lowest-unowned-element scan, then still checks the
instance registry's presence flag. All element owners update the same map;
releasing a non-instance element cannot leave a stale instance-only hint.
Detach/reinsert changes occupancy, while a layer concatenation does not.
The lowest free slot is retained and updated at occupancy changes, so sequential
allocation normally needs no bitmap search. The 3,323-element gameplay pool
uses 436 bytes for this map and no longer needs
the registry's 12-byte native cursor/revision hint or the links owner's 4-byte
vacancy revision (420 bytes net additional storage). This is CPU-side storage;
it does not allocate Chip RAM or change Copper/bitplane ownership.
Boundary and mixed-owner tests compare selections with the original linear
scan, including full pools, reset, rebind release and non-word-sized capacities.

Phases 87 (sprite bounds geometry) and 88 (bounds-cache update and spatial-index
publication) further split refresh. Publication includes the source no-resource
branch, which updates the cache without inserting into the index. When nested
in phase 83, their time is excluded from that phase's total.

Animation phases 89 and 90 split the initial simulation work: 89 covers the
room-instance animation walk (including lifetime validation and previous-position
capture), while 90 covers flash, smoke and corpse-expiry animation. Both exclude
their nested phases and are excluded from simulation phase 3. These brackets
run once per update rather than once per instance to limit probe overhead.

Phases 92–95 split spatial publication into insertion, removal, node partition
and child selection. Nested costs are exclusive: partition/selection are
excluded from insertion/removal, and all are excluded from phase 88 where
nested. These probes compile away in ordinary and FPS-only builds.

`Spatial publications` counts admitted bounds publications after normalization:
`first` has no existing index-membership flag; `reinsert_equal` and
`reinsert_changed` distinguish already-indexed instances by normalized bounds.
The counts describe attempts (an index failure can still follow), not every
geometry refresh or dirty-queue mark. They exclude direct index operations
outside the bounds-publication adapter and activity before the profile resets
its counters. Equal bounds still execute source removal/reinsertion; equality
does not prove that skipping publication would preserve traversal order.
Use these counts to distinguish new population from repeated live updates
before choosing a scheduling or caching change.

Phase 91 brackets only enemy, bullet, script-effect and debris animation calls
inside the native room walk, after lifetime validation and previous-position
capture. It is excluded from 89. Entries with no animation dispatch have no
inner probe: probing every entry can overwhelm the actual traversal cost.
Compare 89+91 with older outer-only traces, allowing for probe overhead; use
low-overhead FPS tapes for ordinary execution timing.

Use `SKIP_LEVEL=1` for level completion and `SKIP_INTRO=1` for style selection
instead of the gameplay shortcut. Use `PROFILE_LASER=1` when investigating
the upgraded Laser Sight fixture. Keep identical inputs, seeds, build options
and emulator configuration for before/after comparisons.

Use `SKIP_TO_GAME=1 PROFILE_CONTACT=1` with either mode for the 1,800-update
natural-contact tape. It centers on the entrance bowl, fires to descend, then
sweeps left/right. It does not inject enemies, change health or call the damage
script directly. Both reporting scripts reject this tape if no damage number
was born. The ordinary 600-update tape is retained for existing comparisons;
zero damage births there is not a measurement of damage handling. Contact mode
requires autorun and cannot be combined with the level-completion shortcut.

The input policy and duration live in `GameplayContactProfileInput.h` and are
shared with the host contact gate. The profiling wrapper runs this gate before
building contact instrumentation. It can also be run separately:

```sh
make verify-contact-profile
```

This gate starts at the ordinary seed-2 gameplay checkpoint, sends identical
inputs to both worlds, and requires a real damage-number lifetime while checking
movement, health, effect ownership and complete RNG state. If a behavior fix
invalidates the tape, the optional `DOWNWELL_CONTACT_PROFILE_SEARCH=1` environment
variable reports the ten fire-phase variants; it does not bypass the normal
gate's contact requirement. Rebaseline both timing modes after changing inputs;
old/new FPS values from different trajectories are not optimization evidence.

Damage phases are 75 (contact direction math), 76 (number construction),
77 (number Step, including empty-owner calls) and 78 (native text drawing,
including the initial blitter drain). The birth counter records successful
allocations only and is available in both modes. Nested scopes restore their
enclosing phase, so their costs are exclusive in the phase totals. Maximum Step
cost can include first-update trigonometry; do not interpret it as the cost of
every surviving number's update. Compile-time-disabled hooks change no state.

Controller composition has its own nested phase 79: currency and smoke
placement/stamping, enemy shade subtraction and final visibility testing.
It is excluded from phase 42, which measures the surrounding actor-list
collection. Older traces included both under 42; compare their combined totals
when reading those traces, allowing for the extra instrumentation overhead.
Returning from the nested scope starts another 42 bracket, so its call count
is not a count of actor lists or frames. Neither phase includes later surface
publication or hardware-sprite channel selection.

The wrapper builds the selected instrumentation, runs the bounded debugger
fixture and restores the ordinary startup build on exit, including failure.
Results are copied to `amiga/.run/profile-fps.log` or `profile-probes.log`;
restoration output is in `amiga/.run/profile-restore.log`. Preserve relevant
evidence before another run overwrites it. Stop only the emulator owned by the
test; do not leave it running after the measurement.

## Interpret the two modes separately

`fps` uses low-overhead field counting for throughput and worst field gaps.
`probes` attributes time to simulation, terrain, HUD, actors and publication.
Fine-grained instrumentation changes execution cost: do not report its FPS as
shipping performance or subtract its timings from an uninstrumented run.

Terrain wall-selection counters measure retained-record passes, records
examined and records passing the vertical footprint filter. The examined count
is added once per pass, avoiding an instrumentation write for every rejected
record. Row candidates are not actual blits: hidden/inactive/non-wall records,
horizontal clipping and presentation validation can still suppress drawing.
These counters isolate selection cost from the exposed-row raster policy;
a large examined count does not mean the whole level is being repainted.

Phase 96 isolates pending destroyed-instance sweeps at Draw entry from phase 4
(the historical `title` label also covers gameplay HUD/overlay updates). It
excludes nested spatial removals; frames without a pending sweep do not enter
this bracket. This distinguishes room cleanup from pixel drawing costs.

Birth phases 97–99 separate lifetime registration, resource binding/defaults,
and sorted room-list insertion from phase 82. Spatial bounds publication and
layer attachment retain their existing separate brackets. These probes do not
defer or reorder synchronous Create work, and compile away in ordinary builds.

Bullet phases 100–102 isolate enemy queries/impacts, solid queries/impacts,
and first-Step angle/per-Step position publication from phase 17. The remaining
phase 17 includes motion, view checks and loop/retirement work. Query brackets
include their existing hit callbacks; do not interpret them as pure geometry
timings. Nested bounds/spatial probes remain exclusive.
Phase 103 isolates split-coordinate conversion from its publication caller;
compare it separately from lifetime lookup, setter and bounds-update costs.
Phases 104–105 isolate the binary32 fallback in spatial partition and child
selection. Their time is excluded from phases 94–95. Compare fallback calls
with their parent request counts before attributing spatial cost to software
floating point: the exact-integer path may already handle the measured workload.
Phases 106–109 separate alarm registration capture, snapshot sorting, live
candidate scanning/metadata resolution and callback execution from phase 18.
All are exclusive. Callback brackets count admitted countdown callbacks, not
just alarms that fire; their instrumentation cost must not be treated as
shipping overhead. Use the low-overhead FPS tape for end-to-end comparisons.
Phases 110–115 separate enemy-family movement from shared position and
presentation publication. Terrain queries and other nested phases remain
excluded. Family phases include their synchronous sprite/transform callbacks;
phase 115 includes common post-movement setters, not rendering. Per-enemy
brackets add probe overhead, so compare like-for-like instrumented tapes.

Inner generation probes are opt-in with `GENERATION_DETAIL=1`; frequent phase
switches can substantially inflate generation bursts. Prefer coarse phases
for comparisons. The wrapper resets this option when restoring the build.

Record the executable hash, revision, emulator CPU/memory configuration,
build flags, tape length, field count, worst gaps and probe clock-clamp count.
Averages alone do not establish the 20 ms requirement. Report scene preparation
separately from steady-state updates and distinguish waits from useful work.

## Pair timing with correctness

Preserve full RNG state, synchronous Create order, collision order and effect
ownership while optimizing. Re-run deterministic replay and fresh-versus-retained
render comparisons for the affected path, then check the restored build's
68000 instruction/helper audits.

Use separate dense-combat, horizontal-shop, HUD-change and selection-transition
cases. A passing fixed tape proves neither all-scene timing nor original-game
fidelity. Expanded-memory emulator runs do not establish stock A500 memory fit.
