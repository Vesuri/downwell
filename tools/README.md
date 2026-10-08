# Tools

This directory is for reproducible, read-only GameMaker extraction, IR
normalization, asset conversion, replay generation, and state comparison.

Every extractor must:

1. Refuse to write under `data/`.
2. Verify or at least report the input hashes.
3. Record its own version/commit and schema version.
4. Preserve resource IDs and raw VM assembly.
5. Write bulk output only under `extracted/` or `generated/`.

## UndertaleModTool

`bootstrap-utmt.sh` downloads the pinned, self-contained Apple-silicon CLI for
UndertaleModTool 0.9.2.0 into ignored `tools/vendor/`. It verifies the official
release SHA-256 before extracting or executing it:

```sh
tools/bootstrap-utmt.sh
```

## Reproducible extraction

`extract.sh` first verifies the exact `data.win` SHA-256, then exports:

- UndertaleModTool's parsed build information;
- decoded PNGs for every embedded texture page;
- all root decompiled GML entries and strings;
- exact VM assembly for every root code entry;
- a schema-3 JSON manifest containing resource IDs, symbols, authoritative room
  and instance-creation order, object/event bindings, room instances/views/layers,
  layer membership, object/tile instance allocator boundaries, sprite atlas links,
  sound groups, font metrics, constants, code relationships, variables, and
  functions.

It writes only to ignored `extracted/` and `generated/` directories:

```sh
tools/extract.sh
```

`verify-extraction.sh` checks the immutable input hash, byte-identical parser
round trip, manifest counts/ID ranges/references, export counts, and reported
decompiler/disassembler failures. Run it directly or with:

```sh
make verify-extraction
```

## Native room pack

`generate-room-pack.py` lowers schema-3 room data into the deterministic `DWRM`
format documented in `docs/room-pack.md`. The copyrighted payload is generated
locally under ignored `generated/`; only the generator and format are committed.

`generate-builder-pattern-pack.py` lowers recovered level-fragment strings into
the deterministic `DWBP` format documented in `docs/builder-pattern-pack.md`.
It verifies the pinned Steam build through the extraction manifest and keeps
the derived cell payload under ignored `generated/`.

## Prepared Amiga ammo dividers

`make generated/amiga-ammo-dividers.h` composes resource 421's ordered divider
draws for capacities 0–16 and each ammo value. The ignored generated header
contains a dictionary of eight-row planar blocks and per-image byte indices;
block zero is transparent. The renderer reads these blocks directly, without
runtime expansion or layered sprite composition. The companion JSON records
generator, source, extractor, input-pack and output hashes.

`make verify-prepared-ammo-dividers` compares every composition with the
portable binary64 draw sequence and runs the complete HUD pixel suite.
The Amiga comparison switch is `PREPARED_AMMO_DIVIDERS=1`; zero retains the
original compositor. Changed source artwork and capacities above 16 use that
compositor even when the switch is enabled.

`make verify-ammo-segment-probe` measures a host-precomposed gauge segment bank
and checks its final words against ordered original drawing. At a fixed capacity,
rows between adjacent charge thresholds share a target history; equal initial
states remain equal. A segment needs its own and the preceding segment's frame
because the artwork overlaps by eight rows. A capacity change requires a fresh
uniformity check: arbitrary inherited row state is not covered by that proof.
The ignored header and manifest include capacities 1–16, all charge/meter/frame
combinations, deduplicated run templates, memory costs and source/tool hashes.
This is host-only preparation for further work, **not an enabled optimization**:
the interior-only native experiment failed its worst-frame timing gate. See
[the audit](../docs/amiga-frame-cost.md#prepared-gauge-segments).

`make verify-ammo-complete-probe` extends that feasibility check to the gauge's
caps, red rim, background and single-digit number at every supported offset.
The separate C++ witness uses the original ordered renderer and compares
whole-region pixel digests for deterministic row-animation histories. It emits
ignored bank data and a provenance/cost manifest, **not native assets**. Its
naive combined bank costs 268–435 KiB, so neither measured layout is suitable
for adoption; independent shell/number and column states need factoring.
See the [complete-bank budget](../docs/amiga-frame-cost.md#complete-gauge-representation-budget).

`tools/probe-ammo-complete.py --factored` generates the experimental complete
column, independent outer-rim patches, and labels into ignored files. Exact
source metadata/bytes qualify its native use. `make verify-factored-ammo-hud`
checks full HUD pixels and fallback transitions; `bash amiga/profile.sh ammo`
runs a separate original-renderer oracle on the A500. The generator additionally
proves low-plane independence from row animation and budgets a future sparse
representation. `AMMO_FACTORED=1/2` compares the retained/factored implementation;
zero is the normal default. The recharge and full-pacing gates remain open;
see the [native evidence](../docs/amiga-frame-cost.md#factored-gauge-pixel-correct-experiment-pacing-gate-open).

## Prepared Amiga field images

`make generated/amiga-field-images.bin` rasterizes all 512 small-circle
radius phases with the retained portable geometry. It deduplicates them into
21 immutable 64×56 low/high/mask images. The 29,264-byte ignored pack starts
with a 16-byte big-endian header and 512 two-byte image indices. Its JSON
manifest pins the generator, geometry inputs and output hash. This verifies
the portable raster, not equivalence to the original Windows GPU raster.

`make verify-prepared-time-fields` exercises independent pixel copies against
the retained sparse overlay, including clipping, double buffering, wrap,
target replacement, pending draws and fallback transitions. The native
comparison switch is `PREPARED_FIELDS=1`; `PREPARED_FIELDS_BACKING=1` admits
multiple small circles using the pristine terrain underlay, and
`PREPARED_FIELDS_BACKING=2` additionally admits large/mixed circles.
`bash amiga/profile.sh fields` checks native backing capture
and masked publication against a full-buffer pixel oracle; its timings are
not performance measurements. Use the normal `fps`/`trace` modes for timing.
With `PREPARED_FIELDS=1`, `PROFILE_FIELD_REFERENCE=1` selects the compiled
sparse oracle by changing only the initial selector data. The profile prints
the executable text-section hash so code identity can be checked between
the two builds; both reserve and load the same scene-bank images.

`make generated/downwell-field-columns.h` prepares the 512 large-circle radius
phases as deduplicated columns of planar words and destination offsets. The
73,024-byte bank avoids runtime row expansion and whole offscreen-column
traversal; horizontal shifting, viewport clipping and ring wrapping are still
performed when consuming it. Its manifest pins the generator, portable
geometry inputs and output. `make verify-field-columns` checks all radius and
horizontal-shift phases against independent pixels and physical-row footprints;
`verify-prepared-time-fields` checks integration and mixed source order. These
are portable-raster checks, not claims of Windows GPU raster equivalence.

`make analyze-field-layouts` reports unchanged-column/word opportunities and
deduplicated bitmap-band storage/counts for those large rasters. Its uniform
radius-pair model is a geometry/storage audit, not a replacement for native
timing or a measured distribution of gameplay inputs. See the frame-cost
audit before choosing another large-field representation.

## Prepared level-clear motion

`make generated/clear-motion.inc` builds the host generator and emits the
canonical Result-room player/camera easing sequence. The generator compiles
the retained arithmetic path and checks each state against host binary64.
The `.json` manifest records generator version, extraction version, source
hashes and output hash. Payload and manifest stay under ignored `generated/`.
`make test-level-clear-motion` checks canonical, mutated and noncanonical
states against independent binary64 calculations. `REFERENCE_CLEAR_MOTION=1`
retains arithmetic throughout on either target; perform a clean rebuild when
comparing test executables built with different reference flags.

## Prepared moving damage text

`make generated/amiga-damage-text.h` composes the common `-0` through `-9`
labels from the extracted font glyphs on the host. The immutable bank holds
the final cardinal-outline mask and ink words in ordinary memory, not Chip
RAM. Its companion JSON pins extraction/generator versions, derived resource
IDs and input/output hashes; both generated files remain ignored.

`make test-amiga-damage-text` compares complete framebuffers against the
five-pass glyph renderer, including every horizontal shift, clipping, ring
wrap, uncommon signed integers and changed-font fallback. The bank is admitted
only when its glyphs match the bound font. Other integers retain the runtime
cache; replacing the font requires invalidation/rebinding.
`REFERENCE_DAMAGE_BANK=1` in the Amiga build retains the old runtime cache for
matched timing/pixel comparisons. `REFERENCE_DAMAGE_TEXT=1` retains the older
uncached glyph compositor. The profiling script clears both when restoring.

## Prepared health-HUD artwork

`make generated/amiga-health-art.h` prepares the background and divider/rim
layers for the original four-piece heart meter with the retained renderer.
The native HUD borrows these ordinary-memory words instead of drawing scratch
layers into Chip RAM and reading them back at entry. Mutable fills, text and
changed-rectangle publication are unchanged. Other divider counts retain the
runtime path. Binding compares the exact source directory records and payloads;
changed packs fall back, and immutable pack replacement requires rebinding.

`make test-amiga-health-art` checks complete HUD pixels, including uncommon
capacities/dividers, source mutation, rebinding and surrounding pixels, and
checks generator reproducibility. The ignored JSON manifest records source
hashes, resource IDs and extraction/generator versions. Amiga
`REFERENCE_HEALTH_ART=1` retains scratch rendering for matched measurements;
the profiling script clears it when restoring the normal executable.

For the native embedded HUD, a SHA-256-named symbol marks the exact `.incbin`
payload used to generate the artwork. The immutable resident-copy owner binds
that identity after its byte copy (or residency reuse), avoiding another scan
of Chip RAM. A stale artwork object cannot link to a different payload identity.
Generic packs still use exact byte admission. `make -C amiga verify-health-link`
in the Amiga environment verifies both matching and mismatched object pairs.
`REFERENCE_HEALTH_ADMISSION=1` retains the resident-copy byte scan for timing
and pixel comparisons; profiling restoration clears this selector as well.

## Original audio

After the UTMT extraction, run `python3 tools/extract-audio.py` to extract all
361 sound resources from `data.win`, `audiogroup1-6.dat`, and the three loose Ogg
files into `extracted/audio/`. Original `bgm` resources go into `music/`; other
sounds go into `sound-effects/`. Filenames retain the numeric sound resource ID
and resource name. WAV and Ogg payloads are copied without re-encoding.

The extractor verifies the resource metadata against the `data.win` hash,
checks AUDO boundaries and complete resource coverage, and writes
`extracted/audio/manifest.json` with source and output hashes, original sound
settings, group/audio IDs, byte offsets, and extractor versions.
