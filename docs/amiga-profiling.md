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
