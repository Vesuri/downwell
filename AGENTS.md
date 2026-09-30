# Downwell port working agreement

This repository is a behavior-first port of the Windows Steam release of
Downwell to an SDL host reference implementation and then to native Amiga 500
68000 code.

## Source of truth

- `data/` is the user's untouched Windows Steam build. Treat it as read-only.
- Never rewrite, patch, normalize, or delete anything under `data/`.
- GameMaker VM assembly and serialized resource metadata outrank decompiled
  GML when they disagree.
- Decompiled GML is an auditable view, not recovered original source.
- Keep numeric resource IDs stable and record extraction tool versions and
  input hashes in every generated manifest.

## Porting method

- Preserve behavior before simplifying GameMaker semantics.
- Shared game logic belongs in `src/core`; host integration belongs in
  `src/platform/sdl`; target-only code belongs in `src/platform/amiga`.
- Keep platform dependencies out of `src/core`.
- The SDL build is a development reference, not an independent authority.
  Validate uncertain behavior against the Windows build and VM bytecode.
- Prefer deterministic input/RNG replays and state checks over visual judgment.
- Establish a portable C/C++ implementation and evidence before adding a
  68000 assembly twin. Keep the portable implementation as its oracle.
- Prioritize measured architectural wins and eliminated work. For the frame
  budget, replace duplicated per-block terrain ownership before pursuing
  isolated arithmetic, copying, or compiler-level micro-optimizations. Keep
  preparatory changes directly tied to integrating that replacement.
- Before a performance change, identify the work it removes and estimate the
  saving from the native profile. Afterwards compare the same deterministic
  A500 workload, including its worst frame, with the retained reference.
  Correctness tests and added infrastructure are not performance wins. Revise
  or discard experiments that miss their target; a better average does not
  excuse a worse worst-case frame.

## Generated and copyrighted material

- Put bulk exports in `extracted/`, generated tables in `generated/`, captures
  in `captures/`, and disposable work in `tmp/`; all are ignored by Git.
- Commit tools, manifests without copyrighted payloads, hand-authored code,
  and documentation. Do not commit original or byte-for-byte derived assets.

## Work tracking

- Keep `docs/open-work.md` as the sole unfinished-work queue. Remove completed
  items and record implementation/test history in commits, not status reports.
- Keep technical contracts and source evidence in focused references without
  maintaining separate priority lists.

## Builds

- Host: `make`, then `make run`.
- Amiga environment: `. amiga/env.sh`.
- Amiga scaffold/tool check: `make amiga` or `cd amiga && . ./env.sh && make`.
