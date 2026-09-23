# Amiga performance instrumentation

Run from the repository root:

```
bash amiga/profile.sh fps
bash amiga/profile.sh probes
```

Each command clean-builds its own configuration, drives the same deterministic
garden input (40 idle frames, then right), and freezes after 180 completed
garden iterations. GDB stops only once, after that window; it never samples
with conditional breakpoints inside the measurement. The emulator has a
60-second host safety timeout, overridable with `VERIFY_TIMEOUT_SECONDS`.
Timeout and success both clean up the emulator; the wrapper restores a normal
build on exit. Reports remain in `amiga/.run/profile-fps.log` and
`amiga/.run/profile-probes.log` (ignored).

## Two distinct modes

`FPSCOUNT=1` counts completed loop iterations and elapsed real VERTB fields,
plus the worst per-iteration field gap, per scene. The garden publishes one
frame per loop. FPS is `50 * completed garden frames / elapsed PAL fields`.
This is an emulated-machine measurement, not emulator host throughput. The
profiling launcher uses A500 CPU settings and the existing 512 KB Chip/8 MB
Fast configuration; this is not a claim about stock-A500 memory requirements.

`PROBES=1` adds exclusive phase totals, call counts, maximum bracket lengths
and maximum whole-loop time. These numbers include instrumentation overhead;
do not quote their frame rate as production FPS. They follow the Revs
beam-epoch scheme: VERTB advances an epoch by `313 * 256` ticks, and a stable
VPOSR/VHPOSR sample supplies the position within the field. Retry interrupted
samples and vertical-high-bit transitions; clamp backward samples in the
beam-wrap/interrupt-latency interval and count those clamps. Conversion is
nominal PAL time: `ticks * 20 / 80128` milliseconds. It is a beam timing
estimate, not an exact CPU-cycle counter, and includes interrupts/DMA stalls.

Phases are other, VBlank waiting, input, simulation, title updates, sprite
selection/position, scrolling calculations, copper updates and publication.
Phase totals are exclusive, not nested. `max_bracket_ms` is a single bracket,
not a whole frame's aggregate when a phase is visited more than once. The
RAII frame boundary closes measurements on `continue` paths as well. Small
loop-boundary/profiler bookkeeping outside that boundary is not in the phase
totals. Timing counters freeze at the completed-frame boundary, while the
underlying VBI clock may continue; the report uses the frozen scene counters.

Garden stages have detailed brackets. Other scenes currently have wait,
input/simulation and an undifferentiated remainder; startup asset loading
before the main loop is not yet bracketed. Neither instrumentation mode is
enabled in normal builds. Verification builds are rejected in combination
with profiling: their reference rendering must not pollute performance data.

## First measurement

The detailed 180-frame garden run after introducing these helpers reported
180 PAL fields, no clock clamps, approximately 16.306 ms/frame waiting and
3.504 ms/frame in the remaining measured phases. Largest average work phases:
sprite selection/header updates 0.782 ms, simulation 0.653 ms, copper 0.499 ms,
scroll calculations 0.490 ms. These are instrumented results, not budgets or
guarantees for every game scene. Title changes and first-use copper construction
produce spikes that the maximum columns retain.

The separate lightweight run reported 180 frames in 180 PAL fields: 50.00 FPS,
with a maximum one-field iteration gap. This establishes the sampled garden
window only, not the whole game's frame rate.
