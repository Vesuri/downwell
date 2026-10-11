# Downwell port working agreement

This repository is a behavior-first port of the Windows Steam release of
Downwell to an SDL host reference implementation and then to native Amiga 500
68000 code.

## Shared environment and knowledge

The toolchain, emulators, Kickstarts, WHDLoad and the Python environment come from AmigaXDev
(`make setup` there); `amiga/env.sh` sources `~/.local/share/amiga/env.sh`. Lessons that hold
for any port live in AmigaXDev's docs: add or correct them there, and keep only what is
specific to this game in `docs/`.

| AmigaXDev doc | For |
| --- | --- |
| `docs/environment.md` | the shared variables and installed files |
| `docs/playbooks/new-port-playbook.md` | the phases of a port and their gates |
| `docs/reference/amiga-hardware.md` | Chip bus, takeover, interrupts, copper, blitter, Paula, AGA, the dA JoRMaS framework |
| `docs/reference/m68k-optimisation.md` | 68000/68020 cost model and code generation |
| `docs/reference/measurement-method.md` | profiling instruments, A/B discipline, pricing a change |
| `docs/reference/validation-harness.md` | faithfulness to the original: references, differentials, sabotage |
| `docs/reference/headless-fsuae.md` | the shared launcher, FS-UAE builds, gdb stub, diag runs |
| `docs/reference/port-toolchain.md` | flags, linking, link audits, WHDLoad slaves, release archives |
| `docs/native-port-validation.md` | crash dumps as regressions, installer updates, release archive checks |

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
  or discard experiments that miss their practical target; a better average
  alone does not excuse worse gameplay stalls. Target <=20 ms during active
  gameplay. Judge scene handoffs by overall transition latency and visible
  stalls, not individual-frame limits: a few milliseconds or one or two fields
  of transition variation, including redistributed VBlank waiting, is not by
  itself a reason to reject a substantial overall improvement.

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
