# Windows alarm phase

Read-only disassembly of `data/Downwell.exe`, SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`.
`tools/verify-runner-alarms.py` pins the reviewed regions; it is a change guard,
not execution of the original game or proof about other runner versions.

## Storage and phase evidence

- The `alarm` string is at `0x7fe358`; registration at `0x430c23` binds the
  getter wrapper `0x42c560` and setter wrapper `0x431260`.
- The underlying getter `0x469060` and setter `0x46bf80` access twelve signed
  32-bit values at instance offset `0xf4 + index*4`.
- The phase initializes EBX (alarm index) to zero at `0x509da8`. Within its
  per-index traversal it visits instance lists and filters instance flags and
  creation timestamps before touching an alarm (`0x509dc0`–`0x509e12`).
- At `0x509e15` it reads that alarm. Negative values skip decrement; otherwise
  it reads, decrements and writes back at `0x509e21`–`0x509e2b`. It reads again
  at `0x509e33` and dispatches only when the current value is zero.
- Dispatch at `0x509e42`–`0x509e47` supplies index EBX and event type 2 to
  `0x444910`, with the instance as both self and other. It returns to the
  traversal immediately, not to a deferred event-mask pass.
- After visiting the lists for that index, `0x509e71` increments EBX. The
  table cursor advances eight bytes from `0x93b544` to `0x93b5a4`, covering
  twelve slots in ascending order.

Consequently, all eligible instances' Alarm 0 processing precedes Alarm 1,
and so on. An event's write to a later alarm is seen during this same phase;
a write to an already-visited alarm waits until the next phase. Countdown of
zero retires it to -1 without dispatch. Negative values are left unchanged.
This audit does not yet establish every instance-list ordering or mutation
rule; those require further inspection before replacing the global owner.

## Skeleton consequence

Its Alarm 2 sets Alarm 3 to 15. Alarm 3 is then decremented to 14 in the same
phase, even if its old value was 1. After fourteen more alarm phases, Alarm 3
restores `imgSp = 0.3`. A snapshot of all countdown results before dispatch
would incorrectly fire the old Alarm 3 or delay the restart.

`SkeletonTiming::advanceAlarm(index)` supports the required per-slot immediate
dispatch. The future owner must iterate indices outside instances; calling
all four slots for one instance at a time is not generally equivalent.
Tests pin the overwritten-expiring-alarm case, restart interval and zero
retirement. The skeleton remains unregistered pending remaining behavior.

Existing `AlarmBank::advance()` is a snapshot API. This finding does not make
every existing use wrong, but cross-alarm event writes must be audited before
claiming complete event-order fidelity. The live enemy owner currently handles
alarms per enemy; new skeleton admission must not inherit that shortcut.
