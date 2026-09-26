# Upgrade source reference

This is an index of original resource IDs and source entry points, not an
implementation-status table. All unfinished work and its priority belong in
[the work queue](open-work.md); completed implementation and verification belong
in the git log.

## Evidence

IDs, offer caps and flag mappings come from UTMT 0.9.2.0 exports of `scrUpgrades`,
`scrIncrementPug` and `scrGainUg`. Input release SHA-256:
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`.
Recovered files live in `extracted/utmt-0.9.2.0/{Assembly,CodeEntries}` and must
not be committed. VM assembly outranks decompiled GML.

## Ordinary offer IDs

| ID | Upgrade | Source entry points |
|---|---|---|
| 0 | Blast Module | `scrEnemyStomped`, `bulletBlast` events |
| 1 | Reverse Engineering | `GunModule_Step_0` |
| 3 | Rest in Pieces | `objDeadBody` events |
| 4 | Gem Powered | `scrGainGem`, `scrRechargeGem` |
| 5 | Gunpowder Blocks | `objBox_n_Step_0` |
| 6 | Youth | `scrGainUg`, Result Create |
| 7 | Candle | `scrPDamage` |
| 8 | Hot Casing | `bulletCasing` events |
| 9 | Drone | `scrPlayerInit`, `objDrone` events |
| 10 | Gem Attractor | `objCurrency` Create/Step |
| 11 | Knife and Fork | `objDeadBody`, `uniDead` collision events |
| 12 | Laser Sight | [Laser Sight source contract](laser-sight.md) |
| 13 | Apple | `scrGainUg`, `gainHp` |
| 14 | Member's Card | Builder setup, `Shop_Create_0` |
| 15 | Heart Balloon | `scrPlayerInit`, `decoyBalloon` events |
| 16 | Popping Gems | `scrGainGem` |
| 17 | Timeout | `scrTypicalDamage` |
| 18 | Rocket Jump | `scrPlayerInit`, `scrJump` |
| 20 | Gem Sick | `scrGainGem`, `nonGemGainGem`, `scrDrawHud4x3Top` |
| 22 | Safety Jetpack | `scrUpButtonFunctions`, `scrPlMovement`, `playerDraw` |

Keep these IDs in the original offer pool even while their implementation is
incomplete. Do not hide unsupported upgrades to make a playthrough appear complete.

## Disabled ordinary offers

| ID | Name in this release | Source rule |
|---|---|---|
| 2 | Dumbbell | Cap zero; no ordinary offer. Internal flag `pugGravsuit` is not the displayed name. |
| 19 | Super Charge | Cap zero; direct grant preserves refund branch, then capacity +4/refill. |
| 21 | Sudden Fortune | Cap zero; direct grant preserves refund branch, then currency/run gems +400 without pickup streak effects. |

## Verification contract

An accepted grant, an icon or a helper test does not prove ongoing gameplay
behavior. Validate inventory and offer rules separately from live effects.
Each effect needs deterministic encounter coverage, original-runner evidence
and native execution/presentation checks. Port-to-port agreement alone does
not establish fidelity, and instrumentation must distinguish ordinary frame
cost from profiling overhead.
