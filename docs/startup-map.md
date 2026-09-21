# Recovered startup map

This is the first behavior slice selected for the SDL reference. Every name and
numeric ID below comes from `extracted/utmt-0.9.2.0/manifest.json`; behavior is
checked against both the decompiled GML and corresponding VM assembly.

## Authoritative room order

The physical `ROOM` resource table begins with seven YoYo/Steam example rooms,
so room-table order alone is misleading. `GEN8.RoomOrder` is authoritative and
starts with:

| Order | Room ID | Name |
| ---: | ---: | --- |
| 0 | 7 | `rmInit` |
| 1 | 8 | `rmPlayMenu` |
| 2 | 9 | `rmMenuBackup` |
| 3 | 10 | `rmDebug` |
| 4 | 11 | `rmMain` |

The remaining game rooms follow, with the Steam example rooms last. The host
runtime must use the serialized room-order list rather than assuming that room
ID zero is the entry room.

## Entry room

`rmInit` is room ID 7. It is 1000 by 3000 in room coordinates and contains two
instances:

| Instance ID | Object ID | Object | Position |
| ---: | ---: | --- | ---: |
| 100065 | 37 | `objControlerN` | -64, 80 |
| 100066 | 36 | `objInitRm` | -32, 32 |

Its room creation code is code ID 648, `gml_Room_rmInit_Create`. It sets camera
zero to 160 pixels wide by `global.g_cameraHeight`. On desktop,
`scrInitialize` establishes a 160x284 application surface and a 2x scale. This
distinction matters: 320x568 is a presentation size, not the native simulation
coordinate system.

## Persistent controller

Object ID 37, `objControlerN`, is persistent and has 31 event bindings. Its
Create event (code ID 869) performs the global bootstrap:

1. Increment `global.initTimes` and call `scrInitialize` (script/code ID 4).
2. Initialize HUD-local state through `scrDrawHudInitStuff` (ID 426).
3. Mark the Windows build as splash-enabled after platform detection.
4. Initialize pause/input bookkeeping.

`scrInitialize` is the main dependency fan-out. It establishes platform flags,
camera and window geometry, render surfaces, controls, save data, languages,
audio groups, unlocks, upgrades, player records and level curves. Its direct
project-script dependencies are:

| Script ID | Name |
| ---: | --- |
| 6 | `scrGlobalStuff` |
| 8 | `scrPlayerRecord` |
| 12 | `scrLoadSave` |
| 36 | `scrPlayerGlobalStat` |
| 86 | `scrControlInit` |
| 88 | `scrTouchButtonsCreation` |
| 112 | `soundInitialize` |
| 374 | `scrUpgrades` |
| 444 | `scrPlayerLevelCurve` |
| 450 | `bStatInitialize` |
| 544 | `scrShaderListInit` |
| 554 | `unlockInit` |
| 580 | `steamAchInit` |
| 586 | `langString` |

Steam calls such as `steam_update` and achievement login are platform services,
not missing GML scripts. The host runtime should initially implement them as
explicit deterministic adapters/no-ops, rather than allowing them to disappear
from the behavioral model.

## First transition

Object ID 36, `objInitRm`, starts with `roomDestSet = 0`. Its End Step event
(code ID 867) selects room ID 20, `rmMenu`, in this build: `rRand` is explicitly
set to zero, so the first branch always wins. With splash enabled, the object
advances a timed splash state before calling `room_goto(20)`; without it, the
transition happens immediately.

This gives the first bounded host milestone:

```text
load authoritative entry room
→ create persistent controller and initialize globals
→ create init-room transition object
→ reproduce splash/no-splash timing
→ transition deterministically to rmMenu (resource ID 20)
```

The first implementation should model event ordering and state changes without
trying to render the complete menu. That keeps the slice small while exercising
the parts of GameMaker semantics needed by the rest of the port: room ordering,
instance creation, Create/End Step/Alarm events, persistence, globals, arrays,
script calls and room transitions.

## Host status

`src/core/StartupRuntime` now implements this bounded state model independently
of SDL. Tests pin the recovered initialization state, immediate non-desktop
transition, input-driven splash skipping, alarm delays, and the automatic
desktop transition at tick 731. The current SDL view is deliberately diagnostic
and contains no extracted art.
