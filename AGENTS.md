# Downwell port working agreement

This repository is a behavior-first port of the Windows Steam release of
Downwell to an SDL host reference implementation and then to native Amiga
code. The target machine is an Amiga 1200 with 4 MB Fast RAM (68EC020, AGA,
2 MB Chip, 4 MB Fast); the game is built for the 68020 and refuses a 68000
or 68010 at start. The game must start on any Amiga with a 68020 or later,
enough Chip memory for its DMA data and enough public memory (Chip or Fast)
for everything else.

## Commits

- Commit as Vesa Halttunen <vesuri@jormas.com>: no signing, no hooks, no
  co-author trailers.

## Shared environment and knowledge

Shared guidance starts at `~/.local/share/amiga/docs/index.md`: read the rows that match the
task. It routes to the playbooks (this project follows `docs/playbooks/new-port-playbook.md`),
the references (measurement, validation, 68000 optimisation, Amiga hardware, headless FS-UAE,
toolchain) and the shared tools and material (`docs/environment.md`; local notes:
`docs/toolchain.md`). The environment comes from AmigaXDev's `make setup`; `amiga/env.sh`
sources `~/.local/share/amiga/env.sh` and selects the arm64 FS-UAE. Generic lessons go into
AmigaXDev's docs (edit them there, then `make share`); this repo keeps what is specific to
Downwell.

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
  workload on the target machine, including its worst frame, with the
  retained reference. Correctness tests and added infrastructure are not
  performance wins. Revise
  or discard experiments that miss their practical target; a better average
  alone does not excuse worse gameplay stalls. Target <=20 ms during active
  gameplay. Judge scene handoffs by overall transition latency and visible
  stalls, not individual-frame limits: a few milliseconds or one or two fields
  of transition variation, including redistributed VBlank waiting, is not by
  itself a reason to reject a substantial overall improvement.
- Keep the shipping executable as small as possible: link only the code the
  shipped content uses, allocate only DMA data in Chip, and fit the whole
  footprint (code, data, BSS, banks, buffers) into the target machine.
  Debug, reference, verify and profile code belongs only in the development
  builds that need it, never in the shipping build.

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
