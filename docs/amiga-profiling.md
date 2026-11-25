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

The target is the A1200 with 4 MB Fast RAM (68EC020 at 14.19 MHz, AGA,
2 MB Chip), and the startup, replay, profile and run launchers default to it
(`MACHINE=a1200-4m`, `amiga/machine.sh`). `MACHINE=a500` (1 MiB Chip, ECS
8372A Agnus, 8 MiB Fast) and `MACHINE=a500-512k` remain for relative
comparisons with the older A500 evidence; `VERIFY_CHIP_KB=1024` and `512` are
their older spellings. The launcher prints the selected machine.

Every measured optimization is part of the normal build. The Amiga Makefile
options that select an older path (for example `TABLE_CONTACT_ATAN=0`,
`TERRAIN_DIRECT=0`, `PREPARED_FIELDS=0`, `AMMO_FACTORED=0`) exist only as
references for paired measurements and pixel/replay oracles; never ship one.

For the deterministic no-input fall, use `bash amiga/profile.sh quiet-fall`
for the complete 164-update landing/cadence witness,
`bash amiga/profile.sh quiet-state` for its 165 state samples (compared with
`docs/amiga-quiet-states.txt`, identical on every machine;
`QUIET_STATE_RECORD=1` rewrites it after an intended behaviour change), and
`bash amiga/profile.sh quiet-window` for instruction-level costs. Both run on
`MACHINE` like every other mode, cap the emulator run at 90 seconds and
restore the normal binary. On `a1200-4m` the fall must present every update
in one field (164 fields); other machines are judged only with an explicit
`QUIET_MAX_FIELDS`, otherwise only the landing witness is checked. The latter captures 75 PAL fields from mark 56, without debugger stops
inside the window, in a fresh `tmp/trace/quiet-window.*` directory. It writes
`quiet.txt` for complete updates 57–121, `generation-122.txt` for the first
generation burst, and `frames.json` containing exact cycles and the ELF hash.
`verify-quiet-window.py` checks that hash against the frozen ELF and compares
against the baseline of the capture's machine: on the A500 the whole frames
in `docs/amiga-quiet-frame-baseline.json` (its updates overran their field,
so frame time was work); on `a1200-4m` the work in
`docs/amiga-quiet-work-baseline-a1200-4m.json`, from the `work.json` the mode
also writes for updates 57–122 (each frame minus `AmigaPlatform::waitPublished`,
since every quiet frame fills its field there). A machine with neither is not
judged. Quiet mean may rise by at most
0.5 ms, quiet worst and update 122 by at most 1 ms. These are regression
tolerances around the recovered build, not the historical restoration target
or a 20 ms requirement. The guard judges whole windows, not which update owns
a redistributed publication wait. Its sabotage tests are
`python3 tests/quiet-window-guard-test.py`.
Missing even one requested complete update fails the report: a slow run must
not silently turn into a shorter, easier sample. Keep reports with the measured
commit and delete disposable raw traces afterward. Neither command establishes
combat performance, original-runner fidelity or completion of the restoration
goal; compare the same windows, workload and memory configuration.

Cross-CPU replay digests must use the same contact-direction precision policy
on both sides. Both builds use the table by default; a reference built with
`DOWNWELL_PORTABLE_CONTACT_ATAN` (host) or `TABLE_CONTACT_ATAN=0` (Amiga) must
be paired with the same choice on the other CPU. Comparing the two policies
intentionally changes the damage-number presentation digest; keep the separate
angular-error and gameplay-state checks against portable atan.

To see what a gameplay frame contains, `DUMP_TICKS=600,640 bash amiga/profile.sh
frames` (with the usual tape variables) saves the published buffer and sprites
at each tick as `tmp/frames/frame-<tick>.png`. That is memory, not the beam:
DMA-level faults (a sprite that never reaches its stop line, a wrong fetch
window) only show in the per-field screenshots embedded in instruction traces;
`tools/extract-trace-screenshots.py` writes them out as JPEGs.

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

Use `VERIFY_TIMEOUT_SECONDS=180 bash amiga/profile.sh entry` for natural
gameplay entry. The shared input-only tape advances the splash pages, waits
40 garden updates before walking right, confirms the default style on its
120th update, and samples 120 gameplay frames with released controls. The
host gate (`make verify-entry-profile-input`) checks the route and confirmation;
the native gate requires all four scene windows and exact worst-frame phase
accounting. No shortcut or debugger state writes are used.

Unlike steady-state gameplay profiling, this mode retains the cold first
gameplay frame. The frame that confirms the transition is charged to the
scene active at frame start: inspect the **last style frame** as well as the
first gameplay frame. Its handoff brackets distinguish field binding, world
creation, asset staging, raster preparation and backing construction. Inner
terrain/HUD brackets remain separate, so sum exclusive phases rather than
adding inclusive timings. These PROBES measurements include instrumentation;
they are not shipping FPS. Pre-loop loading remains outside this tape.

Use `bash amiga/profile.sh shop` for the natural seed-4 two-level shop route
(600-second emulator bound by default). Before building, the host pairs the
route against the decoded startup adapter and exports only its buttons to
`generated/shop-profile-input.h`, with resource/tool hashes in the adjacent
manifest. No target-side route searches, injected shop, currency, health or
terrain are used. The verified seed-4 room-entry RNG checkpoint replaces the
usual seed-2 profiling checkpoint. Upgrade input is held fire; gameplay tape
cursors advance only after a real Step, not display preparation.

The native completion gate requires side-camera and shop-menu coverage,
return to the shaft alive, and the exported final health/currency/position
and complete RNG state. Sections are 0 first level, 1 upgrade/room change,
2 second-level shaft, 3 side camera, 4 shop menu (selected before Step).
The first cold gameplay setup is excluded, as in the contact tape; subsequent
upgrade and reentry stalls remain included. These are continuous profiling
frame numbers, not the per-level input cursors.

`PROBES=1 bash amiga/profile.sh shop` selects exclusive phase measurements,
including exact worst-frame accounting. Do not compare its instrumented FPS
with an ordinary build. To trace the same route, use `PROFILE_SHOP=1
SKIP_TO_GAME=1 TRACE_FRAMES=... TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=600
bash amiga/profile.sh trace`. All modes restore an uninstrumented normal build.

For a stall longer than the capture limit, the report's explicit
`--partial-frame=N` option attributes just the captured slice. It rejects
whole-frame filters and labels every duration as slice time, not frame time.
This can identify a hot caller, but cannot prove a frame-budget improvement;
use the full-frame phase measurement for that. Default reports still reject
incomplete frames.

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
entry and restores its parent on exit. `DW_PROFILE_PHASE` also publishes the
current phase, so top-level wait, simulation, rendering and publication work
are distinguished rather than folded into `profileOther`. Nested scopes restore
the most recent phase at their entry, including after early returns. The
existing VBlank handler counts the
innermost marked phase once per field, both for the full gameplay window and
for `SAMPLE_FRAME` (default 701, one-based after the discarded setup frame).
`SAMPLE phase` numbers follow `DownwellProfilePhase` in `src/core/Profile.h`.
`profileTerrainRecord` covers terrain-pool append outside nested birth scopes;
`profileTerrainCreate` covers the following source Create property writes.
No gameplay callbacks or RNG consumption are moved. The scope markers still
have overhead; compare the same tape with plain FPS. Samples are not precise
phase durations: periodic sampling can alias repeated work and short phases
can receive no samples. Unscoped work is attributed to its enclosing scope or
the most recent phase marker. `profileWait` includes unbracketed idle servicing
as well as waiting; it is not a CPU-idle duration. Inner
`DW_GENERATION_SCOPE` detail probes remain disabled. This option is mutually
exclusive with full probes and coarse generation timing. The script rejects
an unobserved focus frame and restores the playable build with sampling off.

### Per-update work census

`CENSUS=1 bash amiga/profile.sh MODE` measures every main-loop iteration of
a whole route without a 500 MB trace. The build (`CENSUS=1`, an `FPSCOUNT`
build; `CENSUS_UPDATES`, default 16384, sets the ring) records one 16-byte
record per iteration, mark to mark as the trace splits frames:

- its span and its work, the span minus every display wait
  (`AmigaPlatform::waitVblank` and `waitPublished`, with their idle work);
- its scene, room speed and run stage (area and level);
- flags: room speed below 60 (fxTimeChange, momentDelay, damage), a
  deliberate wait (room speed, sleep), a scene or level handoff (the update
  where it happened and the first of the new scene), the level clear,
  results or upgrade overlay, a side-room camera column change, and the
  first gameplay update.

Times are beam positions (313 × 256 ticks per PAL field), so they mean the
same on every CPU. The ring is written in Fast RAM and read once by the
debugger at the mode's ready marker or where a route ends. A debugger stop
halts the whole emulated machine, beam included: with a stop at every input
sample of the quiet fall, plus a memory read or a joystick event at each, the
ring stayed byte-identical to a run without stops.
A full ring keeps the newest records and the report says how many were
overwritten. The modes that run to the freeze take it as they are (`fps`,
`idle`, `quiet-fall`, `shop`, `overflow-state`, `draw-cache`, `draw-order`,
`ammo`); `entry` and `transition` run their routes as census builds without
probes (`census.gdb`). `CENSUS=1 ROUTE=name bash amiga/profile.sh route`
replays a host route (`tmp/sound-route-name`, from `make
test-sound-routes`) from boot with joystick events only, on an FPSCOUNT
build without autorun: `tools/verify-world-one-native.py` keeps its witness
and sound checks and reads the ring where the route ends, with the VBlank
field count as a check on the beam clock. `hard-mode-death*` routes start
from a hard-mode save and end at the results. `ROUTE_END_TICK=T` ends the
route at simulation tick T, for a short census. With
`CELL_REPAIR_VERIFY=1 DOWNWELL_ROUTE_CELL_ORACLE=1` (not a timing build), each
merged destroyed-cell repair is compared with the per-cell reference, and
the route fails at T unless spans were merged and all of them matched.
`EFFECT_CLEAR_VERIFY=1 DOWNWELL_ROUTE_EFFECT_CLEAR_ORACLE=1` (not a timing
build) checks the effect surface's rectangle clear: before every stamp and
clear, each nonzero stored word must lie in the tracked rectangles, and
after each clear the whole storage, guard words included, must be zero.
`EFFECT_PIXEL_VERIFY=1 DOWNWELL_ROUTE_EFFECT_PIXEL_ORACLE=1` compares the
blitter surface with BOB_VERIFY's CPU reference surface after every
composition, without SCENE_VERIFY's scripted input; the route stops at the
first mismatch.
`EFFECT_BLIT_CENSUS=1` (with the census) adds a parallel ring, dumped to
`.run/effect-census.bin`: per update, the effect stamps and their blit
words by kind (smoke, gems, other, shade), the clear's and the publish
copy's words, the CPU's waits for the effect queue, and the time from the
first stamp to the end of the first wait. `EFFECT_BLIT_CENSUS=2` also
drains around each effect blit and times it alone; its update totals no
longer describe the overlapped game.
`BLIT_CENSUS=1` (with the census) adds a ring of every blit started per
update (`AmigaHardware`), dumped to `blit-census.bin` in the census output:
counts and words by kind, the queued ones, and the per-plane twins (a blit
repeating the previous one's kind and size one plane stride further on,
which one interleaved blit would cover). It also times every blit with
the blitter-finished interrupt, which this build takes on every blit (the
game takes it only while blits wait): the blitter's busy time, the time it
sat finished while blits waited, and the CPU's drain waits. The interrupt
and hooks add about 1 ms per gameplay update. Read it with
`tools/amiga-blit-census-report.py CENSUS_DIR`.
Without the census,
`ROUTE_TRACE=N` records `ROUTE_TRACE_FIELDS` (default 20) whole fields of
every instruction from the input sample of gameplay update N-1 into
`TRACE_OUTPUT` (default `tmp/trace/route-name-N`), then ends; read it with
`tools/amiga-trace-report.py`. No input changes during the capture, so
update N uses update N-1's input, and the log says when the tape differs
there. Every verification run is warped (`verify_startup.sh`), so wall time
goes to the builds and the route driver's per-sample round trips.

`tools/amiga-census-report.py census.bin capture.log` (run by profile.sh,
output in `CENSUS_OUTPUT`, default `tmp/census/MODE`) prints the work mean,
worst update and updates over 20 ms with their excess for each class:
gameplay, slowdowns, handoffs, overlays, camera jumps and the front end.
Handoffs are also grouped into transitions with their total latency, as
handoffs are judged. `--list=over|all` lists updates, `--frames=N-M`
restricts to gameplay frames, `--json` writes everything. On the initial fall
the census matches the cycle-exact traces' work (frame minus
`AmigaPlatform::waitPublished`) within 0.17 ms per update, 0.02 ms on average.

### Cycle-exact instruction traces

For exact attribution with no in-game brackets, record FS-UAE's instruction
trace (the Barto remote debugger's `monitor profile`) of an FPS-only build:

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FRAMES=50,250,690,1300 TRACE_FIELDS=100 \
  VERIFY_TIMEOUT_SECONDS=900 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/run --frames=1300-1338
```

`TRACE_FRAMES` lists one-based gameplay frames, numbered as in FPS reports.
Each capture spans `TRACE_FIELDS` PAL fields (at most 100, about 500 MB each).
Keep listed frames further apart than that. A capture must start a few
frames before a burst frame (frame 701 is captured from 699 in its own build).
If a faster renderer makes a capture overlap a later requested marker, the
debugger stops at the finite tape's end and reports the missed marker rather
than waiting until the wall-clock deadline. Separate those windows and rerun.
The [frame-cost audit](amiga-frame-cost.md#configuration-and-reproduction)
lists the complete command set to repeat it. The build adds one
`downwellTraceFrameMark` call per gameplay frame, and a debugger stop at each
listed frame before its capture. Neither reads the clock or changes game state.
Emulated time does not advance while GDB is stopped. Recording starts at the
next vertical blank, after the first frame has already begun executing.
Simulation work can occur before that blank: the first frame is partial,
not merely missing idle time. Both first and last frames are excluded from
the report. Start captures before the frame being measured. Output goes to
`TRACE_OUTPUT` (default `tmp/trace/run`) with the matching ELF and GDB log.

Every executed instruction is recorded with its cycle cost and registers.
The report rebuilds user-mode call stacks from `jsr`/`bsr` pushes and SP, and
keeps interrupts on a separate stack. It expands every frame through
addr2line's inline chain, so LTO-inlined functions and lambdas remain visible.
It prints per-frame totals with their largest parts, self and inclusive time,
out-of-line call counts and cycles per call, and a call tree. It also splits
cycles by instruction class: call/return, `movem`, pushes, stack-frame
operands, other memory operands, helpers and the wait loop. `--causes`
charges each stack to a cost owner, such as runner bookkeeping, R-tree upkeep
or a rendering subsystem. The owner patterns are listed in the tool. `--frames`,
`--min-fields`, `--focus`, `--lines` and `--collapsed` (folded stacks for
flame graphs) narrow the view. Cycles are CPU clocks of the emulated machine,
not wall-clock time: a PAL field is 142,102 on the A500's 68000 and 284,204
on the A1200's 68EC020. The report reads each capture's cycle unit from its
header, requires every field to be that long, refuses captures of different
clocks, and records the field length in its summaries; `amiga-fall-budget.py`
and `amiga-frame-compare.py` scale by it and refuse to mix machines. The
publication wait is the `runDownwell()` lambda that calls
`AmigaPlatform::waitPublished` (`{lambda#10}` today); its number moves with the
code, so check it in the tree before passing `--work-wait` or `--wait`. On the
020 core a profiled field runs slightly differently from an unprofiled one, so
compare only captures that profiled the same fields. Gameplay must
still be checked against deterministic replays. The trace describes one run
of one tape.

With blitter priority enabled, a stalled instruction can cross VBlank and
FS-UAE records its entire cost in the following field. The reporter keeps
those cycles attached to that instruction. It accepts a field deficit only
when the next field's first instruction covers it and the two field totals
reconcile within the existing trace tolerance. An unexplained surplus,
consecutive deficits, or an unresolved deficit at capture end still fails.
No cycles are added, dropped, or reassigned to manufacture a timing gain.

Logical attribution keys retain untruncated template/owner/member names.
Tables elide template arguments only when displaying labels; labels must never
be used to merge stack frames or costs. `--focus` accepts an exact logical name
or a substring (for example `attachPreparedTerrainBirth`) and prints each
matching specialization separately. `--lines` accepts a substring and aggregates
matching leaf frames by source line. Folded-stack exports keep the untruncated
keys. An older report that shortened keys to 110 characters can merge distinct
methods of a long templated owner: regenerate such method-level reports from
their original captures. Raw frame cycles and raw-symbol call counts do not
depend on those shortened logical keys.

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

The host gate also reports a camera-centre census for enemies, bullets,
currency, smoke and debris. It excludes inactive/destroy-requested records and
hidden debris. Actor positions are relative to `(240,176)`; particle positions
are absolute. Counts use the source camera's half-open `160x284` rectangle;
they are not pixel visibility or occlusion measurements. Family maxima may
occur on different frames, so use the separate simultaneous peak composition.
The report includes total frames and longest consecutive run with at least 24
centres. That reporting threshold alone is not a dense-workload acceptance gate.

For bounded, host-only input exploration:

```sh
make verify-encounter-coverage
DOWNWELL_ENCOUNTER_PROFILE_SEARCH=1 make verify-contact-profile
DOWNWELL_ENCOUNTER_PROFILE_SEARCH=1 DOWNWELL_ENCOUNTER_SEEDS=16 make verify-contact-profile
DOWNWELL_ENCOUNTER_PROFILE_SEARCH=1 DOWNWELL_ENCOUNTER_CASE=8:50 make verify-contact-profile
```

This checks 64 routes of 1,800 updates, retaining the seed-2 entry checkpoint
and comparing each update and all RNG words with the source-derived runtime.
Only left/right/fire inputs vary. It reports candidate counts; it does not
select or install a native profiling tape. The native wrapper clears both
search variables before running its pinned contact gate, plus the seed/case
selectors. `DOWNWELL_ENCOUNTER_SEEDS=1..64` repeats the 64 routes for consecutive
ordinary startup seeds beginning at 2. Each seed fast-forwards the usual
startup and imports its builder/RNG state before generation; it does not
overwrite an existing level's RNG. `DOWNWELL_ENCOUNTER_CASE=seed:route` selects
one seed (1..65535) and route (0..63), without a multi-seed sweep. The case is
printed before simulation so failures can be reproduced without replaying
earlier routes. Actor pixel mismatches report identity and both precise
coordinates without relaxing any parity assertion.

Damage phases are 75 (contact direction math), 76 (number construction),
77 (number Step, including empty-owner calls) and 78 (native text drawing,
including the initial blitter drain). The birth counter records successful
allocations only and is available in both modes. Nested scopes restore their
enclosing phase, so their costs are exclusive in the phase totals. Maximum Step
cost can include first-update trigonometry; do not interpret it as the cost of
every surviving number's update. Compile-time-disabled hooks change no state.

Controller composition has its own nested phase 79: currency and smoke
placement/stamping, enemy shade subtraction and final visibility testing.
With the effect-stage scopes below enabled, these producers have exclusive
child phases; include 126–132 when comparing against older phase-79 totals.
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

`Collision consumers` reports four independent operations: direct native
cached-bounds reads (including explicit `nativeActorBounds`), collision-pair
collection, mixed dirty-bound flushes and native object-query preparation.
These are entry counts, including failed calls, not timings; a pair pass may
also perform a dirty flush and direct reads. They compile away outside PROBES.
Like the other gameplay counters, they reset after cold gameplay setup.
Zero tree queries alone does not establish that cached geometry is unused.
Zero consumer counts establish only the measured live workload, not cold
construction, unvisited events or later areas. Use source call-path checks and
explicit admission guards before removing state needed by these consumers.

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

Effect phases 126–133 split controller composition into scratch clearing,
currency, smoke, debris, unconditional cover, shade subtraction, visibility,
and native publication respectively. Producer phases include enumeration,
placement arithmetic and masked word composition, not just bitmap writes.
Publication includes the pending-blitter drain and row-damage bookkeeping.
The subsequent restoration of damaged background rows remains in phase 15
(game copy); it is not charged to the effect that caused the damage. Compare
these phases with projectile queries and instance births before attributing a
firing/destruction slowdown to sprite drawing. These scopes compile away in
ordinary builds and retain painter order and the offline asset representation.
`REFERENCE_EFFECT_COMPOSITION=1` retains separate-plane effect stamps and
publication. The default surface uses interleaved alpha/low/high rows and
offline repeated-mask smoke/gem assets for one masked blit per stamp.
It still owns a complete alpha raster: overlap and visibility queries do not
replay producer commands. Uncommon masks retain the scalar or separate-plane
fallback with the same row stride. `make test-effect-stamps` checks assets and
identity admission; `make verify-palette-effects` checks both storage layouts.
The native BOB oracle compares DMA composition and completed publication
against scalar pixels, and reference/candidate frame dumps check live play.
Phases 134–136 further separate `drawFx` origin visibility/relative placement,
raster rounding plus sprite-origin subtraction, and masked surface-word
stamping. These are exclusive children of the producer phases. Placement
covers currency/smoke/debris `drawFx`, not the different shade/cover placement
rules; raster and stamping cover all producers. The remaining currency phase
includes enumeration, frame selection and conversion of published integer
coordinates to the portable representation. Sum children when comparing with
older producer totals; none of these phase counts is an effect-instance count.
Before selecting another compositor trace window, run
`make test-amiga-currency-promotion`. Its source-backed contact replay builds
the actual native actor prefix and reports hardware, empty and raster-fallback
frames, plus smoke/debris, entry-cover and channel-overflow coverage. Run
`./build/amiga-currency-promotion-test --census` for per-frame CSV rows followed
by the verification summary. Raster fallback is not itself evidence of shade
subtraction: establish that producer/mask intersection separately. This is a
coverage check, not a timing measurement or a dense-encounter substitute.
The replay keeps its source/runtime state comparisons enabled and compares
the raster remainder plus promoted currency suffix with the complete scalar
surface, independently of the earlier actor commands. Hardware counts include
partial frames; the separate partial count identifies those that also retain
a raster prefix. Raster-only frames exclude that partial count.

Currency admission inspects the source-topmost end first. A wholly unshaded
pass uses the actor compositor, with BOBs for hardware-channel overflow. When
shade interrupts admission, only the channel-fitting topmost suffix escapes;
it and the retained shade-aware raster preserve the controller boundary.
`REFERENCE_CURRENCY_BOBS=1` retains the channel-limited raster fallback.
The suffix is published in original
Draw order above that raster and the ordinary actors. Intersection tests read
the existing packed alpha masks in clipped word slices, without an overlap
asset bank or a temporary mask. `REFERENCE_SHADE_PARTITION=1` on the Amiga
retains the earlier whole-surface shade rejection and pixel-scanning admission.
The host target tests both selectors, checks packed intersections against
scalar pixels (including row padding and viewport edges), and requires natural
shaded frames with a promoted suffix. Compare native `frames` captures with
this selector as well: host composition alone does not prove sprite DMA.

For a natural channel-overflow and shade-subtraction workload, run
`./build/amiga-currency-promotion-test --overflow --census`. This uses ordinary
startup seed 87 and encounter input route 34, not injected currency or changed
RNG state. `make test-amiga-currency-promotion` runs both tapes. Reproduce its
source replay independently with
`DOWNWELL_ENCOUNTER_PROFILE_SEARCH=1 DOWNWELL_ENCOUNTER_CASE=87:34 make verify-contact-profile`.
The shared route helper is checked against the search's original button
formula, including entrance boundaries. CSV rows include required sprite parts,
death state and actual shade pixel changes: the latter compares the unshaded
currency image with the scalar compositor on live gem-only frames, rather than
inferring subtraction from a failed promotion. Reasons can overlap. This host
fixture verifies the promoted suffix and unchanged actor prefix on fallback;
it does not verify native fallback DMA pixels or time its composition. It is
not evidence of smoke/debris coverage or complete dense-combat coverage.
The search's `max_visible_currency` excludes disappearance-flash frames;
`max_currency` counts live centres and cannot establish sprite overflow.

Use `SKIP_TO_GAME=1 PROFILE_OVERFLOW=1 VERIFY_TIMEOUT_SECONDS=600 bash amiga/profile.sh fps`
for its native 1,800-update window. The wrapper first runs both host renderer
tapes and verifies the seed-87 entry checkpoint (including all RNG words), then
restores the ordinary executable on exit. `PROFILE_OVERFLOW=1` also selects
this tape for `trace` and `frames`; do not combine it with another tape flag.
For example, capture all-raster shade subtraction (1233–1242), a shaded
prefix with an unshaded sprite suffix (1493–1502), and nine-channel overflow
(frames 1523–1528, after two eight-part frames) separately:

```sh
SKIP_TO_GAME=1 PROFILE_OVERFLOW=1 TRACE_FRAMES=1232,1492,1522 TRACE_FIELDS=40 \
  TRACE_OUTPUT="$PWD/tmp/trace/overflow" VERIFY_TIMEOUT_SECONDS=600 \
  bash amiga/profile.sh trace
```

The host census uses one-based post-Step frames, as do the trace frame labels.
Native timing runs retain the ordinary uninstrumented compositor; host
coverage is not a replacement for native state and completed-DMA pixel checks.

`./build/amiga-currency-promotion-test --overflow --shade-census` isolates each
visible gem and applies the source shade pass. `SHADE` rows distinguish wholly
hidden, partly erased and unaffected producers, with summed opaque-pixel counts
before and after subtraction. Counts include overlapping producers separately;
they are not final framebuffer occupancy or a runtime culling implementation.
Use them before proposing removal of hidden producers, alongside native costs.

For source-backed currency pixels, generate a witness with
`./build/amiga-currency-promotion-test --overflow --pixel-witness > tmp/currency-pixels.txt`.
Capture ticks `1240,1500,1523,1524,1525,1526,1527,1528` with
`PROFILE_OVERFLOW=1`, `DUMP_TICKS` and `profile.sh frames`, then run
`python3 tools/verify-amiga-currency-frames.py tmp/currency-frames tmp/currency-pixels.txt
--reference tmp/currency-reference-frames` (on one command line). The reference
directory must contain matching native captures with `REFERENCE_CURRENCY_BOBS=1`.
The verifier checks opaque source-compositor pixels and, with the reference,
every visible RGB pixel, including transparent holes and ordinary actors.
Optional `DUMP_DETAILS=1` saves the submitted BOB records and resident artwork
bank alongside native frame dumps for diagnosing publication mismatches.

For generation-publication structure rather than effect coverage, run
`./build/amiga-currency-promotion-test --generation-census` (also accepts
`--overflow`). Each `GENERATION` row describes a newly emitted fragment on
the same source-checked replay: resource family/index, orientation, fixed
terrain segments, event cells, unused RNG draws, terrain creations and paired
occupancy words. `groups` joins only consecutive fixed segments across row
ends; every event cell and pillar builder-position boundary ends a group.
`segments - groups` is an upper bound on removable row-boundary callbacks,
not evidence that a multi-row admission preserves capacity/failure prefixes.
The census does not change emission, advance RNG, or time native execution.
Combine its counts with the matching frame's folded native stacks before
proposing a publication change; do not treat parser/census work as a frame win.

`bash amiga/profile.sh overflow-state` (180-second emulator limit) checks five
post-Step observation digests on every one of the 1,800 native updates. The
wrapper generates expected observations from the host replay with its existing
decoded-source comparisons enabled. A mismatch stops at its first frame/group;
the groups cover player/camera, all 16 RNG words and position in the stream,
enemy publication state, currency motion/animation/lifetimes, and published
live bullets/damage numbers/time fields. Bullet slot holes and destroy-marked
enemy records awaiting Draw cleanup are excluded; surviving order and lifetime
keys are compared. Inactive surviving enemies remain included. The host fixture
compacts destroyed enemies at End Step, so its physical records cannot be
compared directly with the native pre-cleanup array. On an enemy-group mismatch,
the debugger prints the observed fields; compare with the host test's
`--enemy-witness` output at the same frame. Fields are explicitly serialized, never
raw struct bytes. The generator records input, source and executable hashes.
This is a correctness-only build: digest computation and the expected table
are absent from FPS/trace builds. Its timings must not be reported as game FPS.

These observations are not full runner-state coverage: they do not include
terrain records, global source instance IDs, all private enemy-family state or
the native ownership/event-order lists. Local lifetime keys are not source IDs.
Keep the ownership replays, source comparisons and completed-DMA pixel checks;
do not replace them with this cross-target witness.
The native overflow checkpoint uses released entry input, matching the host
fixture. The first route button is a gameplay input, not a held style-confirm
button; ordinary interactive scene entry retains its actual held-button state.

Phases 137–138 split enemy finalization's integer position publication and
sprite/mask/scale/image-index publication from phase 115. The latter includes
bat presentation-field updates. Coordinate rounding, sprite selection and
crawler angle publication remain in 115. These brackets preserve setter order
and include validation/ownership work below each callback; they are not render
timings and do not imply that unchanged setters can be skipped safely.

Record the executable hash, revision, emulator CPU/memory configuration,
build flags, tape length, field count, worst gaps and probe clock-clamp count.
Averages alone do not establish the 20 ms requirement. Report scene preparation
separately from steady-state updates and distinguish waits from useful work.

## Pair timing with correctness

`REFERENCE_HUD_COMPOSITION=1` retains runtime digit-column layer composition
inside the prepared HUD renderer. The default reads offline pair columns;
dirty digit identities, background ownership and framebuffer publication are
unchanged. This is separate from `REFERENCE_HUD_WORDS=1` (the older staged
compositor) and `REFERENCE_HUD_TEXT=1` (the layered renderer).
The host generator proves that each word's left byte depends only on its
tail/middle digits and its right byte on middle/head, for all 3,993
digit/blank triples and alignment cases. It preserves per-digit currency
layer order and whole-string streak precedence. `make test-amiga-prepared-hud`
compares complete layers against source-ordered rendering, including resets,
external-overlay invalidation and digit-count changes. Pair timing at frames
488 and 660 with completed-DMA native dumps through 640–900 and require zero
prepared HUD fallbacks. Profiling restoration unsets the reference selector.

`bash amiga/profile.sh draw-cache` is a diagnostic, not a timing comparison.
It starts directly in gameplay and defaults to the contact tape, counting
cache hits, identical refills and changed actor lists. The default native path
refills from a mutation-maintained Draw projection: its revision changes are
reported as layer invalidations, with no generic capture invalidations.
`REFERENCE_DRAW_LIST=1` retains the original live-layer refills and their
capture/layer revision counters. Buckets cover the full tape, frames
645/701/844, early gameplay and contact. Each refill compares its
kind/handle/depth sequence with the preceding sequence before replacing it.

`bash amiga/profile.sh draw-order` runs the same bounded gameplay tape with
the existing live-layer versus cached Draw-order oracle on every frame,
without forcing actors into planar rendering. Both modes restore the normal
build, and their instrumentation is absent from ordinary builds. Use the
same build as the timing run. With `REFERENCE_DRAW_LIST=1`,
`LOCAL_DRAW_CAPTURE=1/2` selects global/local (default) invalidation; it does
not select a different mutation-maintained implementation. Do not change
selectors during a run. `make verify-local-draw-capture` exercises host
lifecycle and ordering tests, including the prepared projection's activation,
depth service, retirement, reuse and interrupted callback cases.

`REFERENCE_VALIDATION=1` retains recursive child validity polling and
unconditional phase reconciliation/service for comparison with native
mutation-propagated failure and pending-work checks. Pair the same contact
tape and trace windows; charge mutation costs as well as phase costs. The
ordinary typed identity checks and drone callback status remain live in both
paths. Profiling restores the normal default after either success or failure.

`REFERENCE_ANIMATION=1` retains the generic per-actor snapshot/decode/lifetime
adapter. The native default consumes consecutive enemy/bullet family runs
directly from the active room links. Pair the contact tape, including births
and generation frames; no additional room projection is prepared each frame.
The selector does not alter animation arithmetic or room order.

`REFERENCE_GENERATED_BOXES=1` retains synchronous breakable creation through
the deferred command and collection/world adapters. The native default emits
only the selected Create values directly to terrain, tile and family owners.
Pair generation frames on the contact tape; `REFERENCE_BREAKABLES=1` is the
separate pool-allocation oracle. Both selectors are unset when profiling
restores the normal build.

`REFERENCE_INITIAL_TERRAIN=1` retains runtime composition of pristine room
terrain. The default borrows a host-prepared row dictionary at a fresh
room-load boundary, before generated terrain,
decorations or shops exist. The bank owns pixels, not instances; source Create
effects and RNG remain at entry. Invalid banks and non-pristine terrain use
the retained painter. `make generate-initial-terrain` compares every generated
row with the scalar compositor and records input/tool hashes in the ignored
manifest. `bash amiga/profile.sh terrain-shop` checks native pixels on both
entries of the source-checked natural shop tape. Pair `profile.sh shop` with
the reference selector for uninstrumented timing, and `PROBES=1` for entry
phase attribution. Restoration unsets the selector.

DWIT v2 additionally contains sixteen periodic corridor rows (704 bytes of
ordinary-memory artwork, no Chip allocation). Dirty central-well spans may
borrow them only after checking the live wall neighbourhood, including
offscreen neighbours which select the border frames, and excluding tile,
decoration, shop and retained wall-presentation producers. This eliminates
composition and mutable-raster writes, not the final circular-buffer transfer.
Geometry, lifetimes and RNG are never prepared ahead of their source events.
Invalidation leaves borrowed pixels dirty until re-admitted or repainted;
both CPU composition and Chip readback retire the borrowed row representation.
Version 1 banks remain readable and take the retained painter for corridors.
The generator checks all sixteen phase offsets against scalar pixels and
sabotages wall neighbours/platforms; `test-amiga-terrain-raster` covers mixed
borrowed/modified rows, ring wrapping, publication debts and room re-entry.

`REFERENCE_INITIAL_RASTER=1` retains initial terrain preparation through
scratch Chip pixels followed by readback into the retained Fast raster. The
default uses the prepared bank when eligible, otherwise the runtime
direct-cache painter, with the same fallback for unsupported artwork.
This selector bypasses both optimizations. Pair `profile.sh entry` to measure this cold work; the
ordinary contact FPS tape excludes initialization. Check initial spans with
`PROFILE_ENTRY=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh terrain`:
the input-only entry tape runs with exact terrain pixel comparisons enabled.
The reference selector is unset when profiling restores the normal build.

`REFERENCE_INITIAL_RASTER_STORAGE=1` keeps the prepared bank but expands it
into mutable Fast raster storage at entry. The default borrows immutable
dictionary rows until they are repainted. Compare both sides of the handoff
with `profile.sh entry`, then early descent transfers with the contact tape;
the entry saving must include the extra dictionary-gather cost in backing
setup. `test-amiga-terrain-raster` covers borrowed-row replacement, re-entry,
ring wrap and pipelined display copies. Native `terrain` and `terrain-shop`
oracles compare the borrowed pixels with the retained painter. The selector
is unset during normal-build restoration.

`REFERENCE_ROOM_PREPARE=1` retains repeated serialized room lookup and generic
placeholder-to-lifetime binding at room entry. The default admits immutable
room metadata before display, then publishes the same layers, reservations and
backgrounds in source order. A single empty-lifetime check proves that no
survivor can bind to a placeholder. The bounded plan uses 4,368 bytes of ordinary
RAM, no Chip RAM; malformed or oversized plans use the generic loader and its
failure prefix. The room pack must remain immutable. Validate with
`make verify-prepared-empty-room`, cross replay and native scene checks; pair
`bash amiga/profile.sh shop` and `PROBES=1 bash amiga/profile.sh shop` with the
reference selector for reentry timing. Profiling restoration unsets it.

`REFERENCE_FIELD_BANK=1` forces field-image/arc uploads on every gameplay
entry. The default retains those immutable Chip ranges across upgrade and
level reentry, invalidating them before a boot/front bank load. Both paths
retain synchronous completion for short/skipped menus; the default stages
arc images during the garden and small images after garden retirement.
`profile.sh entry` compares the resident small-image bank byte for byte with
the offline pack after natural entry. Upload completion does not establish
bound-bank residency or skip large-field plan initialization. Both paths
discard per-ring overlays and rebind their underlays. Pair the natural shop
tape for field-binding phase and full-frame timings; `SHOP_FIELD_BANK`
reports the two entries as uploads/reuses. Use `make verify-prepared-time-fields`
and `bash amiga/profile.sh fields-clear` for image, overwrite-invalidation
and reentry pixel checks. The selector is unset on profiling restoration.

`REFERENCE_FIELD_ADMISSION=1` retains the repeated small-image source
validation at bind, independently of staging and residency. Normally a
successful staging admission certifies the immutable source pointer and exact
byte count until `invalidateBank()`. Binding still checks destination capacity;
an incomplete upload or changed destination still copies the complete bank.
A different source/extent is validated normally. Source bytes must not change
while admitted. The host prepared-field test covers revoked admission and
changed inputs; the native entry probe verifies all resident bank bytes.
Profiling restoration unsets this reference selector too.

The offline mirrored HUD border exposes its zero-filled DMA row padding as
its logical width, without changing its anchor or stored pixels. This avoids
the Bitmap class's partial-word edge preservation when drawing the tall right
border. `REFERENCE_HUD_BORDER_EXTENT=1` restores the original logical width
while retaining the same mirrored payload; compare `profile.sh entry` and
native frame dumps with this selector. Profiling restoration unsets it.
The renderer test compares both extents and the runtime-reflection oracle
over clipped positions and existing pixels, including zero alpha in padding.

`REFERENCE_EMPTY_TERRAIN=1` retains generic landing-index reconstruction for
an empty world. The default resets occupancy, landing candidates, contact
records and provenance directly when there are no room instances or pattern
fragments to reconstruct. Later room Creates still publish in source order;
populated-world reconstruction is unchanged. Compare `profile.sh shop` and
its `PROBES=1` world-entry phase, not steady-state gameplay, for this change.
`make verify-terrain-ownership` compares the reset with reconstruction after
dirty state, every landing query and subsequent Creates. Profiling restoration
unsets the selector.

`REFERENCE_ROOM_TERRAIN=1` retains serialized room-instance decoding at each
terrain handoff and raster-extent calculation. The default admits a bounded,
source-ordered resource plan before display; source views, player identity,
terrain Creates and their RNG effects are still published at the handoff.
The immutable room pack must outlive the plan. Invalid or oversized admission
falls back to the generic decoder, including partial failures and no-retry
semantics. The plan has 256 record slots, uses ordinary RAM only, and owns no
gameplay instances. Pair the natural shop tape and world/raster entry phases;
`make verify-terrain-ownership` checks resource fields/order, capacity rejection,
callback failure prefixes and full replay ownership. Restoration unsets it.

`bash amiga/profile.sh ammo` checks the factored gauge compositor on the
bounded contact tape, starting directly in gameplay. Every HUD update is
compared byte-for-byte with full source-order reconstruction. It requires at
least 100 prepared-path updates and no mismatches, then restores the normal
build. Do not use its instrumented timing as performance evidence;
`AMMO_FACTORED=1` selects the retained compositor for timing comparisons.

Preserve full RNG state, synchronous Create order, collision order and effect
ownership while optimizing. Re-run deterministic replay and fresh-versus-retained
render comparisons for the affected path, then check the restored build's
68000 instruction/helper audits.

Use separate dense-combat, horizontal-shop, HUD-change and selection-transition
cases. A passing fixed tape proves neither all-scene timing nor original-game
fidelity. Expanded-memory emulator runs do not establish stock A500 memory fit.
