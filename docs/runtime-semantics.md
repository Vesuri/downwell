# GameMaker runtime semantics ledger

This ledger separates behavior that is visible in recovered Downwell code from
runtime behavior supplied by GameMaker itself. The latter must eventually be
confirmed with a focused Windows trace or a bytecode/runtime reference test.

## Alarms

Downwell uses the twelve conventional per-instance alarm slots (`alarm[0]`
through `alarm[11]`; the recovered player object reaches slot 10). Recovered
events repeatedly rearm themselves by assigning a positive tick count from
inside an Alarm event.

The portable `AlarmBank` currently models the required runner behavior:

- twelve signed 32-bit slots with no allocation;
- reset value `-1` for every slot;
- positive values decrease once per simulation tick;
- reaching zero returns the alarm in a dispatch bitmask and makes the slot
  inactive before its handler runs;
- a handler may immediately rearm the same slot;
- zero and negative values do not count down.

`StartupRuntime` now uses this primitive for the recovered splash cadence. Its
existing tick-731 and golden-state-digest tests remain unchanged, which guards
the refactor against behavior drift.

The exact position of alarm dispatch relative to Begin Step, input, Step,
collision, and End Step remains an explicit open item for the general scheduler.
It must not be inferred merely from the fact that the startup slice passes.

## Instance and room order

For this GameMaker 2022.5 data format, UndertaleModLib exposes the serialized
room object list as creation order; the newer separate `InstanceIDList` is
absent. Manifest schema 3 records this encoding explicitly and verifies that
creation-order entries and layer memberships are exact permutations of each
room's instances.

Confirmed startup order is:

1. instance 100065, object 37, `objControlerN`;
2. instance 100066, object 36, `objInitRm`.

Confirmed `rmMenu` order begins with instance 104218, object 35,
`objPlayer_n`. Persistent-instance transfer and the timing of deferred room
changes are not yet implemented in the general scheduler.
