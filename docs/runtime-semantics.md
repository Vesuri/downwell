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
- reaching zero returns the alarm in a dispatch bitmask and leaves the visible
  value at zero for that event;
- an unrearmed zero becomes `-1` on the following step;
- a handler may immediately rearm the same slot;
- assigning zero does not dispatch an event and becomes `-1` next step;
- slots without corresponding Alarm events do not count down.

`StartupRuntime` now uses this primitive for the recovered splash cadence. Its
existing tick-731 and golden-state-digest tests remain unchanged, which guards
the refactor against behavior drift.

The exact position of alarm dispatch relative to Begin Step, input, Step,
collision, and End Step is now represented through `EventScheduler`: all Begin
Step handlers, alarms, all Step handlers, then all End Step handlers. Movement
and collision phases have not yet been added.

These rules and the Begin Step → alarms → Step placement are documented by the
[GameMaker alarm reference](https://manual.gamemaker.io/monthly/en/GameMaker_Language/GML_Reference/Asset_Management/Instances/Instance_Variables/alarm.htm)
and [event-order reference](https://manual.gamemaker.io/lts/en/The_Asset_Editors/Object_Properties/Event_Order.htm).

## Step dispatch

`EventScheduler` is allocation-free and operates on stable instance IDs rather
than keeping pointers across callbacks. Each phase snapshots currently active
IDs, skips instances deleted before their turn, and allows instances created in
an earlier phase to join later phases in the same tick. Alarm masks are supplied
per object, so nonexistent Alarm events neither count down nor dispatch.

The engine-defined phase order is documented. Ordering of instances within one
phase is explicitly not guaranteed by GameMaker and can differ by runner or
target. The portable runtime currently chooses stable creation order to remain
deterministic; this is a port policy, not yet a claim about the Windows runner.
Creation/deletion behavior across phase boundaries is similarly isolated behind
tests and remains subject to targeted Windows tracing if Downwell depends on it.

`StartupRuntime` now executes `objInitRm` Alarm 0 and End Step through this
dispatcher. Its automatic timing, replay, and golden state remain unchanged.

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
`objPlayer_n`. Core step and room-entry phases are implemented; movement,
collision, draw ordering, and the exact timing of deferred room changes remain.

## Instance storage

`InstanceStore` is a fixed-capacity, allocation-free container with stable
creation order and unique 32-bit instance IDs. Each record carries its object
and room IDs, transform/image state, creation-code references, resolved layer
ID/depth, persistence/active flags, and its own `AlarmBank`. Erasure and room
changes compact the array without reordering survivors.

The startup model uses a two-slot specialization populated from the recovered
`rmInit` records. Transitioning to `rmMenu` now removes instance 100066
(`objInitRm`) and preserves instance 100065 (`objControlerN`), including its
per-instance state. Loading the 192 serialized `rmMenu` instances is a separate
room-loader milestone; the startup store deliberately does not pretend they
already exist.

## Room entry

`RoomLoader` now implements the documented entry sequence over ordered room
definitions:

1. validate the complete transition for capacity and duplicate instance IDs;
2. retain persistent instances and discard source-room instances;
3. for each new instance in serialized order, dispatch PreCreate, Create, then
   instance creation code;
4. dispatch Game Start to all instances only for the first game room;
5. run room creation code once;
6. dispatch Room Start to persistent and newly created instances.

Validation is atomic: an invalid destination leaves the source store untouched.
The startup model now enters `rmInit` through this loader, so its controller and
transition-object initialization use the same event path intended for later
rooms. Its bounded transition still loads no `rmMenu` definitions; importing
those 192 records from generated resource tables remains the next room-data
step.

Room-entry callbacks currently may not create or destroy instances. That
behavior will use the same deferred command mechanism needed for safe mutation
during step/collision dispatch rather than being guessed inside the loader.
