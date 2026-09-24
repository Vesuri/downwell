# Shop implementation notes

Source: local UTMT 0.9.2.0 `Assembly/gml_Object_Shop_Create_0.asm` stock/price
branches and `Assembly/gml_Object_Shop_Step_0.asm` blocks 74–80. Consumable base
prices are in `scrUpgrades` (IDs 100–105). Decompiled views were cross-checked
against VM order for randomness, price calculation and payment.

`ShopInventory` preserves these rules:

- Slot 1 rolls 100–101 even when health <=2 subsequently forces item 100.
- Slot 2 rerolls 100–103 until distinct from slot 1.
- Slot 3 rolls 102–105 once, discards that result, then rerolls until distinct
  from the first two. This otherwise redundant roll advances the game RNG.
- Price adds 200 per area after area one, doubles in hard mode, applies style-4
  discount, then membership discount to the already-discounted price. Both use
  ties-to-even `round(price/10)`; combining the percentages changes prices.
- The one-based item cursor checks stock before funds, decrements stock, calls
  the upgrade grant synchronously, then subtracts price. The grant can add
  currency. Spending alone does not change total collected gems or streak state.

The injected grant must implement `scrGainUg`, not immediately heal or refill
for IDs 100–105: those counts are consumed later by `scrControlBeginstepCheck`,
one unit per item type per update in increasing ID order. Curry increases maximum
health before `gainHp(1)`. At ownership cap, `scrGainUg` refunds 100 currency.
`Consumables` now implements that deferred grant/consumption path in `RunSession`.
Both runtimes call it at the controller boundary before player movement. Six
independent ID-ordered checks consume at most one of each item per update; they
are outside the source's pause/time-stop guard. Batteries refill current charge
after increasing capacity. Energy Drink heals before increasing charge. Curry
raises maximum health, then heals only one point (not to full health). Existing
`HealthReserve` preserves overflow healing and heart-piece conversion.

The queue resets with the run. It exposes ordered notification requests and
healing/max-health message requests without drawing or playing sounds; those
presentation consumers remain open. It is not a live shop menu: shop entry,
purchase input, case destruction and merchant feedback still need integration.

The range callback must use the shared Windows-runner RNG. This module covers
only the stock-selection segment, not random effects from other objects created
earlier/later during Shop Create. A full shop replay must preserve those too.

`make test-shop-inventory` verifies fixed random-call sequences, duplicate
rejection, the low-health override, all six prices over areas 1–5 and discount
combinations, callback/payment ordering, sold-out precedence, exact-price and
failed purchases, unchanged collection totals, and fresh stock on recreation.
The focused tests pass. Instantiating both paths with the native 68000 compiler
succeeds; undefined arithmetic helpers are only 32-bit unsigned divide/modulo.
No emulator run is claimed for this unconnected module, and neither playable
executable is changed by this initial stock/transaction implementation.

`make test-consumables` checks all six effects together in source order, stacked
items over successive updates, notifications, injured Curry behavior, and a
capped grant through the actual purchase transaction. The full host suite also
exercises queued batteries/Curry through both live runtime controllers, comparing
counts and resulting health/charge while requiring native HUD invalidation.
The native cache verifier includes an isolated three-update consumable fixture
on the 68000; it does not change the main gameplay tape or pretend to visit a shop.
Full host tests and that bounded direct-entry A500 verifier pass: three consumable
updates, 101 fresh terrain compositions and the task-stack guard. Native 68000
instruction/helper audits are clean. Notification art/audio are still requests,
not implemented presentation, and no end-to-end shop purchase is claimed.

## Interaction controller

`ShopInteraction` implements the interaction portion of Shop Step, including the
state-zero selector and its independent transition into the item-selection branch.
The caller supplies collision/interactable predicates, input **edges**, the shared
`choose(1,2,3)` RNG operation, upgrade grant and ordered feedback sink. Input is
mutable: opening/state-zero confirmation consumes confirm; item confirmation
suppresses cancel but leaves confirm set, as in the VM. Right then left are
independent branches, including a second navigation check when state zero enters
state one in the same update. Cursor zero is Leave; slots one through three use
the shared purchase transaction. Reentry does not regenerate sold stock.

Feedback signals preserve greeting/sold-out random draws, navigation, purchase,
poor response, goodbye and mask invalidation. Adapters must translate these into
merchant/case/text/audio changes; these are not implemented NPC animations.
The death/criminal tail disables interaction but does not itself clear noControl;
only the source exit block clears that flag. Do not introduce an early death
return: original interaction runs before that tail when not criminal. The normal
controller's interactable predicate still determines whether opening is allowed.
Draw-time cursor animation/decay, radio/view responses and world instance dispatch
remain separate integration work.

`make test-shop-interaction` covers eligibility, opening/confirmation consumption,
queued purchase with simultaneous cancel, sold-out RNG, poor response without
RNG, direction order/wrap, thanked exit, reentry, cancel, state-zero fall-through
and death/criminal ordering. It passes alongside inventory and consumable tests.
The callable template compiles for 68000 without floating-point/64-bit helpers.
No emulator or playable-shop coverage is claimed for this unconnected controller.

## World-hook audit and player input prerequisite

The builder's `$` marker emits object 277 (Shop). Its resource sprite 480 uses
an axis-aligned mask at x-10..x-1, y-1..y+8, not the whole 24x24 image. Shop Create
adds the table at x+64, three cases at tableX-19/0/+19 and y-9, an invisible wall
at x,y+8, keeper at x+18,y, and item/text children. Registering only a counter
and menu would omit collision and presentation objects. Child/inherited event
dispatch and RNG order still need integration review.

`scrLevelPat1` suppresses shops in ordinary area 1/level 1 but NOT style 4. The
current generator only implements the ordinary first-level setup; its default
must stay unchanged. Later levels or the special style need their source setup,
not an arbitrary shop inserted into normal 1-1.

`objPlayer_n` Begin Step computes interactable from a parentInteractable overlap,
zero horizontal speed, grounded state and the instance's interactOk. The existing
shared player lacked that up-button gate. It now accepts the retained interactable
state and suppresses the grounded press/jump branch only, as in
`scrUpButtonFunctions`; airborne held fire is a separate branch. World adapters
now supply this state for explicitly registered shops (see below); ordinary
generation still does not register the shop and its children.

The same audit found `controlsLocked` suppressed jumping/horizontal input but
not firing, empty-charge braking or release handling. `scrPlMovement` skips the
whole up-button handler under noControl. The shared player now does too, while
continuing gravity, alarm countdown and raw edge tracking. A release that occurs
while locked must neither shorten a jump nor be replayed after unlocking.
Focused player tests cover those cases, held-fire resumption, the interaction
press guard and the independently allowed airborne held-fire branch.
The full host suite and bounded direct-entry A500 600-update cache check pass;
68000 instruction/helper audits remain clean. That target tape is a regression
check, not a visit to a shop; focused shared-player tests exercise the new guards.

## Registered shop world integration

`RunSession::shops` owns up to eight registered counters, inventories and menus.
Registration reports exhaustion with a null pointer before consuming randomness.
Run reset clears instances, input history, interaction eligibility and the shop
control lock. End Step retires counters outside the source's strict
viewY-320/viewY+3000 bounds, without inventing an unlock on destruction.

Both runtime adapters expose `registerShop` for the currently supported default
style/normal-mode setup and use their existing shared RNG stream. Player movement
sees retained eligibility; the post-movement mask query refreshes it before menu
input. Raw input edges continue advancing while movement is locked. Opening faces
the counter, consumes confirmation and suppresses jumping; purchases queue the
existing controller-phase consumables. Directional edges navigate without moving
the player. The shared world supports criminal/mask invalidation, but the adapters
still supply criminal=false because that gameplay state is not implemented.

The host paired-runtime fixture explicitly registers a counter at the entrance
floor, compares randomized stock, opens it, checks held input and insufficient
funds, then uses a test-only free battery to verify deferred grant timing. Leave
and reentry retain sold stock. This is separate from the ordinary generation and
combat tape. `test-shop-world` also covers inclusive integer mask edges, movement
eligibility, capacity exhaustion, retirement and run reset.

Remaining integration: production creation and children, source-wide ordinary
Step dispatch (including multiple shops), fractional mask queries, cancellation
input mapping, merchant/case/audio feedback and rendering. Current runtime hooks
run the menu immediately after player movement and retain presentation requests;
they are not a claim that the full original Shop event has been implemented.

Validation: the full host suite (including both runtime fixtures) and normal SDL
build pass. The bounded SKIP_TO_GAME A500 600-update cache verifier passes with
clean 68000 instruction/helper audits. That emulator tape contains no shop;
registered-shop lifecycle evidence is currently from the host paired fixture.

`ShopFeedback` now consumes the ordered interaction effects in shared world
state. Buying exhausted stock clears that case's stored-item flag and removes
its item child, not the glass case. Keeper thankful/confused/goodbye mutations
and image-reset requests follow Shop Step; opening clears only thankful/goodbye.
Per-update rising-text identifiers and ordered sound requests retain original
resource IDs, volume and merchant/UI channels. Purchases request merchant 104
at volume 90 before UI 105 at volume 50. Held input produces no repeated requests.
The bounded sound queue reports overflow explicitly.

These requests are not audio playback, localized text rendering, or the keeper's
own alarm/animation controller. Focused tests cover all reply IDs, sound ordering,
stock disappearance, transient request reset and opening flag preservation. The
paired runtime purchase fixture compares both targets' child and feedback state.

## Static child geometry and player collision

Registered shops now contribute their original child rectangles to both player
solid queries and nearest-landing queries. These are resource masks, not artwork
extents: table 278/sprite 481 is x+34..x+93,y..y+7; the three cases 279/sprite 482
are centered at x+45/64/83,y-9 with offsets -9..8 on each axis; invisible wall
98/sprite 46 is x-8..x+7,y..y+15. Purchased cases stay solid. The counter itself
does not become a wall.

The invisible wall's End Step calls `scrOutofview` (strict viewY-160 and
viewY+3000), independently of the counter's -320 threshold. The table and cases
have no retirement Step in their parent chain and survive until room teardown.
Consequently retiring a counter does not recycle its occupied shared slot. The
eight-slot bound remains explicit; future room-transition integration must clear
room-owned shops rather than silently discard surviving child objects.

Tests check inclusive cropped mask bounds and gaps, nearest tops, shared-player
landings on wall/case, sold-case solidity, and separate lifetimes. The paired
runtime fixture approaches from outside the invisible wall and walks against it
after leaving the menu. Both retain the same fractional position while remaining
in the last unblocked integer query cell. This validates the current shared
integer-cell player adapter, not unresolved Windows fractional-query semantics.

Enemy/projectile queries, case damage/theft, dynamic child state, presentation
and production generation remain open. The new rectangles are player collision
hooks, not a complete implementation of those child objects. Empty shop worlds
return immediately from geometry queries, avoiding an eight-slot scan in normal
first-level gameplay. Native callback wrappers rebind each update so host-side
runtime copies cannot retain a pointer to the previous wrapper owner.

Validation: full host tests and SDL build pass, including the paired counter-wall
fixture. The bounded SKIP_TO_GAME 600-update A500 cache regression passes with
clean 68000 instruction/helper audits. No target-side visit to a shop is claimed;
the ordinary first-level emulator tape checks existing gameplay for regressions.

## Static fixture drawing

Both backends now consume `drawShopFixtures`: counter 480, table 481 and three
intact cases 482, frame zero, at recovered Shop Create positions. The counter
disappears on its retirement; surviving table/cases remain drawable. Purchases
do not change the case image. The helper emits creation order; final overlap
ordering with merchant/items and original equal-depth instances still needs
Windows presentation validation.

The game planar pack includes those three frames, converted offline by the
existing generator. SDL draws the shared stream before the player; the Amiga
terrain compositor draws it into cached static rows. Registration/counter
retirement advances a fixture revision and marks only intersecting art rows
dirty in the terrain cache, to be refreshed on subsequent composition. A purchase
does not invalidate furniture. This does not yet implement live generation or
an immediate visible-region refresh for dynamically registered on-screen shops.

Merchant animation, item-child drawing and active menu/text remain unfinished.
Normal 1-1 still contains no shop. Focused tests verify the static command stream,
resource IDs, coordinates and revision behavior; existing planar renderer tests
exercise the generated pack. No complete shop screenshot is claimed yet.

The generated game pack is 294020 bytes, SHA-256
`cf990f9f640477e3b14b6fb43d434634a1f3882b5cce1ec2ba092b95d3d912b8`;
copyrighted payload remains ignored. Full host tests, SDL build, offline pack
verification and the bounded direct-entry A500 600-update cache regression pass.
The resident Chip-art budget assertion and 68000 helper/instruction audits pass.
This target tape still exercises ordinary gameplay, not an on-screen shop.
