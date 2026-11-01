# Porting method

The generic method this port follows is in AmigaXDev's shared docs, installed under
`~/.local/share/amiga/docs/`: `playbooks/new-port-playbook.md`
(reference hierarchy, lossless seam, behaviour-first reimplementation, platform boundary),
`reference/validation-harness.md` (determinism, positive controls) and
`reference/measurement-method.md` (measuring a complete path before changing it).

Here the reference hierarchy is: the Windows build and its VM bytecode are the behavioural
authority, decompiled GML is a readable hypothesis, the SDL build is an executable model and the
Amiga build is the shipping target. Exact VM assembly and resource metadata are stored next to the
normalized output so decompiler errors, coercions and event-order assumptions stay auditable.
