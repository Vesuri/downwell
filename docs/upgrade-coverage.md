# Upgrade coverage and implementation dependencies

Initial inventory audit: `218b7f6`; implementation status below is maintained
as behavior is wired. This is a feature inventory, not a percentage of the
whole game. Both ports must match the original Windows release; agreement
between ports alone is insufficient. Update this table when adding behavior.

## Evidence and interpretation

Resource IDs, enabled caps and flag mappings were checked against local UTMT
0.9.2.0 exports of `scrUpgrades`, `scrIncrementPug` and `scrGainUg`. Arithmetic
for recently wired upgrades was checked against their VM assembly, as recorded
in [feature-parity.md](feature-parity.md). Input release SHA-256:
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`.
Recovered files are under `extracted/utmt-0.9.2.0/{Assembly,CodeEntries}`;
do not commit those payloads. Script/event names below identify audit entry
points, not proof that every branch of the event is implemented.

All 23 IDs have shared inventory/cap handling and stable IDs. Twenty are
eligible for ordinary offers. Of those twenty, eight have some implemented
gameplay effect and twelve have no wired gameplay effect. None is marked
fully original-validated here. A displayed icon, accepted grant, test of a
helper or matching default native replay does not prove upgraded live behavior.

## Enabled upgrades

| ID | Upgrade | Shared gameplay currently wired | Missing behavior / source entry points |
|---|---|---|---|
| 0 | Blast Module | None | Stomp blast actor and effects; `scrEnemyStomped`, `bulletBlast` events. |
| 1 | Reverse Engineering | None | Gun-module hit/reshuffle lifecycle; `GunModule_Step_0`. |
| 3 | Rest in Pieces | None | Corpse production, projectile contact and explosion; `objDeadBody` events. |
| 4 | Gem Powered | Pickup charge, rounding and cap | Recharge sprite 111, sound 27, shake and exact collision phase; `scrGainGem`, `scrRechargeGem`. |
| 5 | Gunpowder Blocks | None | Block-destruction projectiles and effects; `objBox_n_Step_0`. |
| 6 | Youth | Immediate healing/reserve and additional offer | Full grant presentation and native upgraded-room validation; `scrGainUg`, Result Create. |
| 7 | Candle | 240-update contact immunity | Other damage sources and original damage pacing/FX; `scrPDamage`. |
| 8 | Hot Casing | None | Casing creation, lifetime, movement and enemy damage; `bulletCasing` events. |
| 9 | Drone | None | Companion lifecycle, movement and firing; `scrPlayerInit`, `objDrone` events. |
| 10 | Gem Attractor | Birth-time radius/delay | General currency time-stop, water, embedded-wall relocation and collision ordering; `objCurrency` Create/Step. |
| 11 | Knife and Fork | None | Edible corpse lifecycle, consumption and healing; `objDeadBody`, `uniDead` collision events. |
| 12 | Laser Sight | Centred muzzle on player creation; new machine-gun shots retain zero spread and one RNG draw | Visible ray, aim angles, other weapon emission patterns and ranged-projectile lifetime; see `laser-sight.md`. |
| 13 | Apple | Immediate healing and overflow reserve | Full grant presentation and native acquisition validation; `scrGainUg`, `gainHp`. |
| 14 | Member's Card | Area-one generation setup and new-shop discount | Other areas/styles/difficulty routing and native discounted-shop coverage; builder setup, `Shop_Create_0`. |
| 15 | Heart Balloon | None | Spawn/follow/collision/lifetime; `scrPlayerInit`, `decoyBalloon` events. |
| 16 | Popping Gems | None | Upward projectile, RNG, effects and sound on pickup; `scrGainGem`. |
| 17 | Timeout | None | Damage-triggered time field and consistent affected-object pause semantics; `scrTypicalDamage`. |
| 18 | Rocket Jump | None | Jump impulse configuration, blast and effects; `scrPlayerInit`, `scrJump`. |
| 20 | Gem Sick | Pickup and combo timer extension | Special HUD animation and full gem-high presentation; `scrGainGem`, `nonGemGainGem`, `scrDrawHud4x3Top`. |
| 22 | Safety Jetpack | None | Empty-charge input, fuel/reset, thrust and presentation; `scrUpButtonFunctions`, `scrPlMovement`, `playerDraw`. |

“None” means only inventory/selection/art support exists, not that the ID
should be removed from the original offer pool. Do not hide unsupported
upgrades to make a playthrough appear complete.

## Disabled ordinary offers

| ID | Name in this release | Current handling |
|---|---|---|
| 2 | Dumbbell | Cap zero; no normal offer. Its internal flag is named `pugGravsuit`; do not mistake that for the displayed name. |
| 19 | Super Charge | Cap zero; direct grant preserves refund branch then capacity +4/refill. |
| 21 | Sudden Fortune | Cap zero; direct grant preserves refund branch then currency/run gems +400 without pickup streak effects. |

## What the tests establish

- `UpgradeInventoryTest`: caps, order, healing/reserve, direct disabled grants
  and exhausted offer fallback. It does not exercise ongoing effects.
- `LevelClearControllerTest`: Youth option count combined with style rules.
- `EnemyContactTest` and paired Candle combat: damage duration and expiry.
- `GemHomingTest`, reward fixtures in `AmigaPortRuntimeTest` and paired Attractor combat: birth
  configuration/acquisition. The native gem replay also includes Attractor.
- `ComboStateTest`, `GemHomingTest` and paired Gem Sick combat: shared reward
  timing. Paired Gem Powered combat compares charge after pickup effects;
  the shared pickup fixture pins ownership, rounding and capacity limits.
- Paired shop/next-room fixture: Member's Card pricing snapshot and carry-over.

Except for the explicit native Attractor fixture, recent upgraded combat
fixtures run both adapters on the host. Bounded native default replays are
regression checks, not upgraded gameplay coverage. Original visual validation
and A500 20 ms budgets remain separate open gates for every row.

## Dependency-led implementation order

1. Replace the fixed usual-gun impact path with shared source projectile
   state and lifecycle: damage, direction, speed, range, penetration, sprite,
   creation RNG and collision order. Preserve existing default-gun tests.
   This is required by weapons, Popping Gems, Gunpowder Blocks, Drone,
   Hot Casing and Laser Sight; duplicating each in backend code is not acceptable.
2. Establish shared effect/actor creation and destruction ordering, with
   offline native assets and a deterministic event trace. Add blast actors
   and deferred enemy death/corpses for Blast Module, Rocket Jump, Rest in
   Pieces and Knife and Fork.
3. Add source gun-module behavior and weapon changes, then Reverse Engineering.
4. Integrate companion and time-field lifecycles. Existing isolated pause
   parameters are not a complete Timeout implementation.
5. Add the remaining player mechanics (including Safety Jetpack) and complete
   pickup/grant/HUD audiovisual effects. Retain a source-derived test for each.
6. For each completed slice: host shared tests, paired live upgraded tape,
   bounded native execution/presentation evidence and measured frame cost.

These dependencies do not replace other open goal requirements: later areas,
additional enemies, all styles, persistence, audio, full startup/presentation
fidelity and A500 performance still require their own coverage inventories.
