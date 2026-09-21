# Downwell port working agreement

This repository is a behavior-first port of the Windows Steam release of
Downwell to an SDL host reference implementation and then to native Amiga
68020 code.

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
  68020 assembly twin. Keep the portable implementation as its oracle.

## Generated and copyrighted material

- Put bulk exports in `extracted/`, generated tables in `generated/`, captures
  in `captures/`, and disposable work in `tmp/`; all are ignored by Git.
- Commit tools, manifests without copyrighted payloads, hand-authored code,
  and documentation. Do not commit original or byte-for-byte derived assets.

## Builds

- Host: `make`, then `make run`.
- Amiga environment: `. amiga/env.sh`.
- Amiga scaffold/tool check: `make amiga` or `cd amiga && . ./env.sh && make`.

