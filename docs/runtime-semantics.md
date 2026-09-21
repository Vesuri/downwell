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

The host reference uses a provisional 2048-slot specialization sized to admit
`rmMain`'s serialized 244 instances plus procedural builder output. This is a
measurement aid, not the eventual Amiga memory budget; the target specialization
will be selected after instance lifetimes and deactivation are implemented.
With the generated room pack, `rmInit` is populated from recovered data.
Transitioning to `rmMenu` removes instance 100066 (`objInitRm`), preserves
instance 100065 (`objControlerN`), and creates all 192 serialized menu-room
instances with their recovered native fields. The smaller hard-coded `rmInit`
fixture remains only as a testable fallback when no pack is supplied.

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
The startup model now enters both `rmInit` and `rmMenu` through
`PackedRoomLoader` when a valid pack is supplied. Its controller and
transition-object initialization use the same event path intended for later
rooms, while the menu transition imports all recovered instance definitions.
Object-specific PreCreate, Create, instance-creation-code, Game Start, and Room
Start handlers beyond the two startup objects are not implemented yet; room
entry currently records those instances without executing their game logic.

## Windows random stream

The source-of-truth `Downwell.exe` runner implements its random stream as a
16-word WELL512 generator. `random_set_seed` expands the supplied 32-bit seed
with the Microsoft LCG constants `0x343fd` and `0x269ec3`, storing each upper
16-bit result. `random()` scales one WELL output by exactly 2^-32. The runner's
`irandom()` consumes two outputs, masks the second to 31 bits, joins them into a
positive 63-bit integer, and takes an inclusive-range modulus.

`GameMakerRandom` is the portable transcription of those runner routines. Its
tests pin raw WELL output plus `random` and `irandom` behavior. This is
intentionally Windows-runner compatibility: GameMaker documents that seeded
results can differ between target platforms.

The replay seed is treated as the value produced by the runner's startup
`randomize()` call. The translated path now consumes the shared stream for the
menu sleeping pose, decorative grass and trees, landing/footstep sample
selection, title particles (even though those particles are not drawn yet), and
the initial area-one side-room rows. The interactive SDL reference uses seed 2
when no replay supplies one, giving a stable development run.

When both native packs are supplied, lifecycle and step dispatch resolve the
object's recovered event binding (including inheritance) to a VM code-entry ID
before selecting a native handler. The four implemented `objInitRm`/
`objControlerN` handlers are selected by code IDs 865–867 and 869 rather than by
object ID. Every resolved but untranslated code call is counted explicitly;
the runtime does not silently treat broader menu behavior as implemented.

Room creation code IDs 648 (`rmInit`) and 714 (`rmMenu`) are translated as
deterministic core state. They establish the 160x284 camera, select sound 193
(`bgmOverworld`), emit gain/play requests, lock control, and enqueue creation of
object 382 (`ditherFade`). With the complete native packs, the command flushes
between room creation code and Room Start, producing instance 105186 from the
source-backed allocator boundary and running its recovered PreCreate/Create
bindings before it joins Room Start.

The translated `ditherFade` Create code freezes animation and arms Alarm 0 for
30 ticks. Its recovered Alarm 0 then sets image speed to 0.2 and clears the
global control lock. Its Draw event targets the off-screen `surfaceDissipate`;
it must not be tiled directly onto the application surface. Animation End
destruction and surface compositing remain untranslated.

`DeferredInstanceQueue` now provides the bounded, allocation-free command
primitive for that next step. It validates an entire create batch before
changing instance storage or the `GEN8.LastObj`-seeded allocator, preserves
command order, imports object persistence/depth, and invokes a lifecycle hook
for each new instance.

Room-entry callbacks currently may not create or destroy instances. That
behavior will use the same deferred command mechanism needed for safe mutation
during step/collision dispatch rather than being guessed inside the loader.
